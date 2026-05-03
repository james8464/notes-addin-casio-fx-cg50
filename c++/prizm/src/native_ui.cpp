#include "native_ui.hpp"

#include "device/fixed_string.hpp"

#include <fxcg/display.h>
#include <fxcg/keyboard.h>

namespace casio::prizm
{
namespace
{

constexpr int kContentRows = 5;
constexpr int kFKeyTop = 192;
constexpr int kFKeyBottom = 215;
constexpr int kFKeyWidth = 64;

void fill_rect_vram(int x, int y, int w, int h, color_t color)
{
    if(w <= 0 || h <= 0) return;
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > LCD_WIDTH_PX) w = LCD_WIDTH_PX - x;
    if(y + h > LCD_HEIGHT_PX) h = LCD_HEIGHT_PX - y;
    if(w <= 0 || h <= 0) return;

    volatile color_t *vram = reinterpret_cast<volatile color_t *>(GetVRAMAddress());
    for(int yy = y; yy < y + h; yy++) {
        for(int xx = x; xx < x + w; xx++) {
            vram[yy * LCD_WIDTH_PX + xx] = color;
        }
    }
}

void stroke_rect_vram(int x, int y, int w, int h, color_t color)
{
    fill_rect_vram(x, y, w, 1, color);
    fill_rect_vram(x, y + h - 1, w, 1, color);
    fill_rect_vram(x, y, 1, h, color);
    fill_rect_vram(x + w - 1, y, 1, h, color);
}

void print_at(int col, int row, const char *text, int mode = TEXT_MODE_NORMAL, int color = TEXT_COLOR_BLACK)
{
    casio::device::FixedString<80> out;
    out.append("  ");
    out.append(text ? text : "");
    while(out.size() < 23) out.append_char(' ');
    PrintXY(col, row, out.c_str(), mode, color);
}

void print_line(int row, const char *text, int mode = TEXT_MODE_NORMAL, int color = TEXT_COLOR_BLACK)
{
    print_at(1, row, text, mode, color);
}

int glyph_index(char c)
{
    if(c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
    if(c >= 'A' && c <= 'Z') return c - 'A';
    if(c >= '0' && c <= '9') return 26 + (c - '0');
    if(c == '?') return 36;
    if(c == '<') return 37;
    if(c == '>') return 38;
    return 39;
}

const unsigned char *glyph_rows(char c)
{
    static const unsigned char kGlyphs[40][7] = {
        {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
        {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
        {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
        {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
        {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
        {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
        {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
        {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
        {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}, // I
        {0x01,0x01,0x01,0x01,0x11,0x11,0x0E}, // J
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
        {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
        {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
        {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
        {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
        {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
        {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
        {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
        {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
        {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
        {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
        {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
        {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}, // W
        {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
        {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
        {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
        {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
        {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
        {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
        {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}, // 3
        {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
        {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}, // 5
        {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}, // 6
        {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
        {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
        {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}, // 9
        {0x0E,0x11,0x01,0x02,0x04,0x00,0x04}, // ?
        {0x02,0x04,0x08,0x10,0x08,0x04,0x02}, // <
        {0x08,0x04,0x02,0x01,0x02,0x04,0x08}, // >
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // blank
    };
    return kGlyphs[glyph_index(c)];
}

int softkey_text_width(const char *text)
{
    if(text == nullptr || text[0] == '\0') return 0;
    int count = 0;
    while(text[count] != '\0' && count < 5) count++;
    return count * 12 - 2;
}

void draw_softkey_text(int x, int y, const char *text)
{
    if(text == nullptr) return;
    constexpr int scale = 2;
    int cursor = x;
    for(int i = 0; text[i] != '\0' && i < 5; i++) {
        const unsigned char *rows = glyph_rows(text[i]);
        for(int row = 0; row < 7; row++) {
            for(int col = 0; col < 5; col++) {
                if((rows[row] & (1 << (4 - col))) != 0) {
                    fill_rect_vram(cursor + col * scale, y + row * scale, scale, scale, COLOR_WHITE);
                }
            }
        }
        cursor += 6 * scale;
    }
}

int cstr_len_local(const char *text)
{
    int n = 0;
    while(text != nullptr && text[n] != '\0') n++;
    return n;
}

void print_mini_px(int x, int y, const char *text, int mode = TEXT_MODE_NORMAL,
                   int color = TEXT_COLOR_BLACK, int back = TEXT_COLOR_WHITE)
{
    int px = x;
    int py = y;
    PrintMini(&px, &py, text ? text : "", mode, LCD_WIDTH_PX - 1,
              0, 0, color, back, 1, 0);
}

int bracket_color_for_depth(int depth)
{
    static const int colors[] = {
        TEXT_COLOR_BLUE,
        TEXT_COLOR_RED,
        TEXT_COLOR_GREEN,
        TEXT_COLOR_PURPLE,
        TEXT_COLOR_CYAN,
    };
    if(depth < 0) depth = 0;
    return colors[depth % (int)(sizeof(colors) / sizeof(colors[0]))];
}

bool is_open_bracket(unsigned char c)
{
    return c == '(' || c == '[' || c == '{' ||
           c == (unsigned char)KEY_CHAR_LPAR ||
           c == (unsigned char)KEY_CHAR_LBRCKT ||
           c == (unsigned char)KEY_CHAR_LBRACE;
}

bool is_close_bracket(unsigned char c)
{
    return c == ')' || c == ']' || c == '}' ||
           c == (unsigned char)KEY_CHAR_RPAR ||
           c == (unsigned char)KEY_CHAR_RBRCKT ||
           c == (unsigned char)KEY_CHAR_RBRACE;
}

char bracket_ascii(unsigned char c)
{
    if(c == '[' || c == (unsigned char)KEY_CHAR_LBRCKT) return '[';
    if(c == ']' || c == (unsigned char)KEY_CHAR_RBRCKT) return ']';
    if(c == '{' || c == (unsigned char)KEY_CHAR_LBRACE) return '{';
    if(c == '}' || c == (unsigned char)KEY_CHAR_RBRACE) return '}';
    if(is_open_bracket(c)) return '(';
    return ')';
}

void draw_editor_brackets(unsigned char *input, int input_start)
{
    if(input == nullptr) return;
    int depth = 0;
    for(int i = 0; input[i] != '\0' && i < input_start; i++) {
        if(is_open_bracket(input[i])) depth++;
        else if(is_close_bracket(input[i]) && depth > 0) depth--;
    }

    for(int i = input_start, col = 2; input[i] != '\0' && col <= 21; i++, col++) {
        unsigned char c = input[i];
        if(is_open_bracket(c)) {
            char text[2] = {bracket_ascii(c), 0};
            PrintXY(col, 6, text, TEXT_MODE_TRANSPARENT_BACKGROUND, bracket_color_for_depth(depth));
            depth++;
        }
        else if(is_close_bracket(c)) {
            if(depth > 0) depth--;
            char text[2] = {bracket_ascii(c), 0};
            PrintXY(col, 6, text, TEXT_MODE_TRANSPARENT_BACKGROUND, bracket_color_for_depth(depth));
        }
    }
}

}

void init_native_screen(const char *title, const char *mode)
{
    Bdisp_AllClr_VRAM();
    EnableStatusArea(1);
    char c1 = 0;
    char c2 = 0;
    DefineStatusAreaFlags(
        DSA_SETDEFAULT,
        SAF_BATTERY | SAF_ALPHA_SHIFT | SAF_TEXT,
        &c1,
        &c2);
    static char status_msg[40];
    casio::device::FixedString<40> status;
    status.append(title ? title : "CasioCAS");
    if(mode != nullptr && mode[0] != '\0') {
        status.append(" ");
        status.append(mode);
    }
    casio::device::copy_cstr(status_msg, (int)sizeof(status_msg), status.c_str());
    DefineStatusMessage(status_msg, 0, 0, 0);
    DisplayStatusArea();
    DrawHeaderLine();
}

void draw_input_box(int x, int y, int w, int h)
{
    fill_rect_vram(x, y, w, h, COLOR_WHITE);
    stroke_rect_vram(x, y, w, h, COLOR_BLACK);
}

void draw_status_line(const char *text)
{
    print_line(6, text);
}

void draw_softkeys(const char *k1, const char *k2, const char *k3,
                   const char *k4, const char *k5, const char *k6)
{
    const char *labels[6] = {k1, k2, k3, k4, k5, k6};
    fill_rect_vram(0, kFKeyTop, LCD_WIDTH_PX, kFKeyBottom - kFKeyTop + 1, COLOR_BLACK);
    fill_rect_vram(0, kFKeyTop, LCD_WIDTH_PX, 1, COLOR_WHITE);
    for(int i = 1; i < 6; i++) {
        fill_rect_vram(i * kFKeyWidth - 1, kFKeyTop + 2, 2, kFKeyBottom - kFKeyTop - 3, COLOR_WHITE);
    }
    for(int i = 0; i < 6; i++) {
        const char *text = labels[i] ? labels[i] : "";
        int width = softkey_text_width(text);
        int x = i * kFKeyWidth + (kFKeyWidth - width) / 2;
        if(x < i * kFKeyWidth + 2) x = i * kFKeyWidth + 2;
        draw_softkey_text(x, kFKeyTop + 5, text);
    }
}

void draw_fkey_popup(const char *const *items, int count, int selected, int fkey_index)
{
    if(items == nullptr || count <= 0) return;
    if(selected < 0) selected = 0;
    if(selected >= count) selected = count - 1;
    if(fkey_index < 0) fkey_index = 0;
    if(fkey_index > 5) fkey_index = 5;

    int longest = 0;
    for(int i = 0; i < count; i++) {
        int len = cstr_len_local(items[i]) + 3;
        if(len > longest) longest = len;
    }
    if(longest < 8) longest = 8;
    if(longest > 18) longest = 18;

    const int row_h = 18;
    int width = longest * 8 + 10;
    int height = count * row_h + 4;
    int x = fkey_index * kFKeyWidth;
    if(x + width >= LCD_WIDTH_PX) x = LCD_WIDTH_PX - width - 1;
    if(x < 0) x = 0;
    int y = kFKeyTop - height;
    if(y < 25) y = 25;

    fill_rect_vram(x, y, width, height, COLOR_WHITE);
    stroke_rect_vram(x, y, width, height, COLOR_BLACK);

    for(int i = 0; i < count; i++) {
        int row = count - 1 - i;
        int yy = y + 2 + row * row_h;
        casio::device::FixedString<32> line;
        line.append_int(i + 1);
        line.append(": ");
        line.append(items[i] ? items[i] : "");
        print_mini_px(x + 4, yy + 2, line.c_str(), i == selected ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL);
    }
    Bdisp_PutDisp_DD();
}

void draw_home(void)
{
    init_native_screen("CasioCAS", "DEG");
    print_line(1, "F1 Derive  F2 Algebra");
    print_line(2, "F3 Trig    F4 Integr");
    print_line(4, "F5 SUVAT   F6 Shell");
    print_line(6, "EXE: menu");
    draw_softkeys("DRV", "ALG", "TRG", "INT", "SUV", "SHL");
    Bdisp_PutDisp_DD();
}

void draw_menu(const char *title, const char *const *items, int count, int selected, int top)
{
    init_native_screen(title, nullptr);
    for(int row = 0; row < kContentRows; row++) {
        int index = top + row;
        if(index >= count) break;
        casio::device::FixedString<64> line;
        line.append_int(index + 1);
        line.append(". ");
        line.append(items[index]);
        print_line(row + 2, line.c_str(), index == selected ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL);
    }
    if(count == 0) print_line(3, "No entries");
    
    draw_softkeys("SEL", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

void draw_shell(const char *title, const char *mode, const char *const *lines, int count, int top, int selected,
                unsigned char *input, int input_start, int input_cursor,
                const char *k1, const char *k2, const char *k3,
                const char *k4, const char *k5, const char *k6)
{
    init_native_screen(title, mode);
    for(int row = 0; row < kShellVisibleRows; row++) {
        int index = top + row;
        if(index >= count) break;
        print_line(row + 1, lines[index], index == selected ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL);
    }
    print_line(6, ">");
    DisplayMBString(input, input_start, input_cursor, 2, 6);
    draw_editor_brackets(input, input_start);
    draw_softkeys(k1, k2, k3, k4, k5, k6);
    Bdisp_PutDisp_DD();
}

void draw_lines(const char *title, casio::device::OutputLines const &lines, int top, bool more_above, bool more_below)
{
    init_native_screen(title, nullptr);
    for(int row = 0; row < kContentRows; row++) {
        int index = top + row;
        if(index >= lines.count()) break;
        print_line(row + 2, lines.line(index));
    }
    
    draw_softkeys("BACK", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

}

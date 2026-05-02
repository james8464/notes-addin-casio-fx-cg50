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

void print_at(int col, int row, const char *text, int mode = TEXT_MODE_NORMAL, int color = TEXT_COLOR_BLACK)
{
    casio::device::FixedString<80> out;
    out.append("  ");
    out.append(text ? text : "");
    PrintXY(col, row, out.c_str(), mode, color);
}

void print_line(int row, const char *text, int mode = TEXT_MODE_NORMAL, int color = TEXT_COLOR_BLACK)
{
    print_at(1, row, text, mode, color);
}

void print_mini_at(int x, int y, const char *text, int color, int back_color)
{
    int px = x;
    int py = y;
    PrintMini(&px, &py, text ? text : "", TEXT_MODE_NORMAL, LCD_WIDTH_PX - 1,
              0, 0, color, back_color, 1, 0);
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
        int x = i * kFKeyWidth + 14;
        if(text[0] != '\0' && text[1] != '\0' && text[2] != '\0' && text[3] != '\0') x = i * kFKeyWidth + 10;
        print_mini_at(x, kFKeyTop + 6, text, TEXT_COLOR_WHITE, TEXT_COLOR_BLACK);
    }
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
    
    bool has_more = (top + kContentRows) < count;
    casio::device::FixedString<32> nav;
    if(top > 0) nav.append("↑ ");
    if(has_more) nav.append("↓");
    if(top > 0 || has_more) draw_status_line(nav.c_str());
    
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
    
    casio::device::FixedString<16> nav;
    if(more_above) nav.append("↑");
    if(more_above && more_below) nav.append(" ");
    if(more_below) nav.append("↓");
    if(more_above || more_below) draw_status_line(nav.c_str());
    
    draw_softkeys("BACK", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

}

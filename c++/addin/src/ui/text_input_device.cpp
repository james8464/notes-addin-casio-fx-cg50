#include "ui/text_input_device.hpp"

#include "device/fixed_string.hpp"
#include "ui/theme.hpp"

#include <gint/keyboard.h>

namespace casio::ui
{
namespace
{

static int text_len(const char *s)
{
    return casio::device::cstr_len(s);
}

static void draw_input(const char *title, const char *help, const char *text, int cursor)
{
    draw_frame(title, "INPUT");
    draw_section_label(4, kContentTop, "Expression");

    int len = text_len(text);
    if(cursor < 0) cursor = 0;
    if(cursor > len) cursor = len;

    int start = 0;
    int max_chars = 34;
    if(cursor > max_chars) start = cursor - max_chars;
    if(len - start > max_chars && cursor <= start) start = cursor;

    casio::device::FixedString<40> shown;
    if(start > 0) shown.append("...");
    for(int i = start; text != nullptr && text[i] != '\0' && i < start + max_chars; i++) {
        shown.append_char(text[i]);
    }

    int box_y0 = kContentTop + 22;
    int box_y1 = box_y0 + 28;
    drect(4, box_y0, DWIDTH - 12, box_y1, kPaper);
    dline(4, box_y0, DWIDTH - 12, box_y0, kInk);
    dline(4, box_y1, DWIDTH - 12, box_y1, kInk);
    dline(4, box_y0, 4, box_y1, kInk);
    dline(DWIDTH - 12, box_y0, DWIDTH - 12, box_y1, kInk);
    dtext(10, box_y0 + 8, kInk, shown.c_str());

    int cursor_x = 10 + (cursor - start + (start > 0 ? 3 : 0)) * 7;
    if(cursor_x < 10) cursor_x = 10;
    if(cursor_x > DWIDTH - 20) cursor_x = DWIDTH - 20;
    dline(cursor_x, box_y1 - 5, cursor_x + 6, box_y1 - 5, kInk);

    dtext(4, box_y1 + 12, kBlue, "Hint");
    draw_limited_text(42, box_y1 + 12, kInk, help ? help : "", 36);
    dtext(4, box_y1 + 30, kInk, "EXE ok  DEL back  AC clear");
    dtext(4, box_y1 + 46, kInk, "Arrows edit, F-keys insert vars");
    draw_softkeys("s", "u", "v", "a", "t", "?");
    dupdate();
}

static const char *key_to_text(int key)
{
    if(key == KEY_0) return "0";
    if(key == KEY_1) return "1";
    if(key == KEY_2) return "2";
    if(key == KEY_3) return "3";
    if(key == KEY_4) return "4";
    if(key == KEY_5) return "5";
    if(key == KEY_6) return "6";
    if(key == KEY_7) return "7";
    if(key == KEY_8) return "8";
    if(key == KEY_9) return "9";
    if(key == KEY_PLUS) return "+";
    if(key == KEY_MINUS || key == KEY_NEG) return "-";
    if(key == KEY_MUL) return "*";
    if(key == KEY_DIV) return "/";
    if(key == KEY_DOT) return ".";
    if(key == KEY_COMMA) return ",";
    if(key == KEY_LEFTP) return "(";
    if(key == KEY_RIGHTP) return ")";
    if(key == KEY_EQUALS) return "=";
    if(key == KEY_X || key == KEY_XOT) return "x";
    if(key == KEY_Y) return "y";
    if(key == KEY_Z) return "z";
    if(key == KEY_POWER) return "^";
    if(key == KEY_SIN) return "sin(";
    if(key == KEY_COS) return "cos(";
    if(key == KEY_TAN) return "tan(";
    if(key == KEY_LOG) return "pi";
    if(key == KEY_LN) return "ln(";
    if(key == KEY_F1) return "s";
    if(key == KEY_F2) return "u";
    if(key == KEY_F3) return "v";
    if(key == KEY_F4) return "a";
    if(key == KEY_F5) return "t";
    if(key == KEY_F6) return "?";
    return nullptr;
}

static void insert_text(char *buffer, int capacity, int &len, int &cursor, const char *text)
{
    if(text == nullptr) return;
    for(int i = 0; text[i] != '\0'; i++) {
        if(len + 1 >= capacity) break;
        for(int j = len; j >= cursor; j--) buffer[j + 1] = buffer[j];
        buffer[cursor] = text[i];
        len++;
        cursor++;
    }
}

} // namespace

bool text_input(char *buffer, int capacity, const char *title, const char *help)
{
    if(buffer == nullptr || capacity <= 0) return false;
    int len = text_len(buffer);
    if(len >= capacity) {
        len = capacity - 1;
        buffer[len] = '\0';
    }
    int cursor = len;

    draw_input(title, help, buffer, cursor);
    while(true) {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;

        if(ev.key == KEY_EXIT) return false;
        if(ev.key == KEY_EXE) return true;

        if(ev.key == KEY_LEFT) {
            if(cursor > 0) cursor--;
            draw_input(title, help, buffer, cursor);
            continue;
        }

        if(ev.key == KEY_RIGHT) {
            if(cursor < len) cursor++;
            draw_input(title, help, buffer, cursor);
            continue;
        }

        if(ev.key == KEY_DEL) {
            if(cursor > 0 && len > 0) {
                for(int i = cursor - 1; i < len; i++) buffer[i] = buffer[i + 1];
                len--;
                cursor--;
                draw_input(title, help, buffer, cursor);
            }
            continue;
        }

        if(ev.key == KEY_ACON) {
            len = 0;
            cursor = 0;
            buffer[0] = '\0';
            draw_input(title, help, buffer, cursor);
            continue;
        }

        const char *text = key_to_text(ev.key);
        if(text != nullptr) {
            insert_text(buffer, capacity, len, cursor, text);
            draw_input(title, help, buffer, cursor);
        }
    }
}

} // namespace casio::ui

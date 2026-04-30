#include "ui/text_input_device.hpp"

#include "device/fixed_string.hpp"

#include <gint/display.h>
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
    dclear(C_WHITE);
    dtext(2, 2, C_BLACK, title ? title : "");
    dline(0, 18, DWIDTH - 1, 18, C_BLACK);

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

    dtext(2, 28, C_BLACK, shown.c_str());
    int cursor_x = 2 + (cursor - start + (start > 0 ? 3 : 0)) * 7;
    if(cursor_x < 2) cursor_x = 2;
    if(cursor_x > DWIDTH - 8) cursor_x = DWIDTH - 8;
    dline(cursor_x, 44, cursor_x + 6, 44, C_BLACK);
    dline(0, 56, DWIDTH - 1, 56, C_BLACK);
    dtext(2, 62, C_BLACK, help ? help : "");
    dtext(2, 82, C_BLACK, "F1 s  F2 u  F3 v  F4 a  F5 t  F6 ?");
    dtext(2, DHEIGHT - 18, C_BLACK, "EXE ok  DEL back  LEFT/RIGHT  AC clear");
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

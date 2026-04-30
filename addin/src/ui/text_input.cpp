#include "text_input.hpp"

#include <gint/display.h>
#include <gint/keyboard.h>

namespace casio::ui
{
namespace
{

static void draw_frame(const char *title, const char *help, std::string const &text, std::size_t cursor)
{
    dclear(C_WHITE);
    dtext(2, 2, C_BLACK, title ? title : "");
    dline(0, 18, DWIDTH - 1, 18, C_BLACK);

    // Show input (simple single-line, clipped).
    std::size_t start = 0;
    std::size_t max_chars = 30;
    if(cursor > max_chars) start = cursor - max_chars;
    if(text.size() - start > max_chars) {
        // ok
    }
    std::string shown = text.substr(start, max_chars);
    if(start > 0) shown = "…" + shown.substr(1);

    dtext(2, 24, C_BLACK, shown.c_str());
    // Cursor indicator under the current character (rough monospace assumption).
    std::size_t cur_in_view = (cursor >= start) ? (cursor - start) : 0;
    int cx = 2 + int(cur_in_view) * 8;
    if(start > 0 && cur_in_view > 0) cx -= 8; // account for ellipsis
    dline(cx, 40, cx + 6, 40, C_BLACK);

    dline(0, 44, DWIDTH - 1, 44, C_BLACK);
    dtext(2, 48, C_BLACK, help ? help : "");
    dupdate();
}

static bool map_key_to_char(int key, char &out)
{
    // Minimal mapping for Boolean syntax and simple math tokens.
    // NOTE: gint key codes are many; we only map what we need now.
    // Letters A-Z
    if(key >= KEY_A && key <= KEY_Z) {
        out = char('A' + (key - KEY_A));
        return true;
    }
    // Digits
    if(key >= KEY_0 && key <= KEY_9) {
        out = char('0' + (key - KEY_0));
        return true;
    }
    // Punctuation for boolean
    if(key == KEY_PLUS) { out = '+'; return true; }
    if(key == KEY_DOT) { out = '.'; return true; }
    if(key == KEY_COMMA) { out = ','; return true; }
    if(key == KEY_LEFTP) { out = '('; return true; }
    if(key == KEY_RIGHTP) { out = ')'; return true; }
    // Operators for math (subset)
    if(key == KEY_MINUS) { out = '-'; return true; }
    if(key == KEY_MULT) { out = '*'; return true; }
    if(key == KEY_DIV) { out = '/'; return true; }

    return false;
}

} // namespace

bool text_input(std::string &io_text, const char *title, const char *help)
{
    std::size_t cursor = io_text.size();
    draw_frame(title, help, io_text, cursor);

    while(true) {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;

        if(ev.key == KEY_EXIT) return false;
        if(ev.key == KEY_EXE) return true;

        if(ev.key == KEY_LEFT) {
            if(cursor > 0) cursor--;
            draw_frame(title, help, io_text, cursor);
            continue;
        }
        if(ev.key == KEY_RIGHT) {
            if(cursor < io_text.size()) cursor++;
            draw_frame(title, help, io_text, cursor);
            continue;
        }

        if(ev.key == KEY_DEL) {
            if(cursor > 0 && !io_text.empty()) {
                io_text.erase(io_text.begin() + (cursor - 1));
                cursor--;
            }
            draw_frame(title, help, io_text, cursor);
            continue;
        }
        if(ev.key == KEY_AC) {
            io_text.clear();
            cursor = 0;
            draw_frame(title, help, io_text, cursor);
            continue;
        }

        char ch = 0;
        if(map_key_to_char(ev.key, ch)) {
            if(io_text.size() < 200) {
                io_text.insert(io_text.begin() + cursor, ch);
                cursor++;
            }
            draw_frame(title, help, io_text, cursor);
        }
    }
}

} // namespace casio::ui


#include "text_input.hpp"

#include <gint/display.h>
#include <gint/keyboard.h>

namespace casio::ui
{
namespace
{

static void draw_frame(const char *title, const char *help, std::string const &text)
{
    dclear(C_WHITE);
    dtext(2, 2, C_BLACK, title ? title : "");
    dline(0, 18, DWIDTH - 1, 18, C_BLACK);

    // Show input (simple single-line, clipped).
    std::string shown = text;
    if((int)shown.size() > 30) {
        shown = shown.substr(shown.size() - 30);
        shown = "…" + shown;
    }
    dtext(2, 24, C_BLACK, shown.c_str());
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
    draw_frame(title, help, io_text);

    while(true) {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;

        if(ev.key == KEY_EXIT) return false;
        if(ev.key == KEY_EXE) return true;

        if(ev.key == KEY_DEL) {
            if(!io_text.empty()) io_text.pop_back();
            draw_frame(title, help, io_text);
            continue;
        }
        if(ev.key == KEY_AC) {
            io_text.clear();
            draw_frame(title, help, io_text);
            continue;
        }

        char ch = 0;
        if(map_key_to_char(ev.key, ch)) {
            if(io_text.size() < 200) io_text.push_back(ch);
            draw_frame(title, help, io_text);
        }
    }
}

} // namespace casio::ui


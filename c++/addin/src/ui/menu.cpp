#include "menu.hpp"

#include <gint/display.h>
#include <gint/keyboard.h>

namespace casio::ui
{
namespace
{

static void draw_menu(const char *title, std::vector<std::string> const &items, int sel, int top)
{
    dclear(C_WHITE);
    dtext(2, 2, C_BLACK, title ? title : "");
    dline(0, 18, DWIDTH - 1, 18, C_BLACK);

    int rows = 9;
    for(int r = 0; r < rows; r++) {
        int i = top + r;
        if(i >= (int)items.size()) break;
        int y = 22 + r * 16;
        bool active = (i == sel);
        if(active) {
            drect(0, y - 1, DWIDTH - 1, y + 14, C_BLACK);
            dtext(2, y, C_WHITE, items[i].c_str());
        }
        else {
            dtext(2, y, C_BLACK, items[i].c_str());
        }
    }

    dtext(2, DHEIGHT - 18, C_BLACK, "UP/DOWN move  EXE select  EXIT back");
    dupdate();
}

} // namespace

int menu_select(const char *title, std::vector<std::string> const &items, int start_index)
{
    if(items.empty()) return -1;
    int sel = start_index;
    if(sel < 0) sel = 0;
    if(sel >= (int)items.size()) sel = (int)items.size() - 1;
    int top = 0;

    auto ensure_visible = [&]() {
        int rows = 9;
        if(sel < top) top = sel;
        if(sel >= top + rows) top = sel - rows + 1;
        if(top < 0) top = 0;
        int max_top = (int)items.size() - rows;
        if(max_top < 0) max_top = 0;
        if(top > max_top) top = max_top;
    };

    ensure_visible();
    draw_menu(title, items, sel, top);

    while(true) {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;
        if(ev.key == KEY_EXIT) return -1;
        if(ev.key == KEY_EXE) return sel;
        if(ev.key == KEY_UP) {
            if(sel > 0) sel--;
            ensure_visible();
            draw_menu(title, items, sel, top);
            continue;
        }
        if(ev.key == KEY_DOWN) {
            if(sel + 1 < (int)items.size()) sel++;
            ensure_visible();
            draw_menu(title, items, sel, top);
            continue;
        }
    }
}

} // namespace casio::ui


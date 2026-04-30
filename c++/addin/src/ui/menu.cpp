#include "menu.hpp"

#include <gint/display.h>
#include <gint/keyboard.h>

namespace casio::ui
{
namespace
{

static void draw_menu(const char *title, MenuItem const *items, int count, int sel, int top)
{
    dclear(C_WHITE);
    dtext(2, 2, C_BLACK, title ? title : "");
    dline(0, 18, DWIDTH - 1, 18, C_BLACK);

    int rows = 9;
    for(int r = 0; r < rows; r++) {
        int i = top + r;
        if(i >= count) break;
        int y = 22 + r * 16;
        bool active = (i == sel);
        const char *label = items[i].label ? items[i].label : "";
        if(active) {
            drect(0, y - 1, DWIDTH - 1, y + 14, C_BLACK);
            dtext(2, y, C_WHITE, label);
        }
        else {
            dtext(2, y, C_BLACK, label);
        }
    }

    dtext(2, DHEIGHT - 18, C_BLACK, "UP/DOWN move  EXE select  EXIT back");
    dupdate();
}

} // namespace

int menu_select(const char *title, MenuItem const *items, int count, int start_index)
{
    if(items == nullptr || count <= 0) return -1;
    int sel = start_index;
    if(sel < 0) sel = 0;
    if(sel >= count) sel = count - 1;
    int top = 0;

    auto ensure_visible = [&]() {
        int rows = 9;
        if(sel < top) top = sel;
        if(sel >= top + rows) top = sel - rows + 1;
        if(top < 0) top = 0;
        int max_top = count - rows;
        if(max_top < 0) max_top = 0;
        if(top > max_top) top = max_top;
    };

    ensure_visible();
    draw_menu(title, items, count, sel, top);

    while(true) {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;
        if(ev.key == KEY_EXIT) return -1;
        if(ev.key == KEY_EXE) return sel;
        if(ev.key == KEY_UP) {
            if(sel > 0) sel--;
            ensure_visible();
            draw_menu(title, items, count, sel, top);
            continue;
        }
        if(ev.key == KEY_DOWN) {
            if(sel + 1 < count) sel++;
            ensure_visible();
            draw_menu(title, items, count, sel, top);
            continue;
        }
    }
}

} // namespace casio::ui

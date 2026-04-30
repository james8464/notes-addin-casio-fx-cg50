#include "menu.hpp"

#include "device/fixed_string.hpp"
#include "ui/theme.hpp"

#include <gint/keyboard.h>

namespace casio::ui
{
namespace
{

static void draw_menu(const char *title, MenuItem const *items, int count, int sel, int top)
{
    draw_frame(title, "RAD");
    draw_section_label(4, kContentTop, "Function Catalog");

    int rows = visible_rows() - 1;
    for(int r = 0; r < rows; r++) {
        int i = top + r;
        if(i >= count) break;
        int y = kContentTop + 18 + r * kRowH;

        casio::device::FixedString<80> label;
        label.append_int(i + 1);
        label.append("  ");
        label.append(items[i].label ? items[i].label : "");
        draw_list_row(y, label.c_str(), i == sel);
    }

    draw_scrollbar(top, rows, count);
    draw_softkeys("SEL", "UP", "DOWN", "HELP", "", "EXIT");
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
        int rows = visible_rows() - 1;
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

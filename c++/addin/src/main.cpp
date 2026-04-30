#include <gint/display.h>
#include <gint/keyboard.h>

#include "ui/menu.hpp"

namespace
{

template <int N>
constexpr int array_count(const char *const (&)[N])
{
    return N;
}

template <int N>
constexpr int array_count(const casio::ui::MenuItem (&)[N])
{
    return N;
}

static void draw_centered(const char *title, const char *line1, const char *line2, const char *footer)
{
    dclear(C_WHITE);

    if(title != nullptr) {
        dtext(18, 18, C_BLACK, title);
    }

    if(line1 != nullptr) {
        dtext(18, 54, C_BLACK, line1);
    }

    if(line2 != nullptr) {
        dtext(18, 72, C_BLACK, line2);
    }

    if(footer != nullptr) {
        dtext(6, DHEIGHT - 18, C_BLACK, footer);
    }

    dupdate();
}

static void view_lines(const char *title, const char *const *lines, int count)
{
    int top = 0;
    while(true) {
        dclear(C_WHITE);
        dtext(2, 2, C_BLACK, title ? title : "");
        dline(0, 18, DWIDTH - 1, 18, C_BLACK);

        for(int row = 0; row < 9; row++) {
            int index = top + row;
            if(index >= count) break;
            dtext(2, 22 + row * 16, C_BLACK, lines[index] ? lines[index] : "");
        }

        dtext(2, DHEIGHT - 18, C_BLACK, "UP/DOWN scroll  EXIT back");
        dupdate();

        key_event_t event = getkey();
        if(event.type != KEYEV_DOWN) continue;
        if(event.key == KEY_EXIT || event.key == KEY_EXE) return;
        if(event.key == KEY_UP) {
            if(top > 0) top--;
            continue;
        }
        if(event.key == KEY_DOWN) {
            if(top + 9 < count) top++;
            continue;
        }
    }
}

static void show_module_status(const char *module_name)
{
    const char *lines[] = {
        "Freestanding device build",
        "keeps STL and exceptions",
        "out of the CG50 target.",
        "",
        "This menu entry is host-only",
        "for now.",
        "",
        "Use the Python TUI or",
        "casio_host for full solver",
        "behaviour while the device",
        "port is rewritten.",
    };

    view_lines(module_name, lines, array_count(lines));
}

} // namespace

int main(void)
{
    const casio::ui::MenuItem home_items[] = {
        {"CAS Shell"},
        {"SUVAT"},
        {"Derive"},
        {"Integrate"},
        {"Algebra"},
        {"Trig"},
    };

    draw_centered(
        "CasioCAS",
        "CG50 freestanding build",
        "EXE opens the menu",
        "EXE continue   EXIT quit");

    while(true) {
        key_event_t event = getkey();
        if(event.type != KEYEV_DOWN) continue;

        if(event.key == KEY_EXIT) break;
        if(event.key != KEY_EXE) continue;

        int app = casio::ui::menu_select("Home", home_items, array_count(home_items), 0);
        if(app < 0) {
            draw_centered(
                "CasioCAS",
                "CG50 freestanding build",
                "EXE opens the menu",
                "EXE continue   EXIT quit");
            continue;
        }

        switch(app) {
            case 0: show_module_status("CAS Shell"); break;
            case 1: show_module_status("SUVAT"); break;
            case 2: show_module_status("Derive"); break;
            case 3: show_module_status("Integrate"); break;
            case 4: show_module_status("Algebra"); break;
            case 5: show_module_status("Trig"); break;
            default: break;
        }

        draw_centered(
            "CasioCAS",
            "CG50 freestanding build",
            "EXE opens the menu",
            "EXE continue   EXIT quit");
    }

    return 1;
}

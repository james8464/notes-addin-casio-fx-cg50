#include <gint/display.h>
#include <gint/keyboard.h>

#include "device/device_solver.hpp"
#include "device/fixed_string.hpp"
#include "ui/menu.hpp"
#include "ui/text_input_device.hpp"

namespace
{

template <int N>
constexpr int array_count(const casio::ui::MenuItem (&)[N])
{
    return N;
}

static void draw_home()
{
    dclear(C_WHITE);
    dtext(18, 18, C_BLACK, "CasioCAS");
    dtext(18, 48, C_BLACK, "Exam working on CG50");
    dtext(18, 66, C_BLACK, "bounded device mode");
    dtext(6, DHEIGHT - 18, C_BLACK, "EXE menu   EXIT quit");
    dupdate();
}

static void view_lines(const char *title, casio::device::OutputLines const &lines)
{
    int top = 0;
    while(true) {
        dclear(C_WHITE);
        dtext(2, 2, C_BLACK, title ? title : "");
        dline(0, 18, DWIDTH - 1, 18, C_BLACK);

        for(int row = 0; row < 9; row++) {
            int index = top + row;
            if(index >= lines.count()) break;
            dtext(2, 22 + row * 16, C_BLACK, lines.line(index));
        }

        dtext(2, DHEIGHT - 18, C_BLACK, "UP/DOWN scroll  EXE/EXIT back");
        dupdate();

        key_event_t event = getkey();
        if(event.type != KEYEV_DOWN) continue;
        if(event.key == KEY_EXIT || event.key == KEY_EXE) return;
        if(event.key == KEY_UP) {
            if(top > 0) top--;
            continue;
        }
        if(event.key == KEY_DOWN) {
            if(top + 9 < lines.count()) top++;
            continue;
        }
    }
}

static void run_module(
    const char *title,
    casio::device::Module module,
    const char *initial,
    const char *help)
{
    char input[128];
    casio::device::copy_cstr(input, (int)sizeof(input), initial);
    if(!casio::ui::text_input(input, (int)sizeof(input), title, help)) return;

    casio::device::OutputLines lines;
    casio::device::solve(module, input, lines);
    view_lines(title, lines);
}

} // namespace

int main(void)
{
    const casio::ui::MenuItem home_items[] = {
        {"CAS Shell"},
        {"Simplify"},
        {"Algebra"},
        {"Derive"},
        {"Integrate"},
        {"Trig"},
        {"SUVAT"},
    };

    draw_home();
    while(true) {
        key_event_t event = getkey();
        if(event.type != KEYEV_DOWN) continue;
        if(event.key == KEY_EXIT) break;
        if(event.key != KEY_EXE) continue;

        int app = casio::ui::menu_select("Home", home_items, array_count(home_items), 0);
        if(app < 0) {
            draw_home();
            continue;
        }

        switch(app) {
            case 0:
                run_module("CAS Shell", casio::device::Module::Shell, "2x+3=7", "expr, eqn, diff(...), int(...)");
                break;
            case 1:
                run_module("Simplify", casio::device::Module::Simplify, "2x+3-x+4", "linear expression");
                break;
            case 2:
                run_module("Algebra", casio::device::Module::Algebra, "2x+3=7", "linear equation");
                break;
            case 3:
                run_module("Derive", casio::device::Module::Derive, "3x^2+2x+1", "polynomial up to x^5");
                break;
            case 4:
                run_module("Integrate", casio::device::Module::Integrate, "3x^2+2x+1", "polynomial up to x^5");
                break;
            case 5:
                run_module("Trig", casio::device::Module::Trig, "sin(30)", "exact common angles");
                break;
            case 6:
                run_module("SUVAT", casio::device::Module::Suvat, "s=10,u=0,v=?,a=2,t=5", "use ? for target");
                break;
            default:
                break;
        }

        draw_home();
    }

    return 1;
}

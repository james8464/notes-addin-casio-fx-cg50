#include "device/device_solver.hpp"
#include "device/fixed_string.hpp"
#include "native_input.hpp"
#include "native_ui.hpp"

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>

namespace
{

template <int N>
constexpr int count_of(const char *const (&)[N])
{
    return N;
}

int select_menu(const char *title, const char *const *items, int count)
{
    int selected = 0;
    int top = 0;
    casio::prizm::draw_menu(title, items, count, selected, top);

    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT) return -1;
        if(key == KEY_CTRL_EXE) return selected;
        if(key == KEY_CTRL_UP && selected > 0) selected--;
        if(key == KEY_CTRL_DOWN && selected + 1 < count) selected++;
        if(selected < top) top = selected;
        if(selected >= top + 6) top = selected - 5;
        casio::prizm::draw_menu(title, items, count, selected, top);
    }
}

void view_lines(const char *title, casio::device::OutputLines const &lines)
{
    int top = 0;
    casio::prizm::draw_lines(title, lines, top);
    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT || key == KEY_CTRL_EXE) return;
        if(key == KEY_CTRL_UP && top > 0) top--;
        if(key == KEY_CTRL_DOWN && top + 6 < lines.count()) top++;
        casio::prizm::draw_lines(title, lines, top);
    }
}

void run_module(const char *title, casio::device::Module module,
                const char *initial, const char *help)
{
    char input[128];
    casio::device::copy_cstr(input, (int)sizeof(input), initial);
    if(!casio::prizm::text_input(input, (int)sizeof(input), title, help)) return;

    casio::device::OutputLines lines;
    casio::device::solve(module, input, lines);
    view_lines(title, lines);
}

}

extern "C" int main(void)
{
    const char *items[] = {
        "CAS Shell",
        "Simplify",
        "Algebra",
        "Derive",
        "Integrate",
        "Trig",
        "SUVAT",
    };

    while(true) {
        casio::prizm::draw_home();

        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT) break;

        int app = -1;
        if(key == KEY_CTRL_F1) app = 0;
        if(key == KEY_CTRL_F2) app = 2;
        if(key == KEY_CTRL_F3) app = 3;
        if(key == KEY_CTRL_F4) app = 4;
        if(key == KEY_CTRL_F5) app = 5;
        if(key == KEY_CTRL_F6) app = 6;
        if(key == KEY_CTRL_EXE) app = select_menu("Home", items, count_of(items));
        if(app < 0) continue;

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
    }

    return 0;
}

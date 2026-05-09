#include <gint/display.h>
#include <gint/keyboard.h>

#include "device/device_solver.hpp"
#include "device/fixed_string.hpp"
#include "ui/menu.hpp"
#include "ui/theme.hpp"
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
    casio::ui::draw_frame("CasioCAS", "RAD");
    casio::ui::draw_section_label(4, casio::ui::kContentTop, "CAS Session");

    dtext(4, casio::ui::kContentTop + 20, casio::ui::kInk, "Exact working, bounded CG50 port");
    dtext(4, casio::ui::kContentTop + 38, casio::ui::kInk, "F1 Shell   F2 Algebra   F3 Derive");
    dtext(4, casio::ui::kContentTop + 54, casio::ui::kInk, "F4 Integr  F5 Trig      F6 SUVAT");

    drect(0, casio::ui::kContentTop + 78, DWIDTH - 9, casio::ui::kContentTop + 94, casio::ui::kInk);
    dtext(4, casio::ui::kContentTop + 80, casio::ui::kPaper, "EXE opens Function Catalog");
    dtext(4, casio::ui::kContentTop + 106, casio::ui::kBlue, "Status");
    dtext(50, casio::ui::kContentTop + 106, casio::ui::kInk, "No STL/exceptions in device target");
    casio::ui::draw_scrollbar(0, 1, 1);
    casio::ui::draw_softkeys("SHELL", "ALG", "DERIV", "INT", "TRIG", "SUVAT");
    dupdate();
}

static void view_lines(const char *title, casio::device::OutputLines const &lines)
{
    constexpr int kMaxTextCols = 34;

    auto chunks_for = [&](const char *text) {
        int len = casio::device::cstr_len(text);
        int chunks = (len + kMaxTextCols - 1) / kMaxTextCols;
        return chunks > 0 ? chunks : 1;
    };

    auto wrapped_count = [&]() {
        int total = 0;
        for(int i = 0; i < lines.count(); i++) total += chunks_for(lines.line(i));
        return total;
    };

    auto render_wrapped = [&](int virtual_row, casio::device::FixedString<96> &out) {
        int row = virtual_row;
        for(int i = 0; i < lines.count(); i++) {
            const char *line = lines.line(i);
            int chunks = chunks_for(line);
            if(row >= chunks) {
                row -= chunks;
                continue;
            }

            if(row != 0) out.append("   ");

            int start = row * kMaxTextCols;
            for(int j = 0; j < kMaxTextCols && line[start + j] != '\0'; j++) {
                out.append_char(line[start + j]);
            }
            return;
        }
    };

    int top = 0;
    while(true) {
        casio::ui::draw_frame(title, "WORK");
        casio::ui::draw_section_label(4, casio::ui::kContentTop, "Working");

        int rows = casio::ui::visible_rows() - 1;
        int total = wrapped_count();
        for(int row = 0; row < rows; row++) {
            int index = top + row;
            if(index >= total) break;
            int y = casio::ui::kContentTop + 18 + row * casio::ui::kRowH;
            casio::device::FixedString<96> out;
            render_wrapped(index, out);
            casio::ui::draw_limited_text(4, y + 1, casio::ui::kInk, out.c_str(), 38);
        }

        casio::ui::draw_scrollbar(top, rows, total);
        casio::ui::draw_softkeys("BACK", "PGUP", "PGDN", "", "", "EXIT");
        dupdate();

        key_event_t event = casio::ui::wait_key_with_live_status(title, "WORK");
        if(event.type != KEYEV_DOWN) continue;
        if(event.key == KEY_EXIT || event.key == KEY_EXE) return;
        if(event.key == KEY_UP) {
            if(top > 0) top--;
            continue;
        }
        if(event.key == KEY_DOWN) {
            if(top + rows < total) top++;
            continue;
        }
        if(event.key == KEY_LEFT) {
            top -= rows;
            if(top < 0) top = 0;
            continue;
        }
        if(event.key == KEY_RIGHT) {
            if(top + rows < total) top += rows;
            if(top + rows > total && total > rows) top = total - rows;
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
        {"Stats"},
        {"SUVAT"},
    };

    draw_home();
    while(true) {
        key_event_t event = casio::ui::wait_key_with_live_status("CasioCAS", "RAD");
        if(event.type != KEYEV_DOWN) continue;
        if(event.key == KEY_EXIT) break;

        int quick_app = -1;
        if(event.key == KEY_F1) quick_app = 0;
        if(event.key == KEY_F2) quick_app = 2;
        if(event.key == KEY_F3) quick_app = 3;
        if(event.key == KEY_F4) quick_app = 4;
        if(event.key == KEY_F5) quick_app = 5;
        if(event.key == KEY_F6) quick_app = 6;
        if(event.key != KEY_EXE && quick_app < 0) continue;

        int app = quick_app;
        if(app < 0) app = casio::ui::menu_select("Home", home_items, array_count(home_items), 0);
        if(app < 0) {
            draw_home();
            continue;
        }

        switch(app) {
            case 0:
                run_module("CAS Shell", casio::device::Module::Shell, "2x+3=7", "expr, eqn, stats(...), diff(...)");
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
                run_module("Stats", casio::device::Module::Stats, "stats(1,2,3,4)", "stats(...), binomcdf(n,p,r)");
                break;
            case 7:
                run_module("SUVAT", casio::device::Module::Suvat, "s=10,u=0,v=?,a=2,t=5", "use ? for target");
                break;
            default:
                break;
        }

        draw_home();
    }

    return 1;
}

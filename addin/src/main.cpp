#include <gint/display.h>
#include <gint/keyboard.h>

#include "core/arena.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/format_expr.hpp"
#include "core/format_exam.hpp"

#include "modules/boolean/boolean.hpp"
#include "modules/suvat/suvat.hpp"
#include "modules/integrate/integrate.hpp"
#include "modules/algebra/algebra.hpp"
#include "modules/trig/trig.hpp"
#include "modules/derive/derive.hpp"
#include "ui/text_input.hpp"
#include "ui/menu.hpp"

static void draw_centered(const char *s)
{
    dclear(C_WHITE);
    int w = dtext_width(s, nullptr);
    int x = (DWIDTH - w) / 2;
    int y = (DHEIGHT - 16) / 2;
    dtext(x, y, C_BLACK, s);
    dtext(2, DHEIGHT - 18, C_BLACK, "EXE: continue   EXIT: quit");
    dupdate();
}

static void view_lines(const char *title, std::vector<std::string> const &lines)
{
    int idx = 0;
    while(true) {
        dclear(C_WHITE);
        dtext(2, 2, C_BLACK, title ? title : "");
        dline(0, 18, DWIDTH - 1, 18, C_BLACK);
        for(int row = 0; row < 9; row++) {
            int li = idx + row;
            if(li >= (int)lines.size()) break;
            dtext(2, 22 + row * 16, C_BLACK, lines[li].c_str());
        }
        dtext(2, DHEIGHT - 18, C_BLACK, "UP/DOWN scroll  EXE page  EXIT back");
        dupdate();
        key_event_t e2 = getkey();
        if(e2.type != KEYEV_DOWN) continue;
        if(e2.key == KEY_EXIT) break;
        if(e2.key == KEY_UP) {
            if(idx > 0) idx--;
            continue;
        }
        if(e2.key == KEY_DOWN) {
            if(idx + 1 < (int)lines.size()) idx++;
            continue;
        }
        if(e2.key == KEY_EXE) {
            idx += 9;
            if(idx >= (int)lines.size()) idx = 0;
        }
    }
}

int main(void)
{
    casio::Arena arena;
    (void)arena;

    draw_centered("CasioCAS (MVP)");

    while(true)
    {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;

        if(ev.key == KEY_EXIT) break;
        if(ev.key == KEY_EXE)
        {
            try {
                std::vector<std::string> home = {
                    "Boolean",
                    "SUVAT",
                    "Derive",
                    "Integrate",
                    "Algebra",
                    "Trig",
                };
                int app = casio::ui::menu_select("Home", home, 0);
                if(app < 0) continue;

                if(app == 0) {
                    std::vector<std::string> lines;
                    lines.push_back("simplify");
                    lines.push_back("nand form");
                    lines.push_back("nor form");
                    lines.push_back("prove");
                    int mode = casio::ui::menu_select("Boolean", lines, 0);
                    if(mode < 0) continue;
                    mode += 1;

                    if(mode == 1) {
                    std::string expr = "A.(B+C)";
                    if(!casio::ui::text_input(expr, "Simplify", "EXE ok  EXIT cancel")) continue;
                    auto cur = casio::boolean::parse(expr);
                    std::vector<std::string> out;
                    out.push_back("1. " + casio::boolean::show(cur));
                    for(int n = 2; n <= 50; n++) {
                        auto hit = casio::boolean::step(cur);
                        if(!hit.first) break;
                        cur = hit.first;
                        out.push_back(std::to_string(n) + ". " + casio::boolean::show(cur) + " (" + hit.second + ")");
                    }
                    out.push_back("Result: " + casio::boolean::show(cur));
                    view_lines("Simplify", out);
                    }
                    else if(mode == 2) {
                    std::string expr = "A.B";
                    if(!casio::ui::text_input(expr, "To NAND", "EXE ok  EXIT cancel")) continue;
                    auto cur = casio::boolean::parse(expr);
                    auto nanded = casio::boolean::normalise(casio::boolean::to_nand(cur));
                    view_lines("NAND form", { "1. " + casio::boolean::show(cur), "2. " + casio::boolean::show(nanded) });
                    }
                    else if(mode == 3) {
                    std::string expr = "A+B";
                    if(!casio::ui::text_input(expr, "To NOR", "EXE ok  EXIT cancel")) continue;
                    auto cur = casio::boolean::parse(expr);
                    auto nored = casio::boolean::normalise(casio::boolean::to_nor(cur));
                    view_lines("NOR form", { "1. " + casio::boolean::show(cur), "2. " + casio::boolean::show(nored) });
                    }
                    else if(mode == 4) {
                    std::string lhs = "A.(B+C)";
                    std::string rhs = "A.B+A.C";
                    if(!casio::ui::text_input(lhs, "Prove LHS", "EXE ok  EXIT cancel")) continue;
                    if(!casio::ui::text_input(rhs, "Prove RHS", "EXE ok  EXIT cancel")) continue;
                    auto [plines, err] = casio::boolean::prove(lhs, rhs);
                    if(!err.empty()) view_lines("Prove", { "Error:", err });
                    else view_lines("Prove", plines);
                    }
                }
                else if(app == 1) {
                    casio::suvat::Inputs in;
                    in.s = "10";
                    in.u = "0";
                    in.v = "";
                    in.a = "2";
                    in.t = "5";
                    in.target = "v";
                    casio::ui::text_input(in.s, "s", "blank or ,target");
                    casio::ui::text_input(in.u, "u", "blank or ,target");
                    casio::ui::text_input(in.v, "v", "blank or ,target");
                    casio::ui::text_input(in.a, "a", "blank or ,target");
                    casio::ui::text_input(in.t, "t", "blank or ,target");
                    auto lines = casio::suvat::solve_all(arena, in);
                    view_lines("SUVAT", lines);
                }
                else if(app == 2) {
                    casio::derive::Request req;
                    req.mode = 1;
                    req.expr = "x^2";
                    casio::ui::text_input(req.expr, "Derive", "expr or expr,var");
                    auto lines = casio::derive::run(arena, req);
                    view_lines("Derive", lines);
                }
                else if(app == 3) {
                    casio::integrate::Request req;
                    req.expr = "sinx";
                    casio::ui::text_input(req.expr, "Integrate", "enter integrand");
                    auto lines = casio::integrate::run(arena, req);
                    view_lines("Integrate", lines);
                }
                else if(app == 4) {
                    casio::algebra::Request req;
                    req.expr = "2x+3=7";
                    casio::ui::text_input(req.expr, "Algebra", "enter expression/equation");
                    auto lines = casio::algebra::run(arena, req);
                    view_lines("Algebra", lines);
                }
                else if(app == 5) {
                    casio::trig::Request req;
                    req.expr = "sin(30)";
                    casio::ui::text_input(req.expr, "Trig", "enter trig expression");
                    auto lines = casio::trig::run(arena, req);
                    view_lines("Trig", lines);
                }
            }
            catch(...) {
                draw_centered("Input error");
            }
        }
    }

    return 1;
}


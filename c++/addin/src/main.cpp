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

static void shell_draw(std::vector<std::string> const &scrollback, std::string const &cmd)
{
    dclear(C_WHITE);
    dtext(2, 2, C_BLACK, "CAS Shell");
    dline(0, 18, DWIDTH - 1, 18, C_BLACK);

    // Show last 8 lines of scrollback.
    int max_lines = 8;
    int start = 0;
    if((int)scrollback.size() > max_lines) start = (int)scrollback.size() - max_lines;
    for(int row = 0; row < max_lines; row++) {
        int li = start + row;
        if(li >= (int)scrollback.size()) break;
        dtext(2, 22 + row * 14, C_BLACK, scrollback[li].c_str());
    }

    dline(0, DHEIGHT - 44, DWIDTH - 1, DHEIGHT - 44, C_BLACK);
    std::string prompt = "> " + cmd;
    dtext(2, DHEIGHT - 40, C_BLACK, prompt.c_str());
    dtext(2, DHEIGHT - 18, C_BLACK, "EXE edit/run  F1 hist  F2 clear  EXIT back");
    dupdate();
}

static std::string replace_word(std::string s, std::string const &name, std::string const &value)
{
    if(name.empty()) return s;
    std::string out;
    out.reserve(s.size() + value.size());
    auto is_word = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; };
    for(std::size_t i = 0; i < s.size();) {
        bool match = false;
        if(i + name.size() <= s.size() && s.compare(i, name.size(), name) == 0) {
            char left = (i == 0) ? '\0' : s[i - 1];
            char right = (i + name.size() >= s.size()) ? '\0' : s[i + name.size()];
            if(!is_word(left) && !is_word(right)) match = true;
        }
        if(match) {
            out += value;
            i += name.size();
        }
        else {
            out.push_back(s[i]);
            i++;
        }
    }
    return out;
}

static void run_shell(casio::Arena &arena)
{
    std::vector<std::string> scrollback;
    std::vector<std::string> history;
    std::string ans_expr;
    std::string cmd;

    scrollback.push_back("Type: expr | eqn | diff(x^2) | int(x^2)");
    while(true) {
        shell_draw(scrollback, cmd);
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;
        if(ev.key == KEY_EXIT) return;

        if(ev.key == KEY_F2) {
            scrollback.clear();
            scrollback.push_back("Cleared.");
            continue;
        }
        if(ev.key == KEY_F1) {
            if(history.empty()) continue;
            int pick = casio::ui::menu_select("History", history, (int)history.size() - 1);
            if(pick >= 0 && pick < (int)history.size()) cmd = history[pick];
            continue;
        }
        if(ev.key != KEY_EXE) continue;

        std::string edit = cmd;
        if(!casio::ui::text_input(edit, "CAS Shell", "EXE ok  EXIT cancel")) continue;
        cmd = edit;
        if(cmd.empty()) continue;
        history.push_back(cmd);
        if(history.size() > 50) history.erase(history.begin());

        std::string run = cmd;
        if(!ans_expr.empty()) run = replace_word(run, "ans", ans_expr);

        try {
            std::vector<std::string> out;
            if(run.rfind("diff(", 0) == 0 && run.size() >= 6 && run.back() == ')') {
                casio::derive::Request req;
                req.mode = 1;
                req.expr = run.substr(5, run.size() - 6);
                out = casio::derive::run(arena, req);
            }
            else if(run.rfind("int(", 0) == 0 && run.size() >= 5 && run.back() == ')') {
                casio::integrate::Request req;
                req.mode = 1;
                req.expr = run.substr(4, run.size() - 5);
                out = casio::integrate::run(arena, req);
            }
            else if(run.find('=') != std::string::npos) {
                casio::algebra::Request req;
                req.mode = 6;
                req.expr = run;
                out = casio::algebra::run(arena, req);
            }
            else {
                NodeId parsed = casio::parse_expr(arena, run);
                NodeId simp = casio::simplify(arena, parsed);
                std::string h = casio::format_expr_human(arena, simp);
                out = casio::format_exam_working("simplify", {"Normalize: " + casio::normalize_text(run)}, h);
            }

            scrollback.push_back("> " + cmd);
            for(auto const &ln : out) scrollback.push_back(ln);

            // Update ans from last Answer: line if present.
            for(auto const &ln : out) {
                std::string low;
                low.reserve(ln.size());
                for(char c : ln) low.push_back((char)std::tolower((unsigned char)c));
                auto pos = low.find("answer:");
                if(pos != std::string::npos) {
                    std::string rhs = ln.substr(pos + 7);
                    while(!rhs.empty() && (rhs.front() == ' ' || rhs.front() == '\t')) rhs.erase(rhs.begin());
                    ans_expr = rhs;
                }
            }
        }
        catch(...) {
            scrollback.push_back("> " + cmd);
            scrollback.push_back("ERR: failed to evaluate.");
        }
        if(scrollback.size() > 200) {
            scrollback.erase(scrollback.begin(), scrollback.begin() + (scrollback.size() - 200));
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
                    "CAS Shell",
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
                    run_shell(arena);
                }
                else if(app == 1) {
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
                else if(app == 2) {
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
                else if(app == 3) {
                    casio::derive::Request req;
                    req.mode = 1;
                    req.expr = "x^2";
                    casio::ui::text_input(req.expr, "Derive", "expr or expr,var");
                    auto lines = casio::derive::run(arena, req);
                    view_lines("Derive", lines);
                }
                else if(app == 4) {
                    casio::integrate::Request req;
                    req.expr = "sinx";
                    casio::ui::text_input(req.expr, "Integrate", "enter integrand");
                    auto lines = casio::integrate::run(arena, req);
                    view_lines("Integrate", lines);
                }
                else if(app == 5) {
                    casio::algebra::Request req;
                    req.expr = "2x+3=7";
                    casio::ui::text_input(req.expr, "Algebra", "enter expression/equation");
                    auto lines = casio::algebra::run(arena, req);
                    view_lines("Algebra", lines);
                }
                else if(app == 6) {
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


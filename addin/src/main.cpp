#include <gint/display.h>
#include <gint/keyboard.h>

#include "core/arena.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/format_expr.hpp"
#include "core/format_exam.hpp"

#include "modules/boolean/boolean.hpp"
#include "ui/text_input.hpp"

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

int main(void)
{
    casio::Arena arena;
    (void)arena;

    draw_centered("CasioCAS (Boolean MVP)");

    while(true)
    {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;

        if(ev.key == KEY_EXIT) break;
        if(ev.key == KEY_EXE)
        {
            std::string expr = "A.(B+C)";
            if(!casio::ui::text_input(expr, "Boolean expr", "EXE ok  EXIT cancel")) {
                draw_centered("Cancelled");
                continue;
            }
            try {
                auto cur = casio::boolean::parse(expr);
                // Build output lines like python mode 1 (limit 50).
                std::vector<std::string> lines;
                lines.push_back("1. " + casio::boolean::show(cur));
                for(int n = 2; n <= 50; n++) {
                    auto hit = casio::boolean::step(cur);
                    if(!hit.first) break;
                    cur = hit.first;
                    lines.push_back(std::to_string(n) + ". " + casio::boolean::show(cur) + " (" + hit.second + ")");
                }
                lines.push_back("Result: " + casio::boolean::show(cur));

                // Simple viewer: page through lines with EXE, exit with EXIT.
                int idx = 0;
                while(true) {
                    dclear(C_WHITE);
                    dtext(2, 2, C_BLACK, "Boolean simplify");
                    dline(0, 18, DWIDTH - 1, 18, C_BLACK);
                    for(int row = 0; row < 9; row++) {
                        int li = idx + row;
                        if(li >= (int)lines.size()) break;
                        dtext(2, 22 + row * 16, C_BLACK, lines[li].c_str());
                    }
                    dtext(2, DHEIGHT - 18, C_BLACK, "EXE next  EXIT back");
                    dupdate();
                    key_event_t e2 = getkey();
                    if(e2.type != KEYEV_DOWN) continue;
                    if(e2.key == KEY_EXIT) break;
                    if(e2.key == KEY_EXE) {
                        idx += 9;
                        if(idx >= (int)lines.size()) idx = 0;
                    }
                }
            }
            catch(...) {
                draw_centered("Input error");
            }
        }
    }

    return 1;
}


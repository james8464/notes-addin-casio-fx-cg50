#include <gint/display.h>
#include <gint/keyboard.h>

#include "core/arena.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/format_expr.hpp"
#include "core/format_exam.hpp"

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

    draw_centered("CasioCAS (port in progress)");

    while(true)
    {
        key_event_t ev = getkey();
        if(ev.type != KEYEV_DOWN) continue;

        if(ev.key == KEY_EXIT) break;
        if(ev.key == KEY_EXE)
        {
            try {
                auto root = casio::parse_expr(arena, "sin^-1x + √x + |x| + 2x");
                std::string demo = casio::format_expr(arena, root);
                std::string demo2 = casio::format_equation_human_readable(arena, root);
                draw_centered((demo + " || " + demo2).c_str());
            }
            catch(...) {
                draw_centered("parse error");
            }
        }
    }

    return 1;
}


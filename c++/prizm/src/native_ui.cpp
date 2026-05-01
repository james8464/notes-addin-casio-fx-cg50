#include "native_ui.hpp"

#include "device/fixed_string.hpp"

#include <fxcg/display.h>

namespace casio::prizm
{
namespace
{

constexpr int kRows = 7;
constexpr int kLineHeight = 20;

void print_line(int row, const char *text, int mode = TEXT_MODE_NORMAL, int color = TEXT_COLOR_BLACK)
{
    PrintXY(1, row * kLineHeight + 20, text ? text : "", mode, color);
}

void print_centered(int row, const char *text)
{
    PrintXY(64, row * kLineHeight + 20, text ? text : "", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
}

}

void init_native_screen(const char *title)
{
    Bdisp_AllClr_VRAM();
    EnableStatusArea(1);
    char c1 = 0;
    char c2 = 0;
    DefineStatusAreaFlags(
        DSA_SETDEFAULT,
        SAF_BATTERY | SAF_ALPHA_SHIFT | SAF_SETUP_ANGLE,
        &c1,
        &c2);
    DisplayStatusArea();
    DrawHeaderLine();
    PrintXY(5, 5, title ? title : "CasioCAS", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
}

void draw_status_line(const char *text)
{
    PrintXY(1, 218, text ? text : "", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
}

void draw_softkeys(const char *k1, const char *k2, const char *k3,
                   const char *k4, const char *k5, const char *k6)
{
    PrintXY(1, 218, k1 ? k1 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(49, 218, k2 ? k2 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(97, 218, k3 ? k3 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(145, 218, k4 ? k4 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(193, 218, k5 ? k5 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(241, 218, k6 ? k6 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
}

void draw_home(void)
{
    init_native_screen("CasioCAS");
    print_line(0, "Press F-keys for tools:");
    print_line(1, "");
    print_line(2, "F1: CAS Shell        F2: Simplify");
    print_line(3, "F3: Algebra          F4: Derive");
    print_line(4, "F5: Integrate        F6: Trig");
    print_line(5, "");
    print_line(6, "Or press EXE for menu");
    draw_softkeys("SHELL", "SMP", "ALG", "DER", "INT", "TRG");
    Bdisp_PutDisp_DD();
}

void draw_menu(const char *title, const char *const *items, int count, int selected, int top)
{
    init_native_screen(title);
    for(int row = 0; row < kRows; row++) {
        int index = top + row;
        if(index >= count) break;
        casio::device::FixedString<64> line;
        line.append_int(index + 1);
        line.append(". ");
        line.append(items[index]);
        print_line(row, line.c_str(), index == selected ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL);
    }
    
    bool has_more = (top + kRows) < count;
    casio::device::FixedString<32> nav;
    if(top > 0) nav.append("↑ ");
    if(has_more) nav.append("↓");
    if(top > 0 || has_more) draw_status_line(nav.c_str());
    
    draw_softkeys("SEL", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

void draw_lines(const char *title, casio::device::OutputLines const &lines, int top, bool more_above, bool more_below)
{
    init_native_screen(title);
    for(int row = 0; row < kRows; row++) {
        int index = top + row;
        if(index >= lines.count()) break;
        print_line(row, lines.line(index));
    }
    
    casio::device::FixedString<16> nav;
    if(more_above) nav.append("↑");
    if(more_above && more_below) nav.append(" ");
    if(more_below) nav.append("↓");
    if(more_above || more_below) draw_status_line(nav.c_str());
    
    draw_softkeys("BACK", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

}

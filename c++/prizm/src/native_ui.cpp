#include "native_ui.hpp"

#include "device/fixed_string.hpp"

#include <fxcg/display.h>

namespace casio::prizm
{
namespace
{

constexpr int kRows = 8;

void print_line(int row, const char *text, int mode = TEXT_MODE_NORMAL)
{
    PrintXY(1, row, text ? text : "", mode, TEXT_COLOR_BLACK);
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
    print_line(1, title ? title : "CasioCAS");
}

void draw_softkeys(const char *k1, const char *k2, const char *k3,
                   const char *k4, const char *k5, const char *k6)
{
    PrintXY(1, 8, k1 ? k1 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(7, 8, k2 ? k2 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(13, 8, k3 ? k3 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(19, 8, k4 ? k4 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(25, 8, k5 ? k5 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(31, 8, k6 ? k6 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
}

void draw_home(void)
{
    init_native_screen("CasioCAS");
    print_line(2, "EXE: Function Catalog");
    print_line(3, "F1 Shell  F2 Algebra");
    print_line(4, "F3 Deriv  F4 Integr");
    print_line(5, "F5 Trig   F6 SUVAT");
    draw_softkeys("SHELL", "ALG", "DER", "INT", "TRIG", "SUV");
    Bdisp_PutDisp_DD();
}

void draw_menu(const char *title, const char *const *items, int count, int selected, int top)
{
    init_native_screen(title);
    for(int row = 0; row < kRows - 2; row++) {
        int index = top + row;
        if(index >= count) break;
        casio::device::FixedString<64> line;
        line.append_int(index + 1);
        line.append(" ");
        line.append(items[index]);
        print_line(row + 2, line.c_str(), index == selected ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL);
    }
    draw_softkeys("SEL", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

void draw_lines(const char *title, casio::device::OutputLines const &lines, int top)
{
    init_native_screen(title);
    for(int row = 0; row < kRows - 2; row++) {
        int index = top + row;
        if(index >= lines.count()) break;
        print_line(row + 2, lines.line(index));
    }
    draw_softkeys("BACK", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

}

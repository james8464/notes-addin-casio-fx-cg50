#pragma once

#include "device/line_buffer.hpp"

namespace casio::prizm
{

constexpr int kShellVisibleRows = 4;

void init_native_screen(const char *title, const char *mode = nullptr);
void draw_input_box(int x, int y, int w, int h);
void draw_editor_brackets(const unsigned char *input, int input_start, int col, int row, int max_cols);
void draw_home(void);
void draw_menu(const char *title, const char *const *items, int count, int selected, int top);
void draw_shell(const char *title, const char *mode, const char *const *lines, int count, int top, int selected,
                unsigned char *input, int input_start, int input_cursor,
                const char *k1, const char *k2, const char *k3,
                const char *k4, const char *k5, const char *k6);
void draw_lines(const char *title, casio::device::OutputLines const &lines, int top, bool more_above, bool more_below);
void draw_softkeys(const char *k1, const char *k2, const char *k3,
                   const char *k4, const char *k5, const char *k6);
void draw_status_line(const char *text);

}

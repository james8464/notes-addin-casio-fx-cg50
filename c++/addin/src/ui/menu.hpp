#pragma once

namespace casio::ui
{

struct MenuItem {
    const char *label;
};

// Simple on-device menu.
// - Up/Down: move
// - EXE: select
// - EXIT: cancel (-1)
int menu_select(const char *title, MenuItem const *items, int count, int start_index = 0);

} // namespace casio::ui

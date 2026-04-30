#pragma once

#include <string>
#include <vector>

namespace casio::ui
{

// Simple on-device menu.
// - Up/Down: move
// - EXE: select
// - EXIT: cancel (-1)
int menu_select(const char *title, std::vector<std::string> const &items, int start_index = 0);

} // namespace casio::ui


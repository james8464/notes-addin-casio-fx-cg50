#pragma once

#include <string>

namespace casio::ui
{

// Very small on-calc text editor for ASCII-ish expressions.
// - EXE: accept
// - EXIT: cancel (returns false)
// - DEL: delete last char
// - AC: clear
//
// Allowed chars depend on key mapping implemented in .cpp.
bool text_input(std::string &io_text, const char *title, const char *help);

} // namespace casio::ui


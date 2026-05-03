#pragma once

namespace casio::ui
{

// Bounded on-device text input. The buffer is always NUL-terminated.
bool text_input(char *buffer, int capacity, const char *title, const char *help);

} // namespace casio::ui

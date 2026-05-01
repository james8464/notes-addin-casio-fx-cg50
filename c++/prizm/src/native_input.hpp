#pragma once

namespace casio::prizm
{

bool text_input(char *buffer, int capacity, const char *title, const char *help);
void show_error(const char *message);
void show_info(const char *message);

}

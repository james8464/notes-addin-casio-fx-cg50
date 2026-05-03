#pragma once

namespace casio::prizm
{

bool text_input(char *buffer, int capacity, const char *title, const char *help);
void editor_from_ascii(unsigned char *editor, int capacity, const char *text);
void editor_insert_ascii(unsigned char *editor, int capacity, int &cursor, const char *text);
bool editor_to_ascii(const unsigned char *editor, char *out, int capacity, bool uppercase);
void show_error(const char *message);
void show_info(const char *message);

}

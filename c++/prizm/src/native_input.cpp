#include "native_input.hpp"

#include "device/fixed_string.hpp"
#include "native_ui.hpp"

#include <fxcg/display.h>
#include <fxcg/keyboard.h>

namespace casio::prizm
{
namespace
{

int len(const char *s)
{
    int n = 0;
    while(s != nullptr && s[n] != '\0') n++;
    return n;
}

void print_row(int row, const char *text, int mode = TEXT_MODE_NORMAL, int color = TEXT_COLOR_BLACK)
{
    casio::device::FixedString<80> out;
    out.append("  ");
    out.append(text ? text : "");
    PrintXY(1, row, out.c_str(), mode, color);
}

void append_text(char *buffer, int capacity, int &length, const char *text)
{
    if(text == nullptr) return;
    for(int i = 0; text[i] != '\0'; i++) {
        if(length + 1 >= capacity) break;
        buffer[length++] = text[i];
        buffer[length] = '\0';
    }
}

void append_char(char *buffer, int capacity, int &length, char c)
{
    if(length + 1 >= capacity) return;
    buffer[length++] = c;
    buffer[length] = '\0';
}

bool translate_editor_buffer(const unsigned char *editor, char *out, int capacity)
{
    if(editor == nullptr || out == nullptr || capacity <= 0) return false;
    int length = 0;
    out[0] = '\0';
    for(int i = 0; editor[i] != '\0'; i++) {
        unsigned char c = editor[i];

        if(c >= '0' && c <= '9') append_char(out, capacity, length, (char)c);
        else if(c >= 'A' && c <= 'Z') append_char(out, capacity, length, (char)(c - 'A' + 'a'));
        else if(c >= 'a' && c <= 'z') append_char(out, capacity, length, (char)c);
        else if(c == (unsigned char)KEY_CHAR_SPACE) append_char(out, capacity, length, ' ');
        else if(c == (unsigned char)KEY_CHAR_DP) append_char(out, capacity, length, '.');
        else if(c == (unsigned char)KEY_CHAR_PLUS) append_char(out, capacity, length, '+');
        else if(c == (unsigned char)KEY_CHAR_MINUS || c == (unsigned char)KEY_CHAR_PMINUS) append_char(out, capacity, length, '-');
        else if(c == (unsigned char)KEY_CHAR_MULT) append_char(out, capacity, length, '*');
        else if(c == (unsigned char)KEY_CHAR_DIV || c == (unsigned char)KEY_CHAR_FRAC) append_char(out, capacity, length, '/');
        else if(c == (unsigned char)KEY_CHAR_LPAR) append_char(out, capacity, length, '(');
        else if(c == (unsigned char)KEY_CHAR_RPAR) append_char(out, capacity, length, ')');
        else if(c == (unsigned char)KEY_CHAR_COMMA) append_char(out, capacity, length, ',');
        else if(c == (unsigned char)KEY_CHAR_EQUAL) append_char(out, capacity, length, '=');
        else if(c == (unsigned char)KEY_CHAR_LBRCKT) append_char(out, capacity, length, '[');
        else if(c == (unsigned char)KEY_CHAR_RBRCKT) append_char(out, capacity, length, ']');
        else if(c == (unsigned char)KEY_CHAR_LBRACE) append_char(out, capacity, length, '{');
        else if(c == (unsigned char)KEY_CHAR_RBRACE) append_char(out, capacity, length, '}');
        else if(c == (unsigned char)KEY_CHAR_LOG) append_text(out, capacity, length, "log(");
        else if(c == (unsigned char)KEY_CHAR_LN) append_text(out, capacity, length, "ln(");
        else if(c == (unsigned char)KEY_CHAR_SIN) append_text(out, capacity, length, "sin(");
        else if(c == (unsigned char)KEY_CHAR_COS) append_text(out, capacity, length, "cos(");
        else if(c == (unsigned char)KEY_CHAR_TAN) append_text(out, capacity, length, "tan(");
        else if(c == (unsigned char)KEY_CHAR_ASIN) append_text(out, capacity, length, "asin(");
        else if(c == (unsigned char)KEY_CHAR_ACOS) append_text(out, capacity, length, "acos(");
        else if(c == (unsigned char)KEY_CHAR_ATAN) append_text(out, capacity, length, "atan(");
        else if(c == (unsigned char)KEY_CHAR_ROOT) append_text(out, capacity, length, "sqrt(");
        else if(c == (unsigned char)KEY_CHAR_CUBEROOT) append_text(out, capacity, length, "cbrt(");
        else if(c == (unsigned char)KEY_CHAR_SQUARE) append_text(out, capacity, length, "^2");
        else if(c == (unsigned char)KEY_CHAR_POW) append_char(out, capacity, length, '^');
        else if(c == (unsigned char)KEY_CHAR_EXPN) append_text(out, capacity, length, "exp(");
        else if(c == (unsigned char)KEY_CHAR_EXPN10) append_text(out, capacity, length, "10^(");
        else if(c == (unsigned char)KEY_CHAR_RECIP) append_text(out, capacity, length, "^-1");
        else if(c == (unsigned char)KEY_CHAR_PI) append_text(out, capacity, length, "pi");
        else if(c == (unsigned char)KEY_CHAR_ANS) append_text(out, capacity, length, "ans");
        else if(c == (unsigned char)KEY_CHAR_VALR) append_text(out, capacity, length, "r");
        else if(c == (unsigned char)KEY_CHAR_THETA) append_text(out, capacity, length, "theta");
        else if(c == (unsigned char)KEY_CHAR_EXP) append_char(out, capacity, length, 'e');
        else if(c == 0x7f && editor[i + 1] != '\0') {
            unsigned char next = editor[++i];
            if(next == 0x50) append_char(out, capacity, length, 'i');
            else if(next == 0x54) append_text(out, capacity, length, "angle");
        }
    }
    return true;
}

void insert_ascii(unsigned char *buffer, int capacity, int &cursor, const char *text)
{
    if(buffer == nullptr || text == nullptr) return;
    for(int i = 0; text[i] != '\0'; i++) {
        cursor = EditMBStringChar(buffer, capacity, cursor, (unsigned char)text[i]);
    }
}

void draw_input(const char *title, const char *help, unsigned char *buffer, int start, int cursor)
{
    init_native_screen(title);
    print_row(2, help ? help : "");
    DisplayMBString(buffer, start, cursor, 1, 4);
    draw_softkeys("SQRT", "LOG", "SIN", "COS", "PI", "?");
    Bdisp_PutDisp_DD();
}

}

void show_error(const char *message)
{
    init_native_screen("Error");
    print_row(3, message ? message : "Unknown error");
    draw_softkeys("", "", "", "", "", "OK");
    Bdisp_PutDisp_DD();
    
    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXE || key == KEY_CTRL_EXIT) break;
    }
}

void show_info(const char *message)
{
    init_native_screen("Info");
    print_row(3, message ? message : "");
    draw_softkeys("", "", "", "", "", "OK");
    Bdisp_PutDisp_DD();
    
    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXE || key == KEY_CTRL_EXIT) break;
    }
}

bool text_input(char *buffer, int capacity, const char *title, const char *help)
{
    if(buffer == nullptr || capacity <= 0) return false;

    unsigned char editor[128];
    int editor_capacity = (int)sizeof(editor);
    if(capacity < editor_capacity) editor_capacity = capacity;
    for(int i = 0; i < editor_capacity; i++) editor[i] = 0;
    int initial = len(buffer);
    if(initial >= editor_capacity) initial = editor_capacity - 1;
    for(int i = 0; i < initial; i++) editor[i] = (unsigned char)buffer[i];
    editor[initial] = '\0';

    int start = 0;
    int cursor = initial;
    draw_input(title, help, editor, start, cursor);

    while(true) {
        int key = 0;
        GetKey(&key);

        if(key == KEY_CTRL_EXIT) return false;
        if(key == KEY_CTRL_EXE) return translate_editor_buffer(editor, buffer, capacity);
        if(key == KEY_CTRL_AC) {
            start = 0;
            cursor = 0;
            editor[0] = '\0';
            draw_input(title, help, editor, start, cursor);
            continue;
        }

        if(key == KEY_CTRL_F1) insert_ascii(editor, editor_capacity, cursor, "sqrt(");
        else if(key == KEY_CTRL_F2) insert_ascii(editor, editor_capacity, cursor, "log(");
        else if(key == KEY_CTRL_F3) insert_ascii(editor, editor_capacity, cursor, "sin(");
        else if(key == KEY_CTRL_F4) insert_ascii(editor, editor_capacity, cursor, "cos(");
        else if(key == KEY_CTRL_F5) insert_ascii(editor, editor_capacity, cursor, "pi");
        else if(key == KEY_CTRL_F6) insert_ascii(editor, editor_capacity, cursor, "?");
        else if(key && key < 30000) cursor = EditMBStringChar(editor, editor_capacity, cursor, key);
        else EditMBStringCtrl(editor, editor_capacity, &start, &cursor, &key, 1, 4);

        draw_input(title, help, editor, start, cursor);
    }
}

}

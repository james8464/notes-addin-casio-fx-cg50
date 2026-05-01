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

void insert_text(char *buffer, int capacity, int &length, int &cursor, const char *text)
{
    if(text == nullptr) return;
    for(int i = 0; text[i] != '\0'; i++) {
        if(length + 1 >= capacity) break;
        for(int j = length; j >= cursor; j--) buffer[j + 1] = buffer[j];
        buffer[cursor] = text[i];
        length++;
        cursor++;
    }
}

const char *key_to_text(int key)
{
    if(key >= KEY_CHAR_0 && key <= KEY_CHAR_9) {
        static char digit[2];
        digit[0] = (char)key;
        digit[1] = '\0';
        return digit;
    }

    if(key >= KEY_CHAR_A && key <= KEY_CHAR_Z) {
        static char letter[2];
        letter[0] = (char)('a' + (key - KEY_CHAR_A));
        letter[1] = '\0';
        return letter;
    }

    if(key == KEY_CHAR_DP) return ".";
    if(key == KEY_CHAR_PLUS) return "+";
    if(key == KEY_CHAR_MINUS) return "-";
    if(key == KEY_CHAR_MULT) return "*";
    if(key == KEY_CHAR_DIV) return "/";
    if(key == KEY_CHAR_LPAR) return "(";
    if(key == KEY_CHAR_RPAR) return ")";
    if(key == KEY_CHAR_COMMA) return ",";
    if(key == KEY_CHAR_EQUAL) return "=";
    if(key == KEY_CHAR_LOG) return "log(";
    if(key == KEY_CHAR_LN) return "ln(";
    if(key == KEY_CHAR_SIN) return "sin(";
    if(key == KEY_CHAR_COS) return "cos(";
    if(key == KEY_CHAR_TAN) return "tan(";
    if(key == KEY_CHAR_SQUARE) return "^2";
    if(key == KEY_CHAR_POW) return "^";
    if(key == KEY_CHAR_ROOT) return "sqrt(";
    if(key == KEY_CHAR_EXPN) return "exp(";
    if(key == KEY_CHAR_EXPN10) return "10^(";
    if(key == KEY_CHAR_ASIN) return "asin(";
    if(key == KEY_CHAR_ACOS) return "acos(";
    if(key == KEY_CHAR_ATAN) return "atan(";
    if(key == KEY_CHAR_PI) return "pi";
    return nullptr;
}

void draw_input(const char *title, const char *help, const char *buffer)
{
    init_native_screen(title);
    PrintXY(1, 2, help ? help : "", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    PrintXY(1, 4, buffer ? buffer : "", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    draw_softkeys("sqrt", "log", "sin", "cos", "pi", "=");
    Bdisp_PutDisp_DD();
}

}

bool text_input(char *buffer, int capacity, const char *title, const char *help)
{
    if(buffer == nullptr || capacity <= 0) return false;

    int length = len(buffer);
    if(length >= capacity) {
        length = capacity - 1;
        buffer[length] = '\0';
    }
    int cursor = length;

    draw_input(title, help, buffer);

    while(true) {
        int key = 0;
        GetKey(&key);

        if(key == KEY_CTRL_EXIT) return false;
        if(key == KEY_CTRL_EXE) return true;

        if(key == KEY_CTRL_LEFT) {
            if(cursor > 0) cursor--;
            continue;
        }
        if(key == KEY_CTRL_RIGHT) {
            if(cursor < length) cursor++;
            continue;
        }
        if(key == KEY_CTRL_DEL) {
            if(cursor > 0) {
                for(int i = cursor - 1; i < length; i++) buffer[i] = buffer[i + 1];
                length--;
                cursor--;
                draw_input(title, help, buffer);
            }
            continue;
        }
        if(key == KEY_CTRL_AC) {
            length = 0;
            cursor = 0;
            buffer[0] = '\0';
            draw_input(title, help, buffer);
            continue;
        }

        if(key == KEY_CTRL_F1) insert_text(buffer, capacity, length, cursor, "sqrt(");
        else if(key == KEY_CTRL_F2) insert_text(buffer, capacity, length, cursor, "log(");
        else if(key == KEY_CTRL_F3) insert_text(buffer, capacity, length, cursor, "sin(");
        else if(key == KEY_CTRL_F4) insert_text(buffer, capacity, length, cursor, "cos(");
        else if(key == KEY_CTRL_F5) insert_text(buffer, capacity, length, cursor, "pi");
        else if(key == KEY_CTRL_F6) insert_text(buffer, capacity, length, cursor, "=");
        else insert_text(buffer, capacity, length, cursor, key_to_text(key));

        draw_input(title, help, buffer);
    }
}

}

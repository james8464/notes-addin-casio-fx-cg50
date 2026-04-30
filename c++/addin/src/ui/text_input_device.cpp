#include "ui/text_input_device.hpp"

#include "device/fixed_string.hpp"
#include "ui/theme.hpp"

#include <gint/drivers/keydev.h>
#include <gint/gint.h>
#include <gint/keyboard.h>
#include <gint/timer.h>

namespace casio::ui
{
namespace
{

static int text_len(const char *s)
{
    return casio::device::cstr_len(s);
}

enum class EditorMode
{
    Normal,
    Shift,
    Alpha,
    AlphaLock,
};

static const char *mode_label(EditorMode mode)
{
    if(mode == EditorMode::Shift) return "SHIFT";
    if(mode == EditorMode::Alpha) return "ALPHA";
    if(mode == EditorMode::AlphaLock) return "A-LOCK";
    return "INPUT";
}

static const char *mode_help(EditorMode mode)
{
    if(mode == EditorMode::Shift) return "SHIFT: next key uses yellow label";
    if(mode == EditorMode::Alpha) return "ALPHA: next key types a letter";
    if(mode == EditorMode::AlphaLock) return "ALPHA LOCK: ALPHA exits";
    return "SHIFT/ALPHA work like RUN-MAT";
}

static void draw_mode_softkeys(EditorMode mode)
{
    if(mode == EditorMode::Shift) {
        draw_softkeys("sqrt", "10^", "asin", "acos", "atan", "exp");
        return;
    }
    draw_softkeys("sqrt", "log", "sin", "cos", "pi", "=");
}

static void draw_input(
    const char *title,
    const char *help,
    const char *text,
    int cursor,
    bool caret_on,
    EditorMode mode)
{
    draw_frame(title, mode_label(mode));
    draw_section_label(4, kContentTop, "Expression");

    int len = text_len(text);
    if(cursor < 0) cursor = 0;
    if(cursor > len) cursor = len;

    int start = 0;
    int max_chars = 34;
    if(cursor > max_chars) start = cursor - max_chars;
    if(len - start > max_chars && cursor <= start) start = cursor;

    casio::device::FixedString<40> shown;
    if(start > 0) shown.append("...");
    for(int i = start; text != nullptr && text[i] != '\0' && i < start + max_chars; i++) {
        shown.append_char(text[i]);
    }

    int box_y0 = kContentTop + 22;
    int box_y1 = box_y0 + 28;
    drect(4, box_y0, DWIDTH - 12, box_y1, kPaper);
    dline(4, box_y0, DWIDTH - 12, box_y0, kInk);
    dline(4, box_y1, DWIDTH - 12, box_y1, kInk);
    dline(4, box_y0, 4, box_y1, kInk);
    dline(DWIDTH - 12, box_y0, DWIDTH - 12, box_y1, kInk);
    dtext(10, box_y0 + 8, kInk, shown.c_str());

    int cursor_x = 10 + (cursor - start + (start > 0 ? 3 : 0)) * 7;
    if(cursor_x < 10) cursor_x = 10;
    if(cursor_x > DWIDTH - 20) cursor_x = DWIDTH - 20;
    if(caret_on) {
        dline(cursor_x, box_y0 + 5, cursor_x, box_y1 - 5, kInk);
        dline(cursor_x + 1, box_y0 + 5, cursor_x + 1, box_y1 - 5, kInk);
    }

    dtext(4, box_y1 + 12, kBlue, "Hint");
    draw_limited_text(42, box_y1 + 12, kInk, help ? help : "", 36);
    dtext(4, box_y1 + 30, kInk, mode_help(mode));
    dtext(4, box_y1 + 46, kInk, "EXE ok  DEL back  AC clear");
    draw_mode_softkeys(mode);
    dupdate();
}

static const char *alpha_key_to_text(int key)
{
    if(key == KEY_XOT) return "x";
    if(key == KEY_LOG) return "a";
    if(key == KEY_LN) return "b";
    if(key == KEY_SIN) return "c";
    if(key == KEY_COS) return "d";
    if(key == KEY_TAN) return "e";
    if(key == KEY_FRAC) return "f";
    if(key == KEY_FD) return "g";
    if(key == KEY_LEFTP) return "h";
    if(key == KEY_RIGHTP) return "i";
    if(key == KEY_COMMA) return "j";
    if(key == KEY_7) return "k";
    if(key == KEY_8) return "l";
    if(key == KEY_9) return "m";
    if(key == KEY_4) return "n";
    if(key == KEY_5) return "o";
    if(key == KEY_6) return "p";
    if(key == KEY_1) return "q";
    if(key == KEY_2) return "r";
    if(key == KEY_3) return "s";
    if(key == KEY_0) return "t";
    if(key == KEY_DOT) return "u";
    if(key == KEY_EXP) return "v";
    if(key == KEY_NEG) return "w";
    return nullptr;
}

static const char *shift_key_to_text(int key)
{
    if(key == KEY_SQUARE) return "sqrt(";
    if(key == KEY_POWER) return "^(1/";
    if(key == KEY_LOG) return "10^(";
    if(key == KEY_LN) return "exp(";
    if(key == KEY_SIN) return "asin(";
    if(key == KEY_COS) return "acos(";
    if(key == KEY_TAN) return "atan(";
    if(key == KEY_EXP) return "pi";
    if(key == KEY_FRAC) return "abs(";
    if(key == KEY_FD) return "pi";
    if(key == KEY_F1) return "sqrt(";
    if(key == KEY_F2) return "10^(";
    if(key == KEY_F3) return "asin(";
    if(key == KEY_F4) return "acos(";
    if(key == KEY_F5) return "atan(";
    if(key == KEY_F6) return "exp(";
    return nullptr;
}

static const char *plain_key_to_text(int key)
{
    if(key == KEY_0) return "0";
    if(key == KEY_1) return "1";
    if(key == KEY_2) return "2";
    if(key == KEY_3) return "3";
    if(key == KEY_4) return "4";
    if(key == KEY_5) return "5";
    if(key == KEY_6) return "6";
    if(key == KEY_7) return "7";
    if(key == KEY_8) return "8";
    if(key == KEY_9) return "9";
    if(key == KEY_PLUS) return "+";
    if(key == KEY_MINUS || key == KEY_NEG) return "-";
    if(key == KEY_MUL) return "*";
    if(key == KEY_DIV) return "/";
    if(key == KEY_DOT) return ".";
    if(key == KEY_COMMA) return ",";
    if(key == KEY_LEFTP) return "(";
    if(key == KEY_RIGHTP) return ")";
    if(key == KEY_EQUALS || key == KEY_ARROW) return "=";
    if(key == KEY_X || key == KEY_XOT) return "x";
    if(key == KEY_Y) return "y";
    if(key == KEY_Z) return "z";
    if(key == KEY_SQUARE) return "^2";
    if(key == KEY_POWER) return "^";
    if(key == KEY_SIN) return "sin(";
    if(key == KEY_COS) return "cos(";
    if(key == KEY_TAN) return "tan(";
    if(key == KEY_LOG) return "log(";
    if(key == KEY_LN) return "ln(";
    if(key == KEY_FRAC) return "/";
    if(key == KEY_EXP) return "e";
    if(key == KEY_F1) return "sqrt(";
    if(key == KEY_F2) return "log(";
    if(key == KEY_F3) return "sin(";
    if(key == KEY_F4) return "cos(";
    if(key == KEY_F5) return "pi";
    if(key == KEY_F6) return "=";
    return nullptr;
}

static const char *event_to_text(int key, EditorMode mode)
{
    if(mode == EditorMode::Alpha || mode == EditorMode::AlphaLock) {
        const char *text = alpha_key_to_text(key);
        if(text != nullptr) return text;
    }
    if(mode == EditorMode::Shift) {
        const char *text = shift_key_to_text(key);
        if(text != nullptr) return text;
    }
    return plain_key_to_text(key);
}

static void insert_text(char *buffer, int capacity, int &len, int &cursor, const char *text)
{
    if(text == nullptr) return;
    for(int i = 0; text[i] != '\0'; i++) {
        if(len + 1 >= capacity) break;
        for(int j = len; j >= cursor; j--) buffer[j + 1] = buffer[j];
        buffer[cursor] = text[i];
        len++;
        cursor++;
    }
}

} // namespace

bool text_input(char *buffer, int capacity, const char *title, const char *help)
{
    if(buffer == nullptr || capacity <= 0) return false;
    int len = text_len(buffer);
    if(len >= capacity) {
        len = capacity - 1;
        buffer[len] = '\0';
    }
    int cursor = len;
    bool caret_on = true;
    EditorMode mode = EditorMode::Normal;

    keydev_t *keyboard = keydev_std();
    keydev_transform_t previous_transform = keydev_transform(keyboard);
    keydev_transform_t editor_transform = previous_transform;
    editor_transform.enabled = KEYDEV_TR_REPEATS | KEYDEV_TR_DELETE_RELEASES;
    keydev_set_transform(keyboard, editor_transform);

    draw_input(title, help, buffer, cursor, caret_on, mode);
    while(true) {
        volatile int timeout = 0;
        int timer = timer_configure(TIMER_ANY, 450000, GINT_CALL_SET_STOP(&timeout));
        if(timer >= 0) timer_start(timer);
        key_event_t ev = keydev_read(keyboard, true, &timeout);
        if(timer >= 0) timer_stop(timer);

        if(timeout && ev.type == KEYEV_NONE) {
            caret_on = !caret_on;
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }
        if(ev.type == KEYEV_HOLD && ev.key != KEY_LEFT && ev.key != KEY_RIGHT) continue;
        if(ev.type != KEYEV_DOWN && ev.type != KEYEV_HOLD) continue;
        caret_on = true;

        if(ev.key == KEY_EXIT) {
            keydev_set_transform(keyboard, previous_transform);
            return false;
        }
        if(ev.key == KEY_EXE) {
            keydev_set_transform(keyboard, previous_transform);
            return true;
        }
        if(ev.key == KEY_MENU && mode == EditorMode::Normal) {
            gint_osmenu();
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }
        if(ev.key == KEY_SHIFT) {
            if(mode == EditorMode::Alpha || mode == EditorMode::AlphaLock) {
                mode = EditorMode::AlphaLock;
            }
            else {
                mode = EditorMode::Shift;
            }
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }
        if(ev.key == KEY_ALPHA) {
            if(mode == EditorMode::Shift) {
                mode = EditorMode::AlphaLock;
            }
            else if(mode == EditorMode::AlphaLock) {
                mode = EditorMode::Normal;
            }
            else {
                mode = EditorMode::Alpha;
            }
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }

        if(ev.key == KEY_LEFT) {
            if(cursor > 0) cursor--;
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }

        if(ev.key == KEY_RIGHT) {
            if(cursor < len) cursor++;
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }

        if(ev.key == KEY_DEL) {
            if(cursor > 0 && len > 0) {
                for(int i = cursor - 1; i < len; i++) buffer[i] = buffer[i + 1];
                len--;
                cursor--;
                draw_input(title, help, buffer, cursor, caret_on, mode);
            }
            continue;
        }

        if(ev.key == KEY_ACON) {
            if(mode == EditorMode::Shift) {
                mode = EditorMode::Normal;
                gint_poweroff(true);
                draw_input(title, help, buffer, cursor, caret_on, mode);
                continue;
            }
            len = 0;
            cursor = 0;
            buffer[0] = '\0';
            draw_input(title, help, buffer, cursor, caret_on, mode);
            continue;
        }

        const char *text = event_to_text(ev.key, mode);
        bool one_shot_mode = (mode == EditorMode::Shift || mode == EditorMode::Alpha);
        if(one_shot_mode) mode = EditorMode::Normal;
        if(text != nullptr) {
            insert_text(buffer, capacity, len, cursor, text);
            draw_input(title, help, buffer, cursor, caret_on, mode);
        }
        else if(one_shot_mode) {
            draw_input(title, help, buffer, cursor, caret_on, mode);
        }
    }
}

} // namespace casio::ui

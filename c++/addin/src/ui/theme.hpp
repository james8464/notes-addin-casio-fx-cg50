#pragma once

#include "device/fixed_string.hpp"

#include <gint/display.h>

namespace casio::ui
{

constexpr int kStatusH = 24;
constexpr int kSoftKeyH = 24;
constexpr int kContentTop = kStatusH + 4;
constexpr int kContentBottom = DHEIGHT - kSoftKeyH - 3;
constexpr int kRowH = 16;

constexpr color_t kInk = C_BLACK;
constexpr color_t kPaper = C_WHITE;
constexpr color_t kBlue = C_RGB(3, 18, 31);
constexpr color_t kSoftBlue = C_RGB(22, 48, 31);
constexpr color_t kLine = C_RGB(10, 24, 18);
constexpr color_t kPale = C_RGB(29, 60, 31);

inline int clamp_int(int value, int lo, int hi)
{
    if(value < lo) return lo;
    if(value > hi) return hi;
    return value;
}

inline int text_width_approx(const char *text)
{
    int w = 0;
    int h = 0;
    dsize(text ? text : "", dfont_default(), &w, &h);
    return w;
}

inline void use_native_font()
{
    dfont(dfont_default());
}

inline struct dwindow full_window()
{
    return {0, 0, DWIDTH, DHEIGHT};
}

inline struct dwindow app_window()
{
    return {0, kStatusH, DWIDTH, DHEIGHT};
}

inline void draw_limited_text(int x, int y, color_t color, const char *text, int max_chars)
{
    use_native_font();
    casio::device::FixedString<96> out;
    if(text != nullptr) {
        int i = 0;
        while(text[i] != '\0' && i < max_chars) {
            out.append_char(text[i]);
            i++;
        }
        if(text[i] != '\0' && max_chars > 1) {
            out.clear();
            for(int j = 0; j < max_chars - 1 && text[j] != '\0'; j++) out.append_char(text[j]);
            out.append_char('~');
        }
    }
    dtext(x, y, color, out.c_str());
}

inline void draw_status(const char *title, const char *mode)
{
    struct dwindow previous_window = dwindow_set(full_window());
    use_native_font();
    drect(0, 0, DWIDTH - 1, kStatusH - 1, kPaper);
    dtext(3, 5, kInk, title ? title : "");
    dtext(155, 5, kInk, "RAD");
    if(mode != nullptr && mode[0] != '\0') {
        int x = DWIDTH - 2 - text_width_approx(mode);
        if(x < 210) x = 210;
        dtext(x, 5, kInk, mode);
    }
    dline(0, kStatusH - 1, DWIDTH - 1, kStatusH - 1, kInk);
    dwindow_set(previous_window);
}

inline void draw_frame(const char *title, const char *mode)
{
    dwindow_set(full_window());
    use_native_font();
    drect(0, kStatusH, DWIDTH - 1, DHEIGHT - 1, kPaper);
    draw_status(title, mode);
    dline(0, kContentBottom, DWIDTH - 1, kContentBottom, kLine);
    dwindow_set(app_window());
}

inline void draw_section_label(int x, int y, const char *label)
{
    use_native_font();
    dtext(x, y, kBlue, label ? label : "");
    dline(x, y + 13, DWIDTH - 10, y + 13, kSoftBlue);
}

inline void draw_softkeys(
    const char *k1,
    const char *k2,
    const char *k3,
    const char *k4,
    const char *k5,
    const char *k6)
{
    use_native_font();
    const char *labels[6] = {k1, k2, k3, k4, k5, k6};
    int y0 = DHEIGHT - kSoftKeyH;
    int w = DWIDTH / 6;
    drect(0, y0, DWIDTH - 1, DHEIGHT - 1, kInk);
    for(int i = 0; i < 6; i++) {
        int x0 = i * w;
        int x1 = (i == 5) ? DWIDTH - 1 : ((i + 1) * w - 1);
        dline(x0, y0, x0, DHEIGHT - 1, kPaper);
        const char *label = labels[i] ? labels[i] : "";
        int tx = x0 + ((x1 - x0 + 1) - text_width_approx(label)) / 2;
        tx = clamp_int(tx, x0 + 3, x1 - 8);
        dtext(tx, y0 + 7, kPaper, label);
    }
}

inline void draw_scrollbar(int top, int visible_rows, int total_rows)
{
    int x0 = DWIDTH - 7;
    int y0 = kContentTop;
    int y1 = kContentBottom - 3;
    drect(x0, y0, DWIDTH - 2, y1, kPaper);
    drect(x0 + 1, y0, DWIDTH - 3, y1, C_RGB(25, 55, 31));
    if(total_rows <= visible_rows || total_rows <= 0) {
        drect(x0 + 1, y0, DWIDTH - 3, y1, kBlue);
        return;
    }

    int track_h = y1 - y0 + 1;
    int thumb_h = (track_h * visible_rows) / total_rows;
    if(thumb_h < 12) thumb_h = 12;
    int max_top = total_rows - visible_rows;
    int thumb_y = y0 + ((track_h - thumb_h) * top) / (max_top > 0 ? max_top : 1);
    drect(x0 + 1, thumb_y, DWIDTH - 3, thumb_y + thumb_h, kBlue);
}

inline void draw_list_row(int y, const char *label, bool selected)
{
    if(selected) {
        drect(0, y - 1, DWIDTH - 9, y + kRowH - 2, kInk);
        draw_limited_text(4, y + 1, kPaper, label, 34);
    }
    else {
        drect(0, y - 1, DWIDTH - 9, y + kRowH - 2, kPaper);
        draw_limited_text(4, y + 1, kInk, label, 34);
    }
}

inline int visible_rows()
{
    return (kContentBottom - kContentTop - 2) / kRowH;
}

} // namespace casio::ui

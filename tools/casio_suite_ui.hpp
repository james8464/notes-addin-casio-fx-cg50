#ifndef CASIO_SUITE_UI_HPP
#define CASIO_SUITE_UI_HPP

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <string.h>

extern "C" {
#include <fxcg/rtc.h>
}

#define RGB565(r, g, b) (unsigned short)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

extern "C" void DirectDrawRectangle(int x1, int y1, int x2, int y2, int color);

static const unsigned short UI_WHITE = COLOR_WHITE;
static const unsigned short UI_BLACK = COLOR_BLACK;
static const unsigned short UI_PINK = 0xF81F;
static const unsigned short UI_BLUE = COLOR_BLUE;
static const unsigned short UI_LIGHT = RGB565(242, 243, 245);
static const unsigned short UI_GRAY = RGB565(210, 213, 218);
static const unsigned short UI_FRAME = RGB565(74, 74, 82);
static const unsigned UI_R_BLINK_PERIOD = 384;
static const unsigned UI_R_VISIBLE_TICKS = 256;

static void ui_pixel(int x, int y, unsigned short color) {
  if (x < 0 || x >= LCD_WIDTH_PX || y < 0 || y >= LCD_HEIGHT_PX) return;
  Bdisp_SetPoint_VRAM(x, y, color);
}

static void ui_fill(int x, int y, int w, int h, unsigned short color) {
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx)
      ui_pixel(xx, yy, color);
}

static void ui_hline(int x1, int x2, int y, unsigned short color) {
  for (int x = x1; x <= x2; ++x) ui_pixel(x, y, color);
}

static void ui_border() {
  DirectDrawRectangle(0, 0, 5, 223, UI_PINK);
  DirectDrawRectangle(390, 0, 395, 223, UI_PINK);
  DirectDrawRectangle(0, 217, 395, 223, UI_PINK);
}

static void ui_flush() {
  Bdisp_PutDisp_DD();
}

static void ui_status(bool r_visible) {
  DefineStatusAreaFlags(3,
                        SAF_BATTERY | SAF_SETUP_INPUT_OUTPUT | SAF_SETUP_FRAC_RESULT |
                            SAF_SETUP_ANGLE | SAF_SETUP_COMPLEX_MODE | SAF_SETUP_DISPLAY,
                        0, 0);
  EnableStatusArea(2);
  DisplayStatusArea();
  if (r_visible) {
    ui_fill(339, 0, 21, 23, UI_BLUE);
    PrintCXY(342, 1, "R", 0x40, -1, UI_WHITE, UI_BLUE, 1, 0);
  } else {
    ui_fill(339, 0, 21, 23, UI_WHITE);
  }
  ui_fill(339, 23, 21, 2, UI_WHITE);
  ui_hline(339, 359, 23, UI_BLACK);
}

static bool ui_r_visible(unsigned start_tick) {
  unsigned phase = ((unsigned)RTC_GetTicks() - start_tick) % UI_R_BLINK_PERIOD;
  return phase < UI_R_VISIBLE_TICKS;
}

static int ui_key_poll() {
  int key = 0;
  GetKey(&key);
  return key;
}

static bool ui_menu_handle_key(int key, int count, int visible, int *sel, int *top) {
  if (count <= 0) return false;
  switch (key) {
    case KEY_CTRL_DOWN:
      if (*sel + 1 >= count) {
        *sel = 0;
        *top = 0;
      } else {
        ++(*sel);
        if (*sel >= *top + visible) *top = *sel - visible + 1;
      }
      return true;
    case KEY_CTRL_UP:
      if (*sel <= 0) {
        *sel = count - 1;
        *top = count > visible ? count - visible : 0;
      } else {
        --(*sel);
        if (*sel < *top) *top = *sel;
      }
      return true;
    case KEY_CTRL_PAGEDOWN:
      *sel += 6;
      if (*sel >= count) *sel = count - 1;
      if (*sel >= *top + visible) *top = *sel - visible + 1;
      return true;
    case KEY_CTRL_PAGEUP:
      *sel -= 6;
      if (*sel < 0) *sel = 0;
      if (*sel < *top) *top = *sel;
      return true;
    case KEY_CHAR_1: case KEY_CHAR_2: case KEY_CHAR_3:
    case KEY_CHAR_4: case KEY_CHAR_5: case KEY_CHAR_6:
    case KEY_CHAR_7: case KEY_CHAR_8: case KEY_CHAR_9: {
      int n = key - KEY_CHAR_0;
      if (n <= count) {
        *sel = n - 1;
        if (*sel < *top) *top = *sel;
        if (*sel >= *top + visible) *top = *sel - visible + 1;
        return true;
      }
      break;
    }
    case KEY_CHAR_0:
      if (count >= 10) {
        *sel = 9;
        if (*sel >= *top + visible) *top = *sel - visible + 1;
        return true;
      }
      break;
  }
  return false;
}

static void ui_fkey(int slot, int id) {
  int ptr = 0;
  GetFKeyPtr(id, &ptr);
  FKey_Display(slot, (int *)ptr);
}

static void ui_label_fkey(int slot, const char *text) {
  if (!text || !text[0]) return;
  char label[7];
  int n = (int)strlen(text);
  if (n > 6) n = 6;
  for (int i = 0; i < n; ++i) label[i] = text[i];
  label[n] = 0;
  ui_fkey(slot, 0x38);
  ui_fill(slot * 64 + 4, 200, 56, 12, UI_BLACK);
  int x = slot * 64 + 32 - n * 3;
  if (x < slot * 64 + 4) x = slot * 64 + 4;
  int y = 177;
  PrintMini(&x, &y, (unsigned char *)label, 0, 0xffffffff, 0, 0, UI_WHITE, UI_BLACK, 1, 0);
}

static void ui_softkeys(const char *f1, const char *f2, const char *f3, const char *f4, const char *f5, const char *f6) {
  const char *v[6] = {f1, f2, f3, f4, f5, f6};
  for (int i = 0; i < 6; ++i) {
    if (v[i] && v[i][0]) ui_label_fkey(i, v[i]);
  }
}

static void ui_chrome(const char *title, bool r_visible) {
  Bdisp_AllClr_VRAM();
  ui_fill(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, UI_WHITE);
  ui_status(r_visible);
  Bdisp_MMPrint(18, 24, title, 0x40, 0xffffffff, 0, 0, UI_BLUE, UI_WHITE, 1, 0);
  ui_hline(18, 377, 47, UI_BLACK);
}

static void ui_print(int x, int y, const char *s) {
  Bdisp_MMPrint(x, y, s, 0x40, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 1, 0);
}

static void ui_menu_keys(const char *title, const char *const *items, int count, int top, int sel, bool r_visible,
                         const char *f1, const char *f2, const char *f3, const char *f4, const char *f5, const char *f6) {
  ui_chrome(title, r_visible);
  ui_softkeys(f1, f2, f3, f4, f5, f6);
  for (int i = 0; i < 7 && top + i < count; ++i) {
    int idx = top + i;
    int y = 54 + i * 20;
    char line[72];
    if (count < 10) {
      line[0] = (char)('1' + idx);
      line[1] = ' ';
      line[2] = 0;
    } else {
      line[0] = idx + 1 >= 10 ? (char)('0' + ((idx + 1) / 10)) : ' ';
      line[1] = (char)('0' + ((idx + 1) % 10));
      line[2] = ' ';
      line[3] = 0;
    }
    strncat(line, items[idx], sizeof(line) - strlen(line) - 1);
    if (idx == sel) {
      ui_fill(18, y - 2, 342, 18, UI_BLUE);
      Bdisp_MMPrint(22, y, line, 0x40, 0xffffffff, 0, 0, UI_WHITE, UI_BLUE, 1, 0);
    } else {
      Bdisp_MMPrint(22, y, line, 0x40, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 1, 0);
    }
  }
  if (count > 7) {
    int bar_top = 54;
    int bar_h = 138;
    int thumb_h = (7 * bar_h) / count;
    if (thumb_h < 12) thumb_h = 12;
    int max_top = count - 7;
    int thumb_y = bar_top + (max_top > 0 ? (top * (bar_h - thumb_h)) / max_top : 0);
    ui_fill(372, bar_top, 5, bar_h, UI_GRAY);
    ui_fill(372, thumb_y, 5, thumb_h, UI_BLUE);
  }
  ui_flush();
}

static void ui_menu(const char *title, const char *const *items, int count, int top, int sel, bool r_visible) {
  ui_menu_keys(title, items, count, top, sel, r_visible, "OPEN", "", "", "", "", "BACK");
}

static void ui_page(const char *title, const char *const *lines, int count, int top, bool r_visible) {
  ui_chrome(title, r_visible);
  ui_softkeys("", "BACK", "UP", "DOWN", "", "");
  for (int i = 0; i < 8 && top + i < count; ++i)
    ui_print(14, 55 + i * 17, lines[top + i]);
  ui_flush();
}

static void ui_wait_page(const char *title, const char *const *lines, int count, unsigned *tick) {
  int top = 0;
  bool rv = ui_r_visible(*tick);
  ui_page(title, lines, count, top, rv);
  for (;;) {
    int key = ui_key_poll();
    bool nr = ui_r_visible(*tick);
    if (nr != rv) {
      rv = nr;
      ui_page(title, lines, count, top, rv);
    }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6) return;
    if (key == KEY_CTRL_UP && top > 0) ui_page(title, lines, count, --top, rv);
    if (key == KEY_CTRL_DOWN && top + 8 < count) ui_page(title, lines, count, ++top, rv);
    OS_InnerWait_ms(35);
  }
}

static int ui_key_char(int key) {
  if (key >= 32 && key <= 126) return key;
  if (key == KEY_CHAR_PLUS) return '+';
  if (key == KEY_CHAR_MINUS || key == KEY_CHAR_PMINUS) return '-';
  if (key == KEY_CHAR_MULT) return '*';
  if (key == KEY_CHAR_DIV) return '/';
  if (key == KEY_CHAR_LPAR) return '(';
  if (key == KEY_CHAR_RPAR) return ')';
  if (key == KEY_CHAR_COMMA) return ',';
  if (key == KEY_CHAR_EQUAL) return '=';
  if (key == KEY_CHAR_STORE) return '>';
  return 0;
}

static bool ui_input(const char *title, char *buf, int cap, unsigned *tick) {
  int len = (int)strlen(buf);
  static char hist[6][96];
  static int hist_count = 0;
  int hist_pos = hist_count;
  bool rv = ui_r_visible(*tick);
  for (;;) {
    Bdisp_AllClr_VRAM();
    ui_fill(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, UI_WHITE);
    ui_status(rv);
    Bdisp_MMPrint(10, 28, title, 0x40, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 1, 0);
    DirectDrawRectangle(8, 47, 387, 49, RGB565(70, 70, 80));
    ui_print(14, 58, "Type command, EXE runs.");
    ui_fill(12, 82, 370, 38, RGB565(245, 245, 245));
    Bdisp_MMPrint(16, 90, buf, 0x40, 0xffffffff, 0, 0, UI_BLACK, RGB565(245, 245, 245), 1, 0);
    ui_softkeys("RUN", "BACK", "DEL", "PREV", "NEXT", "");
    ui_flush();
    int key = ui_key_poll();
    bool nr = ui_r_visible(*tick);
    if (nr != rv) { rv = nr; continue; }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F2 || key == KEY_CTRL_F6) return false;
    if (key == KEY_CTRL_EXE || key == KEY_CTRL_F1) {
      if (buf[0] && (hist_count == 0 || strcmp(hist[(hist_count - 1) % 6], buf))) {
        strncpy(hist[hist_count % 6], buf, 95);
        hist[hist_count % 6][95] = 0;
        ++hist_count;
      }
      return true;
    }
    if ((key == KEY_CTRL_DEL || key == KEY_CTRL_F3) && len > 0) buf[--len] = 0;
    if ((key == KEY_CTRL_UP || key == KEY_CTRL_F4) && hist_count > 0) {
      int hist_min = hist_count > 6 ? hist_count - 6 : 0;
      if (hist_pos > hist_min) --hist_pos;
      strncpy(buf, hist[hist_pos % 6], cap - 1);
      buf[cap - 1] = 0;
      len = (int)strlen(buf);
    }
    if ((key == KEY_CTRL_DOWN || key == KEY_CTRL_F5) && hist_count > 0) {
      if (hist_pos + 1 < hist_count) ++hist_pos;
      strncpy(buf, hist[hist_pos % 6], cap - 1);
      buf[cap - 1] = 0;
      len = (int)strlen(buf);
    }
    int ch = ui_key_char(key);
    if (ch && len + 1 < cap) { buf[len++] = (char)ch; buf[len] = 0; }
    OS_InnerWait_ms(35);
  }
}

#endif

#ifndef CASIO_SUITE_UI_HPP
#define CASIO_SUITE_UI_HPP

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>

extern "C" {
#include <fxcg/rtc.h>
}

#define RGB565(r, g, b) (unsigned short)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

extern "C" void DirectDrawRectangle(int x1, int y1, int x2, int y2, int color);

static const unsigned short UI_WHITE = COLOR_WHITE;
static const unsigned short UI_BLACK = COLOR_BLACK;
static const unsigned short UI_PINK = 0xF81F;
static const unsigned short UI_BLUE = COLOR_BLUE;

static void ui_pixel(int x, int y, unsigned short color) {
  if (x < 0 || x >= LCD_WIDTH_PX || y < 0 || y >= LCD_HEIGHT_PX) return;
  Bdisp_SetPoint_VRAM(x, y, color);
}

static void ui_fill(int x, int y, int w, int h, unsigned short color) {
  for (int yy = y; yy < y + h; ++yy)
    for (int xx = x; xx < x + w; ++xx)
      ui_pixel(xx, yy, color);
}

static void ui_border() {
  DirectDrawRectangle(0, 0, 5, 223, UI_PINK);
  DirectDrawRectangle(390, 0, 395, 223, UI_PINK);
  DirectDrawRectangle(0, 217, 395, 223, UI_PINK);
}

static void ui_flush() {
  Bdisp_PutDisp_DD();
  ui_border();
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
}

static bool ui_r_visible(unsigned start_tick) {
  unsigned phase = ((unsigned)RTC_GetTicks() - start_tick) % 384;
  return phase < 256;
}

static int ui_key_poll() {
  int col = 0, row = 0;
  unsigned short keycode = 0;
  int ret = GetKeyWait_OS(&col, &row, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &keycode);
  if (ret != KEYREP_KEYEVENT) return KEY_CTRL_NOP;
  if (col == 1) return KEY_CTRL_AC;
  if (col == 4 && row == 9) return KEY_CTRL_MENU;
  if (col == 4 && row == 8) return KEY_CTRL_EXIT;
  return keycode;
}

static void ui_fkey(int slot, int id) {
  int ptr = 0;
  GetFKeyPtr(id, &ptr);
  FKey_Display(slot, (int *)ptr);
}

static void ui_text_fkey(int slot, const char *text) {
  ui_fkey(slot, 0x38);
  ui_fill(slot * 64 + 2, 198, 60, 16, UI_BLACK);
  Bdisp_MMPrint(slot * 64 + 8, 196, text, 0x40, 0xffffffff, 0, 0, UI_WHITE, UI_BLACK, 1, 0);
}

static void ui_chrome(const char *title, bool r_visible) {
  Bdisp_AllClr_VRAM();
  ui_fill(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, UI_WHITE);
  ui_status(r_visible);
  Bdisp_MMPrint(10, 28, title, 0x40, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 1, 0);
  DirectDrawRectangle(8, 47, 387, 49, RGB565(70, 70, 80));
  ui_text_fkey(0, "OPEN");
  ui_text_fkey(1, "BACK");
  ui_text_fkey(2, "UP");
  ui_text_fkey(3, "DOWN");
}

static void ui_print(int x, int y, const char *s) {
  Bdisp_MMPrint(x, y, s, 0x40, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 1, 0);
}

static void ui_menu(const char *title, const char *const *items, int count, int top, int sel, bool r_visible) {
  ui_chrome(title, r_visible);
  for (int i = 0; i < 7 && top + i < count; ++i) {
    int y = 56 + i * 18;
    if (top + i == sel) {
      ui_fill(11, y - 2, 370, 17, UI_BLUE);
      Bdisp_MMPrint(16, y, items[top + i], 0x40, 0xffffffff, 0, 0, UI_WHITE, UI_BLUE, 1, 0);
    } else {
      ui_print(16, y, items[top + i]);
    }
  }
  ui_flush();
}

static void ui_page(const char *title, const char *const *lines, int count, int top, bool r_visible) {
  ui_chrome(title, r_visible);
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
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC) return;
    if (key == KEY_CTRL_UP && top > 0) ui_page(title, lines, count, --top, rv);
    if (key == KEY_CTRL_DOWN && top + 8 < count) ui_page(title, lines, count, ++top, rv);
    OS_InnerWait_ms(35);
  }
}

#endif

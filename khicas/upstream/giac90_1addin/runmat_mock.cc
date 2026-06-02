#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>

extern "C" {
#include <fxcg/rtc.h>
}

#define RGB565(r, g, b) (unsigned short)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

extern "C" void DirectDrawRectangle(int x1, int y1, int x2, int y2, int color);

static const unsigned short kWhite = COLOR_WHITE;
static const unsigned short kBlack = COLOR_BLACK;
static const unsigned short kFrame = RGB565(74, 74, 82);
static const unsigned short kLightBlue = RGB565(30, 190, 255);
static const unsigned kRBlinkPeriodTicks = 384;
static const unsigned kRVisibleTicks = 256;

static void pixel(int x, int y, unsigned short color) {
  if (x < 0 || x >= LCD_WIDTH_PX || y < 0 || y >= LCD_HEIGHT_PX) return;
  Bdisp_SetPoint_VRAM(x, y, color);
}

static void fill_rect(int x, int y, int w, int h, unsigned short color) {
  for (int yy = y; yy < y + h; ++yy) {
    for (int xx = x; xx < x + w; ++xx) {
      pixel(xx, yy, color);
    }
  }
}

static void hline(int x1, int x2, int y, unsigned short color) {
  for (int x = x1; x <= x2; ++x) pixel(x, y, color);
}

static void vline(int x, int y1, int y2, unsigned short color) {
  for (int y = y1; y <= y2; ++y) pixel(x, y, color);
}

static void rect_outline(int x, int y, int w, int h, unsigned short color) {
  hline(x, x + w - 1, y, color);
  hline(x, x + w - 1, y + h - 1, color);
  vline(x, y, y + h - 1, color);
  vline(x + w - 1, y, y + h - 1, color);
}

static void drawCasioCasBorder() {
  const unsigned short kCasioCasPink = 0xF81F;
  DirectDrawRectangle(0, 0, 5, 223, kCasioCasPink);
  DirectDrawRectangle(390, 0, 395, 223, kCasioCasPink);
  DirectDrawRectangle(0, 217, 395, 223, kCasioCasPink);
}

static void flush_screen() {
  Bdisp_PutDisp_DD();
  drawCasioCasBorder();
}

static void draw_status_area() {
  DefineStatusAreaFlags(3,
                        SAF_BATTERY | SAF_SETUP_INPUT_OUTPUT | SAF_SETUP_FRAC_RESULT |
                            SAF_SETUP_ANGLE | SAF_SETUP_COMPLEX_MODE | SAF_SETUP_DISPLAY,
                        0, 0);
  EnableStatusArea(2);
  DisplayStatusArea();
}

static void draw_r_indicator(bool visible) {
  fill_rect(339, 0, 21, 24, visible ? COLOR_BLUE : kWhite);
  if (visible) {
    Bdisp_MMPrint(342, 0, "R", 0x40, 0xffffffff, 0, 0, COLOR_WHITE, COLOR_BLUE, 1, 0);
  }
}

static bool r_is_visible(unsigned start_tick) {
  unsigned phase = ((unsigned)RTC_GetTicks() - start_tick) % kRBlinkPeriodTicks;
  return phase < kRVisibleTicks;
}

static void display_fkey(int slot, int id) {
  if (id < 0) return;
  int ptr = 0;
  GetFKeyPtr(id, &ptr);
  FKey_Display(slot, (int *)ptr);
}

static void draw_custom_fkey_text(int slot, const char *text) {
  int x = slot == 2 ? 128 : 203;
  int w = slot == 2 ? 74 : 56;
  fill_rect(x, 192, w, 18, COLOR_BLACK);
  Bdisp_MMPrint(x + 4, 196, text, 0x40, 0xffffffff, 0, 0, COLOR_WHITE, COLOR_BLACK, 1, 0);
}

static void draw_soft_labels() {
  const int kFKeyJump = 508;
  const int kFKeyDelete = 0x38;
  display_fkey(0, kFKeyJump);
  display_fkey(1, kFKeyDelete);
  display_fkey(4, -1);
  display_fkey(5, -1);
  draw_custom_fkey_text(2, "MAT/VCT");
  draw_custom_fkey_text(3, "MATH");
}

static void draw_static_screen(bool r_visible) {
  Bdisp_AllClr_VRAM();
  fill_rect(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, kWhite);

  draw_status_area();
  hline(6, 389, 24, kBlack);
  draw_r_indicator(r_visible);

  rect_outline(13, 31, 14, 17, kBlack);
  fill_rect(13, 31, 2, 17, kFrame);

  fill_rect(369, 32, 5, 136, kLightBlue);
  rect_outline(368, 31, 7, 138, kFrame);
  fill_rect(371, 32, 2, 136, COLOR_BLUE);

  draw_soft_labels();
}

static int key_poll() {
  int col = 0, row = 0;
  unsigned short keycode = 0;
  int ret = GetKeyWait_OS(&col, &row, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &keycode);
  if (ret != KEYREP_KEYEVENT) return KEY_CTRL_NOP;
  if (col == 1) return KEY_CTRL_AC;
  if (col == 4 && row == 9) return KEY_CTRL_MENU;
  if (col == 4 && row == 8) return KEY_CTRL_EXIT;
  return keycode;
}

int main() {
  Bdisp_EnableColor(1);
  unsigned r_start_tick = (unsigned)RTC_GetTicks();
  bool r_visible = true;

  draw_static_screen(r_visible);
  flush_screen();

  for (;;) {
    int key = key_poll();
    bool next_r_visible = r_is_visible(r_start_tick);
    if (next_r_visible != r_visible) {
      r_visible = next_r_visible;
      draw_r_indicator(r_visible);
      flush_screen();
    }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_MENU) return 0;
    OS_InnerWait_ms(40);
  }
}

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>

#define RGB565(r, g, b) (unsigned short)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

static const unsigned short kWhite = 0xffff;
static const unsigned short kBlack = 0x0000;
static const unsigned short kPink = 0xf81f;
static const unsigned short kBlue = 0x001f;
static const unsigned short kLightBlue = RGB565(30, 190, 255);
static const unsigned short kFrame = RGB565(74, 74, 82);
static const unsigned short kSoft = RGB565(62, 67, 82);
static const int kRBlinkPeriodTicks = 12;
static const int kRVisibleTicks = 8;
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

static int text_width_mm(const char *label) {
  int n = 0;
  while (label[n]) ++n;
  return n * 8;
}

static void print_mm(int x, int y, const char *label, int fg, int bg) {
  Bdisp_MMPrint(x, y, label, 0, 0xffffffff, 0, 0, fg, bg, 1, 0);
}

static void print_mini(int x, int y, const char *label, int fg, int bg) {
  int tx = x, ty = y;
  PrintMini(&tx, &ty, (unsigned char *)label, 0, 0xffffffff, 0, 0, fg, bg, 1, 0);
}

static void status_box(int x, const char *label) {
  int width = text_width_mm(label) + 6;
  fill_rect(x, 5, width, 15, kWhite);
  rect_outline(x, 5, width, 15, kFrame);
  print_mm(x + 3, 6, label, COLOR_BLACK, COLOR_WHITE);
}

static void draw_battery() {
  rect_outline(14, 5, 9, 14, kFrame);
  rect_outline(16, 2, 5, 4, kFrame);
  fill_rect(15, 15, 5, 2, kBlue);
  fill_rect(15, 12, 5, 2, kLightBlue);
  fill_rect(15, 9, 5, 2, kLightBlue);
}

static void draw_r_indicator(bool visible) {
  fill_rect(339, 1, 21, 22, visible ? kBlue : kWhite);
  if (!visible) {
    rect_outline(339, 1, 21, 22, kFrame);
    return;
  }
  print_mini(343, 2, "R", COLOR_WHITE, COLOR_BLUE);
}

static void soft_label(int x, int width, const char *label) {
  fill_rect(x, 192, width, 18, kSoft);
  rect_outline(x, 192, width, 18, kSoft);
  print_mini(x + 3, 193, label, COLOR_WHITE, COLOR_BLACK);
}

static void draw_soft_labels() {
  soft_label(7, 56, "JUMP");
  soft_label(64, 66, "DELETE");
  soft_label(131, 70, "MAT/VCT");
  soft_label(202, 56, "MATH");
}

static void draw_static_screen() {
  fill_rect(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, kWhite);
  fill_rect(0, 0, 7, LCD_HEIGHT_PX, kPink);
  fill_rect(LCD_WIDTH_PX - 7, 0, 7, LCD_HEIGHT_PX, kPink);
  fill_rect(0, LCD_HEIGHT_PX - 7, LCD_WIDTH_PX, 7, kPink);

  fill_rect(7, 0, LCD_WIDTH_PX - 14, LCD_HEIGHT_PX - 7, kWhite);
  hline(7, LCD_WIDTH_PX - 8, 24, kBlack);
  draw_battery();
  status_box(42, "Math");
  status_box(76, "Deg");
  status_box(107, "Norm1");
  status_box(193, "d/c");
  status_box(226, "Real");
  draw_r_indicator(true);

  rect_outline(13, 31, 14, 17, kBlack);
  fill_rect(13, 31, 2, 17, kFrame);

  fill_rect(369, 32, 5, 136, kLightBlue);
  rect_outline(368, 31, 7, 138, kFrame);
  fill_rect(371, 32, 2, 136, kBlue);

  draw_soft_labels();
}

static int key_wait_250ms() {
  int col = 0, row = 0;
  unsigned short keycode = 0;
  int ret = GetKeyWait_OS(&col, &row, KEYWAIT_HALTON_TIMERON, 25, 1, &keycode);
  if (ret != KEYREP_KEYEVENT) return KEY_CTRL_NOP;
  if (col == 1) return KEY_CTRL_AC;
  if (col == 4 && row == 9) return KEY_CTRL_MENU;
  if (col == 4 && row == 8) return KEY_CTRL_EXIT;
  return keycode;
}

int main() {
  Bdisp_EnableColor(1);
  EnableStatusArea(0);
  draw_static_screen();
  Bdisp_PutDisp_DD();
  int r_ticks = 0;

  for (;;) {
    int key = key_wait_250ms();
    r_ticks = (r_ticks + 1) % kRBlinkPeriodTicks;
    draw_r_indicator(r_ticks < kRVisibleTicks);
    Bdisp_PutDisp_DD();
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_MENU) return 0;
  }
}

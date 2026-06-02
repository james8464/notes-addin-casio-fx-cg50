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

static unsigned char glyph(char c, int row) {
  if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
  const unsigned char *g = 0;
  static const unsigned char A[7]={14,17,17,31,17,17,17}, C[7]={14,17,16,16,16,17,14};
  static const unsigned char D[7]={30,17,17,17,17,17,30}, E[7]={31,16,16,30,16,16,31};
  static const unsigned char G[7]={14,17,16,23,17,17,14}, H[7]={17,17,17,31,17,17,17};
  static const unsigned char J[7]={7,2,2,2,18,18,12}, L[7]={16,16,16,16,16,16,31};
  static const unsigned char M[7]={17,27,21,21,17,17,17}, N[7]={17,25,21,19,17,17,17};
  static const unsigned char O[7]={14,17,17,17,17,17,14}, P[7]={30,17,17,30,16,16,16};
  static const unsigned char R[7]={30,17,17,30,20,18,17}, T[7]={31,4,4,4,4,4,4};
  static const unsigned char U[7]={17,17,17,17,17,17,14}, V[7]={17,17,17,17,10,10,4};
  static const unsigned char Y[7]={17,17,10,4,4,4,4}, Z[7]={31,1,2,4,8,16,31};
  static const unsigned char n0[7]={14,17,19,21,25,17,14}, n1[7]={4,12,4,4,4,4,14};
  static const unsigned char slash[7]={1,2,2,4,8,8,16}, blank[7]={0,0,0,0,0,0,0};
  switch (c) {
    case 'A': g=A; break; case 'C': g=C; break; case 'D': g=D; break; case 'E': g=E; break;
    case 'G': g=G; break; case 'H': g=H; break; case 'J': g=J; break; case 'L': g=L; break;
    case 'M': g=M; break; case 'N': g=N; break; case 'O': g=O; break; case 'P': g=P; break;
    case 'R': g=R; break; case 'T': g=T; break; case 'U': g=U; break; case 'V': g=V; break;
    case 'Y': g=Y; break; case 'Z': g=Z; break; case '0': g=n0; break; case '1': g=n1; break;
    case '/': g=slash; break; default: g=blank; break;
  }
  return g[row];
}

static int pixel_text_width(const char *s, int sx) {
  int n = 0;
  while (s[n]) ++n;
  return n ? n * 5 * sx + (n - 1) * sx : 0;
}

static void draw_pixel_text(int x, int y, const char *s, int sx, int sy, unsigned short fg) {
  for (const char *p = s; *p; ++p) {
    for (int row = 0; row < 7; ++row) {
      unsigned char bits = glyph(*p, row);
      for (int col = 0; col < 5; ++col) {
        if (bits & (1 << (4 - col)))
          fill_rect(x + col * sx, y + row * sy, sx, sy, fg);
      }
    }
    x += 6 * sx;
  }
}

static void status_box(int x, const char *label) {
  int width = pixel_text_width(label, 1) + 7;
  fill_rect(x, 5, width, 17, kWhite);
  rect_outline(x, 5, width, 17, kFrame);
  draw_pixel_text(x + 3, 7, label, 1, 2, kFrame);
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
  draw_pixel_text(344, 4, "R", 2, 2, kWhite);
}

static void soft_label(int x, int width, const char *label) {
  fill_rect(x, 192, width, 18, kSoft);
  rect_outline(x, 192, width, 18, kSoft);
  draw_pixel_text(x + 4, 194, label, 1, 2, kWhite);
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
  status_box(73, "Deg");
  status_box(101, "Norm1");
  status_box(193, "d/c");
  status_box(224, "Real");
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

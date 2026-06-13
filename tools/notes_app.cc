#include "casio_suite_ui.hpp"
#include <fxcg/file.h>
#include <string.h>
#include <stdio.h>

typedef struct {
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
} file_type_t;

struct NoteEntry {
  char name[64];
  char path[280];
  int is_folder;
  int size;
};

static NoteEntry entries[80];
static int entry_count = 0;
static char cwd_path[280] = "\\\\fls0\\";
static char file_buf[8192];
static const char *view_lines[128];
static char line_store[128][48];
static unsigned char bmp_row[1280];

static int lower_char(int c) {
  if (c >= 'A' && c <= 'Z') return c + 32;
  return c;
}

static int contains_ci(const char *s, const char *needle) {
  if (!needle || !needle[0]) return 1;
  for (int i = 0; s[i]; ++i) {
    int j = 0;
    while (needle[j] && s[i + j] && lower_char(s[i + j]) == lower_char(needle[j])) ++j;
    if (!needle[j]) return 1;
  }
  return 0;
}

static int ends_with(const char *s, const char *tail) {
  int n = strlen(s), m = strlen(tail);
  if (n < m) return 0;
  for (int i = 0; i < m; ++i) {
    char a = s[n - m + i], b = tail[i];
    if (a >= 'A' && a <= 'Z') a += 32;
    if (b >= 'A' && b <= 'Z') b += 32;
    if (a != b) return 0;
  }
  return 1;
}

static void name_only(const char *path, char *out) {
  int i = strlen(path) - 1;
  while (i >= 0 && path[i] != '\\') --i;
  strncpy(out, path + i + 1, 63);
  out[63] = 0;
}

static void up_folder() {
  int n = strlen(cwd_path);
  if (n <= 7) return;
  if (cwd_path[n - 1] == '\\') cwd_path[n - 1] = 0;
  n = strlen(cwd_path) - 1;
  while (n >= 7 && cwd_path[n] != '\\') --n;
  cwd_path[n + 1] = 0;
}

static int scan_dir() {
  entry_count = 0;
  unsigned short path[300], found[300];
  char query[300], name[300];
  strcpy(query, cwd_path);
  strcat(query, "*");
  Bfile_StrToName_ncpy(path, (const unsigned char *)query, 300);
  int handle = 0;
  file_type_t info;
  int ret = Bfile_FindFirst_NON_SMEM(path, &handle, found, &info);
  while (!ret && entry_count < 80) {
    Bfile_NameToStr_ncpy((unsigned char *)name, found, 300);
    if (strcmp(name, ".") && strcmp(name, "..") && strcmp(name, "@MainMem")) {
      int is_folder = info.fsize == 0;
      if (is_folder || ends_with(name, ".txt") || ends_with(name, ".bmp")) {
        strncpy(entries[entry_count].name, name, 63);
        entries[entry_count].name[63] = 0;
        strcpy(entries[entry_count].path, cwd_path);
        strcat(entries[entry_count].path, name);
        if (is_folder) strcat(entries[entry_count].path, "\\");
        entries[entry_count].is_folder = is_folder;
        entries[entry_count].size = info.fsize;
        ++entry_count;
      }
    }
    ret = Bfile_FindNext_NON_SMEM(handle, found, (char *)&info);
  }
  Bfile_FindClose(handle);
  return entry_count;
}

static int load_text(const char *path) {
  unsigned short p[300];
  Bfile_StrToName_ncpy(p, (const unsigned char *)path, 300);
  int h = Bfile_OpenFile_OS(p, READWRITE);
  if (h < 0) return 0;
  int sz = Bfile_GetFileSize_OS(h);
  if (sz > (int)sizeof(file_buf) - 1) sz = sizeof(file_buf) - 1;
  int got = Bfile_ReadFile_OS(h, file_buf, sz, 0);
  Bfile_CloseFile_OS(h);
  if (got < 0) return 0;
  file_buf[got] = 0;
  int line = 0, col = 0;
  for (int i = 0; i < got && line < 128; ++i) {
    char c = file_buf[i];
    if (c == '\r') continue;
    if (c == '\n' || col >= 43) {
      line_store[line][col] = 0;
      view_lines[line] = line_store[line];
      ++line;
      col = 0;
      if (c == '\n') continue;
    }
    if (c >= 32 && c <= 126 && line < 128) line_store[line][col++] = c;
  }
  if (line < 128 && col) {
    line_store[line][col] = 0;
    view_lines[line] = line_store[line];
    ++line;
  }
  return line;
}

static unsigned read_u16(const unsigned char *p) {
  return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned read_u32(const unsigned char *p) {
  return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

static int read_s32(const unsigned char *p) {
  return (int)read_u32(p);
}

static void wait_for_back(unsigned *tick) {
  bool rv = ui_r_visible(*tick);
  for (;;) {
    int key = ui_key_poll();
    bool nr = ui_r_visible(*tick);
    if (nr != rv) {
      rv = nr;
      ui_status(rv);
      ui_flush();
    }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC) return;
    OS_InnerWait_ms(35);
  }
}

static void show_bmp(NoteEntry *e, unsigned *tick) {
  unsigned short p[300];
  Bfile_StrToName_ncpy(p, (const unsigned char *)e->path, 300);
  int h = Bfile_OpenFile_OS(p, READWRITE);
  if (h < 0) {
    static const char *const err[] = {"Could not open BMP file."};
    ui_wait_page("NOTES", err, 1, tick);
    return;
  }

  unsigned char head[54];
  int got = Bfile_ReadFile_OS(h, head, 54, 0);
  if (got < 54 || head[0] != 'B' || head[1] != 'M') {
    Bfile_CloseFile_OS(h);
    static const char *const err[] = {"Unsupported BMP file."};
    ui_wait_page("NOTES", err, 1, tick);
    return;
  }

  int off = (int)read_u32(head + 10);
  int w = read_s32(head + 18);
  int raw_h = read_s32(head + 22);
  int bpp = (int)read_u16(head + 28);
  int comp = (int)read_u32(head + 30);
  int top_down = raw_h < 0;
  int hh = top_down ? -raw_h : raw_h;
  if (w <= 0 || hh <= 0 || w > 384 || hh > 192 || bpp != 24 || comp != 0) {
    Bfile_CloseFile_OS(h);
    static const char *const err[] = {
      "Use 24-bit uncompressed BMP.",
      "Run tools/prepare_notes_assets.py",
      "then copy the converted file."
    };
    ui_wait_page("BMP", err, sizeof(err)/sizeof(err[0]), tick);
    return;
  }

  int stride = ((w * 3 + 3) / 4) * 4;
  if (stride > (int)sizeof(bmp_row)) {
    Bfile_CloseFile_OS(h);
    static const char *const err[] = {"BMP row too wide."};
    ui_wait_page("BMP", err, 1, tick);
    return;
  }

  Bdisp_AllClr_VRAM();
  ui_fill(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX, UI_WHITE);
  ui_status(ui_r_visible(*tick));
  Bdisp_MMPrint(10, 28, e->name, 0x40, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 1, 0);
  ui_text_fkey(1, "BACK");
  int x0 = (LCD_WIDTH_PX - w) / 2;
  int y0 = 48 + (168 - hh) / 2;
  if (y0 < 48) y0 = 48;
  for (int y = 0; y < hh; ++y) {
    int src_y = top_down ? y : (hh - 1 - y);
    int row_off = off + src_y * stride;
    if (Bfile_ReadFile_OS(h, bmp_row, stride, row_off) < stride) break;
    for (int x = 0; x < w; ++x) {
      int b = bmp_row[x * 3 + 0];
      int g = bmp_row[x * 3 + 1];
      int r = bmp_row[x * 3 + 2];
      ui_pixel(x0 + x, y0 + y, RGB565(r, g, b));
    }
  }
  Bfile_CloseFile_OS(h);
  ui_flush();
  wait_for_back(tick);
}

static void notes_menu(const char *title, const char *const *items, int count, int top, int sel, bool rv) {
  ui_menu(title, items, count, top, sel, rv);
  ui_text_fkey(1, "FIND");
  ui_flush();
}

static void notes_text_page(const char *title, const char *const *lines, int count, int top, bool rv) {
  ui_chrome(title, rv);
  ui_text_fkey(0, "FIND");
  ui_text_fkey(1, "BACK");
  ui_text_fkey(2, "UP");
  ui_text_fkey(3, "DOWN");
  for (int i = 0; i < 8 && top + i < count; ++i)
    ui_print(14, 55 + i * 17, lines[top + i]);
  ui_flush();
}

static void wait_text_page(const char *title, const char *const *lines, int count, unsigned *tick) {
  int top = 0;
  bool rv = ui_r_visible(*tick);
  notes_text_page(title, lines, count, top, rv);
  for (;;) {
    int key = ui_key_poll();
    bool nr = ui_r_visible(*tick);
    if (nr != rv) {
      rv = nr;
      notes_text_page(title, lines, count, top, rv);
    }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC) return;
    if (key == KEY_CTRL_UP && top > 0) notes_text_page(title, lines, count, --top, rv);
    if (key == KEY_CTRL_DOWN && top + 8 < count) notes_text_page(title, lines, count, ++top, rv);
    if (key == KEY_CTRL_F1) {
      char q[32] = "";
      if (ui_input("Find text", q, sizeof(q), tick) && q[0]) {
        int found = -1;
        for (int i = top + 1; i < count; ++i) {
          if (contains_ci(lines[i], q)) { found = i; break; }
        }
        if (found < 0) {
          for (int i = 0; i <= top && i < count; ++i) {
            if (contains_ci(lines[i], q)) { found = i; break; }
          }
        }
        if (found >= 0) {
          top = found;
          if (top + 8 > count) top = count > 8 ? count - 8 : 0;
          notes_text_page(title, lines, count, top, rv);
        } else {
          static const char *const none[] = {"No matching text."};
          ui_wait_page("Find text", none, 1, tick);
          notes_text_page(title, lines, count, top, rv);
        }
      } else {
        notes_text_page(title, lines, count, top, rv);
      }
    }
    OS_InnerWait_ms(35);
  }
}

static void search_names(int *sel, int *top, unsigned *tick) {
  char q[32] = "";
  if (!ui_input("Find file", q, sizeof(q), tick) || !q[0]) return;
  for (int i = 0; i < entry_count; ++i) {
    if (contains_ci(entries[i].name, q)) {
      *sel = i;
      *top = i > 6 ? i - 6 : 0;
      return;
    }
  }
  static const char *const none[] = {"No matching file."};
  ui_wait_page("Find file", none, 1, tick);
}

static void show_file(NoteEntry *e, unsigned *tick) {
  if (ends_with(e->name, ".bmp")) {
    show_bmp(e, tick);
    return;
  }
  int lines = load_text(e->path);
  if (!lines) {
    static const char *const err[] = {"Could not open text file."};
    ui_wait_page("NOTES", err, 1, tick);
    return;
  }
  wait_text_page(e->name, view_lines, lines, tick);
}

int main() {
  Bdisp_EnableColor(1);
  unsigned tick = (unsigned)RTC_GetTicks();
  int sel = 0, top = 0;
  scan_dir();
  bool rv = ui_r_visible(tick);
  for (;;) {
    const char *names[80];
    for (int i = 0; i < entry_count; ++i) names[i] = entries[i].name;
    notes_menu("NOTES", names, entry_count, top, sel, rv);
    int key = ui_key_poll();
    bool nr = ui_r_visible(tick);
    if (nr != rv) rv = nr;
    if (key == KEY_CTRL_MENU || key == KEY_CTRL_AC) return 0;
    if (key == KEY_CTRL_EXIT) { up_folder(); sel = top = 0; scan_dir(); }
    if (key == KEY_CTRL_UP && sel > 0) { --sel; if (sel < top) --top; }
    if (key == KEY_CTRL_DOWN && sel + 1 < entry_count) { ++sel; if (sel >= top + 7) ++top; }
    if (key == KEY_CTRL_F2 && entry_count) search_names(&sel, &top, &tick);
    if ((key == KEY_CTRL_EXE || key == KEY_CTRL_F1) && entry_count) {
      if (entries[sel].is_folder) { strcpy(cwd_path, entries[sel].path); sel = top = 0; scan_dir(); }
      else show_file(&entries[sel], &tick);
    }
    OS_InnerWait_ms(35);
  }
}

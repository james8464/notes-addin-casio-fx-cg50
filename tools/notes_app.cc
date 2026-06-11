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

static void show_file(NoteEntry *e, unsigned *tick) {
  if (ends_with(e->name, ".bmp")) {
    static const char *const bmp[] = {
      "BMP file found.",
      "Image preview is reserved for",
      "converted calculator BMP assets.",
      "Use tools/prepare_notes_assets.py",
      "before copying notes to calculator."
    };
    ui_wait_page(e->name, bmp, sizeof(bmp)/sizeof(bmp[0]), tick);
    return;
  }
  int lines = load_text(e->path);
  if (!lines) {
    static const char *const err[] = {"Could not open text file."};
    ui_wait_page("NOTES", err, 1, tick);
    return;
  }
  ui_wait_page(e->name, view_lines, lines, tick);
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
    ui_menu("NOTES", names, entry_count, top, sel, rv);
    int key = ui_key_poll();
    bool nr = ui_r_visible(tick);
    if (nr != rv) rv = nr;
    if (key == KEY_CTRL_MENU || key == KEY_CTRL_AC) return 0;
    if (key == KEY_CTRL_EXIT) { up_folder(); sel = top = 0; scan_dir(); }
    if (key == KEY_CTRL_UP && sel > 0) { --sel; if (sel < top) --top; }
    if (key == KEY_CTRL_DOWN && sel + 1 < entry_count) { ++sel; if (sel >= top + 7) ++top; }
    if ((key == KEY_CTRL_EXE || key == KEY_CTRL_F1) && entry_count) {
      if (entries[sel].is_folder) { strcpy(cwd_path, entries[sel].path); sel = top = 0; scan_dir(); }
      else show_file(&entries[sel], &tick);
    }
    OS_InnerWait_ms(35);
  }
}

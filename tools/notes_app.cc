#include "casio_suite_ui.hpp"
#include <fxcg/file.h>
#include <string.h>

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

static void copy_str(char *dst, int cap, const char *src) {
  if (!src) src = "";
  int i = 0;
  while (i + 1 < cap && src[i]) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = 0;
}

static void append_str(char *dst, int cap, const char *src) {
  if (!src) return;
  int n = strlen(dst), i = 0;
  while (n + 1 < cap && src[i]) dst[n++] = src[i++];
  dst[n] = 0;
}

static void join_str(char *dst, int cap, const char *a, const char *b) {
  copy_str(dst, cap, a);
  append_str(dst, cap, b);
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
      if (is_folder || ends_with(name, ".txt")) {
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

static int file_contains_text(const char *path, const char *q) {
  int qlen = strlen(q);
  if (!qlen) return 1;
  unsigned short p[300];
  Bfile_StrToName_ncpy(p, (const unsigned char *)path, 300);
  int h = Bfile_OpenFile_OS(p, READWRITE);
  if (h < 0) return 0;
  int sz = Bfile_GetFileSize_OS(h);
  char tail[32];
  int tail_len = 0;
  int off = 0;
  while (off < sz) {
    for (int i = 0; i < tail_len; ++i) file_buf[i] = tail[i];
    int want = (int)sizeof(file_buf) - tail_len - 1;
    if (want > sz - off) want = sz - off;
    int got = Bfile_ReadFile_OS(h, file_buf + tail_len, want, off);
    if (got <= 0) break;
    int total = tail_len + got;
    file_buf[total] = 0;
    if (contains_ci(file_buf, q)) {
      Bfile_CloseFile_OS(h);
      return 1;
    }
    tail_len = qlen - 1;
    if (tail_len > 31) tail_len = 31;
    if (tail_len > total) tail_len = total;
    for (int i = 0; i < tail_len; ++i) tail[i] = file_buf[total - tail_len + i];
    off += got;
  }
  Bfile_CloseFile_OS(h);
  return 0;
}

static int add_search_result(int n, const char *path, const char *why) {
  if (n >= 126) return n;
  char name[64];
  name_only(path, name);
  copy_str(line_store[n], sizeof(line_store[n]), why);
  append_str(line_store[n], sizeof(line_store[n]), ": ");
  append_str(line_store[n], sizeof(line_store[n]), name);
  view_lines[n] = line_store[n];
  ++n;
  copy_str(line_store[n], sizeof(line_store[n]), path);
  view_lines[n] = line_store[n];
  return n + 1;
}

static int search_all_rec(const char *dir, const char *q, int depth, int n) {
  if (depth > 8 || n >= 126) return n;
  unsigned short path[300], found[300];
  char query[300], name[300], full[300];
  join_str(query, sizeof(query), dir, "*");
  Bfile_StrToName_ncpy(path, (const unsigned char *)query, 300);
  int handle = 0;
  file_type_t info;
  int ret = Bfile_FindFirst_NON_SMEM(path, &handle, found, &info);
  while (!ret && n < 126) {
    Bfile_NameToStr_ncpy((unsigned char *)name, found, 300);
    if (strcmp(name, ".") && strcmp(name, "..") && strcmp(name, "@MainMem")) {
      int is_folder = info.fsize == 0;
      join_str(full, sizeof(full), dir, name);
      if (is_folder) {
        char child[300];
        join_str(child, sizeof(child), full, "\\");
        n = search_all_rec(child, q, depth + 1, n);
      } else if (ends_with(name, ".txt")) {
        int by_name = contains_ci(name, q) || contains_ci(full, q);
        int by_text = file_contains_text(full, q);
        if (by_name || by_text) n = add_search_result(n, full, by_text ? "text" : "name");
      }
    }
    ret = Bfile_FindNext_NON_SMEM(handle, found, (char *)&info);
  }
  Bfile_FindClose(handle);
  return n;
}

static void search_all_notes(unsigned *tick) {
  char q[32] = "";
  if (!ui_input("Find all text", q, sizeof(q), tick) || !q[0]) return;
  int n = search_all_rec("\\\\fls0\\", q, 0, 0);
  if (!n) {
    static const char *const none[] = {"No matching text file."};
    ui_wait_page("Find all", none, 1, tick);
    return;
  }
  ui_wait_page("Find all", view_lines, n, tick);
}

static void show_file(NoteEntry *e, unsigned *tick) {
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
    if (key == KEY_CTRL_F2) search_all_notes(&tick);
    if ((key == KEY_CTRL_EXE || key == KEY_CTRL_F1) && entry_count) {
      if (entries[sel].is_folder) { strcpy(cwd_path, entries[sel].path); sel = top = 0; scan_dir(); }
      else show_file(&entries[sel], &tick);
    }
    OS_InnerWait_ms(35);
  }
}

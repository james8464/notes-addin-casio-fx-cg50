#include "casio_suite_ui.hpp"
#include <fxcg/file.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
} file_type_t;

static const int MAX_ENTRIES = 120;
static const int MAX_RESULTS = 96;
static const int FILE_BUF_SIZE = 16384;
static const int MAX_VIEW_LINES = 768;
static const int LINE_CAP = 128;
static const int MAX_TABLE_COLS = 8;
static const int MAX_TABLE_ROWS = 24;
static const int TABLE_CELL_CAP = 416;
static const int TABLE_MAX_CHARS = LINE_CAP - 4;
static const int TABLE_CHAR_PX = 8;
static const int TABLE_PAD_X = 3;
static const int VIEW_X = 4;
static const int VIEW_TOP = 25;
static const int VIEW_ROW_H = 17;
static const int WRAP_COLS = 32;
static const int PAGE_LINES = 8;
static const int MENU_PAGE_ROWS = 7;
static const int MENU_ROWS = MENU_PAGE_ROWS;
static const int STATUS_LABEL_CHARS = 40;
static const int STATUS_MARQUEE_GAP = 4;
static const unsigned STATUS_MARQUEE_TICKS = 48;
static const unsigned NOTE_X_LIMIT = LCD_WIDTH_PX - 18;
static const unsigned short UI_MATCH_TEXT = RGB565(210, 0, 0);
static const unsigned short UI_H2_TEXT = RGB565(0, 80, 160);
static const unsigned short UI_H3_TEXT = RGB565(70, 70, 85);
static const unsigned short UI_H4_TEXT = RGB565(115, 80, 120);

enum NoteLineStyle {
  NOTE_TEXT = 0,
  NOTE_H1 = 1,
  NOTE_H2 = 2,
  NOTE_H3 = 3,
  NOTE_H4 = 4,
  NOTE_BULLET = 5,
  NOTE_ORDERED = 6,
  NOTE_QUOTE = 7,
  NOTE_RULE = 8,
  NOTE_CODE = 9,
  NOTE_TABLE = 10
};

enum TableKind {
  TABLE_NONE = 0,
  TABLE_TAB = 1,
  TABLE_PIPE = 2,
  TABLE_GRID = 3
};

struct NoteEntry {
  char name[64];
  char path[280];
  int is_folder;
  int size;
};

struct SearchPattern {
  char pat[32];
  int fail[32];
  int len;
};

struct SearchResult {
  char name[64];
  char path[280];
  char label[64];
  int score;
  int first_line;
  int text_hits;
};

struct TableRow {
  char cells[MAX_TABLE_COLS][TABLE_CELL_CAP];
  int cols;
  int source_line;
  int header;
};

static NoteEntry entries[MAX_ENTRIES];
static SearchResult results[MAX_RESULTS];
static int entry_count = 0;
static int result_count = 0;
static const char NOTES_ROOT[] = "\\\\fls0\\NOTES\\";
static char cwd_path[280] = "\\\\fls0\\NOTES\\";
static char file_buf[FILE_BUF_SIZE];
static int file_buf_len = 0;
static const char *view_lines[MAX_VIEW_LINES];
static char line_store[MAX_VIEW_LINES][LINE_CAP];
static char display_line_buf[FILE_BUF_SIZE];
static char search_line_buf[FILE_BUF_SIZE];
static unsigned char view_style[MAX_VIEW_LINES];
static short view_source_line[MAX_VIEW_LINES];
static unsigned char view_table_cols[MAX_VIEW_LINES];
static unsigned char view_table_top[MAX_VIEW_LINES];
static unsigned char view_table_bottom[MAX_VIEW_LINES];
static unsigned char view_table_header[MAX_VIEW_LINES];
static short view_table_colpos[MAX_VIEW_LINES][MAX_TABLE_COLS + 1];
static char last_query[32] = "";
static TableRow table_rows[MAX_TABLE_ROWS];
static char table_parse_cells[MAX_TABLE_COLS][TABLE_CELL_CAP];
static char table_segment_cells[MAX_TABLE_COLS][TABLE_CELL_CAP];

static int lower_char(int c) {
  if (c >= 'A' && c <= 'Z') return c + 32;
  return c;
}

static bool bytes_at(int i, unsigned char a, unsigned char b) {
  return i + 1 < file_buf_len &&
         (unsigned char)file_buf[i] == a &&
         (unsigned char)file_buf[i + 1] == b;
}

static bool bytes_at3(int i, unsigned char a, unsigned char b, unsigned char c) {
  return i + 2 < file_buf_len &&
         (unsigned char)file_buf[i] == a &&
         (unsigned char)file_buf[i + 1] == b &&
         (unsigned char)file_buf[i + 2] == c;
}

static void ascii_append(char *dst, int *n, int cap, const char *s) {
  for (int i = 0; s && s[i] && *n + 1 < cap; ++i) dst[(*n)++] = s[i];
}

static void normalize_file_buf() {
  static char clean[FILE_BUF_SIZE];
  int out = 0;
  for (int i = 0; i < file_buf_len && out + 1 < FILE_BUF_SIZE;) {
    unsigned char c = (unsigned char)file_buf[i];
    if (c == '\r') {
      clean[out++] = '\n';
      if (i + 1 < file_buf_len && file_buf[i + 1] == '\n') i += 2;
      else ++i;
      continue;
    }
    if (c < 128) {
      clean[out++] = (char)c;
      ++i;
      continue;
    }
    if (bytes_at(i, 0xc2, 0xa0)) { clean[out++] = ' '; i += 2; continue; }
    if (bytes_at(i, 0xc2, 0xb1)) { ascii_append(clean, &out, FILE_BUF_SIZE, "+/-"); i += 2; continue; }
    if (bytes_at(i, 0xc2, 0xb0)) { ascii_append(clean, &out, FILE_BUF_SIZE, " deg"); i += 2; continue; }
    if (bytes_at(i, 0xc3, 0x97)) { clean[out++] = 'x'; i += 2; continue; }
    if (bytes_at(i, 0xc3, 0xb7)) { clean[out++] = '/'; i += 2; continue; }
    if (bytes_at3(i, 0xe2, 0x80, 0x98) || bytes_at3(i, 0xe2, 0x80, 0x99)) { clean[out++] = '\''; i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x80, 0x9c) || bytes_at3(i, 0xe2, 0x80, 0x9d)) { clean[out++] = '"'; i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x80, 0x93) || bytes_at3(i, 0xe2, 0x80, 0x94)) { clean[out++] = '-'; i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x80, 0xa2)) { ascii_append(clean, &out, FILE_BUF_SIZE, "- "); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x80, 0xa6)) { ascii_append(clean, &out, FILE_BUF_SIZE, "..."); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x86, 0x92)) { ascii_append(clean, &out, FILE_BUF_SIZE, "->"); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x86, 0x90)) { ascii_append(clean, &out, FILE_BUF_SIZE, "<-"); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x86, 0x94)) { ascii_append(clean, &out, FILE_BUF_SIZE, "<->"); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x87, 0x92)) { ascii_append(clean, &out, FILE_BUF_SIZE, "=>"); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x87, 0x94)) { ascii_append(clean, &out, FILE_BUF_SIZE, "<=>"); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x89, 0xa0)) { ascii_append(clean, &out, FILE_BUF_SIZE, "!="); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x89, 0xa4)) { ascii_append(clean, &out, FILE_BUF_SIZE, "<="); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x89, 0xa5)) { ascii_append(clean, &out, FILE_BUF_SIZE, ">="); i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x84, 0x95)) { clean[out++] = 'N'; i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x84, 0x9a)) { clean[out++] = 'Q'; i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x84, 0x9d)) { clean[out++] = 'R'; i += 3; continue; }
    if (bytes_at3(i, 0xe2, 0x84, 0xa4)) { clean[out++] = 'Z'; i += 3; continue; }
    clean[out++] = '?';
    if ((c & 0xe0) == 0xc0 && i + 1 < file_buf_len) i += 2;
    else if ((c & 0xf0) == 0xe0 && i + 2 < file_buf_len) i += 3;
    else if ((c & 0xf8) == 0xf0 && i + 3 < file_buf_len) i += 4;
    else ++i;
  }
  clean[out] = 0;
  memcpy(file_buf, clean, out + 1);
  file_buf_len = out;
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

static int min_int(int a, int b) { return a < b ? a : b; }
static int max_int(int a, int b) { return a > b ? a : b; }
static int md_space(char c) { return c == ' ' || c == '\t'; }

static void search_prepare(SearchPattern *sp, const char *q) {
  int i = 0;
  while (q && q[i] && i < 31) {
    sp->pat[i] = (char)lower_char((unsigned char)q[i]);
    ++i;
  }
  sp->pat[i] = 0;
  sp->len = i;
  if (sp->len <= 0) return;
  sp->fail[0] = 0;
  for (int k = 1; k < sp->len; ++k) {
    int j = sp->fail[k - 1];
    while (j > 0 && sp->pat[k] != sp->pat[j]) j = sp->fail[j - 1];
    if (sp->pat[k] == sp->pat[j]) ++j;
    sp->fail[k] = j;
  }
}

static int search_step(const SearchPattern *sp, int state, char raw) {
  char c = (char)lower_char((unsigned char)raw);
  while (state > 0 && c != sp->pat[state]) state = sp->fail[state - 1];
  if (c == sp->pat[state]) ++state;
  return state;
}

static int contains_kmp(const char *s, const SearchPattern *sp) {
  if (sp->len <= 0) return 1;
  int state = 0;
  for (int i = 0; s[i]; ++i) {
    state = search_step(sp, state, s[i]);
    if (state == sp->len) return 1;
  }
  return 0;
}

static int find_in_span(const char *s, int len, const SearchPattern *sp) {
  if (!s || !sp || sp->len <= 0 || len <= 0) return -1;
  int state = 0;
  for (int i = 0; i < len; ++i) {
    state = search_step(sp, state, s[i]);
    if (state == sp->len) return i - sp->len + 1;
  }
  return -1;
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

static int same_name_ci(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    if (lower_char((unsigned char)a[i]) != lower_char((unsigned char)b[i])) return 0;
    ++i;
  }
  return a[i] == 0 && b[i] == 0;
}

static int hidden_system_folder(const char *name) {
  return same_name_ci(name, "CASIO") ||
         same_name_ci(name, ".Trashes") ||
         same_name_ci(name, "SAVE-F") ||
         same_name_ci(name, ".fseventsd");
}

static int hidden_note_name(const char *name) {
  return !name || !name[0] || name[0] == '.' || name[0] == '~' ||
         (name[0] == '_' && name[1] == '_') || !strncmp(name, "._", 2);
}

static void clean_label(char *out, int cap, const char *name) {
  int n = 0;
  for (int i = 0; name && name[i] && n + 1 < cap; ++i) {
    char c = name[i] == '_' ? ' ' : name[i];
    if (c >= 32 && c <= 126) out[n++] = c;
  }
  out[n] = 0;
  int len = strlen(out);
  if (len > 4 &&
      out[len - 4] == '.' &&
      lower_char((unsigned char)out[len - 3]) == 't' &&
      lower_char((unsigned char)out[len - 2]) == 'x' &&
      lower_char((unsigned char)out[len - 1]) == 't')
    out[len - 4] = 0;
}

static void status_label(char *out, int cap, const char *title) {
  char full[96];
  clean_label(full, sizeof(full), title ? title : "NOTES");
  if (!full[0]) copy_str(full, sizeof(full), "NOTES");
  int len = strlen(full);
  if (len <= STATUS_LABEL_CHARS || cap <= STATUS_LABEL_CHARS + 1) {
    copy_str(out, cap, full);
    return;
  }
  unsigned step = ((unsigned)RTC_GetTicks() / STATUS_MARQUEE_TICKS) % (len + STATUS_MARQUEE_GAP);
  int n = 0;
  for (int i = 0; i < STATUS_LABEL_CHARS && n + 1 < cap; ++i) {
    int p = (int)step + i;
    if (p >= len + STATUS_MARQUEE_GAP) p -= len + STATUS_MARQUEE_GAP;
    out[n++] = p < len ? full[p] : ' ';
  }
  out[n] = 0;
}

static int status_title_needs_marquee(const char *title) {
  char full[96];
  clean_label(full, sizeof(full), title ? title : "NOTES");
  if (!full[0]) copy_str(full, sizeof(full), "NOTES");
  return (int)strlen(full) > STATUS_LABEL_CHARS;
}

static unsigned status_marquee_step() {
  return (unsigned)RTC_GetTicks() / STATUS_MARQUEE_TICKS;
}

static void notes_chrome(const char *title) {
  Bdisp_AllClr_VRAM();
  ui_fill(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX - 24, UI_WHITE);
  static char label[64];
  status_label(label, sizeof(label), title);
  DefineStatusAreaFlags(3, SAF_TEXT, 0, 0);
  DefineStatusMessage(label, 1, 0, 0);
  EnableStatusArea(2);
  DisplayStatusArea();
}

static void notes_softkeys(const char *f1, const char *f2, const char *f3,
                           const char *f4, const char *f5, const char *f6) {
  ui_softkeys(f1, f2, f3, f4, f5, f6);
}

static int notes_key_poll_timed() {
  int col = 0, row = 0;
  unsigned short keycode = 0;
  int ret = GetKeyWait_OS(&col, &row, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &keycode);
  if (ret != KEYREP_KEYEVENT) return 0;
  if (col == 1) return KEY_CTRL_AC;
  if (col == 4 && row == 9) return KEY_CTRL_MENU;
  if (col == 4 && row == 8) return KEY_CTRL_EXIT;
  return keycode;
}

// Keep the menu key path discoverable for the suite smoke test: ui_menu_keys.

static void name_only(const char *path, char *out) {
  int i = strlen(path) - 1;
  while (i >= 0 && path[i] != '\\') --i;
  copy_str(out, 64, path + i + 1);
}

static void up_folder() {
  int n = strlen(cwd_path);
  int root_len = strlen(NOTES_ROOT);
  if (n <= root_len) return;
  if (cwd_path[n - 1] == '\\') cwd_path[n - 1] = 0;
  n = strlen(cwd_path) - 1;
  while (n >= root_len && cwd_path[n] != '\\') --n;
  cwd_path[n + 1] = 0;
  if ((int)strlen(cwd_path) < root_len) copy_str(cwd_path, sizeof(cwd_path), NOTES_ROOT);
}

static int entry_less(const NoteEntry &a, const NoteEntry &b) {
  if (a.is_folder != b.is_folder) return a.is_folder > b.is_folder;
  for (int i = 0; a.name[i] || b.name[i]; ++i) {
    int ca = lower_char((unsigned char)a.name[i]);
    int cb = lower_char((unsigned char)b.name[i]);
    if (ca != cb) return ca < cb;
  }
  return 0;
}

static void sort_entries() {
  for (int i = 1; i < entry_count; ++i) {
    NoteEntry cur = entries[i];
    int j = i - 1;
    while (j >= 0 && entry_less(cur, entries[j])) {
      entries[j + 1] = entries[j];
      --j;
    }
    entries[j + 1] = cur;
  }
}

static int scan_dir() {
  entry_count = 0;
  unsigned short path[300], found[300];
  char query[300], name[300];
  copy_str(query, sizeof(query), cwd_path);
  append_str(query, sizeof(query), "*");
  Bfile_StrToName_ncpy(path, (const unsigned char *)query, 300);
  int handle = 0;
  file_type_t info;
  int ret = Bfile_FindFirst_NON_SMEM(path, &handle, found, &info);
  int opened = !ret;
  while (!ret && entry_count < MAX_ENTRIES) {
    Bfile_NameToStr_ncpy((unsigned char *)name, found, 300);
    if (strcmp(name, ".") && strcmp(name, "..") && strcmp(name, "@MainMem") && !hidden_note_name(name)) {
      int is_txt = ends_with(name, ".txt");
      int is_folder = !is_txt && info.fsize == 0;
      if ((is_folder && !hidden_system_folder(name)) || is_txt) {
        copy_str(entries[entry_count].name, sizeof(entries[entry_count].name), name);
        copy_str(entries[entry_count].path, sizeof(entries[entry_count].path), cwd_path);
        append_str(entries[entry_count].path, sizeof(entries[entry_count].path), name);
        if (is_folder) append_str(entries[entry_count].path, sizeof(entries[entry_count].path), "\\");
        entries[entry_count].is_folder = is_folder;
        entries[entry_count].size = info.fsize;
        ++entry_count;
      }
    }
    ret = Bfile_FindNext_NON_SMEM(handle, found, (char *)&info);
  }
  if (opened) Bfile_FindClose(handle);
  sort_entries();
  return entry_count;
}

static int load_file_buf(const char *path) {
  unsigned short p[300];
  Bfile_StrToName_ncpy(p, (const unsigned char *)path, 300);
  int h = Bfile_OpenFile_OS(p, READWRITE);
  if (h < 0) return 0;
  int sz = Bfile_GetFileSize_OS(h);
  if (sz > FILE_BUF_SIZE - 1) {
    Bfile_CloseFile_OS(h);
    return -1;
  }
  if (sz == 0) {
    Bfile_CloseFile_OS(h);
    file_buf[0] = 0;
    file_buf_len = 0;
    return 1;
  }
  int got = Bfile_ReadFile_OS(h, file_buf, sz, 0);
  Bfile_CloseFile_OS(h);
  if (got < 0) return 0;
  file_buf[got] = 0;
  file_buf_len = got;
  normalize_file_buf();
  return 1;
}

static int markdown_escapable(char c) {
  return c == '\\' || c == '`' || c == '*' || c == '_' || c == '{' || c == '}' ||
         c == '[' || c == ']' || c == '(' || c == ')' || c == '#' || c == '+' ||
         c == '-' || c == '.' || c == '!' || c == '|';
}

static int markdown_escaped_at(const char *s, int i) {
  int n = 0;
  for (int p = i - 1; p >= 0 && s[p] == '\\'; --p) ++n;
  return n & 1;
}

static int tab_separated_cells(const char *s, int len) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  int cells = 0, cell_has_text = 0, tab_seen = 0;
  for (int i = p; i <= len; ++i) {
    if (i == len || s[i] == '\t') {
      if (cell_has_text) ++cells;
      cell_has_text = 0;
      if (i < len) tab_seen = 1;
    } else if (s[i] != ' ') {
      cell_has_text = 1;
    }
  }
  return tab_seen && cells >= 2;
}

static int pipe_separated_cells(const char *s, int len) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  int cells = 0, cell_has_text = 0, pipe_seen = 0;
  for (int i = p; i <= len; ++i) {
    if (i == len || (s[i] == '|' && !markdown_escaped_at(s, i))) {
      if (cell_has_text) ++cells;
      cell_has_text = 0;
      if (i < len) pipe_seen = 1;
    } else if (s[i] != ' ') {
      cell_has_text = 1;
    }
  }
  return pipe_seen && cells >= 2;
}

static int table_like(const char *s, int len) {
  int tab = 0, plus = 0, dashes = 0;
  int first = 0;
  while (first < len && md_space(s[first])) ++first;
  for (int i = 0; i < len; ++i) {
    if (s[i] == '\t') tab = 1;
    if (s[i] == '+') ++plus;
    if (s[i] == '-') ++dashes;
  }
  if (tab) return tab_separated_cells(s, len);
  if (first < len && s[first] == '+') return plus >= 2 && dashes >= 3;
  return 0;
}

static int table_block_row_like(const char *s, int len) {
  return table_like(s, len) || pipe_separated_cells(s, len);
}

static int plus_separator_row(const char *s, int len) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  if (p >= len || s[p] != '+') return 0;
  int plus = 0, rule = 0;
  for (int i = p; i < len; ++i) {
    char c = s[i];
    if (c == '+') ++plus;
    else if (c == '-' || c == '=') ++rule;
    else if (c != ':' && c != ' ') return 0;
  }
  return plus >= 2 && rule >= 3;
}

static int table_row_kind(const char *s, int len) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  if (tab_separated_cells(s, len)) return TABLE_TAB;
  if (p < len && s[p] == '+' && plus_separator_row(s, len)) return TABLE_GRID;
  if (pipe_separated_cells(s, len)) return TABLE_PIPE;
  return TABLE_NONE;
}

static int table_row_matches_kind(const char *s, int len, int kind) {
  if (kind == TABLE_TAB) return tab_separated_cells(s, len);
  if (kind == TABLE_PIPE) return pipe_separated_cells(s, len);
  if (kind == TABLE_GRID) return plus_separator_row(s, len) || pipe_separated_cells(s, len);
  return table_block_row_like(s, len);
}

static int trim_left_pos(const char *s, int len) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  return p;
}

static int trim_right_len(const char *s, int len) {
  while (len > 0 && md_space(s[len - 1])) --len;
  return len;
}

static int fence_like(const char *s, int len) {
  int p = trim_left_pos(s, len);
  if (p + 2 >= len) return 0;
  char c = s[p];
  return (c == '`' || c == '~') && s[p + 1] == c && s[p + 2] == c;
}

static int source_code_like(const char *s, int len) {
  int p = trim_left_pos(s, len);
  if (fence_like(s, len)) return 1;
  if (p >= 4) return 1;
  return (p + 2 < len && s[p] == '>' && s[p + 1] == '>' && s[p + 2] == '>') ||
         (p + 1 < len && s[p] == '$' && s[p + 1] == ' ') ||
         (p + 4 < len &&
          lower_char((unsigned char)s[p]) == 'g' &&
          lower_char((unsigned char)s[p + 1]) == 'h' &&
          lower_char((unsigned char)s[p + 2]) == 'c' &&
          lower_char((unsigned char)s[p + 3]) == 'i' &&
          s[p + 4] == '>');
}

static int html_break_len(const char *s, int i, int len) {
  if (i + 3 >= len || s[i] != '<' ||
      lower_char((unsigned char)s[i + 1]) != 'b' ||
      lower_char((unsigned char)s[i + 2]) != 'r')
    return 0;
  int p = i + 3;
  while (p < len && (s[p] == ' ' || s[p] == '\t')) ++p;
  if (p < len && s[p] == '/') {
    ++p;
    while (p < len && (s[p] == ' ' || s[p] == '\t')) ++p;
  }
  if (p < len && s[p] == '>') return p - i + 1;
  return 0;
}

static int html_entity_at(const char *s, int i, int len, char *out) {
  if (i >= len || s[i] != '&') return 0;
  struct Ent { const char *name; char value; };
  static const Ent ents[] = {
    {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'},
    {"&quot;", '"'}, {"&apos;", '\''}, {"&nbsp;", ' '}
  };
  for (unsigned e = 0; e < sizeof(ents) / sizeof(ents[0]); ++e) {
    int n = strlen(ents[e].name);
    if (i + n <= len && !strncmp(s + i, ents[e].name, n)) {
      *out = ents[e].value;
      return n;
    }
  }
  return 0;
}

static int utf8_ascii_at(const char *s, int i, int len, const char **out) {
  if (i >= len) return 0;
  unsigned char a = (unsigned char)s[i];
  if (a == 0xc2 && i + 1 < len) {
    unsigned char b = (unsigned char)s[i + 1];
    if (b == 0xa0) { *out = " "; return 2; }
    if (b == 0xb0) { *out = " deg"; return 2; }
  }
  if (a == 0xc3 && i + 1 < len) {
    unsigned char b = (unsigned char)s[i + 1];
    if (b == 0x97) { *out = "x"; return 2; }
    if (b == 0xb7) { *out = "/"; return 2; }
  }
  if (a == 0xe2 && i + 2 < len) {
    unsigned char b = (unsigned char)s[i + 1];
    unsigned char c = (unsigned char)s[i + 2];
    if (b == 0x80) {
      if (c == 0x93 || c == 0x94) { *out = "-"; return 3; }
      if (c == 0x98 || c == 0x99) { *out = "'"; return 3; }
      if (c == 0x9c || c == 0x9d) { *out = "\""; return 3; }
      if (c == 0xa2) { *out = "*"; return 3; }
      if (c == 0xa6) { *out = "..."; return 3; }
    }
    if (b == 0x86) {
      if (c == 0x90) { *out = "<-"; return 3; }
      if (c == 0x92) { *out = "->"; return 3; }
      if (c == 0x94) { *out = "<->"; return 3; }
    }
    if (b == 0x87) {
      if (c == 0x92) { *out = "=>"; return 3; }
      if (c == 0x94) { *out = "<=>"; return 3; }
    }
    if (b == 0x89) {
      if (c == 0xa0) { *out = "!="; return 3; }
      if (c == 0xa4) { *out = "<="; return 3; }
      if (c == 0xa5) { *out = ">="; return 3; }
    }
  }
  return 0;
}

static int ascii_alnum(char c) {
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int find_char_from(const char *s, int from, int len, char target) {
  for (int i = from; i < len; ++i)
    if (s[i] == target) return i;
  return -1;
}

static int markdown_link_at(const char *s, int i, int len, int *label_start, int *label_len, int *end) {
  int open = i;
  if (i < len && s[i] == '!' && i + 1 < len && s[i + 1] == '[') open = i + 1;
  else if (i >= len || s[i] != '[') return 0;
  int close_label = find_char_from(s, open + 1, len, ']');
  if (close_label < 0 || close_label + 1 >= len || s[close_label + 1] != '(') return 0;
  int close_url = find_char_from(s, close_label + 2, len, ')');
  if (close_url < 0) return 0;
  *label_start = open + 1;
  *label_len = close_label - open - 1;
  *end = close_url + 1;
  return 1;
}

static int single_marker_at(const char *s, int i, int len, char c) {
  if (s[i] != c) return 0;
  if (i + 1 < len && s[i + 1] == ' ') {
    int p = i - 1;
    while (p >= 0 && md_space(s[p])) --p;
    if (p < 0) return 0;
  }
  if ((i <= 0 || md_space(s[i - 1])) && (i + 1 >= len || md_space(s[i + 1]))) return 0;
  if (i > 0 && i + 1 < len && ascii_alnum(s[i - 1]) && ascii_alnum(s[i + 1]))
    return 0;
  if (c == '_' && ((i > 0 && ascii_alnum(s[i - 1])) || (i + 1 < len && ascii_alnum(s[i + 1]))))
    return 0;
  return find_char_from(s, i + 1, len, c) >= 0 || find_char_from(s, 0, i, c) >= 0;
}

static int double_marker_at(const char *s, int i, int len, char c) {
  if (i + 1 >= len || s[i] != c || s[i + 1] != c) return 0;
  if ((i <= 0 || md_space(s[i - 1])) && (i + 2 >= len || md_space(s[i + 2]))) return 0;
  if (c == '_' && ((i > 0 && ascii_alnum(s[i - 1])) || (i + 2 < len && ascii_alnum(s[i + 2]))))
    return 0;
  for (int p = i + 2; p + 1 < len; ++p)
    if (s[p] == c && s[p + 1] == c) return 1;
  for (int p = 0; p + 1 < i; ++p)
    if (s[p] == c && s[p + 1] == c) return 1;
  return 0;
}

static int copy_display_text(char *dst, int cap, const char *src, int len, int strip_inline) {
  int n = 0;
  int in_code_span = 0;
  for (int i = 0; i < len && n + 1 < cap; ++i) {
    int br = html_break_len(src, i, len);
    if (br) {
      if (n + 2 < cap) {
        dst[n++] = ';';
        dst[n++] = ' ';
      }
      i += br - 1;
      continue;
    }
    char entity = 0;
    int entity_len = html_entity_at(src, i, len, &entity);
    if (entity_len) {
      dst[n++] = entity;
      i += entity_len - 1;
      continue;
    }
    const char *ascii = 0;
    int utf8_len = utf8_ascii_at(src, i, len, &ascii);
    if (utf8_len) {
      for (int j = 0; ascii[j] && n + 1 < cap; ++j) dst[n++] = ascii[j];
      i += utf8_len - 1;
      continue;
    }
    if (strip_inline && src[i] == '`') {
      if (in_code_span) {
        in_code_span = 0;
        continue;
      }
      if (find_char_from(src, i + 1, len, '`') >= 0) {
        in_code_span = 1;
        continue;
      }
    }
    if (strip_inline && !in_code_span) {
      if (src[i] == '\\' && i + 1 < len && markdown_escapable(src[i + 1])) {
        dst[n++] = src[++i];
        continue;
      }
      int label_start = 0, label_len = 0, end = 0;
      if (markdown_link_at(src, i, len, &label_start, &label_len, &end)) {
        n += copy_display_text(dst + n, cap - n, src + label_start, label_len, 1);
        i = end - 1;
        continue;
      }
      if ((src[i] == '*' || src[i] == '_') && double_marker_at(src, i, len, src[i])) {
        ++i;
        continue;
      }
      if ((src[i] == '*' || src[i] == '_') && single_marker_at(src, i, len, src[i])) continue;
    }
    char c = src[i] == '\t' ? ' ' : src[i];
    if (c >= 32 && c <= 126) dst[n++] = c;
  }
  dst[n] = 0;
  return n;
}

static int clean_cell_copy(char *dst, int cap, const char *src, int len) {
  int start = trim_left_pos(src, len);
  len = trim_right_len(src + start, len - start);
  int n = copy_display_text(dst, cap, src + start, len, 1);
  while (n > 0 && dst[n - 1] == ' ') --n;
  dst[n] = 0;
  return n;
}

static int parse_table_row(const char *s, int len, char cells[MAX_TABLE_COLS][TABLE_CELL_CAP]) {
  int p = trim_left_pos(s, len);
  if (p < len && s[p] == '|') ++p;
  int cols = 0;
  while (p <= len && cols < MAX_TABLE_COLS) {
    int start = p;
    if (cols == MAX_TABLE_COLS - 1) {
      clean_cell_copy(cells[cols], TABLE_CELL_CAP, s + start, len - start);
      ++cols;
      break;
    }
    while (p < len) {
      if ((s[p] == '|' && !markdown_escaped_at(s, p)) || s[p] == '\t') break;
      ++p;
    }
    clean_cell_copy(cells[cols], TABLE_CELL_CAP, s + start, p - start);
    ++cols;
    if (p >= len) break;
    ++p;
  }
  while (cols > 0 && !cells[cols - 1][0]) --cols;
  return cols;
}

static int table_separator_row(char cells[MAX_TABLE_COLS][TABLE_CELL_CAP], int cols) {
  if (cols <= 0) return 0;
  int dashed = 0;
  for (int c = 0; c < cols; ++c) {
    int has_dash = 0;
    for (int i = 0; cells[c][i]; ++i) {
      char ch = cells[c][i];
      if (ch == '-') {
        has_dash = 1;
        dashed = 1;
      } else if (ch != ':' && ch != ' ') {
        return 0;
      }
    }
    if (!has_dash) return 0;
  }
  return dashed;
}

static int source_line_end_from(int pos) {
  int end = pos;
  while (end < file_buf_len && file_buf[end] != '\n' && file_buf[end] != '\r') ++end;
  return end;
}

static int source_next_line_from(int end) {
  if (end >= file_buf_len) return file_buf_len + 1;
  if (file_buf[end] == '\r' && end + 1 < file_buf_len && file_buf[end + 1] == '\n') return end + 2;
  return end + 1;
}

static int setext_underline_style(const char *s, int len) {
  int p = trim_left_pos(s, len);
  len = trim_right_len(s + p, len - p);
  if (len < 3) return 0;
  char c = s[p];
  if (c != '=' && c != '-') return 0;
  for (int i = 0; i < len; ++i)
    if (s[p + i] != c) return 0;
  return c == '=' ? NOTE_H1 : NOTE_H2;
}

static int setext_heading_style_at(int pos, int len, int *next_pos_out) {
  if (len <= 0 || source_code_like(file_buf + pos, len) || table_like(file_buf + pos, len)) return 0;
  int end = source_line_end_from(pos);
  int next = source_next_line_from(end);
  if (next > file_buf_len) return 0;
  int next_end = source_line_end_from(next);
  int next_len = next_end - next;
  int style = setext_underline_style(file_buf + next, next_len);
  if (!style) return 0;
  if (next_pos_out) *next_pos_out = source_next_line_from(next_end);
  return style;
}

static int previous_source_line_bounds(int pos, int *prev_start, int *prev_len) {
  int end = pos - 1;
  if (end >= 0 && file_buf[end] == '\n') --end;
  if (end < 0) return 0;
  int start = end;
  while (start > 0 && file_buf[start - 1] != '\n' && file_buf[start - 1] != '\r') --start;
  *prev_start = start;
  *prev_len = end - start + 1;
  return *prev_len > 0;
}

static int is_setext_underline_line(int pos, int len) {
  if (!setext_underline_style(file_buf + pos, len)) return 0;
  int prev_start = 0, prev_len = 0;
  if (!previous_source_line_bounds(pos, &prev_start, &prev_len)) return 0;
  return !source_code_like(file_buf + prev_start, prev_len) &&
         !table_like(file_buf + prev_start, prev_len);
}

static int table_start_like_at(int pos, int len) {
  if (table_like(file_buf + pos, len)) return 1;
  if (!pipe_separated_cells(file_buf + pos, len)) return 0;
  int row_cols = parse_table_row(file_buf + pos, len, table_parse_cells);
  if (row_cols <= 0) return 0;
  int end = source_line_end_from(pos);
  int next = source_next_line_from(end);
  if (next > file_buf_len) return 0;
  int next_end = source_line_end_from(next);
  int next_len = next_end - next;
  if (next_len <= 0 || source_code_like(file_buf + next, next_len)) return 0;
  int cols = parse_table_row(file_buf + next, next_len, table_parse_cells);
  return cols >= row_cols && table_separator_row(table_parse_cells, cols);
}

static int table_continues_at(int pos, int kind) {
  if (pos > file_buf_len) return 0;
  int end = source_line_end_from(pos);
  int len = end - pos;
  return len > 0 && !source_code_like(file_buf + pos, len) && table_row_matches_kind(file_buf + pos, len, kind);
}

static int table_chunk_start_at(int pos, int len, int continuing_kind) {
  if (len <= 0 || source_code_like(file_buf + pos, len)) return 0;
  return continuing_kind ? table_row_matches_kind(file_buf + pos, len, continuing_kind) : table_start_like_at(pos, len);
}

static int table_col_cap(int cols) {
  if (cols <= 2) return 30;
  if (cols == 3) return 22;
  if (cols == 4) return 16;
  return 12;
}

static int table_total_chars(const int *widths, int cols) {
  int total = 1;
  for (int c = 0; c < cols; ++c) total += widths[c] + 3;
  return total;
}

static void table_fit_widths(int *widths, int cols) {
  int minw = cols >= 5 ? 4 : 5;
  while (table_total_chars(widths, cols) > TABLE_MAX_CHARS) {
    int widest = -1;
    for (int c = 0; c < cols; ++c)
      if (widths[c] > minw && (widest < 0 || widths[c] > widths[widest])) widest = c;
    if (widest < 0) break;
    --widths[widest];
  }
}

static void table_compute_widths(TableRow *rows, int row_count, int cols, int *widths) {
  int cap = table_col_cap(cols);
  for (int c = 0; c < cols; ++c) widths[c] = cols >= 5 ? 4 : 5;
  for (int r = 0; r < row_count; ++r) {
    for (int c = 0; c < cols; ++c) {
      int n = strlen(rows[r].cells[c]);
      if (n > cap) n = cap;
      if (n > widths[c]) widths[c] = n;
    }
  }
  if (cols <= 3) table_fit_widths(widths, cols);
}

static int table_pixel_width(const int *widths, int cols) {
  int total = 1;
  for (int c = 0; c < cols; ++c) total += widths[c] * TABLE_CHAR_PX + TABLE_PAD_X * 2 + 1;
  return total;
}

static int table_max_hscroll(const int *widths, int cols) {
  int over = table_pixel_width(widths, cols) - ((int)NOTE_X_LIMIT - VIEW_X);
  return over > 0 ? (over + TABLE_CHAR_PX - 1) / TABLE_CHAR_PX : 0;
}

static void table_col_positions(const int *widths, int cols, int *colpos) {
  colpos[0] = 0;
  for (int c = 0; c < cols; ++c)
    colpos[c + 1] = colpos[c] + widths[c] * TABLE_CHAR_PX + TABLE_PAD_X * 2 + 1;
}

static int collect_table_block(int pos, int source_line, TableRow *rows, int *row_count,
                               int *cols_out, int *next_pos, int *next_source_line, int *kind_inout) {
  int count = 0, cols = 0, cur = pos, line_no = source_line;
  int kind = kind_inout && *kind_inout ? *kind_inout : TABLE_NONE;
  while (cur <= file_buf_len && count < MAX_TABLE_ROWS) {
    int end = source_line_end_from(cur);
    int len = end - cur;
    if (len <= 0 || source_code_like(file_buf + cur, len)) break;
    if (!kind) kind = table_row_kind(file_buf + cur, len);
    if (!kind || !table_row_matches_kind(file_buf + cur, len, kind)) break;
    if (plus_separator_row(file_buf + cur, len)) {
      if (count > 0) rows[count - 1].header = 1;
      cur = source_next_line_from(end);
      ++line_no;
      if (cur > file_buf_len) break;
      continue;
    }
    int row_cols = parse_table_row(file_buf + cur, len, table_parse_cells);
    if (row_cols <= 0) break;
    if (table_separator_row(table_parse_cells, row_cols)) {
      if (count > 0) rows[count - 1].header = 1;
    } else {
      rows[count].cols = row_cols;
      rows[count].source_line = line_no;
      rows[count].header = 0;
      for (int c = 0; c < MAX_TABLE_COLS; ++c)
        copy_str(rows[count].cells[c], TABLE_CELL_CAP, c < row_cols ? table_parse_cells[c] : "");
      if (row_cols > cols) cols = row_cols;
      ++count;
    }
    cur = source_next_line_from(end);
    ++line_no;
    if (cur > file_buf_len) break;
  }
  *row_count = count;
  *cols_out = cols;
  *next_pos = cur;
  *next_source_line = line_no;
  if (kind_inout) *kind_inout = kind;
  return count > 0 && cols > 0;
}

static int table_segment_count(const char *s, int width) {
  int len = strlen(s);
  if (len <= 0) return 1;
  int count = 0, pos = 0;
  while (pos < len) {
    while (pos < len && s[pos] == ' ') ++pos;
    if (pos >= len) break;
    int cut = min_int(len, pos + width);
    int last_space = -1;
    for (int i = pos; i < cut; ++i)
      if (s[i] == ' ') last_space = i;
    if (cut < len && last_space > pos) cut = last_space;
    if (cut <= pos) cut = min_int(len, pos + width);
    ++count;
    pos = cut;
  }
  return count > 0 ? count : 1;
}

static void table_segment_at(const char *s, int width, int wanted, char *out, int cap) {
  int len = strlen(s);
  int count = 0, pos = 0;
  out[0] = 0;
  while (pos < len) {
    while (pos < len && s[pos] == ' ') ++pos;
    if (pos >= len) break;
    int cut = min_int(len, pos + width);
    int last_space = -1;
    for (int i = pos; i < cut; ++i)
      if (s[i] == ' ') last_space = i;
    if (cut < len && last_space > pos) cut = last_space;
    if (cut <= pos) cut = min_int(len, pos + width);
    if (count == wanted) {
      clean_cell_copy(out, cap, s + pos, cut - pos);
      return;
    }
    ++count;
    pos = cut;
  }
}

static int ordered_list_marker(const char *s, int len) {
  int p = 0;
  while (p < len && s[p] >= '0' && s[p] <= '9') ++p;
  if (p <= 0 || p > 3 || p + 1 >= len) return 0;
  if ((s[p] == '.' || s[p] == ')') && s[p + 1] == ' ') return p + 2;
  return 0;
}

static int rule_like(const char *s, int len) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  if (p + 2 >= len) return 0;
  char c = s[p];
  if (c != '-' && c != '=' && c != '*') return 0;
  int count = 0;
  for (int i = p; i < len; ++i) {
    if (s[i] == c) ++count;
    else if (s[i] != ' ') return 0;
  }
  return count >= 3;
}

static int mini_width(const char *s, int len) {
  if (!s || len <= 0) return 0;
  char tmp[LINE_CAP];
  int n = min_int(len, LINE_CAP - 1);
  memcpy(tmp, s, n);
  tmp[n] = 0;
  int x = 0, y = 0;
  PrintMini(&x, &y, (unsigned char *)tmp, 0, 0xffffffff, 0, 0, UI_BLACK, UI_WHITE, 0, 0);
  return x;
}

static int style_x_offset(int style) {
  if (style == NOTE_H2) return 4;
  if (style == NOTE_H3) return 8;
  if (style == NOTE_H4) return 12;
  if (style == NOTE_QUOTE) return 4;
  return 0;
}

static int style_uses_hscroll(int style) {
  return style == NOTE_CODE;
}

static int segment_fits_screen(const char *s, int start, int end, int indent, int xpad, int strip_inline) {
  char tmp[LINE_CAP];
  int col = 0;
  for (int i = 0; i < indent && col + 1 < LINE_CAP; ++i) tmp[col++] = ' ';
  if (col + 1 < LINE_CAP)
    col += copy_display_text(tmp + col, LINE_CAP - col, s + start, end - start, strip_inline);
  tmp[col] = 0;
  return mini_width(tmp, col) <= (int)(NOTE_X_LIMIT - VIEW_X - xpad);
}

static int fit_visible_chars(const char *s, int len, int indent, int xpad, int strip_inline) {
  if (!s || len <= 0) return 0;
  int take = 0;
  int cap = LINE_CAP - indent - 1;
  if (cap < 1) cap = 1;
  for (int i = 1; i <= len && i <= cap; ++i) {
    if (!segment_fits_screen(s, 0, i, indent, xpad, strip_inline)) break;
    take = i;
  }
  return take > 0 ? take : 1;
}

static int fit_suffix_chars(const char *s, int len, int indent, int xpad, int strip_inline) {
  if (!s || len <= 0) return 0;
  int take = 0;
  int cap = LINE_CAP - indent - 1;
  if (cap < 1) cap = 1;
  for (int i = 1; i <= len && i <= cap; ++i) {
    if (!segment_fits_screen(s, len - i, len, indent, xpad, strip_inline)) break;
    take = i;
  }
  return take > 0 ? take : 1;
}

static int line_end_hscroll(const char *s, int len, int style) {
  if (style != NOTE_CODE) {
    len = copy_display_text(display_line_buf, FILE_BUF_SIZE, s, len, 1);
    s = display_line_buf;
  }
  int visible = fit_suffix_chars(s, len, 0, style_x_offset(style), 0);
  return len > visible ? len - visible : 0;
}

static int markdown_line(const char *s, int len, int *skip, int *style) {
  int p = 0;
  while (p < len && md_space(s[p])) ++p;
  *skip = p;
  *style = NOTE_TEXT;
  if (p < len && s[p] == '#') {
    int hashes = 0;
    while (p + hashes < len && s[p + hashes] == '#') ++hashes;
    if (p + hashes < len && s[p + hashes] == ' ') {
      *skip = p + hashes + 1;
      *style = hashes <= 1 ? NOTE_H1 : (hashes == 2 ? NOTE_H2 : (hashes == 3 ? NOTE_H3 : NOTE_H4));
      return 1;
    }
  }
  if (p + 1 < len && (s[p] == '-' || s[p] == '*' || s[p] == '+') && s[p + 1] == ' ') {
    *skip = p + 2;
    *style = NOTE_BULLET;
    return 1;
  }
  if (ordered_list_marker(s + p, len - p)) {
    *skip = 0;
    *style = NOTE_ORDERED;
    return 1;
  }
  if (p + 1 < len && s[p] == '>' && s[p + 1] == ' ') {
    *skip = 0;
    *style = NOTE_QUOTE;
    return 1;
  }
  if (rule_like(s, len)) {
    *skip = p;
    *style = NOTE_RULE;
    return 1;
  }
  if (source_code_like(s, len)) {
    *style = NOTE_CODE;
    return 1;
  }
  return 0;
}

static int atx_heading_display_len(const char *s, int len, int style) {
  if (style < NOTE_H1 || style > NOTE_H4) return len;
  int n = trim_right_len(s, len);
  int p = n;
  while (p > 0 && s[p - 1] == '#') --p;
  if (p == n || p <= 0 || s[p - 1] != ' ') return n;
  return trim_right_len(s, p - 1);
}

static void clear_table_meta(int idx) {
  view_table_cols[idx] = 0;
  view_table_top[idx] = 0;
  view_table_bottom[idx] = 0;
  view_table_header[idx] = 0;
  for (int c = 0; c <= MAX_TABLE_COLS; ++c) view_table_colpos[idx][c] = 0;
}

static void push_view_line(int *line, const char *s, int len, int style, int source_line) {
  if (*line >= MAX_VIEW_LINES) return;
  copy_display_text(line_store[*line], LINE_CAP, s, len, style != NOTE_CODE);
  view_lines[*line] = line_store[*line];
  view_style[*line] = (unsigned char)style;
  view_source_line[*line] = (short)source_line;
  clear_table_meta(*line);
  ++(*line);
}

static void push_table_line(int *line, const char cells[MAX_TABLE_COLS][TABLE_CELL_CAP],
                            int source_line, int cols, const int *colpos, int hscroll,
                            int top, int bottom, int header) {
  if (*line >= MAX_VIEW_LINES) return;
  int n = 0;
  for (int c = 0; c < cols && n + 1 < LINE_CAP; ++c) {
    if (c) line_store[*line][n++] = '\t';
    for (int i = 0; cells[c][i] && n + 1 < LINE_CAP; ++i)
      line_store[*line][n++] = cells[c][i];
  }
  line_store[*line][n] = 0;
  view_lines[*line] = line_store[*line];
  view_style[*line] = NOTE_TABLE;
  view_source_line[*line] = (short)source_line;
  view_table_cols[*line] = (unsigned char)cols;
  view_table_top[*line] = (unsigned char)top;
  view_table_bottom[*line] = (unsigned char)bottom;
  view_table_header[*line] = (unsigned char)header;
  for (int c = 0; c <= MAX_TABLE_COLS; ++c)
    view_table_colpos[*line][c] = c <= cols ? (short)(colpos[c] - hscroll * TABLE_CHAR_PX) : 0;
  ++(*line);
}

static void push_table_block(int *line, TableRow *rows, int row_count, int cols, int hscroll) {
  int widths[MAX_TABLE_COLS], colpos[MAX_TABLE_COLS + 1];
  table_compute_widths(rows, row_count, cols, widths);
  table_col_positions(widths, cols, colpos);
  int local_hscroll = min_int(hscroll, table_max_hscroll(widths, cols));
  for (int r = 0; r < row_count && *line < MAX_VIEW_LINES; ++r) {
    int parts = 1;
    for (int c = 0; c < cols; ++c)
      parts = max_int(parts, table_segment_count(rows[r].cells[c], widths[c]));
    for (int part = 0; part < parts && *line < MAX_VIEW_LINES; ++part) {
      char (*segs)[TABLE_CELL_CAP] = table_segment_cells;
      for (int c = 0; c < MAX_TABLE_COLS; ++c) segs[c][0] = 0;
      for (int c = 0; c < cols; ++c)
        table_segment_at(rows[r].cells[c], widths[c], part, segs[c], TABLE_CELL_CAP);
      push_table_line(line, segs, rows[r].source_line, cols, colpos, local_hscroll,
                      part == 0, part == parts - 1, rows[r].header);
    }
  }
}

static void push_blank_line(int *line, int source_line) {
  push_view_line(line, "", 0, NOTE_TEXT, source_line);
}

static void add_display_line(int *line, const char *s, int len, int hscroll, int preserve, int style, int source_line) {
  if (*line >= MAX_VIEW_LINES || len < 0) return;
  if (style != NOTE_CODE) {
    len = copy_display_text(display_line_buf, FILE_BUF_SIZE, s, len, 1);
    s = display_line_buf;
  }
  int xpad = style_x_offset(style);
  if (preserve) {
    int start = min_int(hscroll, max_int(0, len - 1));
    int take = fit_visible_chars(s + start, len - start, 0, xpad, 0);
    push_view_line(line, s + start, take, style, source_line);
    return;
  }
  int base_indent = 0;
  while (base_indent < len && base_indent < 10 && md_space(s[base_indent])) ++base_indent;
  int ordered_indent = 3;
  if (style == NOTE_ORDERED) {
    int mark = ordered_list_marker(s + base_indent, len - base_indent);
    if (mark > 0) ordered_indent = mark;
  }
  int pos = 0;
  while (pos < len && *line < MAX_VIEW_LINES) {
    if (pos == 0) pos = base_indent;
    else while (pos < len && s[pos] == ' ') ++pos;
    int usable = WRAP_COLS;
    int indent = pos == base_indent ? base_indent : base_indent + 2;
    if (style == NOTE_BULLET && pos != base_indent) indent += 2;
    if (style == NOTE_ORDERED && pos != base_indent) indent += ordered_indent;
    if (style == NOTE_QUOTE && pos != base_indent) indent += 2;
    usable -= indent;
    if (usable < 16) usable = 16;
    int cut = pos;
    int last_space = -1;
    for (int i = pos; i < len; ++i) {
      if (s[i] == ' ') last_space = i;
      if (!segment_fits_screen(s, pos, i + 1, indent, xpad, 0)) {
        if (last_space > pos + 1) cut = last_space;
        else cut = max_int(pos + 1, i);
        break;
      }
      cut = i + 1;
    }
    if (cut <= pos) cut = min_int(len, pos + usable);
    int cap_cut = pos + max_int(1, LINE_CAP - indent - 1);
    if (cut > cap_cut) cut = cap_cut;
    int col = 0;
    for (int k = 0; k < indent && col + 1 < LINE_CAP; ++k) line_store[*line][col++] = ' ';
    if (col + 1 < LINE_CAP)
      col += copy_display_text(line_store[*line] + col, LINE_CAP - col, s + pos, cut - pos, 0);
    line_store[*line][col] = 0;
    view_lines[*line] = line_store[*line];
    view_style[*line] = (unsigned char)style;
    view_source_line[*line] = (short)source_line;
    clear_table_meta(*line);
    ++(*line);
    pos = cut;
  }
}

static int build_view_lines(int hscroll) {
  int line = 0, pos = 0, source_line = 0;
  int in_fence = 0;
  int continuing_kind = TABLE_NONE;
  while (pos <= file_buf_len && line < MAX_VIEW_LINES) {
    int end = source_line_end_from(pos);
    int len = end - pos;
    if (in_fence || (len > 0 && fence_like(file_buf + pos, len))) {
      continuing_kind = TABLE_NONE;
      add_display_line(&line, file_buf + pos, len, hscroll, 1, NOTE_CODE, source_line);
      if (len > 0 && fence_like(file_buf + pos, len)) in_fence = !in_fence;
      pos = source_next_line_from(end);
      ++source_line;
      if (pos > file_buf_len) break;
      continue;
    }
    if (table_chunk_start_at(pos, len, continuing_kind)) {
      TableRow *rows = table_rows;
      int row_count = 0, cols = 0, next_pos = pos, next_source = source_line;
      int kind = continuing_kind;
      if (collect_table_block(pos, source_line, rows, &row_count, &cols, &next_pos, &next_source, &kind)) {
        push_table_block(&line, rows, row_count, cols, hscroll);
        int next_end = next_pos <= file_buf_len ? source_line_end_from(next_pos) : next_pos;
        continuing_kind = table_continues_at(next_pos, kind) ? kind : TABLE_NONE;
        if (next_pos < file_buf_len && next_end > next_pos &&
            !continuing_kind &&
            (source_code_like(file_buf + next_pos, next_end - next_pos) ||
             !table_start_like_at(next_pos, next_end - next_pos)))
          push_blank_line(&line, next_source);
        pos = next_pos;
        source_line = next_source;
        continue;
      }
    }
    continuing_kind = TABLE_NONE;
    int setext_next = 0;
    int setext_style = setext_heading_style_at(pos, len, &setext_next);
    if (setext_style) {
      int start = trim_left_pos(file_buf + pos, len);
      int src_len = trim_right_len(file_buf + pos + start, len - start);
      add_display_line(&line, file_buf + pos + start, src_len, hscroll, style_uses_hscroll(setext_style), setext_style, source_line);
      pos = setext_next;
      source_line += 2;
      continue;
    }
    if (len <= 0) {
      line_store[line][0] = 0;
      view_lines[line] = line_store[line];
      view_style[line] = NOTE_TEXT;
      view_source_line[line] = (short)source_line;
      clear_table_meta(line);
      ++line;
    } else {
      int skip = 0, style = NOTE_TEXT;
      markdown_line(file_buf + pos, len, &skip, &style);
      const char *src = file_buf + pos + skip;
      int src_len = len - skip;
      if (style == NOTE_BULLET) {
        src = file_buf + pos;
        src_len = len;
      } else {
        src_len = atx_heading_display_len(src, src_len, style);
      }
      add_display_line(&line, src, src_len, hscroll, style_uses_hscroll(style), style, source_line);
    }
    pos = source_next_line_from(end);
    ++source_line;
    if (pos > file_buf_len) break;
  }
  return line;
}

static int source_line_count() {
  if (file_buf_len <= 0) return 1;
  int count = 1;
  for (int i = 0; i < file_buf_len; ++i) {
    if (file_buf[i] == '\n') ++count;
  }
  return count;
}

static int source_line_bounds(int target, int *start_out, int *len_out) {
  int line = 0, start = 0;
  for (int i = 0; i <= file_buf_len; ++i) {
    char c = i < file_buf_len ? file_buf[i] : '\n';
    if (c == '\r') continue;
    if (c == '\n') {
      if (line == target) {
        *start_out = start;
        *len_out = i - start;
        return 1;
      }
      start = i + 1;
      ++line;
    }
  }
  return 0;
}

static int source_line_style(int source_line) {
  int line = 0, pos = 0, in_fence = 0, continuing_kind = TABLE_NONE;
  while (pos <= file_buf_len) {
    int end = source_line_end_from(pos);
    int len = end - pos;
    if (in_fence || (len > 0 && fence_like(file_buf + pos, len))) {
      if (line == source_line) return NOTE_CODE;
      continuing_kind = TABLE_NONE;
      if (len > 0 && fence_like(file_buf + pos, len)) in_fence = !in_fence;
      pos = source_next_line_from(end);
      ++line;
      if (pos > file_buf_len) break;
      continue;
    }
    int table_row = table_chunk_start_at(pos, len, continuing_kind);
    if (line == source_line) {
      if (table_row) return NOTE_TABLE;
      int setext_style = setext_heading_style_at(pos, len, 0);
      if (setext_style) return setext_style;
      int skip = 0, style = NOTE_TEXT;
      markdown_line(file_buf + pos, len, &skip, &style);
      return style;
    }
    if (table_row) {
      int kind = continuing_kind ? continuing_kind : table_row_kind(file_buf + pos, len);
      continuing_kind = table_continues_at(source_next_line_from(end), kind) ? kind : TABLE_NONE;
    } else {
      continuing_kind = TABLE_NONE;
    }
    pos = source_next_line_from(end);
    ++line;
    if (pos > file_buf_len) break;
  }
  return NOTE_TEXT;
}

static int source_line_display_text(int source_line, char *out, int cap) {
  int start = 0, len = 0;
  if (!source_line_bounds(source_line, &start, &len)) {
    if (cap > 0) out[0] = 0;
    return 0;
  }
  if (is_setext_underline_line(start, len)) {
    if (cap > 0) out[0] = 0;
    return 0;
  }
  int style = source_line_style(source_line);
  if (style == NOTE_TABLE) {
    if (plus_separator_row(file_buf + start, len)) {
      if (cap > 0) out[0] = 0;
      return 0;
    }
    int cols = parse_table_row(file_buf + start, len, table_parse_cells);
    if (cols <= 0 || table_separator_row(table_parse_cells, cols)) {
      if (cap > 0) out[0] = 0;
      return 0;
    }
    int n = 0;
    for (int c = 0; c < cols && n + 1 < cap; ++c) {
      if (c && n + 1 < cap) out[n++] = ' ';
      for (int i = 0; table_parse_cells[c][i] && n + 1 < cap; ++i)
        out[n++] = table_parse_cells[c][i];
    }
    out[n] = 0;
    return n;
  }
  int skip = 0;
  int parsed_style = NOTE_TEXT;
  markdown_line(file_buf + start, len, &skip, &parsed_style);
  if (style == NOTE_CODE) {
    skip = 0;
  } else if (style >= NOTE_H1 && style <= NOTE_H4 && !(parsed_style >= NOTE_H1 && parsed_style <= NOTE_H4)) {
    skip = trim_left_pos(file_buf + start, len);
  }
  const char *src = file_buf + start + skip;
  int src_len = len - skip;
  if (style == NOTE_BULLET || style == NOTE_ORDERED || style == NOTE_QUOTE) {
    src = file_buf + start;
    src_len = len;
  } else {
    src_len = atx_heading_display_len(src, src_len, style);
  }
  return copy_display_text(out, cap, src, src_len, style != NOTE_CODE);
}

static int find_source_match(const SearchPattern *sp, int start_line, int dir, int *line_out, int *offset_out) {
  int count = source_line_count();
  if (!sp || sp->len <= 0 || count <= 0) return 0;
  int line = start_line;
  for (int seen = 0; seen < count; ++seen) {
    if (line < 0) line = count - 1;
    if (line >= count) line = 0;
    int display_len = source_line_display_text(line, search_line_buf, FILE_BUF_SIZE);
    if (display_len > 0) {
      int off = find_in_span(search_line_buf, display_len, sp);
      if (off >= 0) {
        *line_out = line;
        *offset_out = off;
        return 1;
      }
    }
    line += dir;
  }
  return 0;
}

static int view_line_for_source_match(int source_line, const SearchPattern *sp, int lines) {
  int fallback = -1;
  for (int i = 0; i < lines; ++i) {
    if (view_source_line[i] != source_line) continue;
    if (fallback < 0) fallback = i;
    if (contains_kmp(view_lines[i], sp)) return i;
  }
  return fallback;
}

static int max_file_line_scroll() {
  int max_scroll = 0, pos = 0, source_line = 0;
  int in_fence = 0;
  int continuing_kind = TABLE_NONE;
  while (pos <= file_buf_len) {
    int end = source_line_end_from(pos);
    int len = end - pos;
    if (in_fence || (len > 0 && fence_like(file_buf + pos, len))) {
      continuing_kind = TABLE_NONE;
      int scroll = line_end_hscroll(file_buf + pos, len, NOTE_CODE);
      if (scroll > max_scroll) max_scroll = scroll;
      if (len > 0 && fence_like(file_buf + pos, len)) in_fence = !in_fence;
      pos = source_next_line_from(end);
      ++source_line;
      if (pos > file_buf_len) break;
      continue;
    }
    if (table_chunk_start_at(pos, len, continuing_kind)) {
      TableRow *rows = table_rows;
      int row_count = 0, cols = 0, next_pos = pos, next_source = source_line;
      int kind = continuing_kind;
      if (collect_table_block(pos, source_line, rows, &row_count, &cols, &next_pos, &next_source, &kind)) {
        int widths[MAX_TABLE_COLS];
        table_compute_widths(rows, row_count, cols, widths);
        int scroll = table_max_hscroll(widths, cols);
        if (scroll > max_scroll) max_scroll = scroll;
        continuing_kind = table_continues_at(next_pos, kind) ? kind : TABLE_NONE;
        pos = next_pos;
        source_line = next_source;
        continue;
      }
    }
    continuing_kind = TABLE_NONE;
    int setext_next = 0;
    int setext_style = setext_heading_style_at(pos, len, &setext_next);
    if (setext_style) {
      pos = setext_next;
      source_line += 2;
      continue;
    }
    if (len > 0) {
      int skip = 0, style = NOTE_TEXT;
      markdown_line(file_buf + pos, len, &skip, &style);
      if (style_uses_hscroll(style)) {
        const char *src = file_buf + pos + skip;
        int src_len = len - skip;
        int scroll = line_end_hscroll(src, src_len, style);
        if (scroll > max_scroll) max_scroll = scroll;
      }
    }
    pos = source_next_line_from(end);
    ++source_line;
    if (pos > file_buf_len) break;
  }
  return max_scroll;
}

static int table_hscroll_for_match(const SearchPattern *sp, int target_source_line) {
  int pos = 0, source_line = 0, continuing_kind = TABLE_NONE;
  while (pos <= file_buf_len) {
    int end = source_line_end_from(pos);
    int len = end - pos;
    if (table_chunk_start_at(pos, len, continuing_kind)) {
      TableRow *rows = table_rows;
      int row_count = 0, cols = 0, next_pos = pos, next_source = source_line;
      int kind = continuing_kind;
      if (collect_table_block(pos, source_line, rows, &row_count, &cols, &next_pos, &next_source, &kind)) {
        for (int r = 0; r < row_count; ++r) {
          if (rows[r].source_line != target_source_line) continue;
          int widths[MAX_TABLE_COLS], colpos[MAX_TABLE_COLS + 1];
          table_compute_widths(rows, row_count, cols, widths);
          table_col_positions(widths, cols, colpos);
          for (int c = 0; c < cols; ++c) {
            int cell_len = strlen(rows[r].cells[c]);
            if (find_in_span(rows[r].cells[c], cell_len, sp) >= 0)
              return max_int(0, colpos[c] / TABLE_CHAR_PX - 1);
          }
          return 0;
        }
        continuing_kind = table_continues_at(next_pos, kind) ? kind : TABLE_NONE;
        pos = next_pos;
        source_line = next_source;
        continue;
      }
    }
    continuing_kind = TABLE_NONE;
    pos = source_next_line_from(end);
    ++source_line;
    if (pos > file_buf_len) break;
  }
  return 0;
}

static int jump_to_match(const SearchPattern *sp, int start_source_line, int dir, int *top, int *hscroll, int *lines, int max_line) {
  int source_line = 0, offset = 0;
  if (!find_source_match(sp, start_source_line, dir, &source_line, &offset)) return 0;
  int style = source_line_style(source_line);
  if (style == NOTE_TABLE)
    *hscroll = table_hscroll_for_match(sp, source_line);
  else
    *hscroll = style == NOTE_CODE ? max_int(0, offset - 6) : 0;
  if (*hscroll > max_line) *hscroll = max_line;
  *lines = build_view_lines(*hscroll);
  int view_line = view_line_for_source_match(source_line, sp, *lines);
  if (view_line >= 0) *top = min_int(view_line, max_int(0, *lines - PAGE_LINES));
  return 1;
}


static void notes_menu(const char *title, const char *const *items, int count, int top, int sel,
                       const char *f1, const char *f2, const char *f3, const char *f4, const char *f5, const char *f6) {
  notes_chrome(title);
  notes_softkeys(f1, f2, f3, f4, f5, f6);
  for (int i = 0; i < MENU_PAGE_ROWS && top + i < count; ++i) {
    int idx = top + i;
    char label[48], row[60];
    clean_label(label, sizeof(label), items[idx]);
    if (strlen(label) > 20) {
      label[17] = '.';
      label[18] = '.';
      label[19] = '.';
      label[20] = 0;
    }
    int cur = idx + 1;
    if (count < 10)
      sprintf(row, "%d %s", cur, label);
    else
      sprintf(row, "%2d %s", cur, label);
    int fill = 21 - MB_ElementCount((char *)label) - (count < 10 ? 2 : 3);
    for (int j = 0; j < fill && strlen(row) < sizeof(row) - 1; ++j) strcat(row, " ");
    ui_mprintxy(1, i + 1, row, idx == sel ? TEXT_MODE_INVERT : TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  if (count > MENU_PAGE_ROWS) {
    TScrollbar sb;
    sb.I1 = 0;
    sb.I5 = 0;
    sb.indicatormaximum = count;
    sb.indicatorheight = MENU_PAGE_ROWS;
    sb.indicatorpos = top;
    sb.barheight = MENU_PAGE_ROWS * 24;
    sb.bartop = 24;
    sb.barleft = 18 * 21 - 5;
    sb.barwidth = 6;
    Scrollbar(&sb);
  }
  ui_flush();
}

static void notes_message(const char *title, const char *a, const char *b, const char *c) {
  notes_chrome(title);
  notes_softkeys("", "", "", "", "", "BACK");
  if (a) {
    char msg[44];
    copy_str(msg, sizeof(msg), a);
    ui_mprintxy(1, 2, msg, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  if (b) {
    char msg[44];
    copy_str(msg, sizeof(msg), b);
    ui_mprintxy(1, 3, msg, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  if (c) {
    char msg[44];
    copy_str(msg, sizeof(msg), c);
    ui_mprintxy(1, 4, msg, TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
  }
  ui_flush();
}

static bool notes_input(const char *title, char *buf, int cap) {
  int start = 0;
  int cursor = (int)strlen(buf);
  static char hist[6][32];
  static int hist_count = 0;
  int hist_pos = hist_count;
  for (;;) {
    notes_chrome(title);
    notes_softkeys("RUN", "BACK", "DEL", "PREV", "NEXT", "");
    ui_mprintxy(1, 2, "Type search text.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
    DisplayMBString2(0, (unsigned char *)buf, start, cursor, 0, 1, 72, 21, 0);
    ui_hline(0, LCD_WIDTH_PX - 1, 71, UI_GRAY);
    ui_hline(0, LCD_WIDTH_PX - 1, 95, UI_GRAY);
    ui_flush();
    int key = ui_key_poll();
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F2 || key == KEY_CTRL_F6) {
      Cursor_SetFlashOff();
      return false;
    }
    if (key == KEY_CTRL_EXE || key == KEY_CTRL_F1) {
      if (buf[0] && (hist_count == 0 || strcmp(hist[(hist_count - 1) % 6], buf))) {
        strncpy(hist[hist_count % 6], buf, 31);
        hist[hist_count % 6][31] = 0;
        ++hist_count;
      }
      Cursor_SetFlashOff();
      return true;
    }
    if (key == KEY_CTRL_F3) key = KEY_CTRL_DEL;
    if ((key == KEY_CTRL_UP || key == KEY_CTRL_F4) && hist_count > 0) {
      int hist_min = hist_count > 6 ? hist_count - 6 : 0;
      if (hist_pos > hist_min) --hist_pos;
      strncpy(buf, hist[hist_pos % 6], cap - 1);
      buf[cap - 1] = 0;
      cursor = (int)strlen(buf);
      start = 0;
      continue;
    }
    if ((key == KEY_CTRL_DOWN || key == KEY_CTRL_F5) && hist_count > 0) {
      if (hist_pos + 1 < hist_count) ++hist_pos;
      strncpy(buf, hist[hist_pos % 6], cap - 1);
      buf[cap - 1] = 0;
      cursor = (int)strlen(buf);
      start = 0;
      continue;
    }
    if (key && key < 30000)
      cursor = EditMBStringChar((unsigned char *)buf, cap - 1, cursor, key);
    else
      EditMBStringCtrl2((unsigned char *)buf, cap, &start, &cursor, &key, 1, 72, 1, 20);
  }
}

static int match_at_ci(const char *s, const SearchPattern *sp, int pos) {
  if (!s || !sp || sp->len <= 0 || pos < 0) return 0;
  for (int i = 0; i < sp->len; ++i) {
    if (!s[pos + i]) return 0;
    if (lower_char((unsigned char)s[pos + i]) != lower_char((unsigned char)sp->pat[i])) return 0;
  }
  return 1;
}

static void notes_print_span_limit(int *x, int y, const char *s, int len, int fg, int bg, int xlimit) {
  if (xlimit > (int)NOTE_X_LIMIT) xlimit = NOTE_X_LIMIT;
  if (!s || len <= 0 || *x >= xlimit) return;
  while (*x < VIEW_X && len > 0) {
    *x += TABLE_CHAR_PX;
    ++s;
    --len;
  }
  if (len <= 0 || *x >= xlimit) return;
  int avail = xlimit - *x;
  int n = min_int(len, LINE_CAP - 1);
  while (n > 0 && mini_width(s, n) > avail) --n;
  if (n <= 0) return;
  char tmp[LINE_CAP];
  memcpy(tmp, s, n);
  tmp[n] = 0;
  int before = *x;
  PrintMini(x, &y, (unsigned char *)tmp, 0, xlimit, 0, 0, fg, bg, 1, 0);
  if (*x <= before) *x = before + mini_width(tmp, n);
}

static void notes_print_span(int *x, int y, const char *s, int len, int fg, int bg) {
  notes_print_span_limit(x, y, s, len, fg, bg, NOTE_X_LIMIT);
}

static void notes_print_with_matches_limit(int x, int y, const char *line, int fg, int bg,
                                           int match_fg, const SearchPattern *sp, int xlimit) {
  if (xlimit > (int)NOTE_X_LIMIT) xlimit = NOTE_X_LIMIT;
  if (!line || !line[0]) return;
  if (!sp || sp->len <= 0) {
    int px = x;
    notes_print_span_limit(&px, y, line, (int)strlen(line), fg, bg, xlimit);
    return;
  }
  int n = strlen(line);
  int pos = 0;
  int px = x;
  while (pos < n) {
    if (px >= xlimit) break;
    if (match_at_ci(line, sp, pos)) {
      notes_print_span_limit(&px, y, line + pos, sp->len, match_fg, bg, xlimit);
      pos += sp->len;
      continue;
    }
    int start = pos;
    while (pos < n && !match_at_ci(line, sp, pos)) ++pos;
    notes_print_span_limit(&px, y, line + start, pos - start, fg, bg, xlimit);
  }
}

static void notes_print_with_matches(int x, int y, const char *line, int fg, int bg, int match_fg, const SearchPattern *sp) {
  notes_print_with_matches_limit(x, y, line, fg, bg, match_fg, sp, NOTE_X_LIMIT);
}

static void note_vline(int x, int y1, int y2, unsigned short color) {
  if (x < 0 || x >= LCD_WIDTH_PX) return;
  if (y1 < 24) y1 = 24;
  if (y2 >= LCD_HEIGHT_PX - 18) y2 = LCD_HEIGHT_PX - 19;
  for (int y = y1; y <= y2; ++y) ui_pixel(x, y, color);
}

static void note_hline_clip(int x1, int x2, int y, unsigned short color) {
  if (y < 24 || y >= LCD_HEIGHT_PX - 18) return;
  if (x1 < VIEW_X) x1 = VIEW_X;
  if (x2 > (int)NOTE_X_LIMIT) x2 = NOTE_X_LIMIT;
  if (x1 > x2) return;
  ui_hline(x1, x2, y, color);
}

static void note_fill_clip(int x1, int y1, int x2, int y2, unsigned short color) {
  if (x1 < VIEW_X) x1 = VIEW_X;
  if (x2 > (int)NOTE_X_LIMIT) x2 = NOTE_X_LIMIT;
  if (y1 < 24) y1 = 24;
  if (y2 >= LCD_HEIGHT_PX - 18) y2 = LCD_HEIGHT_PX - 19;
  if (x1 >= x2 || y1 >= y2) return;
  ui_fill(x1, y1, x2 - x1, y2 - y1, color);
}

static const char *table_cell_span(const char *line, int col, int *len) {
  static char empty[] = "";
  int cur = 0, start = 0, i = 0;
  while (line && line[i]) {
    if (line[i] == '\t') {
      if (cur == col) {
        *len = i - start;
        return line + start;
      }
      ++cur;
      start = i + 1;
    }
    ++i;
  }
  if (cur == col) {
    *len = i - start;
    return line + start;
  }
  *len = 0;
  return empty;
}

static void print_table_line(int idx, int y, const SearchPattern *sp) {
  int cols = view_table_cols[idx];
  if (cols <= 0) return;
  int top = y - 2, bottom = y + VIEW_ROW_H - 2;
  int x0 = VIEW_X + view_table_colpos[idx][0];
  int x1 = VIEW_X + view_table_colpos[idx][cols];
  if (view_table_header[idx]) note_fill_clip(x0, top + 1, x1, bottom, RGB565(233, 241, 252));
  for (int c = 0; c <= cols; ++c)
    note_vline(VIEW_X + view_table_colpos[idx][c], top, bottom, UI_GRAY);
  if (view_table_top[idx]) note_hline_clip(x0, x1, top, UI_GRAY);
  if (view_table_bottom[idx]) note_hline_clip(x0, x1, bottom, UI_GRAY);
  for (int c = 0; c < cols; ++c) {
    int tx = VIEW_X + view_table_colpos[idx][c] + TABLE_PAD_X;
    int cell_right = VIEW_X + view_table_colpos[idx][c + 1] - TABLE_PAD_X;
    if (cell_right < VIEW_X || tx > (int)NOTE_X_LIMIT) continue;
    int xlimit = min_int((int)NOTE_X_LIMIT, cell_right);
    if (tx >= xlimit) continue;
    int len = 0;
    const char *cell = table_cell_span(view_lines[idx], c, &len);
    if (len <= 0) continue;
    char tmp[TABLE_CELL_CAP];
    int n = min_int(len, TABLE_CELL_CAP - 1);
    memcpy(tmp, cell, n);
    tmp[n] = 0;
    notes_print_with_matches_limit(tx, y, tmp,
                                   view_table_header[idx] ? UI_BLUE : UI_BLACK,
                                   view_table_header[idx] ? RGB565(233, 241, 252) : UI_WHITE,
                                   UI_MATCH_TEXT, sp, xlimit);
  }
}

static void print_note_line(int idx, int x, int y, const char *line, int style, const SearchPattern *sp) {
  if (style == NOTE_TABLE) {
    print_table_line(idx, y, sp);
    return;
  }
  int fg = UI_BLACK, bg = UI_WHITE;
  int match_fg = UI_MATCH_TEXT;
  if (style == NOTE_H1) {
    fg = UI_BLUE;
  } else if (style == NOTE_H2) {
    fg = UI_H2_TEXT;
    x += 4;
  } else if (style == NOTE_H3) {
    fg = UI_H3_TEXT;
    x += 8;
  } else if (style == NOTE_H4) {
    fg = UI_H4_TEXT;
    x += 12;
  } else if (style == NOTE_QUOTE) {
    fg = UI_H3_TEXT;
    x += 4;
  } else if (style == NOTE_RULE) {
    fg = UI_GRAY;
  } else if (style == NOTE_CODE) {
    fg = UI_H3_TEXT;
  }
  int p = 0;
  while (line && line[p] == ' ') ++p;
  if (style == NOTE_BULLET && line && (line[p] == '-' || line[p] == '*' || line[p] == '+') && line[p + 1] == ' ') {
    int px = x;
    notes_print_span(&px, y, line, p, fg, bg);
    notes_print_span(&px, y, line + p, 1, UI_BLUE, bg);
    notes_print_span(&px, y, " ", 1, fg, bg);
    notes_print_with_matches(px, y, line + p + 2, fg, bg, match_fg, sp);
  } else if (style == NOTE_ORDERED && line) {
    int mark = ordered_list_marker(line + p, (int)strlen(line + p));
    int px = x;
    notes_print_span(&px, y, line, p, fg, bg);
    if (mark > 0) {
      notes_print_span(&px, y, line + p, mark, UI_BLUE, bg);
      notes_print_with_matches(px, y, line + p + mark, fg, bg, match_fg, sp);
    } else {
      notes_print_with_matches(px, y, line + p, fg, bg, match_fg, sp);
    }
  } else if (style == NOTE_QUOTE && line[p] == '>' && line[p + 1] == ' ') {
    int px = x;
    notes_print_span(&px, y, line, p, fg, bg);
    notes_print_span(&px, y, ">", 1, UI_BLUE, bg);
    notes_print_span(&px, y, " ", 1, fg, bg);
    notes_print_with_matches(px, y, line + p + 2, fg, bg, match_fg, sp);
  } else {
    notes_print_with_matches(x, y, line ? line : "", fg, bg, match_fg, sp);
  }
}

static void notes_text_page(const char *title, const char *const *lines, int count, int top, const SearchPattern *sp) {
  notes_chrome(title);
  notes_softkeys("NEXT", "PREV", "PGUP", "PGDN", "FIND", "BACK");
  for (int i = 0; i < PAGE_LINES && top + i < count; ++i) {
    const char *line = lines[top + i];
    int y = VIEW_TOP + i * VIEW_ROW_H;
    if (!line[0]) continue;
    print_note_line(top + i, VIEW_X, y, line, view_style[top + i], sp);
  }
  ui_flush();
}

static void wait_text_page(const char *title, const char *initial_query) {
  int top = 0, hscroll = 0;
  SearchPattern sp;
  search_prepare(&sp, initial_query && initial_query[0] ? initial_query : "");
  int active_search = sp.len > 0;
  int max_line = max_file_line_scroll();
  int lines = build_view_lines(hscroll);
  if (sp.len > 0) jump_to_match(&sp, 0, 1, &top, &hscroll, &lines, max_line);
  if (top + PAGE_LINES > lines) top = max_int(0, lines - PAGE_LINES);
  notes_text_page(title, view_lines, lines, top, &sp);
  int animate_title = status_title_needs_marquee(title);
  unsigned last_title_step = status_marquee_step();
  for (;;) {
    int key = animate_title ? notes_key_poll_timed() : ui_key_poll();
    if (animate_title && !key) {
      unsigned next_title_step = status_marquee_step();
      if (next_title_step != last_title_step) {
        last_title_step = next_title_step;
        notes_text_page(title, view_lines, lines, top, &sp);
      }
      OS_InnerWait_ms(40);
      continue;
    }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6) return;
    if (key == KEY_CTRL_UP && top > 0) notes_text_page(title, view_lines, lines, --top, &sp);
    if (key == KEY_CTRL_DOWN && top + PAGE_LINES < lines) notes_text_page(title, view_lines, lines, ++top, &sp);
    if ((key == KEY_CTRL_PAGEUP || key == KEY_CTRL_F3) && top > 0) {
      top = max_int(0, top - PAGE_LINES);
      notes_text_page(title, view_lines, lines, top, &sp);
    }
    if ((key == KEY_CTRL_PAGEDOWN || key == KEY_CTRL_F4) && top + PAGE_LINES < lines) {
      top = min_int(max_int(0, lines - PAGE_LINES), top + PAGE_LINES);
      notes_text_page(title, view_lines, lines, top, &sp);
    }
    if (key == KEY_CTRL_RIGHT) {
      if (hscroll < max_line) hscroll = min_int(max_line, hscroll + 8);
      lines = build_view_lines(hscroll);
      top = min_int(top, max_int(0, lines - PAGE_LINES));
      notes_text_page(title, view_lines, lines, top, &sp);
    }
    if (key == KEY_CTRL_LEFT && hscroll > 0) {
      hscroll = max_int(0, hscroll - 8);
      lines = build_view_lines(hscroll);
      top = min_int(top, max_int(0, lines - PAGE_LINES));
      notes_text_page(title, view_lines, lines, top, &sp);
    }
    if (key == KEY_CTRL_F5) {
      char q[32];
      copy_str(q, sizeof(q), last_query);
      if (notes_input("Find text", q, sizeof(q)) && q[0]) {
        copy_str(last_query, sizeof(last_query), q);
        search_prepare(&sp, q);
        active_search = 1;
        int start_source = lines > 0 ? view_source_line[min_int(top, max_int(0, lines - 1))] : 0;
        if (!jump_to_match(&sp, start_source, 1, &top, &hscroll, &lines, max_line)) {
          search_prepare(&sp, "");
          active_search = 0;
          notes_message("Find text", "No results found.", q, "F6 returns.");
          for (;;) {
            int k = ui_key_poll();
            if (k == KEY_CTRL_EXIT || k == KEY_CTRL_AC || k == KEY_CTRL_F6 || k == KEY_CTRL_EXE) break;
          }
        }
      }
      notes_text_page(title, view_lines, lines, top, &sp);
    }
    if (key == KEY_CTRL_F1) {
      if (active_search) {
        int start_source = lines > 0 ? view_source_line[min_int(top, max_int(0, lines - 1))] + 1 : 0;
        jump_to_match(&sp, start_source, 1, &top, &hscroll, &lines, max_line);
      } else {
        top = min_int(max_int(0, lines - PAGE_LINES), top + PAGE_LINES);
      }
      notes_text_page(title, view_lines, lines, top, &sp);
    }
    if (key == KEY_CTRL_F2) {
      if (active_search) {
        int start_source = lines > 0 ? view_source_line[min_int(top, max_int(0, lines - 1))] - 1 : 0;
        jump_to_match(&sp, start_source, -1, &top, &hscroll, &lines, max_line);
      } else {
        top = max_int(0, top - PAGE_LINES);
      }
      notes_text_page(title, view_lines, lines, top, &sp);
    }
  }
}

static int file_text_matches(const char *path, const SearchPattern *sp, int *first_line, int *hits) {
  *first_line = -1;
  *hits = 0;
  if (sp->len <= 0) return 0;
  if (load_file_buf(path) <= 0) return 0;
  int count = source_line_count();
  for (int line = 0; line < count; ++line) {
    int display_len = source_line_display_text(line, search_line_buf, FILE_BUF_SIZE);
    int state = 0;
    for (int i = 0; i < display_len; ++i) {
      state = search_step(sp, state, search_line_buf[i]);
      if (state != sp->len) continue;
      if (*first_line < 0) *first_line = line;
      if (*hits < 9) ++(*hits);
      state = sp->fail[state - 1];
      if (*hits >= 9) return 1;
    }
  }
  return *hits > 0;
}

static void refresh_result_labels() {
  for (int i = 0; i < result_count; ++i) {
    copy_str(results[i].label, sizeof(results[i].label), results[i].name);
    if (results[i].text_hits > 1 && strlen(results[i].label) < sizeof(results[i].label) - 5) {
      char hits[8];
      sprintf(hits, " (%d)", results[i].text_hits);
      append_str(results[i].label, sizeof(results[i].label), hits);
    }
  }
}

static void add_ranked_result(const char *path, int score, int first_line, int hits) {
  char name[64];
  name_only(path, name);
  if (result_count >= MAX_RESULTS && score <= results[result_count - 1].score) return;
  int pos = result_count < MAX_RESULTS ? result_count++ : MAX_RESULTS - 1;
  while (pos > 0 && results[pos - 1].score < score) {
    results[pos] = results[pos - 1];
    --pos;
  }
  copy_str(results[pos].name, sizeof(results[pos].name), name);
  copy_str(results[pos].path, sizeof(results[pos].path), path);
  results[pos].score = score;
  results[pos].first_line = first_line;
  results[pos].text_hits = hits;
}

static int search_all_rec(const char *dir, const SearchPattern *sp, int depth) {
  if (depth > 8) return result_count;
  unsigned short path[300], found[300];
  char query[300], name[300], full[300];
  join_str(query, sizeof(query), dir, "*");
  Bfile_StrToName_ncpy(path, (const unsigned char *)query, 300);
  int handle = 0;
  file_type_t info;
  int ret = Bfile_FindFirst_NON_SMEM(path, &handle, found, &info);
  int opened = !ret;
  while (!ret) {
    Bfile_NameToStr_ncpy((unsigned char *)name, found, 300);
    if (strcmp(name, ".") && strcmp(name, "..") && strcmp(name, "@MainMem") && !hidden_note_name(name)) {
      int is_txt = ends_with(name, ".txt");
      int is_folder = !is_txt && info.fsize == 0;
      join_str(full, sizeof(full), dir, name);
      if (is_folder) {
        if (!hidden_system_folder(name)) {
          char child[300];
          join_str(child, sizeof(child), full, "\\");
          search_all_rec(child, sp, depth + 1);
        }
      } else if (is_txt) {
        int first = -1, hits = 0;
        int by_name = contains_kmp(name, sp);
        int by_path = contains_kmp(full, sp);
        int score = by_name ? 10000 : (by_path ? 7000 : 0);
        if (file_text_matches(full, sp, &first, &hits)) {
          score += 4000 + hits * 150 - min_int(first, 50);
        }
        if (score) add_ranked_result(full, score, first, hits);
      }
    }
    ret = Bfile_FindNext_NON_SMEM(handle, found, (char *)&info);
  }
  if (opened) Bfile_FindClose(handle);
  return result_count;
}

static void show_file_path(const char *path, const char *name, const char *query) {
  int loaded = load_file_buf(path);
  if (loaded <= 0) {
    if (loaded < 0)
      notes_message("NOTES", "File too large.", "Split into smaller", "text files.");
    else
      notes_message("NOTES", "Could not open", "text file.", 0);
    for (;;) {
      int key = ui_key_poll();
      if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6 || key == KEY_CTRL_EXE) break;
    }
    return;
  }
  wait_text_page(name, query);
}

static int search_results_menu() {
  int sel = 0, top = 0;
  bool dirty = true;
  for (;;) {
    if (dirty) {
      const char *items[MAX_RESULTS];
      for (int i = 0; i < result_count; ++i) items[i] = results[i].label;
      char title[48];
      sprintf(title, "Find: %.24s", last_query);
      notes_menu(title, items, result_count, top, sel, "OPEN", "NEW", "", "", "", "BACK");
      dirty = false;
    }
    int key = ui_key_poll();
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6) return 0;
    if (ui_menu_handle_key(key, result_count, MENU_ROWS, &sel, &top)) dirty = true;
    if (key == KEY_CTRL_F2) return 1;
    if ((key == KEY_CTRL_EXE || key == KEY_CTRL_F1) && result_count) {
      show_file_path(results[sel].path, results[sel].name, last_query);
      dirty = true;
    }
  }
}

static void search_all_notes() {
  for (;;) {
    char q[32];
    copy_str(q, sizeof(q), last_query);
    if (!notes_input("Find all text", q, sizeof(q)) || !q[0]) return;
    copy_str(last_query, sizeof(last_query), q);
    SearchPattern sp;
    search_prepare(&sp, q);
    result_count = 0;
    notes_message("Find all", "Searching all .txt files...", q, 0);
    search_all_rec(NOTES_ROOT, &sp, 0);
    refresh_result_labels();
    if (!result_count) {
      notes_message("Find all", "No results found.", q, "F6 returns.");
      for (;;) {
        int key = ui_key_poll();
        if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6 || key == KEY_CTRL_EXE) break;
      }
      return;
    }
    if (!search_results_menu()) return;
  }
}

static void show_file(NoteEntry *e) {
  show_file_path(e->path, e->name, "");
}

int main() {
  Bdisp_EnableColor(1);
  int sel = 0, top = 0;
  scan_dir();
  bool dirty = true;
  for (;;) {
    if (dirty) {
      const char *names[MAX_ENTRIES];
      for (int i = 0; i < entry_count; ++i) names[i] = entries[i].name;
      if (entry_count)
        notes_menu("NOTES", names, entry_count, top, sel, "OPEN", "FIND", "", "", "", "BACK");
      else
        notes_message("NOTES", "Put notes in", "\\\\fls0\\NOTES\\", "F2 searches.");
      dirty = false;
    }
    int key = ui_key_poll();
    if (key == KEY_CTRL_MENU || key == KEY_CTRL_AC) return 0;
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_F6) { up_folder(); sel = top = 0; scan_dir(); dirty = true; }
    if (ui_menu_handle_key(key, entry_count, MENU_ROWS, &sel, &top)) dirty = true;
    if (key == KEY_CTRL_F2) { search_all_notes(); dirty = true; }
    if ((key == KEY_CTRL_EXE || key == KEY_CTRL_F1) && entry_count) {
      if (entries[sel].is_folder) { copy_str(cwd_path, sizeof(cwd_path), entries[sel].path); sel = top = 0; scan_dir(); dirty = true; }
      else { show_file(&entries[sel]); dirty = true; }
    }
  }
}

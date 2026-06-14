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

static const int MAX_ENTRIES = 80;
static const int MAX_RESULTS = 64;
static const int FILE_BUF_SIZE = 8192;
static const int MAX_VIEW_LINES = 160;
static const int LINE_CAP = 64;
static const int WRAP_COLS = 54;
static const int PAGE_LINES = 11;
static const int MENU_ROWS = UI_MENU_ROWS;

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
static char last_query[32] = "";

static int lower_char(int c) {
  if (c >= 'A' && c <= 'Z') return c + 32;
  return c;
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
  while (!ret && entry_count < MAX_ENTRIES) {
    Bfile_NameToStr_ncpy((unsigned char *)name, found, 300);
    if (strcmp(name, ".") && strcmp(name, "..") && strcmp(name, "@MainMem")) {
      int is_folder = info.fsize == 0;
      if ((is_folder && !hidden_system_folder(name)) || (!is_folder && ends_with(name, ".txt"))) {
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
  Bfile_FindClose(handle);
  return entry_count;
}

static int load_file_buf(const char *path) {
  unsigned short p[300];
  Bfile_StrToName_ncpy(p, (const unsigned char *)path, 300);
  int h = Bfile_OpenFile_OS(p, READWRITE);
  if (h < 0) return 0;
  int sz = Bfile_GetFileSize_OS(h);
  if (sz > FILE_BUF_SIZE - 1) sz = FILE_BUF_SIZE - 1;
  int got = Bfile_ReadFile_OS(h, file_buf, sz, 0);
  Bfile_CloseFile_OS(h);
  if (got < 0) return 0;
  file_buf[got] = 0;
  file_buf_len = got;
  return got;
}

static int table_like(const char *s, int len) {
  int bars = 0, commas = 0, tab = 0, gaps = 0;
  for (int i = 0; i < len; ++i) {
    if (s[i] == '|') ++bars;
    if (s[i] == ',') ++commas;
    if (s[i] == '\t') tab = 1;
    if (i + 1 < len && s[i] == ' ' && s[i + 1] == ' ') ++gaps;
  }
  return tab || bars >= 2 || commas >= 3 || gaps >= 2;
}

static void add_display_line(int *line, const char *s, int len, int hscroll, int preserve) {
  if (*line >= MAX_VIEW_LINES || len < 0) return;
  if (preserve) {
    int start = min_int(hscroll, len);
    int take = min_int(WRAP_COLS, len - start);
    int col = 0;
    for (int i = 0; i < take && col + 1 < LINE_CAP; ++i) {
      char c = s[start + i] == '\t' ? ' ' : s[start + i];
      if (c >= 32 && c <= 126) line_store[*line][col++] = c;
    }
    line_store[*line][col] = 0;
    view_lines[*line] = line_store[*line];
    ++(*line);
    return;
  }
  int pos = 0;
  while (pos < len && *line < MAX_VIEW_LINES) {
    while (pos < len && s[pos] == ' ') ++pos;
    int limit = min_int(len, pos + WRAP_COLS);
    int cut = limit;
    if (limit < len) {
      for (int j = limit; j > pos + 12; --j) {
        if (s[j] == ' ') { cut = j; break; }
      }
    }
    int col = 0;
    for (int i = pos; i < cut && col + 1 < LINE_CAP; ++i) {
      char c = s[i] == '\t' ? ' ' : s[i];
      if (c >= 32 && c <= 126) line_store[*line][col++] = c;
    }
    line_store[*line][col] = 0;
    view_lines[*line] = line_store[*line];
    ++(*line);
    pos = cut;
  }
}

static int build_view_lines(int hscroll) {
  int line = 0, start = 0;
  for (int i = 0; i <= file_buf_len && line < MAX_VIEW_LINES; ++i) {
    char c = i < file_buf_len ? file_buf[i] : '\n';
    if (c == '\r') continue;
    if (c == '\n') {
      int len = i - start;
      if (len <= 0) {
        line_store[line][0] = 0;
        view_lines[line] = line_store[line];
        ++line;
      } else {
        add_display_line(&line, file_buf + start, len, hscroll, table_like(file_buf + start, len));
      }
      start = i + 1;
    }
  }
  return line;
}

static void notes_menu(const char *title, const char *const *items, int count, int top, int sel,
                       const char *f1, const char *f2, const char *f3, const char *f4, const char *f5, const char *f6) {
  ui_menu_keys(title, items, count, top, sel, f1, f2, f3, f4, f5, f6);
}

static void notes_message(const char *title, const char *a, const char *b, const char *c) {
  ui_chrome(title);
  ui_softkeys("", "", "", "", "", "BACK");
  if (a) ui_print(18, 55, a);
  if (b) ui_print(18, 72, b);
  if (c) ui_print(18, 89, c);
  ui_flush();
}

static void notes_text_page(const char *title, const char *const *lines, int count, int top, int hscroll) {
  ui_chrome(title);
  ui_softkeys("NEXT", "PREV", "PGUP", "PGDN", "FIND", "BACK");
  char pos[48];
  sprintf(pos, "%d/%d", count ? top + 1 : 0, count);
  ui_print(310, 28, pos);
  if (hscroll > 0) ui_print(250, 28, ">");
  for (int i = 0; i < PAGE_LINES && top + i < count; ++i)
    ui_print(12, 52 + i * 13, lines[top + i]);
  ui_flush();
}

static int find_line(const char *const *lines, int count, const SearchPattern *sp, int start, int dir) {
  if (sp->len <= 0 || count <= 0) return -1;
  int i = start;
  for (int seen = 0; seen < count; ++seen) {
    if (i < 0) i = count - 1;
    if (i >= count) i = 0;
    if (contains_kmp(lines[i], sp)) return i;
    i += dir;
  }
  return -1;
}

static void wait_text_page(const char *title, const char *path, const char *initial_query, unsigned *tick) {
  (void)tick;
  int top = 0, hscroll = 0;
  SearchPattern sp;
  search_prepare(&sp, initial_query && initial_query[0] ? initial_query : "");
  int lines = build_view_lines(hscroll);
  if (sp.len > 0) {
    int f = find_line(view_lines, lines, &sp, 0, 1);
    if (f >= 0) top = f;
  }
  if (top + PAGE_LINES > lines) top = max_int(0, lines - PAGE_LINES);
  notes_text_page(title, view_lines, lines, top, hscroll);
  for (;;) {
    int key = ui_key_poll();
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6) return;
    if (key == KEY_CTRL_UP && top > 0) notes_text_page(title, view_lines, lines, --top, hscroll);
    if (key == KEY_CTRL_DOWN && top + PAGE_LINES < lines) notes_text_page(title, view_lines, lines, ++top, hscroll);
    if ((key == KEY_CTRL_PAGEUP || key == KEY_CTRL_F3) && top > 0) {
      top = max_int(0, top - PAGE_LINES);
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    if ((key == KEY_CTRL_PAGEDOWN || key == KEY_CTRL_F4) && top + PAGE_LINES < lines) {
      top = min_int(max_int(0, lines - PAGE_LINES), top + PAGE_LINES);
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    if (key == KEY_CTRL_RIGHT) {
      hscroll += 8;
      lines = build_view_lines(hscroll);
      top = min_int(top, max_int(0, lines - PAGE_LINES));
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    if (key == KEY_CTRL_LEFT && hscroll > 0) {
      hscroll = max_int(0, hscroll - 8);
      lines = build_view_lines(hscroll);
      top = min_int(top, max_int(0, lines - PAGE_LINES));
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    if (key == KEY_CTRL_F5) {
      char q[32];
      copy_str(q, sizeof(q), last_query);
      if (ui_input("Find text", q, sizeof(q), tick) && q[0]) {
        copy_str(last_query, sizeof(last_query), q);
        search_prepare(&sp, q);
        int f = find_line(view_lines, lines, &sp, top, 1);
        if (f >= 0) top = min_int(f, max_int(0, lines - PAGE_LINES));
      }
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    if (key == KEY_CTRL_F1) {
      if (sp.len <= 0 && last_query[0]) search_prepare(&sp, last_query);
      int f = find_line(view_lines, lines, &sp, top + 1, 1);
      if (f >= 0) top = min_int(f, max_int(0, lines - PAGE_LINES));
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    if (key == KEY_CTRL_F2) {
      if (sp.len <= 0 && last_query[0]) search_prepare(&sp, last_query);
      int f = find_line(view_lines, lines, &sp, top - 1, -1);
      if (f >= 0) top = min_int(f, max_int(0, lines - PAGE_LINES));
      notes_text_page(title, view_lines, lines, top, hscroll);
    }
    (void)path;
  }
}

static int file_text_matches(const char *path, const SearchPattern *sp, int *first_line, int *hits) {
  *first_line = -1;
  *hits = 0;
  if (sp->len <= 0) return 0;
  unsigned short p[300];
  Bfile_StrToName_ncpy(p, (const unsigned char *)path, 300);
  int h = Bfile_OpenFile_OS(p, READWRITE);
  if (h < 0) return 0;
  int sz = Bfile_GetFileSize_OS(h);
  int off = 0, state = 0, line = 0;
  while (off < sz) {
    int want = FILE_BUF_SIZE - 1;
    if (want > sz - off) want = sz - off;
    int got = Bfile_ReadFile_OS(h, file_buf, want, off);
    if (got <= 0) break;
    for (int i = 0; i < got; ++i) {
      state = search_step(sp, state, file_buf[i]);
      if (state == sp->len) {
        if (*first_line < 0) *first_line = line;
        if (*hits < 9) ++(*hits);
        state = sp->fail[state - 1];
      }
      if (file_buf[i] == '\n') ++line;
    }
    off += got;
    if (*hits >= 9) break;
  }
  Bfile_CloseFile_OS(h);
  return *hits > 0;
}

static void refresh_result_labels() {
  for (int i = 0; i < result_count; ++i) {
    char line_no[16] = "";
    if (results[i].first_line >= 0) sprintf(line_no, " L%d", results[i].first_line + 1);
    copy_str(results[i].label, sizeof(results[i].label), results[i].text_hits > 0 ? "T " : "N ");
    append_str(results[i].label, sizeof(results[i].label), results[i].name);
    append_str(results[i].label, sizeof(results[i].label), line_no);
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

static int search_all_rec(const char *dir, const SearchPattern *sp, int depth, unsigned *tick) {
  if (depth > 8 || result_count >= MAX_RESULTS) return result_count;
  unsigned short path[300], found[300];
  char query[300], name[300], full[300];
  join_str(query, sizeof(query), dir, "*");
  Bfile_StrToName_ncpy(path, (const unsigned char *)query, 300);
  int handle = 0;
  file_type_t info;
  int ret = Bfile_FindFirst_NON_SMEM(path, &handle, found, &info);
  while (!ret && result_count < MAX_RESULTS) {
    Bfile_NameToStr_ncpy((unsigned char *)name, found, 300);
    if (strcmp(name, ".") && strcmp(name, "..") && strcmp(name, "@MainMem")) {
      int is_folder = info.fsize == 0;
      join_str(full, sizeof(full), dir, name);
      if (is_folder) {
        if (!hidden_system_folder(name)) {
          char child[300];
          join_str(child, sizeof(child), full, "\\");
          search_all_rec(child, sp, depth + 1, tick);
        }
      } else if (ends_with(name, ".txt")) {
        int first = -1, hits = 0;
        int by_name = contains_kmp(name, sp);
        int by_path = contains_kmp(full, sp);
        int score = by_name ? 10000 : (by_path ? 7000 : 0);
        if (!score && file_text_matches(full, sp, &first, &hits))
          score = 4000 + hits * 150 - min_int(first, 50);
        if (score) add_ranked_result(full, score, first, hits);
      }
    }
    ret = Bfile_FindNext_NON_SMEM(handle, found, (char *)&info);
    (void)tick;
  }
  Bfile_FindClose(handle);
  return result_count;
}

static void show_file_path(const char *path, const char *name, const char *query, unsigned *tick) {
  if (!load_file_buf(path)) {
    static const char *const err[] = {"Could not open text file."};
    ui_wait_page("NOTES", err, 1, tick);
    return;
  }
  wait_text_page(name, path, query, tick);
}

static int search_results_menu(unsigned *tick) {
  (void)tick;
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
      show_file_path(results[sel].path, results[sel].name, last_query, tick);
      dirty = true;
    }
  }
}

static void search_all_notes(unsigned *tick) {
  for (;;) {
    char q[32];
    copy_str(q, sizeof(q), last_query);
    if (!ui_input("Find all text", q, sizeof(q), tick) || !q[0]) return;
    copy_str(last_query, sizeof(last_query), q);
    SearchPattern sp;
    search_prepare(&sp, q);
    result_count = 0;
    notes_message("Find all", "Searching all .txt files...", q, 0);
    search_all_rec(NOTES_ROOT, &sp, 0, tick);
    refresh_result_labels();
    if (!result_count) {
      notes_message("Find all", "No results found.", q, "F6 returns.");
      for (;;) {
        int key = ui_key_poll();
        if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_F6 || key == KEY_CTRL_EXE) break;
      }
      return;
    }
    if (!search_results_menu(tick)) return;
  }
}

static void show_file(NoteEntry *e, unsigned *tick) {
  show_file_path(e->path, e->name, "", tick);
}

int main() {
  Bdisp_EnableColor(1);
  unsigned tick = (unsigned)RTC_GetTicks();
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
        notes_message("NOTES", "Put notes in", "\\\\fls0\\NOTES\\", "then reopen.");
      dirty = false;
    }
    int key = ui_key_poll();
    if (key == KEY_CTRL_MENU || key == KEY_CTRL_AC) return 0;
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_F6) { up_folder(); sel = top = 0; scan_dir(); dirty = true; }
    if (ui_menu_handle_key(key, entry_count, MENU_ROWS, &sel, &top)) dirty = true;
    if (key == KEY_CTRL_F2) { search_all_notes(&tick); dirty = true; }
    if ((key == KEY_CTRL_EXE || key == KEY_CTRL_F1) && entry_count) {
      if (entries[sel].is_folder) { copy_str(cwd_path, sizeof(cwd_path), entries[sel].path); sel = top = 0; scan_dir(); dirty = true; }
      else { show_file(&entries[sel], &tick); dirty = true; }
    }
  }
}

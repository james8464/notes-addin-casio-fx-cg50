#include "cscalc_engine.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

static int add(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], int n, const char *fmt, ...) {
  if (n >= CSCALC_MAX_LINES) return n;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(out[n], CSCALC_LINE_LEN, fmt, ap);
  va_end(ap);
  return n + 1;
}

static void clean(const char *in, char *out, int cap) {
  int j = 0;
  for (int i = 0; in && in[i] && j + 1 < cap; ++i) {
    unsigned char c = (unsigned char)in[i];
    if (isspace(c)) continue;
    out[j++] = (char)tolower(c);
  }
  out[j] = 0;
}

static void raw_clean(const char *in, char *out, int cap) {
  int j = 0;
  for (int i = 0; in && in[i] && j + 1 < cap; ++i) {
    unsigned char c = (unsigned char)in[i];
    if (isalnum(c) || c == '.' || c == '-' || c == '\'') out[j++] = (char)tolower(c);
    else out[j++] = ',';
  }
  out[j] = 0;
}

static bool starts(const char *s, const char *p) {
  return strncmp(s, p, strlen(p)) == 0;
}

static bool has(const char *s, const char *p) { return strstr(s, p) != 0; }

static bool starts2(const char *s, const char *a, const char *b) {
  return starts(s, a) || starts(s, b);
}

static bool starts3(const char *s, const char *a, const char *b, const char *c) {
  return starts(s, a) || starts(s, b) || starts(s, c);
}

static int args(const char *s, char a[][48], int maxa) {
  const char *l = strchr(s, '('), *r = strrchr(s, ')');
  if (!l || !r || r <= l) return 0;
  int n = 0, j = 0, depth = 0;
  for (const char *p = l + 1; p < r && n < maxa; ++p) {
    if (*p == '(') depth++;
    if (*p == ')') depth--;
    if (*p == ',' && depth == 0) {
      a[n][j] = 0; n++; j = 0;
    } else if (j < 47) a[n][j++] = *p;
  }
  if (n < maxa) { a[n][j] = 0; n++; }
  return n;
}

static int scan_nums(const char *s, double v[], int maxv) {
  int n = 0;
  for (int i = 0; s[i] && n < maxv; ++i) {
    bool neg = s[i] == '-' && isdigit((unsigned char)s[i+1]);
    if (!isdigit((unsigned char)s[i]) && !neg) continue;
    int j = i + (neg ? 1 : 0); double x = 0, scale = 1;
    while (isdigit((unsigned char)s[j])) { x = x * 10 + (s[j++] - '0'); }
    if (s[j] == '.') for (++j; isdigit((unsigned char)s[j]); ++j) { scale *= 10; x += (s[j] - '0') / scale; }
    v[n++] = neg ? -x : x; i = j - 1;
  }
  return n;
}

static int scan_bits(const char *s, char b[][48], int maxb) {
  int n = 0;
  for (int i = 0; s[i] && n < maxb;) {
    if (s[i] != '0' && s[i] != '1') { ++i; continue; }
    int j = i, k = 0;
    while ((s[j] == '0' || s[j] == '1') && k < 47) b[n][k++] = s[j++];
    b[n][k] = 0;
    if (k > 1) ++n;
    i = j;
  }
  return n;
}

static bool scan_fixed_bits(const char *s, char *buf, int cap) {
  for (int i = 0; s[i]; ++i) {
    if (s[i] != '0' && s[i] != '1') continue;
    int j = i, k = 0; bool dot = false;
    while ((s[j] == '0' || s[j] == '1' || s[j] == '.') && k + 1 < cap) {
      if (s[j] == '.') dot = true;
      buf[k++] = s[j++];
    }
    buf[k] = 0;
    if (dot && k > 2) return true;
    i = j;
  }
  return false;
}

static double read_num(const char *s) {
  double sign = 1, v = 0, scale = 1;
  if (*s == '-') { sign = -1; ++s; }
  while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
  if (*s == '.') for (++s; *s >= '0' && *s <= '9'; ++s) { scale *= 10; v += (*s - '0') / scale; }
  return sign * v;
}

static bool label_num(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0;
    while (j < nl && s[i+j] && tolower((unsigned char)s[i+j]) == name[j]) ++j;
    if (j != nl) continue;
    int k = i + j;
    while (s[k] == ' ' || s[k] == '\t') ++k;
    if (s[k] != '=') continue;
    ++k;
    while (s[k] == ' ' || s[k] == '\t') ++k;
    *v = read_num(s + k);
    return true;
  }
  return false;
}

static double num(const char *s) {
  double sign = 1, v = 0, scale = 1;
  if (*s == '-') { sign = -1; ++s; }
  for (; *s && *s != '.'; ++s) if (*s >= '0' && *s <= '9') v = v * 10 + (*s - '0');
  if (*s == '.') for (++s; *s; ++s) if (*s >= '0' && *s <= '9') { scale *= 10; v += (*s - '0') / scale; }
  return sign * v;
}

static long long parse_int(const char *s) { return (long long)num(s); }

static long long parse_base(const char *s, int base) {
  long long v = 0;
  for (int i = 0; s[i]; ++i) {
    int d = s[i] >= '0' && s[i] <= '9' ? s[i] - '0' : s[i] >= 'a' && s[i] <= 'f' ? s[i] - 'a' + 10 : s[i] >= 'A' && s[i] <= 'F' ? s[i] - 'A' + 10 : -1;
    if (d >= 0 && d < base) v = v * base + d;
  }
  return v;
}

static double pow2(int e) {
  double r = 1; int n = e < 0 ? -e : e;
  while (n--) r *= 2;
  return e < 0 ? 1 / r : r;
}

static double round_nearest(double x) {
  return x >= 0 ? (long long)(x + 0.5) : (long long)(x - 0.5);
}

static int bin_unsigned(const char *s) {
  int v = 0;
  for (int i = 0; s[i]; ++i) if (s[i] == '0' || s[i] == '1') v = v * 2 + (s[i] - '0');
  return v;
}

static int ceil_log2_ll(long long x) {
  if (x <= 1) return 0;
  int b = 0; long long v = 1;
  while (v < x && b < 62) { v <<= 1; ++b; }
  return b;
}

static void to_bin(long long v, int width, char *buf);

static bool printable_ascii(int c) { return c >= 32 && c <= 126; }

static int add_code_lines(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], int code, bool unicode, int ch) {
  char bits[65]; to_bin(code, code < 256 ? 8 : 16, bits);
  int n = add(out, 0, unicode ? "Unicode code point conversion." : "ASCII code conversion.");
  if (ch >= 0) n = add(out, n, "'%c' has denary code %d.", ch, code);
  else if (!unicode && printable_ascii(code)) n = add(out, n, "ASCII code %d represents '%c'.", code, code);
  else if (unicode) n = add(out, n, "Unicode code point = U+%04X = %d_10.", code, code);
  else n = add(out, n, "Code = %d_10.", code);
  return add(out, n, "%d_10 = %s_2 = %X_16", code, bits, code);
}

static bool find_code_char(const char *in, int *ch) {
  for (int i = 0; in && in[i]; ++i) {
    if ((in[i] == '\'' || in[i] == '"') && in[i+1] && in[i+2] == in[i]) {
      *ch = (unsigned char)in[i+1]; return true;
    }
  }
  for (int i = 0; in && in[i]; ++i) {
    if ((i == 0 || !isalnum((unsigned char)in[i-1])) && isalpha((unsigned char)in[i]) &&
        !isalnum((unsigned char)in[i+1])) {
      if (in[i] >= 'A' && in[i] <= 'Z') { *ch = (unsigned char)in[i]; return true; }
    }
  }
  const char *p = strstr(in ? in : "", " for ");
  if (p) {
    p += 5; while (*p == ' ' || *p == '\t') ++p;
    if (*p && !isalnum((unsigned char)p[1])) { *ch = (unsigned char)*p; return true; }
  }
  return false;
}

static bool word_is(const char *w, const char *k) { return strcmp(w, k) == 0; }

static bool rle_skip_word(const char *w) {
  return word_is(w, "run") || word_is(w, "length") || word_is(w, "encoding") ||
         word_is(w, "encode") || word_is(w, "rle") || word_is(w, "runs") || word_is(w, "text") ||
         word_is(w, "string") || word_is(w, "symbol") || word_is(w, "bits") ||
         word_is(w, "count") || word_is(w, "with") || word_is(w, "using");
}

static bool find_rle_sequence(const char *in, char *seq, int cap) {
  int best = 0; seq[0] = 0;
  for (int i = 0; in && in[i];) {
    while (in[i] && !isalpha((unsigned char)in[i])) ++i;
    char w[48]; int j = 0;
    while (isalpha((unsigned char)in[i]) && j + 1 < (int)sizeof(w)) w[j++] = (char)tolower((unsigned char)in[i++]);
    w[j] = 0;
    if (j <= 1 || rle_skip_word(w)) continue;
    bool repeated = false; for (int k = 1; k < j; ++k) if (w[k] == w[k-1]) repeated = true;
    if (repeated && j < cap && j > best) { strcpy(seq, w); best = j; }
  }
  return best > 0;
}

static bool rpn_op_token(const char *s, char *op);

static bool make_rpn_cmd(const char *in, char *cmd, int cap) {
  int p = 0, count = 0;
  p += sprintf(cmd + p, "rpn(");
  for (int i = 0; in && in[i] && p < cap - 16;) {
    unsigned char c = (unsigned char)in[i];
    if (isspace(c) || c == ',' || c == '(' || c == ')') { ++i; continue; }
    if ((c == '-' && isdigit((unsigned char)in[i+1])) || isdigit(c)) {
      if (count++) cmd[p++] = ',';
      if (c == '-') cmd[p++] = in[i++];
      while ((isdigit((unsigned char)in[i]) || in[i] == '.') && p < cap - 2) cmd[p++] = in[i++];
      cmd[p] = 0;
      continue;
    }
    if (c == '+' || c == '*' || c == '/') {
      if (count++) cmd[p++] = ',';
      cmd[p++] = (char)c; cmd[p] = 0; ++i; continue;
    }
    if (c == '-') {
      if (count++) cmd[p++] = ',';
      cmd[p++] = '-'; cmd[p] = 0; ++i; continue;
    }
    if (isalpha(c)) {
      char w[16]; int j = 0;
      while (isalpha((unsigned char)in[i]) && j + 1 < (int)sizeof(w)) w[j++] = (char)tolower((unsigned char)in[i++]);
      w[j] = 0;
      char op = 0;
      if (strcmp(w, "rpn") == 0 || strcmp(w, "postfix") == 0 || strcmp(w, "evaluate") == 0 || strcmp(w, "reverse") == 0 || strcmp(w, "polish") == 0) continue;
      if (rpn_op_token(w, &op)) {
        if (count++) cmd[p++] = ',';
        cmd[p++] = op; cmd[p] = 0;
      }
      continue;
    }
    ++i;
  }
  if (count < 3 || p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static bool make_ds_cmd(const char *in, const char *name, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  int p = sprintf(cmd, "%s(", name), count = 0;
  for (int i = 0; t[i] && count < 32;) {
    while (t[i] == ',') ++i;
    char w[20]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    bool number = (w[0] == '-' && isdigit((unsigned char)w[1])) || isdigit((unsigned char)w[0]);
    bool remove = word_is(w, "pop") || word_is(w, "dequeue") || word_is(w, "remove");
    if (!number && !remove) continue;
    if (count++) cmd[p++] = ',';
    p += sprintf(cmd + p, "%s", remove ? "pop" : w);
  }
  if (!count || p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static bool tree_skip_word(const char *w) {
  return word_is(w, "tree") || word_is(w, "traversal") || word_is(w, "traverse") ||
         word_is(w, "nodes") || word_is(w, "node") || word_is(w, "level") ||
         word_is(w, "order") || word_is(w, "preorder") || word_is(w, "pre") ||
         word_is(w, "inorder") || word_is(w, "in") || word_is(w, "postorder") ||
         word_is(w, "post") || word_is(w, "binary") || word_is(w, "root") ||
         word_is(w, "left") || word_is(w, "right") || word_is(w, "child") ||
         word_is(w, "children") || word_is(w, "edge") || word_is(w, "edges") ||
         word_is(w, "triple") || word_is(w, "triples");
}

static bool make_tree_cmd(const char *in, const char *name, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  char tok[32][16]; int count = 0;
  for (int i = 0; t[i] && count < 32;) {
    while (t[i] == ',') ++i;
    char w[16]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0] || tree_skip_word(w)) continue;
    if (count < 32) strcpy(tok[count++], w);
  }
  if (count < 1) return false;
  bool explicit_shape = has(t, "root") || has(t, "left") || has(t, "right") ||
                        has(t, "child") || has(t, "children") || has(t, "edge");
  int p = explicit_shape && count >= 4 && ((count - 1) % 3) == 0 ?
          sprintf(cmd, "%stree(", name) : sprintf(cmd, "%s(", name);
  for (int i = 0; i < count && p < cap - 16; ++i) {
    if (i) cmd[p++] = ',';
    p += sprintf(cmd + p, "%s", tok[i]);
  }
  if (p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static bool dijkstra_skip_word(const char *w) {
  return word_is(w, "dijkstra") || word_is(w, "shortest") || word_is(w, "path") ||
         word_is(w, "route") || word_is(w, "graph") || word_is(w, "from") ||
         word_is(w, "to") || word_is(w, "edge") || word_is(w, "edges") ||
         word_is(w, "node") || word_is(w, "nodes") || word_is(w, "using") ||
         word_is(w, "with") || word_is(w, "weight") || word_is(w, "weights") ||
         word_is(w, "distance") || word_is(w, "find") || word_is(w, "calculate");
}

static bool make_dijkstra_cmd(const char *in, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  char tok[32][16]; int nt = 0;
  for (int i = 0; t[i] && nt < 32;) {
    while (t[i] == ',') ++i;
    char w[16]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0] || dijkstra_skip_word(w)) continue;
    strcpy(tok[nt++], w);
  }
  if (nt < 5) return false;
  int p = sprintf(cmd, "dijkstra(%s,%s", tok[0], tok[1]);
  for (int i = 2; i < nt && p < cap - 20; ++i) p += sprintf(cmd + p, ",%s", tok[i]);
  if (((nt - 2) % 3) != 0 || p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static bool fsm_skip_word(const char *w) {
  return word_is(w, "fsm") || word_is(w, "finite") || word_is(w, "state") ||
         word_is(w, "machine") || word_is(w, "trace") || word_is(w, "start") ||
         word_is(w, "input") || word_is(w, "string") || word_is(w, "transition") ||
         word_is(w, "transitions") || word_is(w, "table") || word_is(w, "read") ||
         word_is(w, "with") || word_is(w, "output") || word_is(w, "outputs") ||
         word_is(w, "mealy");
}

static bool make_fsm_cmd(const char *in, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  char tok[32][48]; int nt = 0;
  for (int i = 0; t[i] && nt < 32;) {
    while (t[i] == ',') ++i;
    char w[48]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0] || fsm_skip_word(w)) continue;
    strcpy(tok[nt++], w);
  }
  if (nt < 5) return false;
  bool out = has(t, "mealy") || has(t, "output");
  int group = out ? 4 : 3;
  if (((nt - 2) % group) != 0) return false;
  int p = sprintf(cmd, out ? "fsmout(%s,%s" : "fsm(%s,%s", tok[0], tok[1]);
  for (int i = 2; i < nt && p < cap - 50; ++i) p += sprintf(cmd + p, ",%s", tok[i]);
  if (p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static const char *sql_op_text(const char *op) {
  if (strcmp(op, ">") == 0 || word_is(op, "gt") || word_is(op, "greater") || word_is(op, "more") || word_is(op, "above")) return ">";
  if (strcmp(op, ">=") == 0 || word_is(op, "gte") || word_is(op, "atleast")) return ">=";
  if (strcmp(op, "<") == 0 || word_is(op, "lt") || word_is(op, "less") || word_is(op, "below") || word_is(op, "under")) return "<";
  if (strcmp(op, "<=") == 0 || word_is(op, "lte") || word_is(op, "atmost")) return "<=";
  if (strcmp(op, "!=") == 0 || word_is(op, "ne") || word_is(op, "not")) return "<>";
  return "=";
}

static bool make_sql_cmd(const char *in, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  char tok[24][32]; int nt = 0;
  for (int i = 0; t[i] && nt < 24;) {
    while (t[i] == ',') ++i;
    char w[32]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (w[0]) strcpy(tok[nt++], w);
  }
  int sel = -1, from = -1, where = -1;
  for (int i = 0; i < nt; ++i) {
    if (word_is(tok[i], "select")) sel = i;
    if (word_is(tok[i], "from")) from = i;
    if (word_is(tok[i], "where")) where = i;
  }
  bool count = false;
  for (int i = 0; i < nt; ++i) if (word_is(tok[i], "count") || word_is(tok[i], "countrows") || word_is(tok[i], "countrecords")) count = true;
  if (from < 0 || where < 0 || where + 3 >= nt) return false;
  const char *show = count ? "*" : (sel >= 0 && sel + 1 < nt ? tok[sel + 1] : "*");
  const char *table = tok[from + 1];
  const char *field = tok[where + 1];
  const char *op = tok[where + 2];
  const char *value = tok[where + 3];
  if ((word_is(op, "greater") || word_is(op, "less") || word_is(op, "more")) && word_is(value, "than") && where + 4 < nt) value = tok[where + 4];
  if (word_is(op, "at") && word_is(value, "least") && where + 4 < nt) { op = "gte"; value = tok[where + 4]; }
  if (word_is(op, "at") && word_is(value, "most") && where + 4 < nt) { op = "lte"; value = tok[where + 4]; }
  (void)cap;
  if (count) sprintf(cmd, "sqlcount(%s,%s,%s,%s)", table, field, sql_op_text(op), value);
  else sprintf(cmd, "sqlselect(%s,%s,%s,%s,%s)", table, show, field, sql_op_text(op), value);
  return true;
}

static bool minterm_skip_word(const char *w) {
  return word_is(w, "kmap") || word_is(w, "karnaugh") || word_is(w, "map") ||
         word_is(w, "minterm") || word_is(w, "minterms") || word_is(w, "ones") ||
         word_is(w, "cells") || word_is(w, "cell") || word_is(w, "for") ||
         word_is(w, "variables") || word_is(w, "variable") || word_is(w, "vars") ||
         word_is(w, "with") || word_is(w, "simplify") || word_is(w, "boolean") ||
         word_is(w, "logic") || word_is(w, "output") || word_is(w, "is") ||
         word_is(w, "are") || word_is(w, "at");
}

static bool make_minterm_cmd(const char *in, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  char vars[8] = ""; int vc = 0, mins[32], mc = 0;
  for (int i = 0; t[i];) {
    while (t[i] == ',') ++i;
    char w[32]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0] || minterm_skip_word(w)) continue;
    if ((w[0] == '-' && isdigit((unsigned char)w[1])) || isdigit((unsigned char)w[0])) {
      if (mc < 32) mins[mc++] = (int)parse_int(w);
      continue;
    }
    if (j <= 6) {
      for (int k = 0; w[k] && vc < 6; ++k) if (isalpha((unsigned char)w[k])) {
        char c = (char)toupper((unsigned char)w[k]);
        bool seen = false; for (int q = 0; q < vc; ++q) if (vars[q] == c) seen = true;
        if (!seen) { vars[vc++] = c; vars[vc] = 0; }
      }
    }
  }
  if (!mc) return false;
  if (!vc) {
    int maxm = 0; for (int i = 0; i < mc; ++i) if (mins[i] > maxm) maxm = mins[i];
    while ((1 << vc) <= maxm && vc < 6) ++vc;
    if (vc < 1) vc = 1;
    for (int i = 0; i < vc; ++i) vars[i] = (char)('A' + i);
    vars[vc] = 0;
  }
  int p = sprintf(cmd, "minterms(");
  for (int i = 0; i < vc && p < cap - 24; ++i) {
    if (i) cmd[p++] = ',';
    cmd[p++] = vars[i]; cmd[p] = 0;
  }
  for (int i = 0; i < mc && p < cap - 24; ++i) p += sprintf(cmd + p, ",%d", mins[i]);
  if (p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static int is_bits(const char *s) {
  if (!s || !*s) return 0;
  for (int i = 0; s[i]; ++i) if (s[i] != '0' && s[i] != '1' && s[i] != '.') return 0;
  return 1;
}

static int twos_decode(const char *s) {
  int n = (int)strlen(s), u = bin_unsigned(s);
  if (n == 0 || s[0] == '0') return u;
  return u - (1 << n);
}

static void add_bits(const char *a, const char *b, int width, char *buf, int *carry_out) {
  int carry = 0;
  for (int i = 0; i < width; ++i) {
    int ai = (int)strlen(a) - 1 - i, bi = (int)strlen(b) - 1 - i;
    int av = ai >= 0 && a[ai] == '1', bv = bi >= 0 && b[bi] == '1';
    int sum = av + bv + carry;
    buf[width - 1 - i] = (sum & 1) ? '1' : '0';
    carry = sum >> 1;
  }
  buf[width] = 0;
  if (carry_out) *carry_out = carry;
}

static void to_bin(long long v, int width, char *buf) {
  unsigned long long mask = width >= 63 ? ~0ULL : ((1ULL << width) - 1);
  unsigned long long u = ((unsigned long long)v) & mask;
  for (int i = width - 1, j = 0; i >= 0; --i, ++j) buf[j] = ((u >> i) & 1) ? '1' : '0';
  buf[width] = 0;
}

static void insert_point(const char *bits, int whole, char *buf) {
  int j = 0;
  for (int i = 0; bits[i] && j < 63; ++i) {
    if (i == whole) buf[j++] = '.';
    buf[j++] = bits[i];
  }
  buf[j] = 0;
}

static double fixed_decode(const char *s) {
  const char *dot = strchr(s, '.');
  double v = 0.0;
  int int_len = dot ? (int)(dot - s) : (int)strlen(s);
  for (int i = 0; i < int_len; ++i) if (s[i] == '1') v += pow2(int_len - 1 - i);
  if (dot) for (int i = 1; dot[i]; ++i) if (dot[i] == '1') v += pow2(-i);
  return v;
}

static double fixed_tc_decode(const char *s) {
  const char *dot = strchr(s, '.');
  if (!dot) return (double)twos_decode(s);
  char whole[48]; int wi = 0;
  for (int i = 0; s[i] && s[i] != '.' && wi < 47; ++i) whole[wi++] = s[i];
  whole[wi] = 0;
  double v = (double)twos_decode(whole);
  for (int i = 1; dot[i]; ++i) if (dot[i] == '1') v += pow2(-i);
  return v;
}

static double mantissa_decode(const char *s) {
  double v = s[0] == '1' ? -1.0 : 0.0;
  for (int i = 1; s[i]; ++i) if (s[i] == '1') v += pow2(-i);
  return v;
}

static void mantissa_encode(double m, int mb, char *mant) {
  mant[0] = m < 0 ? '1' : '0';
  double rem = m < 0 ? m + 1.0 : m;
  for (int i = 1; i < mb; ++i) {
    double bitv = pow2(-i);
    if (rem >= bitv) { mant[i] = '1'; rem -= bitv; }
    else mant[i] = '0';
  }
  mant[mb] = 0;
}

static int conv(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], char a[][48], int na) {
  if (na < 1) return 0;
  if (starts(a[0], "0b")) {
    int v = bin_unsigned(a[0] + 2);
    int n = add(out, 0, "Binary place values: 128 64 32 16 8 4 2 1");
    return add(out, n, "%s_2 = %d_10", a[0] + 2, v);
  }
  return 0;
}

static int eval_base(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts3(s, "bin(", "binary(", "tobin(") && na == 1) {
    long long v = parse_int(a[0]); char b[65]; int w = v < 256 ? 8 : 16; to_bin(v, w, b);
    int n = add(out, 0, "Repeated division by 2, read remainders upwards.");
    return add(out, n, "%lld_10 = %s_2", v, b);
  }
  if (starts3(s, "hex(", "hexadecimal(", "tohex(") && na == 1) {
    long long v = parse_int(a[0]);
    int n = add(out, 0, "Convert denary to hex by repeated division by 16.");
    return add(out, n, "%lld_10 = %llX_16", v, v);
  }
  if (starts3(s, "den(", "denary(", "decimal(") && na >= 1) {
    int base = na > 1 ? (int)parse_int(a[1]) : (is_bits(a[0]) ? 2 : 16);
    long long v = parse_base(a[0], base);
    int n = add(out, 0, "Multiply each digit by its base place value.");
    return add(out, n, "%s_%d = %lld_10", a[0], base, v);
  }
  if (starts2(s, "convert(", "base(") && na == 3) {
    long long v = parse_base(a[0], (int)parse_int(a[1]));
    if (parse_int(a[2]) == 2) { char b[65]; to_bin(v, v < 256 ? 8 : 16, b); return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %s_2", v, b); }
    if (parse_int(a[2]) == 16) return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %llX_16", v, v);
    return add(out, 0, "%s_%s = %lld_10", a[0], a[1], v);
  }
  return conv(out, a, na);
}

static int eval_twos(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts2(s, "unsignedrange(", "urange(") && na == 1) {
    int w = (int)parse_int(a[0]);
    return add(out, add(out, 0, "n-bit unsigned range:"), "0 to 2^%d-1 = 0 to %d", w, (1<<w)-1);
  }
  if (starts3(s, "twosrange(", "tcrange(", "twoscomprange(") && na == 1) {
    int w = (int)parse_int(a[0]);
    return add(out, add(out, 0, "n-bit two's complement range:"), "-2^(%d) to 2^(%d)-1 = %d to %d", w-1, w-1, -(1<<(w-1)), (1<<(w-1))-1);
  }
  if ((starts(s, "twosdec(") || starts(s, "tcdec(") || starts(s, "twosdecode(")) && na == 1) {
    int n = add(out, 0, "MSB is sign bit.");
    if (a[0][0] == '0') n = add(out, n, "MSB=0, so use unsigned place values.");
    else n = add(out, n, "MSB=1, so subtract 2^%d from unsigned value.", (int)strlen(a[0]));
    return add(out, n, "%s = %d", a[0], twos_decode(a[0]));
  }
  if ((starts(s, "twos(") || starts(s, "tc(") || starts(s, "twoscomp(")) && na >= 2) {
    long long v = parse_int(a[0]); int w = (int)parse_int(a[1]); char b[65]; to_bin(v, w, b);
    int n = add(out, 0, "%d-bit two's complement.", w);
    if (v < 0) n = add(out, n, "Encode negative: add 2^%d to %lld.", w, v);
    return add(out, n, "%lld -> %s", v, b);
  }
  if (starts2(s, "twosadd(", "tcadd(") && na == 2) {
    int w = (int)strlen(a[0]), x = twos_decode(a[0]), y = twos_decode(a[1]); char b[65]; to_bin(x+y, w, b);
    int n = add(out, 0, "Decode/add in fixed width, discard carry beyond %d bits.", w);
    n = add(out, n, "%s=%d, %s=%d", a[0], x, a[1], y);
    n = add(out, n, "%d+%d=%d -> %s", x, y, x+y, b);
    bool ov = (x >= 0 && y >= 0 && b[0] == '1') || (x < 0 && y < 0 && b[0] == '0');
    return add(out, n, ov ? "overflow: same signs gave opposite sign bit." : "no two's complement overflow.");
  }
  if ((starts(s, "twossub(") || starts(s, "tcsub(") || starts(s, "twossubtract(")) && na == 2) {
    int w = (int)strlen(a[0]), x = twos_decode(a[0]), y = twos_decode(a[1]); char b[65], neg[65]; to_bin(x-y, w, b); to_bin(-y, w, neg);
    int n = add(out, 0, "Subtraction: add the two's complement of the second value.");
    n = add(out, n, "%s=%d, %s=%d", a[0], x, a[1], y);
    n = add(out, n, "two's complement of second value = %s", neg);
    n = add(out, n, "%s + %s = %s in %d bits", a[0], neg, b, w);
    return add(out, n, "%d-%d=%d -> %s", x, y, x-y, b);
  }
  return 0;
}

static int eval_binary_arith(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[16][48]; int na = args(s, a, 16);
  if (starts3(s, "binadd(", "binaryadd(", "addbits(") && na >= 2) {
    int w = na > 2 ? (int)parse_int(a[2]) : (int)((strlen(a[0]) > strlen(a[1])) ? strlen(a[0]) : strlen(a[1]));
    char b[65]; int carry = 0; add_bits(a[0], a[1], w, b, &carry);
    int n = add(out, 0, "Add from the right, carrying 1 when a column totals 2 or 3.");
    n = add(out, n, "%s + %s = %s", a[0], a[1], b);
    if (carry) n = add(out, n, "carry beyond %d bits is overflow if width is fixed.", w);
    return n;
  }
  if (starts2(s, "shift(", "binshift(") && na >= 3) {
    int k = (int)parse_int(a[2]); char b[65]; int len = (int)strlen(a[0]);
    for (int i = 0; i < len && i < 64; ++i) b[i] = a[0][i]; b[len] = 0;
    bool left = a[1][0] == 'l' || a[1][0] == '+';
    for (int step = 0; step < k; ++step) {
      if (left) { for (int i = 0; i < len - 1; ++i) b[i] = b[i+1]; b[len-1] = '0'; }
      else { for (int i = len - 1; i > 0; --i) b[i] = b[i-1]; b[0] = '0'; }
    }
    int n = add(out, 0, left ? "Logical left shift: move bits left, fill right with 0." : "Logical right shift: move bits right, fill left with 0.");
    n = add(out, n, "%s shifted %s by %d = %s", a[0], left ? "left" : "right", k, b);
    return add(out, n, left ? "This multiplies unsigned value by 2^%d if no overflow." : "This divides unsigned value by 2^%d, discarding remainder.", k);
  }
  if (starts2(s, "parity(", "paritybit(") && na >= 1) {
    int ones = 0; for (int i = 0; a[0][i]; ++i) if (a[0][i] == '1') ones++;
    bool odd = na > 1 && a[1][0] == 'o';
    int bit = odd ? (ones % 2 ? 0 : 1) : (ones % 2 ? 1 : 0);
    int n = add(out, 0, odd ? "Odd parity means total number of 1s is odd." : "Even parity means total number of 1s is even.");
    n = add(out, n, "%s has %d one-bits.", a[0], ones);
    return add(out, n, "parity bit = %d, transmitted bits = %s%d", bit, a[0], bit);
  }
  if (starts3(s, "xorbits(", "andbits(", "orbits(") && na >= 2) {
    int len = (int)strlen(a[0]); if ((int)strlen(a[1]) < len) len = (int)strlen(a[1]);
    char b[65]; bool isxor=starts(s,"xorbits("), isand=starts(s,"andbits(");
    for (int i = 0; i < len && i < 64; ++i) {
      int x=a[0][i]=='1', y=a[1][i]=='1';
      b[i] = (isxor ? (x^y) : isand ? (x&y) : (x|y)) ? '1' : '0';
    }
    b[len] = 0;
    int n = add(out, 0, isxor ? "XOR gives 1 when the bits are different." : isand ? "AND gives 1 only when both bits are 1." : "OR gives 1 when either bit is 1.");
    n = add(out, n, "%s", a[0]);
    n = add(out, n, "%s", a[1]);
    return add(out, n, "result = %s", b);
  }
  if (starts2(s, "notbits(", "invertbits(") && na >= 1) {
    int len = (int)strlen(a[0]); char b[65];
    for (int i = 0; i < len && i < 64; ++i) b[i] = a[0][i] == '1' ? '0' : '1';
    b[len] = 0;
    int n = add(out, 0, "NOT flips each bit.");
    return add(out, n, "%s -> %s", a[0], b);
  }
  if (starts3(s, "hamming(", "hammingdistance(", "bitdiff(") && na >= 2) {
    int len = (int)strlen(a[0]), d = 0; char pos[80] = ""; int pp = 0;
    if ((int)strlen(a[1]) < len) len = (int)strlen(a[1]);
    for (int i = 0; i < len; ++i) if (a[0][i] != a[1][i]) {
      d++; pp += sprintf(pos + pp, "%s%d", pp ? "," : "", i + 1);
    }
    int n = add(out, 0, "Hamming distance counts bit positions that differ.");
    n = add(out, n, "%s", a[0]);
    n = add(out, n, "%s", a[1]);
    return add(out, n, "different positions: %s, distance = %d", d ? pos : "none", d);
  }
  if (starts3(s, "checksum(", "checksummod(", "binarychecksum(") && na >= 2) {
    int w = (int)parse_int(a[0]), sum = 0; char b[65];
    for (int i = 1; i < na; ++i) sum += bin_unsigned(a[i]);
    int mod = w >= 30 ? 0 : (1 << w), chk = mod ? sum % mod : sum;
    to_bin(chk, w, b);
    int n = add(out, 0, "Checksum: add blocks as unsigned binary, then keep the lowest %d bits.", w);
    n = add(out, n, "decimal sum = %d", sum);
    return add(out, n, "checksum = %d mod 2^%d = %s", chk, w, b);
  }
  if (starts3(s, "checkdigit(", "modcheck(", "weightedcheck(") && na >= 3) {
    int mod = (int)parse_int(a[1]), sum = 0, len = (int)strlen(a[0]);
    int n = add(out, 0, "Use weighted modulo check digit.");
    n = add(out, n, "Multiply each digit by its weight and add.");
    for (int i = 0; i < len && i + 2 < na; ++i) {
      int d = a[0][i] >= '0' && a[0][i] <= '9' ? a[0][i] - '0' : 0;
      int w = (int)parse_int(a[i + 2]);
      sum += d * w;
    }
    int rem = mod ? sum % mod : 0;
    int digit = mod ? (mod - rem) % mod : 0;
    n = add(out, n, "sum = %d, remainder on division by %d is %d", sum, mod, rem);
    return add(out, n, "check digit = (%d-%d) mod %d = %d", mod, rem, mod, digit);
  }
  return 0;
}

static bool rpn_op_token(const char *s, char *op) {
  if (!s || !s[0]) return false;
  if (s[1] == 0 && (s[0] == '+' || s[0] == '-' || s[0] == '*' || s[0] == '/')) { *op = s[0]; return true; }
  if (strcmp(s, "plus") == 0 || strcmp(s, "add") == 0) { *op = '+'; return true; }
  if (strcmp(s, "minus") == 0 || strcmp(s, "subtract") == 0) { *op = '-'; return true; }
  if (strcmp(s, "times") == 0 || strcmp(s, "multiply") == 0 || strcmp(s, "mul") == 0) { *op = '*'; return true; }
  if (strcmp(s, "divide") == 0 || strcmp(s, "div") == 0) { *op = '/'; return true; }
  return false;
}

static int eval_rpn(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[20][48]; int na = args(s, a, 20);
  if (!starts3(s, "rpn(", "postfix(", "reversepolish(") || na < 1) return 0;
  double st[20]; int sp = 0;
  int n = add(out, 0, "Evaluate postfix/RPN with a stack.");
  for (int i = 0; i < na; ++i) {
    char op = 0;
    if (rpn_op_token(a[i], &op)) {
      if (sp < 2) return add(out, n, "Need two stack values before operator %c.", op);
      double b = st[--sp], aa = st[--sp], r = 0;
      if (op == '+') r = aa + b;
      else if (op == '-') r = aa - b;
      else if (op == '*') r = aa * b;
      else r = aa / b;
      st[sp++] = r;
      n = add(out, n, "%.10g %c %.10g = %.10g; push %.10g", aa, op, b, r, r);
    } else {
      if (sp >= 20) return add(out, n, "Stack full.");
      st[sp++] = num(a[i]);
      n = add(out, n, "read %s, push %.10g", a[i], st[sp-1]);
    }
  }
  if (sp != 1) n = add(out, n, "Stack has %d values, so expression is incomplete.", sp);
  else n = add(out, n, "answer = %.10g", st[0]);
  return n;
}

static void app_ch(char *s, int *pos, int cap, char c);
static void app_str(char *s, int *pos, int cap, const char *t);
static void app_int(char *s, int *pos, int cap, int v);

static void list_text(long long v[], int n, char *buf, int cap) {
  int p = 0; app_ch(buf, &p, cap, '[');
  for (int i = 0; i < n; ++i) {
    if (i) app_ch(buf, &p, cap, ',');
    app_int(buf, &p, cap, (int)v[i]);
  }
  app_ch(buf, &p, cap, ']');
}

static void token_add(char *buf, int *p, int cap, const char *tok) {
  if (*p > 0) app_ch(buf, p, cap, ',');
  for (int i = 0; tok[i] && *p + 1 < cap; ++i) app_ch(buf, p, cap, tok[i]);
}

static void tree_walk(char a[][48], int n, int i, int mode, char *buf, int *p, int cap) {
  if (i >= n || !a[i][0]) return;
  if (mode == 0) token_add(buf, p, cap, a[i]);
  tree_walk(a, n, 2*i + 1, mode, buf, p, cap);
  if (mode == 1) token_add(buf, p, cap, a[i]);
  tree_walk(a, n, 2*i + 2, mode, buf, p, cap);
  if (mode == 2) token_add(buf, p, cap, a[i]);
}

static bool tree_null(const char *s) {
  return !s[0] || word_is(s, "none") || word_is(s, "nil") || word_is(s, "null") || word_is(s, "-");
}

static int tree_find(char node[][48], int n, const char *s) {
  for (int i = 0; i < n; ++i) if (strcmp(node[i], s) == 0) return i;
  return -1;
}

static void tree_walk_links(char node[][48], int left[], int right[], int n, int i, int mode, char *buf, int *p, int cap) {
  if (i < 0 || i >= n) return;
  if (mode == 0) token_add(buf, p, cap, node[i]);
  tree_walk_links(node, left, right, n, left[i], mode, buf, p, cap);
  if (mode == 1) token_add(buf, p, cap, node[i]);
  tree_walk_links(node, left, right, n, right[i], mode, buf, p, cap);
  if (mode == 2) token_add(buf, p, cap, node[i]);
}

static int graph_node(char nodes[], int *nn, const char *s) {
  char c = 0;
  for (int i = 0; s[i]; ++i) if (isalnum((unsigned char)s[i])) { c = (char)tolower((unsigned char)s[i]); break; }
  if (!c) return -1;
  for (int i = 0; i < *nn; ++i) if (nodes[i] == c) return i;
  if (*nn >= 8) return -1;
  nodes[*nn] = c;
  return (*nn)++;
}

static void path_text(char nodes[], int prev[], int at, char *buf, int cap) {
  char rev[8]; int rc = 0;
  while (at >= 0 && rc < 8) { rev[rc++] = nodes[at]; at = prev[at]; }
  int p = 0;
  for (int i = rc - 1; i >= 0; --i) {
    if (p) app_str(buf, &p, cap, "->");
    app_ch(buf, &p, cap, rev[i]);
  }
}

static int eval_trace(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[32][48]; int na = args(s, a, 32);
  if ((starts2(s, "fsm(", "dfa(") || starts2(s, "fsmout(", "mealy(")) && na >= 5) {
    bool with_out = starts2(s, "fsmout(", "mealy(");
    int group = with_out ? 4 : 3;
    if ((na - 2) % group != 0) {
      return add(out, 0, with_out ? "Use fsmout(start,input,state,symbol,next,output,...)." :
                                    "Use fsm(start,input,state,symbol,next,...).");
    }
    char state[48], outputs[80] = ""; strcpy(state, a[0]);
    int n = add(out, 0, with_out ? "Trace finite state machine with outputs." :
                                   "Trace finite state machine transitions.");
    n = add(out, n, "start state = %s, input = %s", state, a[1]);
    for (int pos = 0; a[1][pos] && n < CSCALC_MAX_LINES - 2; ++pos) {
      char sym[2] = { a[1][pos], 0 };
      int found = -1;
      for (int i = 2; i + group - 1 < na; i += group) {
        if (strcmp(a[i], state) == 0 && strcmp(a[i + 1], sym) == 0) { found = i; break; }
      }
      if (found < 0) return add(out, n, "no transition for state %s on %s", state, sym);
      if (with_out) {
        int op = (int)strlen(outputs);
        if (op + (int)strlen(a[found + 3]) < (int)sizeof(outputs) - 1) strcat(outputs, a[found + 3]);
        n = add(out, n, "read %s: %s --%s/%s--> %s", sym, state, sym, a[found + 3], a[found + 2]);
      } else {
        n = add(out, n, "read %s: %s --%s--> %s", sym, state, sym, a[found + 2]);
      }
      strcpy(state, a[found + 2]);
    }
    n = add(out, n, "final state = %s", state);
    if (with_out) n = add(out, n, "output = %s", outputs);
    return n;
  }
  if (starts3(s, "dijkstra(", "shortestpath(", "shortpath(") && na >= 5) {
    char nodes[8]; int nn = 0;
    int start = graph_node(nodes, &nn, a[0]), target = graph_node(nodes, &nn, a[1]);
    int eu[12], ev[12]; long long ew[12]; int ne = 0;
    for (int i = 2; i + 2 < na && ne < 12; i += 3) {
      int u = graph_node(nodes, &nn, a[i]), v = graph_node(nodes, &nn, a[i+1]);
      if (u < 0 || v < 0) return add(out, 0, "Use edge triples: from,to,weight.");
      eu[ne] = u; ev[ne] = v; ew[ne] = parse_int(a[i+2]); ++ne;
    }
    if (start < 0 || target < 0 || ne < 1) return add(out, 0, "Use dijkstra(start,end,from,to,weight,...).");
    const long long INF = 9000000000LL;
    long long dist[8]; int prev[8]; bool used[8];
    for (int i = 0; i < nn; ++i) { dist[i] = INF; prev[i] = -1; used[i] = false; }
    dist[start] = 0;
    int n = add(out, 0, "Dijkstra: fix the unvisited node with smallest distance.");
    n = add(out, n, "Edges are treated as undirected.");
    while (n < CSCALC_MAX_LINES - 2) {
      int u = -1;
      for (int i = 0; i < nn; ++i) if (!used[i] && dist[i] < INF && (u < 0 || dist[i] < dist[u])) u = i;
      if (u < 0) break;
      used[u] = true;
      n = add(out, n, "fix %c at %lld", nodes[u], dist[u]);
      if (u == target) break;
      for (int e = 0; e < ne && n < CSCALC_MAX_LINES - 2; ++e) {
        int v = eu[e] == u ? ev[e] : ev[e] == u ? eu[e] : -1;
        if (v < 0 || used[v]) continue;
        long long cand = dist[u] + ew[e];
        if (cand < dist[v]) {
          n = add(out, n, "%c->%c: %lld+%lld=%lld, update", nodes[u], nodes[v], dist[u], ew[e], cand);
          dist[v] = cand; prev[v] = u;
        }
      }
    }
    if (dist[target] >= INF) return add(out, n, "No route found from %c to %c.", nodes[start], nodes[target]);
    char path[40] = ""; path_text(nodes, prev, target, path, sizeof(path));
    n = add(out, n, "shortest path: %s", path);
    return add(out, n, "distance = %lld", dist[target]);
  }
  if ((starts3(s, "preordertree(", "treeprelinks(", "prelinks(") ||
       starts3(s, "inordertree(", "treeinlinks(", "inlinks(") ||
       starts3(s, "postordertree(", "treepostlinks(", "postlinks(")) && na >= 4) {
    int mode = 2;
    if (starts3(s, "preordertree(", "treeprelinks(", "prelinks(")) mode = 0;
    else if (starts3(s, "inordertree(", "treeinlinks(", "inlinks(")) mode = 1;
    const char *name = mode == 0 ? "Pre-order" : mode == 1 ? "In-order" : "Post-order";
    const char *rule = mode == 0 ? "root, left, right" : mode == 1 ? "left, root, right" : "left, right, root";
    if ((na - 1) % 3 != 0) return add(out, 0, "Use root,node,left,right triples.");
    char node[12][48]; int left[12], right[12], nc = 0;
    for (int i = 1; i + 2 < na && nc < 12; i += 3) {
      strcpy(node[nc], a[i]); left[nc] = right[nc] = -1; ++nc;
    }
    for (int i = 1, j = 0; i + 2 < na && j < nc; i += 3, ++j) {
      if (!tree_null(a[i + 1])) left[j] = tree_find(node, nc, a[i + 1]);
      if (!tree_null(a[i + 2])) right[j] = tree_find(node, nc, a[i + 2]);
      if ((!tree_null(a[i + 1]) && left[j] < 0) || (!tree_null(a[i + 2]) && right[j] < 0)) {
        return add(out, 0, "Every child must also have a node triple.");
      }
    }
    int root = tree_find(node, nc, a[0]);
    if (root < 0) return add(out, 0, "Root must match one node triple.");
    char result[96] = ""; int p = 0; tree_walk_links(node, left, right, nc, root, mode, result, &p, sizeof(result));
    int n = add(out, 0, "Use explicit binary-tree links: node,left,right.");
    n = add(out, n, "root = %s", a[0]);
    n = add(out, n, "%s traversal uses %s.", name, rule);
    return add(out, n, "%s: %s", name, result);
  }
  if ((starts3(s, "preorder(", "treepre(", "pretraverse(") ||
       starts3(s, "inorder(", "treein(", "intraverse(") ||
       starts3(s, "postorder(", "treepost(", "posttraverse(")) && na >= 1) {
    int mode = 2;
    if (starts3(s, "preorder(", "treepre(", "pretraverse(")) mode = 0;
    else if (starts3(s, "inorder(", "treein(", "intraverse(")) mode = 1;
    const char *name = mode == 0 ? "Pre-order" : mode == 1 ? "In-order" : "Post-order";
    const char *rule = mode == 0 ? "root, left, right" : mode == 1 ? "left, root, right" : "left, right, root";
    char result[96] = ""; int p = 0; tree_walk(a, na, 0, mode, result, &p, sizeof(result));
    int n = add(out, 0, "Use level-order input: node i has left 2i+1 and right 2i+2.");
    n = add(out, n, "%s traversal uses %s.", name, rule);
    return add(out, n, "%s: %s", name, result);
  }
  if (starts3(s, "stack(", "stacktrace(", "pushpop(") && na >= 1) {
    long long st[16]; int sp = 0;
    char buf[80]; int n = add(out, 0, "Stack is LIFO: last in, first out.");
    for (int i = 0; i < na && n < CSCALC_MAX_LINES - 1; ++i) {
      if (word_is(a[i], "pop")) {
        if (sp <= 0) return add(out, n, "pop from empty stack: underflow");
        long long x = st[--sp]; list_text(st, sp, buf, sizeof(buf));
        n = add(out, n, "pop %lld -> stack %s", x, buf);
      } else {
        if (sp >= 16) return add(out, n, "push to full stack: overflow");
        st[sp++] = parse_int(a[i]); list_text(st, sp, buf, sizeof(buf));
        n = add(out, n, "push %lld -> stack %s", st[sp-1], buf);
      }
    }
    return n;
  }
  if (starts3(s, "queue(", "queuetrace(", "enqueue(") && na >= 1) {
    long long q[16]; int len = 0;
    char buf[80]; int n = add(out, 0, "Queue is FIFO: first in, first out.");
    for (int i = 0; i < na && n < CSCALC_MAX_LINES - 1; ++i) {
      if (word_is(a[i], "pop") || word_is(a[i], "dequeue")) {
        if (len <= 0) return add(out, n, "dequeue from empty queue: underflow");
        long long x = q[0]; for (int j = 1; j < len; ++j) q[j-1] = q[j]; --len;
        list_text(q, len, buf, sizeof(buf));
        n = add(out, n, "dequeue %lld -> queue %s", x, buf);
      } else {
        if (len >= 16) return add(out, n, "enqueue to full queue: overflow");
        q[len++] = parse_int(a[i]); list_text(q, len, buf, sizeof(buf));
        n = add(out, n, "enqueue %lld -> queue %s", q[len-1], buf);
      }
    }
    return n;
  }
  if (starts3(s, "binarysearch(", "binsearch(", "bsearch(") && na >= 2) {
    long long target = parse_int(a[0]), v[15]; int nvals = na - 1;
    for (int i = 0; i < nvals; ++i) v[i] = parse_int(a[i+1]);
    int n = add(out, 0, "Binary search: compare middle item of sorted list.");
    n = add(out, n, "target = %lld", target);
    int lo = 0, hi = nvals - 1;
    while (lo <= hi && n < CSCALC_MAX_LINES - 1) {
      int mid = (lo + hi) / 2;
      if (v[mid] == target) return add(out, add(out, n, "low=%d high=%d mid=%d value=%lld", lo+1, hi+1, mid+1, v[mid]), "found at position %d", mid+1);
      n = add(out, n, "low=%d high=%d mid=%d value=%lld -> %s", lo+1, hi+1, mid+1, v[mid], target < v[mid] ? "left" : "right");
      if (target < v[mid]) hi = mid - 1; else lo = mid + 1;
    }
    return add(out, n, "not found");
  }
  if (starts3(s, "linearsearch(", "linsearch(", "seqsearch(") && na >= 2) {
    long long target = parse_int(a[0]);
    int n = add(out, 0, "Linear search: check each item in order.");
    for (int i = 1; i < na && n < CSCALC_MAX_LINES - 1; ++i) {
      long long x = parse_int(a[i]);
      if (x == target) return add(out, add(out, n, "position %d: %lld = target", i, x), "found at position %d", i);
      n = add(out, n, "position %d: %lld != target", i, x);
    }
    return add(out, n, "not found");
  }
  if (starts2(s, "bubblesort(", "bubble(") && na >= 2) {
    long long v[16]; for (int i = 0; i < na; ++i) v[i] = parse_int(a[i]);
    char buf[80]; list_text(v, na, buf, sizeof(buf));
    int n = add(out, 0, "Bubble sort: swap adjacent items if left > right.");
    n = add(out, n, "start %s", buf);
    for (int pass = 0; pass < na - 1 && n < CSCALC_MAX_LINES - 1; ++pass) {
      bool swapped = false;
      for (int i = 0; i < na - 1 - pass; ++i) if (v[i] > v[i+1]) { long long t = v[i]; v[i] = v[i+1]; v[i+1] = t; swapped = true; }
      list_text(v, na, buf, sizeof(buf));
      n = add(out, n, "pass %d: %s", pass + 1, buf);
      if (!swapped) break;
    }
    return n;
  }
  if (starts2(s, "insertionsort(", "insertion(") && na >= 2) {
    long long v[16]; for (int i = 0; i < na; ++i) v[i] = parse_int(a[i]);
    char buf[80]; list_text(v, na, buf, sizeof(buf));
    int n = add(out, 0, "Insertion sort: insert next item into sorted left part.");
    n = add(out, n, "start %s", buf);
    for (int i = 1; i < na && n < CSCALC_MAX_LINES - 1; ++i) {
      long long key = v[i]; int j = i - 1;
      while (j >= 0 && v[j] > key) { v[j+1] = v[j]; --j; }
      v[j+1] = key; list_text(v, na, buf, sizeof(buf));
      n = add(out, n, "insert %lld: %s", key, buf);
    }
    return n;
  }
  if (starts2(s, "selectionsort(", "selection(") && na >= 2) {
    long long v[16]; for (int i = 0; i < na; ++i) v[i] = parse_int(a[i]);
    char buf[80]; list_text(v, na, buf, sizeof(buf));
    int n = add(out, 0, "Selection sort: find smallest in unsorted part.");
    n = add(out, n, "start %s", buf);
    for (int i = 0; i < na - 1 && n < CSCALC_MAX_LINES - 1; ++i) {
      int m = i;
      for (int j = i + 1; j < na; ++j) if (v[j] < v[m]) m = j;
      long long t = v[i]; v[i] = v[m]; v[m] = t;
      list_text(v, na, buf, sizeof(buf));
      n = add(out, n, "place %lld at position %d: %s", v[i], i + 1, buf);
    }
    return n;
  }
  if (starts2(s, "mergesort(", "merge(") && na >= 2) {
    long long v[16], tmp[16]; for (int i = 0; i < na; ++i) v[i] = parse_int(a[i]);
    char buf[80]; list_text(v, na, buf, sizeof(buf));
    int n = add(out, 0, "Merge sort: split to single items, then merge sorted runs.");
    n = add(out, n, "start %s", buf);
    for (int w = 1; w < na && n < CSCALC_MAX_LINES - 1; w *= 2) {
      for (int l = 0; l < na; l += 2 * w) {
        int m = l + w, r = l + 2 * w; if (m > na) m = na; if (r > na) r = na;
        int i = l, j = m, k = l;
        while (i < m && j < r) tmp[k++] = v[i] <= v[j] ? v[i++] : v[j++];
        while (i < m) tmp[k++] = v[i++];
        while (j < r) tmp[k++] = v[j++];
      }
      for (int i = 0; i < na; ++i) v[i] = tmp[i];
      list_text(v, na, buf, sizeof(buf));
      n = add(out, n, "merge runs of %d: %s", w, buf);
    }
    return n;
  }
  return 0;
}

static int eval_storage(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[10][48]; int na = args(s, a, 10);
  if (starts3(s, "image(", "bitmap(", "imagesize(") && na >= 3) {
    long long bits = parse_int(a[0]) * parse_int(a[1]) * parse_int(a[2]);
    double bytes = bits / 8.0;
    int n = add(out, 0, "Image bits = width * height * colour depth.");
    n = add(out, n, "%s*%s*%s = %lld bits", a[0], a[1], a[2], bits);
    n = add(out, n, "= %.6g bytes", bytes);
    return add(out, n, "= %.6g MB", bytes / 1000000.0);
  }
  if (starts3(s, "imagecolors(", "bitmapcolors(", "colours(") && na >= 3) {
    int depth = ceil_log2_ll(parse_int(a[2]));
    long long bits = parse_int(a[0]) * parse_int(a[1]) * depth;
    double bytes = bits / 8.0;
    int n = add(out, 0, "Colour depth = ceil(log2(number of colours)).");
    n = add(out, n, "ceil(log2(%s)) = %d bits per pixel", a[2], depth);
    n = add(out, n, "Image bits = width * height * colour depth.");
    n = add(out, n, "%s*%s*%d = %lld bits = %.6g bytes", a[0], a[1], depth, bits, bytes);
    return add(out, n, "= %.6g MB", bytes / 1000000.0);
  }
  if (starts3(s, "sound(", "audio(", "soundsize(") && na >= 3) {
    long long chans = na > 3 ? parse_int(a[3]) : 1;
    double bits = num(a[0]) * num(a[1]) * num(a[2]) * chans;
    double bytes = bits / 8.0;
    int n = add(out, 0, "Sound bits = sample rate * seconds * resolution * channels.");
    n = add(out, n, "%s*%s*%s*%lld = %.10g bits", a[0], a[1], a[2], chans, bits);
    n = add(out, n, "= %.10g bytes", bytes);
    return add(out, n, "= %.10g MB", bytes / 1000000.0);
  }
  if (starts3(s, "bitrate(", "datarate(", "rate(") && na >= 2) {
    double rate = num(a[0]) / num(a[1]);
    int n = add(out, 0, "Bit rate = bits / seconds.");
    return add(out, n, "%s/%s = %.10g bit/s", a[0], a[1], rate);
  }
  if (starts3(s, "transfer(", "transfertime(", "time(") && na >= 2) {
    double t = num(a[0]) / num(a[1]);
    int n = add(out, 0, "Transfer time = file size / bit rate.");
    return add(out, n, "%s/%s = %.10g s", a[0], a[1], t);
  }
  if (starts3(s, "transfermb(", "megabytetransfer(", "mbtombit(") && na >= 2) {
    double mb = num(a[0]), rate = num(a[1]), megabits = mb * 8.0, t = megabits / rate;
    int n = add(out, 0, "Convert megabytes to megabits before dividing by Mbit/s.");
    n = add(out, n, "%.10g MB = %.10g Mbit", mb, megabits);
    return add(out, n, "time = %.10g/%.10g = %.10g s", megabits, rate, t);
  }
  if (starts3(s, "transferkb(", "kilobytetransfer(", "kbtokbit(") && na >= 2) {
    double kb = num(a[0]), rate = num(a[1]), kilobits = kb * 8.0, t = kilobits / rate;
    int n = add(out, 0, "Convert kilobytes to kilobits before dividing by kbit/s.");
    n = add(out, n, "%.10g KB = %.10g kbit", kb, kilobits);
    return add(out, n, "time = %.10g/%.10g = %.10g s", kilobits, rate, t);
  }
  if (starts3(s, "ascii(", "charcode(", "codepoint(") && na >= 1) {
    if (isdigit((unsigned char)a[0][0])) return add_code_lines(out, (int)parse_int(a[0]), false, -1);
    if (a[0][0] && !a[0][1]) return add_code_lines(out, (unsigned char)a[0][0], false, (unsigned char)a[0][0]);
  }
  if (starts3(s, "unicode(", "unicodepoint(", "ucode(") && na >= 1) {
    if (isdigit((unsigned char)a[0][0])) return add_code_lines(out, (int)parse_int(a[0]), true, -1);
    if (a[0][0] && !a[0][1]) return add_code_lines(out, (unsigned char)a[0][0], true, (unsigned char)a[0][0]);
  }
  if (starts3(s, "chars(", "textsize(", "characters(") && na >= 2) {
    long long bits = parse_int(a[0]) * parse_int(a[1]);
    double bytes = bits / 8.0;
    int n = add(out, 0, "Text bits = characters * bits per character.");
    n = add(out, n, "%s*%s = %lld bits = %.6g bytes", a[0], a[1], bits, bytes);
    return add(out, n, "= %.6g MB", bytes / 1000000.0);
  }
  if (starts3(s, "charset(", "charsetsize(", "textsymbols(") && na >= 2) {
    int bpc = ceil_log2_ll(parse_int(a[1]));
    long long bits = parse_int(a[0]) * bpc;
    double bytes = bits / 8.0;
    int n = add(out, 0, "Bits per character = ceil(log2(character set size)).");
    n = add(out, n, "ceil(log2(%s)) = %d bits per character", a[1], bpc);
    n = add(out, n, "text bits = %s*%d = %lld bits", a[0], bpc, bits);
    return add(out, n, "= %.6g bytes", bytes);
  }
  if (starts3(s, "compress(", "compression(", "ratio(") && na >= 2) {
    double oldv = num(a[0]), newv = num(a[1]);
    int n = add(out, 0, "Compression ratio = original / compressed.");
    n = add(out, n, "ratio = %.10g : 1", oldv / newv);
    return add(out, n, "saving = %.6g%%", (oldv - newv) * 100.0 / oldv);
  }
  if (starts3(s, "rle(", "runlength(", "runlengthencoding(") && na >= 3) {
    long long bits = parse_int(a[0]) * (parse_int(a[1]) + parse_int(a[2]));
    int n = add(out, 0, "Run-length bits = runs * (symbol bits + count bits).");
    return add(out, n, "%s*(%s+%s) = %lld bits = %.6g bytes", a[0], a[1], a[2], bits, bits / 8.0);
  }
  if (starts3(s, "rletext(", "rlestring(", "runencode(") && na >= 3) {
    int runs = 0, len = (int)strlen(a[0]);
    char summary[72] = ""; int sp = 0;
    for (int i = 0; i < len;) {
      int j = i + 1; while (j < len && a[0][j] == a[0][i]) ++j;
      if (runs && sp + 1 < 72) summary[sp++] = ',';
      if (sp + 8 < 72) { summary[sp++] = a[0][i]; summary[sp++] = 'x'; int c = j - i; if (c >= 10) summary[sp++] = (char)('0' + c / 10); summary[sp++] = (char)('0' + c % 10); summary[sp] = 0; }
      runs++; i = j;
    }
    long long orig = (long long)len * parse_int(a[1]);
    long long enc = (long long)runs * (parse_int(a[1]) + parse_int(a[2]));
    int n = add(out, 0, "Run-length encode consecutive repeated symbols.");
    n = add(out, n, "runs: %s", summary);
    n = add(out, n, "original bits = %d*%s = %lld", len, a[1], orig);
    return add(out, n, "encoded bits = %d*(%s+%s) = %lld", runs, a[1], a[2], enc);
  }
  if (starts3(s, "huffman(", "huff(", "huffmancode(") && na >= 2) {
    char leaf[10]; long long lw[10]; int depth[10] = {0};
    char nsym[20][32]; long long nw[20]; int active[20];
    int lc = 0, nc = 0;
    for (int i = 0; i < na && lc < 10; ++i) {
      char c = 0;
      for (int j = 0; a[i][j]; ++j) if (isalpha((unsigned char)a[i][j])) { c = (char)toupper((unsigned char)a[i][j]); break; }
      if (!c) c = (char)('A' + lc);
      leaf[lc] = c; lw[lc] = parse_int(a[i]);
      nsym[nc][0] = c; nsym[nc][1] = 0; nw[nc] = lw[lc]; active[nc] = 1;
      ++lc; ++nc;
    }
    if (lc < 2) return add(out, 0, "Huffman needs at least 2 symbols.");
    int n = add(out, 0, "Build Huffman tree: combine two smallest weights.");
    while (true) {
      int a1 = -1, a2 = -1;
      for (int i = 0; i < nc; ++i) if (active[i]) {
        if (a1 < 0 || nw[i] < nw[a1]) { a2 = a1; a1 = i; }
        else if (a2 < 0 || nw[i] < nw[a2]) a2 = i;
      }
      if (a2 < 0 || nc >= 20) break;
      if (n < CSCALC_MAX_LINES - 3) n = add(out, n, "%s(%lld)+%s(%lld)=%lld", nsym[a1], nw[a1], nsym[a2], nw[a2], nw[a1] + nw[a2]);
      for (int k = 0; k < lc; ++k) if (strchr(nsym[a1], leaf[k]) || strchr(nsym[a2], leaf[k])) depth[k]++;
      sprintf(nsym[nc], "%s%s", nsym[a1], nsym[a2]);
      nw[nc] = nw[a1] + nw[a2]; active[nc] = 1; active[a1] = active[a2] = 0; ++nc;
    }
    char ls[80] = ""; int p = 0; long long bits = 0, total = 0;
    for (int i = 0; i < lc; ++i) {
      if (i) app_ch(ls, &p, 80, ',');
      app_ch(ls, &p, 80, leaf[i]); app_ch(ls, &p, 80, ':'); app_int(ls, &p, 80, depth[i]);
      bits += lw[i] * depth[i]; total += lw[i];
    }
    int fixed = ceil_log2_ll(lc);
    n = add(out, n, "code lengths: %s", ls);
    n = add(out, n, "encoded bits = sum(freq*length) = %lld", bits);
    return add(out, n, "fixed length = %lld*%d = %lld bits", total, fixed, total * fixed);
  }
  if (starts3(s, "sqlselect(", "selectwhere(", "sqlquery(") && na >= 5) {
    const char *op = sql_op_text(a[3]);
    int n = add(out, 0, "SQL SELECT: choose fields, choose table, then apply WHERE.");
    n = add(out, n, "SELECT %s", a[1]);
    n = add(out, n, "FROM %s", a[0]);
    n = add(out, n, "WHERE %s %s %s", a[2], op, a[4]);
    return add(out, n, "query = SELECT %s FROM %s WHERE %s %s %s", a[1], a[0], a[2], op, a[4]);
  }
  if (starts3(s, "sqlcount(", "countwhere(", "countrecords(") && na >= 4) {
    const char *op = sql_op_text(a[2]);
    int n = add(out, 0, "SQL COUNT: filter rows with WHERE, then count matching records.");
    n = add(out, n, "SELECT COUNT(*)");
    n = add(out, n, "FROM %s", a[0]);
    n = add(out, n, "WHERE %s %s %s", a[1], op, a[3]);
    return add(out, n, "query = SELECT COUNT(*) FROM %s WHERE %s %s %s", a[0], a[1], op, a[3]);
  }
  if (starts3(s, "records(", "recordsize(", "database(") && na >= 2) {
    double bytes = num(a[0]) * num(a[1]);
    int n = add(out, 0, "File size = records * bytes per record.");
    n = add(out, n, "%s*%s = %.10g bytes", a[0], a[1], bytes);
    return add(out, n, "= %.10g MB", bytes / 1000000.0);
  }
  if (starts3(s, "addressspace(", "addresses(", "addressbus(") && na >= 1) {
    int bits = (int)parse_int(a[0]);
    double addresses = pow2(bits);
    int n = add(out, 0, "Addressable locations = 2^(address bits).");
    return add(out, n, "2^%d = %.10g addresses", bits, addresses);
  }
  if (starts3(s, "memorycapacity(", "addresscapacity(", "memorybus(") && na >= 2) {
    int abits = (int)parse_int(a[0]), wbits = (int)parse_int(a[1]);
    double addresses = pow2(abits), bits = addresses * wbits, bytes = bits / 8.0;
    int n = add(out, 0, "Memory capacity = addressable locations * word size.");
    n = add(out, n, "locations = 2^%d = %.10g", abits, addresses);
    n = add(out, n, "bits = %.10g*%d = %.10g", addresses, wbits, bits);
    return add(out, n, "= %.10g bytes", bytes);
  }
  if (starts3(s, "hashmod(", "hashtable(", "modhash(") && na >= 2) {
    long long size = parse_int(a[0]);
    if (size <= 0) return add(out, 0, "Table size must be positive.");
    long long seen[10]; int ns = 0;
    int n = add(out, 0, "Hash address = key mod table size.");
    n = add(out, n, "table size = %lld", size);
    for (int i = 1; i < na; ++i) {
      long long key = parse_int(a[i]);
      long long addr = key % size; if (addr < 0) addr += size;
      bool collision = false;
      for (int j = 0; j < ns; ++j) if (seen[j] == addr) collision = true;
      if (ns < 10) seen[ns++] = addr;
      if (collision) n = add(out, n, "%lld mod %lld = %lld, collision", key, size, addr);
      else n = add(out, n, "%lld mod %lld = %lld", key, size, addr);
    }
    return n;
  }
  if (starts3(s, "hashlinear(", "linearprobe(", "hashprobe(") && na >= 2) {
    long long size = parse_int(a[0]);
    if (size <= 0 || size > 64) return add(out, 0, "Use table size 1 to 64.");
    int used[64] = {0};
    int n = add(out, 0, "Insert using hash address = key mod table size.");
    n = add(out, n, "If occupied, use linear probing: try next slot.");
    for (int i = 1; i < na; ++i) {
      long long key = parse_int(a[i]);
      int home = (int)(key % size); if (home < 0) home += (int)size;
      int slot = home, probes = 0;
      while (used[slot] && probes < size) { slot = (slot + 1) % (int)size; probes++; }
      if (probes >= size) return add(out, n, "Table full before inserting %lld.", key);
      used[slot] = 1;
      if (probes == 0) n = add(out, n, "%lld mod %lld = %d, place at %d", key, size, home, slot);
      else n = add(out, n, "%lld mod %lld = %d occupied; probe %d, place at %d", key, size, home, probes, slot);
    }
    return n;
  }
  return 0;
}

static int eval_float(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts2(s, "fixed(", "fixeddec(") && na == 1) {
    double v = fixed_decode(a[0]);
    int n = add(out, 0, "Add binary fixed-point place values.");
    return add(out, n, "%s_2 = %.10g_10", a[0], v);
  }
  if (starts3(s, "fixedtc(", "fixedtwos(", "fixedtwosdec(") && na == 1) {
    double v = fixed_tc_decode(a[0]);
    int n = add(out, 0, "Decode the whole part as two's complement, then add fractional places.");
    return add(out, n, "%s_2 = %.10g_10", a[0], v);
  }
  if (starts3(s, "fixedenc(", "fixedencode(", "fixedpointenc(") && na >= 3) {
    double value = num(a[0]); int whole = (int)parse_int(a[1]), frac = (int)parse_int(a[2]), total = whole + frac;
    long long scaled = (long long)round_nearest(value * pow2(frac));
    char bits[65], fixed[65]; to_bin(scaled, total, bits); insert_point(bits, whole, fixed);
    int n = add(out, 0, "Encode fixed point by scaling by 2^fraction bits.");
    n = add(out, n, "scaled integer = %.10g * 2^%d = %lld", value, frac, scaled);
    n = add(out, n, "%d whole bits and %d fractional bits.", whole, frac);
    return add(out, n, "fixed point = %s", fixed);
  }
  if (starts3(s, "fixedtcenc(", "fixedtwosenc(", "fixedtwosencode(") && na >= 3) {
    double value = num(a[0]); int whole = (int)parse_int(a[1]), frac = (int)parse_int(a[2]), total = whole + frac;
    long long scaled = (long long)round_nearest(value * pow2(frac));
    char bits[65], fixed[65]; to_bin(scaled, total, bits); insert_point(bits, whole, fixed);
    int n = add(out, 0, "Encode signed fixed point as a two's-complement scaled integer.");
    n = add(out, n, "scaled integer = %.10g * 2^%d = %lld", value, frac, scaled);
    n = add(out, n, "write %lld in %d-bit two's complement.", scaled, total);
    return add(out, n, "fixed point = %s", fixed);
  }
  if ((starts(s, "floatdec(") || starts(s, "fpdec(") || starts(s, "floatdecode(")) && na >= 2) {
    double m = mantissa_decode(a[0]);
    int e = twos_decode(a[1]);
    int n = add(out, 0, "Mantissa uses two's complement with point after sign bit.");
    n = add(out, n, "mantissa %s = %.10g", a[0], m);
    n = add(out, n, "exponent %s = %d", a[1], e);
    return add(out, n, "value = %.10g * 2^%d = %.10g", m, e, m * pow2(e));
  }
  if ((starts(s, "floatnorm(") || starts(s, "fpnorm(") || starts(s, "normalise(") || starts(s, "normalize(")) && na >= 2) {
    double m = mantissa_decode(a[0]);
    int e = twos_decode(a[1]);
    int mb = (int)strlen(a[0]), eb = (int)strlen(a[1]), lefts = 0, rights = 0;
    int n = add(out, 0, "Normalise so the first two mantissa bits differ.");
    n = add(out, n, "start: %.10g * 2^%d", m, e);
    while (m != 0 && m > -0.5 && m < 0.5 && lefts < mb + eb + 2) { m *= 2.0; e--; lefts++; }
    while ((m >= 1.0 || m < -1.0) && rights < mb + eb + 4) { m /= 2.0; e++; rights++; }
    char mant[65], expb[65]; mantissa_encode(m, mb, mant); to_bin(e, eb, expb);
    if (lefts) n = add(out, n, "shift mantissa left %d place(s), subtract %d from exponent.", lefts, lefts);
    if (rights) n = add(out, n, "shift mantissa right %d place(s), add %d to exponent.", rights, rights);
    n = add(out, n, "mantissa = %s", mant);
    return add(out, n, "exponent = %s", expb);
  }
  if (starts(s, "normal(") && na >= 1) {
    int n = add(out, 0, "Normalised AQA mantissa starts 01 if positive, 10 if negative.");
    bool ok = (a[0][0] == '0' && a[0][1] == '1') || (a[0][0] == '1' && a[0][1] == '0');
    return add(out, n, "%s is %snormalised", a[0], ok ? "" : "not ");
  }
  if ((starts(s, "floatenc(") || starts(s, "fpenc(") || starts(s, "floatencode(")) && na >= 3) {
    double value = num(a[0]); int mb = (int)parse_int(a[1]), eb = (int)parse_int(a[2]);
    int e = 0; double m = value;
    if (m != 0) {
      while (m >= 1.0 || m < -1.0) { m /= 2.0; ++e; }
      while (m > -0.5 && m < 0.5) { m *= 2.0; --e; }
    }
    char mant[65], expb[65]; mantissa_encode(m, mb, mant); to_bin(e, eb, expb);
    int n = add(out, 0, "Normalise so mantissa starts 01 for positive or 10 for negative.");
    n = add(out, n, "%.10g = %.10g * 2^%d", value, m, e);
    n = add(out, n, "mantissa (%d bits) = %s", mb, mant);
    return add(out, n, "exponent (%d bits) = %s", eb, expb);
  }
  if (starts3(s, "floatprecision(", "fpprecision(", "floatstep(") && na >= 2) {
    int mb = (int)parse_int(a[0]), e = (int)parse_int(a[1]);
    double step = pow2(e - (mb - 1));
    int n = add(out, 0, "Precision is the value of the last mantissa bit after scaling.");
    n = add(out, n, "step = 2^(exponent-(mantissa bits-1))");
    return add(out, n, "step = 2^(%d-(%d-1)) = %.10g", e, mb, step);
  }
  if (starts3(s, "floatnearest(", "fpnearest(", "closestfloat(") && na >= 3) {
    double value = num(a[0]); int mb = (int)parse_int(a[1]), eb = (int)parse_int(a[2]);
    int e = 0; double m = value;
    if (m != 0) {
      while (m >= 1.0 || m < -1.0) { m /= 2.0; ++e; }
      while (m > -0.5 && m < 0.5) { m *= 2.0; --e; }
    }
    double step = pow2(e - (mb - 1));
    double rounded = round_nearest(value / step) * step;
    double rm = rounded; int re = 0;
    if (rm != 0) {
      while (rm >= 1.0 || rm < -1.0) { rm /= 2.0; ++re; }
      while (rm > -0.5 && rm < 0.5) { rm *= 2.0; --re; }
    }
    char mant[65], expb[65]; mantissa_encode(rm, mb, mant); to_bin(re, eb, expb);
    int n = add(out, 0, "Find the closest representable floating-point value.");
    n = add(out, n, "%.10g = %.10g * 2^%d", value, m, e);
    n = add(out, n, "step at this exponent = 2^(%d-(%d-1)) = %.10g", e, mb, step);
    n = add(out, n, "nearest multiple = %.10g", rounded);
    n = add(out, n, "mantissa (%d bits) = %s", mb, mant);
    return add(out, n, "exponent (%d bits) = %s", eb, expb);
  }
  if (starts3(s, "floatrange(", "fprange(", "realrange(") && na >= 2) {
    int mb = (int)parse_int(a[0]), eb = (int)parse_int(a[1]);
    int emin = -(1 << (eb - 1)), emax = (1 << (eb - 1)) - 1;
    double step = pow2(-(mb - 1));
    double minpos = 0.5 * pow2(emin);
    double maxpos = (1.0 - step) * pow2(emax);
    int n = add(out, 0, "Exponent is %d-bit two's complement.", eb);
    n = add(out, n, "exponent range = %d to %d", emin, emax);
    n = add(out, n, "mantissa step = 2^-(%d) = %.10g", mb - 1, step);
    n = add(out, n, "smallest positive normal = 0.5*2^%d = %.10g", emin, minpos);
    return add(out, n, "largest positive = (1-step)*2^%d = %.10g", emax, maxpos);
  }
  return 0;
}

struct BParser {
  const char *s; int pos; int mask; char vars[8]; int vc;
  int var_value(char c) {
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < vc; ++i) if (vars[i] == c) return (mask >> (vc - 1 - i)) & 1;
    return 0;
  }
  bool eat(char c) { if (s[pos] == c) { pos++; return true; } return false; }
  int factor() {
    if (eat('!') || eat('~')) return !factor();
    if (eat('(')) { int v = expr(); eat(')'); if (eat('\'')) v = !v; return v; }
    char c = s[pos++];
    int v = isalpha((unsigned char)c) ? var_value(c) : (c == '1');
    if (eat('\'')) v = !v;
    return v;
  }
  int term() {
    int v = factor();
    while (s[pos] == '&' || s[pos] == '*' || s[pos] == '.') {
      pos++;
      int r = factor();
      v = v && r;
    }
    return v;
  }
  int expr() {
    int v = term();
    while (s[pos] == '+' || s[pos] == '|' || s[pos] == '^' || s[pos] == '@' || s[pos] == '#') {
      char op = s[pos++]; int r = term();
      if (op == '^') v = v != r;
      else if (op == '@') v = !(v && r);
      else if (op == '#') v = !(v || r);
      else v = v || r;
    }
    return v;
  }
};

static void collect_vars(const char *e, char *vars, int *vc) {
  *vc = 0;
  for (int i = 0; e[i]; ++i) if (isalpha((unsigned char)e[i])) {
    char c = (char)toupper((unsigned char)e[i]);
    if (c < 'A' || c > 'Z') continue;
    bool seen = false; for (int j = 0; j < *vc; ++j) if (vars[j] == c) seen = true;
    if (!seen && *vc < 6) vars[(*vc)++] = c;
  }
}

static bool bool_equiv_expr(const char *a, const char *b) {
  char both[192];
  sprintf(both, "%s%s", a, b);
  char vars[8]; int vc; collect_vars(both, vars, &vc);
  if (vc > 6) return false;
  int rows = 1 << vc;
  for (int m = 0; m < rows; ++m) {
    BParser pa = { a, 0, m, {0}, vc }, pb = { b, 0, m, {0}, vc };
    for (int i = 0; i < vc; ++i) pa.vars[i] = pb.vars[i] = vars[i];
    if (pa.expr() != pb.expr()) return false;
  }
  return true;
}

static void app_ch(char *s, int *pos, int cap, char c);
static void app_str(char *s, int *pos, int cap, const char *t);

static void bool_norm(const char *src, char *dst, int cap) {
  int j = 0;
  for (int i = 0; src[i] && j + 2 < cap;) {
    if (strncmp(src + i, "nand", 4) == 0) { dst[j++] = '@'; i += 4; continue; }
    if (strncmp(src + i, "nor", 3) == 0) { dst[j++] = '#'; i += 3; continue; }
    if (strncmp(src + i, "xor", 3) == 0) { dst[j++] = '^'; i += 3; continue; }
    if (strncmp(src + i, "and", 3) == 0) { dst[j++] = '&'; i += 3; continue; }
    if (strncmp(src + i, "not", 3) == 0) { dst[j++] = '!'; i += 3; continue; }
    if (strncmp(src + i, "or", 2) == 0) { dst[j++] = '+'; i += 2; continue; }
    dst[j++] = src[i++];
  }
  dst[j] = 0;
  char tmp[96]; int k = 0;
  for (int i = 0; dst[i] && k + 2 < (int)sizeof(tmp); ++i) {
    char c = dst[i], n = dst[i+1];
    tmp[k++] = c;
    bool left = isalpha((unsigned char)c) || c == ')' || c == '\'';
    bool right = isalpha((unsigned char)n) || n == '(' || n == '!';
    if (left && right) tmp[k++] = '&';
  }
  tmp[k] = 0;
  int p = 0; app_str(dst, &p, cap, tmp);
}

struct Imp { int bits, mask, used; };

static int bitcount(int x) { int c=0; while (x) { c += x & 1; x >>= 1; } return c; }

static bool covers(const Imp &p, int m) { return (m & ~p.mask) == (p.bits & ~p.mask); }

static void app_ch(char *s, int *pos, int cap, char c) {
  if (*pos + 1 >= cap) return;
  s[(*pos)++] = c; s[*pos] = 0;
}

static void app_str(char *s, int *pos, int cap, const char *t) {
  for (int i = 0; t[i]; ++i) app_ch(s, pos, cap, t[i]);
}

static void app_int(char *s, int *pos, int cap, int v) {
  char tmp[16]; int n = 0;
  if (v == 0) { app_ch(s, pos, cap, '0'); return; }
  if (v < 0) { app_ch(s, pos, cap, '-'); v = -v; }
  while (v && n < 15) { tmp[n++] = (char)('0' + (v % 10)); v /= 10; }
  while (n--) app_ch(s, pos, cap, tmp[n]);
}

static void imp_text(const Imp &p, const char *vars, int vc, char *buf) {
  int j = 0;
  for (int i = 0; i < vc; ++i) {
    int bit = 1 << (vc - 1 - i);
    if (p.mask & bit) continue;
    buf[j++] = vars[i];
    if (!(p.bits & bit)) buf[j++] = '\'';
  }
  if (j == 0) buf[j++] = '1';
  buf[j] = 0;
}

static void strip_outer(const char *s, char *b, int cap) {
  int len = (int)strlen(s);
  if (len > 1 && s[0] == '(' && s[len-1] == ')') {
    int d = 0; bool outer = true;
    for (int i = 0; i < len - 1; ++i) {
      if (s[i] == '(') d++;
      if (s[i] == ')') d--;
      if (d == 0 && i < len - 2) outer = false;
    }
    if (outer) {
      int n = len - 2; if (n >= cap) n = cap - 1;
      memcpy(b, s + 1, n); b[n] = 0; return;
    }
  }
  strncpy(b, s, cap - 1); b[cap - 1] = 0;
}

static int top_op(const char *s, char op) {
  int d = 0;
  for (int i = 0; s[i]; ++i) {
    if (s[i] == '(') d++;
    else if (s[i] == ')') d--;
    else if (d == 0 && s[i] == op) return i;
  }
  return -1;
}

static bool is_comp_pair(const char *a, const char *b) {
  int la = (int)strlen(a), lb = (int)strlen(b);
  return (la == lb + 1 && a[la-1] == '\'' && strncmp(a, b, lb) == 0) ||
         (lb == la + 1 && b[lb-1] == '\'' && strncmp(b, a, la) == 0);
}

static int split_top_parts(const char *s, char op, char parts[][40], int maxp) {
  int d = 0, start = 0, n = 0;
  for (int i = 0; ; ++i) {
    char c = s[i];
    if (c == '(') d++;
    else if (c == ')') d--;
    if ((c == op && d == 0) || c == 0) {
      if (n >= maxp) return n;
      char tmp[40]; int len = i - start;
      if (len > 39) len = 39;
      memcpy(tmp, s + start, len); tmp[len] = 0;
      strip_outer(tmp, parts[n], 40);
      ++n;
      start = i + 1;
      if (c == 0) return n;
    }
  }
}

static bool has_lit(char parts[][40], int n, const char *lit) {
  for (int i = 0; i < n; ++i) if (strcmp(parts[i], lit) == 0) return true;
  return false;
}

static bool same_lit_set(char a[][40], int na, char b[][40], int nb) {
  if (na != nb) return false;
  for (int i = 0; i < na; ++i) if (!has_lit(b, nb, a[i])) return false;
  return true;
}

static void join_terms(char terms[][40], int count, int skip, char *res) {
  int p = 0; res[0] = 0;
  for (int i = 0; i < count; ++i) if (i != skip) {
    if (p) app_ch(res, &p, 80, '+');
    app_str(res, &p, 80, terms[i]);
  }
}

static bool bool_law_once(const char *expr, char *res, char *law) {
  char e[96]; strip_outer(expr, e, sizeof(e));
  if (e[0] == '!' && e[1] == '!') {
    strcpy(law, "Double complement"); strcpy(res, e + 2); return true;
  }
  if (e[0] == '!' && e[1] == '(') {
    char inner[80]; strip_outer(e + 1, inner, sizeof(inner));
    int p = top_op(inner, '&');
    if (p < 0) p = top_op(inner, '+');
    if (p > 0) {
      char a[40], b[40]; int op = inner[p];
      memcpy(a, inner, p); a[p] = 0; strcpy(b, inner + p + 1);
      int rp = 0; app_str(res, &rp, 80, a); app_ch(res, &rp, 80, '\'');
      app_ch(res, &rp, 80, op == '&' ? '+' : '&');
      app_str(res, &rp, 80, b); app_ch(res, &rp, 80, '\'');
      strcpy(law, "De Morgan's law"); return true;
    }
  }
  {
    int p = top_op(e, '^');
    if (p > 0) {
      char a[40], b[40]; memcpy(a, e, p); a[p] = 0; strcpy(b, e + p + 1);
      sprintf(res, "%s'&%s+%s&%s'", a, b, a, b);
      strcpy(law, "XOR identity"); return true;
    }
  }
  for (int oi = 0; oi < 2; ++oi) {
    char op = oi ? '&' : '+';
    int p = top_op(e, op);
    if (p <= 0) continue;
    char a[40], b[40]; memcpy(a, e, p); a[p] = 0; strcpy(b, e + p + 1);
    char aa[40], bb[40]; strip_outer(a, aa, sizeof(aa)); strip_outer(b, bb, sizeof(bb));
    if (strcmp(aa, bb) == 0) { strcpy(res, aa); strcpy(law, "Idempotent law"); return true; }
    if (is_comp_pair(aa, bb)) { strcpy(res, op == '+' ? "1" : "0"); strcpy(law, "Complement law"); return true; }
    if (op == '+') {
      if (strcmp(aa, "0") == 0) { strcpy(res, bb); strcpy(law, "Identity law"); return true; }
      if (strcmp(bb, "0") == 0) { strcpy(res, aa); strcpy(law, "Identity law"); return true; }
      if (strcmp(aa, "1") == 0 || strcmp(bb, "1") == 0) { strcpy(res, "1"); strcpy(law, "Dominance law"); return true; }
      int q = top_op(bb, '&');
      if (q > 0) {
        char l[40], r[40]; memcpy(l, bb, q); l[q] = 0; strcpy(r, bb + q + 1);
        if (strcmp(aa, l) == 0 || strcmp(aa, r) == 0) { strcpy(res, aa); strcpy(law, "Absorption law"); return true; }
      }
      q = top_op(aa, '&');
      if (q > 0) {
        char l[40], r[40]; memcpy(l, aa, q); l[q] = 0; strcpy(r, aa + q + 1);
        if (strcmp(bb, l) == 0 || strcmp(bb, r) == 0) { strcpy(res, bb); strcpy(law, "Absorption law"); return true; }
      }
      int qa = top_op(aa, '&'), qb = top_op(bb, '&');
      if (qa > 0 && qb > 0) {
        char al[40], ar[40], bl[40], br[40], common[40], ao[40], bo[40];
        memcpy(al, aa, qa); al[qa] = 0; strcpy(ar, aa + qa + 1);
        memcpy(bl, bb, qb); bl[qb] = 0; strcpy(br, bb + qb + 1);
        common[0] = ao[0] = bo[0] = 0;
        if (strcmp(al, bl) == 0) { strcpy(common, al); strcpy(ao, ar); strcpy(bo, br); }
        else if (strcmp(al, br) == 0) { strcpy(common, al); strcpy(ao, ar); strcpy(bo, bl); }
        else if (strcmp(ar, bl) == 0) { strcpy(common, ar); strcpy(ao, al); strcpy(bo, br); }
        else if (strcmp(ar, br) == 0) { strcpy(common, ar); strcpy(ao, al); strcpy(bo, bl); }
        if (common[0]) {
          sprintf(res, "%s&(%s+%s)", common, ao, bo);
          strcpy(law, "Distributive law"); return true;
        }
      }
    } else {
      if (strcmp(aa, "1") == 0) { strcpy(res, bb); strcpy(law, "Identity law"); return true; }
      if (strcmp(bb, "1") == 0) { strcpy(res, aa); strcpy(law, "Identity law"); return true; }
      if (strcmp(aa, "0") == 0 || strcmp(bb, "0") == 0) { strcpy(res, "0"); strcpy(law, "Dominance law"); return true; }
      int q = top_op(bb, '+');
      if (q > 0) {
        char l[40], r[40]; memcpy(l, bb, q); l[q] = 0; strcpy(r, bb + q + 1);
        if (strcmp(aa, l) == 0 || strcmp(aa, r) == 0) { strcpy(res, aa); strcpy(law, "Absorption law"); return true; }
      }
      q = top_op(aa, '+');
      if (q > 0) {
        char l[40], r[40]; memcpy(l, aa, q); l[q] = 0; strcpy(r, aa + q + 1);
        if (strcmp(bb, l) == 0 || strcmp(bb, r) == 0) { strcpy(res, bb); strcpy(law, "Absorption law"); return true; }
      }
      int qa = top_op(aa, '+'), qb = top_op(bb, '+');
      if (qa > 0 && qb > 0) {
        char al[40], ar[40], bl[40], br[40], common[40], ao[40], bo[40];
        memcpy(al, aa, qa); al[qa] = 0; strcpy(ar, aa + qa + 1);
        memcpy(bl, bb, qb); bl[qb] = 0; strcpy(br, bb + qb + 1);
        common[0] = ao[0] = bo[0] = 0;
        if (strcmp(al, bl) == 0) { strcpy(common, al); strcpy(ao, ar); strcpy(bo, br); }
        else if (strcmp(al, br) == 0) { strcpy(common, al); strcpy(ao, ar); strcpy(bo, bl); }
        else if (strcmp(ar, bl) == 0) { strcpy(common, ar); strcpy(ao, al); strcpy(bo, br); }
        else if (strcmp(ar, br) == 0) { strcpy(common, ar); strcpy(ao, al); strcpy(bo, bl); }
        if (common[0]) {
          sprintf(res, "%s+%s&%s", common, ao, bo);
          strcpy(law, "Distributive law"); return true;
        }
      }
    }
  }
  {
    char terms[6][40]; int tc = split_top_parts(e, '+', terms, 6);
    if (tc >= 3) {
      for (int i = 0; i < tc; ++i) for (int j = i + 1; j < tc; ++j) {
        char li[6][40], lj[6][40], rest[10][40];
        int ni = split_top_parts(terms[i], '&', li, 6), nj = split_top_parts(terms[j], '&', lj, 6);
        for (int a0 = 0; a0 < ni; ++a0) for (int b0 = 0; b0 < nj; ++b0) if (is_comp_pair(li[a0], lj[b0])) {
          int rc = 0;
          for (int r = 0; r < ni; ++r) if (r != a0 && rc < 10) strcpy(rest[rc++], li[r]);
          for (int r = 0; r < nj; ++r) if (r != b0 && !has_lit(rest, rc, lj[r]) && rc < 10) strcpy(rest[rc++], lj[r]);
          if (!rc) continue;
          for (int k = 0; k < tc; ++k) if (k != i && k != j) {
            char lk[10][40]; int nk = split_top_parts(terms[k], '&', lk, 10);
            if (same_lit_set(rest, rc, lk, nk)) {
              join_terms(terms, tc, k, res);
              strcpy(law, "Consensus theorem"); return true;
            }
          }
        }
      }
    }
  }
  return false;
}

static int add_bool_law_trace(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], int n, const char *expr) {
  char cur[96], next[96], law[32];
  strncpy(cur, expr, sizeof(cur) - 1); cur[sizeof(cur) - 1] = 0;
  for (int step = 0; step < 5; ++step) {
    if (!bool_law_once(cur, next, law)) break;
    if (strcmp(cur, next) == 0) break;
    if (strcmp(law, "XOR identity") != 0 && !bool_equiv_expr(cur, next)) break;
    if (step == 0) n = add(out, n, "Simplify by Boolean algebra.");
    n = add(out, n, "%s -> %s (%s)", cur, next, law);
    strncpy(cur, next, sizeof(cur) - 1); cur[sizeof(cur) - 1] = 0;
  }
  return n;
}

static int eval_bool(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[2][48]; int na = args(s, a, 2);
  char exprbuf[96];
  const char *expr = (starts(s, "bool(") || starts(s, "truth(") || starts(s, "boolean(") || starts(s, "logic(")) && na ? a[0] : 0;
  if (!expr) return 0;
  bool_norm(expr, exprbuf, sizeof(exprbuf));
  expr = exprbuf;
  char vars[8]; int vc; collect_vars(expr, vars, &vc);
  if (vc == 0 || vc > 6) return add(out, 0, "Use up to 6 Boolean variables.");
  int mins[64], mc = 0, rows = 1 << vc;
  for (int m = 0; m < rows; ++m) {
    BParser p = { expr, 0, m, {0}, vc };
    for (int i = 0; i < vc; ++i) p.vars[i] = vars[i];
    if (p.expr()) mins[mc++] = m;
  }
  int n = 0;
  n = add_bool_law_trace(out, n, expr);
  n = add(out, n, "Make truth table, list rows where output is 1.");
  char ml[80] = ""; int pos = 0;
  for (int i = 0; i < mc; ++i) {
    if (i) app_ch(ml, &pos, 80, ',');
    app_int(ml, &pos, 80, mins[i]);
  }
  n = add(out, n, "minterms: %s", mc ? ml : "none");
  if (mc == 0) return add(out, n, "simplified = 0");
  if (mc == rows) return add(out, n, "simplified = 1");

  Imp cur[128], next[128], primes[128]; int cc = 0, pc = 0;
  for (int i = 0; i < mc; ++i) cur[cc++] = { mins[i], 0, 0 };
  for (;;) {
    int nc = 0; for (int i = 0; i < cc; ++i) cur[i].used = 0;
    for (int i = 0; i < cc; ++i) for (int j = i + 1; j < cc; ++j) {
      if (cur[i].mask != cur[j].mask) continue;
      int d = (cur[i].bits ^ cur[j].bits) & ~cur[i].mask;
      if (d && (d & (d - 1)) == 0) {
        cur[i].used = cur[j].used = 1;
        Imp q = { cur[i].bits & ~d, cur[i].mask | d, 0 };
        bool seen = false; for (int k = 0; k < nc; ++k) if (next[k].bits == q.bits && next[k].mask == q.mask) seen = true;
        if (!seen && nc < 128) next[nc++] = q;
      }
    }
    for (int i = 0; i < cc && pc < 128; ++i) if (!cur[i].used) primes[pc++] = cur[i];
    if (!nc) break;
    for (int i = 0; i < nc; ++i) cur[i] = next[i];
    cc = nc;
  }
  bool covered[64] = {0}; Imp chosen[32]; int chc = 0;
  while (true) {
    int best = -1, bestc = -1;
    for (int i = 0; i < pc; ++i) {
      int c = 0; for (int j = 0; j < mc; ++j) if (!covered[j] && covers(primes[i], mins[j])) c++;
      if (c > bestc || (c == bestc && best >= 0 && bitcount(primes[i].mask) > bitcount(primes[best].mask))) { bestc = c; best = i; }
    }
    if (bestc <= 0 || chc >= 32) break;
    chosen[chc++] = primes[best];
    for (int j = 0; j < mc; ++j) if (covers(primes[best], mins[j])) covered[j] = true;
  }
  char sim[80] = ""; int sp = 0;
  for (int i = 0; i < chc; ++i) {
    char t[32]; imp_text(chosen[i], vars, vc, t);
    if (i) app_ch(sim, &sp, 80, '+');
    app_str(sim, &sp, 80, t);
  }
  return add(out, n, "simplified = %s", sim);
}

static void minterm_expr(int m, const char *vars, int vc, char *buf, int cap) {
  int p = 0;
  for (int i = 0; i < vc; ++i) {
    if (i) app_ch(buf, &p, cap, '&');
    int bit = 1 << (vc - 1 - i);
    app_ch(buf, &p, cap, vars[i]);
    if (!(m & bit)) app_ch(buf, &p, cap, '\'');
  }
  if (!p) app_ch(buf, &p, cap, '1');
}

static int eval_minterms(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[40][48]; int na = args(s, a, 40);
  if (!starts3(s, "minterms(", "kmap(", "karnaugh(") || na < 1) return 0;
  char vars[8] = ""; int vc = 0, mins[32], mc = 0;
  for (int i = 0; i < na; ++i) {
    if (isalpha((unsigned char)a[i][0])) {
      for (int j = 0; a[i][j] && vc < 6; ++j) if (isalpha((unsigned char)a[i][j])) {
        char c = (char)toupper((unsigned char)a[i][j]);
        bool seen = false; for (int k = 0; k < vc; ++k) if (vars[k] == c) seen = true;
        if (!seen) { vars[vc++] = c; vars[vc] = 0; }
      }
    } else if (mc < 32) mins[mc++] = (int)parse_int(a[i]);
  }
  if (!mc) return add(out, 0, "Give minterm numbers, e.g. minterms(A,B,1,2).");
  if (!vc) {
    int maxm = 0; for (int i = 0; i < mc; ++i) if (mins[i] > maxm) maxm = mins[i];
    while ((1 << vc) <= maxm && vc < 6) ++vc;
    if (vc < 1) vc = 1;
    for (int i = 0; i < vc; ++i) vars[i] = (char)('A' + i);
    vars[vc] = 0;
  }
  int rows = 1 << vc;
  char sop[96] = ""; int sp = 0;
  int n = add(out, 0, "K-map/minterm method: write 1-cells as SOP terms.");
  char vl[32] = ""; int vp = 0;
  for (int i = 0; i < vc; ++i) { if (i) app_ch(vl, &vp, sizeof(vl), ','); app_ch(vl, &vp, sizeof(vl), vars[i]); }
  n = add(out, n, "variables: %s", vl);
  for (int i = 0; i < mc; ++i) {
    if (mins[i] < 0 || mins[i] >= rows) return add(out, n, "minterm %d is outside 0 to %d.", mins[i], rows - 1);
    char term[32]; minterm_expr(mins[i], vars, vc, term, sizeof(term));
    if (i) app_ch(sop, &sp, sizeof(sop), '+');
    app_str(sop, &sp, sizeof(sop), term);
    if (n < CSCALC_MAX_LINES - 6) n = add(out, n, "m%d = %s", mins[i], term);
  }
  n = add(out, n, "SOP = %s", sop);
  char cmd[112]; sprintf(cmd, "bool(%s)", sop);
  char tmp[CSCALC_MAX_LINES][CSCALC_LINE_LEN]; int tn = eval_bool(cmd, tmp);
  for (int i = 0; i < tn && n < CSCALC_MAX_LINES; ++i) {
    if (starts(tmp[i], "Make truth table")) continue;
    n = add(out, n, "%s", tmp[i]);
  }
  return n;
}

struct GNode { char op, v; int l, r; };

struct GParser {
  const char *s; int pos; GNode *nodes; int nc;
  bool eat(char c) { if (s[pos] == c) { pos++; return true; } return false; }
  int node(char op, char v, int l, int r) {
    if (nc >= 63) return 0;
    nodes[nc] = { op, v, l, r };
    return nc++;
  }
  int factor() {
    if (eat('!') || eat('~')) return node('!', 0, factor(), -1);
    int id = 0;
    if (eat('(')) { id = expr(); eat(')'); }
    else if (isalpha((unsigned char)s[pos])) id = node('v', (char)toupper((unsigned char)s[pos++]), -1, -1);
    else id = node('v', '?', -1, -1);
    while (eat('\'')) id = node('!', 0, id, -1);
    return id;
  }
  int term() {
    int id = factor();
    while (s[pos] == '&' || s[pos] == '*' || s[pos] == '.') { pos++; id = node('&', 0, id, factor()); }
    return id;
  }
  int expr() {
    int id = term();
    while (s[pos] == '+' || s[pos] == '|' || s[pos] == '^' || s[pos] == '@' || s[pos] == '#') {
      char op = s[pos++]; if (op == '|') op = '+';
      id = node(op, 0, id, term());
    }
    return id;
  }
};

static void gate_app(char *b, int *p, int cap, const char *s) {
  for (int i = 0; s[i] && *p + 1 < cap; ++i) b[(*p)++] = s[i];
  b[*p] = 0;
}

static void gate_nand(GNode *n, int id, char *b, int *p, int cap);
static void gate_nor(GNode *n, int id, char *b, int *p, int cap);

static void gate_nand_not(GNode *n, int id, char *b, int *p, int cap) {
  gate_app(b, p, cap, "("); gate_nand(n, id, b, p, cap); gate_app(b, p, cap, " NAND "); gate_nand(n, id, b, p, cap); gate_app(b, p, cap, ")");
}

static void gate_nor_not(GNode *n, int id, char *b, int *p, int cap) {
  gate_app(b, p, cap, "("); gate_nor(n, id, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, id, b, p, cap); gate_app(b, p, cap, ")");
}

static void gate_nand(GNode *n, int id, char *b, int *p, int cap) {
  GNode &x = n[id];
  if (x.op == 'v') { char t[2] = { x.v, 0 }; gate_app(b, p, cap, t); return; }
  if (x.op == '!') { gate_nand_not(n, x.l, b, p, cap); return; }
  if (x.op == '@') { gate_app(b, p, cap, "("); gate_nand(n, x.l, b, p, cap); gate_app(b, p, cap, " NAND "); gate_nand(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '&') { gate_app(b, p, cap, "("); gate_nand(n, x.l, b, p, cap); gate_app(b, p, cap, " NAND "); gate_nand(n, x.r, b, p, cap); gate_app(b, p, cap, ") NAND ("); gate_nand(n, x.l, b, p, cap); gate_app(b, p, cap, " NAND "); gate_nand(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '+') { gate_app(b, p, cap, "("); gate_nand_not(n, x.l, b, p, cap); gate_app(b, p, cap, " NAND "); gate_nand_not(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '^') { gate_app(b, p, cap, "(A NAND (A NAND B)) NAND (B NAND (A NAND B))"); return; }
  gate_nand_not(n, id, b, p, cap);
}

static void gate_nor(GNode *n, int id, char *b, int *p, int cap) {
  GNode &x = n[id];
  if (x.op == 'v') { char t[2] = { x.v, 0 }; gate_app(b, p, cap, t); return; }
  if (x.op == '!') { gate_nor_not(n, x.l, b, p, cap); return; }
  if (x.op == '#') { gate_app(b, p, cap, "("); gate_nor(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '+') { gate_app(b, p, cap, "("); gate_nor(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, x.r, b, p, cap); gate_app(b, p, cap, ") NOR ("); gate_nor(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '&') { gate_app(b, p, cap, "("); gate_nor_not(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor_not(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '^') { gate_app(b, p, cap, "(A NOR B) NOR ((A NOR A) NOR (B NOR B))"); return; }
  gate_nor_not(n, id, b, p, cap);
}

static int eval_gate_form(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[2][48]; int na = args(s, a, 2);
  bool want_nand = starts2(s, "nandform(", "onlynand(");
  bool want_nor = starts2(s, "norform(", "onlynor(");
  if ((!want_nand && !want_nor) || na < 1) return 0;
  char e[96]; bool_norm(a[0], e, sizeof(e));
  GNode nodes[64]; GParser p = { e, 0, nodes, 0 };
  int root = p.expr();
  char form[192] = ""; int fp = 0;
  if (want_nand) gate_nand(nodes, root, form, &fp, sizeof(form));
  else gate_nor(nodes, root, form, &fp, sizeof(form));
  int n = add(out, 0, want_nand ? "Use NAND as a universal gate." : "Use NOR as a universal gate.");
  n = add(out, n, want_nand ? "NOT A = A NAND A; A+B = A' NAND B'." : "NOT A = A NOR A; AB = A' NOR B'.");
  return add(out, n, "%s form: %s", want_nand ? "NAND" : "NOR", form);
}

static int eval_bool_prove(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[2][48]; int na = args(s, a, 2);
  if (!starts3(s, "boolprove(", "provebool(", "logicprove(") || na < 2) return 0;
  char lbuf[96], rbuf[96], both[192];
  bool_norm(a[0], lbuf, sizeof(lbuf));
  bool_norm(a[1], rbuf, sizeof(rbuf));
  sprintf(both, "%s%s", lbuf, rbuf);
  char vars[8]; int vc; collect_vars(both, vars, &vc);
  if (vc == 0 || vc > 6) return add(out, 0, "Use up to 6 Boolean variables.");
  int rows = 1 << vc, ml[64], mr[64], lc = 0, rc = 0; bool same_rows = true;
  for (int m = 0; m < rows; ++m) {
    BParser lp = { lbuf, 0, m, {0}, vc }, rp = { rbuf, 0, m, {0}, vc };
    for (int i = 0; i < vc; ++i) lp.vars[i] = rp.vars[i] = vars[i];
    int lv = lp.expr(), rv = rp.expr();
    if (lv) ml[lc++] = m;
    if (rv) mr[rc++] = m;
    if (lv != rv) same_rows = false;
  }
  char ls[80] = "", rs[80] = ""; int lp = 0, rp = 0;
  for (int i = 0; i < lc; ++i) { if (i) app_ch(ls, &lp, 80, ','); app_int(ls, &lp, 80, ml[i]); }
  for (int i = 0; i < rc; ++i) { if (i) app_ch(rs, &rp, 80, ','); app_int(rs, &rp, 80, mr[i]); }
  int n = add(out, 0, "Make truth tables for LHS and RHS.");
  n = add(out, n, "LHS output-1 rows: %s", lc ? ls : "none");
  n = add(out, n, "RHS output-1 rows: %s", rc ? rs : "none");
  return add(out, n, same_rows ? "Same output rows, so LHS = RHS." : "Different output rows, so not identical.");
}

static const char *skip_bool_words(const char *e) {
  bool moved = true;
  while (moved) {
    moved = false;
    if (starts(e, "simplify")) { e += 8; moved = true; }
    if (starts(e, "prove")) { e += 5; moved = true; }
    if (starts(e, "showthat")) { e += 8; moved = true; }
    if (starts(e, "show")) { e += 4; moved = true; }
    if (starts(e, "identity")) { e += 8; moved = true; }
    if (starts(e, "boolean")) { e += 7; moved = true; }
    if (starts(e, "logic")) { e += 5; moved = true; }
    if (starts(e, "expression")) { e += 10; moved = true; }
  }
  return e;
}

static int eval_free_text(const char *input, const char *compact, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char t[192]; raw_clean(input, t, sizeof(t));
  double v[8]; int nv = scan_nums(t, v, 8);
  char bits[4][48]; int nb = scan_bits(t, bits, 4);
  char cmd[160];
  double width=0, height=0, depth=0;
  if (has(t, "ascii") || has(t, "unicode") || has(t, "codepoint") || has(t, "charactercode")) {
    int ch = -1;
    bool uni = has(t, "unicode") || has(t, "codepoint");
    if (find_code_char(input, &ch)) return add_code_lines(out, ch, uni, ch);
    if (nv >= 1) return add_code_lines(out, (int)v[0], uni, -1);
  }
  if (has(t, "rpn") || has(t, "postfix") || (has(t, "reverse") && has(t, "polish"))) {
    if (make_rpn_cmd(input, cmd, sizeof(cmd))) return eval_rpn(cmd, out);
  }
  if (has(t, "dijkstra") || (has(t, "shortest") && (has(t, "path") || has(t, "route")))) {
    if (make_dijkstra_cmd(input, cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if (has(t, "fsm") || (has(t, "finite") && has(t, "state") && has(t, "machine"))) {
    if (make_fsm_cmd(input, cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if ((has(t, "tree") || has(t, "traversal") || has(t, "traverse")) &&
      (has(t, "preorder") || has(t, "pre,order") || has(t, "pre"))) {
    if (make_tree_cmd(input, "preorder", cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if ((has(t, "tree") || has(t, "traversal") || has(t, "traverse")) &&
      (has(t, "inorder") || has(t, "in,order"))) {
    if (make_tree_cmd(input, "inorder", cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if ((has(t, "tree") || has(t, "traversal") || has(t, "traverse")) &&
      (has(t, "postorder") || has(t, "post,order") || has(t, "post"))) {
    if (make_tree_cmd(input, "postorder", cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if (has(t, "stack") && (has(t, "push") || has(t, "pop"))) {
    if (make_ds_cmd(input, "stack", cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if (has(t, "queue") && (has(t, "enqueue") || has(t, "dequeue") || has(t, "remove"))) {
    if (make_ds_cmd(input, "queue", cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if ((has(t, "binarysearch") || (has(t, "binary") && has(t, "search"))) && nv >= 2) {
    int p = sprintf(cmd, "binarysearch(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_trace(cmd, out);
  }
  if ((has(t, "linearsearch") || has(t, "sequentialsearch") || (has(t, "linear") && has(t, "search"))) && nv >= 2) {
    int p = sprintf(cmd, "linearsearch(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_trace(cmd, out);
  }
  if ((has(t, "bubblesort") || (has(t, "bubble") && has(t, "sort"))) && nv >= 2) {
    int p = sprintf(cmd, "bubblesort(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_trace(cmd, out);
  }
  if ((has(t, "insertionsort") || (has(t, "insertion") && has(t, "sort"))) && nv >= 2) {
    int p = sprintf(cmd, "insertionsort(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_trace(cmd, out);
  }
  if ((has(t, "selectionsort") || (has(t, "selection") && has(t, "sort"))) && nv >= 2) {
    int p = sprintf(cmd, "selectionsort(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_trace(cmd, out);
  }
  if ((has(t, "mergesort") || (has(t, "merge") && has(t, "sort"))) && nv >= 2) {
    int p = sprintf(cmd, "mergesort(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_trace(cmd, out);
  }
  if ((has(t, "image") || has(t, "bitmap")) && label_num(input,"width",&width) && label_num(input,"height",&height) && (label_num(input,"depth",&depth) || label_num(input,"bits",&depth))) {
    sprintf(cmd, "image(%lld,%lld,%lld)", (long long)width, (long long)height, (long long)depth); return eval_storage(cmd, out);
  }
  if ((has(t, "image") || has(t, "bitmap")) && label_num(input,"width",&width) && label_num(input,"height",&height) && (label_num(input,"colours",&depth) || label_num(input,"colors",&depth))) {
    sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)width, (long long)height, (long long)depth); return eval_storage(cmd, out);
  }
  if ((has(t, "denary") || has(t, "decimal")) && nb >= 1) {
    sprintf(cmd, "den(%s,2)", bits[0]); return eval_base(cmd, out);
  }
  if (has(t, "binary") && nv >= 1 && nb == 0) {
    sprintf(cmd, "bin(%lld)", (long long)v[0]); return eval_base(cmd, out);
  }
  if ((has(t, "hex") || has(t, "hexadecimal")) && nv >= 1) {
    sprintf(cmd, "hex(%lld)", (long long)v[0]); return eval_base(cmd, out);
  }
  bool tc = has(t, "twos") || (has(t, "two") && has(t, "complement"));
  if (has(t, "unsigned") && has(t, "range") && nv >= 1) {
    sprintf(cmd, "unsignedrange(%lld)", (long long)v[0]); return eval_twos(cmd, out);
  }
  if (tc && has(t, "range") && nv >= 1) {
    sprintf(cmd, "twosrange(%lld)", (long long)v[0]); return eval_twos(cmd, out);
  }
  if (tc && nb >= 1 && (has(t, "decode") || has(t, "denary") || has(t, "decimal"))) {
    sprintf(cmd, "twosdec(%s)", bits[0]); return eval_twos(cmd, out);
  }
  if (tc && nb >= 2 && (has(t, "subtract") || has(t, "minus") || has(t, "takeaway"))) {
    if (has(compact, "from")) sprintf(cmd, "twossub(%s,%s)", bits[1], bits[0]);
    else sprintf(cmd, "twossub(%s,%s)", bits[0], bits[1]);
    return eval_twos(cmd, out);
  }
  if (tc && nb >= 2 && (has(t, "add") || has(t, "sum") || has(t, "plus"))) {
    sprintf(cmd, "twosadd(%s,%s)", bits[0], bits[1]); return eval_twos(cmd, out);
  }
  if (tc && nv >= 2 && nb == 0) {
    sprintf(cmd, "twos(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_twos(cmd, out);
  }
  if ((has(t, "xor") || has(t, "exclusiveor")) && nb >= 2) {
    sprintf(cmd, "xorbits(%s,%s)", bits[0], bits[1]); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "bitwiseand") || has(t, "andbits") || starts(t, "and,")) && nb >= 2) {
    sprintf(cmd, "andbits(%s,%s)", bits[0], bits[1]); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "bitwiseor") || has(t, "orbits") || has(t, ",or,") || starts(t, "or,")) && nb >= 2) {
    sprintf(cmd, "orbits(%s,%s)", bits[0], bits[1]); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "not") || has(t, "invert")) && nb >= 1) {
    sprintf(cmd, "notbits(%s)", bits[0]); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "hamming") || has(t, "bitdiff")) && nb >= 2) {
    sprintf(cmd, "hamming(%s,%s)", bits[0], bits[1]); return eval_binary_arith(cmd, out);
  }
  if (has(t, "checksum") && nb >= 1) {
    int p = sprintf(cmd, "checksum(%d", (int)strlen(bits[0]));
    for (int i = 0; i < nb && p < (int)sizeof(cmd) - 50; ++i) p += sprintf(cmd + p, ",%s", bits[i]);
    sprintf(cmd + p, ")");
    return eval_binary_arith(cmd, out);
  }
  if (has(t, "add") && nb >= 2) {
    sprintf(cmd, "binadd(%s,%s,%d)", bits[0], bits[1], (int)strlen(bits[0])); return eval_binary_arith(cmd, out);
  }
  if (has(t, "shift") && nb >= 1 && nv >= 1) {
    sprintf(cmd, "shift(%s,%s,%lld)", bits[0], has(t, "right") ? "right" : "left", (long long)v[nv-1]); return eval_binary_arith(cmd, out);
  }
  if (has(t, "parity") && nb >= 1) {
    sprintf(cmd, "parity(%s,%s)", bits[0], has(t, "odd") ? "odd" : "even"); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "checkdigit") || (has(t, "check") && has(t, "digit"))) && nv >= 3) {
    int p = sprintf(cmd, "checkdigit(%lld,%lld", (long long)v[0], (long long)v[nv-1]);
    for (int i = 1; i < nv - 1 && p < (int)sizeof(cmd) - 20; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_binary_arith(cmd, out);
  }
  char fixed[48];
  if (has(t, "fixed") && (has(t, "encode") || has(t, "represent") || has(t, "convert")) && nv >= 3) {
    sprintf(cmd, (tc || has(t, "signed") || has(t, "negative")) ? "fixedtcenc(%.10g,%lld,%lld)" : "fixedenc(%.10g,%lld,%lld)", v[0], (long long)v[1], (long long)v[2]);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && scan_fixed_bits(t, fixed, sizeof(fixed))) {
    sprintf(cmd, (tc || has(t, "complement")) ? "fixedtc(%s)" : "fixed(%s)", fixed); return eval_float(cmd, out);
  }
  if ((has(t, "normalise") || has(t, "normalize")) && nb >= 2) {
    sprintf(cmd, "floatnorm(%s,%s)", bits[0], bits[1]); return eval_float(cmd, out);
  }
  if ((has(t, "closest") || has(t, "nearest")) && (has(t, "float") || has(t, "floating") || has(t, "representable")) && nv >= 3) {
    sprintf(cmd, "floatnearest(%.10g,%lld,%lld)", v[0], (long long)v[1], (long long)v[2]); return eval_float(cmd, out);
  }
  if ((has(t, "float") || has(t, "floating")) && (has(t, "encode") || has(t, "represent") || has(t, "convert")) && nv >= 3) {
    sprintf(cmd, "floatenc(%.10g,%lld,%lld)", v[0], (long long)v[1], (long long)v[2]); return eval_float(cmd, out);
  }
  if ((has(t, "precision") || has(t, "smallestchange") || has(t, "step")) && (has(t, "float") || has(t, "mantissa")) && nv >= 2) {
    sprintf(cmd, "floatprecision(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_float(cmd, out);
  }
  if ((has(t, "mantissa") || has(t, "floating") || has(t, "float")) && has(t, "exponent") && nb >= 2) {
    sprintf(cmd, "floatdec(%s,%s)", bits[0], bits[1]); return eval_float(cmd, out);
  }
  if ((has(t, "float") || has(t, "real")) && has(t, "range") && nv >= 2) {
    sprintf(cmd, "floatrange(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_float(cmd, out);
  }
  if ((has(t, "image") || has(t, "bitmap")) && nv >= 3) {
    if (has(t, "colours") || has(t, "colors")) {
      sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "image(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
  }
  if ((has(t, "sound") || has(t, "audio")) && nv >= 3) {
    sprintf(cmd, "sound(%lld,%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2], nv > 3 ? (long long)v[3] : 1); return eval_storage(cmd, out);
  }
  if ((has(t, "transfer") || has(t, "download") || has(t, "transmit")) && (has(t, "time") || has(t, "seconds")) && nv >= 2) {
    if ((has(t, "megabyte") || has(t, "mbyte")) && (has(t, "megabit") || has(t, "mbit"))) {
      sprintf(cmd, "transfermb(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
    }
    if ((has(t, "kilobyte") || has(t, "kbyte")) && (has(t, "kilobit") || has(t, "kbit"))) {
      sprintf(cmd, "transferkb(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "transfer(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "compress") || has(t, "compression")) && nv >= 2) {
    sprintf(cmd, "compress(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "huffman") || has(t, "huffmancode")) && nv >= 2) {
    int p = sprintf(cmd, "huffman(%.10g", v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%.10g", v[i]);
    sprintf(cmd + p, ")");
    return eval_storage(cmd, out);
  }
  if ((has(t, "runlength") || has(t, "rle") || (has(t, "run") && has(t, "length"))) && nv >= 2) {
    char seq[48];
    if (find_rle_sequence(input, seq, sizeof(seq))) {
      sprintf(cmd, "rletext(%s,%lld,%lld)", seq, (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
    }
  }
  if ((has(t, "runlength") || has(t, "rle") || (has(t, "run") && has(t, "length"))) && nv >= 3) {
    sprintf(cmd, "rle(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
  }
  if ((has(t, "sql") || has(t, "select") || has(t, "where") || has(t, "count")) && make_sql_cmd(input, cmd, sizeof(cmd))) {
    return eval_storage(cmd, out);
  }
  if ((has(t, "record") || has(t, "database")) && nv >= 2) {
    sprintf(cmd, "records(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if (has(t, "address") && (has(t, "bus") || has(t, "space") || has(t, "locations")) && nv >= 1) {
    if ((has(t, "word") || has(t, "capacity") || has(t, "memory")) && nv >= 2) {
      sprintf(cmd, "memorycapacity(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "addressspace(%lld)", (long long)v[0]); return eval_storage(cmd, out);
  }
  if ((has(t, "hash") || has(t, "hashtable")) && nv >= 2) {
    int p = sprintf(cmd, "hashmod(%lld", (long long)v[0]);
    if (has(t, "linear") || has(t, "probe") || has(t, "probing")) p = sprintf(cmd, "hashlinear(%lld", (long long)v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    sprintf(cmd + p, ")");
    return eval_storage(cmd, out);
  }
  if ((has(t, "character") || has(t, "text")) && nv >= 2) {
    if (has(t, "characterset") || has(t, "symbols") || has(t, "alphabet")) {
      sprintf(cmd, "charset(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "chars(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "kmap") || has(t, "karnaugh") || has(t, "minterm") || has(t, "minterms")) && make_minterm_cmd(input, cmd, sizeof(cmd))) {
    return eval_minterms(cmd, out);
  }
  if (nv == 0 && starts(compact, "nandform")) {
    sprintf(cmd, "nandform(%s)", compact + 8); return eval_gate_form(cmd, out);
  }
  if (nv == 0 && starts(compact, "norform")) {
    sprintf(cmd, "norform(%s)", compact + 7); return eval_gate_form(cmd, out);
  }
  if (nv == 0 && starts(compact, "onlynand")) {
    sprintf(cmd, "nandform(%s)", compact + 8); return eval_gate_form(cmd, out);
  }
  if (nv == 0 && starts(compact, "onlynor")) {
    sprintf(cmd, "norform(%s)", compact + 7); return eval_gate_form(cmd, out);
  }
  if (nv == 0 && has(compact, "=")) {
    const char *e = skip_bool_words(compact);
    const char *eq = strchr(e, '=');
    if (eq && eq > e && eq[1]) {
      char lhs[48], rhs[48]; int li = 0, ri = 0;
      for (const char *p = e; p < eq && li < 47; ++p) lhs[li++] = *p;
      lhs[li] = 0;
      for (const char *p = eq + 1; *p && ri < 47; ++p) rhs[ri++] = *p;
      rhs[ri] = 0;
      sprintf(cmd, "boolprove(%s,%s)", lhs, rhs); return eval_bool_prove(cmd, out);
    }
  }
  if (nv == 0 && (has(compact, "nand") || has(compact, "nor") || has(compact, "xor") || has(compact, "and") || has(compact, "or") || has(compact, "+") || has(compact, "*") || has(compact, "'"))) {
    const char *e = skip_bool_words(compact);
    sprintf(cmd, "bool(%s)", e); return eval_bool(cmd, out);
  }
  return 0;
}

int cscalc_eval(const char *input, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  for (int i = 0; i < CSCALC_MAX_LINES; ++i) out[i][0] = 0;
  char s[192]; clean(input, s, sizeof(s));
  if (!s[0]) return add(out, 0, "Enter a CS calculation command.");
  if (is_bits(s) && !strchr(s, '.')) {
    int n2 = add(out, 0, "Bare 0/1 input treated as unsigned binary.");
    return add(out, n2, "%s_2 = %d_10", s, bin_unsigned(s));
  }
  int n = eval_base(s, out); if (n) return n;
  n = eval_twos(s, out); if (n) return n;
  n = eval_binary_arith(s, out); if (n) return n;
  n = eval_rpn(s, out); if (n) return n;
  n = eval_trace(s, out); if (n) return n;
  n = eval_float(s, out); if (n) return n;
  n = eval_storage(s, out); if (n) return n;
  n = eval_gate_form(s, out); if (n) return n;
  n = eval_minterms(s, out); if (n) return n;
  n = eval_bool_prove(s, out); if (n) return n;
  n = eval_bool(s, out); if (n) return n;
  n = eval_free_text(input, s, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "bin hex den convert twos twosdec twosadd twossub fixed fixedenc parity xorbits andbits orbits notbits hamming checksum checkdigit rpn");
  n = add(out, n, "floatdec floatrange normal image sound bitrate transfer transfermb");
  return add(out, n, "compress huffman rle records sqlselect sqlcount hashmod hashlinear addressspace chars ascii unicode stack queue preorder inorder postorder dijkstra fsm fsmout binarysearch bubblesort selectionsort mergesort bool truth minterms kmap nandform norform");
}

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

static bool has_word(const char *s, const char *w) {
  int wl = (int)strlen(w);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    if (strncmp(s + i, w, wl) == 0 && !isalnum((unsigned char)s[i + wl])) return true;
  }
  return false;
}

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

static int alpha_token_len_after(const char *t, const char *word) {
  int wl = (int)strlen(word);
  for (int i = 0; t[i];) {
    while (t[i] == ',') ++i;
    int start = i;
    while (t[i] && t[i] != ',') ++i;
    if (i - start == wl && strncmp(t + start, word, wl) == 0) {
      while (t[i] == ',') ++i;
      int n = 0;
      while (isalpha((unsigned char)t[i])) { ++n; ++i; }
      return n;
    }
  }
  return 0;
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

static bool scan_spaced_bit_column(const char *s, char *buf, int cap) {
  int k = 0;
  for (int i = 0; s[i] && k + 1 < cap;) {
    while (s[i] == ',') ++i;
    if ((s[i] == '0' || s[i] == '1') && (s[i+1] == 0 || s[i+1] == ',')) {
      buf[k++] = s[i++];
      while (s[i] == ',') ++i;
    } else {
      while (s[i] && s[i] != ',') ++i;
    }
  }
  buf[k] = 0;
  return k >= 2;
}

static int scan_spaced_bit_groups(const char *s, char g[][48], int maxg) {
  int n = 0, k = 0;
  for (int i = 0; s[i] && n < maxg;) {
    while (s[i] == ',') ++i;
    if ((s[i] == '0' || s[i] == '1') && (s[i+1] == 0 || s[i+1] == ',')) {
      if (k < 47) g[n][k++] = s[i];
      ++i;
    } else {
      if (k > 1) { g[n][k] = 0; ++n; }
      k = 0;
      while (s[i] && s[i] != ',') ++i;
    }
  }
  if (k > 1 && n < maxg) { g[n][k] = 0; ++n; }
  return n;
}

static void join_bits(char b[][48], int n, char *out, int cap) {
  out[0] = 0;
  for (int i = 0; i < n && (int)strlen(out) + (int)strlen(b[i]) + 1 < cap; ++i) strcat(out, b[i]);
}

static bool scan_hex_token(const char *s, char *buf, int cap) {
  for (int i = 0; s[i];) {
    if (!isxdigit((unsigned char)s[i])) { ++i; continue; }
    int j = i, k = 0; bool has_alpha = false, has_digit = false;
    while (isxdigit((unsigned char)s[j]) && k + 1 < cap) {
      char c = (char)toupper((unsigned char)s[j]);
      if (c >= 'A' && c <= 'F') has_alpha = true;
      if (c >= '0' && c <= '9') has_digit = true;
      buf[k++] = c; ++j;
    }
    buf[k] = 0;
    if (has_alpha && has_digit) return true;
    i = j;
  }
  return false;
}

static bool scan_hex_word_token(const char *s, char *buf, int cap) {
  for (int i = 0; s[i];) {
    while (s[i] && !isalnum((unsigned char)s[i])) ++i;
    int j = i, k = 0; bool has_alpha = false, ok = true;
    while (isalnum((unsigned char)s[j])) {
      if (!isxdigit((unsigned char)s[j])) ok = false;
      char c = (char)toupper((unsigned char)s[j]);
      if (c >= 'A' && c <= 'F') has_alpha = true;
      if (k + 1 < cap) buf[k++] = c;
      ++j;
    }
    buf[k] = 0;
    if (ok && has_alpha && k > 0) return true;
    i = j;
  }
  return false;
}

static bool scan_fixed_bits(const char *s, char *buf, int cap) {
  for (int i = 0; s[i]; ++i) {
    if (s[i] != '0' && s[i] != '1') continue;
    int j = i, k = 0; bool dot = false;
    while ((s[j] == '0' || s[j] == '1' || s[j] == '.') && k + 1 < cap) {
      if (s[j] == '.') {
        if (dot) break;
        dot = true;
      }
      buf[k++] = s[j++];
    }
    buf[k] = 0;
    if (dot && k > 2) return true;
    i = j;
  }
  return false;
}

static int scan_fixed_bit_tokens(const char *s, char out[][48], int maxn) {
  int n = 0;
  for (int i = 0; s[i] && n < maxn; ++i) {
    if (s[i] != '0' && s[i] != '1') continue;
    int j = i, k = 0; bool dot = false;
    while ((s[j] == '0' || s[j] == '1' || s[j] == '.') && k + 1 < 48) {
      if (s[j] == '.') {
        if (dot) break;
        dot = true;
      }
      out[n][k++] = s[j++];
    }
    out[n][k] = 0;
    if (dot && k > 2) ++n;
    i = j;
  }
  return n;
}

static double read_num(const char *s) {
  double sign = 1, v = 0, scale = 1;
  if (*s == '-') { sign = -1; ++s; }
  while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
  if (*s == '.') for (++s; *s >= '0' && *s <= '9'; ++s) { scale *= 10; v += (*s - '0') / scale; }
  return sign * v;
}

static bool scan_after_word_num(const char *s, const char *word, double *v) {
  int wl = (int)strlen(word);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    if (strncmp(s + i, word, wl) != 0 || isalnum((unsigned char)s[i + wl])) continue;
    int j = i + wl;
    while (s[j] == ',') ++j;
    if (s[j] == '-' || isdigit((unsigned char)s[j])) { *v = read_num(s + j); return true; }
  }
  return false;
}

static bool scan_before_word_num(const char *s, const char *word, double *v) {
  int wl = (int)strlen(word);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    if (strncmp(s + i, word, wl) != 0 || isalnum((unsigned char)s[i + wl])) continue;
    int j = i - 1;
    while (j >= 0 && s[j] == ',') --j;
    int e = j + 1;
    while (j >= 0 && (isdigit((unsigned char)s[j]) || s[j] == '.' || s[j] == '-')) --j;
    if (e > j + 1) { *v = read_num(s + j + 1); return true; }
  }
  return false;
}

static int scan_binary_fraction_tokens(const char *s, char out[][48], int maxn) {
  int n = 0;
  for (int i = 0; s && s[i] && n < maxn; ++i) {
    if ((s[i] != '0' && s[i] != '1') || s[i+1] != '.' || (s[i+2] != '0' && s[i+2] != '1')) continue;
    int p = 0, j = i; bool dot = false;
    while ((s[j] == '0' || s[j] == '1' || s[j] == '.') && p + 1 < 48) {
      if (s[j] == '.') dot = true;
      else out[n][p++] = s[j];
      ++j;
    }
    out[n][p] = 0;
    if (dot && p > 1) ++n;
    i = j;
  }
  return n;
}

static bool scan_scaled_before_word_num(const char *s, const char *word, double *v) {
  if (scan_before_word_num(s, word, v)) return true;
  char pat[40]; sprintf(pat, "million,%s", word);
  const char *p = strstr(s, pat);
  if (!p) return false;
  int j = (int)(p - s) - 1;
  while (j >= 0 && s[j] == ',') --j;
  int e = j + 1;
  while (j >= 0 && (isdigit((unsigned char)s[j]) || s[j] == '.' || s[j] == '-')) --j;
  if (e <= j + 1) return false;
  *v = read_num(s + j + 1) * 1000000.0;
  return true;
}

static double scaled_colour_count(const char *t, double v) {
  return (has(t, "million,colours") || has(t, "million,colors")) && v < 1000 ? v * 1000000.0 : v;
}

static bool scan_bit_width_before_label(const char *s, const char *label, double *v) {
  const char *p = strstr(s, label);
  if (!p) return false;
  const char *q = p - 1;
  while (q >= s && *q == ',') --q;
  const char *endword = q + 1;
  while (q >= s && isalpha((unsigned char)*q)) --q;
  const char *word = q + 1;
  if (!((endword - word == 3 && strncmp(word, "bit", 3) == 0) ||
        (endword - word == 4 && strncmp(word, "bits", 4) == 0))) return false;
  while (q >= s && (*q == ',' || *q == '-')) --q;
  const char *endnum = q + 1;
  while (q >= s && (isdigit((unsigned char)*q) || *q == '.')) --q;
  if (endnum <= q + 1) return false;
  char b[32]; int len = (int)(endnum - (q + 1));
  if (len <= 0 || len >= (int)sizeof(b)) return false;
  memcpy(b, q + 1, len); b[len] = 0;
  *v = read_num(b);
  return true;
}

static bool label_num(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0, q = i;
    while (j < nl && s[q]) {
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '_' || s[q] == '-') ++q;
      if (tolower((unsigned char)s[q]) != name[j]) break;
      ++j; ++q;
    }
    if (j != nl) continue;
    int k = q;
    while (s[k] == ' ' || s[k] == '\t') ++k;
    if (s[k] != '=') continue;
    ++k;
    while (s[k] == ' ' || s[k] == '\t') ++k;
    *v = read_num(s + k);
    return true;
  }
  return false;
}

static void scale_time_unit(const char *t, double *seconds) {
  if (has(t, "millisecond") || has(t, "ms")) *seconds /= 1000.0;
  else if (has(t, "minute") || has(t, "mins")) *seconds *= 60.0;
  else if (has(t, "hour")) *seconds *= 3600.0;
}

static void scale_frequency_unit(const char *t, double *rate) {
  if (has(t, "khz") || has(t, "kilohertz")) *rate *= 1000.0;
  else if (has(t, "mhz") || has(t, "megahertz")) *rate *= 1000000.0;
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

static long long mod_pow_ll(long long a, long long e, long long m) {
  long long r = 1 % m;
  a %= m;
  while (e > 0) {
    if (e & 1) r = (r * a) % m;
    a = (a * a) % m;
    e >>= 1;
  }
  return r;
}

static long long mod_inverse_small(long long e, long long phi) {
  for (long long d = 1; d < phi; ++d) if ((d * e) % phi == 1) return d;
  return 0;
}

static void to_base(long long v, int base, char *buf, int cap) {
  const char *dig = "0123456789ABCDEF";
  char tmp[65]; int n = 0;
  if (base < 2 || base > 16 || cap < 2) { buf[0] = 0; return; }
  if (v == 0) tmp[n++] = '0';
  while (v && n < (int)sizeof(tmp) - 1) { tmp[n++] = dig[v % base]; v /= base; }
  int p = 0;
  while (n && p + 1 < cap) buf[p++] = tmp[--n];
  buf[p] = 0;
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

static int whole_bits_for(double value) {
  double x = value < 0 ? -value : value;
  int bits = 1;
  while (pow2(bits) <= x && bits < 62) bits++;
  return bits;
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

static bool find_rle_encoded_sequence(const char *in, char *seq, int cap) {
  int best = 0; seq[0] = 0;
  for (int i = 0; in && in[i]; ++i) {
    if (!isdigit((unsigned char)in[i])) continue;
    char tmp[48]; int j = i, k = 0, pairs = 0;
    while (isdigit((unsigned char)in[j]) && k + 2 < (int)sizeof(tmp)) {
      while (isdigit((unsigned char)in[j]) && k + 1 < (int)sizeof(tmp)) tmp[k++] = in[j++];
      if (!isalpha((unsigned char)in[j])) break;
      tmp[k++] = (char)toupper((unsigned char)in[j++]);
      ++pairs;
    }
    tmp[k] = 0;
    if (pairs >= 2 && k > best && k < cap) { strcpy(seq, tmp); best = k; i = j; }
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

static int add_infix_postfix_lines(const char *input, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char expr[96]; int ep = 0;
  bool seen = false;
  for (int i = 0; input[i] && ep + 1 < (int)sizeof(expr); ++i) {
    unsigned char c = (unsigned char)input[i];
    if (isalnum(c) || c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')') {
      if (isalpha(c)) {
        char w[12]; int j = 0, k = i;
        while (isalpha((unsigned char)input[k]) && j + 1 < (int)sizeof(w)) w[j++] = (char)tolower((unsigned char)input[k++]);
        w[j] = 0;
        if (seen && (word_is(w, "to") || word_is(w, "reverse") || word_is(w, "polish") ||
                     word_is(w, "notation") || word_is(w, "postfix"))) break;
        if (word_is(w, "convert") || word_is(w, "infix") || word_is(w, "expression") ||
            word_is(w, "reverse") || word_is(w, "polish") || word_is(w, "notation") ||
            word_is(w, "to") || word_is(w, "postfix") || word_is(w, "the")) { i = k - 1; continue; }
      }
      seen = true;
      expr[ep++] = (char)c;
    } else if (seen && !isspace(c)) {
      break;
    }
  }
  expr[ep] = 0;
  if (!expr[0]) return 0;
  char outp[96]; int op = 0;
  char st[32]; int sp = 0;
  for (int i = 0; expr[i] && op + 2 < (int)sizeof(outp); ++i) {
    char c = expr[i];
    if (isalnum((unsigned char)c)) outp[op++] = c;
    else if (c == '(') st[sp++] = c;
    else if (c == ')') {
      while (sp && st[sp-1] != '(') outp[op++] = st[--sp];
      if (sp && st[sp-1] == '(') --sp;
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
      int pc = (c == '*' || c == '/') ? 2 : 1;
      while (sp && st[sp-1] != '(') {
        int ps = (st[sp-1] == '*' || st[sp-1] == '/') ? 2 : 1;
        if (ps < pc) break;
        outp[op++] = st[--sp];
      }
      st[sp++] = c;
    }
  }
  while (sp && op + 1 < (int)sizeof(outp)) if (st[sp-1] != '(') outp[op++] = st[--sp]; else --sp;
  outp[op] = 0;
  int n = add(out, 0, "Use operator precedence and a stack to convert infix to postfix.");
  n = add(out, n, "infix = %s", expr);
  return add(out, n, "postfix = %s", outp);
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
  char start[16] = "", end[16] = "";
  char edge[30][16]; int ne = 0; bool in_edges = false;
  for (int i = 0; t[i] && ne < 30;) {
    while (t[i] == ',') ++i;
    char w[16]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0]) continue;
    if (word_is(w, "from")) {
      while (t[i] == ',') ++i;
      j = 0; while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(start)) start[j++] = t[i++]; start[j] = 0;
      continue;
    }
    if (word_is(w, "to")) {
      while (t[i] == ',') ++i;
      j = 0; while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(end)) end[j++] = t[i++]; end[j] = 0;
      continue;
    }
    if (word_is(w, "edges") || word_is(w, "edge")) { in_edges = true; continue; }
    if (in_edges && !dijkstra_skip_word(w)) strcpy(edge[ne++], w);
  }
  if (start[0] && end[0] && ne >= 3 && ne % 3 == 0) {
    int p = sprintf(cmd, "dijkstra(%s,%s", start, end);
    for (int i = 0; i < ne && p < cap - 20; ++i) p += sprintf(cmd + p, ",%s", edge[i]);
    if (p >= cap - 2) return false;
    cmd[p++] = ')'; cmd[p] = 0;
    return true;
  }
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
  if (strcmp(op, ">=") == 0 || word_is(op, "gte") || word_is(op, "atleast")) return ">=";
  if (strcmp(op, ">") == 0 || word_is(op, "gt") || word_is(op, "greater") || word_is(op, "more") || word_is(op, "above")) return ">";
  if (strcmp(op, "<=") == 0 || word_is(op, "lte") || word_is(op, "atmost")) return "<=";
  if (strcmp(op, "<") == 0 || word_is(op, "lt") || word_is(op, "less") || word_is(op, "below") || word_is(op, "under")) return "<";
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
    while (j > 0 && !isalnum((unsigned char)w[j-1])) w[--j] = 0;
    if (w[0]) strcpy(tok[nt++], w);
  }
  int sel = -1, from = -1, where = -1, table_word = -1;
  for (int i = 0; i < nt; ++i) {
    if (word_is(tok[i], "select")) sel = i;
    if (word_is(tok[i], "from")) from = i;
    if (word_is(tok[i], "where")) where = i;
    if (word_is(tok[i], "table")) table_word = i;
  }
  bool count = false;
  for (int i = 0; i < nt; ++i) if (word_is(tok[i], "count") || word_is(tok[i], "countrows") || word_is(tok[i], "countrecords")) count = true;
  if (where < 0 || where + 2 >= nt) return false;
  if (from < 0 && (table_word < 0 || table_word + 1 >= nt)) return false;
  char showbuf[96] = "*";
  if (!count && sel >= 0 && sel + 1 < nt) {
    int end = from >= 0 ? from : where, p = 0;
    showbuf[0] = 0;
    for (int i = sel + 1; i < end && p < (int)sizeof(showbuf) - 4; ++i) {
      if (word_is(tok[i], "and") || word_is(tok[i], ",")) continue;
      p += sprintf(showbuf + p, "%s%s", p ? "," : "", tok[i]);
    }
    if (!showbuf[0]) strcpy(showbuf, tok[sel + 1]);
  }
  const char *show = count ? "*" : showbuf;
  const char *table = from >= 0 ? tok[from + 1] : tok[table_word + 1];
  const char *field = tok[where + 1];
  const char *op = tok[where + 2];
  const char *value = tok[where + 3];
  const char *where_text = strstr(in, "where");
  if (!where_text) where_text = strstr(in, "WHERE");
  char opbuf[3] = "";
  if (where_text) {
    if (strstr(where_text, ">=")) strcpy(opbuf, ">=");
    else if (strstr(where_text, "<=")) strcpy(opbuf, "<=");
    else if (strstr(where_text, "!=") || strstr(where_text, "<>")) strcpy(opbuf, "!=");
    else if (strstr(where_text, ">")) strcpy(opbuf, ">");
    else if (strstr(where_text, "<")) strcpy(opbuf, "<");
  }
  if (opbuf[0] && where + 2 < nt) {
    op = opbuf;
    value = tok[where + 2];
  }
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
         word_is(w, "draw") || word_is(w, "make") || word_is(w, "create") ||
         word_is(w, "minterm") || word_is(w, "minterms") || word_is(w, "ones") ||
         word_is(w, "maxterm") || word_is(w, "maxterms") || word_is(w, "zeros") ||
         word_is(w, "dc") || word_is(w, "dont") || word_is(w, "don't") ||
         word_is(w, "care") || word_is(w, "cares") ||
         word_is(w, "cells") || word_is(w, "cell") || word_is(w, "for") ||
         word_is(w, "variables") || word_is(w, "variable") || word_is(w, "vars") ||
         word_is(w, "find") || word_is(w, "sum") || word_is(w, "sums") ||
         word_is(w, "product") || word_is(w, "products") || word_is(w, "from") ||
         word_is(w, "of") || word_is(w, "sop") || word_is(w, "pos") ||
         word_is(w, "with") || word_is(w, "simplify") || word_is(w, "boolean") ||
         word_is(w, "logic") || word_is(w, "output") || word_is(w, "and") ||
         word_is(w, "or") || word_is(w, "is") ||
         word_is(w, "are") || word_is(w, "at") || word_is(w, "use") ||
         word_is(w, "using") || word_is(w, "to") || word_is(w, "the") ||
         word_is(w, "a") || word_is(w, "an");
}

static bool minterm_dc_word(const char *w) {
  return word_is(w, "dc") || word_is(w, "x") || word_is(w, "dont") ||
         word_is(w, "don't") || word_is(w, "dontcare") ||
         word_is(w, "dontcares") || word_is(w, "don'tcare") ||
         word_is(w, "don'tcares");
}

static bool make_minterm_cmd(const char *in, char *cmd, int cap) {
  char t[192]; raw_clean(in, t, sizeof(t));
  char vars[8] = ""; int vc = 0, mins[32], mc = 0, dcs[32], dc = 0;
  bool dcpart = false, varpart = false;
  for (int i = 0; t[i];) {
    while (t[i] == ',') ++i;
    char w[32]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    while (j > 0 && !isalnum((unsigned char)w[j-1])) w[--j] = 0;
    if (word_is(w, "minterm") || word_is(w, "minterms") || word_is(w, "ones")) {
      dcpart = false;
      varpart = false;
      continue;
    }
    if (word_is(w, "variable") || word_is(w, "variables") || word_is(w, "vars")) {
      varpart = true;
      continue;
    }
    if (word_is(w, "for") && mc > 0) {
      varpart = true;
      continue;
    }
    if (minterm_dc_word(w)) { dcpart = true; continue; }
    if (!w[0] || (!varpart && minterm_skip_word(w))) continue;
    if ((w[0] == '-' && isdigit((unsigned char)w[1])) || isdigit((unsigned char)w[0])) {
      if (dcpart) { if (dc < 32) dcs[dc++] = (int)parse_int(w); }
      else if (mc < 32) mins[mc++] = (int)parse_int(w);
      continue;
    }
    if (varpart && !(j == 1 && isalpha((unsigned char)w[0]))) continue;
    if ((varpart && j == 1 && isalpha((unsigned char)w[0])) || (!varpart && j <= 6)) {
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
  if (dc && p < cap - 24) {
    p += sprintf(cmd + p, ",dc");
    for (int i = 0; i < dc && p < cap - 24; ++i) p += sprintf(cmd + p, ",%d", dcs[i]);
  }
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
  if (width < 1) width = 1;
  if (width > 64) width = 64;
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
  if (starts2(s, "bcd(", "bcdenc(") && na == 1) {
    int n = add(out, 0, "BCD encodes each denary digit using 4 bits.");
    char all[96]; all[0] = 0;
    for (int i = 0; a[0][i]; ++i) if (isdigit((unsigned char)a[0][i])) {
      char b[8]; to_bin(a[0][i] - '0', 4, b);
      n = add(out, n, "%c -> %s", a[0][i], b);
      if (strlen(all) + 5 < sizeof(all)) strcat(all, b);
    }
    return add(out, n, "BCD = %s", all);
  }
  if (starts3(s, "bcddec(", "bcddecode(", "denbcd(") && na == 1) {
    int n = add(out, 0, "Decode BCD in groups of 4 bits.");
    char dec[32]; int d = 0;
    for (int i = 0; a[0][i] && a[0][i+3] && d + 1 < (int)sizeof(dec); i += 4) {
      char q[5]; memcpy(q, a[0] + i, 4); q[4] = 0;
      int v = bin_unsigned(q);
      n = add(out, n, "%s -> %d", q, v);
      dec[d++] = (char)('0' + v);
    }
    dec[d] = 0;
    return add(out, n, "denary = %s", dec);
  }
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
    int from = (int)parse_int(a[1]), to = (int)parse_int(a[2]);
    long long v = parse_base(a[0], from);
    if (to == 2) {
      char b[65];
      int w = from == 16 ? (int)strlen(a[0]) * 4 : from == 8 ? (int)strlen(a[0]) * 3 : 0;
      if (w > 0 && w < 65) to_bin(v, w, b); else to_base(v, 2, b, sizeof(b));
      return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %s_2", v, b);
    }
    if (parse_int(a[2]) == 16) return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %llX_16", v, v);
    char b[65]; to_base(v, (int)parse_int(a[2]), b, sizeof(b));
    return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %s_%s", v, b, a[2]);
  }
  return conv(out, a, na);
}

static int eval_twos(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts3(s, "bitsneeded(", "minbits(", "bitwidth(") && na >= 1) {
    long long v = parse_int(a[0]);
    bool tc = na > 1 && (starts(a[1], "twos") || starts(a[1], "tc") || starts(a[1], "signed"));
    bool sm = na > 1 && (starts(a[1], "sign") || starts(a[1], "sm"));
    if (tc) {
      int w = 1;
      while (w < 62 && (v < -(1LL << (w - 1)) || v > ((1LL << (w - 1)) - 1))) ++w;
      int n = add(out, 0, "Find the minimum two's-complement width.");
      n = add(out, n, "Need -2^(n-1) <= %lld <= 2^(n-1)-1.", v);
      return add(out, n, "n = %d gives range %lld to %lld, so %d bits are needed.", w, -(1LL << (w - 1)), (1LL << (w - 1)) - 1, w);
    }
    if (sm) {
      long long mag = v < 0 ? -v : v;
      int m = 1;
      while (m < 62 && mag > ((1LL << m) - 1)) ++m;
      int w = m + 1;
      int n = add(out, 0, "Find the minimum sign-and-magnitude width.");
      n = add(out, n, "Need 1 sign bit and m magnitude bits with |%lld| <= 2^m-1.", v);
      return add(out, n, "m = %d gives magnitude range 0 to %lld, so total bits = %d.", m, (1LL << m) - 1, w);
    }
    if (v < 0) return add(out, 0, "Unsigned binary cannot represent a negative value.");
    int w = 1;
    while (w < 63 && v > ((1LL << w) - 1)) ++w;
    int n = add(out, 0, "Find the minimum unsigned binary width.");
    n = add(out, n, "Need 0 <= %lld <= 2^n-1.", v);
    return add(out, n, "n = %d gives range 0 to %lld, so %d bits are needed.", w, (1LL << w) - 1, w);
  }
  if (starts2(s, "unsignedrange(", "urange(") && na == 1) {
    int w = (int)parse_int(a[0]);
    return add(out, add(out, 0, "n-bit unsigned range:"), "0 to 2^%d-1 = 0 to %d", w, (1<<w)-1);
  }
  if (starts3(s, "signmagrange(", "signmagnituderange(", "smrange(") && na == 1) {
    int w = (int)parse_int(a[0]), m = (1 << (w - 1)) - 1;
    int n = add(out, 0, "n-bit sign-and-magnitude range:");
    n = add(out, n, "1 sign bit and %d magnitude bits.", w - 1);
    return add(out, n, "-(2^%d-1) to +(2^%d-1) = -%d to %d, with +0 and -0.", w - 1, w - 1, m, m);
  }
  if (starts3(s, "twosrange(", "tcrange(", "twoscomprange(") && na == 1) {
    int w = (int)parse_int(a[0]);
    return add(out, add(out, 0, "n-bit two's complement range:"), "-2^(%d) to 2^(%d)-1 = %d to %d", w-1, w-1, -(1<<(w-1)), (1<<(w-1))-1);
  }
  if (starts3(s, "signmagdec(", "signmagnitudedec(", "smdec(") && na == 1) {
    int mag = bin_unsigned(a[0] + 1);
    int val = a[0][0] == '1' ? -mag : mag;
    int n = add(out, 0, "Sign-and-magnitude: first bit is sign, remaining bits are magnitude.");
    n = add(out, n, "sign bit %c means %s.", a[0][0], a[0][0] == '1' ? "negative" : "positive");
    n = add(out, n, "magnitude bits %s = %d.", a[0] + 1, mag);
    return add(out, n, "%s = %d", a[0], val);
  }
  if ((starts(s, "twosdec(") || starts(s, "tcdec(") || starts(s, "twosdecode(")) && na == 1) {
    int n = add(out, 0, "MSB is sign bit.");
    if (a[0][0] == '0') n = add(out, n, "MSB=0, so use unsigned place values.");
    else n = add(out, n, "MSB=1, so subtract 2^%d from unsigned value.", (int)strlen(a[0]));
    return add(out, n, "%s = %d", a[0], twos_decode(a[0]));
  }
  if (starts3(s, "signmag(", "signmagnitude(", "sm(") && na >= 2) {
    long long v = parse_int(a[0]); int w = (int)parse_int(a[1]);
    long long mag = v < 0 ? -v : v; char mb[65], b[66];
    to_bin(mag, w - 1, mb);
    b[0] = v < 0 ? '1' : '0';
    strcpy(b + 1, mb);
    int n = add(out, 0, "%d-bit sign-and-magnitude.", w);
    n = add(out, n, "sign bit = %d because the value is %s.", v < 0 ? 1 : 0, v < 0 ? "negative" : "positive");
    n = add(out, n, "magnitude = |%lld| = %lld -> %s", v, mag, mb);
    return add(out, n, "%lld -> %s", v, b);
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
  if (starts3(s, "binsub(", "binarysub(", "subtractbits(") && na >= 2) {
    int w = na > 2 ? (int)parse_int(a[2]) : (int)((strlen(a[0]) > strlen(a[1])) ? strlen(a[0]) : strlen(a[1]));
    long long x = bin_unsigned(a[0]), y = bin_unsigned(a[1]), diff = x - y;
    char b[65]; to_bin(diff, w, b);
    int n = add(out, 0, "For binary subtraction, subtract place values or add the two's complement of the subtrahend.");
    n = add(out, n, "%s_2 = %lld, %s_2 = %lld", a[0], x, a[1], y);
    n = add(out, n, "%lld - %lld = %lld", x, y, diff);
    if (diff < 0) n = add(out, n, "negative result shown modulo 2^%d in fixed width.", w);
    return add(out, n, "result = %s", b);
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
  if (starts3(s, "arithshift(", "arithmeticshift(", "signedshift(") && na >= 3) {
    int k = (int)parse_int(a[2]); char b[65]; int len = (int)strlen(a[0]);
    for (int i = 0; i < len && i < 64; ++i) b[i] = a[0][i]; b[len] = 0;
    bool left = a[1][0] == 'l' || a[1][0] == '+';
    char sign = b[0];
    for (int step = 0; step < k; ++step) {
      if (left) { for (int i = 0; i < len - 1; ++i) b[i] = b[i+1]; b[len-1] = '0'; }
      else { for (int i = len - 1; i > 0; --i) b[i] = b[i-1]; b[0] = sign; }
    }
    int before = twos_decode(a[0]), after = twos_decode(b);
    int n = add(out, 0, left ? "Arithmetic left shift: move bits left and fill right with 0." : "Arithmetic right shift: preserve the sign bit and move bits right.");
    n = add(out, n, "%s = %d in two's complement.", a[0], before);
    n = add(out, n, "%s shifted %s by %d = %s", a[0], left ? "left" : "right", k, b);
    n = add(out, n, "%s = %d in two's complement.", b, after);
    if (left && b[0] != sign) return add(out, n, "overflow risk: sign bit changed.");
    return add(out, n, left ? "value is multiplied by 2^%d if no overflow." : "value is divided by 2^%d, rounding toward negative infinity.", k);
  }
  if (starts2(s, "parity(", "paritybit(") && na >= 1) {
    int ones = 0; for (int i = 0; a[0][i]; ++i) if (a[0][i] == '1') ones++;
    bool odd = na > 1 && a[1][0] == 'o';
    int bit = odd ? (ones % 2 ? 0 : 1) : (ones % 2 ? 1 : 0);
    int n = add(out, 0, odd ? "Odd parity means total number of 1s is odd." : "Even parity means total number of 1s is even.");
    n = add(out, n, "%s has %d one-bits.", a[0], ones);
    return add(out, n, "parity bit = %d, transmitted bits = %s%d", bit, a[0], bit);
  }
  if (starts3(s, "repeatenc(", "repetitionenc(", "repeatencode(") && na >= 1) {
    int group = na > 1 ? (int)parse_int(a[1]) : 3;
    if (group < 2) group = 3;
    char enc[96]; int p = 0;
    for (int i = 0; a[0][i] && p + group + 1 < (int)sizeof(enc); ++i) {
      if (a[0][i] != '0' && a[0][i] != '1') continue;
      for (int j = 0; j < group; ++j) enc[p++] = a[0][i];
    }
    enc[p] = 0;
    int n = add(out, 0, "Repetition code: repeat each data bit %d times.", group);
    n = add(out, n, "data bits = %s", a[0]);
    return add(out, n, "transmitted bits = %s", enc);
  }
  if (starts3(s, "repeatdec(", "repetitiondec(", "majority(") && na >= 1) {
    int group = na > 1 ? (int)parse_int(a[1]) : 3;
    if (group < 2) group = 3;
    char dec[65], corr[96]; int dp = 0, cp = 0, errors = 0;
    int n = add(out, 0, "Repetition code: split into groups of %d and use majority voting.", group);
    for (int i = 0; a[0][i] && dp < 64; i += group) {
      int ones = 0, zeros = 0; char chunk[16]; int k = 0;
      for (int j = 0; j < group && a[0][i+j] && k + 1 < (int)sizeof(chunk); ++j) {
        chunk[k++] = a[0][i+j];
        if (a[0][i+j] == '1') ones++;
        else if (a[0][i+j] == '0') zeros++;
      }
      chunk[k] = 0;
      if (k < group) break;
      char bit = ones >= zeros ? '1' : '0';
      dec[dp++] = bit;
      for (int j = 0; j < group && cp + 1 < (int)sizeof(corr); ++j) corr[cp++] = bit;
      if ((bit == '1' && zeros) || (bit == '0' && ones)) errors++;
      n = add(out, n, "%s: %d ones, %d zeros -> %c", chunk, ones, zeros, bit);
    }
    dec[dp] = 0; corr[cp] = 0;
    n = add(out, n, "corrected transmitted bits = %s", corr);
    n = add(out, n, "decoded data bits = %s", dec);
    return add(out, n, errors ? "errors corrected by majority vote: %d group(s)." : "no group needed correction.", errors);
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
    n = add(out, n, "= %.6g KiB", bytes / 1024.0);
    n = add(out, n, "= %.6g MB", bytes / 1000000.0);
    return add(out, n, "= %.6g MiB", bytes / 1048576.0);
  }
  if (starts3(s, "imagecolors(", "bitmapcolors(", "colours(") && na >= 3) {
    int depth = ceil_log2_ll(parse_int(a[2]));
    long long bits = parse_int(a[0]) * parse_int(a[1]) * depth;
    double bytes = bits / 8.0;
    int n = add(out, 0, "Colour depth = ceil(log2(number of colours)).");
    n = add(out, n, "ceil(log2(%s)) = %d bits per pixel", a[2], depth);
    n = add(out, n, "Image bits = width * height * colour depth.");
    n = add(out, n, "%s*%s*%d = %lld bits = %.6g bytes", a[0], a[1], depth, bits, bytes);
    n = add(out, n, "= %.6g KiB", bytes / 1024.0);
    n = add(out, n, "= %.6g MB", bytes / 1000000.0);
    return add(out, n, "= %.6g MiB", bytes / 1048576.0);
  }
  if (starts3(s, "colourdepth(", "colordepth(", "bitsperpixel(") && na >= 1) {
    int colours = (int)parse_int(a[0]);
    int depth = ceil_log2_ll(colours);
    int n = add(out, 0, "Colour depth = ceil(log2(number of colours)).");
    n = add(out, n, "Need 2^b >= %d colours.", colours);
    return add(out, n, "ceil(log2(%d)) = %d bits per pixel", colours, depth);
  }
  if (starts3(s, "colourcount(", "colorcount(", "pixelcolours(") && na >= 1) {
    int depth = (int)parse_int(a[0]);
    long long colours = depth >= 62 ? 0 : (1LL << depth);
    int n = add(out, 0, "Number of colours = 2^(bits per pixel).");
    return add(out, n, "2^%d = %lld colours", depth, colours);
  }
  if (starts3(s, "symbolbits(", "symbolsbits(", "bitsforsymbols(") && na >= 1) {
    int symbols = (int)parse_int(a[0]);
    int bits = ceil_log2_ll(symbols);
    int n = add(out, 0, "Bits per symbol = ceil(log2(number of symbols)).");
    n = add(out, n, "Need 2^b >= %d symbols.", symbols);
    return add(out, n, "ceil(log2(%d)) = %d bits", symbols, bits);
  }
  if (starts3(s, "sound(", "audio(", "soundsize(") && na >= 3) {
    long long chans = na > 3 ? parse_int(a[3]) : 1;
    double bits = num(a[0]) * num(a[1]) * num(a[2]) * chans;
    double bytes = bits / 8.0;
    int n = add(out, 0, "Sound bits = sample rate * seconds * resolution * channels.");
    n = add(out, n, "%s*%s*%s*%lld = %.10g bits", a[0], a[1], a[2], chans, bits);
    n = add(out, n, "= %.10g bytes", bytes);
    n = add(out, n, "= %.10g MB", bytes / 1000000.0);
    return add(out, n, "= %.10g MiB", bytes / 1048576.0);
  }
  if (starts3(s, "bitrate(", "datarate(", "rate(") && na >= 2) {
    double rate = num(a[0]) / num(a[1]);
    int n = add(out, 0, "Bit rate = bits / seconds.");
    return add(out, n, "%s/%s = %.10g bit/s", a[0], a[1], rate);
  }
  if (starts3(s, "bitratemb(", "datemb(", "mbyterate(") && na >= 2) {
    double mb = num(a[0]), seconds = num(a[1]), megabits = mb * 8.0, rate = megabits / seconds;
    int n = add(out, 0, "Convert megabytes to megabits before finding bit rate.");
    n = add(out, n, "%.10g MB = %.10g Mbit", mb, megabits);
    return add(out, n, "bit rate = %.10g/%.10g = %.10g Mbit/s", megabits, seconds, rate);
  }
  if (starts3(s, "bitratekb(", "datekb(", "kbyterate(") && na >= 2) {
    double kb = num(a[0]), seconds = num(a[1]), kilobits = kb * 8.0, rate = kilobits / seconds;
    int n = add(out, 0, "Convert kilobytes to kilobits before finding bit rate.");
    n = add(out, n, "%.10g KB = %.10g kbit", kb, kilobits);
    return add(out, n, "bit rate = %.10g/%.10g = %.10g kbit/s", kilobits, seconds, rate);
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
    n = add(out, n, "= %.6g KiB", bytes / 1024.0);
    return add(out, n, "= %.6g MB", bytes / 1000000.0);
  }
  if (starts3(s, "charset(", "charsetsize(", "textsymbols(") && na >= 2) {
    int bpc = ceil_log2_ll(parse_int(a[1]));
    long long bits = parse_int(a[0]) * bpc;
    double bytes = bits / 8.0;
    int n = add(out, 0, "Bits per character = ceil(log2(character set size)).");
    n = add(out, n, "ceil(log2(%s)) = %d bits per character", a[1], bpc);
    n = add(out, n, "text bits = %s*%d = %lld bits", a[0], bpc, bits);
    n = add(out, n, "= %.6g bytes", bytes);
    return add(out, n, "= %.6g KiB", bytes / 1024.0);
  }
  if (starts3(s, "compress(", "compression(", "ratio(") && na >= 2) {
    double oldv = num(a[0]), newv = num(a[1]);
    int n = add(out, 0, "Compression ratio = original / compressed.");
    n = add(out, n, "ratio = %.10g : 1", oldv / newv);
    return add(out, n, "percentage reduction = %.6g%%", (oldv - newv) * 100.0 / oldv);
  }
  if (starts3(s, "dictcompress(", "dictionary(", "lzdict(") && na >= 4) {
    double orig = num(a[0]), dict = num(a[1]), refs = num(a[2]), rb = num(a[3]);
    double enc = dict + refs * rb;
    int n = add(out, 0, "Dictionary compression size = dictionary bits + reference bits.");
    n = add(out, n, "reference bits = %s*%s = %.10g", a[2], a[3], refs * rb);
    n = add(out, n, "compressed bits = %s + %.10g = %.10g", a[1], refs * rb, enc);
    n = add(out, n, "ratio = %.10g : 1", orig / enc);
    return add(out, n, "percentage reduction = %.6g%%", (orig - enc) * 100.0 / orig);
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
    return add(out, n, "compressed bits = encoded bits = %d*(%s+%s) = %lld", runs, a[1], a[2], enc);
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
    int ci = na - 3;
    char fields[96] = "";
    int p = 0;
    for (int i = 1; i < ci && p < (int)sizeof(fields) - 8; ++i)
      p += sprintf(fields + p, "%s%s", p ? "," : "", a[i]);
    const char *op = sql_op_text(a[ci + 1]);
    int n = add(out, 0, "SQL SELECT: choose fields, choose table, then apply WHERE.");
    n = add(out, n, "SELECT %s", fields);
    n = add(out, n, "FROM %s", a[0]);
    n = add(out, n, "WHERE %s %s %s", a[ci], op, a[ci + 2]);
    return add(out, n, "query = SELECT %s FROM %s WHERE %s %s %s", fields, a[0], a[ci], op, a[ci + 2]);
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
  if (starts3(s, "addressbits(", "minaddressbits(", "addresslines(") && na >= 1) {
    double bytes = num(a[0]);
    int wbits = na > 1 ? (int)parse_int(a[1]) : 8;
    double locations = bytes * 8.0 / wbits;
    int bits = ceil_log2_ll((long long)(locations + 0.999999));
    int n = add(out, 0, "Address bits = ceil(log2(addressable locations)).");
    n = add(out, n, "locations = %.10g*8/%d = %.10g", bytes, wbits, locations);
    return add(out, n, "ceil(log2(%.10g)) = %d bits", locations, bits);
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

static bool image_depth_is_bytes(const char *t) {
  bool bit_word = has(t, "bits") || has(t, "bit,") || has(t, "bit.") || has(t, "bitperpixel");
  return has(t, "bytesperpixel") || has(t, "byteperpixel") ||
         has(t, "bytes,per,pixel") || has(t, "byte,per,pixel") ||
         ((has(t, "bytes") || has(t, "byte")) && !bit_word);
}

static int add_image_byte_depth_lines(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], long long w, long long h, long long bytes_per_pixel) {
  long long depth = bytes_per_pixel * 8;
  long long bits = w * h * depth;
  double bytes = bits / 8.0;
  int n = add(out, 0, "%lld bytes per pixel = %lld bits per pixel.", bytes_per_pixel, depth);
  n = add(out, n, "Image bits = width * height * colour depth.");
  n = add(out, n, "%lld*%lld*%lld = %lld bits", w, h, depth, bits);
  n = add(out, n, "= %.6g bytes", bytes);
  n = add(out, n, "= %.6g KiB", bytes / 1024.0);
  return add(out, n, "= %.6g MB", bytes / 1000000.0);
}

static bool storage_size_bits(const char *t, double size, double *bits, const char **unit) {
  if (has(t, "gib")) { *bits = size * 1073741824.0 * 8.0; *unit = "GiB"; return true; }
  if (has(t, "gb") || has(t, "gigabyte")) { *bits = size * 1000000000.0 * 8.0; *unit = "GB"; return true; }
  if (has(t, "mib")) { *bits = size * 1048576.0 * 8.0; *unit = "MiB"; return true; }
  if (has(t, "mb") || has(t, "megabyte")) { *bits = size * 1000000.0 * 8.0; *unit = "MB"; return true; }
  if (has(t, "kib")) { *bits = size * 1024.0 * 8.0; *unit = "KiB"; return true; }
  if (has(t, "kb") || has(t, "kilobyte")) { *bits = size * 1000.0 * 8.0; *unit = "KB"; return true; }
  if (has(t, "byte")) { *bits = size * 8.0; *unit = "bytes"; return true; }
  if (has(t, "bit")) { *bits = size; *unit = "bits"; return true; }
  return false;
}

static bool bytes_before_unit(const char *t, const char *unit, double mult, double *bytes, double *shown) {
  double v = 0;
  if (!scan_before_word_num(t, unit, &v)) return false;
  *shown = v; *bytes = v * mult;
  return true;
}

static int add_compression_unit_lines(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], const char *t) {
  const char *names[] = {"GiB","GB","MiB","MB","KiB","KB","bytes"};
  const char *keys[] = {"gib","gb","mib","mb","kib","kb","bytes"};
  double mult[] = {1073741824.0,1000000000.0,1048576.0,1000000.0,1024.0,1000.0,1.0};
  double b[4], shown[4]; const char *unit[4]; int nb = 0;
  for (int i = 0; i < 7 && nb < 4; ++i) {
    if (bytes_before_unit(t, keys[i], mult[i], &b[nb], &shown[nb])) { unit[nb] = names[i]; ++nb; }
  }
  if (nb < 2) return 0;
  int oi = b[0] >= b[1] ? 0 : 1, ni = oi ? 0 : 1;
  double oldb = b[oi], newb = b[ni];
  if (oldb <= 0 || newb <= 0) return 0;
  int n = add(out, 0, "Compression ratio = original / compressed.");
  n = add(out, n, "%.10g %s = %.10g bytes", shown[oi], unit[oi], oldb);
  n = add(out, n, "%.10g %s = %.10g bytes", shown[ni], unit[ni], newb);
  n = add(out, n, "ratio = %.10g : 1", oldb / newb);
  return add(out, n, "percentage reduction = %.6g%%", (oldb - newb) * 100.0 / oldb);
}

static bool storage_rate_bits(const char *t, double rate, double *bps, const char **unit) {
  if (has(t, "gbit") || has(t, "gigabit")) { *bps = rate * 1000000000.0; *unit = "Gbit/s"; return true; }
  if (has(t, "gbps")) { *bps = rate * 1000000000.0; *unit = "Gbit/s"; return true; }
  if (has(t, "mbit") || has(t, "megabit")) { *bps = rate * 1000000.0; *unit = "Mbit/s"; return true; }
  if (has(t, "mbps")) { *bps = rate * 1000000.0; *unit = "Mbit/s"; return true; }
  if (has(t, "kbit") || has(t, "kilobit")) { *bps = rate * 1000.0; *unit = "kbit/s"; return true; }
  if (has(t, "kbps")) { *bps = rate * 1000.0; *unit = "kbit/s"; return true; }
  if (has(t, "bit")) { *bps = rate; *unit = "bit/s"; return true; }
  return false;
}

static int add_transfer_unit_lines(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], double size, double rate, const char *t) {
  double bits=0, bps=0; const char *su="", *ru="";
  if (!storage_size_bits(t, size, &bits, &su) || !storage_rate_bits(t, rate, &bps, &ru) || bps == 0) return 0;
  double seconds = bits / bps;
  int n = add(out, 0, "Transfer time = file size in bits / bit rate.");
  n = add(out, n, "%.10g %s = %.10g bits", size, su, bits);
  n = add(out, n, "%.10g %s = %.10g bit/s", rate, ru, bps);
  n = add(out, n, "time = %.10g/%.10g = %.10g s", bits, bps, seconds);
  if (has(t, "minute")) return add(out, n, "= %.10g minutes", seconds / 60.0);
  if (has(t, "hour")) return add(out, n, "= %.10g hours", seconds / 3600.0);
  return n;
}

static int add_bitrate_unit_lines(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], double size, double seconds, const char *t) {
  double bits=0; const char *su="";
  if (!storage_size_bits(t, size, &bits, &su) || seconds == 0) return 0;
  int n = add(out, 0, "Bit rate = file size in bits / seconds.");
  n = add(out, n, "%.10g %s = %.10g bits", size, su, bits);
  double bps = bits / seconds;
  n = add(out, n, "bit rate = %.10g/%.10g = %.10g bit/s", bits, seconds, bps);
  if (has(t, "mbit") || has(t, "megabit")) return add(out, n, "= %.10g Mbit/s", bps / 1000000.0);
  if (has(t, "kbit") || has(t, "kilobit")) return add(out, n, "= %.10g kbit/s", bps / 1000.0);
  if (has(t, "gbit") || has(t, "gigabit")) return add(out, n, "= %.10g Gbit/s", bps / 1000000000.0);
  return n;
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
    const char *dot = strchr(a[0], '.');
    if (dot) {
      char whole[48]; int wi = 0;
      for (int i = 0; a[0][i] && a[0][i] != '.' && wi + 1 < (int)sizeof(whole); ++i) whole[wi++] = a[0][i];
      whole[wi] = 0;
      n = add(out, n, "whole part %s is %d in two's complement.", whole, twos_decode(whole));
      double frac = 0;
      for (int i = 1; dot[i]; ++i) if (dot[i] == '1') {
        frac += pow2(-i);
        n = add(out, n, "fraction bit %d contributes 2^-%d = %.10g", i, i, pow2(-i));
      }
      n = add(out, n, "value = %d + %.10g", twos_decode(whole), frac);
    }
    return add(out, n, "%s_2 = %.10g_10", a[0], v);
  }
  if (starts3(s, "fixedenc(", "fixedencode(", "fixedpointenc(") && na >= 3) {
    double value = num(a[0]); int whole = (int)parse_int(a[1]), frac = (int)parse_int(a[2]), total = whole + frac;
    long long scaled = (long long)round_nearest(value * pow2(frac));
    char bits[65], fixed[65]; to_bin(scaled, total, bits); insert_point(bits, whole, fixed);
    int n = add(out, 0, "Encode fixed point by scaling by 2^fraction bits.");
    n = add(out, n, "scaled integer: %.10g * 2^%d = %.10g, so store nearest integer %lld", value, frac, value * pow2(frac), scaled);
    n = add(out, n, "%d whole bits and %d fractional bits.", whole, frac);
    return add(out, n, "fixed point = %s", fixed);
  }
  if (starts3(s, "fixedfrac(", "fixedfraction(", "fixedfracenc(") && na >= 2) {
    double value = num(a[0]); int frac = (int)parse_int(a[1]), whole = whole_bits_for(value), total = whole + frac;
    long long scaled = (long long)round_nearest(value * pow2(frac));
    char bits[65], fixed[65]; to_bin(scaled, total, bits); insert_point(bits, whole, fixed);
    int n = add(out, 0, "Choose enough whole bits, then scale by 2^fraction bits.");
    n = add(out, n, "whole bits needed for %.10g = %d", value, whole);
    n = add(out, n, "scaled integer: %.10g * 2^%d = %.10g, so store nearest integer %lld", value, frac, value * pow2(frac), scaled);
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
  if ((starts3(s, "floatadd(", "fpadd(", "floatingadd(") || starts3(s, "floatsub(", "fpsub(", "floatingsub(")) && na >= 4) {
    bool sub = starts3(s, "floatsub(", "fpsub(", "floatingsub(");
    double m1 = mantissa_decode(a[0]), m2 = mantissa_decode(a[2]);
    int e1 = twos_decode(a[1]), e2 = twos_decode(a[3]);
    int mb = (int)strlen(a[0]), eb = (int)strlen(a[1]), common = e1 > e2 ? e1 : e2;
    double am1 = m1 * pow2(e1 - common), am2 = m2 * pow2(e2 - common);
    double rm = sub ? am1 - am2 : am1 + am2;
    int re = common;
    while (rm != 0 && (rm >= 1.0 || rm < -1.0)) { rm /= 2.0; ++re; }
    while (rm != 0 && rm > -0.5 && rm < 0.5) { rm *= 2.0; --re; }
    char mant[65], expb[65]; mantissa_encode(rm, mb, mant); to_bin(re, eb, expb);
    int n = add(out, 0, sub ? "Subtract floating-point numbers by aligning exponents." : "Add floating-point numbers by aligning exponents.");
    n = add(out, n, "first: mantissa %s = %.10g, exponent %s = %d", a[0], m1, a[1], e1);
    n = add(out, n, "second: mantissa %s = %.10g, exponent %s = %d", a[2], m2, a[3], e2);
    n = add(out, n, "use common exponent %d.", common);
    n = add(out, n, "%.10g*2^%d = %.10g*2^%d", m1, e1, am1, common);
    n = add(out, n, "%.10g*2^%d = %.10g*2^%d", m2, e2, am2, common);
    n = add(out, n, sub ? "mantissas: %.10g - %.10g = %.10g" : "mantissas: %.10g + %.10g = %.10g", am1, am2, sub ? am1-am2 : am1+am2);
    n = add(out, n, "normalise: %.10g * 2^%d", rm, re);
    n = add(out, n, "mantissa (%d bits) = %s", mb, mant);
    return add(out, n, "exponent (%d bits) = %s", eb, expb);
  }
  if ((starts3(s, "floatmul(", "fpmul(", "floatingmul(") || starts3(s, "floatdiv(", "fpdiv(", "floatingdiv(")) && na >= 4) {
    bool div = starts3(s, "floatdiv(", "fpdiv(", "floatingdiv(");
    double m1 = mantissa_decode(a[0]), m2 = mantissa_decode(a[2]);
    int e1 = twos_decode(a[1]), e2 = twos_decode(a[3]);
    int mb = (int)strlen(a[0]), eb = (int)strlen(a[1]);
    if (div && m2 == 0) return add(out, 0, "Cannot divide by zero mantissa.");
    double rm = div ? m1 / m2 : m1 * m2;
    int re = div ? e1 - e2 : e1 + e2;
    double rawm = rm; int rawe = re;
    while (rm != 0 && (rm >= 1.0 || rm < -1.0)) { rm /= 2.0; ++re; }
    while (rm != 0 && rm > -0.5 && rm < 0.5) { rm *= 2.0; --re; }
    char mant[65], expb[65]; mantissa_encode(rm, mb, mant); to_bin(re, eb, expb);
    int n = add(out, 0, div ? "Divide floating-point numbers: divide mantissas and subtract exponents." : "Multiply floating-point numbers: multiply mantissas and add exponents.");
    n = add(out, n, "first: mantissa %s = %.10g, exponent %s = %d", a[0], m1, a[1], e1);
    n = add(out, n, "second: mantissa %s = %.10g, exponent %s = %d", a[2], m2, a[3], e2);
    n = add(out, n, div ? "mantissas: %.10g / %.10g = %.10g" : "mantissas: %.10g * %.10g = %.10g", m1, m2, rawm);
    n = add(out, n, div ? "exponents: %d - %d = %d" : "exponents: %d + %d = %d", e1, e2, rawe);
    n = add(out, n, "normalise: %.10g * 2^%d", rm, re);
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
  if (starts3(s, "floatbitsadd(", "mantissabitsadd(", "exactfloatbits(") && na >= 3) {
    double value = num(a[0]); int mb = (int)parse_int(a[1]);
    int e = 0; double m = value;
    if (m != 0) {
      while (m >= 1.0 || m < -1.0) { m /= 2.0; ++e; }
      while (m > -0.5 && m < 0.5) { m *= 2.0; --e; }
    }
    int addbits = 0;
    for (; addbits <= 32; ++addbits) {
      double step = pow2(e - ((mb + addbits) - 1));
      double q = value / step, rq = round_nearest(q);
      double diff = q > rq ? q - rq : rq - q;
      if (diff < 1e-9) break;
    }
    double oldstep = pow2(e - (mb - 1));
    int n = add(out, 0, "Find the mantissa precision needed for an exact value.");
    n = add(out, n, "%.10g = %.10g * 2^%d", value, m, e);
    n = add(out, n, "current step = 2^(%d-(%d-1)) = %.10g", e, mb, oldstep);
    if (addbits > 32) return add(out, n, "More than 32 extra mantissa bits needed in this search.");
    double newstep = pow2(e - ((mb + addbits) - 1));
    n = add(out, n, "exact step = 2^(%d-(%d-1)) = %.10g", e, mb + addbits, newstep);
    return add(out, n, "extra mantissa bits needed = %d", addbits);
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
    n = add(out, n, "most negative normal = -1*2^%d = %.10g", emax, -pow2(emax));
    return add(out, n, "largest positive = (1-step)*2^%d = %.10g", emax, maxpos);
  }
  if (starts3(s, "floatcanrepresent(", "floatrepresentable(", "fpcan(") && na >= 3) {
    double value = num(a[0]); int mb = (int)parse_int(a[1]), eb = (int)parse_int(a[2]);
    int emin = -(1 << (eb - 1)), emax = (1 << (eb - 1)) - 1;
    double step = pow2(-(mb - 1));
    double minpos = 0.5 * pow2(emin);
    double maxpos = (1.0 - step) * pow2(emax);
    int n = add(out, 0, "Check representability using the normalised floating-point range.");
    n = add(out, n, "exponent range = %d to %d", emin, emax);
    n = add(out, n, "smallest positive normal = 0.5*2^%d = %.10g", emin, minpos);
    n = add(out, n, "largest positive = (1-step)*2^%d = %.10g", emax, maxpos);
    if (value > 0 && value < minpos) return add(out, n, "%.10g is too small for a normalised value.", value);
    if (value > maxpos) return add(out, n, "%.10g is too large for this format.", value);
    return add(out, n, "%.10g is within range; use closest-representable rounding if it is not exact.", value);
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
    if (src[i] == '.' || src[i] == '*') { dst[j++] = '&'; ++i; continue; }
    if (src[i] == ',') { dst[j++] = '\''; ++i; continue; }
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

static void join_bool_parts(char parts[][40], int count, char op, char *res, int cap) {
  int p = 0; res[0] = 0;
  for (int i = 0; i < count; ++i) {
    if (i) app_ch(res, &p, cap, op);
    bool wrap = op == '&' && top_op(parts[i], '+') > 0;
    if (wrap) app_ch(res, &p, cap, '(');
    app_str(res, &p, cap, parts[i]);
    if (wrap) app_ch(res, &p, cap, ')');
  }
}

static void join_except(char parts[][40], int count, int skip, char op, char *res, int cap) {
  int p = 0; res[0] = 0;
  for (int i = 0; i < count; ++i) if (i != skip) {
    if (p) app_ch(res, &p, cap, op);
    app_str(res, &p, cap, parts[i]);
  }
  if (!p) app_ch(res, &p, cap, '1');
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
    if (op == '&' && top_op(e, '+') > 0) continue;
    char parts[6][40]; int pc = split_top_parts(e, op, parts, 6);
    if (pc < 2) continue;
    for (int i = 0; i < pc; ++i) {
      char child[80], childlaw[32];
      if (bool_law_once(parts[i], child, childlaw) && strcmp(parts[i], child) != 0 && bool_equiv_expr(parts[i], child)) {
        strncpy(parts[i], child, sizeof(parts[i]) - 1); parts[i][sizeof(parts[i]) - 1] = 0;
        join_bool_parts(parts, pc, op, res, 80);
        strncpy(law, childlaw, 31); law[31] = 0;
        return true;
      }
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
      char pa[6][40], pb[6][40];
      int na = split_top_parts(aa, '&', pa, 6), nb = split_top_parts(bb, '&', pb, 6);
      if (nb > 1) {
        for (int i = 0; i < nb; ++i) if (is_comp_pair(aa, pb[i])) {
          char rem[80]; join_except(pb, nb, i, '&', rem, sizeof(rem));
          if (strcmp(rem, "1") == 0) strcpy(res, "1");
          else sprintf(res, "%s+%s", aa, rem);
          strcpy(law, "Covering law"); return true;
        }
      }
      if (na > 1) {
        for (int i = 0; i < na; ++i) if (is_comp_pair(bb, pa[i])) {
          char rem[80]; join_except(pa, na, i, '&', rem, sizeof(rem));
          if (strcmp(rem, "1") == 0) strcpy(res, "1");
          else sprintf(res, "%s+%s", bb, rem);
          strcpy(law, "Covering law"); return true;
        }
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
      char pa[6][40], pb[6][40];
      int na = split_top_parts(aa, '+', pa, 6), nb = split_top_parts(bb, '+', pb, 6);
      if (na > 1 && nb > 1) {
        bool a_in_b = true, b_in_a = true;
        for (int i = 0; i < na; ++i) if (!has_lit(pb, nb, pa[i])) a_in_b = false;
        for (int i = 0; i < nb; ++i) if (!has_lit(pa, na, pb[i])) b_in_a = false;
        if (a_in_b) { strcpy(res, aa); strcpy(law, "Absorption law"); return true; }
        if (b_in_a) { strcpy(res, bb); strcpy(law, "Absorption law"); return true; }
      }
      if (nb > 1) {
        for (int i = 0; i < nb; ++i) if (is_comp_pair(aa, pb[i])) {
          char rem[80]; join_except(pb, nb, i, '+', rem, sizeof(rem));
          if (strcmp(rem, "1") == 0) strcpy(res, "0");
          else sprintf(res, "%s&%s", aa, rem);
          strcpy(law, "Covering law"); return true;
        }
      }
      if (na > 1) {
        for (int i = 0; i < na; ++i) if (is_comp_pair(bb, pa[i])) {
          char rem[80]; join_except(pa, na, i, '+', rem, sizeof(rem));
          if (strcmp(rem, "1") == 0) strcpy(res, "0");
          else sprintf(res, "%s&%s", bb, rem);
          strcpy(law, "Covering law"); return true;
        }
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
  for (int step = 0; step < 10; ++step) {
    if (!bool_law_once(cur, next, law)) break;
    if (strcmp(cur, next) == 0) break;
    if (strcmp(law, "XOR identity") != 0 && !bool_equiv_expr(cur, next)) break;
    if (step == 0) n = add(out, n, "Simplify by Boolean algebra.");
    n = add(out, n, "%s -> %s (%s)", cur, next, law);
    strncpy(cur, next, sizeof(cur) - 1); cur[sizeof(cur) - 1] = 0;
  }
  return n;
}

static int add_named_bool_law_trace(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], int n, const char *name, const char *expr) {
  char cur[96], next[96], law[32];
  strncpy(cur, expr, sizeof(cur) - 1); cur[sizeof(cur) - 1] = 0;
  for (int step = 0; step < 10; ++step) {
    if (!bool_law_once(cur, next, law)) break;
    if (strcmp(cur, next) == 0) break;
    if (strcmp(law, "XOR identity") != 0 && !bool_equiv_expr(cur, next)) break;
    n = add(out, n, "%s: %s -> %s (%s)", name, cur, next, law);
    strncpy(cur, next, sizeof(cur) - 1); cur[sizeof(cur) - 1] = 0;
  }
  return n;
}

static bool int_seen(const int *v, int n, int x) {
  for (int i = 0; i < n; ++i) if (v[i] == x) return true;
  return false;
}

static int minimise_rows(const int *ones, int oc, const int *dcs, int dc, Imp *chosen, int maxchosen) {
  Imp cur[256], next[256], primes[256]; int cc = 0, pc = 0;
  for (int i = 0; i < oc && cc < 256; ++i)
    if (!int_seen(ones, i, ones[i])) cur[cc++] = { ones[i], 0, 0 };
  for (int i = 0; i < dc && cc < 256; ++i)
    if (!int_seen(ones, oc, dcs[i]) && !int_seen(dcs, i, dcs[i])) cur[cc++] = { dcs[i], 0, 0 };
  if (!oc) return 0;
  for (;;) {
    int nc = 0; for (int i = 0; i < cc; ++i) cur[i].used = 0;
    for (int i = 0; i < cc; ++i) for (int j = i + 1; j < cc; ++j) {
      if (cur[i].mask != cur[j].mask) continue;
      int d = (cur[i].bits ^ cur[j].bits) & ~cur[i].mask;
      if (d && (d & (d - 1)) == 0) {
        cur[i].used = cur[j].used = 1;
        Imp q = { cur[i].bits & ~d, cur[i].mask | d, 0 };
        bool seen = false; for (int k = 0; k < nc; ++k) if (next[k].bits == q.bits && next[k].mask == q.mask) seen = true;
        if (!seen && nc < 256) next[nc++] = q;
      }
    }
    for (int i = 0; i < cc && pc < 256; ++i) if (!cur[i].used) primes[pc++] = cur[i];
    if (!nc) break;
    for (int i = 0; i < nc; ++i) cur[i] = next[i];
    cc = nc;
  }
  bool covered[64] = {0}; int chc = 0;
  while (true) {
    int best = -1, bestc = -1;
    for (int i = 0; i < pc; ++i) {
      int c = 0; for (int j = 0; j < oc; ++j) if (!covered[j] && covers(primes[i], ones[j])) c++;
      if (c > bestc || (c == bestc && best >= 0 && bitcount(primes[i].mask) > bitcount(primes[best].mask))) { bestc = c; best = i; }
    }
    if (bestc <= 0 || chc >= maxchosen) break;
    chosen[chc++] = primes[best];
    for (int j = 0; j < oc; ++j) if (covers(primes[best], ones[j])) covered[j] = true;
  }
  return chc;
}

static void imp_sop_list(const Imp *chosen, int chc, const char *vars, int vc, char *buf, int cap) {
  int p = 0; buf[0] = 0;
  for (int i = 0; i < chc; ++i) {
    char t[32]; imp_text(chosen[i], vars, vc, t);
    if (i) app_ch(buf, &p, cap, '+');
    app_str(buf, &p, cap, t);
  }
}

static void bool_simplified_from_rows(const int *mins, int mc, int rows, const char *vars, int vc, char *buf, int cap) {
  if (mc == 0) { strncpy(buf, "0", cap - 1); buf[cap - 1] = 0; return; }
  if (mc == rows) { strncpy(buf, "1", cap - 1); buf[cap - 1] = 0; return; }
  Imp chosen[32]; int chc = minimise_rows(mins, mc, 0, 0, chosen, 32);
  imp_sop_list(chosen, chc, vars, vc, buf, cap);
}

static void imp_pos_text(const Imp &p, const char *vars, int vc, char *buf, int cap) {
  int pos = 0, parts = 0;
  app_ch(buf, &pos, cap, '(');
  for (int i = 0; i < vc; ++i) {
    int bit = 1 << (vc - 1 - i);
    if (p.mask & bit) continue;
    if (parts++) app_ch(buf, &pos, cap, '+');
    app_ch(buf, &pos, cap, vars[i]);
    if (p.bits & bit) app_ch(buf, &pos, cap, '\'');
  }
  if (!parts) app_ch(buf, &pos, cap, '0');
  app_ch(buf, &pos, cap, ')');
}

static void imp_pos_list(const Imp *chosen, int chc, const char *vars, int vc, char *buf, int cap) {
  int p = 0; buf[0] = 0;
  for (int i = 0; i < chc; ++i) {
    char t[32]; imp_pos_text(chosen[i], vars, vc, t, sizeof(t));
    if (i) app_ch(buf, &p, cap, '&');
    app_str(buf, &p, cap, t);
  }
}

static int eval_bool(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[2][48]; int na = args(s, a, 2);
  char exprbuf[96], inner[96];
  bool truthmode = starts(s, "truth(") || starts(s, "truthtable(") || starts(s, "truthrows(");
  const char *expr = (starts(s, "bool(") || truthmode || starts(s, "boolean(") || starts(s, "logic(")) && na ? a[0] : 0;
  if ((starts(s, "bool(") || truthmode || starts(s, "boolean(") || starts(s, "logic(")) &&
      strchr(s, '(') && strrchr(s, ')')) {
    const char *l = strchr(s, '('), *r = strrchr(s, ')');
    int p = 0;
    for (const char *q = l + 1; q < r && p + 1 < (int)sizeof(inner); ++q) inner[p++] = *q;
    inner[p] = 0;
    expr = inner;
  }
  if (!expr) return 0;
  bool_norm(expr, exprbuf, sizeof(exprbuf));
  expr = exprbuf;
  char vars[8]; int vc; collect_vars(expr, vars, &vc);
  if (vc == 0 || vc > 6) return add(out, 0, "Use up to 6 Boolean variables.");
  int mins[64], mc = 0, rows = 1 << vc;
  int vals[64];
  for (int m = 0; m < rows; ++m) {
    BParser p = { expr, 0, m, {0}, vc };
    for (int i = 0; i < vc; ++i) p.vars[i] = vars[i];
    vals[m] = p.expr() ? 1 : 0;
    if (vals[m]) mins[mc++] = m;
  }
  int n = 0;
  n = add_bool_law_trace(out, n, expr);
  if (truthmode) {
    char head[48] = ""; int hp = 0;
    for (int i = 0; i < vc; ++i) {
      if (i) app_ch(head, &hp, sizeof(head), ' ');
      app_ch(head, &hp, sizeof(head), vars[i]);
    }
    n = add(out, n, "Truth table for %s.", expr);
    n = add(out, n, "%s | F", head);
    if (vc <= 4) {
      for (int m = 0; m < rows && n < CSCALC_MAX_LINES - 4; ++m) {
        char row[48] = ""; int rp = 0;
        for (int i = 0; i < vc; ++i) {
          if (i) app_ch(row, &rp, sizeof(row), ' ');
          app_ch(row, &rp, sizeof(row), ((m >> (vc - 1 - i)) & 1) ? '1' : '0');
        }
        n = add(out, n, "%s | %d", row, vals[m]);
      }
    } else {
      n = add(out, n, "%d rows; use minterms for compact working.", rows);
    }
  }
  n = add(out, n, "Make truth table, list rows where output is 1.");
  char ml[80] = ""; int pos = 0;
  for (int i = 0; i < mc; ++i) {
    if (i) app_ch(ml, &pos, 80, ',');
    app_int(ml, &pos, 80, mins[i]);
  }
  n = add(out, n, "minterms: %s", mc ? ml : "none");
  if (mc == 0) return add(out, n, "simplified = 0");
  if (mc == rows) return add(out, n, "simplified = 1");

  Imp cur[256], next[256], primes[256]; int cc = 0, pc = 0;
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
        if (!seen && nc < 256) next[nc++] = q;
      }
    }
    for (int i = 0; i < cc && pc < 256; ++i) if (!cur[i].used) primes[pc++] = cur[i];
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

static void maxterm_expr(int m, const char *vars, int vc, char *buf, int cap) {
  int p = 0;
  app_ch(buf, &p, cap, '(');
  for (int i = 0; i < vc; ++i) {
    if (i) app_ch(buf, &p, cap, '+');
    int bit = 1 << (vc - 1 - i);
    app_ch(buf, &p, cap, vars[i]);
    if (m & bit) app_ch(buf, &p, cap, '\'');
  }
  app_ch(buf, &p, cap, ')');
}

static int eval_posform(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[2][48]; int na = args(s, a, 2);
  if (!(starts3(s, "posform(", "cnf(", "productofsums(") && na >= 1)) return 0;
  char exprbuf[96]; bool_norm(a[0], exprbuf, sizeof(exprbuf));
  char vars[8]; int vc; collect_vars(exprbuf, vars, &vc);
  if (vc == 0 || vc > 6) return add(out, 0, "Use up to 6 Boolean variables.");
  int rows = 1 << vc, zeros[64], zc = 0;
  for (int m = 0; m < rows; ++m) {
    BParser p = { exprbuf, 0, m, {0}, vc };
    for (int i = 0; i < vc; ++i) p.vars[i] = vars[i];
    if (!p.expr()) zeros[zc++] = m;
  }
  int n = add(out, 0, "Product-of-sums method: make a truth table and list 0-cells.");
  char vl[32] = ""; int vp = 0;
  for (int i = 0; i < vc; ++i) { if (i) app_ch(vl, &vp, sizeof(vl), ','); app_ch(vl, &vp, sizeof(vl), vars[i]); }
  n = add(out, n, "variables: %s", vl);
  char zl[96] = ""; int zp = 0;
  for (int i = 0; i < zc; ++i) { if (i) app_ch(zl, &zp, sizeof(zl), ','); app_int(zl, &zp, sizeof(zl), zeros[i]); }
  n = add(out, n, "zero rows: %s", zc ? zl : "none");
  if (!zc) return add(out, n, "POS = 1");
  if (zc == rows) return add(out, n, "POS = 0");
  char pos[112] = ""; int pp = 0;
  for (int i = 0; i < zc; ++i) {
    char term[32]; maxterm_expr(zeros[i], vars, vc, term, sizeof(term));
    if (i) app_ch(pos, &pp, sizeof(pos), '&');
    app_str(pos, &pp, sizeof(pos), term);
    if (n < CSCALC_MAX_LINES - 5) n = add(out, n, "M%d = %s", zeros[i], term);
  }
  n = add(out, n, "POS = %s", pos);
  Imp chosen[32]; int chc = minimise_rows(zeros, zc, 0, 0, chosen, 32);
  char sim[96]; imp_pos_list(chosen, chc, vars, vc, sim, sizeof(sim));
  return add(out, n, "simplified POS = %s", chc ? sim : "1");
}

static int eval_truthbits(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[10][48]; int na = args(s, a, 10);
  if (!(starts3(s, "truthbits(", "truthout(", "outputbits(") && na >= 1)) return 0;
  const char *bits = a[na - 1];
  if (!is_bits(bits) || strchr(bits, '.')) return add(out, 0, "Give output bits, e.g. truthbits(A,B,0110).");
  int rows = (int)strlen(bits), vc = 0;
  while ((1 << vc) < rows && vc < 6) ++vc;
  if ((1 << vc) != rows || vc == 0) return add(out, 0, "Output bit count must be 2^variables.");
  char vars[8] = "";
  for (int i = 0; i < na - 1 && vc < 6; ++i) {
    for (int j = 0; a[i][j] && (int)strlen(vars) < vc; ++j) if (isalpha((unsigned char)a[i][j])) {
      char c = (char)toupper((unsigned char)a[i][j]);
      if (!strchr(vars, c)) { int l = (int)strlen(vars); vars[l] = c; vars[l+1] = 0; }
    }
  }
  while ((int)strlen(vars) < vc) { int l = (int)strlen(vars); vars[l] = (char)('A' + l); vars[l+1] = 0; }
  int mins[64], zeros[64], mc = 0, zc = 0;
  for (int i = 0; i < rows; ++i) (bits[i] == '1' ? mins[mc++] : zeros[zc++]) = i;
  int n = add(out, 0, "Truth-table output-column method.");
  char vl[32] = ""; int vp = 0;
  for (int i = 0; i < vc; ++i) { if (i) app_ch(vl, &vp, sizeof(vl), ','); app_ch(vl, &vp, sizeof(vl), vars[i]); }
  n = add(out, n, "variables: %s", vl);
  n = add(out, n, "output bits: %s", bits);
  char ml[96] = "", zl[96] = ""; int mp = 0, zp = 0;
  for (int i = 0; i < mc; ++i) { if (i) app_ch(ml, &mp, sizeof(ml), ','); app_int(ml, &mp, sizeof(ml), mins[i]); }
  for (int i = 0; i < zc; ++i) { if (i) app_ch(zl, &zp, sizeof(zl), ','); app_int(zl, &zp, sizeof(zl), zeros[i]); }
  n = add(out, n, "1-cells/minterms: %s", mc ? ml : "none");
  n = add(out, n, "0-cells/maxterms: %s", zc ? zl : "none");
  if (!mc) return add(out, n, "simplified = 0");
  if (mc == rows) return add(out, n, "simplified = 1");
  char sop[112] = ""; int sp = 0;
  for (int i = 0; i < mc; ++i) {
    char term[32]; minterm_expr(mins[i], vars, vc, term, sizeof(term));
    if (i) app_ch(sop, &sp, sizeof(sop), '+');
    app_str(sop, &sp, sizeof(sop), term);
  }
  n = add(out, n, "SOP = %s", sop);
  Imp chosen[32]; int chc = minimise_rows(mins, mc, 0, 0, chosen, 32);
  char sim[96]; imp_sop_list(chosen, chc, vars, vc, sim, sizeof(sim));
  n = add(out, n, "simplified SOP = %s", chc ? sim : "0");
  if (zc) {
    char pos[112] = ""; int pp = 0;
    for (int i = 0; i < zc; ++i) {
      char term[32]; maxterm_expr(zeros[i], vars, vc, term, sizeof(term));
      if (i) app_ch(pos, &pp, sizeof(pos), '&');
      app_str(pos, &pp, sizeof(pos), term);
    }
    n = add(out, n, "POS = %s", pos);
    Imp pchosen[32]; int pc = minimise_rows(zeros, zc, 0, 0, pchosen, 32);
    char psim[96]; imp_pos_list(pchosen, pc, vars, vc, psim, sizeof(psim));
    n = add(out, n, "simplified POS = %s", pc ? psim : "1");
  }
  return n;
}

static int eval_minterms(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[40][48]; int na = args(s, a, 40);
  bool maxmode = starts3(s, "maxterms(", "pos(", "zeros(");
  bool dcmode = starts2(s, "kmapdc(", "mintermsdc(") || starts2(s, "dcminterms(", "dontcare(");
  if (!(starts3(s, "minterms(", "kmap(", "karnaugh(") || maxmode || dcmode) || na < 1) return 0;
  char vars[8] = ""; int vc = 0, mins[32], mc = 0, dcs[32], dc = 0;
  bool dcpart = false;
  for (int i = 0; i < na; ++i) {
    if (minterm_dc_word(a[i])) { dcpart = true; continue; }
    if (isalpha((unsigned char)a[i][0])) {
      for (int j = 0; a[i][j] && vc < 6; ++j) if (isalpha((unsigned char)a[i][j])) {
        char c = (char)toupper((unsigned char)a[i][j]);
        bool seen = false; for (int k = 0; k < vc; ++k) if (vars[k] == c) seen = true;
        if (!seen) { vars[vc++] = c; vars[vc] = 0; }
      }
    } else if (dcpart) {
      if (dc < 32) dcs[dc++] = (int)parse_int(a[i]);
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
  if (maxmode) {
    char pos[112] = ""; int pp = 0;
    int n = add(out, 0, "Maxterm/POS method: write 0-cells as sum factors.");
    char vl[32] = ""; int vp = 0;
    for (int i = 0; i < vc; ++i) { if (i) app_ch(vl, &vp, sizeof(vl), ','); app_ch(vl, &vp, sizeof(vl), vars[i]); }
    n = add(out, n, "variables: %s", vl);
    for (int i = 0; i < mc; ++i) {
      if (mins[i] < 0 || mins[i] >= rows) return add(out, n, "maxterm %d is outside 0 to %d.", mins[i], rows - 1);
      char term[32]; maxterm_expr(mins[i], vars, vc, term, sizeof(term));
      if (i) app_ch(pos, &pp, sizeof(pos), '&');
      app_str(pos, &pp, sizeof(pos), term);
      if (n < CSCALC_MAX_LINES - 6) n = add(out, n, "M%d = %s", mins[i], term);
    }
    n = add(out, n, "POS = %s", pos);
    if (dc) {
      char dl[80] = ""; int dp = 0;
      for (int i = 0; i < dc; ++i) {
        if (dcs[i] < 0 || dcs[i] >= rows) return add(out, n, "don't-care row %d is outside 0 to %d.", dcs[i], rows - 1);
        if (i) app_ch(dl, &dp, sizeof(dl), ',');
        app_int(dl, &dp, sizeof(dl), dcs[i]);
      }
      n = add(out, n, "don't-care rows: %s", dl);
      n = add(out, n, "Use don't-cares only if they make larger zero groups.");
      Imp chosen[32]; int chc = minimise_rows(mins, mc, dcs, dc, chosen, 32);
      char sim[96]; imp_pos_list(chosen, chc, vars, vc, sim, sizeof(sim));
      return add(out, n, "simplified POS = %s", chc ? sim : "1");
    }
    char cmd[128]; sprintf(cmd, "bool(%s)", pos);
    char tmp[CSCALC_MAX_LINES][CSCALC_LINE_LEN]; int tn = eval_bool(cmd, tmp);
    for (int i = 0; i < tn && n < CSCALC_MAX_LINES; ++i) {
      if (starts(tmp[i], "Make truth table")) continue;
      n = add(out, n, "%s", tmp[i]);
    }
    return n;
  }
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
  if (dc) {
    char dl[80] = ""; int dp = 0;
    for (int i = 0; i < dc; ++i) {
      if (dcs[i] < 0 || dcs[i] >= rows) return add(out, n, "don't-care row %d is outside 0 to %d.", dcs[i], rows - 1);
      if (int_seen(mins, mc, dcs[i])) return add(out, n, "row %d cannot be both 1 and don't-care.", dcs[i]);
      if (i) app_ch(dl, &dp, sizeof(dl), ',');
      app_int(dl, &dp, sizeof(dl), dcs[i]);
    }
    n = add(out, n, "don't-care rows: %s", dl);
    n = add(out, n, "Use don't-cares only if they make larger groups.");
    Imp chosen[32]; int chc = minimise_rows(mins, mc, dcs, dc, chosen, 32);
    char sim[96]; imp_sop_list(chosen, chc, vars, vc, sim, sizeof(sim));
    return add(out, n, "simplified = %s", chc ? sim : "0");
  }
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
  if (x.op == '^') {
    gate_app(b, p, cap, "(");
    gate_nand(n, x.l, b, p, cap);
    gate_app(b, p, cap, " NAND (");
    gate_nand(n, x.l, b, p, cap);
    gate_app(b, p, cap, " NAND ");
    gate_nand(n, x.r, b, p, cap);
    gate_app(b, p, cap, ")) NAND (");
    gate_nand(n, x.r, b, p, cap);
    gate_app(b, p, cap, " NAND (");
    gate_nand(n, x.l, b, p, cap);
    gate_app(b, p, cap, " NAND ");
    gate_nand(n, x.r, b, p, cap);
    gate_app(b, p, cap, "))");
    return;
  }
  gate_nand_not(n, id, b, p, cap);
}

static void gate_nor(GNode *n, int id, char *b, int *p, int cap) {
  GNode &x = n[id];
  if (x.op == 'v') { char t[2] = { x.v, 0 }; gate_app(b, p, cap, t); return; }
  if (x.op == '!') { gate_nor_not(n, x.l, b, p, cap); return; }
  if (x.op == '#') { gate_app(b, p, cap, "("); gate_nor(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '+') { gate_app(b, p, cap, "("); gate_nor(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, x.r, b, p, cap); gate_app(b, p, cap, ") NOR ("); gate_nor(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '&') { gate_app(b, p, cap, "("); gate_nor_not(n, x.l, b, p, cap); gate_app(b, p, cap, " NOR "); gate_nor_not(n, x.r, b, p, cap); gate_app(b, p, cap, ")"); return; }
  if (x.op == '^') {
    gate_app(b, p, cap, "(");
    gate_nor(n, x.l, b, p, cap);
    gate_app(b, p, cap, " NOR ");
    gate_nor(n, x.r, b, p, cap);
    gate_app(b, p, cap, ") NOR ((");
    gate_nor(n, x.l, b, p, cap);
    gate_app(b, p, cap, " NOR ");
    gate_nor(n, x.l, b, p, cap);
    gate_app(b, p, cap, ") NOR (");
    gate_nor(n, x.r, b, p, cap);
    gate_app(b, p, cap, " NOR ");
    gate_nor(n, x.r, b, p, cap);
    gate_app(b, p, cap, "))");
    return;
  }
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
  char lsim[80], rsim[80];
  bool_simplified_from_rows(ml, lc, rows, vars, vc, lsim, sizeof(lsim));
  bool_simplified_from_rows(mr, rc, rows, vars, vc, rsim, sizeof(rsim));
  int n = add(out, 0, "Simplify both sides, then compare output rows.");
  n = add_named_bool_law_trace(out, n, "LHS", lbuf);
  n = add_named_bool_law_trace(out, n, "RHS", rbuf);
  n = add(out, n, "LHS simplifies to %s", lsim);
  n = add(out, n, "RHS simplifies to %s", rsim);
  n = add(out, n, "Make truth tables for LHS and RHS.");
  n = add(out, n, "LHS output-1 rows: %s", lc ? ls : "none");
  n = add(out, n, "RHS output-1 rows: %s", rc ? rs : "none");
  return add(out, n, same_rows ? "Same output rows, so LHS = RHS." : "Different output rows, so not identical.");
}

static const char *skip_bool_words(const char *e) {
  bool moved = true;
  while (moved) {
    moved = false;
    if (starts(e, "simplify")) { e += 8; moved = true; }
    if (starts(e, "draw")) { e += 4; moved = true; }
    if (starts(e, "make")) { e += 4; moved = true; }
    if (starts(e, "create")) { e += 6; moved = true; }
    if (starts(e, "construct")) { e += 9; moved = true; }
    if (starts(e, "atruthtable") || starts(e, "aboolean") || starts(e, "anexpression")) { e += 1; moved = true; }
    if (starts(e, "write")) { e += 5; moved = true; }
    if (starts(e, "use")) { e += 3; moved = true; }
    if (starts(e, "to")) { e += 2; moved = true; }
    if (starts(e, "the")) { e += 3; moved = true; }
    if (starts(e, "using")) { e += 5; moved = true; }
    if (starts(e, "algebra")) { e += 7; moved = true; }
    if (starts(e, "prove")) { e += 5; moved = true; }
    if (starts(e, "bydemorgan")) { e += 10; moved = true; }
    if (starts(e, "demorgans")) { e += 9; moved = true; }
    if (starts(e, "demorgan")) { e += 8; moved = true; }
    if (starts(e, "showthat")) { e += 8; moved = true; }
    if (starts(e, "show")) { e += 4; moved = true; }
    if (starts(e, "that")) { e += 4; moved = true; }
    if (starts(e, "identity")) { e += 8; moved = true; }
    if (starts(e, "boolean")) { e += 7; moved = true; }
    if (starts(e, "expression")) { e += 10; moved = true; }
    if (starts(e, "logic")) { e += 5; moved = true; }
    if (starts(e, "productofsums")) { e += 13; moved = true; }
    if (starts(e, "posform")) { e += 7; moved = true; }
    if (starts(e, "cnf")) { e += 3; moved = true; }
    if (starts(e, "pos")) { e += 3; moved = true; }
    if (starts(e, "form")) { e += 4; moved = true; }
    if (starts(e, "truthtable")) { e += 10; moved = true; }
    if (starts(e, "build")) { e += 5; moved = true; }
    if (starts(e, "truth")) { e += 5; moved = true; }
    if (starts(e, "table")) { e += 5; moved = true; }
    if (starts(e, "output")) { e += 6; moved = true; }
    if (starts(e, "expression")) { e += 10; moved = true; }
    if (starts(e, "for")) { e += 3; moved = true; }
  }
  return e;
}

static void bool_arg_for_cmd(const char *src, char *dst, int cap) {
  int p = 0;
  for (int i = 0; src[i] && p + 1 < cap;) {
    if (starts(src + i, "nand")) { dst[p++] = '@'; i += 4; continue; }
    if (starts(src + i, "nor")) { dst[p++] = '#'; i += 3; continue; }
    if (starts(src + i, "xor")) { dst[p++] = '^'; i += 3; continue; }
    if (starts(src + i, "not")) { dst[p++] = '!'; i += 3; continue; }
    if (starts(src + i, "and")) { dst[p++] = '&'; i += 3; continue; }
    if (starts(src + i, "or")) { dst[p++] = '+'; i += 2; continue; }
    if ((src[i] == '.' || src[i] == '?' || src[i] == '!') && src[i+1] == 0) { ++i; continue; }
    dst[p++] = src[i] == ',' ? '\'' : src[i];
    ++i;
  }
  dst[p] = 0;
}

static void bool_clean_tail(const char *src, char *dst, int cap) {
  strncpy(dst, src, cap - 1); dst[cap - 1] = 0;
  const char *cut[] = {"using", "bydemorgan", "demorgan", "demorgans", "law", "method"};
  for (int i = 0; i < 6; ++i) {
    char *p = strstr(dst, cut[i]);
    if (p) *p = 0;
  }
  int n = (int)strlen(dst);
  while (n > 0 && (dst[n-1] == '.' || dst[n-1] == '?' || dst[n-1] == '!')) dst[--n] = 0;
}

static void truthbits_cmd_from_text(const char *t, const char *bits, char *cmd, int cap) {
  char vars[8] = ""; int vc = 0;
  const char *p = strstr(t, "variables");
  if (!p) p = strstr(t, "vars");
  if (p) {
    while (*p && *p != ',') ++p;
    while (*p && vc < 6) {
      while (*p == ',') ++p;
      if (isalpha((unsigned char)*p) && (p[1] == 0 || p[1] == ',')) {
        char c = (char)toupper((unsigned char)*p);
        if (!strchr(vars, c)) { vars[vc++] = c; vars[vc] = 0; }
      }
      while (*p && *p != ',') ++p;
    }
  }
  int outp = sprintf(cmd, "truthbits(");
  for (int i = 0; i < vc && outp < cap - 8; ++i) outp += sprintf(cmd + outp, "%c,", vars[i]);
  sprintf(cmd + outp, "%s)", bits);
}

static bool cidr_prefix_from_text(const char *input, int *prefix) {
  const char *p = strchr(input, '/');
  if (!p || !isdigit((unsigned char)p[1])) return false;
  int v = 0; ++p;
  while (isdigit((unsigned char)*p)) v = v*10 + (*p++ - '0');
  if (v < 0 || v > 32) return false;
  *prefix = v;
  return true;
}

static bool ipv4_from_text(const char *input, unsigned long *addr, int oct[4]) {
  for (int i = 0; input && input[i]; ++i) {
    if (!isdigit((unsigned char)input[i])) continue;
    int a=-1,b=-1,c=-1,d=-1,n=0;
    if (sscanf(input + i, "%d.%d.%d.%d%n", &a, &b, &c, &d, &n) == 4 &&
        a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 && d >= 0 && d <= 255) {
      oct[0]=a; oct[1]=b; oct[2]=c; oct[3]=d;
      *addr = ((unsigned long)a << 24) | ((unsigned long)b << 16) | ((unsigned long)c << 8) | (unsigned long)d;
      return true;
    }
  }
  return false;
}

static bool make_gate_form_cmd(const char *input, bool nand, char *cmd, int cap) {
  char expr[96] = ""; int p = 0;
  for (int i = 0; input[i] && p + 1 < (int)sizeof(expr);) {
    unsigned char c = (unsigned char)input[i];
    if (isalpha(c)) {
      char w[24]; int j = 0;
      while (isalpha((unsigned char)input[i]) && j + 1 < (int)sizeof(w)) w[j++] = (char)tolower((unsigned char)input[i++]);
      w[j] = 0;
      if (word_is(w, "or")) { expr[p++] = '+'; continue; }
      if (word_is(w, "and")) { expr[p++] = '*'; continue; }
      if (word_is(w, "not")) { expr[p++] = '!'; continue; }
      if (word_is(w, "find") || word_is(w, "the") || word_is(w, "write") ||
          word_is(w, "convert") || word_is(w, "to") || word_is(w, "using") || word_is(w, "use") ||
          word_is(w, "only") || word_is(w, "form") || word_is(w, "for") || word_is(w, "expression") ||
          word_is(w, "gate") || word_is(w, "gates") ||
          word_is(w, "boolean") || word_is(w, "logic") || word_is(w, nand ? "nand" : "nor")) continue;
      for (int k = 0; w[k] && p + 1 < (int)sizeof(expr); ++k) expr[p++] = w[k];
    } else {
      if (input[i] == '+' || input[i] == '*' || input[i] == '&' || input[i] == '|' || input[i] == '\'' ||
          input[i] == ',' || input[i] == '(' || input[i] == ')') expr[p++] = input[i] == ',' ? '\'' : input[i];
      ++i;
    }
  }
  expr[p] = 0;
  if (!expr[0]) return false;
  if (cap < 12) return false;
  sprintf(cmd, "%sform(%s)", nand ? "nand" : "nor", expr);
  return true;
}

static bool make_named_bool_rhs_cmd(const char *compact, const char *fn, char *cmd, int cap) {
  const char *e = skip_bool_words(compact);
  const char *eq = strchr(e, '='); int elen = 1;
  const char *eq2 = strstr(e, "isequalto");
  if (eq2 && (!eq || eq2 < eq)) { eq = eq2; elen = 9; }
  eq2 = strstr(e, "isequivalentto");
  if (eq2 && (!eq || eq2 < eq)) { eq = eq2; elen = 14; }
  eq2 = strstr(e, "equals");
  if (eq2 && (!eq || eq2 < eq)) { eq = eq2; elen = 6; }
  if (!eq || eq <= e || !eq[elen]) return false;
  if (eq != e + 1 || !isalpha((unsigned char)e[0])) return false;
  char rhs[96], nrhs[96]; int ri = 0;
  for (const char *p = eq + elen; *p && ri + 1 < (int)sizeof(rhs); ++p) rhs[ri++] = *p;
  rhs[ri] = 0;
  char clean[96];
  bool_clean_tail(rhs, clean, sizeof(clean));
  bool_arg_for_cmd(clean, nrhs, sizeof(nrhs));
  if (cap < (int)strlen(fn) + (int)strlen(nrhs) + 3) return false;
  sprintf(cmd, "%s(%s)", fn, nrhs);
  return true;
}

static int eval_free_text(const char *input, const char *compact, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char t[192]; raw_clean(input, t, sizeof(t));
  double v[8]; int nv = scan_nums(t, v, 8);
  char bits[4][48]; int nb = scan_bits(t, bits, 4);
  char fbits[4][48]; int nfb = scan_binary_fraction_tokens(input, fbits, 4);
  char fixed_tokens[4][48]; int nfixed = scan_fixed_bit_tokens(input, fixed_tokens, 4);
  char bitcol[65]; bool has_bitcol = scan_spaced_bit_column(t, bitcol, sizeof(bitcol));
  char bitgrp[4][48]; int nbg = scan_spaced_bit_groups(t, bitgrp, 4);
  char hex_tok[32]; bool has_hex_tok = scan_hex_token(t, hex_tok, sizeof(hex_tok));
  if (!has_hex_tok && (has(t, "hex") || has(t, "hexadecimal") || has(t, "base,16") || has(t, "base16"))) {
    has_hex_tok = scan_hex_word_token(t, hex_tok, sizeof(hex_tok));
  }
  char cmd[160];
  double width=0, height=0, depth=0;
  int prefix = 0;
  if (has(t, "fixed") && has(t, "binary") &&
      (has(t, "denary") || has(t, "decimal") || strstr(input, "denary") || strstr(input, "decimal")) &&
      (has(t, "before") || strstr(input, "before")) && (has(t, "after") || strstr(input, "after")) && nv >= 3) {
    bool early_tc = has(t, "twos") || (has(t, "two") && has(t, "complement"));
    sprintf(cmd, (early_tc || has(t, "signed") || has(t, "negative") || v[0] < 0) ?
            "fixedtcenc(%.10g,%lld,%lld)" : "fixedenc(%.10g,%lld,%lld)",
            v[0], (long long)v[nv-2], (long long)v[nv-1]);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && has(t, "binary") &&
      (has(t, "denary") || has(t, "decimal") || strstr(input, "denary") || strstr(input, "decimal")) &&
      !(has(t, "to,denary") || has(t, "to,decimal") || strstr(input, "to denary") || strstr(input, "to decimal")) &&
      (has(t, "fractional") || has(t, "fraction") || has(t, "after")) && nv >= 2) {
    sprintf(cmd, "fixedfrac(%.10g,%lld)", v[0], (long long)v[nv-1]);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && has(t, "binary") &&
      (has(t, "denary") || has(t, "decimal") || strstr(input, "denary") || strstr(input, "decimal")) &&
      !(has(t, "to,denary") || has(t, "to,decimal") || strstr(input, "to denary") || strstr(input, "to decimal")) &&
      !(has(t, "before") || has(t, "after") || has(t, "fractional") || has(t, "whole") || has(t, "integer")) && nv >= 1) {
    double value = v[0];
    long long whole = (long long)value;
    double rem = value - whole;
    if (rem < 0) rem = -rem;
    char whole_bits[65]; to_bin(whole, whole_bits_for(value), whole_bits);
    char frac_bits[33]; int fp = 0;
    int n = add(out, 0, "Convert the fractional part by repeated multiplication by 2.");
    n = add(out, n, "whole part %.0f gives %s", (double)whole, whole_bits);
    while (rem > 1e-10 && fp < 16 && n < CSCALC_MAX_LINES - 2) {
      rem *= 2.0;
      int bit = rem >= 1.0 ? 1 : 0;
      if (bit) rem -= 1.0;
      frac_bits[fp++] = (char)('0' + bit);
      n = add(out, n, "multiply by 2 -> bit %d, remainder %.10g", bit, rem);
    }
    frac_bits[fp] = 0;
    return add(out, n, "%.10g_10 = %s.%s_2", value, whole_bits, fp ? frac_bits : "0");
  }
  if (has(t, "rsa") && (has(t, "decrypt") || has(t, "plaintext")) && nv >= 3) {
    double cv=0, nv0=0, dv=0;
    bool hc = label_num(input, "ciphertext", &cv) || label_num(input, "c", &cv) || scan_after_word_num(t, "ciphertext", &cv);
    bool hn = label_num(input, "n", &nv0) || scan_after_word_num(t, "n", &nv0);
    bool hd = label_num(input, "d", &dv) || scan_after_word_num(t, "d", &dv);
    if (!hc) cv = v[0];
    if (!hn) nv0 = v[1];
    if (!hd) dv = v[2];
    long long ciph = (long long)cv, nval = (long long)nv0, d = (long long)dv;
    long long plain = nval ? mod_pow_ll(ciph, d, nval) : 0;
    int n = add(out, 0, "RSA decryption uses plaintext = ciphertext^d mod n.");
    n = add(out, n, "ciphertext = %lld, d = %lld, n = %lld", ciph, d, nval);
    return add(out, n, "%lld^%lld mod %lld = %lld", ciph, d, nval, plain);
  }
  if (has(t, "rsa") && nv >= 4) {
    double pd=0, qd=0, ed=0, md=0;
    bool hp = label_num(input, "p", &pd);
    bool hq = label_num(input, "q", &qd);
    bool he = label_num(input, "e", &ed);
    bool hm = label_num(input, "message", &md) || label_num(input, "plaintext", &md) || label_num(input, "m", &md);
    if (!hp) pd = v[0];
    if (!hq) qd = v[1];
    if (!he) ed = v[2];
    if (!hm) md = v[3];
    long long p = (long long)pd, q = (long long)qd, e = (long long)ed, msg = (long long)md;
    long long nval = p * q, phi = (p - 1) * (q - 1);
    long long d = mod_inverse_small(e, phi);
    long long ciph = nval ? mod_pow_ll(msg, e, nval) : 0;
    int n = add(out, 0, "RSA modulus n = p*q.");
    n = add(out, n, "n = %lld*%lld = %lld", p, q, nval);
    n = add(out, n, "phi(n) = (p-1)(q-1) = %lld", phi);
    n = add(out, n, "private key d satisfies ed = 1 mod phi(n).");
    n = add(out, n, "d = %lld", d);
    n = add(out, n, "ciphertext = message^e mod n");
    return add(out, n, "%lld^%lld mod %lld = %lld", msg, e, nval, ciph);
  }
  if ((has(t, "largest") || has(t, "smallest") || has(t, "compare")) &&
      (has(t, "binary") || has(t, "base,2") || has(t, "base2")) &&
      (has(t, "hex") || has(t, "hexadecimal") || has(t, "base,16") || has(t, "base16")) &&
      (has(t, "decimal") || has(t, "denary")) && nb >= 1 && has_hex_tok && nv >= 1) {
    double dec = v[nv - 1];
    scan_after_word_num(t, "decimal", &dec);
    scan_after_word_num(t, "denary", &dec);
    long long bv = bin_unsigned(bits[0]), hv = parse_base(hex_tok, 16), dv = (long long)dec;
    long long best = bv; const char *best_name = "binary";
    if ((has(t, "largest") && hv > best) || (has(t, "smallest") && hv < best)) { best = hv; best_name = "hexadecimal"; }
    if ((has(t, "largest") && dv > best) || (has(t, "smallest") && dv < best)) { best = dv; best_name = "decimal"; }
    int n = add(out, 0, "Convert all values to denary before comparing.");
    n = add(out, n, "%s_2 = %lld_10", bits[0], bv);
    n = add(out, n, "%s_16 = %lld_10", hex_tok, hv);
    n = add(out, n, "%lld_10 = %lld_10", dv, dv);
    return add(out, n, "%s value is %s = %lld", has(t, "smallest") ? "smallest" : "largest", best_name, best);
  }
  if ((has(t, "float") || has(t, "floating") || has(t, "normalised") || has(t, "normalized")) && nb >= 4 &&
      (has(t, "add") || has(t, "sum") || has(t, "plus") || has(t, "subtract") || has(t, "minus") ||
       has(t, "multiply") || has(t, "times") || has(t, "product") || has(t, "divide"))) {
    const char *op = "floatadd";
    if (has(t, "subtract") || has(t, "minus")) op = "floatsub";
    else if (has(t, "multiply") || has(t, "times") || has(t, "product")) op = "floatmul";
    else if (has(t, "divide")) op = "floatdiv";
    if (nfb >= 2) sprintf(cmd, "%s(%s,%s,%s,%s)", op, fbits[0], bits[1], fbits[1], bits[3]);
    else sprintf(cmd, "%s(%s,%s,%s,%s)", op, bits[0], bits[1], bits[2], bits[3]);
    return eval_float(cmd, out);
  }
  if ((has(t, "subnet") || has(t, "ipv4") || has(t, "cidr") || strchr(input, '/')) && cidr_prefix_from_text(input, &prefix)) {
    unsigned long ip = 0; int oct[4];
    if ((has(t, "network") || has(t, "broadcast")) && ipv4_from_text(input, &ip, oct)) {
      unsigned long mask = prefix == 0 ? 0UL : (0xffffffffUL << (32 - prefix)) & 0xffffffffUL;
      unsigned long net = ip & mask;
      unsigned long bcast = net | (~mask & 0xffffffffUL);
      int n = add(out, 0, "CIDR /%d gives subnet mask %lu.%lu.%lu.%lu.", prefix, (mask>>24)&255, (mask>>16)&255, (mask>>8)&255, mask&255);
      n = add(out, n, "IP address = %d.%d.%d.%d", oct[0], oct[1], oct[2], oct[3]);
      n = add(out, n, "network address = IP AND mask");
      n = add(out, n, "%lu.%lu.%lu.%lu", (net>>24)&255, (net>>16)&255, (net>>8)&255, net&255);
      return add(out, n, "broadcast address = %lu.%lu.%lu.%lu", (bcast>>24)&255, (bcast>>16)&255, (bcast>>8)&255, bcast&255);
    }
    if (has(t, "mask")) {
      unsigned long mask = prefix == 0 ? 0UL : (0xffffffffUL << (32 - prefix)) & 0xffffffffUL;
      int n = add(out, 0, "CIDR /%d means %d network bits and %d host bits.", prefix, prefix, 32-prefix);
      n = add(out, n, "subnet mask = %lu.%lu.%lu.%lu", (mask>>24)&255, (mask>>16)&255, (mask>>8)&255, mask&255);
      if (has(t, "host")) {
        int host_bits = 32 - prefix;
        double total = pow2(host_bits), usable = host_bits >= 2 ? total - 2 : total;
        n = add(out, n, "host bits = %d, usable hosts = 2^%d - 2", host_bits, host_bits);
        return add(out, n, "usable host addresses = %.10g", usable);
      }
      return n;
    }
    int host_bits = 32 - prefix;
    double total = pow2(host_bits), usable = host_bits >= 2 ? total - 2 : total;
    int n = add(out, 0, "CIDR /%d leaves %d host bits.", prefix, host_bits);
    n = add(out, n, "total addresses = 2^%d = %.10g", host_bits, total);
    return add(out, n, "usable host addresses = %.10g", usable);
  }
  if ((has(t, "subnet") || has(t, "host")) && (has(t, "hostbits") || (has(t, "host") && has(t, "bits")) ||
      has(t, "hosts")) && (has(t, "needed") || has(t, "need") || has(t, "atleast") || has(t, "at,least") || has(t, "minimum")) && nv >= 1) {
    double hosts = v[0];
    int h = 0;
    while (h < 32 && pow2(h) - 2 < hosts) ++h;
    int n = add(out, 0, "For IPv4 subnet hosts, usable hosts = 2^h - 2.");
    n = add(out, n, "Need 2^h - 2 >= %.10g", hosts);
    n = add(out, n, "h = %d host bits gives %.10g usable hosts", h, pow2(h) - 2);
    return add(out, n, "CIDR prefix would be /%d", 32 - h);
  }
  if (has(t, "littleendian") || (has(t, "little") && has(t, "endian")) ||
      has(t, "bigendian") || (has(t, "big") && has(t, "endian"))) {
    char endian_hex[32] = "";
    if (has_hex_tok) strncpy(endian_hex, hex_tok, sizeof(endian_hex)-1);
    if (!endian_hex[0]) {
      const char *hp = strstr(t, "hexadecimal,");
      if (!hp) hp = strstr(t, "hex,");
      if (hp) {
        hp = strchr(hp, ',');
        if (hp) {
          ++hp; int j = 0;
          while (isxdigit((unsigned char)hp[j]) && j + 1 < (int)sizeof(endian_hex)) {
            endian_hex[j] = hp[j]; ++j;
          }
          endian_hex[j] = 0;
        }
      }
    }
    if (!endian_hex[0]) {
      for (int i = 0; input[i] && !endian_hex[0];) {
        while (input[i] && !isxdigit((unsigned char)input[i])) ++i;
        int j = 0; char tok[32];
        while (isxdigit((unsigned char)input[i]) && j + 1 < (int)sizeof(tok)) tok[j++] = input[i++];
        tok[j] = 0;
        if (j >= 4 && !strstr("abcdefABCDEF", tok)) strncpy(endian_hex, tok, sizeof(endian_hex)-1);
      }
    }
    if (!endian_hex[0]) return add(out, 0, "Give the hexadecimal bytes to order.");
    char bytes[48] = ""; int len = (int)strlen(endian_hex), bp = 0;
    if (len % 2 && bp + 2 < (int)sizeof(bytes)) { bytes[bp++] = '0'; bytes[bp++] = (char)toupper((unsigned char)endian_hex[0]); }
    for (int i = len % 2; i + 1 < len && bp + 3 < (int)sizeof(bytes); i += 2) {
      if (bp) bytes[bp++] = ' ';
      bytes[bp++] = (char)toupper((unsigned char)endian_hex[i]);
      bytes[bp++] = (char)toupper((unsigned char)endian_hex[i+1]);
    }
    bytes[bp] = 0;
    int n = add(out, 0, "Split the hexadecimal value into bytes.");
    n = add(out, n, "%s -> %s", endian_hex, bytes);
    if (has(t, "little")) {
      char rev[48] = ""; int rp = 0;
      for (int i = bp - 2; i >= 0; i -= 3) {
        if (rp) rev[rp++] = ' ';
        rev[rp++] = bytes[i]; rev[rp++] = bytes[i+1];
      }
      rev[rp] = 0;
      return add(out, n, "little-endian order = %s", rev);
    }
    return add(out, n, "big-endian order = %s", bytes);
  }
  if ((has(t, "addressbus") || (has(t, "address") && has(t, "bus"))) &&
      (has(t, "databus") || (has(t, "data") && has(t, "bus"))) && nv >= 2) {
    double ab=0, db=0, locations=0;
    bool ha = scan_before_word_num(t, "address", &ab) || scan_before_word_num(t, "addressbus", &ab);
    bool hd = scan_before_word_num(t, "data", &db) || scan_before_word_num(t, "databus", &db);
    if (!ha) ab = v[0];
    if (!hd) db = v[1];
    locations = pow2((int)ab);
    if (nv >= 3) {
      for (int i = 0; i < nv; ++i) if (v[i] > locations / 2 && v[i] <= locations * 2) { locations = v[i]; break; }
    }
    double bits = locations * db, bytes = bits / 8.0;
    int n = add(out, 0, "Memory size = number of addresses * data bus width.");
    n = add(out, n, "address bus %.0f bits gives 2^%.0f = %.10g addresses", ab, ab, pow2((int)ab));
    n = add(out, n, "data bus width = %.10g bits", db);
    n = add(out, n, "memory size = %.10g*%.10g = %.10g bits", locations, db, bits);
    return add(out, n, "= %.10g bytes", bytes);
  }
  if (has(t, "cache") && has(t, "block") && nv >= 2) {
    double cache = 0, block = 0, ways = 1, addr = 0;
    bool hc = scan_before_word_num(t, "kib", &cache) || scan_before_word_num(t, "kb", &cache) ||
              scan_before_word_num(t, "bytes", &cache);
    bool hb = scan_before_word_num(t, "block", &block) || scan_before_word_num(t, "bytes", &block);
    bool hw = scan_before_word_num(t, "way", &ways) || scan_before_word_num(t, "associativity", &ways);
    bool ha = scan_before_word_num(t, "bitaddress", &addr) || scan_before_word_num(t, "address", &addr);
    if (!hc) cache = v[0];
    if (!hb) block = v[1];
    if (!hw) {
      for (int i = 0; i < nv; ++i) {
        double dc = v[i] > cache ? v[i] - cache : cache - v[i];
        double db = v[i] > block ? v[i] - block : block - v[i];
        if (dc > 1e-9 && db > 1e-9 && v[i] > 1 && v[i] <= 64) { ways = v[i]; break; }
      }
    }
    if (!ha && has(t, "address")) {
      for (int i = nv - 1; i >= 0; --i) {
        double db = v[i] > block ? v[i] - block : block - v[i];
        double dw = v[i] > ways ? v[i] - ways : ways - v[i];
        if (db > 1e-9 && dw > 1e-9 && v[i] >= 8 && v[i] <= 64) { addr = v[i]; ha = true; break; }
      }
    }
    if (has(t, "kib") || has(t, "kb")) cache *= 1024.0;
    double blocks = block ? cache / block : 0;
    double sets = ways ? blocks / ways : blocks;
    int off = ceil_log2_ll((long long)(block + 0.5));
    int idx = ceil_log2_ll((long long)(sets + 0.5));
    int n = add(out, 0, "Cache address fields use powers of 2.");
    n = add(out, n, "cache size = %.10g bytes, block size = %.10g bytes", cache, block);
    n = add(out, n, "number of blocks = %.10g/%.10g = %.10g", cache, block, blocks);
    if (ways > 1) n = add(out, n, "sets = blocks/ways = %.10g/%.10g = %.10g", blocks, ways, sets);
    n = add(out, n, "offset bits = log2(%.10g) = %d", block, off);
    n = add(out, n, "index bits = log2(%.10g) = %d", sets, idx);
    if (ha && addr > off + idx) return add(out, n, "tag bits = %.0f - %d - %d = %.0f", addr, idx, off, addr - idx - off);
    return n;
  }
  if ((has(t, "average") && has(t, "memory") && has(t, "access") && has(t, "time")) || has(t, "amat")) {
    if (nv >= 3) {
      double hit = v[0], cachet = v[1], maint = v[2];
      if (hit > 1) hit /= 100.0;
      double miss = 1.0 - hit, ans = hit*cachet + miss*maint;
      int n = add(out, 0, "Average access time = hit part + miss part.");
      n = add(out, n, "miss rate = 1 - %.10g = %.10g", hit, miss);
      n = add(out, n, "AMAT = %.10g*%.10g + %.10g*%.10g", hit, cachet, miss, maint);
      return add(out, n, "= %.10g ns", ans);
    }
  }
  if (has(t, "adc") && (has(t, "resolution") || has(t, "volts") || has(t, "voltage")) && nv >= 2) {
    double bitsw=0; bool hb = scan_before_word_num(t, "bit", &bitsw) || scan_before_word_num(t, "bits", &bitsw);
    if (!hb) bitsw = v[0];
    double lo = 0, hi = 0;
    double d0 = v[0] > bitsw ? v[0] - bitsw : bitsw - v[0];
    if (nv >= 3 && d0 < 1e-9) { lo = v[1]; hi = v[2]; }
    else { hi = v[nv-1]; }
    double levels = pow2((int)bitsw), res = (hi - lo) / levels;
    int n = add(out, 0, "ADC resolution = input voltage range / number of levels.");
    n = add(out, n, "number of levels = 2^%d = %.10g", (int)bitsw, levels);
    n = add(out, n, "voltage range = %.10g - %.10g = %.10g V", hi, lo, hi-lo);
    return add(out, n, "resolution = %.10g/%.10g = %.10g V", hi-lo, levels, res);
  }
  if ((has(t, "decode") || has(t, "decompress")) && (has(t, "rle") || has(t, "runlength") || (has(t, "run") && has(t, "length")))) {
    char enc[48];
    if (find_rle_encoded_sequence(input, enc, sizeof(enc))) {
      char dec[72] = ""; int dp = 0, n = add(out, 0, "Decode RLE by repeating each symbol by its count.");
      for (int i = 0; enc[i] && n < CSCALC_MAX_LINES - 1;) {
        int count = 0;
        while (isdigit((unsigned char)enc[i])) count = count*10 + (enc[i++] - '0');
        char sym = enc[i++];
        n = add(out, n, "%d%c -> repeat %c %d times", count, sym, sym, count);
        while (count-- > 0 && dp + 1 < (int)sizeof(dec)) dec[dp++] = sym;
      }
      dec[dp] = 0;
      return add(out, n, "decoded string = %s", dec);
    }
  }
  if (has(t, "hamming") && has(t, "parity") && (has(t, "bits") || has(t, "bit")) &&
      (has(t, "required") || has(t, "needed") || has(t, "need") || has(t, "howmany") || has(t, "how,many")) && nv >= 1) {
    int data = (int)v[0], r = 0;
    while (r < 16 && pow2(r) < data + r + 1) ++r;
    int n = add(out, 0, "For Hamming code parity bits, choose r so 2^r >= data bits + r + 1.");
    n = add(out, n, "Need 2^r >= %d + r + 1", data);
    return add(out, n, "r = %d parity bits", r);
  }
  if ((has(t, "ascii") || has(t, "unicode")) &&
      (has(t, "storage") || has(t, "store") || has(t, "size") || has(t, "encoded") || has(t, "characters") || has(t, "text")) &&
      nv >= 1) {
    int bpc = has(t, "unicode") ? 16 : 8;
    long long chars = (long long)v[0];
    int slen = alpha_token_len_after(t, "string");
    if (has(t, "bit") && nv >= 2 && v[0] > 0 && v[0] <= 64) {
      bpc = (int)v[0];
      chars = (long long)v[1];
    }
    if (slen > 0) chars = slen;
    sprintf(cmd, "chars(%lld,%d)", chars, bpc);
    return eval_storage(cmd, out);
  }
  if (has(t, "ascii") || has(t, "unicode") || has(t, "codepoint") || has(t, "charactercode")) {
    int ch = -1;
    bool uni = has(t, "unicode") || has(t, "codepoint");
    if (find_code_char(input, &ch)) return add_code_lines(out, ch, uni, ch);
    if (nv >= 1) return add_code_lines(out, (int)v[0], uni, -1);
  }
  if (has(t, "rpn") || has(t, "postfix") || (has(t, "reverse") && has(t, "polish"))) {
    if (make_rpn_cmd(input, cmd, sizeof(cmd))) return eval_rpn(cmd, out);
  }
  if (has(compact, "bigo") || has(compact, "big-o") || has(compact, "complexity") || has(compact, "timecomplexity")) {
    int n = add(out, 0, "Count how the number of operations grows with input size n.");
    if ((has(compact, "nested") || has(compact, "inner") || has(compact, "fori")) && has(compact, "loop") &&
        (has(compact, "from1ton") || has(compact, "1ton") || has(compact, "n")) && has(compact, "j")) {
      n = add(out, n, "Outer loop runs n times.");
      n = add(out, n, "Inner loop runs n times for each outer iteration.");
      n = add(out, n, "total operations proportional to n*n = n^2");
      return add(out, n, "Big O = O(n^2)");
    }
    if (has(compact, "binarysearch") || (has(compact, "halve") || has(compact, "halves") || has(compact, "divideandconquer"))) {
      n = add(out, n, "Binary search halves the search space each comparison.");
      return add(out, n, "Big O = O(log n)");
    }
    if (has(compact, "single") && has(compact, "loop")) {
      n = add(out, n, "A single loop over n items grows linearly.");
      return add(out, n, "Big O = O(n)");
    }
    if (has(compact, "constant")) return add(out, n, "Big O = O(1)");
  }
  if (has(t, "dijkstra") || (has(t, "shortest") && (has(t, "path") || has(t, "route")))) {
    if (make_dijkstra_cmd(input, cmd, sizeof(cmd))) return eval_trace(cmd, out);
  }
  if (has(t, "caesar") && (has(t, "encrypt") || has(t, "decrypt")) && nv >= 1) {
    char word[40] = "";
    for (int i = 0; input[i];) {
      while (input[i] && !isalpha((unsigned char)input[i])) ++i;
      char tmp[40]; int j = 0;
      while (isalpha((unsigned char)input[i]) && j + 1 < (int)sizeof(tmp)) tmp[j++] = (char)tolower((unsigned char)input[i++]);
      tmp[j] = 0;
      if (j > 1 && !word_is(tmp, "caesar") && !word_is(tmp, "shift") && !word_is(tmp, "encrypt") &&
          !word_is(tmp, "decrypt") && !word_is(tmp, "using") && !word_is(tmp, "with") && !word_is(tmp, "the")) strcpy(word, tmp);
    }
    if (word[0]) {
      int sh = (int)v[0] % 26; if (has(t, "decrypt")) sh = -sh;
      char enc[40]; int i = 0;
      for (; word[i] && i + 1 < (int)sizeof(enc); ++i) enc[i] = (char)('A' + (word[i] - 'a' + sh + 26) % 26);
      enc[i] = 0;
      int n = add(out, 0, "Caesar shift moves each letter by %d places.", sh < 0 ? -sh : sh);
      n = add(out, n, "%s -> %s", word, enc);
      return add(out, n, "%s text = %s", has(t, "decrypt") ? "decrypted" : "encrypted", enc);
    }
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
  if ((has(t, "infix") || has(t, "expression")) &&
      (has(t, "postfix") || has(t, "reversepolish") || (has(t, "reverse") && has(t, "polish")))) {
    int n = add_infix_postfix_lines(input, out);
    if (n) return n;
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
  if ((has(t, "image") || has(t, "bitmap")) && (has(t, "megapixel") || has(t, "megapixels")) && nv >= 2) {
    double pixels = v[0] * 1000000.0, depth = v[1];
    int colour_depth = 0;
    if (has(t, "colours") || has(t, "colors")) { double cv = scaled_colour_count(t, v[1]); colour_depth = ceil_log2_ll((long long)cv); depth = colour_depth; }
    if (image_depth_is_bytes(t)) depth *= 8.0;
    double bits_total = pixels * depth, bytes = bits_total / 8.0;
    int n = add(out, 0, "Image bits = pixels * colour depth.");
    n = add(out, n, "%.10g megapixels = %.10g pixels", v[0], pixels);
    if (colour_depth) n = add(out, n, "ceil(log2(%.10g)) = %d bits per pixel", scaled_colour_count(t, v[1]), colour_depth);
    else if (image_depth_is_bytes(t)) n = add(out, n, "%.10g bytes per pixel = %.10g bits per pixel", v[1], depth);
    n = add(out, n, "%.10g*%.10g = %.10g bits", pixels, depth, bits_total);
    n = add(out, n, "= %.10g bytes", bytes);
    n = add(out, n, "= %.10g MB", bytes / 1000000.0);
    return add(out, n, "= %.10g MiB", bytes / 1048576.0);
  }
  if ((has(t, "image") || has(t, "bitmap")) && image_depth_is_bytes(t) && nv >= 3) {
    return add_image_byte_depth_lines(out, (long long)v[0], (long long)v[1], (long long)v[2]);
  }
  if ((has(t, "image") || has(t, "bitmap")) && (has(t, "colours") || has(t, "colors")) && nv >= 3) {
    double colours=0; bool hc = label_num(input, "colours", &colours) || label_num(input, "colors", &colours) ||
                                scan_scaled_before_word_num(t, "colours", &colours) || scan_scaled_before_word_num(t, "colors", &colours);
    if (hc) {
      double wh[2]; int nw = 0;
      for (int i = 0; i < nv && nw < 2; ++i) if ((long long)v[i] != (long long)colours) wh[nw++] = v[i];
      if (nw == 2) {
        sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)wh[0], (long long)wh[1], (long long)colours);
        return eval_storage(cmd, out);
      }
    }
    sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)scaled_colour_count(t, v[2])); return eval_storage(cmd, out);
  }
  if ((has(t, "image") || has(t, "bitmap")) && label_num(input,"width",&width) && label_num(input,"height",&height) && (label_num(input,"depth",&depth) || label_num(input,"bits",&depth))) {
    if (image_depth_is_bytes(t)) return add_image_byte_depth_lines(out, (long long)width, (long long)height, (long long)depth);
    sprintf(cmd, "image(%lld,%lld,%lld)", (long long)width, (long long)height, (long long)depth); return eval_storage(cmd, out);
  }
  if ((has(t, "image") || has(t, "bitmap")) && label_num(input,"width",&width) && label_num(input,"height",&height) && (label_num(input,"colours",&depth) || label_num(input,"colors",&depth))) {
    depth = scaled_colour_count(t, depth);
    sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)width, (long long)height, (long long)depth); return eval_storage(cmd, out);
  }
  if ((has(t, "image") || has(t, "bitmap")) && nv >= 3) {
    if (has(t, "colour") || has(t, "color")) {
      if (has(t, "colourdepth") || has(t, "colordepth") || has(t, "depth") ||
          has(t, "bitcolour") || has(t, "bitcolor") || has(t, "bits") || has_word(t, "bit")) {
        sprintf(cmd, "image(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
      }
      sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)scaled_colour_count(t, v[2])); return eval_storage(cmd, out);
    }
  }
  if ((has(t, "symbol") || has(t, "symbols")) && (has(t, "bits") || has(t, "bit")) &&
      (has(t, "needed") || has(t, "need") || has(t, "represent") || has(t, "howmany") || has(t, "how,many")) && nv >= 1) {
    if ((has(t, "character") || has(t, "characters") || has(t, "text")) &&
        (has(t, "characterset") || has(t, "character,set")) && nv >= 2) {
      sprintf(cmd, "charset(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "symbolbits(%lld)", (long long)v[0]); return eval_storage(cmd, out);
  }
  if ((has(t, "colour") || has(t, "color")) &&
      (has(t, "needed") || has(t, "need") || has(t, "bits") || has(t, "bit") || has(t, "per,pixel") || has(t, "perpixel")) &&
      !(has(t, "howmanycolours") || has(t, "how,many,colours") || has(t, "howmanycolors") ||
        has(t, "how,many,colors") || has(t, "numberofcolours") || has(t, "numberofcolors")) &&
      nv >= 1 && !(has(t, "width") || has(t, "height") || has(t, "resolution"))) {
    double colours=0;
    if (!(label_num(input, "colours", &colours) || label_num(input, "colors", &colours) ||
          scan_scaled_before_word_num(t, "colours", &colours) || scan_scaled_before_word_num(t, "colors", &colours))) colours = scaled_colour_count(t, v[0]);
    sprintf(cmd, "colourdepth(%lld)", (long long)colours); return eval_storage(cmd, out);
  }
  if ((has(t, "colour") || has(t, "color")) && (has(t, "howmanycolours") || has(t, "how,many,colours") ||
      has(t, "howmanycolors") || has(t, "how,many,colors") || has(t, "numberofcolours") || has(t, "numberofcolors")) &&
      (has(t, "depth") || has(t, "bits") || has(t, "bit")) && nv >= 1) {
    sprintf(cmd, "colourcount(%lld)", (long long)v[0]); return eval_storage(cmd, out);
  }
  if ((has(t, "colour") || has(t, "color")) &&
      (has(t, "bitsperpixel") || has(t, "bitperpixel") || has(t, "bits,per,pixel") ||
       has(t, "bit,per,pixel") || has(t, "depth") || has(t, "represented")) && nv >= 1) {
    if ((has(t, "bits") && (has(t, "needed") || has(t, "need"))) || has(t, "bitsneeded") || has(t, "depth")) {
      sprintf(cmd, "colourdepth(%lld)", (long long)v[0]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "colourcount(%lld)", (long long)v[0]); return eval_storage(cmd, out);
  }
  if (has(t, "sound") || has(t, "audio")) {
    double rate=0, seconds=0, res=0, channels=1;
    double dur=0;
    bool wordRate = scan_before_word_num(t, "khz", &rate) || scan_before_word_num(t, "kilohertz", &rate) ||
                    scan_before_word_num(t, "hz", &rate) || scan_before_word_num(t, "hertz", &rate);
    bool wordDur = scan_before_word_num(t, "minutes", &dur) || scan_before_word_num(t, "minute", &dur) ||
                   scan_before_word_num(t, "mins", &dur) || scan_before_word_num(t, "seconds", &dur) ||
                   scan_before_word_num(t, "second", &dur) || scan_before_word_num(t, "hours", &dur) ||
                   scan_before_word_num(t, "hour", &dur);
    if (wordRate && wordDur) {
      double rawRate = rate;
      seconds = dur;
      scale_frequency_unit(t, &rate);
      scale_time_unit(t, &seconds);
      if (has(t, "number,of,samples") || has(t, "sample,count") || has(t, "how,many,samples")) {
        int n = add(out, 0, "Number of samples = sample rate * seconds.");
        n = add(out, n, "%.10g samples/s * %.10g s", rate, seconds);
        return add(out, n, "samples = %.10g", rate * seconds);
      }
      if (nv >= 3) {
        for (int i = 0; i < nv; ++i)
          if (v[i] != dur && v[i] != rawRate && v[i] > 1 && v[i] <= 64) { res = v[i]; break; }
        if (res <= 0) res = v[2];
        if (!(scan_before_word_num(t, "channels", &channels) || scan_before_word_num(t, "channel", &channels)))
          channels = has(t, "stereo") ? 2 : 1;
        sprintf(cmd, "sound(%lld,%lld,%lld,%lld)", (long long)rate, (long long)seconds, (long long)res, (long long)channels);
        return eval_storage(cmd, out);
      }
    }
    bool hR=label_num(input,"samplerate",&rate) || label_num(input,"rate",&rate) || label_num(input,"frequency",&rate);
    bool hS=label_num(input,"seconds",&seconds) || label_num(input,"duration",&seconds) || label_num(input,"time",&seconds);
    bool hB=label_num(input,"resolution",&res) || label_num(input,"depth",&res) || label_num(input,"bits",&res);
    bool hC=label_num(input,"channels",&channels) || label_num(input,"channel",&channels);
    if (!hC && has(t, "stereo")) { channels = 2; hC = true; }
    if (!hC && has(t, "mono")) { channels = 1; hC = true; }
    if (hR && hS && hB) {
      scale_frequency_unit(t, &rate);
      scale_time_unit(t, &seconds);
      sprintf(cmd, "sound(%lld,%lld,%lld,%lld)", (long long)rate, (long long)seconds, (long long)res, hC ? (long long)channels : 1);
      return eval_storage(cmd, out);
    }
    if (nv >= 3) {
      int ch = nv > 3 ? (int)v[3] : has(t, "stereo") ? 2 : 1;
      rate = v[0];
      seconds = v[1];
      scale_frequency_unit(t, &rate);
      scale_time_unit(t, &seconds);
      sprintf(cmd, "sound(%lld,%lld,%lld,%d)", (long long)rate, (long long)seconds, (long long)v[2], ch);
      return eval_storage(cmd, out);
    }
  }
  if ((has(compact, "bitrate") || has(compact, "datarate") || (has(t, "bit") && has(t, "rate")) || (has(t, "network") && (has(t, "sends") || has_word(t, "sent")))) &&
      (has(t, "file") || has(t, "size") || has(t, "transmit") || has_word(t, "sent") || has(t, "sends") || has(t, "network")) &&
      (has(t, "second") || has(t, "minute") || has(t, "hour") || has(t, "time") || has(t, "duration")) &&
      !has(compact, "downloadtime") && !has(compact, "transfertime") && nv >= 2) {
    double size=0, seconds=0;
    bool hSize=label_num(input,"size",&size) || label_num(input,"filesize",&size) || label_num(input,"file",&size);
    bool hSec=label_num(input,"seconds",&seconds) || label_num(input,"time",&seconds) || label_num(input,"duration",&seconds);
    if (!hSec) hSec = scan_before_word_num(t, "minutes", &seconds) || scan_before_word_num(t, "minute", &seconds) ||
                     scan_before_word_num(t, "hours", &seconds) || scan_before_word_num(t, "hour", &seconds) ||
                     scan_before_word_num(t, "seconds", &seconds) || scan_before_word_num(t, "second", &seconds);
    if (!hSize) size = v[0];
    if (!hSec) seconds = v[1];
    scale_time_unit(t, &seconds);
    if (has(t, "gib") || has(t, "mib") || has(t, "kib")) {
      int un = add_bitrate_unit_lines(out, size, seconds, t);
      if (un) return un;
    }
    if (has(t, "megabyte") || has(t, "mbyte") || has(t, "mb")) sprintf(cmd, "bitratemb(%.10g,%.10g)", size, seconds);
    else if (has(t, "kilobyte") || has(t, "kbyte") || has(t, "kb")) sprintf(cmd, "bitratekb(%.10g,%.10g)", size, seconds);
    else sprintf(cmd, "bitrate(%.10g,%.10g)", size, seconds);
    return eval_storage(cmd, out);
  }
  if ((has(t, "packet") || has(t, "packets")) && has(t, "payload") && has(t, "header") &&
      (has(t, "total") || has(t, "transmitted") || has(t, "transmit")) && nv >= 3) {
    double size = v[0], payload = 0, header = 0;
    bool hp = scan_before_word_num(t, "payload", &payload) || label_num(input, "payload", &payload);
    bool hh = scan_before_word_num(t, "header", &header) || label_num(input, "header", &header);
    if (!hp) payload = v[1];
    if (!hh) header = v[2];
    double bits = 0; const char *su = "";
    if (payload > 0 && storage_size_bits(t, size, &bits, &su)) {
      double bytes = bits / 8.0;
      long long packets = (long long)((bytes + payload - 1) / payload);
      double total = packets * (payload + header);
      int n = add(out, 0, "Total transmitted size includes payload plus header for each packet.");
      n = add(out, n, "%.10g %s = %.10g bytes", size, su, bytes);
      n = add(out, n, "number of packets = ceil(%.10g/%.10g) = %lld", bytes, payload, packets);
      n = add(out, n, "bytes per packet sent = payload + header = %.10g + %.10g = %.10g", payload, header, payload + header);
      n = add(out, n, "total transmitted = %lld*%.10g = %.10g bytes", packets, payload + header, total);
      return add(out, n, "= %.10g KiB", total / 1024.0);
    }
  }
  if ((has(t, "transfer") || has(t, "download") || has(t, "transmit") || has(t, "transmission") || has_word(t, "sent")) &&
      has(t, "overhead") && has(t, "percent") && nv >= 3) {
    double size = v[0], overhead = v[1], rate = v[2];
    double bits=0, bps=0; const char *su="", *ru="";
    if (storage_size_bits(t, size, &bits, &su) && storage_rate_bits(t, rate, &bps, &ru) && bps != 0) {
      double tx = bits * (1.0 + overhead / 100.0);
      double seconds = tx / bps;
      int n = add(out, 0, "Transmission time uses payload bits plus packet overhead.");
      n = add(out, n, "%.10g %s = %.10g bits", size, su, bits);
      n = add(out, n, "include %.10g%% overhead: transmitted bits = %.10g*%.10g = %.10g", overhead, bits, 1.0 + overhead/100.0, tx);
      n = add(out, n, "%.10g %s = %.10g bit/s", rate, ru, bps);
      n = add(out, n, "time = %.10g/%.10g = %.10g s", tx, bps, seconds);
      if (has(t, "minute")) return add(out, n, "= %.10g minutes", seconds / 60.0);
      return n;
    }
  }
  if ((has(t, "overhead") || (has(t, "header") && has(t, "payload"))) && has(t, "percent") && nv >= 2) {
    double payload=0, header=0;
    bool hp=scan_before_word_num(t, "payload", &payload) || label_num(input, "payload", &payload);
    bool hh=scan_before_word_num(t, "header", &header) || label_num(input, "header", &header);
    if (!hp) payload = v[0];
    if (!hh) header = v[1];
    double total = payload + header;
    int n = add(out, 0, "Overhead percentage = overhead / total packet size * 100.");
    n = add(out, n, "total = payload + header = %.10g + %.10g = %.10g bytes", payload, header, total);
    n = add(out, n, "overhead = %.10g/%.10g * 100 = %.10g%%", header, total, total ? header/total*100.0 : 0);
    if (has(t, "efficiency") || has(t, "efficient")) return add(out, n, "efficiency = payload/total * 100 = %.10g%%", total ? payload/total*100.0 : 0);
    return n;
  }
  if ((has(t, "serial") || has(t, "character") || has(t, "characters")) &&
      (has(t, "bitpercharacter") || has(t, "bitspercharacter") || (has(t, "bits") && has(t, "character"))) &&
      (has(t, "bit/s") || has(t, "bitspersecond") || has(t, "rate") || has(t, "over")) && nv >= 3) {
    double bits_per_char = v[0], chars = v[1], rate = v[2];
    double tmp = 0;
    if (scan_before_word_num(t, "bitspercharacter", &tmp) || scan_before_word_num(t, "bitpercharacter", &tmp) ||
        scan_before_word_num(t, "bits", &tmp)) bits_per_char = tmp;
    if (scan_before_word_num(t, "characters", &tmp) || scan_before_word_num(t, "character", &tmp)) chars = tmp;
    if (scan_before_word_num(t, "bitspersecond", &tmp) || scan_before_word_num(t, "bit/s", &tmp) ||
        scan_before_word_num(t, "bits/s", &tmp)) rate = tmp;
    double total = bits_per_char * chars;
    int n = add(out, 0, "Transmission time = total bits / bit rate.");
    n = add(out, n, "total bits = %.10g characters * %.10g bits = %.10g bits", chars, bits_per_char, total);
    return add(out, n, "time = %.10g/%.10g = %.10g s", total, rate, rate ? total / rate : 0);
  }
  if (has(t, "transfer") || has(t, "download") || has(t, "transmit") || has(t, "transmission") || has_word(t, "sent")) {
    double size=0, rate=0;
    bool hSize=label_num(input,"size",&size) || label_num(input,"filesize",&size) || label_num(input,"file",&size);
    bool hRate=label_num(input,"rate",&rate) || label_num(input,"bitrate",&rate) || label_num(input,"speed",&rate);
    if (!hSize && nv >= 1) { size = v[0]; hSize = true; }
    if (!hRate && nv >= 2) { rate = v[1]; hRate = true; }
    if (hSize && hRate) {
      int rn = add_transfer_unit_lines(out, size, rate, t);
      if (rn) return rn;
    }
    if (hSize && hRate) {
      if ((has(t, "megabyte") || has(t, "mbyte") || has(t, "mb,")) && (has(t, "megabit") || has(t, "mbit"))) sprintf(cmd, "transfermb(%.10g,%.10g)", size, rate);
      else if ((has(t, "kilobyte") || has(t, "kbyte") || has(t, "kb,")) && (has(t, "kilobit") || has(t, "kbit"))) sprintf(cmd, "transferkb(%.10g,%.10g)", size, rate);
      else sprintf(cmd, "transfer(%.10g,%.10g)", size, rate);
      return eval_storage(cmd, out);
    }
  }
  if (has(t, "dictionary") || has(t, "dict") || has(t, "lz")) {
    double orig=0, dict=0, refs=0, rb=0;
    bool hO=label_num(input,"original",&orig) || label_num(input,"old",&orig);
    bool hD=label_num(input,"dictionary",&dict) || label_num(input,"dict",&dict);
    bool hRefs=label_num(input,"references",&refs) || label_num(input,"refs",&refs);
    bool hRb=label_num(input,"referencebits",&rb) || label_num(input,"refbits",&rb);
    if (hO && hD && hRefs && hRb) {
      sprintf(cmd, "dictcompress(%.10g,%.10g,%.10g,%.10g)", orig, dict, refs, rb);
      return eval_storage(cmd, out);
    }
  }
  if (has(t, "compress") || has(t, "compression")) {
    int un = add_compression_unit_lines(out, t);
    if (un) return un;
    double oldv=0, newv=0;
    bool hO=label_num(input,"original",&oldv) || label_num(input,"before",&oldv) || label_num(input,"old",&oldv);
    bool hN=label_num(input,"compressed",&newv) || label_num(input,"after",&newv) || label_num(input,"new",&newv);
    if (hO && hN) {
      sprintf(cmd, "compress(%.10g,%.10g)", oldv, newv);
      return eval_storage(cmd, out);
    }
    double mb=0, kb=0;
    if ((scan_before_word_num(t, "mb", &mb) || scan_before_word_num(t, "megabytes", &mb) || scan_before_word_num(t, "megabyte", &mb)) &&
        (scan_before_word_num(t, "kb", &kb) || scan_before_word_num(t, "kilobytes", &kb) || scan_before_word_num(t, "kilobyte", &kb))) {
      double oldb = mb * 1000000.0, newb = kb * 1000.0;
      int n = add(out, 0, "Compression ratio = original / compressed.");
      n = add(out, n, "%.10g MB = %.10g bytes", mb, oldb);
      n = add(out, n, "%.10g KB = %.10g bytes", kb, newb);
      n = add(out, n, "ratio = %.10g : 1", oldb / newb);
      return add(out, n, "percentage reduction = %.6g%%", (oldb - newb) * 100.0 / oldb);
    }
  }
  if (has(t, "runlength") || has(t, "rle") || (has(t, "run") && has(t, "length"))) {
    double runs=0, sb=0, cb=0;
    bool hRuns=label_num(input,"runs",&runs);
    bool hSb=label_num(input,"symbolbits",&sb) || label_num(input,"symbol",&sb);
    bool hCb=label_num(input,"countbits",&cb) || label_num(input,"count",&cb);
    if (hRuns && hSb && hCb) {
      sprintf(cmd, "rle(%lld,%lld,%lld)", (long long)runs, (long long)sb, (long long)cb);
      return eval_storage(cmd, out);
    }
  }
  if ((has(t, "record") || has(t, "database")) && !has(t, "field") &&
      (label_num(input,"records",&width) || label_num(input,"rows",&width)) &&
      (label_num(input,"bytes",&height) || label_num(input,"recordsize",&height) || label_num(input,"bytesperrecord",&height))) {
    sprintf(cmd, "records(%.10g,%.10g)", width, height); return eval_storage(cmd, out);
  }
  if ((has(t, "record") || has(t, "database") || has(t, "relation")) && has(t, "field") && nv >= 3) {
    double recs=0;
    bool hr = label_num(input,"records",&recs) || label_num(input,"rows",&recs) ||
              scan_before_word_num(t, "records", &recs) || scan_before_word_num(t, "rows", &recs);
    if (hr) {
      double bytes_per_record = 0;
      for (int i = 0; i < nv; ++i) if ((long long)v[i] != (long long)recs) bytes_per_record += v[i];
      int n = add(out, 0, "Record size = sum of field sizes.");
      n = add(out, n, "bytes per record = %.10g", bytes_per_record);
      n = add(out, n, "file size = records * bytes per record");
      return add(out, n, "%.10g*%.10g = %.10g bytes", recs, bytes_per_record, recs * bytes_per_record);
    }
  }
  if ((has(t, "character") || has(t, "text")) && (label_num(input,"characters",&width) || label_num(input,"chars",&width)) &&
      (label_num(input,"bits",&height) || label_num(input,"bitspercharacter",&height))) {
    sprintf(cmd, "chars(%lld,%lld)", (long long)width, (long long)height); return eval_storage(cmd, out);
  }
  if ((has(t, "character") || has(t, "text")) && (label_num(input,"characters",&width) || label_num(input,"chars",&width)) &&
      (label_num(input,"characterset",&height) || label_num(input,"symbols",&height) || label_num(input,"alphabet",&height))) {
    sprintf(cmd, "charset(%lld,%lld)", (long long)width, (long long)height); return eval_storage(cmd, out);
  }
  bool tc = has(t, "twos") || (has(t, "two") && has(t, "complement"));
  bool sm = has(t, "signmagnitude") || (has(t, "sign") && has(t, "magnitude"));
  if ((has(t, "encode") || has(t, "convert") || (has(t, "represent") && !has(t, "representable") && !has(t, "represented") && !has(t, "closest") && !has(t, "explain"))) && (has(t, "floating") || has(t, "mantissa")) &&
      (has(t, "mantissa") && has(t, "exponent")) && nv >= 3) {
    double mb=0, eb=0, tmp=0, value=v[0];
    bool hM = scan_bit_width_before_label(t, "mantissa", &tmp) || scan_before_word_num(t, "mantissa", &tmp); if (hM) mb = tmp;
    bool hE = scan_bit_width_before_label(t, "exponent", &tmp) || scan_before_word_num(t, "exponent", &tmp); if (hE) eb = tmp;
    if (!hM) mb = v[0];
    if (!hE) eb = v[1];
    for (int i = 0; i < nv; ++i) {
      double dm = v[i] > mb ? v[i] - mb : mb - v[i];
      double de = v[i] > eb ? v[i] - eb : eb - v[i];
      if (dm > 1e-9 && de > 1e-9) { value = v[i]; break; }
    }
    sprintf(cmd, "floatenc(%.10g,%lld,%lld)", value, (long long)mb, (long long)eb);
    return eval_float(cmd, out);
  }
  if ((has(t, "normalise") || has(t, "normalize")) && (has(t, "denary") || has(t, "decimal") || has(t, "value")) &&
      (has(t, "mantissa") || has(t, "exponent") || has(t, "floating")) && nv >= 3) {
    double mb=0, eb=0, tmp=0;
    bool hM = scan_bit_width_before_label(t, "mantissa", &tmp) || scan_before_word_num(t, "mantissa", &tmp); if (hM) mb = tmp;
    bool hE = scan_bit_width_before_label(t, "exponent", &tmp) || scan_before_word_num(t, "exponent", &tmp); if (hE) eb = tmp;
    if (!hM) mb = v[1];
    if (!hE) eb = v[2];
    sprintf(cmd, "floatenc(%.10g,%lld,%lld)", v[0], (long long)mb, (long long)eb);
    return eval_float(cmd, out);
  }
  if ((has(t, "normalise") || has(t, "normalize")) && !has(t, "normalised") && !has(t, "normalized") && nb >= 2) {
    sprintf(cmd, "floatnorm(%s,%s)", bits[0], bits[1]); return eval_float(cmd, out);
  }
  if ((has(t, "float") || has(t, "floating") || (has(t, "mantissa") && has(t, "exponent"))) &&
      (has(t, "decode") || has(t, "denary") || has(t, "decimal") || has(t, "number")) && nb >= 2) {
    sprintf(cmd, "floatdec(%s,%s)", bits[0], bits[1]); return eval_float(cmd, out);
  }
  if ((has(t, "arithmeticshift") || (has(t, "arithmetic") && has(t, "shift")) || (has(t, "signed") && has(t, "shift"))) && nb >= 1 && nv >= 1) {
    double sh = v[nv-1]; scan_after_word_num(t, "by", &sh);
    sprintf(cmd, "arithshift(%s,%s,%lld)", bits[0], has(t, "right") ? "right" : "left", (long long)sh); return eval_binary_arith(cmd, out);
  }
  if (tc && has(t, "shift") && nb == 0 && nv >= 3) {
    double bw = 0, sh = v[nv-1];
    long long bitsw = (long long)v[0], val = (long long)v[1];
    if ((scan_before_word_num(t, "bit", &bw) || scan_before_word_num(t, "bits", &bw)) && bw > 0) {
      bitsw = (long long)bw;
      for (int i = 0; i < nv; ++i) {
        if ((long long)v[i] == bitsw || (long long)v[i] == (long long)sh) continue;
        val = (long long)v[i]; break;
      }
    }
    scan_after_word_num(t, "by", &sh);
    char enc[65]; to_bin(val, (int)bitsw, enc);
    sprintf(cmd, "arithshift(%s,%s,%lld)", enc, has(t, "right") ? "right" : "left", (long long)sh);
    return eval_binary_arith(cmd, out);
  }
  if (tc && !has(t, "fixed") && nv >= 2 && (has(t, "encode") || has(t, "convert")) && (has(t, "binary") || has(t, "bit"))) {
    double bw = 0; long long bitsw = (long long)v[1], val = (long long)v[0];
    if ((scan_before_word_num(t, "bit", &bw) || scan_before_word_num(t, "bits", &bw)) && bw > 0) {
      bitsw = (long long)bw;
      for (int i = 0; i < nv; ++i) if ((long long)v[i] != bitsw) { val = (long long)v[i]; break; }
    }
    if ((has(t, "minus") || has(t, "negative")) && val > 0) val = -val;
    sprintf(cmd, "twos(%lld,%lld)", val, bitsw); return eval_twos(cmd, out);
  }
  if (tc && !has(t, "fixed") && nb >= 1 && (has(t, "decode") || has(t, "denary") || has(t, "decimal") || has(t, "value"))) {
    sprintf(cmd, "twosdec(%s)", bits[0]); return eval_twos(cmd, out);
  }
  if (has(t, "bcd") && (has(t, "decode") || has(t, "denary") || has(t, "decimal")) && (nb >= 1 || nbg >= 1)) {
    char all[96];
    if (nb >= 1) join_bits(bits, nb, all, sizeof(all));
    else join_bits(bitgrp, nbg, all, sizeof(all));
    sprintf(cmd, "bcddec(%s)", all);
    return eval_base(cmd, out);
  }
  if (has(t, "bcd") && nv >= 1) {
    sprintf(cmd, "bcd(%lld)", (long long)v[0]); return eval_base(cmd, out);
  }
  if (has(t, "base") && ((has(t, "base,2") || has(t, "base,two")) && (has(t, "base,8") || has(t, "base,eight")))) {
    const char *p2 = strstr(t, "base,2"); if (!p2) p2 = strstr(t, "base,two");
    const char *p8 = strstr(t, "base,8"); if (!p8) p8 = strstr(t, "base,eight");
    if (p2 && p8 && p2 < p8 && nb >= 1) {
      sprintf(cmd, "convert(%s,2,8)", bits[0]); return eval_base(cmd, out);
    }
    if (p8 && p2 && p8 < p2 && nv >= 1) {
      sprintf(cmd, "convert(%lld,8,2)", (long long)v[0]); return eval_base(cmd, out);
    }
  }
  if (has(t, "base") && has(t, "to") && has(t, "16") && has(t, "10") && has_hex_tok && has(t, "to,base,10")) {
    sprintf(cmd, "den(%s,16)", hex_tok); return eval_base(cmd, out);
  }
  if (has(t, "base") && has(t, "to") && (has(t, "base,16,to,base,2") || has(t, "base16,to,base2")) && has_hex_tok) {
    sprintf(cmd, "convert(%s,16,2)", hex_tok); return eval_base(cmd, out);
  }
  if (has(t, "base") && has(t, "to") && (has(t, "base,2,to,base,16") || has(t, "base2,to,base16")) && nb >= 1) {
    sprintf(cmd, "convert(%s,2,16)", bits[0]); return eval_base(cmd, out);
  }
  if (has(t, "base") && has(t, "to") && (has(t, "base,2,to,base,8") || has(t, "base2,to,base8")) && nb >= 1) {
    sprintf(cmd, "convert(%s,2,8)", bits[0]); return eval_base(cmd, out);
  }
  if (has(t, "base") && has(t, "to") && (has(t, "base,8,to,base,2") || has(t, "base8,to,base2")) && nv >= 1) {
    sprintf(cmd, "convert(%lld,8,2)", (long long)v[0]); return eval_base(cmd, out);
  }
  if (has(t, "base") && has(t, "to") && has(t, "base,10,to,base,16") && nv >= 1) {
    sprintf(cmd, "convert(%lld,10,16)", (long long)v[0]); return eval_base(cmd, out);
  }
  if (has(t, "base") && has(t, "to") && has(t, "base,10,to,base,2") && nv >= 1) {
    sprintf(cmd, "convert(%lld,10,2)", (long long)v[0]); return eval_base(cmd, out);
  }
  if ((has(t, "denary") || has(t, "decimal") || has(t, "base,10")) && (has(t, "octal") || has(t, "base,8") || has(t, "base8")) && nv >= 1) {
    sprintf(cmd, "convert(%lld,10,8)", (long long)v[0]); return eval_base(cmd, out);
  }
  if ((has(t, "octal") || has(t, "base,8") || has(t, "base8")) && has(t, "binary")) {
    const char *bp = strstr(t, "binary");
    const char *op = strstr(t, "octal");
    if (!op) op = strstr(t, "base,8");
    if (!op) op = strstr(t, "base8");
    if (bp && op && bp < op && nb >= 1) {
      sprintf(cmd, "convert(%s,2,8)", bits[0]); return eval_base(cmd, out);
    }
    if (op && bp && op < bp && nv >= 1) {
      sprintf(cmd, "convert(%lld,8,2)", (long long)v[0]); return eval_base(cmd, out);
    }
  }
  if (has(t, "fixed") && nfixed >= 1 && (has(t, "hex") || has(t, "hexadecimal")) &&
      (has(t, "binary") || (!has(t, "denary") && !has(t, "decimal")))) {
    double val = fixed_decode(fixed_tokens[0]);
    const char *dot = strchr(fixed_tokens[0], '.');
    char hx[48]; long long whole = (long long)val; sprintf(hx, "%llX", whole);
    int hp = (int)strlen(hx);
    if (dot && dot[1] && hp + 2 < (int)sizeof(hx)) {
      int flen = (int)strlen(dot + 1);
      hx[hp++] = '.';
      for (int off = 0; off < flen && hp + 1 < (int)sizeof(hx); off += 4) {
        int nib = 0;
        for (int j = 0; j < 4; ++j) nib = nib * 2 + (off + j < flen && dot[1+off+j] == '1' ? 1 : 0);
        hx[hp++] = (char)(nib < 10 ? '0' + nib : 'A' + nib - 10);
      }
      hx[hp] = 0;
    }
    int n = add(out, 0, "Add binary fixed-point place values.");
    n = add(out, n, "%s_2 = %.10g_10", fixed_tokens[0], val);
    return add(out, n, "%s_2 = %s_16", fixed_tokens[0], hx);
  }
  if (has(t, "binary") && nb >= 1 && (has(t, "hex") || has(t, "hexadecimal")) &&
      !(has(t, "add") || has(t, "sum") || has(t, "plus") || has(t, "subtract") || has(t, "minus"))) {
    char all[96]; join_bits(bits, nb, all, sizeof(all));
    sprintf(cmd, "convert(%s,2,16)", all); return eval_base(cmd, out);
  }
  if ((has(t, "hex") || has(t, "hexadecimal")) && has_hex_tok && has(t, "binary")) {
    sprintf(cmd, "convert(%s,16,2)", hex_tok); return eval_base(cmd, out);
  }
  if ((has(t, "hex") || has(t, "hexadecimal")) && has_hex_tok && (has(t, "denary") || has(t, "decimal"))) {
    sprintf(cmd, "den(%s,16)", hex_tok); return eval_base(cmd, out);
  }
  char fixed_early[48];
  if (has(t, "fixed") && (has(t, "add") || has(t, "sum") || has(t, "plus") ||
      has(t, "subtract") || has(t, "minus")) && nfixed >= 2) {
    bool sub = has(t, "subtract") || has(t, "minus");
    double a0 = fixed_decode(fixed_tokens[0]), b0 = fixed_decode(fixed_tokens[1]), sum = sub ? a0 - b0 : a0 + b0;
    const char *d0 = strchr(fixed_tokens[0], '.');
    const char *d1 = strchr(fixed_tokens[1], '.');
    int frac0 = d0 ? (int)strlen(d0 + 1) : 0, frac1 = d1 ? (int)strlen(d1 + 1) : 0;
    int whole0 = d0 ? (int)(d0 - fixed_tokens[0]) : (int)strlen(fixed_tokens[0]);
    int whole1 = d1 ? (int)(d1 - fixed_tokens[1]) : (int)strlen(fixed_tokens[1]);
    int frac = frac0 > frac1 ? frac0 : frac1, whole = whole0 > whole1 ? whole0 : whole1;
    while (sum >= pow2(whole) && whole < 62) ++whole;
    long long scaled = (long long)round_nearest(sum * pow2(frac));
    char bits_out[65], fixed_out[65]; to_bin(scaled, whole + frac, bits_out); insert_point(bits_out, whole, fixed_out);
    int n = add(out, 0, sub ? "Subtract binary fixed-point values by place value." : "Add binary fixed-point values by place value.");
    n = add(out, n, "%s_2 = %.10g_10", fixed_tokens[0], a0);
    n = add(out, n, "%s_2 = %.10g_10", fixed_tokens[1], b0);
    n = add(out, n, sub ? "%.10g - %.10g = %.10g" : "%.10g + %.10g = %.10g", a0, b0, sum);
    return add(out, n, "using %d fractional bits, result = %s", frac, fixed_out);
  }
  if (has(t, "fixed") && nfixed >= 1 && (has(t, "binary") || (!has(t, "denary") && !has(t, "decimal"))) &&
      (has(t, "denary") || has(t, "decimal") || has(t, "hex") || has(t, "hexadecimal"))) {
    double val = fixed_decode(fixed_tokens[0]);
    int n = add(out, 0, "Add binary fixed-point place values.");
    n = add(out, n, "%s_2 = %.10g_10", fixed_tokens[0], val);
    if (has(t, "hex") || has(t, "hexadecimal")) {
      const char *dot = strchr(fixed_tokens[0], '.');
      char hx[48]; int hp = 0;
      long long whole = (long long)fixed_decode(fixed_tokens[0]);
      sprintf(hx, "%llX", whole);
      hp = (int)strlen(hx);
      if (dot && dot[1] && hp + 2 < (int)sizeof(hx)) {
        int flen = (int)strlen(dot + 1);
        hx[hp++] = '.';
        for (int off = 0; off < flen && hp + 1 < (int)sizeof(hx); off += 4) {
          int nib = 0;
          for (int j = 0; j < 4; ++j) nib = nib * 2 + (off + j < flen && dot[1+off+j] == '1' ? 1 : 0);
          hx[hp++] = (char)(nib < 10 ? '0' + nib : 'A' + nib - 10);
        }
        hx[hp] = 0;
      }
      return add(out, n, "%s_2 = %s_16", fixed_tokens[0], hx);
    }
    return n;
  }
  if (has(t, "fixed") && (has(t, "convert") || has(t, "encode") || has(t, "represent")) &&
      (has(t, "denary") || has(t, "decimal")) && !has(t, "to,denary") && !has(t, "to,decimal") && nv >= 1 &&
      !has(t, "after") && !has(t, "fractional") && !has(t, "before") && !has(t, "whole") && !has(t, "integer")) {
    double value = v[0];
    long long whole = (long long)value;
    double rem = value - whole;
    if (rem < 0) rem = -rem;
    char whole_bits[65]; to_bin(whole, whole_bits_for(value), whole_bits);
    char frac_bits[33]; int fp = 0;
    int n = add(out, 0, "Convert the fractional part by repeated multiplication by 2.");
    n = add(out, n, "whole part %.0f gives %s", (double)whole, whole_bits);
    while (rem > 1e-10 && fp < 16 && n < CSCALC_MAX_LINES - 2) {
      rem *= 2.0;
      int bit = rem >= 1.0 ? 1 : 0;
      if (bit) rem -= 1.0;
      frac_bits[fp++] = (char)('0' + bit);
      n = add(out, n, "multiply by 2 -> bit %d, remainder %.10g", bit, rem);
    }
    frac_bits[fp] = 0;
    return add(out, n, "%.10g_10 = %s.%s_2", value, whole_bits, fp ? frac_bits : "0");
  }
  if (has(t, "fixed") && (has(t, "encode") || has(t, "represent") || has(t, "convert")) &&
      (has(t, "decimal") || has(t, "denary")) && (has(t, "fractional") || has(t, "fraction") || has(t, "after")) &&
      nv >= 2 && !has(t, "before") && !has(t, "whole") && !has(t, "integer")) {
    sprintf(cmd, (tc || has(t, "signed") || has(t, "negative") || v[0] < 0) ? "fixedtcenc(%.10g,1,%lld)" : "fixedfrac(%.10g,%lld)", v[0], (long long)v[1]);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && (has(t, "encode") || has(t, "represent") || has(t, "convert")) && nv >= 3) {
    long long whole = (long long)v[1], frac = (long long)v[2];
    double wi=0, fb=0;
    bool explicit_widths = (scan_before_word_num(t, "integer", &wi) || scan_before_word_num(t, "whole", &wi)) &&
        (scan_before_word_num(t, "fractional", &fb) || scan_before_word_num(t, "fraction", &fb));
    if (explicit_widths) {
      whole = (long long)wi; frac = (long long)fb;
    }
    if (!explicit_widths && (has(t, "after") || has(t, "afterthepoint") || has(t, "fractional")) && !has(t, "before") && !has(t, "whole")) {
      whole = (long long)v[1] - (long long)v[2];
      if (whole < 1) whole = 1;
    }
    sprintf(cmd, (tc || has(t, "signed") || has(t, "negative") || v[0] < 0) ? "fixedtcenc(%.10g,%lld,%lld)" : "fixedenc(%.10g,%lld,%lld)", v[0], whole, frac);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && (has(t, "to,denary") || has(t, "to,decimal") ||
      (!has(t, "denary") && !has(t, "decimal") && !strstr(input, "denary") && !strstr(input, "decimal"))) &&
      scan_fixed_bits(t, fixed_early, sizeof(fixed_early))) {
    sprintf(cmd, (tc || has(t, "complement")) ? "fixedtc(%s)" : "fixed(%s)", fixed_early); return eval_float(cmd, out);
  }
  if (has(t, "binary") && !strstr(input, "denary value") && !strstr(input, "decimal value") &&
      (has(t, "to,denary") || has(t, "to,decimal") || strstr(input, "to denary") || strstr(input, "to decimal")) &&
      scan_fixed_bits(t, fixed_early, sizeof(fixed_early)) &&
      !(has(t, "add") || has(t, "sum") || has(t, "plus") || has(t, "subtract") || has(t, "minus"))) {
    sprintf(cmd, "fixed(%s)", fixed_early); return eval_float(cmd, out);
  }
  if ((has(t, "denary") || has(t, "decimal")) && nb >= 1 &&
      !(has(t, "add") || has(t, "sum") || has(t, "plus") || has(t, "subtract") || has(t, "minus"))) {
    char all[96]; join_bits(bits, nb, all, sizeof(all));
    sprintf(cmd, "den(%s,2)", all); return eval_base(cmd, out);
  }
  if (tc && nbg >= 2 && (has(t, "add") || has(t, "sum") || has(t, "plus"))) {
    sprintf(cmd, "twosadd(%s,%s)", bitgrp[0], bitgrp[1]); return eval_twos(cmd, out);
  }
  if (tc && nv >= 3 && nb < 2 && nbg < 2 && (has(t, "add") || has(t, "sum") || has(t, "plus"))) {
    double bw=0; long long bitsw = 0, vals[2]; int got = 0;
    if (scan_before_word_num(t, "bit", &bw) || scan_before_word_num(t, "bits", &bw)) bitsw = (long long)bw;
    if (bitsw > 0) {
      for (int i = 0; i < nv && got < 2; ++i) {
        if ((long long)v[i] == bitsw) continue;
        vals[got++] = (long long)v[i];
      }
      if (got == 2) {
        char a[65], b[65]; to_bin(vals[0], (int)bitsw, a); to_bin(vals[1], (int)bitsw, b);
        sprintf(cmd, "twosadd(%s,%s)", a, b); return eval_twos(cmd, out);
      }
    }
  }
  if (has(t, "unsigned") && nbg >= 2 && (strchr(input, '+') || has(t, "overflow"))) {
    sprintf(cmd, "binadd(%s,%s,%d)", bitgrp[0], bitgrp[1], (int)strlen(bitgrp[0]));
    return eval_binary_arith(cmd, out);
  }
  if (has(t, "unsigned") && nb >= 2 && (strchr(input, '+') || has(t, "overflow"))) {
    sprintf(cmd, "binadd(%s,%s,%d)", bits[0], bits[1], (int)strlen(bits[0]));
    return eval_binary_arith(cmd, out);
  }
  if ((has(t, "add") || has(t, "sum") || has(t, "plus")) && nbg >= 2 && (has(t, "binary") || has(t, "bits"))) {
    sprintf(cmd, "binadd(%s,%s,%d)", bitgrp[0], bitgrp[1], (int)strlen(bitgrp[0])); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "subtract") || has(t, "minus") || strchr(input, '-')) && nbg >= 2 &&
      (has(t, "binary") || has(t, "bits") || !tc)) {
    sprintf(cmd, "binsub(%s,%s,%d)", bitgrp[0], bitgrp[1], (int)strlen(bitgrp[0]));
    return eval_binary_arith(cmd, out);
  }
  if (has(t, "binary") && !has(t, "fixed") && !tc && !sm && nv >= 1 && nb == 0 &&
      !(has(t, "bitsneeded") || has(t, "bitwidth") || (has(t, "minimum") && has(t, "bits")) ||
        (has(t, "fewest") && has(t, "bits")) || (has(t, "smallest") && has(t, "bits")))) {
    sprintf(cmd, "bin(%lld)", (long long)v[0]); return eval_base(cmd, out);
  }
  if ((has(t, "hex") || has(t, "hexadecimal")) && nv >= 1) {
    sprintf(cmd, "hex(%lld)", (long long)v[0]); return eval_base(cmd, out);
  }
  if (has(t, "unsigned") && has(t, "range") && nv >= 1) {
    sprintf(cmd, "unsignedrange(%lld)", (long long)v[0]); return eval_twos(cmd, out);
  }
  if (sm && has(t, "range") && nv >= 1) {
    sprintf(cmd, "signmagrange(%lld)", (long long)v[0]); return eval_twos(cmd, out);
  }
  if (!has(t, "mantissa") && !has(t, "exponent") &&
      (has(t, "bitsneeded") || has(t, "bitwidth") || (has(t, "minimum") && has(t, "bits")) ||
       (has(t, "fewest") && has(t, "bits")) || (has(t, "smallest") && has(t, "bits"))) && nv >= 1) {
    if (tc) sprintf(cmd, "bitsneeded(%lld,twos)", (long long)v[0]);
    else if (sm) sprintf(cmd, "bitsneeded(%lld,signmag)", (long long)v[0]);
    else sprintf(cmd, "bitsneeded(%lld,unsigned)", (long long)v[0]);
    return eval_twos(cmd, out);
  }
  if (sm && nb >= 1 && (has(t, "decode") || has(t, "denary") || has(t, "decimal"))) {
    sprintf(cmd, "signmagdec(%s)", bits[0]); return eval_twos(cmd, out);
  }
  if (sm && nv >= 2 && nb == 0) {
    sprintf(cmd, "signmag(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_twos(cmd, out);
  }
  if (tc && has(t, "range") && nv >= 1) {
    sprintf(cmd, "twosrange(%lld)", (long long)v[0]); return eval_twos(cmd, out);
  }
  if (tc && !has(t, "fixed") && nb >= 1 && (has(t, "decode") || has(t, "denary") || has(t, "decimal"))) {
    sprintf(cmd, "twosdec(%s)", bits[0]); return eval_twos(cmd, out);
  }
  if (tc && nv >= 3 && nb < 2 && nbg < 2 && (has(t, "subtract") || has(t, "minus") || has(t, "takeaway") || strchr(input, '-'))) {
    double bw=0; long long bitsw = 0, vals[2]; int got = 0;
    if (scan_before_word_num(t, "bit", &bw) || scan_before_word_num(t, "bits", &bw)) bitsw = (long long)bw;
    if (bitsw > 0) {
      for (int i = 0; i < nv && got < 2; ++i) {
        if ((long long)v[i] == bitsw) continue;
        vals[got++] = (long long)v[i];
      }
      if (got == 2) {
        if (has(compact, "from")) { long long q = vals[0]; vals[0] = vals[1]; vals[1] = q; }
        char a[65], b[65]; to_bin(vals[0], (int)bitsw, a); to_bin(vals[1], (int)bitsw, b);
        sprintf(cmd, "twossub(%s,%s)", a, b); return eval_twos(cmd, out);
      }
    }
  }
  if (has(t, "booth") && (has(t, "multiply") || has(t, "multiplication")) && nv >= 3) {
    long long a0 = (long long)v[0], b0 = (long long)v[1], bitsw = (long long)v[2];
    char ab[65], bb[65], rb[65];
    to_bin(a0, (int)bitsw, ab);
    to_bin(b0, (int)bitsw, bb);
    to_bin(a0*b0, (int)(2*bitsw), rb);
    int n = add(out, 0, "Booth multiplication uses signed two's-complement operands.");
    n = add(out, n, "multiplicand = %lld -> %s, multiplier = %lld -> %s", a0, ab, b0, bb);
    n = add(out, n, "Use pairs Q0,Q-1: 01 add M, 10 subtract M, 00/11 shift only.");
    n = add(out, n, "product = %lld * %lld = %lld", a0, b0, a0*b0);
    return add(out, n, "%lld-bit product = %s", 2*bitsw, rb);
  }
  if (tc && nb >= 2 && (has(t, "subtract") || has(t, "minus") || has(t, "takeaway") || strchr(input, '-'))) {
    if (has(compact, "from")) sprintf(cmd, "twossub(%s,%s)", bits[1], bits[0]);
    else sprintf(cmd, "twossub(%s,%s)", bits[0], bits[1]);
    return eval_twos(cmd, out);
  }
  if (tc && nbg >= 2 && (has(t, "subtract") || has(t, "minus") || has(t, "takeaway") || strchr(input, '-'))) {
    if (has(compact, "from")) sprintf(cmd, "twossub(%s,%s)", bitgrp[1], bitgrp[0]);
    else sprintf(cmd, "twossub(%s,%s)", bitgrp[0], bitgrp[1]);
    return eval_twos(cmd, out);
  }
  if (tc && nb >= 2 && (has(t, "add") || has(t, "sum") || has(t, "plus"))) {
    sprintf(cmd, "twosadd(%s,%s)", bits[0], bits[1]); return eval_twos(cmd, out);
  }
  if (tc && nv >= 2 && !(has(t, "decode") || has(t, "denary") || has(t, "decimal") ||
                          has(t, "add") || has(t, "sum") || has(t, "plus") ||
                          has(t, "subtract") || has(t, "minus") || has(t, "takeaway") || strchr(input, '-'))) {
    long long val = (long long)v[0], bitsw = (long long)v[1];
    double bw = 0;
    if ((scan_before_word_num(t, "bit", &bw) || scan_before_word_num(t, "bits", &bw)) && bw > 0) {
      bitsw = (long long)bw;
      for (int i = 0; i < nv; ++i) if ((long long)v[i] != bitsw) { val = (long long)v[i]; break; }
    }
    if ((has(t, "minus") || has(t, "negative")) && !has(t, "subtract") && val > 0) val = -val;
    sprintf(cmd, "twos(%lld,%lld)", val, bitsw); return eval_twos(cmd, out);
  }
  if ((has(t, "add") || has(t, "sum") || has(t, "plus")) && nbg >= 2 && (has(t, "binary") || has(t, "bits"))) {
    sprintf(cmd, "binadd(%s,%s,%d)", bitgrp[0], bitgrp[1], (int)strlen(bitgrp[0])); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "repetition") || has(t, "repeat") || has(t, "majorityvote") || has(t, "majority")) && nb >= 1) {
    int group = 3;
    if (nv >= 2 && v[nv - 1] >= 2 && v[nv - 1] <= 15) group = (int)v[nv - 1];
    const char *op = (has(t, "encode") || has(t, "transmit")) ? "repeatenc" : "repeatdec";
    sprintf(cmd, "%s(%s,%d)", op, bits[0], group);
    return eval_binary_arith(cmd, out);
  }
  if ((has(t, "float") || has(t, "floating")) && nb >= 4 &&
      (has(t, "add") || has(t, "sum") || has(t, "plus") || has(t, "subtract") || has(t, "minus") ||
       has(t, "multiply") || has(t, "times") || has(t, "product") || has(t, "divide"))) {
    const char *op = "floatadd";
    if (has(t, "subtract") || has(t, "minus")) op = "floatsub";
    else if (has(t, "multiply") || has(t, "times") || has(t, "product")) op = "floatmul";
    else if (has(t, "divide")) op = "floatdiv";
    sprintf(cmd, "%s(%s,%s,%s,%s)", op, bits[0], bits[1], bits[2], bits[3]);
    return eval_float(cmd, out);
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
  if (has(t, "hamming") && (has(t, "decode") || has(t, "code")) && nb >= 1 && strlen(bits[0]) == 7) {
    const char *b = bits[0];
    int s1 = (b[0]-'0') ^ (b[2]-'0') ^ (b[4]-'0') ^ (b[6]-'0');
    int s2 = (b[1]-'0') ^ (b[2]-'0') ^ (b[5]-'0') ^ (b[6]-'0');
    int s4 = (b[3]-'0') ^ (b[4]-'0') ^ (b[5]-'0') ^ (b[6]-'0');
    int pos = s1 + 2*s2 + 4*s4;
    int n = add(out, 0, "For Hamming(7,4), calculate the parity syndrome.");
    n = add(out, n, "s1=%d, s2=%d, s4=%d", s1, s2, s4);
    if (!pos) return add(out, n, "syndrome = 0, no single-bit error detected.");
    char corr[8]; strncpy(corr, b, 8); corr[pos-1] = corr[pos-1] == '1' ? '0' : '1';
    n = add(out, n, "syndrome = %d, so bit %d is wrong.", pos, pos);
    return add(out, n, "corrected code = %s", corr);
  }
  if (has(t, "checksum") && (has(t, "modulo") || has(t, "mod")) && nv >= 2) {
    int mod = (int)v[0], sum = 0;
    for (int i = 1; i < nv; ++i) sum += (int)v[i];
    int rem = mod ? sum % mod : sum;
    int n = add(out, 0, "Modulo checksum: add the decimal digits/blocks, then take the remainder.");
    n = add(out, n, "sum = %d", sum);
    return add(out, n, "checksum = %d mod %d = %d", sum, mod, rem);
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
  if ((has(t, "subtract") || has(t, "minus") || strchr(input, '-')) && nb >= 2 && !tc) {
    sprintf(cmd, "binsub(%s,%s,%d)", bits[0], bits[1], (int)strlen(bits[0]));
    return eval_binary_arith(cmd, out);
  }
  if ((has(t, "arithmeticshift") || (has(t, "arithmetic") && has(t, "shift")) || (has(t, "signed") && has(t, "shift"))) && nb >= 1 && nv >= 1) {
    double sh = v[nv-1]; scan_after_word_num(t, "by", &sh);
    sprintf(cmd, "arithshift(%s,%s,%lld)", bits[0], has(t, "right") ? "right" : "left", (long long)sh); return eval_binary_arith(cmd, out);
  }
  if (has(t, "shift") && nb >= 1 && nv >= 1) {
    double sh = v[nv-1]; scan_after_word_num(t, "by", &sh);
    sprintf(cmd, "shift(%s,%s,%lld)", bits[0], has(t, "right") ? "right" : "left", (long long)sh); return eval_binary_arith(cmd, out);
  }
  if (has(t, "parity") && nb >= 1) {
    sprintf(cmd, "parity(%s,%s)", bits[0], has(t, "odd") ? "odd" : "even"); return eval_binary_arith(cmd, out);
  }
  if ((has(t, "isbn") || has(t, "ean")) && (has(t, "checkdigit") || (has(t, "check") && has(t, "digit"))) && nv >= 1) {
    char digits[20]; sprintf(digits, "%lld", (long long)v[0]);
    int len = (int)strlen(digits), sum = 0;
    if (len == 12 || len == 13) {
      int upto = len == 13 ? 12 : len;
      int n = add(out, 0, "ISBN-13/EAN check digit uses alternating weights 1 and 3.");
      for (int i = 0; i < upto; ++i) {
        int d = digits[i] - '0', w = (i % 2) ? 3 : 1;
        sum += d * w;
      }
      int rem = sum % 10, check = (10 - rem) % 10;
      n = add(out, n, "weighted sum of first %d digits = %d", upto, sum);
      n = add(out, n, "check digit = (10 - %d) mod 10", rem);
      return add(out, n, "check digit = %d", check);
    }
  }
  if ((has(t, "checkdigit") || (has(t, "check") && has(t, "digit"))) && nv >= 3) {
    double av[32]; int nav = scan_nums(t, av, 32); double mod = nav > 1 ? av[nav-1] : v[nv-1];
    scan_after_word_num(t, "modulo", &mod); scan_after_word_num(t, "mod", &mod);
    int p = sprintf(cmd, "checkdigit(%lld,%lld", (long long)av[0], (long long)mod);
    int end = nav;
    if (nav > 1 && (long long)av[nav-1] == (long long)mod) end = nav - 1;
    for (int i = 1; i < end && p < (int)sizeof(cmd) - 20; ++i) p += sprintf(cmd + p, ",%lld", (long long)av[i]);
    sprintf(cmd + p, ")");
    return eval_binary_arith(cmd, out);
  }
  char fixed[48];
  if (has(t, "fixed") && (has(t, "encode") || has(t, "represent") || has(t, "convert")) &&
      (has(t, "after") || has(t, "afterthepoint") || has(t, "fractional")) &&
      !has(t, "before") && !has(t, "whole") && nv >= 3) {
    long long total = (long long)v[1], frac = (long long)v[2], whole = total - frac;
    if (whole < 1) whole = 1;
    sprintf(cmd, (tc || has(t, "signed") || v[0] < 0) ? "fixedtcenc(%.10g,%lld,%lld)" : "fixedenc(%.10g,%lld,%lld)",
            v[0], whole, frac);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && (has(t, "encode") || has(t, "represent") || has(t, "convert")) &&
      (has(t, "fractional") || has(t, "fraction")) && nv >= 2 && !has(t, "whole")) {
    sprintf(cmd, "fixedfrac(%.10g,%lld)", v[0], (long long)v[1]);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && (has(t, "encode") || has(t, "represent") || has(t, "convert")) && nv >= 3) {
    sprintf(cmd, (tc || has(t, "signed") || has(t, "negative") || v[0] < 0) ? "fixedtcenc(%.10g,%lld,%lld)" : "fixedenc(%.10g,%lld,%lld)", v[0], (long long)v[1], (long long)v[2]);
    return eval_float(cmd, out);
  }
  if (has(t, "fixed") && (has(t, "to,denary") || has(t, "to,decimal") ||
      (!has(t, "denary") && !has(t, "decimal") && !strstr(input, "denary") && !strstr(input, "decimal"))) &&
      scan_fixed_bits(t, fixed, sizeof(fixed))) {
    sprintf(cmd, (tc || has(t, "complement")) ? "fixedtc(%s)" : "fixed(%s)", fixed); return eval_float(cmd, out);
  }
  if ((has(t, "float") || has(t, "floating") || has(t, "real")) &&
      (has(t, "decode") || has(t, "denary") || has(t, "decimal") || has(t, "number")) && nb >= 2) {
    sprintf(cmd, "floatdec(%s,%s)", bits[0], bits[1]); return eval_float(cmd, out);
  }
  if ((has(t, "normalise") || has(t, "normalize")) && nb >= 2) {
    sprintf(cmd, "floatnorm(%s,%s)", bits[0], bits[1]); return eval_float(cmd, out);
  }
  if ((has(t, "represent") || has(t, "representable")) &&
      (has(t, "float") || has(t, "floating") || has(t, "normalised") || has(t, "normalized") || (has(t, "mantissa") && has(t, "exponent"))) &&
      !(has(t, "closest") || has(t, "nearest")) &&
      nv >= 3) {
    double value = v[0], mb = v[1], eb = v[2], tmp = 0;
    bool hM = scan_bit_width_before_label(t, "mantissa", &tmp) || scan_before_word_num(t, "mantissa", &tmp); if (hM) mb = tmp;
    bool hE = scan_bit_width_before_label(t, "exponent", &tmp) || scan_before_word_num(t, "exponent", &tmp); if (hE) eb = tmp;
    if (hM || hE) for (int i = 0; i < nv; ++i) {
      if ((hM && (long long)v[i] == (long long)mb) || (hE && (long long)v[i] == (long long)eb)) continue;
      value = v[i];
    }
    sprintf(cmd, "floatcanrepresent(%.10g,%lld,%lld)", value, (long long)mb, (long long)eb); return eval_float(cmd, out);
  }
  if ((has(t, "normalised") || has(t, "normalized") || has(t, "normalise") || has(t, "normalize")) &&
      (has(t, "float") || has(t, "floating") || has(t, "mantissa")) && nb >= 1) {
    sprintf(cmd, "normal(%s)", bits[0]); return eval_float(cmd, out);
  }
  if ((has(t, "float") || has(t, "floating") || has(t, "normalised") || has(t, "normalized") || (has(t, "mantissa") && has(t, "exponent"))) &&
      (has(t, "range") || has(t, "largest") || has(t, "smallest")) &&
      !(has(t, "added") || has(t, "add") || has(t, "need") || has(t, "needed") || has(t, "exact")) && nv >= 2) {
    sprintf(cmd, "floatrange(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_float(cmd, out);
  }
  if (has(t, "exponent") && has(t, "range") && nv >= 1) {
    int eb = (int)v[0], emin = -(1 << (eb - 1)), emax = (1 << (eb - 1)) - 1;
    int n = add(out, 0, "Exponent is stored as %d-bit two's complement.", eb);
    n = add(out, n, "minimum exponent = -2^(%d-1) = %d", eb, emin);
    return add(out, n, "maximum exponent = 2^(%d-1)-1 = %d", eb, emax);
  }
  if ((has(t, "closest") || has(t, "nearest")) && (has(t, "float") || has(t, "floating") || has(t, "representable")) && nv >= 3) {
    double value=v[0], mb=v[1], eb=v[2], tmp=0;
    bool hM=scan_bit_width_before_label(t, "mantissa", &tmp) || scan_before_word_num(t, "mantissa", &tmp); if (hM) mb = tmp;
    bool hE=scan_bit_width_before_label(t, "exponent", &tmp) || scan_before_word_num(t, "exponent", &tmp); if (hE) eb = tmp;
    if (hM || hE) for (int i=0; i<nv; ++i) {
      if ((hM && (long long)v[i] == (long long)mb) || (hE && (long long)v[i] == (long long)eb)) continue;
      value = v[i];
    }
    sprintf(cmd, "floatnearest(%.10g,%lld,%lld)", value, (long long)mb, (long long)eb); return eval_float(cmd, out);
  }
  if ((has(t, "bits") || has(t, "bit")) && has(t, "mantissa") &&
      (has(t, "add") || has(t, "added") || has(t, "need") || has(t, "needed") || has(t, "exact")) && nv >= 3) {
    double value=0, mb=0, eb=0;
    bool hV=label_num(input,"value",&value) || label_num(input,"decimal",&value) || label_num(input,"number",&value);
    bool hM=label_num(input,"mantissabits",&mb) || label_num(input,"mantissa",&mb);
    bool hE=label_num(input,"exponentbits",&eb) || label_num(input,"exponent",&eb);
    sprintf(cmd, "floatbitsadd(%.10g,%lld,%lld)", hV ? value : v[0], (long long)(hM ? mb : v[1]), (long long)(hE ? eb : v[2]));
    return eval_float(cmd, out);
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
  if (has(t, "address") && (has(t, "bits") || has(t, "lines")) &&
      (has(t, "locations") || has(t, "addresses")) && nv >= 1) {
    int bitsw = ceil_log2_ll((long long)(v[0] + 0.999999));
    int n = add(out, 0, "Address bits = ceil(log2(addressable locations)).");
    n = add(out, n, "Need 2^b >= %.10g", v[0]);
    return add(out, n, "b = %d address bits", bitsw);
  }
  if ((has(t, "image") || has(t, "bitmap")) && nv >= 3) {
    if (has(t, "colours") || has(t, "colors")) {
      double colours=0; bool hc = label_num(input, "colours", &colours) || label_num(input, "colors", &colours) ||
                                  scan_scaled_before_word_num(t, "colours", &colours) || scan_scaled_before_word_num(t, "colors", &colours);
      if (hc) {
        double wh[2]; int nw = 0;
        for (int i = 0; i < nv && nw < 2; ++i) if ((long long)v[i] != (long long)colours) wh[nw++] = v[i];
        if (nw == 2) {
          sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)wh[0], (long long)wh[1], (long long)colours);
          return eval_storage(cmd, out);
        }
      }
      sprintf(cmd, "imagecolors(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)scaled_colour_count(t, v[2])); return eval_storage(cmd, out);
    }
    if (image_depth_is_bytes(t)) return add_image_byte_depth_lines(out, (long long)v[0], (long long)v[1], (long long)v[2]);
    sprintf(cmd, "image(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
  }
  if ((has(t, "sound") || has(t, "audio")) && nv >= 3) {
    int ch = nv > 3 ? (int)v[3] : has(t, "stereo") ? 2 : 1;
    double rate = v[0], seconds = v[1];
    scale_frequency_unit(t, &rate);
    scale_time_unit(t, &seconds);
    sprintf(cmd, "sound(%lld,%lld,%lld,%d)", (long long)rate, (long long)seconds, (long long)v[2], ch); return eval_storage(cmd, out);
  }
  if ((has(t, "transfer") || has(t, "download") || has(t, "transmit") || has_word(t, "sent")) && (has(t, "time") || has(t, "second") || has(t, "minute") || has(t, "hour")) && nv >= 2) {
    int rn = add_transfer_unit_lines(out, v[0], v[1], t);
    if (rn) return rn;
    if ((has(t, "megabyte") || has(t, "mbyte")) && (has(t, "megabit") || has(t, "mbit"))) {
      sprintf(cmd, "transfermb(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
    }
    if ((has(t, "kilobyte") || has(t, "kbyte")) && (has(t, "kilobit") || has(t, "kbit"))) {
      sprintf(cmd, "transferkb(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "transfer(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "dictionary") || has(t, "dict") || has(t, "lz")) && (has(t, "compress") || has(t, "compression")) && nv >= 4) {
    sprintf(cmd, "dictcompress(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_storage(cmd, out);
  }
  if ((has(t, "compress") || has(t, "compression")) && nv >= 2) {
    int un = add_compression_unit_lines(out, t);
    if (un) return un;
    double mb=0, kb=0;
    if ((scan_before_word_num(t, "mb", &mb) || scan_before_word_num(t, "megabytes", &mb) || scan_before_word_num(t, "megabyte", &mb)) &&
        (scan_before_word_num(t, "kb", &kb) || scan_before_word_num(t, "kilobytes", &kb) || scan_before_word_num(t, "kilobyte", &kb))) {
      double oldb = mb * 1000000.0, newb = kb * 1000.0;
      int n = add(out, 0, "Compression ratio = original / compressed.");
      n = add(out, n, "%.10g MB = %.10g bytes", mb, oldb);
      n = add(out, n, "%.10g KB = %.10g bytes", kb, newb);
      n = add(out, n, "ratio = %.10g : 1", oldb / newb);
      return add(out, n, "percentage reduction = %.6g%%", (oldb - newb) * 100.0 / oldb);
    }
    sprintf(cmd, "compress(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "huffman") || has(t, "huffmancode")) && nv >= 2) {
    int p = sprintf(cmd, "huffman(%.10g", v[0]);
    for (int i = 1; i < nv && p < (int)sizeof(cmd) - 24; ++i) p += sprintf(cmd + p, ",%.10g", v[i]);
    sprintf(cmd + p, ")");
    return eval_storage(cmd, out);
  }
  if ((has(t, "runlength") || has(t, "rle") || (has(t, "run") && has(t, "length")))) {
    char seq[48];
    if (find_rle_sequence(input, seq, sizeof(seq))) {
      if (nv >= 2) {
        sprintf(cmd, "rletext(%s,%lld,%lld)", seq, (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
      }
      char summary[72] = ""; int sp = 0, runs = 0, len = (int)strlen(seq);
      for (int i = 0; i < len;) {
        int j = i + 1; while (j < len && seq[j] == seq[i]) ++j;
        if (runs && sp + 1 < 72) summary[sp++] = ',';
        if (sp + 8 < 72) { summary[sp++] = (char)toupper((unsigned char)seq[i]); summary[sp++] = 'x'; int c0 = j - i; if (c0 >= 10) summary[sp++] = (char)('0' + c0 / 10); summary[sp++] = (char)('0' + c0 % 10); summary[sp] = 0; }
        runs++; i = j;
      }
      int n = add(out, 0, "Run-length encode consecutive repeated symbols.");
      n = add(out, n, "runs: %s", summary);
      return add(out, n, "compressed form uses %d runs.", runs);
    }
  }
  if ((has(t, "runlength") || has(t, "rle") || (has(t, "run") && has(t, "length"))) && nv >= 3) {
    sprintf(cmd, "rle(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
  }
  if ((has(t, "sql") || has(t, "select") || has(t, "where") || has(t, "count")) && make_sql_cmd(input, cmd, sizeof(cmd))) {
    return eval_storage(cmd, out);
  }
  if ((has(t, "record") || has(t, "database")) && (has(t, "overhead") || has(t, "extra")) && nv >= 3) {
    double recs = v[0], bytes = v[1], overhead = v[2];
    double total = recs * (bytes + overhead);
    int n = add(out, 0, "Total storage = records * (record size + overhead per record).");
    n = add(out, n, "per record = %.10g + %.10g = %.10g bytes", bytes, overhead, bytes + overhead);
    n = add(out, n, "total = %.10g * %.10g", recs, bytes + overhead);
    return add(out, n, "= %.10g bytes", total);
  }
  if ((has(t, "record") || has(t, "database")) && nv >= 2) {
    sprintf(cmd, "records(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if (has(t, "address") && (has(t, "memory") || has(t, "ram")) && (has(t, "bits") || has(t, "lines") || has(t, "addressable")) && nv >= 1) {
    double bytes = v[0], wbits = 8;
    if (has(t, "gb") || has(t, "gib")) bytes *= 1073741824.0;
    else if (has(t, "mb") || has(t, "mib")) bytes *= 1048576.0;
    else if (has(t, "kb") || has(t, "kib")) bytes *= 1024.0;
    if (label_num(input, "wordbits", &wbits) || label_num(input, "word", &wbits)) {}
    else if (has(t, "word") && nv >= 2) wbits = v[1];
    sprintf(cmd, "addressbits(%.0f,%.10g)", bytes, wbits);
    return eval_storage(cmd, out);
  }
  if (has(t, "address") && (has(t, "stores") || has(t, "store") || has(t, "each")) &&
      (has(t, "byte") || has(t, "bytes")) && nv >= 2) {
    double bytes_per_address = v[1];
    int n = add(out, 0, "Memory capacity = addressable locations * bytes per address.");
    n = add(out, n, "locations = 2^%lld", (long long)v[0]);
    n = add(out, n, "bytes = 2^%lld * %.10g", (long long)v[0], bytes_per_address);
    return add(out, n, "= %.10g bytes", pow2((int)v[0]) * bytes_per_address);
  }
  if (has(t, "address") && (has(t, "bus") || has(t, "space") || has(t, "locations")) && nv >= 1) {
    if ((has(t, "word") || has(t, "capacity") || has(t, "memory")) && nv >= 2) {
      sprintf(cmd, "memorycapacity(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "addressspace(%lld)", (long long)v[0]); return eval_storage(cmd, out);
  }
  if ((has(t, "hash") || has(t, "hashtable")) && nv >= 2) {
    double size = v[0];
    bool hs = label_num(input, "size", &size) || scan_after_word_num(t, "size", &size);
    int p = sprintf(cmd, "hashmod(%lld", (long long)size);
    if (has(t, "linear") || has(t, "probe") || has(t, "probing")) p = sprintf(cmd, "hashlinear(%lld", (long long)size);
    for (int i = 0; i < nv && p < (int)sizeof(cmd) - 24; ++i) {
      if (hs && (long long)v[i] == (long long)size) continue;
      if (!hs && i == 0) continue;
      p += sprintf(cmd + p, ",%lld", (long long)v[i]);
    }
    sprintf(cmd + p, ")");
    return eval_storage(cmd, out);
  }
  if ((has(t, "character") || has(t, "text")) && nv >= 2) {
    if (has(t, "characterset") || has(t, "symbols") || has(t, "alphabet")) {
      sprintf(cmd, "charset(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
    }
    sprintf(cmd, "chars(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "maxterm") || has(t, "maxterms") || has(t, "zeros")) && make_minterm_cmd(input, cmd, sizeof(cmd))) {
    char tmp[160]; sprintf(tmp, "maxterms(%s", cmd + 9);
    strcpy(cmd, tmp);
    return eval_minterms(cmd, out);
  }
  if (has_bitcol && (has(t, "truth") || has(t, "output")) &&
      (has(t, "column") || has(t, "outputs") || has(t, "bits") || has(t, "table"))) {
    truthbits_cmd_from_text(t, bitcol, cmd, sizeof(cmd)); return eval_truthbits(cmd, out);
  }
  if (nb >= 1 && (has(t, "truth") || has(t, "output") || has(t, "column")) &&
      (has(t, "bits") || has(t, "column") || has(t, "derive") || has(t, "expression") ||
       has(t, "variables") || has(t, "vars") || has(t, "table") ||
       has(t, "sumofproducts") || has(t, "sop"))) {
    truthbits_cmd_from_text(t, bits[0], cmd, sizeof(cmd)); return eval_truthbits(cmd, out);
  }
  if ((has(t, "kmap") || has(t, "karnaugh") || has(t, "minterm") || has(t, "minterms") ||
       has(t, "dontcare") || has(t, "don'tcare") || (has(t, "dont") && has(t, "care")) ||
       (has(t, "don't") && has(t, "care")) || has(t, "dc")) && make_minterm_cmd(input, cmd, sizeof(cmd))) {
    return eval_minterms(cmd, out);
  }
  if (nv == 0 && ((has(compact, "nand") && (has(compact, "only") || has(compact, "form") || has(compact, "convert"))) || starts(compact, "nandform") || starts(compact, "onlynand"))) {
    if (make_gate_form_cmd(input, true, cmd, sizeof(cmd))) return eval_gate_form(cmd, out);
  }
  if (nv == 0 && ((has(compact, "nor") && (has(compact, "only") || has(compact, "form") || has(compact, "convert"))) || starts(compact, "norform") || starts(compact, "onlynor"))) {
    if (make_gate_form_cmd(input, false, cmd, sizeof(cmd))) return eval_gate_form(cmd, out);
  }
  if (nv == 0 && (starts(compact, "posform") || starts(compact, "cnf") || has(compact, "productofsums"))) {
    const char *e = skip_bool_words(compact); char ce[96];
    bool_clean_tail(e, ce, sizeof(ce));
    sprintf(cmd, "posform(%s)", ce); return eval_posform(cmd, out);
  }
  if ((nv == 0 || has(compact, "simplify") || has(compact, "boolean") || has(compact, "logic")) &&
      !has(compact, "prove") && !has(compact, "show") &&
      (has(compact, "simplify") || has(compact, "truth") || has(compact, "boolean") || has(compact, "logic")) &&
      make_named_bool_rhs_cmd(compact, has(compact, "truth") ? "truth" : "bool", cmd, sizeof(cmd))) {
    return eval_bool(cmd, out);
  }
  if (nv == 0 && has(compact, "truth") && has(compact, "=") &&
      !has(compact, "prove") && !has(compact, "show")) {
    const char *eq = strchr(compact, '=');
    if (eq && eq[1]) {
      char ce[96], ne[96];
      bool_clean_tail(eq + 1, ce, sizeof(ce));
      bool_arg_for_cmd(ce, ne, sizeof(ne));
      sprintf(cmd, "truth(%s)", ne);
      return eval_bool(cmd, out);
    }
  }
  if (nv == 0 && !has(compact, "orc") && !has(compact, "+c") &&
      (has(compact, "demorgan") || has(compact, "not(aandb)") || has(compact, "not(a*b)")) &&
      (has(compact, "notaorb") || has(compact, "notaornotb") || has(compact, "="))) {
    sprintf(cmd, "boolprove((A&B)',A'+B')");
    return eval_bool_prove(cmd, out);
  }
  if (nv == 0 && (has(compact, "equals") || has(compact, "isequalto") || has(compact, "isequivalentto") || has(compact, "equivalentto"))) {
    const char *e = skip_bool_words(compact);
    const char *eq = strstr(e, "isequalto"); int elen = 9;
    const char *eq2 = strstr(e, "isequivalentto");
    if (eq2 && (!eq || eq2 < eq)) { eq = eq2; elen = 14; }
    if (!eq) { eq = strstr(e, "equivalentto"); elen = 12; }
    if (!eq) { eq = strstr(e, "equals"); elen = 6; }
    const char *that = strstr(e, "that");
    if (eq && that && that < eq) e = that + 4;
    if (eq && eq > e && eq[elen]) {
      char lhs[48], rhs[48], clhs[48], crhs[48], nlhs[48], nrhs[48]; int li = 0, ri = 0;
      for (const char *p = e; p < eq && li < 47; ++p) lhs[li++] = *p;
      lhs[li] = 0;
      for (const char *p = eq + elen; *p && ri < 47; ++p) rhs[ri++] = *p;
      rhs[ri] = 0;
      bool_clean_tail(lhs, clhs, sizeof(clhs));
      bool_clean_tail(rhs, crhs, sizeof(crhs));
      bool_arg_for_cmd(clhs, nlhs, sizeof(nlhs));
      bool_arg_for_cmd(crhs, nrhs, sizeof(nrhs));
      sprintf(cmd, "boolprove(%s,%s)", nlhs, nrhs); return eval_bool_prove(cmd, out);
    }
  }
  if (nv == 0 && has(compact, "=")) {
    const char *e = skip_bool_words(compact);
    const char *eq = strchr(e, '=');
    const char *that = strstr(e, "that");
    if (eq && that && that < eq) e = that + 4;
    if (eq && eq > e && eq[1]) {
      char lhs[48], rhs[48], clhs[48], crhs[48], nlhs[48], nrhs[48]; int li = 0, ri = 0;
      for (const char *p = e; p < eq && li < 47; ++p) lhs[li++] = *p;
      lhs[li] = 0;
      for (const char *p = eq + 1; *p && ri < 47; ++p) rhs[ri++] = *p;
      rhs[ri] = 0;
      bool_clean_tail(lhs, clhs, sizeof(clhs));
      bool_clean_tail(rhs, crhs, sizeof(crhs));
      bool_arg_for_cmd(clhs, nlhs, sizeof(nlhs));
      bool_arg_for_cmd(crhs, nrhs, sizeof(nrhs));
      sprintf(cmd, "boolprove(%s,%s)", nlhs, nrhs); return eval_bool_prove(cmd, out);
    }
  }
  if ((nv == 0 || has(compact, "simplify") || has(compact, "boolean") || has(compact, "logic")) &&
      (has(compact, "nand") || has(compact, "nor") || has(compact, "xor") || has(compact, "and") || has(compact, "or") || has(compact, "not") || has(compact, "+") || has(compact, "*") || has(compact, "'") || has(compact, ","))) {
    const char *e = skip_bool_words(compact); char ce[96], ne[96];
    bool_clean_tail(e, ce, sizeof(ce));
    bool_arg_for_cmd(ce, ne, sizeof(ne));
    sprintf(cmd, "%s(%s)", has(compact, "truth") ? "truth" : "bool", ne); return eval_bool(cmd, out);
  }
  if (has(compact, "bigo") || has(compact, "big-o") || has(compact, "complexity") || has(compact, "timecomplexity")) {
    int n = add(out, 0, "Count how the number of operations grows with input size n.");
    if ((has(compact, "nested") || has(compact, "inner") || has(compact, "fori")) && has(compact, "loop") &&
        (has(compact, "from1ton") || has(compact, "1ton") || has(compact, "n")) && has(compact, "j")) {
      n = add(out, n, "Outer loop runs n times.");
      n = add(out, n, "Inner loop runs n times for each outer iteration.");
      n = add(out, n, "total operations proportional to n*n = n^2");
      return add(out, n, "Big O = O(n^2)");
    }
    if (has(compact, "binarysearch") || (has(compact, "halve") || has(compact, "halves") || has(compact, "divideandconquer"))) {
      n = add(out, n, "The problem size is halved each step.");
      return add(out, n, "Big O = O(log n)");
    }
    if (has(compact, "single") && has(compact, "loop")) {
      n = add(out, n, "A single loop over n items grows linearly.");
      return add(out, n, "Big O = O(n)");
    }
    if (has(compact, "constant")) return add(out, n, "Big O = O(1)");
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
  n = eval_posform(s, out); if (n) return n;
  n = eval_truthbits(s, out); if (n) return n;
  n = eval_minterms(s, out); if (n) return n;
  n = eval_bool_prove(s, out); if (n) return n;
  n = eval_bool(s, out); if (n) return n;
  n = eval_free_text(input, s, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "bin hex den convert twos twosdec twosadd twossub signmag signmagdec fixed fixedenc parity repeatenc repeatdec shift arithshift xorbits andbits orbits notbits hamming checksum checkdigit rpn");
  n = add(out, n, "floatdec floatadd floatsub floatmul floatdiv floatrange floatbitsadd normal image sound bitrate transfer");
  return add(out, n, "compress dictcompress huffman rle records sqlselect sqlcount hashmod hashlinear addressspace addressbits chars ascii unicode stack queue preorder inorder postorder dijkstra fsm fsmout binarysearch bubblesort selectionsort mergesort bool truth truthbits minterms maxterms kmap kmapdc posform nandform norform");
}

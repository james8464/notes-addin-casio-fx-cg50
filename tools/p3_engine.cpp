#include "p3_engine.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int add(char out[P3_MAX_LINES][P3_LINE_LEN], int n, const char *fmt, ...) {
  if (n >= P3_MAX_LINES) return n;
  va_list ap; va_start(ap, fmt);
  vsnprintf(out[n], P3_LINE_LEN, fmt, ap);
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
    if (isalnum(c) || c == '.' || c == '-') out[j++] = (char)tolower(c);
    else out[j++] = ',';
  }
  out[j] = 0;
}

static bool starts(const char *s, const char *p) { return strncmp(s, p, strlen(p)) == 0; }
static bool has(const char *s, const char *p) { return strstr(s, p) != 0; }

static bool starts2(const char *s, const char *a, const char *b) {
  return starts(s, a) || starts(s, b);
}

static bool starts3(const char *s, const char *a, const char *b, const char *c) {
  return starts(s, a) || starts(s, b) || starts(s, c);
}

static double num(const char *s) {
  return strtod(s, 0);
}

static double pwr(double a, int e) {
  double r = 1; int n = e < 0 ? -e : e;
  while (n--) r *= a;
  return e < 0 ? 1 / r : r;
}

static double root(double x) {
  if (x <= 0) return 0;
  double r = x > 1 ? x : 1;
  for (int i = 0; i < 16; ++i) r = 0.5 * (r + x / r);
  return r;
}

static double nth_root(double x, int n) {
  if (x <= 0 || n <= 0) return 0;
  double lo = 0, hi = x > 1 ? x : 1;
  for (int i = 0; i < 48; ++i) {
    double mid = (lo + hi) / 2.0;
    if (pwr(mid, n) < x) lo = mid; else hi = mid;
  }
  return (lo + hi) / 2.0;
}

static double exp_approx(double x) {
  double term = 1, sum = 1;
  for (int i = 1; i <= 18; ++i) { term *= x / i; sum += term; }
  return sum;
}

static double ln_approx(double x) {
  if (x <= 0) return 0;
  double y = x > 1 ? 1 : -1;
  for (int i = 0; i < 14; ++i) {
    double e = exp_approx(y);
    if (e == 0) break;
    y += (x - e) / e;
  }
  return y;
}

static double floor_num(double x) {
  long long i = (long long)x;
  return (x < 0 && (double)i != x) ? (double)(i - 1) : (double)i;
}

static double ceil_num(double x) {
  long long i = (long long)x;
  return (x > 0 && (double)i != x) ? (double)(i + 1) : (double)i;
}

static double sine(double x) {
  while (x > M_PI) x -= 2 * M_PI;
  while (x < -M_PI) x += 2 * M_PI;
  double x2 = x * x;
  return x * (1 - x2 / 6 + x2 * x2 / 120 - x2 * x2 * x2 / 5040);
}

static double cosine(double x) { return sine(M_PI / 2 - x); }

static double arctan(double z) {
  if (z < 0) return -arctan(-z);
  if (z > 1) return M_PI / 2 - arctan(1 / z);
  if (z > 0.5) return M_PI / 4 + arctan((z - 1) / (z + 1));
  double z2 = z * z, p = z, r = z;
  p *= z2; r -= p / 3;
  p *= z2; r += p / 5;
  p *= z2; r -= p / 7;
  p *= z2; r += p / 9;
  p *= z2; r -= p / 11;
  return r;
}

static double deg_sine(double deg) {
  while (deg < 0) deg += 360;
  while (deg >= 360) deg -= 360;
  if (deg < 1e-9 || deg > 360-1e-9 || (deg > 180-1e-9 && deg < 180+1e-9)) return 0;
  if (deg > 90-1e-9 && deg < 90+1e-9) return 1;
  if (deg > 270-1e-9 && deg < 270+1e-9) return -1;
  return sine(deg*M_PI/180.0);
}

static double deg_cosine(double deg) {
  while (deg < 0) deg += 360;
  while (deg >= 360) deg -= 360;
  if (deg < 1e-9 || deg > 360-1e-9) return 1;
  if (deg > 180-1e-9 && deg < 180+1e-9) return -1;
  if ((deg > 90-1e-9 && deg < 90+1e-9) || (deg > 270-1e-9 && deg < 270+1e-9)) return 0;
  return cosine(deg*M_PI/180.0);
}

static double deg_arcsine(double x) {
  if (x >= 1) return 90;
  if (x <= -1) return -90;
  return arctan(x/root(1-x*x))*180.0/M_PI;
}

static double deg_atan2_approx(double y, double x) {
  if (x > -1e-9 && x < 1e-9) return y >= 0 ? 90 : -90;
  double a = arctan(y / x) * 180.0 / M_PI;
  if (x < 0 && y >= 0) a += 180;
  if (x < 0 && y < 0) a -= 180;
  return a;
}

static int args(const char *s, char a[][48], int maxa) {
  const char *l = strchr(s, '('), *r = strrchr(s, ')');
  if (!l || !r || r <= l) return 0;
  int n = 0, j = 0, depth = 0;
  for (const char *p = l + 1; p < r && n < maxa; ++p) {
    if (*p == '(') depth++;
    if (*p == ')') depth--;
    if (*p == ',' && depth == 0) { a[n][j] = 0; n++; j = 0; }
    else if (j < 47) a[n][j++] = *p;
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

static bool kv(char a[][48], int na, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; i < na; ++i) if (strncmp(a[i], name, nl) == 0 && a[i][nl] == '=') {
    *v = num(a[i] + nl + 1); return true;
  }
  return false;
}

static double read_num(const char *s) {
  return strtod(s, 0);
}

static bool coeff_of_t_power_before(const char *start, const char *end, int pow, double *coef) {
  bool found = false;
  *coef = 0;
  for (const char *p = start; p && p < end && *p; ++p) {
    if (*p != 't') continue;
    int got = 1;
    if (p[1] == '^') got = (int)read_num(p + 2);
    if (got != pow) continue;
    const char *q = p - 1;
    if (q >= start && *q == '*') --q;
    const char *e = q + 1;
    while (q >= start && (isdigit((unsigned char)*q) || *q == '.')) --q;
    const char *b = q + 1;
    double c = b < e ? read_num(b) : 1;
    if (q >= start && *q == '-') c = -c;
    *coef += c;
    found = true;
  }
  return found;
}

static bool coded_cmd_from_text(const char *compact, const double v[], int nv, char *cmd, int cap) {
  const char *p = strstr(compact, "y=(x");
  if (!p || nv < 2) return false;
  p += 4;
  double a = 0, b = 1;
  if (*p == '-') { a = strtod(p + 1, (char **)&p); }
  else if (*p == '+') { a = -strtod(p + 1, (char **)&p); }
  const char *div = strstr(p, ")/");
  if (!div) return false;
  b = strtod(div + 2, 0);
  if (b == 0) return false;
  double mean = v[nv - 2], sd = v[nv - 1];
  bool wants_y = strstr(compact, "findmeanofy") || strstr(compact, "findthemeanofy") ||
                 strstr(compact, "findmeanandsdofy") || strstr(compact, "findthemeanandsdofy") ||
                 strstr(compact, "findmeanandstandarddeviationofy") ||
                 strstr(compact, "meanofthecoded") || strstr(compact, "codedmean");
  (void)cap;
  sprintf(cmd, wants_y ? "code(%.10g,%.10g,%.10g,%.10g)" : "uncode(%.10g,%.10g,%.10g,%.10g)",
          mean, sd, a, b);
  return true;
}

static bool grouped_class_freq_cmd(const char *t, char *cmd, int cap) {
  double mid[12], freq[12]; int mc = 0, fc = 0;
  bool after_freq = false;
  for (int i = 0; t[i];) {
    while (t[i] == ',') ++i;
    char w[32]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0]) continue;
    if (strstr(w, "frequenc")) { after_freq = true; continue; }
    char *dash = strchr(w, '-');
    if (dash && dash != w && isdigit((unsigned char)dash[1]) && mc < 12) {
      *dash = 0;
      mid[mc++] = (read_num(w) + read_num(dash + 1)) / 2.0;
      continue;
    }
    if (after_freq && (isdigit((unsigned char)w[0]) || w[0] == '-') && fc < 12) {
      freq[fc++] = read_num(w);
    }
  }
  if (mc < 1 || fc < mc) return false;
  int p = sprintf(cmd, "groupmean(");
  for (int i = 0; i < mc && p < cap - 32; ++i)
    p += sprintf(cmd + p, "%s%.10g,%.10g", i ? "," : "", mid[i], freq[i]);
  if (p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
  return true;
}

static bool discrete_table_cmd(const char *t, char *cmd, int cap) {
  double xs[10], ps[10]; int nx = 0, np = 0, mode = 0;
  for (int i = 0; t[i];) {
    while (t[i] == ',') ++i;
    char w[32]; int j = 0;
    while (t[i] && t[i] != ',' && j + 1 < (int)sizeof(w)) w[j++] = t[i++];
    w[j] = 0;
    if (!w[0]) continue;
    if (!strcmp(w, "x") || !strcmp(w, "values") || !strcmp(w, "value")) { mode = 1; continue; }
    if (!strcmp(w, "p") || !strcmp(w, "prob") || !strcmp(w, "probability") || !strcmp(w, "probabilities")) { mode = 2; continue; }
    if ((w[0] == '-' || isdigit((unsigned char)w[0])) && mode == 1 && nx < 10) xs[nx++] = read_num(w);
    else if ((w[0] == '-' || isdigit((unsigned char)w[0])) && mode == 2 && np < 10) ps[np++] = read_num(w);
  }
  int m = nx < np ? nx : np;
  if (m < 2) return false;
  int p = sprintf(cmd, "discrete(");
  for (int i = 0; i < m && p < cap - 32; ++i)
    p += sprintf(cmd + p, "%s%.10g,%.10g", i ? "," : "", xs[i], ps[i]);
  if (p >= cap - 2) return false;
  cmd[p++] = ')'; cmd[p] = 0;
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

static bool word_num(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0, q = i;
    while (j < nl && s[q]) {
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '_' || s[q] == '-') ++q;
      if (tolower((unsigned char)s[q]) != name[j]) break;
      ++j; ++q;
    }
    if (j != nl || isalnum((unsigned char)s[q])) continue;
    while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    if (tolower((unsigned char)s[q]) == 'i' && tolower((unsigned char)s[q+1]) == 's' &&
        !isalnum((unsigned char)s[q+2])) {
      q += 2;
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    }
    if (tolower((unsigned char)s[q]) == 'o' && tolower((unsigned char)s[q+1]) == 'f' &&
        !isalnum((unsigned char)s[q+2])) {
      q += 2;
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    }
    if (tolower((unsigned char)s[q]) == 'h' && tolower((unsigned char)s[q+1]) == 'a' &&
        tolower((unsigned char)s[q+2]) == 's' && !isalnum((unsigned char)s[q+3])) {
      q += 3;
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    }
    if (tolower((unsigned char)s[q]) == 'w' && tolower((unsigned char)s[q+1]) == 'a' &&
        tolower((unsigned char)s[q+2]) == 's' && !isalnum((unsigned char)s[q+3])) {
      q += 3;
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    }
    if (s[q] != '-' && !isdigit((unsigned char)s[q])) continue;
    *v = read_num(s + q);
    return true;
  }
  return false;
}

static double alpha_from_text(const char *input, const char *t, const char *c, const double *v, int nv, double skip1, double skip2) {
  double alpha = 0;
  if (word_num(input,"percent",&alpha) || word_num(input,"significance",&alpha) || word_num(input,"level",&alpha)) {
    return alpha > 1 ? alpha / 100.0 : alpha;
  }
  for (int i = 0; i < nv; ++i) {
    double d1 = v[i] - skip1, d2 = v[i] - skip2;
    if (d1 < 0) d1 = -d1;
    if (d2 < 0) d2 = -d2;
    if (d1 < 1e-9 || d2 < 1e-9) continue;
    if ((has(c, "%") || has(t, "percent") || has(t, "level")) && v[i] >= 1 && v[i] <= 10) return v[i] / 100.0;
    if (v[i] > 0 && v[i] < 1) return v[i];
  }
  return 0.05;
}

static bool word_num_with_t(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0, q = i;
    while (j < nl && s[q]) {
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '_' || s[q] == '-') ++q;
      if (tolower((unsigned char)s[q]) != name[j]) break;
      ++j; ++q;
    }
    if (j != nl || isalnum((unsigned char)s[q])) continue;
    while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    if ((tolower((unsigned char)s[q]) == 't' && !isalnum((unsigned char)s[q+1])) ||
        (tolower((unsigned char)s[q]) == 't' && tolower((unsigned char)s[q+1]) == 'i' &&
         tolower((unsigned char)s[q+2]) == 'm' && tolower((unsigned char)s[q+3]) == 'e' &&
         !isalnum((unsigned char)s[q+4]))) {
      q += tolower((unsigned char)s[q+1]) == 'i' ? 4 : 1;
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    }
    if (s[q] != '-' && !isdigit((unsigned char)s[q])) continue;
    *v = read_num(s + q);
    return true;
  }
  return false;
}

static int scan_t_values(const char *s, double vals[], int maxv) {
  int n = 0;
  for (int i = 0; s && s[i] && n < maxv; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    char ch = (char)tolower((unsigned char)s[i]);
    int q = i;
    if (ch == 't' && !isalpha((unsigned char)s[i+1])) {
      q = i + 1;
    } else if (ch == 't' && tolower((unsigned char)s[i+1]) == 'i' &&
               tolower((unsigned char)s[i+2]) == 'm' &&
               tolower((unsigned char)s[i+3]) == 'e' &&
               !isalpha((unsigned char)s[i+4])) {
      q = i + 4;
    } else {
      continue;
    }
    while (s[q] == ' ' || s[q] == '\t' || s[q] == '=' || s[q] == ':' || s[q] == ',') ++q;
    if (s[q] != '-' && !isdigit((unsigned char)s[q])) continue;
    vals[n++] = read_num(s + q);
  }
  return n;
}

static bool first_num_after_word(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0, q = i;
    while (j < nl && s[q]) {
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '_' || s[q] == '-') ++q;
      if (tolower((unsigned char)s[q]) != name[j]) break;
      ++j; ++q;
    }
    if (j != nl || isalnum((unsigned char)s[q])) continue;
    int limit = q + 40;
    bool saw_at = false;
    for (; s[q] && q < limit; ++q) {
      if ((q == 0 || !isalnum((unsigned char)s[q-1])) &&
          tolower((unsigned char)s[q]) == 'a' && tolower((unsigned char)s[q+1]) == 't' &&
          !isalnum((unsigned char)s[q+2])) saw_at = true;
      if (s[q] == '-' || isdigit((unsigned char)s[q])) {
        if (!saw_at) return false;
        *v = read_num(s + q);
        return true;
      }
    }
  }
  return false;
}

static bool prev_word_num(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0, q = i;
    while (j < nl && s[q]) {
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '_' || s[q] == '-') ++q;
      if (tolower((unsigned char)s[q]) != name[j]) break;
      ++j; ++q;
    }
    if (j != nl || isalnum((unsigned char)s[q])) continue;
    int k = i - 1;
    while (k >= 0 && (s[k] == ' ' || s[k] == '\t')) --k;
    while (k >= 0 && isalpha((unsigned char)s[k])) --k;
    while (k >= 0 && (s[k] == ' ' || s[k] == '\t')) --k;
    int end = k;
    while (k >= 0 && (isdigit((unsigned char)s[k]) || s[k] == '.')) --k;
    if (end <= k) continue;
    if (k >= 0 && s[k] == '-') --k;
    char b[32]; int len = end - k;
    if (len <= 0 || len >= (int)sizeof(b)) continue;
    memcpy(b, s + k + 1, len); b[len] = 0;
    if (b[0] != '-' && !isdigit((unsigned char)b[0])) continue;
    *v = read_num(b);
    return true;
  }
  return false;
}

static bool num_before_unit(const char *s, const char *unit, double *v) {
  int ul = (int)strlen(unit);
  for (int i = 0; s && s[i]; ++i) {
    int j = 0;
    while (j < ul && s[i+j] && tolower((unsigned char)s[i+j]) == tolower((unsigned char)unit[j])) ++j;
    if (j != ul) continue;
    int k = i - 1;
    while (k >= 0 && isalpha((unsigned char)s[k])) --k;
    while (k >= 0 && (s[k] == ' ' || s[k] == '\t')) --k;
    int end = k;
    while (k >= 0 && (isdigit((unsigned char)s[k]) || s[k] == '.')) --k;
    if (end <= k) continue;
    if (k >= 0 && s[k] == '-') --k;
    char b[32]; int len = end - k;
    if (len <= 0 || len >= (int)sizeof(b)) continue;
    memcpy(b, s + k + 1, len); b[len] = 0;
    if (b[0] != '-' && !isdigit((unsigned char)b[0])) continue;
    *v = read_num(b);
    return true;
  }
  return false;
}

static int scan_load_at_pairs(const char *s, double loads[], double pos[], int maxp) {
  int n = 0;
  for (int i = 0; s && s[i] && n < maxp; ++i) {
    if (tolower((unsigned char)s[i]) != 'n') continue;
    if (i > 0 && isalpha((unsigned char)s[i-1])) continue;
    if (isalpha((unsigned char)s[i+1])) continue;
    int k = i - 1;
    while (k >= 0 && (s[k] == ' ' || s[k] == '\t')) --k;
    int end = k;
    while (k >= 0 && (isdigit((unsigned char)s[k]) || s[k] == '.')) --k;
    if (end <= k) continue;
    char wb[32]; int wlen = end - k;
    if (wlen <= 0 || wlen >= (int)sizeof(wb)) continue;
    memcpy(wb, s + k + 1, wlen); wb[wlen] = 0;
    const char *at = 0;
    for (int q = i + 1; s[q] && q < i + 45; ++q) {
      if (tolower((unsigned char)s[q]) == 'a' && tolower((unsigned char)s[q+1]) == 't' &&
          !isalpha((unsigned char)s[q+2])) { at = s + q + 2; break; }
      if (tolower((unsigned char)s[q]) == 'n') break;
    }
    if (!at) continue;
    while (*at && !isdigit((unsigned char)*at) && *at != '-' && at < s + i + 60) ++at;
    if (!*at || at >= s + i + 60) continue;
    loads[n] = read_num(wb);
    pos[n] = read_num(at);
    ++n;
  }
  return n;
}

static double vec_coeff_before(const char *base, const char *p) {
  const char *q = p - 1;
  while (q >= base && (*q == ' ' || *q == '\t' || *q == ',')) --q;
  const char *end = q + 1;
  while (q >= base && (isdigit((unsigned char)*q) || *q == '.' || *q == '-')) --q;
  const char *start = q + 1;
  if (start >= end) return 1;
  if (start + 1 == end && *start == '-') return -1;
  return read_num(start);
}

static bool vector_after_word(const char *s, const char *word, double *x, double *y) {
  const char *p = strstr(s, word);
  if (!p) return false;
  p += strlen(word);
  const char *pi = strchr(p, 'i');
  const char *pj = strchr(p, 'j');
  if (!pi || !pj || pj < pi) return false;
  *x = vec_coeff_before(p, pi);
  *y = vec_coeff_before(p, pj);
  return true;
}

static int scan_ij_vectors(const char *s, double xs[], double ys[], int maxv) {
  int n = 0;
  const char *seg = s;
  for (const char *p = s; p && *p && n < maxv; ++p) {
    if (tolower((unsigned char)*p) != 'i') continue;
    if ((p > s && isalpha((unsigned char)p[-1])) || isalpha((unsigned char)p[1])) continue;
    const char *pi = p, *pj = 0;
    for (const char *q = pi + 1; *q && q - pi < 32; ++q) {
      if (tolower((unsigned char)*q) != 'j') continue;
      if ((q > s && isalpha((unsigned char)q[-1])) || isalpha((unsigned char)q[1])) continue;
      pj = q;
      break;
    }
    if (!pj) continue;
    xs[n] = vec_coeff_before(seg, pi);
    ys[n] = vec_coeff_before(pi, pj);
    ++n;
    p = pj;
    seg = pj + 1;
  }
  return n;
}

static bool last_label_num(const char *s, const char *name, double *v) {
  int nl = (int)strlen(name);
  bool ok = false;
  for (int i = 0; s && s[i]; ++i) {
    if (i > 0 && isalnum((unsigned char)s[i-1])) continue;
    int j = 0, q = i;
    while (j < nl && s[q]) {
      while (s[q] == ' ' || s[q] == '\t' || s[q] == '_' || s[q] == '-') ++q;
      if (tolower((unsigned char)s[q]) != name[j]) break;
      ++j; ++q;
    }
    if (j != nl) continue;
    while (s[q] == ' ' || s[q] == '\t') ++q;
    if (s[q] != '=') continue;
    ++q;
    while (s[q] == ' ' || s[q] == '\t') ++q;
    if (s[q] == '-' || isdigit((unsigned char)s[q])) { *v = read_num(s + q); ok = true; }
  }
  return ok;
}

static double term_coeff(const char *term, int len) {
  char b[32]; int n = 0;
  for (int i = 0; i < len && n + 1 < (int)sizeof(b); ++i) {
    if (term[i] == '*') break;
    b[n++] = term[i];
  }
  b[n] = 0;
  if (!b[0]) return 1;
  return read_num(b);
}

static bool parse_velocity_quad(const char *input, double *A, double *B, double *C) {
  const char *p = strchr(input, '=');
  p = p ? p + 1 : input;
  char e[160]; int k = 0;
  for (int i = 0; p[i] && k + 1 < (int)sizeof(e); ++i) {
    unsigned char ch = (unsigned char)p[i];
    if (isspace(ch)) continue;
    e[k++] = (char)tolower(ch);
  }
  e[k] = 0;
  char *stop = strstr(e, "find");
  if (stop) *stop = 0;
  stop = strchr(e, '.');
  if (stop) *stop = 0;
  *A = *B = *C = 0;
  bool any = false;
  for (int i = 0; e[i]; ) {
    int sign = 1;
    if (e[i] == '+') ++i;
    else if (e[i] == '-') { sign = -1; ++i; }
    int start = i;
    while (e[i] && e[i] != '+' && e[i] != '-') ++i;
    int len = i - start;
    if (len <= 0) continue;
    const char *t = e + start;
    const char *tp = strchr(t, 't');
    if (tp && tp < t + len) {
      int pre = (int)(tp - t);
      double cf = sign * term_coeff(t, pre);
      if (tp + 1 < t + len && tp[1] == '^') {
        if (tp + 2 >= t + len) return false;
        if (tp[2] == '2') *A += cf;
        else if (tp[2] == '1') *B += cf;
        else return false;
      } else *B += cf;
      any = true;
    } else if (isdigit((unsigned char)t[0]) || t[0] == '.') {
      char tmp[32]; int n = len < 31 ? len : 31;
      memcpy(tmp, t, n); tmp[n] = 0;
      *C += sign * read_num(tmp);
      any = true;
    }
  }
  return any;
}

static bool parse_poly_after_word(const char *input, const char *word, double *A, double *B, double *C) {
  int wl = (int)strlen(word);
  for (int i = 0; input && input[i]; ++i) {
    int j = 0, q = i;
    while (j < wl && input[q] && tolower((unsigned char)input[q]) == word[j]) { ++j; ++q; }
    if (j != wl) continue;
    while (input[q] && input[q] != '-' && input[q] != '+' && input[q] != '.' &&
           !isdigit((unsigned char)input[q]) &&
           !((tolower((unsigned char)input[q]) == 't' || tolower((unsigned char)input[q]) == 'x') &&
             !isalpha((unsigned char)input[q+1]))) ++q;
    if (!input[q]) continue;
    char expr[96]; int k = 0;
    while (input[q] && k + 1 < (int)sizeof(expr)) {
      unsigned char ch = (unsigned char)input[q];
      if (isspace(ch)) { ++q; continue; }
      char lc = (char)tolower(ch);
      if ((lc == 't' || lc == 'x') && isalpha((unsigned char)input[q+1])) break;
      if (!(isdigit(ch) || lc == 't' || lc == 'x' || lc == '+' || lc == '-' || lc == '.' || lc == '^' || lc == '*')) break;
      expr[k++] = lc; ++q;
    }
    expr[k] = 0;
    if (input[q] == '/' || input[q] == '(') return false;
    if (k > 0 && parse_velocity_quad(expr, A, B, C)) return true;
  }
  return false;
}

static bool parse_cubic_poly_after_word(const char *input, const char *word, double *A, double *B, double *C, double *D) {
  int wl = (int)strlen(word);
  for (int i = 0; input && input[i]; ++i) {
    int j = 0, q = i;
    while (j < wl && input[q] && tolower((unsigned char)input[q]) == word[j]) { ++j; ++q; }
    if (j != wl) continue;
    while (input[q] && input[q] != '-' && input[q] != '+' && input[q] != '.' &&
           !isdigit((unsigned char)input[q]) && tolower((unsigned char)input[q]) != 't') ++q;
    if (!input[q]) continue;
    char expr[128]; int k = 0;
    while (input[q] && k + 1 < (int)sizeof(expr)) {
      unsigned char ch = (unsigned char)input[q];
      if (isspace(ch)) { ++q; continue; }
      char lc = (char)tolower(ch);
      if (lc == 't' && isalpha((unsigned char)input[q+1])) break;
      if (!(isdigit(ch) || lc == 't' || lc == '+' || lc == '-' || lc == '.' || lc == '^' || lc == '*')) break;
      expr[k++] = lc; ++q;
    }
    expr[k] = 0;
    *A = *B = *C = *D = 0;
    for (int p = 0; expr[p];) {
      double sign = 1;
      if (expr[p] == '+') ++p;
      else if (expr[p] == '-') { sign = -1; ++p; }
      double coef = 0; bool hc = false;
      while (isdigit((unsigned char)expr[p]) || expr[p] == '.') {
        hc = true;
        coef = read_num(expr + p);
        while (isdigit((unsigned char)expr[p]) || expr[p] == '.') ++p;
        break;
      }
      while (expr[p] == '*') ++p;
      int power = 0;
      if (expr[p] == 't') {
        ++p; power = 1;
        if (expr[p] == '^' && isdigit((unsigned char)expr[p+1])) { ++p; power = expr[p++] - '0'; }
      }
      if (!hc && power > 0) coef = 1;
      coef *= sign;
      if (power == 3) *A += coef;
      else if (power == 2) *B += coef;
      else if (power == 1) *C += coef;
      else *D += coef;
    }
    if ((*A > 1e-9 || *A < -1e-9) || (*B > 1e-9 || *B < -1e-9) ||
        (*C > 1e-9 || *C < -1e-9) || (*D > 1e-9 || *D < -1e-9)) return true;
  }
  return false;
}

static bool parse_quartic_poly_after_word(const char *input, const char *word, double *A, double *B, double *C, double *D, double *E) {
  int wl = (int)strlen(word);
  for (int i = 0; input && input[i]; ++i) {
    int j = 0, q = i;
    while (j < wl && input[q] && tolower((unsigned char)input[q]) == word[j]) { ++j; ++q; }
    if (j != wl) continue;
    while (input[q] && input[q] != '-' && input[q] != '+' && input[q] != '.' &&
           !isdigit((unsigned char)input[q]) && tolower((unsigned char)input[q]) != 't') ++q;
    if (!input[q]) continue;
    char expr[144]; int k = 0;
    while (input[q] && k + 1 < (int)sizeof(expr)) {
      unsigned char ch = (unsigned char)input[q];
      if (isspace(ch)) { ++q; continue; }
      char lc = (char)tolower(ch);
      if (lc == 't' && isalpha((unsigned char)input[q+1])) break;
      if (!(isdigit(ch) || lc == 't' || lc == '+' || lc == '-' || lc == '.' || lc == '^' || lc == '*')) break;
      expr[k++] = lc; ++q;
    }
    expr[k] = 0; *A = *B = *C = *D = *E = 0;
    for (int p = 0; expr[p];) {
      double sign = 1;
      if (expr[p] == '+') ++p;
      else if (expr[p] == '-') { sign = -1; ++p; }
      double coef = 0; bool hc = false;
      while (isdigit((unsigned char)expr[p]) || expr[p] == '.') {
        hc = true; coef = read_num(expr + p);
        while (isdigit((unsigned char)expr[p]) || expr[p] == '.') ++p;
        break;
      }
      while (expr[p] == '*') ++p;
      int power = 0;
      if (expr[p] == 't') {
        ++p; power = 1;
        if (expr[p] == '^' && isdigit((unsigned char)expr[p+1])) { ++p; power = expr[p++] - '0'; }
      }
      if (!hc && power > 0) coef = 1;
      coef *= sign;
      if (power == 4) *A += coef;
      else if (power == 3) *B += coef;
      else if (power == 2) *C += coef;
      else if (power == 1) *D += coef;
      else *E += coef;
    }
    if ((*A > 1e-9 || *A < -1e-9) || (*B > 1e-9 || *B < -1e-9) ||
        (*C > 1e-9 || *C < -1e-9) || (*D > 1e-9 || *D < -1e-9) ||
        (*E > 1e-9 || *E < -1e-9)) return true;
  }
  return false;
}

static bool parse_regression_line(const char *compact, double *m, double *b) {
  const char *p = 0;
  const char *isy = strstr(compact, "isy=");
  if (isy) p = isy + 2;
  for (const char *q = compact; (q = strstr(q, "y=")) != 0; ++q) {
    if (p) break;
    if (q == compact || !isalpha((unsigned char)q[-1])) { p = q; break; }
  }
  if (!p) return false;
  p += 2;
  char *endp = 0;
  double first = strtod(p, &endp);
  if (endp != p && (*endp == '+' || *endp == '-')) {
    const char *q = endp;
    double slope2 = strtod(q, &endp);
    if (endp != q) {
      q = endp;
      while (*q == '*') ++q;
      if (*q == 'x') {
        *m = slope2;
        *b = first;
        return true;
      }
    }
  }
  double slope = strtod(p, &endp);
  if (endp == p) {
    if (*p == 'x') slope = 1;
    else return false;
  }
  p = endp;
  while (*p == '*') ++p;
  if (*p != 'x') return false;
  ++p;
  double intercept = 0;
  if (*p == '+' || *p == '-') intercept = strtod(p, &endp);
  *m = slope; *b = intercept;
  return true;
}

static bool parse_force_x_poly(const char *input, double *A, double *B, double *C, double *D) {
  const char *p = strchr(input, '=');
  if (!p) return false;
  char expr[128]; int k = 0; ++p;
  while (*p && *p != ')' && *p != ',' && k + 1 < (int)sizeof(expr)) {
    unsigned char ch = (unsigned char)*p;
    if (isspace(ch) || ch == '(') { ++p; continue; }
    char lc = (char)tolower(ch);
    if (!(isdigit(ch) || lc == 'x' || lc == '+' || lc == '-' || lc == '.' || lc == '^' || lc == '*')) break;
    expr[k++] = lc; ++p;
  }
  expr[k] = 0;
  if (*p == '/' || *p == '(') return false;
  *A = *B = *C = *D = 0;
  for (int i = 0; expr[i];) {
    double sign = 1; if (expr[i] == '+') ++i; else if (expr[i] == '-') { sign = -1; ++i; }
    double coef = 0; bool hc = false;
    while (isdigit((unsigned char)expr[i]) || expr[i] == '.') {
      hc = true; coef = read_num(expr + i);
      while (isdigit((unsigned char)expr[i]) || expr[i] == '.') ++i;
      break;
    }
    while (expr[i] == '*') ++i;
    int power = 0;
    if (expr[i] == 'x') {
      ++i; power = 1;
      if (expr[i] == '^' && isdigit((unsigned char)expr[i+1])) { ++i; power = expr[i++] - '0'; }
    }
    if (!hc && power > 0) coef = 1;
    coef *= sign;
    if (power == 3) *A += coef;
    else if (power == 2) *B += coef;
    else if (power == 1) *C += coef;
    else *D += coef;
  }
  return (*A > 1e-9 || *A < -1e-9) || (*B > 1e-9 || *B < -1e-9) ||
         (*C > 1e-9 || *C < -1e-9) || (*D > 1e-9 || *D < -1e-9);
}

static bool extract_xy_lists_after_words(const char *input, double *xs, double *ys, int *n) {
  char t[220]; raw_clean(input, t, sizeof(t));
  const char *xp = strstr(t, "x,values,");
  int xskip = 9;
  if (!xp) { xp = strstr(t, "x,value,"); xskip = 8; }
  const char *yp = xp ? strstr(xp + xskip, "y,values,") : 0;
  int yskip = 9;
  if (!yp && xp) { yp = strstr(xp + xskip, "y,value,"); yskip = 8; }
  if (!xp || !yp) {
    xp = strstr(t, "x,");
    yp = strstr(t, ",y,");
    xskip = 2; yskip = 3;
  }
  if (!xp || !yp || yp <= xp) return false;
  double xv[16], yv[16]; int nx = 0, ny = 0;
  const char *p = xp + xskip;
  while (p < yp && nx < 16) {
    if (*p == '-' || *p == '.' || isdigit((unsigned char)*p)) {
      char *e = 0; double val = strtod(p, &e);
      if (e != p) { xv[nx++] = val; p = e; continue; }
    }
    ++p;
  }
  p = yp + yskip;
  while (*p && ny < 16) {
    if (*p == '-' || *p == '.' || isdigit((unsigned char)*p)) {
      char *e = 0; double val = strtod(p, &e);
      if (e != p) { yv[ny++] = val; p = e; continue; }
    }
    ++p;
  }
  if (nx < 2 || ny < nx) return false;
  for (int i = 0; i < nx; ++i) { xs[i] = xv[i]; ys[i] = yv[i]; }
  *n = nx;
  return true;
}

static bool extract_two_lists_around_and(const char *input, double *xs, double *ys, int *n) {
  char t[240]; raw_clean(input, t, sizeof(t));
  const char *ap = strstr(t, ",and,");
  if (!ap) return false;
  char left[160], right[160];
  int llen = (int)(ap - t);
  if (llen >= (int)sizeof(left)) llen = (int)sizeof(left) - 1;
  memcpy(left, t, llen); left[llen] = 0;
  strncpy(right, ap + 5, sizeof(right) - 1); right[sizeof(right) - 1] = 0;
  double lx[16], ry[16];
  int nx = scan_nums(left, lx, 16), ny = scan_nums(right, ry, 16);
  if (nx < 2 || ny != nx) return false;
  for (int i = 0; i < nx; ++i) { xs[i] = lx[i]; ys[i] = ry[i]; }
  *n = nx;
  return true;
}

static bool extract_point_pairs_after_word(const char *input, double *xs, double *ys, int *n) {
  char t[260]; raw_clean(input, t, sizeof(t));
  const char *p = strstr(t, "points,");
  if (!p) p = strstr(t, "pairs,");
  if (!p) p = strstr(t, "data,");
  if (!p) return false;
  char seg[220];
  strncpy(seg, p, sizeof(seg) - 1); seg[sizeof(seg) - 1] = 0;
  const char *stops[] = { ",estimate,", ",predict,", ",when,", ",find," };
  for (int i = 0; i < 4; ++i) {
    char *q = strstr(seg + 6, stops[i]);
    if (q) *q = 0;
  }
  double vals[32];
  int nv = scan_nums(seg, vals, 32);
  if (nv < 4 || (nv & 1)) return false;
  int count = nv / 2;
  if (count > 16) count = 16;
  for (int i = 0; i < count; ++i) { xs[i] = vals[2*i]; ys[i] = vals[2*i + 1]; }
  *n = count;
  return true;
}

static bool parse_linear_inside(const char *p, char var, double *b, double *d, const char **endp) {
  *b = 1; *d = 0;
  if (*p == var) {
    *b = 1; ++p;
    if (*p == '+' || *p == '-') {
      *d = read_num(p);
      if (*p == '+' || *p == '-') ++p;
      while (*p == '.' || isdigit((unsigned char)*p)) ++p;
    }
  } else if (*p == '-' || *p == '.' || isdigit((unsigned char)*p)) {
    double first = read_num(p);
    if (*p == '-' || *p == '+') ++p;
    while (*p == '.' || isdigit((unsigned char)*p)) ++p;
    if (*p == '*') ++p;
    if (*p == var) {
      *b = first; ++p;
      if (*p == '+' || *p == '-') {
        *d = read_num(p);
        if (*p == '+' || *p == '-') ++p;
        while (*p == '.' || isdigit((unsigned char)*p)) ++p;
      }
    } else if (*p == '+' || *p == '-') {
      *d = first;
      bool neg = *p == '-';
      ++p;
      double coef = 1;
      if (*p == '.' || isdigit((unsigned char)*p)) {
        coef = read_num(p);
        while (*p == '.' || isdigit((unsigned char)*p)) ++p;
        if (*p == '*') ++p;
      }
      if (*p != var) return false;
      *b = neg ? -coef : coef;
      ++p;
    } else return false;
  } else return false;
  if (endp) *endp = p;
  return *b != 0;
}

static bool parse_linear_den_power(const char *c, char var, double *b, double *d, int *pow) {
  const char *p = strstr(c, "/(");
  if (!p) return false;
  p += 2;
  const char *e = 0;
  if (!parse_linear_inside(p, var, b, d, &e)) return false;
  p = e;
  if (*p != ')') return false;
  ++p;
  *pow = 1;
  if (*p == '^') *pow = (int)read_num(p + 1);
  return *pow >= 1 && *b != 0;
}

static double linear_den_antideriv(double k, double b, double d, int pow, double x) {
  if (pow == 1) {
    double z = b*x + d;
    if (z < 0) z = -z;
    return (k / b) * ln_approx(z);
  }
  return k * pwr(b*x + d, 1 - pow) / (b * (1 - pow));
}

static double linear_den_x_antideriv(double k, double b, double d, int pow, double x) {
  double u = b*x + d, ab = u < 0 ? -u : u;
  if (pow == 1) return k * (u - d * ln_approx(ab)) / (b*b);
  if (pow == 2) return k * (ln_approx(ab) + d/u) / (b*b);
  double a = pwr(u, 2 - pow) / (2 - pow);
  double q = pwr(u, 1 - pow) / (1 - pow);
  return k * (a - d*q) / (b*b);
}

static double linear_power_antideriv(double k, double b, double d, int pow, double x) {
  return k * pwr(b*x + d, pow + 1) / (b * (pow + 1));
}

static double trailing_constant_after_linear_den(const char *c, char var, int pow) {
  const char *p = strstr(c, "/(");
  if (!p) return 0;
  p = strchr(p + 2, ')');
  if (!p) return 0;
  ++p;
  if (*p == '^') {
    ++p;
    while (isdigit((unsigned char)*p)) ++p;
  } else if (pow != 1) return 0;
  (void)var;
  if (*p == '+' || *p == '-') return read_num(p);
  return 0;
}

static void fmt_linear_var(char *buf, int cap, double b, char var, double d) {
  (void)cap;
  char lhs[32];
  if (fabs(b - 1.0) < 1e-9) sprintf(lhs, "%c", var);
  else if (fabs(b + 1.0) < 1e-9) sprintf(lhs, "-%c", var);
  else sprintf(lhs, "%.10g%c", b, var);
  if (fabs(d) < 1e-9) sprintf(buf, "%s", lhs);
  else sprintf(buf, "%s%+.10g", lhs, d);
}

static void average_ranks(const double *vals, int n, double *ranks) {
  for (int i = 0; i < n; ++i) {
    int less = 0, equal = 0;
    for (int j = 0; j < n; ++j) {
      if (vals[j] < vals[i]) less++;
      double d = vals[j] > vals[i] ? vals[j] - vals[i] : vals[i] - vals[j];
      if (d < 1e-9) equal++;
    }
    ranks[i] = less + (equal + 1) / 2.0;
  }
}

static void paired_sums(const double *xs, const double *ys, int n,
                        double *sx, double *sy, double *sx2, double *sy2, double *sxy) {
  *sx = *sy = *sx2 = *sy2 = *sxy = 0;
  for (int i = 0; i < n; ++i) {
    *sx += xs[i]; *sy += ys[i];
    *sx2 += xs[i]*xs[i]; *sy2 += ys[i]*ys[i];
    *sxy += xs[i]*ys[i];
  }
}

static int add_raw_pmcc_lines(char out[P3_MAX_LINES][P3_LINE_LEN], const double *xs, const double *ys, int count) {
  double sx, sy, sx2, sy2, sxy;
  paired_sums(xs, ys, count, &sx, &sy, &sx2, &sy2, &sxy);
  double top = count*sxy - sx*sy;
  double bx = count*sx2 - sx*sx, by = count*sy2 - sy*sy;
  int n = add(out, 0, "Use PMCC from the paired raw data.");
  n = add(out, n, "n=%d, Sx=%.10g, Sy=%.10g", count, sx, sy);
  n = add(out, n, "Sx2=%.10g, Sy2=%.10g, Sxy=%.10g", sx2, sy2, sxy);
  n = add(out, n, "r=(nSxy-SxSy)/sqrt((nSx2-Sx^2)(nSy2-Sy^2))");
  return add(out, n, "r = %.10g", top / root(bx * by));
}

static int add_raw_spearman_lines(char out[P3_MAX_LINES][P3_LINE_LEN], const double *xs, const double *ys, int count) {
  double rx[16], ry[16], sd2 = 0;
  average_ranks(xs, count, rx);
  average_ranks(ys, count, ry);
  for (int i = 0; i < count; ++i) {
    double d = rx[i] - ry[i];
    sd2 += d*d;
  }
  double den = count * (count*count - 1.0);
  int n = add(out, 0, "Rank both variables, using average ranks for ties.");
  n = add(out, n, "n = %d, sum d^2 = %.10g", count, sd2);
  n = add(out, n, "r_s = 1 - 6*sum(d^2)/(n(n^2-1))");
  return add(out, n, "r_s = %.10g", 1 - 6*sd2/den);
}

static int add_raw_regression_lines(char out[P3_MAX_LINES][P3_LINE_LEN], const double *xs, const double *ys, int count, bool x_on_y, bool have_target, double target) {
  double sx, sy, sx2, sy2, sxy;
  paired_sums(xs, ys, count, &sx, &sy, &sx2, &sy2, &sxy);
  double xb = sx/count, yb = sy/count;
  double Sxx = sx2 - sx*sx/count, Syy = sy2 - sy*sy/count, Sxy = sxy - sx*sy/count;
  int n = add(out, 0, x_on_y ? "Find least-squares regression line x = a + by." : "Find least-squares regression line y = a + bx.");
  n = add(out, n, "xbar = %.10g/%d = %.10g", sx, count, xb);
  n = add(out, n, "ybar = %.10g/%d = %.10g", sy, count, yb);
  if (x_on_y) {
    double b = Sxy/Syy, a = xb - b*yb;
    n = add(out, n, "b = Sxy/Syy = %.10g/%.10g = %.10g", Sxy, Syy, b);
    n = add(out, n, "a = xbar - b*ybar = %.10g", a);
    n = add(out, n, "regression line: x = %.6g + %.6g y", a, b);
    if (have_target) n = add(out, n, "when y=%.6g, x=%.6g", target, a + b*target);
  } else {
    double b = Sxy/Sxx, a = yb - b*xb;
    n = add(out, n, "b = Sxy/Sxx = %.10g/%.10g = %.10g", Sxy, Sxx, b);
    n = add(out, n, "a = ybar - b*xbar = %.10g", a);
    n = add(out, n, "regression line: y = %.6g + %.6g x", a, b);
    if (have_target) n = add(out, n, "when x=%.6g, y=%.6g", target, a + b*target);
  }
  return n;
}

static int add_raw_paired_summary_lines(char out[P3_MAX_LINES][P3_LINE_LEN], const double *xs, const double *ys, int count) {
  double sx, sy, sx2, sy2, sxy;
  paired_sums(xs, ys, count, &sx, &sy, &sx2, &sy2, &sxy);
  double xb = sx/count, yb = sy/count;
  double Sxx = sx2 - sx*sx/count, Syy = sy2 - sy*sy/count, Sxy = sxy - sx*sy/count;
  int n = add(out, 0, "Use paired-data summary statistics.");
  n = add(out, n, "n=%d, Sx=%.10g, Sy=%.10g", count, sx, sy);
  n = add(out, n, "Sx2=%.10g, Sy2=%.10g, Sxy(raw)=%.10g", sx2, sy2, sxy);
  n = add(out, n, "xbar=%.10g, ybar=%.10g", xb, yb);
  n = add(out, n, "Sxx=Sx2-Sx^2/n = %.10g", Sxx);
  n = add(out, n, "Syy=Sy2-Sy^2/n = %.10g", Syy);
  n = add(out, n, "Sxy=Sxy(raw)-SxSy/n = %.10g", Sxy);
  return add(out, n, "covariance = Sxy/n = %.10g", Sxy/count);
}

static void sort_doubles(double *a, int n) {
  for (int i = 0; i < n; ++i) for (int j = i + 1; j < n; ++j) if (a[j] < a[i]) {
    double q = a[i]; a[i] = a[j]; a[j] = q;
  }
}

static double median_slice(const double *a, int lo, int hi) {
  int n = hi - lo;
  if (n <= 0) return 0;
  int mid = lo + n / 2;
  if (n % 2) return a[mid];
  return (a[mid - 1] + a[mid]) / 2.0;
}

static bool is_projectile_text(const char *t) {
  return has(t, "projectile") || has(t, "projectiles") || has(t, "projected") || has(t, "projection") ||
         has(t, "thrown") || has(t, "fired") || has(t, "launched");
}

static bool near_num(double a, double b) {
  double d = a > b ? a - b : b - a;
  return d < 1e-9;
}

static double abs_num(double x) { return x < 0 ? -x : x; }

static double cubic_val(double A, double B, double C, double D, double t) {
  return ((A*t + B)*t + C)*t + D;
}

static double quad_int_val(double A, double B, double C, double t) {
  return A*t*t*t/3.0 + B*t*t/2.0 + C*t;
}

static double cubic_int_val(double A, double B, double C, double D, double t) {
  return A*t*t*t*t/4.0 + B*t*t*t/3.0 + C*t*t/2.0 + D*t;
}

static bool extract_0_x_bound(const char *compact, double *upper) {
  const char *bp = strstr(compact, "0<x<");
  if (!bp) bp = strstr(compact, "0<=x<=");
  if (!bp) return false;
  const char *p = strchr(bp + 1, '<');
  if (!p) return false;
  while (*p == '<' || *p == '=') ++p;
  if (*p == 'x') ++p;
  while (*p == '<' || *p == '=') ++p;
  *upper = read_num(p);
  return *upper > 0;
}

static bool extract_x_interval(const char *compact, double *lower, double *upper) {
  const char *p = strstr(compact, "<x<");
  if (!p) p = strstr(compact, "<=x<=");
  if (!p) p = strstr(compact, "<=x<");
  if (!p) p = strstr(compact, "<x<=");
  if (!p) return false;
  const char *a = p;
  while (a > compact && (isdigit((unsigned char)a[-1]) || a[-1] == '.' || a[-1] == '-')) --a;
  char left[32]; int n = (int)(p - a);
  if (n <= 0 || n >= (int)sizeof(left)) return false;
  memcpy(left, a, n); left[n] = 0;
  const char *b = strchr(p, 'x');
  if (!b) return false;
  b = strchr(b, '<');
  if (!b) return false;
  while (*b == '<' || *b == '=') ++b;
  char *end = 0;
  double lo = strtod(left, 0), hi = strtod(b, &end);
  if (end == b) return false;
  if (hi < lo) { double q = lo; lo = hi; hi = q; }
  *lower = lo; *upper = hi;
  return true;
}

static bool extract_t_interval(const char *compact, double *lower, double *upper) {
  const char *p = strstr(compact, "<t<");
  if (!p) p = strstr(compact, "<=t<=");
  if (!p) p = strstr(compact, "<=t<");
  if (!p) p = strstr(compact, "<t<=");
  if (!p) return false;
  const char *a = p;
  while (a > compact && (isdigit((unsigned char)a[-1]) || a[-1] == '.' || a[-1] == '-')) --a;
  char left[32]; int n = (int)(p - a);
  if (n <= 0 || n >= (int)sizeof(left)) return false;
  memcpy(left, a, n); left[n] = 0;
  const char *b = strchr(p, 't');
  if (!b) return false;
  b = strchr(b, '<');
  if (!b) return false;
  while (*b == '<' || *b == '=') ++b;
  char *end = 0;
  double lo = strtod(left, 0), hi = strtod(b, &end);
  if (end == b) return false;
  if (hi < lo) { double q = lo; lo = hi; hi = q; }
  *lower = lo; *upper = hi;
  return true;
}

static bool parse_y_values(const char *input, double *ys, int *count) {
  const char *p = strstr(input, "values");
  if (!p) p = strstr(input, "ordinates");
  if (!p) return false;
  int n = 0;
  for (; *p && n < 16; ++p) {
    if (n >= 3 && (tolower((unsigned char)p[0]) == 'h' ||
        strncmp(p, "width", 5) == 0 || strncmp(p, "step", 4) == 0)) break;
    if (*p == '-' || *p == '.' || isdigit((unsigned char)*p)) {
      char *e = 0; double val = strtod(p, &e);
      if (e != p) { ys[n++] = val; p = e - 1; }
    }
  }
  *count = n;
  return n >= 3;
}

static double choose(int n, int r) {
  if (r < 0 || r > n) return 0;
  if (r > n - r) r = n - r;
  double c = 1;
  for (int i = 1; i <= r; ++i) c = c * (n - r + i) / i;
  return c;
}

static double binomp(int n, double p, int r) {
  return choose(n, r) * pwr(p, r) * pwr(1 - p, n - r);
}

static double expneg(double x) {
  if (x < 0) return 1 / expneg(-x);
  int m = (int)x + 1;
  double y = x / m, term = 1, sum = 1;
  for (int i = 1; i < 28; ++i) { term *= -y / i; sum += term; }
  return pwr(sum, m);
}

static double normal_cdf(double z) {
  double x = z < 0 ? -z : z;
  double t = 1 / (1 + 0.2316419 * x);
  double poly = (((((1.330274429 * t - 1.821255978) * t) + 1.781477937) * t - 0.356563782) * t + 0.319381530) * t;
  double tail = 0.3989422804014327 * expneg(0.5 * x * x) * poly;
  double ans = 1 - tail;
  return z < 0 ? 1 - ans : ans;
}

static double inv_norm_left(double p) {
  if (p <= 0) return -8;
  if (p >= 1) return 8;
  double lo = -8, hi = 8;
  for (int i = 0; i < 50; ++i) {
    double mid = (lo + hi) / 2;
    if (normal_cdf(mid) < p) lo = mid; else hi = mid;
  }
  return (lo + hi) / 2;
}

static double poissonp(double lam, int r) {
  if (r < 0 || lam < 0) return 0;
  double p = expneg(lam);
  for (int k = 1; k <= r; ++k) p *= lam / k;
  return p;
}

static double poissoncdf(double lam, int r) {
  if (r < 0) return 0;
  double ans = 0;
  for (int k = 0; k <= r; ++k) ans += poissonp(lam, k);
  return ans;
}

static double poissontailprob(double lam, int r, int tail) {
  if (tail < 0) return poissoncdf(lam, tail == -2 ? r - 1 : r);
  int lo = tail == 2 ? r + 1 : r;
  return 1 - poissoncdf(lam, lo - 1);
}

static int prob_tail(const char *c, const char *t) {
  if (has(c, "nomorethan") || has(c, "notmorethan")) return -1;
  if (has(c, "p(x<=") || has(c, "p(x=<") || has(c, "x<=") ||
      has(c, "lessthanorequal") || has(c, "atmost")) return -1;
  if (has(c, "p(x<") || has(c, "x<") || has(c, "lessthan") || has(t, "fewer") ||
      has(c, "below") || has(c, "under")) return -2;
  if (has(c, "p(x>=") || has(c, "p(x=>") || has(c, "x>=") ||
      has(c, "greaterthanorequal") || has(c, "atleast")) return 1;
  if (has(c, "p(x>") || has(c, "x>") || has(c, "greaterthan") || has(c, "morethan") ||
      has(c, "exceeds") || has(c, "exceed") || has(c, "above")) return 2;
  return 0;
}

static bool prob_x_bound(const char *c, double *x, int *tail) {
  const char *ops[] = {"p(x<=", "p(x=<", "x<=", "p(x>=", "p(x=>", "x>=", "p(x<", "x<", "p(x>", "x>"};
  const int tails[] = {-1, -1, -1, 1, 1, 1, -2, -2, 2, 2};
  for (int i = 0; i < 10; ++i) {
    const char *p = strstr(c, ops[i]);
    if (!p) continue;
    p += strlen(ops[i]);
    char *end = 0;
    double val = strtod(p, &end);
    if (end != p) {
      *x = val;
      *tail = tails[i];
      return true;
    }
  }
  return false;
}

static bool prob_x_equal_value(const char *c, int *x, double *prob) {
  const char *p = strstr(c, "p(x=");
  if (!p) p = strstr(c, "p(x==");
  if (!p) return false;
  const char *q = strchr(p, '=');
  if (!q) return false;
  while (*q == '=') ++q;
  char *end = 0;
  double xv = strtod(q, &end);
  if (end == q) return false;
  const char *eq = strchr(end, '=');
  if (!eq) return false;
  ++eq;
  double pv = strtod(eq, &end);
  if (end == eq || pv <= 0 || pv >= 1) return false;
  *x = (int)xv;
  *prob = pv;
  return true;
}

static bool prob_x_interval(const char *c, double *lo, double *hi) {
  bool ls = false, rs = false;
  const char *p = strstr(c, "<=x<=");
  if (!p) { p = strstr(c, "<=x<"); ls = false; rs = true; }
  if (!p) { p = strstr(c, "<x<="); ls = true; rs = false; }
  if (!p) { p = strstr(c, "<x<"); ls = true; rs = true; }
  if (!p) return false;
  const char *a = p;
  while (a > c && (isdigit((unsigned char)a[-1]) || a[-1] == '.' || a[-1] == '-')) --a;
  char left[32]; int n = (int)(p - a);
  if (n <= 0 || n >= (int)sizeof(left)) return false;
  memcpy(left, a, n); left[n] = 0;
  const char *b = strchr(p, 'x');
  if (!b) return false;
  b = strchr(b, '<');
  if (!b) return false;
  while (*b == '<' || *b == '=') ++b;
  char *end = 0;
  double l = strtod(left, 0), h = strtod(b, &end);
  if (end == b) return false;
  if (l > h) { double q = l; l = h; h = q; bool b = ls; ls = rs; rs = b; }
  *lo = ls ? floor_num(l) + 1 : ceil_num(l);
  *hi = rs ? ceil_num(h) - 1 : floor_num(h);
  return true;
}

static bool given_tail_bound(const char *c, double *bound, int *tail) {
  const char *g = strstr(c, "given");
  if (!g) return false;
  const char *p = strstr(g, "x<=");
  if (p) { *tail = -1; *bound = read_num(p + 3); return true; }
  p = strstr(g, "x<");
  if (p) { *tail = -2; *bound = read_num(p + 2); return true; }
  p = strstr(g, "x>=");
  if (p) { *tail = 1; *bound = read_num(p + 3); return true; }
  p = strstr(g, "x>");
  if (p) { *tail = 2; *bound = read_num(p + 2); return true; }
  p = strstr(g, "atleast");
  if (p) { *tail = 1; *bound = read_num(p + 7); return true; }
  p = strstr(g, "greaterthanorequal");
  if (p) { *tail = 1; *bound = read_num(p + 18); return true; }
  p = strstr(g, "morethan");
  if (p) { *tail = 2; *bound = read_num(p + 8); return true; }
  p = strstr(g, "greaterthan");
  if (p) { *tail = 2; *bound = read_num(p + 11); return true; }
  p = strstr(g, "atmost");
  if (p) { *tail = -1; *bound = read_num(p + 6); return true; }
  p = strstr(g, "lessthanorequal");
  if (p) { *tail = -1; *bound = read_num(p + 15); return true; }
  p = strstr(g, "nomorethan");
  if (p) { *tail = -1; *bound = read_num(p + 10); return true; }
  p = strstr(g, "lessthan");
  if (p) { *tail = -2; *bound = read_num(p + 8); return true; }
  return false;
}

static bool prose_count_interval(const char *c, int N, int *lo, int *hi) {
  bool has_lo = false, has_hi = false, lo_strict = false, hi_strict = false;
  double l = 0, h = 0;
  const char *p = strstr(c, "atleast");
  if (p) { has_lo = true; l = read_num(p + 7); }
  if (!p && (p = strstr(c, "greaterthanorequal"))) { has_lo = true; l = read_num(p + 18); }
  if (!p && (p = strstr(c, "morethan"))) { has_lo = true; lo_strict = true; l = read_num(p + 8); }
  if (!p && (p = strstr(c, "greaterthan"))) { has_lo = true; lo_strict = true; l = read_num(p + 11); }
  p = strstr(c, "fewerthan");
  if (p) { has_hi = true; hi_strict = true; h = read_num(p + 9); }
  if (!p && (p = strstr(c, "lessthanorequal"))) { has_hi = true; h = read_num(p + 15); }
  if (!p && (p = strstr(c, "lessthan"))) { has_hi = true; hi_strict = true; h = read_num(p + 8); }
  if (!p && (p = strstr(c, "atmost"))) { has_hi = true; h = read_num(p + 6); }
  if (!p && (p = strstr(c, "nomorethan"))) { has_hi = true; h = read_num(p + 10); }
  if (!has_lo || !has_hi) return false;
  int a = lo_strict ? (int)floor_num(l) + 1 : (int)ceil_num(l);
  int b = hi_strict ? (int)ceil_num(h) - 1 : (int)floor_num(h);
  if (a < 0) a = 0;
  if (b > N) b = N;
  *lo = a; *hi = b;
  return b >= a;
}

static int add_binom_range_lines(char out[P3_MAX_LINES][P3_LINE_LEN], int N, double p, int lo, int hi, const char *label) {
  if (lo < 0) lo = 0;
  if (hi > N) hi = N;
  if (hi < lo) hi = lo;
  double ans = 0;
  for (int k = lo; k <= hi; ++k) ans += binomp(N, p, k);
  int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
  n = add(out, n, "%s = sum P(X=r), r=%d..%d.", label, lo, hi);
  return add(out, n, "= %.10g", ans);
}

static bool dist1(const char *c, const char *name, double *a) {
  const char *p = strstr(c, name);
  if (!p) return false;
  p = strchr(p, '(');
  if (!p) return false;
  *a = read_num(p + 1);
  return true;
}

static bool dist2(const char *c, const char *name, double *a, double *b) {
  const char *p = strstr(c, name);
  if (!p) return false;
  p = strchr(p, '(');
  if (!p) return false;
  const char *q = strchr(p, ',');
  if (!q) return false;
  *a = read_num(p + 1);
  *b = read_num(q + 1);
  return true;
}

static void add_term(char *buf, int cap, const char *fmt, double x, double p, bool first) {
  int len = (int)strlen(buf);
  if (len >= cap - 32) return;
  if (!first) sprintf(buf + len, "+");
  len = (int)strlen(buf);
  if (len >= cap - 32) return;
  sprintf(buf + len, fmt, x, p);
}

static int eval_suvat(const char *s, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char a[8][48]; int na = args(s, a, 8);
  if (!starts(s, "suvat(")) return 0;
  double u=0,v=0,acc=0,dist=0,t=0; bool hu=kv(a,na,"u",&u), hv=kv(a,na,"v",&v), ha=kv(a,na,"a",&acc), hs=kv(a,na,"s",&dist), ht=kv(a,na,"t",&t);
  int n = add(out, 0, "List known values and choose a SUVAT equation.");
  if (!hv && hu && ha && ht) {
    n = add(out, n, "v = u + at");
    n = add(out, n, "v = %.6g + %.6g*%.6g", u, acc, t);
    return add(out, n, "v = %.6g", u + acc * t);
  }
  if (!hu && hv && ha && ht) {
    n = add(out, n, "v = u + at, so u = v - at");
    return add(out, n, "u = %.6g - %.6g*%.6g = %.10g", v, acc, t, v - acc*t);
  }
  if (!hs && hu && ha && ht) {
    n = add(out, n, "s = ut + 1/2 at^2");
    n = add(out, n, "s = %.6g*%.6g + 1/2*%.6g*%.6g^2", u, t, acc, t);
    return add(out, n, "s = %.6g", u*t + 0.5*acc*t*t);
  }
  if (!hs && hu && hv && ht) {
    n = add(out, n, "s = 1/2(u+v)t");
    n = add(out, n, "s = 1/2(%.6g+%.6g)*%.6g", u, v, t);
    return add(out, n, "s = %.10g", 0.5*(u+v)*t);
  }
  if (!hv && hu && hs && ht) {
    n = add(out, n, "s = 1/2(u+v)t, so v = 2s/t - u");
    return add(out, n, "v = 2*%.6g/%.6g - %.6g = %.10g", dist, t, u, 2*dist/t - u);
  }
  if (!hu && hv && hs && ht) {
    n = add(out, n, "s = 1/2(u+v)t, so u = 2s/t - v");
    return add(out, n, "u = 2*%.6g/%.6g - %.6g = %.10g", dist, t, v, 2*dist/t - v);
  }
  if (!hv && !ht && hu && ha && hs) {
    n = add(out, n, "Use v^2 = u^2 + 2as, then s = ut + 1/2at^2 for time.");
    double vv = u*u + 2*acc*dist, vpos = root(vv);
    n = add(out, n, "v^2 = %.6g^2 + 2*%.6g*%.6g", u, acc, dist);
    n = add(out, n, "v = %.10g", vpos);
    double A = 0.5*acc, B = u, C = -dist, D = B*B - 4*A*C;
    if (A*A < 1e-18) {
      n = add(out, n, "Here a=0, so s = ut.");
      return add(out, n, "t = s/u = %.6g/%.6g = %.10g", dist, u, dist/u);
    }
    n = add(out, n, "s = ut + 1/2at^2 gives a quadratic in t.");
    double rD = root(D), t1 = (-B + rD)/(2*A), t2 = (-B - rD)/(2*A);
    return add(out, n, "t = %.10g or %.10g", t1, t2);
  }
  if (!hv && hu && ha && hs) {
    n = add(out, n, "v^2 = u^2 + 2as");
    n = add(out, n, "v^2 = %.6g^2 + 2*%.6g*%.6g", u, acc, dist);
    return add(out, n, "v = %.6g", root(u*u + 2*acc*dist));
  }
  if (!hu && hv && ha && hs) {
    n = add(out, n, "v^2 = u^2 + 2as, so u^2 = v^2 - 2as");
    n = add(out, n, "u^2 = %.6g^2 - 2*%.6g*%.6g", v, acc, dist);
    return add(out, n, "u = %.10g", root(v*v - 2*acc*dist));
  }
  if (!ha && hu && hv && ht) {
    n = add(out, n, "v = u + at, so a=(v-u)/t");
    return add(out, n, "a = (%.6g-%.6g)/%.6g = %.6g", v, u, t, (v-u)/t);
  }
  if (!ha && hu && hv && hs) {
    n = add(out, n, "v^2 = u^2 + 2as, so a=(v^2-u^2)/(2s)");
    return add(out, n, "a = (%.6g^2-%.6g^2)/(2*%.6g) = %.10g", v, u, dist, (v*v-u*u)/(2*dist));
  }
  if (!ha && hu && hs && ht) {
    n = add(out, n, "s = ut + 1/2 at^2, so a = 2(s-ut)/t^2");
    return add(out, n, "a = 2(%.6g-%.6g*%.6g)/%.6g^2 = %.10g", dist, u, t, t, 2*(dist-u*t)/(t*t));
  }
  if (!ha && hv && hs && ht) {
    n = add(out, n, "s = vt - 1/2 at^2, so a = 2(vt-s)/t^2");
    return add(out, n, "a = 2(%.6g*%.6g-%.6g)/%.6g^2 = %.10g", v, t, dist, t, 2*(v*t-dist)/(t*t));
  }
  if (!ht && hu && hv && ha) {
    n = add(out, n, "v = u + at, so t=(v-u)/a");
    return add(out, n, "t = (%.6g-%.6g)/%.6g = %.6g", v, u, acc, (v-u)/acc);
  }
  if (!ht && hu && hv && hs) {
    n = add(out, n, "s = 1/2(u+v)t, so t = 2s/(u+v)");
    return add(out, n, "t = 2*%.6g/(%.6g+%.6g) = %.10g", dist, u, v, 2*dist/(u+v));
  }
  if (!ht && hu && ha && hs) {
    n = add(out, n, "s = ut + 1/2 at^2 gives a quadratic in t.");
    double A = 0.5*acc, B = u, C = -dist, D = B*B - 4*A*C;
    if (A*A < 1e-18) {
      n = add(out, n, "Here a=0, so s = ut.");
      return add(out, n, "t = %.6g/%.6g = %.10g", dist, u, dist/u);
    }
    n = add(out, n, "%.10g t^2 + %.10g t - %.10g = 0", A, B, dist);
    if (D < 0) return add(out, n, "No real time from these values.");
    double rD = root(D), t1 = (-B + rD)/(2*A), t2 = (-B - rD)/(2*A);
    return add(out, n, "t = %.10g or %.10g", t1, t2);
  }
  if (!hu && !hv && hs && ha && ht) {
    n = add(out, n, "Use s = 1/2(u+v)t and v = u + at.");
    double u0 = dist/t - 0.5*acc*t, v0 = dist/t + 0.5*acc*t;
    n = add(out, n, "u = s/t - 1/2at = %.6g/%.6g - 1/2*%.6g*%.6g", dist, t, acc, t);
    n = add(out, n, "v = s/t + 1/2at = %.6g/%.6g + 1/2*%.6g*%.6g", dist, t, acc, t);
    return add(out, n, "u = %.10g, v = %.10g", u0, v0);
  }
  if (!hu && hs && ha && ht) {
    n = add(out, n, "s = ut + 1/2 at^2, so u = (s - 1/2at^2)/t");
    return add(out, n, "u = (%.6g - 1/2*%.6g*%.6g^2)/%.6g = %.10g", dist, acc, t, t, (dist-0.5*acc*t*t)/t);
  }
  if (!hv && hs && ha && ht) {
    n = add(out, n, "s = vt - 1/2 at^2, so v = (s + 1/2at^2)/t");
    return add(out, n, "v = (%.6g + 1/2*%.6g*%.6g^2)/%.6g = %.10g", dist, acc, t, t, (dist+0.5*acc*t*t)/t);
  }
  return add(out, n, "Need any suitable 3 known values, e.g. suvat(u=2,a=3,t=4).");
}

static int eval_mech(const char *s, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char a[8][48]; int na = args(s, a, 8);
  if (starts3(s, "projectileh(", "projectileheight(", "projheight(") && na >= 3) {
    double u = num(a[0]), deg = num(a[1]), h = num(a[2]), g = na > 3 ? num(a[3]) : 9.8;
    double th = deg * M_PI / 180.0, ux = u*cosine(th), uy = u*sine(th);
    double t = (uy + root(uy*uy + 2*g*h)) / g, r = ux*t;
    double maxh = uy > 0 ? h + uy*uy/(2*g) : h;
    int n = add(out, 0, "Resolve, then use vertical motion to find time.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.6g", u, deg, ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.6g", u, deg, uy);
    n = add(out, n, "-h = u_y t - 1/2 g t^2, so h=%.6g", h);
    n = add(out, n, "t = (u_y+sqrt(u_y^2+2gh))/g = %.10g", t);
    n = add(out, n, "greatest height above ground = h + u_y^2/(2g) = %.10g", maxh);
    return add(out, n, "range = u_x t = %.10g", r);
  }
  if (starts3(s, "projectiley(", "projectilelevel(", "projlevel(") && na >= 3) {
    double u = num(a[0]), deg = num(a[1]), y = num(a[2]), h0 = na > 3 ? num(a[3]) : 0, g = na > 4 ? num(a[4]) : 9.8;
    double th = deg * M_PI / 180.0, ux = u*cosine(th), uy = u*sine(th);
    double A = 0.5 * g, B = -uy, C = y - h0, D = B*B - 4*A*C;
    int n = add(out, 0, "Resolve, then solve vertical motion for the requested height.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.6g", u, deg, ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.6g", u, deg, uy);
    n = add(out, n, "y = h0 + u_y t - 1/2 g t^2");
    n = add(out, n, "%.6g = %.6g + %.6g t - 1/2*%.6g t^2", y, h0, uy, g);
    if (D < 0) return add(out, n, "No real time reaches this height.");
    double t1 = (-B - root(D))/(2*A), t2 = (-B + root(D))/(2*A);
    if (t1 < 0 && t2 >= 0) return add(out, n, "time = %.10g s", t2);
    if (t2 < 0 && t1 >= 0) return add(out, n, "time = %.10g s", t1);
    return add(out, n, "times = %.10g s and %.10g s", t1, t2);
  }
  if (starts3(s, "projectileat(", "projectilepoint(", "projat(") && na >= 3) {
    double u = num(a[0]), deg = num(a[1]), x = num(a[2]), h0 = na > 3 ? num(a[3]) : 0, g = na > 4 ? num(a[4]) : 9.8;
    double th = deg * M_PI / 180.0, ux = u*cosine(th), uy = u*sine(th);
    double t = ux == 0 ? 0 : x / ux;
    double y = h0 + uy*t - 0.5*g*t*t;
    int n = add(out, 0, "Resolve velocity, then use horizontal motion to find time.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.6g", u, deg, ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.6g", u, deg, uy);
    n = add(out, n, "Horizontal: x = u_x t, so t = %.6g/%.6g = %.10g", x, ux, t);
    n = add(out, n, "Vertical: y = h + u_y t - 1/2 gt^2");
    n = add(out, n, "y = %.6g + %.6g*%.10g - 1/2*%.6g*%.10g^2", h0, uy, t, g, t);
    return add(out, n, "height at x=%.6g is %.10g", x, y);
  }
  if (starts3(s, "projectileangle(", "projangle(", "targetangle(") && na >= 3) {
    double u = num(a[0]), x = num(a[1]), y = num(a[2]), h0 = na > 3 ? num(a[3]) : 0, g = na > 4 ? num(a[4]) : 9.8;
    double A = g*x*x/(2*u*u), C = A + y - h0, D = x*x - 4*A*C;
    int n = add(out, 0, "Use trajectory equation to find the launch angle.");
    n = add(out, n, "y = h + x tan(theta) - gx^2/(2u^2 cos^2(theta))");
    n = add(out, n, "Let T=tan(theta), so 1/cos^2(theta)=1+T^2.");
    n = add(out, n, "%.10g T^2 - %.10g T + %.10g = 0", A, x, C);
    if (D < 0) return add(out, n, "No real launch angle for this speed and target point.");
    double T1 = (x + root(D))/(2*A), T2 = (x - root(D))/(2*A);
    double a1 = arctan(T1)*180.0/M_PI, a2 = arctan(T2)*180.0/M_PI;
    n = add(out, n, "T = (x +- sqrt(discriminant))/(2A)");
    n = add(out, n, "tan(theta) = %.10g or %.10g", T1, T2);
    return add(out, n, "theta = %.10g deg or %.10g deg", a1, a2);
  }
  if (starts3(s, "projectile(", "proj(", "projectiles(") && na >= 2) {
    double u = num(a[0]), th = num(a[1]) * M_PI / 180.0, g = na > 2 ? num(a[2]) : 9.8;
    double ux = u*cosine(th), uy = u*sine(th), T = 2*uy/g, R = ux*T, H = uy*uy/(2*g);
    int n = add(out, 0, "Resolve velocity into horizontal and vertical components.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.6g", u, num(a[1]), ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.6g", u, num(a[1]), uy);
    n = add(out, n, "time of flight: t = 2u_y/g = %.6g", T);
    n = add(out, n, "range = u_x t = %.6g", R);
    return add(out, n, "max height = u_y^2/(2g) = %.6g", H);
  }
  if (starts3(s, "force(", "newton(", "fma(") && na >= 2) {
    double m=num(a[0]), acc=num(a[1]);
    int n = add(out, 0, "Use Newton's second law in the direction of motion.");
    return add(out, n, "F = ma = %.6g*%.6g = %.6g N", m, acc, m*acc);
  }
  if (starts2(s, "weight(", "mg(") && na >= 1) {
    double m=num(a[0]), g=na>1?num(a[1]):9.8;
    int n = add(out, 0, "Weight acts vertically downwards.");
    return add(out, n, "W = mg = %.6g*%.6g = %.6g N", m, g, m*g);
  }
  if (starts2(s, "friction(", "limitingfriction(") && na >= 2) {
    double mu=num(a[0]), r=num(a[1]);
    int n = add(out, 0, "Limiting friction is F = mu R.");
    return add(out, n, "F = %.6g*%.6g = %.6g N", mu, r, mu*r);
  }
  if (starts2(s, "moment(", "moments(") && na >= 2) {
    double f=num(a[0]), d=num(a[1]);
    int n = add(out, 0, "Moment = force * perpendicular distance.");
    return add(out, n, "M = %.6g*%.6g = %.6g Nm", f, d, f*d);
  }
  if (starts3(s, "beam(", "beamreactions(", "supportreactions(") && na >= 3) {
    double L=num(a[0]), W=num(a[1]), x=num(a[2]), bw=na>3?num(a[3]):0;
    double rb=(W*x + bw*L/2)/L, ra=W+bw-rb;
    int n = add(out, 0, "For a horizontal beam in equilibrium, use moments and vertical forces.");
    n = add(out, n, "Take moments about the left support A.");
    n = add(out, n, "R_B*%.6g = %.6g*%.6g + %.6g*(%.6g/2)", L, W, x, bw, L);
    n = add(out, n, "R_B = %.10g N", rb);
    n = add(out, n, "Vertical equilibrium: R_A + R_B = %.6g + %.6g", W, bw);
    return add(out, n, "R_A = %.10g N", ra);
  }
  if (starts3(s, "ladder(", "ladderwall(", "ladderrough(") && na >= 3) {
    double L=num(a[0]), W=num(a[1]), deg=num(a[2]), P=na>3?num(a[3]):0, d=na>4?num(a[4]):L/2;
    double s=deg_sine(deg), c=deg_cosine(deg);
    double wall = s == 0 ? 0 : (W*(L/2)*c + P*d*c)/(L*s);
    double floor = W + P, fr = wall, mu = floor == 0 ? 0 : fr/floor;
    int n = add(out, 0, "Ladder in equilibrium: smooth wall, rough floor.");
    n = add(out, n, "Forces: floor reaction R, floor friction F, wall reaction S.");
    n = add(out, n, "Vertical equilibrium: R = %.6g + %.6g = %.10g N", W, P, floor);
    n = add(out, n, "Horizontal equilibrium: F = S.");
    n = add(out, n, "Take moments about the foot of the ladder.");
    n = add(out, n, "S*(%.6g sin %.6g) = %.6g*(%.6g/2 cos %.6g) + %.6g*(%.6g cos %.6g)", L, deg, W, L, deg, P, d, deg);
    n = add(out, n, "S = %.10g N, so F = %.10g N", wall, fr);
    return add(out, n, "limiting friction: mu = F/R = %.10g/%.10g = %.10g", fr, floor, mu);
  }
  if (starts3(s, "incline(", "slope(", "plane(") && na >= 2) {
    double m=num(a[0]), th=num(a[1])*M_PI/180.0, g=na>3?num(a[3]):9.8, mu=na>2?num(a[2]):0;
    double parallel=m*g*sine(th), reaction=m*g*cosine(th), fr=mu*reaction;
    int n = add(out, 0, "Resolve weight parallel and perpendicular to the plane.");
    n = add(out, n, "parallel = mg sin theta = %.6g N", parallel);
    n = add(out, n, "R = mg cos theta = %.6g N", reaction);
    if (na > 2) n = add(out, n, "friction = mu R = %.6g N", fr);
    return add(out, n, "net down plane = %.6g N", parallel - fr);
  }
  if (starts3(s, "inclineacc(", "roughplaneacc(", "planeacc(") && na >= 3) {
    double m=num(a[0]), deg=num(a[1]), mu=num(a[2]), g=na>3?num(a[3]):9.8;
    double th=deg*M_PI/180.0, parallel=m*g*sine(th), reaction=m*g*cosine(th), fr=mu*reaction;
    double net=parallel-fr, acc=net/m;
    int n = add(out, 0, "Resolve along and perpendicular to the plane, then use F=ma.");
    n = add(out, n, "down-slope component = mg sin(theta) = %.6g*%.6g sin(%.6g) = %.10g N", m, g, deg, parallel);
    n = add(out, n, "R = mg cos(theta) = %.6g*%.6g cos(%.6g) = %.10g N", m, g, deg, reaction);
    n = add(out, n, "friction = mu R = %.6g*%.10g = %.10g N", mu, reaction, fr);
    n = add(out, n, "net force down plane = %.10g - %.10g = %.10g N", parallel, fr, net);
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, acc);
  }
  if (starts3(s, "connected(", "connectedparticles(", "twoparticles(") && na >= 3) {
    double m1=num(a[0]), m2=num(a[1]), f=num(a[2]), ares=f/(m1+m2), T=m1*ares;
    int n = add(out, 0, "Connected particles: treat the particles as one system for acceleration.");
    n = add(out, n, "a = F/(m1+m2) = %.6g/(%.6g+%.6g)", f, m1, m2);
    n = add(out, n, "a = %.6g m/s^2", ares);
    return add(out, n, "tension on m1: T = m1*a = %.6g N", T);
  }
  if (starts2(s, "pulley(", "pulleys(") && na >= 2) {
    double m1=num(a[0]), m2=num(a[1]), g=na>2?num(a[2]):9.8, ares=(m2-m1)*g/(m1+m2), T=m1*(g+ares);
    int n = add(out, 0, "Connected particles: same acceleration in the light string.");
    n = add(out, n, "a = (m2-m1)g/(m1+m2) = %.6g", ares);
    return add(out, n, "T = m1(g+a) = %.6g N", T);
  }
  if (starts3(s, "inclinepulley(", "pulleyincline(", "connectedincline(") && na >= 3) {
    double m1=num(a[0]), m2=num(a[1]), ang=num(a[2]), g=na>3?num(a[3]):9.8;
    double down=m1*g*deg_sine(ang), hang=m2*g, ares=(hang-down)/(m1+m2), T=down+m1*ares;
    int n = add(out, 0, "Smooth inclined-plane pulley: resolve along the string.");
    n = add(out, n, "For mass on plane: T - m1 g sin(theta) = m1 a.");
    n = add(out, n, "For hanging mass: m2 g - T = m2 a.");
    n = add(out, n, "Add equations: m2 g - m1 g sin(theta) = (m1+m2)a.");
    n = add(out, n, "a = (%.6g*%.6g - %.6g*%.6g sin(%.6g))/(%.6g+%.6g) = %.10g", m2, g, m1, g, ang, m1, m2, ares);
    return add(out, n, "T = m1 g sin(theta) + m1 a = %.10g N", T);
  }
  if (starts3(s, "roughinclinepulley(", "roughpulleyincline(", "roughconnectedincline(") && na >= 4) {
    double m1=num(a[0]), m2=num(a[1]), ang=num(a[2]), mu=num(a[3]), g=na>4?num(a[4]):9.8;
    double down=m1*g*deg_sine(ang), R=m1*g*deg_cosine(ang), fr=mu*R, hang=m2*g;
    double ares=(hang-down-fr)/(m1+m2), T=down+fr+m1*ares;
    int n = add(out, 0, "Rough inclined-plane pulley: resolve along the string and include friction.");
    n = add(out, n, "For mass on plane: T - m1 g sin(theta) - mu R = m1 a.");
    n = add(out, n, "R = m1 g cos(theta), so friction = mu R.");
    n = add(out, n, "For hanging mass: m2 g - T = m2 a.");
    n = add(out, n, "Add equations: m2 g - m1 g sin(theta) - mu m1 g cos(theta) = (m1+m2)a.");
    n = add(out, n, "a = %.10g", ares);
    return add(out, n, "T = m1 g sin(theta) + mu R + m1 a = %.10g N", T);
  }
  if (starts2(s, "impulse(", "momentumchange(") && na >= 3) {
    double m=num(a[0]), u=num(a[1]), v=num(a[2]);
    int n = add(out, 0, "Impulse equals change in momentum.");
    return add(out, n, "I = m(v-u) = %.6g(%.6g-%.6g) = %.6g Ns", m, v, u, m*(v-u));
  }
  if ((starts(s, "momentum(") || starts(s, "momcons(") || starts(s, "consmomentum(")) && na >= 5) {
    double m1=num(a[0]), u1=num(a[1]), m2=num(a[2]), u2=num(a[3]), v1=num(a[4]);
    double before=m1*u1+m2*u2, v2=(before-m1*v1)/m2;
    int n = add(out, 0, "Use conservation of linear momentum.");
    n = add(out, n, "m1u1 + m2u2 = m1v1 + m2v2");
    n = add(out, n, "%.6g*%.6g + %.6g*%.6g = %.6g*%.6g + %.6g*v2", m1, u1, m2, u2, m1, v1, m2);
    return add(out, n, "v2 = (%.6g - %.6g*%.6g)/%.6g = %.10g m/s", before, m1, v1, m2, v2);
  }
  if ((starts(s, "commonvelocity(") || starts(s, "coalesce(") || starts(s, "stick(")) && na >= 4) {
    double m1=num(a[0]), u1=num(a[1]), m2=num(a[2]), u2=num(a[3]);
    double v=(m1*u1+m2*u2)/(m1+m2);
    int n = add(out, 0, "Particles move together, so conserve linear momentum.");
    n = add(out, n, "m1u1 + m2u2 = (m1+m2)v");
    n = add(out, n, "%.6g*%.6g + %.6g*%.6g = (%.6g+%.6g)v", m1, u1, m2, u2, m1, m2);
    return add(out, n, "v = %.10g m/s", v);
  }
  if ((has(s, "variableforce") || (has(s, "force") && has(s, "x^"))) && (has(s, "work") || has(s, "workdone"))) {
    double A=0, B=0, C=0, D=0, nums[8]; int nn = scan_nums(s, nums, 8);
    if (!has(s, "/(") && (has(s, "f=(") || has(s, ")^")) && nn >= 2) {
      const char *p = strstr(s, "f=(");
      if (p) p += 3;
      else if ((p = strchr(s, '('))) ++p;
      double rb=0, rd=0; const char *e=0;
      if (p && parse_linear_inside(p, 'x', &rb, &rd, &e) && *e == ')') {
        int pow = e[1] == '^' ? (int)read_num(e + 2) : 1;
        if (pow >= 1) {
          double lo = nums[nn-2], hi = nums[nn-1];
          double F1 = linear_power_antideriv(1.0, rb, rd, pow, lo);
          double F2 = linear_power_antideriv(1.0, rb, rd, pow, hi);
          char lin[48]; fmt_linear_var(lin, sizeof(lin), rb, 'x', rd);
          int n = add(out, 0, "Work done by a variable force is the integral of force.");
          n = add(out, n, "F(x) = (%s)^%d", lin, pow);
          n = add(out, n, "W = integral from %.6g to %.6g of (%s)^%d dx", lo, hi, lin, pow);
          n = add(out, n, "integral = (%s)^%d/(%.10g*%d)", lin, pow+1, rb, pow+1);
          return add(out, n, "W = %.10g J", F2 - F1);
        }
      }
    }
    if (parse_force_x_poly(s, &A, &B, &C, &D) && nn >= 2) {
      double lo = nums[nn-2], hi = nums[nn-1];
      double W = A*(hi*hi*hi*hi - lo*lo*lo*lo)/4.0 + B*(hi*hi*hi - lo*lo*lo)/3.0 +
                 C*(hi*hi - lo*lo)/2.0 + D*(hi - lo);
      int n = add(out, 0, "Work done by a variable force is the integral of force.");
      n = add(out, n, "F(x) = %.6g x^3 %+.6g x^2 %+.6g x %+.6g", A, B, C, D);
      n = add(out, n, "W = integral from %.6g to %.6g of F(x) dx", lo, hi);
      return add(out, n, "W = %.10g J", W);
    }
  }
  if (starts2(s, "work(", "workdone(") && na >= 2) {
    double f=num(a[0]), d=num(a[1]);
    int n = add(out, 0, "Work done = force * distance in the direction of motion.");
    return add(out, n, "W = %.6g*%.6g = %.6g J", f, d, f*d);
  }
  if (starts2(s, "power(", "powerrate(") && na >= 2) {
    double w=num(a[0]), t=num(a[1]);
    int n = add(out, 0, "Power = work done / time.");
    return add(out, n, "P = %.6g/%.6g = %.6g W", w, t, w/t);
  }
  if (starts3(s, "varacct(", "varacctpoly(", "variableacct(") && na >= 5) {
    double u=num(a[0]), c0=num(a[1]), c1=num(a[2]), c2=num(a[3]), t=num(a[4]), s0=na>5?num(a[5]):0;
    double v = u + c0*t + c1*t*t/2 + c2*t*t*t/3;
    double dist = s0 + u*t + c0*t*t/2 + c1*t*t*t/6 + c2*t*t*t*t/12;
    int n = add(out, 0, "Variable acceleration with a(t)=c0+c1t+c2t^2.");
    n = add(out, n, "v = u + integral a(t) dt");
    n = add(out, n, "v = %.6g + %.6g t + (%.6g/2)t^2 + (%.6g/3)t^3", u, c0, c1, c2);
    n = add(out, n, "at t=%.6g, v = %.10g", t, v);
    n = add(out, n, "s = s0 + integral v(t) dt");
    n = add(out, n, "s = %.6g + %.6g t + (%.6g/2)t^2 + (%.6g/6)t^3 + (%.6g/12)t^4", s0, u, c0, c1, c2);
    return add(out, n, "at t=%.6g, s = %.10g", t, dist);
  }
  if (starts3(s, "varaccx(", "varaccxpoly(", "variableaccx(") && na >= 5) {
    double u=num(a[0]), c0=num(a[1]), c1=num(a[2]), c2=num(a[3]), x=num(a[4]), x0=na>5?num(a[5]):0;
    double F = c0*(x-x0) + c1*(x*x-x0*x0)/2 + c2*(x*x*x-x0*x0*x0)/3;
    double vv = u*u + 2*F, v = vv >= 0 ? root(vv) : 0;
    int n = add(out, 0, "Variable acceleration with a(x)=c0+c1x+c2x^2.");
    n = add(out, n, "Use a = v dv/dx, so integrate v dv = integrate a(x) dx.");
    n = add(out, n, "1/2(v^2-u^2) = integral from %.6g to %.6g of a(x) dx", x0, x);
    n = add(out, n, "integral = %.6g(x-x0)+(%.6g/2)(x^2-x0^2)+(%.6g/3)(x^3-x0^3)", c0, c1, c2);
    n = add(out, n, "v^2 = %.6g^2 + 2*(%.10g)", u, F);
    return add(out, n, "v = %.10g", v);
  }
  if (starts3(s, "workenergyforce(", "energyforce(", "driveforce(") && na >= 5) {
    double m=num(a[0]), u=num(a[1]), v=num(a[2]), h=num(a[3]), d=num(a[4]);
    double r=na>5?num(a[5]):0, g=na>6?num(a[6]):9.8;
    double dk=0.5*m*(v*v-u*u), dg=m*g*h, wr=r*d, total=dk+dg+wr, f=d==0?0:total/d;
    int n = add(out, 0, "Use work-energy, taking motion in the direction of travel.");
    n = add(out, n, "work by driving force = gain in KE + gain in GPE + work against resistance.");
    n = add(out, n, "gain in KE = 1/2*m*(v^2-u^2) = 1/2*%.6g*(%.6g^2-%.6g^2) = %.10g J", m, v, u, dk);
    n = add(out, n, "gain in GPE = mgh = %.6g*%.6g*%.6g = %.10g J", m, g, h, dg);
    n = add(out, n, "work against resistance = R*d = %.6g*%.6g = %.10g J", r, d, wr);
    n = add(out, n, "F*%.6g = %.10g + %.10g + %.10g = %.10g", d, dk, dg, wr, total);
    return add(out, n, "F = %.10g N", f);
  }
  if (starts3(s, "energy(", "kepe(", "workenergy(") && na >= 2) {
    double m=num(a[0]), v=num(a[1]), h=na>2?num(a[2]):0, g=na>3?num(a[3]):9.8;
    int n = add(out, 0, "Use mechanical energy formulae.");
    n = add(out, n, "KE = 1/2 mv^2 = 1/2*%.6g*%.6g^2 = %.6g J", m, v, 0.5*m*v*v);
    if (na > 2) return add(out, n, "GPE = mgh = %.6g*%.6g*%.6g = %.6g J", m, g, h, m*g*h);
    return n;
  }
  if (starts3(s, "restitution(", "impact(", "collision(") && na >= 4) {
    double u1=num(a[0]), u2=num(a[1]), v1=num(a[2]), v2=num(a[3]);
    int n = add(out, 0, "Use Newton's experimental law of restitution.");
    n = add(out, n, "e = speed of separation / speed of approach");
    return add(out, n, "e = (%.6g-%.6g)/(%.6g-%.6g) = %.6g", v2, v1, u1, u2, (v2-v1)/(u1-u2));
  }
  if (starts3(s, "impactsolve(", "collisionsolve(", "restitutionsolve(") && na >= 5) {
    double m1=num(a[0]), u1=num(a[1]), m2=num(a[2]), u2=num(a[3]), e=num(a[4]);
    double sep = e*(u1-u2), before = m1*u1 + m2*u2;
    double v1 = (before - m2*sep)/(m1+m2), v2 = v1 + sep;
    int n = add(out, 0, "Use momentum and Newton's law of restitution.");
    n = add(out, n, "m1u1 + m2u2 = m1v1 + m2v2");
    n = add(out, n, "v2 - v1 = e(u1-u2) = %.6g", sep);
    n = add(out, n, "substitute into momentum: %.6g = %.6g v1 + %.6g(v1+%.6g)", before, m1, m2, sep);
    return add(out, n, "v1 = %.10g, v2 = %.10g", v1, v2);
  }
  if (starts3(s, "vectorkin(", "vectormotion(", "vectorsuvat(") && na >= 7) {
    double x0=num(a[0]), y0=num(a[1]), ux=num(a[2]), uy=num(a[3]), ax=num(a[4]), ay=num(a[5]), t=num(a[6]);
    double x=x0+ux*t+0.5*ax*t*t, y=y0+uy*t+0.5*ay*t*t;
    double vx=ux+ax*t, vy=uy+ay*t, speed=root(vx*vx+vy*vy);
    int n = add(out, 0, "Use vector constant-acceleration formulae component by component.");
    n = add(out, n, "r = r0 + ut + 1/2 at^2");
    n = add(out, n, "r = (%.6g,%.6g) + (%.6g,%.6g)*%.6g + 1/2(%.6g,%.6g)*%.6g^2", x0, y0, ux, uy, t, ax, ay, t);
    n = add(out, n, "position = (%.10g, %.10g)", x, y);
    n = add(out, n, "v = u + at = (%.6g,%.6g) + (%.6g,%.6g)*%.6g", ux, uy, ax, ay, t);
    n = add(out, n, "velocity = (%.10g, %.10g)", vx, vy);
    return add(out, n, "speed = sqrt(vx^2+vy^2) = %.10g", speed);
  }
  if (starts3(s, "vector(", "resultant(", "components(") && na >= 2) {
    double x=num(a[0]), y=num(a[1]), mag=root(x*x+y*y);
    int n = add(out, 0, "Resolve into perpendicular components.");
    n = add(out, n, "|R| = sqrt(x^2+y^2)");
    n = add(out, n, "|R| = sqrt(%.6g^2+%.6g^2) = %.6g", x, y, mag);
    return add(out, n, "direction: use tan(theta)=y/x on calculator.");
  }
  if (starts3(s, "resolve(", "componentsfromforce(", "forcecomponents(") && na >= 2) {
    double F=num(a[0]), deg=num(a[1]);
    int n = add(out, 0, "Resolve the force into perpendicular components.");
    n = add(out, n, "horizontal = F cos(theta) = %.6g cos(%.6g)", F, deg);
    n = add(out, n, "vertical = F sin(theta) = %.6g sin(%.6g)", F, deg);
    return add(out, n, "components = %.10g, %.10g", F*deg_cosine(deg), F*deg_sine(deg));
  }
  if (starts3(s, "equilibrium(", "forcebalance(", "balanceforces(") && na >= 4) {
    double sx=0, sy=0;
    int n = add(out, 0, "For equilibrium, resolve forces and set sum Fx = 0, sum Fy = 0.");
    for (int i=0;i+1<na;i+=2) {
      double x=num(a[i]), y=num(a[i+1]); sx += x; sy += y;
      n = add(out, n, "force %d components: (%.6g, %.6g)", i/2+1, x, y);
    }
    n = add(out, n, "sum Fx = %.10g, sum Fy = %.10g", sx, sy);
    if (sx*sx + sy*sy < 1e-8) return add(out, n, "result: equilibrium");
    return add(out, n, "resultant imbalance = sqrt(Fx^2+Fy^2) = %.10g N", root(sx*sx+sy*sy));
  }
  if (starts3(s, "equilpolar(", "forcepolar(", "balancepolar(") && na >= 4) {
    double sx=0, sy=0;
    int n = add(out, 0, "Resolve each force into horizontal and vertical components.");
    for (int i=0;i+1<na;i+=2) {
      double F=num(a[i]), deg=num(a[i+1]), x=F*deg_cosine(deg), y=F*deg_sine(deg);
      sx += x; sy += y;
      n = add(out, n, "%.6g at %.6g deg: Fx=F cos theta=%.6g, Fy=F sin theta=%.6g", F, deg, x, y);
    }
    n = add(out, n, "sum Fx = %.10g, sum Fy = %.10g", sx, sy);
    if (sx*sx + sy*sy < 0.25) return add(out, n, "result: equilibrium");
    return add(out, n, "resultant imbalance = %.10g N", root(sx*sx+sy*sy));
  }
  if (starts3(s, "varacc(", "variableacc(", "variableacceleration(") && na >= 4) {
    double c=num(a[0]), k=num(a[1]), u=num(a[2]), t=num(a[3]);
    int n = add(out, 0, "Given a(t)=c+kt, integrate to get velocity.");
    n = add(out, n, "v = u + ct + 1/2 kt^2");
    n = add(out, n, "v = %.6g + %.6g*%.6g + 1/2*%.6g*%.6g^2 = %.6g", u, c, t, k, t, u+c*t+0.5*k*t*t);
    return add(out, n, "s = ut + 1/2ct^2 + 1/6kt^3 = %.6g", u*t+0.5*c*t*t+k*t*t*t/6.0);
  }
  return 0;
}

static int eval_stats(const char *s, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char a[8][48]; int na = args(s, a, 8);
  if (has(s, "given") && has(s, "n(") && (has(s, "p(x>") || has(s, "p(x<"))) {
    double mu = 0, second = 0;
    bool hd = dist2(s, "~n", &mu, &second) || dist2(s, "fromn", &mu, &second) ||
              (starts(s, "n(") && dist2(s, "n", &mu, &second));
    if (hd) {
      double sig = has(s, "^2") ? second : root(second);
      const char *given = strstr(s, "given");
      const char *gtp = strstr(s, "p(x>");
      const char *ltp = strstr(s, "p(x<");
      const char *gxgt = given ? strstr(given, "x>") : 0;
      const char *gxlt = given ? strstr(given, "x<") : 0;
      char cmd[120];
      if (gtp && gxgt) {
        double x = read_num(gtp + (gtp[4] == '=' ? 5 : 4));
        double g = read_num(gxgt + (gxgt[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,1)", x, g, mu, sig);
        return eval_stats(cmd, out);
      }
      if (ltp && gxlt) {
        double x = read_num(ltp + (ltp[4] == '=' ? 5 : 4));
        double g = read_num(gxlt + (gxlt[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,-1)", x, g, mu, sig);
        return eval_stats(cmd, out);
      }
      if (ltp && gxgt) {
        double upper = read_num(ltp + (ltp[4] == '=' ? 5 : 4));
        double lower = read_num(gxgt + (gxgt[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcondbetween(%.10g,%.10g,%.10g,%.10g,%.10g,1)", lower, upper, lower, mu, sig);
        return eval_stats(cmd, out);
      }
      if (gtp && gxlt) {
        double lower = read_num(gtp + (gtp[4] == '=' ? 5 : 4));
        double upper = read_num(gxlt + (gxlt[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcondbetween(%.10g,%.10g,%.10g,%.10g,%.10g,-1)", lower, upper, upper, mu, sig);
        return eval_stats(cmd, out);
      }
    }
  }
  if (starts3(s, "outliers(", "iqrfences(", "outlierfences(") && na >= 2) {
    double q1=num(a[0]), q3=num(a[1]), iqr=q3-q1, lo=q1-1.5*iqr, hi=q3+1.5*iqr;
    int n = add(out, 0, "Use the 1.5*IQR rule for outliers.");
    n = add(out, n, "IQR = Q3 - Q1 = %.6g - %.6g = %.10g", q3, q1, iqr);
    n = add(out, n, "lower fence = %.10g", lo);
    n = add(out, n, "upper fence = %.10g", hi);
    for (int i = 2; i < na && n < P3_MAX_LINES; ++i) {
      double x = num(a[i]);
      n = add(out, n, "%.6g is %s", x, (x < lo || x > hi) ? "an outlier" : "not an outlier");
    }
    return n;
  }
  if (starts2(s, "normal_work(", "normalwork(") && na >= 3) {
    char cmd[160];
    if (na >= 4) sprintf(cmd, "normalprob(%s,%s,%s,%s)", a[0], a[1], a[2], a[3]);
    else sprintf(cmd, "normal(%s,%s,%s)", a[0], a[1], a[2]);
    return eval_stats(cmd, out);
  }
  if (starts2(s, "binom_work(", "binomwork(") && na >= 3) {
    char cmd[160];
    if (na >= 4) sprintf(cmd, "binomtail(%s,%s,%s,%s)", a[0], a[1], a[2], a[3]);
    else sprintf(cmd, "binom(%s,%s,%s)", a[0], a[1], a[2]);
    return eval_stats(cmd, out);
  }
  if (starts2(s, "prob_work(", "probwork(") && na >= 2) {
    char cmd[160];
    if (na >= 3) sprintf(cmd, "probor(%s,%s,%s)", a[0], a[1], a[2]);
    else sprintf(cmd, "cond(%s,%s)", a[0], a[1]);
    return eval_stats(cmd, out);
  }
  if (starts2(s, "regress_work(", "regresswork(") && na >= 3) {
    char cmd[192];
    if (na >= 5) {
      if (na >= 6) sprintf(cmd, "regresscalc(%s,%s,%s,%s,%s,%s)", a[0], a[1], a[2], a[3], a[4], a[5]);
      else sprintf(cmd, "regresscalc(%s,%s,%s,%s,%s)", a[0], a[1], a[2], a[3], a[4]);
    } else sprintf(cmd, "regress(%s,%s,%s)", a[0], a[1], a[2]);
    return eval_stats(cmd, out);
  }
  if (starts2(s, "hyp_test(", "hyptest(") && na >= 5) {
    char cmd[192];
    if (na >= 6) sprintf(cmd, "hypnormal(%s,%s,%s,%s,%s,%s)", a[0], a[1], a[2], a[3], a[4], a[5]);
    else sprintf(cmd, "hypbinom(%s,%s,%s,%s,%s)", a[0], a[1], a[2], a[3], a[4]);
    return eval_stats(cmd, out);
  }
  if (starts3(s, "normalvar(", "normalzvar(", "zscorevar(") && na >= 3) {
    double x=num(a[0]), mu=num(a[1]), var=num(a[2]), sig=root(var);
    int n = add(out, 0, "Convert variance to standard deviation first.");
    n = add(out, n, "sigma = sqrt(%.6g) = %.6g", var, sig);
    n = add(out, n, "Standardise using Z=(X-mu)/sigma.");
    return add(out, n, "z = (%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
  }
  if (starts3(s, "normal(", "normalz(", "zscore(") && na >= 3) {
    double x=num(a[0]), mu=num(a[1]), sig=num(a[2]);
    int n = add(out, 0, "Standardise using Z=(X-mu)/sigma.");
    return add(out, n, "z = (%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
  }
  if (starts3(s, "binomstats(", "binommeanvar(", "binomialstats(") && na >= 2) {
    int N=(int)num(a[0]); double p=num(a[1]), mean=N*p, var=N*p*(1-p);
    int n = add(out, 0, "For X ~ B(n,p), use E(X)=np and Var(X)=np(1-p).");
    n = add(out, n, "E(X) = %.6g*%.6g = %.10g", (double)N, p, mean);
    n = add(out, n, "Var(X) = %.6g*%.6g*(1-%.6g) = %.10g", (double)N, p, p, var);
    return add(out, n, "sd = sqrt(Var(X)) = %.10g", root(var));
  }
  if (starts3(s, "binomparams(", "binomfrommeanvar(", "binomialparams(") && na >= 2) {
    double mean=num(a[0]), var=num(a[1]);
    if (mean == 0) return add(out, 0, "Need non-zero mean.");
    double one_minus_p = var / mean;
    double p = 1.0 - one_minus_p;
    double N = p == 0 ? 0 : mean / p;
    int n = add(out, 0, "For X~B(n,p), use E(X)=np and Var(X)=np(1-p).");
    n = add(out, n, "mean = np = %.10g", mean);
    n = add(out, n, "variance = np(1-p) = %.10g", var);
    n = add(out, n, "1-p = variance/mean = %.10g/%.10g = %.10g", var, mean, one_minus_p);
    n = add(out, n, "p = 1 - %.10g = %.10g", one_minus_p, p);
    return add(out, n, "n = mean/p = %.10g/%.10g = %.10g", mean, p, N);
  }
  if (starts3(s, "binom(", "binomial(", "binompdf(") && na >= 3) {
    int N=(int)num(a[0]), r=(int)num(a[2]); double p=num(a[1]), ans=binomp(N,p,r);
    int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
    n = add(out, n, "P(X=%d) = nCr p^r(1-p)^(n-r)", r);
    return add(out, n, "= %dC%d*%.6g^%d*%.6g^%d = %.10g", N, r, p, r, 1-p, N-r, ans);
  }
  if (starts3(s, "binomcdf(", "binomialcdf(", "bincdf(") && na >= 3) {
    int N=(int)num(a[0]), r=(int)num(a[2]); double p=num(a[1]), ans=0;
    for (int k=0;k<=r;++k) ans += binomp(N,p,k);
    int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
    n = add(out, n, "P(X<=%d) = sum P(X=r), r=0..%d.", r, r);
    return add(out, n, "= %.10g", ans);
  }
  if (starts3(s, "binomtail(", "binomialtail(", "bintail(") && na >= 4) {
    int N=(int)num(a[0]), r=(int)num(a[2]), tail=(int)num(a[3]); double p=num(a[1]), ans=0;
    int lo = tail == 2 ? r + 1 : tail == 1 ? r : 0;
    int hi = tail == -2 ? r - 1 : tail == -1 ? r : N;
    for (int k=lo;k<=hi;++k) ans += binomp(N,p,k);
    int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
    n = add(out, n, tail == 2 ? "P(X>%d)" : tail == 1 ? "P(X>=%d)" : tail == -2 ? "P(X<%d)" : "P(X<=%d)", r);
    return add(out, n, "sum from %d to %d = %.10g", lo, hi, ans);
  }
  if (starts3(s, "binomnorm(", "normalapproxbinom(", "binomnormal(") && na >= 4) {
    int N=(int)num(a[0]); double p=num(a[1]), lo=num(a[2]), hi=num(a[3]);
    double mu=N*p, sig=root(N*p*(1-p)), clo=lo-0.5, chi=hi+0.5;
    int n = add(out, 0, "Use normal approximation to X ~ B(%d, %.6g).", N, p);
    n = add(out, n, "mu = np = %.6g, sigma = sqrt(np(1-p)) = %.6g", mu, sig);
    n = add(out, n, "continuity correction: P(%.6g<=X<=%.6g)", lo, hi);
    n = add(out, n, "use %.6g < Y < %.6g", clo, chi);
    n = add(out, n, "z1=(%.6g-%.6g)/%.6g = %.6g", clo, mu, sig, (clo-mu)/sig);
    n = add(out, n, "z2=(%.6g-%.6g)/%.6g = %.6g", chi, mu, sig, (chi-mu)/sig);
    return add(out, n, "NormalCD(lower=%.6g, upper=%.6g, sigma=%.6g, mu=%.6g)", clo, chi, sig, mu);
  }
  if (starts3(s, "poissonapprox(", "poissonapproxbinom(", "binompoisson(") && na >= 3) {
    int N=(int)num(a[0]), r=(int)num(a[2]), tail=na>3?(int)num(a[3]):0; double p=num(a[1]), lam=N*p, ans=0;
    if (tail == 0) ans = poissonp(lam, r);
    else ans = poissontailprob(lam, r, tail);
    int n = add(out, 0, "Use Poisson approximation to X ~ B(%d, %.6g).", N, p);
    n = add(out, n, "lambda = np = %d*%.6g = %.6g", N, p, lam);
    n = add(out, n, "Let Y ~ Po(%.6g).", lam);
    n = add(out, n, tail == 2 ? "Use P(Y>%d)" : tail == 1 ? "Use P(Y>=%d)" : tail == -2 ? "Use P(Y<%d)" : tail == -1 ? "Use P(Y<=%d)" : "Use P(Y=%d)", r);
    return add(out, n, "approx probability = %.10g", ans);
  }
  if (starts3(s, "critbinom(", "criticalbinom(", "criticalregion(") && na >= 4) {
    int N=(int)num(a[0]); double p=num(a[1]), alpha=num(a[2]), tail=num(a[3]);
    double cum = 0, bestp = 0; int crit = tail < 0 ? -1 : N + 1;
    if (tail < 0) {
      for (int k=0;k<=N;++k) { cum += binomp(N,p,k); if (cum <= alpha) { crit = k; bestp = cum; } else break; }
    } else {
      cum = 0;
      for (int k=N;k>=0;--k) { cum += binomp(N,p,k); if (cum <= alpha) { crit = k; bestp = cum; } else break; }
    }
    int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
    n = add(out, n, tail < 0 ? "Lower tail: find largest c with P(X<=c)<=alpha." : "Upper tail: find smallest c with P(X>=c)<=alpha.");
    n = add(out, n, "alpha = %.6g, tail probability = %.10g", alpha, bestp);
    if ((tail < 0 && crit < 0) || (tail >= 0 && crit > N)) return add(out, n, "no critical value at this alpha.");
    return add(out, n, tail < 0 ? "critical region: X <= %d" : "critical region: X >= %d", crit);
  }
  if (starts3(s, "critpoisson(", "criticalpoisson(", "poissoncrit(") && na >= 3) {
    double lam=num(a[0]), alpha=num(a[1]), tail=num(a[2]);
    double bestp = 0; int crit = tail < 0 ? -1 : 999;
    if (tail < 0) {
      for (int k=0;k<80;++k) { double p = poissoncdf(lam, k); if (p <= alpha) { crit = k; bestp = p; } else break; }
    } else {
      for (int k=79;k>=0;--k) { double p = poissontailprob(lam, k, 1); if (p <= alpha) { crit = k; bestp = p; } }
    }
    int n = add(out, 0, "Let X ~ Po(%.6g).", lam);
    n = add(out, n, tail < 0 ? "Lower tail: find largest c with P(X<=c)<=alpha." : "Upper tail: find smallest c with P(X>=c)<=alpha.");
    n = add(out, n, "alpha = %.6g, tail probability = %.10g", alpha, bestp);
    if ((tail < 0 && crit < 0) || (tail >= 0 && crit == 999)) return add(out, n, "no critical value at this alpha.");
    return add(out, n, tail < 0 ? "critical region: X <= %d" : "critical region: X >= %d", crit);
  }
  if (starts3(s, "hypbinom(", "binomtest(", "hypothesistest(") && na >= 5) {
    int N=(int)num(a[0]), x=(int)num(a[2]); double p=num(a[1]), alpha=num(a[3]), tail=num(a[4]), prob=0;
    if (tail < 0) for (int k=0;k<=x;++k) prob += binomp(N,p,k);
    else if (tail > 0) for (int k=x;k<=N;++k) prob += binomp(N,p,k);
    else {
      double lo=0, hi=0;
      for (int k=0;k<=x;++k) lo += binomp(N,p,k);
      for (int k=x;k<=N;++k) hi += binomp(N,p,k);
      prob = 2 * (lo < hi ? lo : hi);
      if (prob > 1) prob = 1;
    }
    int n = add(out, 0, "H0: p = %.6g.", p);
    n = add(out, n, tail == 0 ? "H1: p is different." : tail > 0 ? "H1: p is greater." : "H1: p is smaller.");
    if (tail != 0) n = add(out, n, tail > 0 ? "Use P(X>=%d)." : "Use P(X<=%d).", x);
    n = add(out, n, tail == 0 ? "two-tailed probability = %.10g" : "tail probability = %.10g", prob);
    n = add(out, n, "Compare with alpha = %.6g.", alpha);
    return add(out, n, prob <= alpha ? "Reject H0 in context." : "Do not reject H0 in context.");
  }
  if (starts3(s, "hyppoisson(", "poissontest(", "poissonhyp(") && na >= 4) {
    double lam=num(a[0]), alpha=num(a[2]), tail=num(a[3]); int x=(int)num(a[1]);
    if (alpha > 1 && alpha <= 100) alpha /= 100.0;
    double prob = poissontailprob(lam, x, tail < 0 ? -1 : 1);
    int n = add(out, 0, "H0: lambda = %.6g.", lam);
    n = add(out, n, tail < 0 ? "H1: lambda is smaller." : "H1: lambda is greater.");
    n = add(out, n, tail < 0 ? "Use P(X<=%d)." : "Use P(X>=%d).", x);
    n = add(out, n, "tail probability = %.10g", prob);
    n = add(out, n, "Compare with alpha = %.6g.", alpha);
    return add(out, n, prob <= alpha ? "Reject H0 in context." : "Do not reject H0 in context.");
  }
  if (starts3(s, "normalprob(", "normalcdf(", "normint(") && na >= 4) {
    double lo=num(a[0]), hi=num(a[1]), mu=num(a[2]), sig=num(a[3]);
    int n = add(out, 0, "For X~N(mu,sigma^2), standardise both bounds.");
    n = add(out, n, "z1=(%.6g-%.6g)/%.6g = %.6g", lo, mu, sig, (lo-mu)/sig);
    n = add(out, n, "z2=(%.6g-%.6g)/%.6g = %.6g", hi, mu, sig, (hi-mu)/sig);
    n = add(out, n, "Use fx-CG50 Normal CDF with lower, upper, sigma, mu.");
    return add(out, n, "NormalCD(lower=%.6g, upper=%.6g, sigma=%.6g, mu=%.6g)", lo, hi, sig, mu);
  }
  if (starts3(s, "normalprobvar(", "normalcdfvar(", "normintvar(") && na >= 4) {
    double lo=num(a[0]), hi=num(a[1]), mu=num(a[2]), var=num(a[3]), sig=root(var);
    int n = add(out, 0, "Convert variance to standard deviation first.");
    n = add(out, n, "sigma = sqrt(%.6g) = %.6g", var, sig);
    n = add(out, n, "z1=(%.6g-%.6g)/%.6g = %.6g", lo, mu, sig, (lo-mu)/sig);
    n = add(out, n, "z2=(%.6g-%.6g)/%.6g = %.6g", hi, mu, sig, (hi-mu)/sig);
    return add(out, n, "NormalCD(lower=%.6g, upper=%.6g, sigma=%.6g, mu=%.6g)", lo, hi, sig, mu);
  }
  if (starts3(s, "normaltail(", "normalupper(", "normaltp(") && na >= 4) {
    double x=num(a[0]), mu=num(a[1]), sig=num(a[2]), tail=num(a[3]);
    int n = add(out, 0, "For X~N(mu,sigma^2), choose the correct tail.");
    n = add(out, n, "z=(%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
    n = add(out, n, tail >= 0 ? "This is an upper-tail probability." : "This is a lower-tail probability.");
    n = add(out, n, tail >= 0 ? "Required probability is P(X>=%.6g)." : "Required probability is P(X<=%.6g).", x);
    return add(out, n, tail >= 0 ? "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.6g, mu=%.6g)" : "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.6g, mu=%.6g)", x, sig, mu);
  }
  if (starts3(s, "normaltailvar(", "normaluppervar(", "normaltpvar(") && na >= 4) {
    double x=num(a[0]), mu=num(a[1]), var=num(a[2]), sig=root(var), tail=num(a[3]);
    int n = add(out, 0, "Convert variance to standard deviation first.");
    n = add(out, n, "sigma = sqrt(%.6g) = %.6g", var, sig);
    n = add(out, n, "z=(%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
    n = add(out, n, tail >= 0 ? "This is an upper-tail probability." : "This is a lower-tail probability.");
    n = add(out, n, tail >= 0 ? "Required probability is P(X>=%.6g)." : "Required probability is P(X<=%.6g).", x);
    return add(out, n, tail >= 0 ? "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.6g, mu=%.6g)" : "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.6g, mu=%.6g)", x, sig, mu);
  }
  if (starts3(s, "normalcond(", "normalgiven(", "normalconditional(") && na >= 5) {
    double x=num(a[0]), g=num(a[1]), mu=num(a[2]), sig=num(a[3]), tail=num(a[4]);
    double den, nume, bound;
    int n = add(out, 0, "Use conditional probability P(A|B)=P(A and B)/P(B).");
    n = add(out, n, "X~N(%.6g, %.6g^2).", mu, sig);
    if (tail >= 0) {
      bound = x > g ? x : g;
      den = 1 - normal_cdf((g - mu) / sig);
      nume = 1 - normal_cdf((bound - mu) / sig);
      n = add(out, n, "A: X>=%.6g, B: X>=%.6g, so A and B is X>=%.6g.", x, g, bound);
      n = add(out, n, "P(B)=P(X>=%.6g): z=(%.6g-%.6g)/%.6g=%.6g", g, g, mu, sig, (g-mu)/sig);
      n = add(out, n, "P(A and B)=P(X>=%.6g): z=%.6g", bound, (bound-mu)/sig);
      n = add(out, n, "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.6g, mu=%.6g)", g, sig, mu);
    } else {
      bound = x < g ? x : g;
      den = normal_cdf((g - mu) / sig);
      nume = normal_cdf((bound - mu) / sig);
      n = add(out, n, "A: X<=%.6g, B: X<=%.6g, so A and B is X<=%.6g.", x, g, bound);
      n = add(out, n, "P(B)=P(X<=%.6g): z=(%.6g-%.6g)/%.6g=%.6g", g, g, mu, sig, (g-mu)/sig);
      n = add(out, n, "P(A and B)=P(X<=%.6g): z=%.6g", bound, (bound-mu)/sig);
      n = add(out, n, "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.6g, mu=%.6g)", g, sig, mu);
    }
    n = add(out, n, "P(A|B)=%.10g/%.10g", nume, den);
    return add(out, n, "conditional probability = %.10g", den ? nume/den : 0);
  }
  if (starts3(s, "normalcondbetween(", "normalgivencap(", "normalconditionalbetween(") && na >= 6) {
    double lo=num(a[0]), hi=num(a[1]), g=num(a[2]), mu=num(a[3]), sig=num(a[4]), tail=num(a[5]);
    double den = tail >= 0 ? 1 - normal_cdf((g - mu) / sig) : normal_cdf((g - mu) / sig);
    double alo = lo, ahi = hi;
    if (tail >= 0 && alo < g) alo = g;
    if (tail < 0 && ahi > g) ahi = g;
    double nume = ahi > alo ? normal_cdf((ahi - mu) / sig) - normal_cdf((alo - mu) / sig) : 0;
    int n = add(out, 0, "Use conditional probability P(A|B)=P(A and B)/P(B).");
    n = add(out, n, "X~N(%.6g, %.6g^2).", mu, sig);
    n = add(out, n, "A: %.6g<X<%.6g, B: %s %.6g.", lo, hi, tail >= 0 ? "X>=" : "X<=", g);
    n = add(out, n, "A and B gives %.6g<X<%.6g.", alo, ahi);
    n = add(out, n, "P(B)=%.10g", den);
    n = add(out, n, "P(A and B)=NormalCD(lower=%.6g, upper=%.6g, sigma=%.6g, mu=%.6g)", alo, ahi, sig, mu);
    n = add(out, n, "P(A|B)=%.10g/%.10g", nume, den);
    return add(out, n, "conditional probability = %.10g", den ? nume/den : 0);
  }
  if (starts3(s, "invnormal(", "inversenormal(", "normalinv(") && na >= 3) {
    double area=num(a[0]), mu=num(a[1]), sig=num(a[2]);
    double z = inv_norm_left(area);
    int n = add(out, 0, "For X~N(mu,sigma^2), use inverse normal for a critical value.");
    n = add(out, n, "Area to the left = %.6g.", area);
    n = add(out, n, "Use fx-CG50 InvNorm(area, sigma, mu).");
    n = add(out, n, "InvNorm(%.6g, %.6g, %.6g)", area, sig, mu);
    n = add(out, n, "z = InvNorm(%.6g) = %.10g", area, z);
    return add(out, n, "x = %.6g + %.6g*%.10g = %.10g", mu, sig, z, mu + sig*z);
  }
  if (starts3(s, "invnormalvar(", "inversenormalvar(", "normalinvvar(") && na >= 3) {
    double area=num(a[0]), mu=num(a[1]), var=num(a[2]), sig=root(var);
    double z = inv_norm_left(area);
    int n = add(out, 0, "Convert variance to standard deviation first.");
    n = add(out, n, "sigma = sqrt(%.6g) = %.6g", var, sig);
    n = add(out, n, "Area to the left = %.6g.", area);
    n = add(out, n, "InvNorm(%.6g, %.6g, %.6g)", area, sig, mu);
    n = add(out, n, "z = InvNorm(%.6g) = %.10g", area, z);
    return add(out, n, "x = %.6g + %.6g*%.10g = %.10g", mu, sig, z, mu + sig*z);
  }
  if (starts3(s, "normalparams(", "normalparameters(", "normalmeansd(") && na >= 4) {
    double x1=num(a[0]), p1=num(a[1]), x2=num(a[2]), p2=num(a[3]);
    double z1=inv_norm_left(p1), z2=inv_norm_left(p2);
    int n = add(out, 0, "Let X~N(mu,sigma^2). Use z=(x-mu)/sigma.");
    n = add(out, n, "P(X<=%.6g)=%.6g -> z1=InvNorm(%.6g)=%.6g", x1, p1, p1, z1);
    n = add(out, n, "(%.6g-mu)/sigma = %.6g", x1, z1);
    n = add(out, n, "P(X<=%.6g)=%.6g -> z2=InvNorm(%.6g)=%.6g", x2, p2, p2, z2);
    n = add(out, n, "(%.6g-mu)/sigma = %.6g", x2, z2);
    if (z2 == z1) return add(out, n, "Need two different probabilities.");
    double sig=(x2-x1)/(z2-z1), mu=x1-z1*sig;
    n = add(out, n, "sigma = (%.6g-%.6g)/(%.6g-%.6g) = %.10g", x2, x1, z2, z1, sig);
    n = add(out, n, "mu = %.6g - %.6g*sigma = %.10g", x1, z1, mu);
    return add(out, n, "mean = %.10g, sd = %.10g", mu, sig);
  }
  if (starts3(s, "hypnormal(", "normaltest(", "hypmean(") && na >= 6) {
    double xb=num(a[0]), mu=num(a[1]), sig=num(a[2]), nn=num(a[3]), alpha=num(a[4]), tail=num(a[5]);
    double se = sig/root(nn), z = (xb-mu)/se;
    double left = normal_cdf(z), right = 1 - left;
    double small = left < right ? left : right;
    double pval = tail > 0 ? right : (tail < 0 ? left : 2 * small);
    if (pval > 1) pval = 1;
    bool reject = pval <= alpha;
    int n = add(out, 0, "H0: mu = %.6g. H1: %s.", mu, tail > 0 ? "mu is greater" : (tail < 0 ? "mu is less" : "mu is different"));
    n = add(out, n, "standard error = sigma/sqrt(n) = %.6g/sqrt(%.6g) = %.6g", sig, nn, se);
    n = add(out, n, "z = (xbar-mu)/SE = (%.6g-%.6g)/%.6g = %.6g", xb, mu, se, z);
    if (tail > 0) n = add(out, n, "p = P(Z>=%.6g) = %.10g", z, pval);
    else if (tail < 0) n = add(out, n, "p = P(Z<=%.6g) = %.10g", z, pval);
    else n = add(out, n, "two-tailed p = 2*min(P(Z<=z),P(Z>=z)) = %.10g", pval);
    n = add(out, n, "Reject H0 if p <= alpha %.6g.", alpha);
    return add(out, n, reject ? "Reject H0; conclude there is evidence in context." : "Do not reject H0; insufficient evidence in context.");
  }
  if (starts3(s, "cond(", "conditional(", "given(") && na >= 2) {
    double pab=num(a[0]), pb=num(a[1]);
    int n = add(out, 0, "Use P(A|B)=P(A and B)/P(B).");
    return add(out, n, "P(A|B)=%.6g/%.6g=%.6g", pab, pb, pab/pb);
  }
  if (starts3(s, "probor(", "union(", "aorb(") && na >= 3) {
    double pa=num(a[0]), pb=num(a[1]), pab=num(a[2]);
    int n = add(out, 0, "Use P(A or B)=P(A)+P(B)-P(A and B).");
    return add(out, n, "%.6g+%.6g-%.6g=%.6g", pa, pb, pab, pa+pb-pab);
  }
  if (starts3(s, "bayes(", "bayestheorem(", "reverseconditional(") && na >= 3) {
    double pba=num(a[0]), pa=num(a[1]), pb=num(a[2]);
    int n = add(out, 0, "Use Bayes' theorem: P(A|B)=P(B|A)P(A)/P(B).");
    return add(out, n, "P(A|B)=%.6g*%.6g/%.6g=%.10g", pba, pa, pb, pba*pa/pb);
  }
  if (starts3(s, "independent(", "independence(", "testindependent(") && na >= 3) {
    double pa=num(a[0]), pb=num(a[1]), pab=num(a[2]), prod=pa*pb;
    double diff = pab > prod ? pab - prod : prod - pab;
    int n = add(out, 0, "For independence, check whether P(A and B)=P(A)P(B).");
    n = add(out, n, "P(A)P(B)=%.6g*%.6g=%.10g", pa, pb, prod);
    return add(out, n, diff < 1e-9 ? "Since %.10g = %.10g, A and B are independent." : "Since %.10g != %.10g, A and B are not independent.", pab, prod);
  }
  if (starts2(s, "poisson(", "poissonpdf(") && na >= 2) {
    double lam=num(a[0]); int r=(int)num(a[1]);
    int n = add(out, 0, "For X~Po(lambda), P(X=r)=e^-lambda lambda^r/r!.");
    return add(out, n, "P(X=%d)=e^-%.6g*%.6g^%d/%d! = %.10g", r, lam, lam, r, r, poissonp(lam,r));
  }
  if (starts3(s, "poissonstats(", "poissonmeanvar(", "postats(") && na >= 1) {
    double lam=num(a[0]);
    int n = add(out, 0, "For X ~ Po(lambda), E(X)=lambda and Var(X)=lambda.");
    n = add(out, n, "E(X) = %.10g", lam);
    n = add(out, n, "Var(X) = %.10g", lam);
    return add(out, n, "sd = sqrt(lambda) = %.10g", root(lam));
  }
  if (starts3(s, "poissoncdf(", "poissonle(", "poissontail(") && na >= 2) {
    double lam=num(a[0]); int r=(int)num(a[1]), tail=na>2?(int)num(a[2]):-1; double ans=0;
    int lo = tail == 2 ? r + 1 : tail == 1 ? r : 0;
    int hi = tail == -2 ? r - 1 : tail == -1 ? r : r + 40;
    if (tail < 0) ans = poissoncdf(lam, hi) - poissoncdf(lam, lo - 1);
    else ans = 1 - poissoncdf(lam, lo - 1);
    int n = add(out, 0, "For X~Po(%.6g), use cumulative probabilities.", lam);
    n = add(out, n, tail == 2 ? "P(X>%d)=1-P(X<=%d)" : tail == 1 ? "P(X>=%d)=1-P(X<=%d)" : tail == -2 ? "P(X<%d)" : "P(X<=%d)", r, lo-1);
    return add(out, n, "= %.10g", ans);
  }
  if (starts3(s, "poissonrange(", "poissonbetween(", "porange(") && na >= 3) {
    double lam=num(a[0]); int lo=(int)num(a[1]), hi=(int)num(a[2]);
    if (lo > hi) { int q = lo; lo = hi; hi = q; }
    double ans = poissoncdf(lam, hi) - poissoncdf(lam, lo - 1);
    int n = add(out, 0, "For X~Po(%.6g), use cumulative probabilities.", lam);
    n = add(out, n, "P(%d<=X<=%d)=P(X<=%d)-P(X<=%d)", lo, hi, hi, lo - 1);
    return add(out, n, "= %.10g", ans);
  }
  if (starts3(s, "poissonnorm(", "normalapproxpoisson(", "poissonnormal(") && na >= 3) {
    double lam=num(a[0]), lo=num(a[1]), hi=num(a[2]), sig=root(lam), clo=lo-0.5, chi=hi+0.5;
    int n = add(out, 0, "Use normal approximation to X ~ Po(%.6g).", lam);
    n = add(out, n, "mu = lambda = %.6g, sigma = sqrt(lambda) = %.6g", lam, sig);
    n = add(out, n, "continuity correction: use %.6g < Y < %.6g", clo, chi);
    n = add(out, n, "z1=(%.6g-%.6g)/%.6g = %.6g", clo, lam, sig, (clo-lam)/sig);
    n = add(out, n, "z2=(%.6g-%.6g)/%.6g = %.6g", chi, lam, sig, (chi-lam)/sig);
    return add(out, n, "NormalCD(lower=%.6g, upper=%.6g, sigma=%.6g, mu=%.6g)", clo, chi, sig, lam);
  }
  if (starts3(s, "regress(", "regression(", "predict(") && na >= 3) {
    double a0=num(a[0]), b=num(a[1]), x=num(a[2]);
    int n = add(out, 0, "Use the regression line y = a + bx.");
    n = add(out, n, "y = %.6g + %.6g x", a0, b);
    return add(out, n, "when x=%.6g, y=%.6g", x, a0+b*x);
  }
  if (starts3(s, "regresscalc(", "regressionline(", "lobf(") && na >= 5) {
    double n0=num(a[0]), sx=num(a[1]), sy=num(a[2]), sxx=num(a[3]), sxy=num(a[4]);
    double xb=sx/n0, yb=sy/n0, b=sxy/sxx, a0=yb-b*xb;
    int n = add(out, 0, "Find least-squares regression line y = a + bx.");
    n = add(out, n, "xbar = Sx/n = %.6g/%.6g = %.6g", sx, n0, xb);
    n = add(out, n, "ybar = Sy/n = %.6g/%.6g = %.6g", sy, n0, yb);
    n = add(out, n, "b = Sxy/Sxx = %.6g/%.6g = %.6g", sxy, sxx, b);
    n = add(out, n, "a = ybar - b*xbar = %.6g", a0);
    n = add(out, n, "regression line: y = %.6g + %.6g x", a0, b);
    if (na >= 6) n = add(out, n, "when x=%.6g, y=%.6g", num(a[5]), a0+b*num(a[5]));
    return n;
  }
  if (starts3(s, "regresss(", "regresssummary(", "regsummary(") && na >= 4) {
    double xb=num(a[0]), yb=num(a[1]), sxx=num(a[2]), sxy=num(a[3]);
    double b=sxy/sxx, a0=yb-b*xb;
    int n = add(out, 0, "Find least-squares regression line y = a + bx.");
    n = add(out, n, "b = Sxy/Sxx = %.6g/%.6g = %.6g", sxy, sxx, b);
    n = add(out, n, "a = ybar - b*xbar = %.6g - %.6g*%.6g = %.6g", yb, b, xb, a0);
    n = add(out, n, "regression line: y = %.6g + %.6g x", a0, b);
    if (na >= 5) n = add(out, n, "when x=%.6g, y=%.6g", num(a[4]), a0+b*num(a[4]));
    return n;
  }
  if (starts3(s, "pmcc(", "correlation(", "productmoment(") && na >= 6) {
    double n0=num(a[0]), sx=num(a[1]), sy=num(a[2]), sxy=num(a[3]), sx2=num(a[4]), sy2=num(a[5]);
    double top = n0*sxy - sx*sy, bx = n0*sx2 - sx*sx, by = n0*sy2 - sy*sy;
    int n = add(out, 0, "Use product moment correlation coefficient.");
    n = add(out, n, "r=(nSxy-SxSy)/sqrt((nSx2-Sx^2)(nSy2-Sy^2))");
    n = add(out, n, "top = %.6g*%.6g - %.6g*%.6g = %.6g", n0, sxy, sx, sy, top);
    return add(out, n, "r = %.10g", top / root(bx * by));
  }
  if (starts3(s, "pmccs(", "correlations(", "productmoments(") && na >= 3) {
    double sxx=num(a[0]), syy=num(a[1]), sxy=num(a[2]);
    int n = add(out, 0, "Use PMCC from summary values.");
    n = add(out, n, "r = Sxy/sqrt(Sxx*Syy)");
    return add(out, n, "r = %.6g/sqrt(%.6g*%.6g) = %.10g", sxy, sxx, syy, sxy/root(sxx*syy));
  }
  if (starts3(s, "spearman(", "spearmanrank(", "rankcorr(") && na >= 2) {
    double n0=num(a[0]), sd2=num(a[1]);
    double den = n0 * (n0*n0 - 1);
    double r = 1 - 6*sd2/den;
    int n = add(out, 0, "Use Spearman's rank correlation coefficient.");
    n = add(out, n, "r_s = 1 - 6*sum(d^2)/(n(n^2-1))");
    n = add(out, n, "r_s = 1 - 6*%.6g/(%.6g(%.6g^2-1))", sd2, n0, n0);
    return add(out, n, "r_s = %.10g", r);
  }
  if (starts3(s, "corrtest(", "correlationtest(", "pmcctest(") && na >= 2) {
    double r=num(a[0]), crit=num(a[1]), tail=na>2?num(a[2]):0;
    double ar = r < 0 ? -r : r;
    bool reject = tail > 0 ? r > crit : (tail < 0 ? r < -crit : ar > crit);
    int n = add(out, 0, "Test for correlation using the critical value.");
    n = add(out, n, "H0: rho = 0, H1: %s", tail > 0 ? "rho > 0" : (tail < 0 ? "rho < 0" : "rho != 0"));
    if (tail == 0) n = add(out, n, "Compare |r| with critical value: |%.6g| = %.6g", r, ar);
    else n = add(out, n, "Compare r with critical value in the %s tail.", tail > 0 ? "positive" : "negative");
    n = add(out, n, "%s critical value %.6g", reject ? "Reject H0 because test statistic passes" : "Do not reject H0 because test statistic does not pass", crit);
    return add(out, n, reject ? "There is evidence of correlation in context." : "There is insufficient evidence of correlation in context.");
  }
  if (starts3(s, "meanvar(", "variance(", "summary(") && na >= 3) {
    double n0=num(a[0]), sx=num(a[1]), sx2=num(a[2]), mean=sx/n0, var=sx2/n0-mean*mean;
    int n = add(out, 0, "Use summary statistics formulae.");
    n = add(out, n, "mean = Sx/n = %.6g/%.6g = %.6g", sx, n0, mean);
    n = add(out, n, "variance = Sx2/n - mean^2");
    return add(out, n, "variance = %.10g, sd = %.10g", var, root(var));
  }
  if (starts3(s, "groupmean(", "groupedmean(", "groupstats(") && na >= 4) {
    int m = na / 2; double sf = 0, sfx = 0, sfx2 = 0;
    char l1[128] = "", l2[128] = "";
    for (int i = 0; i < m; ++i) {
      double x = num(a[2*i]), f = num(a[2*i+1]);
      sf += f; sfx += f*x; sfx2 += f*x*x;
      add_term(l1, sizeof(l1), "%.6g*%.6g", f, x, i == 0);
      add_term(l2, sizeof(l2), "%.6g*(%.6g)^2", f, x, i == 0);
    }
    double mean = sfx / sf, var = sfx2 / sf - mean * mean;
    int n = add(out, 0, "For grouped data, use class midpoints with frequencies.");
    n = add(out, n, "sum f = %.10g", sf);
    n = add(out, n, "sum fx = %s = %.10g", l1, sfx);
    n = add(out, n, "mean = sum fx / sum f = %.10g", mean);
    n = add(out, n, "sum fx^2 = %s = %.10g", l2, sfx2);
    n = add(out, n, "variance = sum fx^2/sum f - mean^2 = %.10g", var);
    return add(out, n, "sd = sqrt(sum fx^2/sum f - mean^2) = %.10g", root(var));
  }
  if (starts3(s, "discrete(", "expectation(", "randomvar(") && na >= 4) {
    int m = na / 2; double sp = 0, ex = 0, ex2 = 0; char l1[128] = "", l2[128] = "";
    for (int i = 0; i < m; ++i) {
      double x = num(a[2*i]), p = num(a[2*i+1]);
      sp += p; ex += x*p; ex2 += x*x*p;
      add_term(l1, sizeof(l1), "%.6g*%.6g", x, p, i == 0);
      add_term(l2, sizeof(l2), "(%.6g)^2*%.6g", x, p, i == 0);
    }
    int n = add(out, 0, "For a discrete random variable, use E(X)=sum xp.");
    n = add(out, n, "Check probabilities: sum p = %.10g", sp);
    n = add(out, n, "E(X) = %s = %.10g", l1, ex);
    n = add(out, n, "E(X^2) = %s = %.10g", l2, ex2);
    return add(out, n, "variance = Var(X)=E(X^2)-E(X)^2 = %.10g", ex2 - ex*ex);
  }
  if (starts3(s, "stratified(", "stratifiedsample(", "stratum(") && na >= 3) {
    double pop=num(a[0]), group=num(a[1]), sample=num(a[2]), ans=group/pop*sample;
    int n = add(out, 0, "For stratified sampling, use group/population * sample size.");
    n = add(out, n, "sample from group = %.6g/%.6g * %.6g", group, pop, sample);
    return add(out, n, "= %.10g, then round sensibly for people/items.", ans);
  }
  if (starts3(s, "groupmedian(", "groupedmedian(", "interpolatemedian(") && na >= 5) {
    double L=num(a[0]), cf=num(a[1]), f=num(a[2]), w=num(a[3]), n0=num(a[4]), pos=n0/2.0;
    int n = add(out, 0, "Use linear interpolation in the median class.");
    n = add(out, n, "position = n/2 = %.6g/2 = %.6g", n0, pos);
    n = add(out, n, "median = L + ((position-CF before)/f)*class width");
    return add(out, n, "median = %.6g + ((%.6g-%.6g)/%.6g)*%.6g = %.10g", L, pos, cf, f, w, L + ((pos-cf)/f)*w);
  }
  if (starts3(s, "groupquantile(", "groupedq(", "interpolateq(") && na >= 6) {
    double L=num(a[0]), cf=num(a[1]), f=num(a[2]), w=num(a[3]), n0=num(a[4]), q=num(a[5]), pos=q*n0;
    int n = add(out, 0, "Use linear interpolation in the required quartile/quantile class.");
    n = add(out, n, "position = %.6g*n = %.6g*%.6g = %.6g", q, q, n0, pos);
    n = add(out, n, "value = L + ((position-CF before)/f)*class width");
    return add(out, n, "value = %.6g + ((%.6g-%.6g)/%.6g)*%.6g = %.10g", L, pos, cf, f, w, L + ((pos-cf)/f)*w);
  }
  if (starts3(s, "histdensity(", "frequencydensity(", "freqdensity(") && na >= 2) {
    double f=num(a[0]), w=num(a[1]);
    int n = add(out, 0, "For a histogram, frequency density = frequency / class width.");
    return add(out, n, "frequency density = %.6g/%.6g = %.10g", f, w, f/w);
  }
  if (starts3(s, "histfreq(", "histfrequency(", "frequencyfromdensity(") && na >= 2) {
    double d=num(a[0]), w=num(a[1]);
    int n = add(out, 0, "For a histogram, frequency = frequency density * class width.");
    return add(out, n, "frequency = %.6g*%.6g = %.10g", d, w, d*w);
  }
  if (starts3(s, "uncode(", "decodecoded(", "codedtox(") && na >= 4) {
    double my=num(a[0]), sy=num(a[1]), A=num(a[2]), B=num(a[3]), ab=B < 0 ? -B : B;
    int n = add(out, 0, "For coded data Y=(X-a)/b, rearrange to X=a+bY.");
    n = add(out, n, "mean X = a + b*mean Y");
    n = add(out, n, "mean X = %.6g + %.6g*%.6g = %.6g", A, B, my, A+B*my);
    return add(out, n, "standard deviation X = |b|*standard deviation Y = %.6g*%.6g = %.6g", ab, sy, ab*sy);
  }
  if (starts3(s, "code(", "codex(", "xtocoded(") && na >= 4) {
    double mx=num(a[0]), sx=num(a[1]), A=num(a[2]), B=num(a[3]), ab=B < 0 ? -B : B;
    int n = add(out, 0, "For coded data Y=(X-a)/b.");
    n = add(out, n, "mean Y = (mean X - a)/b");
    n = add(out, n, "mean Y = (%.6g-%.6g)/%.6g = %.6g", mx, A, B, (mx-A)/B);
    return add(out, n, "sd Y = sd X/|b| = %.6g/%.6g = %.6g", sx, ab, sx/ab);
  }
  return 0;
}

static int eval_free_text(const char *input, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char t[192]; raw_clean(input, t, sizeof(t));
  char c[192]; clean(input, c, sizeof(c));
  double v[12]; int nv = scan_nums(t, v, 12);
  char cmd[160];

  if ((has(c, "~n(") || has(c, "x~n(")) && has(c, "|")) {
    double mu0 = 0, second0 = 0;
    if (dist2(c, "~n", &mu0, &second0)) {
      double sd0 = has(c, "^2") ? second0 : root(second0), x0 = 0, g0 = 0;
      const char *bar = strchr(c, '|');
      const char *lp = strstr(c, "x>"), *rp = bar ? strstr(bar, "x>") : 0;
      if (lp && rp && lp < bar) {
        x0 = read_num(lp + (lp[2] == '=' ? 3 : 2));
        g0 = read_num(rp + (rp[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,1)", x0, g0, mu0, sd0);
        return eval_stats(cmd, out);
      }
      lp = strstr(c, "x<"); rp = bar ? strstr(bar, "x<") : 0;
      if (lp && rp && lp < bar) {
        x0 = read_num(lp + (lp[2] == '=' ? 3 : 2));
        g0 = read_num(rp + (rp[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,-1)", x0, g0, mu0, sd0);
        return eval_stats(cmd, out);
      }
    }
  }

  if (!has(t, "variable") && !has(c, "v=(") && !has(t, "projected") && !has(t, "projectile") &&
      (has(t, "velocity") || has(t, "speed")) &&
      (has(t, "increasesfrom") || has(t, "decreasesfrom") || has(t, "from")) &&
      (has(t, "over") || has(t, "distance") || has(t, "metre") || has(t, "meter")) &&
      (has(t, "acceleration") || has(t, "time")) && nv >= 3) {
    double u=0, vv=0, sdist=0;
    bool hu = word_num(input, "from", &u);
    bool hv = word_num(input, "to", &vv);
    bool hs = word_num(input, "over", &sdist) || word_num(input, "distance", &sdist) ||
              prev_word_num(input, "m", &sdist) || prev_word_num(input, "metres", &sdist) ||
              prev_word_num(input, "meters", &sdist);
    if (!hu || !hv || !hs) {
      int start = nv >= 4 && (has(t, "mass") || has(t, "kg")) ? 1 : 0;
      if (!hu) u = v[start];
      if (!hv) vv = v[start+1];
      if (!hs) sdist = v[start+2];
    }
    double acc = (vv*vv - u*u) / (2*sdist);
    double tm = (u + vv) ? 2*sdist/(u+vv) : 0;
    int n = add(out, 0, "Use SUVAT with u, v and s.");
    n = add(out, n, "v^2 = u^2 + 2as");
    n = add(out, n, "a = (%.10g^2-%.10g^2)/(2*%.10g) = %.10g m/s^2", vv, u, sdist, acc);
    n = add(out, n, "s = 1/2(u+v)t");
    return add(out, n, "t = 2*%.10g/(%.10g+%.10g) = %.10g s", sdist, u, vv, tm);
  }

  if ((has(c, "~n(") || has(c, "x~n(") || has(c, "n(") || has(c, "followsn(") || has(c, "distributedn(")) &&
      !has(c, "poisson") && !has(t, "poisson") &&
      (has(c, "p(") || has(t, "probability") || has(c, "<x<") ||
       has(t, "percentile") || has(c, "findk") || has(c, "findx") ||
       has(c, "suchthat")) && nv >= 3) {
    double mu=0, second=0;
    if (!dist2(c, "n(", &mu, &second)) { mu = v[0]; second = v[1]; }
    double var = has(c, "^2") ? second * second : second;
    double u[3]; int nu = 0;
    for (int i = 0; i < nv && nu < 3; ++i) {
      if (near_num(v[i], mu) || near_num(v[i], second)) continue;
      if (has(c, "^2") && near_num(v[i], 2)) continue;
      u[nu++] = v[i];
    }
    if ((has(t, "percentile") || has(c, "findk") || has(c, "findx") || has(c, "suchthat")) && nu >= 1 &&
        !(has(c, "-a<x<") || has(c, "+a") || has(c, "-k<x<") || has(c, "betweenmu") || has(c, "central"))) {
      double area = 0;
      for (int i = 0; i < nu; ++i) {
        if (u[i] > 0 && u[i] < 1) { area = u[i]; break; }
        if (u[i] > 1 && u[i] <= 100 && area == 0) area = u[i] / 100.0;
      }
      if (area > 0) {
        if (has(c, "p(x>k") || has(c, "p(x>=k") || has(c, "morethank") ||
            has(c, "greaterthank") || has(t, "exceeded") || has(t, "above")) {
          int n = 0;
          if (area < 1) n = add(out, 0, "Area to the left = 1 - %.10g = %.10g", area, 1 - area);
          area = 1 - area;
          sprintf(cmd, "invnormalvar(%.10g,%.10g,%.10g)", area, mu, var);
          int m = eval_stats(cmd, out + n);
          return n + m;
        }
        sprintf(cmd, "invnormalvar(%.10g,%.10g,%.10g)", area, mu, var);
        return eval_stats(cmd, out);
      }
    }
    if ((has(c, "-a<x<") || has(c, "+a") || has(c, "central")) && nu >= 1) {
      double prob = 0;
      for (int i = 0; i < nu; ++i) if (u[i] > 0 && u[i] < 1) { prob = u[i]; break; }
      if (prob > 0) {
        double sd = root(var);
        double left = (1 + prob) / 2.0;
        double z = inv_norm_left(left);
        int n = add(out, 0, "For a central normal interval, split the outside probability equally.");
        n = add(out, n, "P(mu-a<X<mu+a)=%.10g", prob);
        n = add(out, n, "area to the left of mu+a = (1+%.10g)/2 = %.10g", prob, left);
        n = add(out, n, "z = InvNorm(%.10g) = %.10g", left, z);
        return add(out, n, "a = z*sigma = %.10g*%.10g = %.10g", z, sd, z*sd);
      }
    }
    if (((has(c, "p(") && (has(c, "<x<") || has(c, "x<"))) || has(c, "between")) && nu >= 2) {
      double lo = u[0], hi = u[1];
      if (lo > hi) { double tmp = lo; lo = hi; hi = tmp; }
      sprintf(cmd, "normalprobvar(%.10g,%.10g,%.10g,%.10g)", lo, hi, mu, var);
      return eval_stats(cmd, out);
    }
    int tail = prob_tail(c, t);
    if (tail && nu >= 1) {
      sprintf(cmd, "normaltailvar(%.10g,%.10g,%.10g,%d)", u[0], mu, var, tail > 0 ? 1 : -1);
      return eval_stats(cmd, out);
    }
  }
  if ((has(t, "workenergy") || (has(t, "work") && has(t, "energy")) ||
       has(t, "stoppingdistance") || (has(t, "stopping") && has(t, "distance"))) &&
      (has(t, "braking") || has(t, "brake") || has(t, "stopping")) &&
      (has(t, "force") || has(t, "resistance")) && (has(t, "rest") || has(t, "stopping")) && nv >= 3) {
    double m=0, u=0, F=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hu=word_num(input,"speed",&u) || word_num(input,"velocity",&u) || word_num(input,"at",&u);
    bool hF=word_num(input,"force",&F) || word_num(input,"brakingforce",&F) ||
            word_num(input,"resistance",&F) || label_num(input,"force",&F) ||
            label_num(input,"resistance",&F);
    if (!hm) m = v[0];
    if (!hu) u = v[1];
    if (!hF) F = v[2];
    double ke = 0.5*m*u*u, s = F ? ke/F : 0;
    int n = add(out, 0, "Use work-energy: work done by braking force removes kinetic energy.");
    n = add(out, n, "initial KE = 1/2*m*u^2 = 1/2*%.6g*%.6g^2 = %.10g J", m, u, ke);
    n = add(out, n, "braking work = F*s = %.10g*s", F);
    return add(out, n, "stopping distance s = %.10g/%.10g = %.10g m", ke, F, s);
  }
  if ((has(t, "acceleration") || has(t, "accn")) && has(c, "/") && has(c, "t+") &&
      (has(c, ")^3") || has(c, "^3")) && nv >= 7) {
    double A = v[0], b = v[1], d = v[2], u = v[4], t0 = v[5], tf = v[6];
    double ant0 = -A / (2.0 * b * (b*t0 + d) * (b*t0 + d));
    double K = u - ant0;
    double antf = -A / (2.0 * b * (b*tf + d) * (b*tf + d));
    double ans = antf + K;
    int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t).");
    n = add(out, n, "a = %.10g/(%.10g t%+.10g)^3", A, b, d);
    n = add(out, n, "v = integral a dt = -%.10g/(2*%.10g(%.10g t%+.10g)^2) + C", A, b, b, d);
    n = add(out, n, "v(%.6g)=%.10g gives C = %.10g", t0, u, K);
    return add(out, n, "v(%.6g) = %.10g", tf, ans);
  }
  if ((has(t, "acceleration") || has(t, "accn")) && has(c, "/(") &&
      (has(t, "velocity") || has(t, "speed")) && (has(t, "att") || has(t, "at") || has(t, "time"))) {
    double rb=0, rd=0; int rpow=0;
    if (parse_linear_den_power(c, 't', &rb, &rd, &rpow) && rpow >= 1) {
      double A = nv > 0 ? v[0] : 1;
      double u0 = 0, tf = 0;
      bool hu = word_num(input, "initialvelocity", &u0) || word_num(input, "velocity", &u0) ||
                word_num(input, "speed", &u0);
      bool ht = word_num_with_t(input, "at", &tf) || word_num_with_t(input, "time", &tf);
      if (!hu && nv >= 2) u0 = v[nv-2];
      if (!ht && nv >= 1) tf = v[nv-1];
      double F0 = linear_den_antideriv(A, rb, rd, rpow, 0);
      double C = u0 - F0;
      double Ft = linear_den_antideriv(A, rb, rd, rpow, tf);
      char den[48]; fmt_linear_var(den, sizeof(den), rb, 't', rd);
      int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t).");
      n = add(out, n, "a = %.10g/(%s)^%d", A, den, rpow);
      if (rpow == 1) n = add(out, n, "v = integral a dt = (%.10g/%.10g)ln|%s| + C", A, rb, den);
      else n = add(out, n, "v = integral a dt = %.10g(%s)^%d/(%.10g*(%d)) + C", A, den, 1-rpow, rb, 1-rpow);
      n = add(out, n, "initial velocity gives C = %.10g", C);
      return add(out, n, "v(%.6g) = %.10g", tf, Ft + C);
    }
  }
  if (has(t, "velocity") && has(t, "displacement") && has(t, "from") && has(c, "/(")) {
    double rb=0, rd=0; int rpow=0;
    if (parse_linear_den_power(c, 't', &rb, &rd, &rpow) && rpow >= 1) {
      double k = nv > 0 ? v[0] : 1;
      double c0 = trailing_constant_after_linear_den(c, 't', rpow);
      double t1 = 0, t2 = 0;
      word_num_with_t(input, "from", &t1); word_num_with_t(input, "to", &t2);
      if (t2 < t1) { double q = t1; t1 = t2; t2 = q; }
      double F1 = linear_den_antideriv(k, rb, rd, rpow, t1) + c0*t1;
      double F2 = linear_den_antideriv(k, rb, rd, rpow, t2) + c0*t2;
      char den[48]; fmt_linear_var(den, sizeof(den), rb, 't', rd);
      int n = add(out, 0, "Displacement is the integral of velocity.");
      n = add(out, n, "v = %.10g/(%s)^%d%+.10g", k, den, rpow, c0);
      if (rpow == 1) n = add(out, n, "integral v dt = (%.10g/%.10g)ln|%s|%+.10g t", k, rb, den, c0);
      else n = add(out, n, "integral v dt = %.10g(%s)^%d/(%.10g*(%d))%+.10g t", k, den, 1-rpow, rb, 1-rpow, c0);
      n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", t1, t2);
      return add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, F2-F1);
    }
  }
  if (has(t, "velocity") && has(t, "acceleration") &&
      (has(t, "at") || has(t, "att") || has(t, "time")) &&
      (has(c, "findacceleration") || has(c, "findaccn") || has(c, "accelerationat")) &&
      !has(t, "vector") && !has(t, "displacement") && !has(t, "distance")) {
    double A=0, B=0, C=0, ta=0;
    bool ok = parse_poly_after_word(input, "velocity", &A, &B, &C);
    if (!ok && has(c, "v=")) ok = parse_velocity_quad(input, &A, &B, &C);
    if (ok) {
      if (!(word_num_with_t(input, "at", &ta) || word_num(input, "at", &ta) ||
            word_num_with_t(input, "time", &ta) || word_num(input, "time", &ta))) {
        ta = nv > 0 ? v[nv-1] : 0;
      }
      int n = add(out, 0, "Differentiate velocity to get acceleration.");
      if (near_num(A, 0)) n = add(out, n, "v = %.6g t %+.6g", B, C);
      else n = add(out, n, "v = %.6g t^2 %+.6g t %+.6g", A, B, C);
      n = add(out, n, "a = dv/dt = %.6g t %+.6g", 2*A, B);
      return add(out, n, "at t=%.6g, a = %.10g", ta, 2*A*ta + B);
    }
  }
  if (has(t, "velocity") && (has(t, "displacement") || (has(t, "distance") && !has(t, "total"))) &&
      ((has(t, "from") && has(t, "to")) || has(t, "between") || has(c, "<t<") || has(c, "<=t")) &&
      !has(t, "vector") && !(has(c, "a=") || has(c, "hasacceleration") || has(c, "withacceleration"))) {
    double A=0, B=0, C=0, t1=0, t2=0;
    if (parse_poly_after_word(input, "velocity", &A, &B, &C) &&
        ((word_num_with_t(input, "from", &t1) || word_num(input, "from", &t1) || word_num_with_t(input, "between", &t1) || extract_t_interval(c, &t1, &t2) || (nv >= 2 && (t1 = v[nv-2], true)))) &&
        ((t2 != 0 || has(c, "<t<") || has(c, "<=t")) || word_num_with_t(input, "to", &t2) || word_num(input, "to", &t2) || word_num_with_t(input, "and", &t2) || (nv >= 2 && (t2 = v[nv-1], true)))) {
      double F1 = A*t1*t1*t1/3.0 + B*t1*t1/2.0 + C*t1;
      double F2 = A*t2*t2*t2/3.0 + B*t2*t2/2.0 + C*t2;
      int n = add(out, 0, "Displacement is the integral of velocity.");
      if (near_num(A, 0)) n = add(out, n, "v = %.6g t %+.6g", B, C);
      else n = add(out, n, "v = %.6g t^2 %+.6g t %+.6g", A, B, C);
      if (has(t, "acceleration")) {
        double ta = 0;
        if (!(word_num_with_t(input, "at", &ta) || word_num(input, "at", &ta) ||
              word_num_with_t(input, "when", &ta) || word_num(input, "when", &ta))) ta = t1;
        n = add(out, n, "a = dv/dt = %.6g t %+.6g", 2*A, B);
        n = add(out, n, "at t=%.6g, a = %.10g", ta, 2*A*ta + B);
      }
      n = add(out, n, "s = integral(v) dt = %.6g t^3 %+.6g t^2 %+.6g t", A/3.0, B/2.0, C);
      n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", t1, t2);
      double disp = F2 - F1;
      if (has(t, "distance")) n = add(out, n, "distance = |%.10g - %.10g|", F2, F1);
      else n = add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, disp);
      return add(out, n, has(t, "distance") ? "distance = %.10g" : "displacement = %.10g", disp < 0 && has(t, "distance") ? -disp : disp);
    }
  }
  if ((has(t, "workdone") || has(t, "work")) && !has(c, "/(") && (has(c, "f=(") || has(c, ")^")) && has(c, "x")) {
    double lo=0, hi=0;
    if ((word_num_with_t(input, "from", &lo) || word_num(input, "from", &lo) || (nv >= 2 && (lo = v[nv-2], true))) &&
        (word_num_with_t(input, "to", &hi) || word_num(input, "to", &hi) || (nv >= 2 && (hi = v[nv-1], true)))) {
      const char *p = strstr(c, "f=(");
      if (p) p += 3;
      else if ((p = strchr(c, '('))) ++p;
      double rb=0, rd=0; const char *e=0;
      if (p && parse_linear_inside(p, 'x', &rb, &rd, &e) && *e == ')') {
        int pow = e[1] == '^' ? (int)read_num(e + 2) : 1;
        if (pow >= 1) {
          double F1 = linear_power_antideriv(1.0, rb, rd, pow, lo);
          double F2 = linear_power_antideriv(1.0, rb, rd, pow, hi);
          char lin[48]; fmt_linear_var(lin, sizeof(lin), rb, 'x', rd);
          int n = add(out, 0, "Work done by a variable force is the integral of force.");
          n = add(out, n, "F(x) = (%s)^%d", lin, pow);
          n = add(out, n, "W = integral from %.6g to %.6g of (%s)^%d dx", lo, hi, lin, pow);
          n = add(out, n, "integral = (%s)^%d/(%.10g*%d)", lin, pow+1, rb, pow+1);
          return add(out, n, "W = %.10g J", F2 - F1);
        }
      }
    }
  }
  if ((has(t, "simpson") || has(t, "simpsons")) && (has(t, "values") || has(t, "ordinates")) && nv >= 2) {
    double ys[16]; int m = 0; double h = 0;
    if (parse_y_values(input, ys, &m) && (label_num(input, "h", &h) || word_num(input, "width", &h) || word_num(input, "step", &h))) {
      if (m % 2 == 0) return add(out, 0, "Simpson's rule needs an odd number of ordinates.");
      double odd = 0, even = 0;
      for (int i = 1; i < m - 1; ++i) {
        if (i % 2) odd += ys[i]; else even += ys[i];
      }
      double ans = h / 3.0 * (ys[0] + ys[m-1] + 4*odd + 2*even);
      int n = add(out, 0, "Use Simpson's rule with equally spaced ordinates.");
      n = add(out, n, "area = h/3[y0+yn+4(odd ordinates)+2(even ordinates)]");
      n = add(out, n, "odd sum = %.10g, even sum = %.10g", odd, even);
      return add(out, n, "area = %.10g/3*(%.10g+%.10g+4*%.10g+2*%.10g) = %.10g", h, ys[0], ys[m-1], odd, even, ans);
    }
  }
  if (has(t, "power") && (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "acceleration") || has(t, "accelerate")) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") &&
      (has(t, "speed") || has(t, "velocity") || has(t, "m/s") || has(t, "m,s") || has(t, "ms^-1") || has(t, "moves") || has(t, "moving") || has(t, "travels")) && nv >= 4) {
    double m=0, P=0, sp=0, R=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hP=word_num(input,"power",&P) || label_num(input,"power",&P) || prev_word_num(input,"kw",&P) || prev_word_num(input,"kilowatt",&P);
    bool hs=num_before_unit(input,"m/s",&sp) || num_before_unit(input,"ms^-1",&sp) || word_num(input,"speed",&sp) || word_num(input,"velocity",&sp);
    bool hR=word_num(input,"resistance",&R) || label_num(input,"resistance",&R);
    if (!hm) m = v[0];
    if (!hP) P = v[1];
    if (!hR) R = v[2];
    if (!hs) sp = v[3];
    if (has(t, "kw") || has(t, "kilowatt")) P *= 1000.0;
    double drive = sp ? P / sp : 0, net = drive - R;
    int n = add(out, 0, "Use P = Fv to convert engine power into driving force.");
    n = add(out, n, "driving force = P/v = %.10g/%.10g = %.10g N", P, sp, drive);
    n = add(out, n, "resultant force = %.10g - %.10g = %.10g N", drive, R, net);
    n = add(out, n, "Use F=ma with the resultant force.");
    return add(out, n, "a = F/m = %.10g/%.10g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "force") || has(c, "f=(") || has(c, "f=")) && has(t, "mass") &&
      (has(c, "t+") || has(c, "t-") || has(c, "*t+")) &&
      (has(t, "speed") || has(t, "velocity")) && nv >= 3) {
    double m=0, u0=0, tt=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hu=word_num(input,"initialvelocity",&u0) || word_num(input,"initialspeed",&u0) ||
            word_num(input,"speed",&u0) || word_num(input,"velocity",&u0);
    bool ht=word_num(input,"after",&tt) || word_num(input,"time",&tt);
    if (!hm) m = v[0];
    if (!hu && has(t, "rest")) u0 = 0;
    if (!ht) tt = v[nv-1];
    double A = 0, B = 0;
    const char *fp = strstr(c, "f=(");
    if (!fp) fp = strstr(c, "f=");
    if (!fp) fp = strstr(c, "force(");
    if (!fp) fp = strstr(c, "force");
    const char *tp = fp ? strchr(fp, 't') : 0;
    if (tp) {
      const char *a0 = tp - 1;
      while (a0 > c && (isdigit((unsigned char)a0[-1]) || a0[-1] == '.' || a0[-1] == '-')) --a0;
      A = read_num(a0);
      if (A == 0 && a0 == tp) A = 1;
      if (tp[1] == '+') B = read_num(tp + 2);
      else if (tp[1] == '-') B = -read_num(tp + 2);
    }
    if (A != 0 || B != 0) {
      double impulse = A*tt*tt/2.0 + B*tt;
      double vfinal = u0 + (m ? impulse/m : 0);
      int n = add(out, 0, "Impulse from a variable force is integral F(t) dt.");
      n = add(out, n, "F(t) = %.10g t %+.10g", A, B);
      n = add(out, n, "impulse = integral from 0 to %.6g of (%.10g t %+.10g) dt", tt, A, B);
      n = add(out, n, "impulse = %.10g Ns", impulse);
      n = add(out, n, "impulse = m(v-u)");
      return add(out, n, "v = %.10g + %.10g/%.10g = %.10g m/s", u0, impulse, m, vfinal);
    }
  }
  if ((has(t, "impulse") || has(t, "momentum")) && has(t, "force") && (has(t, "acts") || has(t, "for")) &&
      (has(t, "finalspeed") || has(t, "final") || has(t, "speed")) && nv >= 4) {
    double m=0,u0=0,F=0,time=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hu=word_num(input,"movingat",&u0) || word_num(input,"speed",&u0) || word_num(input,"velocity",&u0);
    bool hF=word_num(input,"force",&F) || label_num(input,"force",&F);
    bool ht=word_num(input,"for",&time) || word_num(input,"time",&time);
    if (!hm) m = v[0];
    if (!hu) u0 = v[1];
    if (!hF) F = v[2];
    if (!ht) time = v[3];
    double I = F * time, vf = u0 + I / m;
    int n = add(out, 0, "Impulse equals force multiplied by time.");
    n = add(out, n, "I = Ft = %.6g*%.6g = %.10g Ns", F, time, I);
    n = add(out, n, "I = m(v-u)");
    n = add(out, n, "%.10g = %.6g(v-%.6g)", I, m, u0);
    return add(out, n, "final speed v = %.10g m/s", vf);
  }
  if ((has(t, "workenergy") || (has(t, "work") && has(t, "energy")) || (has(t, "energy") && has(t, "driving") && has(t, "force"))) && nv >= 5) {
    double m=0,u0=0,v0=0,h=0,d=0,r=0;
    bool hm=label_num(input,"mass",&m) || label_num(input,"m",&m);
    bool hu=label_num(input,"u",&u0);
    bool hv=label_num(input,"v",&v0);
    bool hh=label_num(input,"height",&h) || label_num(input,"h",&h);
    bool hd=label_num(input,"distance",&d) || label_num(input,"d",&d);
    bool hr=label_num(input,"resistance",&r) || label_num(input,"r",&r);
    if (hm && hu && hv && hh && hd) sprintf(cmd, "workenergyforce(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", m, u0, v0, h, d, hr ? r : 0);
    else sprintf(cmd, nv > 5 ? "workenergyforce(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)" : "workenergyforce(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4], nv > 5 ? v[5] : 0);
    return eval_mech(cmd, out);
  }
  if ((has(t, "drivingforce") || (has(t, "driving") && has(t, "force"))) &&
      (has(t, "resistance") || has(t, "resistive")) && (has(t, "speed") || has(t, "velocity")) &&
      nv >= 5) {
    double m=0,u0=0,v0=0,h=0,d=0,r=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    bool hu=word_num(input,"from",&u0);
    bool hv=word_num(input,"to",&v0);
    bool hd=word_num(input,"over",&d) || word_num(input,"distance",&d);
    bool hh=word_num(input,"rises",&h) || word_num(input,"rise",&h) || word_num(input,"height",&h);
    if (!hh && (has(t, "horizontal") || has(t, "level") || !has(t, "rise"))) { h = 0; hh = true; }
    if (hm && hr && hu && hv && hd && hh) {
      sprintf(cmd, "workenergyforce(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", m, u0, v0, h, d, r);
      return eval_mech(cmd, out);
    }
  }
  if ((has(t, "fall") || has(t, "falls") || has(t, "falling")) &&
      (has(t, "resistance") || has(t, "resistive") || has(t, "againstair")) &&
      (has(t, "speed") || has(t, "velocity")) && nv >= 3) {
    double m=0, d=0, r=0, u0=0, g=9.8;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hd=word_num(input,"through",&d) || word_num(input,"distance",&d) || word_num(input,"over",&d);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    if (!hm) m = v[0];
    if (!hd) d = v[1];
    if (!hr) r = v[2];
    if (has(t, "fromrest")) u0 = 0;
    double work_g = m*g*d, work_r = r*d, ke0 = 0.5*m*u0*u0;
    double vv = (2*(ke0 + work_g - work_r))/m;
    double ans = vv > 0 ? root(vv) : 0;
    int n = add(out, 0, "Use work-energy, taking downward motion as positive.");
    n = add(out, n, "loss of GPE - work against resistance = gain in KE.");
    n = add(out, n, "mgh - Rd = 1/2*m*v^2 - 1/2*m*u^2");
    n = add(out, n, "%.6g*%.6g*%.6g - %.6g*%.6g = 1/2*%.6g*v^2 - %.10g", m, g, d, r, d, m, ke0);
    return add(out, n, "v = %.10g m/s", ans);
  }
  if ((has(t, "resistance") || has(t, "resistive")) &&
      !has(t, "power") &&
      (has(t, "finalspeed") || has(t, "finalvelocity") || (has(t, "find") && has(t, "speed"))) && nv >= 4) {
    double m=0,u0=0,r=0,d=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hu=word_num(input,"at",&u0) || word_num(input,"speed",&u0) || word_num(input,"velocity",&u0);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    bool hd=word_num(input,"for",&d) || word_num(input,"distance",&d);
    if (!hm) m = v[0];
    if (!hu) u0 = v[1];
    if (!hr) r = v[2];
    if (!hd) d = v[3];
    double start = 0.5*m*u0*u0, end = start - r*d, vv = end > 0 ? 2*end/m : 0;
    int n = add(out, 0, "Use work-energy with resistance doing negative work.");
    n = add(out, n, "initial KE - work against resistance = final KE");
    n = add(out, n, "1/2*%.6g*%.6g^2 - %.6g*%.6g = 1/2*%.6g*v^2", m, u0, r, d, m);
    return add(out, n, "v = %.10g m/s", root(vv));
  }
  if ((has(t, "impulse") || has(t, "impulsive")) && (has(t, "velocity") || has(t, "speed")) &&
      (has(t, "i") || has(t, "j"))) {
    double xs[4], ys[4]; int vc = scan_ij_vectors(input, xs, ys, 4);
    double m0 = 0;
    bool hm0 = word_num(input,"mass",&m0) || label_num(input,"mass",&m0) || num_before_unit(input, "kg", &m0);
    if (hm0 && vc >= 2 &&
        (has(t, "initial") || has(t, "final") || has(t, "before") || has(t, "from") || has(t, "to") ||
         has(t, "change") || has(t, "changes") || has(t, "changeinvelocity") ||
         has(t, "beforeimpact") || has(t, "afterimpact"))) {
      double ix = m0*(xs[1]-xs[0]), iy = m0*(ys[1]-ys[0]);
      double mag = root(ix*ix + iy*iy);
      int n = add(out, 0, "Impulse equals change in momentum, component by component.");
      n = add(out, n, "u = %.6g i + %.6g j, v = %.6g i + %.6g j", xs[0], ys[0], xs[1], ys[1]);
      n = add(out, n, "I = m(v-u)");
      n = add(out, n, "I = %.6g((%.6g-%.6g)i + (%.6g-%.6g)j)", m0, xs[1], xs[0], ys[1], ys[0]);
      n = add(out, n, "I = %.10g i + %.10g j Ns", ix, iy);
      return add(out, n, "magnitude = sqrt((%.10g)^2+(%.10g)^2) = %.10g Ns", ix, iy, mag);
    }
    double m=0, ux=0, uy=0, ix=0, iy=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    if (hm && vector_after_word(t, "velocity", &ux, &uy) && vector_after_word(t, "impulse", &ix, &iy)) {
      double vx = ux + ix/m, vy = uy + iy/m;
      int n = add(out, 0, "Impulse equals change in momentum, component by component.");
      n = add(out, n, "I = m(v-u), so v = u + I/m.");
      n = add(out, n, "u = %.6g i + %.6g j, I = %.6g i + %.6g j", ux, uy, ix, iy);
      n = add(out, n, "v = (%.6g + %.6g/%.6g)i + (%.6g + %.6g/%.6g)j", ux, ix, m, uy, iy, m);
      n = add(out, n, "v = %.10g i + %.10g j", vx, vy);
      return add(out, n, "speed = sqrt(%.10g^2 + %.10g^2) = %.10g m/s", vx, vy, root(vx*vx + vy*vy));
    }
  }
  if ((has(t, "impulse") || has(t, "impulsive")) && has(t, "opposite") &&
      (has(t, "velocity") || has(t, "speed")) && nv >= 3) {
    double m=0,u0=0,I=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hu=word_num(input,"velocity",&u0) || word_num(input,"speed",&u0);
    bool hI=word_num(input,"magnitude",&I) || word_num(input,"impulse",&I);
    if (!hm) m = v[0];
    if (!hu) u0 = v[1];
    if (!hI) I = v[2];
    double vv = u0 - I/m;
    int n = add(out, 0, "Impulse equals change in momentum.");
    n = add(out, n, "Take the original direction as positive.");
    n = add(out, n, "Impulse is opposite to the motion, so I = -%.6g Ns.", I);
    n = add(out, n, "I = m(v-u)");
    n = add(out, n, "-%.6g = %.6g(v-%.6g)", I, m, u0);
    return add(out, n, "v = %.10g m/s", vv);
  }
  if ((has(t, "constantacceleration") || (has(t, "constant") && has(t, "acceleration"))) &&
      (has(t, "positionvector") || (has(t, "position") && has(t, "vector"))) &&
      (has(t, "findu") || has(t, "velocityisu") || has(t, "velocity") || has(t, "find"))) {
    double xs[6], ys[6], time = 0;
    int vc = scan_ij_vectors(input, xs, ys, 6);
    last_label_num(input, "t", &time);
    if (vc >= 3 && time > 0) {
      double ax = xs[0], ay = ys[0], r0x = xs[1], r0y = ys[1], r1x = xs[2], r1y = ys[2];
      double ux = (r1x - r0x - 0.5*ax*time*time) / time;
      double uy = (r1y - r0y - 0.5*ay*time*time) / time;
      int n = add(out, 0, "Use vector constant-acceleration formulae component by component.");
      n = add(out, n, "r = r0 + ut + 1/2 at^2");
      n = add(out, n, "(%.6g,%.6g) = (%.6g,%.6g) + u*%.6g + 1/2(%.6g,%.6g)*%.6g^2", r1x, r1y, r0x, r0y, time, ax, ay, time);
      n = add(out, n, "u = (r-r0-1/2 at^2)/t");
      return add(out, n, "u = %.10g i %+.10g j", ux, uy);
    }
  }
  if ((has(t, "accelerationvector") || ((has(t, "acceleration") || has(t, "a=(")) && has(t, "vector"))) &&
      (has(t, "initially") || has(t, "initial")) && has(t, "position") && has(t, "velocity") &&
      (has(t, "findposition") || (has(t, "find") && has(t, "position"))) && nv >= 1) {
    double xs[6], ys[6], time = 0;
    int vc = scan_ij_vectors(input, xs, ys, 6);
    last_label_num(input, "t", &time);
    if (vc >= 3 && time > 0) {
      double ax = xs[0], ay = ys[0], ux = xs[1], uy = ys[1], r0x = xs[2], r0y = ys[2];
      double x = r0x + ux*time + 0.5*ax*time*time;
      double y = r0y + uy*time + 0.5*ay*time*time;
      int n = add(out, 0, "Use r = r0 + ut + 1/2 at^2 component by component.");
      n = add(out, n, "a = %.6g i %+.6g j, u = %.6g i %+.6g j, r0 = %.6g i %+.6g j", ax, ay, ux, uy, r0x, r0y);
      n = add(out, n, "r(%.6g) = r0 + u*%.6g + 1/2*a*%.6g^2", time, time, time);
      return add(out, n, "r = %.10g i %+.10g j", x, y);
    }
  }
  if ((has(t, "velocityvector") || (has(t, "velocity") && has(t, "vector"))) &&
      (has(t, "displacement") || has(t, "acceleration")) &&
      (has(c, "t^2i") || has(c, "t^2*i")) && nv >= 5) {
    double Ax = v[0], By = v[2], Cy = v[3], t0 = v[nv-2], t1 = v[nv-1];
    double ta = t1;
    word_num_with_t(input, "at", &ta);
    if (t1 < t0) { double q = t0; t0 = t1; t1 = q; }
    double ax = 2*Ax*ta, ay = By;
    double sx = Ax*(t1*t1*t1 - t0*t0*t0)/3.0;
    double sy = By*(t1*t1 - t0*t0)/2.0 + Cy*(t1 - t0);
    int n = add(out, 0, "For vector velocity, differentiate for acceleration and integrate for displacement.");
    n = add(out, n, "v = %.6g t^2 i + (%.6g t %+.6g)j", Ax, By, Cy);
    n = add(out, n, "a = dv/dt = %.6g t i + %.6g j", 2*Ax, By);
    n = add(out, n, "at t=%.6g, a = %.10g i %+.10g j", ta, ax, ay);
    n = add(out, n, "displacement = integral v dt from %.6g to %.6g", t0, t1);
    return add(out, n, "displacement = %.10g i %+.10g j", sx, sy);
  }
  if ((has(t, "velocityvector") || (has(t, "velocity") && has(t, "vector"))) &&
      !has(t, "positionvector") && !(has(t, "position") && has(t, "vector")) &&
      has(t, "acceleration") && has(c, "t") && has(t, "i") && has(t, "j")) {
    const char *base = strstr(c, "v=");
    if (!base) base = strstr(c, "velocityvector");
    if (!base) base = c;
    const char *ip = strchr(base, 'i'), *jp = ip ? strchr(ip, 'j') : 0;
    double x2 = 0, x1 = 0, y2 = 0, y1 = 0, ta = 0;
    if (ip && jp && ip < jp) {
      bool got = false;
      got = coeff_of_t_power_before(base, ip, 2, &x2) || got;
      got = coeff_of_t_power_before(base, ip, 1, &x1) || got;
      got = coeff_of_t_power_before(ip, jp, 2, &y2) || got;
      got = coeff_of_t_power_before(ip, jp, 1, &y1) || got;
      if (got) {
        if (!(word_num_with_t(input, "at", &ta) || last_label_num(input, "t", &ta) ||
              word_num(input, "time", &ta))) ta = nv > 0 ? v[nv-1] : 0;
        double ax = 2*x2*ta + x1, ay = 2*y2*ta + y1;
        int n = add(out, 0, "Differentiate the velocity vector component by component.");
        n = add(out, n, "v = (%.6g t^2 %+.6g t)i + (%.6g t^2 %+.6g t)j", x2, x1, y2, y1);
        n = add(out, n, "a = dv/dt = (%.6g t %+.6g)i + (%.6g t %+.6g)j", 2*x2, x1, 2*y2, y1);
        return add(out, n, "at t=%.6g, a = %.10g i %+.10g j", ta, ax, ay);
      }
    }
  }
  if ((has(t, "vector") || has(t, "positionvector") || has(t, "ij")) &&
      (has(t, "velocity") || has(t, "acceleration") || has(t, "motion"))) {
    double x0=0,y0=0,ux=0,uy=0,ax=0,ay=0,time=0;
    if (label_num(input,"x0",&x0) && label_num(input,"y0",&y0) &&
        label_num(input,"ux",&ux) && label_num(input,"uy",&uy) &&
        label_num(input,"ax",&ax) && label_num(input,"ay",&ay) &&
        label_num(input,"t",&time)) {
      sprintf(cmd, "vectorkin(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", x0, y0, ux, uy, ax, ay, time);
      return eval_mech(cmd, out);
    }
    if (nv >= 7) {
      sprintf(cmd, "vectorkin(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
      return eval_mech(cmd, out);
    }
  }
  if ((has(t, "acceleration") || has(t, "accn") || has(c, "a=")) &&
      (has(t, "velocity") || has(c, "v=") || has(c, "u=")) &&
      (has(t, "displacement") || has(t, "distance") || has(t, "position") || has(c, "s=")) &&
      (has(c, "finddisplacement") || has(c, "finds") || has(c, "findposition") || has(t, "at")) &&
      !(has(c, "displacementfrom") && has(t, "to"))) {
    double A=0, B=0, C=0, u=0, s0=0, time=0, t0=0;
    bool parsed_acc = parse_poly_after_word(input, "acceleration", &A, &B, &C);
    if (!parsed_acc && has(c, "a=")) parsed_acc = parse_velocity_quad(input, &A, &B, &C);
    if (parsed_acc && near_num(A, 0) && near_num(B, 0) && !has(c, "a=")) parsed_acc = false;
    bool hu = word_num(input, "velocity", &u) || label_num(input, "v", &u) || label_num(input, "u", &u);
    word_num(input, "displacement", &s0) || word_num(input, "position", &s0) ||
    label_num(input, "s", &s0) || label_num(input, "s0", &s0);
    double tv[6]; int ntv = scan_t_values(input, tv, 6);
    bool ht = last_label_num(input, "t", &time) || word_num(input, "after", &time) ||
              word_num(input, "time", &time) || word_num_with_t(input, "at", &time);
    bool ht0 = word_num_with_t(input, "when", &t0) || word_num_with_t(input, "initially", &t0);
    if (ntv >= 2 && !ht0) { t0 = tv[0]; ht0 = true; }
    if (ntv >= 2) { time = tv[ntv-1]; ht = true; }
    if (!ht0) t0 = 0;
    if (parsed_acc && hu && ht) {
      double vbase0 = A*t0*t0*t0/3.0 + B*t0*t0/2.0 + C*t0;
      double k1 = u - vbase0;
      double vv = A*time*time*time/3.0 + B*time*time/2.0 + C*time + k1;
      double sbase0 = A*t0*t0*t0*t0/12.0 + B*t0*t0*t0/6.0 + C*t0*t0/2.0 + k1*t0;
      double k2 = s0 - sbase0;
      double ss = A*time*time*time*time/12.0 + B*time*time*time/6.0 + C*time*time/2.0 + k1*time + k2;
      int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t), then integrate v(t) to get s(t).");
      if (near_num(A, 0)) {
        n = add(out, n, "a = %.6g t %+.6g", B, C);
        n = add(out, n, "v = (%.6g/2)t^2 + %.6g t + C", B, C);
      } else {
        n = add(out, n, "a = %.6g t^2 %+.6g t %+.6g", A, B, C);
        n = add(out, n, "v = (%.6g/3)t^3 + (%.6g/2)t^2 + %.6g t + C", A, B, C);
      }
      n = add(out, n, "v(%.6g)=%.6g gives C = %.10g", t0, u, k1);
      n = add(out, n, "s(%.6g)=%.6g gives C = %.10g", t0, s0, k2);
      n = add(out, n, "at t=%.6g, v = %.10g", time, vv);
      return add(out, n, "at t=%.6g, s = %.10g", time, ss);
    }
  }
  if (has(t, "variable") && has(t, "acceleration")) {
    double u=0,c0=0,c1=0,c2=0,time=0,x=0,x0=0,s0=0;
    bool hu=label_num(input,"u",&u), h0=label_num(input,"c0",&c0), h1=label_num(input,"c1",&c1), h2=label_num(input,"c2",&c2);
    if (hu && h0 && h1 && h2 && label_num(input,"t",&time)) {
      bool hs0 = label_num(input,"s0",&s0);
      sprintf(cmd, "varacct(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", u, c0, c1, c2, time, hs0 ? s0 : 0);
      return eval_mech(cmd, out);
    }
    if (hu && h0 && h1 && h2 && label_num(input,"x",&x)) {
      bool hx0 = label_num(input,"x0",&x0);
      sprintf(cmd, "varaccx(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", u, c0, c1, c2, x, hx0 ? x0 : 0);
      return eval_mech(cmd, out);
    }
  }
  if (has(t, "velocity") && (has(t, "total") || has(t, "travelled") || has(t, "traveled")) &&
      has(t, "distance") && (has(t, "from") || has(t, "between") || has(t, "first") || has(c, "<t<") || has(c, "<=t"))) {
    double CA=0, CB=0, CC=0, CD=0, ct1=0, ct2=0;
    if (parse_cubic_poly_after_word(input, "velocity", &CA, &CB, &CC, &CD) &&
        !near_num(CA, 0) &&
        (((word_num_with_t(input, "from", &ct1) || word_num_with_t(input, "between", &ct1)) &&
          (word_num_with_t(input, "to", &ct2) || word_num_with_t(input, "and", &ct2))) ||
         extract_t_interval(c, &ct1, &ct2))) {
      double pts[8]; int np = 0; pts[np++] = ct1;
      for (int step = 0; step < 400 && np < 7; ++step) {
        double a0 = ct1 + (ct2 - ct1) * step / 400.0;
        double b0 = ct1 + (ct2 - ct1) * (step + 1) / 400.0;
        double fa = ((CA*a0 + CB)*a0 + CC)*a0 + CD;
        double fb = ((CA*b0 + CB)*b0 + CC)*b0 + CD;
        if (near_num(fa, 0) && a0 > ct1 && a0 < ct2) pts[np++] = a0;
        if (fa * fb < 0) {
          double lo = a0, hi = b0;
          for (int it = 0; it < 40; ++it) {
            double mid = 0.5*(lo+hi);
            double fm = ((CA*mid + CB)*mid + CC)*mid + CD;
            if (fa * fm <= 0) { hi = mid; fb = fm; }
            else { lo = mid; fa = fm; }
          }
          double r = 0.5*(lo+hi);
          bool dup = false; for (int q = 0; q < np; ++q) if (near_num(pts[q], r)) dup = true;
          if (!dup && r > ct1 && r < ct2) pts[np++] = r;
        }
      }
      pts[np++] = ct2;
      for (int i = 0; i < np; ++i) for (int j = i + 1; j < np; ++j) if (pts[j] < pts[i]) { double q = pts[i]; pts[i] = pts[j]; pts[j] = q; }
      int n = add(out, 0, "Total distance is found by splitting where velocity changes sign.");
      n = add(out, n, "v = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", CA, CB, CC, CD);
      if (np > 2) n = add(out, n, "Solve v=0 inside the interval to split the motion.");
      double total = 0;
      for (int i = 0; i + 1 < np; ++i) {
        double a0 = pts[i], b0 = pts[i+1];
        double F1 = cubic_int_val(CA, CB, CC, CD, a0);
        double F2 = cubic_int_val(CA, CB, CC, CD, b0);
        double d = abs_num(F2 - F1); total += d;
        n = add(out, n, "distance on %.6g to %.6g = |%.10g - %.10g| = %.10g", a0, b0, F2, F1, d);
      }
      return add(out, n, "total distance = %.10g", total);
    }
    double A=0, B=0, C=0, t1=0, t2=0;
    if (!parse_velocity_quad(input, &A, &B, &C)) return 0;
    if (!word_num_with_t(input, "from", &t1) || !word_num_with_t(input, "to", &t2)) {
      if (word_num_with_t(input, "between", &t1) && word_num_with_t(input, "and", &t2)) {}
      else if (has(t, "first") && prev_word_num(input, "seconds", &t2)) t1 = 0;
      else if (extract_t_interval(c, &t1, &t2)) {}
      else return 0;
    }
    double pts[4]; int np = 0; pts[np++] = t1;
    double D = B*B - 4*A*C;
    if (near_num(A, 0) && !near_num(B, 0)) {
      double r = -C / B;
      if (r > t1 && r < t2) pts[np++] = r;
    } else if (D >= 0 && !near_num(A, 0)) {
      double r1 = (-B - root(D))/(2*A), r2 = (-B + root(D))/(2*A);
      if (r1 > t1 && r1 < t2) pts[np++] = r1;
      if (r2 > t1 && r2 < t2 && !near_num(r2, r1)) pts[np++] = r2;
    }
    pts[np++] = t2;
    for (int i = 0; i < np; ++i) for (int j = i + 1; j < np; ++j) if (pts[j] < pts[i]) { double q = pts[i]; pts[i] = pts[j]; pts[j] = q; }
    int n = add(out, 0, "Total distance is found by splitting where velocity changes sign.");
    n = add(out, n, "v = %.6g t^2 %+.6g t %+.6g", A, B, C);
    if (np > 2) n = add(out, n, "Solve v=0 inside the interval to split the motion.");
    double total = 0;
    for (int i = 0; i + 1 < np; ++i) {
      double a0 = pts[i], b0 = pts[i+1];
      double F1=A*a0*a0*a0/3.0 + B*a0*a0/2.0 + C*a0;
      double F2=A*b0*b0*b0/3.0 + B*b0*b0/2.0 + C*b0;
      double d = F2 - F1; if (d < 0) d = -d;
      total += d;
      n = add(out, n, "distance on %.6g to %.6g = |%.10g - %.10g| = %.10g", a0, b0, F2, F1, d);
    }
    return add(out, n, "total distance = %.10g", total);
  }
  if (has(t, "displacement") && has(t, "total") && has(t, "distance")) {
    double A4=0, A3=0, A2=0, A1=0, A0=0, qt1=0, qt2=0;
    if (parse_quartic_poly_after_word(input, "displacement", &A4, &A3, &A2, &A1, &A0) && !near_num(A4, 0) &&
        (word_num_with_t(input, "from", &qt1) || word_num_with_t(input, "between", &qt1)) &&
        (word_num_with_t(input, "to", &qt2) || word_num_with_t(input, "and", &qt2))) {
      double pts[10]; int np = 0; pts[np++] = qt1;
      double prevt = qt1, prev = ((4*A4*prevt + 3*A3)*prevt + 2*A2)*prevt + A1;
      for (int i = 1; i <= 800 && np < 9; ++i) {
        double cur = qt1 + (qt2 - qt1) * i / 800.0;
        double f = ((4*A4*cur + 3*A3)*cur + 2*A2)*cur + A1;
        if ((near_num(prev, 0) && prevt > qt1 && prevt < qt2) || prev * f < 0) {
          double lo = prevt, hi = cur, flo = prev;
          for (int k = 0; k < 40; ++k) {
            double mid = (lo + hi) / 2.0;
            double fm = ((4*A4*mid + 3*A3)*mid + 2*A2)*mid + A1;
            if (flo * fm <= 0) hi = mid; else { lo = mid; flo = fm; }
          }
          double r = (lo + hi) / 2.0;
          bool dup = false; for (int q = 0; q < np; ++q) if (abs_num(pts[q] - r) < 1e-6) dup = true;
          if (!dup && r > qt1 && r < qt2) pts[np++] = r;
        }
        prevt = cur; prev = f;
      }
      pts[np++] = qt2;
      for (int i = 0; i < np; ++i) for (int j = i + 1; j < np; ++j) if (pts[j] < pts[i]) { double q = pts[i]; pts[i] = pts[j]; pts[j] = q; }
      int n = add(out, 0, "Total distance is found by splitting where velocity changes sign.");
      n = add(out, n, "s = %.6g t^4 %+.6g t^3 %+.6g t^2 %+.6g t %+.6g", A4, A3, A2, A1, A0);
      n = add(out, n, "v = ds/dt = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", 4*A4, 3*A3, 2*A2, A1);
      double total = 0;
      for (int i = 0; i + 1 < np; ++i) {
        double a0 = pts[i], b0 = pts[i+1];
        double s1 = (((A4*a0 + A3)*a0 + A2)*a0 + A1)*a0 + A0;
        double s2 = (((A4*b0 + A3)*b0 + A2)*b0 + A1)*b0 + A0;
        double d = abs_num(s2 - s1); total += d;
        n = add(out, n, "distance on %.6g to %.6g = |%.10g - %.10g| = %.10g", a0, b0, s2, s1, d);
      }
      return add(out, n, "total distance = %.10g", total);
    }
    double A=0, B=0, C=0, D0=0, t1=0, t2=0;
    if (!parse_cubic_poly_after_word(input, "displacement", &A, &B, &C, &D0)) return 0;
    bool ht1 = word_num_with_t(input, "from", &t1) || word_num_with_t(input, "between", &t1);
    bool ht2 = word_num_with_t(input, "to", &t2) || word_num_with_t(input, "and", &t2);
    if (!ht1 || !ht2) return 0;
    double qa = 3*A, qb = 2*B, qc = C;
    double pts[4]; int np = 0; pts[np++] = t1;
    double disc = qb*qb - 4*qa*qc;
    if (disc >= 0 && !near_num(qa, 0)) {
      double r1 = (-qb - root(disc))/(2*qa), r2 = (-qb + root(disc))/(2*qa);
      if (r1 > t1 && r1 < t2) pts[np++] = r1;
      if (r2 > t1 && r2 < t2 && !near_num(r2, r1)) pts[np++] = r2;
    } else if (!near_num(qb, 0)) {
      double r = -qc/qb; if (r > t1 && r < t2) pts[np++] = r;
    }
    pts[np++] = t2;
    for (int i = 0; i < np; ++i) for (int j = i + 1; j < np; ++j) if (pts[j] < pts[i]) { double q = pts[i]; pts[i] = pts[j]; pts[j] = q; }
    int n = add(out, 0, "Total distance is found by splitting where velocity changes sign.");
    n = add(out, n, "s = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", A, B, C, D0);
    n = add(out, n, "v = ds/dt = %.6g t^2 %+.6g t %+.6g", qa, qb, qc);
    if (np > 2) n = add(out, n, "Use the stationary times inside the interval as split points.");
    double total = 0;
    for (int i = 0; i + 1 < np; ++i) {
      double a0 = pts[i], b0 = pts[i+1];
      double s1 = cubic_val(A, B, C, D0, a0), s2 = cubic_val(A, B, C, D0, b0);
      double d = abs_num(s2 - s1); total += d;
      n = add(out, n, "distance on %.6g to %.6g = |%.10g - %.10g| = %.10g", a0, b0, s2, s1, d);
    }
    return add(out, n, "total distance = %.10g", total);
  }
  if (has(t, "velocity") && has(t, "maximum") && has(t, "displacement")) {
    double A=0, B=0, C=0;
    if (!parse_velocity_quad(input, &A, &B, &C)) return 0;
    double bestt = 0, bests = 0;
    double disc = B*B - 4*A*C;
    if (disc >= 0 && !near_num(A, 0)) {
      double r1 = (-B - root(disc))/(2*A), r2 = (-B + root(disc))/(2*A);
      if (r1 > 0) { double s1 = quad_int_val(A, B, C, r1); if (s1 > bests) { bests = s1; bestt = r1; } }
      if (r2 > 0) { double s2 = quad_int_val(A, B, C, r2); if (s2 > bests) { bests = s2; bestt = r2; } }
    } else if (!near_num(B, 0)) {
      double r = -C/B; if (r > 0) { bestt = r; bests = quad_int_val(A, B, C, r); }
    }
    int n = add(out, 0, "Maximum displacement occurs when velocity changes from positive to negative.");
    n = add(out, n, "v = %.6g t^2 %+.6g t %+.6g", A, B, C);
    n = add(out, n, "Solve v=0 to find the stationary time.");
    n = add(out, n, "s = integral(v) dt = %.6g t^3 %+.6g t^2 %+.6g t", A/3.0, B/2.0, C);
    return add(out, n, "at t=%.10g, maximum displacement = %.10g", bestt, bests);
  }
  if (has(t, "displacement") && has(t, "stationary")) {
    double A4=0, A3=0, A2=0, A1=0, A0=0, t1=0, t2=0;
    if (parse_quartic_poly_after_word(input, "displacement", &A4, &A3, &A2, &A1, &A0) && !near_num(A4, 0)) {
      bool ht1 = word_num_with_t(input, "from", &t1) || word_num_with_t(input, "between", &t1);
      bool ht2 = word_num_with_t(input, "to", &t2) || word_num_with_t(input, "and", &t2);
      if (!ht1) t1 = 0;
      if (!ht2) t2 = 10;
      if (t2 < t1) { double q = t1; t1 = t2; t2 = q; }
      double roots[8]; int nr = 0;
      double prevt = t1, prev = ((4*A4*prevt + 3*A3)*prevt + 2*A2)*prevt + A1;
      if (near_num(prev, 0)) roots[nr++] = prevt;
      for (int i = 1; i <= 800 && nr < 8; ++i) {
        double cur = t1 + (t2 - t1) * i / 800.0;
        double f = ((4*A4*cur + 3*A3)*cur + 2*A2)*cur + A1;
        if (near_num(f, 0) || prev * f < 0) {
          double lo = prevt, hi = cur, flo = prev;
          for (int k = 0; k < 45; ++k) {
            double mid = (lo + hi) / 2.0;
            double fm = ((4*A4*mid + 3*A3)*mid + 2*A2)*mid + A1;
            if (near_num(fm, 0)) { lo = hi = mid; break; }
            if (flo * fm <= 0) { hi = mid; }
            else { lo = mid; flo = fm; }
          }
          double r = (lo + hi) / 2.0;
          bool dup = false; for (int q = 0; q < nr; ++q) if (abs_num(roots[q] - r) < 1e-6) dup = true;
          if (!dup) roots[nr++] = r;
        }
        prevt = cur; prev = f;
      }
      int n = add(out, 0, "Stationary points occur when velocity ds/dt is zero.");
      n = add(out, n, "s = %.6g t^4 %+.6g t^3 %+.6g t^2 %+.6g t %+.6g", A4, A3, A2, A1, A0);
      n = add(out, n, "v = ds/dt = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", 4*A4, 3*A3, 2*A2, A1);
      n = add(out, n, "Solve v=0 on %.6g <= t <= %.6g.", t1, t2);
      if (!nr) return add(out, n, "No stationary time found in the interval.");
      for (int i = 0; i < nr; ++i) n = add(out, n, "t = %.10g", roots[i]);
      return n;
    }
    double A=0, B=0, C=0, D0=0;
    if (parse_cubic_poly_after_word(input, "displacement", &A, &B, &C, &D0)) {
      double qa = 3*A, qb = 2*B, qc = C, disc = qb*qb - 4*qa*qc;
      int n = add(out, 0, "Stationary points occur when velocity ds/dt is zero.");
      n = add(out, n, "s = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", A, B, C, D0);
      n = add(out, n, "v = ds/dt = %.6g t^2 %+.6g t %+.6g", qa, qb, qc);
      if (disc < 0 || near_num(qa, 0)) return add(out, n, "No real stationary time from this quadratic velocity.");
      double r1 = (-qb - root(disc))/(2*qa), r2 = (-qb + root(disc))/(2*qa);
      n = add(out, n, "v=0 gives t = %.10g or %.10g", r1, r2);
      n = add(out, n, "a = dv/dt = %.6g t %+.6g", 2*qa, qb);
      n = add(out, n, "at t=%.10g, a = %.10g", r1, 2*qa*r1 + qb);
      return add(out, n, "at t=%.10g, a = %.10g", r2, 2*qa*r2 + qb);
    }
  }
  if (has(t, "displacement") && has(t, "acceleration") &&
      (has(t, "att") || has(t, "at")) && has(c, "t") && !has(t, "from")) {
    double A=0, B=0, C=0, time=0;
    if (!(word_num_with_t(input, "at", &time) || first_num_after_word(input, "at", &time))) time = nv > 0 ? v[nv-1] : 0;
    double CA=0, CB=0, CC=0, CD=0;
    if (parse_cubic_poly_after_word(input, "displacement", &CA, &CB, &CC, &CD) &&
        (!near_num(CA, 0) || !near_num(CB, 0) || !near_num(CC, 0))) {
      double vv = (3*CA*time + 2*CB)*time + CC;
      double aa = 6*CA*time + 2*CB;
      int n = add(out, 0, "Velocity is ds/dt and acceleration is dv/dt.");
      n = add(out, n, "s = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", CA, CB, CC, CD);
      n = add(out, n, "v = ds/dt = %.6g t^2 %+.6g t %+.6g", 3*CA, 2*CB, CC);
      n = add(out, n, "a = dv/dt = %.6g t %+.6g", 6*CA, 2*CB);
      if (has(t, "velocity")) n = add(out, n, "v(%.6g) = %.10g", time, vv);
      return add(out, n, "a(%.6g) = %.10g", time, aa);
    }
    if (parse_poly_after_word(input, "displacement", &A, &B, &C) &&
        (!near_num(A, 0) || !near_num(B, 0))) {
      double vv = 2*A*time + B;
      double aa = 2*A;
      int n = add(out, 0, "Velocity is ds/dt and acceleration is dv/dt.");
      n = add(out, n, "s = %.6g t^2 %+.6g t %+.6g", A, B, C);
      n = add(out, n, "v = ds/dt = %.6g t %+.6g", 2*A, B);
      n = add(out, n, "a = dv/dt = %.6g", aa);
      n = add(out, n, "v(%.6g) = %.10g", time, vv);
      return add(out, n, "a(%.6g) = %.10g", time, aa);
    }
  }
  if (has(t, "velocity") && has(t, "displacement") && has(t, "from") &&
      !(has(c, "a=") || has(c, "hasacceleration") || has(c, "withacceleration"))) {
    double A4=0, A3=0, A2=0, A1=0, A0=0, qt1=0, qt2=0;
    if (parse_quartic_poly_after_word(input, "velocity", &A4, &A3, &A2, &A1, &A0) &&
        !near_num(A4, 0) && word_num_with_t(input, "from", &qt1) && word_num_with_t(input, "to", &qt2)) {
      double F1=A4*qt1*qt1*qt1*qt1*qt1/5.0 + A3*qt1*qt1*qt1*qt1/4.0 + A2*qt1*qt1*qt1/3.0 + A1*qt1*qt1/2.0 + A0*qt1;
      double F2=A4*qt2*qt2*qt2*qt2*qt2/5.0 + A3*qt2*qt2*qt2*qt2/4.0 + A2*qt2*qt2*qt2/3.0 + A1*qt2*qt2/2.0 + A0*qt2;
      int n = add(out, 0, "Displacement is the integral of velocity.");
      n = add(out, n, "v = %.6g t^4 %+.6g t^3 %+.6g t^2 %+.6g t %+.6g", A4, A3, A2, A1, A0);
      n = add(out, n, "s = integral(v) dt = %.6g t^5 %+.6g t^4 %+.6g t^3 %+.6g t^2 %+.6g t", A4/5.0, A3/4.0, A2/3.0, A1/2.0, A0);
      n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", qt1, qt2);
      return add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, F2-F1);
    }
    double A=0, B=0, C=0, D0=0, t1=0, t2=0;
    if (parse_cubic_poly_after_word(input, "velocity", &A, &B, &C, &D0) &&
        (!near_num(A, 0)) &&
        word_num_with_t(input, "from", &t1) && word_num_with_t(input, "to", &t2)) {
      double F1=A*t1*t1*t1*t1/4.0 + B*t1*t1*t1/3.0 + C*t1*t1/2.0 + D0*t1;
      double F2=A*t2*t2*t2*t2/4.0 + B*t2*t2*t2/3.0 + C*t2*t2/2.0 + D0*t2;
      int n = add(out, 0, "Displacement is the integral of velocity.");
      n = add(out, n, "v = %.6g t^3 %+.6g t^2 %+.6g t %+.6g", A, B, C, D0);
      if (has(t, "acceleration")) {
        double ta = 0; const char *tp = strstr(c, "t=");
        if (tp) ta = read_num(tp + 2);
        else first_num_after_word(input, "when", &ta);
        double acc = 3*A*ta*ta + 2*B*ta + C;
        n = add(out, n, "a = dv/dt = %.6g t^2 %+.6g t %+.6g", 3*A, 2*B, C);
        n = add(out, n, "a(%.6g) = %.10g", ta, acc);
      }
      n = add(out, n, "s = integral(v) dt = %.6g t^4 %+.6g t^3 %+.6g t^2 %+.6g t", A/4.0, B/3.0, C/2.0, D0);
      n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", t1, t2);
      return add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, F2-F1);
    }
  }
  if (has(t, "velocity") && has(t, "displacement") && has(t, "from") &&
      !(has(c, "a=") || has(c, "hasacceleration") || has(c, "withacceleration"))) {
    double rb=0, rd=0; int rpow=0;
    if (has(c, "/(") && parse_linear_den_power(c, 't', &rb, &rd, &rpow) && rpow >= 1) {
      double k = nv > 0 ? v[0] : 1;
      double c0 = trailing_constant_after_linear_den(c, 't', rpow);
      double t1 = 0, t2 = 0;
      word_num_with_t(input, "from", &t1); word_num_with_t(input, "to", &t2);
      if (t2 < t1) { double q = t1; t1 = t2; t2 = q; }
      double F1 = linear_den_antideriv(k, rb, rd, rpow, t1) + c0*t1;
      double F2 = linear_den_antideriv(k, rb, rd, rpow, t2) + c0*t2;
      char den[48]; fmt_linear_var(den, sizeof(den), rb, 't', rd);
      int n = add(out, 0, "Displacement is the integral of velocity.");
      if (near_num(c0, 0)) {
        n = add(out, n, "v = %.10g/(%s)^%d", k, den, rpow);
        if (rpow == 1) n = add(out, n, "integral v dt = (%.10g/%.10g)ln|%s|", k, rb, den);
        else n = add(out, n, "integral v dt = %.10g(%s)^%d/(%.10g*(%d))", k, den, 1-rpow, rb, 1-rpow);
      } else {
        n = add(out, n, "v = %.10g/(%s)^%d%+.10g", k, den, rpow, c0);
        if (rpow == 1) n = add(out, n, "integral v dt = (%.10g/%.10g)ln|%s|%+.10g t", k, rb, den, c0);
        else n = add(out, n, "integral v dt = %.10g(%s)^%d/(%.10g*(%d))%+.10g t", k, den, 1-rpow, rb, 1-rpow, c0);
      }
      n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", t1, t2);
      return add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, F2-F1);
    }
    if (has(c, "/") && has(c, "t+") && (has(c, ")^2") || has(c, "^2"))) {
      double k = nv > 0 ? v[0] : 1, b = 1, d = 1;
      const char *tp = strstr(c, "t+");
      if (tp) {
        d = read_num(tp + 2);
        const char *q = tp - 1;
        while (q > c && isdigit((unsigned char)q[-1])) --q;
        if (q < tp && isdigit((unsigned char)*q)) b = read_num(q);
      }
      double t1 = nv > 3 ? v[nv-2] : 0, t2 = nv > 4 ? v[nv-1] : 0;
      if (t2 < t1) { double q = t1; t1 = t2; t2 = q; }
      double F1 = -k / (b * (b*t1 + d));
      double F2 = -k / (b * (b*t2 + d));
      int n = add(out, 0, "Displacement is the integral of velocity.");
      n = add(out, n, "v = %.10g/(%.10g t%+.10g)^2", k, b, d);
      n = add(out, n, "integral v dt = -%.10g/(%.10g(%.10g t%+.10g))", k, b, b, d);
      n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", t1, t2);
      return add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, F2-F1);
    }
    double A=0, B=0, C=0, t1=0, t2=0;
    if (!parse_velocity_quad(input, &A, &B, &C) ||
        !word_num_with_t(input, "from", &t1) || !word_num_with_t(input, "to", &t2)) return 0;
    double F1=A*t1*t1*t1/3.0 + B*t1*t1/2.0 + C*t1;
    double F2=A*t2*t2*t2/3.0 + B*t2*t2/2.0 + C*t2;
    int n = add(out, 0, "Displacement is the integral of velocity.");
    n = add(out, n, "v = %.6g t^2 %+.6g t %+.6g", A, B, C);
    if (has(t, "acceleration")) {
      double ta = 0;
      if (word_num_with_t(input, "at", &ta) || first_num_after_word(input, "acceleration", &ta)) {
        n = add(out, n, "a = dv/dt = %.6g t %+.6g", 2*A, B);
        n = add(out, n, "at t=%.6g, a = %.10g", ta, 2*A*ta + B);
      }
    }
    n = add(out, n, "s = integral(v) dt = %.6g t^3 %+.6g t^2 %+.6g t", A/3.0, B/2.0, C);
    n = add(out, n, "displacement = [s] from t=%.6g to t=%.6g", t1, t2);
    return add(out, n, "displacement = %.10g - %.10g = %.10g", F2, F1, F2-F1);
  }
  if ((has(t, "acceleration") || has(t, "accn")) && has(t, "t") &&
      has(t, "velocity") && (has(t, "att") || has(t, "at")) &&
      (has(c, "findvatt") || has(c, "findvelocityatt") || has(c, "findv") || has(c, "findvelocity")) &&
      !has(t, "positionvector") && !(has(t, "position") && has(t, "vector")) &&
      nv >= 5) {
    double A=0, B=0, C=0;
    bool parsed_acc = parse_poly_after_word(input, "acceleration", &A, &B, &C);
    if (!parsed_acc && has(c, "a=")) parsed_acc = parse_velocity_quad(input, &A, &B, &C);
    if (parsed_acc) {
      double v0 = v[nv-3], t0 = v[nv-2], tfind = v[nv-1];
      if ((has(t, "starts") || has(t, "initially")) && has(t, "rest")) {
        v0 = 0; t0 = 0;
        word_num(input, "after", &tfind) || word_num_with_t(input, "at", &tfind) || word_num(input, "time", &tfind);
      }
      double ant0 = A*t0*t0*t0/3.0 + B*t0*t0/2.0 + C*t0;
      double K = v0 - ant0;
      double ans = A*tfind*tfind*tfind/3.0 + B*tfind*tfind/2.0 + C*tfind + K;
      int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t).");
      if (near_num(A, 0)) n = add(out, n, "a = %.6g t %+.6g", B, C);
      else n = add(out, n, "a = %.6g t^2 %+.6g t %+.6g", A, B, C);
      n = add(out, n, "v = integral(a) dt + C");
      n = add(out, n, "v(%.6g)=%.6g gives C = %.10g", t0, v0, K);
      return add(out, n, "v(%.6g) = %.10g", tfind, ans);
    }
  }
  if ((has(t, "acceleration") || has(t, "accn")) && has(t, "t") &&
      (has(t, "initial") || has(t, "initially") || has(t, "starts") || has(t, "rest")) && nv >= 3) {
    double A=0, B=0, C=0, u=0, s0=0, time=0;
    bool parsed_acc = parse_poly_after_word(input, "acceleration", &A, &B, &C);
    if (!parsed_acc && has(c, "a=")) parsed_acc = parse_velocity_quad(input, &A, &B, &C);
    bool has_u = label_num(input, "v", &u) || label_num(input, "u", &u) ||
                 word_num(input, "velocity", &u) || word_num(input, "speed", &u) || word_num(input, "u", &u);
    label_num(input, "s", &s0);
    if (!has_u && has(t, "rest")) { u = 0; has_u = true; }
    if (parsed_acc && (!near_num(A, 0) || !near_num(B, 0)) &&
        has_u &&
        (label_num(input, "t", &time) || word_num(input, "after", &time) ||
         word_num(input, "time", &time) || word_num(input, "at", &time))) {
      double vv = A*time*time*time/3.0 + B*time*time/2.0 + C*time + u;
      double ss = A*time*time*time*time/12.0 + B*time*time*time/6.0 + C*time*time/2.0 + u*time + s0;
      int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t), then integrate v(t) to get s(t).");
      if (near_num(A, 0)) {
        n = add(out, n, "a = %.6g t %+.6g", B, C);
        n = add(out, n, "v = integral(a) dt = (%.6g/2)t^2 + %.6g t + C", B, C);
      } else {
        n = add(out, n, "a = %.6g t^2 %+.6g t %+.6g", A, B, C);
        n = add(out, n, "v = integral(a) dt = (%.6g/3)t^3 + (%.6g/2)t^2 + %.6g t + C", A, B, C);
      }
      n = add(out, n, "initial velocity gives C = %.6g", u);
      n = add(out, n, "at t=%.6g, v = %.10g", time, vv);
      if (near_num(A, 0)) n = add(out, n, "s = integral(v) dt = (%.6g/6)t^3 + (%.6g/2)t^2 + %.6g t + C", B, C, u);
      else n = add(out, n, "s = integral(v) dt = (%.6g/12)t^4 + (%.6g/6)t^3 + (%.6g/2)t^2 + %.6g t + C", A, B, C, u);
      n = add(out, n, "initial position gives C = %.6g", s0);
      return add(out, n, "at t=%.6g, s = %.10g", time, ss);
    }
    if (parsed_acc && has_u && (has(t, "comestorest") || has(t, "comes") || has(t, "rest"))) {
      double aa = B/2.0, bb = C, cc = u;
      double rt = 0, disc = bb*bb - 4*aa*cc;
      if (!near_num(aa, 0) && disc >= 0) {
        double r1 = (-bb - root(disc))/(2*aa), r2 = (-bb + root(disc))/(2*aa);
        if (r1 > 0 && (rt == 0 || r1 < rt)) rt = r1;
        if (r2 > 0 && (rt == 0 || r2 < rt)) rt = r2;
      } else if (!near_num(bb, 0)) rt = -cc/bb;
      double ss = A*rt*rt*rt*rt/12.0 + B*rt*rt*rt/6.0 + C*rt*rt/2.0 + u*rt;
      int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t).");
      if (near_num(A, 0)) {
        n = add(out, n, "a = %.6g t %+.6g", B, C);
        n = add(out, n, "v = (%.6g/2)t^2 + %.6g t + %.6g", B, C, u);
      } else {
        n = add(out, n, "a = %.6g t^2 %+.6g t %+.6g", A, B, C);
        n = add(out, n, "v = (%.6g/3)t^3 + (%.6g/2)t^2 + %.6g t + %.6g", A, B, C, u);
      }
      n = add(out, n, "comes to rest when v=0, so t = %.10g", rt);
      n = add(out, n, "distance = integral(v) from 0 to %.10g", rt);
      return add(out, n, "distance = %.10g", ss < 0 ? -ss : ss);
    }
  }
  if ((has(t, "acceleration") || has(t, "accn")) && has(t, "t") &&
      (has(t, "initial") || has(t, "initially")) && (has(c, "findv") || has(c, "findvelocity") || has(c, "findvand")) &&
      nv >= 3) {
    double k = v[0], c0 = v[1], u = v[2], s0 = nv > 3 ? v[3] : 0;
    int n = add(out, 0, "Given a = %.6g t %+.6g, integrate to get velocity.", k, c0);
    n = add(out, n, "v = integral(a) dt = 1/2*%.6g*t^2 %+.6g*t + C", k, c0);
    n = add(out, n, "initial velocity gives C = %.6g", u);
    n = add(out, n, "v = %.6g*t^2 %+.6g*t %+.6g", 0.5*k, c0, u);
    n = add(out, n, "s = integral(v) dt = %.6g*t^3 %+.6g*t^2 %+.6g*t + C", k/6.0, 0.5*c0, u);
    n = add(out, n, "initial position gives C = %.6g", s0);
    return add(out, n, "s = %.6g*t^3 %+.6g*t^2 %+.6g*t %+.6g", k/6.0, 0.5*c0, u, s0);
  }
  if ((has(c, "followsb(") || has(c, "~b(") || has(c, "xb(") || has(c, "distributionb(") || has(c, "b(")) && nv >= 2) {
    double Nd=0, pv=0;
    if (!dist2(c, "b(", &Nd, &pv)) { Nd = v[0]; pv = v[1]; }
    if ((pv <= 0 || pv >= 1) && (has(t, "mean") || has(t, "expected") || has(c, "e(x)")) &&
        (has(t, "variance") || has(t, "standarddeviation") || has(c, "standarddeviation") || has(t, "sd"))) {
      double mean = 0;
      bool hm = word_num(input, "mean", &mean) || word_num(input, "expected", &mean);
      if (!hm) mean = v[1];
      if (Nd <= 0) Nd = v[0];
      if (Nd > 0) {
        pv = mean / Nd;
        int n = add(out, 0, "For X ~ B(n,p), mean = np.");
        n = add(out, n, "p = mean/n = %.10g/%.10g = %.10g", mean, Nd, pv);
        double var = Nd*pv*(1-pv);
        n = add(out, n, "variance = np(1-p)");
        n = add(out, n, "= %.10g*%.10g*(1-%.10g) = %.10g", Nd, pv, pv, var);
        if (has(t, "standarddeviation") || has(c, "standarddeviation") || has(t, "sd")) return add(out, n, "sd = sqrt(%.10g) = %.10g", var, root(var));
        return n;
      }
    }
    if ((pv <= 0 || pv >= 1) && has(c, "p)")) {
      double pword = 0;
      if (word_num(input, "greaterthan", &pword) || word_num(input, "morethan", &pword) ||
          word_num(input, "lessthan", &pword) || word_num(input, "p", &pword)) pv = pword;
      else for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { pv = v[i]; break; }
    }
    int N = (int)Nd;
    int tail = prob_tail(c, t);
    int x = has(c, "one") ? 1 : 0;
    if (has(c, "nomorethan")) tail = -1;
    else if (has(c, "morethan") || has(c, "greaterthan")) tail = 2;
    else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
    else if (has(c, "greaterthan") || has(c, "morethan")) tail = 2;
    else if (has(c, "lessthanorequal") || has(c, "atmost")) tail = -1;
    else if (has(c, "lessthan") || has(t, "fewer")) tail = -2;
    if ((has(t, "find") || has(t, "deduce") || has(c, "find") || has(c, "deduce") ||
         has(c, "solveforp") || has(c, "unknownp") || has(c, ",p)")) && N > 0) {
      int r = -1; double q = 0;
      if (prob_x_equal_value(c, &r, &q) && r >= 0 && r <= N) {
        int n = add(out, 0, "Let X ~ B(%d, p).", N);
        n = add(out, n, "Use P(X=r)=nCr p^r(1-p)^(n-r).");
        n = add(out, n, "%dC%d p^%d(1-p)^%d = %.10g", N, r, r, N-r, q);
        if (r == 0) {
          double ans = 1.0 - nth_root(q, N);
          n = add(out, n, "(1-p)^%d = %.10g", N, q);
          return add(out, n, "p = 1 - %.10g^(1/%d) = %.10g", q, N, ans);
        }
        if (r == N) {
          double ans = nth_root(q, N);
          n = add(out, n, "p^%d = %.10g", N, q);
          return add(out, n, "p = %.10g^(1/%d) = %.10g", q, N, ans);
        }
        double C = choose(N, r), roots[4]; int rc = 0;
        double prevx = 0, prevf = C*pwr(0, r)*pwr(1, N-r) - q;
        for (int step = 1; step <= 100 && rc < 4; ++step) {
          double x0 = step / 100.0;
          double fx = C*pwr(x0, r)*pwr(1-x0, N-r) - q;
          if (prevf == 0) roots[rc++] = prevx;
          else if ((prevf < 0 && fx > 0) || (prevf > 0 && fx < 0)) {
            double alo = prevx, ahi = x0, flo = prevf;
            for (int it = 0; it < 40; ++it) {
              double mid = (alo + ahi) / 2.0;
              double fm = C*pwr(mid, r)*pwr(1-mid, N-r) - q;
              if ((flo < 0 && fm < 0) || (flo > 0 && fm > 0)) { alo = mid; flo = fm; }
              else ahi = mid;
            }
            roots[rc++] = (alo + ahi) / 2.0;
          }
          prevx = x0; prevf = fx;
        }
        if (!rc) return add(out, n, "No solution with 0 <= p <= 1.");
        char buf[96]; int bp = sprintf(buf, "p = ");
        for (int i = 0; i < rc && bp < 80; ++i) bp += sprintf(buf + bp, "%s%.10g", i ? " or " : "", roots[i]);
        return add(out, n, "%s", buf);
      }
    }
    if ((has(t, "smallest") || has(t, "least") || has(t, "minimum") ||
         has(t, "largest") || has(t, "greatest") || has(t, "maximum")) &&
        (has(c, "p(") || has(t, "probability") || has(t, "such")) && nv >= 3) {
      double q = -1;
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], Nd) || near_num(v[i], pv)) continue;
        if (v[i] > 0 && v[i] < 1) q = v[i];
      }
      if (q > 0) {
        bool upper = has(c, "p(x>=k") || has(c, "p(x>k") || has(t, "upper") || has(t, "atleast");
        bool ge = has(c, ">=") || has(c, ">") || has(t, "exceeds") || has(t, "greater");
        int best = upper ? N : 0; double prob = 0;
        if (!upper) {
          double cum = 0;
          for (int k = 0; k <= N; ++k) {
            cum += binomp(N, pv, k);
            if ((ge && cum > q) || (!ge && cum >= q)) { best = k; prob = cum; break; }
          }
          int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, pv);
          n = add(out, n, "Find the first k where the cumulative probability passes %.10g.", q);
          n = add(out, n, "P(X<=%d) = %.10g", best, prob);
          return add(out, n, "k = %d", best);
        } else {
          double cum = 0;
          for (int k = N; k >= 0; --k) {
            cum += binomp(N, pv, k);
            if ((ge && cum > q) || (!ge && cum >= q)) { best = k; prob = cum; break; }
          }
          int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, pv);
          n = add(out, n, "Find the boundary where the upper-tail probability passes %.10g.", q);
          n = add(out, n, "P(X>=%d) = %.10g", best, prob);
          return add(out, n, "k = %d", best);
        }
      }
    }
    if ((has(t, "hypothesis") || has(t, "test")) && (has(t, "observed") || has(c, "x="))) {
      double obs = 0, alpha = 0.05;
      bool ho = word_num(input, "observed", &obs) || label_num(input, "x", &obs);
      if (!ho) {
        for (int i = 0; i < nv; ++i) {
          if (near_num(v[i], Nd) || near_num(v[i], pv)) continue;
          obs = v[i]; ho = true; break;
        }
      }
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], Nd) || near_num(v[i], pv) || near_num(v[i], obs)) continue;
        if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; break; }
        if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
      }
      if (ho) {
        double htail = (tail > 0 || has(t, "upper") || has(t, "greater") || has(t, "more") || has(c, "righttail")) ? 1 :
                       ((tail < 0 || has(t, "lower") || has(t, "less") || has(t, "fewer") || has(t, "smaller")) ? -1 :
                        (obs > N * pv ? 1 : -1));
        sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", N, pv, (int)obs, alpha, htail);
        return eval_stats(cmd, out);
      }
    }
    if ((has(t, "critical") || has(t, "criticalregion") || has(t, "significance")) && nv >= 3) {
      double alpha = 0.05;
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], Nd) || near_num(v[i], pv)) continue;
        alpha = (has(t, "percent") || has(c, "%")) && v[i] >= 1 && v[i] <= 100 ? v[i] / 100.0 : (v[i] > 1 ? v[i] / 100.0 : v[i]);
        break;
      }
      if (has(t, "twotailed") || has(t, "twosided") || (has(t, "two") && (has(t, "tailed") || has(t, "sided")))) {
        double a2 = alpha / 2.0, cum = 0, lowerp = 0, upperp = 0;
        int loCrit = -1, hiCrit = N + 1;
        for (int k=0;k<=N;++k) { cum += binomp(N,pv,k); if (cum <= a2) { loCrit = k; lowerp = cum; } else break; }
        cum = 0;
        for (int k=N;k>=0;--k) { cum += binomp(N,pv,k); if (cum <= a2) { hiCrit = k; upperp = cum; } else break; }
        int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, pv);
        n = add(out, n, "Two-tailed test: split alpha equally between both tails.");
        n = add(out, n, "alpha/2 = %.6g", a2);
        n = add(out, n, "Lower tail probability = %.10g, upper tail probability = %.10g", lowerp, upperp);
        if (loCrit < 0 && hiCrit > N) return add(out, n, "no critical value at this alpha.");
        if (loCrit < 0) return add(out, n, "critical region: X >= %d", hiCrit);
        if (hiCrit > N) return add(out, n, "critical region: X <= %d", loCrit);
        return add(out, n, "critical region: X <= %d or X >= %d", loCrit, hiCrit);
      }
      double ctail = (tail > 0 || has(t, "upper") || has(c, "righttail")) ? 1 : -1;
      sprintf(cmd, "critbinom(%d,%.10g,%.10g,%.0f)", N, pv, alpha, ctail);
      return eval_stats(cmd, out);
    }
    bool hx = false; double ux[3], obs = 0; int nux = 0;
    if (word_num(input, "observed", &obs)) { x = (int)obs; hx = true; }
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], N) || near_num(v[i], pv) || (hx && near_num(v[i], x))) continue;
      if (nux < 3) ux[nux++] = v[i];
      if (!hx) { x = (int)v[i]; hx = true; }
    }
    if (nux >= 2 && has(t, "normal") && (has(t, "approx") || has(t, "approximation"))) {
      double lo = ux[0] < ux[1] ? ux[0] : ux[1];
      double hi = ux[0] < ux[1] ? ux[1] : ux[0];
      sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", N, pv, lo, hi);
      return eval_stats(cmd, out);
    }
    double ilo=0, ihi=0;
    if (prob_x_interval(c, &ilo, &ihi))
      return add_binom_range_lines(out, N, pv, (int)ilo, (int)ihi, "Required probability");
    if (nux >= 2 && has(t, "between")) {
      int lo = (int)(ux[0] < ux[1] ? ux[0] : ux[1]);
      int hi = (int)(ux[0] < ux[1] ? ux[1] : ux[0]);
      if ((has(t, "inclusive") || has(t, "included")) && lo <= hi)
        return add_binom_range_lines(out, N, pv, lo, hi, "Required probability");
      return add_binom_range_lines(out, N, pv, lo + 1, hi - 1, "Required probability");
    }
    double blo=0, bhi=0;
    bool no_more = has(c, "nomorethan");
    bool has_lower = !no_more && (word_num(input, "atleast", &blo) || word_num(input, "greaterthanorequal", &blo) || word_num(input, "morethan", &blo));
    bool lower_strict = !no_more && ((has(c, "morethan") && !has(c, "nomorethan")) || (has(c, "greaterthan") && !has(c, "greaterthanorequal")));
    bool has_upper = word_num(input, "lessthan", &bhi) || word_num(input, "fewerthan", &bhi) ||
                     word_num(input, "fewer", &bhi) || word_num(input, "atmost", &bhi) ||
                     word_num(input, "nomorethan", &bhi) || word_num(input, "lessthanorequal", &bhi);
    bool upper_strict = (has(c, "lessthan") || has(c, "fewerthan") || has(t, "fewer")) && !has(c, "lessthanorequal");
    if (has_lower && has_upper) {
      int lo = (int)blo + (lower_strict ? 1 : 0);
      int hi = (int)bhi - (upper_strict ? 1 : 0);
      return add_binom_range_lines(out, N, pv, lo, hi, "Required probability");
    }
    if (nux >= 2 && (has(c, "<x<") || has(c, "<x<=") || has(c, "x<="))) {
      int lo = (int)ux[0] + (has(c, "<x") ? 1 : 0), hi = (int)ux[1];
      return add_binom_range_lines(out, N, pv, lo, hi, "Required probability");
    }
    if (!hx && nv >= 3) x = (int)v[2];
    if (has(t, "normal") && (has(t, "approx") || has(t, "approximation")) && hx) {
      double lo = tail < 0 ? 0 : x, hi = tail < 0 ? x : N;
      sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", N, pv, lo, hi);
      return eval_stats(cmd, out);
    }
    if ((has(t, "hypothesis") || has(t, "test")) && hx && nv >= 3) {
      double alpha = 0.05;
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], Nd) || near_num(v[i], pv) || near_num(v[i], x)) continue;
        if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; break; }
        if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
      }
      double htail = (tail > 0 || has(t, "upper") || has(t, "greater") || has(t, "more") || has(c, "righttail")) ? 1 :
                     ((tail < 0 || has(t, "lower") || has(t, "less") || has(t, "fewer") || has(t, "smaller")) ? -1 :
                      (x > N * pv ? 1 : -1));
      sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", N, pv, x, alpha, htail);
      return eval_stats(cmd, out);
    }
    if (has(t, "poisson") && (has(t, "approx") || has(t, "approximation")) && hx) {
      sprintf(cmd, "poissonapprox(%d,%.10g,%d,%d)", N, pv, x, tail);
      return eval_stats(cmd, out);
    }
    if (hx && (has(c, "notequal") || has(c, "not=") || has(c, "!="))) {
      double px = binomp(N, pv, x), ans = 1 - px;
      int n = add(out, 0, "Use the complement for not equal.");
      n = add(out, n, "Let X ~ B(%d, %.6g).", N, pv);
      n = add(out, n, "P(X!=%d)=1-P(X=%d)", x, x);
      n = add(out, n, "P(X=%d) = %.10g", x, px);
      return add(out, n, "P(X!=%d) = %.10g", x, ans);
    }
    if (tail) sprintf(cmd, "binomtail(%d,%.10g,%d,%d)", N, pv, x, tail);
    else sprintf(cmd, "binom(%d,%.10g,%d)", N, pv, x);
    return eval_stats(cmd, out);
  }
	  if ((has(c, "~n(") || has(c, "followsn(") || has(c, "distributedn(") || has(c, "xn(")) && nv >= 3) {
	    double mu=0, second=0;
	    if (!dist2(c, "n(", &mu, &second)) { mu = v[0]; second = v[1]; }
	    double var = has(c, "^2") ? second * second : second;
	    int tail = prob_tail(c, t);
    double u[3]; int nu = 0;
    for (int i = 0; i < nv && nu < 3; ++i) {
      if (near_num(v[i], mu) || near_num(v[i], second)) continue;
	      if (has(c, "^2") && near_num(v[i], 2)) continue;
	      u[nu++] = v[i];
	    }
	    if ((has(c, "-a<x<") || has(c, "-k<x<") || has(c, "betweenmu") || has(c, "central")) &&
	        (has(t, "find") || has(t, "such")) && nu >= 1) {
	      double prob = 0;
	      for (int i = 0; i < nu; ++i) if (u[i] > 0 && u[i] < 1) { prob = u[i]; break; }
	      if (prob > 0) {
	        double sd = root(var);
	        double left = (1 + prob) / 2.0;
	        double z = inv_norm_left(left);
	        int n = add(out, 0, "For a central normal interval, split the outside probability equally.");
	        n = add(out, n, "P(mu-a<X<mu+a)=%.10g", prob);
	        n = add(out, n, "area to the left of mu+a = (1+%.10g)/2 = %.10g", prob, left);
	        n = add(out, n, "z = InvNorm(%.10g) = %.10g", left, z);
	        return add(out, n, "a = z*sigma = %.10g*%.10g = %.10g", z, sd, z*sd);
	      }
	    }
	    if ((has(c, "findk") || has(c, "findx") || has(c, "suchthat") || has(t, "percentile")) && nu >= 1 &&
	        !(has(c, "-a<x<") || has(c, "-k<x<") || has(c, "betweenmu") || has(c, "central"))) {
	      double area = 0;
	      for (int i = 0; i < nu; ++i) {
	        if (u[i] > 0 && u[i] < 1) { area = u[i]; break; }
	        if (u[i] > 1 && u[i] <= 100 && area == 0) area = u[i] / 100.0;
	      }
	      if (area <= 0) area = u[0];
	      if (has(c, "p(x>k") || has(c, "p(x>=k") || has(c, "morethank") ||
	          has(c, "greaterthank") || has(t, "exceeded") || has(t, "above")) {
	        int n = add(out, 0, "Area to the left = 1 - %.10g = %.10g", area, 1 - area);
	        area = 1 - area;
	        sprintf(cmd, "invnormalvar(%.10g,%.10g,%.10g)", area, mu, var);
	        int m = eval_stats(cmd, out + n);
	        return n + m;
	      }
	      sprintf(cmd, "invnormalvar(%.10g,%.10g,%.10g)", area, mu, var);
	      return eval_stats(cmd, out);
    }
	    if ((has(c, "p(") && has(c, "<x<")) || (nu >= 2 && (has(c, "between") || has(c, "<x<")))) {
	      if (nu < 2) return 0;
	      double lo = u[0], hi = u[1];
	      if (lo > hi) { double tmp = lo; lo = hi; hi = tmp; }
	      sprintf(cmd, "normalprobvar(%.10g,%.10g,%.10g,%.10g)", lo, hi, mu, var);
	      return eval_stats(cmd, out);
	    }
	    const char *bar = strstr(c, "|x");
	    if (bar) {
	      double centre = mu, radius = 0;
	      const char *xm = strstr(bar, "x-");
	      const char *xp = strstr(bar, "x+");
	      if (xm) centre = read_num(xm + 2);
	      else if (xp) centre = -read_num(xp + 2);
	      const char *rp = strstr(bar + 2, "|<=");
	      int rskip = 3;
	      if (!rp) { rp = strstr(bar + 2, "|<"); rskip = 2; }
	      if (rp) radius = read_num(rp + rskip);
	      if (radius > 0) {
	        double sd = root(var), lo = centre - radius, hi = centre + radius;
	        double z1 = (lo - mu) / sd, z2 = (hi - mu) / sd;
	        double ans = normal_cdf(z2) - normal_cdf(z1);
	        int n = add(out, 0, "P(|X-%.6g|<%.6g) means %.6g < X < %.6g.", centre, radius, lo, hi);
	        n = add(out, n, "X~N(%.6g, %.6g), so sigma = %.10g", mu, var, sd);
	        n = add(out, n, "z1=(%.10g-%.6g)/%.10g = %.10g", lo, mu, sd, z1);
	        n = add(out, n, "z2=(%.10g-%.6g)/%.10g = %.10g", hi, mu, sd, z2);
	        return add(out, n, "probability = %.10g", ans);
	      }
	    }
	    if (tail && nu >= 1) {
	      sprintf(cmd, "normaltailvar(%.10g,%.10g,%.10g,%d)", u[0], mu, var, tail > 0 ? 1 : -1);
	      return eval_stats(cmd, out);
	    }
	  }
	  if ((has(c, "~po(") || has(c, "~poisson(") || has(c, "followspo(") || has(c, "followspoisson(") ||
	       has(c, "xpo(") || has(c, "xpoisson(")) && nv >= 2) {
    if ((has(c, "lambda") || has(c, "poisson(lambda") || has(c, "po(lambda")) &&
        (has(c, "p(x=0)=") || has(c, "p(x=0)")) && nv >= 1) {
      double p0 = v[nv - 1];
      if (p0 > 0 && p0 < 1) {
        double lam0 = -ln_approx(p0);
        int n = add(out, 0, "For X~Po(lambda), P(X=0)=e^-lambda.");
        n = add(out, n, "e^-lambda = %.10g", p0);
        n = add(out, n, "-lambda = ln(%.10g)", p0);
        return add(out, n, "lambda = -ln(%.10g) = %.10g", p0, lam0);
      }
    }
	    double lam = 0;
	    if (!dist1(c, "po(", &lam) && !dist1(c, "poisson(", &lam)) lam = v[0];
	    double xd = 0; bool hx = label_num(input, "x", &xd) || word_num(input, "x", &xd);
	    int x = hx ? (int)xd : 0;
    for (int i = 0; !hx && i < nv; ++i) {
      if (near_num(v[i], lam)) continue;
	      if ((has(t, "percent") || has(t, "significance")) && v[i] > 0 && v[i] <= 10) continue;
	      x = (int)v[i]; hx = true; break;
	    }
	    if (has(t, "normal") && (has(t, "approx") || has(t, "approximation")) && hx) {
	      double sig = root(lam);
	      int n = add(out, 0, "Use normal approximation to X ~ Po(%.6g).", lam);
	      n = add(out, n, "mu = lambda = %.6g, sigma = sqrt(lambda) = %.6g", lam, sig);
	      if (has(c, "x>") || has(t, "greater") || has(t, "more")) {
	        double cc = x + 0.5, z = sig ? (cc - lam) / sig : 0;
	        n = add(out, n, "For P(X>%.6g), use continuity correction P(Y>%.6g).", (double)x, cc);
	        n = add(out, n, "z = (%.6g-%.6g)/%.6g = %.10g", cc, lam, sig, z);
	        return add(out, n, "P(Y>%.6g) = %.10g", cc, 1 - normal_cdf(z));
	      }
	      if (has(c, "x<") || has(t, "less") || has(t, "fewer")) {
	        double cc = x - 0.5, z = sig ? (cc - lam) / sig : 0;
	        n = add(out, n, "For P(X<%.6g), use continuity correction P(Y<%.6g).", (double)x, cc);
	        n = add(out, n, "z = (%.6g-%.6g)/%.6g = %.10g", cc, lam, sig, z);
	        return add(out, n, "P(Y<%.6g) = %.10g", cc, normal_cdf(z));
	      }
	      sprintf(cmd, "poissonnorm(%.10g,%.10g,%.10g)", lam, (double)x, (double)x);
	      return eval_stats(cmd, out);
	    }
	    int tail = prob_tail(c, t);
    if ((has(t, "given") || has(t, "conditional")) && hx) {
      double given = 0; int gtail = 0;
      const char *gp = strstr(c, "givenx<=");
      if (gp) { gtail = -1; given = read_num(gp + 8); }
      else if ((gp = strstr(c, "givenx<"))) { gtail = -2; given = read_num(gp + 7); }
      else if ((gp = strstr(c, "givenx>="))) { gtail = 1; given = read_num(gp + 8); }
      else if ((gp = strstr(c, "givenx>"))) { gtail = 2; given = read_num(gp + 7); }
      else given_tail_bound(c, &given, &gtail);
      if (gtail) {
        int atail = tail;
        const char *ap = strstr(c, "p(x>=");
        if (ap) { atail = 1; x = (int)read_num(ap + 5); }
        else if ((ap = strstr(c, "p(x>"))) { atail = 2; x = (int)read_num(ap + 4); }
        else if ((ap = strstr(c, "p(x<="))) { atail = -1; x = (int)read_num(ap + 5); }
        else if ((ap = strstr(c, "p(x<"))) { atail = -2; x = (int)read_num(ap + 4); }
        int alo = atail > 0 ? (atail == 2 ? x + 1 : x) : 0;
        int ahi = atail < 0 ? (atail == -2 ? x - 1 : x) : 80;
        int blo = gtail > 0 ? (gtail == 2 ? (int)given + 1 : (int)given) : 0;
        int bhi = gtail < 0 ? (gtail == -2 ? (int)given - 1 : (int)given) : 80;
        int lo = alo > blo ? alo : blo;
        int hi = ahi < bhi ? ahi : bhi;
        double nump = 0, denp = 0;
        for (int k = lo; k <= hi; ++k) nump += poissonp(lam, k);
        for (int k = blo; k <= bhi; ++k) denp += poissonp(lam, k);
        int n = add(out, 0, "Use conditional probability P(A|B)=P(A and B)/P(B).");
        n = add(out, n, "Let X ~ Po(%.6g).", lam);
        n = add(out, n, "A gives r=%d..%d, B gives r=%d..%d.", alo, ahi, blo, bhi);
        n = add(out, n, "A and B gives r=%d..%d.", lo, hi);
        return add(out, n, "P(A|B)=%.10g/%.10g = %.10g", nump, denp, denp ? nump / denp : 0);
      }
    }
    if ((has(t, "critical") || has(t, "criticalregion")) && nv >= 2) {
      double alpha = 0.05;
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], lam)) continue;
        if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; break; }
        if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
      }
      double ctail = (tail > 0 || has(t, "upper") || has(t, "right")) ? 1 : -1;
      sprintf(cmd, "critpoisson(%.10g,%.10g,%.0f)", lam, alpha, ctail);
      return eval_stats(cmd, out);
    }
    if ((has(t, "hypothesis") || has(t, "test") || has(t, "significance")) && hx) {
      double alpha = 0.05;
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], lam) || near_num(v[i], x)) continue;
        if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; break; }
        if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
      }
      double htail = (tail > 0 || has(t, "upper") || has(t, "increased") || has(t, "increase")) ? 1 : -1;
      sprintf(cmd, "hyppoisson(%.10g,%d,%.10g,%.0f)", lam, x, alpha, htail);
      return eval_stats(cmd, out);
    }
    double lo = 0, hi = 0;
    if (prob_x_interval(c, &lo, &hi)) {
      sprintf(cmd, "poissonrange(%.10g,%d,%d)", lam, (int)lo, (int)hi);
      return eval_stats(cmd, out);
    }
    if (has(t, "between") && nv >= 3) {
      sprintf(cmd, "poissonrange(%.10g,%d,%d)", lam, (int)v[nv-2], (int)v[nv-1]);
      return eval_stats(cmd, out);
    }
    if ((has(c, "fewerthan") || has(c, "lessthan")) && (has(c, "greaterthan") || has(c, "morethan"))) {
      double lowb = 0, highb = 0;
      const char *fp = strstr(c, "fewerthan");
      if (!fp) fp = strstr(c, "lessthan");
      const char *gp = strstr(c, "greaterthan");
      if (!gp) gp = strstr(c, "morethan");
      if (fp) lowb = read_num(fp + (starts(fp, "fewerthan") ? 9 : 8));
      if (gp) highb = read_num(gp + (starts(gp, "greaterthan") ? 11 : 8));
      if (lowb > 0 && highb > lowb) {
        int lhi = (int)lowb - 1, glo = (int)highb + 1;
        double ans = poissoncdf(lam, lhi) + (1.0 - poissoncdf(lam, glo - 1));
        int n = add(out, 0, "Split the disjoint Poisson tails and add their probabilities.");
        n = add(out, n, "Let X ~ Po(%.6g).", lam);
        n = add(out, n, "P(X<%.0f or X>%.0f)=P(X<=%d)+P(X>=%d)", lowb, highb, lhi, glo);
        return add(out, n, "= %.10g", ans);
      }
    }
    if (has(c, "notequal") || has(c, "not=") || has(c, "!=")) {
      double px = poissonp(lam, x);
      int n = add(out, 0, "Use the complement for not equal.");
      n = add(out, n, "Let X ~ Po(%.6g).", lam);
      n = add(out, n, "P(X!=%d)=1-P(X=%d)", x, x);
      n = add(out, n, "P(X=%d) = %.10g", x, px);
      return add(out, n, "P(X!=%d) = %.10g", x, 1 - px);
    }
    if (tail) sprintf(cmd, "poissontail(%.10g,%d,%d)", lam, x, tail);
    else sprintf(cmd, "poisson(%.10g,%d)", lam, x);
    return eval_stats(cmd, out);
  }
  if (((has(t, "vertical") || has(t, "vertically")) || ((has(t, "upward") || has(t, "upwards")) && !has(t, "angle") && !has(t, "degrees"))) &&
      (has(t, "upward") || has(t, "upwards") || has(t, "projected") || has(t, "thrown")) && nv >= 1) {
    double pos[4]; int np = 0;
    for (int i = 0; i < nv && np < 4; ++i) if (v[i] > 0) pos[np++] = v[i];
    double u0 = pos[0], g = 9.8, target = 0, h0 = 0;
    bool hU = word_num(input, "speed", &u0) || word_num(input, "velocity", &u0) || word_num(input, "initialspeed", &u0);
    bool hHeight = word_num(input, "height", &h0);
    if (!hHeight && np >= 2 && (has(t, "aboveground") || (has(t, "above") && has(t, "ground")))) { h0 = pos[0]; hHeight = true; }
    bool hTarget = hHeight;
    if (hTarget) target = h0;
    if (!hTarget && np >= 2 && (has(t, "height") || has(t, "metres") || has(t, "meters"))) { target = pos[1]; hTarget = true; }
    if ((has(t, "ground") || has(c, "hitsground") || has(c, "hitstheground"))) {
      if (hHeight) target = -h0;
      else if (np >= 2) target = hU ? -pos[0] : -pos[1];
      hTarget = true;
    }
    if (has(t, "greatestheight") || has(t, "maximumheight") || (has(t, "greatest") && has(t, "height")) ||
        ((has(t, "maximum") && has(t, "height")) && !(has(t, "time") || has(t, "return")))) {
      double maxh = h0 + u0*u0/(2*g);
      int n = add(out, 0, "At greatest height, vertical velocity is zero.");
      n = add(out, n, "v^2 = u^2 - 2g(s-h)");
      n = add(out, n, "s-h = u^2/(2g) = %.6g^2/(2*%.6g) = %.10g", u0, g, u0*u0/(2*g));
      return add(out, n, "greatest height above ground = %.10g + %.10g = %.10g m", h0, u0*u0/(2*g), maxh);
    }
    if (hTarget && !near_num(target, u0)) {
      double disc = u0*u0 - 2*g*target;
      int n = add(out, 0, "Use vertical SUVAT with upward positive.");
      n = add(out, n, "s = ut - 1/2 gt^2");
      n = add(out, n, "%.6g = %.6g t - 1/2*%.6g t^2", target, u0, g);
      n = add(out, n, "%.6g t^2 - %.6g t + %.6g = 0", 0.5*g, u0, target);
      if (disc < 0) return add(out, n, "discriminant < 0, so this height is not reached.");
      return add(out, n, "t = %.10g or %.10g", (u0 - root(disc)) / g, (u0 + root(disc)) / g);
    }
  }
  if ((has(t, "accelerating") || has(t, "accelerates") || has(t, "accelerated")) &&
      (has(t, "decelerating") || has(t, "decelerates") || has(t, "decelerated")) &&
      (has(c, "torest") || has(t, "rest")) && (has(t, "totaldistance") || (has(t, "total") && has(t, "distance"))) &&
      nv >= 3) {
    double pos[6]; int np = 0;
    for (int i = 0; i < nv && np < 6; ++i) if (v[i] > 0) pos[np++] = v[i];
    if (np < 3) return 0;
    double a0 = pos[0], t1 = pos[1], t2 = pos[2], u0 = has(t, "rest") ? 0 : pos[0];
    if (has(c, "m/s^2") && np >= 4 && near_num(pos[1], 2)) { t1 = pos[2]; t2 = pos[3]; }
    if (!has(t, "rest") && np >= 4) { a0 = pos[1]; t1 = pos[2]; t2 = pos[3]; }
    double vmax = u0 + a0*t1;
    double s1 = u0*t1 + 0.5*a0*t1*t1;
    double s2 = 0.5*(vmax + 0)*t2;
    int n = add(out, 0, "Split the motion into acceleration and deceleration stages.");
    n = add(out, n, "Stage 1: v = u + at = %.6g + %.6g*%.6g = %.10g", u0, a0, t1, vmax);
    n = add(out, n, "s1 = ut + 1/2 at^2 = %.10g", s1);
    n = add(out, n, "Stage 2: average speed = (%.10g+0)/2", vmax);
    n = add(out, n, "s2 = %.10g*%.6g = %.10g", 0.5*vmax, t2, s2);
    return add(out, n, "total distance = %.10g", s1 + s2);
  }
  if ((has(t, "accelerating") || has(t, "accelerates") || has(t, "accelerated")) &&
      (has(t, "decelerating") || has(t, "decelerates") || has(t, "decelerated")) &&
      (has(c, "torest") || has(t, "rest")) && (has(t, "totaltime") || (has(t, "total") && has(t, "time"))) &&
      nv >= 4) {
    double u0=0, v0=0, total_s=0, final_s=0;
    bool hu = word_num(input, "from", &u0);
    bool hv = word_num(input, "to", &v0);
    total_s = v[0];
    final_s = v[nv-1];
    if (!hu) u0 = v[1];
    if (!hv) v0 = v[2];
    double first_s = total_s - final_s;
    if (first_s > 0 && v0 > 0) {
      double t1 = 2*first_s/(u0+v0);
      double t2 = 2*final_s/(v0+0);
      int n = add(out, 0, "Split the journey into the accelerating part and the decelerating part.");
      n = add(out, n, "first distance = %.6g - %.6g = %.10g m", total_s, final_s, first_s);
      n = add(out, n, "For constant acceleration, s = 1/2(u+v)t.");
      n = add(out, n, "t1 = 2*%.10g/(%.6g+%.6g) = %.10g s", first_s, u0, v0, t1);
      n = add(out, n, "For deceleration to rest, t2 = 2*%.6g/(%.6g+0) = %.10g s", final_s, v0, t2);
      return add(out, n, "total time = %.10g s", t1 + t2);
    }
  }
  if ((has(t, "slope") || has(t, "incline") || has(t, "inclined") || has(t, "plane")) &&
      (has(t, "braking") || has(t, "brake")) &&
      (has(t, "constant") || has(t, "speed")) &&
      (has(t, "resistance") || has(t, "resistive")) && nv >= 3) {
    double m = 0, theta = 0, r = 0;
    bool hm = word_num(input, "mass", &m) || label_num(input, "mass", &m) || prev_word_num(input, "kg", &m);
    bool ha = word_num(input, "angle", &theta) || label_num(input, "angle", &theta) || prev_word_num(input, "degrees", &theta);
    bool hr = word_num(input, "resistance", &r) || label_num(input, "resistance", &r);
    if (!hm) for (int i = 0; i < nv; ++i) if (v[i] > 90) { m = v[i]; hm = true; break; }
    if (!ha) for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && v[i] > 0 && v[i] <= 90) { theta = v[i]; ha = true; break; }
    if (!hr) for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && !near_num(v[i], theta)) { r = v[i]; hr = true; break; }
    double down = m * 9.8 * deg_sine(theta);
    double brake = down - r;
    int n = add(out, 0, "At constant speed down the slope, resultant force along the plane is zero.");
    n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, theta, down);
    n = add(out, n, "Resistance and braking force act up the plane.");
    n = add(out, n, "mg sin(theta) = resistance + braking force");
    return add(out, n, "braking force = %.10g - %.10g = %.10g N", down, r, brake);
  }
    if ((has(c, "torest") || has(c, "comestorest") || has(c, "broughttorest") ||
       has(t, "retardation") || has(t, "deceleration") || has(t, "braking") || has(t, "brake")) &&
      (has(t, "distance") || has(t, "travelling") || has(t, "travels") || has(t, "travelling") || has(t, "travelled") || has(t, "metres") || has(t, "meters")) && nv >= 2) {
    if ((has(t, "incline") || has(t, "inclined") || has(t, "slope") || has(t, "plane")) &&
        (has(t, "rough") || has(t, "friction") || has(t, "coefficient") || has(t, "mu"))) {
      /* Let the rough-plane route resolve components and friction. */
    } else {
    double pos[4]; int np = 0;
    for (int i = 0; i < nv && np < 4; ++i) if (v[i] > 0) pos[np++] = v[i];
    if (np >= 2) {
      double u0 = pos[0], s0 = pos[1], a0 = -u0*u0/(2*s0), t0 = 2*s0/u0;
      int n = add(out, 0, "Use SUVAT for uniform deceleration.");
      n = add(out, n, "final velocity is 0 because it comes to rest.");
      n = add(out, n, "v^2 = u^2 + 2as");
      n = add(out, n, "a = (0^2-%.6g^2)/(2*%.6g) = %.10g", u0, s0, a0);
      n = add(out, n, "deceleration = %.10g", -a0);
      return add(out, n, "t = 2s/(u+v) = 2*%.6g/(%.6g+0) = %.10g", s0, u0, t0);
    }
    }
  }
  if ((has(t, "acceleration") || has(t, "accn") || has(c, "a=")) && has(t, "t") &&
      (has(t, "velocity") || has(t, "speed") || has(t, "rest") || has(c, "v=") || has(c, "u=")) &&
      (has(t, "displacement") || has(t, "distance")) && (has(t, "from") || has(t, "between"))) {
    double A=0, B=0, C=0, u=0, t1=0, t2=0;
    bool parsed_acc = parse_poly_after_word(input, "acceleration", &A, &B, &C);
    if (!parsed_acc && has(c, "a=")) parsed_acc = parse_velocity_quad(input, &A, &B, &C);
    bool has_u = word_num(input, "velocity", &u) || word_num(input, "speed", &u) || label_num(input, "v", &u) || label_num(input, "u", &u);
    if (!has_u && has(t, "rest")) { u = 0; has_u = true; }
    double tv0 = 0;
    bool htv0 = word_num_with_t(input, "at", &tv0);
    if (!htv0 && has(t, "rest")) { tv0 = 0; htv0 = true; }
    bool ht1 = word_num_with_t(input, "from", &t1) || word_num_with_t(input, "between", &t1);
    bool ht2 = word_num_with_t(input, "to", &t2) || word_num_with_t(input, "and", &t2);
    if (parsed_acc && has_u && ht1 && ht2) {
      double ant0 = A*tv0*tv0*tv0/3.0 + B*tv0*tv0/2.0 + C*tv0;
      double K = u - ant0;
      double v2 = A*t2*t2*t2/3.0 + B*t2*t2/2.0 + C*t2 + K;
      double s1=A*t1*t1*t1*t1/12.0 + B*t1*t1*t1/6.0 + C*t1*t1/2.0 + K*t1;
      double s2=A*t2*t2*t2*t2/12.0 + B*t2*t2*t2/6.0 + C*t2*t2/2.0 + K*t2;
      int n = add(out, 0, "Variable acceleration: integrate a(t) to get v(t), then integrate v(t).");
      if (near_num(A, 0)) n = add(out, n, "a = %.6g t %+.6g", B, C);
      else n = add(out, n, "a = %.6g t^2 %+.6g t %+.6g", A, B, C);
      n = add(out, n, "v(t) = %.6g t^3 %+.6g t^2 %+.6g t + C", A/3.0, B/2.0, C);
      n = add(out, n, "v(%.6g)=%.10g gives C = %.10g", tv0, u, K);
      n = add(out, n, "v(%.6g) = %.10g", t2, v2);
      n = add(out, n, "s(t) = %.6g t^4 %+.6g t^3 %+.6g t^2 %+.6g t", A/12.0, B/6.0, C/2.0, K);
      return add(out, n, "displacement = s(%.6g)-s(%.6g) = %.10g", t2, t1, s2 - s1);
    }
  }
  if (!has(t, "variable") && (has(t, "suvat") || has(t, "velocity") || has(t, "acceleration") || has(t, "accelerates") || has(t, "accelerated") || has(t, "distance") || has(t, "displacement") || has(t, "time"))) {
    if ((has(t, "slows") || has(t, "slowing") || has(t, "decelerates") || has(t, "decelerating")) &&
        has(t, "from") && has(t, "to") &&
        (has(t, "second") || has(t, "seconds") || has(t, "time") || has(t, "for") || has(t, "in")) &&
        (has(t, "distance") || has(t, "displacement") || has(t, "travelled") || has(t, "traveled")) && nv >= 3) {
      double u0 = v[0], v0 = v[1], t0 = v[2];
      prev_word_num(input, "seconds", &t0) || prev_word_num(input, "second", &t0) ||
        word_num(input, "time", &t0) || word_num(input, "for", &t0) || word_num(input, "in", &t0);
      double a0 = (v0 - u0) / t0, s0 = 0.5 * (u0 + v0) * t0;
      int n = add(out, 0, "Use SUVAT for constant deceleration.");
      n = add(out, n, "v = u + at, so a=(v-u)/t");
      n = add(out, n, "a = (%.6g-%.6g)/%.6g = %.10g", v0, u0, t0, a0);
      n = add(out, n, "s = 1/2(u+v)t");
      n = add(out, n, "s = 1/2(%.6g+%.6g)*%.6g", u0, v0, t0);
      return add(out, n, "s = %.10g", s0);
    }
    if ((has(t, "brake") || has(t, "brakes") || has(t, "deceleration") || has(t, "decelerates")) && nv >= 2) {
      double u0 = v[0], s0 = v[1], v0 = 0;
      double a0 = (v0*v0-u0*u0)/(2*s0), t0 = 2*s0/(u0+v0);
      int n = add(out, 0, "Use SUVAT for uniform deceleration.");
      n = add(out, n, "final velocity is 0 because it comes to rest.");
      n = add(out, n, "v^2 = u^2 + 2as");
      n = add(out, n, "a = (0^2-%.6g^2)/(2*%.6g) = %.10g", u0, s0, a0);
      n = add(out, n, "deceleration = %.10g", a0 < 0 ? -a0 : a0);
      return add(out, n, "t = 2s/(u+v) = 2*%.6g/(%.6g+0) = %.10g", s0, u0, t0);
    }
    if ((has(t, "vertical") || has(t, "vertically")) && (has(t, "upward") || has(t, "upwards") || has(t, "thrown")) && nv >= 1) {
      double u0 = v[0], g = 9.8, target = 0, h0 = 0;
      bool hU = word_num(input, "speed", &u0) || word_num(input, "velocity", &u0) || word_num(input, "initialspeed", &u0);
      bool hHeight = word_num(input, "height", &h0);
      if (!hHeight && nv >= 2 && (has(t, "aboveground") || (has(t, "above") && has(t, "ground")))) { h0 = v[0]; hHeight = true; }
      bool hTarget = hHeight;
      if (hTarget) target = h0;
      if (!hTarget && (has(t, "above") || has(t, "height")) && nv >= 2) { target = v[1]; hTarget = true; }
      if (has(t, "ground") || has(c, "hitsground") || has(c, "hitstheground")) {
        if (hHeight) target = -h0;
        else if (nv >= 2) target = hU ? -v[0] : -v[1];
        hTarget = true;
      }
      if (has(t, "greatestheight") || has(t, "maximumheight") || (has(t, "greatest") && has(t, "height")) ||
          ((has(t, "maximum") && has(t, "height")) && !(has(t, "time") || has(t, "return")))) {
        double maxh = h0 + u0*u0/(2*g);
        int n = add(out, 0, "At greatest height, vertical velocity is zero.");
        n = add(out, n, "v^2 = u^2 - 2g(s-h)");
        n = add(out, n, "s-h = u^2/(2g) = %.6g^2/(2*%.6g) = %.10g", u0, g, u0*u0/(2*g));
        return add(out, n, "greatest height above ground = %.10g + %.10g = %.10g m", h0, u0*u0/(2*g), maxh);
      }
      if (hTarget && !near_num(target, u0)) {
        double disc = u0*u0 - 2*g*target;
        int n = add(out, 0, "Use vertical SUVAT with upward positive.");
        n = add(out, n, "s = ut - 1/2 gt^2");
        n = add(out, n, "%.6g = %.6g t - 1/2*%.6g t^2", target, u0, g);
        n = add(out, n, "%.6g t^2 - %.6g t + %.6g = 0", 0.5*g, u0, target);
        if (disc < 0) return add(out, n, "discriminant < 0, so this height is not reached.");
        double t1 = (u0 - root(disc)) / g, t2 = (u0 + root(disc)) / g;
        return add(out, n, "t = %.10g or %.10g", t1, t2);
      }
      int n = add(out, 0, "Use vertical SUVAT with upward positive.");
      n = add(out, n, "At maximum height, v = 0 and a = -g.");
      n = add(out, n, "0 = u^2 - 2gs, so s = u^2/(2g)");
      n = add(out, n, "s = %.6g^2/(2*%.6g) = %.10g", u0, g, u0*u0/(2*g));
      n = add(out, n, "time to maximum height: 0 = u - gt, so t = u/g = %.10g", u0/g);
      return add(out, n, "time to return to launch height = 2u/g = %.10g", 2*u0/g);
    }
    if ((has(t, "uniform") || has(t, "uniformly") || has(t, "accelerates")) && has(t, "from") && has(t, "to") &&
        (has(t, "travels") || has(t, "travelled") || has(t, "traveled") || has(t, "distance")) &&
        nv >= 3 && v[0] > v[1] && v[0] > v[2]) {
      double s0 = v[0], u0 = v[1], v0 = v[2];
      double a0 = (v0*v0-u0*u0)/(2*s0), t0 = 2*s0/(u0+v0);
      int n = add(out, 0, "Use SUVAT for constant acceleration.");
      n = add(out, n, "v^2 = u^2 + 2as");
      n = add(out, n, "a = (v^2-u^2)/(2s) = (%.6g^2-%.6g^2)/(2*%.6g) = %.10g", v0, u0, s0, a0);
      n = add(out, n, "s = 1/2(u+v)t");
      return add(out, n, "t = 2s/(u+v) = 2*%.6g/(%.6g+%.6g) = %.10g", s0, u0, v0, t0);
    }
    if ((has(t, "uniform") || has(t, "uniformly") || has(t, "accelerates")) && has(t, "from") && has(t, "to") &&
        (has(t, "second") || has(t, "seconds") || has(t, "time") || has(t, "for") || has(t, "in")) &&
        (has(t, "distance") || has(t, "displacement") || has(t, "travelled") || has(t, "traveled")) && nv >= 3) {
      double u0 = v[0], v0 = v[1], t0 = v[2];
      prev_word_num(input, "seconds", &t0) || prev_word_num(input, "second", &t0) ||
        word_num(input, "time", &t0) || word_num(input, "for", &t0) || word_num(input, "in", &t0);
      double a0 = (v0 - u0) / t0, s0 = 0.5 * (u0 + v0) * t0;
      int n = add(out, 0, "Use SUVAT for constant acceleration.");
      n = add(out, n, "v = u + at, so a=(v-u)/t");
      n = add(out, n, "a = (%.6g-%.6g)/%.6g = %.10g", v0, u0, t0, a0);
      n = add(out, n, "s = 1/2(u+v)t");
      n = add(out, n, "s = 1/2(%.6g+%.6g)*%.6g", u0, v0, t0);
      return add(out, n, "s = %.10g", s0);
    }
    if ((has(t, "uniform") || has(t, "uniformly") || has(t, "accelerates")) && has(t, "from") && has(t, "to") &&
        (has(t, "over") || has(t, "distance")) && nv >= 3) {
      double u0 = v[0], v0 = v[1], s0 = v[2];
      double a0 = (v0*v0-u0*u0)/(2*s0), t0 = 2*s0/(u0+v0);
      int n = add(out, 0, "Use SUVAT for constant acceleration.");
      n = add(out, n, "v^2 = u^2 + 2as");
      n = add(out, n, "a = (v^2-u^2)/(2s) = (%.6g^2-%.6g^2)/(2*%.6g) = %.10g", v0, u0, s0, a0);
      n = add(out, n, "s = 1/2(u+v)t");
      return add(out, n, "t = 2s/(u+v) = 2*%.6g/(%.6g+%.6g) = %.10g", s0, u0, v0, t0);
    }
    if ((has(t, "slows") || has(t, "slowing") || has(t, "decelerates") || has(t, "decelerating")) &&
        has(t, "from") && has(t, "to") && (has(t, "travelling") || has(t, "traveling") || has(t, "over") || has(t, "distance")) &&
        nv >= 3) {
      double u0 = v[0], v0 = v[1], s0 = v[2];
      double a0 = (v0*v0-u0*u0)/(2*s0), t0 = 2*s0/(u0+v0);
      int n = add(out, 0, "Use SUVAT for constant deceleration.");
      n = add(out, n, "v^2 = u^2 + 2as");
      n = add(out, n, "a = (v^2-u^2)/(2s) = (%.6g^2-%.6g^2)/(2*%.6g) = %.10g", v0, u0, s0, a0);
      n = add(out, n, "s = 1/2(u+v)t");
      return add(out, n, "t = 2s/(u+v) = 2*%.6g/(%.6g+%.6g) = %.10g", s0, u0, v0, t0);
    }
    double u=0, vv=0, acc=0, dist=0, time=0;
    bool hu=label_num(input,"u",&u) || label_num(input,"initialvelocity",&u) || label_num(input,"initialspeed",&u) ||
            word_num(input,"initialvelocity",&u) || word_num(input,"initialspeed",&u);
    bool hv=label_num(input,"v",&vv) || label_num(input,"finalvelocity",&vv) || label_num(input,"finalspeed",&vv) ||
            word_num(input,"finalvelocity",&vv) || word_num(input,"finalspeed",&vv) || word_num(input,"reachesspeed",&vv);
    bool ha=label_num(input,"a",&acc) || label_num(input,"acceleration",&acc) ||
            word_num(input,"acceleration",&acc) || word_num(input,"acceleratesat",&acc) || word_num(input,"acceleratedat",&acc) ||
            first_num_after_word(input,"accelerates",&acc) || first_num_after_word(input,"accelerated",&acc) ||
            first_num_after_word(input,"accelerating",&acc);
    bool hs=label_num(input,"s",&dist) || label_num(input,"displacement",&dist) || label_num(input,"distance",&dist) ||
            word_num(input,"displacement",&dist) || word_num(input,"distance",&dist);
    bool ht=label_num(input,"t",&time) || label_num(input,"time",&time) ||
            word_num(input,"time",&time) || word_num(input,"for",&time) || word_num(input,"in",&time);
    bool rest = has(c, "fromrest");
    if (!rest && !hu && has(t, "from") && word_num(input, "speed", &u)) hu = true;
    if (!rest && !hu && has(t, "from") && word_num(input, "velocity", &u)) hu = true;
    if (!hu && rest) { u = 0; hu = true; }
    if (!hv && rest && nv >= 1 && (has(c, "fromrestto") || has(c, "reachesspeed") || has(c, "reachesaspeed") ||
        has(c, "finalspeed") || has(c, "finalvelocity"))) { vv = v[0]; hv = true; }
    if (ht && (has(t, "minute") || has(t, "minutes"))) time *= 60.0;
    if (ht && (has(t, "hour") || has(t, "hours"))) time *= 3600.0;
    int known = (hu?1:0) + (hv?1:0) + (ha?1:0) + (hs?1:0) + (ht?1:0);
    bool wants_dist = has(t, "distance") || has(t, "displacement") || has(t, "travelled") || has(t, "traveled");
    bool wants_v = has(c, "finalspeed") || has(c, "finalvelocity") || has(c, "speedanddistance") || has(c, "velocityanddistance");
    if (!hs && hu && ha && ht && wants_dist) {
      int n = add(out, 0, "List known values and choose a SUVAT equation.");
      if (rest) n = add(out, n, "From rest gives u = 0.");
      if (wants_v) {
        n = add(out, n, "v = u + at");
        n = add(out, n, "v = %.6g + %.6g*%.6g = %.10g", u, acc, time, u + acc*time);
      }
      n = add(out, n, "s = ut + 1/2 at^2");
      n = add(out, n, "s = %.6g*%.6g + 1/2*%.6g*%.6g^2", u, time, acc, time);
      return add(out, n, "s = %.10g", u*time + 0.5*acc*time*time);
    }
    if (!ha && !hs && hu && hv && ht && has(t, "acceleration") && wants_dist) {
      int n = add(out, 0, "List known values and choose SUVAT equations.");
      if (rest) n = add(out, n, "From rest gives u = 0.");
      n = add(out, n, "v = u + at, so a = (v-u)/t");
      n = add(out, n, "a = (%.6g-%.6g)/%.6g = %.10g", vv, u, time, (vv-u)/time);
      n = add(out, n, "s = 1/2(u+v)t");
      n = add(out, n, "s = 1/2(%.6g+%.6g)*%.6g", u, vv, time);
      return add(out, n, "s = %.10g", 0.5*(u+vv)*time);
    }
    if (known >= 3) {
      int p = sprintf(cmd, "suvat(");
      bool any = false;
      if (hu) { p += sprintf(cmd+p, "u=%.10g", u); any = true; }
      if (hv) { p += sprintf(cmd+p, "%sv=%.10g", any ? "," : "", vv); any = true; }
      if (ha) { p += sprintf(cmd+p, "%sa=%.10g", any ? "," : "", acc); any = true; }
      if (hs) { p += sprintf(cmd+p, "%ss=%.10g", any ? "," : "", dist); any = true; }
      if (ht) { p += sprintf(cmd+p, "%st=%.10g", any ? "," : "", time); any = true; }
      sprintf(cmd+p, ")");
      return eval_suvat(cmd, out);
    }
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) &&
      (has(t, "rough") || has(t, "friction") || has(t, "coefficient") || has(t, "mu")) &&
      (has(t, "projected") || has(t, "projection")) &&
      (has(t, "rest") || has(t, "stop") || has(t, "stops")) && nv >= 3) {
    double m=0, ang=0, mu=0, u=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    bool hmu=label_num(input,"mu",&mu) || label_num(input,"coefficient",&mu) || word_num(input,"coefficient",&mu) || word_num(input,"friction",&mu);
    bool hu=label_num(input,"speed",&u) || label_num(input,"u",&u) || word_num(input,"speed",&u) || prev_word_num(input,"m/s",&u);
    if (!hm) m = v[0];
    if (!ha) for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && v[i] > 1.5 && v[i] <= 90) { ang = v[i]; break; }
    if (!hmu) for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && !near_num(v[i], ang) && v[i] > 0 && v[i] < 1.5) { mu = v[i]; break; }
    if (!hu) for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], mu)) { u = v[i]; break; }
    double decel = 9.8 * (deg_sine(ang) + mu * deg_cosine(ang));
    double s = decel ? u*u/(2.0*decel) : 0;
    int n = add(out, 0, "Up a rough plane, weight component and friction oppose motion.");
    n = add(out, n, "R = mg cos(theta), friction = mu R.");
    n = add(out, n, "deceleration = g(sin(theta)+mu cos(theta))");
    n = add(out, n, "deceleration = 9.8*(sin(%.6g)+%.6g*cos(%.6g)) = %.10g", ang, mu, ang, decel);
    n = add(out, n, "Use v^2 = u^2 + 2as with v=0 and a=-%.10g.", decel);
    return add(out, n, "distance = u^2/(2*deceleration) = %.6g^2/(2*%.10g) = %.10g m", u, decel, s);
  }
  if (is_projectile_text(t) && has(c, "maximumheight") && (has(c, "timeofflight") || (has(t, "time") && has(t, "flight"))) &&
      (has(t, "speed") || has(t, "projection")) && nv >= 2) {
    double u=0, ang=0, g=9.8;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) ||
            word_num(input,"speed",&u) || word_num(input,"velocity",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || word_num(input,"angle",&ang) ||
            prev_word_num(input,"degrees",&ang);
    label_num(input,"g",&g);
    if (!hU) u = v[0];
    if (!hA) ang = v[1];
    double uy = u * deg_sine(ang);
    double tof = 2 * uy / g;
    double H = uy * uy / (2 * g);
    int n = add(out, 0, "Resolve vertically, then use vertical motion.");
    n = add(out, n, "u_y = u sin(theta) = %.6g sin %.6g = %.10g", u, ang, uy);
    n = add(out, n, "time of flight: 0 = u_y t - 0.5gt^2");
    n = add(out, n, "t = 2u_y/g = %.10g s", tof);
    n = add(out, n, "maximum height uses v_y=0.");
    return add(out, n, "H = u_y^2/(2g) = %.10g m", H);
  }
	  if (is_projectile_text(t) && has(c, "maximumheight") &&
	      (has(t, "speed") || has(t, "projection") || has(c, "speedofprojection")) && nv >= 2) {
    double H=0, ang=0;
    bool hH=word_num(input,"height",&H) || label_num(input,"height",&H);
    bool hA=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hH) H = v[0];
    if (!hA) ang = v[1];
    double s = deg_sine(ang), u = s ? root(2.0*9.8*H)/s : 0;
    int n = add(out, 0, "At maximum height, vertical velocity is zero.");
	    n = add(out, n, "H = (u sin(theta))^2/(2g)");
	    n = add(out, n, "u sin(%.6g) = sqrt(2*9.8*%.6g)", ang, H);
	    return add(out, n, "u = %.10g m/s", u);
	  }
	  if (is_projectile_text(t) && (has(t, "time") || has(t, "when") || has(t, "times")) &&
	      (has(t, "high") || has(t, "above") || has(t, "height")) && nv >= 3 &&
	      !has(t, "ground") && !has(t, "flight") && !has(t, "hits") && !has(t, "hit")) {
	    double u=0, ang=0, y=0, h0=0, g=0;
	    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u) ||
	            word_num(input,"speed",&u) || word_num(input,"initialspeed",&u) || word_num(input,"velocity",&u);
	    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || label_num(input,"launchangle",&ang) ||
	            word_num(input,"angle",&ang) || word_num(input,"theta",&ang) || prev_word_num(input,"degrees",&ang);
	    bool hY=label_num(input,"height",&y) || label_num(input,"y",&y) || word_num(input,"height",&y) || word_num(input,"above",&y) || word_num(input,"high",&y) ||
	            prev_word_num(input,"high",&y) || prev_word_num(input,"above",&y);
	    bool hH=label_num(input,"initialheight",&h0) || label_num(input,"launchheight",&h0) || label_num(input,"h0",&h0);
	    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
	    if (!hU && nv >= 1) { u = v[0]; hU = true; }
	    if (!hA && nv >= 2) { ang = v[1]; hA = true; }
	    if (hU && hA && !hY) {
	      for (int i = 0; i < nv; ++i) if (!near_num(v[i], u) && !near_num(v[i], ang)) { y = v[i]; hY = true; break; }
	    }
	    if (hU && hA && hY) {
	      sprintf(cmd, hG ? "projectiley(%.10g,%.10g,%.10g,%.10g,%.10g)" : "projectiley(%.10g,%.10g,%.10g,%.10g)", u, ang, y, hH ? h0 : 0, hG ? g : 9.8);
	      return eval_mech(cmd, out);
	    }
	  }
	  if (is_projectile_text(t) && (has(t, "speed") || has(t, "direction")) &&
	      (has(t, "after") || has(t, "seconds")) &&
	      !has(t, "high") && !has(t, "height") && !has(t, "above") && !has(t, "times") && nv >= 3) {
    double u=0, ang=0, time=0, g=9.8;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || word_num(input,"speed",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    bool hT=label_num(input,"t",&time) || label_num(input,"time",&time) || word_num(input,"after",&time);
    label_num(input,"g",&g);
    if (!hU) u = v[0];
    if (!hA) ang = v[1];
    if (!hT) time = v[2];
    double vx = u * deg_cosine(ang);
    double vy = u * deg_sine(ang) - g * time;
    int n = add(out, 0, "Resolve the initial velocity, then use vertical acceleration -g.");
    n = add(out, n, "v_x = u cos(theta) = %.6g cos(%.6g) = %.10g", u, ang, vx);
    n = add(out, n, "v_y = u sin(theta) - gt = %.6g sin(%.6g) - %.6g*%.6g = %.10g", u, ang, g, time, vy);
    double dir = arctan(vy / vx) * 180.0 / M_PI;
    n = add(out, n, "speed = sqrt(v_x^2+v_y^2) = %.10g m/s", root(vx*vx + vy*vy));
    if (has(t, "direction") || has(t, "angle")) return add(out, n, "direction = arctan(v_y/v_x) = %.10g degrees", dir);
    return n;
  }
  if (is_projectile_text(t) && (has(t, "angle") || has(t, "angles") || (has(t, "find") && has(t, "angle"))) &&
      (has(t, "target") || has(t, "point") || has(t, "through") || has(t, "pass")) &&
      !has(t, "ground") && !has(t, "flight") && nv >= 3) {
    double u=0,x=0,y=0,h0=0,g=0;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u);
    bool hX=label_num(input,"x",&x) || label_num(input,"distance",&x) || label_num(input,"range",&x) || label_num(input,"horizontaldistance",&x);
    bool hY=label_num(input,"y",&y) || label_num(input,"height",&y) || label_num(input,"targetheight",&y);
    bool hH=label_num(input,"initialheight",&h0) || label_num(input,"launchheight",&h0) || label_num(input,"h0",&h0);
    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
    if (!hH && nv >= 3 && (has(t, "cliff") || has(t, "height") || has(t, "high"))) { h0 = v[2]; hH = true; }
    if (hU && hX && hY) {
      sprintf(cmd, hG ? "projectileangle(%.10g,%.10g,%.10g,%.10g,%.10g)" : "projectileangle(%.10g,%.10g,%.10g,%.10g)", u, x, y, hH ? h0 : 0, hG ? g : 9.8);
      return eval_mech(cmd, out);
    }
    sprintf(cmd, nv >= 4 ? "projectileangle(%.10g,%.10g,%.10g,%.10g)" : "projectileangle(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv >= 4 ? v[3] : 0); return eval_mech(cmd, out);
  }
  if (is_projectile_text(t) && (has(c, "findangle") || has(c, "findtheangle") || has(t, "launchangle") || has(t, "angles")) && nv >= 2) {
    double u=0,x=0,y=0,h0=0,g=0;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u);
    bool hX=label_num(input,"x",&x) || label_num(input,"distance",&x) || label_num(input,"range",&x) || label_num(input,"horizontaldistance",&x);
    bool hY=label_num(input,"y",&y) || label_num(input,"height",&y) || label_num(input,"targetheight",&y);
    bool hH=label_num(input,"initialheight",&h0) || label_num(input,"launchheight",&h0) || label_num(input,"h0",&h0);
    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
    if (!hH && nv >= 3 && (has(t, "cliff") || has(t, "height") || has(t, "high"))) { h0 = v[2]; hH = true; }
    if (hU && hX && hY) {
      sprintf(cmd, hG ? "projectileangle(%.10g,%.10g,%.10g,%.10g,%.10g)" : "projectileangle(%.10g,%.10g,%.10g,%.10g)", u, x, y, hH ? h0 : 0, hG ? g : 9.8);
      return eval_mech(cmd, out);
    }
    if (nv >= 2 && (has(t, "samelevel") || has(t, "ground") || has(t, "range") || has(t, "away"))) {
      sprintf(cmd, "projectileangle(%.10g,%.10g,0,0)", v[0], v[1]);
      return eval_mech(cmd, out);
    }
  }
  if (is_projectile_text(t) && (has(t, "ground") || has(c, "hitsground") || has(c, "hitstheground")) &&
      (has(t, "time") || has(t, "when") || has(t, "distance")) && nv >= 3) {
    double u=0, ang=0, h0=0, g=9.8;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u) ||
            word_num(input,"speed",&u) || word_num(input,"velocity",&u) ||
            num_before_unit(input,"m/s",&u) || num_before_unit(input,"ms^-1",&u) ||
            prev_word_num(input,"metrespersecond",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || label_num(input,"launchangle",&ang) ||
            word_num(input,"angle",&ang) || word_num(input,"theta",&ang) || prev_word_num(input,"degrees",&ang);
    bool hH=label_num(input,"height",&h0) || label_num(input,"initialheight",&h0) || label_num(input,"launchheight",&h0) ||
            word_num(input,"height",&h0) || word_num(input,"from",&h0) || word_num(input,"above",&h0);
    label_num(input,"g",&g) || label_num(input,"gravity",&g);
    if (!hU && nv >= 1) u = v[0];
    if (!hA && nv >= 2) ang = v[1];
    if (!hH && nv >= 3 && (has(t, "height") || has(t, "above") || has(t, "cliff") || has(t, "ground"))) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], u) && !near_num(v[i], ang)) { h0 = v[i]; hH = true; break; }
    }
    double ux = u * deg_cosine(ang), uy = u * deg_sine(ang);
    double disc = uy*uy + 2*g*h0;
    int n = add(out, 0, "Resolve the velocity, then use vertical motion to hit the ground.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.10g", u, ang, ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.10g", u, ang, uy);
    n = add(out, n, "h=%.6g", h0);
    n = add(out, n, "0 = %.6g + %.10g t - 1/2*%.6g t^2", h0, uy, g);
    if (disc < 0) return add(out, n, "No real time reaches the ground.");
    double time = (uy + root(disc)) / g;
    n = add(out, n, "positive time t = %.10g s", time);
    if (has(t, "greatest") || has(t, "maximum") || has(t, "maxheight")) {
      double maxh = h0 + (uy > 0 ? uy*uy/(2*g) : 0);
      n = add(out, n, "At greatest height, vertical velocity is zero.");
      n = add(out, n, "greatest height above ground = %.10g + %.10g^2/(2*%.6g) = %.10g m", h0, uy, g, maxh);
    }
    return add(out, n, "range = u_x t = %.10g m", ux*time);
  }
  if (is_projectile_text(t) &&
      ((has(t, "distance") || has(t, "metresaway") || has(t, "away")) ||
       ((has(t, "horizontal") || has(t, "horizontally")) && (has(t, "height") || has(t, "high")) && !has(t, "range"))) &&
      nv >= 3) {
    double u=0,ang=0,x=0,h0=0,g=0;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || label_num(input,"launchangle",&ang);
    bool hX=label_num(input,"x",&x) || label_num(input,"distance",&x) || label_num(input,"range",&x) || label_num(input,"horizontaldistance",&x);
    bool hH=label_num(input,"initialheight",&h0) || label_num(input,"height",&h0) || label_num(input,"h0",&h0);
    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
    if (!hH && nv >= 4 && (has(t, "cliff") || has(t, "height") || has(t, "high"))) { h0 = v[3]; hH = true; }
    if (hU && hA && hX) {
      sprintf(cmd, hG ? "projectileat(%.10g,%.10g,%.10g,%.10g,%.10g)" : "projectileat(%.10g,%.10g,%.10g,%.10g)", u, ang, x, hH ? h0 : 0, hG ? g : 9.8);
      return eval_mech(cmd, out);
    }
    sprintf(cmd, nv >= 4 ? "projectileat(%.10g,%.10g,%.10g,%.10g)" : "projectileat(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv >= 4 ? v[3] : 0); return eval_mech(cmd, out);
  }
	  if (is_projectile_text(t) && (has(t, "time") || has(t, "when") || has(t, "times")) &&
	      (has(t, "high") || has(t, "above") || has(t, "height")) && nv >= 3 &&
	      !has(t, "ground") && !has(t, "flight") && !has(t, "hits") && !has(t, "hit")) {
    double u=0, ang=0, y=0, h0=0, g=0;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u) ||
            word_num(input,"speed",&u) || word_num(input,"initialspeed",&u) || word_num(input,"velocity",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || label_num(input,"launchangle",&ang) ||
            word_num(input,"angle",&ang) || word_num(input,"theta",&ang) || prev_word_num(input,"degrees",&ang);
    bool hY=label_num(input,"height",&y) || label_num(input,"y",&y) || word_num(input,"height",&y) || word_num(input,"above",&y) || word_num(input,"high",&y) ||
            prev_word_num(input,"high",&y) || prev_word_num(input,"above",&y);
    bool hH=label_num(input,"initialheight",&h0) || label_num(input,"launchheight",&h0) || label_num(input,"h0",&h0);
    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
    if (hU && !hA && nv >= 2) { ang = v[1]; hA = true; }
    if (hU && hA && !hY) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], u) && !near_num(v[i], ang)) { y = v[i]; hY = true; break; }
    }
    if (hU && hA && hY) {
      sprintf(cmd, hG ? "projectiley(%.10g,%.10g,%.10g,%.10g,%.10g)" : "projectiley(%.10g,%.10g,%.10g,%.10g)", u, ang, y, hH ? h0 : 0, hG ? g : 9.8);
      return eval_mech(cmd, out);
    }
  }
  if (is_projectile_text(t) && has(c, "maximumheight") &&
      (has(t, "speed") || has(t, "projection") || has(c, "speedofprojection")) && nv >= 2) {
    double H=0, ang=0;
    bool hH=word_num(input,"height",&H) || label_num(input,"height",&H);
    bool hA=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hH) H = v[0];
    if (!hA) ang = v[1];
    double s = deg_sine(ang), u = s ? root(2.0*9.8*H)/s : 0;
    int n = add(out, 0, "At maximum height, vertical velocity is zero.");
    n = add(out, n, "H = (u sin(theta))^2/(2g)");
    n = add(out, n, "u sin(%.6g) = sqrt(2*9.8*%.6g)", ang, H);
    return add(out, n, "u = %.10g m/s", u);
  }
  if (is_projectile_text(t) && (has(t, "height") || has(t, "above")) && nv >= 3) {
    double u=0,ang=0,h=0,g=0;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u) ||
            word_num(input,"speed",&u) || word_num(input,"initialspeed",&u) || word_num(input,"velocity",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || label_num(input,"launchangle",&ang) ||
            word_num(input,"angle",&ang) || word_num(input,"theta",&ang);
    bool hH=label_num(input,"height",&h) || label_num(input,"h",&h) || label_num(input,"initialheight",&h) || label_num(input,"launchheight",&h) ||
            word_num(input,"height",&h) || word_num(input,"from",&h) || word_num(input,"above",&h) ||
            (!has(t, "horizontal") && prev_word_num(input,"above",&h)) || prev_word_num(input,"high",&h);
    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
    if (!hH && nv >= 3 && (has(t, "cliff") || has(t, "height") || has(t, "high"))) { h = v[2]; hH = true; }
    if (hU && hA && hH && !has(t, "findheight")) {
      if ((has(t, "belowhorizontal") || (has(t, "below") && has(t, "horizontal")) ||
           has(t, "downward") || has(t, "downwards")) && ang > 0) ang = -ang;
      sprintf(cmd, hG ? "projectileh(%.10g,%.10g,%.10g,%.10g)" : "projectileh(%.10g,%.10g,%.10g)", u, ang, h, hG ? g : 9.8);
      return eval_mech(cmd, out);
    }
    if (!hU && !hA && !hH) {
      sprintf(cmd, "projectileh(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_mech(cmd, out);
    }
  }
  if (is_projectile_text(t) && nv >= 2) {
    double u=0,ang=0,h0=0,g=0;
    bool hU=label_num(input,"speed",&u) || label_num(input,"u",&u) || label_num(input,"initialspeed",&u) || label_num(input,"initialvelocity",&u) ||
            word_num(input,"speed",&u) || word_num(input,"initialspeed",&u) || word_num(input,"velocity",&u);
    bool hA=label_num(input,"angle",&ang) || label_num(input,"theta",&ang) || label_num(input,"launchangle",&ang) ||
            word_num(input,"angle",&ang) || word_num(input,"theta",&ang) || prev_word_num(input,"degrees",&ang);
    bool hG=label_num(input,"g",&g) || label_num(input,"gravity",&g);
    bool hH=label_num(input,"height",&h0) || label_num(input,"initialheight",&h0) || label_num(input,"launchheight",&h0) ||
            word_num(input,"height",&h0) || (!has(t, "horizontal") && prev_word_num(input,"above",&h0)) || prev_word_num(input,"high",&h0);
    if (!hA && nv >= 2 && (has(t, "angle") || has(t, "degrees"))) { ang = v[1]; hA = true; }
    if (!hU && nv >= 1) { u = v[0]; hU = true; }
    if (!hH && nv >= 3 && (has(t, "cliff") || has(t, "height") || has(t, "high"))) { h0 = v[2]; hH = true; }
    if (hU && hA) {
      if ((has(t, "belowhorizontal") || (has(t, "below") && has(t, "horizontal")) ||
           has(t, "downward") || has(t, "downwards")) && ang > 0) ang = -ang;
      if (hH) {
        sprintf(cmd, hG ? "projectileh(%.10g,%.10g,%.10g,%.10g)" : "projectileh(%.10g,%.10g,%.10g)", u, ang, h0, hG ? g : 9.8);
        return eval_mech(cmd, out);
      }
      sprintf(cmd, hG ? "projectile(%.10g,%.10g,%.10g)" : "projectile(%.10g,%.10g)", u, ang, hG ? g : 9.8);
      return eval_mech(cmd, out);
    }
    sprintf(cmd, "projectile(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "equilibrium") || has(t, "balance")) && (has(t, "component") || has(t, "components")) && nv >= 4) {
    int p = sprintf(cmd, "equilibrium(%.10g", v[0]);
    for (int i=1;i<nv && p < (int)sizeof(cmd)-24;++i) p += sprintf(cmd+p, ",%.10g", v[i]);
    sprintf(cmd+p, ")");
    return eval_mech(cmd, out);
  }
  if ((has(t, "equilibrium") || has(t, "balance")) && has(t, "i") && has(t, "j")) {
    double xs[8], ys[8]; int vc = scan_ij_vectors(input, xs, ys, 8);
    if (vc >= 2) {
      double sx = 0, sy = 0;
      for (int i = 0; i < vc; ++i) { sx += xs[i]; sy += ys[i]; }
      int n = add(out, 0, "For equilibrium, the vector sum of forces is zero.");
      n = add(out, n, "sum of known forces = %.10g i %+.10g j", sx, sy);
      return add(out, n, "missing force = %.10g i %+.10g j", -sx, -sy);
    }
  }
  if ((has(t, "force") || has(t, "forces") || has(t, "vector") || has(t, "vectors")) &&
      (has(t, "resultant") || has(t, "magnitude")) && (has(t, "angle") || has(t, "degrees")) &&
      !has(t, "component") && !has(t, "equilibrium") && nv >= 6) {
    double sx = 0, sy = 0;
    int n = add(out, 0, "Resolve each force into horizontal and vertical components.");
    for (int i = 0; i + 1 < nv && n < P3_MAX_LINES - 3; i += 2) {
      double F = v[i], a = v[i + 1];
      double fx = F * deg_cosine(a), fy = F * deg_sine(a);
      sx += fx; sy += fy;
      n = add(out, n, "%.6g N at %.6g deg: Fx=%.10g, Fy=%.10g", F, a, fx, fy);
    }
    double R = root(sx*sx + sy*sy);
    n = add(out, n, "sum Fx = %.10g, sum Fy = %.10g", sx, sy);
    return add(out, n, "resultant = sqrt(Fx^2+Fy^2) = %.10g N", R);
  }
  if ((has(t, "force") || has(t, "forces") || has(t, "vector") || has(t, "vectors")) &&
      (has(t, "resultant") || has(t, "magnitude")) && (has(t, "angle") || has(t, "degrees")) &&
      !has(t, "component") && !has(t, "equilibrium") && nv >= 3) {
    if (nv >= 4) {
      double sx = 0, sy = 0;
      int n = add(out, 0, "Resolve each force into horizontal and vertical components.");
      for (int i = 0; i + 1 < nv && n < P3_MAX_LINES - 4; i += 2) {
        double F = v[i], a = v[i + 1];
        double fx = F * deg_cosine(a), fy = F * deg_sine(a);
        sx += fx; sy += fy;
        n = add(out, n, "%.6g N at %.6g deg: Fx=%.10g, Fy=%.10g", F, a, fx, fy);
      }
      double R = root(sx*sx + sy*sy), dir = deg_atan2_approx(sy, sx);
      n = add(out, n, "sum Fx = %.10g, sum Fy = %.10g", sx, sy);
      n = add(out, n, "resultant = sqrt(Fx^2+Fy^2) = %.10g N", R);
      return add(out, n, "direction = atan(Fy/Fx) = %.10g degrees", dir);
    }
    double F1 = v[0], F2 = v[1], ang = v[2];
    double R = root(F1*F1 + F2*F2 + 2*F1*F2*deg_cosine(ang));
    int n = add(out, 0, "Use the cosine rule with the included angle.");
    n = add(out, n, "R^2 = %.6g^2 + %.6g^2 + 2*%.6g*%.6g*cos(%.6g)", F1, F2, F1, F2, ang);
    return add(out, n, "R = %.10g N", R);
  }
  if ((has(t, "positionvector") || (has(t, "position") && has(t, "vector"))) &&
      has(t, "i") && has(t, "j") && has(c, "t") &&
      (has(t, "position") || has(t, "speed") || has(t, "velocity")) && nv >= 3) {
    double xs[4], ys[4], tt = 0;
    int vc = scan_ij_vectors(input, xs, ys, 4);
    if (vc >= 2 && (last_label_num(input, "t", &tt) || word_num_with_t(input, "at", &tt) || word_num(input, "time", &tt) || nv > 0)) {
      if (tt == 0 && nv > 0) tt = v[nv-1];
      double x = xs[0] + tt*xs[1], y = ys[0] + tt*ys[1];
      double sp = root(xs[1]*xs[1] + ys[1]*ys[1]);
      int n = add(out, 0, "Use the position vector r = r0 + t v.");
      n = add(out, n, "r0 = %.10g i %+.10g j, v = %.10g i %+.10g j", xs[0], ys[0], xs[1], ys[1]);
      n = add(out, n, "r(%.10g) = r0 + %.10g v", tt, tt);
      n = add(out, n, "position = %.10g i %+.10g j", x, y);
      return add(out, n, "speed = |v| = sqrt(%.10g^2 + %.10g^2) = %.10g", xs[1], ys[1], sp);
    }
  }
  if ((has(t, "force") || has(t, "vector")) && has(t, "i") && has(t, "j") &&
      (has(t, "magnitude") || has(t, "direction") || has(t, "resultant")) && nv >= 2) {
    sprintf(cmd, "vector(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "positionvector") || (has(t, "position") && has(t, "vector"))) &&
      (has(t, "velocity") || has(t, "acceleration")) && has(t, "i") && has(t, "j") && nv >= 5) {
    double ax = v[0], bx = v[1], ay = 1, by = v[3], tt = v[nv-1];
    const char *yp = strstr(c, "t^2");
    if (yp && yp > c && isdigit((unsigned char)yp[-1])) {
      const char *a = yp - 1;
      while (a > c && (isdigit((unsigned char)a[-1]) || a[-1] == '.')) --a;
      ay = read_num(a);
    }
    double vx = ax, vy = 2*ay*tt + by, axn = 0, ayn = 2*ay;
    int n = add(out, 0, "Differentiate the position vector component by component.");
    n = add(out, n, "r = (%.6g t %+.6g)i + (%.6g t^2 %+.6g t)j", ax, bx, ay, by);
    n = add(out, n, "v = dr/dt = %.6g i + (%.6g t %+.6g)j", vx, 2*ay, by);
    n = add(out, n, "a = dv/dt = %.6g i + %.6g j", axn, ayn);
    n = add(out, n, "at t=%.6g, v = %.10g i + %.10g j", tt, vx, vy);
    return add(out, n, "at t=%.6g, a = %.10g i + %.10g j", tt, axn, ayn);
  }
  if ((has(t, "equilibrium") || has(t, "balance")) && (has(t, "angle") || has(t, "bearing") || has(t, "degree")) &&
      !has(t, "ladder") && nv >= 4) {
    int p = sprintf(cmd, "equilpolar(%.10g", v[0]);
    for (int i=1;i<nv && p < (int)sizeof(cmd)-24;++i) p += sprintf(cmd+p, ",%.10g", v[i]);
    sprintf(cmd+p, ")");
    return eval_mech(cmd, out);
  }
  if ((has(t, "beam") || has(t, "support") || has(t, "reaction")) && (has(t, "load") || has(t, "weight")) &&
      !has(t, "ladder") && nv >= 3) {
    double pairW[8], pairX[8];
    int pairN = scan_load_at_pairs(input, pairW, pairX, 8);
    if (pairN > 0 && !has(c, "distancex") && !has(c, "findx")) {
      double L = 0, bw = 0, bm = 0;
      bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
      bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
      if (!hb && (word_num(input, "mass", &bm) || prev_word_num(input, "mass", &bm))) { bw = 9.8*bm; hb = true; }
      if (!hL) L = v[0];
      if (L > 0) {
        double sumW = hb ? bw : 0, moment = hb ? bw * L / 2.0 : 0;
        int n = add(out, 0, "For a horizontal beam in equilibrium, use moments and vertical forces.");
        n = add(out, n, "Take moments about the left support A.");
        if (hb) {
          n = add(out, n, "For a uniform rod/beam, include its weight at the midpoint.");
          n = add(out, n, "rod weight moment = %.10g*(%.10g/2)", bw, L);
        }
        char rhs[180]; rhs[0] = 0; int p = 0;
        if (hb) p += sprintf(rhs+p, "%.10g*(%.10g/2)", bw, L);
        for (int i = 0; i < pairN; ++i) {
          sumW += pairW[i]; moment += pairW[i] * pairX[i];
          if (p < (int)sizeof(rhs) - 36) p += sprintf(rhs+p, "%s%.10g*%.10g", p ? " + " : "", pairW[i], pairX[i]);
        }
        double RB = moment / L, RA = sumW - RB;
        n = add(out, n, "R_B*%.10g = %s", L, rhs);
        n = add(out, n, "R_B = %.10g N", RB);
        n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", sumW);
        return add(out, n, "R_A = %.10g N", RA);
      }
    }
    if ((has(t, "rod") || has(t, "uniform") || has(t, "beam")) &&
        has(t, "support") && (has(t, "leftend") || (has(t, "left") && has(t, "end"))) && nv >= 5) {
      double L = 0, bw = 0, a_pos = 0, b_pos = 0, load = 0;
      bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
      bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
      bool hl = word_num(input, "load", &load) || prev_word_num(input, "load", &load);
      if (!hL) L = v[0];
      if (!hb) bw = v[1];
      if (nv >= 4) { a_pos = v[2]; b_pos = v[3]; }
      if (!hl) load = v[nv - 1];
      if (L > 0 && bw > 0 && b_pos > a_pos) {
        double rod_arm = L / 2.0 - a_pos;
        double load_x = has(t, "rightend") ? L : 0;
        double load_arm = load_x - a_pos;
        double support_gap = b_pos - a_pos;
        double RB = (bw * rod_arm + load * load_arm) / support_gap;
        double RA = bw + load - RB;
        int n = add(out, 0, "For a horizontal rod in equilibrium, take moments about support A.");
        n = add(out, n, "A is %.10g m from the left end and B is %.10g m from the left end.", a_pos, b_pos);
        n = add(out, n, "rod weight acts at %.10g m from the left end, so its arm about A is %.10g m", L/2.0, rod_arm);
        n = add(out, n, "load arm about A = %.10g - %.10g = %.10g m", load_x, a_pos, load_arm);
        n = add(out, n, "R_B*(%.10g-%.10g) = %.10g*%.10g + %.10g*%.10g", b_pos, a_pos, bw, rod_arm, load, load_arm);
        n = add(out, n, "R_B = %.10g N", RB);
        n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", bw + load);
        return add(out, n, "R_A = %.10g N", RA);
      }
    }
    if ((has(t, "rod") || has(t, "beam") || has(t, "uniform")) &&
        has(t, "supported") && has(t, "end") && !has(t, "from") && nv >= 3) {
      double L=0, bw=0, load=0;
      bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
      bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
      bool hl = word_num(input, "load", &load) || prev_word_num(input, "load", &load);
      if (!hL) L = v[0];
      if (!hb) bw = v[1];
      if (!hl) load = v[2];
      double RB = bw / 2.0 + load;
      double RA = bw + load - RB;
      int n = add(out, 0, "For a uniform horizontal rod, its weight acts at the midpoint.");
      n = add(out, n, "Take moments about A.");
      n = add(out, n, "R_B*%.10g = %.10g*(%.10g/2) + %.10g*%.10g", L, bw, L, load, L);
      n = add(out, n, "R_B = %.10g N", RB);
      n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", bw + load);
      return add(out, n, "R_A = %.10g N", RA);
    }
    if ((has(t, "rod") || has(t, "uniform") || has(t, "beam")) && has(t, "reaction") &&
        (has(t, "distance") || has(c, "xfrom") || has(t, "findx")) && nv >= 5) {
      double L = v[0], bw = v[1], load = v[2], RB = v[nv-1];
      double x = load ? (RB*L - bw*(L/2.0)) / load : 0;
      int n = add(out, 0, "Take moments about A, including the uniform rod weight at its midpoint.");
      n = add(out, n, "R_B*L = W*(L/2) + load*x");
      n = add(out, n, "%.10g*%.10g = %.10g*(%.10g/2) + %.10g*x", RB, L, bw, L, load);
      return add(out, n, "x = %.10g m", x);
    }
    const char *loads_p = strstr(t, "loads");
    const char *act_p = loads_p ? strstr(loads_p, "act") : 0;
    const char *at_p = loads_p ? strstr(loads_p, "at") : 0;
    if (loads_p && at_p && strstr(loads_p, "n,at") && nv >= 5) {
      double L = 0, bw = 0, bm = 0;
      bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
      bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
      if (!hb && (word_num(input, "mass", &bm) || prev_word_num(input, "mass", &bm))) { bw = 9.8*bm; hb = true; }
      if (!hL) L = v[0];
      double sumW = hb ? bw : 0, moment = hb ? bw * L / 2.0 : 0;
      int n = add(out, 0, "For a horizontal beam in equilibrium, use moments and vertical forces.");
      n = add(out, n, "Take moments about the left support A.");
      char rhs[180]; rhs[0] = 0; int p = 0;
      if (hb) p += sprintf(rhs+p, "%.10g*(%.10g/2)", bw, L);
      for (int i = 1; i + 1 < nv; i += 2) {
        double W = v[i], x = abs_num(v[i+1]);
        sumW += W; moment += W*x;
        if (p < (int)sizeof(rhs) - 36) p += sprintf(rhs+p, "%s%.10g*%.10g", p ? " + " : "", W, x);
      }
      double RB = L ? moment/L : 0, RA = sumW - RB;
      n = add(out, n, "R_B*%.10g = %s", L, rhs);
      n = add(out, n, "R_B = %.10g N", RB);
      n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", sumW);
      return add(out, n, "R_A = %.10g N", RA);
    }
    if (loads_p && at_p && (!act_p || at_p < act_p)) {
      char rt[260]; raw_clean(input, rt, sizeof(rt));
      char *rl = strstr(rt, "loads,");
      char *ra = rl ? strstr(rl, ",at,") : 0;
      char *rf = ra ? strstr(ra + 4, ",from,") : 0;
      if (rl && ra) {
        char load_seg[160], pos_seg[160];
        int llen = (int)(ra - rl);
        if (llen >= (int)sizeof(load_seg)) llen = (int)sizeof(load_seg) - 1;
        memcpy(load_seg, rl, llen); load_seg[llen] = 0;
        const char *pend = rf ? rf : rt + strlen(rt);
        int plen = (int)(pend - (ra + 4));
        if (plen >= (int)sizeof(pos_seg)) plen = (int)sizeof(pos_seg) - 1;
        memcpy(pos_seg, ra + 4, plen); pos_seg[plen] = 0;
        double loads[8], pos[8];
        int nl = scan_nums(load_seg, loads, 8), np = scan_nums(pos_seg, pos, 8);
        double L = 0, bw = 0, bm = 0;
        bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
        bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
        if (!hb && (word_num(input, "mass", &bm) || prev_word_num(input, "mass", &bm))) { bw = 9.8*bm; hb = true; }
        if (hL && nl > 0 && np >= nl) {
          double sumW = hb ? bw : 0, moment = hb ? bw * L / 2.0 : 0;
          int n = add(out, 0, "For a horizontal beam in equilibrium, use moments and vertical forces.");
          n = add(out, n, "Take moments about the left support A.");
          char rhs[180]; rhs[0] = 0; int p = 0;
          if (hb) p += sprintf(rhs+p, "%.10g*(%.10g/2)", bw, L);
          for (int i = 0; i < nl && i < np; ++i) {
            double W = loads[i], x = abs_num(pos[i]);
            sumW += W; moment += W*x;
            if (p < (int)sizeof(rhs) - 36) p += sprintf(rhs+p, "%s%.10g*%.10g", p ? " + " : "", W, x);
          }
          double RB = L ? moment/L : 0, RA = sumW - RB;
          n = add(out, n, "R_B*%.10g = %s", L, rhs);
          n = add(out, n, "R_B = %.10g N", RB);
          n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", sumW);
          return add(out, n, "R_A = %.10g N", RA);
        }
      }
    }
    if (loads_p && act_p) {
      char load_seg[160], pos_seg[160];
      int llen = (int)(act_p - loads_p);
      if (llen >= (int)sizeof(load_seg)) llen = (int)sizeof(load_seg) - 1;
      memcpy(load_seg, loads_p, llen); load_seg[llen] = 0;
      strncpy(pos_seg, act_p, sizeof(pos_seg) - 1); pos_seg[sizeof(pos_seg) - 1] = 0;
      double loads[8], pos[8];
      int nl = scan_nums(load_seg, loads, 8), np = scan_nums(pos_seg, pos, 8);
      double L = 0, bw = 0, bm = 0;
      bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
      bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
      if (!hb && (has(t, "rod") || has(t, "uniform") || has(t, "beam")) &&
          (word_num(input, "mass", &bm) || prev_word_num(input, "mass", &bm))) {
        bw = bm * 9.8;
        hb = true;
      }
      if (hL && nl > 0 && np >= nl) {
        double sumW = hb ? bw : 0, moment = hb ? bw * L / 2.0 : 0;
        int n = add(out, 0, "For a horizontal beam in equilibrium, use moments and vertical forces.");
        n = add(out, n, "Take moments about the left support A.");
        if (hb) n = add(out, n, "beam weight moment = %.10g*(%.10g/2)", bw, L);
        char rhs[160]; rhs[0] = 0; int p = 0;
        if (hb) p += sprintf(rhs+p, "%.10g*(%.10g/2)", bw, L);
        for (int i = 0; i < nl && i < np; ++i) {
          double W = loads[i], x = abs_num(pos[i]);
          sumW += W; moment += W*x;
          if (p < (int)sizeof(rhs) - 36) p += sprintf(rhs+p, "%s%.10g*%.10g", p ? " + " : "", W, x);
        }
        double RB = L ? moment / L : 0, RA = sumW - RB;
        n = add(out, n, "R_B*%.10g = %s", L, rhs);
        n = add(out, n, "R_B = %.10g N", RB);
        n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", sumW);
        return add(out, n, "R_A = %.10g N", RA);
      }
    }
    if ((has(t, "rod") || has(t, "uniform")) && has(t, "force") && nv >= 6) {
      double L = 0, bw = 0;
      bool hL = label_num(input, "length", &L) || word_num(input, "length", &L);
      bool hb = word_num(input, "weight", &bw) || prev_word_num(input, "weight", &bw);
      if (!hL) L = v[0];
      if (!hb) bw = v[1];
      double sumW = bw, moment = bw * L / 2.0;
      int n = add(out, 0, "For a uniform rod/beam in equilibrium, include its weight at the midpoint.");
      n = add(out, n, "Take moments about the left support A.");
      n = add(out, n, "rod weight moment = %.10g*(%.10g/2)", bw, L);
      for (int i = 2; i + 1 < nv; i += 2) {
        double W = v[i], x = v[i + 1];
        sumW += W; moment += W * x;
        n = add(out, n, "load moment = %.10g*%.10g", W, x);
      }
      double RB = L ? moment / L : 0, RA = sumW - RB;
      n = add(out, n, "R_B*%.10g = %.10g", L, moment);
      n = add(out, n, "R_B = %.10g N", RB);
      n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", sumW);
      return add(out, n, "R_A = %.10g N", RA);
    }
    if (nv >= 5 && !has(t, "uniform")) {
      double L = v[0], sumW = 0, moment = 0;
      int n = add(out, 0, "For a horizontal beam in equilibrium, use moments and vertical forces.");
      n = add(out, n, "Take moments about the left support A.");
      char rhs[160]; rhs[0] = 0; int p = 0;
      for (int i = 1; i + 1 < nv; i += 2) {
        double W = v[i], x = v[i+1];
        sumW += W; moment += W*x;
        if (p < (int)sizeof(rhs) - 32) p += sprintf(rhs+p, "%s%.10g*%.10g", i == 1 ? "" : " + ", W, x);
      }
      double RB = L ? moment/L : 0, RA = sumW - RB;
      n = add(out, n, "R_B*%.10g = %s", L, rhs);
      n = add(out, n, "R_B = %.10g N", RB);
      n = add(out, n, "Vertical equilibrium: R_A + R_B = %.10g", sumW);
      return add(out, n, "R_A = %.10g N", RA);
    }
    double L=0,W=0,x=0,bw=0;
    bool hL=label_num(input,"length",&L) || word_num(input,"length",&L);
    bool hW=label_num(input,"load",&W) || word_num(input,"load",&W) || prev_word_num(input,"load",&W);
    bool hx=label_num(input,"distance",&x) || word_num(input,"from",&x) || prev_word_num(input,"from",&x);
    bool hb=label_num(input,"beamweight",&bw) || label_num(input,"bw",&bw) ||
            ((has(t, "uniform") || has(t, "rod") || has(t, "beam")) && (word_num(input,"weight",&bw) || prev_word_num(input,"weight",&bw)));
    if (!hb && (has(t, "uniform") || has(t, "rod") || has(t, "beam"))) {
      double bm = 0;
      if (word_num(input, "mass", &bm) || prev_word_num(input, "mass", &bm)) {
        bw = bm * 9.8;
        hb = true;
      }
    }
    if (hL && hW && hx) sprintf(cmd, "beam(%.10g,%.10g,%.10g,%.10g)", L, W, x, hb ? bw : 0);
    else sprintf(cmd, nv > 3 ? "beam(%.10g,%.10g,%.10g,%.10g)" : "beam(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv > 3 ? v[3] : 0);
    return eval_mech(cmd, out);
  }
  if (has(t, "ladder") && (has(t, "wall") || has(t, "floor") || has(t, "rough") || has(t, "smooth")) &&
      (has(t, "person") || has(t, "man") || has(t, "load")) && nv >= 5) {
    double L=0, W=0, P=0, d=0, ang=0;
    bool hL=word_num(input,"length",&L) || label_num(input,"length",&L);
    bool hW=word_num(input,"weighs",&W) || word_num(input,"weight",&W) || label_num(input,"weight",&W);
    bool hA=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hL) L = v[0];
    if (!hA) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], L) && v[i] > 0 && v[i] <= 90) { ang = v[i]; hA = true; break; }
    if (!hW) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], L) && !near_num(v[i], ang) && v[i] > 10) { W = v[i]; hW = true; break; }
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], L) || near_num(v[i], W) || near_num(v[i], ang)) continue;
      if (v[i] > 10 && (!P || v[i] > P)) P = v[i];
    }
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], L) || near_num(v[i], W) || near_num(v[i], P) || near_num(v[i], ang)) continue;
      if (v[i] > 0 && v[i] <= L) { d = v[i]; break; }
    }
    if (P > 0 && d > 0 && L > 0) {
      double R = W + P;
      double S = (W*(L/2.0)*deg_cosine(ang) + P*d*deg_cosine(ang)) / (L*deg_sine(ang));
      double mu = R ? S/R : 0;
      int n = add(out, 0, "For a ladder at a smooth wall and rough ground, take moments about the foot.");
      n = add(out, n, "Vertical equilibrium: R = %.10g + %.10g = %.10g N", W, P, R);
      n = add(out, n, "Horizontal equilibrium: friction F = wall reaction S.");
      n = add(out, n, "S*(%.6g sin %.6g) = %.10g*(%.6g/2 cos %.6g) + %.10g*(%.6g cos %.6g)", L, ang, W, L, ang, P, d, ang);
      n = add(out, n, "S = %.10g N, so friction = %.10g N", S, S);
      return add(out, n, "If limiting, mu = F/R = %.10g/%.10g = %.10g", S, R, mu);
    }
  }
  if (has(t, "ladder") && (has(t, "wall") || has(t, "floor") || has(t, "rough") || has(t, "smooth")) && nv >= 2) {
    double L=0,W=0,ang=0,P=0,d=0;
    bool hL=label_num(input,"length",&L) || word_num(input,"length",&L);
    bool hW=label_num(input,"weight",&W) || word_num(input,"weight",&W);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    bool hP=label_num(input,"load",&P) || label_num(input,"person",&P);
    bool hd=label_num(input,"distance",&d);
    if (has(c, "roughwall") && (has(c, "roughfloor") || has(c, "floorrough"))) {
      if (!hL && nv > 0) L = v[0];
      if (!hW && nv > 1) W = v[1];
      if (!ha && nv > 2) ang = v[2];
      int n = add(out, 0, "Rough wall and rough floor: include friction at both contacts.");
      n = add(out, n, "At the floor: normal R, friction F. At the wall: normal S, friction G.");
      n = add(out, n, "Vertical equilibrium: R + G = W.");
      n = add(out, n, "Horizontal equilibrium: F = S.");
      n = add(out, n, "Moments about the foot: S*L sin(theta) + G*L cos(theta) = W*(L/2)cos(theta).");
      if (has(t, "coefficient") || has(t, "mu")) n = add(out, n, "Use limiting friction only at contacts stated to be limiting, e.g. F=mu R.");
      return add(out, n, "A second contact condition is needed to find a unique wall friction.");
    }
    if (!ha && (has(t, "limiting") || has(t, "minimum")) && (has(t, "coefficient") || has(t, "mu")) && nv >= 2) {
      double mu = v[1], theta = arctan(1.0/(2.0*mu))*180.0/M_PI;
      int n = add(out, 0, "Ladder in limiting equilibrium: smooth wall, rough ground.");
      n = add(out, n, "For a uniform ladder with no extra load, mu = 1/(2 tan theta).");
      n = add(out, n, "tan theta = 1/(2mu) = 1/(2*%.6g)", mu);
      return add(out, n, "angle theta = %.10g degrees", theta);
    }
    if ((has(t, "whether") || has(t, "check") || has(t, "equilibrium")) &&
        (has(t, "coefficient") || has(t, "mu")) && nv >= 4) {
      if (!hL) L = v[0];
      if (!hW) W = v[1];
      if (!ha) ang = v[2];
      double mu = 0;
      if (!label_num(input, "mu", &mu) && !label_num(input, "coefficient", &mu)) {
        for (int i = 0; i < nv; ++i) if (!near_num(v[i], L) && !near_num(v[i], W) && !near_num(v[i], ang) && v[i] > 0 && v[i] < 1) { mu = v[i]; break; }
      }
      double S = W * deg_cosine(ang) / (2.0 * deg_sine(ang));
      double maxF = mu * W;
      int n = add(out, 0, "For a ladder at a smooth wall and rough ground, take moments about the foot.");
      n = add(out, n, "S*L sin(theta) = W*(L/2) cos(theta)");
      n = add(out, n, "wall reaction S = W/(2 tan(theta)) = %.10g N", S);
      n = add(out, n, "Required floor friction F = S = %.10g N", S);
      n = add(out, n, "Maximum friction = mu R = %.6g*%.6g = %.10g N", mu, W, maxF);
      return add(out, n, maxF + 1e-9 >= S ? "Since maximum friction is enough, the ladder can be in equilibrium." : "Since maximum friction is too small, the ladder cannot be in equilibrium.");
    }
    if (nv < 3) return 0;
    if (hL && hW && ha) sprintf(cmd, "ladder(%.10g,%.10g,%.10g,%.10g,%.10g)", L, W, ang, hP ? P : 0, hd ? d : L/2);
    else sprintf(cmd, nv > 4 ? "ladder(%.10g,%.10g,%.10g,%.10g,%.10g)" : "ladder(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv > 3 ? v[3] : 0, nv > 4 ? v[4] : v[0]/2);
    return eval_mech(cmd, out);
  }
  if ((has(t, "banked") || has(t, "banking")) && (has(t, "curve") || has(t, "bend") || has(t, "round")) &&
      (has(t, "nofriction") || (has(t, "no") && has(t, "friction")) || has(t, "smooth")) && nv >= 2) {
    double u=0, r=0;
    bool hu=word_num(input,"speed",&u) || word_num(input,"velocity",&u);
    bool hr=word_num(input,"radius",&r) || label_num(input,"radius",&r);
    if (!hu) u = v[0];
    if (!hr) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], u) && v[i] > 0) { r = v[i]; break; }
    }
    double tanth = r ? u*u/(9.8*r) : 0;
    double theta = arctan(tanth) * 180.0 / M_PI;
    int n = add(out, 0, "For a smooth banked curve, resolve R to give centripetal force.");
    n = add(out, n, "tan(theta) = v^2/(rg)");
    n = add(out, n, "tan(theta) = %.10g^2/(%.10g*9.8) = %.10g", u, r, tanth);
    return add(out, n, "theta = %.10g degrees", theta);
  }
  if ((has(t, "circular") || has(t, "circle") || has(t, "centripetal") || has(t, "bend")) &&
      (has(t, "angularspeed") || has(t, "omega") || has(t, "rads") || has(c, "rad/s")) && nv >= 2) {
    double r=0,w=0;
    bool hr=word_num(input,"radius",&r) || label_num(input,"radius",&r);
    bool hw=word_num(input,"angularspeed",&w) || label_num(input,"omega",&w);
    if (!hr) r = v[0];
    if (!hw) w = v[1];
    int n = add(out, 0, "For circular motion, v = r*omega and a = r*omega^2.");
    n = add(out, n, "v = %.6g*%.6g = %.10g m/s", r, w, r*w);
    return add(out, n, "centripetal acceleration = %.6g*%.6g^2 = %.10g m/s^2", r, w, r*w*w);
  }
  if ((has(t, "shm") || has(t, "simpleharmonic") || (has(t, "simple") && has(t, "harmonic"))) &&
      (has(t, "amplitude") || has(t, "angularspeed") || has(t, "omega") || has(t, "maximum")) && nv >= 2) {
    double A=0,w=0,T=0;
    bool hA=word_num(input,"amplitude",&A) || label_num(input,"amplitude",&A) || label_num(input,"a",&A);
    bool hw=word_num(input,"angularspeed",&w) || label_num(input,"omega",&w);
    bool hT=word_num(input,"period",&T) || label_num(input,"period",&T);
    if (!hA) A = v[0];
    if (hT && !hw) w = T ? 2.0*M_PI/T : 0;
    if (!hw) {
      if (!hT) for (int i = 0; i < nv; ++i) if (!near_num(v[i], A)) { w = v[i]; break; }
    }
    int n = add(out, 0, "For SHM, maximum speed is A*omega and maximum acceleration is A*omega^2.");
    if (hT) n = add(out, n, "omega = 2*pi/T = 2*pi/%.10g = %.10g", T, w);
    n = add(out, n, "maximum speed = %.10g*%.10g = %.10g", A, w, A*w);
    return add(out, n, "maximum acceleration = %.10g*%.10g^2 = %.10g", A, w, A*w*w);
  }
  if ((has(t, "conical") || (has(t, "pendulum") && has(t, "circle"))) &&
      (has(t, "length") || has(t, "angle")) && (has(t, "speed") || has(t, "velocity")) && nv >= 2) {
    double L=0, ang=0;
    bool hL=word_num(input,"length",&L) || label_num(input,"length",&L);
    bool ha=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hL) L = v[0];
    if (!ha) for (int i = 0; i < nv; ++i) if (!near_num(v[i], L)) { ang = v[i]; break; }
    double r = L * deg_sine(ang);
    double sp = root(r * 9.8 * deg_sine(ang) / deg_cosine(ang));
    int n = add(out, 0, "For a conical pendulum, resolve tension vertically and horizontally.");
    n = add(out, n, "T cos(theta)=mg and T sin(theta)=mv^2/r");
    n = add(out, n, "so tan(theta)=v^2/(rg), with r=L sin(theta)");
    n = add(out, n, "r = %.10g sin(%.10g) = %.10g m", L, ang, r);
    return add(out, n, "v = sqrt(rg tan(theta)) = %.10g m/s", sp);
  }
  if ((has(t, "unbanked") || has(t, "curve") || has(t, "bend")) &&
      (has(t, "coefficient") || has(t, "friction") || has(t, "mu")) &&
      (has(t, "maximumspeed") || has(t, "maxspeed") || (has(t, "maximum") && has(t, "speed"))) && nv >= 2) {
    double r=0, mu=0;
    bool hr=word_num(input,"radius",&r) || label_num(input,"radius",&r);
    bool hmu=word_num(input,"coefficient",&mu) || label_num(input,"coefficient",&mu) || word_num(input,"mu",&mu);
    if (!hr) r = v[0];
    if (!hmu) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], r) && v[i] > 0 && v[i] < 1.5) { mu = v[i]; break; }
    }
    double vmax = root(mu * 9.8 * r);
    int n = add(out, 0, "On an unbanked curve, friction provides the centripetal force.");
    n = add(out, n, "mu R = mv^2/r and R=mg");
    n = add(out, n, "v^2 = mu*g*r = %.6g*9.8*%.6g", mu, r);
    return add(out, n, "maximum speed = %.10g m/s", vmax);
  }
  if ((has(t, "circular") || has(t, "circle") || has(t, "centripetal") || has(t, "bend")) &&
      (has(t, "acceleration") || has(t, "accelerate")) &&
      (has(t, "radius") || has(t, "speed") || has(t, "velocity") || has(t, "m,s")) && nv >= 2) {
    double r=0,u=0;
    bool hr=word_num(input,"radius",&r) || label_num(input,"radius",&r);
    bool hu=word_num(input,"speed",&u) || label_num(input,"speed",&u) || word_num(input,"velocity",&u) || num_before_unit(input,"m/s",&u);
    if (!hr) r = v[0];
    if (!hu) for (int i = 0; i < nv; ++i) if (!near_num(v[i], r) && v[i] > 0) { u = v[i]; break; }
    int n = add(out, 0, "For motion in a circle, centripetal acceleration is v^2/r.");
    n = add(out, n, "a = v^2/r");
    return add(out, n, "a = %.10g^2/%.10g = %.10g m/s^2", u, r, r ? u*u/r : 0);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) &&
      (has(t, "constant") && has(t, "speed")) &&
      (has(t, "power") || has(t, "engine")) &&
      (has(c, "findpower") || has(c, "findenginepower") || (has(t, "find") && has(t, "power") && !has(t, "angle"))) &&
      (has(t, "resistance") || has(t, "resistive")) && nv >= 4) {
    double m=0, ang=0, r=0, spd=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m) || num_before_unit(input,"kg",&m);
    bool ha=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) ||
            prev_word_num(input,"degree",&ang) || prev_word_num(input,"degrees",&ang);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    bool hs=num_before_unit(input,"m/s",&spd) || num_before_unit(input,"ms^-1",&spd) ||
            word_num(input,"speed",&spd) || word_num(input,"velocity",&spd);
    if (!hm) m = v[0];
    if (!ha) for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && v[i] > 0 && v[i] <= 90) { ang = v[i]; break; }
    if (!hr) for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], spd)) { r = v[i]; break; }
    if (!hs) for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], r)) { spd = v[i]; break; }
    double weight = m*9.8*deg_sine(ang);
    double drive = weight + r;
    double P = drive * spd;
    int n = add(out, 0, "At constant speed uphill, driving force balances resistance and mg sin(theta).");
    n = add(out, n, "Driving force = mg sin(theta) + resistance");
    n = add(out, n, "Driving force = %.6g*9.8 sin(%.6g) + %.6g = %.10g N", m, ang, r, drive);
    n = add(out, n, "Driving force = %.10g N", drive);
    return add(out, n, "Power = Fv = %.10g*%.10g = %.10g W", drive, spd, P);
  }
  if ((has(t, "hill") || has(t, "hump") || has(t, "top")) &&
      (has(t, "radius") || has(t, "speed") || has(t, "m/s") || has(t, "power") || has(t, "kw")) && nv >= 3) {
    if (has(t, "power") || has(t, "kw") || has(t, "kilowatt")) {
      double m=0, P=0, spd=0, res=0;
      bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
      bool hP=word_num(input,"power",&P) || label_num(input,"power",&P) || prev_word_num(input,"kw",&P) || prev_word_num(input,"kilowatt",&P);
      bool hs=word_num(input,"speed",&spd) || word_num(input,"velocity",&spd) || prev_word_num(input,"m/s",&spd);
      bool hR=word_num(input,"resistance",&res) || label_num(input,"resistance",&res);
      if (!hm) m = v[0];
      if (!hP) P = v[1];
      if (has(t, "kw") || has(t, "kilowatt")) P *= 1000.0;
      if (!hs) for (int i = 0; i < nv; ++i)
        if (!near_num(v[i], m) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P) && !near_num(v[i], res)) { spd = v[i]; break; }
      if (!hR) for (int i = nv - 1; i >= 0; --i)
        if (!near_num(v[i], m) && !near_num(v[i], spd) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P)) { res = v[i]; break; }
      double drive = spd ? P / spd : 0;
      double s = (drive - res) / (m * 9.8);
      int n = add(out, 0, "At constant speed uphill, driving force balances resistance and mg sin(theta).");
      n = add(out, n, "driving force = P/v = %.10g/%.10g = %.10g N", P, spd, drive);
      n = add(out, n, "sin(theta) = (%.10g-%.10g)/(%.10g*9.8) = %.10g", drive, res, m, s);
      return add(out, n, "theta = %.10g degrees", deg_arcsine(s));
    }
    double m=0,r=0,u=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hr=word_num(input,"radius",&r) || label_num(input,"radius",&r);
    bool hu=word_num(input,"speed",&u) || label_num(input,"speed",&u) || word_num(input,"velocity",&u);
    if (!hm) m = v[0];
    if (!hr) r = v[1];
    if (!hu) u = v[2];
    double R = m*9.8 - (r ? m*u*u/r : 0);
    int n = add(out, 0, "At the top of a circular hill, resultant force towards the centre is mg - R.");
    n = add(out, n, "mg - R = mv^2/r");
    n = add(out, n, "R = mg - mv^2/r");
    return add(out, n, "R = %.6g*9.8 - %.6g*%.6g^2/%.6g = %.10g N", m, m, u, r, R);
  }
  if ((has(t, "circular") || has(t, "circle") || has(t, "centripetal") || has(t, "bend")) &&
      (has(t, "radius") || has(t, "speed") || has(t, "force")) && nv >= 3) {
    double m=0, r=0, u=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hr=word_num(input,"radius",&r) || label_num(input,"radius",&r);
    bool hu=word_num(input,"speed",&u) || label_num(input,"speed",&u) || word_num(input,"velocity",&u);
    if (!hm) m = v[0];
    if (!hr) r = v[1];
    if (!hu) u = v[2];
    double F = r ? m*u*u/r : 0;
    int n = add(out, 0, "For motion in a circle, centripetal force is mv^2/r.");
    n = add(out, n, "F = %.6g*%.6g^2/%.6g", m, u, r);
    return add(out, n, "centripetal force = %.10g N", F);
  }
  if ((has(t, "connected") || (has(t, "two") && has(t, "particles"))) &&
      (has(t, "smooth") || has(t, "table") || has(t, "horizontal")) &&
      (has(t, "force") || has(t, "pull") || has(t, "tension") || has(t, "acceleration")) &&
      !has(t, "pulley") && !has(t, "incline") && !has(t, "inclined") && !has(t, "plane") &&
      !has(t, "rough") && !has(t, "friction") && !has(t, "coefficient") && nv >= 3) {
    double m1 = v[0], m2 = v[1], F = v[nv-1];
    double q = 0;
    if (word_num(input, "force", &q) || word_num(input, "pull", &q) || num_before_unit(input, "n", &q)) F = q;
    double ares = F / (m1 + m2);
    double T = m1 * ares;
    int n = add(out, 0, "Connected particles on a smooth table have the same acceleration.");
    n = add(out, n, "Treat both particles as one system: F = (m1+m2)a.");
    n = add(out, n, "a = %.10g/(%.10g+%.10g) = %.10g m/s^2", F, m1, m2, ares);
    n = add(out, n, "Use one particle to find the string tension.");
    return add(out, n, "T = m1*a = %.10g*%.10g = %.10g N", m1, ares, T);
  }
  if ((has(t, "elastic") || has(t, "modulus") || has(t, "extension") || has(t, "naturallength") ||
       (has(t, "string") && has(t, "tension"))) &&
      (has(t, "modulus") || has(t, "extension") || has(t, "tension") || has(t, "naturallength") || has(t, "lambda")) && nv >= 3) {
    double l=0, lam=0, x=0;
    bool hl=word_num(input,"naturallength",&l) || word_num(input,"length",&l) || label_num(input,"length",&l);
    bool hla=word_num(input,"modulus",&lam) || label_num(input,"modulus",&lam) || word_num(input,"lambda",&lam);
    bool hx=word_num(input,"extension",&x) || label_num(input,"extension",&x);
    if ((has(t, "hang") || has(t, "hanging") || has(t, "equilibrium")) && !hx && nv >= 3) {
      double m = 0;
      bool hm = word_num(input, "mass", &m) || label_num(input, "mass", &m);
      if (!hl) l = v[0];
      if (!hla) lam = v[1];
      if (!hm) m = v[2];
      double T = m * 9.8;
      x = lam ? T * l / lam : 0;
      int n = add(out, 0, "In equilibrium, string tension balances weight.");
      n = add(out, n, "T = mg = %.10g*9.8 = %.10g N", m, T);
      n = add(out, n, "Hooke's law: T = lambda*x/l");
      return add(out, n, "x = Tl/lambda = %.10g*%.10g/%.10g = %.10g m", T, l, lam, x);
    }
    if (!hl) l = v[1];
    if (!hla) lam = v[2];
    if (!hx) x = v[nv-1];
    double T = l ? lam*x/l : 0;
    int n = add(out, 0, "For an elastic string, Hooke's law gives T = lambda*x/l.");
    n = add(out, n, "lambda = %.10g, extension = %.10g, natural length = %.10g", lam, x, l);
    return add(out, n, "T = %.10g*%.10g/%.10g = %.10g N", lam, x, l, T);
  }
  if (has(t, "force") && !has(t, "plane") && !has(t, "rough") && !has(t, "acceleration") &&
      (has(t, "horizontal") || has(t, "vertical") || has(t, "component")) && nv >= 2) {
    sprintf(cmd, "resolve(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "rough") || has(t, "friction")) && (has(t, "plane") || has(t, "inclined") || has(t, "incline")) &&
      (has(t, "equilibrium") || has(t, "held")) && (has(t, "reaction") || has(t, "normal") || has(t, "friction")) && nv >= 3) {
    double m=0, F=0, ang=0;
    bool hm = label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool hF = label_num(input,"force",&F) || word_num(input,"force",&F);
    bool hA = label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hm) m = v[0];
    if (!hF) F = nv > 2 ? v[2] : 0;
    if (!hA) ang = nv > 1 ? v[1] : 0;
    double down = m*9.8*deg_sine(ang), R = m*9.8*deg_cosine(ang), fr = down - F;
    int n = add(out, 0, "Resolve parallel and perpendicular to the inclined plane.");
    n = add(out, n, "Normal reaction: R = mg cos(theta) = %.6g*9.8 cos(%.6g) = %.10g N", m, ang, R);
    n = add(out, n, "Down-slope component: mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
    n = add(out, n, "Equilibrium along the plane: force + friction = mg sin(theta).");
    if (fr >= 0) return add(out, n, "friction = %.10g - %.10g = %.10g N up the plane", down, F, fr);
    return add(out, n, "friction = %.10g - %.10g = %.10g N down the plane", F, down, -fr);
  }
  if ((has(t, "rough") || has(t, "friction")) && (has(t, "plane") || has(t, "inclined") || has(t, "incline")) &&
      (has(t, "coefficient") || has(t, "mu")) && (has(t, "equilibrium") || has(t, "rest") || has(t, "rests") || has(t, "limiting") || has(t, "least")) &&
      !has(t, "force") && nv >= 1) {
    double m=0, ang=0;
    bool hm = label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool hA = label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hm) {
      if (!hA) ang = v[0];
      double mu = deg_cosine(ang) == 0 ? 0 : deg_sine(ang) / deg_cosine(ang);
      int n = add(out, 0, "For limiting equilibrium on a rough inclined plane, friction = mu R.");
      n = add(out, n, "Resolve perpendicular: R = mg cos(theta)");
      n = add(out, n, "Resolve parallel: friction = mg sin(theta)");
      n = add(out, n, "mu mg cos(theta) = mg sin(theta)");
      return add(out, n, "least mu = tan(theta) = tan(%.6g) = %.10g", ang, mu);
    }
    if (!hm) m = v[0];
    if (!hA) ang = nv > 1 ? v[1] : v[0];
    double down = m*9.8*deg_sine(ang), R = m*9.8*deg_cosine(ang), mu = R == 0 ? 0 : down / R;
    int n = add(out, 0, "For limiting equilibrium on a rough inclined plane, friction = mu R.");
    n = add(out, n, "Resolve perpendicular: R = mg cos(theta) = %.6g*9.8 cos(%.6g) = %.10g N", m, ang, R);
    n = add(out, n, "Resolve parallel: friction = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
    n = add(out, n, "mu R = mg sin(theta)");
    return add(out, n, "least mu = tan(theta) = tan(%.6g) = %.10g", ang, mu);
  }
  if ((has(t, "rough") || has(t, "friction")) && (has(t, "plane") || has(t, "inclined") || has(t, "incline")) &&
      (has(t, "coefficient") || has(t, "mu")) && has(t, "force") &&
      (has(t, "least") || has(t, "minimum") || has(t, "smallest")) &&
      (has(t, "move") || has(t, "motion") || has(t, "pointofmoving")) && nv >= 3) {
    double m=0, ang=0, mu=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m) || label_num(input,"m",&m);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || label_num(input,"theta",&ang) || prev_word_num(input,"degrees",&ang);
    bool hmu=label_num(input,"mu",&mu) || label_num(input,"coefficient",&mu) || word_num(input,"coefficient",&mu);
    if (!hm) m = v[0];
    if (!hmu) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1.5) { mu = v[i]; hmu = true; break; }
    if (!ha) {
      for (int i = 0; i < nv; ++i)
        if (!near_num(v[i], m) && !near_num(v[i], mu) && v[i] > 1.5 && v[i] <= 90) { ang = v[i]; break; }
    }
    double down = m*9.8*deg_sine(ang), R = m*9.8*deg_cosine(ang), fr = mu*R;
    bool up = has(t, "up") || has(t, "upwards") || has(t, "up,plane") || !has(t, "down");
    double F = up ? down + fr : down - fr;
    int n = add(out, 0, "At limiting equilibrium, friction is at its maximum: F_f = mu R.");
    n = add(out, n, "Resolve perpendicular: R = mg cos(theta) = %.6g*9.8 cos(%.6g) = %.10g N", m, ang, R);
    n = add(out, n, "friction = mu R = %.6g*%.10g = %.10g N", mu, R, fr);
    n = add(out, n, "down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
    if (up) n = add(out, n, "For impending upward motion, friction acts down the plane.");
    else n = add(out, n, "For impending downward motion, friction acts up the plane.");
    return add(out, n, up ? "least force up the plane = %.10g + %.10g = %.10g N" :
               "least force down the plane = %.10g - %.10g = %.10g N", down, fr, F);
  }
	  if ((has(t, "pulley") || ((has(t, "connected") || has(t, "connectedparticles")) &&
                              (has(t, "hang") || has(t, "hanging") || has(t, "freely")))) &&
	      (has(t, "rough") || has(t, "friction") || has(t, "coefficient")) &&
	      (has(t, "horizontal") || has(t, "table")) &&
	      nv >= 3) {
	    double mu = 0, masses[8]; int mc = 0;
	    for (int i = 0; i < nv; ++i) {
	      if (v[i] > 0 && v[i] < 1) { mu = v[i]; continue; }
	      if (v[i] > 1 && mc < 8) masses[mc++] = v[i];
	    }
	    double m1 = mc > 0 ? masses[0] : v[0];
	    double m2 = mc > 1 ? masses[mc-1] : v[1];
	    if (!(word_num(input, "hanging", &m2) || word_num(input, "hangs", &m2) ||
	          prev_word_num(input, "hanging", &m2) || prev_word_num(input, "hangs", &m2) ||
	          prev_word_num(input, "hang", &m2) || prev_word_num(input, "freely", &m2))) {
	      for (int i = 1; i < mc; ++i) if (masses[i] > m2) m2 = masses[i];
	    }
	    double fr = mu * m1 * 9.8;
    double drive = m2 * 9.8;
    double acc = (drive - fr) / (m1 + m2);
    double T = m1 * acc + fr;
    int n = add(out, 0, "Treat both particles as one system, including friction on the table.");
    n = add(out, n, "friction on table particle = mu R = %.6g*%.6g*9.8 = %.10g N", mu, m1, fr);
    n = add(out, n, "driving force = hanging weight = %.6g*9.8 = %.10g N", m2, drive);
    n = add(out, n, "a = (%.10g-%.10g)/(%.6g+%.6g) = %.10g m/s^2", drive, fr, m1, m2, acc);
    n = add(out, n, "For table particle: T - friction = m a.");
    return add(out, n, "T = %.6g*%.10g + %.10g = %.10g N", m1, acc, fr, T);
  }
  if (has(t, "horizontal") && !has(t, "inclined") && !has(t, "incline") && !has(t, "slope") &&
      (has(t, "plane") || has(t, "rough") || has(t, "smooth") || has(t, "pulled") || has(t, "pull")) &&
      (has(t, "acceleration") || has(t, "accelerate")) && nv >= 2) {
    if ((has(t, "connected") || has(t, "two") || has(t, "particles")) && (has(t, "coefficient") || has(t, "mu")) && nv >= 4) {
      double m1 = v[0], m2 = v[1], mu = 0, F = v[nv-1];
      for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { mu = v[i]; break; }
      double friction = mu * m1 * 9.8;
      double ares = (F - friction) / (m1 + m2);
      double T = m2 * ares;
      int n = add(out, 0, "Treat the connected particles as one system, including friction on the rough plane.");
      n = add(out, n, "friction = mu R = %.6g*%.6g*9.8 = %.10g N", mu, m1, friction);
      n = add(out, n, "F - friction = (m1+m2)a");
      n = add(out, n, "a = (%.6g - %.10g)/(%.6g+%.6g) = %.10g m/s^2", F, friction, m1, m2, ares);
      return add(out, n, "For the second particle, T = m2*a = %.6g*%.10g = %.10g N", m2, ares, T);
    }
    double m = 0, F = 0, mu = 0, ang = 0;
    bool hm = label_num(input,"mass",&m) || word_num(input,"mass",&m) || label_num(input,"m",&m);
    bool hF = label_num(input,"force",&F) || word_num(input,"force",&F) || label_num(input,"pull",&F) ||
              word_num(input,"pull",&F) || first_num_after_word(input,"pulled",&F) ||
              first_num_after_word(input,"pull",&F);
    if (!hF && has(t, "pulled,by,")) {
      const char *pb = strstr(t, "pulled,by,");
      F = read_num(pb + 10);
      hF = true;
    }
    if (!hF && (has(t, "pulled") || has(t, "pull"))) {
      const char *by = strstr(t, ",by,");
      while (by && !hF) {
        double q = read_num(by + 4);
        if (q > 1) { F = q; hF = true; break; }
        by = strstr(by + 4, ",by,");
      }
    }
    bool hmu = label_num(input,"mu",&mu) || label_num(input,"coefficient",&mu) || word_num(input,"coefficient",&mu);
    bool hAng = label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || word_num(input,"degrees",&ang);
    if (!hm) m = v[0];
    if (!hF) F = v[nv-1];
    if (!hmu) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1 && !near_num(v[i], m)) { mu = v[i]; hmu = true; break; }
    if (!hAng && has(t, "degree")) {
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], m) || near_num(v[i], F) || (hmu && near_num(v[i], mu))) continue;
        if (v[i] > 0 && v[i] <= 90) { ang = v[i]; hAng = true; break; }
      }
    }
    int n = add(out, 0, has(t, "rough") || hmu ? "Rough horizontal plane: resolve vertically, then use F=ma horizontally." : "Smooth horizontal plane: use F=ma horizontally.");
    double drive = F, R = m*9.8;
    if (hAng) {
      bool below = has(t, "below") || has(t, "downward") || has(t, "downwards");
      drive = F*deg_cosine(ang);
      R = below ? m*9.8 + F*deg_sine(ang) : m*9.8 - F*deg_sine(ang);
      n = add(out, n, "Resolve the pull: horizontal component = F cos(theta) = %.6g cos(%.6g) = %.10g N", F, ang, drive);
      if (below) {
        n = add(out, n, "Force is below the horizontal, so vertical equilibrium gives R = mg + F sin(theta).");
        n = add(out, n, "R = %.6g*9.8 + %.6g sin(%.6g) = %.10g N", m, F, ang, R);
      } else {
        n = add(out, n, "Force is above the horizontal, so vertical equilibrium gives R + F sin(theta) = mg.");
        n = add(out, n, "R = %.6g*9.8 - %.6g sin(%.6g) = %.10g N", m, F, ang, R);
      }
    } else {
      n = add(out, n, "Vertical equilibrium gives R = mg = %.6g*9.8 = %.10g N", m, R);
    }
    double friction = hmu ? mu*R : 0;
    if (hmu) n = add(out, n, "friction = mu R = %.6g*%.10g = %.10g N", mu, R, friction);
    double net = drive - friction;
    n = add(out, n, "resultant horizontal force = %.10g - %.10g = %.10g N", drive, friction, net);
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "driving") || has(t, "drive")) && has(t, "force") &&
      (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "acceleration") || has(t, "accelerate")) &&
      (has(t, "tow") || has(t, "trailer")) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") && nv >= 5) {
    double m1 = v[0], m2 = v[1], drive = v[2], r1 = v[3], r2 = v[4];
    double m = 0, d = 0, r = 0;
    if (label_num(input, "mass", &m) || word_num(input, "mass", &m)) m1 = m;
    if (word_num(input, "trailermass", &m) || label_num(input, "trailermass", &m)) m2 = m;
    if (word_num(input, "drivingforce", &d) || label_num(input, "drivingforce", &d) || word_num(input, "force", &d)) drive = d;
    if (word_num(input, "resistance", &r) || label_num(input, "resistance", &r)) r1 = r;
    double net = drive - r1 - r2;
    double total = m1 + m2;
    double a = total ? net / total : 0;
    double tension = m2 * a + r2;
    int n = add(out, 0, "Treat the car and trailer as one system to find acceleration.");
    n = add(out, n, "total mass = %.10g + %.10g = %.10g kg", m1, m2, total);
    n = add(out, n, "external resultant = driving force - total resistance");
    n = add(out, n, "F = %.10g - %.10g - %.10g = %.10g N", drive, r1, r2, net);
    n = add(out, n, "a = F/(total mass) = %.10g/%.10g = %.10g m/s^2", net, total, a);
    n = add(out, n, "For the trailer alone: T - %.10g = %.10g*%.10g", r2, m2, a);
    return add(out, n, "T = %.10g N", tension);
  }
  if ((has(t, "driving") || has(t, "drive")) && has(t, "force") &&
      (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "acceleration") || has(t, "accelerate")) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") && nv >= 3) {
    double m = 0, drive = 0, r = 0;
    bool hm = label_num(input, "mass", &m) || word_num(input, "mass", &m);
    bool hd = word_num(input, "drivingforce", &drive) || label_num(input, "drivingforce", &drive) ||
              word_num(input, "force", &drive);
    bool hr = word_num(input, "resistance", &r) || label_num(input, "resistance", &r);
    if (!hm) m = v[0];
    if (!hd) drive = v[1];
    if (!hr) r = v[2];
    double net = drive - r;
    int n = add(out, 0, "Use Newton's second law with the resultant force.");
    n = add(out, n, "resultant force = driving force - resistance");
    n = add(out, n, "F = %.10g - %.10g = %.10g N", drive, r, net);
    return add(out, n, "a = F/m = %.10g/%.10g = %.10g m/s^2", net, m, net / m);
  }
  if ((has(t, "pulled") || has(t, "pull") || has(t, "force")) &&
      (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "acceleration") || has(t, "accelerate")) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") && nv >= 3) {
    double m=0, F=0, r=0;
    bool hm = label_num(input, "mass", &m) || word_num(input, "mass", &m) || prev_word_num(input, "kg", &m);
    bool hF = word_num(input, "force", &F) || prev_word_num(input, "n", &F);
    bool hr = word_num(input, "resistance", &r) || label_num(input, "resistance", &r);
    if (!hm) m = v[0];
    if (!hF) F = v[1];
    if (!hr) r = v[2];
    double net = F - r;
    int n = add(out, 0, "Use Newton's second law with the resultant force.");
    n = add(out, n, "resultant force = pull - resistance");
    n = add(out, n, "F = %.10g - %.10g = %.10g N", F, r, net);
    return add(out, n, "a = F/m = %.10g/%.10g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "force") || has(t, "newton")) && (has(t, "mass") || has(t, "accel")) &&
      !has(t, "connected") && !has(t, "pulley") && !has(t, "plane") && !has(t, "rough") && !has(t, "incline") && !has(t, "slope") && nv >= 2) {
    sprintf(cmd, "force(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "weight") && nv >= 1) {
    sprintf(cmd, "weight(%.10g)", v[0]); return eval_mech(cmd, out);
  }
  if ((has(t, "incline") || has(t, "inclined") || has(t, "slope") || has(t, "plane")) &&
      (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "acceleration") || has(t, "accelerate") || has(c, "findacceleration")) &&
      !has(t, "coefficient") && !has(t, "friction") && !has(t, "power") &&
      !has(t, "driving") && !has(t, "drive") && !has(t, "engine") &&
      !has(t, "pull") && !has(t, "force") && nv >= 3) {
    double m = 0, theta = 0, r = 0;
    bool hm = word_num(input, "mass", &m) || label_num(input, "mass", &m) || prev_word_num(input, "kg", &m);
    bool hA = word_num(input, "angle", &theta) || label_num(input, "angle", &theta) || prev_word_num(input, "degrees", &theta);
    bool hr = word_num(input, "resistance", &r) || label_num(input, "resistance", &r);
    if (!hm) m = v[0];
    if (!hA) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], r) && v[i] > 0 && v[i] <= 90) { theta = v[i]; hA = true; break; }
    if (!hr) for (int i = nv - 1; i >= 0; --i)
      if (!near_num(v[i], m) && !near_num(v[i], theta)) { r = v[i]; hr = true; break; }
    double down = m * 9.8 * deg_sine(theta);
    bool moving_up = has(t, "up") || has(t, "uphill");
    double net = moving_up ? -down - r : down - r;
    int n = add(out, 0, "Resolve along the slope and treat resistance as a force opposing motion.");
    n = add(out, n, "down-slope component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, theta, down);
    if (moving_up) n = add(out, n, "resultant up the slope = -%.10g - %.10g = %.10g N", down, r, net);
    else n = add(out, n, "resultant down the slope = %.10g - %.10g = %.10g N", down, r, net);
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "pulley") || has(t, "connected")) && (has(t, "incline") || has(t, "inclined") || has(t, "plane")) && nv >= 3) {
    double m1=0,m2=0,ang=0,mu=0;
    bool hM=word_num(input,"mass",&m1) || label_num(input,"m1",&m1);
    bool hH=word_num(input,"hangingmass",&m2) || label_num(input,"m2",&m2);
    bool hA=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) || word_num(input,"degrees",&ang);
    bool hMu=label_num(input,"mu",&mu) || label_num(input,"coefficient",&mu) || word_num(input,"coefficient",&mu) ||
             first_num_after_word(input,"coefficient",&mu) || first_num_after_word(input,"friction",&mu);
    if (!hM) m1 = v[0];
    if (!hH) m2 = v[1];
    if (!hA) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], m1) && !near_num(v[i], m2)) { ang = v[i]; break; }
      if (ang == 0) ang = v[2];
    }
    if (!hMu && (has(t, "coefficient") || has(t, "mu") || has(t, "rough"))) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], m1) && !near_num(v[i], m2) && !near_num(v[i], ang) && v[i] >= 0 && v[i] <= 1) { mu = v[i]; hMu = true; break; }
    }
    if (hMu || has(t, "rough")) sprintf(cmd, "roughinclinepulley(%.10g,%.10g,%.10g,%.10g)", m1, m2, ang, hMu ? mu : 0);
    else sprintf(cmd, "inclinepulley(%.10g,%.10g,%.10g)", m1, m2, ang);
    return eval_mech(cmd, out);
  }
  if ((has(t, "rough") || has(t, "friction") || has(t, "coefficient")) &&
      (has(t, "plane") || has(t, "inclined") || has(t, "incline") || has(t, "slope")) &&
      (has(t, "pull") || has(t, "force")) && (has(t, "above") || has(t, "below")) &&
      (has(t, "acceleration") || has(t, "accelerate")) && nv >= 5) {
    double m=0, theta=0, F=0, phi=0, mu=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m) || num_before_unit(input,"kg",&m);
    bool hF=word_num(input,"force",&F) || label_num(input,"force",&F) || num_before_unit(input,"n",&F);
    bool hmu=word_num(input,"coefficient",&mu) || label_num(input,"mu",&mu) || label_num(input,"coefficient",&mu);
    if (!hm) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], F) && v[i] > 0) { m = v[i]; hm = true; break; }
    }
    if (!hF) F = v[2];
    if (!hmu) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { mu = v[i]; hmu = true; break; }
    theta = v[1];
    if (!prev_word_num(input, "above", &phi) && !prev_word_num(input, "below", &phi)) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && !near_num(v[i], theta) && !near_num(v[i], F) && !near_num(v[i], mu) && v[i] > 1) { phi = v[i]; break; }
    }
    double R = m*9.8*deg_cosine(theta) - (has(t, "below") ? -1 : 1)*F*deg_sine(phi);
    double fr = mu * R;
    double up = F*deg_cosine(phi), down = m*9.8*deg_sine(theta);
    double net = up - down - fr;
    int n = add(out, 0, "Resolve parallel and perpendicular to the rough inclined plane.");
    n = add(out, n, "Up-plane pull component = %.6g cos(%.6g) = %.10g N", F, phi, up);
    n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, theta, down);
    n = add(out, n, "R = mg cos(theta) - F sin(phi) = %.10g N", R);
    n = add(out, n, "friction = mu R = %.6g*%.10g = %.10g N", mu, R, fr);
    n = add(out, n, "resultant up the plane = %.10g - %.10g - %.10g = %.10g N", up, down, fr, net);
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "rough") || has(t, "friction") || has(t, "coefficient")) &&
      (has(t, "plane") || has(t, "inclined") || has(t, "incline") || has(t, "slope")) &&
      (has(t, "pull") || has(t, "force")) &&
      (has(c, "findcoefficient") || has(c, "findmu")) &&
      (has(t, "acceleration") || has(t, "accelerates") || has(t, "accelerate")) &&
      nv >= 4) {
    double m=0, theta=0, F=0, a0=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hF=word_num(input,"force",&F) || label_num(input,"force",&F) || word_num(input,"pull",&F);
    bool hA=word_num(input,"angle",&theta) || label_num(input,"angle",&theta) || prev_word_num(input,"degrees",&theta);
    bool ha0=word_num(input,"acceleration",&a0) || word_num(input,"accelerates",&a0) ||
              first_num_after_word(input,"accelerates",&a0) || label_num(input,"acceleration",&a0);
    if (!hm) m = v[0];
    if (!hA) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], F) && !near_num(v[i], a0) && v[i] > 1 && v[i] <= 90) { theta = v[i]; hA = true; break; }
    if (!hF) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], theta) && !near_num(v[i], a0) && v[i] > 1) { F = v[i]; hF = true; break; }
    if (!ha0) for (int i = nv - 1; i >= 0; --i)
      if (!near_num(v[i], m) && !near_num(v[i], theta) && !near_num(v[i], F)) { a0 = v[i]; ha0 = true; break; }
    if (hm || hF || hA || ha0) {
      double down = m*9.8*deg_sine(theta);
      double R = m*9.8*deg_cosine(theta);
      bool pulled_down = has(c, "pulleddown") || has(c, "forcedown") || has(c, "downtheplane") || has(c, "downplane");
      double mu = pulled_down ? (F + down - m*a0) / R : (F - down - m*a0) / R;
      int n = add(out, 0, "Resolve parallel and perpendicular to the rough inclined plane.");
      n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, theta, down);
      n = add(out, n, "Normal reaction R = mg cos(theta) = %.6g*9.8 cos(%.6g) = %.10g N", m, theta, R);
      n = add(out, n, "Use F=ma along the plane and friction = mu R.");
      if (pulled_down) n = add(out, n, "%.10g + %.10g - mu*%.10g = %.6g*%.6g", F, down, R, m, a0);
      else n = add(out, n, "%.10g - %.10g - mu*%.10g = %.6g*%.6g", F, down, R, m, a0);
      return add(out, n, "mu = %.10g", mu);
    }
  }
  if ((has(t, "rough") || has(t, "friction") || has(t, "coefficient")) &&
      (has(t, "plane") || has(t, "inclined") || has(t, "incline") || has(t, "slope")) &&
      (has(t, "pull") || has(t, "force")) &&
      (has(t, "acceleration") || has(t, "accelerate")) && nv >= 4) {
    double m=0, theta=0, F=0, mu=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hF=word_num(input,"force",&F) || label_num(input,"force",&F) || word_num(input,"pull",&F);
    bool hA=word_num(input,"angle",&theta) || label_num(input,"angle",&theta) || prev_word_num(input,"degrees",&theta);
    bool hmu=word_num(input,"coefficient",&mu) || label_num(input,"mu",&mu) || label_num(input,"coefficient",&mu);
    if (!hm) m = v[0];
    if (!hmu) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { mu = v[i]; hmu = true; break; }
    if (!hA) {
      for (int i = 0; i < nv; ++i)
        if (!near_num(v[i], m) && !near_num(v[i], F) && !near_num(v[i], mu) && v[i] > 1 && v[i] <= 90) { theta = v[i]; hA = true; break; }
    }
    if (!hF) {
      for (int i = nv - 1; i >= 0; --i)
        if (!near_num(v[i], m) && !near_num(v[i], theta) && !near_num(v[i], mu)) { F = v[i]; hF = true; break; }
    }
    double down = m*9.8*deg_sine(theta);
    double R = m*9.8*deg_cosine(theta);
    double fr = mu * R;
    bool pulled_down = has(c, "pulleddown") || has(c, "forcedown") || has(c, "downtheplane") || has(c, "downplane");
    double net = pulled_down ? F + down - fr : F - down - fr;
    int n = add(out, 0, "Resolve parallel and perpendicular to the rough inclined plane.");
    n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, theta, down);
    n = add(out, n, "Normal reaction R = mg cos(theta) = %.6g*9.8 cos(%.6g) = %.10g N", m, theta, R);
    n = add(out, n, "friction = mu R = %.6g*%.10g = %.10g N", mu, R, fr);
    if (pulled_down) n = add(out, n, "resultant down the plane = %.10g + %.10g - %.10g = %.10g N", F, down, fr, net);
    else n = add(out, n, "resultant up the plane = %.10g - %.10g - %.10g = %.10g N", F, down, fr, net);
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "smooth") || !has(t, "rough")) &&
      (has(t, "plane") || has(t, "inclined") || has(t, "incline") || has(t, "slope")) &&
      (has(t, "pull") || has(t, "force")) &&
      (has(t, "acceleration") || has(t, "accelerate")) && nv >= 3 &&
      !has(t, "coefficient") && !has(t, "friction") &&
      !has(t, "resistance") && !has(t, "resistive") && !has(t, "braking")) {
    double m=0, theta=0, F=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m) || num_before_unit(input,"kg",&m);
    bool hF=word_num(input,"force",&F) || label_num(input,"force",&F) || num_before_unit(input,"n",&F);
    bool hA=word_num(input,"angle",&theta) || label_num(input,"angle",&theta) || prev_word_num(input,"degrees",&theta);
    if (!hm) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], F) && v[i] > 0) { m = v[i]; hm = true; break; }
    }
    if (!hA) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], m) && !near_num(v[i], F) && v[i] > 0 && v[i] <= 90) { theta = v[i]; hA = true; break; }
    }
    if (!hF) {
      for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], m) && !near_num(v[i], theta)) { F = v[i]; hF = true; break; }
    }
    if (!hm) m = v[0];
    double down = m*9.8*deg_sine(theta), net = F - down;
    int n = add(out, 0, "Smooth inclined plane: resolve along the plane and use F=ma.");
    n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, theta, down);
    n = add(out, n, "Resultant up the plane = %.10g - %.10g = %.10g N", F, down, net);
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) &&
      has(t, "power") && (has(t, "angle") || has(t, "find")) &&
      (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "constant") || has(t, "speed")) && !has(c, "findpower") &&
      !has(c, "findenginepower") && !has(t, "degrees") && nv >= 4) {
    double m=0, P=0, spd=0, r=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool hP=label_num(input,"power",&P) || word_num(input,"power",&P) ||
            num_before_unit(input,"kw",&P) || num_before_unit(input,"kilowatt",&P);
    bool hs=num_before_unit(input,"m/s",&spd) || num_before_unit(input,"ms^-1",&spd) ||
            word_num(input,"speed",&spd) || word_num(input,"velocity",&spd);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    if (!hm) m = v[0];
    if (!hP) P = v[2];
    if (has(t, "kw") || has(t, "kilowatt")) P *= 1000.0;
    if (!hs) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P) && !near_num(v[i], r)) { spd = v[i]; hs = true; break; }
    if (!hr) for (int i = nv - 1; i >= 0; --i)
      if (!near_num(v[i], m) && !near_num(v[i], spd) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P)) { r = v[i]; hr = true; break; }
    double drive = spd ? P / spd : 0;
    double s = (drive - r) / (m * 9.8);
    double theta = deg_arcsine(s);
    int n = add(out, 0, "At constant speed, driving force balances resistance and mg sin(theta).");
    n = add(out, n, "driving force = P/v = %.10g/%.10g = %.10g N", P, spd, drive);
    n = add(out, n, "mg sin(theta) = %.10g - %.10g = %.10g", drive, r, drive-r);
    n = add(out, n, "sin(theta) = (%.10g-%.10g)/(%.10g*9.8) = %.10g", drive, r, m, s);
    return add(out, n, "theta = %.10g degrees", theta);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) &&
      has(t, "power") && (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "speed") || has(t, "velocity") || has(t, "travels") || has(c, "m/s") || has(t, "m,s")) &&
      (has(t, "acceleration") || has(t, "accelerate") || has(c, "findacceleration")) && nv >= 5) {
    double m=0, P=0, spd=0, r=0, ang=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool hP=label_num(input,"power",&P) || word_num(input,"power",&P) ||
            prev_word_num(input,"kw",&P) || prev_word_num(input,"kilowatt",&P) ||
            num_before_unit(input,"kw",&P) || num_before_unit(input,"kilowatt",&P);
    bool hs=num_before_unit(input,"m/s",&spd) || num_before_unit(input,"ms^-1",&spd) ||
            word_num(input,"speed",&spd) || word_num(input,"velocity",&spd);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    if (!hm) m = v[0];
    if (!hP) P = v[2];
    if (has(t, "kw") || has(t, "kilowatt")) P *= 1000.0;
    if (!ha) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], spd) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P) && !near_num(v[i], r) && v[i] > 0 && v[i] <= 90) { ang = v[i]; ha = true; break; }
    if (!hs) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P) && !near_num(v[i], r) && !near_num(v[i], ang) && v[i] > 0) { spd = v[i]; hs = true; break; }
    if (!hr) for (int i = nv - 1; i >= 0; --i)
      if (!near_num(v[i], m) && !near_num(v[i], spd) && !near_num(v[i], P) && !near_num(v[i]*1000.0, P) && !near_num(v[i], ang)) { r = v[i]; hr = true; break; }
    double drive = spd ? P / spd : 0;
    double down = m * 9.8 * deg_sine(ang);
    double net = drive - r - down;
    int n = add(out, 0, "Use P = Fv to convert engine power into driving force.");
    n = add(out, n, "driving force = P/v = %.10g/%.10g = %.10g N", P, spd, drive);
    n = add(out, n, "Down-slope weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
    n = add(out, n, "Taking up the slope as positive: resultant = %.10g - %.10g - %.10g = %.10g N", drive, r, down, net);
    n = add(out, n, "Use F=ma with the resultant force.");
    return add(out, n, "a = F/m = %.10g/%.10g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) &&
      (has(t, "resistance") || has(t, "resistive") || has(t, "braking") || has(t, "brake")) &&
      (has(t, "acceleration") || has(t, "accelerate") || has(c, "findacceleration")) &&
      !has(t, "driving") && !has(t, "drive") && nv >= 4) {
    double m=0, ang=0, r=0, brake=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    bool hb=word_num(input,"brakingforce",&brake) || word_num(input,"braking",&brake) ||
            word_num(input,"brake",&brake) || label_num(input,"brake",&brake);
    if (!hm) m = v[0];
    if (!ha) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && v[i] > 0 && v[i] <= 90) { ang = v[i]; ha = true; break; }
    if (!hr) for (int i = 0; i < nv; ++i)
      if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], brake) && v[i] > 90) { r = v[i]; hr = true; break; }
    if (!hb) for (int i = nv - 1; i >= 0; --i)
      if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], r)) { brake = v[i]; hb = true; break; }
    double down = m * 9.8 * deg_sine(ang);
    double net = (has(t, "down") ? down - r - brake : -down - r - brake);
    int n = add(out, 0, "Resolve along the inclined plane.");
    n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
    if (has(t, "down")) {
      n = add(out, n, "Taking down the plane as positive, resultant = %.10g - %.10g - %.10g", down, r, brake);
    } else {
      n = add(out, n, "Taking up the plane as positive, resultant = -%.10g - %.10g - %.10g", down, r, brake);
    }
    return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) &&
      (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "driving") || has(t, "drive") || has(t, "force") || has(t, "constant") || has(t, "speed")) &&
      nv >= 3) {
    double m=0, ang=0, r=0, a=0, known_drive=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r);
    bool hacc=label_num(input,"acceleration",&a) || word_num(input,"acceleration",&a) || word_num(input,"accelerates",&a);
    bool hdrive=word_num(input,"drivingforce",&known_drive) || label_num(input,"drivingforce",&known_drive);
    if (!hdrive && has(t, "driving") && has(t, "force")) {
      for (int i = nv - 1; i >= 0; --i)
        if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], r) && !near_num(v[i], a) && v[i] > 90) { known_drive = v[i]; hdrive = true; break; }
    }
    if (!hm) m = v[0];
    if (!ha) {
      for (int i = 0; i < nv; ++i)
        if (!near_num(v[i], m) && !near_num(v[i], r) && v[i] > 0 && v[i] <= 90) { ang = v[i]; break; }
    }
    if (!hr) {
      for (int i = nv - 1; i >= 0; --i)
        if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], a)) { r = v[i]; break; }
    }
    double down = m * 9.8 * deg_sine(ang);
    if ((has(t, "brake") || has(t, "braking")) && has(t, "down") &&
        (has(t, "constant") || has(t, "speed")) && !hacc) {
      double brake = down - r;
      int n = add(out, 0, "At constant speed down the slope, resultant force along the plane is zero.");
      n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
      n = add(out, n, "Resistance and braking force act up the plane.");
      n = add(out, n, "mg sin(theta) = resistance + braking force");
      return add(out, n, "braking force = %.10g - %.10g = %.10g N", down, r, brake);
    }
    if (hdrive && (has(t, "acceleration") || has(t, "accelerat")) && !hacc) {
      double net = known_drive - down - r;
      int n = add(out, 0, "Resolve along the inclined plane.");
      n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
      n = add(out, n, "Up the plane: F = drive - weight component - resistance.");
      n = add(out, n, "F = %.10g - %.10g - %.10g = %.10g N", known_drive, down, r, net);
      return add(out, n, "a = F/m = %.10g/%.6g = %.10g m/s^2", net, m, net/m);
    }
    double drive = down + r + (hacc ? m * a : 0);
    int n = add(out, 0, "Resolve along the inclined plane.");
    n = add(out, n, "Down-plane weight component = mg sin(theta) = %.6g*9.8 sin(%.6g) = %.10g N", m, ang, down);
    n = add(out, n, "Resistance acts down the plane: %.10g N", r);
    if (hacc) {
      n = add(out, n, "Driving force - %.10g - %.10g = ma = %.6g*%.6g", down, r, m, a);
    } else {
      n = add(out, n, "Constant speed means resultant force is zero.");
      n = add(out, n, "Driving force = mg sin(theta) + resistance");
    }
    n = add(out, n, "Driving force = %.10g N", drive);
    if (has(t, "power")) {
      double spd = 0;
      bool hs = word_num(input, "speed", &spd) || word_num(input, "velocity", &spd);
      if (!hs) {
        for (int i = 0; i < nv; ++i)
          if (!near_num(v[i], m) && !near_num(v[i], ang) && !near_num(v[i], r) && !near_num(v[i], a)) { spd = v[i]; break; }
      }
      return add(out, n, "Power = Fv = %.10g*%.10g = %.10g W", drive, spd, drive * spd);
    }
    return n;
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) && (has(t, "acceleration") || has(t, "accelerate") || has(t, "rough")) && nv >= 2) {
    double m=0, ang=0, mu=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m) || label_num(input,"m",&m) ||
            num_before_unit(input,"kg",&m);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || label_num(input,"theta",&ang) ||
            prev_word_num(input,"degrees",&ang) || first_num_after_word(input,"inclined",&ang) ||
            first_num_after_word(input,"incline",&ang) || first_num_after_word(input,"angle",&ang);
    bool hmu=label_num(input,"mu",&mu) || word_num(input,"mu",&mu) || word_num(input,"friction",&mu) ||
              word_num(input,"coefficient",&mu) || label_num(input,"coefficient",&mu);
    if (!hm && ha && hmu && (has(t, "slide") || has(t, "slides") || has(t, "down"))) {
      double acc = 9.8 * (deg_sine(ang) - mu * deg_cosine(ang));
      int n = add(out, 0, "For sliding down a rough plane, mass cancels in F=ma.");
      n = add(out, n, "a = g(sin(theta) - mu cos(theta))");
      return add(out, n, "a = 9.8*(sin(%.6g) - %.6g*cos(%.6g)) = %.10g m/s^2", ang, mu, ang, acc);
    }
    if (hm && ha && hmu) sprintf(cmd, "inclineacc(%.10g,%.10g,%.10g)", m, ang, mu);
    else if (nv < 3) return 0;
    else if (nv >= 3 && v[1] > 0 && v[1] < 1.5 && v[2] > 1.5) sprintf(cmd, "inclineacc(%.10g,%.10g,%.10g)", v[0], v[2], v[1]);
    else sprintf(cmd, "inclineacc(%.10g,%.10g,%.10g)", v[0], v[1], v[2]);
    return eval_mech(cmd, out);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) && nv >= 2) {
    double m=0, ang=0, mu=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m) || label_num(input,"m",&m) ||
            num_before_unit(input,"kg",&m);
    bool ha=label_num(input,"angle",&ang) || word_num(input,"angle",&ang) || label_num(input,"theta",&ang) ||
            prev_word_num(input,"degrees",&ang) || first_num_after_word(input,"inclined",&ang) ||
            first_num_after_word(input,"incline",&ang) || first_num_after_word(input,"angle",&ang);
    bool hmu=label_num(input,"mu",&mu) || word_num(input,"mu",&mu) || word_num(input,"friction",&mu) || label_num(input,"coefficient",&mu);
    if (hm && ha) sprintf(cmd, "incline(%.10g,%.10g,%.10g)", m, ang, hmu ? mu : 0);
    else if (nv >= 3 && v[1] > 0 && v[1] < 1.5 && v[2] > 1.5) sprintf(cmd, "incline(%.10g,%.10g,%.10g)", v[0], v[2], v[1]);
    else sprintf(cmd, "incline(%.10g,%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0);
    return eval_mech(cmd, out);
  }
  if ((has(t, "pulley") || ((has(t, "connected") || has(t, "connectedparticles")) &&
                            (has(t, "hang") || has(t, "hanging") || has(t, "freely")))) &&
      (has(t, "rough") || has(t, "friction") || has(t, "coefficient")) &&
      (has(t, "horizontal") || has(t, "table")) &&
      nv >= 3) {
    double m1 = v[0], m2 = v[1], mu = 0;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { mu = v[i]; break; }
    double fr = mu * m1 * 9.8;
    double drive = m2 * 9.8;
    double acc = (drive - fr) / (m1 + m2);
    double T = m1 * acc + fr;
    int n = add(out, 0, "Treat both particles as one system, including friction on the table.");
    n = add(out, n, "friction on table particle = mu R = %.6g*%.6g*9.8 = %.10g N", mu, m1, fr);
    n = add(out, n, "driving force = hanging weight = %.6g*9.8 = %.10g N", m2, drive);
    n = add(out, n, "a = (%.10g-%.10g)/(%.6g+%.6g) = %.10g m/s^2", drive, fr, m1, m2, acc);
    n = add(out, n, "For table particle: T - friction = m a.");
    return add(out, n, "T = %.6g*%.10g + %.10g = %.10g N", m1, acc, fr, T);
  }
  if ((has(t, "friction") || has(t, "coefficientoffriction")) && !has(t, "ladder") && nv >= 2) {
    sprintf(cmd, "friction(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "moment") && !has(t, "momentum") && !has(t, "correlation") && !has(t, "productmoment") && nv >= 2) {
    sprintf(cmd, "moment(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "pulley") || has(t, "connectedparticles") || (has(t, "connected") && has(t, "particle"))) && nv >= 2) {
    sprintf(cmd, has(t, "pulley") ? "pulley(%.10g,%.10g)" : "connected(%.10g,%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "impulse") || has(t, "impulsive")) &&
      (has(t, "speed") || has(t, "velocity")) &&
      (has(t, "before") || has(t, "initial")) &&
      (has(t, "after") || has(t, "find")) &&
      !has(t, "finalvelocity") && !has(t, "finalspeed") && nv >= 3) {
    double m=0, I=0, u=0;
    bool hm=label_num(input,"mass",&m) || word_num(input,"mass",&m);
    bool hI=word_num(input,"impulse",&I) || word_num(input,"magnitude",&I);
    bool hu=word_num(input,"before",&u) || word_num(input,"initialspeed",&u) || word_num(input,"initialvelocity",&u);
    if (!hm) m = v[0];
    if (!hI) I = hm ? v[1] : v[0];
    if (!hu) u = v[nv-1];
    if (has(t, "opposite") && I > 0) I = -I;
    double vf = m ? u + I/m : u;
    int n = add(out, 0, "Impulse equals change in momentum.");
    n = add(out, n, "I = m(v-u)");
    n = add(out, n, "%.10g = %.10g(v-%.10g)", I, m, u);
    return add(out, n, "v = %.10g + %.10g/%.10g = %.10g m/s", u, I, m, vf);
  }
  if ((has(t, "impulse") || has(t, "momentumchange")) && nv >= 3) {
    sprintf(cmd, "impulse(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_mech(cmd, out);
  }
  if ((has(t, "together") || has(t, "coalesce") || has(t, "coalesces") || has(t, "stick") || has(t, "sticks")) &&
      nv >= 4) {
    if ((has(t, "masses") || has(t, "massesoftwo") || has(t, "mass,of")) &&
        (has(t, "speeds") || has(t, "velocities"))) {
      sprintf(cmd, "commonvelocity(%.10g,%.10g,%.10g,%.10g)", v[0], v[2], v[1], v[3]);
    } else {
      sprintf(cmd, "commonvelocity(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]);
    }
    return eval_mech(cmd, out);
  }
  if ((has(t, "collision") || has(t, "collide") || has(t, "collides") || has(t, "impact")) &&
      (has(c, "atrest") || has(c, "fromrest") || has(c, "initiallyatrest") || has(c, "startsfromrest")) &&
      !has(t, "restitution") && !has(t, "coefficient") &&
      (has(t, "after") || has(t, "aftercollision") || has(t, "afterimpact")) &&
      nv >= 4) {
    double m1 = v[0], u1 = v[1], m2 = v[2], u2 = 0, v1 = v[3];
    sprintf(cmd, "momentum(%.10g,%.10g,%.10g,%.10g,%.10g)", m1, u1, m2, u2, v1);
    return eval_mech(cmd, out);
  }
  if ((has(t, "collision") || has(t, "collide") || has(t, "collides") || has(t, "impact")) &&
      (has(t, "after") || has(t, "aftercollision") || has(t, "afterimpact")) &&
      (has(t, "find") || has(t, "velocity") || has(t, "speed")) &&
      !has(t, "restitution") && !has(t, "coefficient") && nv >= 5) {
    double m1 = v[0], m2 = v[1], u1 = v[2], u2 = v[3], v1 = v[nv - 1];
    sprintf(cmd, "momentum(%.10g,%.10g,%.10g,%.10g,%.10g)", m1, u1, m2, u2, v1);
    return eval_mech(cmd, out);
  }
  if ((has(t, "momentum") || has(t, "collision") || has(t, "collide")) && (has(t, "conserve") || has(t, "conservation") || has(t, "mom")) ) {
    double m1=0,u1=0,m2=0,u2=0,v1=0;
    if (label_num(input,"m1",&m1) && label_num(input,"u1",&u1) && label_num(input,"m2",&m2) && label_num(input,"u2",&u2) && label_num(input,"v1",&v1)) {
      sprintf(cmd, "momentum(%.10g,%.10g,%.10g,%.10g,%.10g)", m1, u1, m2, u2, v1); return eval_mech(cmd, out);
    }
    if ((has(t, "together") || has(t, "coalesce") || has(t, "stick")) && nv >= 4) {
      sprintf(cmd, "commonvelocity(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_mech(cmd, out);
    }
    if (nv >= 5) {
      sprintf(cmd, "momentum(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4]); return eval_mech(cmd, out);
    }
  }
  if (has(t, "power") && (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "maximumspeed") || has(t, "maxspeed") || (has(t, "maximum") && has(t, "speed"))) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") && nv >= 2) {
    double P=0, r=0;
    bool hP=word_num(input,"power",&P) || label_num(input,"power",&P) || prev_word_num(input,"kw",&P) || prev_word_num(input,"kilowatt",&P);
    bool hr=word_num(input,"resistance",&r) || label_num(input,"resistance",&r) || word_num(input,"resistive",&r);
    if (!hP) P = v[0];
    if (has(t, "kw") || has(t, "kilowatt")) P *= 1000.0;
    if (!hr) for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], P) && !near_num(v[i]*1000.0, P)) { r = v[i]; break; }
    double vmax = r ? P / r : 0;
    int n = add(out, 0, "At maximum speed on level ground, acceleration is zero.");
    n = add(out, n, "Driving force equals resistance.");
    n = add(out, n, "P = Fv, so v = P/F.");
    return add(out, n, "maximum speed = %.10g/%.10g = %.10g m/s", P, r, vmax);
  }
  if (has(t, "power") && (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "constant") || has(t, "speed") || has(t, "velocity") || has(t, "travels") || has(t, "travelling")) &&
      !(has(t, "acceleration") || has(t, "accelerate")) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") && nv >= 2) {
    double spd=0, r=0;
    bool hs=word_num(input,"speed",&spd) || word_num(input,"velocity",&spd);
    bool hr=word_num(input,"resistance",&r) || word_num(input,"resistive",&r);
    if (!hs) {
      for (int i = 0; i < nv; ++i)
        if (v[i] > 0 && v[i] <= 100 && !near_num(v[i], r)) { spd = v[i]; break; }
    }
    if (!hr) {
      for (int i = nv - 1; i >= 0; --i)
        if (!near_num(v[i], spd)) { r = v[i]; break; }
    }
    int n = add(out, 0, "At constant speed, the driving force balances the resistance.");
    n = add(out, n, "driving force = resistance = %.10g N", r);
    return add(out, n, "Power = Fv = %.10g*%.10g = %.10g W", r, spd, r*spd);
  }
  if (has(t, "power") && !(has(t, "resistance") && (has(t, "acceleration") || has(t, "accelerate"))) && nv >= 2) {
    sprintf(cmd, "power(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "momentum") && (has(t, "velocity") || has(t, "speed")) && (has(t, "i") || has(t, "j"))) {
    double xs[4], ys[4]; int vc = scan_ij_vectors(input, xs, ys, 4);
    double m = 0;
    bool hm = word_num(input,"mass",&m) || label_num(input,"mass",&m) || num_before_unit(input, "kg", &m);
    if (!hm && nv >= 1) m = v[0];
    if (m > 0 && vc >= 1) {
      double px = m*xs[0], py = m*ys[0], sp2 = xs[0]*xs[0] + ys[0]*ys[0], ke = 0.5*m*sp2;
      int n = add(out, 0, "Momentum is mass times velocity, component by component.");
      n = add(out, n, "v = %.10g i %+.10g j", xs[0], ys[0]);
      n = add(out, n, "p = mv = %.10g(%.10g i %+.10g j)", m, xs[0], ys[0]);
      n = add(out, n, "momentum = %.10g i %+.10g j kg m/s", px, py);
      if (has(t, "kinetic") || has(t, "energy")) return add(out, n, "KE = 1/2*m*|v|^2 = 1/2*%.10g*(%.10g^2+%.10g^2) = %.10g J", m, xs[0], ys[0], ke);
      return n;
    }
  }
  if ((has(t, "kinetic") || has(t, "energy") || has(t, "workenergy")) && nv >= 2) {
    sprintf(cmd, nv > 2 ? "energy(%.10g,%.10g,%.10g)" : "energy(%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "workdone") || has(t, "work")) && nv >= 2) {
    double A=0, B=0, C=0, D=0, lo=0, hi=0;
    double rb=0, rd=0; int rpow=0;
    if ((has(t, "force") || has(t, "displacement")) && has(t, "i") && has(t, "j")) {
      double xs[4], ys[4]; int vc = scan_ij_vectors(input, xs, ys, 4);
      if (vc >= 2) {
        double W = xs[0]*xs[1] + ys[0]*ys[1];
        int n = add(out, 0, "Work done by a constant vector force is the scalar product F dot s.");
        n = add(out, n, "F = %.10g i %+.10g j, s = %.10g i %+.10g j", xs[0], ys[0], xs[1], ys[1]);
        return add(out, n, "W = %.10g*%.10g + %.10g*%.10g = %.10g J", xs[0], xs[1], ys[0], ys[1], W);
      }
    }
    if (has(c, "/(") && parse_linear_den_power(c, 'x', &rb, &rd, &rpow) && rpow >= 1 &&
        ((word_num_with_t(input, "from", &lo) || word_num(input, "from", &lo) || (nv >= 2 && (lo = v[nv-2], true))) &&
         (word_num_with_t(input, "to", &hi) || word_num(input, "to", &hi) || (nv >= 2 && (hi = v[nv-1], true))))) {
      double k = v[0];
      double F1 = linear_den_antideriv(k, rb, rd, rpow, lo);
      double F2 = linear_den_antideriv(k, rb, rd, rpow, hi);
      char den[48]; fmt_linear_var(den, sizeof(den), rb, 'x', rd);
      int n = add(out, 0, "Work done by a variable force is the integral of force.");
      n = add(out, n, "F(x) = %.10g/(%s)^%d", k, den, rpow);
      n = add(out, n, "W = integral from %.6g to %.6g of F(x) dx", lo, hi);
      if (rpow == 1) n = add(out, n, "integral = (%.10g/%.10g)ln|%s|", k, rb, den);
      else n = add(out, n, "integral = %.10g(%s)^%d/(%.10g*(%d))", k, den, 1-rpow, rb, 1-rpow);
      return add(out, n, "W = %.10g J", F2 - F1);
    }
    if ((has(c, "/x^") || has(c, "x^-")) &&
        ((word_num_with_t(input, "from", &lo) || word_num(input, "from", &lo) || (nv >= 2 && (lo = v[nv-2], true))) &&
         (word_num_with_t(input, "to", &hi) || word_num(input, "to", &hi) || (nv >= 2 && (hi = v[nv-1], true))))) {
      const char *pp = strstr(c, "/x^");
      int pow = pp && isdigit((unsigned char)pp[3]) ? pp[3] - '0' : 2;
      if (!pp && (pp = strstr(c, "x^-")) && isdigit((unsigned char)pp[3])) pow = pp[3] - '0';
      double k = v[0], W = pow == 1 ? 0 : k * (pwr(hi, 1-pow) - pwr(lo, 1-pow)) / (1-pow);
      int n = add(out, 0, "Work done by a variable force is the integral of force.");
      n = add(out, n, "F(x) = %.10g/x^%d", k, pow);
      n = add(out, n, "W = integral from %.6g to %.6g of %.10g/x^%d dx", lo, hi, k, pow);
      n = add(out, n, "integral = %.10g*x^%d/%d", k, 1-pow, 1-pow);
      return add(out, n, "W = %.10g J", W);
    }
    if ((has(c, ")^") || has(c, ")**")) && has(c, "x") &&
        ((word_num_with_t(input, "from", &lo) || word_num(input, "from", &lo) || (nv >= 2 && (lo = v[nv-2], true))) &&
         (word_num_with_t(input, "to", &hi) || word_num(input, "to", &hi) || (nv >= 2 && (hi = v[nv-1], true))))) {
      const char *p = strchr(c, '(');
      double rb=0, rd=0; const char *e=0;
      if (p && parse_linear_inside(p + 1, 'x', &rb, &rd, &e) && *e == ')') {
        int pow = e[1] == '^' ? (int)read_num(e + 2) : 1;
        if (pow >= 1) {
          double F1 = linear_power_antideriv(1.0, rb, rd, pow, lo);
          double F2 = linear_power_antideriv(1.0, rb, rd, pow, hi);
          char lin[48]; fmt_linear_var(lin, sizeof(lin), rb, 'x', rd);
          int n = add(out, 0, "Work done by a variable force is the integral of force.");
          n = add(out, n, "F(x) = (%s)^%d", lin, pow);
          n = add(out, n, "W = integral from %.6g to %.6g of (%s)^%d dx", lo, hi, lin, pow);
          n = add(out, n, "integral = (%s)^%d/(%.10g*%d)", lin, pow+1, rb, pow+1);
          return add(out, n, "W = %.10g J", F2 - F1);
        }
      }
    }
    if ((has(t, "variable") || has(t, "force") || has(c, "f=") || has(c, "f(")) &&
        parse_force_x_poly(input, &A, &B, &C, &D) &&
        ((word_num_with_t(input, "from", &lo) || word_num(input, "from", &lo) ||
          (nv >= 2 && (lo = v[nv-2], true))) &&
         (word_num_with_t(input, "to", &hi) || word_num(input, "to", &hi) ||
          (nv >= 2 && (hi = v[nv-1], true))))) {
      double W = A*(hi*hi*hi*hi - lo*lo*lo*lo)/4.0 + B*(hi*hi*hi - lo*lo*lo)/3.0 +
                 C*(hi*hi - lo*lo)/2.0 + D*(hi - lo);
      int n = add(out, 0, "Work done by a variable force is the integral of force.");
      n = add(out, n, "F(x) = %.6g x^3 %+.6g x^2 %+.6g x %+.6g", A, B, C, D);
      n = add(out, n, "W = integral from %.6g to %.6g of F(x) dx", lo, hi);
      return add(out, n, "W = %.10g J", W);
    }
    sprintf(cmd, "work(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "restitution") || has(t, "collision") || has(t, "impact")) && nv >= 5) {
    double m1=0,u1=0,m2=0,u2=0,e=0;
    if (label_num(input,"m1",&m1) && label_num(input,"u1",&u1) && label_num(input,"m2",&m2) && label_num(input,"u2",&u2) && label_num(input,"e",&e)) {
      sprintf(cmd, "impactsolve(%.10g,%.10g,%.10g,%.10g,%.10g)", m1, u1, m2, u2, e); return eval_mech(cmd, out);
    }
    if (has(t, "coefficient") || has(t, "given") || has(t, "find")) {
      if ((has(t, "masses") || has(t, "massesoftwo") || has(t, "massesof")) &&
          (has(t, "speed") || has(t, "speeds") || has(t, "velocity") || has(t, "velocities"))) {
        sprintf(cmd, "impactsolve(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[2], v[1], v[3], v[4]); return eval_mech(cmd, out);
      }
      sprintf(cmd, "impactsolve(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4]); return eval_mech(cmd, out);
    }
  }
  if ((has(t, "wall") || has(t, "barrier")) && (has(t, "restitution") || has(t, "impact") || has(t, "hits")) &&
      (has(t, "angle") || has(t, "degrees")) && nv >= 3) {
    double u=0, ang=0, e=0;
    bool hu=word_num(input,"speed",&u) || word_num(input,"velocity",&u);
    bool ha=word_num(input,"angle",&ang) || label_num(input,"angle",&ang) || prev_word_num(input,"degrees",&ang);
    bool he=word_num(input,"restitution",&e) || label_num(input,"e",&e) || word_num(input,"coefficient",&e);
    if (!hu) u = v[0];
    if (!ha) for (int i=0;i<nv;++i) if (!near_num(v[i], u) && v[i] > 1.5 && v[i] <= 90) { ang = v[i]; break; }
    if (!he) for (int i=0;i<nv;++i) if (!near_num(v[i], u) && !near_num(v[i], ang) && v[i] > 0 && v[i] <= 1.5) { e = v[i]; break; }
    bool to_normal = has(t, "normal");
    double parallel = to_normal ? u * deg_sine(ang) : u * deg_cosine(ang);
    double perp = to_normal ? u * deg_cosine(ang) : u * deg_sine(ang);
    double after_perp = e * perp;
    double sp = root(parallel*parallel + after_perp*after_perp);
    int n = add(out, 0, "Resolve velocity into components parallel and perpendicular to the smooth wall.");
    if (to_normal) n = add(out, n, "Angle is to the normal: parallel = u sin(theta), perpendicular = u cos(theta).");
    else n = add(out, n, "Angle is to the wall: parallel = u cos(theta), perpendicular = u sin(theta).");
    n = add(out, n, "parallel component is unchanged: %.10g", parallel);
    n = add(out, n, "perpendicular component after impact = e*%.10g = %.10g", perp, after_perp);
    return add(out, n, "speed after impact = sqrt(%.10g^2+%.10g^2) = %.10g m/s", parallel, after_perp, sp);
  }
  if ((has(t, "restitution") || has(t, "collision") || has(t, "impact")) && nv >= 4) {
    sprintf(cmd, "restitution(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_mech(cmd, out);
  }
  if ((has(t, "centreofmass") || has(t, "centerofmass") || ((has(t, "centre") || has(t, "center")) && has(t, "mass"))) &&
      (has(t, "triangle") || has(t, "triangular")) && nv >= 6) {
    double xbar = (v[0] + v[2] + v[4]) / 3.0;
    double ybar = (v[1] + v[3] + v[5]) / 3.0;
    int n = add(out, 0, "The centre of mass of a uniform triangular lamina is the centroid.");
    n = add(out, n, "xbar = (%.10g+%.10g+%.10g)/3 = %.10g", v[0], v[2], v[4], xbar);
    n = add(out, n, "ybar = (%.10g+%.10g+%.10g)/3 = %.10g", v[1], v[3], v[5], ybar);
    return add(out, n, "centre of mass = (%.10g, %.10g)", xbar, ybar);
  }
  if ((has(t, "centreofmass") || has(t, "centerofmass") || ((has(t, "centre") || has(t, "center")) && has(t, "mass"))) &&
      (has(t, "rod") || has(t, "wire")) && (has(t, "density") || has(t, "lambda")) && has(t, "x") && nv >= 3) {
    double L = v[0], a = v[1], b = v[2];
    if (label_num(input, "length", &L)) {
      int got = 0;
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], L)) { if (!got++) a = v[i]; else { b = v[i]; break; } }
    }
    double mass = a*L*L/2.0 + b*L;
    double moment = a*L*L*L/3.0 + b*L*L/2.0;
    int n = add(out, 0, "For a non-uniform rod, xbar = integral x*lambda dx / integral lambda dx.");
    n = add(out, n, "lambda(x) = %.10g x %+.10g, 0 <= x <= %.10g", a, b, L);
    n = add(out, n, "mass = integral lambda dx = %.10g", mass);
    n = add(out, n, "moment = integral x*lambda dx = %.10g", moment);
    return add(out, n, "xbar = %.10g/%.10g = %.10g", moment, mass, mass ? moment/mass : 0);
  }
  if ((has(t, "centreofmass") || has(t, "centerofmass") || ((has(t, "centre") || has(t, "center")) && has(t, "mass"))) &&
      (has(t, "lamina") || has(t, "rectangle") || has(t, "square")) && has(t, "removed") && nv >= 4) {
    double W=v[0], H=v[1], w=v[2], h=v[3];
    double A0=W*H, A1=w*h, A=A0-A1;
    double xbar = A ? (A0*(W/2.0) - A1*(w/2.0))/A : 0;
    double ybar = A ? (A0*(H/2.0) - A1*(h/2.0))/A : 0;
    int n = add(out, 0, "Treat the missing part as negative area.");
    n = add(out, n, "area = %.10g*%.10g - %.10g*%.10g = %.10g", W, H, w, h, A);
    n = add(out, n, "xbar = (%.10g*%.10g - %.10g*%.10g)/%.10g", A0, W/2.0, A1, w/2.0, A);
    n = add(out, n, "ybar = (%.10g*%.10g - %.10g*%.10g)/%.10g", A0, H/2.0, A1, h/2.0, A);
    return add(out, n, "centre of mass = (%.10g, %.10g)", xbar, ybar);
  }
  if ((has(t, "centreofmass") || has(t, "centerofmass") || ((has(t, "centre") || has(t, "center")) && has(t, "mass"))) &&
      has(t, "particle") && nv >= 4) {
    double msum = 0, mx = 0;
    for (int i = 0; i + 1 < nv; i += 2) {
      double m = v[i], x = v[i+1];
      msum += m;
      mx += m*x;
    }
    int n = add(out, 0, "For particles on a line, use xbar = sum(mx)/sum(m).");
    n = add(out, n, "sum(m) = %.10g", msum);
    n = add(out, n, "sum(mx) = %.10g", mx);
    return add(out, n, "xbar = %.10g/%.10g = %.10g", mx, msum, msum ? mx/msum : 0);
  }
  if ((has(t, "resultant") || has(t, "vector")) && nv >= 2) {
    sprintf(cmd, "vector(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "resolve") || has(t, "components")) && has(t, "force") && nv >= 2) {
    sprintf(cmd, "resolve(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "power") && (has(t, "resistance") || has(t, "resistive")) &&
      (has(t, "acceleration") || has(t, "accelerate")) &&
      !has(t, "incline") && !has(t, "slope") && !has(t, "plane") &&
      (has(t, "speed") || has(t, "velocity") || has(t, "travels") || has(t, "travelling")) && nv >= 4) {
    double m=0, P=0, sp=0, R=0;
    bool hm=word_num(input,"mass",&m) || label_num(input,"mass",&m);
    bool hP=word_num(input,"power",&P) || label_num(input,"power",&P);
    bool hs=word_num(input,"speed",&sp) || word_num(input,"velocity",&sp) || word_num(input,"at",&sp);
    bool hR=word_num(input,"resistance",&R) || label_num(input,"resistance",&R);
    if (!hm) m = v[0];
    if (!hP) P = v[1];
    if (!hs) sp = v[2];
    if (!hR) R = v[3];
    if (has(t, "kw") || has(t, "kilowatt")) P *= 1000.0;
    double drive = sp ? P / sp : 0;
    double net = drive - R;
    int n = add(out, 0, "Use P = Fv to convert engine power into driving force.");
    n = add(out, n, "driving force = P/v = %.10g/%.10g = %.10g N", P, sp, drive);
    n = add(out, n, "resultant force = %.10g - %.10g = %.10g N", drive, R, net);
    n = add(out, n, "Use F=ma with the resultant force.");
    return add(out, n, "a = F/m = %.10g/%.10g = %.10g m/s^2", net, m, net/m);
  }
  if (has(t, "lift") && (has(t, "accelerat") || has(t, "decelerat")) && (has(t, "tension") || has(t, "cable")) && nv >= 2) {
    double m = 0, a0 = 0, g = 9.8;
    bool hm = label_num(input, "mass", &m) || word_num(input, "mass", &m);
    bool ha = label_num(input, "acceleration", &a0) || word_num(input, "acceleration", &a0);
    if (!hm) for (int i = 0; i < nv; ++i) if (v[i] > m) m = v[i];
    if (!ha) for (int i = 0; i < nv; ++i) if (!near_num(v[i], m)) { a0 = v[i]; break; }
    int n = add(out, 0, "For the lift, apply Newton's second law vertically.");
    if (has(t, "down") || has(t, "descend") || (has(t, "decelerat") && (has(t, "upward") || has(t, "upwards"))) ) {
      n = add(out, n, "Taking upward positive: T - mg = -ma.");
      return add(out, n, "T = m(g-a) = %.6g(%.6g-%.6g) = %.10g N", m, g, a0, m*(g-a0));
    }
    n = add(out, n, "Taking upward positive: T - mg = ma.");
    return add(out, n, "T = m(g+a) = %.6g(%.6g+%.6g) = %.10g N", m, g, a0, m*(g+a0));
  }
  if ((has(t, "varacc") || has(t, "variableacceleration") || (has(t, "acceleration") && has(t, "integrate"))) && nv >= 4) {
    sprintf(cmd, "varacc(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_mech(cmd, out);
  }
  if ((has(t, "successes") || has(t, "success") || has(t, "prefer") || has(t, "proportion") || has(c, "p=")) &&
      (has(t, "test") || has(t, "increased") || has(t, "decreased")) &&
      !has(t, "samplemean") && !has(t, "populationmean") && !has(t, "sd") && !has(t, "standarddeviation") && nv >= 4) {
    double N=0, x=0, pv=0, alpha=0.05;
    bool hN=word_num(input,"sample",&N) || word_num(input,"sampleof",&N) || label_num(input,"n",&N);
    bool hx=word_num(input,"successes",&x) || word_num(input,"success",&x) || label_num(input,"x",&x);
    bool hp=label_num(input,"p",&pv) || word_num(input,"p",&pv) || word_num(input,"proportion",&pv);
    if (!hN) N = v[0];
    if (!hp) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { pv = v[i]; hp = true; break; }
    if (!hp && has(t, "percent")) {
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], N) || near_num(v[i], x) || near_num(v[i], alpha*100.0)) continue;
        if (v[i] > 0 && v[i] <= 100) { pv = v[i] / 100.0; hp = true; break; }
      }
    }
    if (!hx) for (int i = 0; i < nv; ++i) if (!near_num(v[i], N) && !near_num(v[i], pv) && v[i] >= 0 && v[i] <= N) { x = v[i]; hx = true; break; }
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], N) || near_num(v[i], x) || near_num(v[i], pv)) continue;
      if (v[i] > 0 && v[i] <= 10) { alpha = v[i]/100.0; break; }
      if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
    }
    double tail = (has(t, "increased") || has(t, "increase") || has(t, "greater") || has(t, "more")) ? 1.0 :
                  ((has(t, "decreased") || has(t, "decrease") || has(t, "less") || has(t, "fewer") || has(t, "smaller")) ? -1.0 :
                   (hx && hp && x > N * pv ? 1.0 : -1.0));
    sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", (int)N, pv, (int)x, alpha, tail);
    return eval_stats(cmd, out);
  }
  if ((has(t, "coin") || has(t, "heads") || has(t, "tails") || has(t, "tosses") || has(t, "tossed")) &&
      (has(t, "critical") || has(t, "criticalregion")) && nv >= 1) {
    int N = (int)v[0];
    double alpha = 0.05;
    for (int i = 1; i < nv; ++i) if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; break; }
    if (has(t, "twotailed") || has(t, "twosided") || (has(t, "two") && (has(t, "tailed") || has(t, "sided")))) {
      double a2 = alpha / 2.0, cum = 0, lowerp = 0, upperp = 0;
      int loCrit = -1, hiCrit = N + 1;
      for (int k=0;k<=N;++k) { cum += binomp(N,0.5,k); if (cum <= a2) { loCrit = k; lowerp = cum; } else break; }
      cum = 0;
      for (int k=N;k>=0;--k) { cum += binomp(N,0.5,k); if (cum <= a2) { hiCrit = k; upperp = cum; } else break; }
      int n = add(out, 0, "Let X ~ B(%d, 0.5) for the number of heads.", N);
      n = add(out, n, "Two-tailed critical region: split alpha between both tails.");
      n = add(out, n, "alpha/2 = %.6g", a2);
      n = add(out, n, "Lower tail probability = %.10g, upper tail probability = %.10g", lowerp, upperp);
      if (loCrit < 0 && hiCrit > N) return add(out, n, "no critical value at this alpha.");
      if (loCrit < 0) return add(out, n, "critical region: X >= %d", hiCrit);
      if (hiCrit > N) return add(out, n, "critical region: X <= %d", loCrit);
      return add(out, n, "critical region: X <= %d or X >= %d", loCrit, hiCrit);
    }
    sprintf(cmd, "critbinom(%d,0.5,%.10g,%.0f)", N, alpha, (has(t, "upper") || has(t, "more")) ? 1.0 : -1.0);
    return eval_stats(cmd, out);
  }
  if ((has(t, "hypothesis") || has(t, "significance") || has(t, "test")) &&
      (has(t, "coin") || has(t, "heads") || has(t, "tails")) && nv >= 2) {
    int ai = -1; double alpha = 0.05;
    if (has(t, "percent") || has(t, "%") || has(t, "significance")) {
      for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; ai = i; break; }
    } else if (nv >= 3 && v[nv - 1] > 0 && v[nv - 1] <= 1) { alpha = v[nv - 1]; ai = nv - 1; }
    double N = -1, x = -1;
    for (int i = 0; i < nv; ++i) if (i != ai && v[i] > N) N = v[i];
    for (int i = 0; i < nv; ++i) if (i != ai && !near_num(v[i], N)) { x = v[i]; break; }
    if (x < 0 && nv >= 2) x = v[1];
    double tail = (has(t, "biased") || has(t, "twotailed") || has(t, "twosided")) ? 0 :
                  ((has(t, "less") || has(t, "fewer") || has(t, "lower") || x < N * 0.5) ? -1 : 1);
    sprintf(cmd, "hypbinom(%d,0.5,%d,%.10g,%.0f)", (int)N, (int)x, alpha, tail);
    return eval_stats(cmd, out);
  }
  if ((has(t, "coin") || has(t, "heads") || has(t, "head") || has(t, "tosses") || has(t, "tossed")) &&
      has(t, "find") && (has(c, "findp") || has(c, "probabilityp") || has(c, "pofheads")) &&
      (has(t, "exactly") || has(c, "p(exactly")) && nv >= 3) {
    double q = -1, N = -1, r = -1;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { q = v[i]; break; }
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], q) && v[i] >= 1 && v[i] == floor_num(v[i]) && v[i] > N) N = v[i];
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], q) && !near_num(v[i], N) && v[i] >= 0 && v[i] == floor_num(v[i])) { r = v[i]; break; }
    if (q > 0 && q < 1 && N >= 1 && r >= 0 && r <= N) {
      double C = choose((int)N, (int)r), roots[4]; int rc = 0;
      double prevx = 0, prevf = C*pwr(0, (int)r)*pwr(1, (int)(N-r)) - q;
      for (int step = 1; step <= 100 && rc < 4; ++step) {
        double x0 = step / 100.0;
        double fx = C*pwr(x0, (int)r)*pwr(1-x0, (int)(N-r)) - q;
        if (prevf == 0) roots[rc++] = prevx;
        else if ((prevf < 0 && fx > 0) || (prevf > 0 && fx < 0)) {
          double lo = prevx, hi = x0, flo = prevf;
          for (int it = 0; it < 40; ++it) {
            double mid = (lo + hi) / 2.0;
            double fm = C*pwr(mid, (int)r)*pwr(1-mid, (int)(N-r)) - q;
            if ((flo < 0 && fm < 0) || (flo > 0 && fm > 0)) { lo = mid; flo = fm; } else hi = mid;
          }
          roots[rc++] = (lo + hi) / 2.0;
        }
        prevx = x0; prevf = fx;
      }
      int n = add(out, 0, "Let X ~ B(%.0f, p) for the number of heads.", N);
      n = add(out, n, "P(X=%.0f) = %.0fC%.0f p^%.0f(1-p)^%.0f", r, N, r, r, N-r);
      n = add(out, n, "%.0fC%.0f p^%.0f(1-p)^%.0f = %.10g", N, r, r, N-r, q);
      if (!rc) return add(out, n, "No solution with 0 <= p <= 1.");
      char buf[96]; int bp = sprintf(buf, "p = ");
      for (int i = 0; i < rc && bp < 80; ++i) bp += sprintf(buf + bp, "%s%.10g", i ? " or " : "", roots[i]);
      return add(out, n, "%s", buf);
    }
  }
  if ((has(t, "coin") || has(t, "heads") || has(t, "tails") || has(t, "tosses") || has(t, "tossed")) &&
      (has(t, "probability") || has(t, "prob") || has(t, "chance")) && nv >= 3) {
    double pv = -1, N = -1, x = -1;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1 && pv < 0) pv = v[i];
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], pv) && v[i] > N) N = v[i];
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], pv) && !near_num(v[i], N)) { x = v[i]; break; }
    if (pv > 0 && N >= 1 && x >= 0) {
      int lo = 0, hi = 0;
      if (prose_count_interval(c, (int)N, &lo, &hi))
        return add_binom_range_lines(out, (int)N, pv, lo, hi, "Required probability");
      int tail = 0;
      if (has(c, "morethan")) tail = 2;
      else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
      else if (has(c, "lessthanorequal") || has(c, "atmost")) tail = -1;
      else if (has(c, "lessthan") || has(t, "fewer")) tail = -2;
      if ((has(t, "normal") || has(t, "approx") || has(t, "approximation")) && tail) {
        double nlo = tail < 0 ? 0 : x, nhi = tail < 0 ? x : N;
        if (tail == 2) nlo = x + 1;
        if (tail == -2) nhi = x - 1;
        sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)N, pv, nlo, nhi);
        return eval_stats(cmd, out);
      }
      if (tail) sprintf(cmd, "binomtail(%d,%.10g,%d,%d)", (int)N, pv, (int)x, tail);
      else sprintf(cmd, "binom(%d,%.10g,%d)", (int)N, pv, (int)x);
      return eval_stats(cmd, out);
    }
  }
  if ((has(t, "coin") || has(t, "heads") || has(t, "tails") || has(t, "tosses") || has(t, "tossed")) &&
      (has(t, "fair") || has(t, "unbiased")) &&
      (has(t, "probability") || has(t, "prob") || has(t, "chance")) && nv >= 2) {
    double N = -1, x = -1;
    for (int i = 0; i < nv; ++i) if (v[i] > N) N = v[i];
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], N)) { x = v[i]; break; }
    if (N >= 1 && x >= 0) {
      int lo = 0, hi = 0;
      if (prose_count_interval(c, (int)N, &lo, &hi))
        return add_binom_range_lines(out, (int)N, 0.5, lo, hi, "Required probability");
      int tail = 0;
      if (has(c, "morethan")) tail = 2;
      else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
      else if (has(c, "lessthanorequal") || has(c, "atmost")) tail = -1;
      else if (has(c, "lessthan") || has(t, "fewer")) tail = -2;
      if (tail) sprintf(cmd, "binomtail(%d,0.5,%d,%d)", (int)N, (int)x, tail);
      else sprintf(cmd, "binom(%d,0.5,%d)", (int)N, (int)x);
      return eval_stats(cmd, out);
    }
  }
  if ((has(t, "die") || has(t, "dice")) &&
      (has(t, "approx") || has(t, "approximation") || has(t, "distributional") || has(t, "normal")) &&
      nv >= 2) {
    double N = 0, x = -1, pv = 1.0 / 6.0;
    bool hN = prev_word_num(input, "times", &N) || prev_word_num(input, "rolls", &N) ||
              prev_word_num(input, "rolled", &N);
    if (!hN) {
      for (int i = 0; i < nv; ++i) if (v[i] > N && !near_num(v[i], 6)) N = v[i];
    }
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], N) || near_num(v[i], 6)) continue;
      x = v[i];
    }
    if (N >= 1 && x >= 0) {
      int tail = prob_tail(c, t);
      double lo = tail < 0 ? 0 : x, hi = tail < 0 ? x : N;
      if (tail == 2) lo = x + 1;
      if (tail == -2) hi = x - 1;
      sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)N, pv, lo, hi);
      return eval_stats(cmd, out);
    }
  }
  if ((has(t, "approx") || has(t, "approximation") || has(t, "distributional")) && nv >= 2) {
    double pv = -1, rawp = -1, N = -1;
    if (label_num(input, "p", &pv) || label_num(input, "probability", &pv) ||
        word_num(input, "p", &pv) ||
        word_num(input, "probability", &pv) || word_num(input, "proportion", &pv) ||
        word_num(input, "chance", &pv)) rawp = pv;
    if (pv < 0 && (has(t, "percent") || has(c, "%") || has(t, "distributional"))) {
      for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 100) { rawp = v[i]; pv = v[i] / 100.0; break; }
    }
    if (pv > 0 && pv < 1 && rawp < 0) rawp = pv;
    if (pv > 1) pv /= 100.0;
    for (int i = 0; i < nv; ++i) {
      if ((rawp >= 0 && near_num(v[i], rawp)) || near_num(v[i], pv)) continue;
      if (v[i] > N && v[i] > 1) N = v[i];
    }
    if (pv > 0 && pv < 1 && N >= 1) {
      double u[4]; int nu = 0;
      for (int i = 0; i < nv && nu < 4; ++i) {
        if (near_num(v[i], N) || near_num(v[i], pv) || (rawp >= 0 && near_num(v[i], rawp))) continue;
        u[nu++] = v[i];
      }
      int tail = prob_tail(c, t);
      if (has(t, "half")) { u[0] = (int)(N / 2); nu = 1; tail = 2; }
      bool poisson = !has(t, "normal") &&
                     ((pv <= 0.15 && N >= 50 && N * pv <= 15) || has(t, "poisson") || has(t, "suitable"));
      if (nu >= 2 && !poisson) {
        double lo = -1, hi = -1;
        for (int i = 0; i < nu; ++i) if (u[i] > hi) { lo = hi; hi = u[i]; } else if (u[i] > lo) lo = u[i];
        if (lo < 0) lo = u[nu-2];
        if (has(c, "morethan") || has(c, "x>")) lo += 1;
        sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)N, pv, lo, hi);
        return eval_stats(cmd, out);
      }
      if (nu >= 1) {
        int x = (int)u[nu-1];
        if (poisson) {
          sprintf(cmd, "poissonapprox(%d,%.10g,%d,%d)", (int)N, pv, x, tail);
        } else {
          double lo = tail > 0 ? (tail == 2 ? x + 1 : x) : 0;
          double hi = tail < 0 ? (tail == -2 ? x - 1 : x) : N;
          sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)N, pv, lo, hi);
        }
        return eval_stats(cmd, out);
      }
    }
  }
  if ((has(c, "b(") || has(c, "~b(") || has(c, "followsb(")) && (has(t, "approx") || has(t, "approximation")) && nv >= 3) {
    double Nd=0, pv=0;
    if (!dist2(c, "b(", &Nd, &pv)) { Nd = v[0]; pv = v[1]; }
    double u[3]; int nu = 0;
    for (int i = 0; i < nv && nu < 3; ++i) {
      if (near_num(v[i], Nd) || near_num(v[i], pv)) continue;
      u[nu++] = v[i];
    }
    int tail = prob_tail(c, t);
    if (has(t, "normal") && nu >= 1) {
      double lo = u[0], hi = u[0];
      if (nu >= 2) {
        lo = u[0] < u[1] ? u[0] : u[1];
        hi = u[0] < u[1] ? u[1] : u[0];
      } else if (tail < 0) {
        lo = 0; hi = u[0];
      } else if (tail > 0) {
        lo = u[0]; hi = Nd;
      }
      sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)Nd, pv, lo, hi);
      return eval_stats(cmd, out);
    }
    if (has(t, "poisson") && nu >= 1) {
      sprintf(cmd, "poissonapprox(%d,%.10g,%d,%d)", (int)Nd, pv, (int)u[0], tail);
      return eval_stats(cmd, out);
    }
  }
  if (has(t, "binom")) {
    double N=0,pv=0,x=0,alpha=0,lo=0,hi=0;
    bool hN=label_num(input,"n",&N) || word_num(input,"n",&N);
    bool hP=label_num(input,"p",&pv) || label_num(input,"probability",&pv) || word_num(input,"p",&pv);
    bool hX=label_num(input,"x",&x) || label_num(input,"r",&x) || word_num(input,"x",&x) || word_num(input,"r",&x);
    bool hA=label_num(input,"alpha",&alpha) || label_num(input,"significance",&alpha) ||
            word_num(input,"alpha",&alpha) || word_num(input,"significance",&alpha);
    bool hLo=label_num(input,"lower",&lo) || label_num(input,"lo",&lo);
    bool hHi=label_num(input,"upper",&hi) || label_num(input,"hi",&hi);
    if (!hN && has(c, "b(") && nv >= 1) { N = v[0]; hN = true; }
    int tail = 0;
    if (has(c, "nomorethan")) tail = -1;
    else if (has(c, "morethan") || has(c, "greaterthan")) tail = 2;
    else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
    else if (has(c, "lessthanorequal") || has(c, "atmost") || has(t, "cdf")) tail = -1;
    else if (has(c, "lessthan") || has(c, "fewerthan") || has(t, "fewer")) tail = -2;
    if ((has(t, "find") || has(t, "deduce") || has(c, "find") || has(c, "deduce") ||
         has(c, "solveforp") || has(c, "unknownp") || has(c, ",p)")) &&
        hN) {
      int r = -1; double q = 0;
      if (prob_x_equal_value(c, &r, &q) && r >= 0 && r <= (int)N) {
        int n = add(out, 0, "Let X ~ B(%d, p).", (int)N);
        n = add(out, n, "Use P(X=r)=nCr p^r(1-p)^(n-r).");
        n = add(out, n, "%dC%d p^%d(1-p)^%d = %.10g", (int)N, r, r, (int)N-r, q);
        if (r == 0) {
          double ans = 1.0 - nth_root(q, (int)N);
          n = add(out, n, "(1-p)^%d = %.10g", (int)N, q);
          return add(out, n, "p = 1 - %.10g^(1/%d) = %.10g", q, (int)N, ans);
        }
        if (r == (int)N) {
          double ans = nth_root(q, (int)N);
          n = add(out, n, "p^%d = %.10g", (int)N, q);
          return add(out, n, "p = %.10g^(1/%d) = %.10g", q, (int)N, ans);
        }
        double C = choose((int)N, r), roots[4]; int rc = 0;
        double prevx = 0, prevf = C*pwr(0, r)*pwr(1, (int)N-r) - q;
        for (int step = 1; step <= 100 && rc < 4; ++step) {
          double x0 = step / 100.0;
          double fx = C*pwr(x0, r)*pwr(1-x0, (int)N-r) - q;
          if (prevf == 0) roots[rc++] = prevx;
          else if ((prevf < 0 && fx > 0) || (prevf > 0 && fx < 0)) {
            double alo = prevx, ahi = x0, flo = prevf;
            for (int it = 0; it < 40; ++it) {
              double mid = (alo + ahi) / 2.0;
              double fm = C*pwr(mid, r)*pwr(1-mid, (int)N-r) - q;
              if ((flo < 0 && fm < 0) || (flo > 0 && fm > 0)) { alo = mid; flo = fm; }
              else ahi = mid;
            }
            roots[rc++] = (alo + ahi) / 2.0;
          }
          prevx = x0; prevf = fx;
        }
        if (!rc) return add(out, n, "No solution with 0 <= p <= 1.");
        char buf[96]; int bp = sprintf(buf, "p = ");
        for (int i = 0; i < rc && bp < 80; ++i) bp += sprintf(buf + bp, "%s%.10g", i ? " or " : "", roots[i]);
        return add(out, n, "%s", buf);
      }
    }
    if ((has(t, "find") || has(t, "deduce")) && has(t, "mean") && has(t, "variance") && nv >= 2 &&
        !(hN && hP)) {
      double mean = 0, var = 0;
      bool hm = word_num(input, "mean", &mean), hv = word_num(input, "variance", &var);
      if (!hm) mean = v[0];
      if (!hv) var = v[1];
      sprintf(cmd, "binomparams(%.10g,%.10g)", mean, var);
      return eval_stats(cmd, out);
    }
    if ((has(t, "mean") || has(t, "expected") || has(c, "e(x)")) &&
        (has(t, "variance") || has(t, "standarddeviation") || has(c, "standarddeviation") || has(t, "sd")) &&
        (has(c, ",p,") || has(c, ",p)") || has(c, "b(") || (has(t, "p") && (!hP || pv == 0))) && nv >= 2) {
      double mean = 0;
      bool hm = word_num(input, "mean", &mean) || word_num(input, "expected", &mean);
      if (!hm) mean = v[1];
      if (!hN || N <= 0) N = v[0];
      if (N > 0) {
        pv = mean / N;
        int n = add(out, 0, "For X ~ B(n,p), mean = np.");
        n = add(out, n, "p = mean/n = %.10g/%.10g = %.10g", mean, N, pv);
        double var = N*pv*(1-pv);
        n = add(out, n, "variance = np(1-p)");
        n = add(out, n, "= %.10g*%.10g*(1-%.10g) = %.10g", N, pv, pv, var);
        if (has(t, "standarddeviation") || has(c, "standarddeviation") || has(t, "sd")) return add(out, n, "sd = sqrt(%.10g) = %.10g", var, root(var));
        return n;
      }
    }
    if (hN && hP && !hA && (has(t, "critical") || has(t, "criticalregion") || has(t, "significance"))) {
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], N) || near_num(v[i], pv)) continue;
        if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; hA = true; break; }
        if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; hA = true; break; }
      }
    }
    if (hN && hP && hA && (has(t, "critical") || has(t, "criticalregion"))) {
      if (has(t, "twotailed") || has(t, "twosided") || (has(t, "two") && (has(t, "tailed") || has(t, "sided")))) {
        double a2 = alpha / 2.0, cum = 0, lowerp = 0, upperp = 0;
        int loCrit = -1, hiCrit = (int)N + 1;
        for (int k=0;k<=(int)N;++k) { cum += binomp((int)N,pv,k); if (cum <= a2) { loCrit = k; lowerp = cum; } else break; }
        cum = 0;
        for (int k=(int)N;k>=0;--k) { cum += binomp((int)N,pv,k); if (cum <= a2) { hiCrit = k; upperp = cum; } else break; }
        int n = add(out, 0, "Let X ~ B(%d, %.6g).", (int)N, pv);
        n = add(out, n, "Two-tailed test: split alpha equally between both tails.");
        n = add(out, n, "alpha/2 = %.6g", a2);
        n = add(out, n, "Lower tail probability = %.10g, upper tail probability = %.10g", lowerp, upperp);
        if (loCrit < 0 && hiCrit > (int)N) return add(out, n, "no critical value at this alpha.");
        if (loCrit < 0) return add(out, n, "critical region: X >= %d", hiCrit);
        if (hiCrit > (int)N) return add(out, n, "critical region: X <= %d", loCrit);
        return add(out, n, "critical region: X <= %d or X >= %d", loCrit, hiCrit);
      }
      sprintf(cmd, "critbinom(%d,%.10g,%.10g,%.0f)", (int)N, pv, alpha, (has(t, "upper") || has(t, "greater") || has(t, "more")) ? 1.0 : -1.0);
      return eval_stats(cmd, out);
    }
    double ilo=0, ihi=0;
    if (hN && hP && prob_x_interval(c, &ilo, &ihi))
      return add_binom_range_lines(out, (int)N, pv, (int)ilo, (int)ihi, "Required probability");
    if (hN && hP && has(t, "between")) {
      double bnd[2]; int bc = 0;
      for (int i = 0; i < nv && bc < 2; ++i) {
        if (near_num(v[i], N) || near_num(v[i], pv)) continue;
        bnd[bc++] = v[i];
      }
      if (bc >= 2) {
        int lo_i = (int)(bnd[0] < bnd[1] ? bnd[0] : bnd[1]);
        int hi_i = (int)(bnd[0] < bnd[1] ? bnd[1] : bnd[0]);
        return add_binom_range_lines(out, (int)N, pv, lo_i, hi_i, "Required probability");
      }
    }
    if (hN && hP && !hX) {
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], N) || near_num(v[i], pv)) continue;
        x = v[i]; hX = true; break;
      }
    }
    if (hN && hP && hX && hA && (has(t, "hypothesis") || has(t, "test"))) {
      double htail = (tail > 0 || has(t, "upper") || has(t, "greater") || has(t, "more")) ? 1.0 :
                     ((tail < 0 || has(t, "lower") || has(t, "less") || has(t, "fewer") || has(t, "smaller")) ? -1.0 :
                      (x > N * pv ? 1.0 : -1.0));
      sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", (int)N, pv, (int)x, alpha, htail);
      return eval_stats(cmd, out);
    }
    if (hN && hP && hLo && hHi && has(t, "normal") && (has(t, "approx") || has(t, "approximation"))) {
      sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)N, pv, lo, hi);
      return eval_stats(cmd, out);
    }
    if (hN && hP && hX && has(t, "normal") && (has(t, "approx") || has(t, "approximation"))) {
      double lower = tail < 0 ? 0 : x, upper = tail < 0 ? x : N;
      sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)N, pv, lower, upper);
      return eval_stats(cmd, out);
    }
    if (hN && hP && hX && has(t, "poisson") && (has(t, "approx") || has(t, "approximation"))) {
      sprintf(cmd, "poissonapprox(%d,%.10g,%d,%d)", (int)N, pv, (int)x, tail);
      return eval_stats(cmd, out);
    }
    if (hN && hP && (has(t, "mean") || has(t, "variance") || has(t, "sd") || has(t, "standarddeviation"))) {
      sprintf(cmd, "binomstats(%d,%.10g)", (int)N, pv);
      return eval_stats(cmd, out);
    }
    double blo=0, bhi=0;
    bool no_more = has(c, "nomorethan");
    bool has_lower = !no_more && (word_num(input, "atleast", &blo) || word_num(input, "greaterthanorequal", &blo) || word_num(input, "morethan", &blo));
    bool lower_strict = !no_more && ((has(c, "morethan") && !has(c, "nomorethan")) || (has(c, "greaterthan") && !has(c, "greaterthanorequal")));
    bool has_upper = word_num(input, "lessthan", &bhi) || word_num(input, "fewerthan", &bhi) ||
                     word_num(input, "fewer", &bhi) || word_num(input, "atmost", &bhi) ||
                     word_num(input, "nomorethan", &bhi) || word_num(input, "lessthanorequal", &bhi);
    bool upper_strict = (has(c, "lessthan") || has(c, "fewerthan") || has(t, "fewer")) && !has(c, "lessthanorequal");
    if (hN && hP && has_lower && has_upper) {
      int lo_i = (int)blo + (lower_strict ? 1 : 0);
      int hi_i = (int)bhi - (upper_strict ? 1 : 0);
      if (lo_i < 0) lo_i = 0;
      if (hi_i > (int)N) hi_i = (int)N;
      double ans = 0;
      for (int k = lo_i; k <= hi_i; ++k) ans += binomp((int)N, pv, k);
      int n = add(out, 0, "Let X ~ B(%d, %.6g).", (int)N, pv);
      n = add(out, n, "Use the integer bounds from the wording.");
      n = add(out, n, "sum P(X=r) for r=%d..%d", lo_i, hi_i);
      return add(out, n, "probability = %.10g", ans);
    }
    if (hN && hP && hX) {
      if (tail) sprintf(cmd, "binomtail(%d,%.10g,%d,%d)", (int)N, pv, (int)x, tail);
      else sprintf(cmd, "binom(%d,%.10g,%d)", (int)N, pv, (int)x);
      return eval_stats(cmd, out);
    }
  }
	  if (has(t, "poisson") && has(t, "normal") && (has(t, "approx") || has(t, "approximation")) && nv >= 2) {
	    double lam = v[0], x = v[1], sig = root(lam);
	    int n = add(out, 0, "Use normal approximation to X ~ Po(%.6g).", lam);
	    n = add(out, n, "mu = lambda = %.6g, sigma = sqrt(lambda) = %.6g", lam, sig);
	    if (has(c, "x>") || has(t, "greater") || has(t, "more")) {
	      double cc = x + 0.5, z = sig ? (cc - lam) / sig : 0;
	      n = add(out, n, "For P(X>%.6g), use continuity correction P(Y>%.6g).", x, cc);
	      n = add(out, n, "z = (%.6g-%.6g)/%.6g = %.10g", cc, lam, sig, z);
	      return add(out, n, "P(Y>%.6g) = %.10g", cc, 1 - normal_cdf(z));
	    }
	    if (has(c, "x<") || has(t, "less") || has(t, "fewer")) {
	      double cc = x - 0.5, z = sig ? (cc - lam) / sig : 0;
	      n = add(out, n, "For P(X<%.6g), use continuity correction P(Y<%.6g).", x, cc);
	      n = add(out, n, "z = (%.6g-%.6g)/%.6g = %.10g", cc, lam, sig, z);
	      return add(out, n, "P(Y<%.6g) = %.10g", cc, normal_cdf(z));
	    }
    sprintf(cmd, "poissonnorm(%.10g,%.10g,%.10g)", lam, x, x);
    return eval_stats(cmd, out);
  }
  if ((has(t, "calls") || has(t, "arrive") || has(t, "arrivals") || has(t, "rate") || has(t, "defect") || has(t, "occur")) &&
      (has(t, "per") || has(t, "minute") || has(t, "metre") || has(t, "meter")) &&
      (has(t, "poisson") || has(t, "probability") || has(t, "calls") || has(t, "defect")) && nv >= 2) {
    double rate=0, minutes=1, x=0, lo=0, hi=0;
    bool hr = word_num(input,"rate",&rate) || word_num(input,"mean",&rate) || word_num(input,"average",&rate);
    if (!hr) rate = v[0];
    bool hm = word_num(input,"in",&minutes);
    if (!hm && (has(c, "in1hour") || has(c, "inanhour") || has(c, "inahour") ||
                has(c, "in1minute") || has(c, "inaminute"))) { minutes = 1; hm = true; }
    if (!hm && !has(c, "perminute") && !has(c, "perhour")) {
      hm = prev_word_num(input,"minutes",&minutes) || prev_word_num(input,"minute",&minutes) ||
           prev_word_num(input,"metres",&minutes) || prev_word_num(input,"metre",&minutes) ||
           prev_word_num(input,"meters",&minutes) || prev_word_num(input,"meter",&minutes);
    }
    double period = 1.0, duration = minutes;
    if (has(c, "perday") || has(c, "per24hours")) period = 24*60;
    else if (has(c, "perhour")) period = 60;
    else if (has(c, "perminute")) period = 1;
    else if (has(c, "perweek")) period = 7*24*60;
    if ((has(c, "perday") || has(c, "perhour") || has(c, "perminute") || has(c, "perweek")) && !has(c, "in")) {
      duration = period;
      hm = true;
    }
    if (has(c, "in1hour") || has(c, "inanhour") || has(c, "inahour")) { duration = 60; hm = true; }
    else if (has(c, "in1minute") || has(c, "inaminute")) { duration = 1; hm = true; }
    else if (prev_word_num(input, "hours", &duration) || prev_word_num(input, "hour", &duration)) { duration *= 60; hm = true; }
    else if (prev_word_num(input, "days", &duration) || prev_word_num(input, "day", &duration)) { duration *= 24*60; hm = true; }
    else if (prev_word_num(input, "minutes", &duration) || prev_word_num(input, "minute", &duration)) { hm = true; }
    else if (hm) {
      if (has(c, "hours") || has(c, "hour")) duration *= 60;
      else if (has(c, "days") || has(c, "day")) duration *= 24*60;
    }
    bool hx = word_num(input,"atleast",&x) || word_num(input,"morethan",&x) || word_num(input,"atmost",&x) ||
              word_num(input,"nomorethan",&x) || word_num(input,"lessthan",&x) || word_num(input,"fewerthan",&x);
    if (!hm && nv >= 3) for (int i = 0; i < nv; ++i) if (!near_num(v[i], rate) && v[i] > minutes) { minutes = v[i]; duration = minutes; }
    if (!hx) for (int i = 0; i < nv; ++i) if (!near_num(v[i], rate) && !near_num(v[i], minutes) && !near_num(v[i], duration)) { x = v[i]; break; }
    if (x == 0) x = v[nv-1];
    double lam = rate * duration / period;
    if (prob_x_interval(c, &lo, &hi)) {
      sprintf(cmd, "poissonrange(%.10g,%d,%d)", lam, (int)lo, (int)hi);
      return eval_stats(cmd, out);
    }
    int tail = prob_tail(c, t);
    if (has(c, "nomorethan")) tail = -1;
    sprintf(cmd, tail ? "poissontail(%.10g,%d,%d)" : "poisson(%.10g,%d)", lam, (int)x, tail);
    return eval_stats(cmd, out);
  }
  if ((has(t, "exponential") || has(t, "exponentially")) &&
      !has(t, "pdf") && !has(t, "density") && !has(c, "exp(-") && nv >= 2) {
    double mean = 0, lam = 0, bound = 0;
    bool hm = word_num(input, "mean", &mean) || word_num(input, "average", &mean);
    bool hl = word_num(input, "lambda", &lam) || word_num(input, "rate", &lam);
    if (!hm && !hl) mean = v[0];
    if (!hl) lam = mean ? 1.0 / mean : 0;
    int tail = prob_tail(c, t);
    if (!prob_x_bound(c, &bound, &tail)) {
      for (int i = 0; i < nv; ++i)
        if (!near_num(v[i], mean) && !near_num(v[i], lam)) { bound = v[i]; break; }
    }
    if (lam > 0 && bound >= 0 && tail) {
      double ans = tail > 0 ? exp_approx(-lam * bound) : 1.0 - exp_approx(-lam * bound);
      int n = add(out, 0, "For X exponential, lambda = 1/mean.");
      if (hm || !hl) n = add(out, n, "lambda = 1/%.10g = %.10g", mean, lam);
      else n = add(out, n, "lambda = %.10g", lam);
      if (tail > 0) {
        n = add(out, n, "P(X>%.6g)=exp(-lambda*x)", bound);
        return add(out, n, "P(X>%.6g)=exp(-%.10g*%.6g)=%.10g", bound, lam, bound, ans);
      }
      n = add(out, n, "P(X<%.6g)=1-exp(-lambda*x)", bound);
      return add(out, n, "P(X<%.6g)=1-exp(-%.10g*%.6g)=%.10g", bound, lam, bound, ans);
    }
  }
  if ((has(t, "hypothesis") || has(t, "significance") || has(t, "test")) &&
	      (has(t, "percent") || has(t, "proportion") || has(t, "percentage")) &&
	      (has(t, "proportion") || has(t, "percentage") || has(t, "prefer") || has(t, "customers") || has(t, "success")) &&
	      (has(t, "sample") || has(t, "customers") || has(t, "people") || has(t, "students")) && nv >= 4) {
    double rawp = -1, alpha = 0.05, N = -1, x = -1;
    prev_word_num(input, "percent", &rawp);
    if (rawp < 0) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 100) { rawp = v[i]; break; }
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] <= 10) alpha = v[i] / 100.0;
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], rawp) || near_num(v[i], alpha*100)) continue;
      if (v[i] > N) N = v[i];
    }
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], rawp) || near_num(v[i], alpha*100) || near_num(v[i], N)) continue;
      x = v[i]; break;
    }
    if (rawp > 0 && rawp < 100 && N >= 1 && x >= 0) {
      double pv = rawp / 100.0;
      double htail = (has(t, "increase") || has(t, "increased") || has(t, "greater") || has(t, "more") || x > N*pv) ? 1.0 : -1.0;
      sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", (int)N, pv, (int)x, alpha, htail);
	      return eval_stats(cmd, out);
	    }
	  }
	  if ((has(t, "least") || has(t, "minimum") || has(c, "smallestn")) &&
	      (has(c, "atleastone") || has(c, "atleast1")) && (has(t, "greater") || has(t, "more")) &&
	      (has(t, "probability") || has(t, "chance") || has(t, "fail") || has(t, "failure") || has(t, "defect")) && nv >= 2) {
	    double pv = -1, target = -1;
	    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1 && pv < 0) pv = v[i];
	    for (int i = nv - 1; i >= 0; --i) if (v[i] > 0 && v[i] < 1 && !near_num(v[i], pv)) { target = v[i]; break; }
	    if (pv > 0 && target > 0 && target < 1) {
	      int N = 1;
	      while (N < 100000 && 1 - pwr(1 - pv, N) <= target) ++N;
	      int n = add(out, 0, "Use the complement for at least one success.");
	      n = add(out, n, "P(at least one)=1-(1-p)^n");
	      n = add(out, n, "Need 1-(1-%.10g)^n > %.10g", pv, target);
	      n = add(out, n, "(1-%.10g)^n < %.10g", pv, 1-target);
	      n = add(out, n, "n > ln(%.10g)/ln(%.10g)", 1-target, 1-pv);
	      return add(out, n, "least n = %d", N);
	    }
	  }
	  if ((has(t, "probability") || has(t, "chance")) && nv >= 2 &&
	      (has(t, "sample") || has(t, "trials") || has(t, "success") || has(t, "faulty") || has(t, "seed") ||
	       has(t, "germinate") || has(t, "plants") || has(t, "affected") || has(t, "disease"))) {
    if ((has(t, "least") || has(t, "minimum") || has(c, "smallestn")) &&
        (has(c, "atleastone") || has(c, "atleast1")) && (has(t, "greater") || has(t, "more")) && nv >= 2) {
      double pv = -1, target = -1;
      for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1 && pv < 0) pv = v[i];
      for (int i = nv - 1; i >= 0; --i) if (v[i] > 0 && v[i] < 1 && !near_num(v[i], pv)) { target = v[i]; break; }
      if (pv > 0 && target > 0 && target < 1) {
        int N = 1;
        while (N < 100000 && 1 - pwr(1 - pv, N) <= target) ++N;
        int n = add(out, 0, "Use the complement for at least one success.");
        n = add(out, n, "P(at least one)=1-(1-p)^n");
        n = add(out, n, "Need 1-(1-%.10g)^n > %.10g", pv, target);
        n = add(out, n, "(1-%.10g)^n < %.10g", pv, 1-target);
        n = add(out, n, "n > ln(%.10g)/ln(%.10g)", 1-target, 1-pv);
        return add(out, n, "least n = %d", N);
      }
    }
    double pv=-1, rawp=-1, N=-1, x=-1;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1 && pv < 0) pv = v[i];
    if (pv < 0 && (has(t, "percent") || has(c, "%"))) for (int i = 0; i < nv; ++i)
      if (v[i] > 0 && v[i] < 100) { rawp = v[i]; pv = v[i] / 100.0; break; }
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], pv) && (rawp < 0 || !near_num(v[i], rawp)) && v[i] > N) N = v[i];
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], pv) && (rawp < 0 || !near_num(v[i], rawp)) && !near_num(v[i], N)) { x = v[i]; break; }
    if (x < 0 && (has(c, "atleastone") || has(c, "atleast1"))) x = 1;
    if (pv > 0 && N >= 1 && x >= 0) {
      int tail = 0;
      if (has(c, "nomorethan")) tail = -1;
      else if (has(c, "morethan")) tail = 2;
      else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
      else if (has(c, "lessthanorequal") || has(c, "atmost")) tail = -1;
      else if (has(c, "lessthan") || has(t, "fewer")) tail = -2;
      if (tail) sprintf(cmd, "binomtail(%d,%.10g,%d,%d)", (int)N, pv, (int)x, tail);
      else sprintf(cmd, "binom(%d,%.10g,%d)", (int)N, pv, (int)x);
      return eval_stats(cmd, out);
    }
  }
  if (has(t, "normal") || has(c, "~n(") || has(c, "x~n(") || has(c, "fromn(")) {
    double x=0, xb=0, mu=0, sig=0, var=0, n0=0, alpha=0, lo=0, hi=0, area=0;
    bool hX=label_num(input,"x",&x) || label_num(input,"value",&x) || word_num(input,"x",&x) || word_num(input,"value",&x);
    bool hXb=label_num(input,"xbar",&xb) || label_num(input,"samplemean",&xb);
    bool hMu=label_num(input,"mu",&mu) || label_num(input,"mean",&mu) || word_num(input,"mu",&mu) || word_num(input,"mean",&mu);
    bool hSig=label_num(input,"sigma",&sig) || label_num(input,"sd",&sig) || label_num(input,"standarddeviation",&sig) || word_num(input,"sigma",&sig) || word_num(input,"sd",&sig) || word_num(input,"standarddeviation",&sig);
    bool hVar=label_num(input,"variance",&var) || label_num(input,"var",&var) ||
              word_num(input,"variance",&var) || word_num(input,"var",&var) ||
              word_num(input,"standarddeviationsquared",&var) || word_num(input,"sigmasquared",&var);
    bool hN=label_num(input,"n",&n0) || word_num(input,"n",&n0) || word_num(input,"sample",&n0) || word_num(input,"samplesize",&n0) || word_num(input,"size",&n0);
    double nfix = 0;
    if (word_num(input, "sampleofsize", &nfix) || word_num(input, "sampleof", &nfix) || word_num(input, "samplesize", &nfix)) {
      n0 = nfix; hN = true;
    }
    const char *sof = strstr(c, "sampleof");
    if (sof && isdigit((unsigned char)sof[8])) { n0 = read_num(sof + 8); hN = true; }
    bool hA=label_num(input,"alpha",&alpha) || label_num(input,"significance",&alpha);
    bool hLo=label_num(input,"lower",&lo) || label_num(input,"lo",&lo);
    bool hHi=label_num(input,"upper",&hi) || label_num(input,"hi",&hi);
    bool hArea=label_num(input,"area",&area) || label_num(input,"probability",&area) ||
               (!has(t, "percentile") && label_num(input,"p",&area));
    double dmu = 0, dsecond = 0;
    bool hDist = dist2(c, "~n", &dmu, &dsecond) || dist2(c, "fromn", &dmu, &dsecond) ||
                 (starts(c, "n(") && dist2(c, "n", &dmu, &dsecond));
    if (hDist) {
      mu = dmu;
      sig = has(c, "^2") ? dsecond : root(dsecond);
      hMu = hSig = true;
      hVar = false;
    }
    if ((has(t, "samplemean") || (has(t, "sample") && has(t, "mean")) || has(t, "xbar")) &&
        (has(t, "probability") || has(c, "p(") || has(t, "more") || has(t, "greater") || has(t, "less")) &&
        hMu && (hSig || hVar) && hN && !has(t, "hypothesis") && !has(t, "test")) {
      double sd = hSig ? sig : root(var), bound = 0;
      const char *bp = strstr(c, "samplemean>");
      bool upper = true, hb = false;
      if (bp) { bound = read_num(bp + 11); hb = true; upper = true; }
      if (!hb) { bp = strstr(c, "xbar>"); if (bp) { bound = read_num(bp + 5); hb = true; upper = true; } }
      if (!hb) { bp = strstr(c, "mean>"); if (bp) { bound = read_num(bp + 5); hb = true; upper = true; } }
      if (!hb) { bp = strstr(c, "samplemean<"); if (bp) { bound = read_num(bp + 11); hb = true; upper = false; } }
      if (!hb) { bp = strstr(c, "xbar<"); if (bp) { bound = read_num(bp + 5); hb = true; upper = false; } }
      if (!hb) { bp = strstr(c, "mean<"); if (bp) { bound = read_num(bp + 5); hb = true; upper = false; } }
      if (!hb) {
        for (int i = 0; i < nv; ++i) {
          if (has(c, "^2") && near_num(v[i], 2)) continue;
          if (near_num(v[i], mu) || near_num(v[i], sd) || near_num(v[i], n0)) continue;
          bound = v[i]; hb = true;
        }
        upper = has(t, "more") || has(t, "greater") || has(c, ">") || has(t, "above");
      }
      if (hb) {
        double se = sd / root(n0);
        double z = se ? (bound - mu) / se : 0;
        double prob = upper ? 1 - normal_cdf(z) : normal_cdf(z);
        int n = add(out, 0, "For a sample mean, use the sampling distribution.");
        n = add(out, n, "Xbar ~ N(%.6g, (%.6g/sqrt(%.6g))^2)", mu, sd, n0);
        n = add(out, n, "standard error = %.6g/sqrt(%.6g) = %.10g", sd, n0, se);
        n = add(out, n, "z = (%.6g-%.6g)/%.10g = %.10g", bound, mu, se, z);
        return add(out, n, "P(Xbar%s%.6g) = %.10g", upper ? ">=" : "<=", bound, prob);
      }
    }
    if (hMu && (hSig || hVar) && strchr(input, '|')) {
      double sd = hSig ? sig : root(var), x0 = 0, g0 = 0;
      char rawbar[160]; clean(input, rawbar, sizeof(rawbar));
      const char *bar = strchr(rawbar, '|');
      const char *lp = strstr(rawbar, "x>"), *rp = bar ? strstr(bar, "x>") : 0;
      if (lp && rp && lp < bar) {
        x0 = read_num(lp + (lp[2] == '=' ? 3 : 2));
        g0 = read_num(rp + (rp[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,1)", x0, g0, mu, sd);
        return eval_stats(cmd, out);
      }
      lp = strstr(rawbar, "x<"); rp = bar ? strstr(bar, "x<") : 0;
      if (lp && rp && lp < bar) {
        x0 = read_num(lp + (lp[2] == '=' ? 3 : 2));
        g0 = read_num(rp + (rp[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,-1)", x0, g0, mu, sd);
        return eval_stats(cmd, out);
      }
    }
    if (has(t, "within") &&
        (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd") || has(t, "sigma")) &&
        hMu && (hSig || hVar)) {
      double k = 0;
      if (!word_num(input, "within", &k)) {
        for (int i = 0; i < nv; ++i)
          if (!near_num(v[i], mu) && (!hSig || !near_num(v[i], sig)) && (!hVar || !near_num(v[i], var))) { k = v[i]; break; }
      }
      if (k > 0) {
        double sd = hSig ? sig : root(var), lo0 = mu - k*sd, hi0 = mu + k*sd;
        int n = add(out, 0, "Within %.10g standard deviations means mu - %.10g*sigma < X < mu + %.10g*sigma.", k, k, k);
        n = add(out, n, "lower = %.10g - %.10g*%.10g = %.10g", mu, k, sd, lo0);
        n = add(out, n, "upper = %.10g + %.10g*%.10g = %.10g", mu, k, sd, hi0);
        n = add(out, n, "NormalCD(lower=%.6g, upper=%.6g, sigma=%.6g, mu=%.6g)", lo0, hi0, sd, mu);
        return add(out, n, "probability = %.10g", normal_cdf(k) - normal_cdf(-k));
      }
    }
    if (!hN) {
      double qn = 0;
      if (prev_word_num(input, "observations", &qn) || prev_word_num(input, "observation", &qn) ||
          prev_word_num(input, "values", &qn)) { n0 = qn; hN = true; }
    }
    if ((has(c, "-a<x<") || has(c, "-k<x<") || has(c, "between")) &&
        (has(t, "find") || has(t, "such")) && hMu && (hSig || hVar) && nv >= 3) {
      double prob = 0;
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], mu) && (!hSig || !near_num(v[i], sig)) && (!hVar || !near_num(v[i], var)) && v[i] > 0 && v[i] < 1) { prob = v[i]; break; }
      if (prob > 0) {
        double sd = hSig ? sig : root(var);
        double left = (1 + prob) / 2.0;
        double z = inv_norm_left(left);
        int n = add(out, 0, "For a central normal interval, split the outside probability equally.");
        n = add(out, n, "P(mu-a<X<mu+a)=%.10g", prob);
        n = add(out, n, "area to the left of mu+a = (1+%.10g)/2 = %.10g", prob, left);
        n = add(out, n, "z = InvNorm(%.10g) = %.10g", left, z);
        return add(out, n, "a = z*sigma = %.10g*%.10g = %.10g", z, sd, z*sd);
      }
    }
    double tail = (has(t, "twotailed") || has(t, "twosided") || (has(t, "two") && (has(t, "tailed") || has(t, "sided"))) || has(t, "different") || has(t, "notequal")) ? 0 :
                  (has(t, "upper") || has(t, "greater") || has(t, "more") || has(c, "x>") || has(c, "x>=") ? 1 : -1);
    if (hMu && (hSig || hVar) && has(t, "given") && nv >= 4) {
      double sd = hSig ? sig : root(var), lower = 0, upper = 0;
      bool hLower = false, hUpper = false;
      const char *gp = strstr(c, "givenxisgreaterthan");
      if (!gp) gp = strstr(c, "givenxgreaterthan");
      if (!gp) gp = strstr(c, "givenx>");
      if (gp) { const char *num = strchr(gp, '>'); lower = num ? read_num(num + 1) : read_num(gp + 19); hLower = true; }
      const char *up = strstr(c, "p(x<");
      if (up) { upper = read_num(up + (up[4] == '=' ? 5 : 4)); hUpper = true; }
      if (!hUpper) {
        up = strstr(c, "findxlessthan");
        if (up) { upper = read_num(up + 13); hUpper = true; }
      }
      if (hLower && hUpper && upper > lower) {
        sprintf(cmd, "normalcondbetween(%.10g,%.10g,%.10g,%.10g,%.10g,1)", lower, upper, lower, mu, sd);
        return eval_stats(cmd, out);
      }
    }
    const char *absx = strstr(c, "abs(x");
    if (absx && hMu && (hSig || hVar)) {
      double centre = mu, radius = 0;
      const char *xm = strstr(absx, "x-");
      const char *xp = strstr(absx, "x+");
      if (xm) centre = read_num(xm + 2);
      else if (xp) centre = -read_num(xp + 2);
      const char *rp = strstr(absx, ")<=");
      int rskip = 3;
      if (!rp) { rp = strstr(absx, ")<"); rskip = 2; }
      if (rp) radius = read_num(rp + rskip);
      if (radius > 0) {
        double sd = hSig ? sig : root(var);
        double lo2 = centre - radius, hi2 = centre + radius;
        double z1 = (lo2 - mu) / sd, z2 = (hi2 - mu) / sd;
        double ans = normal_cdf(z2) - normal_cdf(z1);
        int n = add(out, 0, "P(|X-%.6g|<%.6g) means %.6g-%.6g < X < %.6g+%.6g.", centre, radius, centre, radius, centre, radius);
        if (hVar) n = add(out, n, "sigma = sqrt(%.6g) = %.10g", var, sd);
        n = add(out, n, "So %.10g < X < %.10g.", lo2, hi2);
        n = add(out, n, "z1=(%.10g-%.6g)/%.10g = %.10g", lo2, mu, sd, z1);
        n = add(out, n, "z2=(%.10g-%.6g)/%.10g = %.10g", hi2, mu, sd, z2);
        return add(out, n, "probability = %.10g", ans);
      }
    }
    if (hMu && !hSig && !hVar &&
        (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd") || has(t, "sigma")) &&
        (has(t, "find") || has(t, "unknown") || has(t, "calculate"))) {
      double bound=0; int ptail=0;
      if (prob_x_bound(c, &bound, &ptail)) {
        double prob = 0;
        for (int i = 0; i < nv; ++i) if (!near_num(v[i], mu) && !near_num(v[i], bound) && v[i] > 0 && v[i] < 1) { prob = v[i]; break; }
        if (prob > 0) {
          double left_area = (ptail > 0 || has(c, "x>") || has(t, "greater")) ? 1 - prob : prob;
          double z = inv_norm_left(left_area);
          double sd = z ? (bound - mu) / z : 0;
          if (sd < 0) sd = -sd;
          int n = add(out, 0, "Let X~N(mu,sigma^2). Use z=(x-mu)/sigma.");
          n = add(out, n, "mu = %.6g, x = %.6g", mu, bound);
          n = add(out, n, "area to the left = %.10g, so z = InvNorm(%.10g) = %.10g", left_area, left_area, z);
          n = add(out, n, "(%.6g-%.6g)/sigma = %.10g", bound, mu, z);
          return add(out, n, "sigma = %.10g", sd);
        }
      }
    }
    if ((has(c, "findmean") || has(c, "findthemean")) && (hSig || hVar) && nv >= 3) {
      double bound=0; int ptail=0;
      bool hb = prob_x_bound(c, &bound, &ptail);
      double prob = 0;
      for (int i = 0; i < nv; ++i) {
        if ((hSig && near_num(v[i], sig)) || (hVar && near_num(v[i], var)) || near_num(v[i], bound)) continue;
        if (v[i] > 0 && v[i] < 1) { prob = v[i]; break; }
      }
      if (hb && prob > 0) {
        double sd = hSig ? sig : root(var);
        double left_area = (ptail > 0 || has(c, "x>") || has(t, "greater")) ? 1 - prob : prob;
        double z = inv_norm_left(left_area);
        double mean = bound - z * sd;
        int n = add(out, 0, "Let X~N(mu,sigma^2). Use z=(x-mu)/sigma.");
        n = add(out, n, "sigma = %.6g, x = %.6g", sd, bound);
        n = add(out, n, "area to the left = %.10g, so z = InvNorm(%.10g) = %.10g", left_area, left_area, z);
        n = add(out, n, "(%.6g-mu)/%.6g = %.10g", bound, sd, z);
        return add(out, n, "mu = %.10g", mean);
      }
    }
    if ((has(t, "exceeded") || has(c, "exceededby")) && has(t, "percent") && hMu && (hSig || hVar) && nv >= 3) {
      double pct = 0;
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], mu) || (hSig && near_num(v[i], sig)) || (hVar && near_num(v[i], var))) continue;
        if (v[i] > 0 && v[i] <= 100) { pct = v[i]; break; }
      }
      if (pct > 0) {
        double left_area = 1.0 - pct/100.0;
        sprintf(cmd, hSig ? "invnormal(%.10g,%.10g,%.10g)" : "invnormalvar(%.10g,%.10g,%.10g)", left_area, mu, hSig ? sig : var);
        return eval_stats(cmd, out);
      }
    }
    if ((has(t, "unknown") || has(t, "findthemean") || has(c, "findmean")) && (hSig || hVar) && nv >= 2) {
      double bound=0; int ptail=0;
      bool hb = prob_x_bound(c, &bound, &ptail) || word_num(input, "less than", &bound) || word_num(input, "greater than", &bound);
      double prob = 0;
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], bound) && (!hSig || !near_num(v[i], sig)) && (!hVar || !near_num(v[i], var)) && v[i] > 0 && v[i] < 1) { prob = v[i]; break; }
      if (hb && prob > 0) {
        double sd = hSig ? sig : root(var);
        double left_area = (ptail > 0 || has(t, "greater")) ? 1 - prob : prob;
        double z = inv_norm_left(left_area);
        double mean = bound - z * sd;
        int n = add(out, 0, "Let X~N(mu,sigma^2). Use z=(x-mu)/sigma.");
        n = add(out, n, "sigma = %.6g, x = %.6g", sd, bound);
        n = add(out, n, "area to the left = %.10g, so z = InvNorm(%.10g) = %.10g", left_area, left_area, z);
        n = add(out, n, "(%.6g-mu)/%.6g = %.10g", bound, sd, z);
        return add(out, n, "mu = %.10g", mean);
      }
    }
    if ((has(t, "hypothesis") || has(t, "test")) &&
        (has(t, "samplemean") || (has(t, "sample") && has(t, "mean")) || has(t, "xbar")) &&
        (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sigma") || has(t, "sd"))) {
      double xb2=0, mu2=0, sig2=0, n2=0, alpha2=0.05;
      bool hXb2=label_num(input,"xbar",&xb2) || label_num(input,"samplemean",&xb2) || word_num(input,"samplemean",&xb2) ||
                 ((has(t, "sample") || has(t, "xbar")) && word_num(input,"mean",&xb2));
      bool hMu2=label_num(input,"mu",&mu2) || label_num(input,"populationmean",&mu2) || word_num(input,"populationmean",&mu2);
      if (!hMu2 && (word_num(input,"greaterthan",&mu2) || word_num(input,"morethan",&mu2) || word_num(input,"lessthan",&mu2) ||
                    word_num(input,"exceeds",&mu2) || word_num(input,"exceed",&mu2) || word_num(input,"below",&mu2))) hMu2 = true;
      bool hSig2=label_num(input,"sd",&sig2) || label_num(input,"sigma",&sig2) || label_num(input,"standarddeviation",&sig2) ||
                 word_num(input,"sd",&sig2) || word_num(input,"sigma",&sig2) || word_num(input,"standarddeviation",&sig2);
      bool hN2=label_num(input,"n",&n2) || word_num(input,"samplesize",&n2) || word_num(input,"sample",&n2) || word_num(input,"n",&n2);
      bool hA2=label_num(input,"alpha",&alpha2) || label_num(input,"significance",&alpha2) || word_num(input,"significance",&alpha2);
      if (hA2 && alpha2 > 1 && alpha2 <= 100) alpha2 /= 100.0;
      if (hXb2 && hMu2 && hSig2 && hN2) {
        for (int i = 0; i < nv; ++i) {
          if (near_num(v[i], xb2) || near_num(v[i], mu2) || near_num(v[i], sig2) || near_num(v[i], n2)) continue;
          if (v[i] > 0 && v[i] <= 10) { alpha2 = v[i] / 100.0; break; }
          if (v[i] > 0 && v[i] <= 1) { alpha2 = v[i]; break; }
        }
        sprintf(cmd, "hypnormal(%.10g,%.10g,%.10g,%.10g,%.10g,%.0f)", xb2, mu2, sig2, n2, alpha2, tail);
        return eval_stats(cmd, out);
      }
    }
    if (hMu && (hSig || hVar) && (has(t, "given") || has(t, "conditional")) && nv >= 4) {
      double sd = hSig ? sig : root(var), u[3]; int nu = 0;
      const char *given = strstr(c, "given");
      const char *ltp = strstr(c, "p(x<");
      const char *gtp = strstr(c, "p(x>");
      if (given) {
        const char *gxgt = strstr(given, "x>");
        const char *gxlt = strstr(given, "x<");
        const char *gphrasegt = strstr(given, "xisgreaterthan");
        if (!gphrasegt) gphrasegt = strstr(given, "xgreaterthan");
        if (!gphrasegt) gphrasegt = strstr(given, "xismorethan");
        if (!gphrasegt) gphrasegt = strstr(given, "xmorethan");
        const char *gphraselt = strstr(given, "xislessthan");
        if (!gphraselt) gphraselt = strstr(given, "xlessthan");
        if (!gphraselt) gphraselt = strstr(given, "xisbelow");
        if (!gphraselt) gphraselt = strstr(given, "xbelow");
        if (ltp && (gxgt || gphrasegt)) {
          double upper = read_num(ltp + (ltp[4] == '=' ? 5 : 4));
          int skip = starts(gphrasegt ? gphrasegt : "", "xisgreaterthan") ? 14 :
                     starts(gphrasegt ? gphrasegt : "", "xgreaterthan") ? 12 :
                     starts(gphrasegt ? gphrasegt : "", "xismorethan") ? 11 : 9;
          double lower = gxgt ? read_num(gxgt + (gxgt[2] == '=' ? 3 : 2)) : read_num(gphrasegt + skip);
          sprintf(cmd, "normalcondbetween(%.10g,%.10g,%.10g,%.10g,%.10g,1)", lower, upper, lower, mu, sd);
          return eval_stats(cmd, out);
        }
        if (gtp && (gxlt || gphraselt)) {
          double lower = read_num(gtp + (gtp[4] == '=' ? 5 : 4));
          int skip = starts(gphraselt ? gphraselt : "", "xislessthan") ? 11 :
                     starts(gphraselt ? gphraselt : "", "xlessthan") ? 9 :
                     starts(gphraselt ? gphraselt : "", "xisbelow") ? 8 : 6;
          double upper = gxlt ? read_num(gxlt + (gxlt[2] == '=' ? 3 : 2)) : read_num(gphraselt + skip);
          sprintf(cmd, "normalcondbetween(%.10g,%.10g,%.10g,%.10g,%.10g,-1)", lower, upper, upper, mu, sd);
          return eval_stats(cmd, out);
        }
        if (gtp && (gxgt || gphrasegt)) {
          double x0 = read_num(gtp + (gtp[4] == '=' ? 5 : 4));
          int skip = starts(gphrasegt ? gphrasegt : "", "xisgreaterthan") ? 14 :
                     starts(gphrasegt ? gphrasegt : "", "xgreaterthan") ? 12 :
                     starts(gphrasegt ? gphrasegt : "", "xismorethan") ? 11 : 9;
          double g0 = gxgt ? read_num(gxgt + (gxgt[2] == '=' ? 3 : 2)) : read_num(gphrasegt + skip);
          sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,1)", x0, g0, mu, sd);
          return eval_stats(cmd, out);
        }
        if (ltp && (gxlt || gphraselt)) {
          double x0 = read_num(ltp + (ltp[4] == '=' ? 5 : 4));
          int skip = starts(gphraselt ? gphraselt : "", "xislessthan") ? 11 :
                     starts(gphraselt ? gphraselt : "", "xlessthan") ? 9 :
                     starts(gphraselt ? gphraselt : "", "xisbelow") ? 8 : 6;
          double g0 = gxlt ? read_num(gxlt + (gxlt[2] == '=' ? 3 : 2)) : read_num(gphraselt + skip);
          sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,-1)", x0, g0, mu, sd);
          return eval_stats(cmd, out);
        }
      }
      for (int i = 0; i < nv && nu < 3; ++i) {
        if (near_num(v[i], mu) || (hDist && near_num(v[i], dsecond)) ||
            (hSig && near_num(v[i], sig)) || (hVar && near_num(v[i], var))) continue;
        u[nu++] = v[i];
      }
      if (nu >= 2) {
        if ((has(t, "between") || has(c, "<x<")) && nu >= 3)
          sprintf(cmd, "normalcondbetween(%.10g,%.10g,%.10g,%.10g,%.10g,%.0f)", u[0], u[1], u[2], mu, sd, tail);
        else
          sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,%.0f)", u[0], u[1], mu, sd, tail);
        return eval_stats(cmd, out);
      }
    }
    if (hMu && (hSig || hVar) && (has(c, "interquartilerange") || has(t, "iqr"))) {
      double sd = hSig ? sig : root(var);
      double z = 0.67448975;
      int n = add(out, 0, "For a normal distribution, quartiles use z = +/-0.67448975.");
      if (hVar) n = add(out, n, "sigma = sqrt(%.6g) = %.10g", var, sd);
      n = add(out, n, "Q1 = mu - 0.67448975*sigma = %.6g - 0.67448975*%.10g = %.10g", mu, sd, mu - z*sd);
      n = add(out, n, "Q3 = mu + 0.67448975*sigma = %.6g + 0.67448975*%.10g = %.10g", mu, sd, mu + z*sd);
      return add(out, n, "IQR = Q3 - Q1 = %.10g", 2*z*sd);
    }
    if ((has(t, "total") || has(t, "sum")) && hMu && hSig &&
        (has(t, "probability") || has(t, "find"))) {
      double count=0, bound=0;
      bool hCount = prev_word_num(input, "bags", &count) || prev_word_num(input, "items", &count) ||
                    prev_word_num(input, "values", &count) || word_num(input, "n", &count);
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], mu) || near_num(v[i], sig) || (hCount && near_num(v[i], count))) continue;
        bound = v[i];
      }
      if (hCount && bound != 0) {
        double smu = count * mu, ssig = sig * root(count);
        int n = add(out, 0, "For a total of independent normal variables, add means and variances.");
        n = add(out, n, "T ~ N(n*mu, n*sigma^2).");
        n = add(out, n, "mean = %.6g*%.6g = %.10g", count, mu, smu);
        n = add(out, n, "sd = sqrt(%.6g)*%.6g = %.10g", count, sig, ssig);
        n = add(out, n, "z = (%.6g-%.10g)/%.10g = %.10g", bound, smu, ssig, (bound-smu)/ssig);
        if (tail > 0) return add(out, n, "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.10g, mu=%.10g)", bound, ssig, smu);
        return add(out, n, "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.10g, mu=%.10g)", bound, ssig, smu);
      }
    }
    if (has(t, "sample") && has(t, "mean") && hMu && hSig && hN &&
        (has(t, "probability") || has(t, "find")) && !has(t, "hypothesis") && !has(t, "test")) {
      if (has(t, "between") || has(c, "<xbar<") || has(c, "<samplemean<")) {
        double bounds[2]; int bc = 0;
        for (int i = 0; i < nv && bc < 2; ++i) {
          if (has(c, "^2") && near_num(v[i], 2)) continue;
          if (near_num(v[i], mu) || near_num(v[i], sig) || near_num(v[i], n0)) continue;
          bounds[bc++] = v[i];
        }
        if (bc >= 2) {
          double lo2 = bounds[0] < bounds[1] ? bounds[0] : bounds[1];
          double hi2 = bounds[0] < bounds[1] ? bounds[1] : bounds[0];
          double se = sig / root(n0);
          int n = add(out, 0, "For a sample mean, Xbar ~ N(mu, (sigma/sqrt(n))^2).");
          n = add(out, n, "standard error = %.6g/sqrt(%.6g) = %.10g", sig, n0, se);
          n = add(out, n, "standardise both bounds: z1=(%.6g-%.6g)/%.10g, z2=(%.6g-%.6g)/%.10g", lo2, mu, se, hi2, mu, se);
          return add(out, n, "Use NormalCD(lower=%.6g, upper=%.6g, sigma=%.10g, mu=%.6g)", lo2, hi2, se, mu);
        }
      }
      double bound = 0; bool hb = false;
      for (int i = 0; i < nv; ++i) {
        if (has(c, "^2") && near_num(v[i], 2)) continue;
        if (near_num(v[i], mu) || near_num(v[i], sig) || near_num(v[i], n0)) continue;
        bound = v[i]; hb = true;
      }
      if (hb) {
        double se = sig / root(n0);
        int n = add(out, 0, "For a sample mean, Xbar ~ N(mu, (sigma/sqrt(n))^2).");
        n = add(out, n, "standard error = %.6g/sqrt(%.6g) = %.10g", sig, n0, se);
        n = add(out, n, "z = (%.6g-%.6g)/%.10g = %.10g", bound, mu, se, (bound-mu)/se);
        if (tail > 0) {
          n = add(out, n, "Required probability is P(Xbar>=%.6g).", bound);
          return add(out, n, "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.10g, mu=%.6g)", bound, se, mu);
        }
        n = add(out, n, "Required probability is P(Xbar<=%.6g).", bound);
        return add(out, n, "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.10g, mu=%.6g)", bound, se, mu);
      }
    }
    if (hArea && has(t, "percentile") && area > 1 && area <= 100) area /= 100.0;
    if (!hArea && hMu && (hSig || hVar) &&
        (has(c, "findk") || has(c, "findx") || has(c, "suchthat") || has(t, "percentile"))) {
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], mu) || (hSig && near_num(v[i], sig)) || (hVar && near_num(v[i], var))) continue;
        if (v[i] > 0 && (v[i] < 1 || has(t, "percentile"))) {
          area = (has(t, "percentile") && v[i] > 1) ? v[i] / 100.0 : v[i];
          hArea = true;
          break;
        }
      }
      if (hArea && (has(c, "morethank") || has(c, "greaterthank") || has(c, "p(x>k") || has(c, "p(x>=k"))) area = 1 - area;
      if (hArea && (has(c, "p(x>a") || has(c, "p(x>=a") || has(c, "p(x>q") || has(c, "p(x>=q"))) area = 1 - area;
    }
    if (hArea && hMu && (hSig || hVar) &&
        (has(c, "findk") || has(c, "findx") || has(c, "suchthat") || has(t, "percentile") ||
         has(c, "invnormal") || has(c, "inversenormal") || has(t, "critical"))) {
      sprintf(cmd, hSig ? "invnormal(%.10g,%.10g,%.10g)" : "invnormalvar(%.10g,%.10g,%.10g)", area, mu, hSig ? sig : var);
      return eval_stats(cmd, out);
    }
    if (hMu && (hSig || hVar) && nv >= 2 && (has(t, "between") || has(c, "lessthan") || has(c, "<x<"))) {
      double u[4]; int nu = 0;
      for (int i = 0; i < nv && nu < 4; ++i) {
        if (near_num(v[i], mu) || (hSig && near_num(v[i], sig)) || (hVar && near_num(v[i], var))) continue;
        u[nu++] = v[i];
      }
      if (nu >= 2) {
        double lo2 = u[0] < u[1] ? u[0] : u[1], hi2 = u[0] < u[1] ? u[1] : u[0];
        sprintf(cmd, hSig ? "normalprob(%.10g,%.10g,%.10g,%.10g)" : "normalprobvar(%.10g,%.10g,%.10g,%.10g)", lo2, hi2, mu, hSig ? sig : var);
        return eval_stats(cmd, out);
      }
    }
    if (!hX && hMu && (hSig || hVar) && nv >= 1 &&
        (has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") || has(c, "exceed") ||
         has(c, "above") || has(c, "atleast") || has(c, "lessthan") || has(c, "below") ||
         has(c, "under") || has(c, "atmost"))) {
      for (int i = 0; i < nv; ++i) {
        if (near_num(v[i], mu) || (hSig && near_num(v[i], sig)) || (hVar && near_num(v[i], var))) continue;
        x = v[i]; hX = true; break;
      }
      if (!hX) { x = v[0]; hX = true; }
    }
    if (hXb && hMu && (hSig || hVar) && hN && hA && (has(t, "hypothesis") || has(t, "test"))) {
      sprintf(cmd, "hypnormal(%.10g,%.10g,%.10g,%.10g,%.10g,%.0f)", xb, mu, hSig ? sig : root(var), n0, alpha, tail);
      return eval_stats(cmd, out);
    }
    if (hLo && hHi && hMu && (hSig || hVar)) {
      sprintf(cmd, hSig ? "normalprob(%.10g,%.10g,%.10g,%.10g)" : "normalprobvar(%.10g,%.10g,%.10g,%.10g)", lo, hi, mu, hSig ? sig : var);
      return eval_stats(cmd, out);
    }
    if (hMu && (hSig || hVar) && strchr(input, '|')) {
      double sd = hSig ? sig : root(var), x0 = 0, g0 = 0;
      char rawbar[160]; clean(input, rawbar, sizeof(rawbar));
      const char *bar = strchr(rawbar, '|');
      const char *lp = strstr(rawbar, "x>"), *rp = bar ? strstr(bar, "x>") : 0;
      if (lp && rp && lp < bar) {
        x0 = read_num(lp + (lp[2] == '=' ? 3 : 2));
        g0 = read_num(rp + (rp[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,1)", x0, g0, mu, sd);
        return eval_stats(cmd, out);
      }
      lp = strstr(rawbar, "x<"); rp = bar ? strstr(bar, "x<") : 0;
      if (lp && rp && lp < bar) {
        x0 = read_num(lp + (lp[2] == '=' ? 3 : 2));
        g0 = read_num(rp + (rp[2] == '=' ? 3 : 2));
        sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,-1)", x0, g0, mu, sd);
        return eval_stats(cmd, out);
      }
    }
    if (hX && hMu && (hSig || hVar) && !(has(t, "conditional") || has(t, "given")) &&
        (has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") || has(c, "exceed") ||
         has(c, "above") || has(c, "atleast") || has(c, "lessthan") || has(c, "below") ||
         has(c, "under") || has(c, "atmost"))) {
      double ttail = (has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") ||
                      has(c, "exceed") || has(c, "above") || has(c, "atleast")) ? 1 : -1;
      sprintf(cmd, hSig ? "normaltail(%.10g,%.10g,%.10g,%.0f)" : "normaltailvar(%.10g,%.10g,%.10g,%.0f)", x, mu, hSig ? sig : var, ttail);
      return eval_stats(cmd, out);
    }
    if (hX && hMu && (hSig || hVar) && (has(t, "standardise") || has(t, "standardize") || has(t, "zscore"))) {
      sprintf(cmd, hSig ? "normal(%.10g,%.10g,%.10g)" : "normalvar(%.10g,%.10g,%.10g)", x, mu, hSig ? sig : var);
      return eval_stats(cmd, out);
    }
    if (hArea && hMu && (hSig || hVar) && (has(c, "invnormal") || has(c, "inversenormal") || has(t, "critical") || has(t, "percentile"))) {
      sprintf(cmd, hSig ? "invnormal(%.10g,%.10g,%.10g)" : "invnormalvar(%.10g,%.10g,%.10g)", area, mu, hSig ? sig : var);
      return eval_stats(cmd, out);
    }
  }
  if ((has(t, "critical") || has(t, "criticalregion")) && has(t, "binom") && nv >= 3) {
    double tail = has(t, "upper") ? 1 : -1;
    sprintf(cmd, "critbinom(%d,%.10g,%.10g,%.0f)", (int)v[0], v[1], v[2], tail); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && has(t, "normal") && (has(t, "approx") || has(t, "approximation")) && nv >= 4) {
    sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && has(t, "poisson") && (has(t, "approx") || has(t, "approximation")) && nv >= 3) {
    int tail = prob_tail(c, t);
    if (has(c, "nomorethan") || has(t, "cdf")) tail = -1;
    sprintf(cmd, "poissonapprox(%d,%.10g,%d,%d)", (int)v[0], v[1], (int)v[2], tail); return eval_stats(cmd, out);
  }
  bool ask_normal_params = has(t, "parameters") || has(t, "meansd") || has(t, "meanandsd") ||
    has(c, "findmeanandsd") || has(c, "findthemeanandsd") ||
    has(c, "findmeanandstandarddeviation") || has(c, "findthemeanandstandarddeviation");
  if (has(t, "normal") && ask_normal_params && nv >= 4) {
    sprintf(cmd, "normalparams(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "conditional") || has(t, "given")) && nv >= 4) {
    double tail = (has(c, "lessthan") || has(c, "atmost") || has(c, "<")) ? -1 : 1;
    sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], v[3], tail); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "find") || has(t, "given") || has(t, "parameters")) &&
      (has(t, "meansd") || has(t, "meanandsd") ||
      (has(t, "mean") && (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd"))) ||
      has(t, "parameters")) && nv >= 4) {
    sprintf(cmd, "normalparams(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if ((has(t, "normaldistribution") || has(t, "normalcdf") || (has(t, "normal") && has(t, "between"))) && has(t, "variance") && nv >= 4) {
    sprintf(cmd, "normalprobvar(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if ((has(t, "normaldistribution") || has(t, "normalcdf") || (has(t, "normal") && has(t, "between"))) && nv >= 4) {
    sprintf(cmd, "normalprob(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  double bx = 0; int btail = 0; bool hBound = prob_x_bound(c, &bx, &btail);
  if (has(t, "normal") && has(t, "variance") &&
      (hBound || has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") || has(c, "exceed") ||
       has(c, "above") || has(c, "atleast") || has(c, "lessthan") || has(c, "below") ||
       has(c, "under") || has(c, "atmost")) && nv >= 3) {
    double x = hBound ? bx : v[0], mu = 0, var = 0;
    bool hMu = label_num(input, "mean", &mu) || word_num(input, "mean", &mu);
    bool hVar = label_num(input, "variance", &var) || word_num(input, "variance", &var);
    if (!hMu) mu = hBound ? v[0] : v[1];
    if (!hVar) var = hBound ? v[1] : v[2];
    if (!hBound) for (int i = 0; i < nv; ++i) if (!near_num(v[i], mu) && !near_num(v[i], var)) { x = v[i]; break; }
    double tail = hBound ? btail : ((has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") ||
                                     has(c, "exceed") || has(c, "above") || has(c, "atleast")) ? 1 : -1);
    sprintf(cmd, "normaltailvar(%.10g,%.10g,%.10g,%.0f)", x, mu, var, tail); return eval_stats(cmd, out);
  }
  if (has(t, "normal") &&
      (hBound || has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") || has(c, "exceed") ||
       has(c, "above") || has(c, "atleast") || has(c, "lessthan") || has(c, "below") ||
       has(c, "under") || has(c, "atmost")) && nv >= 3) {
    double x = hBound ? bx : v[0], mu = 0, sig = 0;
    bool hMu = label_num(input, "mean", &mu) || word_num(input, "mean", &mu);
    bool hSig = label_num(input, "sd", &sig) || label_num(input, "sigma", &sig) || label_num(input, "standarddeviation", &sig) ||
                word_num(input, "sd", &sig) || word_num(input, "sigma", &sig) || word_num(input, "standarddeviation", &sig);
    if (!hMu) mu = hBound ? v[0] : v[1];
    if (!hSig) sig = hBound ? v[1] : v[2];
    if (!hBound) for (int i = 0; i < nv; ++i) if (!near_num(v[i], mu) && !near_num(v[i], sig)) { x = v[i]; break; }
    double tail = hBound ? btail : ((has(c, "morethan") || has(c, "greaterthan") || has(c, "exceeds") ||
                                     has(c, "exceed") || has(c, "above") || has(c, "atleast")) ? 1 : -1);
    sprintf(cmd, "normaltail(%.10g,%.10g,%.10g,%.0f)", x, mu, sig, tail); return eval_stats(cmd, out);
  }
  if ((has(c, "invnormal") || has(c, "inversenormal") || (has(c, "normal") && (has(c, "critical") || has(c, "percentile")))) && has(t, "variance") && nv >= 3) {
    sprintf(cmd, "invnormalvar(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(c, "invnormal") || has(c, "inversenormal") || (has(c, "normal") && (has(c, "critical") || has(c, "percentile")))) && nv >= 3) {
    sprintf(cmd, "invnormal(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "outlier") || has(t, "iqr") || has(t, "fence")) && nv >= 2) {
    if ((has(t, "data") || has(t, "values")) && !has(t, "q1") && !has(t, "q3") && nv >= 5) {
      double a[32]; int m = nv < 32 ? nv : 32;
      for (int i = 0; i < m; ++i) a[i] = v[i];
      sort_doubles(a, m);
      double q1 = median_slice(a, 0, m / 2);
      double q3 = median_slice(a, (m + 1) / 2, m);
      double iqr = q3 - q1, lo = q1 - 1.5 * iqr, hi = q3 + 1.5 * iqr;
      int n = add(out, 0, "For raw data, sort the values and find quartiles first.");
      n = add(out, n, "Q1 = %.10g, Q3 = %.10g", q1, q3);
      n = add(out, n, "IQR = %.10g", iqr);
      n = add(out, n, "outlier fences: %.10g and %.10g", lo, hi);
      bool any = false;
      for (int i = 0; i < m; ++i) if (a[i] < lo || a[i] > hi) {
        n = add(out, n, "%.10g is an outlier", a[i]);
        any = true;
      }
      return add(out, n, any ? "Outliers listed above." : "No outliers.");
    }
    double q1=0, q3=0, mn=0, mx=0;
    bool hq1=label_num(input,"q1",&q1) || word_num(input,"q1",&q1);
    bool hq3=label_num(input,"q3",&q3) || word_num(input,"q3",&q3);
    bool hmn=label_num(input,"min",&mn) || label_num(input,"minimum",&mn) || word_num(input,"min",&mn);
    bool hmx=label_num(input,"max",&mx) || label_num(input,"maximum",&mx) || word_num(input,"max",&mx);
    if (hq1 && hq3 && (hmn || hmx)) {
      int p2 = sprintf(cmd, "outliers(%.10g,%.10g", q1, q3);
      if (hmn) p2 += sprintf(cmd+p2, ",%.10g", mn);
      if (hmx) p2 += sprintf(cmd+p2, ",%.10g", mx);
      sprintf(cmd+p2, ")");
      return eval_stats(cmd, out);
    }
    int p = sprintf(cmd, "outliers(%.10g,%.10g", hq1 ? q1 : v[0], hq3 ? q3 : v[1]);
    int start = (hq1 && hq3) ? 0 : 2;
    for (int i = start; i < nv && p < (int)sizeof(cmd)-24; ++i) {
      if ((hq1 && v[i] == q1) || (hq3 && v[i] == q3)) continue;
      p += sprintf(cmd+p, ",%.10g", v[i]);
    }
    sprintf(cmd+p, ")");
    return eval_stats(cmd, out);
  }
  if ((has(t, "median") || has(t, "quartile") || has(t, "interquartile") || has(t, "iqr")) &&
      (has(t, "data") || has(t, "values") || has(t, "coding") || has(t, "questionnaire") || has(t, "observations")) &&
      !has(t, "grouped") && !has(t, "group") && !has(t, "class") && !has(t, "classes") &&
      !has(t, "frequency") && !has(t, "frequencies") && !has(t, "cumulative") && nv >= 3) {
    double a[32]; int m = nv < 32 ? nv : 32;
    for (int i = 0; i < m; ++i) a[i] = v[i];
    sort_doubles(a, m);
    double med = median_slice(a, 0, m);
    double q1 = median_slice(a, 0, m / 2);
    double q3 = median_slice(a, (m + 1) / 2, m);
    int n = add(out, 0, "Sort the raw data, then read the required positions.");
    n = add(out, n, "median = %.10g", med);
    n = add(out, n, "Q1 = %.10g, Q3 = %.10g", q1, q3);
    return add(out, n, "IQR = Q3 - Q1 = %.10g", q3 - q1);
  }
  if (has(t, "normal") && (has(t, "hypothesis") || has(t, "test")) && nv >= 5) {
    double tail = (has(t, "twotailed") || has(t, "twosided") || (has(t, "two") && (has(t, "tailed") || has(t, "sided"))) || has(t, "different") || has(t, "notequal")) ? 0 :
                  (has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 : -1);
    sprintf(cmd, "hypnormal(%.10g,%.10g,%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], v[3], v[4], tail); return eval_stats(cmd, out);
  }
  if ((has(t, "standardise") || has(t, "standardize") || has(t, "zscore")) && has(t, "variance") && nv >= 3) {
    sprintf(cmd, "normalvar(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "standardise") || has(t, "standardize") || has(t, "zscore")) && nv >= 3) {
    sprintf(cmd, "normal(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "hypothesis") || has(t, "significance") || has(t, "binomtest")) && has(t, "binom") && nv >= 4) {
    double tail = has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 :
                  ((has(t, "lower") || has(t, "less") || has(t, "fewer") || has(t, "smaller")) ? -1 :
                   (v[2] > v[0] * v[1] ? 1 : -1));
    sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", (int)v[0], v[1], (int)v[2], v[3], tail); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && (has(t, "mean") || has(t, "variance") || has(t, "standarddeviation") || has(t, "sd")) && nv >= 2) {
    sprintf(cmd, "binomstats(%d,%.10g)", (int)v[0], v[1]); return eval_stats(cmd, out);
  }
  if ((has(t, "bayes") || has(t, "reverseconditional")) && nv >= 3) {
    sprintf(cmd, "bayes(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
	  if ((has(t, "trapezium") || has(t, "trapezoid") || has(t, "trapezoidal")) && nv >= 3) {
	    if ((has(c, "x^2") || has(c, "x^3") || has(c, "ofx")) &&
	        (has(t, "strip") || has(t, "strips")) && (has(t, "from") && has(t, "to"))) {
	      double lo=0, hi=0, strips=0;
	      bool hLo = word_num(input, "from", &lo);
	      bool hHi = word_num(input, "to", &hi);
	      bool hN = prev_word_num(input, "strips", &strips) || word_num(input, "strips", &strips);
	      if (hLo && hHi && hN && strips > 0) {
	        bool recip_quad = has(c, "1/(1+x^2)") || has(c, "1/(x^2+1)");
	        int pwrx = has(c, "x^3") ? 3 : (has(c, "x^2") && !recip_quad ? 2 : 1);
	        double h = (hi - lo) / strips, first = 0, last = 0, middle = 0;
	        for (int i = 0; i <= (int)strips; ++i) {
	          double x = lo + i*h;
	          double y = recip_quad ? 1.0/(1.0+x*x) : (pwrx == 3 ? x*x*x : (pwrx == 2 ? x*x : x));
	          if (i == 0) first = y;
	          else if (i == (int)strips) last = y;
	          else middle += y;
	        }
	        double area = h/2.0*(first + last + 2*middle);
	        int n = add(out, 0, "Use the trapezium rule with %d strips.", (int)strips);
	        n = add(out, n, "h = (%.10g-%.10g)/%d = %.10g", hi, lo, (int)strips, h);
	        if (recip_quad) n = add(out, n, "For y=1/(1+x^2), first y = %.10g, last y = %.10g, middle sum = %.10g", first, last, middle);
	        else n = add(out, n, "For y=x^%d, first y = %.10g, last y = %.10g, middle sum = %.10g", pwrx, first, last, middle);
	        return add(out, n, "Area = %.10g/2*(%.10g+%.10g+2*%.10g) = %.10g", h, first, last, middle, area);
	      }
	    }
	    if (has(c, "xvalues") && has(c, "yvalues") && nv >= 4 && nv % 2 == 0) {
      int m = nv / 2;
      double h = v[1] - v[0];
      if (h < 0) h = -h;
      double sum = v[m] + v[nv - 1];
      for (int i = m + 1; i < nv - 1; ++i) sum += 2 * v[i];
      double area = h * sum / 2.0;
      int n = add(out, 0, "Use the trapezium rule with the y-values.");
      n = add(out, n, "h = x2 - x1 = %.10g - %.10g = %.10g", v[1], v[0], h);
      n = add(out, n, "Area = h/2*(first y + last y + 2*sum of middle y-values)");
      return add(out, n, "Area = %.10g/2 * %.10g = %.10g", h, sum, area);
    }
    double h = v[nv - 1], sum = v[0] + v[nv - 2];
    for (int i = 1; i < nv - 2; ++i) sum += 2 * v[i];
    double area = h * sum / 2.0;
    int n = add(out, 0, "Use the trapezium rule.");
    n = add(out, n, "Area = h/2*(first + last + 2*sum of middle ordinates)");
    n = add(out, n, "h = %.10g", h);
    return add(out, n, "Area = %.10g/2 * %.10g = %.10g", h, sum, area);
  }
  if ((has(t, "coin") || has(t, "heads") || has(t, "head")) &&
      (has(c, "p(noheads)=") || has(c, "p(nohead)=")) && has(t, "find") && nv >= 2) {
    double q = v[nv - 1], trials = 0;
    for (int i = 0; i < nv - 1; ++i) if (v[i] >= 1 && !near_num(v[i], q)) { trials = v[i]; break; }
    if (trials > 0 && q > 0 && q < 1) {
      double fail = exp_approx(ln_approx(q) / trials);
      double p = 1.0 - fail;
      int n = add(out, 0, "If p is the probability of heads, then no heads means all tosses are tails.");
      n = add(out, n, "P(no heads) = (1-p)^n");
      n = add(out, n, "(1-p)^%.0f = %.10g", trials, q);
      n = add(out, n, "1-p = %.10g^(1/%.0f) = %.10g", q, trials, fail);
      return add(out, n, "p = %.10g", p);
    }
  }
  if (has(t, "geometric") && nv >= 2) {
    double p = 0, r = 0;
    bool hp = label_num(input, "p", &p) || word_num(input, "p", &p) ||
              word_num(input, "probability", &p);
    if (!hp) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { p = v[i]; hp = true; break; }
    int tail = prob_tail(c, t);
    if (!prob_x_bound(c, &r, &tail)) {
      for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], p) && v[i] >= 1) { r = v[i]; break; }
    }
    if (p > 0 && p < 1 && r >= 1) {
      int n = add(out, 0, "For X geometric, X is the trial number of the first success.");
      n = add(out, n, "P(X=r)=(1-p)^(r-1)p and P(X>r)=(1-p)^r.");
      if (tail > 0) {
        double prob = tail == 2 ? pwr(1 - p, (int)r) : pwr(1 - p, (int)r - 1);
        return add(out, n, "P(X%s%.0f)=(1-%.6g)^%d = %.10g", tail == 2 ? ">" : ">=", r, p, tail == 2 ? (int)r : (int)r - 1, prob);
      }
      if (tail < 0) {
        double prob = tail == -2 ? 1 - pwr(1 - p, (int)r - 1) : 1 - pwr(1 - p, (int)r);
        return add(out, n, "P(X%s%.0f)=1-(1-%.6g)^%d = %.10g", tail == -2 ? "<" : "<=", r, p, tail == -2 ? (int)r - 1 : (int)r, prob);
      }
      double prob = pwr(1 - p, (int)r - 1) * p;
      return add(out, n, "P(X=%.0f)=(1-%.6g)^%.0f*%.6g = %.10g", r, p, r - 1, p, prob);
    }
  }
  if ((has(t, "firsthead") || (has(t, "first") && (has(t, "head") || has(t, "win") || has(t, "success"))) || has(t, "geometric")) &&
      (has(t, "coin") || has(t, "toss") || has(t, "success") || has(t, "win") || has(t, "attempt")) &&
      (has(t, "after") || has(t, "morethan")) && nv >= 2) {
    double p = 0, r = 0;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { p = v[i]; break; }
    for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], p) && v[i] >= 1) { r = v[i]; break; }
    double prob = pwr(1 - p, (int)r);
    int n = add(out, 0, "For a geometric distribution, first success after r attempts means no success in the first r attempts.");
    n = add(out, n, "P(X>r)=(1-p)^r");
    return add(out, n, "P(X>%.0f)=(1-%.6g)^%.0f = %.10g", r, p, r, prob);
  }
  if ((has(t, "die") || has(t, "dice")) && has(t, "six") && has(t, "first") &&
      (has(t, "after") || has(t, "morethan")) && nv >= 1) {
    double r = 0;
    for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], 6) && v[i] >= 1) { r = v[i]; break; }
    double p = 1.0 / 6.0;
    double prob = pwr(1 - p, (int)r);
    int n = add(out, 0, "For a die, success means rolling a six, so p=1/6.");
    n = add(out, n, "First six after r throws means no six in the first r throws.");
    return add(out, n, "P(X>%.0f)=(5/6)^%.0f = %.10g", r, r, prob);
  }
  if (((has(t, "first") && (has(t, "success") || has(t, "defective") || has(t, "failure") || has(t, "win") || has(t, "head"))) || has(t, "geometric")) &&
      (has(t, "before") || has(t, "within") || has(t, "by") || has(c, "onorbefore") || has(c, "onbefore")) && nv >= 2) {
    double p = 0, r = 0;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { p = v[i]; break; }
    for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], p) && v[i] >= 1) { r = v[i]; break; }
    double prob = 1 - pwr(1 - p, (int)r);
    int n = add(out, 0, "For a geometric distribution, first success on or before r means at least one success.");
    n = add(out, n, "P(X<=r)=1-P(no success in first r trials)");
    return add(out, n, "P(X<=%.0f)=1-(1-%.6g)^%.0f = %.10g", r, p, r, prob);
  }
  if ((has(t, "firsthead") || (has(t, "first") && (has(t, "head") || has(t, "win") || has(t, "success"))) || has(t, "geometric")) &&
      (has(t, "coin") || has(t, "toss") || has(t, "success") || has(t, "win") || has(t, "attempt")) && nv >= 2) {
    double p = 0, r = 0;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { p = v[i]; break; }
    for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], p) && v[i] >= 1) { r = v[i]; break; }
    double prob = pwr(1 - p, (int)r - 1) * p;
    int n = add(out, 0, "For the first success on trial r, use the geometric distribution.");
    n = add(out, n, "P(X=r)=(1-p)^(r-1)p");
    return add(out, n, "P(X=%.0f)=(1-%.6g)^%.0f*%.6g = %.10g", r, p, r - 1, p, prob);
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen")) &&
      (has(t, "alldifferent") || (has(t, "all") && has(t, "different"))) && nv >= 3) {
    double counts[8]; int cc = 0;
    const char *cols[] = {"red","blue","green","yellow","white","black","orange","purple"};
    for (int i = 0; i < 8; ++i) {
      double q = 0;
      if (prev_word_num(input, cols[i], &q) && q > 0) counts[cc++] = q;
    }
    if (cc < 3) {
      cc = 0;
      for (int i = 0; i < nv && cc < 8; ++i) if (v[i] > 0) counts[cc++] = v[i];
    }
    double total = 0, ways = 0;
    for (int i = 0; i < cc; ++i) total += counts[i];
    if (cc >= 3) {
      for (int i = 0; i < cc; ++i) for (int j = i + 1; j < cc; ++j) for (int k = j + 1; k < cc; ++k)
        ways += counts[i] * counts[j] * counts[k];
      double all = choose((int)total, 3);
      int n = add(out, 0, "Without replacement, use combinations for unordered selections.");
      n = add(out, n, "all-different ways = sum of products from different colour groups = %.10g", ways);
      n = add(out, n, "total ways = C(%.0f,3) = %.10g", total, all);
      return add(out, n, "P(all different)=%.10g/%.10g=%.10g", ways, all, all ? ways/all : 0);
    }
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen") || has(t, "selected")) &&
      (has(t, "until") || has(t, "obtained") || has(t, "first")) && nv >= 2) {
    double target = 0, other = 0, r = 0;
    bool blue_target = has(c, "blueisfirst") || has(c, "blueisobtained") || has(c, "blueobtained") ||
                       has(c, "firstblue") || has(c, "untilablue") || has(c, "probabilityblue");
    bool red_target = has(c, "redisfirst") || has(c, "redisobtained") || has(c, "redobtained") ||
                      has(c, "firstred") || has(c, "untilared") || has(c, "probabilityred");
    bool htgt = blue_target ? prev_word_num(input, "blue", &target) :
                (prev_word_num(input, "red", &target) || prev_word_num(input, "success", &target));
    bool hoth = blue_target ? prev_word_num(input, "red", &other) :
                (prev_word_num(input, "blue", &other) || prev_word_num(input, "notred", &other) || prev_word_num(input, "other", &other));
    if (red_target && !htgt) htgt = prev_word_num(input, "red", &target);
    if (!htgt) target = v[0];
    if (!hoth) other = v[1];
    for (int i = nv - 1; i >= 0; --i) if (!near_num(v[i], target) && !near_num(v[i], other) && v[i] >= 1) { r = v[i]; break; }
    if (r == 0) {
      if (has(t, "sixth")) r = 6;
      else if (has(t, "fifth")) r = 5;
      else if (has(t, "fourth")) r = 4;
      else if (has(t, "third")) r = 3;
      else if (has(t, "second")) r = 2;
      else if (has(t, "first")) r = 1;
    }
    if (target > 0 && other > 0 && r >= 1) {
      double total = target + other, prob = 1.0;
      int n = add(out, 0, "Without replacement until first target: first r-1 draws are non-target, then target.");
      for (int i = 0; i < (int)r - 1; ++i) {
        n = add(out, n, "P(non-target on draw %d) = %.10g/%.10g", i + 1, other - i, total - i);
        prob *= (other - i) / (total - i);
      }
      n = add(out, n, "P(target on draw %.0f) = %.10g/%.10g", r, target, total - r + 1);
      prob *= target / (total - r + 1);
      return add(out, n, "P(first target on draw %.0f) = %.10g", r, prob);
    }
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen")) &&
      has(t, "both") && nv >= 2) {
    double a = 0, b = 0;
    bool ha = prev_word_num(input, "red", &a) || prev_word_num(input, "success", &a);
    bool hb = prev_word_num(input, "blue", &b) || prev_word_num(input, "other", &b);
    if (!ha) a = v[0];
    if (!hb) b = v[1];
    double total = a + b;
    double prob = total > 1 ? (a/total) * ((a-1)/(total-1)) : 0;
    int n = add(out, 0, "Without replacement, probabilities change after each draw.");
    n = add(out, n, "P(first target) = %.10g/%.10g", a, total);
    n = add(out, n, "P(second target | first target) = %.10g/%.10g", a-1, total-1);
    return add(out, n, "P(both target) = %.10g/%.10g * %.10g/%.10g = %.10g", a, total, a-1, total-1, prob);
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen") || has(t, "selected")) &&
      (has(t, "differentcolour") || has(t, "differentcolor") || (has(t, "different") && (has(t, "colour") || has(t, "color")))) &&
      nv >= 2) {
    double counts[8]; int cc = 0;
    const char *cols[] = {"red","blue","green","yellow","white","black","orange","purple"};
    for (int i = 0; i < 8; ++i) {
      double q = 0;
      if (prev_word_num(input, cols[i], &q) && q > 0) counts[cc++] = q;
    }
    if (cc < 2) {
      cc = 0;
      for (int i = 0; i < nv && cc < 8; ++i) if (v[i] > 0) counts[cc++] = v[i];
    }
    double total = 0, same = 0;
    for (int i = 0; i < cc; ++i) { total += counts[i]; same += choose((int)counts[i], 2); }
    double all = choose((int)total, 2), diff = all - same;
    int n = add(out, 0, "For two without replacement, different colour is the complement of same colour.");
    n = add(out, n, "same-colour ways = sum C(group size,2) = %.10g", same);
    n = add(out, n, "total ways = C(%.0f,2) = %.10g", total, all);
    return add(out, n, "P(different colour)=%.10g/%.10g=%.10g", diff, all, all ? diff/all : 0);
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen") || has(t, "selected")) &&
      (has(c, "exactlytwo") || has(c, "exactly2") || (has(t, "exactly") && has(t, "two"))) && nv >= 2) {
    double counts[8]; int cc = 0; const char *target_name = "target"; double target = 0;
    const char *cols[] = {"red","blue","green","yellow","white","black","orange","purple"};
    int requested = -1;
    for (int i = 0; i < 8; ++i) {
      char key1[48], key2[48];
      sprintf(key1, "exactlytwo%s", cols[i]);
      sprintf(key2, "exactly2%s", cols[i]);
      if (has(c, key1) || has(c, key2)) { requested = i; break; }
    }
    for (int i = 0; i < 8; ++i) {
      double q = 0;
      if (prev_word_num(input, cols[i], &q) && q > 0) {
        counts[cc++] = q;
        if (!target && (requested == i || (requested < 0 && i == 0))) {
          target = q; target_name = cols[i];
        }
      }
    }
    if (cc < 2) {
      cc = 0;
      for (int i = 0; i < nv && cc < 8; ++i) if (v[i] > 0) counts[cc++] = v[i];
      if (!target && cc) target = counts[0];
    }
    double total = 0;
    for (int i = 0; i < cc; ++i) total += counts[i];
    if (!target && cc) target = counts[0];
    double other = total - target;
    double ways = choose((int)target, 2) * other;
    double all = choose((int)total, 3);
    int n = add(out, 0, "Without replacement and order not specified, use combinations.");
    n = add(out, n, "exactly two %s ways = C(%.0f,2)*%.0f = %.10g", target_name, target, other, ways);
    n = add(out, n, "total ways = C(%.0f,3) = %.10g", total, all);
    return add(out, n, "P(exactly two %s)=%.10g/%.10g=%.10g", target_name, ways, all, all ? ways/all : 0);
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen")) &&
      (has(t, "allthree") || (has(t, "all") && has(t, "three")) || has(t, "three")) && nv >= 2) {
    double counts[8]; int cc = 0; const char *target_name = "target";
    const char *cols[] = {"red","blue","green","yellow","white","black","orange","purple"};
    for (int i = 0; i < 8; ++i) {
      double q = 0;
      if (prev_word_num(input, cols[i], &q) && q > 0) { if (cc == 0) target_name = cols[i]; counts[cc++] = q; }
    }
    if (cc < 1) for (int i = 0; i < nv && cc < 8; ++i) if (v[i] > 0) counts[cc++] = v[i];
    double target = counts[0], total = 0;
    for (int i = 0; i < cc; ++i) total += counts[i];
    double prob = (total > 2) ? (target/total)*((target-1)/(total-1))*((target-2)/(total-2)) : 0;
    int n = add(out, 0, "Without replacement, probabilities change after each draw.");
    n = add(out, n, "P(first %s) = %.10g/%.10g", target_name, target, total);
    n = add(out, n, "P(second %s | first %s) = %.10g/%.10g", target_name, target_name, target-1, total-1);
    n = add(out, n, "P(third %s | first two %s) = %.10g/%.10g", target_name, target_name, target-2, total-2);
    return add(out, n, "P(all three %s) = %.10g", target_name, prob);
  }
  if ((has(t, "withoutreplacement") || (has(t, "without") && has(t, "replacement")) || has(t, "chosen")) &&
      (has(t, "samecolour") || has(t, "samecolor") || (has(t, "same") && (has(t, "colour") || has(t, "color")))) &&
      nv >= 2) {
    double counts[8]; int cc = 0;
    const char *cols[] = {"red","blue","green","yellow","white","black","orange","purple"};
    for (int i = 0; i < 8; ++i) {
      double q = 0;
      if (prev_word_num(input, cols[i], &q) && q > 0) counts[cc++] = q;
    }
    if (cc < 2) {
      cc = 0;
      for (int i = 0; i < nv && cc < 8; ++i) if (v[i] > 0) counts[cc++] = v[i];
    }
    double total = 0, ways_same = 0;
    char ways_line[80]; ways_line[0] = 0; int wp = 0;
    for (int i = 0; i < cc; ++i) {
      total += counts[i];
      ways_same += choose((int)counts[i], 2);
      if (wp < 70) {
        char term[18];
        sprintf(term, "%sC(%.0f,2)", i ? "+" : "", counts[i]);
        for (int j = 0; term[j] && wp < 78; ++j) ways_line[wp++] = term[j];
        ways_line[wp] = 0;
      }
    }
    double ways_all = choose((int)total, 2);
    int n = add(out, 0, "Without replacement, use combinations.");
    n = add(out, n, "same colour ways = %s = %.10g", ways_line, ways_same);
    n = add(out, n, "total ways = C(%.0f,2)", total);
    return add(out, n, "P(same colour)=%.10g/%.10g=%.10g", ways_same, ways_all, ways_all ? ways_same/ways_all : 0);
  }
  if ((has(t, "coded") || has(t, "coding") || has(c, "y=(x")) && has(t, "mean") &&
      has(t, "variance") && nv >= 4) {
    const char *p = strstr(c, "y=(x");
    if (p) {
      p += 4;
      double A = 0, B = 1;
      if (*p == '-') A = strtod(p + 1, (char **)&p);
      else if (*p == '+') A = -strtod(p + 1, (char **)&p);
      const char *div = strstr(p, ")/");
      if (div) B = strtod(div + 2, 0);
      double mean = v[nv - 2], var = v[nv - 1];
      bool wants_y = strstr(c, "findmeanofy") || strstr(c, "findthemeanofy") ||
                     strstr(c, "findmeanandvarianceofy") || strstr(c, "findthemeanandvarianceofy") ||
                     strstr(c, "meanandvarianceofy") || strstr(c, "meanofthecoded") ||
                     strstr(c, "codedmean");
      int n = add(out, 0, "For coded data Y=(X-a)/b, transform mean linearly and variance by b^2.");
      if (wants_y) {
        n = add(out, n, "mean Y = (mean X - a)/b");
        n = add(out, n, "mean Y = (%.6g-%.6g)/%.6g = %.10g", mean, A, B, B ? (mean-A)/B : 0);
        return add(out, n, "variance Y = variance X / b^2 = %.6g/%.6g^2 = %.10g", var, B, B ? var/(B*B) : 0);
      }
      n = add(out, n, "X = a + bY");
      n = add(out, n, "mean X = %.6g + %.6g*%.6g = %.10g", A, B, mean, A+B*mean);
      return add(out, n, "variance X = b^2*variance Y = %.6g^2*%.6g = %.10g", B, var, B*B*var);
    }
  }
  if ((has(t, "coded") || has(t, "coding") || has(c, "y=(x")) && has(t, "mean") &&
      (has(t, "sd") || has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation"))) && nv >= 4) {
    if (!coded_cmd_from_text(c, v, nv, cmd, sizeof(cmd))) {
      if (has(c, "y=(x-") || has(c, "y=(x+")) sprintf(cmd, "uncode(%.10g,%.10g,%.10g,%.10g)", v[2], v[3], -v[0], v[1]);
      else sprintf(cmd, "code(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]);
    }
    return eval_stats(cmd, out);
  }
  if ((has(t, "mutuallyexclusive") || has(t, "exclusive")) && (has(t, "or") || has(t, "union") || has(c, "p(aorb)")) && nv >= 2) {
    double pa = v[0], pb = v[1], por = pa + pb;
    int n = add(out, 0, "For mutually exclusive events, P(A and B)=0.");
    n = add(out, n, "P(A or B)=P(A)+P(B)");
    n = add(out, n, "P(A or B)=%.6g+%.6g=%.10g", pa, pb, por);
    if (has(t, "neither")) return add(out, n, "P(neither)=1-P(A or B)=1-%.10g=%.10g", por, 1 - por);
    return n;
  }
  if ((has(t, "probability") || has(c, "p(a)") || has(c, "p(b)")) &&
      (has(t, "union") || has(c, "aorb") || has(c, "p(aorb)")) &&
      !has(c, "p(aorb)=") &&
      !has(c, "agivenb") && !has(c, "p(a|b)") && !has(t, "conditional") &&
      (has(c, "aandb") || has(t, "and")) && nv >= 3) {
    double pa = v[0], pb = v[1], pab = v[2];
    double por = pa + pb - pab;
    int n = add(out, 0, "Use the addition rule for probabilities.");
    n = add(out, n, "P(A or B)=P(A)+P(B)-P(A and B)");
    n = add(out, n, "P(A or B)=%.6g+%.6g-%.6g=%.10g", pa, pb, pab, por);
    if (has(t, "independent") || has(t, "independence")) {
      double prod = pa * pb;
      n = add(out, n, "For independence, compare P(A and B) with P(A)P(B).");
      n = add(out, n, "P(A)P(B)=%.6g*%.6g=%.10g", pa, pb, prod);
      return add(out, n, abs_num(pab - prod) < 1e-9 ? "A and B are independent." : "A and B are not independent.");
    }
    return n;
  }
  if ((has(c, "bgivena") || has(c, "p(b|a)")) &&
      (has(c, "bgivennota") || has(c, "p(b|a')") || has(c, "p(b|nota)")) &&
      (has(c, "agivenb") || has(c, "p(a|b)") || has(t, "bayes") || has(t, "given")) && nv >= 3) {
    double pa = v[0], pba = v[1], pbna = v[2];
    double pna = 1 - pa;
    double pab = pa * pba;
    double pnb = pna * pbna;
    double pb = pab + pnb;
    int n = add(out, 0, "Use total probability, then Bayes' theorem.");
    n = add(out, n, "P(A and B)=P(A)P(B|A)=%.6g*%.6g=%.10g", pa, pba, pab);
    n = add(out, n, "P(A' and B)=P(A')P(B|A')=(1-%.6g)*%.6g=%.10g", pa, pbna, pnb);
    n = add(out, n, "P(B)=%.10g+%.10g=%.10g", pab, pnb, pb);
    return add(out, n, "P(A|B)=P(A and B)/P(B)=%.10g/%.10g=%.10g", pab, pb, pb ? pab/pb : 0);
  }
  if ((has(c, "p(aandb)=") || has(t, "intersection")) &&
      (has(c, "p(a|b)") || has(c, "agivenb") || has(t, "conditional") || has(t, "union")) && nv >= 3) {
    double pa = v[0], pb = v[1], pab = v[2];
    double cond = pb ? pab / pb : 0;
    double por = pa + pb - pab;
    int n = add(out, 0, "Use P(A|B)=P(A and B)/P(B).");
    n = add(out, n, "P(A|B)=%.6g/%.6g=%.10g", pab, pb, cond);
    n = add(out, n, "Use P(A or B)=P(A)+P(B)-P(A and B).");
    n = add(out, n, "P(A or B)=%.6g+%.6g-%.6g=%.10g", pa, pb, pab, por);
    if (has(t, "independent") || has(t, "independence")) {
      double prod = pa * pb;
      n = add(out, n, "For independence, compare P(A and B) with P(A)P(B).");
      n = add(out, n, "P(A)P(B)=%.6g*%.6g=%.10g", pa, pb, prod);
      return add(out, n, abs_num(pab - prod) < 1e-9 ? "A and B are independent." : "A and B are not independent.");
    }
    return n;
  }
  if ((has(c, "p(agivenb)=") || has(c, "p(a|b)=")) &&
      (has(c, "p(aandb)") || has(c, "p(aorb)") || has(t, "intersection") || has(t, "union") || has(t, "or")) &&
      nv >= 3) {
    double pa = v[0], pb = v[1], pagb = v[2];
    double pab = pagb * pb, por = pa + pb - pab;
    int n = add(out, 0, "Use P(A|B)=P(A and B)/P(B).");
    n = add(out, n, "P(A and B)=P(A|B)P(B)=%.6g*%.6g=%.10g", pagb, pb, pab);
    n = add(out, n, "Then use P(A or B)=P(A)+P(B)-P(A and B).");
    return add(out, n, "P(A or B)=%.6g+%.6g-%.10g=%.10g", pa, pb, pab, por);
  }
  if ((has(c, "p(aorb)") || has(t, "union")) &&
      (has(c, "agivenb") || has(c, "p(a|b)") || has(t, "conditional")) && nv >= 3) {
    double pa = v[0], pb = v[1], por = v[2];
    double pab = pa + pb - por;
    int n = add(out, 0, "Use P(A or B)=P(A)+P(B)-P(A and B).");
    n = add(out, n, "P(A and B)=%.6g+%.6g-%.6g=%.10g", pa, pb, por, pab);
    n = add(out, n, "Then use P(A|B)=P(A and B)/P(B).");
    return add(out, n, "P(A|B)=%.10g/%.6g=%.10g", pab, pb, pb ? pab/pb : 0);
  }
  if ((has(c, "givennotb") || has(c, "|b'") || has(c, "bcomplement")) && nv >= 3) {
    double pa = v[0], pb = v[1], pab = v[2];
    double nume = pa - pab, den = 1 - pb;
    int n = add(out, 0, "Use complements: P(A and B')=P(A)-P(A and B).");
    n = add(out, n, "P(A and B')=%.6g-%.6g=%.10g", pa, pab, nume);
    n = add(out, n, "P(B')=1-P(B)=1-%.6g=%.10g", pb, den);
    return add(out, n, "P(A|B')=%.10g/%.10g=%.10g", nume, den, den ? nume/den : 0);
  }
  if ((has(t, "independent") || has(t, "independence")) &&
      (has(c, "p(neither)=") || has(c, "p(neitheranorb)=") || has(c, "pneither=")) &&
      (has(c, "p(b)") || has(c, "findp(b)") || has(t, "find")) && nv >= 2) {
    double pa = v[0], pneither = v[1];
    double pb = 1 - pneither / (1 - pa);
    double pab = pa * pb;
    int n = add(out, 0, "For independent events, complements also multiply.");
    n = add(out, n, "P(neither) = P(A')P(B') = (1-P(A))(1-P(B)).");
    n = add(out, n, "%.10g = (1-%.10g)(1-P(B))", pneither, pa);
    n = add(out, n, "P(B) = 1 - %.10g/(1-%.10g) = %.10g", pneither, pa, pb);
    return add(out, n, "P(A and B)=P(A)P(B)=%.10g*%.10g=%.10g", pa, pb, pab);
  }
  if ((has(t, "independent") || has(t, "independence")) &&
      has(c, "p(aorb)=") &&
      !has(c, "p(b)=") && (has(c, "p(b)") || has(c, "findp(b)") || has(t, "find")) && nv >= 2 && nv < 3) {
    double pa = v[0], por = v[1];
    double pb = (1 - pa) ? (por - pa) / (1 - pa) : 0;
    double pab = pa * pb;
    int n = add(out, 0, "For independent events, P(A and B)=P(A)P(B).");
    n = add(out, n, "P(A or B)=P(A)+P(B)-P(A)P(B).");
    n = add(out, n, "%.10g = %.10g + P(B) - %.10g*P(B)", por, pa, pa);
    n = add(out, n, "P(B) = (%.10g-%.10g)/(1-%.10g) = %.10g", por, pa, pa, pb);
    return add(out, n, "P(A and B)=%.10g*%.10g=%.10g", pa, pb, pab);
  }
  if ((has(t, "independent") || has(t, "independence")) &&
      (has(t, "union") || has(t, "either") || has(c, "p(aorb)") || has(c, "p(aor b)") || has(t, " or ") || has(c, "p(aandb)")) && nv >= 2 && nv < 3) {
    double pa = v[0], pb = v[1], pab = pa * pb, por = pa + pb - pab;
    int n = add(out, 0, "For independent events, P(A and B)=P(A)P(B).");
    n = add(out, n, "P(A and B)=%.6g*%.6g=%.10g", pa, pb, pab);
    n = add(out, n, "P(A or B)=P(A)+P(B)-P(A and B)");
    n = add(out, n, "P(A or B)=%.6g+%.6g-%.10g=%.10g", pa, pb, pab, por);
    if (has(t, "neither")) return add(out, n, "P(neither)=1-P(A or B)=1-%.10g=%.10g", por, 1 - por);
    return n;
  }
  if ((has(t, "independent") || has(t, "independence")) &&
      (has(t, "union") || has(t, "either") || has(c, "p(aorb)") || has(c, "p(aor b)") || has(t, " or ")) && nv >= 3) {
    double pa = v[0], pb = v[1], pab = v[2];
    double por = pa + pb - pab, prod = pa * pb;
    int n = add(out, 0, "Use P(A or B)=P(A)+P(B)-P(A and B).");
    n = add(out, n, "P(A or B)=%.6g+%.6g-%.6g=%.10g", pa, pb, pab, por);
    n = add(out, n, "For independence, compare P(A and B) with P(A)P(B).");
    n = add(out, n, "P(A)P(B)=%.6g*%.6g=%.10g", pa, pb, prod);
    return add(out, n, abs_num(pab - prod) < 1e-9 ? "A and B are independent." : "A and B are not independent.");
  }
  if ((has(t, "independent") || has(t, "independence")) &&
      (has(c, "p(agivenb)=") || has(c, "p(a|b)=")) && nv >= 3) {
    double pa = v[0], pb = v[1], pagb = v[2];
    double pab = pagb * pb, prod = pa * pb;
    int n = add(out, 0, "Use P(A|B)=P(A and B)/P(B).");
    n = add(out, n, "P(A and B)=P(A|B)P(B)=%.6g*%.6g=%.10g", pagb, pb, pab);
    n = add(out, n, "For independence, compare P(A and B) with P(A)P(B).");
    n = add(out, n, "P(A)P(B)=%.6g*%.6g=%.10g", pa, pb, prod);
    return add(out, n, abs_num(pab - prod) < 1e-9 ? "A and B are independent." : "A and B are not independent.");
  }
  if ((has(t, "independent") || has(t, "independence")) && (has(t, "given") || has(t, "conditional")) && nv >= 3) {
    int n = add(out, 0, "Use P(A|B)=P(A and B)/P(B).");
    n = add(out, n, "P(A|B)=%.6g/%.6g=%.10g", v[2], v[1], v[1] ? v[2]/v[1] : 0);
    double prod = v[0]*v[1], diff = prod > v[2] ? prod - v[2] : v[2] - prod;
    n = add(out, n, "For independence, compare P(A and B) with P(A)P(B).");
    n = add(out, n, "P(A)P(B)=%.6g*%.6g=%.10g", v[0], v[1], prod);
    return add(out, n, diff < 1e-9 ? "A and B are independent." : "A and B are not independent.");
  }
  if ((has(t, "independent") || has(t, "independence")) && nv >= 3) {
    sprintf(cmd, "independent(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "conditional") || has(t, "given")) && nv >= 4) {
    double tail = (has(c, "lessthan") || has(c, "atmost") || has(c, "<")) ? -1 : 1;
    sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], v[3], tail); return eval_stats(cmd, out);
  }
  double fsxx=0, fsyy=0, fsxy=0;
  if ((has(t, "pmcc") || has(t, "correlation") || has(t, "significant")) &&
      label_num(input,"sxx",&fsxx) && label_num(input,"syy",&fsyy) && label_num(input,"sxy",&fsxy)) {
    double crit=0; bool hc=label_num(input,"critical",&crit) || label_num(input,"cv",&crit);
    if (!hc) for (int i = nv - 1; i >= 0; --i) if (v[i] > 0 && v[i] < 1) { crit = v[i]; hc = true; break; }
    double r = fsxy/root(fsxx*fsyy);
    if (hc) {
      int n = add(out, 0, "Use r = Sxy / sqrt(Sxx*Syy).");
      n = add(out, n, "r = %.6g / sqrt(%.6g*%.6g) = %.10g", fsxy, fsxx, fsyy, r);
      n = add(out, n, "Compare |r| with the critical value %.6g.", crit);
      return add(out, n, abs_num(r) > crit ? "Reject H0: there is evidence of correlation." : "Do not reject H0: insufficient evidence of correlation.");
    }
    sprintf(cmd, "pmccs(%.10g,%.10g,%.10g)", fsxx, fsyy, fsxy); return eval_stats(cmd, out);
  }
  if ((has(t, "covariance") || has(t, "sxx") || has(t, "syy") || has(t, "sxy") ||
       (has(t, "summary") && (has(t, "paired") || has(t, "bivariate")))) &&
      !(has(t, "pmcc") || has(t, "correlation") || has(t, "spearman") || has(t, "regression"))) {
    double xs[16], ys[16]; int count = 0;
    if (extract_xy_lists_after_words(input, xs, ys, &count) ||
        extract_point_pairs_after_word(input, xs, ys, &count) ||
        extract_two_lists_around_and(input, xs, ys, &count)) {
      return add_raw_paired_summary_lines(out, xs, ys, count);
    }
  }
  if ((has(t, "conditional") || has(t, "given")) && nv >= 2 &&
      !(has(t, "regression") || has(t, "leastsquares") || has(t, "lineofbestfit") || has(t, "estimate")) &&
      !has(t, "sxx") && !has(t, "sxy") && !has(t, "syy")) {
    sprintf(cmd, "cond(%.10g,%.10g)", v[0], v[1]); return eval_stats(cmd, out);
  }
  if ((has(t, "union") || has(t, "either") || has(t, "orprobability")) && nv >= 3) {
    sprintf(cmd, "probor(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && nv >= 3) {
    int tail = prob_tail(c, t);
    if (has(c, "nomorethan") || has(t, "cdf")) tail = -1;
    double third = v[2];
    if ((has(t, "critical") || has(t, "criticalregion") || has(t, "significance")) &&
        (has(t, "percent") || has(c, "%")) && third > 0 && third <= 100) third /= 100.0;
    if ((has(t, "critical") || has(t, "criticalregion") || has(t, "significance")))
      sprintf(cmd, "critbinom(%d,%.10g,%.10g,%.0f)", (int)v[0], v[1], third, (has(t, "upper") || has(t, "greater") || has(t, "more")) ? 1.0 : -1.0);
    else if (tail) sprintf(cmd, "binomtail(%d,%.10g,%d,%d)", (int)v[0], v[1], (int)v[2], tail);
    else sprintf(cmd, "binom(%d,%.10g,%d)", (int)v[0], v[1], (int)v[2]);
    return eval_stats(cmd, out);
  }
  if (has(t, "poisson") && (has(c, "p(x=0)=") || has(c, "p(x=0)")) &&
      (has(t, "mean") || has(t, "lambda") || has(t, "find")) && nv >= 1) {
    double q = -1;
    for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < 1) { q = v[i]; break; }
    if (q > 0 && q < 1) {
      double lam = -ln_approx(q);
      int n = add(out, 0, "For X~Po(lambda), P(X=0)=e^(-lambda).");
      n = add(out, n, "e^(-lambda) = %.10g", q);
      n = add(out, n, "-lambda = ln(%.10g)", q);
      return add(out, n, "lambda = -ln(%.10g) = %.10g", q, lam);
    }
  }
  if (has(t, "poisson") && nv >= 2) {
    if (has(t, "normal") && (has(t, "approx") || has(t, "approximation")) && nv >= 3) {
      sprintf(cmd, "poissonnorm(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
    }
    if ((has(t, "critical") || has(t, "criticalregion")) && nv >= 2) {
      double tail = has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 : -1;
      sprintf(cmd, "critpoisson(%.10g,%.10g,%.0f)", v[0], v[1], tail); return eval_stats(cmd, out);
    }
    if ((has(t, "hypothesis") || has(t, "significance") || has(t, "test")) && nv >= 3) {
      double lam = 0, obs = 0, alpha = 0.05;
      bool hLam = word_num(input, "lambda", &lam) || word_num(input, "mean", &lam) || label_num(input, "lambda", &lam);
      bool hObs = word_num(input, "observed", &obs) || word_num(input, "value", &obs) || label_num(input, "x", &obs);
      bool hA = word_num(input, "alpha", &alpha) || word_num(input, "significance", &alpha) || word_num(input, "percent", &alpha);
      if (!hLam) lam = v[0];
      if (!hObs) {
        for (int i = 0; i < nv; ++i) if (!near_num(v[i], lam) && v[i] > 0) { obs = v[i]; break; }
      }
      if (!hA) {
        for (int i = 0; i < nv; ++i) {
          if (near_num(v[i], lam) || near_num(v[i], obs)) continue;
          if (v[i] > 0 && v[i] <= 10) { alpha = v[i]; break; }
          if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
        }
      }
      if (alpha > 1 && alpha <= 100) alpha /= 100.0;
      double tail = has(t, "upper") || has(t, "greater") || has(t, "more") || has(t, "increased") || has(t, "increase") ? 1 : -1;
      sprintf(cmd, "hyppoisson(%.10g,%d,%.10g,%.0f)", lam, (int)obs, alpha, tail); return eval_stats(cmd, out);
    }
    double lo=0, hi=0;
    if (prob_x_interval(c, &lo, &hi)) {
      sprintf(cmd, "poissonrange(%.10g,%d,%d)", v[0], (int)lo, (int)hi);
      return eval_stats(cmd, out);
    }
    if (has(t, "between") && nv >= 3) {
      sprintf(cmd, "poissonrange(%.10g,%d,%d)", v[0], (int)v[nv-2], (int)v[nv-1]);
      return eval_stats(cmd, out);
    }
    int tail = prob_tail(c, t);
    if (has(c, "notequal") || has(c, "not=") || has(c, "!=")) {
      int x = (int)v[1];
      double px = poissonp(v[0], x);
      int n = add(out, 0, "Use the complement for not equal.");
      n = add(out, n, "Let X ~ Po(%.6g).", v[0]);
      n = add(out, n, "P(X!=%d)=1-P(X=%d)", x, x);
      n = add(out, n, "P(X=%d) = %.10g", x, px);
      return add(out, n, "P(X!=%d) = %.10g", x, 1 - px);
    }
    if (has(c, "nomorethan") || has(t, "cdf")) tail = -1;
    if (tail) { sprintf(cmd, "poissontail(%.10g,%d,%d)", v[0], (int)v[1], tail); return eval_stats(cmd, out); }
    sprintf(cmd, "poisson(%.10g,%d)", v[0], (int)v[1]); return eval_stats(cmd, out);
  }
  if (has(t, "poisson") && (has(t, "mean") || has(t, "variance") || has(t, "standarddeviation") || has(t, "sd")) && nv >= 1) {
    sprintf(cmd, "poissonstats(%.10g)", v[0]); return eval_stats(cmd, out);
  }
  if ((has(t, "pmcc") || has(t, "correlation") || has(t, "spearman")) &&
      (has(t, "critical") || has(t, "cv")) && (has(t, "test") || has(t, "hypothesis") || has(t, "significant")) && nv >= 2) {
    double r=0, crit=0; bool hr=label_num(input,"r",&r), hc=label_num(input,"critical",&crit) || label_num(input,"cv",&crit);
    if (!hr) for (int i = 0; i < nv; ++i) if (v[i] >= -1 && v[i] <= 1) { r = v[i]; hr = true; break; }
    if (!hc) for (int i = nv - 1; i >= 0; --i) if (v[i] > 0 && v[i] <= 1 && (!hr || !near_num(v[i], r))) { crit = v[i]; hc = true; break; }
    int tail = has(t, "positive") || has(t, "upper") ? 1 : (has(t, "negative") || has(t, "lower") ? -1 : 0);
    sprintf(cmd, "corrtest(%.10g,%.10g,%d)", hr ? r : v[0], hc ? crit : v[1], tail); return eval_stats(cmd, out);
  }
  if ((has(t, "pmcc") || has(t, "correlation") || has(t, "association")) &&
      (has(t, "test") || has(t, "significant") || has(t, "evidence")) && !has(t, "spearman") && nv >= 2) {
    double n0=0, r=0, alpha=0.05;
    bool hn = word_num(input,"sample",&n0) || word_num(input,"sampleof",&n0) || label_num(input,"n",&n0);
    bool hr = label_num(input,"r",&r);
    if (!hn) for (int i = 0; i < nv; ++i) if (v[i] > n0 && v[i] >= 3) n0 = v[i];
    if (!hr) for (int i = 0; i < nv; ++i) if (v[i] >= -1 && v[i] <= 1) { r = v[i]; hr = true; break; }
    alpha = alpha_from_text(input, t, c, v, nv, n0, r);
    int tail = has(t, "positive") || has(t, "greater") || has(t, "upper") ? 1 :
               has(t, "negative") || has(t, "less") || has(t, "lower") ? -1 : 0;
    double den = root(1-r*r);
    double tv = den ? r * root(n0-2) / den : 0;
    double crit = tail ? (alpha <= 0.01 ? 2.326 : (alpha <= 0.025 ? 1.96 : 1.645))
                       : (alpha <= 0.01 ? 2.576 : 1.96);
    bool reject = tail > 0 ? tv > crit : tail < 0 ? tv < -crit : abs_num(tv) > crit;
    int n = add(out, 0, "H0: rho = 0.");
    n = add(out, n, tail > 0 ? "H1: rho > 0." : tail < 0 ? "H1: rho < 0." : "H1: rho != 0.");
    n = add(out, n, "Use t = r*sqrt(n-2)/sqrt(1-r^2).");
    n = add(out, n, "t = %.6g*sqrt(%.0f-2)/sqrt(1-%.6g^2) = %.10g", r, n0, r, tv);
    n = add(out, n, "At alpha = %.6g, compare with critical value %.6g.", alpha, crit);
    return add(out, n, reject ? "Reject H0: there is evidence of correlation." : "Do not reject H0: insufficient evidence of correlation.");
  }
  if (has(t, "spearman") && (has(t, "test") || has(t, "significant") || has(t, "association")) && nv >= 2) {
    double n0=0, r=0;
    bool hn = word_num(input,"sample",&n0) || word_num(input,"sampleof",&n0) || label_num(input,"n",&n0);
    bool hr = label_num(input,"rs",&r) || label_num(input,"r_s",&r) || label_num(input,"r",&r);
    if (!hr) for (int i = 0; i < nv; ++i) if (v[i] >= -1 && v[i] <= 1) { r = v[i]; hr = true; break; }
    if (!hn) for (int i = 0; i < nv; ++i) if (!near_num(v[i], r) && v[i] >= 3) { n0 = v[i]; hn = true; break; }
    double alpha = alpha_from_text(input, t, c, v, nv, n0, r);
    int tail = has(t, "positive") || has(t, "greater") || has(t, "upper") ? 1 :
               has(t, "negative") || has(t, "less") || has(t, "lower") ? -1 : 0;
    double den = root(1-r*r);
    double tv = den ? r * root(n0-2) / den : 0;
    double crit = tail ? (alpha <= 0.01 ? 2.326 : (alpha <= 0.025 ? 1.96 : 1.645))
                       : (alpha <= 0.01 ? 2.576 : 1.96);
    bool reject = tail > 0 ? tv > crit : tail < 0 ? tv < -crit : abs_num(tv) > crit;
    int n = add(out, 0, "H0: rho_s = 0.");
    n = add(out, n, tail > 0 ? "H1: rho_s > 0." : tail < 0 ? "H1: rho_s < 0." : "H1: rho_s != 0.");
    n = add(out, n, "Use the large-sample t check: t = r_s*sqrt(n-2)/sqrt(1-r_s^2).");
    n = add(out, n, "t = %.6g*sqrt(%.0f-2)/sqrt(1-%.6g^2) = %.10g", r, n0, r, tv);
    n = add(out, n, "At alpha = %.6g, compare with critical value %.6g.", alpha, crit);
    return add(out, n, reject ? "Reject H0: there is evidence of rank correlation." : "Do not reject H0: insufficient evidence of rank correlation.");
  }
  if (has(t, "spearman") && nv >= 2) {
    double xs[16], ys[16]; int count = 0;
    if (extract_xy_lists_after_words(input, xs, ys, &count)) {
      return add_raw_spearman_lines(out, xs, ys, count);
    }
    if (extract_point_pairs_after_word(input, xs, ys, &count)) {
      return add_raw_spearman_lines(out, xs, ys, count);
    }
    if (extract_two_lists_around_and(input, xs, ys, &count)) {
      return add_raw_spearman_lines(out, xs, ys, count);
    }
    double n0=0, sd2=0;
    if (label_num(input,"n",&n0) && (label_num(input,"sumd2",&sd2) || label_num(input,"d2",&sd2))) {
      sprintf(cmd, "spearman(%.10g,%.10g)", n0, sd2); return eval_stats(cmd, out);
    }
    sprintf(cmd, "spearman(%.10g,%.10g)", v[0], v[1]); return eval_stats(cmd, out);
  }
  if ((has(t, "pmcc") || has(t, "correlation")) && !has(t, "spearman")) {
    double xs[16], ys[16]; int count = 0;
    if (extract_xy_lists_after_words(input, xs, ys, &count)) {
      return add_raw_pmcc_lines(out, xs, ys, count);
    }
    if (extract_point_pairs_after_word(input, xs, ys, &count)) {
      return add_raw_pmcc_lines(out, xs, ys, count);
    }
    double n0=0, sx=0, sy=0, sxy=0, sx2=0, sy2=0, sxx=0, syy=0;
    if (label_num(input,"sxx",&sxx) && label_num(input,"syy",&syy) && label_num(input,"sxy",&sxy)) {
      sprintf(cmd, "pmccs(%.10g,%.10g,%.10g)", sxx, syy, sxy); return eval_stats(cmd, out);
    }
    if (label_num(input,"n",&n0) && label_num(input,"sx",&sx) && label_num(input,"sy",&sy) && label_num(input,"sxy",&sxy) && (label_num(input,"sx2",&sx2) || label_num(input,"sx^2",&sx2)) && (label_num(input,"sy2",&sy2) || label_num(input,"sy^2",&sy2))) {
      sprintf(cmd, "pmcc(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", n0, sx, sy, sxy, sx2, sy2); return eval_stats(cmd, out);
    }
  }
  if ((has(t, "pmcc") || has(t, "correlation")) && !has(t, "spearman") && nv >= 6) {
    sprintf(cmd, "pmcc(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4], v[5]); return eval_stats(cmd, out);
  }
  if (has(t, "regression") || has(c, "leastsquares") || has(c, "lineofbestfit") || has(c, "lineofbestfit")) {
    double n0=0, sx=0, sy=0, sxx=0, syy=0, sxy=0, xb=0, yb=0, x=0, y=0;
    bool hx = last_label_num(input,"x",&x);
    bool hy = last_label_num(input,"y",&y);
    bool x_on_y = has(c, "xony") || has(c, "xonthey") || has(c, "regressionlineofx");
    double grad = 0;
    bool hxb = word_num(input,"meanx",&xb) || label_num(input,"xbar",&xb);
    bool hyb = word_num(input,"meany",&yb) || label_num(input,"ybar",&yb);
    bool hg = word_num(input,"gradient",&grad) || word_num(input,"slope",&grad) || label_num(input,"gradient",&grad);
    if (!hg && !hxb && !hyb && nv >= 3 && has(t, "gradient") && has(t, "mean")) {
      xb = v[0]; yb = v[1]; grad = v[2]; hxb = hyb = hg = true;
    }
    if (!x_on_y && hxb && hyb && hg) {
      double a0 = yb - grad * xb;
      int n = add(out, 0, "Use y-ybar = b(x-xbar).");
      n = add(out, n, "xbar = %.10g, ybar = %.10g, b = %.10g", xb, yb, grad);
      n = add(out, n, "a = ybar - b*xbar = %.10g - %.10g*%.10g = %.10g", yb, grad, xb, a0);
      n = add(out, n, "regression line: y = %.10g + %.10g x", a0, grad);
      if (hx) n = add(out, n, "when x=%.10g, y=%.10g", x, a0 + grad*x);
      return n;
    }
    double xs[16], ys[16]; int count = 0;
    if (extract_xy_lists_after_words(input, xs, ys, &count)) {
      return add_raw_regression_lines(out, xs, ys, count, x_on_y, x_on_y ? hy : hx, x_on_y ? y : x);
    }
    if (extract_point_pairs_after_word(input, xs, ys, &count)) {
      return add_raw_regression_lines(out, xs, ys, count, x_on_y, x_on_y ? hy : hx, x_on_y ? y : x);
    }
    double m=0, b=0;
    if (hx && parse_regression_line(c, &m, &b)) {
      sprintf(cmd, "regress(%.10g,%.10g,%.10g)", b, m, x);
      return eval_stats(cmd, out);
    }
    if (x_on_y && (label_num(input,"n",&n0) || word_num(input,"n",&n0)) &&
        (label_num(input,"sx",&sx) || word_num(input,"sumx",&sx)) &&
        (label_num(input,"sy",&sy) || word_num(input,"sumy",&sy)) &&
        label_num(input,"syy",&syy) && label_num(input,"sxy",&sxy)) {
      double xb0 = sx/n0, yb0 = sy/n0, bb = sxy/syy, aa = xb0 - bb*yb0;
      int n = add(out, 0, "Find least-squares regression line x = a + by.");
      n = add(out, n, "xbar = Sx/n = %.6g/%.6g = %.6g", sx, n0, xb0);
      n = add(out, n, "ybar = Sy/n = %.6g/%.6g = %.6g", sy, n0, yb0);
      n = add(out, n, "b = Sxy/Syy = %.6g/%.6g = %.6g", sxy, syy, bb);
      n = add(out, n, "a = xbar - b*ybar = %.6g", aa);
      n = add(out, n, "regression line: x = %.6g + %.6g y", aa, bb);
      if (hy) n = add(out, n, "when y=%.6g, x=%.6g", y, aa+bb*y);
      return n;
    }
    if ((label_num(input,"n",&n0) || word_num(input,"n",&n0)) &&
        (label_num(input,"sx",&sx) || word_num(input,"sumx",&sx)) &&
        (label_num(input,"sy",&sy) || word_num(input,"sumy",&sy)) &&
        label_num(input,"sxx",&sxx) && label_num(input,"sxy",&sxy)) {
      if (hx) sprintf(cmd, "regresscalc(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", n0, sx, sy, sxx, sxy, x);
      else sprintf(cmd, "regresscalc(%.10g,%.10g,%.10g,%.10g,%.10g)", n0, sx, sy, sxx, sxy);
      return eval_stats(cmd, out);
    }
    if (label_num(input,"n",&n0) && label_num(input,"sx",&sx) && label_num(input,"sy",&sy) && label_num(input,"sxx",&sxx) && label_num(input,"sxy",&sxy)) {
      if (hx) sprintf(cmd, "regresscalc(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", n0, sx, sy, sxx, sxy, x);
      else sprintf(cmd, "regresscalc(%.10g,%.10g,%.10g,%.10g,%.10g)", n0, sx, sy, sxx, sxy);
      return eval_stats(cmd, out);
    }
    if (label_num(input,"xbar",&xb) && label_num(input,"ybar",&yb) && label_num(input,"sxx",&sxx) && label_num(input,"sxy",&sxy)) {
      if (hx) sprintf(cmd, "regresss(%.10g,%.10g,%.10g,%.10g,%.10g)", xb, yb, sxx, sxy, x);
      else sprintf(cmd, "regresss(%.10g,%.10g,%.10g,%.10g)", xb, yb, sxx, sxy);
      return eval_stats(cmd, out);
    }
  }
  if ((has(t, "coded") || has(t, "coding") || has(c, "y=(x")) && has(t, "mean") &&
      (has(t, "sd") || has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation"))) && nv >= 4) {
    if (!coded_cmd_from_text(c, v, nv, cmd, sizeof(cmd))) {
      if (has(c, "y=(x-") || has(c, "y=(x+")) sprintf(cmd, "uncode(%.10g,%.10g,%.10g,%.10g)", v[2], v[3], -v[0], v[1]);
      else sprintf(cmd, "code(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]);
    }
    return eval_stats(cmd, out);
  }
  if ((has(t, "discrete") || has(t, "randomvariable") || has(t, "expectation") || has(t, "distribution")) &&
      discrete_table_cmd(t, cmd, sizeof(cmd))) {
    return eval_stats(cmd, out);
  }
  if ((has(t, "class") || has(t, "classes")) && (has(t, "frequency") || has(t, "frequencies")) &&
      (has(t, "mean") || has(t, "standarddeviation") || has(t, "sd")) &&
      grouped_class_freq_cmd(t, cmd, sizeof(cmd))) {
    return eval_stats(cmd, out);
  }
  if ((has(t, "grouped") || has(t, "group") || has(t, "midpoint") || has(t, "midpoints")) &&
      (has(t, "mean") || has(t, "standarddeviation") || has(t, "sd")) && nv >= 4 && nv % 2 == 0) {
    int p = sprintf(cmd, "groupmean(");
    int m = nv / 2;
    if ((has(t, "midpoint") || has(t, "midpoints")) && (has(t, "frequency") || has(t, "frequencies"))) {
      for (int i = 0; i < m; ++i) p += sprintf(cmd+p, "%s%.10g,%.10g", i ? "," : "", v[i], v[i+m]);
    } else {
      for (int i = 0; i < m; ++i) p += sprintf(cmd+p, "%s%.10g,%.10g", i ? "," : "", v[2*i], v[2*i+1]);
    }
    sprintf(cmd+p, ")");
    return eval_stats(cmd, out);
  }
  if ((has(t, "discrete") || has(t, "randomvariable") || has(t, "expectation") || has(t, "distribution")) &&
      has(c, "p(x=x)=kx") && nv >= 2) {
    double sx = 0, sx2 = 0;
    for (int i = 0; i < nv; ++i) { sx += v[i]; sx2 += v[i]*v[i]; }
    double k = sx == 0 ? 0 : 1 / sx;
    int n = add(out, 0, "For P(X=x)=kx, first use sum P(X=x)=1.");
    n = add(out, n, "k(%.10g) = 1, so k = %.10g", sx, k);
    return add(out, n, "E(X)=sum xP(X=x)=k sum x^2 = %.10g*%.10g = %.10g", k, sx2, k*sx2);
  }
  if ((has(t, "discrete") || has(t, "randomvariable") || has(t, "expectation") || has(t, "distribution")) &&
      has(t, "proportional") && (has(t, "tox") || has(t, "to,x") || has(c, "tox")) && nv >= 2) {
    double sx = 0, sx2 = 0;
    for (int i = 0; i < nv; ++i) { sx += v[i]; sx2 += v[i]*v[i]; }
    double k = sx == 0 ? 0 : 1 / sx;
    int n = add(out, 0, "If P(X=x) is proportional to x, write P(X=x)=kx.");
    n = add(out, n, "sum P(X=x)=1 gives k*sum x = 1");
    n = add(out, n, "sum x = %.10g, so k = %.10g", sx, k);
    return add(out, n, "E(X)=sum x*kx = k sum x^2 = %.10g*%.10g = %.10g", k, sx2, k*sx2);
  }
  if ((has(t, "cumulative") || has(t, "cdf")) && has(t, "probabilities") &&
      !has(t, "frequency") && !has(t, "frequencies") && nv >= 4) {
    int table_nv = nv;
    if ((table_nv % 2) && strstr(c, "p(x=")) --table_nv;
    if (table_nv < 4 || (table_nv % 2)) return 0;
    int m = table_nv / 2;
    double mean = 0, ex2 = 0, prev = 0;
    int n = add(out, 0, "Convert cumulative probabilities into probabilities by subtraction.");
    for (int i = 0; i < m && n < P3_MAX_LINES - 4; ++i) {
      double x = v[i], cf = v[i + m], p = cf - prev;
      prev = cf;
      mean += x*p;
      ex2 += x*x*p;
      n = add(out, n, "P(X=%.10g) = %.10g", x, p);
    }
    const char *px = strstr(c, "p(x=");
    if (px) {
      double q = read_num(px + 4), prob = 0;
      for (int i = 0; i < m; ++i) {
        double p = v[i + m] - (i ? v[i + m - 1] : 0);
        if (near_num(v[i], q)) prob = p;
      }
      n = add(out, n, "requested P(X=%.10g) = %.10g", q, prob);
    }
    if (has(c, "e(x)") || has(t, "mean") || has(t, "expectation") || has(t, "find")) {
      n = add(out, n, "E(X)=sum xP(X=x) = %.10g", mean);
    }
    if (has(t, "variance") || has(c, "var(x)")) {
      n = add(out, n, "E(X^2)=sum x^2P(X=x) = %.10g", ex2);
      return add(out, n, "Var(X)=E(X^2)-E(X)^2 = %.10g", ex2 - mean*mean);
    }
    return n;
  }
  if ((has(t, "discrete") || has(t, "randomvariable") || has(t, "expectation") || has(t, "distribution")) &&
      ((has(t, "prob") || has(t, "probabilities") || has(c, "p(x=")) && nv >= 4)) {
    int p = sprintf(cmd, "discrete(");
    if ((has(t, "values") || has(t, "xvalues") || has(t, "probabilities") || has(t, "probs")) && nv % 2 == 0) {
      int m = nv / 2;
      for (int i = 0; i < m; ++i) p += sprintf(cmd+p, "%s%.10g,%.10g", i ? "," : "", v[i], v[i+m]);
    } else {
      int m = nv / 2;
      for (int i = 0; i < m; ++i) p += sprintf(cmd+p, "%s%.10g,%.10g", i ? "," : "", v[2*i], v[2*i+1]);
    }
    sprintf(cmd+p, ")");
    return eval_stats(cmd, out);
  }
  double cdf_lo = 0, cdf_hi = 0;
  if ((has(t, "cdf") || has(t, "cumulative")) && has(c, ")/") &&
      extract_x_interval(c, &cdf_lo, &cdf_hi) &&
      (has(c, "p(x<") || has(c, "p(x<=") || has(c, "p(x>") || has(c, "p(x>="))) {
    const char *xp = strstr(c, "x^");
    const char *slash = strstr(c, ")/");
    if (xp && slash) {
      int pow = isdigit((unsigned char)xp[2]) ? xp[2] - '0' : 1;
      double sh = 0;
      const char *after = xp + 3;
      if (*after == '-' || *after == '+') sh = read_num(after);
      double den = read_num(slash + 2);
      double lower_base = pwr(cdf_lo, pow) + sh;
      double upper_base = pwr(cdf_hi, pow) + sh;
      double bound = 0; int btail = 0;
      const char *bp = strstr(c, "p(x<");
      if (bp) { btail = -1; bound = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
      else if ((bp = strstr(c, "p(x>"))) { btail = 1; bound = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
      if (pow >= 1 && den != 0 && near_num(lower_base, 0) && near_num(upper_base / den, 1) && btail != 0) {
        double Fb = (pwr(bound, pow) + sh) / den;
        if (Fb < 0) Fb = 0;
        if (Fb > 1) Fb = 1;
        int n = add(out, 0, "For a CDF, use endpoint values and then substitute the bound.");
        n = add(out, n, "F(x)=(x^%d%+.10g)/%.10g on %.6g<=x<=%.6g.", pow, sh, den, cdf_lo, cdf_hi);
        n = add(out, n, "F(%.6g)=0 and F(%.6g)=1.", cdf_lo, cdf_hi);
        n = add(out, n, "F(%.6g)=%.10g", bound, Fb);
        return add(out, n, btail > 0 ? "P(X>%.6g)=1-F(%.6g)=%.10g" : "P(X<%.6g)=F(%.6g)=%.10g", bound, bound, btail > 0 ? 1 - Fb : Fb);
      }
    }
  }
  if ((has(t, "cdf") || has(t, "cumulative")) &&
      (has(c, "k((") || has(c, "k*((")) && extract_x_interval(c, &cdf_lo, &cdf_hi) &&
      (has(c, "p(x<") || has(c, "p(x<=") || has(c, "p(x>") || has(c, "p(x>="))) {
    const char *kp = strstr(c, "k((");
    if (!kp) kp = strstr(c, "k*((");
    const char *p = kp ? strstr(kp, "((") : 0;
    if (p) {
      p += 2;
      double rb=0, rd=0; const char *e=0;
      if (parse_linear_inside(p, 'x', &rb, &rd, &e) && *e == ')' && e[1] == '^') {
        int pow = (int)read_num(e + 2);
        const char *after = strchr(e + 2, ')');
        double sh = 0;
        if (after && (after[1] == '-' || after[1] == '+')) sh = read_num(after + 1);
        double lowv = pwr(rb*cdf_lo + rd, pow) + sh;
        double hiv = pwr(rb*cdf_hi + rd, pow) + sh;
        double k = (hiv - lowv) ? 1.0 / (hiv - lowv) : 0;
        double bound = 0; int btail = 0;
        const char *bp = strstr(c, "p(x<");
        if (bp) { btail = -1; bound = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
        else if ((bp = strstr(c, "p(x>"))) { btail = 1; bound = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
        double Fb = k * (pwr(rb*bound + rd, pow) + sh - lowv);
        if (Fb < 0) Fb = 0;
        if (Fb > 1) Fb = 1;
        char lin[48]; fmt_linear_var(lin, sizeof(lin), rb, 'x', rd);
        int n = add(out, 0, "For a CDF, use F(lower)=0 and F(upper)=1.");
        n = add(out, n, "F(x)=k((%s)^%d%+.10g) on %.6g<=x<=%.6g.", lin, pow, sh, cdf_lo, cdf_hi);
        n = add(out, n, "k = 1/(%.10g - %.10g) = %.10g", hiv, lowv, k);
        n = add(out, n, "F(%.6g)=%.10g", bound, Fb);
        return add(out, n, btail > 0 ? "P(X>%.6g)=1-F(%.6g)=%.10g" : "P(X<%.6g)=F(%.6g)=%.10g", bound, bound, btail > 0 ? 1 - Fb : Fb);
      }
    }
  }
  if ((has(t, "cdf") || has(t, "cumulative")) &&
      (has(c, "+k)/") || has(c, "-k)/")) && extract_x_interval(c, &cdf_lo, &cdf_hi) &&
      (has(c, "p(x<") || has(c, "p(x<=") || has(c, "p(x>") || has(c, "p(x>=") || has(t, "findk"))) {
    const char *x2 = strstr(c, "x^2");
    const char *kp = strstr(c, "+k)/");
    double signk = 1;
    if (!kp) { kp = strstr(c, "-k)/"); signk = -1; }
    double bx = 0;
    if (x2) {
      const char *q = x2 + 3;
      if (*q == '+' || *q == '-') {
        double sg = *q == '-' ? -1 : 1;
        ++q;
        bx = (*q == 'x') ? sg : sg * read_num(q);
      }
    }
    double den = kp ? read_num(kp + 4) : 0;
    if (den <= 0) den = cdf_hi*cdf_hi + bx*cdf_hi - (cdf_lo*cdf_lo + bx*cdf_lo);
    double kval = -signk * (cdf_lo*cdf_lo + bx*cdf_lo);
    if (near_num(kval, 0)) kval = 0;
    double bound = 0; int btail = 0;
    const char *bp = strstr(c, "p(x<");
    if (bp) { btail = -1; bound = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
    else if ((bp = strstr(c, "p(x>"))) { btail = 1; bound = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
    double Fb = (bound*bound + bx*bound + signk*kval) / den;
    if (Fb < 0) Fb = 0;
    if (Fb > 1) Fb = 1;
    double Fhi = (cdf_hi*cdf_hi + bx*cdf_hi + signk*kval) / den;
    int n = add(out, 0, "For a CDF, use F(lower)=0 and F(upper)=1.");
    n = add(out, n, "F(x)=(x^2%+.10gx%s)/%.10g on %.6g<=x<=%.6g.", bx, signk > 0 ? "+k" : "-k", den, cdf_lo, cdf_hi);
    n = add(out, n, "F(%.6g)=0 gives k = %.10g", cdf_lo, kval);
    n = add(out, n, "F(%.6g)=%.10g", cdf_hi, Fhi);
    if (abs_num(Fhi - 1) > 1e-6) return add(out, n, "These endpoint conditions are inconsistent; check the CDF or interval.");
    if (btail > 0) return add(out, n, "P(X>%.6g)=1-F(%.6g)=%.10g", bound, bound, 1 - Fb);
    if (btail < 0) return add(out, n, "P(X<%.6g)=F(%.6g)=%.10g", bound, bound, Fb);
    return n;
  }
  if ((has(t, "cdf") || has(t, "cumulative")) &&
      (has(c, "k(x^") || has(c, "k*(x^")) && extract_x_interval(c, &cdf_lo, &cdf_hi) &&
      (has(c, "p(x<") || has(c, "p(x<=") || has(c, "p(x>") || has(c, "p(x>="))) {
    const char *kp = strstr(c, "x^");
    int pow = kp && isdigit((unsigned char)kp[2]) ? kp[2] - '0' : 1;
    double base = pwr(cdf_lo, pow), top = pwr(cdf_hi, pow);
    double bx = 0; int btail = 0;
    const char *bp = strstr(c, "p(x<");
    if (bp) { btail = -1; bx = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
    else if ((bp = strstr(c, "p(x>"))) { btail = 1; bx = read_num(bp + (bp[4] == '=' ? 5 : 4)); }
    else prob_x_bound(c, &bx, &btail);
    if (pow >= 1 && top != base && btail != 0) {
      double k = 1.0 / (top - base);
      double Fb = k * (pwr(bx, pow) - base);
      if (Fb < 0) Fb = 0;
      if (Fb > 1) Fb = 1;
      double prob = btail > 0 ? 1.0 - Fb : Fb;
      int n = add(out, 0, "For a CDF, use F(upper)=1 to find k.");
      n = add(out, n, "F(x)=k(x^%d-%.10g) on %.6g<=x<=%.6g.", pow, base, cdf_lo, cdf_hi);
      n = add(out, n, "k(%.6g^%d-%.6g^%d)=1, so k = %.10g", cdf_hi, pow, cdf_lo, pow, k);
      n = add(out, n, "F(%.6g)=%.10g", bx, Fb);
      return add(out, n, btail > 0 ? "P(X>%.6g)=1-F(%.6g)=%.10g" : "P(X<%.6g)=F(%.6g)=%.10g", bx, bx, prob);
    }
  }
  if ((has(t, "cdf") || has(t, "cumulative")) && has(t, "median") &&
      (has(c, "f(x)=kx^") || has(c, "f(x)=k*x^") || has(c, "kx^")) &&
      extract_x_interval(c, &cdf_lo, &cdf_hi)) {
    const char *kp = strstr(c, "x^");
    int pow = kp && isdigit((unsigned char)kp[2]) ? kp[2] - '0' : 1;
    double top = pwr(cdf_hi, pow);
    if (pow >= 1 && top != 0) {
      double k = 1.0 / top;
      double med = nth_root(0.5 / k, pow);
      int n = add(out, 0, "Use F(upper)=1 to find k, then solve F(m)=0.5.");
      n = add(out, n, "F(x)=kx^%d on %.6g<=x<=%.6g", pow, cdf_lo, cdf_hi);
      n = add(out, n, "k*%.6g^%d = 1, so k = %.10g", cdf_hi, pow, k);
      n = add(out, n, "k*m^%d = 0.5", pow);
      return add(out, n, "m = %.10g", med);
    }
  }
  if ((has(t, "cdf") || has(t, "cumulative")) && has(t, "median") &&
      has(c, "ln") && has(c, "-ln") && extract_x_interval(c, &cdf_lo, &cdf_hi) &&
      cdf_lo > 0 && cdf_hi > cdf_lo) {
    double k = 1.0 / ln_approx(cdf_hi / cdf_lo);
    double med = root(cdf_lo * cdf_hi);
    int n = add(out, 0, "For the median m, solve F(m)=0.5.");
    n = add(out, n, "Use F(%.6g)=1 to find k.", cdf_hi);
    n = add(out, n, "k(ln %.6g - ln %.6g)=1, so k = %.10g", cdf_hi, cdf_lo, k);
    n = add(out, n, "k(ln m - ln %.6g)=0.5", cdf_lo);
    n = add(out, n, "ln(m/%.6g)=%.10g", cdf_lo, 0.5/k);
    return add(out, n, "m = %.10g", med);
  }
  if ((has(t, "cdf") || has(t, "cumulative")) && has(t, "median") &&
      (has(c, "kln(x)") || has(c, "k*ln(x)") || has(c, "klog(x)")) &&
      (extract_x_interval(c, &cdf_lo, &cdf_hi) || has(c, "<=x<=e^") || has(c, "<x<e^"))) {
    if (cdf_lo <= 0) cdf_lo = 1;
    double upper_log = 0;
    const char *ep = strstr(c, "e^");
    if (ep) upper_log = read_num(ep + 2);
    if (upper_log <= 0) upper_log = 1;
    double k = 1.0 / upper_log;
    double med_log = 0.5 / k;
    double med = exp_approx(med_log);
    int n = add(out, 0, "For the median m, solve F(m)=0.5.");
    n = add(out, n, "Use F(e^%.6g)=1 to find k.", upper_log);
    n = add(out, n, "k ln(e^%.6g)=1, so k = %.10g", upper_log, k);
    n = add(out, n, "k ln(m)=0.5");
    n = add(out, n, "ln(m)=%.10g", med_log);
    return add(out, n, "m = e^%.10g = %.10g", med_log, med);
  }
  if ((has(t, "cdf") || has(t, "cumulative")) && has(t, "median") &&
      (has(c, "k(x^") || has(c, "k*(x^")) && extract_x_interval(c, &cdf_lo, &cdf_hi)) {
    const char *kp = strstr(c, "x^");
    int pow = kp && isdigit((unsigned char)kp[2]) ? kp[2] - '0' : 1;
    double base = pwr(cdf_lo, pow), top = pwr(cdf_hi, pow);
    if (pow >= 1 && top != base) {
      double k = 1.0 / (top - base);
      double med_base = 0.5/k + base;
      double med = nth_root(med_base, pow);
      int n = add(out, 0, "For the median m, solve F(m)=0.5.");
      n = add(out, n, "Use F(%.6g)=1 to find k.", cdf_hi);
      n = add(out, n, "k(%.6g^%d-%.6g^%d)=1, so k = %.10g", cdf_hi, pow, cdf_lo, pow, k);
      n = add(out, n, "k(m^%d-%.6g^%d)=0.5", pow, cdf_lo, pow);
      return add(out, n, "m = %.10g", med);
    }
  }
  if (has(t, "cumulative") && (has(t, "frequency") || has(t, "frequencies")) &&
      (has(t, "boundary") || has(t, "boundaries") || has(t, "class")) && nv >= 6 && nv % 2 == 0) {
    int m = nv / 2;
    double *b = v, *cf = v + m;
    double total = cf[m - 1];
    int n = add(out, 0, "Use linear interpolation on the cumulative-frequency table.");
    n = add(out, n, "total frequency n = %.10g", total);
    double qs[3], vals[3]; const char *names[3]; int qc = 0;
    if (has(t, "lowerquartile") || has(t, "q1") || has(t, "interquartile") || has(t, "iqr")) { qs[qc] = 0.25; names[qc++] = "Q1"; }
    if (has(t, "median")) { qs[qc] = 0.5; names[qc++] = "median"; }
    if (has(t, "upperquartile") || has(t, "q3") || has(t, "interquartile") || has(t, "iqr")) { qs[qc] = 0.75; names[qc++] = "Q3"; }
    if (qc == 0) { qs[qc] = 0.5; names[qc++] = "median"; }
    for (int q = 0; q < qc && n < P3_MAX_LINES - 2; ++q) {
      double pos = qs[q] * total, prev = cf[0], ans = b[0];
      for (int i = 1; i < m; ++i) {
        if (cf[i] >= pos) {
          double f = cf[i] - prev, w = b[i] - b[i-1];
          ans = b[i-1] + ((pos - prev) / f) * w;
          n = add(out, n, "%s position = %.6g*n = %.10g", names[q], qs[q], pos);
          n = add(out, n, "%s class %.6g to %.6g: %.10g", names[q], b[i-1], b[i], ans);
          break;
        }
        prev = cf[i];
      }
      vals[q] = ans;
    }
    if (has(t, "iqr") || has(t, "interquartile")) return add(out, n, "IQR = Q3 - Q1 = %.10g", vals[qc-1] - vals[0]);
    return n;
  }
  if ((has(t, "cumulative") || has(t, "cdf")) && has(t, "median") &&
      (has(c, "1-exp(-") || has(c, "1-e^(-"))) {
    const char *ep = strstr(c, "exp(-");
    int skip = 5;
    if (!ep) { ep = strstr(c, "e^(-"); skip = 4; }
    double lam = ep ? read_num(ep + skip) : 0;
    if (lam > 0) {
      double med = ln_approx(2.0) / lam;
      int n = add(out, 0, "For the median m, solve F(m)=0.5.");
      n = add(out, n, "1-exp(-%.10g m)=0.5", lam);
      n = add(out, n, "exp(-%.10g m)=0.5", lam);
      n = add(out, n, "%.10g m = ln(2)", lam);
      return add(out, n, "m = ln(2)/%.10g = %.10g", lam, med);
    }
  }
  if ((has(t, "cumulative") || has(t, "cdf")) && has(t, "median") &&
      !has(t, "frequency") && !has(t, "frequencies") && !has(t, "table") && !has(t, "class") && nv >= 1) {
    double denom = 0;
    for (int i = 0; i < nv; ++i) if (v[i] > denom) denom = v[i];
    if (denom > 0) {
      int n = add(out, 0, "For the median m, solve F(m)=0.5.");
      n = add(out, n, "Here F(x)=x^2/%.10g on the middle interval.", denom);
      n = add(out, n, "m^2/%.10g = 0.5", denom);
      return add(out, n, "m = sqrt(%.10g/2) = %.10g", denom, root(denom/2.0));
    }
  }
  if ((has(t, "cdf") || has(t, "cumulative")) && has(c, "f(x)=kx^") && has(c, "<x<")) {
    const char *kp = strstr(c, "kx^");
    int pow = kp && isdigit((unsigned char)kp[3]) ? kp[3] - '0' : 1;
    double upper = 0, lo = 0, hi = 0;
    const char *bp = strstr(c, "0<x<"); if (bp) upper = read_num(bp + 4);
    const char *xp = strstr(c, "p(");
    if (xp) {
      const char *mid = strstr(xp, "<x<");
      if (mid) {
        const char *a0 = mid;
        while (a0 > xp && (isdigit((unsigned char)a0[-1]) || a0[-1] == '.' || a0[-1] == '-')) --a0;
        char left[32]; int len = (int)(mid - a0);
        if (len > 0 && len < (int)sizeof(left)) { memcpy(left, a0, len); left[len] = 0; lo = read_num(left); hi = read_num(mid + 3); }
      }
    }
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    int tail = 0; double bound = 0;
    const char *req = strstr(c, "p(x>");
    if (req) { tail = 1; bound = read_num(req + (req[4] == '=' ? 5 : 4)); }
    else if ((req = strstr(c, "p(x<"))) { tail = -1; bound = read_num(req + (req[4] == '=' ? 5 : 4)); }
    else if (!(hi > lo)) prob_x_bound(c, &bound, &tail);
    if (tail && upper > 0) {
      double k = 1.0 / pwr(upper, pow);
      double Fb = k * pwr(bound, pow);
      if (Fb < 0) Fb = 0;
      if (Fb > 1) Fb = 1;
      int n = add(out, 0, "For a CDF, use F(upper)=1 to find k.");
      n = add(out, n, "F(x)=kx^%d on 0<x<%.6g", pow, upper);
      n = add(out, n, "F(%.6g)=k*%.6g^%d=1, so k=%.10g", upper, upper, pow, k);
      n = add(out, n, "F(%.6g)=%.10g", bound, Fb);
      return add(out, n, tail > 0 ? "P(X>%.6g)=1-F(%.6g)=%.10g" : "P(X<%.6g)=F(%.6g)=%.10g", bound, bound, tail > 0 ? 1 - Fb : Fb);
    }
    if (upper > 0 && hi > lo) {
      double k = 1.0 / pwr(upper, pow);
      double prob = k * (pwr(hi, pow) - pwr(lo, pow));
      int n = add(out, 0, "For a CDF, use F(upper)=1 to find k.");
      n = add(out, n, "F(%.6g)=k*%.6g^%d=1", upper, upper, pow);
      n = add(out, n, "k = %.10g", k);
      n = add(out, n, "P(%.6g<X<%.6g)=F(%.6g)-F(%.6g)", lo, hi, hi, lo);
      return add(out, n, "= %.10g", prob);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "k(") || has(c, "k*(")) && has(c, "-x)") && has(c, "0<x<")) {
    double upper = 0, lo = 0, hi = 0;
    const char *bp = strstr(c, "0<x<"); if (bp) upper = read_num(bp + 4);
    const char *xp = strstr(c, "p(");
    if (xp) {
      const char *mid = strstr(xp, "<x<");
      if (mid) {
        const char *a0 = mid;
        while (a0 > xp && (isdigit((unsigned char)a0[-1]) || a0[-1] == '.' || a0[-1] == '-')) --a0;
        char left[32]; int len = (int)(mid - a0);
        if (len > 0 && len < (int)sizeof(left)) { memcpy(left, a0, len); left[len] = 0; lo = read_num(left); hi = read_num(mid + 3); }
      }
    }
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (hi <= lo) hi = 0;
    double k = upper ? 2.0/(upper*upper) : 0;
    int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
    n = add(out, n, "integral from 0 to %.6g of k(%.6g-x) dx = 1", upper, upper);
    n = add(out, n, "k = 2/%.6g^2 = %.10g", upper, k);
    if (has(t, "mean") || has(c, "e(x)")) {
      n = add(out, n, "E(X)=integral x*f(x) dx");
      return add(out, n, "mean = %.10g", upper/3.0);
    }
    if (hi > lo) {
      double prob = k * (upper*(hi-lo) - (hi*hi-lo*lo)/2.0);
      n = add(out, n, "P(%.6g<X<%.6g)=integral from %.6g to %.6g of %.10g(%.6g-x) dx", lo, hi, lo, hi, k, upper);
      return add(out, n, "= %.10g", prob);
    }
    const char *gt = strstr(c, "p(x>");
    const char *lt = strstr(c, "p(x<");
    if (gt || lt) {
      double bound = read_num((gt ? gt : lt) + 4);
      double a = gt ? bound : 0, b = gt ? upper : bound;
      double prob = k * (upper*(b-a) - (b*b-a*a)/2.0);
      n = add(out, n, gt ? "P(X>%.6g)=integral from %.6g to %.6g of %.10g(%.6g-x) dx" :
                           "P(X<%.6g)=integral from %.6g to %.6g of %.10g(%.6g-x) dx", bound, a, b, k, upper);
      return add(out, n, gt ? "P(X>%.6g) = %.10g" : "P(X<%.6g) = %.10g", bound, prob);
    }
    return n;
  }
  double pdf_lo = 0, pdf_hi = 0;
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous") || has(t, "randomvariable")) &&
      (has(c, "kx(") || has(c, "k*x(") || has(c, "k*x*(")) &&
      (has(t, "mean") || has(c, "e(x)") || has(t, "normalise") || has(t, "normalize") ||
       has(t, "findk") || has(t, "probability") || has(c, "p(x>") || has(c, "p(x<")) &&
      extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    double upper = pdf_hi;
    const char *kp = strstr(c, "kx(");
    if (!kp) kp = strstr(c, "k*x(");
    if (!kp) kp = strstr(c, "k*x*(");
    const char *op = kp ? strchr(kp, '(') : 0;
    if (op && (isdigit((unsigned char)op[1]) || op[1] == '.')) upper = read_num(op + 1);
    double area = upper*(pdf_hi*pdf_hi - pdf_lo*pdf_lo)/2.0 - (pwr(pdf_hi, 3)-pwr(pdf_lo, 3))/3.0;
    double k = area ? 1.0 / area : 0;
    double mean = k * (upper*(pwr(pdf_hi, 3)-pwr(pdf_lo, 3))/3.0 - (pwr(pdf_hi, 4)-pwr(pdf_lo, 4))/4.0);
    int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
    n = add(out, n, "f(x)=k*x*(%.6g-x), %.6g<x<%.6g", upper, pdf_lo, pdf_hi);
    n = add(out, n, "integral from %.6g to %.6g of k*x*(%.6g-x) dx = 1", pdf_lo, pdf_hi, upper);
    n = add(out, n, "k[%.6g*x^2/2 - x^3/3] from %.6g to %.6g = 1", upper, pdf_lo, pdf_hi);
    n = add(out, n, "k = %.10g", k);
    if (has(t, "mean") || has(c, "e(x)")) {
      n = add(out, n, "E(X)=integral from %.6g to %.6g of x*k*x*(%.6g-x) dx", pdf_lo, pdf_hi, upper);
      return add(out, n, "mean = %.10g", mean);
    }
    const char *gt = strstr(c, "p(x>");
    const char *lt = strstr(c, "p(x<");
    if (gt || lt) {
      double bound = read_num((gt ? gt : lt) + 4);
      double a = gt ? bound : pdf_lo, b = gt ? pdf_hi : bound;
      double prob = k * (upper*(b*b-a*a)/2.0 - (pwr(b, 3)-pwr(a, 3))/3.0);
      n = add(out, n, gt ? "P(X>%.6g)=integral from %.6g to %.6g of k*x*(%.6g-x) dx" :
                           "P(X<%.6g)=integral from %.6g to %.6g of k*x*(%.6g-x) dx", bound, a, b, upper);
      return add(out, n, gt ? "P(X>%.6g) = %.10g" : "P(X<%.6g) = %.10g", bound, prob);
    }
    return n;
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous") || has(t, "randomvariable")) &&
      (has(t, "mean") || has(t, "variance") || has(c, "var(x)") || has(c, "varx")) &&
      !has(c, "p(x>") && !has(c, "p(x<") && has(c, "f(x)=") && has(c, "x^") &&
      !has(c, "kx^") && !has(c, "k*x^") && extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    const char *fx = strstr(c, "f(x)=");
    const char *xp = fx ? strstr(fx, "x^") : 0;
    if (fx && xp && xp > fx + 5) {
      char cb[32]; int len = (int)(xp - (fx + 5));
      if (len > 0 && len < (int)sizeof(cb)) {
        memcpy(cb, fx + 5, len); cb[len] = 0;
        double coef = 1.0;
        char *slash = strchr(cb, '/');
        if (slash) { *slash = 0; coef = read_num(cb) / read_num(slash + 1); }
        else coef = read_num(cb);
        int pow = (int)read_num(xp + 2);
        if (coef != 0 && pow >= 1) {
          double mean = coef * (pwr(pdf_hi, pow + 2) - pwr(pdf_lo, pow + 2)) / (pow + 2);
          double ex2 = coef * (pwr(pdf_hi, pow + 3) - pwr(pdf_lo, pow + 3)) / (pow + 3);
          double var = ex2 - mean * mean;
          int n = add(out, 0, "Use E(X)=integral x f(x) dx and E(X^2)=integral x^2 f(x) dx.");
          n = add(out, n, "f(x) = %.10g*x^%d on %.6g<x<%.6g", coef, pow, pdf_lo, pdf_hi);
          n = add(out, n, "E(X)=integral from %.6g to %.6g of %.10g*x^%d dx = %.10g", pdf_lo, pdf_hi, coef, pow + 1, mean);
          n = add(out, n, "E(X^2)=integral from %.6g to %.6g of %.10g*x^%d dx = %.10g", pdf_lo, pdf_hi, coef, pow + 2, ex2);
          return add(out, n, "variance = Var(X)=E(X^2)-E(X)^2 = %.10g", var);
        }
      }
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous") || has(t, "randomvariable")) &&
      has(c, "exp(-") && (has(c, "p(x>") || has(c, "p(x<"))) {
    const char *ep = strstr(c, "exp(-");
    double lam = ep ? read_num(ep + 5) : 0;
    if (lam <= 0 && nv > 0) lam = v[0] > 0 ? v[0] : -v[0];
    const char *gt = strstr(c, "p(x>");
    const char *lt = strstr(c, "p(x<");
    double bound = 0, ans = 0;
    if (gt) { bound = read_num(gt + (gt[4] == '=' ? 5 : 4)); ans = exp_approx(-lam * bound); }
    if (lt) { bound = read_num(lt + (lt[4] == '=' ? 5 : 4)); ans = 1.0 - exp_approx(-lam * bound); }
    if (lam > 0 && bound >= 0) {
      int n = add(out, 0, "For exponential pdf f(x)=lambda exp(-lambda x).");
      n = add(out, n, "lambda = %.10g", lam);
      if (gt) {
        n = add(out, n, "P(X>%.6g)=exp(-%.10g*%.6g)", bound, lam, bound);
        return add(out, n, "P(X>%.6g) = %.10g", bound, ans);
      }
      n = add(out, n, "P(X<%.6g)=1-exp(-%.10g*%.6g)", bound, lam, bound);
      return add(out, n, "P(X<%.6g) = %.10g", bound, ans);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      has(c, "k/(") && extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    double rb=0, rd=0; int rpow=0;
    if (parse_linear_den_power(c, 'x', &rb, &rd, &rpow) && rpow >= 1) {
      double raw_area = linear_den_antideriv(1.0, rb, rd, rpow, pdf_hi) -
                        linear_den_antideriv(1.0, rb, rd, rpow, pdf_lo);
      double k = raw_area ? 1.0 / raw_area : 0;
      double a = pdf_lo, b = pdf_hi, bound = 0;
      const char *gt = strstr(c, "p(x>");
      const char *lt = strstr(c, "p(x<");
      if (gt) { bound = read_num(gt + (gt[4] == '=' ? 5 : 4)); a = bound; }
      if (lt) { bound = read_num(lt + (lt[4] == '=' ? 5 : 4)); b = bound; }
      double prob = linear_den_antideriv(k, rb, rd, rpow, b) -
                    linear_den_antideriv(k, rb, rd, rpow, a);
      char den[48]; fmt_linear_var(den, sizeof(den), rb, 'x', rd);
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "integral from %.6g to %.6g of k/(%s)^%d dx = 1", pdf_lo, pdf_hi, den, rpow);
      if (rpow == 1) n = add(out, n, "integral of 1/(%s) is (1/%.10g)ln|%s|", den, rb, den);
      n = add(out, n, "k = %.10g", k);
      if (gt) {
        n = add(out, n, "P(X>%.6g)=integral from %.6g to %.6g of %.10g/(%s)^%d dx", bound, a, b, k, den, rpow);
        return add(out, n, "P(X>%.6g) = %.10g", bound, prob);
      }
      if (lt) {
        n = add(out, n, "P(X<%.6g)=integral from %.6g to %.6g of %.10g/(%s)^%d dx", bound, a, b, k, den, rpow);
        return add(out, n, "P(X<%.6g) = %.10g", bound, prob);
      }
      if (has(t, "mean") || has(c, "e(x)")) {
        double mean = linear_den_x_antideriv(k, rb, rd, rpow, pdf_hi) -
                      linear_den_x_antideriv(k, rb, rd, rpow, pdf_lo);
        n = add(out, n, "E(X)=integral from %.6g to %.6g of x*f(x) dx", pdf_lo, pdf_hi);
        return add(out, n, "mean = %.10g", mean);
      }
      if (has(t, "median") && rpow == 1 && rb != 0) {
        double loarg = rb * pdf_lo + rd, hiarg = rb * pdf_hi + rd;
        if (loarg > 0 && hiarg > 0) {
          double med = (exp_approx((ln_approx(loarg) + ln_approx(hiarg)) / 2.0) - rd) / rb;
          n = add(out, n, "For the median m, integral from %.6g to m of f(x) dx = 0.5.", pdf_lo);
          n = add(out, n, "ln|%.10g*m%+.10g| is halfway between the endpoint log values.", rb, rd);
          return add(out, n, "median = %.10g", med);
        }
      }
      return n;
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "k(") || has(c, "k*(")) && extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    const char *kp = strstr(c, "k(");
    if (!kp) kp = strstr(c, "k*(");
    const char *p = strchr(kp, '(');
    double rb=0, rd=0; int rpow=1;
    if (p) {
      ++p;
      const char *e = 0;
      if (!parse_linear_inside(p, 'x', &rb, &rd, &e)) rb = 0;
      else p = e;
      if (rb != 0 && *p == ')') {
        ++p;
        if (*p == '^') rpow = (int)read_num(p + 1);
        if (rpow < 1) rpow = 1;
        double raw_area = linear_power_antideriv(1.0, rb, rd, rpow, pdf_hi) -
                          linear_power_antideriv(1.0, rb, rd, rpow, pdf_lo);
        double k = raw_area ? 1.0 / raw_area : 0;
        double a = pdf_lo, b = pdf_hi, bound = 0;
        const char *gt = strstr(c, "p(x>");
        const char *lt = strstr(c, "p(x<");
        if (gt) { bound = read_num(gt + (gt[4] == '=' ? 5 : 4)); a = bound; }
        if (lt) { bound = read_num(lt + (lt[4] == '=' ? 5 : 4)); b = bound; }
        double prob = linear_power_antideriv(k, rb, rd, rpow, b) -
                      linear_power_antideriv(k, rb, rd, rpow, a);
        char den[48]; fmt_linear_var(den, sizeof(den), rb, 'x', rd);
        int n = add(out, 0, "For a pdf, first normalise: total area under f(x) is 1.");
        n = add(out, n, "integral from %.6g to %.6g of k(%s)^%d dx = 1", pdf_lo, pdf_hi, den, rpow);
        n = add(out, n, "k = %.10g", k);
        if (has(t, "mean") || has(c, "e(x)")) {
          double uh = rb*pdf_hi + rd, ul = rb*pdf_lo + rd;
          double raw_mean = (pwr(uh, rpow + 2) - pwr(ul, rpow + 2)) / (rpow + 2) -
                            rd * (pwr(uh, rpow + 1) - pwr(ul, rpow + 1)) / (rpow + 1);
          double mean = k * raw_mean / (rb * rb);
          n = add(out, n, "E(X)=integral from %.6g to %.6g of x*f(x) dx", pdf_lo, pdf_hi);
          return add(out, n, "mean = %.10g", mean);
        }
        if (gt) {
          n = add(out, n, "P(X>%.6g)=integral from %.6g to %.6g of %.10g(%s)^%d dx", bound, a, b, k, den, rpow);
          return add(out, n, "P(X>%.6g) = %.10g", bound, prob);
        }
        if (lt) {
          n = add(out, n, "P(X<%.6g)=integral from %.6g to %.6g of %.10g(%s)^%d dx", bound, a, b, k, den, rpow);
          return add(out, n, "P(X<%.6g) = %.10g", bound, prob);
        }
        return n;
      }
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "k/(x+") || has(c, "k/(x-")) && has(c, ")^2") &&
      extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    const char *xp = strstr(c, "x+");
    double sh = 0;
    if (xp) sh = read_num(xp + 2);
    else if ((xp = strstr(c, "x-"))) sh = -read_num(xp + 2);
    double area = 1.0/(pdf_lo + sh) - 1.0/(pdf_hi + sh);
    double k = area ? 1.0 / area : 0;
    int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
    n = add(out, n, "integral from %.6g to %.6g of k/(x%+.6g)^2 dx = 1", pdf_lo, pdf_hi, sh);
    n = add(out, n, "k[-1/(x%+.6g)] from %.6g to %.6g = 1", sh, pdf_lo, pdf_hi);
    return add(out, n, "k = %.10g", k);
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "k/x^2") || has(c, "k/(x^2)") || has(c, "kx^-2")) &&
      extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    double area = 1.0/pdf_lo - 1.0/pdf_hi;
    double k = area ? 1.0 / area : 0;
    double a = pdf_lo, b = pdf_hi, bound = 0;
    const char *gt = strstr(c, "p(x>");
    const char *lt = strstr(c, "p(x<");
    if (gt) { bound = read_num(gt + 4); a = bound; }
    if (lt) { bound = read_num(lt + 4); b = bound; }
    double prob = k * (1.0/a - 1.0/b);
    int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
    n = add(out, n, "integral from %.6g to %.6g of k/x^2 dx = 1", pdf_lo, pdf_hi);
    n = add(out, n, "k[-1/x] from %.6g to %.6g = 1", pdf_lo, pdf_hi);
    n = add(out, n, "k = %.10g", k);
    n = add(out, n, "P(%.6g<X<%.6g)=integral from %.6g to %.6g of %.10g/x^2 dx", a, b, a, b, k);
    if (gt) return add(out, n, "P(X>%.6g) = %.10g", bound, prob);
    if (lt) return add(out, n, "P(X<%.6g) = %.10g", bound, prob);
    return add(out, n, "= %.10g", prob);
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      has(c, "kx^") && extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    const char *kp = strstr(c, "kx^");
    int pow = kp && isdigit((unsigned char)kp[3]) ? kp[3] - '0' : 1;
    double area = (pwr(pdf_hi, pow + 1) - pwr(pdf_lo, pow + 1)) / (pow + 1);
    double k = area ? 1.0 / area : 0;
    int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
    n = add(out, n, "integral from %.6g to %.6g of kx^%d dx = 1", pdf_lo, pdf_hi, pow);
    n = add(out, n, "k = %.10g", k);
    if (has(t, "mean") || has(c, "e(x)")) {
      double mean = k * (pwr(pdf_hi, pow + 2) - pwr(pdf_lo, pow + 2)) / (pow + 2);
      n = add(out, n, "E(X)=integral from %.6g to %.6g of x*f(x) dx", pdf_lo, pdf_hi);
      return add(out, n, "mean = %.10g", mean);
    }
    double a = pdf_lo, b = pdf_hi, bound = 0;
    const char *gt = strstr(c, "p(x>");
    const char *lt = strstr(c, "p(x<");
    const char *pp = strstr(c, "p(");
    const char *mid = pp ? strstr(pp, "<x<") : 0;
    if (gt) { bound = read_num(gt + 4); a = bound; }
    else if (lt) { bound = read_num(lt + 4); b = bound; }
    else if (mid) {
      const char *a0 = mid;
      while (a0 > c && (isdigit((unsigned char)a0[-1]) || a0[-1] == '.' || a0[-1] == '-')) --a0;
      char left[32]; int len = (int)(mid - a0);
      if (len > 0 && len < (int)sizeof(left)) { memcpy(left, a0, len); left[len] = 0; a = read_num(left); b = read_num(mid + 3); }
    }
    if (a > b) { double q = a; a = b; b = q; }
    if (b > a) {
      double prob = k * (pwr(b, pow + 1) - pwr(a, pow + 1)) / (pow + 1);
      n = add(out, n, "P(%.6g<X<%.6g)=integral from %.6g to %.6g of %.10g*x^%d dx", a, b, a, b, k, pow);
      if (gt) return add(out, n, "P(X>%.6g) = %.10g", bound, prob);
      if (lt) return add(out, n, "P(X<%.6g) = %.10g", bound, prob);
      return add(out, n, "= %.10g", prob);
    }
    return n;
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) && has(c, "kx^") && has(c, "p(x<")) {
    const char *kp = strstr(c, "kx^");
    int pow = kp && isdigit((unsigned char)kp[3]) ? kp[3] - '0' : 1;
    double upper = 0, bound = 0;
    const char *bp = strstr(c, "0<x<"); if (bp) upper = read_num(bp + 4);
    const char *pp = strstr(c, "p(x<"); if (pp) bound = read_num(pp + 4);
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (bound <= 0) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < upper) bound = v[i];
    if (upper > 0 && bound > 0 && pow >= 1) {
      double k = (pow + 1) / pwr(upper, pow + 1);
      double prob = k * pwr(bound, pow + 1) / (pow + 1);
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "integral from 0 to %.6g of kx^%d dx = 1", upper, pow);
      n = add(out, n, "k = %.10g", k);
      n = add(out, n, "P(X<%.6g)=integral from 0 to %.6g of %.10g*x^%d dx", bound, bound, k, pow);
      return add(out, n, "P(X<%.6g) = %.10g", bound, prob);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) && has(c, "kx^") && has(c, "<x<")) {
    const char *kp = strstr(c, "kx^");
    int pow = kp && isdigit((unsigned char)kp[3]) ? kp[3] - '0' : 1;
    double upper = 0, lo = 0, hi = 0;
    const char *bp = strstr(c, "0<x<"); if (bp) upper = read_num(bp + 4);
    const char *xp = strstr(c, "p(");
    if (xp) {
      const char *mid = strstr(xp, "<x<");
      if (mid) {
        const char *a0 = mid;
        while (a0 > xp && (isdigit((unsigned char)a0[-1]) || a0[-1] == '.' || a0[-1] == '-')) --a0;
        char left[32]; int len = (int)(mid - a0);
        if (len > 0 && len < (int)sizeof(left)) { memcpy(left, a0, len); left[len] = 0; lo = read_num(left); hi = read_num(mid + 3); }
      }
    }
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (upper > 0 && hi > lo && pow >= 1) {
      double k = (pow + 1) / pwr(upper, pow + 1);
      double prob = k * (pwr(hi, pow + 1) - pwr(lo, pow + 1)) / (pow + 1);
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "integral from 0 to %.6g of kx^%d dx = 1", upper, pow);
      n = add(out, n, "k = %.10g", k);
      n = add(out, n, "P(%.6g<X<%.6g)=integral from %.6g to %.6g of %.10g*x^%d dx", lo, hi, lo, hi, k, pow);
      return add(out, n, "= %.10g", prob);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) && has(c, "kx^") && (has(t, "mean") || has(c, "e(x)")) && nv >= 2) {
    const char *kp = strstr(c, "kx^");
    int pow = kp && isdigit((unsigned char)kp[3]) ? kp[3] - '0' : 1;
    double upper = 0;
    const char *bp = strstr(c, "0<x<"); if (bp) upper = read_num(bp + 4);
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (upper > 0 && pow >= 1) {
      double k = (pow + 1) / pwr(upper, pow + 1);
      double mean = k * pwr(upper, pow + 2) / (pow + 2);
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "integral from 0 to %.6g of kx^%d dx = 1", upper, pow);
      n = add(out, n, "k*x^%d/%d from 0 to %.6g = 1", pow + 1, pow + 1, upper);
      n = add(out, n, "k = %.10g", k);
      n = add(out, n, "E(X) = integral x*f(x) dx = integral %.10g*x^%d dx", k, pow + 1);
      return add(out, n, "mean = %.10g", mean);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "k(x+") || has(c, "k*(x+") || has(c, "kx+") || has(c, "k*x+"))) {
    double upper = 0, lower = 0, shift = 1;
    bool got_interval = extract_x_interval(c, &lower, &upper);
    if (!got_interval) extract_0_x_bound(c, &upper);
    const char *kp = strstr(c, "x+");
    if (kp) shift = read_num(kp + 2);
    const char *gt = strstr(c, "p(x>");
    double prob_lo = lower;
    if (gt) prob_lo = read_num(gt + 4);
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (!got_interval && !gt && upper > 0) for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < upper) lower = v[i];
    if (upper > lower) {
      double area = (upper*upper - lower*lower)/2.0 + shift*(upper - lower);
      double k = 1.0 / area;
      double prob = k * ((upper*upper - prob_lo*prob_lo)/2.0 + shift*(upper - prob_lo));
      int n = add(out, 0, "For a pdf, first normalise f(x).");
      n = add(out, n, "integral from %.6g to %.6g of k(x+%.6g) dx = 1", lower, upper, shift);
      n = add(out, n, "k = %.10g", k);
      n = add(out, n, "P(X>%.6g)=integral from %.6g to %.6g of %.10g(x+%.6g) dx", prob_lo, prob_lo, upper, k, shift);
      return add(out, n, "P(X>%.6g) = %.10g", prob_lo, prob);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "k(") || has(c, "k*(")) && (has(c, "-x)") || has(c, "x-")) &&
      extract_x_interval(c, &pdf_lo, &pdf_hi)) {
    double a = 0, b = 1;
    bool descending = has(c, "-x)");
    if (descending) {
      const char *kp = strstr(c, "k(");
      a = kp ? read_num(kp + 2) : 0;
      b = -1;
    } else {
      const char *xp = strstr(c, "x-");
      a = xp ? -read_num(xp + 2) : 0;
      b = 1;
    }
    double area = b*(pdf_hi*pdf_hi - pdf_lo*pdf_lo)/2.0 + a*(pdf_hi - pdf_lo);
    double k = area ? 1.0 / area : 0;
    double prob_lo = pdf_lo, prob_hi = pdf_hi;
    const char *gt = strstr(c, "p(x>");
    const char *lt = strstr(c, "p(x<");
    if (gt) prob_lo = read_num(gt + 4);
    if (lt) prob_hi = read_num(lt + 4);
    double prob = k * (b*(prob_hi*prob_hi - prob_lo*prob_lo)/2.0 + a*(prob_hi - prob_lo));
    int n = add(out, 0, "For a pdf, first normalise f(x).");
    n = add(out, n, "integral from %.6g to %.6g of k(%.6g%+.6gx) dx = 1", pdf_lo, pdf_hi, a, b);
    n = add(out, n, "k = %.10g", k);
    if (has(t, "mean") || has(c, "e(x)")) {
      double mean = k * (b*(pwr(pdf_hi,3)-pwr(pdf_lo,3))/3.0 + a*(pdf_hi*pdf_hi-pdf_lo*pdf_lo)/2.0);
      n = add(out, n, "E(X)=integral from %.6g to %.6g of x*f(x) dx", pdf_lo, pdf_hi);
      return add(out, n, "mean = %.10g", mean);
    }
    n = add(out, n, "P(%.6g<X<%.6g)=integral from %.6g to %.6g of %.10g(%.6g%+.6gx) dx", prob_lo, prob_hi, prob_lo, prob_hi, k, a, b);
    if (gt) return add(out, n, "P(X>%.6g) = %.10g", prob_lo, prob);
    if (lt) return add(out, n, "P(X<%.6g) = %.10g", prob_hi, prob);
    return add(out, n, "= %.10g", prob);
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "p(") && has(c, "<x<")) && !has(c, "x^2") && nv >= 4) {
    double coef = v[0], denom = 1, lo = 0, hi = 0;
    const char *xp = strstr(c, "x/");
    if (xp) denom = read_num(xp + 2);
    const char *pp = strstr(c, "p(");
    if (pp) {
      const char *mid = strstr(pp, "<x<");
      if (mid) { lo = read_num(pp + 2); hi = read_num(mid + 3); }
    }
    if (hi <= lo) { lo = v[nv-2]; hi = v[nv-1]; }
    double ans = (coef/denom)*(hi*hi - lo*lo)/2.0;
    int n = add(out, 0, "Use the pdf and integrate over the requested interval.");
    n = add(out, n, "P(%.6g<X<%.6g) = integral from %.6g to %.6g of (%.6g/%.6g)x dx", lo, hi, lo, hi, coef, denom);
    n = add(out, n, "= (%.6g/%.6g)[x^2/2] from %.6g to %.6g", coef, denom, lo, hi);
    return add(out, n, "P(%.6g<X<%.6g) = %.10g", lo, hi, ans);
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous") || has(t, "randomvariable")) &&
      has(c, "kx") && !has(c, "kx^") && !has(c, "kx(") && !has(c, "k*x(") &&
      (has(t, "mean") || has(c, "e(x)") || has(c, "p(x<") || has(c, "p(x>") ||
       has(t, "findk") || has(t, "probability")) && nv >= 2) {
    double lower = 0, upper = 0;
    if (!extract_x_interval(c, &lower, &upper))
      for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (upper > lower) {
      double area = (upper*upper - lower*lower)/2.0;
      double k = area ? 1.0/area : 0;
      double mean = k*(upper*upper*upper - lower*lower*lower)/3.0;
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "integral from %.6g to %.6g of kx dx = 1", lower, upper);
      n = add(out, n, "k*x^2/2 from %.6g to %.6g gives k*%.6g/2 = 1", lower, upper, upper*upper - lower*lower);
      n = add(out, n, "k = %.10g", k);
      const char *gt = strstr(c, "p(x>");
      const char *lt = strstr(c, "p(x<");
      if (gt || lt) {
        double bound = read_num((gt ? gt : lt) + 4);
        double a = gt ? bound : lower, b = gt ? upper : bound;
        if (a < lower) a = lower;
        if (b > upper) b = upper;
        double prob = k*(b*b - a*a)/2.0;
        n = add(out, n, gt ? "P(X>%.6g)=integral from %.6g to %.6g of %.10g*x dx" :
                             "P(X<%.6g)=integral from %.6g to %.6g of %.10g*x dx", bound, a, b, k);
        return add(out, n, gt ? "P(X>%.6g) = %.10g" : "P(X<%.6g) = %.10g", bound, prob);
      }
      n = add(out, n, "E(X) = integral x*f(x) dx = integral %.10g*x^2 dx", k);
      return add(out, n, "mean = %.10g", mean);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "x^2") || has(c, "xsquared")) && (has(c, "p(x>") || has(t, "greater")) && nv >= 3) {
    double coef = v[0], denom = 1, upper = 0, lower = 0;
    const char *xp = strstr(c, "x^2/");
    if (xp) denom = read_num(xp + 4);
    else if (nv >= 2) denom = v[1];
    const char *bp = strstr(c, "0<x<");
    if (bp) upper = read_num(bp + 4);
    if (upper <= 0) for (int i = 0; i < nv; ++i) if (v[i] > upper && !near_num(v[i], denom)) upper = v[i];
    const char *pp = strstr(c, "p(x>");
    if (pp) lower = read_num(pp + 4);
    else for (int i = 0; i < nv; ++i) if (v[i] > 0 && v[i] < upper) lower = v[i];
    double ans = (coef/denom)*(upper*upper*upper - lower*lower*lower)/3.0;
    int n = add(out, 0, "Use the pdf and integrate over the requested interval.");
    n = add(out, n, "P(X>%.6g) = integral from %.6g to %.6g of (%.6g/%.6g)x^2 dx", lower, lower, upper, coef, denom);
    n = add(out, n, "= (%.6g/%.6g)[x^3/3] from %.6g to %.6g", coef, denom, lower, upper);
    return add(out, n, "P(X>%.6g) = %.10g", lower, ans);
  }
  if ((has(t, "hypothesis") || has(t, "test")) &&
      (has(t, "samplemean") || (has(t, "sample") && has(t, "mean")) || has(t, "xbar")) &&
      (has(t, "populationmean") || has(t, "mu") || has(t, "mean")) &&
      (has(t, "sd") || has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sigma")) && nv >= 4) {
    double xb = 0, mu = 0, sig = 0, n0 = 0, alpha = 0.05;
    bool hXb = label_num(input,"xbar",&xb) || label_num(input,"samplemean",&xb) ||
               word_num(input,"samplemean",&xb) || ((has(t, "sample") || has(t, "xbar")) && word_num(input,"mean",&xb));
    bool hMu = label_num(input,"mu",&mu) || label_num(input,"populationmean",&mu) || word_num(input,"populationmean",&mu);
    bool hSig = label_num(input,"sd",&sig) || label_num(input,"sigma",&sig) || label_num(input,"standarddeviation",&sig) ||
                word_num(input,"sd",&sig) || word_num(input,"sigma",&sig) || word_num(input,"standarddeviation",&sig);
    bool hN = label_num(input,"n",&n0) || word_num(input,"samplesize",&n0) ||
              word_num(input,"sample",&n0) || word_num(input,"size",&n0) || word_num(input,"n",&n0);
    if (!hXb) xb = v[0];
    if (!hMu && (word_num(input,"lessthan",&mu) || word_num(input,"greaterthan",&mu) || word_num(input,"morethan",&mu) ||
                 word_num(input,"exceeds",&mu) || word_num(input,"exceed",&mu) || word_num(input,"below",&mu))) hMu = true;
    if (!hMu) mu = v[1];
    if (!hSig) sig = v[2];
    if (!hN) n0 = v[3];
    for (int i = 0; i < nv; ++i) {
      if (near_num(v[i], xb) || near_num(v[i], mu) || near_num(v[i], sig) || near_num(v[i], n0)) continue;
      if (v[i] > 0 && v[i] <= 10) { alpha = v[i] / 100.0; break; }
      if (v[i] > 0 && v[i] <= 1) { alpha = v[i]; break; }
    }
    double tail = (has(t, "increased") || has(t, "increase") || has(t, "greater") || has(t, "upper") || has(t, "exceeds") || has(t, "exceed")) ? 1 :
                  (has(t, "decreased") || has(t, "decrease") || has(t, "less") || has(t, "lower") || has(t, "below")) ? -1 : 0;
    sprintf(cmd, "hypnormal(%.10g,%.10g,%.10g,%.10g,%.10g,%.0f)", xb, mu, sig, n0, alpha, tail);
    return eval_stats(cmd, out);
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "cx(") || has(c, "c*x(") || has(c, "c*x*(")) &&
      (has(t, "mean") || has(c, "e(x)") || has(t, "normalise") || has(t, "normalize"))) {
    double upper = 0;
    if (!extract_0_x_bound(c, &upper)) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (upper > 0) {
      double k = 6.0 / (upper * upper * upper);
      double mean = upper / 2.0;
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "f(x)=c*x*(%.6g-x), 0<x<%.6g", upper, upper);
      n = add(out, n, "integral from 0 to %.6g of c*x*(%.6g-x) dx = 1", upper, upper);
      n = add(out, n, "c*[%.6g*x^2/2 - x^3/3] from 0 to %.6g = 1", upper, upper);
      n = add(out, n, "c = %.10g", k);
      n = add(out, n, "E(X)=integral x*f(x) dx");
      return add(out, n, "mean = %.10g", mean);
    }
  }
  if ((has(t, "pdf") || has(t, "density") || has(t, "continuous")) &&
      (has(c, "kx(") || has(c, "k*x(") || has(c, "k*x*(")) &&
      (has(t, "mean") || has(c, "e(x)") || has(t, "normalise") || has(t, "normalize") || has(t, "findk"))) {
    double upper = 0;
    if (!extract_0_x_bound(c, &upper)) for (int i = 0; i < nv; ++i) if (v[i] > upper) upper = v[i];
    if (upper > 0) {
      double k = 6.0 / (upper * upper * upper);
      double mean = upper / 2.0;
      int n = add(out, 0, "For a pdf, total area under f(x) is 1.");
      n = add(out, n, "f(x)=k*x*(%.6g-x), 0<x<%.6g", upper, upper);
      n = add(out, n, "integral from 0 to %.6g of k*x*(%.6g-x) dx = 1", upper, upper);
      n = add(out, n, "k*[%.6g*x^2/2 - x^3/3] from 0 to %.6g = 1", upper, upper);
      n = add(out, n, "k = %.10g", k);
      n = add(out, n, "E(X)=integral x*f(x) dx");
      return add(out, n, "mean = %.10g", mean);
    }
  }
  if ((has(t, "confidenceinterval") || (has(t, "confidence") && has(t, "interval"))) &&
      (has(t, "mean") || has(t, "sample")) && nv >= 3) {
    double n0=0, mean=0, sd=0, level=95;
    bool hn=word_num(input,"sample",&n0) || word_num(input,"samplesize",&n0) || label_num(input,"n",&n0);
    bool hm=word_num(input,"mean",&mean) || label_num(input,"mean",&mean);
    bool hs=word_num(input,"standarddeviation",&sd) || word_num(input,"sd",&sd) || label_num(input,"sd",&sd);
    if (!hn) n0 = v[0];
    if (!hm) mean = hn ? v[1] : v[0];
    if (!hs) sd = hn ? v[2] : v[1];
    for (int i = 0; i < nv; ++i) if (!near_num(v[i], n0) && !near_num(v[i], mean) && !near_num(v[i], sd) && v[i] > 50) level = v[i];
    double z = level >= 99 ? 2.576 : (level >= 95 ? 1.96 : (level >= 90 ? 1.645 : 1.96));
    double se = sd / root(n0), m = z * se;
    int n = add(out, 0, "For a confidence interval for a mean, use xbar +/- z*s/sqrt(n).");
    n = add(out, n, "standard error = %.6g/sqrt(%.6g) = %.10g", sd, n0, se);
    n = add(out, n, "%.6g%% gives z = %.6g", level, z);
    n = add(out, n, "margin = %.6g*%.10g = %.10g", z, se, m);
    return add(out, n, "CI = %.10g to %.10g", mean - m, mean + m);
  }
  if ((has(t, "samplemean") || (has(t, "sample") && has(t, "mean"))) &&
      (has(t, "probability") || has(c, "p(") || has(t, "morethan") || has(t, "greaterthan") || has(t, "lessthan")) &&
      !has(t, "hypothesis") && !has(t, "test") && nv >= 4) {
    double mu = v[0], sig = v[1], n0 = v[2], bound = v[3];
    double tmp = 0;
    if (word_num(input, "mean", &tmp) || word_num(input, "mu", &tmp)) mu = tmp;
    if (word_num(input, "standarddeviation", &tmp) || word_num(input, "sd", &tmp)) sig = tmp;
    if (label_num(input, "n", &tmp)) n0 = tmp;
    if (word_num(input, "sampleofsize", &tmp) || word_num(input, "sampleof", &tmp) ||
        word_num(input, "samplesize", &tmp) || word_num(input, "size", &tmp)) n0 = tmp;
    const char *sof2 = strstr(c, "sampleof");
    if (sof2 && isdigit((unsigned char)sof2[8])) n0 = read_num(sof2 + 8);
    if (has(t, "between") || has(c, "<xbar<") || has(c, "<samplemean<")) {
      double bounds[2]; int bc = 0;
      for (int i = 0; i < nv && bc < 2; ++i) {
        if (has(c, "^2") && near_num(v[i], 2)) continue;
        if (near_num(v[i], mu) || near_num(v[i], sig) || near_num(v[i], n0)) continue;
        bounds[bc++] = v[i];
      }
      if (bc == 2) {
        double lo = bounds[0] < bounds[1] ? bounds[0] : bounds[1];
        double hi = bounds[0] < bounds[1] ? bounds[1] : bounds[0];
        double se = sig / root(n0);
        double z1 = se ? (lo - mu) / se : 0, z2 = se ? (hi - mu) / se : 0;
        double prob = normal_cdf(z2) - normal_cdf(z1);
        int n = add(out, 0, "For a sample mean, use the sampling distribution.");
        n = add(out, n, "Xbar ~ N(%.6g, (%.6g/sqrt(%.6g))^2)", mu, sig, n0);
        n = add(out, n, "standard error = %.6g/sqrt(%.6g) = %.10g", sig, n0, se);
        n = add(out, n, "z1 = (%.6g-%.6g)/%.10g = %.10g", lo, mu, se, z1);
        n = add(out, n, "z2 = (%.6g-%.6g)/%.10g = %.10g", hi, mu, se, z2);
        return add(out, n, "P(%.6g<Xbar<%.6g) = %.10g", lo, hi, prob);
      }
    }
    for (int i = 0; i < nv; ++i) {
      if (has(c, "^2") && near_num(v[i], 2)) continue;
      if (near_num(v[i], sig) || near_num(v[i], n0)) continue;
      if (near_num(v[i], mu)) continue;
      bound = v[i];
    }
    double se = sig / root(n0);
    double z = se ? (bound - mu) / se : 0;
    bool upper = has(t, "morethan") || has(t, "greaterthan") || (has(t, "more") && has(t, "than")) ||
                 (has(t, "greater") && has(t, "than")) || has(c, "xbar>") || has(c, "mean>");
    double prob = upper ? 1 - normal_cdf(z) : normal_cdf(z);
    int n = add(out, 0, "For a sample mean, use the sampling distribution.");
    n = add(out, n, "Xbar ~ N(%.6g, (%.6g/sqrt(%.6g))^2)", mu, sig, n0);
    n = add(out, n, "standard error = %.6g/sqrt(%.6g) = %.10g", sig, n0, se);
    n = add(out, n, "z = (%.6g-%.6g)/%.10g = %.10g", bound, mu, se, z);
    return add(out, n, "P(Xbar%s%.6g) = %.10g", upper ? ">" : "<", bound, prob);
  }
  if ((has(t, "interpolation") || has(t, "interpolate") || has(t, "estimate")) &&
      !(has(t, "regression") || has(t, "leastsquares") || has(t, "lineofbestfit")) &&
      (has(t, "whenx") || has(c, "atx") || has(c, "x=") || has(t, "values") || has(t, "points")) && nv >= 5) {
    double x1=v[0], y1=v[1], x2=v[2], y2=v[3], x=v[4];
    double y = y1 + (x - x1) * (y2 - y1) / (x2 - x1);
    int n = add(out, 0, "Use linear interpolation between the two surrounding points.");
    n = add(out, n, "gradient = (%.6g-%.6g)/(%.6g-%.6g)", y2, y1, x2, x1);
    n = add(out, n, "y = %.6g + (%.6g-%.6g)*(%.6g-%.6g)/(%.6g-%.6g)", y1, x, x1, y2, y1, x2, x1);
    return add(out, n, "y = %.10g", y);
  }
  if ((has(t, "data") || has(t, "values") || has(t, "observations")) &&
      (has(t, "mean") || has(t, "variance") || has(t, "standarddeviation") ||
       (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) &&
      !has(t, "sumx") && !has(t, "sumofsquares") && !has(c, "sumx") &&
      !has(c, "sumxsquared") && !has(c, "sumxsquare") &&
      !has(t, "normal") && !has(t, "binom") &&
      !has(t, "poisson") && !has(t, "added") && !has(t, "add") &&
      !has(t, "removed") && !has(t, "remove") && !has(t, "combined") &&
      !has(t, "another") && !has(c, "hasmean") && !has(c, "havemean") &&
      !has(c, "meanof") && nv >= 2) {
    double sx = 0, sx2 = 0;
    for (int i = 0; i < nv; ++i) { sx += v[i]; sx2 += v[i] * v[i]; }
    double n0 = nv, mean = sx / n0, var = sx2 / n0 - mean * mean;
    int n = add(out, 0, "For raw data, first find sum x and sum x^2.");
    n = add(out, n, "n = %.0f, sum x = %.10g", n0, sx);
    n = add(out, n, "sum x^2 = %.10g", sx2);
    n = add(out, n, "mean = sum x/n = %.10g/%.0f = %.10g", sx, n0, mean);
    n = add(out, n, "variance = sum x^2/n - mean^2 = %.10g/%.0f - %.10g^2 = %.10g", sx2, n0, mean, var);
    if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd"))
      return add(out, n, "sd = sqrt(variance) = %.10g", root(var));
    return n;
  }
  if ((has(t, "summarystatistics") || has(t, "summary") || has(t, "sumx") || has(t, "sumofsquares") ||
       (has(t, "sum") && (has(t, "squared") || has(t, "squares")))) &&
      (has(t, "mean") || has(t, "variance") || has(t, "standarddeviation") || has(t, "sd")) &&
      !has(t, "added") && !has(t, "add") && !has(t, "removed") && !has(t, "remove") &&
      !has(t, "normal") && !has(t, "binom") && !has(t, "poisson")) {
    double n0 = 0, sx = 0, sx2 = 0;
    bool hn = label_num(input, "n", &n0) || word_num(input, "n", &n0) || word_num(input, "sample", &n0);
    bool hsx = label_num(input, "sumx", &sx) || label_num(input, "sx", &sx) || word_num(input, "sumx", &sx);
    bool hsx2 = label_num(input, "sumx2", &sx2) || label_num(input, "sx2", &sx2) ||
                label_num(input, "sumxsquared", &sx2) || label_num(input, "sumofsquares", &sx2) ||
                word_num(input, "sumxsquared", &sx2) || word_num(input, "sumofsquares", &sx2);
    if (!hn && nv >= 1) n0 = v[0];
    if (!hsx && nv >= 2) sx = v[1];
    if (!hsx2 && nv >= 3) sx2 = v[2];
    if (n0 > 0 && sx2 > 0) {
      double mean = sx / n0;
      double var = sx2 / n0 - mean * mean;
      int n = add(out, 0, "Use summary statistics formulae.");
      n = add(out, n, "mean = Sx/n = %.10g/%.10g = %.10g", sx, n0, mean);
      n = add(out, n, "variance = Sx2/n - mean^2");
      n = add(out, n, "variance = %.10g/%.10g - %.10g^2 = %.10g", sx2, n0, mean, var);
      if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd"))
        return add(out, n, "sd = sqrt(variance) = %.10g", root(var));
      return n;
    }
  }
  if (has(t, "mean") && nv >= 3 &&
      (has(t, "added") || has(t, "add") || has(t, "another")) &&
      (has(t, "becomes") || has(t, "newmean") || has(c, "meanbecomes")) &&
      !has(t, "normal") && !has(t, "binom") && !has(t, "poisson")) {
    double count = v[0], old_mean = v[1], new_mean = v[2];
    double old_total = count * old_mean;
    double new_total = (count + 1) * new_mean;
    int n = add(out, 0, "Use total = number of values * mean.");
    n = add(out, n, "old total = %.10g*%.10g = %.10g", count, old_mean, old_total);
    n = add(out, n, "new number of values = %.10g + 1 = %.10g", count, count + 1);
    n = add(out, n, "new total = %.10g*%.10g = %.10g", count + 1, new_mean, new_total);
    return add(out, n, "added value = %.10g - %.10g = %.10g", new_total, old_total, new_total - old_total);
  }
	  if (has(t, "mean") && nv >= 3 &&
	      (has(t, "removed") || has(t, "remove") || has(t, "added") || has(t, "add")) &&
	      !has(t, "normal") && !has(t, "binom") && !has(t, "poisson")) {
	    double count = v[0], mean = v[1], value = v[nv-1];
	    bool adding = has(t, "added") || has(t, "add");
	    if ((has(t, "variance") || has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) && nv >= 4) {
	      double old_var = 0, labelled_value = 0;
	      bool hv = word_num(input, "variance", &old_var) || word_num(input, "sd", &old_var) ||
	                word_num(input, "standarddeviation", &old_var);
	      bool hval = word_num(input, "value", &labelled_value);
	      if (hval) value = labelled_value;
	      if (hv && !near_num(old_var, value)) {
	        double old_total = count * mean;
	        double new_total = adding ? old_total + value : old_total - value;
	        double new_count = adding ? count + 1 : count - 1;
	        double new_mean = new_count ? new_total/new_count : 0;
	        if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) old_var *= old_var;
	        double old_sx2 = count * (old_var + mean*mean);
	        double new_sx2 = adding ? old_sx2 + value*value : old_sx2 - value*value;
	        double variance = new_count ? new_sx2/new_count - new_mean*new_mean : 0;
	        int n = add(out, 0, "Use total = number of values * mean.");
	        n = add(out, n, "old total = %.10g*%.10g = %.10g", count, mean, old_total);
	        n = add(out, n, adding ? "new total = %.10g + %.10g = %.10g" : "new total = %.10g - %.10g = %.10g", old_total, value, new_total);
	        n = add(out, n, adding ? "new number of values = %.10g + 1 = %.10g" : "new number of values = %.10g - 1 = %.10g", count, new_count);
	        n = add(out, n, "new mean = %.10g/%.10g = %.10g", new_total, new_count, new_mean);
	        n = add(out, n, "Use variance = sum x^2/n - mean^2, so sum x^2 = n(variance+mean^2).");
	        n = add(out, n, "old sum x^2 = %.10g*(%.10g+%.10g^2) = %.10g", count, old_var, mean, old_sx2);
	        n = add(out, n, adding ? "new sum x^2 = %.10g + %.10g^2 = %.10g" : "new sum x^2 = %.10g - %.10g^2 = %.10g", old_sx2, value, new_sx2);
	        n = add(out, n, "variance = %.10g/%.10g - %.10g^2 = %.10g", new_sx2, new_count, new_mean, variance);
	        if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) return add(out, n, "sd = sqrt(variance) = %.10g", root(variance));
	        return n;
	      }
	    }
	    double old_total = count * mean;
	    double new_total = adding ? old_total + value : old_total - value;
	    double new_count = adding ? count + 1 : count - 1;
	    int n = add(out, 0, "Use total = number of values * mean.");
	    n = add(out, n, "old total = %.10g*%.10g = %.10g", count, mean, old_total);
	    n = add(out, n, adding ? "new total = %.10g + %.10g = %.10g" : "new total = %.10g - %.10g = %.10g", old_total, value, new_total);
	    n = add(out, n, adding ? "new number of values = %.10g + 1 = %.10g" : "new number of values = %.10g - 1 = %.10g", count, new_count);
	    double new_mean = new_count ? new_total/new_count : 0;
	    n = add(out, n, "new mean = %.10g/%.10g = %.10g", new_total, new_count, new_mean);
	    if ((has(t, "variance") || has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) &&
	        (has(c, "sumofsquares") || (has(t, "sum") && has(t, "squares"))) && nv >= 4) {
	      double old_sx2 = v[2];
	      double new_sx2 = adding ? old_sx2 + value*value : old_sx2 - value*value;
	      double variance = new_count ? new_sx2/new_count - new_mean*new_mean : 0;
	      n = add(out, n, adding ? "new sum x^2 = %.10g + %.10g^2 = %.10g" : "new sum x^2 = %.10g - %.10g^2 = %.10g", old_sx2, value, new_sx2);
	      n = add(out, n, "variance = sum x^2/n - mean^2");
	      n = add(out, n, "variance = %.10g/%.10g - %.10g^2 = %.10g", new_sx2, new_count, new_mean, variance);
	      if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) return add(out, n, "sd = sqrt(variance) = %.10g", root(variance));
	      return n;
	    }
	    if ((has(t, "variance") || has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) && nv >= 4) {
	      double old_var = v[2];
	      if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) old_var *= old_var;
	      double old_sx2 = count * (old_var + mean*mean);
	      double new_sx2 = adding ? old_sx2 + value*value : old_sx2 - value*value;
	      double variance = new_count ? new_sx2/new_count - new_mean*new_mean : 0;
	      n = add(out, n, "Use variance = sum x^2/n - mean^2, so sum x^2 = n(variance+mean^2).");
	      n = add(out, n, "old sum x^2 = %.10g*(%.10g+%.10g^2) = %.10g", count, old_var, mean, old_sx2);
	      n = add(out, n, adding ? "new sum x^2 = %.10g + %.10g^2 = %.10g" : "new sum x^2 = %.10g - %.10g^2 = %.10g", old_sx2, value, new_sx2);
	      n = add(out, n, "variance = %.10g/%.10g - %.10g^2 = %.10g", new_sx2, new_count, new_mean, variance);
	      if (has(t, "standarddeviation") || (has(t, "standard") && has(t, "deviation")) || has(t, "sd")) return add(out, n, "sd = sqrt(variance) = %.10g", root(variance));
	      return n;
	    }
	    return n;
	  }
  if ((has(t, "combinedmean") || (has(t, "combined") && has(t, "mean")) ||
       ((has(t, "another") || has(t, "second")) && has(t, "sample") && has(t, "mean"))) &&
      nv >= 4 && !has(t, "normal") && !has(t, "binom") && !has(t, "poisson")) {
    double n1 = v[0], m1 = v[1], n2 = v[2], m2 = v[3];
    double total1 = n1*m1, total2 = n2*m2, combined = (total1 + total2) / (n1 + n2);
    int n = add(out, 0, "Use total = number of values * mean.");
    n = add(out, n, "first total = %.10g*%.10g = %.10g", n1, m1, total1);
    n = add(out, n, "second total = %.10g*%.10g = %.10g", n2, m2, total2);
    n = add(out, n, "combined total = %.10g + %.10g = %.10g", total1, total2, total1 + total2);
    return add(out, n, "combined mean = %.10g/%.10g = %.10g", total1 + total2, n1 + n2, combined);
  }
  if ((has(t, "total") || has(t, "sum")) && has(t, "sample") && has(t, "mean") &&
      nv >= 2 && !has(t, "normal") && !has(t, "binom") && !has(t, "poisson")) {
    double count = 0, mean = 0;
    bool hc = word_num(input, "sampleof", &count) || word_num(input, "sample", &count) || label_num(input, "n", &count);
    bool hm = word_num(input, "mean", &mean) || label_num(input, "mean", &mean);
    if (!hc) count = v[0];
    if (!hm) mean = v[1];
    int n = add(out, 0, "Use total = number of values * mean.");
    return add(out, n, "total = %.10g*%.10g = %.10g", count, mean, count * mean);
  }
  if ((has(t, "mean") || has(t, "variance") || has(t, "standarddeviation")) && nv >= 3 &&
      !has(t, "normal") && !has(t, "binom") && !has(t, "poisson") &&
      !has(t, "discrete") && !has(t, "randomvariable") && !has(t, "grouped") &&
      !has(t, "pdf") && !has(t, "density") && !has(t, "continuous")) {
    sprintf(cmd, "meanvar(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "stratified") || has(t, "stratum")) && nv >= 3) {
    double pop=0, group=0, sample=0;
    bool hPop=word_num(input,"population",&pop), hGroup=word_num(input,"stratum",&group) || word_num(input,"group",&group);
    bool hSample=word_num(input,"sample",&sample);
    if (hPop && hSample && nv >= 4 && (has(t, "groups") || has(t, "group") || has(t, "sizes") || has(t, "strata"))) {
      int n = add(out, 0, "For stratified sampling, use stratum size / total population * sample size.");
      n = add(out, n, "total population = %.10g", pop);
      int si = 1;
      for (int i = 0; i < nv && n < P3_MAX_LINES - 1; ++i) {
        if (near_num(v[i], pop) || near_num(v[i], sample)) continue;
        n = add(out, n, "stratum %d: %.6g/%.6g * %.6g = %.10g", si++, v[i], pop, sample, pop ? v[i]/pop*sample : 0);
      }
      return add(out, n, "round sensibly so the samples add to %.10g.", sample);
    }
    if ((has(c, "groupsizes") || (has(t, "group") && has(t, "sizes"))) &&
        (has(c, "totalpopulation") || (has(t, "total") && has(t, "population"))) &&
        (has(c, "totalsample") || (has(t, "total") && has(t, "sample"))) && nv >= 4) {
      double total = v[0], sample = v[1];
      int n = add(out, 0, "For stratified sampling, use stratum size / total population * sample size.");
      n = add(out, n, "total population = %.10g", total);
      for (int i = 2; i < nv && n < P3_MAX_LINES - 1; ++i)
        n = add(out, n, "stratum %d: %.6g/%.6g * %.6g = %.10g", i - 1, v[i], total, sample, total ? v[i]/total*sample : 0);
      return add(out, n, "round sensibly so the samples add to %.10g.", sample);
    }
    if ((has(t, "sizes") || has(t, "strata") || has(t, "stratums")) && nv >= 4) {
      double sample = v[nv - 1], total = 0;
      for (int i = 0; i < nv - 1; ++i) total += v[i];
      int n = add(out, 0, "For stratified sampling, use stratum size / total population * sample size.");
      n = add(out, n, "total population = %.10g", total);
      for (int i = 0; i < nv - 1 && n < P3_MAX_LINES - 1; ++i)
        n = add(out, n, "stratum %d: %.6g/%.6g * %.6g = %.10g", i + 1, v[i], total, sample, total ? v[i]/total*sample : 0);
      return add(out, n, "round sensibly so the samples add to %.10g.", sample);
    }
    if (hPop && hSample && !hGroup) {
      for (int i = 0; i < nv; ++i) if (!near_num(v[i], pop) && !near_num(v[i], sample)) { group = v[i]; hGroup = true; break; }
    }
    if (hPop && hGroup && hSample) sprintf(cmd, "stratified(%.10g,%.10g,%.10g)", pop, group, sample);
    else sprintf(cmd, "stratified(%.10g,%.10g,%.10g)", v[0], v[1], v[2]);
    return eval_stats(cmd, out);
  }
  if ((has(t, "grouped") || has(t, "group") || has(t, "classes")) && has(t, "median") &&
      (has(t, "frequency") || has(t, "frequencies")) && nv >= 6 && nv % 3 == 0) {
    int m = nv / 3, fs = 2*m;
    double total = 0; for (int i = 0; i < m; ++i) total += v[fs+i];
    double pos = total/2.0, cf = 0;
    for (int i = 0; i < m; ++i) {
      double f = v[fs+i], lo = abs_num(v[2*i]), hi = abs_num(v[2*i+1]);
      if (hi < lo) { double q = lo; lo = hi; hi = q; }
      if (cf + f >= pos) {
        double w = hi - lo, med = lo + ((pos - cf)/f)*w;
        int n = add(out, 0, "Use linear interpolation in the median class.");
        n = add(out, n, "total frequency n = %.10g, so position = n/2 = %.10g", total, pos);
        n = add(out, n, "median class is %.6g to %.6g; CF before = %.6g, f = %.6g", lo, hi, cf, f);
        n = add(out, n, "median = L + ((position-CF before)/f)*class width");
        return add(out, n, "median = %.6g + ((%.6g-%.6g)/%.6g)*%.6g = %.10g", lo, pos, cf, f, w, med);
      }
      cf += f;
    }
  }
  if ((has(t, "grouped") || has(t, "interpolate") || has(t, "interpolation")) && has(t, "median") && nv >= 5) {
    sprintf(cmd, "groupmedian(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4]); return eval_stats(cmd, out);
  }
  if ((has(t, "grouped") || has(t, "interpolate") || has(t, "interpolation")) && (has(t, "quartile") || has(t, "quantile")) && nv >= 5) {
    double q = nv >= 6 ? v[5] : (has(t, "upper") || has(t, "q3") ? 0.75 : (has(t, "lower") || has(t, "q1") ? 0.25 : 0.5));
    sprintf(cmd, "groupquantile(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4], q); return eval_stats(cmd, out);
  }
  if ((has(t, "histogram") || has(t, "frequencydensity") || has(t, "classwidth")) && (has(t, "density") || has(t, "densities") || has(t, "frequencydensity") || has(t, "width")) && nv >= 2) {
    if (has(t, "class") && has(t, "density") &&
        !has(c, "findfrequencydensity") && !has(c, "calculatefrequencydensity") &&
        (has(c, "findfrequencies") || has(c, "findthefrequencies") || has(c, "findfrequency")) &&
        nv >= 6 && nv % 3 == 0) {
      int n = add(out, 0, "For each histogram class, frequency = frequency density * class width.");
      for (int i = 0; i + 2 < nv && n < P3_MAX_LINES; i += 3) {
        double lo = abs_num(v[i]), hi = abs_num(v[i + 1]), dens = v[i + 2];
        if (hi < lo) { double q = lo; lo = hi; hi = q; }
        double w = hi - lo;
        n = add(out, n, "class %.10g-%.10g: frequency = %.10g*%.10g = %.10g", lo, hi, dens, w, dens*w);
      }
      return n;
    }
    if ((has(t, "classinterval") || (has(t, "class") && has(t, "interval"))) &&
        (has(t, "densities") || has(t, "frequencydensities")) &&
        (has(t, "frequencies") || has(c, "findthefrequencies") || has(c, "findfrequencies")) &&
        nv >= 6 && nv % 3 == 0) {
      int m = nv / 3;
      int n = add(out, 0, "For each histogram class, frequency = frequency density * class width.");
      for (int i = 0; i < m && n < P3_MAX_LINES; ++i) {
        double lo = abs_num(v[2*i]), hi = abs_num(v[2*i + 1]), dens = v[2*m + i];
        if (hi < lo) { double q = lo; lo = hi; hi = q; }
        double w = hi - lo;
        n = add(out, n, "class %.10g-%.10g: frequency = %.10g*%.10g = %.10g", lo, hi, dens, w, dens*w);
      }
      return n;
    }
    if (has(t, "classes") && (has(t, "frequencies") || has(t, "frequency")) && nv >= 6 && nv % 3 == 0) {
      int m = nv / 3;
      int n = add(out, 0, "For each histogram class, frequency density = frequency / class width.");
      for (int i = 0; i < m && n < P3_MAX_LINES; ++i) {
        double lo = abs_num(v[2*i]), hi = abs_num(v[2*i + 1]), f = v[2*m + i];
        if (hi < lo) { double q = lo; lo = hi; hi = q; }
        double w = hi - lo;
        n = add(out, n, "class %.10g-%.10g: density = %.10g/%.10g = %.10g", lo, hi, f, w, w ? f / w : 0);
      }
      return n;
    }
    double freq=0, dens=0, width0=0;
    bool hW = word_num(input,"classwidth",&width0) || word_num(input,"width",&width0);
    bool hD = word_num(input,"frequencydensity",&dens) || word_num(input,"density",&dens);
    bool hF = word_num(input,"frequency",&freq);
    if ((has(t, "classinterval") || (has(t, "class") && has(t, "interval"))) &&
        has(t, "frequency") && hD && nv >= 3 &&
        !has(c, "findfrequencydensity") &&
        (has(c, "findfrequency") || has(c, "calculatefrequency") || !hF)) {
      double lo = v[0], hi = v[1] < 0 ? -v[1] : v[1];
      if (hi < lo) { double q = lo; lo = hi; hi = q; }
      width0 = hi - lo;
      int n = add(out, 0, "For a histogram, frequency = frequency density * class width.");
      n = add(out, n, "class width = %.10g - %.10g = %.10g", hi, lo, width0);
      return add(out, n, "frequency = %.10g*%.10g = %.10g", dens, width0, dens * width0);
    }
    if ((has(t, "classinterval") || (has(t, "class") && has(t, "interval"))) &&
        has(t, "frequency") && nv >= 3 && !hD) {
      double lo = v[0], hi = v[1];
      if (hi < lo) { double q = lo; lo = hi; hi = q; }
      freq = hF ? freq : v[2];
      width0 = hi - lo;
      int n = add(out, 0, "For a histogram, frequency density = frequency / class width.");
      n = add(out, n, "class width = %.10g - %.10g = %.10g", hi, lo, width0);
      return add(out, n, "frequency density = %.10g/%.10g = %.10g", freq, width0, width0 ? freq/width0 : 0);
    }
    if (!hW && has(t, "class") && has(t, "to")) {
      double lo=0, hi=0;
      if (word_num(input, "class", &lo) && word_num(input, "to", &hi) && hi > lo) {
        width0 = hi - lo;
        hW = true;
      }
    }
    if ((has(c, "findclasswidth") || has(c, "findwidth") || (has(t, "calculate") && has(t, "width"))) && hF && hD) {
      int n = add(out, 0, "For a histogram, frequency density = frequency / class width.");
      n = add(out, n, "class width = frequency / frequency density");
      return add(out, n, "class width = %.10g/%.10g = %.10g", freq, dens, freq / dens);
    }
    if ((has(c, "findfrequency") || has(c, "calculatefrequency") || (has(t, "calculate") && has(t, "frequency"))) &&
        !has(c, "findfrequencydensity") && !has(c, "calculatefrequencydensity") && hW && (hD || hF))
      sprintf(cmd, "histfreq(%.10g,%.10g)", hD ? dens : freq, width0);
    else if (hF && hW) sprintf(cmd, "histdensity(%.10g,%.10g)", freq, width0);
    else if ((has(c, "findfrequency") || has(c, "findthefrequency")) && !has(c, "findfrequencydensity")) {
      if (has(t, "class") && has(t, "width") && (has(t, "density") || has(t, "frequencydensity")))
        sprintf(cmd, "histfreq(%.10g,%.10g)", v[1], v[0]);
      else sprintf(cmd, "histfreq(%.10g,%.10g)", v[0], v[1]);
    }
    else if (has(t, "frequencyfromdensity")) sprintf(cmd, "histfreq(%.10g,%.10g)", v[0], v[1]);
    else sprintf(cmd, "histdensity(%.10g,%.10g)", v[0], v[1]);
    return eval_stats(cmd, out);
  }
  return 0;
}

int p3_eval(const char *input, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  for (int i=0;i<P3_MAX_LINES;++i) out[i][0]=0;
  char s[192]; clean(input, s, sizeof(s));
  if (!s[0]) return add(out, 0, "Enter a Paper 3 command.");
  if ((has(s, "acceleration") || has(s, "accn")) && has(s, "/")) {
    int nf = eval_free_text(input, out); if (nf) return nf;
  }
  if (has(s, "given") && has(s, "n(")) {
    int ns = eval_stats(s, out); if (ns) return ns;
    int nf = eval_free_text(input, out); if (nf) return nf;
  }
  if (has(s, "|") && (has(s, "~n(") || has(s, "normal"))) {
    int nf = eval_free_text(input, out); if (nf) return nf;
  }
  if ((has(s, "samplemean") || (has(s, "sample") && has(s, "mean"))) &&
      (has(s, "normal") || has(s, "~n(") || has(s, "populationmean"))) {
    int nf = eval_free_text(input, out); if (nf) return nf;
  }
  int n = eval_suvat(s, out); if (n) return n;
  n = eval_mech(s, out); if (n) return n;
  n = eval_stats(s, out); if (n) return n;
  n = eval_free_text(input, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "suvat projectile projectileh projectileat projectileangle force weight friction moment incline inclineacc");
  n = add(out, n, "beam ladder connected pulley impulse momentum work power energy workenergyforce restitution vector resolve vectorkin varacc");
  return add(out, n, "normal normal_work normalvar normalprob normaltail normalcond invnormal normalparams binom binom_work binomstats binomtail critbinom hypbinom hyp_test cond prob_work probor bayes independent poisson poissonstats poissontail poissonnorm critpoisson hyppoisson regress regress_work pmcc spearman meanvar discrete stratified groupmedian histdensity code");
}

#include "cscalc_engine.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

static bool starts(const char *s, const char *p) {
  return strncmp(s, p, strlen(p)) == 0;
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

static int bin_unsigned(const char *s) {
  int v = 0;
  for (int i = 0; s[i]; ++i) if (s[i] == '0' || s[i] == '1') v = v * 2 + (s[i] - '0');
  return v;
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

static void to_bin(long long v, int width, char *buf) {
  unsigned long long mask = width >= 63 ? ~0ULL : ((1ULL << width) - 1);
  unsigned long long u = ((unsigned long long)v) & mask;
  for (int i = width - 1, j = 0; i >= 0; --i, ++j) buf[j] = ((u >> i) & 1) ? '1' : '0';
  buf[width] = 0;
}

static double fixed_decode(const char *s) {
  const char *dot = strchr(s, '.');
  double v = 0.0;
  int int_len = dot ? (int)(dot - s) : (int)strlen(s);
  for (int i = 0; i < int_len; ++i) if (s[i] == '1') v += pow2(int_len - 1 - i);
  if (dot) for (int i = 1; dot[i]; ++i) if (dot[i] == '1') v += pow2(-i);
  return v;
}

static double mantissa_decode(const char *s) {
  double v = s[0] == '1' ? -1.0 : 0.0;
  for (int i = 1; s[i]; ++i) if (s[i] == '1') v += pow2(-i);
  return v;
}

static int conv(char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN], char a[][48], int na) {
  if (na < 1) return add(out, 0, "Use bin(n), hex(n), den(bits,2), convert(n,from,to).");
  if (starts(a[0], "0b")) {
    int v = bin_unsigned(a[0] + 2);
    int n = add(out, 0, "Binary place values: 128 64 32 16 8 4 2 1");
    return add(out, n, "%s_2 = %d_10", a[0] + 2, v);
  }
  return 0;
}

static int eval_base(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts(s, "bin(") && na == 1) {
    long long v = parse_int(a[0]); char b[65]; int w = v < 256 ? 8 : 16; to_bin(v, w, b);
    int n = add(out, 0, "Repeated division by 2, read remainders upwards.");
    return add(out, n, "%lld_10 = %s_2", v, b);
  }
  if (starts(s, "hex(") && na == 1) {
    long long v = parse_int(a[0]);
    int n = add(out, 0, "Convert denary to hex by repeated division by 16.");
    return add(out, n, "%lld_10 = %llX_16", v, v);
  }
  if (starts(s, "den(") && na >= 1) {
    int base = na > 1 ? (int)parse_int(a[1]) : (is_bits(a[0]) ? 2 : 16);
    long long v = parse_base(a[0], base);
    int n = add(out, 0, "Multiply each digit by its base place value.");
    return add(out, n, "%s_%d = %lld_10", a[0], base, v);
  }
  if (starts(s, "convert(") && na == 3) {
    long long v = parse_base(a[0], (int)parse_int(a[1]));
    if (parse_int(a[2]) == 2) { char b[65]; to_bin(v, v < 256 ? 8 : 16, b); return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %s_2", v, b); }
    if (parse_int(a[2]) == 16) return add(out, add(out, 0, "%s_%s = %lld_10", a[0], a[1], v), "%lld_10 = %llX_16", v, v);
    return add(out, 0, "%s_%s = %lld_10", a[0], a[1], v);
  }
  return conv(out, a, na);
}

static int eval_twos(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts(s, "twosrange(") && na == 1) {
    int w = (int)parse_int(a[0]);
    return add(out, add(out, 0, "n-bit two's complement range:"), "-2^(%d) to 2^(%d)-1 = %d to %d", w-1, w-1, -(1<<(w-1)), (1<<(w-1))-1);
  }
  if ((starts(s, "twosdec(") || starts(s, "tcdec(")) && na == 1) {
    int n = add(out, 0, "MSB is sign bit.");
    if (a[0][0] == '0') n = add(out, n, "MSB=0, so use unsigned place values.");
    else n = add(out, n, "MSB=1, so subtract 2^%d from unsigned value.", (int)strlen(a[0]));
    return add(out, n, "%s = %d", a[0], twos_decode(a[0]));
  }
  if ((starts(s, "twos(") || starts(s, "tc(")) && na >= 2) {
    long long v = parse_int(a[0]); int w = (int)parse_int(a[1]); char b[65]; to_bin(v, w, b);
    int n = add(out, 0, "%d-bit two's complement.", w);
    if (v < 0) n = add(out, n, "Encode negative: add 2^%d to %lld.", w, v);
    return add(out, n, "%lld -> %s", v, b);
  }
  if (starts(s, "twosadd(") && na == 2) {
    int w = (int)strlen(a[0]), x = twos_decode(a[0]), y = twos_decode(a[1]); char b[65]; to_bin(x+y, w, b);
    int n = add(out, 0, "Decode/add in fixed width, discard carry beyond %d bits.", w);
    n = add(out, n, "%s=%d, %s=%d", a[0], x, a[1], y);
    return add(out, n, "%d+%d=%d -> %s", x, y, x+y, b);
  }
  return 0;
}

static int eval_storage(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[5][48]; int na = args(s, a, 5);
  if (starts(s, "image(") && na >= 3) {
    long long bits = parse_int(a[0]) * parse_int(a[1]) * parse_int(a[2]);
    int n = add(out, 0, "Image bits = width * height * colour depth.");
    n = add(out, n, "%s*%s*%s = %lld bits", a[0], a[1], a[2], bits);
    return add(out, n, "= %.6g bytes", bits / 8.0);
  }
  if (starts(s, "sound(") && na >= 3) {
    long long chans = na > 3 ? parse_int(a[3]) : 1;
    double bits = num(a[0]) * num(a[1]) * num(a[2]) * chans;
    int n = add(out, 0, "Sound bits = sample rate * seconds * resolution * channels.");
    n = add(out, n, "%s*%s*%s*%lld = %.10g bits", a[0], a[1], a[2], chans, bits);
    return add(out, n, "= %.10g bytes", bits / 8.0);
  }
  if (starts(s, "bitrate(") && na >= 2) {
    double rate = num(a[0]) / num(a[1]);
    int n = add(out, 0, "Bit rate = bits / seconds.");
    return add(out, n, "%s/%s = %.10g bit/s", a[0], a[1], rate);
  }
  if (starts(s, "chars(") && na >= 2) {
    long long bits = parse_int(a[0]) * parse_int(a[1]);
    int n = add(out, 0, "Text bits = characters * bits per character.");
    return add(out, n, "%s*%s = %lld bits = %.6g bytes", a[0], a[1], bits, bits / 8.0);
  }
  if (starts(s, "compress(") && na >= 2) {
    double oldv = num(a[0]), newv = num(a[1]);
    int n = add(out, 0, "Compression ratio = original / compressed.");
    n = add(out, n, "ratio = %.10g : 1", oldv / newv);
    return add(out, n, "saving = %.6g%%", (oldv - newv) * 100.0 / oldv);
  }
  return 0;
}

static int eval_float(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
  if (starts(s, "fixed(") && na == 1) {
    double v = fixed_decode(a[0]);
    int n = add(out, 0, "Add binary fixed-point place values.");
    return add(out, n, "%s_2 = %.10g_10", a[0], v);
  }
  if ((starts(s, "floatdec(") || starts(s, "fpdec(")) && na >= 2) {
    double m = mantissa_decode(a[0]);
    int e = twos_decode(a[1]);
    int n = add(out, 0, "Mantissa uses two's complement with point after sign bit.");
    n = add(out, n, "mantissa %s = %.10g", a[0], m);
    n = add(out, n, "exponent %s = %d", a[1], e);
    return add(out, n, "value = %.10g * 2^%d = %.10g", m, e, m * pow2(e));
  }
  if (starts(s, "normal(") && na >= 1) {
    int n = add(out, 0, "Normalised AQA mantissa starts 01 if positive, 10 if negative.");
    bool ok = (a[0][0] == '0' && a[0][1] == '1') || (a[0][0] == '1' && a[0][1] == '0');
    return add(out, n, "%s is %snormalised", a[0], ok ? "" : "not ");
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
    while (s[pos] == '&' || s[pos] == '*' || s[pos] == '.') { pos++; v = v && factor(); }
    return v;
  }
  int expr() {
    int v = term();
    while (s[pos] == '+' || s[pos] == '|' || s[pos] == '^') {
      char op = s[pos++]; int r = term();
      v = op == '^' ? (v != r) : (v || r);
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

static int eval_bool(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[2][48]; int na = args(s, a, 2);
  const char *expr = (starts(s, "bool(") || starts(s, "truth(")) && na ? a[0] : 0;
  if (!expr) return 0;
  char vars[8]; int vc; collect_vars(expr, vars, &vc);
  if (vc == 0 || vc > 6) return add(out, 0, "Use up to 6 Boolean variables.");
  int mins[64], mc = 0, rows = 1 << vc;
  for (int m = 0; m < rows; ++m) {
    BParser p = { expr, 0, m, {0}, vc };
    for (int i = 0; i < vc; ++i) p.vars[i] = vars[i];
    if (p.expr()) mins[mc++] = m;
  }
  int n = add(out, 0, "Make truth table, list rows where output is 1.");
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

int cscalc_eval(const char *input, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  for (int i = 0; i < CSCALC_MAX_LINES; ++i) out[i][0] = 0;
  char s[192]; clean(input, s, sizeof(s));
  if (!s[0]) return add(out, 0, "Enter a CS calculation command.");
  int n = eval_base(s, out); if (n) return n;
  n = eval_twos(s, out); if (n) return n;
  n = eval_float(s, out); if (n) return n;
  n = eval_storage(s, out); if (n) return n;
  n = eval_bool(s, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "bin hex den convert twos twosdec fixed");
  n = add(out, n, "floatdec normal image sound bitrate");
  return add(out, n, "compress chars bool truth");
}

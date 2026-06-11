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
    int w = (int)strlen(a[0]), x = twos_decode(a[0]), y = twos_decode(a[1]); char b[65]; to_bin(x-y, w, b);
    int n = add(out, 0, "Subtraction: add the two's complement of the second value.");
    n = add(out, n, "%s=%d, %s=%d", a[0], x, a[1], y);
    return add(out, n, "%d-%d=%d -> %s", x, y, x-y, b);
  }
  return 0;
}

static int eval_binary_arith(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[4][48]; int na = args(s, a, 4);
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
  return 0;
}

static int eval_storage(const char *s, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char a[5][48]; int na = args(s, a, 5);
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
  if (starts3(s, "chars(", "textsize(", "characters(") && na >= 2) {
    long long bits = parse_int(a[0]) * parse_int(a[1]);
    double bytes = bits / 8.0;
    int n = add(out, 0, "Text bits = characters * bits per character.");
    n = add(out, n, "%s*%s = %lld bits = %.6g bytes", a[0], a[1], bits, bytes);
    return add(out, n, "= %.6g MB", bytes / 1000000.0);
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
  if (starts3(s, "records(", "recordsize(", "database(") && na >= 2) {
    double bytes = num(a[0]) * num(a[1]);
    int n = add(out, 0, "File size = records * bytes per record.");
    n = add(out, n, "%s*%s = %.10g bytes", a[0], a[1], bytes);
    return add(out, n, "= %.10g MB", bytes / 1000000.0);
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
  if ((starts(s, "floatdec(") || starts(s, "fpdec(") || starts(s, "floatdecode(")) && na >= 2) {
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
  if ((starts(s, "floatenc(") || starts(s, "fpenc(") || starts(s, "floatencode(")) && na >= 3) {
    double value = num(a[0]); int mb = (int)parse_int(a[1]), eb = (int)parse_int(a[2]);
    int e = 0; double m = value;
    if (m != 0) {
      while (m >= 1.0 || m < -1.0) { m /= 2.0; ++e; }
      while (m > -0.5 && m < 0.5) { m *= 2.0; --e; }
    }
    char mant[65], expb[65]; mant[0] = m < 0 ? '1' : '0';
    double rem = m < 0 ? m + 1.0 : m;
    for (int i = 1; i < mb; ++i) {
      double bitv = pow2(-i);
      if (rem >= bitv) { mant[i] = '1'; rem -= bitv; }
      else mant[i] = '0';
    }
    mant[mb] = 0; to_bin(e, eb, expb);
    int n = add(out, 0, "Normalise so mantissa starts 01 for positive or 10 for negative.");
    n = add(out, n, "%.10g = %.10g * 2^%d", value, m, e);
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
    while (s[pos] == '&' || s[pos] == '*' || s[pos] == '.') { pos++; v = v && factor(); }
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

static int eval_free_text(const char *input, const char *compact, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]) {
  char t[192]; raw_clean(input, t, sizeof(t));
  double v[8]; int nv = scan_nums(t, v, 8);
  char bits[4][48]; int nb = scan_bits(t, bits, 4);
  char cmd[160];
  double width=0, height=0, depth=0;
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
  if (tc && nv >= 2 && nb == 0) {
    sprintf(cmd, "twos(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_twos(cmd, out);
  }
  if (has(t, "add") && nb >= 2) {
    sprintf(cmd, "binadd(%s,%s,%d)", bits[0], bits[1], (int)strlen(bits[0])); return eval_binary_arith(cmd, out);
  }
  if (has(t, "shift") && nb >= 1 && nv >= 1) {
    sprintf(cmd, "shift(%s,%s,%lld)", bits[0], has(t, "right") ? "right" : "left", (long long)v[nv-1]); return eval_binary_arith(cmd, out);
  }
  char fixed[48];
  if (has(t, "fixed") && scan_fixed_bits(t, fixed, sizeof(fixed))) {
    sprintf(cmd, (tc || has(t, "complement")) ? "fixedtc(%s)" : "fixed(%s)", fixed); return eval_float(cmd, out);
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
    sprintf(cmd, "transfer(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "compress") || has(t, "compression")) && nv >= 2) {
    sprintf(cmd, "compress(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "runlength") || has(t, "rle") || (has(t, "run") && has(t, "length"))) && nv >= 3) {
    sprintf(cmd, "rle(%lld,%lld,%lld)", (long long)v[0], (long long)v[1], (long long)v[2]); return eval_storage(cmd, out);
  }
  if ((has(t, "record") || has(t, "database")) && nv >= 2) {
    sprintf(cmd, "records(%.10g,%.10g)", v[0], v[1]); return eval_storage(cmd, out);
  }
  if ((has(t, "character") || has(t, "text")) && nv >= 2) {
    sprintf(cmd, "chars(%lld,%lld)", (long long)v[0], (long long)v[1]); return eval_storage(cmd, out);
  }
  if (nv == 0 && (has(compact, "nand") || has(compact, "nor") || has(compact, "xor") || has(compact, "and") || has(compact, "or") || has(compact, "'"))) {
    const char *e = compact;
    bool moved = true;
    while (moved) {
      moved = false;
      if (starts(e, "simplify")) { e += 8; moved = true; }
      if (starts(e, "boolean")) { e += 7; moved = true; }
      if (starts(e, "logic")) { e += 5; moved = true; }
      if (starts(e, "expression")) { e += 10; moved = true; }
    }
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
  n = eval_float(s, out); if (n) return n;
  n = eval_storage(s, out); if (n) return n;
  n = eval_bool(s, out); if (n) return n;
  n = eval_free_text(input, s, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "bin hex den convert twos twosdec fixed");
  n = add(out, n, "floatdec floatrange normal image sound bitrate transfer");
  return add(out, n, "compress rle records chars bool truth");
}

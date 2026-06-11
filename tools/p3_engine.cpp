#include "p3_engine.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

static bool starts(const char *s, const char *p) { return strncmp(s, p, strlen(p)) == 0; }

static double num(const char *s) {
  double sign = 1, v = 0, scale = 1;
  if (*s == '-') { sign = -1; ++s; }
  for (; *s && *s != '.'; ++s) if (*s >= '0' && *s <= '9') v = v * 10 + (*s - '0');
  if (*s == '.') for (++s; *s; ++s) if (*s >= '0' && *s <= '9') { scale *= 10; v += (*s - '0') / scale; }
  return sign * v;
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

static double sine(double x) {
  while (x > M_PI) x -= 2 * M_PI;
  while (x < -M_PI) x += 2 * M_PI;
  double x2 = x * x;
  return x * (1 - x2 / 6 + x2 * x2 / 120 - x2 * x2 * x2 / 5040);
}

static double cosine(double x) { return sine(M_PI / 2 - x); }

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

static bool kv(char a[][48], int na, const char *name, double *v) {
  int nl = (int)strlen(name);
  for (int i = 0; i < na; ++i) if (strncmp(a[i], name, nl) == 0 && a[i][nl] == '=') {
    *v = num(a[i] + nl + 1); return true;
  }
  return false;
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
  if (!hs && hu && ha && ht) {
    n = add(out, n, "s = ut + 1/2 at^2");
    n = add(out, n, "s = %.6g*%.6g + 1/2*%.6g*%.6g^2", u, t, acc, t);
    return add(out, n, "s = %.6g", u*t + 0.5*acc*t*t);
  }
  if (!hv && hu && ha && hs) {
    n = add(out, n, "v^2 = u^2 + 2as");
    n = add(out, n, "v^2 = %.6g^2 + 2*%.6g*%.6g", u, acc, dist);
    return add(out, n, "v = %.6g", root(u*u + 2*acc*dist));
  }
  if (!ha && hu && hv && ht) {
    n = add(out, n, "v = u + at, so a=(v-u)/t");
    return add(out, n, "a = (%.6g-%.6g)/%.6g = %.6g", v, u, t, (v-u)/t);
  }
  if (!ht && hu && hv && acc) {
    n = add(out, n, "v = u + at, so t=(v-u)/a");
    return add(out, n, "t = (%.6g-%.6g)/%.6g = %.6g", v, u, acc, (v-u)/acc);
  }
  return add(out, n, "Need any suitable 3 known values, e.g. suvat(u=2,a=3,t=4).");
}

static int eval_mech(const char *s, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char a[8][48]; int na = args(s, a, 8);
  if (starts(s, "projectile(") && na >= 2) {
    double u = num(a[0]), th = num(a[1]) * M_PI / 180.0, g = na > 2 ? num(a[2]) : 9.8;
    double ux = u*cosine(th), uy = u*sine(th), T = 2*uy/g, R = ux*T, H = uy*uy/(2*g);
    int n = add(out, 0, "Resolve velocity into horizontal and vertical components.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.6g", u, num(a[1]), ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.6g", u, num(a[1]), uy);
    n = add(out, n, "same height: t = 2u_y/g = %.6g", T);
    n = add(out, n, "range = u_x t = %.6g", R);
    return add(out, n, "max height = u_y^2/(2g) = %.6g", H);
  }
  if (starts(s, "force(") && na >= 2) {
    double m=num(a[0]), acc=num(a[1]);
    int n = add(out, 0, "Use Newton's second law in the direction of motion.");
    return add(out, n, "F = ma = %.6g*%.6g = %.6g N", m, acc, m*acc);
  }
  if (starts(s, "weight(") && na >= 1) {
    double m=num(a[0]), g=na>1?num(a[1]):9.8;
    int n = add(out, 0, "Weight acts vertically downwards.");
    return add(out, n, "W = mg = %.6g*%.6g = %.6g N", m, g, m*g);
  }
  if (starts(s, "friction(") && na >= 2) {
    double mu=num(a[0]), r=num(a[1]);
    int n = add(out, 0, "Limiting friction is F = mu R.");
    return add(out, n, "F = %.6g*%.6g = %.6g N", mu, r, mu*r);
  }
  if (starts(s, "moment(") && na >= 2) {
    double f=num(a[0]), d=num(a[1]);
    int n = add(out, 0, "Moment = force * perpendicular distance.");
    return add(out, n, "M = %.6g*%.6g = %.6g Nm", f, d, f*d);
  }
  return 0;
}

static int eval_stats(const char *s, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char a[8][48]; int na = args(s, a, 8);
  if (starts(s, "normal(") && na >= 3) {
    double x=num(a[0]), mu=num(a[1]), sig=num(a[2]);
    int n = add(out, 0, "Standardise using Z=(X-mu)/sigma.");
    return add(out, n, "z = (%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
  }
  if (starts(s, "binom(") && na >= 3) {
    int N=(int)num(a[0]), r=(int)num(a[2]); double p=num(a[1]), ans=binomp(N,p,r);
    int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
    n = add(out, n, "P(X=%d) = nCr p^r(1-p)^(n-r)", r);
    return add(out, n, "= %dC%d*%.6g^%d*%.6g^%d = %.10g", N, r, p, r, 1-p, N-r, ans);
  }
  if (starts(s, "binomcdf(") && na >= 3) {
    int N=(int)num(a[0]), r=(int)num(a[2]); double p=num(a[1]), ans=0;
    for (int k=0;k<=r;++k) ans += binomp(N,p,k);
    int n = add(out, 0, "Let X ~ B(%d, %.6g).", N, p);
    n = add(out, n, "P(X<=%d) = sum P(X=r), r=0..%d.", r, r);
    return add(out, n, "= %.10g", ans);
  }
  if (starts(s, "hypbinom(") && na >= 5) {
    int N=(int)num(a[0]), x=(int)num(a[2]); double p=num(a[1]), alpha=num(a[3]), tail=num(a[4]), prob=0;
    if (tail < 0) for (int k=0;k<=x;++k) prob += binomp(N,p,k);
    else for (int k=x;k<=N;++k) prob += binomp(N,p,k);
    int n = add(out, 0, "H0: p = %.6g. H1 uses the stated tail.", p);
    n = add(out, n, "tail probability = %.10g", prob);
    n = add(out, n, "Compare with alpha = %.6g.", alpha);
    return add(out, n, prob <= alpha ? "Reject H0 in context." : "Do not reject H0 in context.");
  }
  return 0;
}

int p3_eval(const char *input, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  for (int i=0;i<P3_MAX_LINES;++i) out[i][0]=0;
  char s[192]; clean(input, s, sizeof(s));
  if (!s[0]) return add(out, 0, "Enter a Paper 3 command.");
  int n = eval_suvat(s, out); if (n) return n;
  n = eval_mech(s, out); if (n) return n;
  n = eval_stats(s, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "suvat projectile force weight friction moment");
  return add(out, n, "normal binom binomcdf hypbinom");
}

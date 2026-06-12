#include "p3_engine.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
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
  if (starts3(s, "projectileh(", "projectileheight(", "projheight(") && na >= 3) {
    double u = num(a[0]), deg = num(a[1]), h = num(a[2]), g = na > 3 ? num(a[3]) : 9.8;
    double th = deg * M_PI / 180.0, ux = u*cosine(th), uy = u*sine(th);
    double t = (uy + root(uy*uy + 2*g*h)) / g, r = ux*t;
    int n = add(out, 0, "Resolve, then use vertical motion to find time.");
    n = add(out, n, "u_x = %.6g cos %.6g = %.6g", u, deg, ux);
    n = add(out, n, "u_y = %.6g sin %.6g = %.6g", u, deg, uy);
    n = add(out, n, "-h = u_y t - 1/2 g t^2, so h=%.6g", h);
    n = add(out, n, "t = (u_y+sqrt(u_y^2+2gh))/g = %.10g", t);
    return add(out, n, "range = u_x t = %.10g", r);
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
    n = add(out, n, "same height: t = 2u_y/g = %.6g", T);
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
    int n = add(out, 0, "Treat connected particles as one system for acceleration.");
    n = add(out, n, "a = F/(m1+m2) = %.6g/(%.6g+%.6g)", f, m1, m2);
    n = add(out, n, "a = %.6g m/s^2", ares);
    return add(out, n, "tension on m1: T = m1*a = %.6g N", T);
  }
  if (starts2(s, "pulley(", "pulleys(") && na >= 2) {
    double m1=num(a[0]), m2=num(a[1]), g=na>2?num(a[2]):9.8, ares=(m2-m1)*g/(m1+m2), T=m1*(g+ares);
    int n = add(out, 0, "For a light inextensible string, both masses have the same acceleration.");
    n = add(out, n, "a = (m2-m1)g/(m1+m2) = %.6g", ares);
    return add(out, n, "T = m1(g+a) = %.6g N", T);
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
    if (sx*sx + sy*sy < 1e-12) return add(out, n, "result: equilibrium");
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
    if (sx*sx + sy*sy < 1e-12) return add(out, n, "result: equilibrium");
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
    else for (int k=x;k<=N;++k) prob += binomp(N,p,k);
    int n = add(out, 0, "H0: p = %.6g. H1 uses the stated tail.", p);
    n = add(out, n, "tail probability = %.10g", prob);
    n = add(out, n, "Compare with alpha = %.6g.", alpha);
    return add(out, n, prob <= alpha ? "Reject H0 in context." : "Do not reject H0 in context.");
  }
  if (starts3(s, "hyppoisson(", "poissontest(", "poissonhyp(") && na >= 4) {
    double lam=num(a[0]), alpha=num(a[2]), tail=num(a[3]); int x=(int)num(a[1]);
    double prob = poissontailprob(lam, x, tail < 0 ? -1 : 1);
    int n = add(out, 0, "H0: lambda = %.6g. H1 uses the stated tail.", lam);
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
    n = add(out, n, tail >= 0 ? "Required probability is P(X>=%.6g)." : "Required probability is P(X<=%.6g).", x);
    return add(out, n, tail >= 0 ? "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.6g, mu=%.6g)" : "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.6g, mu=%.6g)", x, sig, mu);
  }
  if (starts3(s, "normaltailvar(", "normaluppervar(", "normaltpvar(") && na >= 4) {
    double x=num(a[0]), mu=num(a[1]), var=num(a[2]), sig=root(var), tail=num(a[3]);
    int n = add(out, 0, "Convert variance to standard deviation first.");
    n = add(out, n, "sigma = sqrt(%.6g) = %.6g", var, sig);
    n = add(out, n, "z=(%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
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
  if (starts3(s, "invnormal(", "inversenormal(", "normalinv(") && na >= 3) {
    double area=num(a[0]), mu=num(a[1]), sig=num(a[2]);
    int n = add(out, 0, "For X~N(mu,sigma^2), use inverse normal for a critical value.");
    n = add(out, n, "Area to the left = %.6g.", area);
    n = add(out, n, "Use fx-CG50 InvNorm(area, sigma, mu).");
    return add(out, n, "InvNorm(%.6g, %.6g, %.6g)", area, sig, mu);
  }
  if (starts3(s, "invnormalvar(", "inversenormalvar(", "normalinvvar(") && na >= 3) {
    double area=num(a[0]), mu=num(a[1]), var=num(a[2]), sig=root(var);
    int n = add(out, 0, "Convert variance to standard deviation first.");
    n = add(out, n, "sigma = sqrt(%.6g) = %.6g", var, sig);
    n = add(out, n, "Area to the left = %.6g.", area);
    return add(out, n, "InvNorm(%.6g, %.6g, %.6g)", area, sig, mu);
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
    return add(out, n, "Var(X)=E(X^2)-E(X)^2 = %.10g", ex2 - ex*ex);
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
    int n = add(out, 0, "Use linear interpolation in the required class.");
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
    return add(out, n, "sd X = |b|*sd Y = %.6g*%.6g = %.6g", ab, sy, ab*sy);
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
  double v[8]; int nv = scan_nums(t, v, 8);
  char cmd[160];
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
  if (has(t, "suvat") || has(t, "velocity") || has(t, "acceleration") || has(t, "distance") || has(t, "displacement") || has(t, "time")) {
    double u=0, vv=0, acc=0, dist=0, time=0; bool hu=label_num(input,"u",&u), hv=label_num(input,"v",&vv), ha=label_num(input,"a",&acc), hs=label_num(input,"s",&dist), ht=label_num(input,"t",&time);
    if (hu || hv || ha || hs || ht) {
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
  if ((has(t, "projectile") || has(t, "projectiles")) && (has(t, "angle") || has(t, "angles")) &&
      (has(t, "target") || has(t, "point") || has(t, "through") || has(t, "pass")) && nv >= 3) {
    sprintf(cmd, nv >= 4 ? "projectileangle(%.10g,%.10g,%.10g,%.10g)" : "projectileangle(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv >= 4 ? v[3] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "projectile") || has(t, "projectiles")) && (has(t, "distance") || has(t, "metresaway") || has(t, "away")) && nv >= 3) {
    sprintf(cmd, nv >= 4 ? "projectileat(%.10g,%.10g,%.10g,%.10g)" : "projectileat(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv >= 4 ? v[3] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "projectile") || has(t, "projectiles")) && (has(t, "height") || has(t, "above")) && nv >= 3) {
    sprintf(cmd, "projectileh(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_mech(cmd, out);
  }
  if ((has(t, "projectile") || has(t, "projectiles")) && nv >= 2) {
    sprintf(cmd, "projectile(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "equilibrium") || has(t, "balance")) && (has(t, "component") || has(t, "components")) && nv >= 4) {
    int p = sprintf(cmd, "equilibrium(%.10g", v[0]);
    for (int i=1;i<nv && p < (int)sizeof(cmd)-24;++i) p += sprintf(cmd+p, ",%.10g", v[i]);
    sprintf(cmd+p, ")");
    return eval_mech(cmd, out);
  }
  if ((has(t, "equilibrium") || has(t, "balance")) && (has(t, "angle") || has(t, "bearing") || has(t, "degree")) && nv >= 4) {
    int p = sprintf(cmd, "equilpolar(%.10g", v[0]);
    for (int i=1;i<nv && p < (int)sizeof(cmd)-24;++i) p += sprintf(cmd+p, ",%.10g", v[i]);
    sprintf(cmd+p, ")");
    return eval_mech(cmd, out);
  }
  if ((has(t, "beam") || has(t, "support") || has(t, "reaction")) && (has(t, "load") || has(t, "weight")) && nv >= 3) {
    double L=0,W=0,x=0,bw=0;
    bool hL=label_num(input,"length",&L), hW=label_num(input,"load",&W), hx=label_num(input,"distance",&x);
    bool hb=label_num(input,"beamweight",&bw) || label_num(input,"bw",&bw);
    if (hL && hW && hx) sprintf(cmd, "beam(%.10g,%.10g,%.10g,%.10g)", L, W, x, hb ? bw : 0);
    else sprintf(cmd, nv > 3 ? "beam(%.10g,%.10g,%.10g,%.10g)" : "beam(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv > 3 ? v[3] : 0);
    return eval_mech(cmd, out);
  }
  if (has(t, "ladder") && (has(t, "wall") || has(t, "floor") || has(t, "rough") || has(t, "smooth")) && nv >= 3) {
    double L=0,W=0,ang=0,P=0,d=0;
    bool hL=label_num(input,"length",&L), hW=label_num(input,"weight",&W), ha=label_num(input,"angle",&ang);
    bool hP=label_num(input,"load",&P) || label_num(input,"person",&P);
    bool hd=label_num(input,"distance",&d);
    if (hL && hW && ha) sprintf(cmd, "ladder(%.10g,%.10g,%.10g,%.10g,%.10g)", L, W, ang, hP ? P : 0, hd ? d : L/2);
    else sprintf(cmd, nv > 4 ? "ladder(%.10g,%.10g,%.10g,%.10g,%.10g)" : "ladder(%.10g,%.10g,%.10g)", v[0], v[1], v[2], nv > 3 ? v[3] : 0, nv > 4 ? v[4] : v[0]/2);
    return eval_mech(cmd, out);
  }
  if ((has(t, "force") || has(t, "newton")) && (has(t, "mass") || has(t, "accel")) && !has(t, "connected") && !has(t, "pulley") && nv >= 2) {
    sprintf(cmd, "force(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "weight") && nv >= 1) {
    sprintf(cmd, "weight(%.10g)", v[0]); return eval_mech(cmd, out);
  }
  if ((has(t, "friction") || has(t, "coefficientoffriction")) && nv >= 2) {
    sprintf(cmd, "friction(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "moment") && !has(t, "momentum") && nv >= 2) {
    sprintf(cmd, "moment(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) && (has(t, "acceleration") || has(t, "accelerate") || has(t, "rough")) && nv >= 3) {
    sprintf(cmd, "inclineacc(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_mech(cmd, out);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) && nv >= 2) {
    sprintf(cmd, "incline(%.10g,%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "pulley") || has(t, "connectedparticles") || (has(t, "connected") && has(t, "particle"))) && nv >= 2) {
    sprintf(cmd, has(t, "pulley") ? "pulley(%.10g,%.10g)" : "connected(%.10g,%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "impulse") || has(t, "momentumchange")) && nv >= 3) {
    sprintf(cmd, "impulse(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_mech(cmd, out);
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
  if (has(t, "power") && nv >= 2) {
    sprintf(cmd, "power(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "kinetic") || has(t, "energy") || has(t, "workenergy")) && nv >= 2) {
    sprintf(cmd, nv > 2 ? "energy(%.10g,%.10g,%.10g)" : "energy(%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "workdone") || has(t, "work")) && nv >= 2) {
    sprintf(cmd, "work(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "restitution") || has(t, "collision") || has(t, "impact")) && nv >= 5) {
    double m1=0,u1=0,m2=0,u2=0,e=0;
    if (label_num(input,"m1",&m1) && label_num(input,"u1",&u1) && label_num(input,"m2",&m2) && label_num(input,"u2",&u2) && label_num(input,"e",&e)) {
      sprintf(cmd, "impactsolve(%.10g,%.10g,%.10g,%.10g,%.10g)", m1, u1, m2, u2, e); return eval_mech(cmd, out);
    }
    if (has(t, "coefficient") || has(t, "given") || has(t, "find")) {
      sprintf(cmd, "impactsolve(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4]); return eval_mech(cmd, out);
    }
  }
  if ((has(t, "restitution") || has(t, "collision") || has(t, "impact")) && nv >= 4) {
    sprintf(cmd, "restitution(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_mech(cmd, out);
  }
  if ((has(t, "resultant") || has(t, "vector")) && nv >= 2) {
    sprintf(cmd, "vector(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "resolve") || has(t, "components")) && has(t, "force") && nv >= 2) {
    sprintf(cmd, "resolve(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "varacc") || has(t, "variableacceleration") || (has(t, "acceleration") && has(t, "integrate"))) && nv >= 4) {
    sprintf(cmd, "varacc(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_mech(cmd, out);
  }
  if ((has(t, "critical") || has(t, "criticalregion")) && has(t, "binom") && nv >= 3) {
    double tail = has(t, "upper") ? 1 : -1;
    sprintf(cmd, "critbinom(%d,%.10g,%.10g,%.0f)", (int)v[0], v[1], v[2], tail); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && has(t, "normal") && (has(t, "approx") || has(t, "approximation")) && nv >= 4) {
    sprintf(cmd, "binomnorm(%d,%.10g,%.10g,%.10g)", (int)v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && has(t, "poisson") && (has(t, "approx") || has(t, "approximation")) && nv >= 3) {
    int tail = 0;
    if (has(c, "morethan")) tail = 2;
    else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
    else if (has(c, "lessthan")) tail = -2;
    else if (has(c, "atmost") || has(t, "cdf")) tail = -1;
    sprintf(cmd, "poissonapprox(%d,%.10g,%d,%d)", (int)v[0], v[1], (int)v[2], tail); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "conditional") || has(t, "given")) && nv >= 4) {
    double tail = (has(c, "lessthan") || has(c, "atmost") || has(c, "<")) ? -1 : 1;
    sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], v[3], tail); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "find") || has(t, "given") || has(t, "parameters")) &&
      (has(t, "meansd") || has(t, "meanandsd") ||
      (has(t, "mean") && (has(t, "standarddeviation") || has(t, "sd"))) ||
      has(t, "parameters")) && nv >= 4) {
    sprintf(cmd, "normalparams(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if ((has(t, "normaldistribution") || has(t, "normalcdf") || (has(t, "normal") && has(t, "between"))) && has(t, "variance") && nv >= 4) {
    sprintf(cmd, "normalprobvar(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if ((has(t, "normaldistribution") || has(t, "normalcdf") || (has(t, "normal") && has(t, "between"))) && nv >= 4) {
    sprintf(cmd, "normalprob(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && has(t, "variance") && (has(c, "morethan") || has(c, "greaterthan") || has(c, "atleast") || has(c, "lessthan") || has(c, "atmost")) && nv >= 3) {
    double tail = (has(c, "morethan") || has(c, "greaterthan") || has(c, "atleast")) ? 1 : -1;
    sprintf(cmd, "normaltailvar(%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], tail); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(c, "morethan") || has(c, "greaterthan") || has(c, "atleast") || has(c, "lessthan") || has(c, "atmost")) && nv >= 3) {
    double tail = (has(c, "morethan") || has(c, "greaterthan") || has(c, "atleast")) ? 1 : -1;
    sprintf(cmd, "normaltail(%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], tail); return eval_stats(cmd, out);
  }
  if ((has(c, "invnormal") || has(c, "inversenormal") || (has(c, "normal") && (has(c, "critical") || has(c, "percentile")))) && has(t, "variance") && nv >= 3) {
    sprintf(cmd, "invnormalvar(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(c, "invnormal") || has(c, "inversenormal") || (has(c, "normal") && (has(c, "critical") || has(c, "percentile")))) && nv >= 3) {
    sprintf(cmd, "invnormal(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "outlier") || has(t, "iqr") || has(t, "fence")) && nv >= 2) {
    double q1=0, q3=0; bool hq1=label_num(input,"q1",&q1), hq3=label_num(input,"q3",&q3);
    int p = sprintf(cmd, "outliers(%.10g,%.10g", hq1 ? q1 : v[0], hq3 ? q3 : v[1]);
    int start = (hq1 && hq3) ? 0 : 2;
    for (int i = start; i < nv && p < (int)sizeof(cmd)-24; ++i) {
      if ((hq1 && v[i] == q1) || (hq3 && v[i] == q3)) continue;
      p += sprintf(cmd+p, ",%.10g", v[i]);
    }
    sprintf(cmd+p, ")");
    return eval_stats(cmd, out);
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
    double tail = has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 : -1;
    sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", (int)v[0], v[1], (int)v[2], v[3], tail); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && (has(t, "mean") || has(t, "variance") || has(t, "standarddeviation") || has(t, "sd")) && nv >= 2) {
    sprintf(cmd, "binomstats(%d,%.10g)", (int)v[0], v[1]); return eval_stats(cmd, out);
  }
  if ((has(t, "bayes") || has(t, "reverseconditional")) && nv >= 3) {
    sprintf(cmd, "bayes(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "independent") || has(t, "independence")) && nv >= 3) {
    sprintf(cmd, "independent(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "conditional") || has(t, "given")) && nv >= 4) {
    double tail = (has(c, "lessthan") || has(c, "atmost") || has(c, "<")) ? -1 : 1;
    sprintf(cmd, "normalcond(%.10g,%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], v[3], tail); return eval_stats(cmd, out);
  }
  if ((has(t, "conditional") || has(t, "given")) && nv >= 2) {
    sprintf(cmd, "cond(%.10g,%.10g)", v[0], v[1]); return eval_stats(cmd, out);
  }
  if ((has(t, "union") || has(t, "either") || has(t, "orprobability")) && nv >= 3) {
    sprintf(cmd, "probor(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if (has(t, "binom") && nv >= 3) {
    int tail = 0;
    if (has(c, "morethan")) tail = 2;
    else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
    else if (has(c, "lessthan")) tail = -2;
    else if (has(c, "atmost") || has(t, "cdf")) tail = -1;
    if (tail) sprintf(cmd, "binomtail(%d,%.10g,%d,%d)", (int)v[0], v[1], (int)v[2], tail);
    else sprintf(cmd, "binom(%d,%.10g,%d)", (int)v[0], v[1], (int)v[2]);
    return eval_stats(cmd, out);
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
      double tail = has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 : -1;
      sprintf(cmd, "hyppoisson(%.10g,%d,%.10g,%.0f)", v[0], (int)v[1], v[2], tail); return eval_stats(cmd, out);
    }
    int tail = 0;
    if (has(c, "morethan")) tail = 2;
    else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
    else if (has(c, "lessthan")) tail = -2;
    else if (has(c, "atmost") || has(t, "cdf")) tail = -1;
    if (tail) { sprintf(cmd, "poissontail(%.10g,%d,%d)", v[0], (int)v[1], tail); return eval_stats(cmd, out); }
    sprintf(cmd, "poisson(%.10g,%d)", v[0], (int)v[1]); return eval_stats(cmd, out);
  }
  if (has(t, "poisson") && (has(t, "mean") || has(t, "variance") || has(t, "standarddeviation") || has(t, "sd")) && nv >= 1) {
    sprintf(cmd, "poissonstats(%.10g)", v[0]); return eval_stats(cmd, out);
  }
  if ((has(t, "pmcc") || has(t, "correlation") || has(t, "spearman")) &&
      (has(t, "critical") || has(t, "cv")) && (has(t, "test") || has(t, "hypothesis") || has(t, "significant")) && nv >= 2) {
    double r=0, crit=0; bool hr=label_num(input,"r",&r), hc=label_num(input,"critical",&crit) || label_num(input,"cv",&crit);
    int tail = has(t, "positive") || has(t, "upper") ? 1 : (has(t, "negative") || has(t, "lower") ? -1 : 0);
    sprintf(cmd, "corrtest(%.10g,%.10g,%d)", hr ? r : v[0], hc ? crit : v[1], tail); return eval_stats(cmd, out);
  }
  if (has(t, "pmcc") || has(t, "correlation")) {
    double n0=0, sx=0, sy=0, sxy=0, sx2=0, sy2=0, sxx=0, syy=0;
    if (label_num(input,"sxx",&sxx) && label_num(input,"syy",&syy) && label_num(input,"sxy",&sxy)) {
      sprintf(cmd, "pmccs(%.10g,%.10g,%.10g)", sxx, syy, sxy); return eval_stats(cmd, out);
    }
    if (label_num(input,"n",&n0) && label_num(input,"sx",&sx) && label_num(input,"sy",&sy) && label_num(input,"sxy",&sxy) && (label_num(input,"sx2",&sx2) || label_num(input,"sx^2",&sx2)) && (label_num(input,"sy2",&sy2) || label_num(input,"sy^2",&sy2))) {
      sprintf(cmd, "pmcc(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", n0, sx, sy, sxy, sx2, sy2); return eval_stats(cmd, out);
    }
  }
  if ((has(t, "pmcc") || has(t, "correlation")) && nv >= 6) {
    sprintf(cmd, "pmcc(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4], v[5]); return eval_stats(cmd, out);
  }
  if (has(t, "spearman") && nv >= 2) {
    double n0=0, sd2=0;
    if (label_num(input,"n",&n0) && (label_num(input,"sumd2",&sd2) || label_num(input,"d2",&sd2))) {
      sprintf(cmd, "spearman(%.10g,%.10g)", n0, sd2); return eval_stats(cmd, out);
    }
    sprintf(cmd, "spearman(%.10g,%.10g)", v[0], v[1]); return eval_stats(cmd, out);
  }
  if (has(t, "regression") || has(t, "leastsquares") || has(t, "lineofbestfit")) {
    double n0=0, sx=0, sy=0, sxx=0, sxy=0, xb=0, yb=0, x=0;
    bool hx = label_num(input,"x",&x);
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
  if ((has(t, "coded") || has(t, "coding")) && has(t, "mean") && (has(t, "sd") || has(t, "standarddeviation")) && nv >= 4) {
    if (has(c, "y=(x-") || has(c, "y=(x+")) sprintf(cmd, "uncode(%.10g,%.10g,%.10g,%.10g)", v[2], v[3], -v[0], v[1]);
    else sprintf(cmd, "code(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]);
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
  if ((has(t, "mean") || has(t, "variance") || has(t, "standarddeviation")) && nv >= 3) {
    sprintf(cmd, "meanvar(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "discrete") || has(t, "randomvariable") || has(t, "expectation")) && has(t, "prob") && nv >= 4) {
    int p = sprintf(cmd, "discrete(");
    if ((has(t, "values") || has(t, "xvalues")) && (has(t, "probabilities") || has(t, "probs")) && nv % 2 == 0) {
      int m = nv / 2;
      for (int i = 0; i < m; ++i) p += sprintf(cmd+p, "%s%.10g,%.10g", i ? "," : "", v[i], v[i+m]);
    } else {
      int m = nv / 2;
      for (int i = 0; i < m; ++i) p += sprintf(cmd+p, "%s%.10g,%.10g", i ? "," : "", v[2*i], v[2*i+1]);
    }
    sprintf(cmd+p, ")");
    return eval_stats(cmd, out);
  }
  if ((has(t, "stratified") || has(t, "stratum")) && nv >= 3) {
    sprintf(cmd, "stratified(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "grouped") || has(t, "interpolate") || has(t, "interpolation")) && has(t, "median") && nv >= 5) {
    sprintf(cmd, "groupmedian(%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4]); return eval_stats(cmd, out);
  }
  if ((has(t, "grouped") || has(t, "interpolate") || has(t, "interpolation")) && (has(t, "quartile") || has(t, "quantile")) && nv >= 6) {
    sprintf(cmd, "groupquantile(%.10g,%.10g,%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3], v[4], v[5]); return eval_stats(cmd, out);
  }
  if ((has(t, "histogram") || has(t, "frequencydensity")) && (has(t, "density") || has(t, "frequencydensity")) && has(t, "width") && nv >= 2) {
    if (has(t, "findfrequency") || has(t, "frequencyfromdensity")) sprintf(cmd, "histfreq(%.10g,%.10g)", v[0], v[1]);
    else sprintf(cmd, "histdensity(%.10g,%.10g)", v[0], v[1]);
    return eval_stats(cmd, out);
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
  n = eval_free_text(input, out); if (n) return n;
  n = add(out, 0, "Supported:");
  n = add(out, n, "suvat projectile projectileh projectileat projectileangle force weight friction moment incline inclineacc");
  n = add(out, n, "beam ladder connected pulley impulse momentum work power energy workenergyforce restitution vector resolve vectorkin varacc");
  return add(out, n, "normal normalvar normalprob normaltail normalcond invnormal normalparams binom binomstats binomtail critbinom hypbinom cond probor bayes independent poisson poissonstats poissontail poissonnorm critpoisson hyppoisson regress pmcc spearman meanvar discrete stratified groupmedian histdensity code");
}

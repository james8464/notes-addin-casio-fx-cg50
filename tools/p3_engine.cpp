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

static double poissonp(double lam, int r) {
  double fact=1; for(int i=2;i<=r;++i) fact*=i;
  double e = 1.0, term = 1.0; for(int i=1;i<18;++i){ term *= -lam/i; e += term; }
  return e*pwr(lam,r)/fact;
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
  if (starts3(s, "incline(", "slope(", "plane(") && na >= 2) {
    double m=num(a[0]), th=num(a[1])*M_PI/180.0, g=na>3?num(a[3]):9.8, mu=na>2?num(a[2]):0;
    double parallel=m*g*sine(th), reaction=m*g*cosine(th), fr=mu*reaction;
    int n = add(out, 0, "Resolve weight parallel and perpendicular to the plane.");
    n = add(out, n, "parallel = mg sin theta = %.6g N", parallel);
    n = add(out, n, "R = mg cos theta = %.6g N", reaction);
    if (na > 2) n = add(out, n, "friction = mu R = %.6g N", fr);
    return add(out, n, "net down plane = %.6g N", parallel - fr);
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
  if (starts3(s, "vector(", "resultant(", "components(") && na >= 2) {
    double x=num(a[0]), y=num(a[1]), mag=root(x*x+y*y);
    int n = add(out, 0, "Resolve into perpendicular components.");
    n = add(out, n, "|R| = sqrt(x^2+y^2)");
    n = add(out, n, "|R| = sqrt(%.6g^2+%.6g^2) = %.6g", x, y, mag);
    return add(out, n, "direction: use tan(theta)=y/x on calculator.");
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
  if (starts3(s, "normal(", "normalz(", "zscore(") && na >= 3) {
    double x=num(a[0]), mu=num(a[1]), sig=num(a[2]);
    int n = add(out, 0, "Standardise using Z=(X-mu)/sigma.");
    return add(out, n, "z = (%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
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
  if (starts3(s, "hypbinom(", "binomtest(", "hypothesistest(") && na >= 5) {
    int N=(int)num(a[0]), x=(int)num(a[2]); double p=num(a[1]), alpha=num(a[3]), tail=num(a[4]), prob=0;
    if (tail < 0) for (int k=0;k<=x;++k) prob += binomp(N,p,k);
    else for (int k=x;k<=N;++k) prob += binomp(N,p,k);
    int n = add(out, 0, "H0: p = %.6g. H1 uses the stated tail.", p);
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
  if (starts3(s, "normaltail(", "normalupper(", "normaltp(") && na >= 4) {
    double x=num(a[0]), mu=num(a[1]), sig=num(a[2]), tail=num(a[3]);
    int n = add(out, 0, "For X~N(mu,sigma^2), choose the correct tail.");
    n = add(out, n, "z=(%.6g-%.6g)/%.6g = %.6g", x, mu, sig, (x-mu)/sig);
    n = add(out, n, tail >= 0 ? "Required probability is P(X>=%.6g)." : "Required probability is P(X<=%.6g).", x);
    return add(out, n, tail >= 0 ? "Use NormalCD(lower=%.6g, upper=1E99, sigma=%.6g, mu=%.6g)" : "Use NormalCD(lower=-1E99, upper=%.6g, sigma=%.6g, mu=%.6g)", x, sig, mu);
  }
  if (starts3(s, "invnormal(", "inversenormal(", "normalinv(") && na >= 3) {
    double area=num(a[0]), mu=num(a[1]), sig=num(a[2]);
    int n = add(out, 0, "For X~N(mu,sigma^2), use inverse normal for a critical value.");
    n = add(out, n, "Area to the left = %.6g.", area);
    n = add(out, n, "Use fx-CG50 InvNorm(area, sigma, mu).");
    return add(out, n, "InvNorm(%.6g, %.6g, %.6g)", area, sig, mu);
  }
  if (starts3(s, "hypnormal(", "normaltest(", "hypmean(") && na >= 6) {
    double xb=num(a[0]), mu=num(a[1]), sig=num(a[2]), nn=num(a[3]), alpha=num(a[4]), tail=num(a[5]);
    double se = sig/root(nn), z = (xb-mu)/se;
    int n = add(out, 0, "H0: mu = %.6g. H1 uses the stated tail.", mu);
    n = add(out, n, "standard error = sigma/sqrt(n) = %.6g/sqrt(%.6g) = %.6g", sig, nn, se);
    n = add(out, n, "z = (xbar-mu)/SE = (%.6g-%.6g)/%.6g = %.6g", xb, mu, se, z);
    n = add(out, n, tail >= 0 ? "Find P(Z>=%.6g) and compare with alpha %.6g." : "Find P(Z<=%.6g) and compare with alpha %.6g.", z, alpha);
    return add(out, n, "Reject H0 if tail probability <= %.6g; conclude in context.", alpha);
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
  if (starts2(s, "poisson(", "poissonpdf(") && na >= 2) {
    double lam=num(a[0]); int r=(int)num(a[1]);
    int n = add(out, 0, "For X~Po(lambda), P(X=r)=e^-lambda lambda^r/r!.");
    return add(out, n, "P(X=%d)=e^-%.6g*%.6g^%d/%d! = %.10g", r, lam, lam, r, r, poissonp(lam,r));
  }
  if (starts3(s, "poissoncdf(", "poissonle(", "poissontail(") && na >= 2) {
    double lam=num(a[0]); int r=(int)num(a[1]), tail=na>2?(int)num(a[2]):-1; double ans=0;
    int lo = tail == 2 ? r + 1 : tail == 1 ? r : 0;
    int hi = tail == -2 ? r - 1 : tail == -1 ? r : r + 40;
    if (tail < 0) for (int k=lo;k<=hi;++k) ans += poissonp(lam,k);
    else { double below=0; for (int k=0;k<lo;++k) below += poissonp(lam,k); ans = 1 - below; }
    int n = add(out, 0, "For X~Po(%.6g), use cumulative probabilities.", lam);
    n = add(out, n, tail == 2 ? "P(X>%d)=1-P(X<=%d)" : tail == 1 ? "P(X>=%d)=1-P(X<=%d)" : tail == -2 ? "P(X<%d)" : "P(X<=%d)", r, lo-1);
    return add(out, n, "= %.10g", ans);
  }
  if (starts3(s, "regress(", "regression(", "predict(") && na >= 3) {
    double a0=num(a[0]), b=num(a[1]), x=num(a[2]);
    int n = add(out, 0, "Use the regression line y = a + bx.");
    n = add(out, n, "y = %.6g + %.6g x", a0, b);
    return add(out, n, "when x=%.6g, y=%.6g", x, a0+b*x);
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
  if (starts3(s, "meanvar(", "variance(", "summary(") && na >= 3) {
    double n0=num(a[0]), sx=num(a[1]), sx2=num(a[2]), mean=sx/n0, var=sx2/n0-mean*mean;
    int n = add(out, 0, "Use summary statistics formulae.");
    n = add(out, n, "mean = Sx/n = %.6g/%.6g = %.6g", sx, n0, mean);
    n = add(out, n, "variance = Sx2/n - mean^2");
    return add(out, n, "variance = %.10g, sd = %.10g", var, root(var));
  }
  return 0;
}

static int eval_free_text(const char *input, char out[P3_MAX_LINES][P3_LINE_LEN]) {
  char t[192]; raw_clean(input, t, sizeof(t));
  char c[192]; clean(input, c, sizeof(c));
  double v[8]; int nv = scan_nums(t, v, 8);
  char cmd[160];
  if (has(t, "suvat")) {
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
  if ((has(t, "projectile") || has(t, "projectiles")) && nv >= 2) {
    sprintf(cmd, "projectile(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "force") || has(t, "newton")) && (has(t, "mass") || has(t, "accel")) && nv >= 2) {
    sprintf(cmd, "force(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "weight") && nv >= 1) {
    sprintf(cmd, "weight(%.10g)", v[0]); return eval_mech(cmd, out);
  }
  if ((has(t, "friction") || has(t, "coefficientoffriction")) && nv >= 2) {
    sprintf(cmd, "friction(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if (has(t, "moment") && nv >= 2) {
    sprintf(cmd, "moment(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "incline") || has(t, "slope") || has(t, "plane")) && nv >= 2) {
    sprintf(cmd, "incline(%.10g,%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
  }
  if ((has(t, "pulley") || has(t, "connectedparticles")) && nv >= 2) {
    sprintf(cmd, has(t, "pulley") ? "pulley(%.10g,%.10g)" : "connected(%.10g,%.10g,%.10g)", v[0], v[1], nv > 2 ? v[2] : 0); return eval_mech(cmd, out);
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
  if ((has(t, "restitution") || has(t, "collision") || has(t, "impact")) && nv >= 4) {
    sprintf(cmd, "restitution(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_mech(cmd, out);
  }
  if ((has(t, "resultant") || has(t, "vector")) && nv >= 2) {
    sprintf(cmd, "vector(%.10g,%.10g)", v[0], v[1]); return eval_mech(cmd, out);
  }
  if ((has(t, "normaldistribution") || has(t, "normalcdf") || (has(t, "normal") && has(t, "between"))) && nv >= 4) {
    sprintf(cmd, "normalprob(%.10g,%.10g,%.10g,%.10g)", v[0], v[1], v[2], v[3]); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(c, "morethan") || has(c, "greaterthan") || has(c, "atleast") || has(c, "lessthan") || has(c, "atmost")) && nv >= 3) {
    double tail = (has(c, "morethan") || has(c, "greaterthan") || has(c, "atleast")) ? 1 : -1;
    sprintf(cmd, "normaltail(%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], tail); return eval_stats(cmd, out);
  }
  if ((has(c, "invnormal") || has(c, "inversenormal") || (has(c, "normal") && (has(c, "critical") || has(c, "percentile")))) && nv >= 3) {
    sprintf(cmd, "invnormal(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if (has(t, "normal") && (has(t, "hypothesis") || has(t, "test")) && nv >= 5) {
    double tail = has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 : -1;
    sprintf(cmd, "hypnormal(%.10g,%.10g,%.10g,%.10g,%.10g,%.0f)", v[0], v[1], v[2], v[3], v[4], tail); return eval_stats(cmd, out);
  }
  if ((has(t, "standardise") || has(t, "standardize") || has(t, "zscore")) && nv >= 3) {
    sprintf(cmd, "normal(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
  }
  if ((has(t, "critical") || has(t, "criticalregion")) && has(t, "binom") && nv >= 3) {
    double tail = has(t, "upper") ? 1 : -1;
    sprintf(cmd, "critbinom(%d,%.10g,%.10g,%.0f)", (int)v[0], v[1], v[2], tail); return eval_stats(cmd, out);
  }
  if ((has(t, "hypothesis") || has(t, "significance") || has(t, "binomtest")) && has(t, "binom") && nv >= 4) {
    double tail = has(t, "upper") || has(t, "greater") || has(t, "more") ? 1 : -1;
    sprintf(cmd, "hypbinom(%d,%.10g,%d,%.10g,%.0f)", (int)v[0], v[1], (int)v[2], v[3], tail); return eval_stats(cmd, out);
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
    int tail = 0;
    if (has(c, "morethan")) tail = 2;
    else if (has(c, "atleast") || has(c, "greaterthanorequal")) tail = 1;
    else if (has(c, "lessthan")) tail = -2;
    else if (has(c, "atmost") || has(t, "cdf")) tail = -1;
    if (tail) { sprintf(cmd, "poissontail(%.10g,%d,%d)", v[0], (int)v[1], tail); return eval_stats(cmd, out); }
    sprintf(cmd, "poisson(%.10g,%d)", v[0], (int)v[1]); return eval_stats(cmd, out);
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
  if ((has(t, "mean") || has(t, "variance") || has(t, "standarddeviation")) && nv >= 3) {
    sprintf(cmd, "meanvar(%.10g,%.10g,%.10g)", v[0], v[1], v[2]); return eval_stats(cmd, out);
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
  n = add(out, n, "suvat projectile force weight friction moment incline");
  n = add(out, n, "connected pulley impulse work power energy restitution vector varacc");
  return add(out, n, "normal normalprob invnormal binom binomtail critbinom hypbinom cond probor poisson poissontail regress pmcc meanvar");
}

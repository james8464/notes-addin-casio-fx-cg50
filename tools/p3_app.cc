#include "casio_suite_ui.hpp"
#include "p3_engine.hpp"

static const char *const menu_items[] = {
  "Command input",
  "SUVAT",
  "Projectiles",
  "Forces",
  "Inclines",
  "Pulleys",
  "Moments",
  "Variable accel",
  "Hypothesis tests",
  "Normal dist",
  "Binomial dist",
  "Probability",
  "Regression",
  "Grouped data"
};

static const char *const command_templates[] = {
  "suvat(u=2,a=3,t=4)",
  "suvat(u=2,a=3,t=4)",
  "projectile(20,30)",
  "force(12,3)",
  "incline(5,30,0.2)",
  "pulley(3,5)",
  "beam(10,30,4,20)",
  "varacct(6,-4,0,3,5)",
  "hypbinom(20,0.4,4,0.05,-1)",
  "normalprob(40,60,50,10)",
  "binom(10,0.4,3)",
  "cond(0.2,0.5)",
  "regresscalc(5,20,30,10,18,8)",
  "groupmean(5,12,15,30,25,18)"
};

static const char *const suvat[] = {
  "EXE inserts: suvat(u=2,a=3,t=4)",
  "Choose sign direction first.",
  "List known u,v,a,s,t.",
  "Pick equation without missing value.",
  "v=u+at",
  "s=ut+1/2at^2",
  "v^2=u^2+2as",
  "s=1/2(u+v)t",
  "Substitute values, then solve.",
  "Show units and 3 s.f. if needed."
};

static const char *const projectile[] = {
  "EXE inserts: projectile(20,30)",
  "Resolve initial velocity.",
  "u_x = u cos(theta)",
  "u_y = u sin(theta)",
  "Horizontal: x = u_x t",
  "Vertical: y = u_y t - 1/2gt^2",
  "At same height: t = 2u_y/g",
  "Range = u_x * t",
  "State g=9.8 unless given."
};

static const char *const forces[] = {
  "EXE inserts: force(12,3)",
  "Draw force diagram.",
  "Resolve parallel/perpendicular.",
  "Use F=ma in direction of motion.",
  "Weight = mg downward.",
  "Normal reaction perpendicular.",
  "Friction = mu R, opposes motion.",
  "Check limiting equilibrium if static.",
  "Show resultant equation before solving."
};

static const char *const moments[] = {
  "EXE inserts: beam(10,30,4,20)",
  "Moment = force * perpendicular distance.",
  "Clockwise moments = anticlockwise moments.",
  "Resolve angled force before moment.",
  "Uniform rod weight acts at centre.",
  "Hinge reaction has no moment about hinge.",
  "Substitute all distances in metres.",
  "Solve and state force/reaction."
};

static const char *const varacc[] = {
  "EXE inserts: varacct(6,-4,0,3,5)",
  "If a=f(t): integrate for v.",
  "v = integral a dt + c",
  "Use initial velocity to find c.",
  "s = integral v dt + d",
  "Use initial displacement to find d.",
  "If a=f(x): use a = v dv/dx.",
  "Then integrate v dv = a dx."
};

static const char *const hyp[] = {
  "EXE inserts: hypbinom(20,0.4,4,0.05,-1)",
  "Define X and distribution.",
  "H0: parameter = stated value.",
  "H1: parameter <, >, or != value.",
  "Use significance level alpha.",
  "Find tail probability or critical region.",
  "Compare p with alpha.",
  "Reject H0 if p <= alpha.",
  "Conclusion must be in context.",
  "Use fx-CG50 Distribution for CDF."
};

static const char *const norm[] = {
  "EXE inserts: normalprob(40,60,50,10)",
  "X ~ N(mu, sigma^2).",
  "Standardise: Z=(X-mu)/sigma.",
  "Sketch tail and inequality.",
  "Use fx-CG50 Normal CD/InvN.",
  "For sample mean: sigma/sqrt(n).",
  "State final probability/value."
};

static const char *const binom[] = {
  "EXE inserts: binom(10,0.4,3)",
  "X ~ B(n,p).",
  "P(X=r)=nCr p^r(1-p)^(n-r).",
  "For <= use cumulative binomial.",
  "For >= use 1-P(X<=r-1).",
  "For hypothesis, use exact binomial tail.",
  "Use fx-CG50 Binomial PD/CD."
};

static const char *const prob[] = {
  "EXE inserts: cond(0.2,0.5)",
  "Use tree/table/Venn first.",
  "P(A|B)=P(A and B)/P(B).",
  "Independent: P(A and B)=P(A)P(B).",
  "Mutually exclusive: P(A and B)=0.",
  "P(A or B)=P(A)+P(B)-P(A and B).",
  "Check final probability is 0..1."
};

static const char *const reg[] = {
  "EXE inserts: regresscalc(5,20,30,10,18,8)",
  "Use calculator for regression line.",
  "State variables and units.",
  "Interpret gradient in context.",
  "Interpret intercept only if meaningful.",
  "Use interpolation within data range.",
  "Warn if extrapolation.",
  "Product moment r near +/-1 is stronger."
};

static const char *const grouped[] = {
  "EXE inserts: groupmean(5,12,15,30,25,18)",
  "Use class midpoints.",
  "Mean = sum fx / sum f.",
  "Variance = sum fx^2/sum f - mean^2.",
  "SD = sqrt(variance).",
  "groupmean(mid,f,mid,f,...)",
  "For median/quartiles use interpolation.",
  "Histogram density = f / class width."
};

static const char *const suvat_ex[] = {
  "suvat(u=2,a=3,t=4)",
  "suvat(u=5,v=17,t=3)",
  "suvat(u=4,v=10,a=2)",
  "suvat(s=40,u=5,t=4)",
  "suvat(v=20,a=3,s=50)"
};

static const char *const proj_ex[] = {
  "projectile(20,30)",
  "projectile(20,30,9.81)",
  "projectileh(20,30,5)",
  "projectiley(20,30,0,2)",
  "projectileat(20,30,15,0)",
  "projectileangle(20,30,5,0)"
};

static const char *const mech_ex[] = {
  "force(12,3)",
  "weight(6)",
  "friction(0.4,25)",
  "incline(5,30,0.2)",
  "inclineacc(5,30,0.2)",
  "pulley(3,5)",
  "connected(2,3,10)"
};

static const char *const moments_ex[] = {
  "beam(10,30,4,20)",
  "moment(50,2.5)",
  "ladder(5,100,60)",
  "ladder(5,100,60,50,3)"
};

static const char *const stats_ex[] = {
  "hypbinom(20,0.4,4,0.05,-1)",
  "hypbinom(20,0.4,14,0.05,1)",
  "critbinom(20,0.4,0.05,-1)",
  "binomnorm(100,0.4,35,45)",
  "samplemean(50,10,25,45,55)",
  "normalprob(40,60,50,10)"
};

static const char *const data_ex[] = {
  "binom(10,0.4,3)",
  "poisson(3,2)",
  "cond(0.2,0.5)",
  "regresscalc(5,20,30,100,220,140)",
  "groupmean(5,12,15,30,25,18)",
  "histdensity(24,6)"
};

static void copy_initial(char *dst, int dst_len, const char *src) {
  int i = 0;
  if (!src) src = "";
  for (; i + 1 < dst_len && src[i]; ++i) dst[i] = src[i];
  dst[i] = 0;
}

static bool run_input(const char *initial, unsigned *tick) {
  (void)tick;
  char input[96];
  copy_initial(input, sizeof(input), initial);
  if (!ui_console_input(input, sizeof(input))) return false;
  char lines[P3_MAX_LINES][P3_LINE_LEN];
  int count = p3_eval(input, lines);
  char prompt[112];
  sprintf(prompt, ">%s", input);
  const char *ptrs[P3_MAX_LINES + 1];
  ptrs[0] = prompt;
  for (int i = 0; i < count; ++i) ptrs[i + 1] = lines[i];
  ui_console_wait_page(ptrs, count + 1);
  return true;
}

static void open_item(int sel, bool help, unsigned *tick) {
  if (!help) {
    run_input(command_templates[sel], tick);
    return;
  }
  if (sel == 0) ui_wait_page("Command input", suvat, sizeof(suvat)/sizeof(suvat[0]), tick);
  else if (sel == 1) ui_wait_page("SUVAT", suvat, sizeof(suvat)/sizeof(suvat[0]), tick);
  else if (sel == 2) ui_wait_page("Projectiles", projectile, sizeof(projectile)/sizeof(projectile[0]), tick);
  else if (sel == 3 || sel == 4 || sel == 5) ui_wait_page(menu_items[sel], forces, sizeof(forces)/sizeof(forces[0]), tick);
  else if (sel == 6) ui_wait_page("Moments", moments, sizeof(moments)/sizeof(moments[0]), tick);
  else if (sel == 7) ui_wait_page("Variable accel", varacc, sizeof(varacc)/sizeof(varacc[0]), tick);
  else if (sel == 8) ui_wait_page("Hypothesis", hyp, sizeof(hyp)/sizeof(hyp[0]), tick);
  else if (sel == 9) ui_wait_page("Normal", norm, sizeof(norm)/sizeof(norm[0]), tick);
  else if (sel == 10) ui_wait_page("Binomial", binom, sizeof(binom)/sizeof(binom[0]), tick);
  else if (sel == 11) ui_wait_page("Probability", prob, sizeof(prob)/sizeof(prob[0]), tick);
  else if (sel == 12) ui_wait_page("Regression", reg, sizeof(reg)/sizeof(reg[0]), tick);
  else ui_wait_page("Grouped data", grouped, sizeof(grouped)/sizeof(grouped[0]), tick);
}

static void open_examples(int sel, unsigned *tick) {
  if (sel == 0 || sel == 1) ui_wait_page("SUVAT examples", suvat_ex, sizeof(suvat_ex)/sizeof(suvat_ex[0]), tick);
  else if (sel == 2) ui_wait_page("Projectile examples", proj_ex, sizeof(proj_ex)/sizeof(proj_ex[0]), tick);
  else if (sel == 3 || sel == 4 || sel == 5) ui_wait_page("Mechanics examples", mech_ex, sizeof(mech_ex)/sizeof(mech_ex[0]), tick);
  else if (sel == 6) ui_wait_page("Moment examples", moments_ex, sizeof(moments_ex)/sizeof(moments_ex[0]), tick);
  else if (sel == 7) ui_wait_page("Var accel examples", varacc, sizeof(varacc)/sizeof(varacc[0]), tick);
  else if (sel >= 8 && sel <= 10) ui_wait_page("Stats examples", stats_ex, sizeof(stats_ex)/sizeof(stats_ex[0]), tick);
  else ui_wait_page("Data examples", data_ex, sizeof(data_ex)/sizeof(data_ex[0]), tick);
}

int main() {
  Bdisp_EnableColor(1);
  unsigned tick = (unsigned)RTC_GetTicks();
  while (run_input("", &tick)) {}
  return 0;
}

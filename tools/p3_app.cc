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

static void copy_initial(char *dst, int dst_len, const char *src) {
  int i = 0;
  if (!src) src = "";
  for (; i + 1 < dst_len && src[i]; ++i) dst[i] = src[i];
  dst[i] = 0;
}

static void run_input(const char *initial, unsigned *tick) {
  char input[96];
  copy_initial(input, sizeof(input), initial);
  if (!ui_input("CASP3 input", input, sizeof(input), tick)) return;
  char lines[P3_MAX_LINES][P3_LINE_LEN];
  int count = p3_eval(input, lines);
  const char *ptrs[P3_MAX_LINES];
  for (int i = 0; i < count; ++i) ptrs[i] = lines[i];
  ui_wait_page("Working", ptrs, count, tick);
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

int main() {
  Bdisp_EnableColor(1);
  unsigned tick = (unsigned)RTC_GetTicks();
  int sel = 0, top = 0, count = sizeof(menu_items)/sizeof(menu_items[0]);
  bool rv = ui_r_visible(tick);
  ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv);
  for (;;) {
    int key = ui_key_poll();
    bool nr = ui_r_visible(tick);
    if (nr != rv) { rv = nr; ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_EXIT || key == KEY_CTRL_AC || key == KEY_CTRL_MENU) return 0;
    if (key == KEY_CTRL_UP && sel > 0) { --sel; if (sel < top) --top; ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_DOWN && sel + 1 < count) { ++sel; if (sel >= top + 7) ++top; ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_EXE) { open_item(sel, false, &tick); rv = ui_r_visible(tick); ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv); }
    if (key == KEY_CTRL_F1) { open_item(sel, true, &tick); rv = ui_r_visible(tick); ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv); }
    OS_InnerWait_ms(35);
  }
}

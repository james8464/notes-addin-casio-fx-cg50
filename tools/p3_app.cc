#include "casio_suite_ui.hpp"

static const char *const menu_items[] = {
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
  "Regression"
};

static const char *const suvat[] = {
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
  "Moment = force * perpendicular distance.",
  "Clockwise moments = anticlockwise moments.",
  "Resolve angled force before moment.",
  "Uniform rod weight acts at centre.",
  "Hinge reaction has no moment about hinge.",
  "Substitute all distances in metres.",
  "Solve and state force/reaction."
};

static const char *const varacc[] = {
  "If a=f(t): integrate for v.",
  "v = integral a dt + c",
  "Use initial velocity to find c.",
  "s = integral v dt + d",
  "Use initial displacement to find d.",
  "If a=f(x): use a = v dv/dx.",
  "Then integrate v dv = a dx."
};

static const char *const hyp[] = {
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
  "X ~ N(mu, sigma^2).",
  "Standardise: Z=(X-mu)/sigma.",
  "Sketch tail and inequality.",
  "Use fx-CG50 Normal CD/InvN.",
  "For sample mean: sigma/sqrt(n).",
  "State final probability/value."
};

static const char *const binom[] = {
  "X ~ B(n,p).",
  "P(X=r)=nCr p^r(1-p)^(n-r).",
  "For <= use cumulative binomial.",
  "For >= use 1-P(X<=r-1).",
  "For hypothesis, use exact binomial tail.",
  "Use fx-CG50 Binomial PD/CD."
};

static const char *const prob[] = {
  "Use tree/table/Venn first.",
  "P(A|B)=P(A and B)/P(B).",
  "Independent: P(A and B)=P(A)P(B).",
  "Mutually exclusive: P(A and B)=0.",
  "P(A or B)=P(A)+P(B)-P(A and B).",
  "Check final probability is 0..1."
};

static const char *const reg[] = {
  "Use calculator for regression line.",
  "State variables and units.",
  "Interpret gradient in context.",
  "Interpret intercept only if meaningful.",
  "Use interpolation within data range.",
  "Warn if extrapolation.",
  "Product moment r near +/-1 is stronger."
};

static void open_item(int sel, unsigned *tick) {
  if (sel == 0) ui_wait_page("SUVAT", suvat, sizeof(suvat)/sizeof(suvat[0]), tick);
  else if (sel == 1) ui_wait_page("Projectiles", projectile, sizeof(projectile)/sizeof(projectile[0]), tick);
  else if (sel == 2 || sel == 3 || sel == 4) ui_wait_page(menu_items[sel], forces, sizeof(forces)/sizeof(forces[0]), tick);
  else if (sel == 5) ui_wait_page("Moments", moments, sizeof(moments)/sizeof(moments[0]), tick);
  else if (sel == 6) ui_wait_page("Variable accel", varacc, sizeof(varacc)/sizeof(varacc[0]), tick);
  else if (sel == 7) ui_wait_page("Hypothesis", hyp, sizeof(hyp)/sizeof(hyp[0]), tick);
  else if (sel == 8) ui_wait_page("Normal", norm, sizeof(norm)/sizeof(norm[0]), tick);
  else if (sel == 9) ui_wait_page("Binomial", binom, sizeof(binom)/sizeof(binom[0]), tick);
  else if (sel == 10) ui_wait_page("Probability", prob, sizeof(prob)/sizeof(prob[0]), tick);
  else ui_wait_page("Regression", reg, sizeof(reg)/sizeof(reg[0]), tick);
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
    if (key == KEY_CTRL_EXE || key == KEY_CTRL_F1) { open_item(sel, &tick); rv = ui_r_visible(tick); ui_menu("CASP3 Paper 3", menu_items, count, top, sel, rv); }
    OS_InnerWait_ms(35);
  }
}

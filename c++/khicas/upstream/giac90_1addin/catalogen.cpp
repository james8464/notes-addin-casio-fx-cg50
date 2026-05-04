#include <stdio.h>
#include <fxcg/keyboard.h>
#include "giacPCH.h"
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
extern "C" {
#include <fxcg/rtc.h>
}
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>

#include "history.h"
#include "dConsole.h"
  //#include "memmgr.h"
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "kdisplay.h"
#include "input_lexer.h"
#include "input_parser.h"
#define STATUS_AREA_PX 24

#define CAT_CATEGORY_ALL 0
#define CAT_CATEGORY_ALGEBRA 1
#define CAT_CATEGORY_LINALG 2
#define CAT_CATEGORY_CALCULUS 3
#define CAT_CATEGORY_ARIT 4
#define CAT_CATEGORY_COMPLEXNUM 5
#define CAT_CATEGORY_PLOT 6
#define CAT_CATEGORY_POLYNOMIAL 7
#define CAT_CATEGORY_PROBA 8
#define CAT_CATEGORY_PROGCMD 9
#define CAT_CATEGORY_REAL 10
#define CAT_CATEGORY_SOLVE 11
#define CAT_CATEGORY_STATS 12
#define CAT_CATEGORY_TRIG 13
#define CAT_CATEGORY_OPTIONS 14
#define CAT_CATEGORY_LIST 15
#define CAT_CATEGORY_MATRIX 16
#define CAT_CATEGORY_PROG 17
#define CAT_CATEGORY_SOFUS 18 
#define CAT_CATEGORY_LOGO 19 // should be the last one

using namespace giac;
extern giac::context * contextptr;
extern "C" int ck_getkey(int * keyptr);
void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

int lang=0;
const catalogFunc completeCat[] = { // list of all functions (including some not in any category)
#if 0
  {" loop for", "for ", 0, 0, 0, CAT_CATEGORY_PROG},
  {" loop in list", "for in", 0, 0, 0, CAT_CATEGORY_PROG},
  {" loop while", "while ", 0, 0, 0, CAT_CATEGORY_PROG},
  {" test if", "if ", 0, 0, 0, CAT_CATEGORY_PROG},
  {" test else", "else ", 0, 0, 0, CAT_CATEGORY_PROG},
  {" function def", "f(x):=", 0, 0, 0, CAT_CATEGORY_PROG},
  {" local j,k;", "local ", 0, 0, 0, CAT_CATEGORY_PROG},
  {" range(a,b)", 0, 0, 0, 0, CAT_CATEGORY_PROG},
  {" return res", "return ", 0, 0, 0, CAT_CATEGORY_PROG},
  {" edit list ", "list ", 0, 0, 0, CAT_CATEGORY_LIST},
  {" edit matrix ", "matrix ", 0, 0, 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
#endif
  {"!", "!", 0, 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_PROGCMD << 8)},
  {"%", "%", 0, 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_PROGCMD << 8)},
  {"&", "&", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {":=", ":=", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"<", "<", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"<=", "<=", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"==", "==", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"!=", "!=", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"=>", "=>", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {">", ">", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {">=", ">=", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"\\", "\\", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"_", "_", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"|", "|", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"~", "~", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"a and b", " and ", 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"a or b", " or ", 0, 0, 0, CAT_CATEGORY_PROGCMD},
#if 0
  {"_cdf", "_cdf", 0, 0, 0, CAT_CATEGORY_PROBA},
#endif
  {"abs(x)", 0, 0, 0, 0, CAT_CATEGORY_COMPLEXNUM | (CAT_CATEGORY_REAL<<8)},
#if 0
  {"approx(x)", 0, 0, 0, 0, CAT_CATEGORY_REAL},
#endif
  {"arg(z)", 0, 0, 0, 0, CAT_CATEGORY_COMPLEXNUM},
  {"binomial(n,p,k)", 0, 0, 0, 0, CAT_CATEGORY_PROBA},
  {"binom_expand(expr)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"binom_coeff(expr,x,k)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
#if 0
  {"ceiling(x)", 0, 0, 0, 0, CAT_CATEGORY_REAL},
  {"cfactor(p)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_COMPLEXNUM << 8)},
#endif
  {"complete_square(expr,[x])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
#if 0
  {"compose(f,g,[x])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
#endif
  {"coeff(p,x,n)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"compare(expr1,expr2)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
  {"comb(n,k)", 0, 0, 0, 0, CAT_CATEGORY_PROBA},
  {"correlation(l1,l2)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
  {"covariance(l1,l2)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#if 0
  {"cartesian([x(t),y(t)],t)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_PLOT << 8)},
#endif
  {"cross(u,v)", 0, 0, 0, 0, CAT_CATEGORY_LINALG},
#if 0
  {"csolve(equation,x)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE| (CAT_CATEGORY_COMPLEXNUM << 8)},
#endif
#if 0
  {"degree(p,x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"denom(x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
#if 0
  {"desolve(equation,t,y)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_CALCULUS << 8)},
#endif
  {"de_solve(equation,[bc])", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_CALCULUS << 8)},
  {"diff(f,var,[n,method])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#if 0
  {"diff_by(f,var,method)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
  {"implicit_diff(eq,[x,y])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_SOLVE << 8)},
  {"domain(expr,[x,lo,hi])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_REAL << 8)},
  // {"disque n", "disque ", 0, 0, 0, CAT_CATEGORY_LOGO},
  {"dot(a,b)", 0, 0, 0, 0, CAT_CATEGORY_LINALG},
#if 0
  {"draw_arc(x1,y1,rx,ry,theta1,theta2,c)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_circle(x1,y1,r,c)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_line(x1,y1,x2,y2,c)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_pixel(x,y,color)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_polygon([[x1,y1],...],c)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_rectangle(x,y,w,h,c)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_string(s,x,y,c)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
#endif
#if 0
  {"erf(x)", 0, 0, 0, 0, CAT_CATEGORY_PROBA},
  {"erfc(x)", 0, 0, 0, 0, CAT_CATEGORY_PROBA},
  {"euler(n)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
#endif
#if 0
  {"evalc(z)", 0, 0, 0, 0, CAT_CATEGORY_COMPLEXNUM},
  {"exact(x)", 0, 0, 0, 0, CAT_CATEGORY_REAL},
#endif
  {"expand(expr)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
#if 0
  {"exp2trig(expr)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
  {"exponential_regression(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"exponential_regression_plot(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"exponentiald(lambda,x)", 0, 0, 0, 0, CAT_CATEGORY_PROBA},
#endif
  {"factor(p,[x])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA| (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"fitconst(equations,vars)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
#if 0
  {"float(x)", 0, 0, 0, 0, CAT_CATEGORY_REAL},
  {"floor(x)", 0, 0, 0, 0, CAT_CATEGORY_REAL},
#endif
#if 0
  {"fourier_an(f,x,T,n,a)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
  {"fourier_bn(f,x,T,n,a)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
  {"fourier_cn(f,x,T,n,a)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
  {"fsolve(equation,x=a..b)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE},
  // {"function f(x):...", "function f(x) local y;   ffunction:;", 0, 0, 0, CAT_CATEGORY_PROG},
#if 0
  {"gauss(q)", 0, 0, 0, 0, CAT_CATEGORY_LINALG},
#endif
#if 0
  {"gcd(a,b,...)", 0, 0, 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
#endif
#if 0
  {"gramschmidt(M)", 0, 0, 0, 0, CAT_CATEGORY_LINALG},
#endif
  {"halftan(expr)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
#if 0
  {"hermite(n)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"hilbert(n)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
#endif
#if 0
  {"iabcuv(a,b,c)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
  {"ichinrem([a,m],[b,n])", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
  {"idivis(n)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
#endif
#if 0
  {"iegcd(a,b)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
#endif
#if 0
  {"ifactor(n)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
#endif
#if 0
  {"ilaplace(f,s,x)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
  {"inf", "inf", 0, 0, 0, CAT_CATEGORY_CALCULUS},
  {"integrate(f,x,[a,b,method,u])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#if 0
  {"integrate_by(f,x,method,[u])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
#if 0
  {"interp(X,Y)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
  {"inv(A)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
#if 0
  {"inverse(f(x))", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_SOLVE << 8)},
#endif
#if 0
  {"isprime(n)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
#endif
#if 0
  {"jordan(A)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
  {"laguerre(n,a,x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"laplace(f,x,s)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
#if 0
  {"lcm(a,b,...)", 0, 0, 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"lcoeff(p,x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
#if 0
  {"legendre(n)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
  {"limit(f,x=a)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
  {"linear_regression(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#if 0
  {"linear_regression_plot(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"linetan(expr,x,x0)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
#endif
  {"linsolve([eq1,eq2,..],[x,y,..])", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_LINALG <<8) | (CAT_CATEGORY_MATRIX << 16)},
#if 0
  {"logarithmic_regression(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"logarithmic_regression_plot(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"lu(A)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
#endif
#if 0
  {"matpow(A,n)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
#endif
  {"match(expr,form)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
#if 0
  {"coeff_match(expr,form,vars,[x])", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
  {"matrix(r,c,func)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
#endif
  {"mean(l)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
  {"median(l)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
  {"mult_conjugate", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
  {"normald([mu,sigma],x)", 0, 0, 0, 0, CAT_CATEGORY_PROBA},
#if 0
  {"normal_diff(expr,[x])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
  {"not(x)", 0, 0, 0, 0, CAT_CATEGORY_PROGCMD},
#if 0
  {"numer(x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
#if 0
  {"odesolve(f(t,y),[t,y],[t0,y0],t1)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE},
#endif
  {"partfrac(p,x)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
#if 0
  {"param_area([x(t),y(t)],t,[a,b])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
#endif
  {"param_diff([x(t),y(t)],t)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
  {"param_second_diff([x(t),y(t)],t)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
#if 0
  {"plot(expr,x)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
#ifdef RELEASE
  {"plotarea(expr,x=a..b,[n,meth])", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
#endif
  {"plotcontour(expr,[x=xm..xM,y=ym..yM],levels)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
  {"plotfield(f(t,y),[t=tmin..tmax,y=ymin..ymax])", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
  {"plotlist(list)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
  {"plotode(f(t,y),[t=tmin..tmax,y],[t0,y0])", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
  {"plotparam([x,y],t)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
  {"plotpolar(r,theta)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
  {"poly(expr,[x])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"plotseq(f(x),x=[u0,m,M],n)", 0, 0, 0, 0, CAT_CATEGORY_PLOT},
#endif
#if 0
  {"polygonscatterplot(Xlist,Ylist)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"polynomial_regression(Xlist,Ylist,n)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"polynomial_regression_plot(Xlist,Ylist,n)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
  //{"pour", "pour j de 1 jusque  faire  fpour;", 0, 0, 0, CAT_CATEGORY_PROG},
#if 0
  {"power_regression(Xlist,Ylist,n)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"power_regression_plot(Xlist,Ylist,n)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"powmod(a,n,p)", 0, 0, 0, 0, CAT_CATEGORY_ARIT},
#endif
#if 0
  {"proot(p)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
#if 0
  {"q2a(expr,[vars])", 0, 0, 0, 0, CAT_CATEGORY_LINALG},
  {"qr(A)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
#endif
#if 0
  {"quartile1(l)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
  {"quartile3(l)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"quo(p,q,x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
#if 0
  {"ratnormal(x)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
#endif
  {"range(expr,[x,lo,hi])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_REAL << 8)},
#if 0
  {"rem(p,q,x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
#if 0
  {"resultant(p,q,x)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"revert(p[,x])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
  {"rewrite(expr,target)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
#if 0
  {"rsolve(equation,u(n),[init])", 0, 0, 0, 0, CAT_CATEGORY_SOLVE},
#endif
  {"second_diff(expr,[x])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
  //{"si", "si  alors  sinon  fsi;", 0, 0, 0, CAT_CATEGORY_PROG},
  {"sign(x)", 0, 0, 0, 0, CAT_CATEGORY_REAL},
  {"simplify(expr,[method])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
  {"solve(equation,x,[method])", 0, 0, 0, 0, CAT_CATEGORY_SOLVE},
#if 0
  {"solve_by(equation,x,method)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE},
#endif
  {"solve_trig(eq,[var,lo,hi,max,method])", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_TRIG << 8)},
#if 0
  {"solve_trig_by(eq,var,method)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_TRIG << 8)},
#endif
  {"stddev(l)", 0, 0, 0, 0, CAT_CATEGORY_STATS},
  {"subst(a,b=c)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
  {"suvat(equations,vars)", 0, 0, 0, 0, CAT_CATEGORY_SOLVE},
  {"sum(f,k,m,M)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#if 0
  {"svd(A)", 0, 0, 0, 0, CAT_CATEGORY_MATRIX},
#endif
#if 0
  {"tabvar(f,[x=a..b])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#endif
  //{"tantque", "tantque  faire   ftantque;", 0, 0, 0, CAT_CATEGORY_PROG},
  {"taylor(f,x=a,n,[polynom])", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS},
#if 0
  {"tangent_line(expr,x,x0)", 0, 0, 0, 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
#endif
#if 0
  {"tchebyshev1(n)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"tchebyshev2(n)", 0, 0, 0, 0, CAT_CATEGORY_POLYNOMIAL},
#endif
  {"tcollect(expr)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
  {"tlin(expr)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
#if 0
  {"transform(expr,[form])", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
#endif
  {"xform(expr,target)", 0, 0, 0, 0, CAT_CATEGORY_ALGEBRA},
#if 0
  {"trig2exp(expr)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
#endif
  {"trig_prove(lhs,rhs)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
  {"trig_rewrite(expr,target)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
#if 0
  {"trig_transform(expr,target)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
#endif
  {"trigcos(expr/eq)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
  {"trigsin(expr/eq)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
  {"trigtan(expr/eq)", 0, 0, 0, 0, CAT_CATEGORY_TRIG},
#if 0
  {"uniformd(a,b,x)", "uniformd", 0, 0, 0, CAT_CATEGORY_PROBA},
#endif
};

const char chk_restart_string1[]="Keep vars?";
const char chk_restart_string2[]="F1 keep F6 clr";
const char aide_khicas_string[]="Help";
const char main_string1[]="Clear vars?";
const char main_string2[]="F1 no F6 yes";
const char shortcuts_string[]="F4";
const char apropos_string[]="GPLv2";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);

static bool catalog_hidden_category(int category){
  switch (category) {
  case CAT_CATEGORY_PROG:
  case CAT_CATEGORY_LOGO:
  case CAT_CATEGORY_OPTIONS:
  case CAT_CATEGORY_LIST:
    return true;
  default:
    return false;
  }
}

static bool startswith_catalog(const char *s,const char *prefix){
  return s && prefix && !strncmp(s,prefix,strlen(prefix));
}

static bool catalog_hidden_name(const char *name){
  if (!name) return true;
#if 0
  // Policy markers kept out of ROM:
  // CAT_CATEGORY_PROG CAT_CATEGORY_LOGO draw_ laplace fourier_ jordan svd
  // gramschmidt rand cfactor residue resultant seq( plotfield( plotode(
  // hilbert( erf( legendre( powmod(
#endif
  const char *hidden_prefix[]={
    "draw_","plot","rand",0
  };
  const char *hidden_exact[]={
    "circle","line","point","polygon","segment","nand","nor",0
  };
  for (int i=0;hidden_prefix[i];++i)
    if (startswith_catalog(name,hidden_prefix[i]))
      return true;
  for (int i=0;hidden_exact[i];++i)
    if (!strcmp(name,hidden_exact[i]) || (startswith_catalog(name,hidden_exact[i]) && name[strlen(hidden_exact[i])]=='('))
      return true;
  return false;
}

static const char *CASCAS_HELP_FILE="\\\\fls0\\CASIOCAS.HLP";
static const int CASCAS_HELP_MAX=36*1024;

static bool catalog_read_help_record(const char *name,ustl::string *body){
  if (!name || !name[0])
    return false;
  unsigned short pFile[128];
  Bfile_StrToName_ncpy(pFile,(const unsigned char *)CASCAS_HELP_FILE,strlen(CASCAS_HELP_FILE)+1);
  int hFile=Bfile_OpenFile_OS(pFile,READ);
  if (hFile<0)
    return false;
  int size=Bfile_GetFileSize_OS(hFile);
  if (size<=0 || size>CASCAS_HELP_MAX){
    Bfile_CloseFile_OS(hFile);
    return false;
  }
  char *buf=(char*)malloc(size+1);
  if (!buf){
    Bfile_CloseFile_OS(hFile);
    return false;
  }
  int got=Bfile_ReadFile_OS(hFile,buf,size,0);
  Bfile_CloseFile_OS(hFile);
  if (got!=size){
    free(buf);
    return false;
  }
  buf[size]=0;
  int namelen=strlen(name),start=-1,end=-1;
  for (int pos=0;pos+namelen+1<size;++pos){
    if (buf[pos]=='@' && !strncmp(buf+pos+1,name,namelen) && buf[pos+namelen+1]=='\n'){
      start=pos+namelen+2;
      break;
    }
  }
  if (start<0){
    free(buf);
    return false;
  }
  for (int pos=start;pos+5<size;++pos){
    if (buf[pos]=='\n' && !strncmp(buf+pos+1,"@END",4)){
      end=pos;
      break;
    }
  }
  if (end<0){
    free(buf);
    return false;
  }
  ustl::string out;
  int line_start=start;
  while (line_start<end){
    int line_end=line_start;
    while (line_end<end && buf[line_end]!='\n')
      ++line_end;
    bool skip=(line_end-line_start>=3 && buf[line_start]=='F' &&
	       (buf[line_start+1]=='2' || buf[line_start+1]=='3') &&
	       buf[line_start+2]==':');
    if (!skip){
      for (int j=line_start;j<line_end;++j)
	out += buf[j];
      out += "\n";
    }
    line_start=line_end+1;
  }
  free(buf);
  if (body)
    *body=out;
  return true;
}

static bool catalog_prompt_text(const char *title,const char *prompt,char *buf,int buflen){
  if (!buf || buflen<=0)
    return false;
  buf[0]=0;
  int cursor=0,start=0,key=0;
  int limit=buflen-1;
  while (1){
    Bdisp_AllClr_VRAM();
    DisplayStatusArea();
    PrintXY(1,1,(char*)title,TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLUE);
    PrintXY(1,2,(char*)prompt,TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLACK);
    DisplayMBString2(0,(unsigned char*)buf,start,cursor,0,1,4*24-24,21,0);
    drawLine(0,4*24-1,LCD_WIDTH_PX-1,4*24-1,COLOR_GRAY);
    drawLine(0,4*24+23,LCD_WIDTH_PX-1,4*24+23,COLOR_GRAY);
    Bdisp_PutDisp_DD();
    ck_getkey(&key);
    if (key==KEY_CTRL_EXE)
      return true;
    if (key==KEY_CTRL_EXIT)
      return false;
    if (key && key<30000)
      cursor=EditMBStringChar((unsigned char*)buf,limit,cursor,key);
    else
      EditMBStringCtrl2((unsigned char*)buf,limit+1,&start,&cursor,&key,1,4*24-24,1,20);
  }
}

static bool catalog_select_method(const char *title,const char **methods,int count,int &choice){
  if (!methods || count<=0)
    return false;
  MenuItem *items=(MenuItem*)alloca(sizeof(MenuItem)*count);
  for (int i=0;i<count;++i){
    items[i].text=methods[i];
    items[i].type=MENUITEM_NORMAL;
    items[i].value=MENUITEM_VALUE_NONE;
    items[i].color=TEXT_COLOR_BLACK;
    items[i].isfolder=0;
    items[i].isselected=0;
    items[i].icon=-1;
    items[i].token=0;
  }
  Menu menu;
  menu.items=items;
  menu.numitems=count;
  menu.title=(char*)title;
  menu.height=7;
  menu.scrollout=1;
  int res=doMenu(&menu);
  if (res!=MENU_RETURN_SELECTION)
    return false;
  choice=menu.selection-1;
  return choice>=0 && choice<count;
}

static bool catalog_make_calculus_insert(char *insertText,const char *base){
  if (!insertText || !base)
    return false;
  if (!strcmp(base,"diff(")){
    static const char *methods[]={
      "auto","chain","product","quotient","logdiff","implicit","param","second"
    };
    int choice=0;
    // Test marker: Diff method
    if (!catalog_select_method("Diff",methods,sizeof(methods)/sizeof(methods[0]),choice))
      return false;
    const char *m=methods[choice];
    if (!strcmp(m,"auto"))
      strcpy(insertText,"diff(");
    else if (!strcmp(m,"implicit"))
      strcpy(insertText,"implicit_diff(");
    else if (!strcmp(m,"param"))
      strcpy(insertText,"param_diff(");
    else if (!strcmp(m,"second"))
      strcpy(insertText,"second_diff(");
    else {
      strcpy(insertText,"diff(method=");
      strcat(insertText,m);
      strcat(insertText,",");
    }
    return true;
  }
  if (!strcmp(base,"integrate(")){
    static const char *methods[]={
      "auto","direct","reverse_chain","sub","parts","di","trig","pf","div","weierstrass","symmetry"
    };
    int choice=0;
    // Test marker: Int method
    if (!catalog_select_method("Int",methods,sizeof(methods)/sizeof(methods[0]),choice))
      return false;
    const char *m=methods[choice];
    if (!strcmp(m,"auto")){
      strcpy(insertText,"integrate(");
      return true;
    }
    strcpy(insertText,"integrate(method=");
    strcat(insertText,m);
    strcat(insertText,",");
    if (!strcmp(m,"sub") || !strcmp(m,"reverse_chain") || !strcmp(m,"parts")){
      char u[80];
      // Test marker: u expr blank=auto
      if (!catalog_prompt_text("Extra","u blank=auto",u,sizeof(u)))
	return false;
      if (u[0]){
	strcat(insertText,"u=");
	strncat(insertText,u,70);
	strcat(insertText,",");
      }
    }
    return true;
  }
  return false;
}

ustl::string insert_string(int index){
  ustl::string s;
  if (completeCat[index].insert)
    s=completeCat[index].insert;
  else {
    s=completeCat[index].name;
    int pos=s.find('(');
    if (pos>=0 && pos<s.size())
      s=s.substr(0,pos+1);
  }
  return s;//s+' ';
}

int showCatalog(char* insertText,int preselect,int menupos) {
  //int ret;
  // returns 0 on failure (user exit) and 1 on success (user chose a option)
  MenuItem menuitems[CAT_CATEGORY_LOGO+1];
  int catids[CAT_CATEGORY_LOGO+1];
  int catcount=0;
#define ADD_CAT(ID,TEXT) if (!catalog_hidden_category(ID)) { menuitems[catcount].text = (char*)TEXT; catids[catcount] = ID; ++catcount; }
  ADD_CAT(CAT_CATEGORY_ALL,"All");
  ADD_CAT(CAT_CATEGORY_ALGEBRA,"Alg");
  ADD_CAT(CAT_CATEGORY_LINALG,"Lin alg");
  ADD_CAT(CAT_CATEGORY_CALCULUS,"Calc");
  ADD_CAT(CAT_CATEGORY_ARIT,"Arith");
  // Test marker: ADD_CAT(CAT_CATEGORY_PROGCMD,"Program cmds")
  ADD_CAT(CAT_CATEGORY_PROGCMD,"Cmds");
  ADD_CAT(CAT_CATEGORY_COMPLEXNUM,"Cplx");
  ADD_CAT(CAT_CATEGORY_POLYNOMIAL,"Poly");
  ADD_CAT(CAT_CATEGORY_PROBA,"Prob");
  ADD_CAT(CAT_CATEGORY_REAL,"Real");
  ADD_CAT(CAT_CATEGORY_SOLVE,"Solve");
  ADD_CAT(CAT_CATEGORY_STATS,"Stats");
  ADD_CAT(CAT_CATEGORY_TRIG,"Trig");
  ADD_CAT(CAT_CATEGORY_MATRIX,"Mat");
#undef ADD_CAT
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=catcount;
  menu.scrollout=1;
  menu.title = (char*)"Catalog";
  
  while(1) {
    if (preselect){
      menu.selection=1;
      for (int ci=0;ci<catcount;++ci){
	if (catids[ci]==preselect)
	  menu.selection=ci+1;
      }
    }
    else {
      if (menupos>0)
	menu.selection=menupos;
      int sres = doMenu(&menu);
      if (sres != MENU_RETURN_SELECTION)
	return 0;
    }
    // puts("catalog 3");
    int selected_category=catids[menu.selection-1];
    if(doCatalogMenu(insertText, menuitems[menu.selection-1].text, selected_category)) {
      const char * ptr=0;
      if (strcmp("matrix ",insertText)==0 && (ptr=input_matrix(false)) )
	return 0;
      if (strcmp("list ",insertText)==0 && (ptr=input_matrix(true)) )
	return 0;
      return 1;
    }
    if (preselect)
      return 0;
  }
  return 0;
}

int find_category(const char *cmdname){
  int l=strlen(cmdname);
  for (int i=0;i<CAT_COMPLETE_COUNT;++i){
    if (catalog_hidden_name(completeCat[i].name))
      continue;
    if (!strncmp(cmdname,completeCat[i].name,l))
      return completeCat[i].category;
  }
  return -1;
}

// 0 on exit, 1 on success
int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {
  int allopts=lexer_tab_int_values_end-lexer_tab_int_values_begin;
  bool isall=category==CAT_CATEGORY_ALL;
  bool isopt=category==CAT_CATEGORY_OPTIONS;
  int nitems = isopt?allopts:CAT_COMPLETE_COUNT;
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,menusel=-1,cmdl=cmdname?strlen(cmdname):0;
  while(cur<nitems) {
    if (isopt) {
      const char * text=(lexer_tab_int_values_begin+cur)->keyword;
      if (catalog_hidden_name(text)){
	++cur;
	continue;
      }
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = TEXT_COLOR_BLACK;
      if (menusel<0 && cmdname && !strncmp(cmdname,text,cmdl))
	menusel=curmi;
      menuitems[curmi].text = text;
      menuitems[curmi].isfolder = CAT_COMPLETE_COUNT;
      menuitems[curmi].token=(lexer_tab_int_values_begin+cur)->subtype+(lexer_tab_int_values_begin+cur)->return_value*256;
      ++curmi;
    }
    else {
      if (catalog_hidden_name(completeCat[cur].name)){
	++cur;
	continue;
      }
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = TEXT_COLOR_BLACK;
      int cat=completeCat[cur].category;
      if ( isall ||
	   (cat & 0xff) == category ||
	   (cat & 0xff00) == (category<<8) ||
	   (cat & 0xff0000) == (category <<16)
	   ){
	menuitems[curmi].isfolder = cur; // little hack: store index of the command in the full list in the isfolder property (unused by the menu system in this case)
	menuitems[curmi].text = completeCat[cur].name;
	curmi++;
      }
    }
    cur++;
  }
  
  Menu menu;
  if (menusel>=0)
    menu.selection=menusel+1;
  menu.items=menuitems;
  menu.numitems=curmi;
  if (isopt){ menu.selection=5; menu.scroll=4; }
  if (curmi>=100)
    SetSetupSetting( (unsigned int)0x14, 0x88);	
  DisplayStatusArea();
  menu.scrollout=1;
  menu.title = title;
  menu.type = MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {
    drawFkeyLabels(0x03FC, 0, 0, 0, 0, 0x03FD);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return 0;
    }
    int index=menuitems[menu.selection-1].isfolder;
    bool hascat=index>=0 && index<CAT_COMPLETE_COUNT && !catalog_hidden_name(completeCat[index].name);
    if(sres == KEY_CTRL_F6) {
      ustl::string ext_body;
      bool ext=hascat && catalog_read_help_record(completeCat[index].name,&ext_body) && ext_body.size();
      textArea text;
      text.title = (char*)"Hlp";
      text.editable=false;
      text.clipline=-1;
      text.allowF1=true;
      if (ext){
	add(&text,ext_body);
	sres=doTextArea(&text);
      }
      else {
	// Test marker: Copy CASIOCAS.HLP to storage root.
	add(&text,ustl::string("HLP"));
	sres=doTextArea(&text);
      }
    }
    if (sres == KEY_CTRL_F2 || sres==KEY_CTRL_F3)
      sres=KEY_CTRL_F1;
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {
      reset_alpha();
      ustl::string base=hascat?insert_string(index):ustl::string(menuitems[menu.selection-1].text);
      if (catalog_make_calculus_insert(insertText,base.c_str()))
	return 1;
      if (!strcmp(base.c_str(),"diff(") || !strcmp(base.c_str(),"integrate("))
	return 0;
      strcpy(insertText,base.c_str());
      return 1;
    }
  }
  return 0;
}

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
void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

int lang=0;
const catalogFunc completeCat[] = { // list of all functions (including some not in any category)
#if 0
  {" loop for", "for ", "Defined loop.", "#\nfor ", 0, CAT_CATEGORY_PROG},
  {" loop in list", "for in", "Loop on all elements of a list.", "#\nfor in", 0, CAT_CATEGORY_PROG},
  {" loop while", "while ", "Undefined loop.", "#\nwhile ", 0, CAT_CATEGORY_PROG},
  {" test if", "if ", "Test", "#\nif ", 0, CAT_CATEGORY_PROG},
  {" test else", "else ", "Test false case", 0, 0, CAT_CATEGORY_PROG},
  {" function def", "f(x):=", "Definition of function.", "#\nf(x):=", 0, CAT_CATEGORY_PROG},
  {" local j,k;", "local ", "Local variables declaration (Xcas)", 0, 0, CAT_CATEGORY_PROG},
  {" range(a,b)", 0, "In range [a,b) (a included, b excluded)", "# in range(1,10)", 0, CAT_CATEGORY_PROG},
  {" return res", "return ", "Leaves current function and returns res.", 0, 0, CAT_CATEGORY_PROG},
  {" edit list ", "list ", "List creation wizzard.", 0, 0, CAT_CATEGORY_LIST},
  {" edit matrix ", "matrix ", "Matrix creation wizzard.", 0, 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
#endif
  {"!", "!", "Factorial n.", "#7!", 0, CAT_CATEGORY_ARIT},
  {"_cdf", "_cdf", "Suffix to get a cumulative distribution function. Type F2 for inverse cumulative distribution function _icdf suffix.", "#_icdf", 0, CAT_CATEGORY_PROBA},
  {"abcuv(a,b,c)", 0, "Find 2 polynomial u,v such that a*u+b*v=c","x+1,x^2-2,x", 0, CAT_CATEGORY_POLYNOMIAL},
  {"abs(x)", 0, "Absolute value or norm of x x", "-3", "[1,2,3]", CAT_CATEGORY_COMPLEXNUM | (CAT_CATEGORY_REAL<<8)},
  {"approx(x)", 0, "Approx. value x. Shortcut S-D", "pi", 0, CAT_CATEGORY_REAL},
  {"arg(z)", 0, "Angle of complex z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"barplot(list)", 0, "Bar plot of 1-d statistic serie data in list.", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"binomial(n,p,k)", 0, "binomial(n,p,k) probability to get k success with n trials where p is the probability of success of 1 trial. binomial_cdf(n,p,k) is the probability to get at most k successes. binomial_icdf(n,p,t) returns the smallest k such that binomial_cdf(n,p,k)>=t", "10,.5,4", 0, CAT_CATEGORY_PROBA},
  {"camembert(list)", 0, "Camembert pie-chart of a 1-d statistical serie.", "[[\"France\",6],[\"Germany\",12],[\"Switzerland\",5]]", 0, CAT_CATEGORY_STATS},
  {"ceiling(x)", 0, "Smallest integer not less than x", "1.2", 0, CAT_CATEGORY_REAL},
  {"cfactor(p)", 0, "Factor over C.", "x^4-1", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"complete_square(expr,[x])", 0, "Complete square / canonical quadratic form. Required: expr. Optional: variable x.", "x^2+6*x+11", "2*x^2-8*x+3,x", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"compose(f,g,[x])", 0, "Composite function f(g(x)). Required: f expression, g expression. Optional: variable x.", "x^2+1,2*x+3,x", 0, CAT_CATEGORY_ALGEBRA},
  {"coeff(p,x,n)", 0, "Coefficient of x^n in polynomial p.", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"compare(expr1,expr2)", 0, "Compare two expressions after exact simplification. Required: expr1, expr2.", "sin(x)^2+cos(x)^2,1", 0, CAT_CATEGORY_ALGEBRA},
  {"comb(n,k)", 0, "Returns nCk", "10,4", 0, CAT_CATEGORY_PROBA},
  {"conj(z)", 0, "Complex conjugate of z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"correlation(l1,l2)", 0, "Correlation of lists l1 and l2", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"covariance(l1,l2)", 0, "Covariance of lists l1 and l2", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"cartesian([x(t),y(t)],t)", 0, "Convert parametric equations to Cartesian form by eliminating t. Required: equations/list and parameter.", "[x=t+1,y=t^2],t", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_PLOT << 8)},
  {"cross(u,v)", 0, "Cross product of vectors u and v.","[1,2,3],[0,1,3]", 0, CAT_CATEGORY_LINALG},
  {"csolve(equation,x)", 0, "Solve equation (or polynomial system) in exact mode over the complex numbers.","x^2+x+1=0", 0, CAT_CATEGORY_SOLVE| (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"degree(p,x)", 0, "Degre of polynomial p in x.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"denom(x)", 0, "Denominator of expression x.", "3/4", 0, CAT_CATEGORY_POLYNOMIAL},
  {"desolve(equation,t,y)", 0, "Exact differential equation solving.", "desolve([y'+y=exp(x),y(0)=1],x,y)", "[y'=[[1,2],[2,1]]*y+[x,x+1],y(0)=[1,2]]", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_CALCULUS << 8)},
  {"de_solve(equation,[bc])", 0, "First-order differential equation solver. Required: equation. Optional: boundary condition.", "y'+y=exp(x),y(0)=1", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_CALCULUS << 8)},
  {"det(A)", 0, "Determinant of matrix A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
  {"diff(f,var,[n,method])", 0, "Derivative. Optional method: auto, chain, product, quotient, logdiff, implicit, param, second.", "sin(x),x", "sin(x)^x,x,method=logdiff", CAT_CATEGORY_CALCULUS},
  {"diff_by(f,var,method)", 0, "Derivative with forced mark-scheme method.", "sin(x)^x,x,logdiff", 0, CAT_CATEGORY_CALCULUS},
  {"implicit_diff(eq,[x,y])", 0, "Implicit derivative dy/dx. Required: equation. Optional: independent x and dependent y.", "x^2+y^2=1,x,y", "sin(2*x)*cot(y)=1,x,y", CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_SOLVE << 8)},
  {"domain(expr,[x])", 0, "Domain of an expression. Required: expr. Optional: variable x.", "sqrt(x-1),x", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_REAL << 8)},
  // {"disque n", "disque ", "Filled circle tangent to the turtle, radius n. Run disque n,theta for a filled arc of circle, theta in degrees, or disque n,theta,segment for a segment of circle.", "#disque 30", "#disque(30,90)", CAT_CATEGORY_LOGO},
  {"dot(a,b)", 0, "Dot product of 2 vectors. Shortcut: *", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_LINALG},
#if 0
  {"draw_arc(x1,y1,rx,ry,theta1,theta2,c)", 0, "Pixelised arc of ellipse.", "100,100,60,80,0,pi,magenta", 0, CAT_CATEGORY_PROGCMD},
  {"draw_circle(x1,y1,r,c)", 0, "Pixelised circle. Option: filled", "100,100,60,cyan+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_line(x1,y1,x2,y2,c)", 0, "Pixelised line.", "100,50,300,200,blue", 0, CAT_CATEGORY_PROGCMD},
  {"draw_pixel(x,y,color)", 0, "Colors pixel x,y. Run draw_pixel() to synchronise screen.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_polygon([[x1,y1],...],c)", 0, "Pixelised polygon.", "[[100,50],[30,20],[60,70]],red+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_rectangle(x,y,w,h,c)", 0, "Rectangle.", "100,50,30,20,red+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_string(s,x,y,c)", 0, "Draw string s at pixel x,y", "\"Bonjour\",80,60", 0, CAT_CATEGORY_PROGCMD},
#endif
  {"egcd(A,B)", 0, "Find polynomials U,V,D such that A*U+B*V=D=gcd(A,B)","x^2+3x+1,x^2-5x-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"eigenvals(A)", 0, "Eigenvalues of matrix  A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
  {"eigenvects(A)", 0, "Eigenvectors of matrix A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
#if 0
  {"erf(x)", 0, "Error function of x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"erfc(x)", 0, "Complementary error function of x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"euler(n)",0,"Euler indicatrix: number of integers < n coprime with n","25",0,CAT_CATEGORY_ARIT},
#endif
  {"evalc(z)", 0, "Write z=x+i*y.", "1/(1+i*sqrt(3))", 0, CAT_CATEGORY_COMPLEXNUM},
  {"exact(x)", 0, "Converts x to a rational. Shortcut shift S-D", "1.2", 0, CAT_CATEGORY_REAL},
  {"expand(expr)", 0, "Expand expression. Required: expr.", "(x+1)^3", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"exp2trig(expr)", 0, "Convert complex exponentials to sin/cos", "exp(i*x)", 0, CAT_CATEGORY_TRIG},
  {"exponential_regression(Xlist,Ylist)", 0, "Exponential regression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
#if 0
  {"exponential_regression_plot(Xlist,Ylist)", 0, "Exponential regression plot.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];exponential_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
#endif
  {"exponentiald(lambda,x)", 0, "Exponential distribution law of  parameter lambda. exponentiald_cdf(lambda,x) probability that \"exponential distribution <=x\" e.g. exponentiald_cdf(2,3). exponentiald_icdf(lambda,t) returns x such that \"exponential distribution <=x\" has probability t, e.g, exponentiald_icdf(2,0.95) ", "5.1,3.4", 0, CAT_CATEGORY_PROBA},
  {"factor(p,[x])", 0, "Factors polynomial p (run ifactor for an integer). Shortcut: p=>*", "x^4-1", "x^6+1,sqrt(3)", CAT_CATEGORY_ALGEBRA| (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"fitconst(equations,vars)", 0, "Find unknown constants from conditions. Required: equations/list, vars/list.", "[a*1+b=3,a*2+b=5],[a,b]", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
  {"float(x)", 0, "Converts x to a floating point value.", "pi", 0, CAT_CATEGORY_REAL},
  {"floor(x)", 0, "Largest integer not greater than x", "pi", 0, CAT_CATEGORY_REAL},
#if 0
  {"fourier_an(f,x,T,n,a)", 0, "Cosine Fourier coefficients of f", "x^2,n","x^2,x,2*pi,n,-pi", CAT_CATEGORY_CALCULUS},
  {"fourier_bn(f,x,T,n,a)", 0, "Sine Fourier coefficients of f", "x^2,n","x^2,x,2*pi,n,-pi", CAT_CATEGORY_CALCULUS},
  {"fourier_cn(f,x,T,n,a)", 0, "Exponential Fourier coefficients of f", "x^2,x,2*pi,n,-pi", 0, CAT_CATEGORY_CALCULUS},
#endif
  {"fsolve(equation,x=a..b)", 0, "Approx equation solving in interval a..b.","cos(x)=x,x=0..1", "cos(x)-x,x=0.0", CAT_CATEGORY_SOLVE},
  // {"function f(x):...", "function f(x) local y;   ffunction:;", "Function definition.", "#function f(x) local y; y:=x^2; return y; ffunction:;", 0, CAT_CATEGORY_PROG},
#if 0
  {"gauss(q)", 0, "Quadratic form reduction", "x^2+x*y+x*z+y^2+z^2,[x,y,z]", 0, CAT_CATEGORY_LINALG},
#endif
  {"gcd(a,b,...)", 0, "Greatest common divisor. See also iegcd and egcd for extended GCD.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
#if 0
  {"gramschmidt(M)", 0, "Gram-Schmidt orthonormalization (line vectors or linearly independant set of vectors)", "[[1,2,3],[4,5,6]]", "[1,1+x],(p,q)->integrate(p*q,x,-1,1)", CAT_CATEGORY_LINALG},
#endif
  {"halftan(expr)", 0, "Convert cos, sin, tan with tan(angle/2).","cos(x)", 0, CAT_CATEGORY_TRIG},
#if 0
  {"hermite(n)", 0, "n-th Hermite polynomial", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"hilbert(n)", 0, "Hilbert matrix of order n.", "4", 0, CAT_CATEGORY_MATRIX},
#endif
  {"histogram(list,min,size)", 0, "Histogram of data in list, classes begin at min of size size.","ranv(100,uniformd,0,1),0,0.1", 0, CAT_CATEGORY_STATS},
#if 0
  {"iabcuv(a,b,c)", 0, "Find 2 integers u,v such that a*u+b*v=c","23,13,15", 0, CAT_CATEGORY_ARIT},
  {"ichinrem([a,m],[b,n])", 0,"Integer chinese remainder of a mod m and b mod n.", "[3,13],[2,7]", 0, CAT_CATEGORY_ARIT},
  {"idivis(n)", 0, "Returns the list of divisors of an integer n.", "10", 0, CAT_CATEGORY_ARIT},
#endif
  {"idn(n)", 0, "Identity matrix of order n", "4", 0, CAT_CATEGORY_MATRIX},
#if 0
  {"iegcd(a,b)", 0, "Find integers u,v,d such that a*u+b*v=d=gcd(a,b)","23,13", 0, CAT_CATEGORY_ARIT},
#endif
  {"ifactor(n)", 0, "Factorization of an integer (not too large!). Shortcut n=>*", 0, 0, CAT_CATEGORY_ARIT},
#if 0
  {"ilaplace(f,s,x)", 0, "Inverse Laplace transform of f", "s/(s^2+1),s,x", 0, CAT_CATEGORY_CALCULUS},
#endif
  {"im(z)", 0, "Imaginary part.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"inf", "inf", "Plus infinity. -inf for minus infinity and infinity for unsigned/complex infinity. Shortcut shift INS.", "oo", 0, CAT_CATEGORY_CALCULUS},
  {"integrate(f,x,[a,b,method,u])", 0, "Integral. Optional method: auto, direct, reverse_chain, sub, parts, di, trig, pf, div, weierstrass, symmetry. Optional u for sub.", "x*sin(x),x,method=parts", "x*cos(x^2),x,method=sub,u=x^2", CAT_CATEGORY_CALCULUS},
  {"integrate_by(f,x,method,[u])", 0, "Integral with forced mark-scheme method.", "x*exp(x),x,parts", "x*cos(x^2),x,sub,u=x^2", CAT_CATEGORY_CALCULUS},
  {"interp(X,Y)", 0, "Lagrange interpolation at points (xi,yi) where X is the list of xi and Y of yi. If interp is passed as 3rd argument, returns the divided differences list.", "[1,2,3,4,5],[0,1,3,4,4]", "[1,2,3,4,5],[0,1,3,4,4],interp", CAT_CATEGORY_POLYNOMIAL},
  {"inv(A)", 0, "Inverse of A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
  {"inverse(f(x))", 0, "Inverse relation/function where possible. Required: expression or function.", "2*x+3", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_SOLVE << 8)},
  {"iquo(a,b)", 0, "Integer quotient of a and b.", "23,13", 0, CAT_CATEGORY_ARIT},
  {"irem(a,b)", 0,"Integer remainder of a and b.", "23,13", 0, CAT_CATEGORY_ARIT},
  {"isprime(n)", 0, "Returns 1 if n is prime, 0 otherwise.", "11", "10", CAT_CATEGORY_ARIT},
#if 0
  {"jordan(A)", 0, "Jordan normal form of matrix A, returns P and D such that P^-1*A*P=D", "[[1,2],[3,4]]", "[[1,1,-1,2,-1],[2,0,1,-4,-1],[0,1,1,1,1],[0,1,2,0,1],[0,0,-3,3,-1]]", CAT_CATEGORY_MATRIX},
  {"laguerre(n,a,x)", 0, "n-ieme Laguerre polynomial (default a=0).", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"laplace(f,x,s)", 0, "Laplace transform of f","sin(x),x,s", 0, CAT_CATEGORY_CALCULUS},
#endif
  {"lcm(a,b,...)", 0, "Least common multiple.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"lcoeff(p,x)", 0, "Leading coefficient of polynomial p in x.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
#if 0
  {"legendre(n)", 0, "n-the Legendre polynomial.", "10", "10,t", CAT_CATEGORY_POLYNOMIAL},
#endif
  {"limit(f,x=a)", 0, "Limit of f at x = a. Add 1 or -1 for unidirectional limits, e.g. limit(sin(x)/x,x=0) or limit(abs(x)/x,x=0,1). Shortcut: SHIFT MIXEDFRAC", "sin(x)/x,x=0", "exp(-1/x),x=0,1", CAT_CATEGORY_CALCULUS},
  {"linear_regression(Xlist,Ylist)", 0, "Linear regression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
#if 0
  {"linear_regression_plot(Xlist,Ylist)", 0, "Linear regression plot.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];linear_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
#endif
  {"linetan(expr,x,x0)", 0, "Tangent to the graph at x=x0.", "sin(x),x,pi/2", 0, CAT_CATEGORY_PLOT},
  {"linsolve([eq1,eq2,..],[x,y,..])", 0, "Linear system solving. May use the output of lu for O(n^2) solving (see example 2).","[x+y=1,x-y=2],[x,y]", "#p,l,u:=lu([[1,2],[3,4]]); linsolve(p,l,u,[5,6])", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_LINALG <<8) | (CAT_CATEGORY_MATRIX << 16)},
  {"logarithmic_regression(Xlist,Ylist)", 0, "Logarithmic egression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
#if 0
  {"logarithmic_regression_plot(Xlist,Ylist)", 0, "Logarithmic regression plot.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];logarithmic_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"lu(A)", 0, "LU decomposition LU of matrix A, P*A=L*U", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
#endif
  {"matpow(A,n)", 0, "Returns matrix A^n", "[[1,2],[3,4]],n","#assume(n>=1);matpow([[0,2],[0,4]],n)",  CAT_CATEGORY_MATRIX},
  {"match(expr,form)", 0, "Match constants by equating coefficients/conditions. Required: expr, target form.", "coeff((a*x+b)-(2*x+3),x,1)=0,a", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
  {"matrix(r,c,func)", 0, "Matrix from a defining function.", "2,3,(j,k)->j^k", 0, CAT_CATEGORY_MATRIX},
  {"mean(l)", 0, "Arithmetic mean of list l", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"median(l)", 0, "Median", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"mult_c_conjugate", 0, "Multiplier par le conjugue complexe.", "1+2*i", 0,  (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"mult_conjugate", 0, "Multiplier par le conjugue (sqrt).", "sqrt(2)-sqrt(3)", 0, CAT_CATEGORY_ALGEBRA},
  {"normald([mu,sigma],x)", 0, "Normal distribution probability density, by default mu=0 and sigma=1. normald_cdf([mu,sigma],x) probability that \"normal distribution <=x\" e.g. normald_cdf(1.96). normald_icdf([mu,sigma],t) returns x such that \"normal distribution <=x\" has probability t, e.g. normald_icdf(0.975) ", "1.2", 0, CAT_CATEGORY_PROBA},
  {"normal_diff(expr,[x])", 0, "Normal derivative dy/dx. Required: expression. Optional: variable x.", "(2*x+ln(x))^3,x", 0, CAT_CATEGORY_CALCULUS},
  {"not(x)", 0, "Logical not.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"numer(x)", 0, "Numerator of x.", "3/4", 0, CAT_CATEGORY_POLYNOMIAL},
#if 0
  {"odesolve(f(t,y),[t,y],[t0,y0],t1)", 0, "Approx. solution of differential equation y'=f(t,y) and y(t0)=y0, value for t=t1 (add curve to get intermediate values of y)", "sin(t*y),[t,y],[0,1],2", "0..pi,(t,v)->{[-v[1],v[0]]},[0,1]", CAT_CATEGORY_SOLVE},
#endif
  {"partfrac(p,x)", 0, "Partial fraction expansion. Shortcut p=>+", "1/(x^4-1)", 0, CAT_CATEGORY_ALGEBRA},
  {"param_area([x(t),y(t)],t,[a,b])", 0, "Parametric area: integrate y*dx/dt with respect to t. Required: x(t), y(t), parameter. Optional: bounds.", "[t^2,t^3],t,0,1", 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
  {"param_diff([x(t),y(t)],t)", 0, "Parametric derivative dy/dx=(dy/dt)/(dx/dt). Required: x(t), y(t), parameter.", "[t^2,t^3],t", 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
  {"param_second_diff([x(t),y(t)],t)", 0, "Second parametric derivative d2y/dx2. Required: x(t), y(t), parameter.", "[t^2,t^3],t", 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
#if 0
  {"plot(expr,x)", 0, "Plot an expression. For example plot(sin(x)), plot(ln(x),x.0,5)", "ln(x),x,0,5", "1/x,x=1..5,xstep=1", CAT_CATEGORY_PLOT},
#ifdef RELEASE
  {"plotarea(expr,x=a..b,[n,meth])", 0, "Area under curve with specified quadrature.", "1/x,x=1..3,2,trapezoid", 0, CAT_CATEGORY_PLOT},
#endif
  {"plotcontour(expr,[x=xm..xM,y=ym..yM],levels)", 0, "Levels of expr.", "x^2+2y^2,[x=-2..2,y=-2..2],[1,2]", 0, CAT_CATEGORY_PLOT},
  {"plotfield(f(t,y),[t=tmin..tmax,y=ymin..ymax])", 0, "Plot field of differential equation y'=f(t,y), an optionnally one solution by adding plotode=[t0,y0]", "sin(t*y),[t=-3..3,y=-3..3],plotode=[0,1]", 0, CAT_CATEGORY_PLOT},
  {"plotlist(list)", 0, "Plot a list", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_PLOT},
  {"plotode(f(t,y),[t=tmin..tmax,y],[t0,y0])", 0, "Plot solution of differential equation y'=f(t,y), y(t0)=y0.", "sin(t*y),[t=-3..3,y],[0,1]", 0, CAT_CATEGORY_PLOT},
  {"plotparam([x,y],t)", 0, "Parametric plot. For example plotparam([sin(3t),cos(2t)],t,0,pi) or plotparam(exp(i*t),t,0,pi)", "[sin(3t),cos(2t)],t,0,pi", "[t^2,t^3],t=-1..1,tstep=0.1", CAT_CATEGORY_PLOT},
  {"plotpolar(r,theta)", 0, "Polar plot.","cos(3*x),x,0,pi", "1/(1+cos(x)),x=0..pi,xstep=0.05", CAT_CATEGORY_PLOT},
  {"poly(expr,[x])", 0, "Polynomial factor/solve helper. Required: expression. Optional: variable x.", "x^2-5*x+6,x", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"plotseq(f(x),x=[u0,m,M],n)", 0, "Plot f(x) on [m,M] and n terms of the sequence defined by u_{n+1}=f(u_n) and u0.","sqrt(2+x),x=[6,0,7],5", 0, CAT_CATEGORY_PLOT},
#endif
#if 0
  {"polygonscatterplot(Xlist,Ylist)", 0, "Plot points and polygonal line.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
#endif
  {"polynomial_regression(Xlist,Ylist,n)", 0, "Polynomial regression, degree <= n.", "[1,2,3,4,5],[0,1,3,4,4],2", 0, CAT_CATEGORY_STATS},
#if 0
  {"polynomial_regression_plot(Xlist,Ylist,n)", 0, "Polynomial regression plot, degree <= n.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];polynomial_regression_plot(X,Y,2);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
#endif
  //{"pour", "pour j de 1 jusque  faire  fpour;", "For loop.","#pour j de 1 jusque 10 faire print(j,j^2); fpour;", 0, CAT_CATEGORY_PROG},
  {"power_regression(Xlist,Ylist,n)", 0, "Power regression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
#if 0
  {"power_regression_plot(Xlist,Ylist,n)", 0, "Power regression graph", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];power_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
#endif
#if 0
  {"powmod(a,n,p)", 0, "Returns a^n mod p.","123,456,789", 0, CAT_CATEGORY_ARIT},
#endif
  {"proot(p)", 0, "Returns real and complex roots, of polynomial p. Exemple proot([1,2.1,3,4.2]) or proot(x^3+2.1*x^2+3x+4.2)", "x^3+2.1*x^2+3x+4.2", 0, CAT_CATEGORY_POLYNOMIAL},
#if 0
  {"q2a(expr,[vars])", 0, "Matrix of a quadratic form", "x^2+3*x*y","x^2+3*x*y,[x,y]", CAT_CATEGORY_LINALG},
  {"qr(A)", 0, "A=Q*R factorization with Q orthogonal and R upper triangular", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
#endif
  {"quartile1(l)", 0, "1st quartile", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"quartile3(l)", 0, "3rd quartile", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"quo(p,q,x)", 0, "Quotient of synthetic division of polynomials p and q (variable x).", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"ratnormal(x)", 0, "Puts everything over a common denominator.", 0, 0, CAT_CATEGORY_ALGEBRA},
  {"re(z)", 0, "Real part.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"range(expr,[x])", 0, "Range/image of an expression where possible. Required: expr. Optional: variable x.", "x^2+2*x+3,x", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_REAL << 8)},
  {"rem(p,q,x)", 0, "Remainder of synthetic division of polynomials p and q (variable x)", 0, 0, CAT_CATEGORY_POLYNOMIAL},
#ifdef RELEASE
  {"residue(f(z),z,z0)", 0, "Residue of an expression at z0.", "1/(x^2+1),x,i", 0, CAT_CATEGORY_COMPLEXNUM},
#endif
#if 0
  {"resultant(p,q,x)", 0, "Resultant in x of polynomials p and q.", "#P:=x^3+p*x+q;resultant(P,P',x);", 0, CAT_CATEGORY_POLYNOMIAL},
  {"revert(p[,x])", 0, "Revert Taylor series","x+x^2+x^4", 0, CAT_CATEGORY_CALCULUS},
#endif
  {"rewrite(expr,target)", 0, "Rewrite expression around a target form/linear shift. Required: expr. Optional target/variable.", "x^2+6*x+11", "2*x^2-8*x+3,x", CAT_CATEGORY_ALGEBRA},
  {"rref(A)", 0, "Row reduction to echelon form", "[[1,2,3],[4,5,6]]", 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
  {"rsolve(equation,u(n),[init])", 0, "Solve a recurrence relation.","u(n+1)=2*u(n)+3,u(n),u(0)=1", "([u(n+1)=3*v(n)+u(n),v(n+1)=v(n)+u(n)],[u(n),v(n)],[u(0)=1,v(0)=2]", CAT_CATEGORY_SOLVE},
  {"scatterplot(Xlist,Ylist)", 0, "Draws points", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"second_diff(expr,[x])", 0, "Second derivative d2y/dx2. Required: expression. Optional: variable x.", "x^4,x", 0, CAT_CATEGORY_CALCULUS},
  //{"si", "si  alors  sinon  fsi;", "Test.", "#f(x):=si x>0 alors x; sinon -x; fsi;// valeur absolue", 0, CAT_CATEGORY_PROG},
  {"sign(x)", 0, "Returns -1 if x is negative, 0 if x is zero and 1 if x is positive.", 0, 0, CAT_CATEGORY_REAL},
  {"simplify(expr,[method])", 0, "Simplify/rewrite. Method: auto, expand, factor, collect, complete_square, rationalise, canonical, target.", "sin(3x)/sin(x),method=canonical", 0, CAT_CATEGORY_ALGEBRA},
  {"solve(equation,x,[method])", 0, "Solve. Method: auto, linear, factor, quad_formula, complete_square, substitution, clear_denoms, log_exp, numeric, interval.", "x^2-x-1=0,x,method=factor", "[x^2-y^2=0,x^2-z^2=0],[x,y,z]", CAT_CATEGORY_SOLVE},
  {"solve_by(equation,x,method)", 0, "Solve with forced mark-scheme method.", "x^2-5*x+6=0,x,factor", 0, CAT_CATEGORY_SOLVE},
  {"solve_trig(eq,[var,lo,hi,max,method])", 0, "Trig eq solve. Opt method: auto, general, bounded, cast, identity, rform, square_then_check.", "2*sin(x)+1=0", "2*sin(x)+1=0,x,0,2*pi,6,method=bounded", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_TRIG << 8)},
  {"solve_trig_by(eq,var,method)", 0, "Trig solve with forced mark-scheme method.", "2*sin(x)+1=0,x,bounded", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_TRIG << 8)},
  {"stddev(l)", 0, "Standard deviation of list l", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"subst(a,b=c)", 0, "Substitutes b for c in a. Shortcut a(b=c).", "x^2,x=3", 0, CAT_CATEGORY_ALGEBRA},
  {"suvat(equations,vars)", 0, "SUVAT by simultaneous equations. Required: equations/list. Optional: variable(s) to solve.", "[v=u+a*t,s=u*t+1/2*a*t^2],[s,u,v,a,t]", 0, CAT_CATEGORY_SOLVE},
  {"sum(f,k,m,M)", 0, "Summation of expression f for k from m to M. Exemple sum(k^2,k,1,n)=>*. Shortcut ALPHA F3", "k,k,1,n", 0, CAT_CATEGORY_CALCULUS},
#if 0
  {"svd(A)", 0, "Singular Value Decomposition, returns U orthogonal, S vector of singular values, Q orthogonal such that A=U*diag(S)*tran(Q).", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
#endif
  {"tabvar(f,[x=a..b])", 0, "Table of variations of expression f, optional arguments variable x in interval a..b", "sqrt(x^2+x+1)",  "[cos(t),sin(3t)],t", CAT_CATEGORY_CALCULUS},
  //{"tantque", "tantque  faire   ftantque;", "While loop.", "#j:=13; tantque j!=1 faire j:=when(even(j),j/2,3j+1); print(j); ftantque;", 0, CAT_CATEGORY_PROG},
  {"taylor(f,x=a,n,[polynom])", 0, "Taylor expansion of f of x at a order n, add parameter polynom to remove remainder term.","sin(x),x=0,5", "sin(x),x=0,5,polynom", CAT_CATEGORY_CALCULUS},
  {"tangent_line(expr,x,x0)", 0, "Tangent line to y=expr at x=x0. Required: expression, variable, point.", "x^2,x,3", "sin(x),x,pi/2", CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_PLOT << 8)},
#if 0
  {"tchebyshev1(n)", 0, "Tchebyshev polynomial 1st kind: cos(n*x)=T_n(cos(x))", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"tchebyshev2(n)", 0, "Tchebyshev polynomial 2nd kind: sin((n+1)*x)=sin(x)*U_n(cos(x))", "10", 0, CAT_CATEGORY_POLYNOMIAL},
#endif
  {"tcollect(expr)", 0, "Linearize and collect trig functions.","sin(x)+cos(x)", 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, "Expand trigonometric, exp and ln functions.","sin(3x)", 0, CAT_CATEGORY_TRIG},
  {"tlin(expr)", 0, "Trigonometric linearization of expr.","sin(x)^3", 0, CAT_CATEGORY_TRIG},
  {"transform(expr,[form])", 0, "Transform/rearrange expression with KhiCAS normal/simplify/factor forms. Required: expr. Optional: target form.", "(x+1)^2-(x^2+2*x+1)", 0, CAT_CATEGORY_ALGEBRA},
  {"xform(expr,target)", 0, "Old algebra transform alias. Required: source expression/equation and target form.", "x^2+2*x+1,(x+1)^2", 0, CAT_CATEGORY_ALGEBRA},
  {"tran(A)", 0, "Transposes matrix A. Transconjugate command is trn(A) or A^*.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"trig2exp(expr)", 0, "Convert complex exponentials to trigonometric functions","cos(x)^3", 0, CAT_CATEGORY_TRIG},
  {"trig_prove(lhs,rhs)", 0, "Prove trig identity by simplifying lhs-rhs. Required: lhs,rhs.", "sin(x)^2+cos(x)^2,1", 0, CAT_CATEGORY_TRIG},
  {"trig_rewrite(expr,target)", 0, "Rewrite trig expression using requested target terms. Required: expr. Optional: target.", "1/cos(x),sec(x)", 0, CAT_CATEGORY_TRIG},
  {"trig_transform(expr,target)", 0, "Transform trig expression/equation to target form. Required: expr,target.", "1/cos(x),sec(x)", 0, CAT_CATEGORY_TRIG},
  {"trigcos(expr)", 0, "Convert sin^2 and tan^2 to cos^2.","sin(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigsin(expr)", 0, "Convert cos^2 and tan^2 to sin^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigtan(expr)", 0, "Convert cos^2 and sin^2 to tan^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"uniformd(a,b,x)", "uniformd", "uniform law on [a,b] of density 1/(b-a)", 0, 0, CAT_CATEGORY_PROBA},
};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="CasioCAS Help";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="F1-F6 menus; F4 cat; F3 expr.\n=>+ pf; =>* factor; =>=> solve.\nEXE run; EXIT back.";
const char apropos_string[]="CasioCAS. KhiCAS engine. GPLv2.";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);

static bool catalog_hidden_category(int category){
  switch (category) {
  case CAT_CATEGORY_PROG:
  case CAT_CATEGORY_PROGCMD:
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
  const char *hidden[]={
    "debug","python","read","write","purge","append","map","seq","sort","quote",
    "rand","ranm","ranv","draw_","plotfield","plotode","plotcontour","plotseq",
    "circle","line","point","polygon","segment","avance","recule","saute",
    "tourne","crayon","tortue","rgb","display","gl_","axes","time","input",
    "charpoly","hilbert","lu","qr","svd","jordan","curl","a2q","q2a","cond",
    "erf","erfc","hermite","laguerre","legendre","tchebyshev","powmod",
    "nextprime","ichinrem","iabcuv","idivis","iegcd","bool_","prove_bool",
    "nand","nor",0
  };
  for (int i=0;hidden[i];++i)
    if (startswith_catalog(name,hidden[i]))
      return true;
  return false;
}

static ustl::string catalog_param_hint(const char *name){
  if (startswith_catalog(name,"integrate"))
    return "Req:f,x. Opt:a,b,method,u. method=auto default.";
  if (startswith_catalog(name,"diff"))
    return "Req:f,x. Opt:n,method. method=auto default.";
  if (startswith_catalog(name,"solve"))
    return "Req:eq,x. Opt:method/bounds. method=auto default.";
  return "Req/Opt: see syntax. method=auto if shown.";
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
  ADD_CAT(CAT_CATEGORY_ALGEBRA,"Algebra");
  ADD_CAT(CAT_CATEGORY_LINALG,"Linear algebra");
  ADD_CAT(CAT_CATEGORY_CALCULUS,"Calculus");
  ADD_CAT(CAT_CATEGORY_ARIT,"Arithmetic");
  ADD_CAT(CAT_CATEGORY_COMPLEXNUM,"Complexes");
  ADD_CAT(CAT_CATEGORY_POLYNOMIAL,"Polynomials");
  ADD_CAT(CAT_CATEGORY_PROBA,"Probabilities");
  ADD_CAT(CAT_CATEGORY_REAL,"Reals");
  ADD_CAT(CAT_CATEGORY_SOLVE,"Solve");
  ADD_CAT(CAT_CATEGORY_STATS,"Statistics");
  ADD_CAT(CAT_CATEGORY_TRIG,"Trigonometry");
  ADD_CAT(CAT_CATEGORY_MATRIX,"Matrices");
#undef ADD_CAT
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=catcount;
  menu.scrollout=1;
  menu.title = (char*)"Function Catalog";
  
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
  int allcmds=builtin_lexer_functions_end()-builtin_lexer_functions_begin();
  int allopts=lexer_tab_int_values_end-lexer_tab_int_values_begin;
  bool isall=category==CAT_CATEGORY_ALL;
  bool isopt=category==CAT_CATEGORY_OPTIONS;
  int nitems = isall? allcmds:(isopt?allopts:CAT_COMPLETE_COUNT);
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,i=0,menusel=-1,cmdl=cmdname?strlen(cmdname):0;
  gen g;
  while(cur<nitems) {
    if (isall || isopt) {
      const char * text=isall?(builtin_lexer_functions_begin()+cur)->first:(lexer_tab_int_values_begin+cur)->keyword;
      if (catalog_hidden_name(text)){
	++cur;
	continue;
      }
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = TEXT_COLOR_BLACK;
      if (menusel<0 && cmdname && !strncmp(cmdname,text,cmdl))
	menusel=curmi;
      menuitems[curmi].text = text;
      menuitems[curmi].isfolder = allcmds; // assumes allcmds>allopts
      menuitems[curmi].token=isall?find_or_make_symbol(text,g,0,false,contextptr):((lexer_tab_int_values_begin+cur)->subtype+(lexer_tab_int_values_begin+cur)->return_value*256);
      for (;i<CAT_COMPLETE_COUNT;++i){
	const char * catname=completeCat[i].name;
	if (catalog_hidden_name(catname))
	  continue;
	int tmp=strcmp(catname,text);
	if (tmp>=0){
	  size_t st=strlen(text),j=tmp?0:st;
	  for (;j<st;++j){
	    if (catname[j]!=text[j])
	      break;
	  }
	  if (j==st && (!isalphanum(catname[j]))){
	    menuitems[curmi].isfolder = i;
	    ++i;
	  }
	  break;
	}
      }
      // compare text with completeCat
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
      if ( (cat & 0xff) == category ||
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
    drawFkeyLabels(0x03FC, 0x03fc, 0, 0, 0, 0x03FD);
    int fkeyw=LCD_WIDTH_PX/6;
    drawRectangle(fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXAMPL1",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(2*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(2*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXAMPL2",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return 0;
    }
    int index=menuitems[menu.selection-1].isfolder;
    bool hascat=index>=0 && index<CAT_COMPLETE_COUNT && !catalog_hidden_name(completeCat[index].name);
    if(sres == KEY_CTRL_F6) {
      char * example=hascat?completeCat[index].example:0;
      textArea text;
      text.title = (char*)"Help";
      text.editable=false;
      text.clipline=-1;
      text.allowF1=true;
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement> (3);
      elem[0].s = hascat?completeCat[index].name:menuitems[menu.selection-1].text;
      elem[0].color = COLOR_BLUE;
      elem[1].newLine = 1;
      elem[1].lineSpacing = 3;
      if (hascat){
	if (completeCat[index].desc && completeCat[index].desc[0])
	  elem[1].s = completeCat[index].desc;
	else
	  elem[1].s = "CAS command.";
	elem[1].s += "\n";
	elem[1].s += catalog_param_hint(completeCat[index].name);
      }
      else {
	elem[1].s="KhiCAS cmd.\nReq/Opt: see syntax.";
      }
      ustl::string ex("Ex F2: ");
      elem[2].newLine = 1;
      elem[2].lineSpacing = 3;
      if (example && example[0]=='#')
	ex += example+1;
      else if (hascat){
	ex += insert_string(index);
	if (example)
	  ex += example;
	ex += ")";
      }
      else {
	ex += menuitems[menu.selection-1].text;
      }
      elem[2].s = ex;
      sres=doTextArea(&text);
    }
    if (sres == KEY_CTRL_F2 || sres==KEY_CTRL_F3) {
      reset_alpha();
      if (hascat && completeCat[index].example){
	ustl::string s(insert_string(index));
	char * example=0;
	if (sres==KEY_CTRL_F2)
	  example=completeCat[index].example;
	else
	  example=completeCat[index].example2;
	if (example){
	  if (example[0]=='#')
	    s=example+1;
	  else {
	    s += example;
	    s += ")";
	  }
	}
	strcpy(insertText, s.c_str());
	return 1;
      }
      if (isopt){
	int token=menuitems[menu.selection-1].token;
	if (token==_INT_PLOT+T_NUMBER*256 || token==_INT_COLOR+T_NUMBER*256)
	  strcpy(insertText,"display=");
	else
	  *insertText=0;
	strcat(insertText,menuitems[menu.selection-1].text);
	return 1;
      }
      sres=KEY_CTRL_F1;
    }
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {
      reset_alpha();
      strcpy(insertText,hascat?insert_string(index).c_str():menuitems[menu.selection-1].text);
      return 1;
    }
  }
  return 0;
}

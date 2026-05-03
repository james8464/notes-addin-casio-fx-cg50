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
  {"!", "!", "Logical not (prefix) or factorial of n (suffix).", "#7!", "~!b", CAT_CATEGORY_PROGCMD},
  {"#", "#", "Python comment, for Xcas comment type //. Shortcut ALPHA F2", 0, 0, CAT_CATEGORY_PROG},
  {"%", "%", "a % b means a modulo b", 0, 0, CAT_CATEGORY_ARIT | (CAT_CATEGORY_PROGCMD << 8)},
  {"&", "&", "Logical and or +", "#1&2", 0, CAT_CATEGORY_PROGCMD},
  {":=", ":=", "Set variable value. Shortcut SHIFT F1", "#a:=3", 0, CAT_CATEGORY_PROGCMD|(CAT_CATEGORY_SOFUS<<8)},
  {"<", "<", "Shortcut SHIFT F2", 0, 0, CAT_CATEGORY_PROGCMD},
  {"=>", "=>", "Store value in variable or conversion (touche ->). For example 5=>a or x^4-1=>* or (x+1)^2=>+ or sin(x)^2=>cos.", "#5=>a", 0, CAT_CATEGORY_PROGCMD},
  {">", ">", "Shortcut F2.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"\\", "\\", "\\ char", 0, 0, CAT_CATEGORY_PROGCMD},
  {"_", "_", "_ char, shortcut (-).", 0, 0, CAT_CATEGORY_PROGCMD},
  {"_cdf", "_cdf", "Suffix to get a cumulative distribution function. Type F2 for inverse cumulative distribution function _icdf suffix.", "#_icdf", 0, CAT_CATEGORY_PROBA},
  {"a and b", " and ", "Logical and", 0, 0, CAT_CATEGORY_PROGCMD},
  {"a or b", " or ", "Logical or", 0, 0, CAT_CATEGORY_PROGCMD},
  {"a2q(A,[vars])", 0, "Matrix to quadratic form", "[[1,2],[2,3]]","[[1,2],[2,3]],[x,y]", CAT_CATEGORY_LINALG},
  {"abcuv(a,b,c)", 0, "Find 2 polynomial u,v such that a*u+b*v=c","x+1,x^2-2,x", 0, CAT_CATEGORY_POLYNOMIAL},
  {"abs(x)", 0, "Absolute value or norm of x x", "-3", "[1,2,3]", CAT_CATEGORY_COMPLEXNUM | (CAT_CATEGORY_REAL<<8)},
  {"append", 0, "Adds an element at the end of a list","#l.append(x)", 0, CAT_CATEGORY_LIST},
  {"approx(x)", 0, "Approx. value x. Shortcut S-D", "pi", 0, CAT_CATEGORY_REAL},
  {"arg(z)", 0, "Angle of complex z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"asc(string)", 0, "List of ASCII codes os a string", "\"Hello\"", 0, CAT_CATEGORY_ARIT},
  {"assume(hyp)", 0, "Assumption on variable.", "x>1", "x>-1 and x<1", CAT_CATEGORY_PROGCMD|(CAT_CATEGORY_SOFUS<<8)},
  {"avance n", "avance ", "Turtle forward n steps, default n=10", "#avance 30", 0, CAT_CATEGORY_LOGO},
  {"axes", "axes", "Axes visible or not axes=1 or 0", "#axes=0", 0, CAT_CATEGORY_PROGCMD << 8},
  {"baisse_crayon ", "baisse_crayon ", "Turtle moves with the pen writing.", 0, 0, CAT_CATEGORY_LOGO},
  {"barplot(list)", 0, "Bar plot of 1-d statistic serie data in list.", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"binomial(n,p,k)", 0, "binomial(n,p,k) probability to get k success with n trials where p is the probability of success of 1 trial. binomial_cdf(n,p,k) is the probability to get at most k successes. binomial_icdf(n,p,t) returns the smallest k such that binomial_cdf(n,p,k)>=t", "10,.5,4", 0, CAT_CATEGORY_PROBA},
  {"bitxor", "bitxor", "Exclusive or", "#bitxor(1,2)", 0, CAT_CATEGORY_PROGCMD},
  {"black", "black", "Display option", "#display=black", 0, CAT_CATEGORY_PROGCMD},
  {"blue", "blue", "Display option", "#display=blue", 0, CAT_CATEGORY_PROGCMD},
  {"bool_simplify(expr)", "simplify(", "Simplify a Boolean/logical expression. Required: expr.", "not(A and B) or A", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_PROGCMD << 8)},
  {"camembert(list)", 0, "Camembert pie-chart of a 1-d statistical serie.", "[[\"France\",6],[\"Germany\",12],[\"Switzerland\",5]]", 0, CAT_CATEGORY_STATS},
  {"cache_tortue ", "cache_tortue ", "Hide turtle (once the picture has been drawn).", 0, 0, CAT_CATEGORY_LOGO},
  {"ceiling(x)", 0, "Smallest integer not less than x", "1.2", 0, CAT_CATEGORY_REAL},
  {"cfactor(p)", 0, "Factorization over C.", "x^4-1", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"char(liste)", 0, "Converts a list of ASCII codes to a string.", "[97,98,99]", 0, CAT_CATEGORY_ARIT},
  {"charpoly(M,x)", 0, "Characteristic polynomial of matrix M in variable x.", "[[1,2],[3,4]],x", 0, CAT_CATEGORY_MATRIX},
  {"complete_square(expr,[x])", "canonical_form(", "Complete square / canonical quadratic form. Required: expr. Optional: variable x.", "x^2+6*x+11", "2*x^2-8*x+3,x", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"circle(center,radius)", 0, "Circle", "2+i,3", "1-i,1+i", CAT_CATEGORY_PROGCMD},
  {"clearscreen()", "clearscreen()", "Clear screen.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"coeff(p,x,n)", 0, "Coefficient of x^n in polynomial p.", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"compare(expr1,expr2)", 0, "Compare two expressions after exact simplification. Required: expr1, expr2.", "sin(x)^2+cos(x)^2,1", 0, CAT_CATEGORY_ALGEBRA},
  {"comb(n,k)", 0, "Returns nCk", "10,4", 0, CAT_CATEGORY_PROBA},
  {"cond(A,[1,2,inf])", 0, "Nombre de condition d'une matrice par rapport a la norme specifiee (par defaut 1)", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"conj(z)", 0, "Complex conjugate of z.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"correlation(l1,l2)", 0, "Correlation of lists l1 and l2", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"covariance(l1,l2)", 0, "Covariance of lists l1 and l2", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"cpartfrac(p,x)", 0, "Partial fraction decomposition over C.", "1/(x^4-1)", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"cartesian([x(t),y(t)],t)", "eliminate(", "Convert parametric equations to Cartesian form by eliminating t. Required: equations/list and parameter.", "[x=t+1,y=t^2],t", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_PLOT << 8)},
  {"crayon ", "crayon ", "Turtle drawing color", "#crayon red", 0, CAT_CATEGORY_LOGO},
  {"cross(u,v)", 0, "Cross product of vectors u and v.","[1,2,3],[0,1,3]", 0, CAT_CATEGORY_LINALG},
  {"csolve(equation,x)", 0, "Solve equation (or polynomial system) in exact mode over the complex numbers.","x^2+x+1=0", 0, CAT_CATEGORY_SOLVE| (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"curl(u,vars)", 0, "Curl of vector u.", "[2*x*y,x*z,y*z],[x,y,z]", 0, CAT_CATEGORY_LINALG},
  {"cyan", "cyan", "Display option", "#display=cyan", 0, CAT_CATEGORY_PROGCMD},
  {"debug(f(args))", 0, "Runs user function f in step by step mode.", 0, 0, CAT_CATEGORY_PROG},
  {"degree(p,x)", 0, "Degre of polynomial p in x.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"denom(x)", 0, "Denominator of expression x.", "3/4", 0, CAT_CATEGORY_POLYNOMIAL},
  {"desolve(equation,t,y)", 0, "Exact differential equation solving.", "desolve([y'+y=exp(x),y(0)=1],x,y)", "[y'=[[1,2],[2,1]]*y+[x,x+1],y(0)=[1,2]]", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_CALCULUS << 8)},
  {"det(A)", 0, "Determinant of matrix A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
  {"diff(f,var,[n])", 0, "Derivative of expression f with respect to var (order n, n=1 by default), for example diff(sin(x),x) or diff(x^3,x,2). For derivation with respect to x, run f' (shortcut F3). For the gradient of f, var is the list of variables.", "sin(x),x", "sin(x^2),x,3", CAT_CATEGORY_CALCULUS},
  {"domain(expr,[x])", 0, "Domain of an expression. Required: expr. Optional: variable x.", "sqrt(x-1),x", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_REAL << 8)},
  {"display", "display", "Display option", "#display=red", 0, CAT_CATEGORY_PROGCMD},
  // {"disque n", "disque ", "Filled circle tangent to the turtle, radius n. Run disque n,theta for a filled arc of circle, theta in degrees, or disque n,theta,segment for a segment of circle.", "#disque 30", "#disque(30,90)", CAT_CATEGORY_LOGO},
  {"dot(a,b)", 0, "Dot product of 2 vectors. Shortcut: *", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_LINALG},
  {"draw_arc(x1,y1,rx,ry,theta1,theta2,c)", 0, "Pixelised arc of ellipse.", "100,100,60,80,0,pi,magenta", 0, CAT_CATEGORY_PROGCMD},
  {"draw_circle(x1,y1,r,c)", 0, "Pixelised circle. Option: filled", "100,100,60,cyan+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_line(x1,y1,x2,y2,c)", 0, "Pixelised line.", "100,50,300,200,blue", 0, CAT_CATEGORY_PROGCMD},
  {"draw_pixel(x,y,color)", 0, "Colors pixel x,y. Run draw_pixel() to synchronise screen.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"draw_polygon([[x1,y1],...],c)", 0, "Pixelised polygon.", "[[100,50],[30,20],[60,70]],red+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_rectangle(x,y,w,h,c)", 0, "Rectangle.", "100,50,30,20,red+filled", 0, CAT_CATEGORY_PROGCMD},
  {"draw_string(s,x,y,c)", 0, "Draw string s at pixel x,y", "\"Bonjour\",80,60", 0, CAT_CATEGORY_PROGCMD},
#ifndef TURTLETAB
  {"ecris ", "ecris ", "Write at turtle position", "#ecris \"hello\"", 0, CAT_CATEGORY_LOGO},
#endif
  {"efface", "efface", "Reset turtle", 0, 0, CAT_CATEGORY_LOGO},
  {"egcd(A,B)", 0, "Find polynomials U,V,D such that A*U+B*V=D=gcd(A,B)","x^2+3x+1,x^2-5x-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"elif test", "elif ", "Test cascade", 0, 0, CAT_CATEGORY_PROG},
  {"eigenvals(A)", 0, "Eigenvalues of matrix  A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
  {"eigenvects(A)", 0, "Eigenvectors of matrix A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX |  (CAT_CATEGORY_LINALG<<8)},
  {"erf(x)", 0, "Error function of x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"erfc(x)", 0, "Complementary error function of x.", "1.2", 0, CAT_CATEGORY_PROBA},
  {"euler(n)",0,"Euler indicatrix: number of integers < n coprime with n","25",0,CAT_CATEGORY_ARIT},
  {"eval(f)", 0, "Evals f.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"evalc(z)", 0, "Write z=x+i*y.", "1/(1+i*sqrt(3))", 0, CAT_CATEGORY_COMPLEXNUM},
  {"exact(x)", 0, "Converts x to a rational. Shortcut shift S-D", "1.2", 0, CAT_CATEGORY_REAL},
  {"exp2trig(expr)", 0, "Convert complex exponentials to sin/cos", "exp(i*x)", 0, CAT_CATEGORY_TRIG},
  {"exponential_regression(Xlist,Ylist)", 0, "Exponential regression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"exponential_regression_plot(Xlist,Ylist)", 0, "Exponential regression plot.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];exponential_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  {"exponentiald(lambda,x)", 0, "Exponential distribution law of  parameter lambda. exponentiald_cdf(lambda,x) probability that \"exponential distribution <=x\" e.g. exponentiald_cdf(2,3). exponentiald_icdf(lambda,t) returns x such that \"exponential distribution <=x\" has probability t, e.g, exponentiald_icdf(2,0.95) ", "5.1,3.4", 0, CAT_CATEGORY_PROBA},
  {"extend", 0, "Merge 2 lists. Note that + does not merge lists, it adds vectors","#l1.extend(l2)", 0, CAT_CATEGORY_LIST},
  {"factor(p,[x])", 0, "Factors polynomial p (run ifactor for an integer). Shortcut: p=>*", "x^4-1", "x^6+1,sqrt(3)", CAT_CATEGORY_ALGEBRA| (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"fitconst(equations,vars)", "solve(", "Find unknown constants from conditions. Required: equations/list, vars/list.", "[a*1+b=3,a*2+b=5],[a,b]", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
  {"filled", "filled", "Display option", 0, 0, CAT_CATEGORY_PROGCMD},
  {"float(x)", 0, "Converts x to a floating point value.", "pi", 0, CAT_CATEGORY_REAL},
  {"floor(x)", 0, "Largest integer not greater than x", "pi", 0, CAT_CATEGORY_REAL},
  {"fourier_an(f,x,T,n,a)", 0, "Cosine Fourier coefficients of f", "x^2,n","x^2,x,2*pi,n,-pi", CAT_CATEGORY_CALCULUS},
  {"fourier_bn(f,x,T,n,a)", 0, "Sine Fourier coefficients of f", "x^2,n","x^2,x,2*pi,n,-pi", CAT_CATEGORY_CALCULUS},
  {"fourier_cn(f,x,T,n,a)", 0, "Exponential Fourier coefficients of f", "x^2,x,2*pi,n,-pi", 0, CAT_CATEGORY_CALCULUS},
  {"from math/... import *", "from math import *", "Access to math or to random functions ([random]) or turtle with English commandnames [turtle]. Math import is not required in KhiCAS", "#from random import *", "#from turtle import *", CAT_CATEGORY_PROG},
  {"fsolve(equation,x=a..b)", 0, "Approx equation solving in interval a..b.","cos(x)=x,x=0..1", "cos(x)-x,x=0.0", CAT_CATEGORY_SOLVE},
  // {"function f(x):...", "function f(x) local y;   ffunction:;", "Function definition.", "#function f(x) local y; y:=x^2; return y; ffunction:;", 0, CAT_CATEGORY_PROG},
  {"gauss(q)", 0, "Quadratic form reduction", "x^2+x*y+x*z+y^2+z^2,[x,y,z]", 0, CAT_CATEGORY_LINALG},
  {"gcd(a,b,...)", 0, "Greatest common divisor. See also iegcd and egcd for extended GCD.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"gl_x", "gl_x", "Display settings X gl_x=xmin..xmax", "#gl_x=0..2", 0, CAT_CATEGORY_PROGCMD},
  {"gl_y", "gl_y", "Display settings Y gl_y=ymin..ymax", "#gl_y=-1..1", 0, CAT_CATEGORY_PROGCMD},
  {"gramschmidt(M)", 0, "Gram-Schmidt orthonormalization (line vectors or linearly independant set of vectors)", "[[1,2,3],[4,5,6]]", "[1,1+x],(p,q)->integrate(p*q,x,-1,1)", CAT_CATEGORY_LINALG},
  {"green", "green", "Display option", "#display=green", 0, CAT_CATEGORY_PROGCMD},
  {"halftan(expr)", 0, "Convert cos, sin, tan with tan(angle/2).","cos(x)", 0, CAT_CATEGORY_TRIG},
  {"hermite(n)", 0, "n-th Hermite polynomial", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"hilbert(n)", 0, "Hilbert matrix of order n.", "4", 0, CAT_CATEGORY_MATRIX},
  {"histogram(list,min,size)", 0, "Histogram of data in list, classes begin at min of size size.","ranv(100,uniformd,0,1),0,0.1", 0, CAT_CATEGORY_STATS},
  {"iabcuv(a,b,c)", 0, "Find 2 integers u,v such that a*u+b*v=c","23,13,15", 0, CAT_CATEGORY_ARIT},
  {"ichinrem([a,m],[b,n])", 0,"Integer chinese remainder of a mod m and b mod n.", "[3,13],[2,7]", 0, CAT_CATEGORY_ARIT},
  {"idivis(n)", 0, "Returns the list of divisors of an integer n.", "10", 0, CAT_CATEGORY_ARIT},
  {"idn(n)", 0, "Identity matrix of order n", "4", 0, CAT_CATEGORY_MATRIX},
  {"iegcd(a,b)", 0, "Find integers u,v,d such that a*u+b*v=d=gcd(a,b)","23,13", 0, CAT_CATEGORY_ARIT},
  {"ifactor(n)", 0, "Factorization of an integer (not too large!). Shortcut n=>*", 0, 0, CAT_CATEGORY_ARIT},
  {"ilaplace(f,s,x)", 0, "Inverse Laplace transform of f", "s/(s^2+1),s,x", 0, CAT_CATEGORY_CALCULUS},
  {"im(z)", 0, "Imaginary part.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"inf", "inf", "Plus infinity. -inf for minus infinity and infinity for unsigned/complex infinity. Shortcut shift INS.", "oo", 0, CAT_CATEGORY_CALCULUS},
  {"input()", "input()", "Read a string from keyboard", 0, 0, CAT_CATEGORY_PROG},
  {"integrate(f,x,[a,b])", 0, "Antiderivative of f with respect to x, like integrate(x*sin(x),x). For definite integral enter optional arguments a and b, like integrate(x*sin(x),x,0,pi). Shortcut SHIFT F3.", "x*sin(x),x", "cos(x)/(1+x^4),x,0,inf", CAT_CATEGORY_CALCULUS},
  {"interp(X,Y)", 0, "Lagrange interpolation at points (xi,yi) where X is the list of xi and Y of yi. If interp is passed as 3rd argument, returns the divided differences list.", "[1,2,3,4,5],[0,1,3,4,4]", "[1,2,3,4,5],[0,1,3,4,4],interp", CAT_CATEGORY_POLYNOMIAL},
  {"inv(A)", 0, "Inverse of A.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
  {"inverse(f(x))", 0, "Inverse relation/function where possible. Required: expression or function.", "2*x+3", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_SOLVE << 8)},
  {"iquo(a,b)", 0, "Integer quotient of a and b.", "23,13", 0, CAT_CATEGORY_ARIT},
  {"irem(a,b)", 0,"Integer remainder of a and b.", "23,13", 0, CAT_CATEGORY_ARIT},
  {"isprime(n)", 0, "Returns 1 if n is prime, 0 otherwise.", "11", "10", CAT_CATEGORY_ARIT},
  {"jordan(A)", 0, "Jordan normal form of matrix A, returns P and D such that P^-1*A*P=D", "[[1,2],[3,4]]", "[[1,1,-1,2,-1],[2,0,1,-4,-1],[0,1,1,1,1],[0,1,2,0,1],[0,0,-3,3,-1]]", CAT_CATEGORY_MATRIX},
  {"laguerre(n,a,x)", 0, "n-ieme Laguerre polynomial (default a=0).", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"laplace(f,x,s)", 0, "Laplace transform of f","sin(x),x,s", 0, CAT_CATEGORY_CALCULUS},
  {"lcm(a,b,...)", 0, "Least common multiple.", "23,13", "x^2-1,x^3-1", CAT_CATEGORY_ARIT | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"lcoeff(p,x)", 0, "Leading coefficient of polynomial p in x.", "x^4-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"legendre(n)", 0, "n-the Legendre polynomial.", "10", "10,t", CAT_CATEGORY_POLYNOMIAL},
#ifdef RELEASE
  {"len(l)", 0, "Size of a list.", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_LIST},
#endif
  {"leve_crayon ", "leve_crayon ", "Turtle moves without trace.", 0, 0, CAT_CATEGORY_LOGO},
  {"limit(f,x=a)", 0, "Limit of f at x = a. Add 1 or -1 for unidirectional limits, e.g. limit(sin(x)/x,x=0) or limit(abs(x)/x,x=0,1). Shortcut: SHIFT MIXEDFRAC", "sin(x)/x,x=0", "exp(-1/x),x=0,1", CAT_CATEGORY_CALCULUS},
  {"line(equation)", 0, "Line of equation", "y=2x+1", 0, CAT_CATEGORY_PROGCMD},
  {"line_width_", "line_width_", "Width prefix (2 to 8)", 0, 0, CAT_CATEGORY_PROGCMD},
  {"linear_regression(Xlist,Ylist)", 0, "Linear regression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"linear_regression_plot(Xlist,Ylist)", 0, "Linear regression plot.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];linear_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  {"linetan(expr,x,x0)", 0, "Tangent to the graph at x=x0.", "sin(x),x,pi/2", 0, CAT_CATEGORY_PLOT},
  {"linsolve([eq1,eq2,..],[x,y,..])", 0, "Linear system solving. May use the output of lu for O(n^2) solving (see example 2).","[x+y=1,x-y=2],[x,y]", "#p,l,u:=lu([[1,2],[3,4]]); linsolve(p,l,u,[5,6])", CAT_CATEGORY_SOLVE | (CAT_CATEGORY_LINALG <<8) | (CAT_CATEGORY_MATRIX << 16)},
  {"logarithmic_regression(Xlist,Ylist)", 0, "Logarithmic egression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"logarithmic_regression_plot(Xlist,Ylist)", 0, "Logarithmic regression plot.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];logarithmic_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  {"lu(A)", 0, "LU decomposition LU of matrix A, P*A=L*U", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"magenta", "magenta", "Display option", "#display=magenta", 0, CAT_CATEGORY_PROGCMD},
  {"map(l,f)", 0, "Maps f on element of list l.","[1,2,3],x->x^2", 0, CAT_CATEGORY_LIST},
  {"matpow(A,n)", 0, "Returns matrix A^n", "[[1,2],[3,4]],n","#assume(n>=1);matpow([[0,2],[0,4]],n)",  CAT_CATEGORY_MATRIX},
  {"match(expr,form)", "solve(", "Match constants by equating coefficients/conditions. Required: expr, target form.", "coeff((a*x+b)-(2*x+3),x,1)=0,a", 0, CAT_CATEGORY_SOLVE | (CAT_CATEGORY_ALGEBRA << 8)},
  {"matrix(r,c,func)", 0, "Matrix from a defining function.", "2,3,(j,k)->j^k", 0, CAT_CATEGORY_MATRIX},
  {"mean(l)", 0, "Arithmetic mean of list l", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"median(l)", 0, "Median", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"montre_tortue ", "montre_tortue ", "Displays the turtle", 0, 0, CAT_CATEGORY_LOGO},
  {"mult_c_conjugate", 0, "Multiplier par le conjugue complexe.", "1+2*i", 0,  (CAT_CATEGORY_COMPLEXNUM << 8)},
  {"mult_conjugate", 0, "Multiplier par le conjugue (sqrt).", "sqrt(2)-sqrt(3)", 0, CAT_CATEGORY_ALGEBRA},
  {"nand(a,b)", "not(", "Boolean NAND: not(a and b). Required: a,b.", "A and B", 0, CAT_CATEGORY_PROGCMD},
  {"nor(a,b)", "not(", "Boolean NOR: not(a or b). Required: a,b.", "A or B", 0, CAT_CATEGORY_PROGCMD},
  {"normald([mu,sigma],x)", 0, "Normal distribution probability density, by default mu=0 and sigma=1. normald_cdf([mu,sigma],x) probability that \"normal distribution <=x\" e.g. normald_cdf(1.96). normald_icdf([mu,sigma],t) returns x such that \"normal distribution <=x\" has probability t, e.g. normald_icdf(0.975) ", "1.2", 0, CAT_CATEGORY_PROBA},
  {"not(x)", 0, "Logical not.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"numer(x)", 0, "Numerator of x.", "3/4", 0, CAT_CATEGORY_POLYNOMIAL},
  {"odesolve(f(t,y),[t,y],[t0,y0],t1)", 0, "Approx. solution of differential equation y'=f(t,y) and y(t0)=y0, value for t=t1 (add curve to get intermediate values of y)", "sin(t*y),[t,y],[0,1],2", "0..pi,(t,v)->{[-v[1],v[0]]},[0,1]", CAT_CATEGORY_SOLVE},
  {"partfrac(p,x)", 0, "Partial fraction expansion. Shortcut p=>+", "1/(x^4-1)", 0, CAT_CATEGORY_ALGEBRA},
  {"pas_de_cote n", "pas_de_cote ", "Turtle side jump from n steps, by default n=10", "#pas_de_cote 30", 0, CAT_CATEGORY_LOGO},
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
  {"plotseq(f(x),x=[u0,m,M],n)", 0, "Plot f(x) on [m,M] and n terms of the sequence defined by u_{n+1}=f(u_n) and u0.","sqrt(2+x),x=[6,0,7],5", 0, CAT_CATEGORY_PLOT},
  {"plus_point", "plus_point", "Display option", "#display=blue+plus_point", 0, CAT_CATEGORY_PROGCMD},
  {"point(x,y)", 0, "Point", "1,2", 0, CAT_CATEGORY_PLOT},
  {"polygon(list)", 0, "Closed polygon.", "1-i,2+i,3", 0, CAT_CATEGORY_PROGCMD},
  {"polygonscatterplot(Xlist,Ylist)", 0, "Plot points and polygonal line.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"polynomial_regression(Xlist,Ylist,n)", 0, "Polynomial regression, degree <= n.", "[1,2,3,4,5],[0,1,3,4,4],2", 0, CAT_CATEGORY_STATS},
  {"polynomial_regression_plot(Xlist,Ylist,n)", 0, "Polynomial regression plot, degree <= n.", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];polynomial_regression_plot(X,Y,2);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  //{"pour", "pour j de 1 jusque  faire  fpour;", "For loop.","#pour j de 1 jusque 10 faire print(j,j^2); fpour;", 0, CAT_CATEGORY_PROG},
  {"power_regression(Xlist,Ylist,n)", 0, "Power regression.", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"power_regression_plot(Xlist,Ylist,n)", 0, "Power regression graph", "#X,Y:=[1,2,3,4,5],[0,1,3,4,4];power_regression_plot(X,Y);scatterplot(X,Y)", 0, CAT_CATEGORY_STATS},
  {"powmod(a,n,p)", 0, "Returns a^n mod p.","123,456,789", 0, CAT_CATEGORY_ARIT},
  {"prove_bool(lhs,rhs)", "compare(", "Prove two Boolean/expression forms equivalent. Required: lhs,rhs.", "A and B,B and A", 0, CAT_CATEGORY_ALGEBRA},
  {"print(expr)", 0, "Print expr in console", 0, 0, CAT_CATEGORY_PROG},
  {"proot(p)", 0, "Returns real and complex roots, of polynomial p. Exemple proot([1,2.1,3,4.2]) or proot(x^3+2.1*x^2+3x+4.2)", "x^3+2.1*x^2+3x+4.2", 0, CAT_CATEGORY_POLYNOMIAL},
  {"purge(x)", 0, "Clear assigned variable x. Shortcut SHIFT-FORMAT", 0, 0, CAT_CATEGORY_PROGCMD|(CAT_CATEGORY_SOFUS<<8)},
  {"python(f)", 0, "Displays f in Python syntax.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"python_compat(0|1|2)", 0, "python_compat(0) Xcas syntax, python_compat(1) Python syntax with ^ interpreted as power, python_compat(2) ^ as bit xor", "0", "1", CAT_CATEGORY_PROG},
  {"q2a(expr,[vars])", 0, "Matrix of a quadratic form", "x^2+3*x*y","x^2+3*x*y,[x,y]", CAT_CATEGORY_LINALG},
  {"qr(A)", 0, "A=Q*R factorization with Q orthogonal and R upper triangular", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"quartile1(l)", 0, "1st quartile", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"quartile3(l)", 0, "3rd quartile", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"quo(p,q,x)", 0, "Quotient of synthetic division of polynomials p and q (variable x).", 0, 0, CAT_CATEGORY_POLYNOMIAL},
  {"quote(x)", 0, "Returns expression x unevaluated.", 0, 0, CAT_CATEGORY_ALGEBRA},
  {"rand()", "rand()", "Random real between 0 and 1", 0, 0, CAT_CATEGORY_PROBA},
  {"randint(n)", 0, "Random integer between 1 and n", "6", 0, CAT_CATEGORY_PROBA},
  {"ranm(n,m,[loi,parametres])", 0, "Random matrix with integer coefficients or according to a probability law (ranv for a vector). Examples ranm(2,3), ranm(3,2,binomial,20,.3), ranm(4,2,normald,0,1)", "4,2,normald,0,1", "3,3,10", CAT_CATEGORY_MATRIX},
  {"ranv(n,[loi,parametres])", 0, "Random vector.", "4,normald,0,1", "10,30", CAT_CATEGORY_LINALG},
  {"ratnormal(x)", 0, "Puts everything over a common denominator.", 0, 0, CAT_CATEGORY_ALGEBRA},
  {"re(z)", 0, "Real part.", "1+i", 0, CAT_CATEGORY_COMPLEXNUM},
  {"read(\"filename\")", "read(\"", "Read a file.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"range(expr,[x])", 0, "Range/image of an expression where possible. Required: expr. Optional: variable x.", "x^2+2*x+3,x", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_REAL << 8)},
  {"rectangle_plein a,b", "rectangle_plein ", "Direct filled rectangle from turtle position, if b is omitted b==a", "#rectangle_plein 30","#rectangle_plein 20,40", CAT_CATEGORY_LOGO},
  {"recule n", "recule ", "Turtle backward n steps, n=10 by default", "#recule 30", 0, CAT_CATEGORY_LOGO},
  {"red", "red", "Display option", "#display=red", 0, CAT_CATEGORY_PROGCMD},
  {"rem(p,q,x)", 0, "Remainder of synthetic division of polynomials p and q (variable x)", 0, 0, CAT_CATEGORY_POLYNOMIAL},
#ifdef RELEASE
  {"residue(f(z),z,z0)", 0, "Residue of an expression at z0.", "1/(x^2+1),x,i", 0, CAT_CATEGORY_COMPLEXNUM},
#endif
  {"resultant(p,q,x)", 0, "Resultant in x of polynomials p and q.", "#P:=x^3+p*x+q;resultant(P,P',x);", 0, CAT_CATEGORY_POLYNOMIAL},
  {"revert(p[,x])", 0, "Revert Taylor series","x+x^2+x^4", 0, CAT_CATEGORY_CALCULUS},
  {"rewrite(expr,target)", "canonical_form(", "Rewrite expression around a target form/linear shift. Required: expr. Optional target/variable.", "x^2+6*x+11", "2*x^2-8*x+3,x", CAT_CATEGORY_ALGEBRA},
  {"rgb(r,g,b)", 0, "color defined from red, green, blue from 0 to 255", "255,0,255", 0, CAT_CATEGORY_PROGCMD},
  {"rhombus_point", "rhombus_point", "Display option", "#display=magenta+rhombus_point", 0, CAT_CATEGORY_PROGCMD},
  {"rond n", "rond ", "Circle tangent to the turtle, radius n. Run rond n,theta for an arc of circle of theta degrees", 0, 0, CAT_CATEGORY_LOGO},
  {"rref(A)", 0, "Row reduction to echelon form", "[[1,2,3],[4,5,6]]", 0, CAT_CATEGORY_MATRIX|  (CAT_CATEGORY_LINALG<<8)},
  {"rsolve(equation,u(n),[init])", 0, "Solve a recurrence relation.","u(n+1)=2*u(n)+3,u(n),u(0)=1", "([u(n+1)=3*v(n)+u(n),v(n+1)=v(n)+u(n)],[u(n),v(n)],[u(0)=1,v(0)=2]", CAT_CATEGORY_SOLVE},
  {"saute n", "saute ", "Turtle jumps n steps, by default n=10", "#saute 30", 0, CAT_CATEGORY_LOGO},
  {"scatterplot(Xlist,Ylist)", 0, "Draws points", "[1,2,3,4,5],[0,1,3,4,4]", 0, CAT_CATEGORY_STATS},
  {"segment(A,B)", 0, "Segment", "1,2+i", 0, CAT_CATEGORY_PROGCMD},
  {"seq(expr,var,a,b)", 0, "Generates a list from an expression.","j^2,j,1,10", 0, CAT_CATEGORY_PROGCMD},
  //{"si", "si  alors  sinon  fsi;", "Test.", "#f(x):=si x>0 alors x; sinon -x; fsi;// valeur absolue", 0, CAT_CATEGORY_PROG},
  {"sign(x)", 0, "Returns -1 if x is negative, 0 if x is zero and 1 if x is positive.", 0, 0, CAT_CATEGORY_REAL},
  {"simplify(expr)", 0, "Returns x in a simpler form. Shortcut expr=>/", "sin(3x)/sin(x)", 0, CAT_CATEGORY_ALGEBRA},
  {"solve(equation,x)", 0, "Exact solving of equation w.r.t. x (or of a polynomial system). Run csolve for complex solutions, linsolve for a linear system. Shortcut SHIFT XthetaT", "x^2-x-1=0,x", "[x^2-y^2=0,x^2-z^2=0],[x,y,z]", CAT_CATEGORY_SOLVE},
  {"sort(l)", 0, "Sorts a list.","[3/2,2,1,1/2,3,2,3/2]", "[[1,2],[2,3],[4,3]],(x,y)->when(x[1]==y[1],x[0]>y[0],x[1]>y[1]", CAT_CATEGORY_LIST},
  {"square_point", "square_point", "Display option", "#display=cyan+square_point", 0, CAT_CATEGORY_PROGCMD},
  {"star_point", "star_point", "Display option", "#display=magenta+star_point", 0, CAT_CATEGORY_PROGCMD},
  {"stddev(l)", 0, "Standard deviation of list l", "[3/2,2,1,1/2,3,2,3/2]", 0, CAT_CATEGORY_STATS},
  {"subst(a,b=c)", 0, "Substitutes b for c in a. Shortcut a(b=c).", "x^2,x=3", 0, CAT_CATEGORY_ALGEBRA},
  {"suvat(equations,vars)", "solve(", "SUVAT by simultaneous equations. Required: equations/list. Optional: variable(s) to solve.", "[v=u+a*t,s=u*t+1/2*a*t^2],[s,u,v,a,t]", 0, CAT_CATEGORY_SOLVE},
  {"sum(f,k,m,M)", 0, "Summation of expression f for k from m to M. Exemple sum(k^2,k,1,n)=>*. Shortcut ALPHA F3", "k,k,1,n", 0, CAT_CATEGORY_CALCULUS},
  {"svd(A)", 0, "Singular Value Decomposition, returns U orthogonal, S vector of singular values, Q orthogonal such that A=U*diag(S)*tran(Q).", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"tabvar(f,[x=a..b])", 0, "Table of variations of expression f, optional arguments variable x in interval a..b", "sqrt(x^2+x+1)",  "[cos(t),sin(3t)],t", CAT_CATEGORY_CALCULUS},
  //{"tantque", "tantque  faire   ftantque;", "While loop.", "#j:=13; tantque j!=1 faire j:=when(even(j),j/2,3j+1); print(j); ftantque;", 0, CAT_CATEGORY_PROG},
  {"taylor(f,x=a,n,[polynom])", 0, "Taylor expansion of f of x at a order n, add parameter polynom to remove remainder term.","sin(x),x=0,5", "sin(x),x=0,5,polynom", CAT_CATEGORY_CALCULUS},
  {"tchebyshev1(n)", 0, "Tchebyshev polynomial 1st kind: cos(n*x)=T_n(cos(x))", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"tchebyshev2(n)", 0, "Tchebyshev polynomial 2nd kind: sin((n+1)*x)=sin(x)*U_n(cos(x))", "10", 0, CAT_CATEGORY_POLYNOMIAL},
  {"tcollect(expr)", 0, "Linearize and collect trig functions.","sin(x)+cos(x)", 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, "Expand trigonometric, exp and ln functions.","sin(3x)", 0, CAT_CATEGORY_TRIG},
  {"time(cmd)", 0, "Time to run a command or set the clock","int(1/(x^4+1),x)","8,0", CAT_CATEGORY_PROG},
  {"tlin(expr)", 0, "Trigonometric linearization of expr.","sin(x)^3", 0, CAT_CATEGORY_TRIG},
  {"transform(expr,[form])", "normal(", "Transform/rearrange expression with KhiCAS normal/simplify/factor forms. Required: expr. Optional: target form.", "(x+1)^2-(x^2+2*x+1)", 0, CAT_CATEGORY_ALGEBRA},
  {"tourne_droite n", "tourne_droite ", "Turtle turns right n degrees, n=90 by default", 0, 0, CAT_CATEGORY_LOGO},
  {"tourne_gauche n", "tourne_gauche ", "Turtle turns left n degrees, n=90 by default", 0, 0, CAT_CATEGORY_LOGO},
  {"tran(A)", 0, "Transposes matrix A. Transconjugate command is trn(A) or A^*.", "[[1,2],[3,4]]", 0, CAT_CATEGORY_MATRIX},
  {"triangle_point", "triangle_point", "Display option", "#display=yellow+triangle_point", 0, CAT_CATEGORY_PROGCMD},
  {"trig2exp(expr)", 0, "Convert complex exponentials to trigonometric functions","cos(x)^3", 0, CAT_CATEGORY_TRIG},
  {"trigcos(expr)", 0, "Convert sin^2 and tan^2 to cos^2.","sin(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigsin(expr)", 0, "Convert cos^2 and tan^2 to sin^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"trigtan(expr)", 0, "Convert cos^2 and sin^2 to tan^2.","cos(x)^4", 0, CAT_CATEGORY_TRIG},
  {"uniformd(a,b,x)", "uniformd", "uniform law on [a,b] of density 1/(b-a)", 0, 0, CAT_CATEGORY_PROBA},
  //{"version", "version()", "Khicas 1.5.0, (c) B. Parisse et al. www-fourier.ujf-grenoble.fr/~parisse\nLicense GPL version 2. Interface adapted from Eigenmath for Casio, G. Maia, http://gbl08ma.com. Do not use if CAS calculators are forbidden.", 0, 0, CAT_CATEGORY_PROGCMD},
  {"write(\"filename\",var)", "write(\"", "Save 1 or more variables in a file. For example f(x):=x^2; write(\"func_f\",f).",  0, 0, CAT_CATEGORY_PROGCMD},
  {"yellow", "yellow", "Display option", "#display=yellow", 0, CAT_CATEGORY_PROGCMD},
  {"|", "|", "Logical or", "#1|2", 0, CAT_CATEGORY_PROGCMD},
  {"~", "~", "Complement", "#~7", 0, CAT_CATEGORY_PROGCMD},
};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="CasioCAS Help";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="To set the system clock, run hh,mm=>, for example 13,45=>,\nKeyboard shortcuts (shell and editor)\nF1-F3: according to the legends\nF4: catalog\nF5: lowercase lock or switch lowercase/uppercase\nF6: file menu\n(-): _\nshift-OPTN: programming commands including pixelised graphs\nshift-PRGM: programming characters\nshift-FRAC: plot commands\nOPTN: options\nshift-QUIT: turtle\nshift-Lst: list editor or commands\nshift-Mtr: matrix editor or commands\nVARS: list of variables (shell) or turtle picture (editor)\nshift-FORMAT: purge\n=>+: partfrac\n=>*: factor\n=>sin/cos/tan\n=>=>: solve\n\n*** Shell ***:\ndownkey: completion/help, shift-SETUP: configuration\nF3: 2-d expression editor or graphic view or text editor\nalpha-F3: text editor\n\n*** Expression editor ***\npad: move selection inside expression tree\nshift-left/right exchange selection with right or left argument\nALPHA-left/right inside a sum or product: increase selection adding left or right argument\nF3: Edit selection, shift-F3: increase fontsize, ALPHA-F3: decrease fontsize\nF4: catalog\nF5: lower/uppercase\nF6: Eval selection, shift-F6: approx value, ALPHA-F6: regroup command\nDEL: suppress root operator in selection\n\n*** Script editor: ***\nfraction key (G): indent, shift-fraction: help/completion, shift-CLIP: begins selection, move cursor to the end then DEL to remove or shift-CLIP to copy to clipboard. shift-PASTE to paste.\nF6-6 Search only: enter word then EXE then EXIT. Type EXE for next occurence, AC to cancel.\nF6-6: Replace: enter word then EXE then replacement then EXE. Type EXE or EXIT to replace or skip replacement and go to the next occurence, AC to cancel.\nshift-Ans: check syntax\n\n*** Graphs: ***\n+ - zoom\n(-): zoomout along y\n*: autoscale\n/: orthonormalize\nOPTN: axes on/off";
const char apropos_string[]="CasioCAS, powered by KhiCAS 1.5.0, (c) 2019 B. Parisse et al. www-fourier.univ-grenoble-alpes.fr/~parisse\nLicense GPL version 2.\nInterface adapted from Eigenmath for Casio, by G. Maia, http://gbl08ma.com, Mike Smith, Nemhardy, LePhenixNoir\nDo not use if CAS calculators are forbidden!";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);

bool catalog_hidden_category(int category){
  return category==CAT_CATEGORY_PROG || category==CAT_CATEGORY_LOGO;
}

bool startswith_catalog(const char *s,const char *prefix){
  return s && prefix && !strncmp(s,prefix,strlen(prefix));
}

bool catalog_hidden_name(const char *name){
  if (!name)
    return true;
  if (startswith_catalog(name," loop ") || startswith_catalog(name," test ") ||
      startswith_catalog(name," function ") || startswith_catalog(name," local ") ||
      startswith_catalog(name," return ") || startswith_catalog(name,"debug(") ||
      startswith_catalog(name,"from math") || startswith_catalog(name,"input()") ||
      startswith_catalog(name,"print(") || startswith_catalog(name,"python(") ||
      startswith_catalog(name,"python_compat") || startswith_catalog(name,"time("))
    return true;
  if (startswith_catalog(name,"avance") || startswith_catalog(name,"baisse_crayon") ||
      startswith_catalog(name,"cache_tortue") || startswith_catalog(name,"crayon") ||
      startswith_catalog(name,"efface") || startswith_catalog(name,"ecris") ||
      startswith_catalog(name,"leve_crayon") || startswith_catalog(name,"montre_tortue") ||
      startswith_catalog(name,"pas_de_cote") || startswith_catalog(name,"rectangle_plein") ||
      startswith_catalog(name,"recule") || startswith_catalog(name,"rond") ||
      startswith_catalog(name,"saute") || startswith_catalog(name,"tourne_"))
    return true;
  if (startswith_catalog(name,"draw_") || startswith_catalog(name,"display") ||
      startswith_catalog(name,"axes") || startswith_catalog(name,"gl_") ||
      startswith_catalog(name,"line_width") || startswith_catalog(name,"rgb(") ||
      startswith_catalog(name,"black") || startswith_catalog(name,"blue") ||
      startswith_catalog(name,"cyan") || startswith_catalog(name,"filled") ||
      startswith_catalog(name,"green") || startswith_catalog(name,"magenta") ||
      startswith_catalog(name,"red") || startswith_catalog(name,"yellow") ||
      strstr(name,"_point"))
    return true;
  if (startswith_catalog(name,"laplace") || startswith_catalog(name,"ilaplace") ||
      startswith_catalog(name,"fourier_") || startswith_catalog(name,"jordan") ||
      startswith_catalog(name,"svd") || startswith_catalog(name,"gramschmidt") ||
      startswith_catalog(name,"cond(") || startswith_catalog(name,"resultant"))
    return true;
  if (startswith_catalog(name,"rand") || startswith_catalog(name,"ranv") ||
      startswith_catalog(name,"ranm"))
    return true;
  if (startswith_catalog(name,"residue") || startswith_catalog(name,"cfactor") ||
      startswith_catalog(name,"cpartfrac"))
    return true;
  return false;
}

ustl::string catalog_param_hint(const char *name){
  const char *open=strchr(name,'(');
  if (!open)
    return "Args: see syntax in command name.";
  const char *close=strchr(open,')');
  if (!close || close==open+1)
    return "Args: none.";
  ustl::string req,opt;
  bool optional=false;
  for (const char *p=open+1;p<close;++p){
    if (*p=='['){ optional=true; continue; }
    if (*p==']'){ optional=false; continue; }
    if (optional)
      opt += *p;
    else
      req += *p;
  }
  ustl::string out("Args: required ");
  if (req.size())
    out += req;
  else
    out += "none";
  out += "; optional ";
  if (opt.size())
    out += opt;
  else
    out += "none";
  out += ".";
  return out;
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
  ADD_CAT(CAT_CATEGORY_ALGEBRA,"Algebra");
  ADD_CAT(CAT_CATEGORY_LINALG,"Linear algebra");
  ADD_CAT(CAT_CATEGORY_CALCULUS,"Calculus");
  ADD_CAT(CAT_CATEGORY_ARIT,"Arithmetic, crypto");
  ADD_CAT(CAT_CATEGORY_COMPLEXNUM,"Complexes");
  ADD_CAT(CAT_CATEGORY_PLOT,"Graphs");
  ADD_CAT(CAT_CATEGORY_POLYNOMIAL,"Polynomials");
  ADD_CAT(CAT_CATEGORY_PROBA,"Probabilities");
  ADD_CAT(CAT_CATEGORY_PROG,"Programs");
  ADD_CAT(CAT_CATEGORY_PROGCMD,"Program_cmds");
  ADD_CAT(CAT_CATEGORY_REAL,"Reals");
  ADD_CAT(CAT_CATEGORY_SOLVE,"Solve");
  ADD_CAT(CAT_CATEGORY_STATS,"Statistics");
  ADD_CAT(CAT_CATEGORY_TRIG,"Trigonometry");
  ADD_CAT(CAT_CATEGORY_OPTIONS,"Options");
  ADD_CAT(CAT_CATEGORY_LIST,"Lists");
  ADD_CAT(CAT_CATEGORY_MATRIX,"Matrices");
  ADD_CAT(CAT_CATEGORY_SOFUS,"Variable handling");
  ADD_CAT(CAT_CATEGORY_LOGO,"Turtle shift QUIT");
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
      char * example2=hascat?completeCat[index].example2:0;
      textArea text;
      text.title = (char*)"Help on command";
      text.editable=false;
      text.clipline=-1;
      text.allowF1=true;
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement> (example2?4:3);
      elem[0].s = hascat?completeCat[index].name:menuitems[menu.selection-1].text;
      elem[0].color = COLOR_BLUE;
      elem[1].newLine = 1;
      elem[1].lineSpacing = 3;
      ustl::string autoexample;
      if (hascat){
	if (completeCat[index].desc && completeCat[index].desc[0])
	  elem[1].s = completeCat[index].desc;
	else
	  elem[1].s = "CAS command.";
	elem[1].s += "\n";
	elem[1].s += catalog_param_hint(completeCat[index].name);
      }
      else {
	elem[1].s="KhiCAS built-in command.\nArgs: see command syntax; required/optional args depend on command.";
	autoexample=elem[0].s+"()";
	int token=menuitems[menu.selection-1].token;
	if (isopt){
	  if (token==_INT_PLOT+T_NUMBER*256){
	    autoexample="display="+elem[0].s;
	    elem[1].s="Graphic option: "+autoexample;
	  }
	  if (token==_INT_COLOR+T_NUMBER*256){
	    autoexample="display="+elem[0].s;
	    elem[1].s="Color: "+autoexample;
	  }
	  if (token==_INT_SOLVER+T_NUMBER*256){
	    autoexample=elem[0].s;
	    elem[1].s="fsolve option: " + autoexample;
	  }
	  if (token==_INT_TYPE+T_TYPE_ID*256){
	    autoexample=elem[0].s;
	    elem[1].s="Object type: " + autoexample;
	  }
	}
	if (isall){
	  if (token==T_UNARY_OP || token==T_UNARY_OP_38){
	    elem[1].s="KhiCAS built-in function.\nArgs: required argument(s), optional argument(s) depend on command.";
	    autoexample=elem[0].s+"(x)";
	  }
	}
      }
      ustl::string ex("Example (F2): ");
      if (example || hascat){
	elem[2].newLine = 1;
	elem[2].lineSpacing = 3;
	if (example && example[0]=='#')
	  ex += example+1;
	else {
	  ex += insert_string(index);
	  if (example)
	    ex += example;
	  ex += ")";
	}
	elem[2].s = ex;
	if (example2){
	  string ex2="Example 2 (F3): ";
	  if (example2[0]=='#')
	    ex2 += example2+1;
	  else {
	    ex2 += insert_string(index);
	    ex2 += example2;
	    ex2 += ")";
	  }
	  elem[3].newLine = 1;
	  elem[3].lineSpacing = 3;
	  elem[3].s=ex2;
	}
      }
      else {
	if (autoexample.size())
	  elem[2].s=ex+autoexample;
	else
	  elem.pop_back();
      }
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

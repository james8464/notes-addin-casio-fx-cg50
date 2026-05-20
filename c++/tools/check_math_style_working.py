#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / "c++" / "addin" / "host" / "build" / "casio_host"

CASES = [
    (
        "chain sum",
        ["--derive", "sin(x^2)+atan(x/3),x,method=chain"],
        ("d/dx(sin(x^2)) =", "d/dx(atan(x/3)) =", "dy/dx ="),
    ),
    (
        "hyperbolic compact only",
        ["--derive", "sinh(x^2)+atanh(x/3),x,method=chain"],
        ("dy/dx =",),
        ("d/dx(sinh(x^2)) =", "d/dx(atanh(x/3)) =", "u =", "du/dx ="),
    ),
    (
        "product",
        ["--derive", "(x^2+1)*sin(x),x"],
        ("f1 = x^2 + 1", "f1' = 2*x", "f2 = sin(x)", "f2' = cos(x)", "dy/dx = f1'*f2 + f1*f2'"),
    ),
    (
        "single chain",
        ["--derive", "sin((x+1)^2),x,method=chain"],
        ("u = (x + 1)^2", "du/dx = 2*(x + 1)", "dy/dx = cos(u)*du/dx"),
    ),
    (
        "quotient",
        ["--derive", "x^2/(3*x-1),x"],
        ("u = x^2", "u' = 2*x", "v = 3*x - 1", "v' = 3", "y' = (u'v-u*v')/v^2", "dy/dx = [(2*x)*(3*x - 1) - (x^2)*(3)]/(3*x - 1)^2"),
    ),
    (
        "linear quotient simplifies numerator",
        ["--derive", "(4*x+1)/(1-2*x),x,method=quotient"],
        ("u = 4*x + 1", "v = - 2*x + 1", "[(4)*(- 2*x + 1)-(4*x + 1)*(-2)] = 6", "dy/dx = 6/(- 2*x + 1)^2"),
        ("dy/dx = [(4)*(- 2*x + 1) - (4*x + 1)*(-2)]/(- 2*x + 1)^2",),
    ),
    (
        "implicit log direct",
        ["--derive", "ln(x+y)=x*y,x,method=implicit"],
        ("d/dx[ln(x + y)] = (1 + dy/dx)/(x + y)", "d/dx[x*y] = y + x*dy/dx", "(1 + dy/dx)/(x + y) = y + x*dy/dx", "dy/dx*(1 - x*(x + y)) = y*(x + y) - 1", "dy/dx = (y*(x + y) - 1)/(1 - x*(x + y))"),
        ("F_x", "F_y"),
    ),
    (
        "implicit variable powers",
        ["--derive", "x^y=y^x,x,method=implicit"],
        ("ln(x^y)=ln(y^x)", "y*ln(x)=x*ln(y)", "dy/dx*ln(x)-x*dy/dx/y=ln(y)-y/x", "dy/dx=(ln(y)-y/x)/(ln(x)-x/y)", "dy/dx = y*(x*ln(y) - y)/(x*(y*ln(x) - x))"),
        ("F_x", "F_y", "-F_x/F_y"),
    ),
    (
        "implicit generic substitution",
        ["--derive", "sin(x*y)+x^2=y^2,x,method=implicit"],
        ("F_x + F_y*dy/dx = 0", "(cos(x*y)*y + 2*x) + (cos(x*y)*x - 2*y)*dy/dx = 0", "(cos(x*y)*x - 2*y)*dy/dx = -(cos(x*y)*y + 2*x)", "dy/dx = -(cos(x*y)*y + 2*x)/(cos(x*y)*x - 2*y)", "dy/dx = (y*cos(x*y) + 2*x)/(2*y - x*cos(x*y))"),
    ),
    (
        "implicit rational clears denominators",
        ["--derive", "1/(2*x+1)+1/(y+1)=x^2,x,method=implicit"],
        ("Domain: denoms !=0", "y + 2*x - x^2*(2*x + 1)*(y + 1) + 2 = 0", "d/dx(LHS) = d/dx(RHS)", "collect dy/dx", "dy/dx = (2*x*(2*x + 1)*(y + 1) + 2*x^2*(y + 1) - 2)/(- x^2*(2*x + 1) + 1)"),
        ("F_x", "F_y", "-F_x/F_y"),
    ),
    (
        "param generic",
        ["--derive", "x=3+2*cos(theta),y=-3+2*sin(theta),theta,x,method=param"],
        ("dx/dt = -2*sin(theta)", "dy/dt = 2*cos(theta)", "dy/dx=(dy/dt)/(dx/dt)", "dy/dx = (2*cos(theta))/(-2*sin(theta))"),
    ),
    (
        "param no duplicate final",
        ["--derive", "t^5+4,t^5-5*t,t,method=param"],
        ("dx/dt = 5*t^4", "dy/dt = 5*t^4 - 5", "dy/dx = (5*t^4 - 5)/(5*t^4)"),
        ("dy/dx = (5*t^4 - 5)/(5*t^4)\ndy/dx = (5*t^4 - 5)/(5*t^4)",),
    ),
    (
        "param reciprocal trig simplify",
        ["--derive", "sec(theta),ln(1+cos(2*theta)),theta,method=param"],
        ("dx/dt = sec(theta)*tan(theta)", "tan(u) = sin(u)/cos(u), sec(u) = 1/cos(u)", "dy/dx = -2*cos(theta)"),
        ("dy/dx = -4*1/(2*cos(theta)^2)*cos(theta)*sin(theta)/(sec(theta)*tan(theta))\n",),
    ),
    (
        "param double angle simplify",
        ["--derive", "cos(2*theta),2*sin(theta)^3,theta,method=param"],
        ("sin(2u) = 2sin(u)cos(u)", "dy/dx = -3*sin(theta)/2"),
    ),
    (
        "param reciprocal power simplify",
        ["--derive", "1/t+1/t^2,1/t-1/t^2,t,method=param"],
        ("dy/dx = ((- t^-2 + 2*t^-3)*t^3)/((- t^-2 - 2*t^-3)*t^3)", "dy/dx = (t - 2)/(t + 2)"),
    ),
    (
        "exam chain final",
        ["--derive", "(2*x+ln(x))^3,x"],
        ("y = (2*x + ln(x))^3", "u = 2*x + ln(x)", "du/dx = 1/x + 2", "dy/dx = 3*(2*x + ln(x))^2*(1/x + 2)"),
    ),
    (
        "log rational derivative quotient",
        ["--derive", "ln((x-1)/(x+2)),x"],
        ("u = (x - 1)/(x + 2)", "Domain: x < -2 or x > 1", "u' = ((1)*(x + 2)-(x - 1)*(1))/(x + 2)^2", "u' = 3/(x + 2)^2", "dy/dx = [3/(x + 2)^2]/[(x - 1)/(x + 2)]", "dy/dx = 3/((x + 2)*(x - 1))"),
    ),
    (
        "log radical conjugate derivative family",
        ["--derive", "ln((sqrt(e^x+1)-1)/(sqrt(e^x+1)+1)),x"],
        ("s = sqrt(e^(x) + 1)", "dy/dx = 1/sqrt(e^(x) + 1)"),
        ("du/dx/u",),
    ),
    (
        "log radical conjugate x family",
        ["--derive", "ln((sqrt(1-x^2)-1)/(sqrt(1-x^2)+1)),x"],
        ("s = sqrt(- x^2 + 1)", "dy/dx = 2/(x*sqrt(- x^2 + 1))"),
        ("du/dx/u",),
    ),
    (
        "log asinh style radical family",
        ["--derive", "ln(x-2+sqrt(x^2-4*x+13)),x"],
        ("u = x - 2", "dy/dx = 1/sqrt(x^2 - 4*x + 13)"),
        ("du/dx/u",),
    ),
    (
        "evalat exact fraction",
        ["--alg", "evalat(sin(x)^2-cos(x)^2,x,pi/3)"],
        ("x = pi/3", "f(pi/3) = 1/2"),
        ("ERR:", "Done"),
    ),
    (
        "atan complement derivative family",
        ["--derive", "atan(x)+atan((1-x)/(1+x)),x"],
        ("v = (1-u)/(1+u)", "dy/dx = 0"),
        ("dy/dx = 1/(x^2 + 1) +",),
    ),
    (
        "reciprocal trig derivative identity",
        ["--derive", "cosec(x)^2-cot(x)^2,x,method=auto"],
        ("cosec(u)^2 - cot(u)^2 = 1", "Domain: sin(x) != 0", "dy/dx = 0"),
        ("y = cosec", "Unexpected token", "ERR:"),
    ),
    (
        "looping parts final",
        ["--int", "e^(2*x)*cos(3*x),method=parts"],
        ("I = Int(e^(2*x)*cos(3*x)) dx", "u = cos(3*x)", "dv = e^(2*x) dx", "J = Int(e^(2*x)*sin(3*x)) dx", "Sub J:", "Collect: I terms", "13/4*I =", "Solve: I =", "I = (1/2*e^(2*x)*cos(3*x) + 3/4*e^(2*x)*sin(3*x))/(13/4)", "e^(2*x)*(2*cos(3*x) + 3*sin(3*x))/13 + C"),
    ),
    (
        "rform final",
        ["--trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform"],
        ("R = sqrt(3^2 + 4^2) = 5", "3*cos(x)+4*sin(x)=5*cos(x-alpha)", "x = [arctan(4/3) + arccos(2/5), 2*pi + arctan(4/3) - arccos(2/5)]"),
    ),
    (
        "signed rform rewrite",
        ["--trig", "4*sin(x)-3*cos(x),method=rform"],
        ("R = sqrt(4^2+3^2) = 5", "R*sin(x-alpha)", "5*sin(x-atan(3/4))"),
    ),
    (
        "affine trig range",
        ["--alg", "range(3+2*sin(4*x-pi/3))"],
        ("-1 <= sin(4*x - pi/3) <= 1", "1 <= 2*sin(4*x - pi/3) + 3 <= 5", "1 <= y <= 5"),
    ),
    (
        "sincos r range",
        ["--alg", "range(3*cos(x)+4*sin(x))"],
        ("R = sqrt((4)^2 + (3)^2) = 5", "-5 <= 3*cos(x) + 4*sin(x) <= 5", "-5 <= y <= 5"),
    ),
    (
        "trig period",
        ["--alg", "period(tan(4*x-pi/3))"],
        ("u = 4*x - pi/3", "du/dx = 4", "period(tan) = pi", "pi/4"),
    ),
    (
        "direct inverse trig composition",
        ["--alg", "sin(asin(x))"],
        ("Domain: -1 <= x <= 1", "sin(asin(x)) = x"),
    ),
    (
        "inverse trig exact solve",
        ["--alg", "solve(asin(x)=pi/6,x)"],
        ("asin(x) = pi/6", "x = sin(pi/6)", "x = 1/2"),
    ),
    (
        "affine inverse trig solve",
        ["--alg", "solve(pi-3*acos(x+1)=0,x)"],
        ("acos(x + 1) = pi/3", "-1 <= x + 1 <= 1", "x + 1 = cos(pi/3)", "x = -1/2"),
        ("x = []", "ERR:"),
    ),
    (
        "scaled inverse trig solve",
        ["--alg", "solve(2*asin(x)-pi/3=0,x)"],
        ("asin(x) = pi/6", "-1 <= x <= 1", "x = sin(pi/6)", "x = 1/2"),
        ("x = []", "ERR:"),
    ),
    (
        "noisy defint parse",
        ["--int", "defint(3 *(x)/sqrt((3)(x)+ 6),x,0,8)"],
        ("u = sqrt(3*x + 6)", "x = 0 => u = sqrt(6), x = 8 => u = sqrt(30)", "8/3*sqrt(30) + 8/3*sqrt(6)"),
        ("ERR:", "Expected )"),
    ),
    (
        "parenthesised binomial var",
        ["--alg", "binomial((1+3 * x)^( -  1/(2)),(x),0,4)"],
        ("n = -1/2", "Simplified terms: T0 = 1, T1 = -3/2*x", "Valid for abs(x) < 1/3"),
        ("Supported forms", "ERR:"),
    ),
    (
        "sin same fn family",
        ["--trig", "sin(8*x)=sin(5*x),x,0,2*pi,26,method=identity"],
        ("sin(A) = sin(B)", "8*x = 5*x+2*pi*n => x = 2*pi*n/3", "8*x = pi-5*x+2*pi*n => x = (pi+2*pi*n)/13", "x = ["),
    ),
    (
        "sin same fn degree branches",
        ["--trig", "sin(2*x)-sin(x)=0,x,0,360"],
        ("sin(A)-sin(B) = 0", "sin(A)-sin(B) = 2*cos((A+B)/2)*sin((A-B)/2)", "A = 2*x, B = x", "2*x = x+360n => x = 360n", "2*x = 180-x+360n => x = (180+360n)/3", "x = [0, 60, 180, 300, 360]"),
    ),
    (
        "madas general same fn degree",
        ["--trig", "sin(4*x+10)=sin(50),x,0,360,method=general"],
        ("sin(A) = sin(B)", "A = B+360n => x = 10 + n*90", "A = 180-B+360n => x = 30 + n*90", "x = 10 + n*90 or x = 30 + n*90"),
        ("46.302756", "83.697243", "x = []"),
    ),
    (
        "madas general tan square",
        ["--trig", "tan(30-x)^2=3,x,0,360,method=general"],
        ("tan(- x + 30)^2 = 3", "tan(- x + 30) = +/-sqrt(3)", "x = 150 + n*180", "x = 90 + n*180"),
        ("x = []", "ERR:"),
    ),
    (
        "madas general sin2 cos factor",
        ["--trig", "sin(2*x)=cos(x),x,0,2*pi,method=general"],
        ("sin(2A) = 2*sin(A)*cos(A)", "cos(A)*(2*sin(A)-1) = 0", "x = pi/2 + n*pi", "x = pi/6 + n*2*pi or 5*pi/6 + n*2*pi"),
        ("x = []", "Failed to isolate"),
    ),
    (
        "madas general sin sum",
        ["--trig", "sin(5*x)+sin(3*x)=0,x,0,2*pi,method=general"],
        ("sin(A)+sin(B) = 2*sin((A+B)/2)*cos((A-B)/2)", "2*sin(4*x)*cos(x) = 0", "x = 0 + n*pi/4", "x = pi/2 + n*pi"),
        ("x = []", "Failed to isolate"),
    ),
    (
        "madas general costan product",
        ["--trig", "2*cos(theta)*tan(theta)=sqrt(3),theta,0,360,method=general"],
        ("cos(A)*tan(A) = sin(A), cos(A)!=0", "sin(A) = sqrt(3)/2", "theta = 60 + n*360 or 120 + n*360"),
        ("theta = []", "Failed to isolate"),
    ),
    (
        "madas general shifted sine",
        ["--trig", "sqrt(3)*sin(x-pi/6)=sin(x),x,0,2*pi,method=general"],
        ("sin(M+a) = sin(M)cos(a)+cos(M)sin(a)", "tan(x) = sqrt(3)", "x = pi/3 + n*pi"),
        ("x = []", "Failed to isolate"),
    ),
    (
        "madas reciprocal pythagorean sec tan",
        ["--trig", "2*tan(theta)^2=11*sec(theta)-7,theta,0,360"],
        ("tan(theta)^2 = sec(theta)^2-1", "u = sec(theta)", "2u^2-11u+5 = 0", "cos(theta) = 1/5", "theta = [78.4630409672, 281.536959033]"),
    ),
    (
        "madas rational cosec cot",
        ["--trig", "cot(beta)/(cosec(beta)-1)+(cosec(beta)-1)/cot(beta)=4,beta,0,360"],
        ("cot(beta)/(cosec(beta)-1) = (1+sin(beta))/cos(beta)", "(cosec(beta)-1)/cot(beta) = (1-sin(beta))/cos(beta)", "LHS = 2/cos(beta) = 2*sec(beta)", "beta = [60, 300]"),
    ),
    (
        "madas common denominator quartic identity",
        ["--trig", "sin(x)^2*tan(x)+cos(x)^2*cot(x)+2*sin(x)*cos(x)=2,x,0,360"],
        ("Multiply by sin(x)*cos(x)", "(sin(x)^2+cos(x)^2)^2 - 2*sin(x)*cos(x) = 0", "sin(2*x) = 1", "x = [45, 225]"),
    ),
    (
        "madas minor trig ratio powers",
        ["--trig", "cosec(x)/sin(x)^3,method=auto"],
        ("cosec(x)*sin(x)^-3 = cosec(x)^4", "cosec(x)^4"),
    ),
    (
        "madas pythagorean quotient simplification",
        ["--trig", "(1-sin(x)^2)/sin(x)^2,method=auto"],
        ("1-sin(u)^2 = cos(u)^2", "cot(x)^2"),
    ),
    (
        "madas cot sec cancellation",
        ["--trig", "cot(theta)*sec(theta),method=auto"],
        ("cot(theta)*sec(theta) = cosec(theta)", "cosec(theta)"),
    ),
    (
        "nested surd rewrite",
        ["--alg", "rewrite(sqrt(12+sqrt(140)))"],
        ("sqrt(12+sqrt(140)) = sqrt(m)+sqrt(n)", "m+n=12", "sqrt(7)+sqrt(5)"),
    ),
    (
        "log solve exact",
        ["--alg", "solve(log(2,x^2+4*x+3)=4+log(2,x^2+x),x)"],
        ("log(2,(x^2 + 4*x + 3)/(x^2 + x)) = 4", "5*x^2 + 4*x - 1 = 0", "x = [1/5]"),
    ),
    (
        "constant numeric method",
        ["--alg", "1.300567/(pi/3),method=numeric"],
        ("1300567/1000000/(pi/3)", "1.24195000123"),
    ),
    (
        "rational kept root domain",
        ["--alg", "(x^2-16)/(x-4)=14"],
        ("Domain: x - 4 != 0", "10 != 4", "x = [10]"),
    ),
    (
        "removable rational cancellation",
        ["--alg", "solve((x^2-16)/(x-4)=13,x)"],
        ("Domain: x - 4 != 0", "x^2 - 16 = (x - 4)*(x + 4)", "(x^2 - 16)/(x - 4) = x + 4, x - 4 != 0", "x + 4 = 13", "x = 4 rejected by domain", "9 != 4", "x = [9]"),
        ("expand => x^2 - 13*x + 36 = 0", "x = (-b +/- sqrt"),
    ),
    (
        "rational clear denominator steps",
        ["--alg", "(x+1)/(x-2)+(x-2)/(x+1)=5,method=pf"],
        ("Domain: x - 2 != 0", "Domain: x + 1 != 0", "Multiply by x^2 - x - 2", "expand => - 3*x^2 + 3*x + 15 = 0", "x = (-b +/- sqrt(b^2-4ac))/(2a)", "x = [1/2 - sqrt(21)/2, 1/2 + sqrt(21)/2]"),
    ),
    (
        "rational reciprocal domain and clearing",
        ["--alg", "1/(x-1)+1/(x+2)=1,method=auto"],
        ("Domain: x != -2", "x != 1", "(x + 2) + (x - 1) = x^2 + x - 2", "expand => - x^2 + x + 3 = 0", "x = [1/2 - sqrt(13)/2, 1/2 + sqrt(13)/2]"),
    ),
    (
        "negative-power rational exact solve",
        ["--alg", "solve(5/x - x^-2 = 0, x)"],
        ("Domain: x != 0", "Multiply by x^2", "expand => 5*x - 1 = 0", "x = [1/5]"),
        ("x = [0.2]", "Done"),
    ),
    (
        "rational identity common denominator",
        ["--alg", "1/(x+8)+1/(x-8)=2*x/(x^2-64),method=auto"],
        ("Domain: x + 8 != 0 => x != -8", "Domain: x - 8 != 0 => x != 8", "Domain: x^2 - 64 != 0 => x != -8, x != 8", "x^2 - 64 = (x + 8)*(x - 8)", "(x^2 - 64)/(x + 8) = x - 8", "(x^2 - 64)/(x^2 - 64) = 1", "(x - 8) + (x + 8) = 2*x", "expand => 0 = 0", "x in R, x != -8, x != 8"),
    ),
    (
        "absolute linear branch equations",
        ["--alg", "abs(2*x+1)=2,method=auto"],
        ("abs(2*x + 1) = 2", "2*x + 1 = 2 => x = 1/2", "2*x + 1 = -2 => x = -3/2", "x = [-3/2, 1/2]"),
    ),
    (
        "absolute impossible branch",
        ["--alg", "abs(2*x+1)+3=1,method=auto"],
        ("abs(2*x + 1) = -2", "abs(2*x + 1) >= 0", "x = []"),
    ),
    (
        "square equation shows plus minus",
        ["--alg", "solve((x+4)^2=16,x)"],
        ("(x + 4)^2 = 16", "x + 4 = +/-4", "x = [0, -8]"),
        ("Simplify:", "LHS - RHS"),
    ),
    (
        "reciprocal hidden biquadratic",
        ["--alg", "x^2+36/x^2=12,method=auto"],
        ("Domain: x != 0", "Multiply by x^2", "x^4 - 12*x^2 + 36 = 0", "u = x^2", "u = 6", "x = sqrt(6)", "x = -sqrt(6)"),
    ),
    (
        "affine square direct solve",
        ["--alg", "solve((2*x+1)^2=4,x)"],
        ("(2*x + 1)^2 = 4", "2*x + 1 = +/-2", "x = [1/2, -3/2]"),
        ("LHS - RHS", "x + 1/2 = +/-1"),
    ),
    (
        "contradiction maths only",
        ["--alg", "solve((2*x+5)^2=4*x^2+20*x+30,x)"],
        ("4*x^2 + 20*x + 25 = 4*x^2 + 20*x + 30", "25 != 30", "-5 != 0", "x = []"),
        ("Simplify:", "No solution"),
    ),
    (
        "expanded square contradiction",
        ["--alg", "solve((3*x+2)^2=9*x^2+12*x+11,x)"],
        ("9*x^2 + 12*x + 4 = 9*x^2 + 12*x + 11", "4 != 11", "-7 != 0", "x = []"),
    ),
    (
        "constant false equation no fake variable",
        ["--alg", "factorial(4)=0,method=auto"],
        ("24 != 0", "solution = []"),
        ("24 = 0", "x = []"),
    ),
    (
        "constant fraction inverse",
        ["--alg", "inverse((5*x+2)/(5*x+2))"],
        ("y = f(x) = (5*x + 2)/(5*x + 2)", "5*x + 2 != 0", "y = 1", "f^-1(x) = no inverse on all real x"),
    ),
    (
        "affine log inverse",
        ["--alg", "inverse(3-ln(x))"],
        ("y = f(x) = - ln(x) + 3", "y = e^(- x + 3)", "f^-1(x) = e^(- x + 3)"),
    ),
    (
        "affine exp inverse",
        ["--alg", "inverse(2*e^(3*x)-5)"],
        ("y = f(x) = 2*e^(3*x) - 5", "y = ln((x + 5)/2)/3", "f^-1(x) = ln((x + 5)/2)/3"),
    ),
    (
        "divided affine log inverse",
        ["--alg", "inverse((1+ln(x+3))/2)"],
        ("y = f(x) = (ln(x + 3) + 1)/2", "y = e^(2*(x - 1/2)) - 3", "f^-1(x) = e^(2*(x - 1/2)) - 3"),
    ),
    (
        "log of affine exp exact solve",
        ["--alg", "solve(ln(4-2*e^(3*x))=0,x)"],
        ("- 2*e^(3*x) + 4 = 1", "u = e^(3*x), u > 0", "u = 3/2", "x = ln(3/2)/3"),
    ),
    (
        "affine exp interval range",
        ["--alg", "range((e^x+2)/4,x,ln(2),inf)"],
        ("As x=>inf, e^u=>inf", "Range: 1 <= y", "1 <= y"),
    ),
    (
        "affine exp shifted range",
        ["--alg", "range(e^(2*x)-4)"],
        ("e^u > 0 for all real u", "Range: y > -4", "y > -4"),
    ),
    (
        "affine exp negative infinity interval",
        ["--alg", "range(1/2*e^x+1,x,-inf,0)"],
        ("As x=>-inf, e^u=>0", "Range: 1 < y <= 1.5", "1 < y <= 1.5"),
    ),
    (
        "solve interval infinity",
        ["--alg", "solve((x-3)^2+1=17,x,4,inf)"],
        ("Interval: x in [4, inf]", "x = 7", "x = [7]"),
        ("x = -1",),
    ),
    (
        "rational quadratic roots kept",
        ["--alg", "solve(2/(2*x^2+3)=4/7,x)"],
        ("Multiply by 2*x^2 + 3", "x = -1/2", "x = 1/2", "x = [-1/2, 1/2]"),
        ("rejected by domain", "x = []"),
    ),
    (
        "rational quadratic surd denominator parses",
        ["--alg", "solve(1/x-2/3*x=0,x)"],
        ("x = (0 - sqrt(8/3))/(4/3)", "x = (0 + sqrt(8/3))/(4/3)", "x ~= -1.225, 1.225"),
        ("x = []",),
    ),
    (
        "numeric scan rejects asymptote",
        ["--alg", "solve(4/(2*ln(x-1)-3)=-2,x)"],
        ("Domain: 2*ln(x - 1) - 3 != 0", "x = [2.64872127071]"),
        ("5.48168907034",),
    ),
    (
        "restricted quadratic inverse",
        ["--alg", "inverse(4-x^2,x,2,4)"],
        ("y = f(x) = - x^2 + 4", "f^-1(x) = sqrt(- x + 4)"),
        ("Constant/non-monotone",),
    ),
    (
        "half-line quadratic inverse",
        ["--alg", "inverse(x^2+1,x>=0)"],
        ("y = f(x) = x^2 + 1", "f^-1(x) = sqrt(x - 1)", "Given domain: x>=0"),
        ("Constant/non-monotone",),
    ),
    (
        "sqrt exponential inverse",
        ["--alg", "inverse(sqrt(e^x-1),x,0,inf)"],
        ("y = f(x) = sqrt(e^(x) - 1)", "f^-1(x) = ln(x^2 + 1)"),
        ("Constant/non-monotone",),
    ),
    (
        "sqrt linear half-line range",
        ["--alg", "range(sqrt(x+1),x,0,inf)"],
        ("Endpoint gives y = 1", "Range: y >= 1"),
        ("Range: y >= 0",),
    ),
    (
        "affine sqrt interval range",
        ["--alg", "range(sqrt(x)-3,x,0,9)"],
        ("y(0) = -3, y(9) = 0", "Range: -3 <= y <= 0"),
        ("unrestricted", "Range: y >= 0"),
    ),
    (
        "linear fractional endpoint asymptote range",
        ["--alg", "range((3*x+1)/(x+4),x,-4,inf)"],
        ("Vertical asymptote x = -4", "Horizontal asymptote y = 3", "Range: y < 3"),
        ("unrestricted",),
    ),
    (
        "two-term rational factor cancellation",
        ["--alg", "simplify((2*x-1)/(x^2-x-2)-1/(x-2))"],
        ("x^2 - x - 2 = (x - 2)*(x + 1)", "= 1/(x + 1)"),
        ("ERR:",),
    ),
    (
        "open half-line reciprocal range",
        ["--alg", "range(1/(x+1),x>4)"],
        ("Interval of interest: x on (4, inf]", "Range: 0 < y < 1/5"),
        ("Range: y = 1/(x + 1)", "0 < y <= 1/5"),
    ),
    (
        "left open asymptote range",
        ["--alg", "range((x-5)/(x-4),x<4)"],
        ("Vertical asymptote x = 4", "Horizontal asymptote y = 1", "Range: y > 1"),
        ("unrestricted",),
    ),
    (
        "quadratic square exact solve",
        ["--alg", "solve((4-x^2)^2=4,x)"],
        ("- x^2 + 4 = +/-2", "x = [-sqrt(6), -sqrt(2), sqrt(2), sqrt(6)]"),
        ("x = -2.44948974278", "0 + sqrt", "0 - sqrt"),
    ),
    (
        "restricted quadratic left range",
        ["--alg", "range(4*(x+1)^2,x<=-2)"],
        ("Range: y >= 4 on the interval",),
        ("Range: y >= 0",),
    ),
    (
        "restricted quadratic inverse simplified",
        ["--alg", "inverse(4*(x+1)^2,x<=-2)"],
        ("f^-1(x) = - sqrt(x)/2 - 1",),
        ("sqrt(- 16",),
    ),
    (
        "open interval product solve",
        ["--alg", "solve((x-1)*(x-3)=0,x>1)"],
        ("Interval: x in (1, inf]", "x = 1 rejected by interval", "x = [3]"),
        ("x = [1, 3]",),
    ),
    (
        "open half-line quadratic range",
        ["--alg", "range(x^2+2,x>0)"],
        ("Range: y > 2 on the interval",),
        ("Range: y >= 2",),
    ),
    (
        "open half-line linear range",
        ["--alg", "range(3*x-1,x>4)"],
        ("Range: y > 11",),
        ("Range: y >= 11",),
    ),
    (
        "compose wrapper expands quadratic",
        ["--alg", "compose(x^2-2,2*x+3)"],
        ("f(g(x)) = 4*x^2 + 12*x + 7",),
        ("Err:", "(2*x + 3)^2 - 2"),
    ),
    (
        "log exp product solve",
        ["--alg", "solve(4+ln(e*x^2)=6,x>0)"],
        ("ln(e*x^2) = 2*ln(x) + 1", "ln(x) = 1/2", "x = [sqrt(e)]"),
        ("x = []", "x ~= 1.649"),
    ),
    (
        "ln e power solve simplification",
        ["--alg", "solve(ln(e^(2*x+1))+3=10,x)"],
        ("2*x - 6 = 0", "x = [3]"),
        ("x = []",),
    ),
    (
        "log fraction derivative simplification",
        ["--derive", "1/2*ln((x+4)/3),x"],
        ("f' = 1/(x + 4)", "dy/dx = 1/(2*(x + 4))"),
        ("1/(6*(x + 4)/3)",),
    ),
    (
        "affine exp square solve",
        ["--alg", "solve(4*(3*e^t+1)^2=1000^3,t)"],
        ("(3*e^(t) + 1)^2 = 250000000", "e^(t) = (sqrt(250000000) - 1)/3", "t ~= 8.570"),
        ("t = []",),
    ),
    (
        "consecutive exp powers factor",
        ["--alg", "solve(-4*e^(-x)+4*e^(-2*x)+3*e^(-3*x)=0,x)"],
        ("u = e^(-x), u > 0", "- 4*u + 4*u^2 + 3*u^3 = 0", "u*(3*u^2 + 4*u - 4) = 0", "x = [ln(3/2)]"),
        ("all real values in domain",),
    ),
    (
        "inverse sec alias parses",
        ["--derive", "arcsec(x),x"],
        ("y = acos(1/x)", "u = 1/x", "dy/dx = x^-2/sqrt(- (1/x)^2 + 1)"),
        ("arcsec*x",),
    ),
    (
        "radical solve no generic",
        ["--alg", "sqrt(x+5)-sqrt(x-3)=2,method=auto"],
        ("Domain:", "sqrt(x + 5) = 2 + sqrt(x - 3)", "(x + 5) - (x - 3) - 4 = 2*2*sqrt(x - 3)", "x = [4]"),
    ),
    (
        "single radical exact solve",
        ["--alg", "solve(sqrt(2*x+3)=x-1,x)"],
        ("2*x + 3 >= 0 => x >= -3/2", "x - 1 >= 0 => x >= 1", "2*x + 3 = (x - 1)^2", "expand => x^2 - 4*x - 2 = 0", "x = 2 - sqrt(6) rejected by domain", "x = [2 + sqrt(6)]"),
        ("4.449",),
    ),
    (
        "log abs domain proof",
        ["--alg", "domain(log(abs(2*x-3)+2))"],
        ("abs(2*x - 3) + 2 > 0", "abs(2*x - 3) >= 0", "abs(2*x - 3) + 2 >= 2 > 0", "all real x"),
    ),
    (
        "log rational domain sign chart",
        ["--alg", "domain(ln((x-1)/(x+2)))"],
        ("(x - 1)/(x + 2) > 0, x + 2 != 0", "x - 1 = 0 => x = 1, x + 2 = 0 => x = -2", "-2, 1 => x < -2 or x > 1", "x < -2 or x > 1"),
    ),
    (
        "log abs impossible solve",
        ["--alg", "ln(abs(x)+2)=0,method=auto"],
        ("abs(x) + 2 = e^0", "abs(x) + 2 = 1", "abs(x) = -1", "abs(x) >= 0", "x = []"),
        ("log_b(A)-log_b(B)", "|A| = B =>", "No real solution"),
    ),
    (
        "wrapped domain call",
        ["--alg", "(domain(sqrt(log(2,x-9))))"],
        ("base > 1", "Domain: x >= 10", "x >= 10"),
    ),
    (
        "wrapped range call",
        ["--alg", "(range(abs(x-2)+abs(x-3)))"],
        ("roots: x = 2; x = 3", "min y = 1", "Range: y >= 1"),
    ),
    (
        "binomial coefficient line",
        ["--alg", "binomial((1+2*x)^(-1/2),x,0,4)"],
        ("n = -1/2: C(n,0) = 1", "C(n,4) = 35/128", "Terms: 1 - 1/2*(2*x)", "Valid for abs(x) < 1/2", "1 - x + 3/2*x^2 - 5/2*x^3 + 35/8*x^4"),
    ),
    (
        "binomial two-linear PF series",
        ["--alg", "binomial((5*x+3)/((1-x)*(1+3*x)),x,0,3)"],
        ("PF: 2/(- x + 1) + 1/(3*x + 1)", "Valid for abs(x) < 1/3", "3 - x + 11*x^2 - 25*x^3"),
    ),
    (
        "binomial product with shifted power",
        ["--alg", "binomial(2*x/(1+2*x)^3,x,0,4)"],
        ("(2*x + 1)^-3", "Valid for abs(x) < 1/2", "2*x - 12*x^2 + 48*x^3 - 160*x^4"),
    ),
    (
        "binomial combined cancellation",
        ["--alg", "binomial(2*(1+4*x)^(1/2)+4/(1+x),x,0,3)"],
        ("(4*x + 1)^1/2", "(x + 1)^-1", "Valid for abs(x) < 1/4", "6 + 4*x^3"),
    ),
    (
        "exp substitution no generic",
        ["--alg", "2^(2*x)-5*2^x+4=0,method=log_exp"],
        ("u=a^x", "x = [0, 2]"),
    ),
    (
        "natural log quotient exact",
        ["--alg", "solve(ln(2*w+1)=1+ln(w-1),w)"],
        ("ln((2*w + 1)/(w - 1)) = 1", "(2*w + 1)/(w - 1) = e", "2*w + 1 = e*(w - 1)", "w = [(e + 1)/(e - 2)]"),
        ("w = 5.176", "log_b(A)", "No real solution", "ERR:"),
    ),
    (
        "scaled natural log exact",
        ["--alg", "solve(2*ln(x)=6,x)"],
        ("ln(x) = 3", "x = e^(3)", "x = [e^(3)]"),
        ("20.085", "log_b(A)", "ERR:"),
    ),
    (
        "combined natural log coefficient exact",
        ["--alg", "solve(ln(x)+2*ln(x)=5,x)"],
        ("3*ln(x) = 5", "ln(x) = 5/3", "x = e^(5/3)", "x = [e^(5/3)]"),
        ("5.294", "log_b(A)", "ERR:"),
    ),
    (
        "log law collect exact",
        ["--alg", "simplify(2*ln(56)-(ln(168)-ln(3/7)))"],
        ("ln(168) = 3*ln(2) + ln(3) + ln(7)", "ln(3/7) = ln(3) - ln(7)", "= 3*ln(2)", "3*ln(2)"),
        ("simplify*",),
    ),
    (
        "reciprocal exponential substitution exact",
        ["--alg", "solve(e^x+8*e^(-x)=6,x)"],
        ("u = e^(x), u > 0", "u + 8/u - 6 = 0", "- 6*u + u^2 + 8 = 0", "x = [ln(2), ln(4)]"),
        ("0.693", "1.386", "x = all real", "ERR:"),
    ),
    (
        "negative exponent substitution exact",
        ["--alg", "solve(4*e^(-2*x)-e^(-4*x)=0,x)"],
        ("Divide by e^(-4*x) > 0", "e^(2*x) = 1/4", "x = [-ln(2)]"),
        ("x = all real", "No real solution", "ERR:"),
    ),
    (
        "common exponential factor contradiction",
        ["--alg", "solve(2*e^x=e^(x+1),x)"],
        ("e^(x)*(- e + 2) = 0", "e^(x) > 0", "- e + 2 != 0", "x = []"),
        ("x = all real", "log_a", "ERR:"),
    ),
    (
        "nonzero exponential product factor",
        ["--alg", "solve((1-x^2)*e^(-x^2)=0,x)"],
        ("e^(-x^2) > 0", "- x^2 + 1 = 0", "x = [1, -1]"),
        ("x = all real", "ERR:"),
    ),
    (
        "two exponential terms factor",
        ["--alg", "solve(-4*e^(-2*x)+3*e^(-3*x)=0,x)"],
        ("Divide by e^(-3*x) > 0", "e^(x) = 3/4", "x = ln(3/4)"),
        ("all real values", "x = []", "ERR:"),
    ),
    (
        "fractional reciprocal power substitution",
        ["--alg", "solve(t^(1/3)=2+15*t^(-1/3),t)"],
        ("u = t^(1/3); t = u^3", "u^2 - 2*u - 15 = 0", "t = [-27, 125]"),
        ("t = []", "ERR:"),
    ),
    (
        "equal exponentials",
        ["--alg", "solve(e^(2*x+1)=e^(x-3),x)"],
        ("e^(2*x + 1) = e^(x - 3)", "2*x + 1 = x - 3", "x + 4 = 0", "x = -4"),
        ("all real values in domain", "ERR:"),
    ),
    (
        "exp equals constant exact",
        ["--alg", "solve(e^(x+1)=17,x)"],
        ("e^(x + 1) = 17", "x + 1 = ln(17)", "x = ln(17) - 1"),
        ("1.833", "x = [", "ERR:"),
    ),
    (
        "equal exponential quadratic exponents",
        ["--alg", "solve(e^(x^2)=e^(2*x),x)"],
        ("e^(x^2) = e^(2*x)", "x^2 = 2*x", "x^2 - 2*x = 0", "x = [0, 2]"),
        ("all real values in domain", "ERR:"),
    ),
    (
        "sin reverse chain concise",
        ["--int", "sin(x),method=reverse_chain"],
        ("I = Int(sin(x)) dx", "-cos(x) + C"),
    ),
    (
        "affine exp reverse chain",
        ["--int", "exp(2*x-3),method=auto"],
        ("u = 2*x - 3", "du/dx = 2", "I = 1/2*Int(e^u) du", "I = 1/2*e^u + C", "1/2*e^(2*x - 3) + C"),
    ),
    (
        "affine own exp parts simplification",
        ["--int", "(x+1)*e^(x+1),method=parts"],
        ("u = x + 1", "I = Int(u*e^u) du", "Int(u*e^u) du = (u-1)*e^u", "x*e^(x + 1) + C"),
        ("e^(x + 1) + e^(x + 1)*(x - 1) + C", "1*Int"),
    ),
    (
        "defint reciprocal x log x",
        ["--int", "defint(1/(x*ln(x)),x,e,3)"],
        ("u = ln(x)", "I = Int(1/u) du", "Limits: x = e => u = 1, x = 3 => u = ln(3)", "ln(abs(ln(3)))"),
        ("ERR:", "Expected )", "No elementary"),
    ),
    (
        "improper linear quotient before split",
        ["--int", "(3*x^3+2*x^2-3*x+8)/(x+2)"],
        ("Q = - 4*x + 3*x^2 + 5, R = -2", "Int(Q) dx + R*Int(1/D) dx", "5*x - 2*x^2 + x^3 - 2*ln(abs(x + 2)) + C"),
        ("3*x^3*(x + 2)^-1", "x = u - 2", "No elementary"),
    ),
    (
        "reciprocal exp endpoint log collapse",
        ["--int", "defint(e^x/(e^x+1),x,-1,1),method=sub"],
        ("F(1) = ln(abs(e + 1))", "F(-1) = ln(abs(e^(-1) + 1))", "1"),
        ("ln(abs(e + 1)) - ln(abs(e^(-1) + 1))", "No elementary"),
    ),
    (
        "sqrt endpoint power collapse",
        ["--int", "defint(sec(x)^4,x,0,pi/3),method=sub"],
        ("F(pi/3) = 2*sqrt(3)", "2*sqrt(3)"),
        ("sqrt(3) + 1/3*sqrt(3)^3", "No elementary"),
    ),
    (
        "defint uses special substitution primitive",
        ["--int", "defint(e^(2*x)/(e^(x)+1),x,0,1),method=sub"],
        ("u = e^x + 1", "I = u - ln(abs(u)) + C", "F(1) - F(0)", "e - ln(abs(e + 1)) + ln(2) - 1"),
        ("ERR:", "Expected )", "No elementary", "Integrate; back-substitute"),
    ),
    (
        "sqrt substitution maths lines",
        ["--int", "1/(4*sqrt(x)*sqrt(sqrt(x)-1)),method=sub"],
        ("u = sqrt(x)", "I = Int(1/(2*sqrt(u-1))) du", "I = sqrt(sqrt(x) - 1) + C"),
        ("Integrate", "Simplify to", "No elementary"),
    ),
    (
        "product derivative exp tan",
        ["--int", "e^x*(tan(x)+sec(x)^2)"],
        ("d/dx(e^x tan(x)) = e^x tan(x) + e^x sec(x)^2", "e^x*tan(x) + C"),
        ("No elementary", "ERR:"),
    ),
    (
        "product to sum definite sin cos2x",
        ["--int", "defint(sin(x)*cos(2*x),x,0,pi/6)"],
        ("sin(x)cos(2*x) = 1/2*(sin(3*x)-sin(x))", "1/4*sqrt(3) - 1/3"),
        ("3/16", "No elementary", "ERR:"),
    ),
    (
        "x trig square volume reduction",
        ["--int", "defint(pi*x*cos(2*x)^2,x,0,pi/4)"],
        ("cos(2*x)^2 = (1+cos(2*2*x))/2", "K = Int(1/2*x*cos(4*x))", "1/64*pi^3 - 1/16*pi"),
        ("ERR:", "No elementary primitive found"),
    ),
    (
        "x2 trig square avoids false reverse chain",
        ["--int", "pi*x^2*cos(2*x)^2"],
        ("I = pi*J, J = Int(x^2*cos(2*x)^2) dx", "DI table for x^2*cos(2*2*x)", "+ C"),
        ("I = -1/2*Int(u^(2)) du", "-1/6*cos(2*x)^3 + C"),
    ),
    (
        "xn log square parts",
        ["--int", "defint(pi*x^2*ln(x)^2,x,1,e)"],
        ("u = ln(x)^2, dv = x^2 dx", "repeated parts: Int(x^n ln(x)^2)dx", "5/27*pi*e^(3) - 2/27*pi"),
        ("ERR:", "No elementary primitive found"),
    ),
    (
        "simple substitution keeps all factors",
        ["--int", "e^x*cos(x)^2*sin(x)"],
        ("e^(x)*cos(x)^2*sin(x)",),
        ("-1/3*cos(x)^3 + C", "trig reverse-chain substitution"),
    ),
    (
        "exp cos sin double substitution",
        ["--int", "defint(e^(cos(x))*sin(2*x),x,0,pi/2)"],
        ("sin(2*x) = 2*sin(x)*cos(x)", "u = cos(x)", "Int(u*e^u) du = e^u*(u-1)", "F(pi/2) - F(0)", "2"),
        ("ERR:", "2*e - 2"),
    ),
    (
        "sincos over cos2 square volume",
        ["--int", "defint(pi*((4+sin(x)*cos(x))/cos(2*x))^2,x,pi/12,pi/6)"],
        ("sin(x)*cos(x) = 1/2*sin(2*x)", "(sin(x)*cos(x) + 4)/(cos(2*x)) = 4*sec(2*x) + 1/2*tan(2*x)", "square = 65/4*sec(2*x)^2 + 4*sec(2*x)*tan(2*x) - 1/4", "49/12*pi*sqrt(3) + 4*pi - 1/48*pi^2"),
        ("ERR:", "No elementary primitive found"),
    ),
    (
        "sqrt e endpoint parts",
        ["--int", "defint(16*x^3*ln(x),x,sqrt(e),e),method=parts"],
        ("F(sqrt(e)) = 2*e^2 - e^2", "3*e^(4) - e^2"),
        ("sqrt(e)^4*ln(sqrt(e))", "No elementary", "ERR:"),
    ),
    (
        "exp denominator linear definite",
        ["--int", "defint(6/e^(2-3*x),x,k,1/2)"],
        ("6/e^(- 3*x + 2) = 6*e^(3*x - 2)", "u = 3*x - 2", "F(1/2) - F(k)", "2*e^(-1/2) - 2*e^(3*k - 2)"),
        ("ERR:", "Expected )", "No elementary"),
    ),
    (
        "affine trig reverse chain",
        ["--int", "sin(2*x-3),method=auto"],
        ("u = 2*x - 3", "du/dx = 2", "I = 1/2*Int(sin(u)) du = -cos(u)", "-1/2*cos(2*x - 3) + C"),
    ),
    (
        "cot integral substitution",
        ["--int", "cot(x)"],
        ("cot(u) = cos(u)/sin(u)", "v = sin(u), dv = cos(u) du", "Int(cot(u))du = Int(1/v)dv", "ln(abs(sin(x))) + C"),
    ),
    (
        "sec integral substitution",
        ["--int", "sec(x)"],
        ("v = sec(u)+tan(u)", "dv = sec(u)(sec(u)+tan(u)) du", "Int(sec(u))du = Int(1/v)dv", "ln(abs(sec(x) + tan(x))) + C"),
    ),
    (
        "tan square integral line",
        ["--int", "tan(x)^2"],
        ("tan(x)^2 = sec(x)^2 - 1", "I = Int(sec(x)^2 - 1) dx", "tan(x) - x + C"),
    ),
    (
        "affine tan square substitution",
        ["--int", "tan(3*x+1)^2"],
        ("u = 3*x + 1", "du/dx = 3", "I = 1/3*Int(sec(u)^2-1)du", "I = (tan(u)-u)/3+C", "tan(3*x + 1)/3 - x + C"),
    ),
    (
        "product to sum sin cos integral",
        ["--int", "sin(3*x)*cos(5*x),method=trig"],
        ("A = 3*x, B = 5*x", "2sin(A)cos(B) = sin(A+B)+sin(A-B)", "I = 1/2*Int(sin(8*x)-sin(2*x))dx", "-1/16*cos(8*x) + 1/4*cos(2*x) + C"),
        ("No elementary primitive",),
    ),
    (
        "sin square integral line",
        ["--int", "sin(x)^2"],
        ("sin(x)^2 = (1-cos(2*x))/2", "I = 1/2*Int(1-cos(2*x)) dx", "x/2 - sin(2*x)/4 + C"),
    ),
    (
        "cos square integral line",
        ["--int", "cos(x)^2"],
        ("cos(x)^2 = (1+cos(2*x))/2", "I = 1/2*Int(1+cos(2*x)) dx", "x/2 + sin(2*x)/4 + C"),
    ),
    (
        "tan square plus one integral line",
        ["--int", "tan(x)^2+1"],
        ("tan(u)^2 + 1 = sec(u)^2", "integrand = sec(x)^2", "I = Int(sec(x)^2) dx", "tan(x) + C"),
    ),
    (
        "reciprocal trig identity before integration",
        ["--int", "(cosec(x)^2-cot(x)^2)*exp(x)"],
        ("cosec(x)^2 - cot(x)^2 = 1", "integrand = e^(x)", "I = Int(e^(x)) dx", "e^(x) + C"),
    ),
    (
        "high even trig power identity",
        ["--int", "sin(4*x-pi/6)^4*cos(4*x-pi/6)^4,method=trig"],
        ("Power-reduction", "sin^4(u)cos^4(u)=(3-4cos(4u)+cos(8u))/128", "+ C"),
        ("Done", "Simplify:"),
    ),
    (
        "defint wrapped variable",
        ["--int", "defint(e^(2*x)/(1+e^(2*x)),(x),0,ln(6))"],
        ("I = Int(e^(2*x)/(e^(2*x) + 1)) dx", "u = e^(2*x) + 1", "F(ln(6)) - F(0)", "ln(37/2)"),
        ("ERR:",),
    ),
    (
        "defint outer wrapped",
        ["--int", "(defint(2*x/sqrt(2*x+4),x,(0),(7)))"],
        ("u^2 = 2*x + 4", "x = 0 => u = 2, x = 7 => u = 3*sqrt(2)", "I = Int_2^(3*sqrt(2)) (u^2 - 4) du"),
        ("ERR:",),
    ),
    (
        "defint expands polynomial power",
        ["--int", "defint((4-x^2)^2,x,0,2)"],
        ("(- x^2 + 4)^2 = x^4 - 8*x^2 + 16", "F(2) - F(0)", "256/15"),
    ),
    (
        "volume wrapper x-axis",
        ["--int", "volume_x(4-x^2,x,-2,2)"],
        ("V = pi*Int(y^2) dx", "(- x^2 + 4)^2 = x^4 - 8*x^2 + 16", "512*pi/15"),
    ),
    (
        "area between wrapper exact logs",
        ["--int", "area_between(6+8*e^(-3*x),e^(3*x)-1,x,0,ln(2))"],
        ("A = Int(upper-lower) dx", "F(ln(2)) - F(0)", "7*ln(2)"),
    ),
    (
        "trapezium rule table lines",
        ["--int", "trapezium(1/(1+x),x,0,1,4)"],
        ("h = (1-0)/4 = 0.25", "x_i = 0, 0.25, 0.5, 0.75, 1", "T = h/2", "0.697024"),
    ),
    (
        "simpson rule table lines",
        ["--int", "simpson(e^(-x^2),x,0,1,4)"],
        ("h = (1-0)/4 = 0.25", "S = h/3", "0.746855"),
    ),
    (
        "midordinate rule table lines",
        ["--int", "midordinate(sin(x),x,0,pi,6)"],
        ("h = (pi-0)/6 = 0.523599", "x_mid =", "M = h*sum(y_mid)", "2.02303"),
    ),
    (
        "trapezium table x-y lines",
        ["--int", "trapezium_table([1,2.25,3.5,4.75,6],[9,17,25,21,13])"],
        ("x_i = 1, 2.25, 3.5, 4.75, 6", "h = 1.25", "T = h/2", "92.5"),
    ),
    (
        "simpson table x-y lines",
        ["--int", "simpson_table([0,1,2,3,4],[1,4,9,16,25])"],
        ("x_i = 0, 1, 2, 3, 4", "S = h/3", "41.333333"),
    ),
    (
        "generic denominator derivative log rule",
        ["--int", "x^3/(x^4+2),method=reverse_chain"],
        ("u = x^4 + 2", "du/dx = 4*x^3", "1/4*ln(abs(x^4 + 2)) + C"),
    ),
    (
        "trig denominator derivative log rule",
        ["--int", "4*sec(x)^2/tan(x),method=reverse_chain"],
        ("u = tan(x)", "du/dx = sec(x)^2", "4*ln(abs(tan(x))) + C"),
    ),
    (
        "sqrt denominator reverse chain",
        ["--int", "6/sqrt(3*x+1)"],
        ("6/sqrt(3*x + 1) = 6*(3*x + 1)^(-1/2)", "u = 3*x + 1", "4*sqrt(3*x + 1) + C"),
    ),
    (
        "power derivative reverse chain",
        ["--int", "3*x*sqrt(x^2+1),method=sub"],
        ("u = x^2 + 1", "du/dx = 2*x", "J = 1/2*Int(u^(1/2)) du", "J = 1/3*u^(3/2) + C", "(x^2 + 1)^(3/2) + C"),
    ),
    (
        "cos reverse chain concise",
        ["--int", "cos(x),method=reverse_chain"],
        ("I = Int(cos(x)) dx", "sin(x) + C"),
    ),
    (
        "linear division concise",
        ["--int", "x/(x+1),method=div"],
        ("N/D = Q + R/D", "Q = 1", "R = -1", "x - ln(abs(x + 1)) + C"),
    ),
    (
        "substitution spacing",
        ["--int", "sin(3*x+2)"],
        ("u = 3*x+2; du = 3 dx; dx = du/3", "I = 1/3*Int(sin(u)) du"),
        ("u=3*x+2", "du=3 dx", "dx=du/3", "I=1/3"),
    ),
    (
        "sec power no prose",
        ["--int", "sec(x)^4"],
        ("sec(x)^4 = sec(x)^2*sec(x)^2", "u = tan(x); du = sec(x)^2 dx", "I = Int(1+u^2) du"),
        ("Write ", "u=tan", "du=sec", "I=Int"),
    ),
    (
        "parts no prose",
        ["--int", "x*sin(x),method=parts"],
        ("u=x", "dv=sin(x) dx", "du=dx", "v=-cos(x)", "sin(x) - x*cos(x) + C"),
    ),
    (
        "pf coefficient equations",
        ["--int", "(5*x+7)/((x-1)*(x^2+2)),method=pf"],
        ("A/(x - 1)+(Bx+C)/(x^2 + 2)", "coeff x^2: A+B=0", "coeff x: -B+C=5", "coeff 1: 2*A-C=7", "B=-A", "C=5-A", "3*A-5=7", "3*A=12", "A=4, B=-4, C=1", "PF = 4/(x - 1)+(- 4*x + 1)/(x^2 + 2)", "I = 4*Int(1/(x - 1)) dx + Int((- 4*x + 1)/(x^2 + 2)) dx"),
    ),
    (
        "pf zero coefficient line",
        ["--int", "2/(x*(x^2-1)),method=pf"],
        ("C=0", "-2*ln(abs(x)) + ln(abs(x^2 - 1)) + C"),
    ),
    (
        "definite pf combines endpoint logs",
        ["--int", "defint(4/((2*x+1)*(1-2*x)),x,0,1/4)"],
        ("F(1/4) = ln(3)", "ln(3)"),
    ),
    (
        "definite pf root log simplification",
        ["--int", "defint((x^2+x+2)/(x^2+2*x-3),x,2,3)"],
        ("F(3) - F(2)", "1 + ln(25/18)"),
    ),
    (
        "pf denominator scalar in definite integral",
        ["--int", "defint((5*x^2-8*x+1)/(2*x*(x-1)^2),x,4,9)"],
        ("A = 4, B = -2, C = 1", "-5/24 + ln(32/3)"),
    ),
    (
        "invalid derive no fake working",
        ["--derive", "diff("],
        ("Err:",),
    ),
    (
        "invalid int method no auto working",
        ["--int", "x^2,method=not_a_method"],
        ("Err: invalid method",),
    ),
    (
        "sign derivative branch",
        ["--derive", "sign(sin(x)),x"],
        ("u = sin(x)", "u != 0 => dy/dx = 0", "u = 0 => dy/dx undefined"),
    ),
    (
        "sign branch in sum",
        ["--derive", "sin(x)+sign(x^2-1),x"],
        ("d/dx(sin(x)) = cos(x)", "u = x^2 - 1", "u = 0 => x = +/-1", "x != +/-1 => d/dx(sign(u)) = 0", "x = +/-1 => d/dx(sign(u)) undefined", "dy/dx = cos(x)"),
    ),
    (
        "atan square non elementary",
        ["--int", "atan(2*x+3)^2,method=parts"],
        ("u = 2*x + 3, du = 2 dx", "I = 1/2*Int(atan(u)^2) du", "Int(t^2*sec(t)^2)dt = t^2*tan(t)-Int(2t*tan(t))dt", "Int(2t*tan(t))dt = -2t*ln(abs(cos(t)))+2*Int(ln(abs(cos(t))))dt", "No elementary primitive found"),
    ),
    (
        "trig fallback no fake working",
        ["--trig", "cos(x)+cos(2*x)+cos(3*x)=0,x,0,2*pi,10,method=identity"],
        ("x = []",),
    ),
    (
        "trig tautology interval",
        ["--trig", "sin(8*x)=sin(8*x),x,0,2*pi,32,method=identity"],
        ("A = 8*x, B = 8*x", "A = B", "0 <= x <= 2*pi"),
        ("x = [",),
    ),
    (
        "trig simple general then filter",
        ["--trig", "sin(x)=-1,x,0,360"],
        ("sin(x) = -1", "x = 270 + n*360", "0 <= x <= 360", "x = [270]"),
    ),
    (
        "trig sum product branches",
        ["--trig", "cos(2*x)+cos(x)=0,x,0,360"],
        ("2*cos(3*x/2)*cos(x/2) = 0", "cos(3*x/2) = 0 => x = 60 + n*120", "cos(x/2) = 0 => x = 180 + n*360", "0 <= x <= 360", "x = [60, 180, 300]"),
    ),
    (
        "trig sin cos tan reduction",
        ["--trig", "sin(2*x)+cos(2*x)=0,x,-180,180"],
        ("A = 2*x", "cos(A)=0 => sin(A)+cos(A)=+/-1 != 0", "(sin(A)+cos(A))/cos(A)=0/cos(A)", "tan(2*x)=sin(2*x)/cos(2*x)", "tan(2*x)=-1", "x = -22.5 + n*90", "x = [-112.5, -22.5, 67.5, 157.5]"),
    ),
    (
        "trig rad interval n filter",
        ["--trig", "sin(2*x)+cos(2*x)=0,x,0,pi"],
        ("x = -pi/8 + n*pi/2", "0 <= x <= pi", "x = [3*pi/8, 7*pi/8]"),
    ),
    (
        "trig cos double substitution clean",
        ["--trig", "cos(2*x)=1/2,x,0,360"],
        ("u = cos(x)", "cos(2*x) = 2u^2-1", "2u^2 - 1.5 = 0", "x = [30, 150, 210, 330]"),
        ("2u^2u",),
    ),
    (
        "trig tan rational cos quadratic",
        ["--trig", "4*tan(theta)^2*cos(theta)=15,theta,0,360"],
        ("tan(theta) = s/c", "c!=0; 4s^2-15c = 0", "s^2 = 1-c^2", "-4c^2-15c+4 = 0", "theta = [75.5224878141, 284.477512186]"),
        ("theta = []", "cos(theta) = 0"),
    ),
    (
        "trig quotient tan linear",
        ["--trig", "(sin(x)-cos(x))/cos(x)=2,x,0,360"],
        ("s-3c = 0", "s/c=3", "tan(x) = 3", "x = [71.5650511771, 251.565051177]"),
        ("x = []", "tan(A)"),
    ),
    (
        "tan same function degree family",
        ["--trig", "tan(2*x)-tan(x)=0,x,-180,180,10,method=identity"],
        ("tan(theta+180n)=tan(theta) => A=B+180n", "cos(A)!=0, cos(B)!=0", "2*x = x+180n", "x = [-180, 0, 180]"),
    ),
    (
        "cot power via tan substitution",
        ["--trig", "8*tan(phi)=cot(phi)^2,phi,0,2*pi,8"],
        ("u = tan(phi)", "cot(phi) = 1/u", "8u^3 - 1 = 0", "u = 1/2", "phi = [0.463647609001, 3.60524026259]"),
        ("Failed to isolate", "phi = []"),
    ),
    (
        "cosec power via sin substitution",
        ["--trig", "27*sin(phi)^2+8*cosec(phi)=0,phi,0,2*pi,8"],
        ("u = sin(phi)", "cosec(phi) = 1/u", "27u^3 + 8 = 0", "u = -2/3", "phi = [3.87132030982, 5.55345765095]"),
        ("Failed to isolate", "phi = []"),
    ),
    (
        "sec squared via cos substitution",
        ["--trig", "27*cos(phi)=sec(phi)^2,phi,0,2*pi,8"],
        ("u = cos(phi)", "sec(phi) = 1/u", "27u^3 - 1 = 0", "u = 1/3", "phi = [1.23095941734, 5.05222588984]"),
        ("Failed to isolate", "phi = []"),
    ),
    (
        "sec plus cos half",
        ["--trig", "sec(theta)+cos(theta)=5/2,theta,0,360"],
        ("u = cos(theta)", "sec(theta) = 1/u", "u^2 - 2.5u + 1 = 0", "theta = [60, 300]"),
        ("Failed to isolate", "theta = []"),
    ),
    (
        "sec sin square via cos substitution",
        ["--trig", "2*sec(theta)-1=2*sec(theta)*sin(theta)^2,theta,0,180"],
        ("u = cos(theta)", "sec(theta) = 1/u", "sin(theta)^2 = 1-u^2", "2u^2-u = 0", "Reject u=0", "theta = [60]"),
        ("Failed to isolate", "theta = []"),
    ),
    (
        "cot common denominator",
        ["--trig", "cos(x)*cot(x)+sin(x)+2*cot(x)=0,x,0,360"],
        ("Multiply by sin(x)", "sin(x)^2+cos(x)^2 = 1", "1 + 2*cos(x) = 0", "x = [120, 240]"),
        ("Failed to isolate", "x = []"),
    ),
    (
        "cosec sec common denominator",
        ["--trig", "(cosec(y)-sin(y))*sec(y)^2=2,y,0,pi,8"],
        ("Multiply by sin(y)*cos(y)^2", "1-sin(y)^2 = cos(y)^2", "cos(y)^2*(1 - 2*sin(y)) = 0", "y = [pi/6, 5*pi/6]"),
        ("Failed to isolate", "y = []"),
    ),
    (
        "cosec cot common denominator",
        ["--trig", "cosec(phi)-sin(phi)+2*cos(phi)^2*cot(phi)=0,phi,0,2*pi,8"],
        ("Multiply by sin(phi)", "1-sin(phi)^2 = cos(phi)^2", "cos(phi)^2*(1 + 2*cos(phi)) = 0", "phi = [pi/2, 2*pi/3, 4*pi/3, 3*pi/2]"),
        ("Failed to isolate", "phi = []"),
    ),
    (
        "sec cosec cubic ratio",
        ["--trig", "sec(x)-cos(x)=8*(cosec(x)-sin(x)),x,0,360"],
        ("Multiply by sin(x)*cos(x)", "sin(x)^3 = 8*cos(x)^3", "tan(x) = 2", "x = [63.4349488229, 243.434948823]"),
        ("Failed to isolate", "x = []"),
    ),
    (
        "cot cosec sec common denominator",
        ["--trig", "2*cot(y)-3*cosec(y)=2*sec(y)*cosec(y),y,0,2*pi,8"],
        ("Multiply by sin(y)*cos(y)", "u = cos(y)", "2u^2-3u-2 = 0", "y = [2*pi/3, 4*pi/3]"),
        ("Failed to isolate", "y = []"),
    ),
    (
        "sec cos tan product",
        ["--trig", "(1+sec(phi))*(1-cos(phi))=tan(phi),phi,0,2*pi,8"],
        ("sec(phi) = 1/cos(phi)", "tan(phi) = sin(phi)/cos(phi)", "sin(phi)*(sin(phi)-1) = 0", "sin(phi) = 1 rejected", "phi = [0, pi, 2*pi]"),
        ("Failed to isolate", "phi = []"),
    ),
    (
        "tan double-angle fraction route",
        ["--trig", "(2*sin(x)*cos(x))/(cos(x)^2-sin(x)^2)=sin(x)/cos(x),x,0,180"],
        ("tan(2*x) = 2*sin(x)*cos(x)/(cos(x)^2 - sin(x)^2)", "tan(2*x) = tan(x)", "x = [0, 180]"),
    ),
]

TEXT_WORDS = (
    "Use ",
    "Start with",
    "Differentiate",
    "Simplify",
    "Apply",
    "Let ",
    "Since ",
    "Here ",
    "Then ",
    "Hence ",
    "Therefore",
    "looping integration",
    "Substitute ",
    "Collect ",
    "Inside ",
    "For ",
)

GLOBAL_TEXT_BANS = (
    "Method:",
    "Route:",
    "Answer:",
    "Auto result:",
    "Answer (3 d.p.):",
    "Chk:",
    "Done",
    "pick rule",
    "chain/prod",
    "Std form",
    "Rule/sub/id",
    "Verify",
    "Rearrange to lhs-rhs=0",
    "Use log/exp laws",
    "Exponentiate, solve",
    "State radical domain",
    "Solve the resulting equation",
    "Solve the polynomial in u",
    "substitution/factorisation/rearrangement",
    "u=inner",
    "alpha=base angle",
    "Apply the matching derivative rule set",
    "reverse chain sine rule",
    "reverse chain cosine rule",
    "constant times a polynomial part",
    "ln(abs(...)) for the reciprocal-linear term",
    "the split terms back",
    "Divide polynomial numerator",
)

STANDALONE_LINE_BANS = {
    "A =",
    "B =",
    "C =",
    "y =",
    "u =",
    "v =",
    "w =",
    "I =",
    "dy/d",
    "dx/d",
    "du/dx =",
    "dx/du =",
}

PACK_BANS = (
    "combine logs",
    "solve each case",
    "factor/rearrange",
    "std integral",
    "u-sub/IBP/PF if needed",
    "valid syntax",
    "d/dx rule -> simplify",
    "isolate trig fn",
    "CAST/base angles",
    "method route",
)


def numbered_lines(out: str):
    return [line for line in out.splitlines() if line[:1].isdigit() and ". " in line]


def compact_equals(s: str) -> str:
    return s.replace(" = ", "=").replace("= ", "=").replace(" =", "=")


def run_case(name, args, required, forbidden=()):
    proc = subprocess.run([str(HOST), *args], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout
    bad = []
    expects_error = any(marker.startswith("Err") for marker in required)
    if proc.returncode and not expects_error:
        bad.append(f"exit {proc.returncode}")
    for word in GLOBAL_TEXT_BANS:
        if word in out:
            bad.append(f"user-facing header: {word}")
    for line in (ln.strip() for ln in out.splitlines()):
        if line in STANDALONE_LINE_BANS:
            bad.append(f"standalone fragment: {line}")
    for word in forbidden:
        if word in out:
            bad.append(f"bad formatting: {word}")
    for line in numbered_lines(out):
        for word in TEXT_WORDS:
            if word in line:
                bad.append(f"text in working: {line}")
                break
    compact_out = compact_equals(out)
    for marker in required:
        if compact_equals(marker) not in compact_out:
            bad.append(f"missing: {marker}")
    if bad:
        print(f"FAIL {name}: " + "; ".join(bad))
        print(out)
        return False
    print(f"PASS {name}")
    return True


def main():
    ok = True
    for case in CASES:
        ok = run_case(*case) and ok
    pack = "\n".join(
        p.read_text(errors="replace")
        for p in sorted((ROOT / "c++" / "prizm" / "help").glob("CASIOCAS*.TPL"))
    )
    for marker in PACK_BANS:
        if marker in pack:
            print(f"FAIL pack generic: {marker}")
            ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

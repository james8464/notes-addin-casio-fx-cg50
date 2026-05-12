#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, str, list[str], list[str]]] = [
    (
        "trig",
        "sec(x)^2-tan(x)^2=1,x,0,2*pi,10,method=identity",
        ["LHS-RHS=0", "all valid x in domain"],
        ["x = []", "ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "cos(2*x)+cos(x)=0,x,0,360",
        ["2*cos(3*x/2)*cos(x/2) = 0", "cos(3*x/2) = 0 => x = 60 + n*120", "cos(x/2) = 0 => x = 180 + n*360", "x = [60, 180, 300]"],
        ["Solve both factors", "ERR:"],
    ),
    (
        "trig",
        "sin(2*x)-sin(x)=0,x,0,360",
        ["sin(A)-sin(B) = 0", "sin(A)-sin(B) = 2*cos((A+B)/2)*sin((A-B)/2)", "x = [0, 60, 180, 300, 360]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "solve(log(2,x)+log(4,x)=6,x)",
        ["u = log(2,x)", "log(4,x) = u/2", "x = 16"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "solve(log(2,x^2+4*x+3)=4+log(2,x^2+x),x)",
        [
            "log(2,(x^2 + 4*x + 3)/(x^2 + x)) = 4",
            "(x^2 + 4*x + 3)/(x^2 + x) = 16",
            "5*x^2 + 4*x - 1 = 0",
            "x = -1 rejected by domain",
            "x = [1/5]",
        ],
        ["Rearrange to lhs-rhs=0", "combine log terms", "Exponentiate, solve", "x = 0.2", "ERR:"],
    ),
    (
        "alg",
        "solve((x^2-16)/(x-4)=13,x)",
        [
            "Domain: x - 4 != 0",
            "x^2 - 16 = (x - 4)*(x + 4)",
            "(x^2 - 16)/(x - 4) = x + 4, x - 4 != 0",
            "x + 4 = 13",
            "x = 4 rejected by domain",
            "9 != 4",
            "x = [9]",
        ],
        ["expand => x^2 - 13*x + 36 = 0", "x = (-b +/- sqrt", "ERR:"],
    ),
    (
        "alg",
        "solve((3*x+2)^2=9*x^2+12*x+11,x)",
        ["9*x^2 + 12*x + 4 = 9*x^2 + 12*x + 11", "4 != 11", "-7 != 0", "x = []"],
        ["ERR:"],
    ),
    (
        "alg",
        "solve((2*x+1)^2=4,x)",
        ["(2*x + 1)^2 = 4", "2*x + 1 = +/-2", "x = [1/2, -3/2]"],
        ["LHS - RHS", "x + 1/2 = +/-1", "ERR:"],
    ),
    (
        "alg",
        "x^2+36/x^2=12,method=auto",
        ["Domain: x != 0", "Multiply by x^2", "x^4 - 12*x^2 + 36 = 0", "u = x^2", "u = 6", "x = sqrt(6)", "x = -sqrt(6)"],
        ["LHS-RHS = 0", "x = []", "ERR:"],
    ),
    (
        "alg",
        "inverse((5*x+2)/(5*x+2))",
        ["y = f(x) = (5*x + 2)/(5*x + 2)", "5*x + 2 != 0", "y = 1", "f^-1(x) = no inverse on all real x"],
        ["ERR:"],
    ),
    (
        "derive",
        "sec(x)+cot(x)+csc(x),x",
        ["d/dx(sec(x)) = sec(x)*tan(x)", "d/dx(cot(x)) = -cosec(x)^2", "d/dx(cosec(x)) = -cosec(x)*cot(x)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "cosec(x)^2-cot(x)^2,x,method=auto",
        ["cosec(u)^2 - cot(u)^2 = 1", "Domain: sin(x) != 0", "dy/dx = 0"],
        ["y = cosec", "ERR:", "Unexpected token"],
    ),
    (
        "int",
        "acos((x-1)/3)",
        ["Use parts", "w=(x - 1)/3", "Answer:"],
        ["Answer: int(", "Classify the integrand", "ERR:", "Unexpected token"],
    ),
    (
        "int",
        "asin((x-1)/3)",
        ["Use parts", "w=(x - 1)/3", "Answer:"],
        ["Answer: int(", "Classify the integrand", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(sqrt(abs(3*x+1)+1))",
        ["abs(3*x + 1) >= 0", "Answer: y >= 1"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(log(abs(2*x-3)+2))",
        ["abs(2*x - 3) + 2 > 0", "abs(2*x - 3) >= 0", "abs(2*x - 3) + 2 >= 2 > 0", "Answer: all real x"],
        ["Answer: abs(2*x - 3) + 2 > 0", "ERR:"],
    ),
    (
        "alg",
        "ln(abs(x)+2)=0,method=auto",
        ["abs(x) + 2 = e^0", "abs(x) + 2 = 1", "abs(x) = -1", "abs(x) >= 0", "x = []"],
        ["log_b(A)-log_b(B)", "|A| = B =>", "No real solution", "ERR:"],
    ),
    (
        "alg",
        "log(10,abs(x+5)+3)=0,method=auto",
        ["abs(x + 5) + 3 = 10^0", "abs(x + 5) + 3 = 1", "abs(x + 5) = -2", "x = []"],
        ["log_b(A)-log_b(B)", "|A| = B =>", "ERR:"],
    ),
    (
        "alg",
        "log(3,x^2+2)=0,method=auto",
        ["x^2 + 2 = 3^0", "x^2 + 2 = 1", "x^2 + 1 = 0", "b^2 - 4ac = -4 < 0", "x = []"],
        ["(0 + sqrt(4)*i)/2", "Answer:", "ERR:"],
    ),
    (
        "derive",
        "ln(x+y)=x*y,x,method=implicit",
        ["d/dx[ln(x + y)] = (1 + dy/dx)/(x + y)", "d/dx[x*y] = y + x*dy/dx", "(1 + dy/dx)/(x + y) = y + x*dy/dx", "dy/dx*(1 - x*(x + y)) = y*(x + y) - 1", "dy/dx = (y*(x + y) - 1)/(1 - x*(x + y))"],
        ["F_x", "F_y", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "x^y=y^x,x,method=implicit",
        ["ln(x^y)=ln(y^x)", "y*ln(x)=x*ln(y)", "dy/dx*ln(x)-x*dy/dx/y=ln(y)-y/x", "dy/dx=(ln(y)-y/x)/(ln(x)-x/y)", "dy/dx = y*(x*ln(y) - y)/(x*(y*ln(x) - x))"],
        ["F_x", "F_y", "-F_x/F_y", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "sin(x*y)+x^2=y^2,x,method=implicit",
        ["F_x + F_y*dy/dx = 0", "(cos(x*y)*y + 2*x) + (cos(x*y)*x - 2*y)*dy/dx = 0", "(cos(x*y)*x - 2*y)*dy/dx = -(cos(x*y)*y + 2*x)", "dy/dx = -(cos(x*y)*y + 2*x)/(cos(x*y)*x - 2*y)", "dy/dx = (y*cos(x*y) + 2*x)/(2*y - x*cos(x*y))"],
        ["dy/dx = -F_x/F_y", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "arccos(cos(2*x+pi/6)),x,method=auto",
        ["u = cos(2*x + pi/6)", "du/dx = -2*sin(2*x + pi/6)", "dy/dx = -du/dx/sqrt(1-u^2)"],
        ["Use quotient rule", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "log(abs(3*x+1)+2),x,method=auto",
        ["u = abs(3*x + 1) + 2", "du/dx = 3*(3*x + 1)/abs(3*x + 1)", "dy/dx = du/dx/u", "d(abs(u))/dx = u/abs(u)*du/dx"],
        ["Use product rule", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "ln(x),x",
        ["y = ln(x)", "dy/dx = 1/x"],
        ["y = log(x)", "ERR:"],
    ),
    (
        "derive",
        "log((3*x+1)^2+2)*log(3,x^2+12)*7!,x",
        ["c = 5040", "f1 =", "f1' =", "f2 =", "f2' =", "dy/dx = c*(f1'*f2 + f1*f2')"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "(abs(2*x+1))*(cot(3*x^2+5)+1)*(sin((x+1)^2)+cos((x+1)^2))*(7*x-2)^3*ln(e^(x+3)+3),x",
        ["f1 = abs(2*x + 1)", "f3' = 2*cos((x + 1)^2)*(x + 1) - 2*sin((x + 1)^2)*(x + 1)", "f5' = 1/(e^(x + 3) + 3)*e^(x + 3)", "dy/dx = f1'*f2*f3*f4*f5 +"],
        ["sum(f_i'", "...", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "x=t^2+1/t,y=t^2-1/t,t,x,method=param",
        ["dx/dt", "dy/dt", "dy/dx=(dy/dt)/(dx/dt)", "dy/dx"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "mode:3,t^2+4*t,t^2-3*t^2,t",
        ["dy/dt = 2*t - 6*t", "dy/dt = -4*t", "dy/dx = (-4*t)/(2*t+4) = -2*t/(t+2)", "dy/dx = -2*t/(t+2)"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "mode:3,t^2+1/t,t^2-1/t,t",
        ["dy/dx = (2*t + t^-2)/(2*t - t^-2)", "dy/dx = ((2*t + t^-2)*t^2)/((2*t - t^-2)*t^2)", "dy/dx = (2*t^3 + 1)/(2*t^3 - 1)"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "alg",
        "x^2-5*x+6=0,method=factor",
        ["(x - 2)", "(x - 3)", "x = 3", "x = 2"],
        ["Range:", "Answer: x^2 - 5*x + 6", "ERR:"],
    ),
    (
        "alg",
        "solve([x^2+x*y+y^2=37,x+y+x*y=19],[x,y])",
        ["s=x+y,p=xy", "x^2+xy+y^2=s^2-p", "(x,y) = (3,4) or (4,3)"],
        ["Only one '=' supported", "ERR:"],
    ),
    (
        "alg",
        "domain(csc(2*x+pi/6)^2-cot(2*x+pi/6)^2)",
        ["sin(2*x + pi/6) != 0", "Answer:"],
        ["Answer: all real x", "ERR:"],
    ),
    (
        "alg",
        "domain(1-cos(2*x+pi/6)^2)",
        ["1 - cos(2*x + pi/6)^2 = sin(2*x + pi/6)^2", "Domain: all real x", "Answer: all real x"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(cot(x^2-pi/4)^2+1)",
        ["Answer: y >= 1"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "int",
        "atan(tan(x^2-pi/4))",
        ["branch"],
        ["Answer: int(", "Classify the integrand", "ERR:"],
    ),
    (
        "int",
        "asin((x-3)/8),method=parts",
        ["Use parts", "u=asin(w)", "dv=dw", "du=1/sqrt(1-w^2) dw", "v=w", "Answer:"],
        ["Classify the integrand", "ERR:"],
    ),
    (
        "int",
        "e^(2*x)*cos(5*x),method=parts",
        ["u = cos(5*x)", "dv = e^(2*x) dx", "du = -5*sin(5*x) dx", "v = e^(2*x)/2", "29/4*I"],
        ["IBP missing", "ERR:"],
    ),
    (
        "int",
        "(5*x+7)/((x-1)*(x^2+4)),method=pf",
        ["A/(x-1)+(Bx+C)/(x^2+4)", "A=12/5", "B=-12/5", "C=13/5", "Answer:"],
        ["Partial fractions: A/(x+p)", "ERR:"],
    ),
    (
        "int",
        "(3*x^2+5*x+7)/((x-1)^2*(x^2+1)),method=pf",
        ["A/(x-1)+B/(x-1)^2+(Cx+D)/(x^2+1)", "coefficient equations", "Answer:"],
        ["No elementary primitive found", "ERR:"],
    ),
    (
        "int",
        "(x^2+1)/(x^4+x^2+1),method=sub",
        ["N,D / x^2", "u=x-1/x", "du=(1+1/x^2) dx"],
        ["No elementary primitive found", "ERR:"],
    ),
    (
        "int",
        "(x^2-1)/(x^4+1),method=sub",
        ["u^2-2=(u-sqrt(2))(u+sqrt(2))", "A=1/(2*sqrt(2))", "B=-1/(2*sqrt(2))", "ln(abs((x + 1/x - sqrt(2))/(x + 1/x + sqrt(2))))"],
        ["No elementary primitive found", "ERR:"],
    ),
    (
        "int",
        "(x^2+1)/(x^4+1),method=pf",
        ["Use partial fractions", "x^4+1=(x^2+sqrt(2)*x+1)(x^2-sqrt(2)*x+1)", "Ax+B", "Cx+D", "Equate coefficients", "Answer:"],
        ["No elementary primitive found", "ERR:"],
    ),
    (
        "int",
        "defint(ln(sin(x)),x,0,pi/2)",
        ["u=2x", "du=2 dx", "0=>0", "pi/2=>pi", "-pi*ln(2)/2"],
        ["ERR:"],
    ),
    (
        "int",
        "defint(e^(2*x)/(1+e^(2*x)),x,0,ln(7))",
        ["u=1+e^(2*x)", "du=2*e^(2*x) dx", "I=(1/2)Int(1/u) du", "F(ln(7)) - F(0)"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "int",
        "x^3*e^(x^4),method=sub",
        ["u=x^4", "du=4*x^3 dx", "Int(e^u) du = e^u", "+ C"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "int",
        "exp(2*x-3),method=auto",
        ["u = 2*x - 3", "du/dx = 2", "I = 1/(2)*Int(e^u) du", "Int(e^u) du = e^u", "e^(2*x - 3)/2 + C"],
        ["reverse chain exponential rule", "ERR:"],
    ),
    (
        "int",
        "sin(2*x-3),method=auto",
        ["u = 2*x - 3", "du/dx = 2", "I = 1/(2)*Int(sin(u)) du", "Int(sin(u)) du = -cos(u)", "-cos(2*x - 3)/2 + C"],
        ["reverse chain sine rule", "ERR:"],
    ),
    (
        "int",
        "1/(x*sqrt(x^2-36)),method=trig",
        ["x=6*sec(t)", "Reference triangle", "I=(1/6)Int(1) dt", "acos(6/x)/6"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "int",
        "exp(sqrt(x)),method=sub",
        ["u=u", "dv=e^u du", "du=du", "v=e^u", "Answer:"],
        ["Parts: w=", "dz=", "ERR:"],
    ),
    (
        "int",
        "x^3*e^(2*x),method=di",
        ["D:", "I:", "Signs:", "e^(2*x)*(1/2*x^3 - 3/4*x^2 + 3/4*x - 3/8)"],
        ["Use DI/table integration by parts for x^n*e^(a*x+b).", "ERR:"],
    ),
    (
        "int",
        "x^2*cos(3*x),method=di",
        ["D:", "I:", "Signs:", "1/3*x^2*sin(3*x)"],
        ["Use DI/table integration by parts for x^n times trig.", "ERR:"],
    ),
    (
        "trig",
        "1/cos(x)=5,x,0,2*pi,8,method=identity",
        ["cos(A) = 1/5", "Base angle", "Answer:"],
        ["Answer: x = []", "ERR:"],
    ),
    (
        "trig",
        "sin(3*x)=sin(x),x,0,2*pi,10,method=identity",
        ["sin(A) = sin(B): A = B+2*pi*n or A = pi-B+2*pi*n", "x=n*pi", "x=pi/4+n*pi/2", "x = [0, pi/4"],
        ["ERR:"],
    ),
    (
        "trig",
        "2*sin(x)^2=1+cos(x),x,0,2*pi,10,method=identity",
        ["u=cos(x)", "2u^2+u-1=0", "u=1/2 or u=-1", "alpha = arccos", "cos(x)=1/2 or cos(x)=-1"],
        ["substitution differential", "ERR:"],
    ),
    (
        "trig",
        "2*cos(x)^2+3*cos(x)-2=0,x,0,2*pi,10,method=identity",
        ["u=cos(x)", "2u^2+3u-2=0", "u=1/2 or u=-2", "Reject u=-2", "alpha = arccos", "cos(x)=1/2"],
        ["Solve the quadratic in u, then solve cos(A)=u.", "ERR:"],
    ),
    (
        "trig",
        "4*cos(x)^2-4*cos(x)+1=0,x,0,2*pi,10,method=identity",
        ["u=cos(x)", "4u^2-4u+1=0", "u=1/2", "alpha = arccos", "x = [pi/3, 5*pi/3]"],
        ["u=1/2 or u=1/2", "ERR:"],
    ),
    (
        "alg",
        "domain(sqrt(log(1/2,x-1)))",
        ["x - 1 > 0", "base = 1/2", "0 < x - 1 <= 1", "Answer: 1 < x <= 2"],
        ["Answer: log(x - 1)/log(1/2) >= 0", "ERR:"],
    ),
    (
        "alg",
        "range(abs(x-1)+abs(x-2))",
        ["roots: x = 1; x = 2", "min y = 1", "Answer: y >= 1"],
        ["Answer: y >= 0", "ERR:"],
    ),
    (
        "alg",
        "range(abs(2*x-3)+abs(5*x+1))",
        ["roots: x = 3/2; x = -1/5", "min y = 17/5", "Answer: y >= 17/5"],
        ["inspect graph/transform", "Answer: y >= 0", "ERR:"],
    ),
    (
        "alg",
        "(domain(sqrt(log(2,x-9))))",
        ["base > 1", "Domain: x >= 10", "Answer: x >= 10"],
        ["domain*sqrt", "ERR:"],
    ),
    (
        "alg",
        "(range(abs(x-2)+abs(x-3)))",
        ["roots: x = 2; x = 3", "min y = 1", "Range: y >= 1"],
        ["range*", "ERR:"],
    ),
    (
        "alg",
        "binomial((1+2*x)^(-1/2),x,0,4)",
        ["n = -1/2: C(n,0) = 1", "C(n,4) = 35/128", "Terms: 1 - 1/2*(2*x)", "Valid for abs(x) < 1/2", "Answer: 1 - x + 3/2*x^2 - 5/2*x^3 + 35/8*x^4"],
        ["ERR:"],
    ),
    (
        "alg",
        "fitconst((a*x+b)*(x-2)+c*(x+1)^2=4*x^2+6*x-1,[a,b,c])",
        ["x = 0:", "x = 1:", "x = 2:", "a = 1", "b = 2", "c = 3"],
        ["Unexpected end of input", "Answer: solve(fitconst", "Route for fit constants", "ERR:"],
    ),
    (
        "alg",
        "fitconst(a*(x+1)^2+b*(x-1)^2+c*(x^2-1)=6*x^2+4*x+2,[a,b,c])",
        ["x = 0:", "x = 1:", "x = 2:", "a = 3", "b = 1", "c = 2"],
        ["Unexpected end of input", "Answer: solve(fitconst", "Route for fit constants", "ERR:"],
    ),
    (
        "alg",
        "fitconst((x+a)^2+(y+b)^2=x^2+y^2-6*x+10*y+34,[a,b])",
        ["(x = 0, y = 0):", "(x = 1, y = 0):", "a = -3", "b = 5"],
        ["constants not isolated", "Unexpected end of input", "Answer: solve(fitconst", "ERR:"],
    ),
    (
        "alg",
        "fitconst((a*x+b)/(x+1)=2+3/(x+1),[a,b])",
        ["identity", "a=2", "b=5"],
        ["constants not isolated", "Unexpected end of input", "Answer: solve(fitconst", "ERR:"],
    ),
    (
        "alg",
        "compare((x^2-1)/(x-1),x+1)",
        ["E1 =", "E2 =", "E1-E2 = 0", "equivalent"],
        ["Unexpected end of input", "Answer: solve(compare", "ERR:"],
    ),
    (
        "derive",
        "1/(2*x+1)+1/(y+1)=x^2,x,method=implicit",
        ["Domain: denoms !=0", "y + 2*x - x^2*(2*x + 1)*(y + 1) + 2 = 0", "d/dx(LHS) = d/dx(RHS)", "collect dy/dx", "dy/dx = (2*x*(2*x + 1)*(y + 1) + 2*x^2*(y + 1) - 2)/(- x^2*(2*x + 1) + 1)"],
        ["Domain: log args >0", "F_x", "F_y", "ERR:"],
    ),
    (
        "int",
        "x*ln(5*x)",
        ["u=ln(5*x)", "dv=x dx", "du=1/x dx", "v=x^2/2", "I=uv-Int(vdu)", "1/2*x^2*ln(5*x) - 1/4*x^2 + C"],
        ["Integral not recognised", "Answer: int(", "ERR:"],
    ),
    (
        "int",
        "tan(3*x)",
        ["u = 3*x, du = 3 dx", "tan(u) = sin(u)/cos(u)", "v = cos(u), dv = -sin(u) du", "Answer: -ln(abs(cos(3*x)))/3 + C"],
        ["Integral not recognised", "Answer: int(", "ERR:"],
    ),
    (
        "int",
        "atan(2*x+3)^2,method=parts",
        ["u = 2*x + 3, du = 2 dx", "I = 1/2*Int(atan(u)^2) du", "Int(t^2*sec(t)^2)dt = t^2*tan(t)-Int(2t*tan(t))dt", "Int(2t*tan(t))dt = -2t*ln(abs(cos(t)))+2*Int(ln(abs(cos(t))))dt", "Answer: No elementary primitive found"],
        ["Answer: atan(2*x + 3)^2", "ERR:"],
    ),
    (
        "alg",
        "range(log(10,abs(3*x+1)+3))",
        ["Answer: y >= ln(3)/ln(10)"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "sin(x)^2-sin(x)=0,method=factor",
        ["Let u = sin(x)", "u*(u - 1) = 0", "sin(x) = 0 or sin(x) = 1"],
        ["Unexpected token", "Answer: solve(", "ERR:"],
    ),
    (
        "alg",
        "x^2+a*x+b,method=factor",
        ["Err: numeric coeffs needed"],
        ["Unexpected token"],
    ),
    (
        "alg",
        "rewrite(sqrt(12+sqrt(140)))",
        ["Let sqrt(12+sqrt(140)) = sqrt(m)+sqrt(n)", "m+n=12", "4*m*n=140", "Answer: sqrt(7)+sqrt(5)"],
        ["rewrite*sqrt", "ERR:"],
    ),
    (
        "alg",
        "cot(x^2-pi/4),method=auto",
        ["Use identity cot(u) = cos(u)/sin(u)", "Domain: sin(x^2 - pi/4) != 0", "Answer: cos(x^2 - pi/4)/sin(x^2 - pi/4)"],
        ["Err:", "Unexpected token"],
    ),
    (
        "trig",
        "tan(2*x)=sqrt(3),x,0,2*pi,8,method=cast",
        ["A = pi/3 + n*pi", "x = pi/6 + n*pi/2", "Answer: x = [pi/6, 2*pi/3, 7*pi/6, 5*pi/3]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cot(x)^2+1,method=auto",
        ["Use identity 1 + cot(u)^2 = cosec(u)^2", "Domain: sin(x) != 0", "Answer: cosec(x)^2"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "sqrt((3*x+1)^2),method=auto",
        ["Let u = 3*x + 1", "sqrt(u^2)=abs(u)", "Answer: abs(3*x + 1)"],
        ["ERR:", "Unexpected token", "Answer: sqrt((3*x + 1)^2)"],
    ),
    (
        "derive",
        "mode:5,exp(t)*cos(t),exp(t)*sin(t),t",
        ["d/dt(dy/dx) = 2/(cos(t)-sin(t))^2", "Divide by dx/dt = e^t(cos(t)-sin(t))", "d2y/dx2 = 2/[e^t(cos(t)-sin(t))^3]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "int",
        "sqrt((2*x-3)^2)",
        ["sqrt(u^2)=abs(u)", "Split at u=0, so x=3/2", "For x >= 3/2", "For x < 3/2", "Answer: (2*x - 3)*abs(2*x - 3)/4 + C"],
        ["ERR:", "Unexpected token", "Answer: int("],
    ),
    (
        "alg",
        "arcsin(sin(2*x+pi/6)),method=complete_square",
        ["Complete square is for quadratics", "inverse-trig expression", "not applicable"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "asin(sin(2*x+pi/6)),method=auto",
        ["-pi/2 <= u <= pi/2 => asin(sin(u)) = u", "asin(sin(2*x + pi/6))"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "atan(2*x-3),method=pf",
        ["Err: PF needs rational P(x)/Q(x)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "sin(x+y),method=compound_angle",
        ["sin(A+B)=sin(A)cos(B)+cos(A)sin(B)", "Answer: sin(x)*cos(y)+cos(x)*sin(y)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "cos(7*theta),method=auto",
        ["Basis: let c=cos(theta)", "Answer: 64*c^7 - 112*c^5 + 56*c^3 - 7*c"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "tan(3*x),method=auto",
        ["Let u=tan(x)", "Answer: (3*tan(x)-tan(x)^3)/(1-3*tan(x)^2)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "(sin(5*x)-sin(x))/(cos(5*x)+cos(x)),method=auto",
        ["sum-to-product", "Answer: tan(2*x)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "cos(7*theta),target=cos(theta),method=auto",
        ["Source != target", "cos(7*theta)"],
        ["Source = target", "Answer: cos(theta)", "ERR:"],
    ),
    (
        "trig",
        "sin(x+y),target=sin(2*x),method=auto",
        ["Source != target", "sin(x + y)"],
        ["Source = target", "Answer: sin(2*x)", "ERR:"],
    ),
    (
        "trig",
        "tan(2*x+pi/6),method=auto",
        ["tan(A+B)", "Let N = tan(2*x)+1/sqrt(3)", "Let D = 1-tan(2*x)*1/sqrt(3)", "Answer: N/D"],
        ["Answer: (tan(", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "arctan(2*x-3)=0,method=auto",
        ["arctan has domain all real", "arctan(A)=0 => A=0", "2*x - 3 = 0", "Answer: x = 3/2"],
        ["Answer: 2*x - 3 = 0", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cosec((x)^2-pi/4)^2-cot((x)^2-pi/4)^2,method=auto",
        ["Use identity cosec(u)^2 - cot(u)^2 = 1", "Answer: 1"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(arccos((x-1)/3))",
        ["Domain: -1 <= (x - 1)/3 <= 1", "acos(A): -1<=A<=1 => 0<=y<=pi", "Range: 0 <= y <= pi", "Answer: 0 <= y <= pi"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(atan(tan(2*x+pi/6)))",
        ["Domain: cos(2*x + pi/6) != 0", "atan(A): A in R => -pi/2<y<pi/2", "Range: -pi/2 < y < pi/2"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(atan(tan(x)))",
        ["atan(A) is defined for all real A", "tan(u) needs cos(u) != 0", "Domain: cos(x) != 0", "Domain: x != pi/2 + n*pi"],
        ["1*(pi/2 + n*pi)", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cosec((x+1)/3),method=auto",
        ["Use identity cosec(u) = 1/sin(u)", "Domain: (x + 1)/3 != n*pi", "Domain: x != 3*n*pi - 1", "Answer: 1/sin((x + 1)/3)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "sec((x+1)/3),method=auto",
        ["Use identity sec(u) = 1/cos(u)", "Domain: (x + 1)/3 != pi/2 + n*pi", "Answer: 1/cos((x + 1)/3)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "mode:5,t^2+1/t,t^2-1/t,t",
        ["dy/dx = (2*t^3 + 1)/(2*t^3 - 1)", "d/dt(dy/dx) = -12*t^2/(2*t^3-1)^2", "Answer: d2y/dx2 = -12*t^4/(2*t^3 - 1)^3"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "stats",
        "covariance([1,1,2,3,5,8],[2,5,7,11])",
        ["Err: list lengths differ"],
        ["Traceback"],
    ),
    (
        "stats",
        "median([1,2,3,4,8])",
        ["x_(sorted) = [1, 2, 3, 4, 8]", "n = 5", "median = x_3 = 3", "3"],
        ["mean =", "var(", "sd =", "Traceback"],
    ),
    (
        "stats",
        "mean([1,2,3,4,8])",
        ["sum x = 18", "mean = sum x/n = 18/5 = 3.6", "3.6"],
        ["median =", "var(", "sd =", "Traceback"],
    ),
    (
        "stats",
        "quartiles([1,2,3,4,8])",
        ["Q1 = (x_1 + x_2)/2 = 1.5", "median = x_3 = 3", "Q3 = (x_4 + x_5)/2 = 6", "[1.5, 3, 6]"],
        ["mean =", "var(", "sd =", "Traceback"],
    ),
    (
        "stats",
        "stddev([1,2,3,4,8])",
        ["mean = 18/5 = 3.6", "Sxx = sum((x-mean)^2) = 29.2", "sd = sqrt(5.84) = 2.41660919", "2.41660919"],
        ["median =", "Traceback"],
    ),
    (
        "stats",
        "1,2,3,4,method=summary",
        ["sum x^2 = 30", "Sxx = sum x^2 - (sum x)^2/n = 30 - (10)^2/4 = 5", "sd = sqrt(var) = sqrt(1.25)", "s = sqrt(s^2) = sqrt(1.66666667)"],
        ["var(pop)", "sum((x-mean)^2)", "Traceback"],
    ),
    (
        "stats",
        "-36,14,47,-47,1000001,-1000000,-44,-29,34,method=summary",
        ["sum x^2 = 2.000002e12", "Sxx = sum x^2 - (sum x)^2/n", "sd = sqrt(var)", "s = sqrt(s^2)"],
        ["var(pop)", "Traceback"],
    ),
    (
        "stats",
        "binomial(10000,0.01,102,tail)",
        ["X ~ B(10000, 0.01)", "P(X >= 102) = 1 - P(X <= 101)", "sum_{x=0}^101", "mu = np = 100", "1 - Phi", "P(X >= 102) = 0.44008423"],
        ["Traceback", "normal approx with continuity correction"],
    ),
    (
        "int",
        "sin((x)^2-pi/4)^2+cos((x)^2-pi/4)^2",
        ["Use identity sin(u)^2 + cos(u)^2 = 1", "Answer: x + C"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "x^2-6*x+5=0,method=complete_square",
        ["(x - 3)^2 - 4 = 0", "x - 3 = +/-2", "Answer: x = [5, 1]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "2*x^2+3*x-2=0,method=complete_square",
        ["divide by a: x^2 + 3/2*x - 1 = 0", "h = b/(2a) = 3/4", "add h^2 = 9/16", "(x + 3/4)^2 = 25/16", "Answer: x = [1/2, -2]"],
        ["quadratic formula", "ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "mode:3,exp(t)*cos(t),exp(t)*sin(t),t",
        ["dy/dx = e^t(sin(t)+cos(t))/[e^t(cos(t)-sin(t))]", "Cancel e^t"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(sin(2*x+pi/6)^2+cos(2*x+pi/6)^2)",
        ["Use sin(u)^2 + cos(u)^2 = 1", "Domain: all real x", "Answer: all real x"],
        ["Start with 1", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(cosec((x)^2-pi/4)^2-cot((x)^2-pi/4)^2)",
        ["cosec(u)^2 = 1+cot(u)^2", "cosec(u)^2 - cot(u)^2 = 1", "Domain: sin(x^2 - pi/4) != 0", "sin(x^2 - pi/4) != 0"],
        ["Start with 1", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(cosec((x+1)/3)^2-cot((x+1)/3)^2)",
        ["u = (x+1)/3", "cosec(u)^2 - cot(u)^2 = 1", "Range: y = 1", "Answer: y = 1"],
        ["1 + cot(u)^2 = cosec(u)^2\nDomain", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(sec(x)^2-tan(x)^2)",
        ["sec(u)^2 = 1+tan(u)^2", "sec(u)^2 - tan(u)^2 = 1", "Range: y = 1", "Answer: y = 1"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(csc(x))",
        ["Domain: sin(x) != 0", "cosec(u)=1/sin(u), 0<|sin(u)|<=1", "Range: y <= -1 or y >= 1"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(cot((x)^2-pi/4)^2+1)",
        ["1 + cot(u)^2 = cosec(u)^2", "= cosec(x^2 - pi/4)^2", "Domain: sin(x^2 - pi/4) != 0"],
        ["Start with cosec", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cot((x+1)/3)^2+1=0,method=auto",
        ["1 + cot(u)^2 = cosec(u)^2", "cosec(u) = 1/sin(u); cosec(u)^2 != 0", "x = []"],
        ["LHS-RHS = 0:", "Answer:", "ERR:"],
    ),
    (
        "alg",
        "arcsin(sin((x)^2-pi/4))=0,method=auto",
        ["arcsin(A)=0 => A=0", "sin(u)=0 => u=n*pi", "x = +/-sqrt(pi/4 + n*pi)"],
        ["x = []", "No real solution", "ERR:"],
    ),
    (
        "alg",
        "cosec(2*x+pi/6)^2-cot(2*x+pi/6)^2=0,method=auto",
        ["Domain: sin(2*x + pi/6) != 0", "Use identity cosec(u)^2 - cot(u)^2 = 1", "Answer: x = []"],
        ["log args", "denoms !=0", "ERR:"],
    ),
    (
        "alg",
        "cos(x)=0,method=auto",
        ["cos(x)=0 => x=pi/2 + n*pi", "Answer: x = pi/2 + n*pi, integer n"],
        ["Answer (3 d.p.)", "-98.", "ERR:"],
    ),
    (
        "int",
        "(cosec(x)^2-cot(x)^2)*exp(x),method=auto",
        ["cosec(x)^2 - cot(x)^2 = 1", "integrand = e^(x)", "I = Int(e^(x)) dx", "Answer: e^(x) + C"],
        ["Answer: int", "ERR:"],
    ),
    (
        "int",
        "cot(x),method=auto",
        ["cot(u) = cos(u)/sin(u)", "v = sin(u), dv = cos(u) du", "Int(cot(u))du = Int(1/v)dv", "Answer: ln(abs(sin(x))) + C"],
        ["Integral(cot u)du", "Answer: int", "ERR:"],
    ),
    (
        "int",
        "sec(x),method=auto",
        ["v = sec(u)+tan(u)", "dv = sec(u)(sec(u)+tan(u)) du", "Int(sec(u))du = Int(1/v)dv", "Answer: ln(abs(sec(x) + tan(x))) + C"],
        ["Integral(sec u)du", "Answer: int", "ERR:"],
    ),
    (
        "int",
        "tan(x)^2+1,method=auto",
        ["tan(u)^2 + 1 = sec(u)^2", "integrand = sec(x)^2", "I = Int(sec(x)^2) dx", "Answer: tan(x) + C"],
        ["Answer: int", "ERR:"],
    ),
    (
        "int",
        "cot((x)^2-pi/4)^2+1,method=auto",
        ["1 + cot(u)^2 = cosec(u)^2", "du/dx = 2*x", "du/dx not constant", "No elementary primitive"],
        ["Use the general integration route", "Answer: int", "ERR:"],
    ),
    (
        "alg",
        "range(x/(1+x^2))",
        ["y*(x^2 + 1) = x", "y*x^2 - x + y = 0", "1 - 4*y^2 >= 0", "Range: -1/2 <= y <= 1/2", "Answer: -1/2 <= y <= 1/2"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(1/(x-1)+1/(x+2))",
        ["y*(x^2 + x - 2) = 2*x + 1", "Discriminant = 9*y^2 + 4 > 0", "Range: all real y", "Answer: all real y"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(x^3,x,-1,3)",
        ["x^3 is increasing", "Range: -1 <= y <= 27", "Answer: -1 <= y <= 27"],
        ["unrestricted on interval", "inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(exp(x))",
        ["e^u > 0", "Range: y > 0", "Answer: y > 0"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(1/(2-cos(3*x)))",
        ["cos(3*x) in [-1,1]", "Answer: 1/3 <= y <= 1"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(cot((x+1)/3))",
        ["Range: all real y", "Answer: all real y"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "domain(arccos(sin((x+1)/3)))",
        ["acos(u): -1 <= u <= 1", "-1 <= sin((x + 1)/3) <= 1", "Answer: all real x"],
        ["Answer: -1 <= sin", "inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "domain((3*ln(x)-7)/(ln(x)-2))",
        ["Domain: ln(x) - 2 != 0", "Domain: x != e^2", "Domain: x > 0"],
        ["ERR:"],
    ),
    (
        "trig",
        "sqrt(1-cos(x))=sin(x),x,0,2*pi,8,method=square_then_check",
        ["Square both sides", "cos(x)*(cos(x) - 1) = 0", "Check in original", "Answer: x = [0, pi/2"],
        ["Answer: x = []", "ERR:"],
    ),
    (
        "trig",
        "3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform",
        ["R = sqrt(3^2 + 4^2) = 5", "cos(alpha)=3/5", "cos(x-alpha)=2/5", "Answer: x = [arctan(4/3)+arccos(2/5), 2*pi+arctan(4/3)-arccos(2/5)]"],
        ["Answer: x = [2.", "ERR:"],
    ),
    (
        "trig",
        "sin(x)=-1,x,0,360",
        ["sin(x) = -1", "x = 270 + n*360", "0 <= x <= 360", "Answer: x = [270]"],
        ["alpha = arcsin(-1) = -pi/2 for x\n0 <= x <= 360", "ERR:"],
    ),
    (
        "trig",
        "cos(x)=1/2,x,0,360",
        ["alpha = arccos(1/2)", "x = alpha + 360n or x = 360-alpha + 360n", "0 <= 60 + n*360 <= 360 => n = 0", "Answer: x = [60, 300]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "sin(2*x)+cos(2*x)=0,x,-180,180",
        ["A = 2*x", "cos(A)=0 => sin(A)+cos(A)=+/-1 != 0", "(sin(A)+cos(A))/cos(A)=0/cos(A)", "tan(2*x)=sin(2*x)/cos(2*x)", "tan(2*x)=-1", "x = -22.5 + n*90", "x = [-112.5, -22.5, 67.5, 157.5]"],
        ["Divide by cos(2x) where valid", "Solve the tan equation", "ERR:"],
    ),
    (
        "trig",
        "sin(2*x)+cos(2*x)=0,x,0,pi",
        ["x = -pi/8 + n*pi/2", "0 <= x <= pi", "x = [3*pi/8, 7*pi/8]"],
        ["NEEDS_REVIEW", "ERR:"],
    ),
    (
        "trig",
        "(2*sin(x)*cos(x))/(cos(x)^2-sin(x)^2)=sin(x)/cos(x),x,0,180",
        ["tan(2*x) = 2*sin(x)*cos(x)/(cos(x)^2 - sin(x)^2)", "tan(2*x) = tan(x)", "cos(A)!=0, cos(B)!=0", "x = [0, 180]"],
        ["2sin(x)cos(x)", "ERR:"],
    ),
    (
        "trig",
        "cos(2*x)+2*cos(x)=1,x,0,360",
        ["u=cos(x)", "cos(2*x)=2u^2-1", "2u^2 + 2u - 2 = 0", "Reject u"],
        ["x = []", "ERR:"],
    ),
    (
        "trig",
        "sin(x)^4,method=double_angle",
        ["sin(x)^2=(1-cos(2*x))/2", "cos(2*x)^2=(1+cos(4*x))/2", "Answer: (3 - 4*cos(2*x) + cos(4*x))/8"],
        ["Answer: sin(x)^4", "ERR:"],
    ),
    (
        "int",
        "sin(x)^2",
        ["sin(x)^2 = (1-cos(2*x))/2", "I = 1/2*Int(1-cos(2*x)) dx", "Answer: x/2 - sin(2*x)/4 + C"],
        ["Done", "ERR:"],
    ),
    (
        "int",
        "cos(x)^2",
        ["cos(x)^2 = (1+cos(2*x))/2", "I = 1/2*Int(1+cos(2*x)) dx", "Answer: x/2 + sin(2*x)/4 + C"],
        ["Done", "ERR:"],
    ),
    (
        "int",
        "sin(x)^6,method=trig",
        ["sin(x)^6", "cos(2u)", "cos(4u)", "cos(6u)", "5/16*x", "- 15/64*sin(2*x)", "Answer:"],
        ["No elementary primitive", "Answer: int(", "ERR:"],
    ),
    (
        "int",
        "cos(x)^6,method=trig",
        ["cos(x)^6", "cos(2u)", "cos(4u)", "cos(6u)", "5/16*x", "+ 15/64*sin(2*x)", "Answer:"],
        ["No elementary primitive", "Answer: int(", "ERR:"],
    ),
    (
        "alg",
        "domain(arcsin((x-1)/3))",
        ["Domain: -2 <= x <= 4", "Domain: -1 <= (x - 1)/3 <= 1", "Answer: -2 <= x <= 4"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "int",
        "1-cos(2*x+pi/6)^2,method=auto",
        ["Use sin(u)^2=1-cos(u)^2", "u=2*x + pi/6", "Answer: x/2 - sin(4*x + pi/3)/8 + C"],
        ["No elementary A-level primitive", "Classify the integrand", "ERR:"],
    ),
    (
        "alg",
        "1/(x-1)+1/(x+2)=1,method=auto",
        ["Domain: x != -2", "x != 1", "(x + 2) + (x - 1) = x^2 + x - 2", "expand => - x^2 + x + 3 = 0", "x = [1/2 - sqrt(13)/2, 1/2 + sqrt(13)/2]"],
        [")/-2", "ERR:", "No real solution"],
    ),
    (
        "alg",
        "1/(x+8)+1/(x-8)=2*x/(x^2-64),method=auto",
        ["Domain: x + 8 != 0 => x != -8", "Domain: x - 8 != 0 => x != 8", "Domain: x^2 - 64 != 0 => x != -8, x != 8", "x^2 - 64 = (x + 8)*(x - 8)", "(x^2 - 64)/(x + 8) = x - 8", "(x^2 - 64)/(x^2 - 64) = 1", "(x - 8) + (x + 8) = 2*x", "expand => 0 = 0", "x in R, x != -8, x != 8"],
        ["LHS-RHS = 0", "x = all real\n", "x = all real values in domain"],
    ),
    (
        "alg",
        "abs(2*x+1)=2,method=auto",
        ["abs(2*x + 1) = 2", "2*x + 1 = 2 => x = 1/2", "2*x + 1 = -2 => x = -3/2", "x = [-3/2, 1/2]"],
        ["x = -1.5", "x = 0.5", "ERR:"],
    ),
    (
        "alg",
        "range(sqrt((x+5)^2))",
        ["Domain: all real x", "Range: y >= 0", "Answer: y >= 0"],
        ["Domain: (x + 5)^2 >= 0", "ERR:"],
    ),
    (
        "trig",
        "1-cos(2*x+pi/6)^2,method=auto",
        ["Use identity 1 - cos(u)^2 = sin(u)^2", "u = 2*x + pi/6", "Answer: sin(2*x + pi/6)^2"],
        ["Simplify using exact trig/algebra rules", "Answer: - cos(2*x + pi/6)^2 + 1", "ERR:"],
    ),
    (
        "trig",
        "tan(x),target=sin(x)/cos(x),method=auto",
        ["Use identity tan(u)=sin(u)/cos(u)", "u = x", "Answer: sin(x)/cos(x)"],
        ["Apply standard identities/rearrangement toward the target", "ERR:"],
    ),
    (
        "int",
        "(x^2+2*x+1)/(x+1),method=auto",
        ["N =", "x^2", "D = x + 1", "N/D = x + 1", "x + 1/2*x^2 + C"],
        ["No elementary A - level primitive", "No elementary A-level primitive", "Answer: int(", "ERR:"],
    ),
    (
        "alg",
        "solve(log(2,x)+log(4,x)=6,x,method=log_exp)",
        ["u = log(2,x)", "log(4,x) = u/2", "3u/2 = 6", "x = 16"],
        ["log/exp laws to combine", "Exponentiate, solve", "ERR:"],
    ),
    (
        "alg",
        "log(2,8)",
        ["log(a,b)=c means a^c=b", "3"],
        ["log(8)/log(2)", "ERR:"],
    ),
    (
        "alg",
        "solve(log(2,x)+log(x,2)=5/2,x,method=log_exp)",
        ["u = log(2,x)", "log(x,2) = 1/u", "2u^2 - 5u + 2 = 0", "x = sqrt(2) or 4"],
        ["log/exp laws to combine", "Exponentiate, solve", "ERR:"],
    ),
    (
        "alg",
        "solve(log(3,x)+log(x,27)=4,x,method=log_exp)",
        ["u = log(3,x)", "log(x,27) = 3/u", "u^2 - 4u + 3 = 0", "x = 3 or 27"],
        ["log/exp laws to combine", "Exponentiate, solve", "ERR:"],
    ),
    (
        "alg",
        "solve(2*log(10,4-x)=log(10,x+8),x,method=log_exp)",
        ["log(10,(4-x)^2)", "x = 8 or 1", "x = 8 rejected by domain", "x = [1]"],
        ["log/exp laws to combine", "Exponentiate, solve", "ERR:"],
    ),
    (
        "alg",
        "solve(log(2,x+3)+log(2,x+10)=2+2*log(2,x),x,method=log_exp)",
        ["log(2,(x+3)*(x+10))", "(x + 3)*(x + 10) = 4*x^2", "x = -5/3 rejected by domain", "x = [6]"],
        ["log/exp laws to combine", "Exponentiate, solve", "ERR:"],
    ),
]


def compact(text: str) -> str:
    s = "".join(ch for ch in text if ch not in " \t\r\n*").lower()
    for old, new in (
        ("integral", "int"),
        ("answer:", ""),
        ("result:", ""),
        ("use", ""),
        ("identity", ""),
        ("startwith", ""),
        ("let", ""),
        ("then", ""),
        ("hence", ""),
        ("therefore", ""),
        ("so", ""),
        ("baseangle:", "baseangle"),
        ("equatecoefficients", "coeffs"),
        ("differentiatebothsides", "d/dx"),
        ("cleardenominators", "cleardenoms"),
        ("simplifybothexpressions", "bothexpressions"),
        ("collectdy/dx", "dy/dxterms"),
        ("splitatu=0,so", "splitatu=0;"),
        ("splitatu=0,", "splitatu=0;"),
    ):
        s = s.replace(old, new)
    return s


def run(flag: str, expr: str) -> str:
    proc = subprocess.run(
        [str(HOST), "--" + flag, expr],
        cwd=str(REPO),
        text=True,
        capture_output=True,
        timeout=15,
    )
    return proc.stdout + proc.stderr


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    bad = 0
    for flag, expr, needles, forbidden in CASES:
        out = run(flag, expr)
        norm = compact(out)
        low_signal = {"", "parts", "method", "d/dx", "collectiterms", "partialfractions", "checkinoriginal", "integratex+1"}
        required = [compact(n) for n in needles if compact(n) not in low_signal]
        ok = all(n in norm for n in required)
        ok = ok and not any(f in out for f in forbidden)
        if ok:
            print("OK", flag, expr)
            continue
        bad += 1
        print("FAIL", flag, expr)
        print(out)
    print("done mismatches", bad, "/", len(CASES))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

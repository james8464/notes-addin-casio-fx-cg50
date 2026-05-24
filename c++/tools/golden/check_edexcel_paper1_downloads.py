#!/usr/bin/env python3
"""Paper 1 audit cases from local Edexcel QP/MA PDFs.

Uses representative commandable parts from the downloaded Paper 1 papers.
Model-solution comparison is marker-based: answer + exam-method lines.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "June 2018 Q1 small-angle approximation",
        ["--trig", "(1-cos(4*theta))/(2*theta*sin(3*theta)),method=small"],
        ["cos(4*theta) ~ 1 - (4*theta)^2/2", "sin(3*theta) ~ 3*theta", "~ 8*theta^2/(6*theta^2)", "= 4/3"],
        ["ERR:", "Answer:"],
    ),
    (
        "June 2018 Q2a derivative and second derivative",
        ["--derive", "x^2-2*x-24*sqrt(x),x"],
        ["d/dx(x^2) = 2*x", "d/dx(-24*sqrt(x)) = -12/sqrt(x)", "dy/dx = 2*x - 12/sqrt(x) - 2"],
        ["ERR:", "limite"],
    ),
    (
        "June 2018 Q2a second derivative",
        ["--derive", "x^2-2*x-24*sqrt(x),x,method=second"],
        ["Differentiate once; differentiate dy/dx again", "d2y/dx2 = 6*sqrt(x)^-3 + 2"],
        ["ERR:", "Answer: d/dx"],
    ),
    (
        "June 2018 Q5 trig quotient derivative",
        ["--derive", "3*sin(theta)/(2*sin(theta)+2*cos(theta)),theta"],
        ["quotient rule", "1 + sin(2*theta)", "3/(2*(1 + sin(2*theta)))"],
        ["Answer: d/dtheta", "ERR:"],
    ),
    (
        "June 2018 Q7b parameter inverse proportional integral",
        ["--int", "defint(2/(2*x-k)^2,x,k,2*k)"],
        ["Limits: x = k => u = k, x = 2*k => u = 3*k", "2/(3*k)"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "June 2018 Q8 sine model degree conversion",
        ["--trig", "5+2*sin(30*t*pi/180)=3.8,t,8.5,24"],
        ["sin(30*t*pi/180) = -3/5", "t = 7.22899658819 + n*12 or 10.7710034118 + n*12", "t = [10.7710034118"],
        ["t = []", "n*687", "ERR:"],
    ),
    (
        "June 2018 Q9 implicit cycle-track derivative",
        ["--derive", "x^2-2*x*y+3*y^2=50,x,method=implicit"],
        ["F_x = 2*x - 2*y", "F_y = - 2*x + 6*y", "F_x + F_y*dy/dx = 0", "dy/dx = (- x + y)/(- x + 3*y)"],
        ["Answer: d/dx", "ERR:"],
    ),
    (
        "June 2018 Q9 furthest-west simultaneous solve",
        ["--alg", "solve([y=x/3,x^2-2*x*y+3*y^2=50],[x,y])"],
        ["3*y - x = 0", "x = 3*y", "6*y^2 - 50 = 0", "0 - 5*sqrt(3)/3"],
        ["ERR:", "(x,y) = []"],
    ),
    (
        "June 2018 Q10 separable coaster model",
        ["--int", "de_solve(dH/dt=H*cos(t/4)/40,H(0)=5)"],
        ["1/H dH = cos(t/4)/40 dt", "ln(abs(H)) = 4*sin(t/4)/40 + C", "C = ln(5)", "H = 5*e^(4*sin(t/4)/40)"],
        ["ERR:", "Integral not recognised"],
    ),
    (
        "June 2018 Q10 radian maximum second time",
        ["--trig", "sin(t/4)=1,t,0,40,method=rad"],
        ["sin(t/4) = 1", "t = 2*pi + n*8*pi", "t = [2*pi, 10*pi]"],
        ["t = []", "360 + n*1440", "ERR:"],
    ),
    (
        "June 2018 Q11 rational binomial",
        ["--alg", "binomial(sqrt((1+4*x)/(1-x)),x,0,2)"],
        ["sqrt(4*x + 1)", "(4*x + 1)^1/2 = 1 + 2*x - 2*x^2", "(- x + 1)^-1/2 = 1 + 1/2*x + 3/8*x^2", "5/2*x", "- 5/8*x^2", "Valid for abs(x) < 1/4"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "June 2018 Q11 sqrt6 fraction approximation",
        ["--alg", "2*(1+5/2*(1/11)-5/8*(1/11)^2)"],
        ["1183/484"],
        ["ERR:"],
    ),
    (
        "June 2018 Q12 car model growth factor",
        ["--alg", "solve(p^7=50000/32000,p)"],
        ["p^7 = 25/16", "p = (25/16)^(1/7)", "p = [(25/16)^(1/7)]"],
        ["ERR:", "p = []", "p = [1.065"],
    ),
    (
        "June 2018 Q12 car model growth factor numeric",
        ["--alg", "(25/16)^(1/7),method=numeric"],
        ["1.06583155827"],
        ["ERR:"],
    ),
    (
        "June 2018 Q12 car model initial value",
        ["--alg", "solve(32000=A*(1.0658)^4,A)"],
        ["32000 = 806460091894081/625000000000000*A", "A = 24799.7392568", "A ~= 24799.739"],
        ["ERR:", "1553255926290448384/806460091894081", "A = []"],
    ),
    (
        "June 2018 Q12 car model threshold year",
        ["--alg", "solve(100000=24800*(1.0658)^t,t)"],
        ["ln(100000) = ln(5329/5000)*t + ln(24800)", "t = (- ln(24800) + ln(100000))/ln(5329/5000)"],
        ["ERR:", "t = []"],
    ),
    (
        "June 2018 Q13 linear-root definite integral",
        ["--int", "defint(2*x*sqrt(x+2),x,0,2)"],
        ["u=x+2", "x=u-2", "Limits", "32/15*(2 + sqrt(2))"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "June 2018 Q14 parametric parabola identity",
        ["--alg", "compare(4+2*cos(2*t),6-(3+2*sin(t)-3)^2)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2018 Q14 line parameter upper bound",
        ["--alg", "solve(49-4*(k+3)>0,k)"],
        ["N = 0: k = 37/4", "k < 37/4"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2019 Q6 double-angle tan solve",
        ["--trig", "5*sin(2*theta)=9*tan(theta),theta,-180,180,10,method=identity"],
        ["sin(theta)=0", "cos(theta)^2=9/10", "theta = [", "0, 18.4", "161.6"],
        ["theta = []", "Right side still contains", "ERR:"],
    ),
    (
        "June 2019 Q6 shifted double-angle tan solve",
        ["--trig", "5*sin(2*x-50)=9*tan(x-25),x,0,360,10,method=identity"],
        ["A = x - 25", "10*cos(A)^2-9", "cos(A)^2=9/10", "x = [6.56505117708, 25"],
        ["2*cos(A)^2-9", "x = []", "Right side still contains", "ERR:"],
    ),
    (
        "June 2019 Q9 symbolic log quotient",
        ["--alg", "solve(log(a)-log(b)=log(a-b),a)"],
        ["ln(a) - ln(b) = ln(a - b)", "Domain: a - b > 0", "a - b*(a - b) = 0", "a = -b^2/(- b + 1)"],
        ["symbolic parameters unsupported", "No real solution", "ERR:"],
    ),
    (
        "June 2019 Q11 geometric race total numeric",
        ["--alg", "24+6.3*(1.05^16-1)/(1.05-1),method=numeric"],
        ["173.042"],
        ["25.568", "ERR:"],
    ),
    (
        "June 2019 Q12 bounce height numeric",
        ["--alg", "10*e^(-0.25*(pi+atan(4)))*abs(sin(pi+atan(4))),method=numeric"],
        ["3.175"],
        ["ERR:"],
    ),
    (
        "June 2019 Q13 definite PF area log form",
        ["--int", "defint((15-3*x)/((2*x-4)*(x+3)),x,3,5)"],
        ["PF: A/(2*x - 4)+B/(x + 3)", "A = 9/5, B = -12/5", "F(5) = -63/10*ln(2) + 9/10*ln(3)", "-24/5*ln(2) + 33/10*ln(3)"],
        ["33/10*ln(6) - 12/5*ln(8) - 9/10*ln(2)\n", "ERR:"],
    ),
    (
        "October 2020 Q12 cosec identity solve",
        ["--trig", "cosec(x)-sin(x)=cos(x)*cot(3*x-50),x,0,180,10,method=identity"],
        ["cosec(x)-sin(x)=cos(x)cot(x)", "x=90", "cot(x)=cot(3x-50)", "x = [25, 90, 115]"],
        ["x = []", "Rearrange using identities", "ERR:"],
    ),
    (
        "October 2020 Q15 implicit tan simplification",
        ["--derive", "x^2*tan(y)=9,x,method=implicit"],
        ["Use tan(y)=9/x^2", "Use sec(y)^2=1+tan(y)^2", "-18*x/(x^4 + 81)"],
        ["Domain: positive bases", "Answer: dy/dx = -2*x*tan", "ERR:"],
    ),
    (
        "October 2020 Q1 binomial sqrt expansion",
        ["--alg", "binomial((1+8*x)^(1/2),x,0,2)"],
        ["n = 1/2", "T1 = 4*x", "T2 = -8*x^2", "Valid for abs(x) < 1/8"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "October 2020 Q1 binomial approximation value",
        ["--alg", "1+4*(1/32)-8*(1/32)^2"],
        ["143/128"],
        ["ERR:"],
    ),
    (
        "October 2020 Q2 exponential log solve",
        ["--alg", "solve(4^(3*p-1)=5^210,p)"],
        ["2*(3*p - 1)*ln(2) = 210*ln(5)", "p = (210*ln(5) + 2*ln(2))/(6*ln(2))"],
        ["ERR:", "p = []"],
    ),
    (
        "October 2020 Q2 exponential log numeric",
        ["--alg", "((210*ln(5))/(ln(4))+1)/3,method=numeric"],
        ["81.6008166544"],
        ["ERR:"],
    ),
    (
        "October 2020 Q4 fractional inverse",
        ["--alg", "inverse((3*x-7)/(x-2),x)"],
        ["x = f(y)", "f^-1(x) = (2*x - 7)/(x - 3)", "Domain: x != 3"],
        ["ERR:", "no inverse"],
    ),
    (
        "October 2020 Q4 fractional composite",
        ["--alg", "compose((3*x-7)/(x-2),(3*x-7)/(x-2))"],
        ["f(g(x)) = (2*x - 7)/(x - 3)"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 AP common difference",
        ["--alg", "solve(115=28+5*d,d)"],
        ["-5*d = -87", "d = 87/5"],
        ["ERR:", "d = []"],
    ),
    (
        "October 2020 Q5 gear sequence value",
        ["--alg", "28+2*(87/5),method=numeric"],
        ["314/5", "62.8"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 GP interpolation value",
        ["--alg", "28*(115/28)^(4/5),method=numeric"],
        ["86.6941671016"],
        ["ERR:"],
    ),
    (
        "October 2020 Q6 maximum time",
        ["--alg", "(-atan(2)+pi/2+3)/(pi/12),method=numeric"],
        ["13.2301593144"],
        ["ERR:"],
    ),
    (
        "October 2020 Q7 line and curve coefficients",
        ["--alg", "solve([13=m*(-2)+25,25=a*(0+2)^2+13],[m,a])"],
        ["m = 6", "a = 3", "(m,a) = [(6,3)]"],
        ["ERR:", "(m,a) = []"],
    ),
    (
        "October 2020 Q8 exponential DE form",
        ["--int", "de_solve(dn/dt=k*n)"],
        ["1/n dn = k dt", "ln(abs(n)) = k*t + C", "n = A*e^(k*t)"],
        ["ERR:", "Integral not recognised"],
    ),
    (
        "October 2020 Q9 product derivative",
        ["--derive", "4*(x^2-2)*e^(-2*x),x"],
        ["dy/dx = c*(f1'*f2 + f1*f2')", "dy/dx = 8*x*e^(-2*x) - 8*(x^2 - 2)*e^(-2*x)"],
        ["ERR:", "limite"],
    ),
    (
        "October 2020 Q9 stationary x values",
        ["--alg", "solve(8*(2+x-x^2)*e^(-2*x)=0,x)"],
        ["e^(-2*x) > 0", "8*(x - x^2 + 2) = 0", "x = [2, -1]"],
        ["ERR:", "x = []"],
    ),
    (
        "October 2020 Q9 transformed range g",
        ["--alg", "range(8*(x^2-2)*e^(-2*x))"],
        ["dy/dx = 8*e^(-2*x)*(- 2*x^2 + 2*x + 4)", "y(-1) = -8*e^(2)", "Range: y >= -8*e^(2)"],
        ["ERR:", "inspect graph/transform"],
    ),
    (
        "October 2020 Q9 restricted range h",
        ["--alg", "range(8*(x^2-2)*e^(-2*x)-3,x,0,inf)"],
        ["Interval of interest: x on [0, inf]", "y(2) = 16*e^(-4) - 3", "Range: -19 <= y <= 16*e^(-4) - 3"],
        ["ERR:", "inspect graph/transform"],
    ),
    (
        "October 2020 Q10 root substitution definite integral",
        ["--int", "defint(3/((x-1)*(3+2*sqrt(x-1))),x,5,10)"],
        ["u = sqrt(x-1); x = u^2+1; dx = 2u du", "6/(u*(3+2u)) = 2/u - 4/(3+2u)", "ln(49/36)"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "October 2020 Q11 circle intersection algebra",
        ["--alg", "solve((x-15)^2+100-x^2=40,x)"],
        ["-30*x = -285", "x = 19/2"],
        ["ERR:", "x = []"],
    ),
    (
        "October 2020 Q11 circle perimeter numeric",
        ["--alg", "10*(2*pi-2*acos(9.5/10))+sqrt(40)*(2*pi-2*acos(5.5/sqrt(40))),method=numeric"],
        ["89.6876125946"],
        ["ERR:"],
    ),
    (
        "October 2020 Q13 recurrence condition",
        ["--alg", "compare(k*(k+3)/(k+1)-2,(k^2+k-2)/(k+1))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2020 Q13 recurrence root",
        ["--alg", "solve(k^2+k-2=0,k)"],
        ["k = 1", "k = -2", "k = [1, -2]"],
        ["ERR:", "k = []"],
    ),
    (
        "October 2020 Q13 periodic sum",
        ["--alg", "26*(2-4-1)+2-4"],
        ["-80"],
        ["ERR:"],
    ),
    (
        "October 2020 Q14 deflating balloon DE",
        ["--int", "de_solve(dr/dt=-k/r^2,r(0)=40,r(5)=20)"],
        ["r^2 dr = -k dt", "C = 64000/3", "k = 11200/3", "r^3 = 3*(- 11200/3*t + 64000/3)"],
        ["ERR:", "Integral not recognised"],
    ),
    (
        "October 2020 Q14 validity interval",
        ["--alg", "solve(-11200/3*t+64000/3>=0,t)"],
        ["N = 0: t = 40/7", "t <= 40/7"],
        ["ERR:", "t = []"],
    ),
    (
        "October 2020 Q15 second derivative quotient",
        ["--derive", "-18*x/(x^4+81),x"],
        ["y' = (u'v-u*v')/v^2", "dy/dx = (- 18*(x^4 + 81) + 72*x^4)/(x^4 + 81)^2"],
        ["ERR:", "limite"],
    ),
    (
        "October 2020 Q15 second derivative simplified check",
        ["--alg", "compare((-18*(x^4+81)+72*x^4)/(x^4+81)^2,54*(x^4-27)/(x^4+81)^2)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2020 Q15 inflection x roots",
        ["--alg", "solve(54*(x^4-27)=0,x)"],
        ["x^4 = 27", "x = +/-(27)^(1/4)", "x = [-(27)^(1/4), (27)^(1/4)]"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2024 Q13 beta-style substitution",
        ["--int", "defint(sqrt(x)*sqrt(a-x),x,0,a)"],
        ["x=a*sin(theta)^2", "dx=2*a*sin(theta)*cos(theta)", "sin(2*theta)^2", "pi*a^2/8"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "June 2022 Q15 exact trig endpoints",
        ["--int", "defint(8-8*cos(4*t)+48*sin(t)*cos(t),t,0,pi/4)"],
        ["F(pi/4) - F(0)", "12 + 2*pi"],
        ["cos(pi/4)^2", "ERR:"],
    ),
    (
        "June 2024 Q4 first principles x squared",
        ["--derive", "x^2,x,method=first_principles"],
        ["[(x+h)^2-x^2]/h", "2*x+h", "h->0", "d/dx x^2 = 2*x"],
        ["No first-principles route", "Answer: d/dx(", "ERR:"],
    ),
    (
        "June 2024 Q1 factor theorem parameter",
        ["--alg", "solve(3*3^3-20*3^2+3*(k+17)+k=0,k)"],
        ["3*(k + 17) + k - 99 = 0", "4*k = 48", "k = [12]"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2024 Q2 binomial sqrt validity",
        ["--alg", "binomial((1-9*x)^(1/2),x,0,3)"],
        ["n = 1/2", "T1 = -9/2*x", "T3 = -729/16*x^3", "Valid for abs(x) < 1/9"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "June 2024 Q3 tan composite derivative",
        ["--derive", "x+tan(x/2),x"],
        ["d/dx(x) = 1", "d/dx(tan(x/2)) = 1/2*sec(x/2)^2", "dy/dx = 1/2*sec(x/2)^2 + 1"],
        ["limite", "ERR:"],
    ),
    (
        "June 2024 Q5 quotient derivative",
        ["--derive", "(2*x-3)/(x^2+4),x"],
        ["y' = (u'v-u*v')/v^2", "[(2)*(x^2 + 4)-(2*x - 3)*(2*x)]", "dy/dx = (-2*x^2 + 6*x + 8)/(x^2 + 4)^2"],
        ["ERR:", "Answer: d/dx"],
    ),
    (
        "June 2024 Q5 decreasing interval",
        ["--alg", "solve((-2*x^2+6*x+8)/(x^2+4)^2<0,x)"],
        ["N = 0: x = -1, 4", "sign: (-inf,-1):-", "x < -1 or x > 4"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2024 Q6 modulus branch solve",
        ["--alg", "solve(16-4*x=3*abs(x-2)+5,x)"],
        ["x < 2 => abs(x-2) = -(x-2)", "x >= 2: - 4*x + 16 = 3*(x - 2) + 5", "x = [17/7]"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2024 Q7 leak differential model",
        ["--int", "de_solve(dH/dt=-0.12*e^(-0.2*t),H(0)=1.5)"],
        ["H = 3/5*e^(-1/5*t) + C", "H(0) = 3/2", "C = 9/10", "H = 3/5*e^(-1/5*t) + 9/10"],
        ["ERR:", "Integral not recognised"],
    ),
    (
        "June 2024 Q8 inverse fractional linear domain",
        ["--alg", "inverse(5/(2*x-9),x)"],
        ["x = f(y)", "f^-1(x) = (9*x + 5)/(2*x)", "Domain: x != 0"],
        ["ERR:", "no inverse"],
    ),
    (
        "June 2024 Q8 composite range",
        ["--alg", "range(-5/(1+6*x^2))"],
        ["Discriminant = - 24*y^2 - 120*y", "Range: -5 <= y < 0", "-5 <= y < 0"],
        ["ERR:", "-5 <= y <= 0"],
    ),
    (
        "June 2024 Q8 no-real parameter threshold",
        ["--alg", "range(-5*x^2+6*x+4,x)"],
        ["Domain: all real x", "Range: y <= 29/5", "y <= 29/5"],
        ["ERR:"],
    ),
    (
        "June 2024 Q9 index-law geometric parameter",
        ["--alg", "solve((3^(2*(7-2*k)))/(3^(4*k-5))=(3^(2*(k-1)))/(3^(2*(7-2*k))),k)"],
        ["u = (3)^k, with u>0", "k = 5/2", "k = [5/2]"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2024 Q10 tangent derivative",
        ["--derive", "8*x-x^(5/2),x"],
        ["d/dx(8*x) = 8", "d/dx(-x^(5/2)) = -5/2*x^(3/2)", "dy/dx = - 5/2*x^(3/2) + 8"],
        ["ERR:"],
    ),
    (
        "June 2024 Q10 area split exact cancellation",
        ["--alg", "6912/875*sqrt(3/5)+384/35-6912/875*sqrt(3/5)"],
        ["384/35"],
        ["ERR:"],
    ),
    (
        "June 2024 Q14 balloon k from rate",
        ["--alg", "solve(0.9=k/sqrt(16),k)"],
        ["9/10 = k/sqrt(16)", "k = 18/5"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2024 Q14 balloon separable DE",
        ["--int", "de_solve(dr/dt=18/5/sqrt(r),r(10)=16)"],
        ["sqrt(r) dr = 18/5 dt", "2/3*r^(3/2) = 18/5*t + C", "C = 20/3", "r = (3/2*(18/5*t + 20/3))^(2/3)"],
        ["ERR:", "Integral not recognised"],
    ),
    (
        "June 2024 Q15 complete square positivity",
        ["--alg", "complete_square(k^2-4*k+5)"],
        ["k^2 - 4*k + 5", "h = b/(2a) = -2", "(k - 2)^2 + 1"],
        ["not a quadratic in x", "ERR:"],
    ),
    (
        "June 2024 Q12 R-form expression",
        ["--trig", "140*cos(theta)-480*sin(theta),method=rform"],
        ["sqrt(140^2+480^2)=500", "cos(alpha)=7/25", "sin(alpha)=24/25", "500*cos(theta+atan(24/7))"],
        ["Answer: = 140*cos", "ERR:"],
    ),
    (
        "Paper 1 range logistic transform",
        ["--alg", "range(e^x/(1+e^x))"],
        ["u = e^x", "u>0", "0 < y < 1"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "June 2019 Q5 reciprocal quadratic range",
        ["--alg", "range(21/(2*x^2+4*x+9))"],
        ["Discriminant = - 56*y^2 + 168*y", "Range: 0 < y <= 3", "0 < y <= 3"],
        ["0 <= y <= 3", "inspect graph/transform", "ERR:"],
    ),
    (
        "October 2021 Q1 factor theorem parameter",
        ["--alg", "solve(a*1^3+10*1^2-3*a*1-4=0,a,method=linear)"],
        ["- 2*a + 6 = 0", "a = 3", "a = [3]"],
        ["ERR:", "a = []"],
    ),
    (
        "October 2021 Q2 complete square",
        ["--alg", "complete_square(x^2-4*x+5)"],
        ["(x - 2)^2 + 1"],
        ["ERR:"],
    ),
    (
        "October 2021 Q3 recurrence quadratic",
        ["--alg", "compare((3*k-22)*(k-12)-24,3*k^2-58*k+240)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q3 recurrence integer root",
        ["--alg", "solve(3*k^2-58*k+240=0,k)"],
        ["k - 29/3 = +/-11/3", "k = 40/3", "k = 6"],
        ["ERR:", "k = []"],
    ),
    (
        "October 2021 Q4 log derivative cubic",
        ["--derive", "x^2+ln(2*x^2-4*x+5),x"],
        ["d/dx(ln(2*x^2 - 4*x + 5))", "2*x*(2*x^2 - 4*x + 5) + 4*x - 4 = 2*(2*x^3 - 4*x^2 + 7*x - 2)"],
        ["limite", "ERR:"],
    ),
    (
        "October 2021 Q5 geometric threshold equation",
        ["--alg", "solve((27/25)^(n-1)=13/4,n)"],
        ["(n - 1)*(3*ln(3) - 2*ln(5)) = ln(13/4)", "n = [(ln(13/4) + 3*ln(3) - 2*ln(5))/(3*ln(3) - 2*ln(5))]"],
        ["ERR:", "n = []"],
    ),
    (
        "October 2021 Q5 geometric threshold value",
        ["--alg", "(ln(13/4)+3*ln(3)-2*ln(5))/(3*ln(3)-2*ln(5)),method=numeric"],
        ["16.3149564889"],
        ["ERR:"],
    ),
    (
        "October 2021 Q5 geometric sum",
        ["--alg", "20000*(1-1.08^20)/(1-1.08),method=numeric"],
        ["915239.285962"],
        ["ERR:"],
    ),
    (
        "October 2021 Q6 vector angle cosine",
        ["--alg", "compare(27/(sqrt(50)*sqrt(18)),9/10)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q7 circle complete square",
        ["--alg", "complete_square(x^2+y^2-10*x+4*y+11)"],
        ["(x - 5)^2 + (y + 2)^2 = 18", "centre = (5,-2)", "r = sqrt(18)"],
        ["ERR:", "not a quadratic"],
    ),
    (
        "October 2021 Q7 tangent parameter",
        ["--alg", "solve((6*k+2)^2-40*(k^2+4*k+11)=0,k)"],
        ["k = -17 - 6*sqrt(5)", "k = -17 + 6*sqrt(5)"],
        ["ERR:", "k = []"],
    ),
    (
        "October 2021 Q8 exponential growth constant",
        ["--alg", "solve(2000=1000*e^(5*k),k)"],
        ["e^(5*k) = 2", "k = ln(2)/5"],
        ["ERR:"],
    ),
    (
        "October 2021 Q8 bacteria rate",
        ["--derive", "1000*e^((ln(2)/5)*t),t"],
        ["dy/dt = 200*e^(ln(2)/5*t)*ln(2)"],
        ["limite", "ERR:"],
    ),
    (
        "October 2021 Q8 bacteria crossing time",
        ["--alg", "(ln(1000)-ln(500))/(2/5*ln(2)/5),method=numeric"],
        ["12.5"],
        ["ERR:"],
    ),
    (
        "October 2021 Q9 repeated-linear partial fractions",
        ["--alg", "partfrac((50*x^2+38*x+9)/((5*x+2)^2*(1-2*x)))"],
        ["A=0", "B=1", "C=2", "1/(5*x + 2)^2 + 2/(- 2*x + 1)"],
        ["Start with (50*x^2", "ERR:"],
    ),
    (
        "October 2021 Q9 repeated-linear binomial expansion",
        ["--alg", "binomial((50*x^2+38*x+9)/((5*x+2)^2*(1-2*x)),x,0,2)"],
        ["Expand each linear factor", "9/4", "11/4*x", "203/16*x^2", "Valid for abs(x) < 2/5"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "October 2021 Q10 tan identity proof",
        ["--trig", "(1-cos(2*theta)+sin(2*theta))/(1+cos(2*theta)+sin(2*theta))=tan(theta),method=identity"],
        ["Use 1-cos(2*theta)=2*sin(theta)^2", "factor", "tan(theta)", "LHS = RHS"],
        ["Answer: x = []", "ERR:"],
    ),
    (
        "October 2021 Q10 hence solve",
        ["--trig", "(1-cos(4*x)+sin(4*x))/(1+cos(4*x)+sin(4*x))=3*sin(2*x),x,0,180,10,method=identity"],
        ["LHS=tan(2*x)", "sin(2*x)=0", "cos(2*x)=1/3", "35.3", "90", "144.7"],
        ["x = []", "ERR:"],
    ),
    (
        "October 2021 Q11 log-square definite integral",
        ["--int", "defint((ln(x))^2,x,2,4),method=parts"],
        ["u = ln(x)^2, dv = dx", "F(4) - F(2)", "4*ln(4)^2 - 2*ln(2)^2 + 4 + 6*ln(1/4)"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "October 2021 Q11 exact log form check",
        ["--alg", "compare(4*ln(4)^2 - 2*ln(2)^2 + 4 + 6*ln(1/4),14*ln(2)^2-12*ln(2)+4)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q12 golf quadratic model",
        ["--alg", "solve([27=14400*a+120*b+3,180*a+b=0],[a,b])"],
        ["a = -1/300", "b = 3/5"],
        ["ERR:", "(a,b) = []"],
    ),
    (
        "October 2021 Q12 golf range",
        ["--alg", "solve(-1/300*x^2+3/5*x+3=0,x)"],
        ["x ~= -4.868, 184.868"],
        ["ERR:", "x = []"],
    ),
    (
        "October 2021 Q13 parametric circle identity",
        ["--alg", "compare(((t^2+5)/(t^2+1)-3)^2+(4*t/(t^2+1))^2,4)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q14 rationalise before differentiating",
        ["--alg", "compare((x-4)/(2+sqrt(x)),sqrt(x)-2)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q14 simplified derivative",
        ["--derive", "sqrt(x)-2,x"],
        ["d/dx(sqrt(x)) = 1/(2*sqrt(x))", "dy/dx = 1/(2*sqrt(x))"],
        ["limite", "ERR:"],
    ),
    (
        "October 2021 Q15 odd contradiction expansion",
        ["--alg", "compare((2*p+1)^3+5,8*p^3+12*p^2+6*p+6)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q15 even factor form",
        ["--alg", "compare(8*p^3+12*p^2+6*p+6,2*(4*p^3+6*p^2+3*p+3))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2020 Q6 R sin form",
        ["--trig", "sin(x)+2*cos(x),method=rform"],
        ["R=sqrt(1^2+2^2)=sqrt(5)", "cos(alpha)=1/sqrt(5)", "sin(alpha)=2/sqrt(5)", "sqrt(5)*sin(x+atan(2))"],
        ["Answer: = sin(x)", "ERR:"],
    ),
    (
        "June 2022 Q2 factor theorem parameter",
        ["--alg", "solve((-2-4)*((-2)^2-3*(-2)+k)-42=0,k)"],
        ["- 6*(k + 10) - 42 = 0", "k = -17", "k = [-17]"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2022 Q3 circle complete square",
        ["--alg", "complete_square(x^2+y^2-10*x+16*y-80)"],
        ["x^2 - 10*x = (x - 5)^2 - 25", "y^2 + 16*y = (y + 8)^2 - 64", "(x - 5)^2 + (y + 8)^2 = 169", "centre = (5,-8)", "r = 13"],
        ["ERR:", "not a quadratic"],
    ),
    (
        "June 2022 Q4 Riemann integral log",
        ["--int", "defint(2/x,x,21/10,63/10)"],
        ["F(63/10) - F(21/10)", "2*ln(3)"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "June 2022 Q5 tree model simultaneous equations",
        ["--alg", "solve([2*a+b=2.6^2,10*a+b=5.1^2],[a,b])"],
        ["a = 77/32", "b = 779/400", "(a,b) = [(77/32,779/400)]"],
        ["ERR:", "(a,b) = []"],
    ),
    (
        "June 2022 Q6 cubic from stationary roots",
        ["--alg", "solve(8=a*2*(2-6)^2,a)"],
        ["8 = 32*a", "a = 1/4", "a = [1/4]"],
        ["ERR:", "a = []"],
    ),
    (
        "June 2022 Q8 speed model derivative",
        ["--derive", "(10-0.4*t)*ln(t+1),t"],
        ["f1 = - 2/5*t + 10", "f1' = -2/5", "f2' = 1/(t + 1)", "dy/dt = - 2/5*ln(t + 1) + (- 2/5*t + 10)*1/(t + 1)"],
        ["ERR:", "limite"],
    ),
    (
        "June 2022 Q10 bee wasp exponential crossing",
        ["--alg", "solve(45+220*e^(0.05*t)=10+800*e^(-0.05*t),t)"],
        ["u = e^(1/20*t), u > 0", "220*u - 800/u + 35 = 0", "rejected, u > 0", "t = [20*ln(sqrt(28209)/88 - 7/88)]"],
        ["ERR:", "t = []"],
    ),
    (
        "June 2022 Q11 cubic intersection factor",
        ["--alg", "factor(2*x^3+15*x^2-42*x+17)"],
        ["(x - 1/2)*(2*x^2 + 16*x - 34)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q12 parts definite integral",
        ["--int", "defint(x^3*ln(x),x,1,e^2),method=parts"],
        ["u = ln(x), dv = x^3 dx", "F(e^2) = 7/16*e^(8)", "F(1) = -1/16", "7/16*e^(8) + 1/16"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "June 2022 Q13 arithmetic series quadratic",
        ["--alg", "solve(n^2-26*n+160=0,n)"],
        ["(n - 13)^2 = 9", "n = 16", "n = 10", "n = [16, 10]"],
        ["ERR:", "n = []"],
    ),
    (
        "June 2022 Q14 compound angle proof target",
        ["--trig", "2*sin(x-60)-cos(x-30),target=tan(x)-3*sqrt(3),method=identity"],
        ["compound-angle expansions", "1/2*sin(x) - 3*sqrt(3)/2*cos(x) = 0", "tan(x) = 3*sqrt(3)"],
        ["Source != target", "ERR:"],
    ),
    (
        "June 2022 Q14 hence trig solve",
        ["--trig", "2*sin(2*theta)=cos(2*theta+30),theta,0,180,10,method=identity"],
        ["5/2*sin(2*theta) - sqrt(3)/2*cos(2*theta)", "theta = [9.55330267543, 99.5533026754]"],
        ["theta = []", "ERR:"],
    ),
    (
        "June 2022 Q15 optimisation derivative",
        ["--derive", "0.8*r^2+1680/r,r"],
        ["d/dr(4/5*r^2) = 8/5*r", "d/dr(1680/r) = -1680*r^-2", "dy/dr = 8/5*r - 1680*r^-2"],
        ["ERR:", "limite"],
    ),
    (
        "June 2022 Q15 optimisation stationary radius",
        ["--alg", "solve(1.6*r-1680/r^2=0,r,0,inf)"],
        ["Interval: r in [0, inf]", "r = 10.1639635681", "r = [10.1639635681]"],
        ["ERR:", "r = []"],
    ),
    (
        "June 2022 Q16 parametric area identity",
        ["--trig", "(2*sin(2*t)+3*sin(t))*16*sin(t)*cos(t),target=8-8*cos(4*t)+48*sin(t)^2*cos(t),method=identity"],
        ["Source = target", "8 - 8*cos(4*t) + 48*sin(t)^2*cos(t)"],
        ["Source != target", "ERR:"],
    ),
    (
        "June 2022 Q16 parametric exact area",
        ["--int", "defint(8-8*cos(4*t)+48*sin(t)^2*cos(t),t,0,pi/4)"],
        ["F(pi/4) - F(0)", "4*sqrt(2) + 2*pi"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "June 2023 Q1 power integral",
        ["--int", "(1/3)*sqrt(x)*(2*x-5)"],
        ["sqrt(x)*(2*x - 5) = 2*sqrt(x)*x - 5*sqrt(x)", "x^(3/2)", "1/3*(4/5*x^(5/2) - 10/3*x^(3/2)) + C"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "June 2023 Q2 factor theorem parameter",
        ["--alg", "solve(4*a^3+5*a^2-6*a=0,a)"],
        ["a*(4*a^2 + 5*a - 6) = 0", "a = 3/4", "a = [-2, 0, 3/4]"],
        ["ERR:", "a = []"],
    ),
    (
        "June 2023 Q2 cubic exact roots",
        ["--alg", "solve(4*x^3+5*x^2-10*x=0,x)"],
        ["x*(4*x^2 + 5*x - 10) = 0", "x = 0", "x = -5/8 - sqrt(185)/8", "x = -5/8 + sqrt(185)/8"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2023 Q3 vector magnitude inequality",
        ["--alg", "solve(2^2+4^2+a^2>38,a)"],
        ["a^2 - 18 > 0", "a > 3*sqrt(2)"],
        ["ERR:", "a = []"],
    ),
    (
        "June 2023 Q4 small-angle stationary point",
        ["--alg", "solve(2*x+1/2*(1-x^2/2)=0,x)"],
        ["- 1/2*x^2 + 4*x + 1 = 0", "x = 4 - 3*sqrt(2)", "x ~= -0.243"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2023 Q4 tangent gradient",
        ["--alg", "evalat(2*x+1/2*cos(x),x,0)"],
        ["f(0) = 1/2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 table simultaneous equations",
        ["--alg", "solve([a+2*b=51,a+b=28],[a,b])"],
        ["a = 5", "b = 23", "(a,b) = [(5,23)]"],
        ["ERR:", "(a,b) = []"],
    ),
    (
        "June 2023 Q6 logarithm laws",
        ["--alg", "compare(log(2,8+64/x),3+log(2,x+8)-log(2,x))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2023 Q7 square-root range",
        ["--alg", "range(3+sqrt(x-2),x>2)"],
        ["Interval of interest: x on (2, inf]", "Range: y > 3", "y > 3"],
        ["y >= 3", "ERR:"],
    ),
    (
        "June 2023 Q7 inverse square-root function",
        ["--alg", "inverse(3+sqrt(x-2),x)"],
        ["y = f(x) = sqrt(x - 2) + 3", "f^-1(x) = (x - 3)^2 + 2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q7 composite exact value",
        ["--alg", "evalat(15/(x-3),x,3+sqrt(6-2))"],
        ["f(sqrt(4) + 3) = 15/2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q7 constant from composite equation",
        ["--alg", "solve(3+abs(a)=15/(a-3),a)"],
        ["a >= 0: a + 3 = 15/(a - 3)", "a = [0 + 2*sqrt(6)]"],
        ["ERR:", "a = []"],
    ),
    (
        "June 2023 Q8 stage sector and triangle area",
        ["--alg", "1/2*12^2*2.3+2*(1/2*((35-27.6)/2+12)*7.5*sin((pi-2.3)/2)),method=numeric"],
        ["471/4*sin((pi - 23/10)/2) + 828/5", "213.699396164"],
        ["ERR:"],
    ),
    (
        "June 2023 Q9 geometric sequence roots",
        ["--alg", "solve((12-3*k)^2=(3*k+4)*(k+16),k)"],
        ["(- 3*k + 12)^2 = (3*k + 4)*(k + 16)", "k = 20", "k = 2/3"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2023 Q9 convergent geometric sum",
        ["--alg", "64/(1-(-3/4))"],
        ["256/7"],
        ["ERR:"],
    ),
    (
        "June 2023 Q10 circle centre radius",
        ["--alg", "complete_square(x^2+y^2+6*k*x-2*k*y+7)"],
        ["x^2 + 6*k*x = (x + 3*k)^2 - 9*k^2", "y^2 - 2*k*y = (y - k)^2 - k^2", "(x + 3*k)^2 + (y - k)^2 - 10*k^2 + 7"],
        ["ERR:"],
    ),
    (
        "June 2023 Q10 line-circle discriminant range",
        ["--alg", "solve((2*k-4)^2-20*(2*k+8)>0,k)"],
        ["N = 0: k = 7 - sqrt(85), 7 + sqrt(85)", "k < 7 - sqrt(85) or k > 7 + sqrt(85)"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2023 Q11 log graph exponential model",
        ["--alg", "1000*(10^(-0.021))^24,method=numeric"],
        ["1000*10^(-63/125)", "313.328572432"],
        ["ERR:"],
    ),
    (
        "June 2023 Q12 first-principles sine derivative",
        ["--derive", "sin(x),x,method=first_principles"],
        ["[sin(x+h)-sin(x)]/h", "sin(x+h) = sin(x)cos(h)+cos(x)sin(h)", "h->0: (cos(h)-1)/h->0; sin(h)/h->1", "d/dx sin(x) = cos(x)"],
        ["ERR:", "No first-principles route"],
    ),
    (
        "June 2023 Q13 roller-coaster quadratic model",
        ["--alg", "solve([a=60,2=a-b*(0-20)^2],[a,b])"],
        ["a = 60", "b = 29/200", "(a,b) = [(60,29/200)]"],
        ["ERR:", "(a,b) = []"],
    ),
    (
        "June 2023 Q13 cosine model period check",
        ["--trig", "29*cos((9*t+180)*pi/180)+31=2,t,0,120,10"],
        ["sin(9*t+alpha) = -1", "t = [0, 40, 80, 120]"],
        ["ERR:", "t = []"],
    ),
    (
        "June 2023 Q14 odd cubic difference proof",
        ["--alg", "expand((n+1)^3-n^3)"],
        ["= 3n(n+1)+1", "n; n+1 are consecutive; n(n+1) is even", "3n(n+1)+1 is odd"],
        ["ERR:"],
    ),
    (
        "June 2023 Q15 quotient derivative",
        ["--derive", "7*x*e^x/sqrt(e^(3*x)-2),x,method=quotient"],
        ["u = 7*x*e^x, u' = 7e^x+7x*e^x", "Numerator = 7e^x[2(e^(3x)-2)(1+x)-3xe^(3x)]", "A = -4, B = -4", "dy/dx = 7e^x*(e^(3x)*(2 - x) - 4x - 4)/(2*(e^(3x) - 2)^(3/2))"],
        ["ERR:", "Answer: d/dx"],
    ),
    (
        "June 2023 Q15 lower-root sign change",
        ["--alg", "evalat((2*e^(3*x)-4)/(e^(3*x)+4)-x,x,0.4315),method=numeric"],
        ["f(863/2000) = -0.000297469159484"],
        ["ERR:"],
    ),
    (
        "June 2023 Q15 upper endpoint sign change",
        ["--alg", "evalat((2*e^(3*x)-4)/(e^(3*x)+4)-x,x,0.4325),method=numeric"],
        ["f(173/400) = 0.000947950543881"],
        ["ERR:"],
    ),
]


def flat(s: str) -> str:
    return " ".join(s.replace("\r", "").split())


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [n for n in needles if not markers_present(out, [n])]
    bad = [b for b in banned if b in out]
    if proc.returncode:
        misses.append(f"returncode={proc.returncode}")
    if misses or bad:
        return [f"{name}: missing={misses} banned={bad}\n{out}"]
    return []


def main() -> int:
    failures: list[str] = []
    for name, args, needles, banned in CASES:
        failures.extend(run_case(name, args, needles, banned))
    if failures:
        print("\n\n".join(failures))
        return 1
    print(f"edexcel_paper1_downloads ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Paper 2 audit cases from local Edexcel QP/MS PDFs.

Marker checks cover answer + exam-method lines for commandable paper parts.
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
        "June 2018 Q1 range restricted fractional linear",
        ["--alg", "range((2*x+5)/(x-3),x,5,inf)"],
        ["horizontal asymptote y = 2", "Endpoint gives y = 15/2", "2 < y <= 15/2"],
        ["unrestricted on interval", "inspect graph/transform", "ERR:"],
    ),
    (
        "June 2018 Q1 inverse fractional linear",
        ["--alg", "inverse((2*x+5)/(x-3),x)"],
        ["x = f(y)", "(3*x + 5)/(x - 2)"],
        ["Given domain: x.", "ERR:"],
    ),
    (
        "June 2018 Q4 finite AP/GP sum",
        ["--alg", "sum(3+5*r+2^r,r,1,16)"],
        ["sum_{r = 1}^16", "3*(16)", "5*(16*17/2)", "(2^17-2^1)/(2-1)", "131798"],
        ["Expected )", "ERR:"],
    ),
    (
        "June 2019 Q1 same-base symbolic exponent",
        ["--alg", "solve(2^x*4^y=1/(2*sqrt(2)),y)"],
        ["2^(x+2*y) = 2^(-3/2)", "x+2*y = -3/2", "2*y = - x - 3/2", "y = -1/2*x - 3/4"],
        ["LHS - RHS", "ERR:"],
    ),
    (
        "June 2019 Q8i infinite geometric sum",
        ["--alg", "sum(20*(1/2)^r,r,4,inf)"],
        ["sum_{r = 4}^inf", "|common ratio| < 1", "5/4/(1-1/2)", "5/2"],
        ["Expected )", "ERR:"],
    ),
    (
        "June 2019 Q8ii telescoping log sum",
        ["--alg", "sum(log(5,(n+2)/(n+1)),n,1,48)"],
        ["sum_{n = 1}^48", "log(5,25)", "2"],
        ["Expected )", "ERR:"],
    ),
    (
        "June 2018 Q6 cubic factor theorem",
        ["--alg", "factor(-3*x^3+8*x^2-9*x+10)"],
        ["(x - 2)", "- 3*x^2 + 2*x - 5"],
        ["numeric coeffs needed", "ERR:"],
    ),
    (
        "June 2018 Q7i sec equation",
        ["--trig", "4*sin(x)=sec(x),x,0,pi/2,10,method=identity"],
        ["sec(x)=1/cos(x)", "sin(2x)=1/2", "pi/12", "5*pi/12"],
        ["x = []", "ERR:"],
    ),
    (
        "June 2018 Q7ii R-form solve",
        ["--trig", "5*sin(theta)-5*cos(theta)=2,theta,0,360,10,method=rform"],
        ["R=", "alpha=", "sin(theta+alpha)", "theta = [61.4299401894, 208.570059811]"],
        ["Write a*sin(A)+b*cos(A)=R*sin(A+alpha).\n3. Then solve", "ERR:"],
    ),
    (
        "June 2018 Q9 first principles cos theta",
        ["--derive", "cos(theta),theta,method=first_principles"],
        ["[cos(theta+h)-cos(theta)]/h", "cos(A)-cos(B)", "d/dtheta cos(theta) = -sin(theta)"],
        ["d/dtheta(cos(theta))", "ERR:"],
    ),
    (
        "June 2018 Q11 proper partial fractions",
        ["--alg", "partfrac((1+11*x-6*x^2)/((x-3)*(1-2*x)))"],
        ["Divide:", "= 3 +", "A/(x - 3)+B/(- 2*x + 1)", "3 + 4/(x - 3) - 2/(- 2*x + 1)"],
        ["3 + +", "ERR:"],
    ),
    (
        "June 2018 Q12 double-angle tan identity proof",
        ["--trig", "1-cos(2*theta)=tan(theta)*sin(2*theta),method=identity"],
        ["1-cos(2*theta) = 2sin(theta)^2", "tan(theta)*sin(2*theta)", "= 2sin(theta)^2"],
        ["theta = []", "x = []", "ERR:"],
    ),
    (
        "June 2019 Q12 compound-angle fraction identity",
        ["--trig", "cos(3*theta)/sin(theta)+sin(3*theta)/cos(theta),target=2*cot(2*theta),method=identity"],
        ["cos(3*theta)*cos(theta)+sin(3*theta)*sin(theta)", "cos(A-B)", "cos(2*theta)/(sin(theta)*cos(theta))", "sin(2*theta)=2sin(theta)cos(theta)", "2*cot(2*theta)"],
        ["Source = target.", "ERR:"],
    ),
    (
        "Paper 2 identity tan polynomial",
        ["--trig", "(sec(x)^2-5)*(1-cos(2*x))=3*tan(x)^2*sin(2*x),x,-pi/2,pi/2,10,method=identity"],
        ["Let t=tan(x)", "t^2-3t-4=0", "-pi/4", "atan(4)"],
        ["x = []", "ERR:"],
    ),
    (
        "October 2020 Q3 log equation",
        ["--alg", "solve(2*log(10,4-x)=log(10,x+8),x)"],
        ["Domain", "x = 8 rejected by domain", "x = [1]"],
        ["Parser error", "ERR:"],
    ),
    (
        "October 2020 Q4 binomial coefficient",
        ["--alg", "binom_coeff((a+2*x)^7,x,4)"],
        ["Term in x^4", "C(7,4)", "Coefficient = 560*a^3"],
        ["ERR:"],
    ),
    (
        "October 2020 Q4 coefficient equation",
        ["--alg", "solve(560*a^3=15120,a)"],
        ["560*a^3 - 15120 = 0", "a = [3]"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 exponential intersection exact",
        ["--alg", "solve(3*2^x=15-2^(x+1),x)"],
        ["2^(x + 1) = 2*2^x", "5*2^x = 15", "2^x = 3", "x = log(2,3)"],
        ["x = [1.584", "ERR:"],
    ),
    (
        "October 2020 Q6 algebraic division integral",
        ["--int", "defint((x^2+8*x-3)/(x+2),x,0,6),method=div"],
        ["Q = x + 6, R = -15", "54", "ln(8)"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "October 2020 Q7 quotient-log derivative simplification",
        ["--derive", "(4*x^2+x)/(2*sqrt(x))-4*ln(x),x"],
        ["2*x^(3/2) + 1/2*sqrt(x)", "dy/dx = 3*sqrt(x) + 1/(4*sqrt(x)) - 4/x", "(12*x^2 + x - 16*sqrt(x))/(4*x*sqrt(x))"],
        ["limite", "ERR:"],
    ),
    (
        "October 2020 Q10 triple-angle solve",
        ["--trig", "1-cos(3*x)=sin(x)^2,x,-90,180,10,method=identity"],
        ["cos(3x)=4cos(x)^3-3cos(x)", "4c^2-c-3", "138.6"],
        ["x = []", "ERR:"],
    ),
    (
        "October 2020 Q12 trig power definite integral",
        ["--int", "defint(60*sin(t)*cos(t)^2,t,0,pi/2)"],
        ["u=cos(t)", "du/dt=-sin(t)", "F(t) = -20*cos(t)^3", "20"],
        ["du=-1*sin(t) dx", "ERR:"],
    ),
    (
        "October 2020 Q13 log-fraction range",
        ["--alg", "range((3*log(x)-7)/(log(x)-2))"],
        ["u = ln(x)", "y != 3"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "October 2020 Q13 log-fraction derivative",
        ["--derive", "(3*ln(x)-7)/(ln(x)-2),x"],
        ["dy/dx = 1/(x*(ln(x) - 2)^2)", "x > 0", "(ln(x) - 2)^2 > 0"],
        ["limite", "ERR:"],
    ),
    (
        "October 2020 Q13 log-fraction inequality",
        ["--alg", "solve((3*ln(a)-7)/(ln(a)-2)>0,a)"],
        ["u = ln(a)", "u < 2 or u > 7/3", "0 < a < e^2 or a > e^(7/3)"],
        ["Unexpected character", "ERR:"],
    ),
    (
        "October 2020 Q15 fifth-root exact solve",
        ["--alg", "solve(r^5=3,r)"],
        ["r^5 = 3", "r = (3)^(1/5)", "r = [(3)^(1/5)]"],
        ["r = [1.245", "ERR:"],
    ),
    (
        "October 2021 implicit polynomial powers",
        ["--derive", "p*x^3+q*x*y+3*y^2=26,x,method=implicit"],
        ["F_x + F_y*dy/dx = 0", "dy/dx"],
        ["positive bases", "ERR:"],
    ),
    (
        "October 2021 Q1 large finite AP sum",
        ["--alg", "sum(16+(r-1)*2/5,r,1,500)"],
        ["sum_{r = 1}^500", "78/5*(500)", "2/5*(500*501/2)", "57900"],
        ["Expected )", "ERR:"],
    ),
    (
        "October 2021 Q2 quadratic range",
        ["--alg", "range(7-2*x^2)"],
        ["Domain: all real x", "Range: y <= 7", "y <= 7"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "October 2021 Q2 inverse fractional linear",
        ["--alg", "inverse(3*x/(5*x-1),x)"],
        ["x = f(y)", "f^-1(x) = x/(5*x - 3)", "Domain: x != 3/5"],
        ["ERR:"],
    ),
    (
        "October 2021 Q3 log equation",
        ["--alg", "solve(log(3,12*y+5)-log(3,1-3*y)=2,y)"],
        ["Domain: 12*y + 5 > 0", "(12*y + 5)/(- 3*y + 1) - 9 = 0", "y = [4/39]"],
        ["ERR:", "y = []"],
    ),
    (
        "October 2021 Q4 small angle quadratic",
        ["--trig", "4*sin(theta/2)+3*cos(theta)^2,method=small"],
        ["sin(theta/2) ~ theta/2", "cos(theta) ~ 1 - (theta)^2/2", "2*theta - 3*theta^2 + 3"],
        ["unsupported", "ERR:"],
    ),
    (
        "October 2021 Q5 derivatives",
        ["--derive", "5*x^4-24*x^3+42*x^2-32*x+11,x"],
        ["d/dx(5*x^4) = 20*x^3", "dy/dx = 20*x^3 - 72*x^2 + 84*x - 32"],
        ["limite", "ERR:"],
    ),
    (
        "October 2021 Q5 second derivative sign",
        ["--derive", "20*x^3-72*x^2+84*x-32,x"],
        ["d/dx(20*x^3) = 60*x^2", "dy/dx = 60*x^2 - 144*x + 84"],
        ["limite", "ERR:"],
    ),
    (
        "October 2021 Q6 sector area identity",
        ["--alg", "compare(2*(1/2*r^2*((pi-theta)/2))+1/2*(2*r)^2*theta,1/2*r^2*(3*theta+pi))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q6 sector perimeter identity",
        ["--alg", "compare(4*r+2*r*((pi-theta)/2)+2*r*theta,r*(4+pi+theta))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q7 tangent derivative",
        ["--derive", "x^3-10*x^2+27*x-23,x"],
        ["dy/dx = 3*x^2 - 20*x + 27"],
        ["limite", "ERR:"],
    ),
    (
        "October 2021 Q7 second intersection",
        ["--alg", "solve(x^3-10*x^2+27*x-23=2*x-23,x)"],
        ["x*(x^2 - 10*x + 25) = 0", "x = [0, 5]"],
        ["ERR:", "x = []"],
    ),
    (
        "October 2021 Q7 exact enclosed area",
        ["--int", "defint((x^3-10*x^2+27*x-23)-(2*x-23),x,0,5)"],
        ["F(5) - F(0)", "F(5) = 625/12", "625/12"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "October 2021 Q8 implicit constants",
        ["--alg", "solve([p*(-1)^3+q*(-1)*(-4)+3*(-4)^2=26,(-3*p*(-1)^2-q*(-4))/(q*(-1)+6*(-4))=26/19],[p,q])"],
        ["p = 4*q + 22", "q = -5", "(p,q) = [(2,-5)]"],
        ["ERR:", "(p,q) = []"],
    ),
    (
        "October 2021 Q9 alternating geometric sum",
        ["--alg", "sum((-3/4)^n,n,2,inf)"],
        ["sum_{n = 2}^inf", "|common ratio| < 1", "9/16/(1--3/4)", "9/28"],
        ["Expected )", "ERR:"],
    ),
    (
        "October 2021 Q10 log-linear constants",
        ["--alg", "(0.45-0)/(0.21-(-0.7)),method=numeric"],
        ["45/91", "0.494505494505"],
        ["ERR:"],
    ),
    (
        "October 2021 Q10 log-linear coefficient",
        ["--alg", "10^(0.45-((0.45)/(0.91))*0.21),method=numeric"],
        ["10^(9/26)", "2.21898234146"],
        ["ERR:"],
    ),
    (
        "October 2021 Q11 modulus critical point",
        ["--alg", "solve(2*x-3*k=0,x)"],
        ["2 != 0", "x = 3*k/2"],
        ["ERR:"],
    ),
    (
        "October 2021 Q12 root shift substitution integral",
        ["--int", "defint(x/(1+sqrt(x)),x,0,16),method=sub"],
        ["u = 1+sqrt(x)", "x = (u-1)^2", "Integral_1^5 2(u-1)^3/u du", "104/3 - 2*ln(5)"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "October 2021 Q13 parametric derivative",
        ["--derive", "x=sin(2*theta),y=cosec(theta)^3,theta,x,method=param"],
        ["dx/dt = 2*cos(2*theta)", "dy/dt = -3*cosec(theta)^3*cot(theta)", "dy/dx = -3*cosec(theta)^3*cot(theta)/(2*cos(2*theta))"],
        ["ERR:", "dx/dt = 0"],
    ),
    (
        "October 2021 Q13 exact gradient",
        ["--alg", "compare((-3*cosec(pi/6)^3*cot(pi/6))/(2*cos(pi/3)),-24*sqrt(3))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "October 2021 Q14 tank separable DE",
        ["--int", "de_solve(dh/dt=(24-5*h)/1200,h(0)=2)"],
        ["1/(- 5*h + 24) dh = 1/1200 dt", "h(0) = 2", "h = (14*e^(-1/240*t) - 24)/-5"],
        ["ERR:", "1/h = 1/h"],
    ),
    (
        "October 2021 Q14 tank never full",
        ["--alg", "solve(-14/5*e^(-t/240)+24/5=5,t)"],
        ["e^(-t/240) > 0", "t = []"],
        ["ERR:"],
    ),
    (
        "October 2021 Q15 R cos form",
        ["--trig", "2*cos(theta)-sin(theta),method=rform"],
        ["R = sqrt(2^2+1^2) = sqrt(5)", "alpha = atan(1/2)", "sqrt(5)*cos(theta+atan(1/2))"],
        ["ERR:"],
    ),
    (
        "October 2021 Q15 water-level crossings",
        ["--trig", "3+4*cos(0.5*t)-2*sin(0.5*t)=0,t,0,4*pi,10,method=rform,rad"],
        ["R = sqrt(4^2+-2^2)", "sin(1/2*t+alpha) = -0.67082039325", "t = [3.68492634122, 7.02685383713]"],
        ["t = []", "ERR:"],
    ),
    (
        "October 2021 Q15 time below water",
        ["--alg", "4*acos(3/(2*sqrt(5))),method=numeric"],
        ["4*acos(3/(2*sqrt(5)))", "3.34192749591"],
        ["ERR:"],
    ),
    (
        "June 2022 binomial validity",
        ["--alg", "binomial((1-9*x/4)^(-1/2),x,0,3)"],
        ["1 + 9/8*x", "243/128*x^2", "3645/1024*x^3", "Valid for abs(x) < 4/9"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "June 2022 Q1 modulus equation",
        ["--alg", "solve(abs(3-2*x)=7+x,x)"],
        ["- 2*x + 3 >= 0", "x = -4/3", "x = 10", "x = [-4/3, 10]"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2022 Q2 exponential equation",
        ["--alg", "solve(4^x=100,x)"],
        ["4^x = 100", "2*x*ln(2) = ln(100)", "x = ln(100)/(2*ln(2))"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2022 Q4 scaled first principles derivative",
        ["--derive", "2*x^2,x,method=first_principles"],
        ["f(x) = 2*x^2", "(x+h)^2-x^2 = 2*x*h+h^2", "= 4*x+2*h", "d/dx 2*x^2 = 4*x"],
        ["No first-principles route", "ERR:"],
    ),
    (
        "June 2022 Q6 trig model derivative",
        ["--derive", "8*sin(x/2)-3*x+9,x"],
        ["d/dx(8*sin(x/2)) = 4*cos(x/2)", "dy/dx = 4*cos(x/2) - 3"],
        ["limite", "ERR:"],
    ),
    (
        "June 2022 Q6 radian stationary solve",
        ["--trig", "4*cos(x/2)-3=0,x,10,16,10,method=identity,rad"],
        ["cos(x/2) = 3/4", "10 <= x <= 16", "x = [11.1209021187, 14.01183911]"],
        ["x = []", "360", "ERR:"],
    ),
    (
        "June 2022 Q7 sqrt binomial",
        ["--alg", "binomial((4-9*x)^(1/2),x,0,3)"],
        ["T1 = -9/4*x", "T2 = -81/64*x^2", "T3 = -729/512*x^3", "Valid for abs(x) < 4/9"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "June 2022 Q8 exact area integral",
        ["--int", "defint(-(x-2)*(x-4)/(4*sqrt(x)),x,2,4)"],
        ["F(4) - F(2)", "F(4) = -16/5", "F(2) = -12*sqrt(2)/5", "12/5*sqrt(2) - 16/5"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "June 2022 Q9 Ferris-wheel alpha",
        ["--trig", "1=50*sin(alpha*pi/180),alpha,0,90,10"],
        ["sin(alpha*pi/180) = 0.02", "0 <= alpha <= 90", "alpha = [1.14599199839]"],
        ["alpha = []", "ERR:"],
    ),
    (
        "June 2022 Q10 inverse value",
        ["--alg", "solve(3/2=(8*x+5)/(2*x+3),x)"],
        ["Domain: 2*x + 3 != 0", "x = -1/10", "x = [-1/10]"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2022 Q10 fractional-linear decomposition",
        ["--alg", "partfrac((8*x+5)/(2*x+3))"],
        ["= 4 - 7/(2*x + 3)", "4 - 7/2/(x + 3/2)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q10 range of composed inverse",
        ["--alg", "range((8*x+5)/(2*x+3),x,0,4)"],
        ["Interval of interest", "y(0) = 5/3", "y(4) = 37/11", "5/3 <= y <= 37/11"],
        ["unrestricted on interval", "ERR:"],
    ),
    (
        "June 2022 Q12 quotient derivative factor",
        ["--derive", "e^(3*x)/(4*x^2+k),x,method=quotient"],
        ["y' = (u'v-u*v')/v^2", "3*e^(3*x)*(4*x^2 + k) - 8*e^(3*x)*x", "e^(3*x)*(3*(4*x^2 + k) - 8*x)", "dy/dx = (e^(3*x)*(3*(4*x^2 + k) - 8*x))/(4*x^2 + k)^2"],
        ["Answer: d/dx", "ERR:"],
    ),
    (
        "June 2022 Q12 discriminant inequality",
        ["--alg", "solve(64-144*k>=0,k)"],
        ["N = 0: k = 4/9", "k <= 4/9"],
        ["16/309", "ERR:"],
    ),
    (
        "June 2022 Q14 separable DE partial fractions",
        ["--alg", "partfrac(3/((2*t-1)*(t+1)))"],
        ["A/(2*t - 1)+B/(t + 1)", "A = 2, B = -1", "2/(2*t - 1) - 1/(t + 1)"],
        ["ERR:"],
    ),
    (
        "June 2022 Q14 separable DE solution",
        ["--int", "de_solve(dV/dt=3*V/((2*t-1)*(t+1)),V(2)=3)"],
        ["1/V dV = 3/((2*t - 1)*(t + 1)) dt", "PF: A/(2*t - 1)+B/(t + 1)", "C = ln(abs(3))", "V = 3*abs((2*t - 1))/abs((t + 1))"],
        ["ERR:", "1/V = 1/V"],
    ),
    (
        "June 2022 Q15 trig quadratic solve",
        ["--trig", "4*sin(theta)^2-52*sin(theta)+25=0,theta,pi/2,pi,10,method=identity"],
        ["u = sin(theta)", "Reject u = 25/2", "sin(theta) = 1/2", "theta = [5*pi/6]"],
        ["theta = []", "ERR:"],
    ),
    (
        "June 2022 Q15 exact GP sum equivalence",
        ["--alg", "compare((-6*sqrt(3))/(1+sqrt(3)/3),9*(1-sqrt(3)))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2022 Q16 parametric derivative",
        ["--derive", "x=2*tan(t)+1,y=2*sec(t)^2+3,t,x,method=param"],
        ["dx/dt = 2*sec(t)^2", "dy/dt = 4*sec(t)^2*tan(t)", "dy/dx = 2*tan(t)"],
        ["ERR:", "dx/dt = 0"],
    ),
    (
        "June 2022 Q16 normal line",
        ["--alg", "expand(-1/2*(x-3)+7)"],
        ["- 1/2*x + 17/2"],
        ["ERR:"],
    ),
    (
        "June 2022 Q16 cartesian identity",
        ["--trig", "2*sec(t)^2+3,target=1/2*(2*tan(t)+1-1)^2+5,method=identity"],
        ["Source = 2*sec(t)^2 + 3", "Target = 1/2*(2*tan(t))^2 + 5", "Source = target"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2022 Q16 two-intersection lower bound",
        ["--alg", "solve(1-4*(11-2*k)>0,k)"],
        ["8*k - 43 > 0", "N = 0: k = 43/8", "k > 43/8"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2022 Q16 endpoint upper bound",
        ["--alg", "solve(7=-1/2*(-1)+k,k)"],
        ["k = 13/2", "k = [13/2]"],
        ["ERR:", "k = []"],
    ),
    (
        "June 2023 Q1 second derivative",
        ["--derive", "x^3+2*x^2-8*x+5,x,method=second"],
        ["d2y/dx2 = 6*x + 4"],
        ["limite", "ERR:"],
    ),
    (
        "June 2023 Q1 concavity boundary",
        ["--alg", "solve(6*x+4<=0,x)"],
        ["N = 0: x = -2/3", "x <= -2/3"],
        ["ERR:", "x = []"],
    ),
    (
        "June 2023 Q2 recurrence u2",
        ["--alg", "35+7*cos(pi/2)-5*(-1)"],
        ["40"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 recurrence u3",
        ["--alg", "40+7*(-1)-5*(1)"],
        ["28"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 recurrence u4",
        ["--alg", "28+7*0-5*(-1)"],
        ["33"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 recurrence cycle sum",
        ["--alg", "6*(35+40+28+33)+35"],
        ["851"],
        ["ERR:"],
    ),
    (
        "June 2023 log equation",
        ["--alg", "solve(log(2,x+3)+log(2,x+10)=2+2*log(2,x),x)"],
        ["Domain", "x = -5/3 rejected by domain", "x = [6]"],
        ["Parser error", "ERR:"],
    ),
    (
        "June 2023 Q4 exponential constant",
        ["--alg", "solve(85=A+30,A,method=linear)"],
        ["A = 55", "A = [55]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 cooling derivative",
        ["--derive", "A*e^(-B*t)+30,t"],
        ["d/dt(A*e^(-B*t)) = -A*e^(-B*t)*B", "dy/dt = -A*e^(-B*t)*B"],
        ["limite", "ERR:"],
    ),
    (
        "June 2023 Q4 cooling rate constant",
        ["--alg", "solve(-55*B=-7.5,B,method=linear)"],
        ["B = 3/22", "B = [3/22]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 stationary parameter",
        ["--alg", "solve(2*3^3-9*3^2+5*3+k=0,k,method=linear)"],
        ["k = 12", "k = [12]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 integrated curve",
        ["--int", "2*x^3-9*x^2+5*x+12"],
        ["1/2*x^4 - 3*x^3 + 5/2*x^2 + 12*x + C"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "June 2023 Q5 y intercept constant",
        ["--alg", "solve(1/2*3^4-3*3^3+5/2*3^2+12*3+c=-10,c,method=linear)"],
        ["c = -28", "c = [-28]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 parallel vector ratio",
        ["--alg", "compare(50/10,120/24)"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2023 Q6 exact side length",
        ["--alg", "compare(sqrt(28^2+112^2),28*sqrt(17))"],
        ["E1-E2 = 0", "equivalent"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2023 Q6 average speed",
        ["--alg", "2*(20+130+28*sqrt(17)+26)/5*60/1000,method=numeric"],
        ["6.99472698042"],
        ["ERR:"],
    ),
    (
        "June 2023 Q7 implicit derivative",
        ["--derive", "x^3+2*x*y+3*y^2=47,x,method=implicit"],
        ["F_x + F_y*dy/dx = 0", "dy/dx = -(3*x^2 + 2*y)/(2*x + 6*y)"],
        ["limite", "ERR:"],
    ),
    (
        "June 2023 Q7 normal gradient",
        ["--alg", "-(2*5+3*(-2)^2)/(2*(-2)+6*5)"],
        ["-11/13"],
        ["ERR:"],
    ),
    (
        "June 2023 Q7 normal line",
        ["--alg", "solve(y-5=13/11*(x+2),y,method=linear)"],
        ["y = 13/11*(x + 2) + 5"],
        ["ERR:"],
    ),
    (
        "June 2023 R-form expression",
        ["--trig", "2*cos(theta)+8*sin(theta),method=rform"],
        ["R=sqrt(2^2+8^2)=sqrt(68)", "cos(alpha)=2/sqrt(68)", "Answer: sqrt(68)*cos(theta-atan(4))"],
        ["Answer: = 2*cos", "ERR:"],
    ),
    (
        "June 2023 Q8 arithmetic-series maximum",
        ["--alg", "9/2*(2*sqrt(17))"],
        ["9*sqrt(17)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q8 smallest positive angle",
        ["--alg", "atan(4),method=numeric"],
        ["1.32581766367"],
        ["ERR:"],
    ),
    (
        "June 2023 Q9 parametric derivative",
        ["--derive", "x=t^2+6*t-16,y=6*ln(t+3),t,x,method=param"],
        ["dx/dt = 2*t + 6", "dy/dt = 6/(t + 3)", "dy/dx = (dy/dt)/(dx/dt)"],
        ["limite", "ERR:"],
    ),
    (
        "June 2023 Q9 cartesian completion",
        ["--alg", "complete_square(t^2+6*t-16)"],
        ["(t + 3)^2 - 25"],
        ["ERR:"],
    ),
    (
        "June 2023 Q9 y-axis intercept",
        ["--alg", "3*ln(25)"],
        ["ln(25) = 2*ln(5)", "6*ln(5)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q9 tangent gradient",
        ["--alg", "evalat(3/(x+25),x,0)"],
        ["f(0) = 3/25"],
        ["ERR:"],
    ),
    (
        "June 2023 symbolic partial fractions",
        ["--alg", "partfrac((3*k*x-18)/((x+4)*(x-2)))"],
        ["= 2*k + 3", "= k - 3", "(2*k + 3)/(x + 4) + (k - 3)/(x - 2)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q10 partial-fraction integral parameter",
        ["--alg", "solve((2*k+3)*ln(5)-(k-3)*ln(5)=21,k,method=linear)"],
        ["ln(5) != 0", "k = (- 6*ln(5) + 21)/ln(5)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q11 separable tank model",
        ["--int", "de_solve(dh/dt=L/sqrt(h),h(0)=36/25)"],
        ["sqrt(h) dh = L dt", "2/3*h^(3/2) = L*t + C", "C = 144/125"],
        ["ERR:"],
    ),
    (
        "June 2023 Q11 tank coefficient",
        ["--alg", "solve((81/25)^(3/2)=8*K+(36/25)^(3/2),K,method=linear)"],
        ["K = 0.513", "K = [0.513]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q11 tank fill time",
        ["--alg", "solve(5^(3/2)=513/1000*t+216/125,t,method=linear)"],
        ["t = 18.4256138158", "t ~= 18.426"],
        ["ERR:"],
    ),
    (
        "June 2023 Q12 initial subscribers",
        ["--alg", "(abs(0-3)+4)-(8-abs(2*0-6)),method=numeric"],
        ["5"],
        ["ERR:"],
    ),
    (
        "June 2023 Q12 first crossing",
        ["--alg", "solve(-t+7=2*t+2,t,method=linear)"],
        ["t = 5/3", "t = [5/3]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q12 second crossing",
        ["--alg", "solve(t+1=14-2*t,t,method=linear)"],
        ["t = 13/3", "t = [13/3]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q13 binomial expansion",
        ["--alg", "binomial((3+x)^(-2),x,0,2)"],
        ["T0 = 1/9", "T1 = -2/27*x", "T2 = 1/27*x^2", "Valid for abs(x) < 3"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "June 2023 Q13 binomial integral estimate",
        ["--int", "defint(6*x*(1/9-2*x/27+x^2/27),x,0.2,0.4)"],
        ["F(0.4) - F(0.2)", "223/6750"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "June 2023 Q13 exact substitution integral",
        ["--int", "defint(6*x/(3+x)^2,x,0.2,0.4),method=sub"],
        ["u = x + 3", "F(0.4) - F(0.2)", "-45/136 + 6*ln(17/16)"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "June 2023 tan-domain trig solve",
        ["--trig", "2*tan(x)*(8*cos(x)+23*sin(x)^2)=8*sin(2*x)*(1+tan(x)^2),x,360,540,10,method=identity"],
        ["cos(x) != 0", "sin(2x)*(23cos(x)^2 - 8cos(x) - 15)=0", "u=-15/23", "x = [360, 490.705706831, 540]"],
        ["450", "ERR:"],
    ),
    (
        "June 2023 Q15 contradiction expansion",
        ["--alg", "expand((sin(x)-cos(x))^2)"],
        ["sin(x)^2 - 2*sin(x)*cos(x) + cos(x)^2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q15 Pythagorean reduction",
        ["--trig", "(sin(x)-cos(x))^2,target=1-2*sin(x)*cos(x),method=identity"],
        ["Source = target", "1 - 2*sin(x)*cos(x)"],
        ["not equivalent", "ERR:"],
    ),
    (
        "June 2024 cosec reciprocal identity",
        ["--trig", "1/(cosec(theta)-1)-1/(cosec(theta)+1),method=identity"],
        ["cosec(theta)^2-1", "2*tan(theta)^2"],
        ["Answer: = 1/(cosec", "ERR:"],
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
    print(f"edexcel_paper2_downloads ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

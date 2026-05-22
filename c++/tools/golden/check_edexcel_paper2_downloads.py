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
        "October 2020 Q6 algebraic division integral",
        ["--int", "defint((x^2+8*x-3)/(x+2),x,0,6),method=div"],
        ["Q = x + 6, R = -15", "54", "ln(8)"],
        ["No elementary primitive", "ERR:"],
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
        "October 2021 implicit polynomial powers",
        ["--derive", "p*x^3+q*x*y+3*y^2=26,x,method=implicit"],
        ["F_x + F_y*dy/dx = 0", "dy/dx"],
        ["positive bases", "ERR:"],
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
        "June 2023 log equation",
        ["--alg", "solve(log(2,x+3)+log(2,x+10)=2+2*log(2,x),x)"],
        ["Domain", "x = -5/3 rejected by domain", "x = [6]"],
        ["Parser error", "ERR:"],
    ),
    (
        "June 2023 R-form expression",
        ["--trig", "2*cos(theta)+8*sin(theta),method=rform"],
        ["R=sqrt(2^2+8^2)=sqrt(68)", "cos(alpha)=2/sqrt(68)", "Answer: sqrt(68)*cos(theta-atan(4))"],
        ["Answer: = 2*cos", "ERR:"],
    ),
    (
        "June 2023 symbolic partial fractions",
        ["--alg", "partfrac((3*k*x-18)/((x+4)*(x-2)))"],
        ["= 2*k + 3", "= k - 3", "(2*k + 3)/(x + 4) + (k - 3)/(x - 2)"],
        ["ERR:"],
    ),
    (
        "June 2023 tan-domain trig solve",
        ["--trig", "2*tan(x)*(8*cos(x)+23*sin(x)^2)=8*sin(2*x)*(1+tan(x)^2),x,360,540,10,method=identity"],
        ["cos(x) != 0", "sin(2x)*(23cos(x)^2 - 8cos(x) - 15)=0", "u=-15/23", "x = [360, 490.705706831, 540]"],
        ["450", "ERR:"],
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

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
        ["p^7 = 25/16", "p = [1.06583155827]", "p ~= 1.066"],
        ["ERR:", "p = []"],
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
        "June 2019 Q6 double-angle tan solve",
        ["--trig", "5*sin(2*theta)=9*tan(theta),theta,-180,180,10,method=identity"],
        ["sin(theta)=0", "cos(theta)^2=9/10", "theta = [", "0, 18.4", "161.6"],
        ["theta = []", "Right side still contains", "ERR:"],
    ),
    (
        "June 2019 Q9 symbolic log quotient",
        ["--alg", "solve(log(a)-log(b)=log(a-b),a)"],
        ["ln(a) - ln(b) = ln(a - b)", "Domain: a - b > 0", "a - b*(a - b) = 0", "a = -b^2/(- b + 1)"],
        ["symbolic parameters unsupported", "No real solution", "ERR:"],
    ),
    (
        "October 2020 Q14 cosec identity solve",
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
        ["u = a^k or u = e^k", "k = 5/2", "k = [5/2]"],
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
        "October 2020 Q6 R sin form",
        ["--trig", "sin(x)+2*cos(x),method=rform"],
        ["R=sqrt(1^2+2^2)=sqrt(5)", "cos(alpha)=1/sqrt(5)", "sin(alpha)=2/sqrt(5)", "sqrt(5)*sin(x+atan(2))"],
        ["Answer: = sin(x)", "ERR:"],
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

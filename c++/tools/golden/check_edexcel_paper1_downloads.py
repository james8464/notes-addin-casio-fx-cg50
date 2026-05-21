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
        "June 2018 Q5 trig quotient derivative",
        ["--derive", "3*sin(theta)/(2*sin(theta)+2*cos(theta)),theta"],
        ["quotient rule", "1 + sin(2*theta)", "3/(2*(1 + sin(2*theta)))"],
        ["Answer: d/dtheta", "ERR:"],
    ),
    (
        "June 2018 Q11 rational binomial",
        ["--alg", "binomial(sqrt((1+4*x)/(1-x)),x,0,2)"],
        ["sqrt(4*x + 1)", "(- x + 1)^(-1/2)", "5/2*x", "- 5/8*x^2", "Valid for abs(x) < 1/4"],
        ["unsupported binomial", "ERR:"],
    ),
    (
        "June 2018 Q13 linear-root definite integral",
        ["--int", "defint(2*x*sqrt(x+2),x,0,2)"],
        ["u=x+2", "x=u-2", "Limits", "32/15*sqrt(2) + 64/15"],
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

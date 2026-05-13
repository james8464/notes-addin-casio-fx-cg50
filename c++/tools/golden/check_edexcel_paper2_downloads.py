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
        ["u=cos(t)", "du=-sin(t) dt", "F(t) = -20*cos(t)^3", "20"],
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
        "June 2023 log equation",
        ["--alg", "solve(log(2,x+3)+log(2,x+10)=2+2*log(2,x),x)"],
        ["Domain", "x = -5/3 rejected by domain", "x = [6]"],
        ["Parser error", "ERR:"],
    ),
    (
        "June 2023 R-form expression",
        ["--trig", "2*cos(theta)+8*sin(theta),method=rform"],
        ["R=sqrt(8^2+2^2)=sqrt(68)", "sqrt(68)*sin(theta+atan(1/4))"],
        ["Answer: = 2*cos", "ERR:"],
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

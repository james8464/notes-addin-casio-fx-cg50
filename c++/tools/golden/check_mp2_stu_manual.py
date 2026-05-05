#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++/addin/host/build/casio_host"


CASES: list[tuple[str, list[str], list[str]]] = [
    (
        "S Q3 higher derivative identity",
        ["--derive", "mode:4,ln(1+cos(x))"],
        ["dy/dx = -sin(x)/(1+cos(x))", "d2y/dx2 = -e^(-y)", "d4y/dx4 + e^(-y)*(dy/dx)^2 + e^(-2y) = 0"],
    ),
    (
        "S Q14 substitution integral",
        ["--int", "(1+2*sin(x)*tan(x))/(2*(1+cos(x)*tan(x))*cos(x)*tan(x))"],
        ["Let u=sec(x)+sqrt(tan(x))", "The integral becomes Integral(1/u) du", "log(abs(sec(x) + sqrt(tan(x)))) + C"],
    ),
    (
        "S Q9 DE RHS integral",
        ["--int", "2*x*cosec(x+pi/6)^2"],
        ["Let u=2x and dv=cosec(x+pi/6)^2 dx", "v=-cot(x+pi/6)", "-2*x*cot(x + pi/6) + 2*log(abs(sin(x + pi/6))) + C"],
    ),
    (
        "T Q8 substitution then parts",
        ["--int", "((ln(x^2+1)-2*ln(x))*sqrt(x^2+1))/x^4"],
        ["Let u=sqrt(1+1/x^2)", "The integral becomes -2*Integral(u^2 log(u)) du", "2*(1 + 1/x^2)^(3/2)*(1 - 3*log(sqrt(1 + 1/x^2)))/9 + C"],
    ),
    (
        "T Q15 area",
        ["--int", "defint(sqrt(50-x^2),x,-5*sqrt(2),5)+defint(sqrt(50-x^2),x,-5*sqrt(2),-5)"],
        ["Solve the curve for y: y=-x +/- sqrt(50-x^2)", "x=sqrt(50)sin(theta)", "25*pi"],
    ),
    (
        "U Q14 tan substitution definite integral",
        ["--int", "defint(1/((sin(x)+2*cos(x))*(sin(x)+3*cos(x))),x,asin(3/5),acos(3/5))"],
        ["Partial fractions: 1/((u+2)(u+3)) = 1/(u+2)-1/(u+3)", "u=3/4", "log(150/143)"],
    ),
    (
        "U Q16 log differentiation",
        ["--derive", "2^(3*e^(2*x)),x"],
        ["Take logs: log(y) = 3*e^(2x)*log(2)", "6*e^(2x)*log(2)=2*log(y)", "dy/dx = 2*y*log(y)"],
    ),
]


def main() -> int:
    bad = 0
    for name, args, needles in CASES:
        p = subprocess.run([str(HOST), *args], cwd=str(REPO), text=True, capture_output=True, timeout=12)
        out = p.stdout + p.stderr
        missing = [n for n in needles if n not in out]
        if p.returncode or missing:
            bad += 1
            print(f"FAIL {name}: rc={p.returncode} missing={missing}", file=sys.stderr)
            print(out, file=sys.stderr)
        else:
            print(f"OK {name}")
    if bad:
        print(f"done bad {bad}/{len(CASES)}", file=sys.stderr)
        return 1
    print(f"done bad 0/{len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

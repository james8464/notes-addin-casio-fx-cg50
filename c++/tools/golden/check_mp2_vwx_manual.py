#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++/addin/host/build/casio_host"


CASES: list[tuple[str, list[str], list[str]]] = [
    (
        "V Q3 inverse trig",
        ["--trig", "2*atan(3/x)=asin(6*x/25)"],
        ["tan(A)=3/x", "6*x/(x^2+9)=6*x/25", "x = -4 or x = 4"],
    ),
    (
        "V Q7 derivative",
        ["--derive", "ln(tan(x+pi/4)),x"],
        ["sec(u)^2/tan(u)", "sin(2u)=cos(2x)", "dy/dx = 2*sec(2*x)"],
    ),
    (
        "V Q9 trig identity",
        ["--trig", "sec(x)^2-(1+sqrt(3))*tan(x)+sqrt(3)=1"],
        ["Use sec(x)^2 = 1+tan(x)^2", "(t-1)(t-sqrt(3))=0", "tan(x) = 1 or tan(x) = sqrt(3)"],
    ),
    (
        "V Q14 substitution integral",
        ["--int", "x/(5-4*x-x^2)^(3/2)"],
        ["dx/du=12u/(u^2+1)^2", "Integral((u^2-5)/(18u^2))", "sqrt(5 - 4*x - x^2)/(1 - x)"],
    ),
    (
        "W Q1 first principles",
        ["--derive", "sec(x),x,method=first_principles"],
        ["f(x+h)-f(x)", "cos(A)-cos(B)", "sec(x)*tan(x)"],
    ),
    (
        "W Q3 vertical tangents",
        ["--derive", "2*x*sin(y)+2*cos(2*y)=1,x,method=implicit"],
        ["Product rule", "dy/dx = sin(y)/(2sin(2y)-xcos(y))", "x = -3/2 or x = 3/2"],
    ),
    (
        "W Q15 substitution integral",
        ["--int", "3/((4*x+5)*sqrt(1-x^2)-3*(1-x^2))"],
        ["Integral(6/(1-3u)^2)", "2/(1-3u)+C", "sqrt(1 - x) - 3*sqrt(1 + x)"],
    ),
    (
        "X Q2 area",
        ["--int", "defint(sin(x),x,0,pi/6)+defint(cos(2*x),x,pi/6,pi/4)"],
        ["First intersection", "Area = Integral_0^(pi/6)", "3/2 - 3*sqrt(3)/4"],
    ),
    (
        "X Q5 inverse derivative",
        ["--derive", "asin(x),x,method=first_principles"],
        ["x=sin(y)", "cos(y)=sqrt(1-sin(y)^2)", "1/sqrt(1 - x^2)"],
    ),
    (
        "X Q5 stationary equation",
        ["--derive", "2*asin(x)-4*x^(3/2),x"],
        ["For stationary points", "Square both sides", "9x^3-9x+1=0"],
    ),
    (
        "X Q7 second derivative",
        ["--derive", "mode:4,ln(1+sin(x))"],
        ["d2y/dx2 = [-sin(x)(1+sin(x))-cos(x)^2]/(1+sin(x))^2", "e^y=1+sin(x)", "d2y/dx2 = -e^(-y)"],
    ),
    (
        "X Q8 trig solve",
        ["--trig", "sin(x)*cos(x)*cos(2*x)*cos(4*x)=sqrt(2)/16,x,0,pi/2"],
        ["sin(8x)", "0<=8x<=4pi", "pi/32"],
    ),
    (
        "X Q12 definite integral",
        ["--int", "defint(32*sin(x)*sin(2*x)*sin(3*x),x,0,pi/3)"],
        ["sin(x)sin(3x)", "F(pi/3)=13/3", "9"],
    ),
]


def main() -> int:
    bad = 0
    for name, args, needles in CASES:
        p = subprocess.run([str(HOST), *args], cwd=str(REPO), text=True, capture_output=True, timeout=12)
        out = p.stdout + p.stderr
        missing = [n for n in needles if not markers_present(out, [n])]
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

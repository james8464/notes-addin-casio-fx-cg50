#!/usr/bin/env python3
"""Manual MP2 Q/R paper checks from rendered model solutions.

Run: python3 c++/tools/golden/check_mp2_qr_manual.py
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES = [
    ("Q5 integral parameter", ["--int", "defint((k*cos(x)^2-sec(x)^2)*sin(x),x,0,pi/3)"], ["7/24*k - 1", "u=cos(x)", "F(pi/3)-F(0)"]),
    ("Q5 solve k", ["--alg", "(7*k-24)/24=3/4"], ["k = [6]"]),
    ("Q6 binomial sqrt", ["--alg", "binomial(sqrt(1-x),x,0,3)"], ["1/2*x", "1/8*x^2", "1/16*x^3", "Valid for"]),
    ("Q9 range f", ["--alg", "range(x^2-4,x,8,inf)"], ["y >= 60"]),
    ("Q9 range g", ["--alg", "range(2*x-2,x,3,inf)"], ["y >= 4"]),
    ("Q11 derivative", ["--derive", "exp(x)/sin(x),x"], ["quotient rule", "y*(1 - cot(x))"]),
    ("Q11 second derivative", ["--derive", "mode:4,exp(x)/sin(x)"], ["product rule", "dy/dx*(1 - cot(x)) + y*cosec(x)^2"]),
    ("Q12 param tangent support", ["--derive", "x=3*cos(t),y=4*sin(t),t,x,method=param"], ["dx/dt", "dy/dx"]),
    ("R2 symbolic complete square", ["--alg", "complete_square(x^2+2*k*x+4)"], ["(x + k)^2", "- k^2 + 4"]),
    ("R2 symbolic range", ["--alg", "range(x^2+2*k*x+4)"], ["y >= - k^2 + 4"]),
    ("R5 exp reciprocal range", ["--alg", "range(e^(n*x)+k*e^(-n*x))"], ["AM-GM", "y >= 2*sqrt(k)"]),
    ("R7 exact integral", ["--int", "defint((1+sin(x))^2/cos(x)^2,x,pi/6,pi/3)"], ["sec(x)^2 + 2sec(x)tan(x) + tan(x)^2", "4 - pi/6"]),
    ("R9 partial fractions", ["--alg", "partfrac((16*x^2+3*x-2)/(x^2*(3*x-2)))"], ["A=1", "B=0", "C=16", "1/x^2 + 16/(3*x-2)"]),
    ("R9 binomial", ["--alg", "binomial(1/(3*x-2),x,0,3)"], ["-1/2", "- 3/4*x", "- 9/8*x^2", "- 27/16*x^3"]),
    ("R10 sin triple", ["--trig", "sin(3*theta)\n3*sin(theta)-4*sin(theta)^3"], ["sin(A+B)", "3*sin(theta) - 4*sin(theta)^3"]),
    ("R10 cos triple", ["--trig", "cos(3*theta)\n4*cos(theta)^3-3*cos(theta)"], ["cos(A+B)", "4*cos(theta)^3 - 3*cos(theta)"]),
    ("R10 tan triple", ["--trig", "tan(3*theta)\n(3*tan(theta)-tan(theta)^3)/(1-3*tan(theta)^2)"], ["tan(2theta)", "1 - 3*tan(theta)^2"]),
    ("R11 param", ["--derive", "cos(theta)^3,sin(theta)^3,theta,method=param"], ["dy/dx = -tan(theta)"]),
    ("R11 param second", ["--derive", "cos(theta)^3,sin(theta)^3,theta,method=param_second"], ["d2y/dx2 = 1/(3*cos(theta)^4*sin(theta))"]),
]


def run_case(name: str, args: list[str], needles: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [needle for needle in needles if not markers_present(out, [needle])]
    if proc.returncode != 0:
        misses.append(f"returncode={proc.returncode}")
    if misses:
        return [f"{name}: missing {misses}\n{out}"]
    return []


def main() -> int:
    failures: list[str] = []
    for name, args, needles in CASES:
        failures.extend(run_case(name, args, needles))
    if failures:
        print("\n\n".join(failures))
        return 1
    print(f"mp2_qr_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Manual MP2 N/O/P paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str]]] = [
    ("N5 sqrt substitution", ["--int", "1/(4*sqrt(x)*sqrt(sqrt(x)-1))"], ["u=sqrt(x)", "sqrt(sqrt(x) - 1) + C"]),
    ("N6 range reciprocal", ["--alg", "range(3/(x+2),x,-1,inf)"], ["0 < y <= 3"]),
    ("N6 inverse reciprocal", ["--alg", "make_subject(y=3/(x+2),x)"], ["x = 3/y - 2"]),
    ("N8 derivative parameter", ["--derive", "x^2/(x-a)^2,x"], ["quotient rule", "dy/dx = -2*a*x/(x-a)^3"]),
    ("N8 solve parameter", ["--alg", "subst(-2*a*x/(x-a)^3,x,2*a)=-2"], ["a = [2]"]),
    ("N9 separable DE support", ["--alg", "de_solve(x*(x+2)*dy/dx=y,y(2)=2)"], ["Separate variables", "1/(x*(x+2))", "y^2 = 8*x/(x+2)"]),
    ("N10 tan multiple solve", ["--trig", "tan(4*x)-tan(2*x)=0,x,0,360,8,method=bounded"], ["tan(A)=tan(B)", "x = [0, 90, 180, 270, 360]"]),
    ("N12 implicit stationary", ["--derive", "x*y*(x-y)+16=0,x,method=implicit"], ["dy/dx = -y*(2*x-y)/(x*(x-2*y))", "stationary", "(2,4)"]),
    ("N13 param tangent", ["--derive", "2*t/(1+t^2),(1-t^2)/(1+t^2),t,method=param"], ["dy/dx = -1", "x + y = sqrt(2)"]),
    ("O4 exact integral", ["--int", "defint(3*x/(2+x-x^2),x,0,1)"], ["A_i = N(r_i)/D'(r)", "- ln(abs(x + 1)) - 2*ln(abs(x - 2))", "log(2)"]),
    ("O8 cos triple proof", ["--trig", "cos(3*x)\n4*cos(x)^3-3*cos(x)"], ["cos(A+B)", "4*cos(x)^3 - 3*cos(x)"]),
    ("O8 solve sec cos", ["--trig", "2+cos(6*x)*sec(2*x)=0,x,0,360,8,method=bounded"], ["cos(6x)=4cos(2x)^3-3cos(2x)", "x = 30, 60, 120, 150, 210, 240, 300, 330"]),
    ("O10 implicit tangent", ["--derive", "y*(x-y)=log(y),x,method=implicit"], ["dy/dx", "At y=e", "e*(x-y)=1"]),
    ("O11 param cartesian", ["--alg", "param_cartesian(cos(t),sin(t)-tan(t),t)"], ["x=cos(t)", "y^2 = (x - 1)^2*(1 - x^2)/x^2"]),
    ("P3 abs inequality", ["--alg", "abs(2*x+1)+9<4*x"], ["x > 5/2"]),
    ("P4 separable DE", ["--alg", "de_solve(110*dH/dt=12-H,H(0)=1)"], ["Separate variables", "H = 12 - 11*e^(-t/110)"]),
    ("P7 fourth-root integral", ["--int", "defint(e^(x^(1/4))/sqrt(x),x,0,1)"], ["u=x^(1/4)", "Integral(4u*e^u)", "4"]),
    ("P8 binomial shifted", ["--alg", "binomial((1/4-x)^(-3/2),x,0,3)"], ["8", "48*x", "240*x^2", "1120*x^3", "Valid for abs(x) < 1/4"]),
    ("P8 binomial square-root from reciprocal", ["--alg", "binomial(sqrt(1/4-x),x,0,3,method=from_reciprocal)"], ["Use previous expansion", "1/2 - x - x^2 - 2*x^3"]),
    ("P10 trig implicit derivative", ["--derive", "tan(3*y)=3*tan(x),x,method=implicit"], ["sec(3y)^2", "dy/dx = 1/(1+8*sin(x)^2)"]),
    ("P12 composite domain range", ["--alg", "range((x-6)^2,x,7,10)"], ["1 <= y <= 16"]),
    ("P13 param tangent", ["--derive", "cos(t),sin(2*t)-cos(t),t,method=param"], ["theta=pi/4", "dy/dx = -1", "x + y = 1"]),
    ("P13 param cartesian", ["--alg", "param_cartesian(cos(t),sin(2*t)-cos(t),t)"], ["4*x^2*(1-x^2) = (x+y)^2"]),
]


def run_case(name: str, args: list[str], needles: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [needle for needle in needles if not markers_present(out, [needle])]
    if proc.returncode:
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
    print(f"mp2_nop_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

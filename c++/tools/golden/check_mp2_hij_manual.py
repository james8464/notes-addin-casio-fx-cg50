#!/usr/bin/env python3
"""Manual MP2 H/I/J paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str]]] = [
    (
        "H6 cube-root definite integral",
        ["--int", "defint(2/(x+x^(1/3)),x,0,sqrt(27))"],
        ["u=x^(1/3)", "dx=3*u^2", "6*log(2)"],
    ),
    (
        "H7 implicit exponential derivative",
        ["--derive", "2*x*y=2^x+y^2,x,method=implicit"],
        ["Differentiate", "dy/dx", "(y - 2^(x-1)*log(2))/(y-x)"],
    ),
    (
        "H9 disguised trig derivative",
        ["--derive", "cos(2*x)*tan(2*x),x"],
        ["cos(u)*tan(u)=sin(u)", "dy/dx = 2*cos(2*x)"],
    ),
    (
        "H9 quotient simplification derivative",
        ["--derive", "x^2/(3*x-1)^2,x"],
        ["quotient rule", "dy/dx = -2*x/(3*x-1)^3"],
    ),
    (
        "I3 parts definite integral",
        ["--int", "defint(2*x*(1+cos(x)),x,0,pi)"],
        ["u = 2*x, dv = cos(x)dx", "2*x*sin(x)", "pi^2 - 4"],
    ),
    (
        "I8 binomial expansion",
        ["--alg", "binomial(sqrt(1-x),x,0,3)"],
        ["1 - 1/2*x - 1/8*x^2 - 1/16*x^3", "Valid for abs(x) < 1"],
    ),
    (
        "I11 hidden substitution definite integral",
        ["--int", "defint((1-tan(x)^2)/(sec(x)^2+2*tan(x)),x,0,pi/4)"],
        ["u=1+sin(2*x)", "du=2*cos(2*x)", "1/2*log(2)"],
    ),
    (
        "J1 shifted sec solve",
        ["--trig", "4*sec(2*(x+3*pi/2))-3=5,x,0,2*pi,8"],
        ["u = cos(2*(x + 3*pi/2))", "u = 1/2", "x = [pi/3, 2*pi/3, 4*pi/3, 5*pi/3]"],
    ),
    (
        "J4 reciprocal interval range",
        ["--alg", "range(1/(x-2),x,2,inf)"],
        ["Interval of interest", "y > 0"],
    ),
    (
        "J9 tan substitution definite integral",
        ["--int", "defint((1+cot(x)^2)*sec(x)^2,x,pi/6,pi/3)"],
        ["u=tan(x)", "tan(x)-cot(x)", "4*sqrt(3)/3"],
    ),
    (
        "J10 implicit trig simplification",
        ["--derive", "sin(2*x)*cot(y)=1,x,method=implicit"],
        ["dy/dx", "cot(2*x)*sin(2*y)"],
    ),
    (
        "J12 binomial validity",
        ["--alg", "binomial((1+12*x)^(1/3),x,0,2)"],
        ["1 + 4*x - 16*x^2", "Valid for abs(x) < 1/12"],
    ),
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
    print(f"mp2_hij_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

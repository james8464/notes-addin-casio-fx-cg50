#!/usr/bin/env python3
"""Manual MP2 A/B paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "A10 trig-rational range",
        ["--alg", "range(cos(x)/(3-sin(x)))"],
        ["RHS range gives", "y^2 <= 1/8", "-sqrt(2)/4 <= y <= sqrt(2)/4"],
        ["inspect graph/transform"],
    ),
    (
        "A13 cosec-cot quartic solve",
        ["--trig", "cosec(theta)^4-cot(theta)^4=2+3*cot(theta),theta,0,2*pi"],
        ["cosec(theta)^2 = 1 + cot(theta)^2", "2*u^2 - 3*u - 1 = 0", "cot(theta)=(3+sqrt(17))/4"],
        ["theta = []", "Right side still contains"],
    ),
    (
        "B1 sin first principles",
        ["--derive", "sin(x),x,method=first_principles"],
        ["[sin(x+h)-sin(x)]/h", "sin(x+h)-sin(x)=sin(x)cos(h)+cos(x)sin(h)-sin(x)", "cos(x)"],
        ["No first-principles route", "d/dx(sin(x))"],
    ),
    (
        "B8 trig factor solve",
        ["--trig", "cos(theta)*cos(2*theta)/(cos(theta)+sin(theta))=1/2,theta,0,2*pi"],
        ["cos(2*theta)=(cos(theta)+sin(theta))*(cos(theta)-sin(theta))", "tan(2*theta)=1", "theta = [pi/8, 5*pi/8, 9*pi/8, 13*pi/8]"],
        ["theta = []", "Rearrange using identities"],
    ),
    (
        "B10 separable DE exponential",
        ["--alg", "de_solve(e^x*dy/dx+y^2=x*y^2,y(1)=e)"],
        ["Separate variables: y^(-2)dy = (x-1)*e^(-x) dx", "RHS by parts", "y = e^x/x"],
        ["Only one '=' supported", "solve(de_solve"],
    ),
    (
        "B13 linear radical definite",
        ["--int", "defint(2*a*x/sqrt(a*x-1),x,2/a,17/a)"],
        ["u^2 = a*x - 1", "x=(u^2+1)/a", "Answer: 96/a"],
        ["No elementary primitive", "ERR:"],
    ),
]


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [needle for needle in needles if not markers_present(out, [needle])]
    bad = [needle for needle in banned if needle in out]
    if proc.returncode:
        misses.append(f"returncode={proc.returncode}")
    if misses or bad:
        return [f"{name}: missing {misses}, banned {bad}\n{out}"]
    return []


def main() -> int:
    failures: list[str] = []
    for name, args, needles, banned in CASES:
        failures.extend(run_case(name, args, needles, banned))
    if failures:
        print("\n\n".join(failures))
        return 1
    print(f"mp2_ab_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

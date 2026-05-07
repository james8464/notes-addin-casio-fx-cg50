#!/usr/bin/env python3
"""Manual MP2 K/L/M paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str]]] = [
    ("K5 cos first principles", ["--derive", "cos(x),x,method=first_principles"], ["cos(x+h)-cos(x)", "-2sin(x+h/2)sin(h/2)", "-sin(x)"]),
    ("K6 binomial validity", ["--alg", "binomial((1/2-x)^(-3),x,0,3)"], ["8 + 48*x + 192*x^2 + 640*x^3", "Valid for abs(x) < 1/2"]),
    ("K7 inverse trig solve", ["--trig", "3*asin(x-1)=2*acos(x-1),x"], ["Let u=asin(x-1)", "u=pi/5", "x = 1 + sin(pi/5)"]),
    ("K8 definite partial fractions", ["--int", "defint((2*x^3-5*x^2+5)/((x-2)*(x-1)^3),x,0,1/2)"], ["partial fractions", "A=1", "B=-2", "C=2", "D=1", "5 + log(3/8)"]),
    ("L3 R-form with alpha", ["--trig", "6+13*sin(2*x+atan(5/12))=5*cos(2*x),x,0,360"], ["sin(alpha)=5/13", "12*sin(2*x)+6=0", "x = 105, 165, 285, 345"]),
    ("L9 composite range", ["--alg", "range(2*x^2+1,x,0,3)"], ["1 <= y <= 19"]),
    ("L10 rational definite", ["--int", "defint((4*x+3)/(3*x+4),x,0,32)"], ["u=3*x+4", "128/3 - 7/9*log(25)"]),
    ("L11 implicit second derivative", ["--derive", "y^2-x^2=1,x,method=second"], ["dy/dx = x/y", "d2y/dx2 = 1/y^3"]),
    ("M5 xlnx derivative", ["--derive", "x*ln(x),x"], ["f1 = x", "f1' = 1", "f2 = log(x)", "f2' = 1/x", "y' = f1'*f2 + f1*f2'", "dy/dx = log(x) + 1"]),
    ("M12 half-angle integral", ["--int", "1/(1+cos(2*x))"], ["1+cos(2*x)=2*cos(x)^2", "1/2*tan(x) + C"]),
    ("M13 rational division", ["--alg", "partfrac((50*x^2-142*x+95)/(2*x-5))"], ["25*x - 17/2 + (105/2)/(2*x-5)"]),
]


def compact(s: str) -> str:
    return "".join(ch for ch in s if not ch.isspace())


def run_case(name: str, args: list[str], needles: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    out_compact = compact(out)
    misses = [needle for needle in needles if needle not in out and compact(needle) not in out_compact]
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
    print(f"mp2_klm_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

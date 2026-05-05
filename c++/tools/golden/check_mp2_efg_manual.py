#!/usr/bin/env python3
"""Manual MP2 E/F/G paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "E10 split trig fraction",
        ["--int", "6*sin(x)/(cos(x)+sin(x))"],
        ["Split numerator", "A=3", "B=-3", "3*x - 3*log(abs(cos(x) + sin(x)))"],
        ["No elementary primitive"],
    ),
    (
        "F3 compound-angle solve",
        ["--trig", "sin(x)*cos(pi/5)=1/2-cos(x)*sin(pi/5),x,2*pi,4*pi,6"],
        ["sin(x+pi/5)=1/2", "79*pi/30", "119*pi/30"],
        ["x = []"],
    ),
    (
        "F11 hidden exponential substitution",
        ["--int", "x*(2-3*x)/(exp(3*x)+x^2)"],
        ["Multiply top and bottom by e^(-3*x)", "u=x^2*e^(-3*x)+1", "log(x^2*e^(-3*x) + 1)"],
        ["No elementary primitive"],
    ),
    (
        "G5 second derivative route",
        ["--derive", "(x+1)^2*exp(2*x),x,method=second"],
        ["Differentiate once", "d2y/dx2", "2*(2*x^2 + 8*x + 7)*e^(2*x)"],
        ["d2y/dx2 = 0"],
    ),
    (
        "G10 double-angle cubic solve",
        ["--trig", "64*cos(2*x)*cos(x)+32*sin(2*x)*sin(x)=27,x,0,pi,4"],
        ["Use cos(2A)=2cos(A)^2-1", "cos(x)=3/4", "acos(3/4)"],
        ["x = []"],
    ),
    (
        "G12 rationalise negative-power denominator",
        ["--int", "(5-x)/(2-x^(-1))"],
        ["Multiply numerator and denominator by x", "-1/4*x^2", "9/8*log(abs(2*x - 1))"],
        ["No elementary primitive"],
    ),
]


def compact(s: str) -> str:
    return "".join(ch for ch in s if not ch.isspace())


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    out_compact = compact(out)
    misses = [needle for needle in needles if needle not in out and compact(needle) not in out_compact]
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
    print(f"mp2_efg_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

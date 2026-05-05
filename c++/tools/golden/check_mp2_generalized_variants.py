#!/usr/bin/env python3
"""Variant checks proving MP2 paper fixes are pattern-based, not one-offs."""

from __future__ import annotations

import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "first-principles scaled sine",
        ["--derive", "sin(3*x),x,method=first_principles"],
        ["sin(3*(x+h))-sin(3*x)", "sin(3*h)/(h)", "d/dx sin(3*x) = 3*cos(3*x)"],
        ["No first-principles route", "d/dx(sin(3*x))"],
    ),
    (
        "first-principles scaled cosine",
        ["--derive", "cos(2*x),x,method=first_principles"],
        ["cos(2*(x+h))-cos(2*x)", "sin(2*h/2)/(h/2)->2", "d/dx cos(2*x) = -2*sin(2*x)"],
        ["No first-principles route", "d/dx(cos(2*x))"],
    ),
    (
        "range trig-rational non-x variable",
        ["--alg", "range(cos(t)/(4-sin(t)))"],
        ["Variable = t", "4*y = cos(t) + y*sin(t)", "-1/sqrt(15) <= y <= 1/sqrt(15)"],
        ["Variable = x", "Range: y = cos(t)"],
    ),
    (
        "linear radical definite new symbolic limits",
        ["--int", "defint(2*a*x/sqrt(a*x-1),x,5/a,10/a)"],
        ["u^2 = a*x - 1", "Limits: x=5/a gives u=2; x=10/a gives u=3", "88/(3*a)"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "linear radical definite alternate parameter",
        ["--int", "defint(2*b*x/sqrt(b*x-1),x,2/b,17/b)"],
        ["u^2 = b*x - 1", "Limits: x=2/b gives u=1; x=17/b gives u=4", "96/b"],
        ["ERR:", "No elementary primitive"],
    ),
    (
        "first-principles negative cosine coefficient",
        ["--derive", "cos(-3*x),x,method=first_principles"],
        ["sin(-3*h/2)/(h/2)->-3", "d/dx cos(-3*x) = 3*sin(-3*x)"],
        ["--3"],
    ),
    (
        "range trig-rational scaled numerator",
        ["--alg", "range(2*cos(x)/(5-3*sin(x)))"],
        ["Let y = 2*cos(x)/(5 - 3*sin(x))", "So y^2 <= 1/4", "-1/2 <= y <= 1/2"],
        ["inspect graph/transform"],
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
    print(f"mp2_generalized_variants ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "khicas/upstream/giac90_1addin"

REQUIRED = [
    "diff(f,var",
    "integrate(f,x,[a,b])",
    "solve(equation,x)",
    "factor(p",
    "partfrac(p,x)",
    "simplify(expr)",
    "tcollect(expr)",
    "texpand(expr)",
    "range(expr,[x,a,b])",
    "xform(expr,target)",
    "implicit_diff(eq,[x,y])",
]

REMOVED_CATEGORIES = [
    "Probabilities",
    "Statistiques",
    "Statistics",
    "Matrices ",
    "Programs",
    "Turtle",
    "Curves",
    "Complexes",
]


def main() -> int:
    en = (SRC / "catalogen.cpp").read_text(errors="ignore")
    missing = [item for item in REQUIRED if item not in en]
    if missing:
        raise SystemExit("FAIL catalog: missing " + ", ".join(missing))
    leaks = [item for item in REMOVED_CATEGORIES if item in en]
    if leaks:
        raise SystemExit("FAIL catalog: removed categories visible " + ", ".join(leaks))
    print(f"OK catalog scope required={len(REQUIRED)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


MARKERS = [
    b"2. = ",
    b"Std form.",
    b"Valid:",
    b"Pick rule.",
    b"Collect y' terms",
    b"dy/dx=(dy/d",
    b"Area=Int",
    b"Pick SUVAT",
    b"Binom theorem",
    b"Coeff:",
    b"I = Int[",
    b"Prob/crit",
    b"1. ",
    b"Eq coeffs",
    b"Solve consts",
    b"ax2+bx+c ->",
    b"Int sides.",
    b"Classify.",
    b"IBP:",
    b"Use PF",
    b"Clear den; eq",
]

FORBIDDEN = [
    b"Ans: ",
]


def main() -> int:
    ap = argparse.ArgumentParser(description="Verify source-built CasioCAS .g3a includes working-line hooks.")
    ap.add_argument("g3a", type=Path)
    args = ap.parse_args()
    data = args.g3a.read_bytes()
    missing = [m.decode("ascii") for m in MARKERS if m not in data]
    if missing:
        print("FAIL g3a working markers missing: " + ", ".join(missing))
        return 1
    forbidden = [m.decode("ascii") for m in FORBIDDEN if m in data]
    if forbidden:
        print("FAIL g3a forbidden answer prefixes present: " + ", ".join(forbidden))
        return 1
    print("OK g3a working markers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

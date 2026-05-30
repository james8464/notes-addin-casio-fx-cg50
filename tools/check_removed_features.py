#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "khicas/upstream/giac90_1addin"

REMOVED = [
    "normalcdf",
    "binomcdf",
    "binompdf",
    "mean",
    "median",
    "stddev",
    "variance",
    "det",
    "plot",
    "plotlist",
    "plotparam",
    "plotpolar",
    "plotseq",
    "rand",
    "randint",
    "ranv",
    "ranm",
    "csolve",
    "cfactor",
    "cpartfrac",
    "sinh",
    "cosh",
    "tanh",
    "asinh",
    "acosh",
    "atanh",
    "python",
    "set_pixel",
    "draw_pixel",
    "matrix",
    "comb",
    "perm",
    "binomial",
    "normald",
    "uniformd",
    "poisson",
    "laplace",
    "fourier",
    "odesolve",
]


def fail(msg: str) -> None:
    raise SystemExit(f"FAIL removed-features: {msg}")


def word_hit(text: str, name: str) -> bool:
    if name in {"plot", "rand", "det"}:
        return re.search(rf'(?<![A-Za-z0-9_]){re.escape(name)}\s*\(', text) is not None
    return re.search(rf'(?<![A-Za-z0-9_]){re.escape(name)}(?![A-Za-z0-9_])', text) is not None


def fnv1a32(text: str) -> str:
    value = 2166136261
    for ch in text:
        value ^= ord(ch)
        value = (value * 16777619) & 0xFFFFFFFF
    return f"0x{value:08x}u"


def main() -> int:
    main_cc = (SRC / "main.cc").read_text(errors="ignore")
    if "cascas_reject_removed_feature" not in main_cc:
        fail("main.cc missing runtime reject hook")
    if "Err: unsupported (not A-level scope)" not in main_cc:
        fail("missing canonical unsupported message")

    for name in REMOVED:
        marker = f'"{name}"'
        hashed = fnv1a32(name)
        if marker not in main_cc and hashed not in main_cc:
            fail(f"main.cc denylist missing {name}")

    leaks: list[str] = []
    for rel in ["catalogen.cpp", "catalogfr.cpp"]:
        text = (SRC / rel).read_text(errors="ignore")
        for raw in re.findall(r'\{\s*"([^"]+)"', text):
            entry = raw.split("(", 1)[0].strip().lower()
            for name in REMOVED:
                if entry == name:
                    leaks.append(f"{rel}:{name}")
    help_dir = ROOT / "help/functions"
    for p in help_dir.glob("*.txt"):
        stem = p.stem.lower()
        if stem in REMOVED:
            leaks.append(f"help/functions/{p.name}")
        text = p.read_text(errors="ignore").lower()
        for name in REMOVED:
            if word_hit(text, name):
                leaks.append(f"help/functions/{p.name}:{name}")
                break
    if leaks:
        fail("public leaks " + ", ".join(leaks[:20]))

    print(f"OK removed features hidden/rejected count={len(REMOVED)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELP = ROOT / "help/functions"

REQUIRED = [
    "diff",
    "integrate",
    "solve",
    "factor",
    "partfrac",
    "simplify",
    "range",
    "domain",
    "xform",
    "implicit_diff",
    "log",
    "sin",
    "cos",
    "tan",
    "sec",
    "cosec",
    "cot",
]

FIELDS = ["Args:", "Required:", "Does:", "Examples:"]


def main() -> int:
    missing: list[str] = []
    weak: list[str] = []
    for name in REQUIRED:
        p = HELP / f"{name}.txt"
        if not p.exists():
            missing.append(name)
            continue
        text = p.read_text(errors="ignore")
        absent = [field for field in FIELDS if field not in text]
        if absent:
            weak.append(f"{name}({','.join(absent)})")
    if missing:
        raise SystemExit("FAIL help: missing " + ", ".join(missing))
    if weak:
        raise SystemExit("FAIL help: weak " + ", ".join(weak))
    print(f"OK help quality functions={len(REQUIRED)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

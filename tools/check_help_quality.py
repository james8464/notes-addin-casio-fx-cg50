#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELP = ROOT / "help/functions"
CATALOG = ROOT / "khicas/upstream/giac90_1addin/catalogen.cpp"

EXTRA_REQUIRED = {"domain", "sin", "cos", "tan", "sec", "cosec", "cot"}

FIELDS = ["Args:", "Required:", "Does:", "Examples:"]


def main() -> int:
    catalog = CATALOG.read_text(errors="ignore")
    block = catalog.split("const catalogFunc completeCat[]", 1)[1].split("\n};", 1)[0]
    required = set(EXTRA_REQUIRED)
    for line in block.splitlines():
        line = line.strip()
        if not line.startswith('{"'):
            continue
        name = line.split('"', 2)[1].split("(", 1)[0].split("[", 1)[0]
        if name:
            required.add(name)
    missing: list[str] = []
    weak: list[str] = []
    for name in sorted(required):
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
    print(f"OK help quality functions={len(required)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

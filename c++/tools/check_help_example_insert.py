#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HELP = ROOT / "c++/prizm/help/CASIOCAS.HLP"
CAT = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def main() -> int:
    help_text = HELP.read_text(errors="ignore")
    cat = CAT.read_text(errors="ignore")
    if "Ex F2:" not in help_text:
        return fail("help file has no F2 examples")
    markers = [
        "catalog_insert_help_example",
        "Ex F2:",
        "catalog_insert_help_example(insertText",
    ]
    missing = [m for m in markers if m not in cat]
    if missing:
        return fail("catalog F-key example insertion not wired: " + ", ".join(missing))
    print("OK help example insertion wired")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

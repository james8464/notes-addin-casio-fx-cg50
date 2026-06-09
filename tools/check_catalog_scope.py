#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from scope_manifest import KEPT_COMMANDS, MENU_SIGNATURES, REMOVED_COMMANDS

SRC = ROOT / "khicas/upstream/giac90_1addin"


def main() -> int:
    text = (SRC / "catalogen.cpp").read_text(errors="ignore")
    try:
        block = text.split("const catalogFunc completeCat[]", 1)[1].split("\n};", 1)[0]
    except IndexError as exc:
        raise SystemExit("FAIL catalog: completeCat block missing") from exc

    missing = [sig for sig in MENU_SIGNATURES.values() if sig not in block]
    if missing:
        raise SystemExit("FAIL catalog missing: " + ", ".join(missing))

    leaked = [name for name in REMOVED_COMMANDS if f'"{name}' in block or f'"{name}(' in block]
    if leaked:
        raise SystemExit("FAIL catalog removed leak: " + ", ".join(leaked[:20]))

    if '{"expand(expr)"' in block:
        raise SystemExit("FAIL catalog: visible expand alias; use texpand")
    if '"collect"' in block or '"collect(' in block:
        raise SystemExit("FAIL catalog: visible collect alias; use tcollect")

    hidden_labels = [
        "Statistics", "Probabilities", "Matrices", "Programs", "Options",
        "Turtle", "Graphs", "Arithmetic, crypto", "Variable handling",
    ]
    menu_leaks = [label for label in hidden_labels if label in text]
    if menu_leaks:
        raise SystemExit("FAIL catalog menu leak: " + ", ".join(menu_leaks))

    kept_visible = sum(1 for name in KEPT_COMMANDS if f'"{name}' in block or f'"{name}(' in block)
    print(f"OK catalog scope kept_visible={kept_visible} removed={len(REMOVED_COMMANDS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

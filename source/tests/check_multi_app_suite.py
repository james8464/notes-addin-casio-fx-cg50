#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, needles: list[str]) -> None:
    text = (ROOT / path).read_text(errors="ignore")
    missing = [n for n in needles if n not in text]
    if missing:
        raise AssertionError(f"{path}: missing {missing}")


def main() -> int:
    require(
        "tools/build/build_g3a.sh",
        ["Usage: ./compile", "notes"],
    )
    require(
        "apps/notes/notes_app.cc",
        ["NOTES_ROOT", r"\\\\fls0\\NOTES\\", "Bfile_FindFirst_NON_SMEM", ".txt", "KEY_CTRL_LEFT", "KEY_CTRL_RIGHT", "search_all_rec"],
    )
    require(
        "docs/NOTES_README.md",
        ["does not open images", "search all text files", "wide-line horizontal scroll"],
    )

    notes_dir = ROOT.parent / "calculator" / "NOTES"
    if notes_dir.exists():
        note_files = list(notes_dir.rglob("*.txt"))
        if note_files:
            too_big = [p for p in note_files if p.stat().st_size >= 16384]
            if too_big:
                raise AssertionError(f"notes files exceed viewer buffer: {[str(p) for p in too_big[:5]]}")

    print("OK notes app source checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

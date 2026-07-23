#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NOTES = ROOT.parent / "calculator" / "NOTES"
APP = ROOT / "apps" / "notes" / "notes_app.cc"


def const_int(source: str, name: str) -> int:
    match = re.search(rf"static const int {name} = ([0-9]+);", source)
    if not match:
        raise AssertionError(f"missing integer constant: {name}")
    return int(match.group(1))


def require(source: str, needle: str, label: str) -> None:
    if needle not in source:
        raise AssertionError(f"notes viewer missing {label}: {needle!r}")


def main() -> int:
    source = APP.read_text(errors="ignore")
    require(source, r'NOTES_ROOT[] = "\\\\fls0\\NOTES\\"', "top-level NOTES root")
    require(source, "hidden_system_folder", "system folder filter")
    require(source, 'ends_with(name, ".txt")', "txt-only file listing")
    require(source, "KEY_CTRL_LEFT", "left scroll key")
    require(source, "KEY_CTRL_RIGHT", "right scroll key")
    require(source, "hscroll", "horizontal scroll state")
    require(source, "search_all_rec", "recursive all-file search")
    require(source, "jump_to_match", "open search result at match")
    require(source, "UI_MATCH_TEXT", "text-colour search match")
    require(source, "NOTE_H1", "heading style")
    require(source, "NOTE_H2", "subheading style")
    require(source, "NOTE_H3", "third-level heading style")
    require(source, "NOTE_BULLET", "bullet style")
    require(source, "NOTE_ORDERED", "ordered-list style")
    require(source, "NOTE_QUOTE", "quote style")
    require(source, "normalize_file_buf", "UTF-8/ascii normalisation")
    require(source, "status_title_needs_marquee", "long title marquee")
    require(source, "notes_softkeys", "softkey labels")

    if const_int(source, "FILE_BUF_SIZE") < 16384:
        raise AssertionError("notes file buffer is too small")
    if const_int(source, "MAX_VIEW_LINES") < 512:
        raise AssertionError("notes line buffer is too small")
    if const_int(source, "MAX_RESULTS") < 64:
        raise AssertionError("search result cap is too small")
    if const_int(source, "LINE_CAP") < 90:
        raise AssertionError("render line cap is too small for horizontal scrolling")

    files = sorted(NOTES.rglob("*.txt"))
    if files:
        oversized = [p for p in files if p.stat().st_size >= const_int(source, "FILE_BUF_SIZE")]
        if oversized:
            raise AssertionError(f"notes files exceed app buffer: {[str(p) for p in oversized[:5]]}")
    print("OK notes render layout")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

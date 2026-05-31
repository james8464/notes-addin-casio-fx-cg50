#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "khicas/upstream/giac90_1addin/main.cc"


def body(text: str, name: str) -> str:
    m = re.search(rf"\n(?:void|int) {re.escape(name)}\([^)]*\)\{{(.*?)\n\}}", text, re.S)
    if not m:
        raise SystemExit(f"FAIL session: missing {name}")
    return m.group(1)


def main() -> int:
    text = MAIN.read_text(errors="ignore")
    danger = [
        "save_console_state_smem",
        "load_console_state_smem",
        "Bfile_DeleteEntry",
        "save_script",
    ]
    blocks = "\n".join(body(text, name) for name in ["save", "save_session", "restore_session", "quit_handler"])
    leaks = [item for item in danger if item in blocks]
    if leaks:
        raise SystemExit("FAIL session: file-backed call in session path " + ", ".join(leaks))
    if 'Console_Output((unsigned char*)"Disabled")' not in text:
        raise SystemExit("FAIL session: save command does not report disabled")
    print("OK session disabled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

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
    blocks = "\n".join(body(text, name) for name in ["save", "save_session", "restore_session", "quit_handler"])
    required = [
        'save_console_state_smem("\\\\\\\\fls0\\\\session.xw")',
        'load_console_state_smem("\\\\\\\\fls0\\\\session.xw")',
        'restore_session("session")',
        "save_session();",
    ]
    missing = [item for item in required if item not in text]
    if missing:
        raise SystemExit("FAIL session: shared session path missing " + ", ".join(missing))
    forbidden = [
        'Console_Output((unsigned char*)"Disabled")',
        "Bfile_DeleteEntry",
        "save_script",
    ]
    leaks = [item for item in forbidden if item in blocks]
    if leaks:
        raise SystemExit("FAIL session: unwanted session path call " + ", ".join(leaks))
    print("OK session shared")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

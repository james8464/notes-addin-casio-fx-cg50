#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LLM = ROOT / "c++/tools/tests_cpp/shared_llm.py"
TUI = ROOT / "c++/tools/tests_cpp/run_tests_tui.py"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def main() -> int:
    llm = LLM.read_text(errors="ignore")
    tui = TUI.read_text(errors="ignore")
    markers = [
        "Edexcel A-level/Further Maths examiner",
        "what a student should write in an exam",
        "NEEDS_REVIEW if the answer is correct but working is generic",
        "Generic/meta lines are not valid working",
        "big jumps are NEEDS_REVIEW",
    ]
    missing = [m for m in markers if m not in llm]
    if missing:
        return fail("exam LLM prompt markers missing: " + ", ".join(missing))
    if "check_working_quality=False" not in tui or "verify_batch(items)" not in tui:
        return fail("TUI LLM verification path changed")
    print("OK TUI LLM prompt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

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
        "strict symbolic-maths examiner",
        "would get full method/accuracy marks",
        "When working is NOT required",
        "genuinely trivial tasks",
        "When working IS required",
        "No big jumps",
        "Generic calculator/meta text is not valid working",
        "Prefer A-level/Further Maths methods",
        "do not reject advanced methods for genuinely advanced inputs",
    ]
    missing = [m for m in markers if m not in llm]
    if missing:
        return fail("exam LLM prompt markers missing: " + ", ".join(missing))
    if "check_working_quality=False" not in tui or "verify_batch(items)" not in tui:
        return fail("TUI LLM verification path changed")
    if "_batch_line_for_record" not in tui:
        return fail("TUI LLM batch explanations are not per-record")
    print("OK TUI LLM prompt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
KHICAS = ROOT / "c++/khicas/upstream/giac90_1addin"
THEME = ROOT / "c++/addin/src/ui/theme.hpp"
RUN_TESTS = ROOT / "c++/tools/run_tests_cpp.py"


def fail(msg: str) -> int:
    print(f"FAIL: {msg}")
    return 1


def read(path: Path) -> str:
    return path.read_text(errors="ignore")


def main() -> int:
    console = read(KHICAS / "console.cc")
    maincc = read(KHICAS / "main.cc")
    textgui = read(KHICAS / "textGUI.cpp")
    theme = read(THEME)
    run_tests = read(RUN_TESTS)

    required = [
        (console, "| VOIR | CMDS | A<>A | FICH.", "console French softkeys uppercase"),
        (console, "| VIEW | CMDS | A<>A | FILE ", "console English softkeys uppercase"),
        (maincc, " | EDIT+-| CMDS | A<>A | EVAL", "equation editor softkeys uppercase"),
        (textgui, " TESTS | LOOPS | MISC | CMDS | A<>A |FILE ", "text UI softkeys uppercase"),
        (console, "F1 ALG\\n", "default FMenu labels uppercase"),
        (console, "F2 CALC\\n", "default FMenu labels uppercase"),
        (console, "original_cfg=(unsigned char *)conf_standard", "FMenu uses uppercase built-in config"),
        (run_tests, "ui_labels_status", "run_tests includes UI label/status guard"),
    ]
    for haystack, needle, desc in required:
        if needle not in haystack:
            return fail(desc)

    forbidden = [
        (console, "| voir | cmds | A<>a | Fich.", "lowercase console softkeys"),
        (console, "| view | cmds | A<>a | File ", "lowercase console softkeys"),
        (maincc, " | edit+-| cmds | A<>a | eval", "lowercase editor softkeys"),
        (textgui, " tests | loops | misc | cmds | A<>a |File ", "lowercase text UI softkeys"),
        (console, "F1 alg\\n", "lowercase default FMenu labels"),
        (console, "F2 calc\\n", "lowercase default FMenu labels"),
        (console, 'load_script((char*)"\\\\\\\\fls0\\\\FMENU.cfg"', "stale external FMenu labels can stay lowercase"),
        (theme, "current_time_text", "native status time helper"),
        (theme, "rtc_get_time", "native status RTC read"),
        (theme, "time.c_str()", "native status time draw"),
    ]
    for haystack, needle, desc in forbidden:
        if needle in haystack:
            return fail(desc)

    print("OK: UI labels uppercase; status time removed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

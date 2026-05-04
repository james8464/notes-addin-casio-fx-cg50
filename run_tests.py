#!/usr/bin/env python3
# Copy/paste in terminal to launch TUI:
# cd /Users/james/Developer/Python/CASIO && python3 run_tests.py tui

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parent


def run(cmd: list[str]) -> int:
    return subprocess.call(cmd, cwd=str(REPO))


def main() -> int:
    args = [a.lower() for a in sys.argv[1:]]
    if args and args[-1] == "compile":
        return run(["./compile"])
    if args and args[0] in ("tui", "--tui"):
        return run([sys.executable, "c++/tools/tests_cpp/run_tests_tui.py"] + sys.argv[2:])
    if args and args[0] in ("fuzz", "--fuzz"):
        return run([sys.executable, "c++/tools/run_tests_cpp.py", "--fuzz"])
    if args and args[0] in ("--help", "-h", "help"):
        print("usage: python3 run_tests.py [compile|tui|fuzz]")
        print("default: run C++ host/.g3a parity checks")
        print("compile: build calculator_files/CasioCAS.g3a")
        print("tui: launch C++ host testing TUI")
        print("fuzz: replay C++ regression fuzz cases")
        return 0
    return run([sys.executable, "c++/tools/run_tests_cpp.py"])


if __name__ == "__main__":
    raise SystemExit(main())

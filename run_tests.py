#!/usr/bin/env python3
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
    if args and args[0] in ("--help", "-h", "help"):
        print("usage: python3 run_tests.py [compile]")
        print("default: run C++ host/.g3a parity checks")
        print("compile: build CasioCAS.g3a at repo root")
        return 0
    return run([sys.executable, "c++/tools/run_tests_cpp.py"])


if __name__ == "__main__":
    raise SystemExit(main())

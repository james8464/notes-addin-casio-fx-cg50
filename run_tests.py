#!/usr/bin/env python3
from __future__ import annotations

import runpy
from pathlib import Path


def main() -> None:
    repo = Path(__file__).resolve().parent
    target = repo / "python" / "tests" / "run_tests.py"
    runpy.run_path(str(target), run_name="__main__")


if __name__ == "__main__":
    main()


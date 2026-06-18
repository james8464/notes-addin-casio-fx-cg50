#!/usr/bin/env python3
"""Regression tests for host exact callback support of KhiCAS root(...)."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"

CASES = [
    ("simplify(root(2^2,2))", ("2",)),
    ("simplify(root(27,3))", ("3",)),
    ("simplify(root((-8),3))", ("-2",)),
]


def main() -> int:
    env = os.environ.copy()
    env["CASCAS_HOST_PRODUCTION"] = "1"
    for expr, markers in CASES:
        proc = subprocess.run(
            [str(RUNNER), expr],
            cwd=ROOT,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        out = proc.stdout
        if proc.returncode or "Err:" in out or "unsupported" in out:
            print(f"FAIL host root exact {expr}: {out.strip()}", file=sys.stderr)
            return 1
        missing = [m for m in markers if m not in out]
        if missing:
            print(f"FAIL host root exact {expr}: missing {missing}: {out.strip()}", file=sys.stderr)
            return 1
    print(f"OK host root exact cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

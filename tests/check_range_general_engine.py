#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    (
        "range(sqrt(x-1),x,0,4)",
        ["1 <= x <= 4", "natural domain", "f(1) = 0", "f(4) = sqrt(3)", "0 <= y <= sqrt(3)", "Verified under constraint"],
    ),
    (
        "range(ln(x-1),x,0,4)",
        ["x > 1", "natural domain", "as x -> 1, y -> -infinity", "f(4) = ln(3)", "y <= ln(3)", "Verified under constraint"],
    ),
    (
        "range(tan(x),x,0,pi)",
        ["0 <= x <= pi", "pole at x=pi/2", "split interval at pole", "range: all real", "Verified under constraint"],
    ),
    (
        "range(abs(x^2-4),x,-3,3)",
        ["split at x=-2,2", "f(-2) = 0", "f(2) = 0", "0 <= y <= 5", "Verified under constraint"],
    ),
    (
        "range(x^2,x!=0)",
        ["x != 0", "as x -> 0, y -> 0", "y > 0", "Verified under constraint"],
    ),
]


def run(expr: str) -> str:
    env = dict(os.environ, CASCAS_HOST_PRODUCTION="1")
    proc = subprocess.run(
        [str(RUNNER), expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=30,
        env=env,
    )
    if proc.returncode:
        raise AssertionError(f"{expr}: rc={proc.returncode}\n{proc.stdout}\n{proc.stderr}")
    return proc.stdout.strip()


def main() -> int:
    for expr, markers in CASES:
        out = run(expr)
        flat = out.replace(" ", "")
        missing = [m for m in markers if m not in out and m.replace(" ", "") not in flat]
        if missing:
            raise AssertionError(f"{expr}: missing {missing}\n{out}")
        if "Err:" in out:
            raise AssertionError(f"{expr}: unexpected error\n{out}")
    print(f"OK range general engine cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

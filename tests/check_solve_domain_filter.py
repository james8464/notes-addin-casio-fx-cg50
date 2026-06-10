#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    (
        "solve((x^2-1)/(x-1)=2,x)",
        ["x - 1 != 0", "Reject x=1", "denominator domain fails", "x = []", "Verified by domain check"],
    ),
    (
        "solve((x^2-1)/(x-1)=3,x)",
        ["x - 1 != 0", "Reject x=1", "x = [2]", "Verified by domain check"],
    ),
    (
        "solve(k*(k+3)/(k+1)=2,k)",
        ["k + 1 != 0", "k = [-2, 1]"],
    ),
    (
        "solve(cos(x)=0,x,0,pi)",
        ["0 <= x <= pi", "x = [pi/2]", "Verified under constraint"],
    ),
    (
        "solve(tan(x)=1,x,0,pi)",
        ["0 <= x <= pi", "x = [pi/4]", "Verified under constraint"],
    ),
    (
        "solve(x^2=1,x>0)",
        ["x > 0", "Reject x=-1", "x = [1]", "Verified under constraint"],
    ),
]

FORBIDDEN = [
    "x = [1, 2]",
    "x = [-1, 1]",
    "x = atan(1) + n*pi",
    "Reject k=-1",
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
        bad = [m for m in FORBIDDEN if m in out]
        if bad:
            raise AssertionError(f"{expr}: forbidden {bad}\n{out}")
    print(f"OK solve domain filter cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

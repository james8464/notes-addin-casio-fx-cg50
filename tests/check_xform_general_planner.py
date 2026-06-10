#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    (
        "xform((sec(x)+tan(x))^2,(1+sin(x))/(1-sin(x)))",
        [
            "Target equivalent after simplification",
            "normal(start-target)=0",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(((cos(3*x)/sin(x))+((sin(3*x))/cos(x))),2*cot(2*x))",
        [
            "Verified by equivalence check",
            "2*cot(2*x)",
        ],
    ),
]

FORBIDDEN = [
    "xform(start,target)",
    "no route",
    "not equivalent",
    "not verified",
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
    print(f"OK xform general planner cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

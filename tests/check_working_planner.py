#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLANNER = ROOT / "tools" / "working_planner.py"

CASES = [
    (
        "xform((x+1)^2+(x-1)^2,2*x^2+2)",
        ["Planner search:", "Expand brackets:", "Verified by equivalence check"],
    ),
    (
        "xform(cot(x),cos(x)/sin(x))",
        ["Planner search:", "Reciprocal identities:", "cos(x)/sin(x)", "Verified by equivalence check"],
    ),
    (
        "xform(tan(x),sin(x)/cos(x))",
        ["Planner search:", "Reciprocal identities:", "sin(x)/cos(x)", "Verified by equivalence check"],
    ),
    (
        "xform((cos(x)+sin(x))*(cosec(x)-sec(x)),2*cot(2*x))",
        [
            "Planner search:",
            "Reciprocal identities:",
            "Cancel common factors:",
            "Double-angle identities:",
            "2*cot(2*x)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(2*sin(2*x)*cos(x),sin(3*x)+sin(x))",
        ["Planner search:", "Product-to-sum identities:", "sin(x) + sin(3*x)", "Verified by equivalence check"],
    ),
    (
        "xform(ln(x*y),ln(x)+ln(y))",
        ["Planner search:", "Log laws:", "ln(x) + ln(y)", "Verified by equivalence check"],
    ),
    (
        "xform(ln(x/y),ln(x)-ln(y))",
        ["Planner search:", "Log laws:", "ln(x) - ln(y)", "Verified by equivalence check"],
    ),
    (
        "xform(ln(x^4),4*ln(x))",
        ["Planner search:", "Log laws:", "4*ln(x)", "Verified by equivalence check"],
    ),
    (
        "xform(log(2,x^3),3*log(2,x))",
        ["Planner search:", "Log laws:", "3*ln(x)/ln(2)", "Verified by equivalence check"],
    ),
    (
        "xform(ln(x)+ln(y),ln(x*y))",
        ["Planner search:", "Log laws:", "ln(x*y)", "Verified by equivalence check"],
    ),
]


def compact(s: str) -> str:
    return "".join(s.split()).replace("**", "^").replace("csc", "cosec")


def main() -> int:
    for expr, markers in CASES:
        proc = subprocess.run(
            ["python3", str(PLANNER), expr],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=10,
        )
        if proc.returncode != 0:
            raise AssertionError(f"{expr}: planner failed\nSTDOUT:\n{proc.stdout}\nSTDERR:\n{proc.stderr}")
        packed = compact(proc.stdout)
        for marker in markers:
            if compact(marker) not in packed:
                raise AssertionError(f"{expr}: missing {marker!r}\n{proc.stdout}")
    print(f"OK working planner cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

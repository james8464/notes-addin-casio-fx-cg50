#!/usr/bin/env python3
"""Regression tests for host exact callback parser coverage."""

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
    ("limit(sum((x/2)^k,k,0,2),x,1)", ("7/4",)),
    ("simplify((cos(2*pi/10)+i*sin(2*pi/10))^10)", ("1",)),
    ("simplify((cos(2*pi/30)+i*sin(2*pi/30))^30)", ("1",)),
    ("simplify(binomial(5,2)+binomial(5,3))", ("20",)),
    ("limit((abs(x)-x)/x,x,0,+)", ("0",)),
    ("limit((abs(x)-x)/x,x,0,-)", ("-2",)),
    ("factor(resultant(x^2+a*x+1,x^2+b*x+2,x))", ("2*a^2 - 3*a*b + b^2 + 1",)),
    ("factor(x^60-a^60)", ("(x-a)*sum(x^(60-1-k)*a^k,k,0,59)",)),
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
            timeout=20,
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

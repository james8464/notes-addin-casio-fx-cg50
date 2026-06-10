#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CASES: list[tuple[str, list[str]]] = [
    ("solve(exp(x)+x=3,x)", ["Move all to LHS", "Solve F(x)=0", "Ans:", "LambertW"]),
    ("diff(exp(sin(x)^2)*cos(x),x)", ["Product:", "du/dx", "dv/dx", "u'v+uv'"]),
    ("integrate((x^5+1)/(x^2+x+1),x)", ["Complete square", "log + atan", "Ans:"]),
    ("defint(exp(-x),x,0,inf)", ["F(x)", "Improper integral", "lim T -> oo", "Ans:", "1"]),
    ("domain(ln((x-2)/(x+1)),x)", ["Domain", "x + 1 != 0", "(x-2)/(x+1) > 0", "x < -1 or x > 2"]),
    ("range(exp(-x^2)+x,x)", ["f'(x)", "as x -> -infinity", "range: all real"]),
    ("xform((x^2-1)/(x-1),x+1)", ["Diff powers", "x^2 - 1 = (x-1)(x+1)", "x+1"]),
    ("taylor(ln(1+sin(x)),x,0,5)", ["taylor method", "trace:", "simplify:", "Answer:"]),
    ("normal((x^2-1)/(x-1))", ["Diff powers", "x^2 - 1 = (x-1)(x+1)", "x+1"]),
    ("limit((x^2-1)/(x-1),x=1)", ["limit:", "simplify:", "x + 1", "Ans:", "2"]),
    ("coeff((1+x+x^3)^7,x,9)", ["252"]),
    ("partfrac((x^4+2*x+1)/(x^3-x),x)", ["D:", "A,B over factors", "Answer:"]),
]

FORBIDDEN_ONLY = {"Verified", "F=A"}


def run(expr: str) -> str:
    env = dict(os.environ, CASCAS_HOST_PRODUCTION="1")
    proc = subprocess.run(
        [str(ROOT / "tools/khicas_host_runner"), expr],
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
        if out in FORBIDDEN_ONLY:
            raise AssertionError(f"{expr}: bare fallback output\n{out}")
        if "Verified" in out:
            raise AssertionError(f"{expr}: should not print Verified\n{out}")
        if out.startswith("Ans:\n") and out.count("\n") <= 2:
            raise AssertionError(f"{expr}: exact-only output\n{out}")
        if "roots(F(z))" in out:
            raise AssertionError(f"{expr}: roots placeholder leaked\n{out}")
    print(f"OK khicas fallback explainers cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

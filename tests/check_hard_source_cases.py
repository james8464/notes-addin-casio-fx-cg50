#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


CASES: list[tuple[str, list[str]]] = [
    ("range(log(x)/x,x)", ["f'(x)=(1-ln(x))/x^2", "range: y <= 1/e", "Verified"]),
    ("range((ln(x))^2-4*ln(x)+7,x)", ["Let u=ln(x)", "range: y >= 3", "Verified"]),
    ("range(abs(5*x+1)-abs(x),x)", ["piecewise modulus", "range: y >= -1/5", "Verified"]),
    (
        "xform(log(2,(x^2-1)^3),3*log(2,x-1)+3*log(2,x+1))",
        ["Log law", "x^2-1 = (x-1)(x+1)", "Verified under domain"],
    ),
    (
        "xform(ln((sqrt(x+1)-1)/(sqrt(x+1)+1)),ln(x/(sqrt(x+1)+1)^2))",
        ["Rationalise log", "conjugate", "Verified domain"],
    ),
    (
        "integrate((x^3+1)/(x^2+1),x)",
        ["Divide:", "1/2*x^2 - 1/2*ln(x^2+1) + atan(x) + C", "Verified"],
    ),
    ("integrate(x/(sqrt(x^2+4)),x)", ["sqrt(x^2 + 4) + C", "Verified"]),
    (
        "integrate(sec(2*x)^2/(1+tan(2*x)),x)",
        ["Sub u=1+tan(2*x)", "1/2*ln(abs(1+tan(2*x))) + C", "Verified"],
    ),
    (
        "solve(2^(3*x-1)=5^(x+2),x)",
        ["x*(3*ln(2) - ln(5))", "x = [(2*ln(5) + ln(2))/(3*ln(2) - ln(5))]", "Verified"],
    ),
    (
        "solve(2*sin(x)+sqrt(3)*cos(x)=1,x)",
        ["R-form:", "sqrt(7)*sin", "Verified R-form"],
    ),
    (
        "solve(sin(x)+cos(x)=sqrt(2)*sin(2*x),x)",
        ["R-form/double angle:", "x = pi/4", "Verified by substitution"],
    ),
]


BAD = ("not equivalent", "not verified", "no route")


def run(expr: str) -> str:
    proc = subprocess.run(
        [str(ROOT / "tools/khicas_host_runner"), expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=20,
    )
    if proc.returncode:
        raise AssertionError(f"{expr}: rc={proc.returncode}\n{proc.stdout}\n{proc.stderr}")
    return proc.stdout


def main() -> int:
    for expr, markers in CASES:
        out = run(expr)
        compact = out.replace(" ", "")
        active_markers = [m for m in markers if "Verified" not in m]
        missing = [m for m in active_markers if m not in out and m.replace(" ", "") not in compact]
        if missing:
            raise AssertionError(f"{expr}: missing {missing}\n{out}")
        bad = [m for m in BAD if m in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad markers {bad}\n{out}")
    print(f"OK hard source cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

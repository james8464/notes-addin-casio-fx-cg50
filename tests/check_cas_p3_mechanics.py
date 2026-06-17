#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"

CASES = [
    ("suvat(u=2,a=3,t=4)", ["SUVAT", "v = u + at", "= 14", "s = 32"]),
    ("force(12,3)", ["Newton's second law", "F = ma", "36 N"]),
    ("weight(6)", ["Weight", "W = mg", "294/5 N"]),
    ("moment(10,4)", ["Moment", "M = Fd", "40 N m"]),
    ("friction(2/5,25)", ["Friction", "Fmax = mu R", "10 N"]),
    ("resolve(20,30)", ["Resolve", "adjacent = 20*cos(30)", "opposite = 20*sin(30)"]),
    ("incline(5,30)", ["Incline", "weight down slope", "normal reaction"]),
    ("projectile(20,30)", ["Projectile", "ux =", "time of flight", "range", "max height"]),
    ("projectile(20,30,2)", ["Projectile", "x = ux*t", "y = uy*t - 1/2*g*t^2"]),
]

BANNED = ["Verified", "not checked", "syntax error", "Bad Argument", "unsupported"]


def run(expr: str) -> str:
    return subprocess.check_output([str(RUNNER), expr], cwd=ROOT, text=True, timeout=10)


def main() -> int:
    for expr, needles in CASES:
        out = run(expr)
        if len([line for line in out.splitlines() if line.strip()]) < 2:
            raise AssertionError(f"{expr}: too few lines\n{out}")
        for needle in needles:
            if needle not in out:
                raise AssertionError(f"{expr}: missing {needle!r}\n{out}")
        for bad in BANNED:
            if bad in out:
                raise AssertionError(f"{expr}: banned {bad!r}\n{out}")
    print(f"OK CAS Paper 3 mechanics cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

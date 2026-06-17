#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"

CASES = [
    ("suvat(u=2,a=3,t=4)", ["SUVAT", "v = u + at", "= 14", "s = 32"]),
    ("suvat(v=14,a=3,t=4)", ["SUVAT", "u = v - at", "u = 2", "s = 32"]),
    ("suvat(s=32,v=14,t=4)", ["SUVAT", "u = 2s/t - v", "u = 2", "a = 3"]),
    ("suvat(s=32,a=3,t=4)", ["SUVAT", "u = (s - 1/2 at^2)/t", "u = 2", "v = 14"]),
    ("force(12,3)", ["Newton II", "F=ma", "36 N"]),
    ("weight(6)", ["Weight", "W=mg", "294/5 N"]),
    ("moment(10,4)", ["Moment", "M=Fd", "40 N m"]),
    ("work(12,5)", ["Work", "W=Fd", "60 J"]),
    ("power(120,4)", ["Power", "P=W/t", "30 W"]),
    ("power(F=20,v=3)", ["Power", "P=Fv", "60 W"]),
    ("energy(2,5,3)", ["Energy", "KE=1/2mv^2", "KE=25 J", "GPE=294/5 J"]),
    ("impulse(0.5,4,-2)", ["Impulse", "I=m(v-u)", "-3 Ns"]),
    ("friction(2/5,25)", ["Friction", "Fmax=muR", "10 N"]),
    ("resolve(20,30)", ["Resolve", "adjacent = 20*cos(30)", "opposite = 20*sin(30)"]),
    ("incline(5,30)", ["Incline", "down =", "R ="]),
    ("projectile(20,30)", ["Projectile", "ux =", "T =", "range", "Hmax"]),
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

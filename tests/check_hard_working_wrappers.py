#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CASES: list[tuple[str, list[str]]] = [
    (
        "integrate((10*x^2-23*x+11)/((x-2)*(x-3)*(2*x-1)),x)",
        ["Denominator factor", "Partial fraction form", "Integrate term by term", "KhiCAS exact", "Verified"],
    ),
    (
        "integrate((3*x)/(1+2*x-x^2),x,0,1)",
        ["F(x)", "F(1)-F(0)", "KhiCAS exact", "Verified"],
    ),
    (
        "defint(1/(x^2-1),x,2,inf)",
        ["Improper integral", "lim T -> oo", "F(T)-F(2)", "ln(3)/2", "Verified"],
    ),
    (
        "defint(ln(x)/x^2,x,1,inf)",
        ["Improper integral", "lim T -> oo", "F(T)-F(1)", "KhiCAS exact", "1", "Verified"],
    ),
    (
        "integrate(sin(x)^4*cos(x)^2,x)",
        ["Trig powers", "power-reduction", "KhiCAS exact", "Verified"],
    ),
    (
        "integrate(sin(x)^5*cos(x)^4,x)",
        ["Trig powers", "odd sin power", "substitute", "KhiCAS exact", "Verified"],
    ),
    (
        "integrate(sec(x)^4*tan(x)^3,x)",
        ["Trig powers", "u=sec(x)", "KhiCAS exact", "Verified"],
    ),
    (
        "integrate(1/(x^2+4*x+13),x)",
        ["Complete square", "(x + 2)^2 + 9", "atan", "Verified"],
    ),
    (
        "integrate((x^2+1)/(x^2+2*x+5),x)",
        ["Divide numerator first", "Complete square", "log + atan", "Verified"],
    ),
    (
        "taylor(exp(sin(x)),x,0,6)",
        ["sin(x) series", "exp(u) series", "substitute and truncate", "KhiCAS exact", "Verified"],
    ),
    (
        "solve(z^4=-8-8*sqrt(3)*i,z)",
        ["Polar form", "r = 16", "theta = 4*pi/3", "nth-root formula", "roots:", "Verified"],
    ),
    (
        "diff((1+2*cos(t))*sin(t),t)/diff((1+2*cos(t))*cos(t),t)",
        ["dy/dt", "dx/dt", "dy/dx = (dy/dt)/(dx/dt)", "KhiCAS exact", "Verified"],
    ),
]


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
    return proc.stdout


def main() -> int:
    for expr, markers in CASES:
        out = run(expr)
        flat = out.replace(" ", "")
        missing = [m for m in markers if m not in out and m.replace(" ", "") not in flat]
        if missing:
            raise AssertionError(f"{expr}: missing {missing}\n{out}")
        if "inf)" in out or "/(inf)" in out:
            raise AssertionError(f"{expr}: unevaluated infinity in output\n{out}")
    print(f"OK hard working wrappers cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

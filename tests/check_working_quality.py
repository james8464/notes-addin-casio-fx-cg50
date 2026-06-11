#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    (
        "xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2x),k)",
        [
            "Search:",
            "Isolate param:",
            "k = 2",
            "Substitute:",
        ],
    ),
    (
        "diff(7*x*exp(x)/sqrt(exp(2*x)-2),x)",
        [
            "Quotient:",
            "u = 7*x*exp(x)",
            "du/dx = 7*exp(x) + 7*x*exp(x)",
            "dv/dx =",
            "(u'v-uv')/v^2",
            "=",
        ],
    ),
    (
        "diff(atan(9/x^2),x)",
        [
            "Chain:",
            "u = 9*x^-2",
            "du/dx = -18*x^-3",
            "d/dx atan(u)",
            "/(1+(",
        ],
    ),
    (
        "diff(9/(2*x^3),x)",
        [
            "Neg powers:",
            "9/(2*x^3) = 9/2*x^-3",
            "-27/2*x^-4",
        ],
    ),
    (
        "xform(sin(x),cos(x)+7)",
        [
            "Try:",
            "Target:",
        ],
    ),
    (
        "solve(5*x+8=4*x+4,x)",
        [
            "Solve: 5*x+8=4*x+4",
            "x = -((8) - (4))/((5) - (4))",
        ],
    ),
    (
        "solve([3*y+2*x=22,y=3/2*x+3],[x,y])",
        [
            "Solve: [3*y+2*x=22,y=3/2*x+3]",
            "Collect:",
            "[x,y] = [2, 6]",
            "[[x=2, y=6]]",
        ],
    ),
    (
        "solve(log(((((((n)+(z)))*(ln((b))))+(((b)-(k)-(-596.87526)))))^2+8,sqrt(abs(((t)+(sqrt(56))+(u)))+((((pi))+(e)))^2))=(z))",
        [
            "Move terms to LHS",
            "F(b)=L-R",
            "Domain",
            "b = roots(F(b))",
        ],
    ),
    (
        "defint(1/((sin(x)+2*cos(x))*(sin(x)+3*cos(x))),x,asin(3/5),acos(3/5))",
        [
            "u=tan(x)",
            "sin(x)=u*cos(x)",
            "bounds: u=3/4 to 4/3",
            "A/(u + 2)+B/(u + 3)",
            "F(4/3) - F(3/4)",
            "ln(150/143)",
        ],
    ),
    (
        "integrate(sin(log(14,log(tan(sqrt(abs(1676*u^52-1314*u^51+2509*u^50-3057*u^49+6241*u^48-7626*u^47+1772*u^46-6044*u^45+7219*u^44-9404*u^43+7302*u^42+352*u^41)))+8,((v)+(((939/834)*sqrt(571))))))))",
        [
            "Err: complexity guard",
        ],
    ),
    (
        "lcm(tan(cos(abs(exp(sqrt(2554*u^44+7170*u^43+5818*u^42+2085*u^41-6467*u^40-5271*u^39-1035*u^38-8230*u^37+1817*u^36+466*u^35-4014*u^34-4723*u^33))))),x)",
        [
            "General Pure method:",
        ],
    ),
]

BAD = [
    "symbolic form",
    "use distributive law\n",
    "\n/2\n",
    "fake",
    "no verified a-level working route found",
    "solution_set",
    "A=integrand",
    "no exact form",
    "use ac+bd=0 and ad=-bc",
    "result kept unevaluated",
    "integral(a,",
]


def run(expr: str) -> str:
    proc = subprocess.run(
        [str(RUNNER), "--alg", expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=10,
    )
    if proc.returncode:
        raise AssertionError(proc.stderr)
    return proc.stdout


def main() -> int:
    for expr, markers in CASES:
        out = run(expr)
        missing = [m for m in markers if m not in out]
        if missing:
            raise SystemExit(f"FAIL quality {expr}: missing {missing}\n{out}")
        if "Verified" in out:
            raise SystemExit(f"FAIL quality {expr}: should not print Verified\n{out}")
        bad = [m for m in BAD if m in out.lower()]
        if bad:
            raise SystemExit(f"FAIL quality {expr}: bad {bad}\n{out}")
    print(f"OK working quality cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

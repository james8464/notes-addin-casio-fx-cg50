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
            "Planner search:",
            "Parameter isolation:",
            "k = 2",
            "Verified by substitution",
            "Verified by equivalence check",
        ],
    ),
    (
        "diff(7*x*e^x/sqrt(e^(2*x)-2),x)",
        [
            "Quotient:",
            "u = 7*x*e^x",
            "du/dx = 7*x*exp(x) + 7*exp(x)",
            "dv/dx =",
            "(u'v-uv')/v^2",
            "Verified",
        ],
    ),
    (
        "diff(atan(9/x^2),x)",
        [
            "Chain:",
            "u = 9*x^-2",
            "du/dx = -18*x^-3",
            "d/dx atan(u)",
            "Verified",
        ],
    ),
    (
        "diff(9/(2*x^3),x)",
        [
            "Neg powers:",
            "9/(2*x^3) = 9/2*x^-3",
            "-27/2*x^-4",
            "Verified",
        ],
    ),
    (
        "xform(sin(x),cos(x)+7)",
        [
            "Check equivalence:",
            "not equivalent",
        ],
    ),
    (
        "solve(5*x+8=4*x+4,x)",
        [
            "Solve: 5*x+8=4*x+4",
            "KhiCAS exact evaluation:",
            "x = [-4]",
            "Verified",
        ],
    ),
    (
        "solve([3*y+2*x=22,y=3/2*x+3],[x,y])",
        [
            "Solve: [3*y+2*x=22,y=3/2*x+3]",
            "Collect:",
            "[x,y] = [2, 6]",
            "KhiCAS exact evaluation:",
            "[[x=2, y=6]]",
            "Verified",
        ],
    ),
    (
        "solve(log(((((((n)+(z)))*(ln((b))))+(((b)-(k)-(-596.87526)))))^2+8,sqrt(abs(((t)+(sqrt(56))+(u)))+((((pi))+(e)))^2))=(z))",
        [
            "Move all terms to one side",
            "F(b)=L-R",
            "Domain",
            "b = roots(F(b))",
        ],
    ),
    (
        "integrate(sin(log(14,log(tan(sqrt(abs(1676*u^52-1314*u^51+2509*u^50-3057*u^49+6241*u^48-7626*u^47+1772*u^46-6044*u^45+7219*u^44-9404*u^43+7302*u^42+352*u^41)))+8,((v)+(((939/834)*sqrt(571))))))))",
        [
            "Let A be the integrand.",
            "Search A-level methods.",
            "integral(A,x) + C",
        ],
    ),
    (
        "lcm(tan(cos(abs(exp(sqrt(2554*u^44+7170*u^43+5818*u^42+2085*u^41-6467*u^40-5271*u^39-1035*u^38-8230*u^37+1817*u^36+466*u^35-4014*u^34-4723*u^33))))),x)",
        [
            "Least common multiple:",
            "Result kept unevaluated:",
            "lcm(A,x)",
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
        positive_verified = any(m in out for m in ("Verified by", "\nVerified", "Verified\n"))
        if "not equivalent" in out.lower() and positive_verified:
            raise SystemExit(f"FAIL quality {expr}: non-equivalent route marked verified\n{out}")
        bad = [m for m in BAD if m in out.lower()]
        if bad:
            raise SystemExit(f"FAIL quality {expr}: bad {bad}\n{out}")
    print(f"OK working quality cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

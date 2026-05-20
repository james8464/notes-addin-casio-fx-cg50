#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from working_audit_utils import compact_math, markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


GLOBAL_BANS = (
    "Done",
    "Chk:",
    "pick rule",
    "Use chain rule",
    "Simplify:",
)


@dataclass(frozen=True)
class ContractCase:
    route: str
    args: tuple[str, str]
    marks: tuple[str, ...]
    bans: tuple[str, ...] = ()
    min_lines: int = 3


CASES = (
    ContractCase(
        "solve/log/domain/quadratic",
        ("--alg", "solve(log(2,x^2+4*x+3)=4+log(2,x^2+x),x)"),
        (
            "Domain: x^2 + 4*x + 3 > 0",
            "Domain: x^2 + x > 0",
            "log(2,(x^2 + 4*x + 3)/(x^2 + x)) = 4",
            "(x^2 + 4*x + 3)/(x^2 + x) = 16",
            "x = -1 rejected by domain",
            "x = [1/5]",
        ),
    ),
    ContractCase(
        "solve/square/plus-minus",
        ("--alg", "solve((x+4)^2=16,x)"),
        ("(x + 4)^2 = 16", "x + 4 = +/-4", "x = [0, -8]"),
    ),
    ContractCase(
        "solve/contradiction/no-fake-root",
        ("--alg", "solve((2*x+5)^2=4*x^2+20*x+30,x)"),
        ("-5 != 0", "x = []"),
        min_lines=2,
    ),
    ContractCase(
        "derive/chain/substitution",
        ("--derive", "(2*x+ln(x))^3,x"),
        (
            "y = (2*x + ln(x))^3",
            "u = 2*x + ln(x)",
            "du/dx = 1/x + 2",
            "dy/dx = 3*u^2*du/dx",
            "dy/dx = 3*(2*x + ln(x))^2*(1/x + 2)",
        ),
    ),
    ContractCase(
        "derive/product/quotient/formula",
        ("--derive", "x^2/(3*x-1),x"),
        ("u = x^2", "v = 3*x - 1", "u' = 2*x", "v' = 3", "y' = (u'v-u*v')/v^2"),
    ),
    ContractCase(
        "derive/implicit/collect",
        ("--derive", "ln(x+y)=x*y,x,method=implicit"),
        (
            "d/dx[ln(x + y)] = (1 + dy/dx)/(x + y)",
            "(1 + dy/dx)/(x + y) = y + x*dy/dx",
            "dy/dx*(1 - x*(x + y)) = y*(x + y) - 1",
        ),
    ),
    ContractCase(
        "derive/parametric/no-duplicate",
        ("--derive", "t^5+4,t^5-5*t,t,method=param"),
        ("dx/dt = 5*t^4", "dy/dt = 5*t^4 - 5", "dy/dx = (dy/dt)/(dx/dt), dx/dt != 0"),
        ("dy/dx = (5*t^4 - 5)/(5*t^4)\ndy/dx = (5*t^4 - 5)/(5*t^4)",),
    ),
    ContractCase(
        "integrate/parts/looping",
        ("--int", "e^(2*x)*cos(3*x),method=parts"),
        ("I = Int(e^(2*x)*cos(3*x)) dx", "J = Int(e^(2*x)*sin(3*x)) dx", "Sub J:", "Collect: I terms", "13/4*I =", "I ="),
    ),
    ContractCase(
        "integrate/substitution/limits",
        ("--int", "defint(e^x/(e^x+1),x,-1,1),method=sub"),
        ("D = e^(x) + 1", "D' = e^(x)", "F(1)", "F(-1)"),
    ),
    ContractCase(
        "integrate/partial-fractions/coefficients",
        ("--int", "(5*x+7)/((x-1)*(x^2+2)),method=pf"),
        ("PF: A/(x - 1)+(Bx+C)/(x^2 + 2)", "coeff x^2:", "coeff x:", "coeff 1:", "A = 4, B = -4, C = 1"),
    ),
    ContractCase(
        "trig/general/branches/filter",
        ("--trig", "sin(2*x)=cos(x),x,0,2*pi,method=general"),
        ("A = x", "sin(2A) = 2*sin(A)*cos(A)", "cos(A) = 0 or sin(A) = 1/2", "x = pi/2 + n*pi"),
    ),
    ContractCase(
        "trig/rform/method",
        ("--trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform"),
        ("R = sqrt(3^2 + 4^2) = 5", "3*cos(x)+4*sin(x)=5*cos(x-alpha)", "x = ["),
    ),
    ContractCase(
        "functions/composition/expand-cancel",
        ("--alg", "compose(x^2-10*x,e^x+5)"),
        ("f(x) = x^2 - 10*x", "g(x) = e^(x) + 5", "f(g(x)) = e^(2*x) - 25"),
        ("(e^(x) + 5)^2",),
    ),
    ContractCase(
        "statistics/hypothesis/standardise/decision",
        ("--stats", "ztest(5.4,5,1.2,36,0.05,gt)"),
        ("H0: mu = 5", "z = (xbar-mu)/(sigma/sqrt(n)) = 2", "p = P(Z > 2)", "p < 0.05", "reject H0"),
    ),
    ContractCase(
        "mechanics/suvat/formula/substitute",
        ("--suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=suvat"),
        ("s = ut + 1/2at^2", "= 0*5 + 1/2*2*5^2", "s = 25"),
    ),
)


def meaningful_lines(text: str) -> list[str]:
    return [line.strip() for line in text.splitlines() if line.strip()]


def final_line_ok(text: str) -> bool:
    lines = meaningful_lines(text)
    if not lines:
        return False
    return not re.search(r"\b(done|answer|ans|check|method)\b", lines[-1], flags=re.I)


def main() -> int:
    if not HOST.exists():
        print(f"FAIL missing host: {HOST}")
        return 1

    bad = 0
    for case in CASES:
        proc = subprocess.run([str(HOST), *case.args], cwd=REPO, text=True, capture_output=True, timeout=15)
        out = proc.stdout + proc.stderr
        missing = [m for m in case.marks if not markers_present(out, [m])]
        low_out = out.lower()
        banned = [b for b in GLOBAL_BANS if b.lower() in low_out]
        banned += [b for b in case.bans if b and compact_math(b) in compact_math(out)]
        lines = meaningful_lines(out)
        if proc.returncode == 0 and not missing and not banned and len(lines) >= case.min_lines and final_line_ok(out):
            print("OK", case.route)
            continue
        bad += 1
        print("FAIL", case.route)
        if missing:
            print("  missing:", " | ".join(missing))
        if banned:
            print("  banned:", " | ".join(banned))
        if len(lines) < case.min_lines:
            print(f"  too few lines: {len(lines)} < {case.min_lines}")
        if not final_line_ok(out):
            print("  final line not maths-only")
        print(out)

    print(f"done bad {bad} / {len(CASES)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

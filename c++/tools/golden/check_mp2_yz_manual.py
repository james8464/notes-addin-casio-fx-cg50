#!/usr/bin/env python3
"""Manual MP2 Y/Z paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "Y3 sqrt second derivative",
        ["--derive", "x-sqrt(x),x,method=second"],
        ["dy/dx = 1 - 1/(2*sqrt(x))", "d2y/dx2 = 1/(4*x^(3/2))"],
        ["d2y/dx2 = 2*1/(2*sqrt(x))"],
    ),
    (
        "Y4 arctan-sum equation",
        ["--trig", "arctan((x-5)/(x-1))+arctan((x-4)/(x-3))=pi/4,x"],
        ["tan(A+B)", "x != 1,3", "x = 6"],
        ["x = []", "Failed to isolate"],
    ),
    (
        "Y12 sqrt substitution integral",
        ["--int", "(x+3)*sqrt(x)/(x+1)^2,method=sub"],
        ["Let u=sqrt(x)", "Integral becomes Integral(2u^2(u^2+3)/(u^2+1)^2) du", "2*sqrt(x) - 2*sqrt(x)/(x + 1) + C"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "Z2 Newton two iterations",
        ["--alg", "newton(x^10+5*x-449,x,1.8,2)"],
        ["Use Newton-Raphson", "f(x) = x^10 + 5*x - 449", "x = 1.838"],
        ["Expected )", "Unexpected end", "solve(newton"],
    ),
    (
        "Z3 rationalised log derivative",
        ["--derive", "ln(1/(sqrt(x^2+1)-x)),x"],
        ["Rationalise", "sqrt(x^2+1)+x", "dy/dx = 1/sqrt(x^2 + 1)"],
        ["d/dx("],
    ),
    (
        "Z5 hidden log derivative integral",
        ["--int", "cos(x)^3/((1+sin(x)^2)*sin(x)),method=sub,u=sin(x)+cosec(x)"],
        ["Try F=log(abs(sin(x)))-log(1+sin(x)^2)", "Use 1-sin(x)^2=cos(x)^2", "log(abs(sin(x)/(1 + sin(x)^2))) + C"],
        ["No elementary primitive"],
    ),
    (
        "Z7 parametric derivative",
        ["--derive", "x=tan(t)-sec(t),y=cot(t)-cosec(t),t,x,method=param"],
        ["dx/dt = sec(t)^2 - sec(t)tan(t)", "dy/dt = -cosec(t)^2 + cosec(t)cot(t)", "dy/dx = -(y^2 - 1)/(2*x)"],
        ["ERR:", "No parametric route"],
    ),
    (
        "Z8 inverse and domain",
        ["--alg", "inverse(ln(4-2*x),x,y)"],
        ["Swap x and y", "f^-1(x) = (4 - e^x)/2"],
        ["not isolated"],
    ),
    (
        "Z9 separable DE",
        ["--alg", "de_solve((1+x)*dy/dx=y*(1-x),y(0)=1)"],
        ["Separate variables", "ln(y) = -x + 2*ln(1+x) + C", "y = (x + 1)^2*e^(-x)"],
        ["Only one '=' supported", "solve(de_solve"],
    ),
]


def compact(s: str) -> str:
    return "".join(ch for ch in s if not ch.isspace())


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    out_compact = compact(out)
    misses = [needle for needle in needles if needle not in out and compact(needle) not in out_compact]
    bad = [needle for needle in banned if needle in out]
    if proc.returncode:
        misses.append(f"returncode={proc.returncode}")
    if misses or bad:
        return [f"{name}: missing {misses}, banned {bad}\n{out}"]
    return []


def main() -> int:
    failures: list[str] = []
    for name, args, needles, banned in CASES:
        failures.extend(run_case(name, args, needles, banned))
    if failures:
        print("\n\n".join(failures))
        return 1
    print(f"mp2_yz_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

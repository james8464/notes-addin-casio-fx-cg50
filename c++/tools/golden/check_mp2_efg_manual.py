#!/usr/bin/env python3
"""Manual MP2 E/F/G paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "E1a chain derivative",
        ["--derive", "(2*x+ln(x))^3,x"],
        ["u = 2*x + log(x)", "du/dx = 1/x + 2", "dy/dx = 3*u^2*du/dx", "3*(2*x + log(x))^2*(1/x + 2)"],
        ["Use chain rule", "limite(", "Answer: d/dx("],
    ),
    (
        "E1b quotient derivative",
        ["--derive", "x^2/(3*x-1),x"],
        ["u = x^2", "u' = 2*x", "v = 3*x - 1", "v' = 3", "y' = (u'v-u*v')/v^2"],
        ["Use quotient rule", "Answer: d/dx("],
    ),
    (
        "E1c chain trig derivative",
        ["--derive", "sin(3*x)^4,x"],
        ["u = sin(3*x)", "du/dx = 3*cos(3*x)", "dy/dx = 4*u^3*du/dx", "12*sin(3*x)^3*cos(3*x)"],
        ["Use chain rule", "Answer: d/dx("],
    ),
    (
        "E6 related-rate derivative core",
        ["--derive", "x^3*e^(-x^2),x"],
        ["f1 = x^3", "f1' = 3*x^2", "f2 = e^(-x^2)", "f2' = -2*e^(-x^2)*x", "dy/dx = f1'*f2 + f1*f2'"],
        ["Use product rule", "Answer: d/dx("],
    ),
    (
        "E9 R-form perimeter",
        ["--trig", "5*cos(theta)+12*sin(theta),method=rform"],
        ["R=sqrt(12^2+5^2)=13", "cos(alpha)=12/13", "sin(alpha)=5/13", "13*sin(theta+atan(5/12))"],
        ["Answer: = 5*cos", "ERR:"],
    ),
    (
        "E9 R-form solve",
        ["--trig", "5*cos(theta)+12*sin(theta)=10,theta,0,90,10,method=rform"],
        ["sin(theta+alpha)=10/13", "0 <= theta <= 90", "theta = [27.6649978201]"],
        ["theta = []", "ERR:"],
    ),
    (
        "E10 split trig fraction",
        ["--int", "6*sin(x)/(cos(x)+sin(x))"],
        ["6*sin(x)=3*(sin(x)+cos(x))-3*(cos(x)-sin(x))", "A=3", "B=-3", "3*x - 3*log(abs(cos(x) + sin(x)))"],
        ["No elementary primitive"],
    ),
    (
        "F3 compound-angle solve",
        ["--trig", "sin(x)*cos(pi/5)=1/2-cos(x)*sin(pi/5),x,2*pi,4*pi,6"],
        ["sin(x+pi/5)=1/2", "79*pi/30", "119*pi/30"],
        ["x = []"],
    ),
    (
        "F8 parametric derivative",
        ["--derive", "x=3+2*cos(theta),y=-3+2*sin(theta),theta,x,method=param"],
        ["dx/dt = -2*sin(theta)", "dy/dt = 2*cos(theta)", "dy/dx=(dy/dt)/(dx/dt)", "dy/dx = -cos(theta)/sin(theta)"],
        ["Use dy/dx", "Simplify:", "Answer: d/dx("],
    ),
    (
        "F11 hidden exponential substitution",
        ["--int", "x*(2-3*x)/(exp(3*x)+x^2)"],
        ["integrand = x*(2-3*x)*e^(-3*x)/(1+x^2*e^(-3*x))", "u=x^2*e^(-3*x)+1", "log(x^2*e^(-3*x) + 1)"],
        ["No elementary primitive"],
    ),
    (
        "G5 second derivative route",
        ["--derive", "(x+1)^2*exp(2*x),x,method=second"],
        ["Differentiate once", "d2y/dx2", "2*(2*x^2 + 8*x + 7)*e^(2*x)"],
        ["d2y/dx2 = 0"],
    ),
    (
        "G10 double-angle cubic solve",
        ["--trig", "64*cos(2*x)*cos(x)+32*sin(2*x)*sin(x)=27,x,0,pi,4"],
        ["Use cos(2A)=2cos(A)^2-1", "cos(x)=3/4", "acos(3/4)"],
        ["x = []"],
    ),
    (
        "G12 rationalise negative-power denominator",
        ["--int", "(5-x)/(2-x^(-1))"],
        ["Integrand becomes (- x + 5)*x/(2*x - 1)", "u = 2*x - 1", "-1/16*(2*x - 1)^2", "9/8*ln(abs(2*x - 1))"],
        ["No elementary primitive"],
    ),
]


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [needle for needle in needles if not markers_present(out, [needle])]
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
    print(f"mp2_efg_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

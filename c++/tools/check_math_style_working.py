#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / "c++" / "addin" / "host" / "build" / "casio_host"

CASES = [
    (
        "chain sum",
        ["--derive", "sinh(x^2)+atanh(x/3),x,method=chain"],
        ("d/dx(sinh(x^2)) =", "d/dx(atanh(x/3)) =", "dy/dx ="),
    ),
    (
        "product",
        ["--derive", "(x^2+1)*sin(x),x"],
        ("f1 = x^2 + 1", "f1' = 2*x", "f2 = sin(x)", "f2' = cos(x)", "y' = f1'*f2 + f1*f2'"),
    ),
    (
        "single chain",
        ["--derive", "sin((x+1)^2),x,method=chain"],
        ("u = (x + 1)^2", "du/dx = 2*(x + 1)", "dy/dx = cos(u)*du/dx"),
    ),
    (
        "quotient",
        ["--derive", "x^2/(3*x-1),x"],
        ("u = x^2", "u' = 2*x", "v = 3*x - 1", "v' = 3", "y' = (u'v-u*v')/v^2"),
    ),
    (
        "param generic",
        ["--derive", "x=3+2*cos(theta),y=-3+2*sin(theta),theta,x,method=param"],
        ("dx/dt = -2*sin(theta)", "dy/dt = 2*cos(theta)", "dy/dx=(dy/dt)/(dx/dt)", "dy/dx = (2*cos(theta))/(-2*sin(theta))"),
    ),
    (
        "exam chain final",
        ["--derive", "(2*x+ln(x))^3,x"],
        ("y = (2*x + log(x))^3", "u = 2*x + log(x)", "du/dx = 1/x + 2", "dy/dx = 3*(2*x + log(x))^2*(1/x + 2)"),
    ),
    (
        "looping parts final",
        ["--int", "e^(2*x)*cos(3*x),method=parts"],
        ("I = Integral [e^(2*x)*cos(3*x)] dx", "u = cos(3*x)", "dv = e^(2*x) dx", "J = Integral(e^(2*x)*sin(3*x)) dx", "e^(2*x)*(2*cos(3*x) + 3*sin(3*x))/13 + C"),
    ),
    (
        "rform final",
        ["--trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform"],
        ("R = sqrt(3^2 + 4^2) = 5", "3*cos(x)+4*sin(x)=5*cos(x-alpha)", "x = [arctan(4/3) + arccos(2/5), 2*pi + arctan(4/3) - arccos(2/5)]"),
    ),
    (
        "nested surd rewrite",
        ["--alg", "rewrite(sqrt(12+sqrt(140)))"],
        ("sqrt(12+sqrt(140)) = sqrt(m)+sqrt(n)", "m+n=12", "sqrt(7)+sqrt(5)"),
    ),
]

TEXT_WORDS = (
    "Use ",
    "Start with",
    "Differentiate",
    "Simplify",
    "Apply",
    "Let ",
    "Here ",
    "Then ",
    "Hence ",
    "Therefore",
    "looping integration",
    "Substitute ",
    "Collect ",
    "Inside ",
    "For ",
)

GLOBAL_TEXT_BANS = (
    "Method:",
    "Route:",
    "Answer:",
    "Chk:",
    "pick rule",
    "chain/prod",
    "Std form",
    "Rule/sub/id",
    "Verify",
)


def numbered_lines(out: str):
    return [line for line in out.splitlines() if line[:1].isdigit() and ". " in line]


def run_case(name, args, required):
    proc = subprocess.run([str(HOST), *args], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout
    bad = []
    if proc.returncode:
        bad.append(f"exit {proc.returncode}")
    for word in GLOBAL_TEXT_BANS:
        if word in out:
            bad.append(f"user-facing header: {word}")
    for line in numbered_lines(out):
        for word in TEXT_WORDS:
            if word in line:
                bad.append(f"text in working: {line}")
                break
    for marker in required:
        if marker not in out:
            bad.append(f"missing: {marker}")
    if bad:
        print(f"FAIL {name}: " + "; ".join(bad))
        print(out)
        return False
    print(f"PASS {name}")
    return True


def main():
    ok = all(run_case(*case) for case in CASES)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

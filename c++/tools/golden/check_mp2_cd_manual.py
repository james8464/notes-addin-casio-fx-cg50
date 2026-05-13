#!/usr/bin/env python3
"""Manual MP2 C/D paper checks from rendered model solutions."""

from __future__ import annotations

import subprocess
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "C7 param derivative simplification",
        ["--derive", "x=(3*t-2)/(t-1),y=(t^2-2*t+2)/(t-1),t,x,method=param"],
        ["dx/dt = -1/(t-1)^2", "dy/dt = (t^2-2*t)/(t-1)^2", "dy/dx = 2*t - t^2"],
        ["((2*t - 2)*(t - 1)"],
    ),
    (
        "C8 exponential substitution integral",
        ["--int", "defint(e^(2*x)/(e^x+1),x,log(2),log(8))"],
        ["u=e^x+1", "Integral becomes Integral((u-1)/u) du", "6 - log(3)"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "C10 cot second derivative in y",
        ["--derive", "cot(x),x,method=second"],
        ["dy/dx = -cosec(x)^2", "cosec(x)^2 = 1+cot(x)^2", "d2y/dx2 = 2*y*(y^2+1)"],
        [],
    ),
    (
        "C11 exact R-form rewrite",
        ["--trig", "sqrt(3)*sin(x)+cos(x),method=rform"],
        ["R=sqrt(3+1)=2", "2*cos(x-pi/3)"],
        ["Answer: sqrt(3)*sin(x) + cos(x)"],
    ),
    (
        "D6 interval range",
        ["--alg", "range((2*x+3)^2-4,x,-1,4)"],
        ["-3 <= y <= 117"],
        [],
    ),
    (
        "D11 reciprocal exponential substitution",
        ["--int", "defint((1/x^2+1/x^3)*e^(1/x),x,1/log(3),1/log(2))"],
        ["u=1/x", "Integral becomes Integral((1+u)*e^u) du", "log(27/4)"],
        ["No elementary primitive", "ERR:"],
    ),
    (
        "D12 parametric area integral",
        ["--int", "defint(8*(sec(t)^2-2+cos(t)^2),t,0,pi/4)"],
        ["I = Int(8*(sec(t)^2 + cos(t)^2 - 2)", "Int(sec(u)^2) du = tan(u)", "10 - 3*pi"],
        ["No elementary primitive", "ERR:"],
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
    print(f"mp2_cd_manual ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

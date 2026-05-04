#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, str, list[str], list[str]]] = [
    (
        "int",
        "acos((x-1)/3)",
        ["Use parts", "w=(x - 1)/3", "Answer:"],
        ["Answer: int(", "Classify the integrand", "ERR:", "Unexpected token"],
    ),
    (
        "int",
        "asin((x-1)/3)",
        ["Use parts", "w=(x - 1)/3", "Answer:"],
        ["Answer: int(", "Classify the integrand", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(sqrt(abs(3*x+1)+1))",
        ["abs(3*x + 1) >= 0", "Answer: y >= 1"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "ln(x+y)=x*y,x,method=implicit",
        ["Differentiate both sides", "dy/dx"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "x=t^2+1/t,y=t^2-1/t,t,x,method=param",
        ["dx/dt", "dy/dx"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "alg",
        "x^2-5*x+6,method=factor",
        ["Answer:", "(x - 2)", "(x - 3)"],
        ["Range:", "Answer: x^2 - 5*x + 6", "ERR:"],
    ),
    (
        "alg",
        "domain(csc(2*x+pi/6)^2-cot(2*x+pi/6)^2)",
        ["sin(2*x + pi/6) != 0", "Answer:"],
        ["Answer: all real x", "ERR:"],
    ),
    (
        "alg",
        "range(cot(x^2-pi/4)^2+1)",
        ["Answer: y >= 1"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "int",
        "atan(tan(x^2-pi/4))",
        ["branch"],
        ["Answer: int(", "Classify the integrand", "ERR:"],
    ),
    (
        "int",
        "x*ln(5*x)",
        ["Integration by parts", "Answer: 1/2*x^2*log(5*x) - 1/4*x^2 + C"],
        ["Integral not recognised", "Answer: int(", "ERR:"],
    ),
    (
        "int",
        "tan(3*x)",
        ["Use Integral(tan u)", "Answer: -log(abs(cos(3*x)))/3 + C"],
        ["Integral not recognised", "Answer: int(", "ERR:"],
    ),
]


def compact(text: str) -> str:
    return "".join(ch for ch in text if ch not in " \t\r\n*")


def run(flag: str, expr: str) -> str:
    proc = subprocess.run(
        [str(HOST), "--" + flag, expr],
        cwd=str(REPO),
        text=True,
        capture_output=True,
        timeout=15,
    )
    return proc.stdout + proc.stderr


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    bad = 0
    for flag, expr, needles, forbidden in CASES:
        out = run(flag, expr)
        norm = compact(out)
        ok = all(compact(n) in norm for n in needles)
        ok = ok and not any(f in out for f in forbidden)
        if ok:
            print("OK", flag, expr)
            continue
        bad += 1
        print("FAIL", flag, expr)
        print(out)
    print("done mismatches", bad, "/", len(CASES))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

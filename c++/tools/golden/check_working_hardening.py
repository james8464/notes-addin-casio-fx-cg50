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

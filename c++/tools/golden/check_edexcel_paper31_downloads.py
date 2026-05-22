#!/usr/bin/env python3
"""Paper 31 audit cases from local Edexcel QP/MS PDFs."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "June 2023 Q1 independence",
        ["--alg", "solve(0.3=(0.3+0.05+0.25)*(0.3+p),p)"],
        ["3/10 = 3/5*(p + 3/10)", "p = 1/5"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 binomial pmf",
        ["--stats", "binom(40,1/7,6)"],
        ["X ~ B(40, 0.14285714)", "P(X = 6)", "0.17273044"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 binomial cdf",
        ["--stats", "binomcdf(40,1/7,2)"],
        ["P(X <= 2)", "sum_{x = 0}^2", "0.06158711"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 box-count probability",
        ["--stats", "binom(5,0.06158711,2)"],
        ["X ~ B(5, 0.06158711)", "P(X = 2)", "0.03134451"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 binomial test probability",
        ["--stats", "binomcdf(110,1/7,9)"],
        ["X ~ B(110, 0.14285714)", "P(X <= 9)", "0.03829234"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 grouped estimates",
        ["--alg", "sqrt(4336/184-(390/184)^2),method=numeric"],
        ["sqrt(161431/8464)", "4.36722574188"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 normal upper tail",
        ["--stats", "normalcdf(175.4,6.8,180,inf)"],
        ["Z = (X-175.4)/6.8", "Phi(inf) - Phi(0.67647059)", "0.24937096"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 sample mean normal test",
        ["--stats", "normalcdf(175.4,6.8/sqrt(52),177.2,inf)"],
        ["X ~ N(175.4, (6.8/sqrt(52))^2)", "Z = (X-175.4)/(6.8/sqrt(52))", "0.02814258"],
        ["P(52 < X", "ERR:"],
    ),
    (
        "June 2023 Q5 spinner distribution",
        ["--alg", "solve((2/5)*b+b+(8/5)*b+2*b=1,b)"],
        ["5*b = 1", "b = 1/5"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 histogram probability",
        ["--alg", "48.4/90,method=numeric"],
        ["121/225", "0.537777777778"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 integration by parts",
        ["--int", "defint(x*e^(-x),x,0,n)"],
        ["D: x, 1, 0", "I: -e^(-x), e^(-x)", "F(n) - F(0)", "e^(-n)*(- n - 1) + 1"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 normal model probability",
        ["--stats", "normalcdf(14.9,9.3,10,30)"],
        ["Z = (X-14.9)/9.3", "Phi(1.62365591) - Phi(-0.52688172)", "0.6486375"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 curve model probability",
        ["--alg", "99*((1-4*e^(-3))-(1-2*e^(-1)))/90,method=numeric"],
        ["0.590271669759"],
        ["ERR:"],
    ),
]


def run_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    misses = [n for n in needles if not markers_present(out, [n])]
    bad = [b for b in banned if b in out]
    if proc.returncode:
        misses.append(f"returncode={proc.returncode}")
    if misses or bad:
        return [f"{name}: missing={misses} banned={bad}\n{out}"]
    return []


def main() -> int:
    failures: list[str] = []
    for name, args, needles, banned in CASES:
        failures.extend(run_case(name, args, needles, banned))
    if failures:
        print("\n\n".join(failures))
        return 1
    print(f"edexcel_paper31_downloads ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

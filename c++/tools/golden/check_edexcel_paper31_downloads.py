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
        "June 2022 Q1 binomial pmf",
        ["--stats", "binom(15,0.48,3)"],
        ["X ~ B(15, 0.48)", "P(X = 3)", "0.01966868"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 binomial upper tail",
        ["--stats", "binomial(15,0.48,5,ge)"],
        ["P(X >= 5) = 1 - P(X <= 4)", "0.92013108"],
        ["ERR:"],
    ),
    (
        "June 2022 Q1 normal approximation",
        ["--stats", "normalcdf(120,sqrt(62.4),110.5,inf)"],
        ["X ~ N(120, (sqrt(62.4))^2)", "110.5", "0.88543985"],
        ["ERR:"],
    ),
    (
        "June 2022 Q2 normal sd from percentile",
        ["--alg", "solve((7.902-8)/x=-1.95996398454,x)"],
        ["x != 0", "x ~= 0.050"],
        ["ERR:"],
    ),
    (
        "June 2022 Q2 normal central proportion",
        ["--stats", "normalcdf(8,0.05,7.94,8.09)"],
        ["z1 = (7.94-8)/0.05 = -1.2", "z2 = (8.09-8)/0.05 = 1.8", "0.84900001"],
        ["ERR:"],
    ),
    (
        "June 2022 Q2 normal profit tails",
        ["--stats", "normalcdf(8,0.05,-inf,7.94)"],
        ["P(-inf < X < 7.94)", "0.11506967"],
        ["ERR:"],
    ),
    (
        "June 2022 Q2 expected profit",
        ["--alg", "500*(0.30*0.84900001-0.15*0.11506967+0.20*0.03593032),method=numeric"],
        ["122.31280825"],
        ["ERR:", "-3.766"],
    ),
    (
        "June 2022 Q2 batch acceptance probability",
        ["--stats", "binomcdf(200,0.015,5)"],
        ["X ~ B(200, 0.015)", "P(X <= 5)", "0.9176083"],
        ["ERR:"],
    ),
    (
        "June 2022 Q3 rainfall mean",
        ["--alg", "174.9/31,method=numeric"],
        ["1749/310", "5.64193548387"],
        ["ERR:"],
    ),
    (
        "June 2022 Q3 rainfall sd",
        ["--alg", "sqrt(3523.283/31-(174.9/31)^2),method=numeric"],
        ["sqrt(78631763/961000)", "9.04559861581"],
        ["ERR:"],
    ),
    (
        "June 2022 Q4 lower critical tail",
        ["--stats", "binom(50,0.1,0)"],
        ["X ~ B(50, 0.1)", "P(X = 0)", "0.00515378"],
        ["ERR:"],
    ),
    (
        "June 2022 Q4 upper critical tail",
        ["--stats", "binomial(50,0.1,10,ge)"],
        ["P(X >= 10) = 1 - P(X <= 9)", "0.02453794"],
        ["ERR:"],
    ),
    (
        "June 2022 Q4 actual significance decimal sum",
        ["--alg", "0.005153775207+0.02453793601,method=numeric"],
        ["29691711217/1000000000000", "0.029691711217"],
        ["ERR:", "-3.766"],
    ),
    (
        "June 2022 Q5 Venn cell arithmetic",
        ["--alg", "90*0.4+80*0.05"],
        ["40"],
        ["ERR:"],
    ),
    (
        "June 2022 Q5 conditional probability",
        ["--alg", "(247+481)/(247+481+123+40),method=numeric"],
        ["728/891", "0.817059483726"],
        ["ERR:"],
    ),
    (
        "June 2022 Q6 log model",
        ["--alg", "solve(log10(h)=-0.05*log10(m)+1.92,h)"],
        ["Domain: h > 0", "Domain: m > 0", "h = 10^(48/25)*m^(-1/20)"],
        ["ERR:"],
    ),
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
    (
        "June 2024 Q1 binomial pmf",
        ["--stats", "binom(10,1/6,3)"],
        ["X ~ B(10, 0.16666667)", "P(X = 3)", "0.15504536"],
        ["ERR:"],
    ),
    (
        "June 2024 Q1 binomial lower tail",
        ["--stats", "binomcdf(10,1/6,2)"],
        ["P(X <= 2)", "0.7752268"],
        ["ERR:"],
    ),
    (
        "June 2024 Q1 repeated days upper tail",
        ["--stats", "binomial(60,0.15504536,12,ge)"],
        ["X ~ B(60, 0.15504536)", "P(X >= 12) = 1 - P(X <= 11)", "0.21180918"],
        ["ERR:"],
    ),
    (
        "June 2024 Q1 expected total sixes",
        ["--alg", "600*(1/6)"],
        ["100"],
        ["ERR:"],
    ),
    (
        "June 2024 Q1 normal approximation",
        ["--stats", "normalcdf(100,sqrt(250/3),95.5,inf)"],
        ["X ~ N(100, (sqrt(250/3))^2)", "z1 = (95.5-100)/(sqrt(250/3))", "0.68897615"],
        ["ERR:"],
    ),
    (
        "June 2024 Q3 Perth mean",
        ["--alg", "2801.2/184,method=numeric"],
        ["7003/460", "15.2239130435"],
        ["ERR:"],
    ),
    (
        "June 2024 Q3 Perth standard deviation",
        ["--alg", "sqrt(44695.4/184-(2801.2/184)^2),method=numeric"],
        ["sqrt(2357701/211600)", "3.33800153585"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 lower critical tail",
        ["--stats", "binom(40,0.1,0)"],
        ["X ~ B(40, 0.1)", "P(X = 0)", "0.01478088"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 upper critical tail",
        ["--stats", "binomial(40,0.1,9,ge)"],
        ["P(X >= 9) = 1 - P(X <= 8)", "0.01549531"],
        ["ERR:"],
    ),
    (
        "June 2024 Q4 actual significance level",
        ["--alg", "0.01478088+0.01549531,method=numeric"],
        ["0.03027619"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 high jump upper tail",
        ["--stats", "normalcdf(1.4,0.15,1.6,inf)"],
        ["X ~ N(1.4, 0.15^2)", "z1 = (1.6-1.4)/0.15", "0.09121122"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 running lower tail",
        ["--stats", "normalcdf(330,26,-inf,300)"],
        ["X ~ N(330, 26^2)", "z2 = (300-330)/26", "0.12428162"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 independent event product",
        ["--alg", "0.09121122*0.12428162,method=numeric"],
        ["0.0113358781838"],
        ["ERR:"],
    ),
    (
        "June 2024 Q5 discus sigma",
        ["--alg", "solve(12.7=s*(1.28155156+0.52440051),s,method=linear)"],
        ["127/10 = 180595207/100000000*s", "s = 1270000000/180595207", "s ~= 7.032"],
        ["ERR:", "s = []"],
    ),
    (
        "June 2024 Q5 discus mu",
        ["--alg", "solve(16.3=m-0.52440051*7.032115,m,method=linear)"],
        ["163/10 = m - 73752893847573/20000000000000", "m ~= 19.988"],
        ["ERR:", "m = []"],
    ),
    (
        "June 2024 Q6 Venn independence p",
        ["--alg", "solve((0.08+0.25+0.27)*(0.27+0.05+p)=0.27,p,method=linear)"],
        ["3/5*(p + 8/25) = 27/100", "p = 13/100", "p ~= 0.130"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 q plus r",
        ["--alg", "solve(q+r=1-0.65-0.13,r,method=linear)"],
        ["q + r = 11/50", "r = - q + 11/50"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 greatest conditional probability",
        ["--alg", "0.22/(1-(0.05+0.13+0.27)),method=numeric"],
        ["2/5", "0.4"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 conditional equation r",
        ["--alg", "solve((0.27+0.13)/(0.6+0.13+r)=0.5,r,method=linear)"],
        ["2/5/(r + 73/100) = 1/2", "r = 7/100", "r ~= 0.070"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 q from q plus r",
        ["--alg", "0.22-0.07"],
        ["3/20"],
        ["ERR:"],
    ),
    (
        "June 2024 Q6 final probability",
        ["--alg", "0.25+0.08"],
        ["33/100"],
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

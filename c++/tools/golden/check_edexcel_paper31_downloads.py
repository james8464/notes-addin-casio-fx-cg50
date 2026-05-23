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
        "June 2019 Q1 three green probability",
        ["--alg", "9/10*4/5*2/3"],
        ["12/25"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 at least one each colour",
        ["--alg", "9/10*1/5+9/10*4/5*1/3"],
        ["21/50"],
        ["ERR:"],
    ),
    (
        "June 2019 Q1 conditional red from B",
        ["--alg", "(9/10*1/5)/(1/10+9/10*1/5+9/10*4/5*1/3),method=numeric"],
        ["9/26", "0.346153"],
        ["ERR:"],
    ),
    (
        "June 2019 Q2 standard deviation from Sxx",
        ["--alg", "sqrt(4952.906/184),method=numeric"],
        ["sqrt(2476453/92000)", "5.188252"],
        ["ERR:"],
    ),
    (
        "June 2019 Q2 normal interpercentile range",
        ["--alg", "2*1.28155156*5.19,method=numeric"],
        ["13.302505"],
        ["ERR:"],
    ),
    (
        "June 2019 Q3 log-linear power model",
        ["--alg", "power_model(-1.82,0.89,x,y)"],
        ["log10(y) = -91/50 + 89/100*log10(x)", "y = 10^(-91/50)*x^(89/100)", "0.0151*x^(0.89)"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 cloud cover sample probability",
        ["--alg", "132/184,method=numeric"],
        ["33/46", "0.717391"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 binomial upper tail",
        ["--stats", "binomial(8,0.76,6,ge)"],
        ["X ~ B(8, 0.76)", "P(X >= 6) = 1 - P(X <= 5)", "0.70327766"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 expected number of sevens",
        ["--alg", "184*0.28118774,method=numeric"],
        ["51.73854416"],
        ["ERR:"],
    ),
    (
        "June 2019 Q4 following-day proportion",
        ["--alg", "23/28,method=numeric"],
        ["23/28", "0.821428"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 recover normal sigma",
        ["--alg", "solve((24.63-25)/s=-1.036433389,s)"],
        ["s = 370000000/1036433389", "s ~= 0.357"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 percentile k",
        ["--alg", "solve((k-25)/0.357004=0.253347103,k,method=linear)"],
        ["k ~= 25.090"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 normal approximation",
        ["--stats", "normalcdf(90,sqrt(49.5),-inf,99.5)"],
        ["X ~ N(90, (sqrt(49.5))^2)", "z2 = (99.5-90)/(sqrt(49.5))", "0.9115355"],
        ["ERR:"],
    ),
    (
        "June 2019 Q5 mean test p-value",
        ["--stats", "normalcdf(25,0.16/sqrt(20),-inf,24.94)"],
        ["X ~ N(25, (0.16/sqrt(20))^2)", "z2 = (24.94-25)/(0.16/sqrt(20))", "0.04676626"],
        ["ERR:"],
    ),
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
    (
        "October 2020 Q1 Venn p",
        ["--alg", "0.4-0.07-0.24,method=numeric"],
        ["9/100", "0.09"],
        ["ERR:"],
    ),
    (
        "October 2020 Q1 Venn q by independence",
        ["--alg", "solve((q+0.16+0.24)*0.4=0.24,q)"],
        ["2/5*(q + 2/5) = 6/25", "q = 1/5", "q ~= 0.200"],
        ["ERR:"],
    ),
    (
        "October 2020 Q1 Venn r by conditional",
        ["--alg", "solve(r/(r+0.09)=0.64,r)"],
        ["r/(r + 9/100) = 16/25", "r = 4/25", "r ~= 0.160"],
        ["ERR:"],
    ),
    (
        "October 2020 Q1 Venn s",
        ["--alg", "1-(0.20+0.24+0.16+0.07+0.09+0.16),method=numeric"],
        ["2/25", "0.08"],
        ["ERR:"],
    ),
    (
        "October 2020 Q3 mean from sums",
        ["--alg", "607.5/27,method=numeric"],
        ["45/2", "22.5"],
        ["ERR:"],
    ),
    (
        "October 2020 Q3 standard deviation from sums",
        ["--alg", "sqrt(17623.25/27-(607.5/27)^2),method=numeric"],
        ["sqrt(7909/54)", "12.102188"],
        ["ERR:"],
    ),
    (
        "October 2020 Q3 outlier threshold",
        ["--alg", "22.5+3*12.10218,method=numeric"],
        ["58.80654"],
        ["ERR:"],
    ),
    (
        "October 2020 Q4 distribution k",
        ["--alg", "solve(k*(1/10+1/20+1/30+1/40+1/50)=1,k)"],
        ["137/600*k = 1", "k = 600/137"],
        ["ERR:"],
    ),
    (
        "October 2020 Q4 sum to 80",
        ["--alg", "(600/137/30)*(600/137/50)*2+(600/137/40)^2,method=numeric"],
        ["705/18769", "0.037561937"],
        ["ERR:"],
    ),
    (
        "October 2020 Q4 angle probability",
        ["--alg", "(600/137)/10+(600/137)/20"],
        ["90/137"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 normal upper tail",
        ["--stats", "normalcdf(10,4,15,inf)"],
        ["X ~ N(10, 4^2)", "z1 = (15-10)/4 = 1.25", "0.10564977"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 sample mean p-value",
        ["--stats", "normalcdf(10,4/sqrt(20),11.5,inf)"],
        ["X ~ N(10, (4/sqrt(20))^2)", "z1 = (11.5-10)/(4/sqrt(20))", "0.04676626"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 dentist lower tail",
        ["--stats", "normalcdf(5,3.5,-inf,2)"],
        ["X ~ N(5, 3.5^2)", "z2 = (2-5)/3.5", "0.19568297"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 dentist conditional",
        ["--alg", "0.119119/(0.923436),method=numeric"],
        ["119119/923436", "0.128995404"],
        ["ERR:"],
    ),
    (
        "October 2020 Q5 truncated median",
        ["--alg", "5+3.5*0.253347103,method=numeric"],
        ["5.8867148605"],
        ["ERR:"],
    ),
    (
        "October 2021 Q1 binomial pmf",
        ["--stats", "binom(36,0.08,4)"],
        ["X ~ B(36, 0.08)", "P(X = 4)", "0.16738733"],
        ["ERR:"],
    ),
    (
        "October 2021 Q1 binomial upper tail",
        ["--stats", "binomial(36,0.08,7,ge)"],
        ["P(X >= 7) = 1 - P(X <= 6)", "0.02223384"],
        ["ERR:"],
    ),
    (
        "October 2021 Q1 dance and tango probability",
        ["--alg", "0.4*0.08,method=numeric"],
        ["4/125", "0.032"],
        ["ERR:"],
    ),
    (
        "October 2021 Q1 fewer than three tango",
        ["--stats", "binomcdf(50,0.032,2)"],
        ["P(X <= 2)", "0.78508154"],
        ["ERR:"],
    ),
    (
        "October 2021 Q3 coded mean",
        ["--alg", "214/30+1010,method=numeric"],
        ["15257/15", "1017.13333333"],
        ["ERR:"],
    ),
    (
        "October 2021 Q3 coded standard deviation",
        ["--alg", "sqrt(5912/30-(214/30)^2),method=numeric"],
        ["sqrt(32891/225)", "12.0905840315"],
        ["ERR:"],
    ),
    (
        "October 2021 Q4 exactly one magazine",
        ["--alg", "0.08+0.09+0.36,method=numeric"],
        ["53/100", "0.53"],
        ["ERR:"],
    ),
    (
        "October 2021 Q4 Venn q",
        ["--alg", "solve(0.08+0.05+q=0.25,q,method=linear)"],
        ["q = 3/25", "q ~= 0.120"],
        ["ERR:"],
    ),
    (
        "October 2021 Q4 Venn r",
        ["--alg", "solve(r/(r+0.09+0.05)=5/12,r,method=linear)"],
        ["r = 1/10", "r ~= 0.100"],
        ["ERR:"],
    ),
    (
        "October 2021 Q4 Venn t",
        ["--alg", "1-(0.08+0.05+0.09+0.12+0.10+0.36),method=numeric"],
        ["1/5", "0.2"],
        ["ERR:"],
    ),
    (
        "October 2021 Q4 independence check",
        ["--alg", "compare((0.36+0.12)*0.25,0.12)"],
        ["E1-E2 = 0", "equivalent"],
        ["ERR:", "not equivalent"],
    ),
    (
        "October 2021 Q5 normal percentile k",
        ["--alg", "solve((k-166.5)/6.1=-2.326347874,k,method=linear)"],
        ["k ~= 152.309"],
        ["ERR:"],
    ),
    (
        "October 2021 Q5 normal central probability",
        ["--stats", "normalcdf(166.5,6.1,150,175)"],
        ["X ~ N(166.5, 6.1^2)", "0.91484095"],
        ["ERR:"],
    ),
    (
        "October 2021 Q5 conditional numerator",
        ["--stats", "normalcdf(166.5,6.1,160,175)"],
        ["P(160 < X < 175)", "0.77494883"],
        ["ERR:"],
    ),
    (
        "October 2021 Q5 conditional ratio",
        ["--alg", "0.7749487/0.91484095,method=numeric"],
        ["15498974/18296819", "0.847085714735"],
        ["ERR:"],
    ),
    (
        "October 2021 Q5 sample mean p-value",
        ["--stats", "normalcdf(166.5,7.4/sqrt(50),-inf,164.6)"],
        ["X ~ N(166.5, (7.4/sqrt(50))^2)", "0.03472014"],
        ["ERR:"],
    ),
    (
        "October 2021 Q6 distribution product",
        ["--alg", "compare(2*3*6,36)"],
        ["E1-E2 = 0", "equivalent"],
        ["ERR:", "not equivalent"],
    ),
    (
        "October 2021 Q6 log probabilities sum",
        ["--alg", "log(36,2)+log(36,3)+log(36,6),method=numeric"],
        ["ln(2)/ln(36) + ln(3)/ln(36) + ln(6)/ln(36)", "1"],
        ["ERR:"],
    ),
    (
        "October 2021 Q6 same-distribution probability",
        ["--alg", "(log(36,2))^2+(log(36,3))^2+(log(36,6))^2,method=numeric"],
        ["0.381401143615"],
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

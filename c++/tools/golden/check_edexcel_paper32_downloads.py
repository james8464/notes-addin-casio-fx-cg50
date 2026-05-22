#!/usr/bin/env python3
"""Paper 32 audit cases from local Edexcel QP/MS PDFs."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "June 2023 Q1 speed",
        ["--suvat", "u=0,a=3.2,t=5,target=v"],
        ["v = u + at", "v = 16"],
        ["ERR:"],
    ),
    (
        "June 2023 Q1 distance",
        ["--suvat", "s=,u=0,a=3.2,t=5,target=s"],
        ["s = ut + 1/2at^2", "s = 40"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 normal reaction",
        ["--alg", "solve(R=5*9.8,R,method=linear)"],
        ["R = 49"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 friction equation",
        ["--alg", "solve(28-F=5*1.4,F,method=linear)"],
        ["-F = -21", "F = 21"],
        ["ERR:"],
    ),
    (
        "June 2023 Q2 coefficient of friction",
        ["--alg", "solve(mu*49=21,mu,method=linear)"],
        ["49*mu = 21", "mu = 3/7", "mu ~= 0.429"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 initial speed",
        ["--alg", "sqrt(7^2+(-3)^2)"],
        ["sqrt(58)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 parallel direction time",
        ["--alg", "solve(t^2-3*t+7=2*t^2-3,t,0,inf)"],
        ["Interval: t in [0, inf]", "t = [2]"],
        ["t = [-5", "ERR:"],
    ),
    (
        "June 2023 Q3 acceleration i component",
        ["--derive", "t^2-3*t+7,t"],
        ["d/dt(t^2) = 2*t", "dy/dt = 2*t - 3"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 acceleration j component",
        ["--derive", "2*t^2-3,t"],
        ["d/dt(2*t^2) = 4*t", "dy/dt = 4*t"],
        ["ERR:"],
    ),
    (
        "June 2023 Q3 acceleration perpendicular to i",
        ["--alg", "solve(2*t-3=0,t,method=linear)"],
        ["2*t = 3", "t = 3/2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 velocity i at B",
        ["--suvat", "u=-16,a=2.4,t=5,target=v"],
        ["v = u + at", "v = -4"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 velocity j at B",
        ["--suvat", "u=-3,a=1,t=5,target=v"],
        ["v = u + at", "v = 2"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 speed at B",
        ["--alg", "sqrt((-4)^2+2^2)"],
        ["sqrt(20)"],
        ["ERR:"],
    ),
    (
        "June 2023 Q4 time at C",
        ["--alg", "solve(4=-16*T+1/2*2.4*T^2+44,T,5,inf)"],
        ["Interval: T in [5, inf]", "T = [10]"],
        ["T = [10/3", "ERR:"],
    ),
    (
        "June 2023 Q4 c coordinate",
        ["--alg", "-3*10+1/2*1*10^2-10"],
        ["10"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 horizontal projectile time",
        ["--alg", "solve(28*cos(a)*T=40,T,method=linear)"],
        ["28*cos(a)*T = 40", "T = 10/(7*cos(a))"],
        ["T = 40/(28*cos(a))", "ERR:"],
    ),
    (
        "June 2023 Q5 tan quadratic",
        ["--alg", "solve(20=40*y-10*(1+y^2),y)"],
        ["10*y^2 - 40*y + 30 = 0", "y = [3, 1]"],
        ["ERR:"],
    ),
    (
        "June 2023 Q5 maximum height",
        ["--suvat", "u=28*3/sqrt(10),v=0,a=-9.8,target=s"],
        ["v^2 = u^2 + 2as", "s = (v^2-u^2)/(2a)", "s = 36 m"],
        ["= v^2 = u^2 + 2as", "ERR:"],
    ),
    (
        "June 2023 Q6 moment equation",
        ["--alg", "solve(S*2*a*sin(theta)=M*g*a*cos(theta),S,method=linear)"],
        ["2*S*a*sin(theta) = M*g*a*cos(theta)", "S = M*g*cos(theta)/(2*sin(theta))"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 coefficient of friction",
        ["--alg", "solve((1/2)*M*g*(4/3)=mu*M*g,mu,method=linear)"],
        ["2/3*M*g = mu*M*g", "mu = 2/3"],
        ["ERR:"],
    ),
    (
        "June 2023 Q6 resultant at A",
        ["--alg", "sqrt((2/3)^2+1^2)*M*g"],
        ["sqrt(13/9)*M*g"],
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
    print(f"edexcel_paper32_downloads ok ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

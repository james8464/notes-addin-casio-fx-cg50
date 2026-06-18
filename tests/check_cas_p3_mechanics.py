#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"
ENV = dict(os.environ, CASCAS_HOST_PRODUCTION="1")

CASES = [
    ("suvat(u=2,a=3,t=4)", ["SUVAT", "eq:", "s=32", "u=2", "v=14", "a=3", "t=4"]),
    ("suvat(v=14,a=3,t=4)", ["SUVAT", "eq:", "s=32", "u=2", "v=14", "a=3", "t=4"]),
    ("suvat(s=32,v=14,t=4)", ["SUVAT", "eq:", "s=32", "u=2", "v=14", "a=3", "t=4"]),
    ("suvat(s=32,a=3,t=4)", ["SUVAT", "eq:", "s=32", "u=2", "v=14", "a=3", "t=4"]),
    ("suvat(u=4,v=10,a=2)", ["SUVAT", "eq:", "s=21", "u=4", "v=10", "a=2", "t=3"]),
    ("suvat(u=1/2,a=3/4,t=8)", ["SUVAT", "s=28", "u=1/2", "v=13/2", "a=3/4", "t=8"]),
    ("suvat(u=x,a=2,t=3)", ["SUVAT", "s=3*(x + 3)", "u=x", "v=x + 6", "a=2", "t=3"]),
    ("suvat(u=sin(pi/6),a=1,t=2)", ["SUVAT", "u=1/2", "v=5/2", "s=3"]),
    ("suvat(s=10,u=2,a=3)", ["SUVAT", "s=10", "u=2", "v=[-8, 8]", "a=3", "t=[-10/3, 2]", "phys:t=2, v=8"]),
    ("suvat(s=10,v=8,a=3)", ["SUVAT", "s=10", "u=[2, -2]", "v=8", "a=3", "t=[2, 10/3]", "phys:t=2"]),
    ("suvat(s=10,u=2,v=8)", ["SUVAT", "s=10", "u=2", "v=8", "a=3", "t=2"]),
    ("suvat(s=10,u=2,t=4)", ["SUVAT", "s=10", "u=2", "v=3", "a=1/4", "t=4"]),
    ("suvat(u=2,v=8,t=2)", ["SUVAT", "s=10", "u=2", "v=8", "a=3", "t=2"]),
    ("suvat(displacement=10,initial=2,acceleration=3)", ["SUVAT", "s=10", "u=2", "a=3", "phys:t=2, v=8"]),
    ("suvat(distance=10,start=2,acc=3)", ["SUVAT", "s=10", "u=2", "a=3", "phys:t=2, v=8"]),
    ("suvat(d=10,start=2,acc=3)", ["SUVAT", "s=10", "u=2", "a=3", "phys:t=2, v=8"]),
    ("suvat(u=2,a=0,t=5)", ["SUVAT", "s=10", "u=2", "v=2", "a=0", "t=5"]),
    ("suvat(u=0,a=5,s=10)", ["SUVAT", "s=10", "u=0", "v=[-10, 10]", "a=5", "t=[-2, 2]", "phys:t=2, v=10"]),
    ("suvat(s=0,u=0,v=0)", ["SUVAT", "s=0", "u=0", "v=0", "Unres"], ["\na=0\n", "\nt=0\n"]),
    ("suvat(u=5,v=0,a=-1)", ["SUVAT", "s=25/2", "u=5", "v=0", "a=-1", "t=5"]),
    ("suvat(u=2,a=3)", ["SUVAT", "u=2", "a=3", "Need 3"], ["s=v^2", "t=v/"]),
    ("suvat(10,u=2,a=3)", ["SUVAT", "u=2", "a=3", "Ignored:10", "Need 3"], ["s=v^2", "t=v/"]),
    ("suvat(u=,a=3,t=4)", ["SUVAT", "a=3", "t=4", "Ignored:u", "Need 3"], ["\nu=\n", "Unres"]),
    ("suvat(u=2,a=0,v=2)", ["SUVAT", "u=2", "v=2", "a=0", "Unres"], ["s=2*t", "t=(-u + v)/a"]),
    ("suvat(s=0,u=5,v=-5)", ["SUVAT", "s=0", "u=5", "v=-5", "Unres"], ["a=-10/t", "t=2*s/(u + v)"]),
    ("suvat(u=2,a=3,t=4,v=99)", ["SUVAT", "u=2", "v=99", "a=3", "t=4", "No sol"]),
    ("suvat(s=10,u=2,a=3,t=99)", ["SUVAT", "s=10", "u=2", "a=3", "t=99", "No sol"]),
    ("force(12,3)", ["Newton II", "F=ma", "36 N"]),
    ("force(m=x,a=y)", ["Newton II", "F=ma", "x*y N"]),
    ("force(F=24,m=6)", ["force(m,a)", "F=ma"]),
    ("moment(M=40,F=10)", ["moment(F,d)", "M=F*d"]),
    ("connected(2,3,10)", ["Connected", "a=F/(m1+m2)", "a=10/(2+3)=2", "T=m1*a=2*2=4 N"]),
    ("pulley(2,3)", ["Pulley", "a=(m2-m1)g/(m1+m2)", "a=(3-2)*49/5/(2+3)=49/25", "T=m1(g+a)=2*(49/5+49/25)=588/25 N"]),
    ("lift(5,2)", ["Lift", "R-mg=ma", "R=m(g+a)", "R=5*(49/5+2)=59 N"]),
    ("varacc(6*t,t,4)", ["Variable acceleration", "v=int a dt", "v=3*t^2+C", "v(0)=4", "v=3*t^2+4"]),
    ("varacc(6*t,t,4,2)", ["Variable acceleration", "v=3*t^2+4", "s=int v dt", "s(0)=2", "s=t^3+4*t+2"]),
    ("weight(6)", ["Weight", "W=mg", "294/5 N"]),
    ("moment(10,4)", ["Moment", "M=Fd", "40 N m"]),
    ("work(12,5)", ["Work", "W=Fd", "60 J"]),
    ("power(120,4)", ["Power", "P=W/t", "30 W"]),
    ("power(F=20,v=3)", ["Power", "P=Fv", "60 W"]),
    ("energy(2,5,3)", ["Energy", "KE=1/2mv^2", "KE=25 J", "GPE=294/5 J"]),
    ("energy(m=2,u=3,v=5)", ["Energy", "Delta KE=1/2m(v^2-u^2)", "16 J"]),
    ("impulse(0.5,4,-2)", ["Impulse", "I=m(v-u)", "-3 Ns"]),
    ("impulse(1/2,4,-2,1/10)", ["Impulse", "I=m(v-u)", "-3 Ns", "F=I/t", "-30 N"]),
    ("impulse(F=12,t=1/2)", ["Impulse", "I=Ft", "12*1/2=6 Ns"]),
    ("momentum(3,8)", ["Momentum", "p=mv", "24 kg m/s"]),
    ("momentum(m=x,v=y)", ["Momentum", "p=mv", "x*y kg m/s"]),
    ("friction(2/5,25)", ["Friction", "Fmax=muR", "10 N"]),
    ("friction(2/5,25,12)", ["Friction", "Fmax=muR", "10 N", "F>Fmax: sliding"]),
    ("friction(2/5,25,12,4)", ["Friction", "Fmax=muR", "10 N", "F>Fmax: sliding", "F-Fmax=ma", "1/2 m/s^2"]),
    ("friction(mu=k,R=r,F=p,m=q)", ["Friction", "Fmax=muR", "k*r N", "if sliding", "(p-k*r)/q"]),
    ("resolve(20,30)", ["Resolve", "adjacent = 20*cos(30)", "opposite = 20*sin(30)"]),
]

BANNED = ["Verified", "not checked", "syntax error", "Bad Argument", "unsupported"]


def run(expr: str) -> str:
    return subprocess.check_output([str(RUNNER), expr], cwd=ROOT, text=True, timeout=10, env=ENV)


def main() -> int:
    for case in CASES:
        expr, needles = case[0], case[1]
        absent = case[2] if len(case) > 2 else []
        out = run(expr)
        if len([line for line in out.splitlines() if line.strip()]) < 2:
            raise AssertionError(f"{expr}: too few lines\n{out}")
        for needle in needles:
            if needle not in out:
                raise AssertionError(f"{expr}: missing {needle!r}\n{out}")
        for needle in absent:
            if needle in out:
                raise AssertionError(f"{expr}: should not contain {needle!r}\n{out}")
        for bad in BANNED:
            if bad in out:
                raise AssertionError(f"{expr}: banned {bad!r}\n{out}")
    print(f"OK CAS Paper 3 mechanics cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

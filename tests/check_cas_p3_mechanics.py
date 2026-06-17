#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"

CASES = [
    ("suvat(u=2,a=3,t=4)", ["SUVAT", "v = u + at", "= 14", "s = 32"]),
    ("suvat(v=14,a=3,t=4)", ["SUVAT", "u = v - at", "u = 2", "s = 32"]),
    ("suvat(s=32,v=14,t=4)", ["SUVAT", "u = 2s/t - v", "u = 2", "a = 3"]),
    ("suvat(s=32,a=3,t=4)", ["SUVAT", "u = (s - 1/2 at^2)/t", "u = 2", "v = 14"]),
    ("force(12,3)", ["Newton II", "F=ma", "36 N"]),
    ("force(m=x,a=y)", ["Newton II", "F=ma", "x*y N"]),
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
    ("incline(5,30)", ["Incline", "down =", "R ="]),
    ("projectile(20,30)", ["Projectile", "ux =", "T =", "range", "Hmax"]),
    ("projectile(20,30,2)", ["Projectile", "x = ux*t", "y = uy*t - 1/2*g*t^2"]),
]

BANNED = ["Verified", "not checked", "syntax error", "Bad Argument", "unsupported"]


def run(expr: str) -> str:
    return subprocess.check_output([str(RUNNER), expr], cwd=ROOT, text=True, timeout=10)


def main() -> int:
    for expr, needles in CASES:
        out = run(expr)
        if len([line for line in out.splitlines() if line.strip()]) < 2:
            raise AssertionError(f"{expr}: too few lines\n{out}")
        for needle in needles:
            if needle not in out:
                raise AssertionError(f"{expr}: missing {needle!r}\n{out}")
        for bad in BANNED:
            if bad in out:
                raise AssertionError(f"{expr}: banned {bad!r}\n{out}")
    print(f"OK CAS Paper 3 mechanics cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

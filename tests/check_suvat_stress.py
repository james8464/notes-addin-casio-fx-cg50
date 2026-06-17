#!/usr/bin/env python3
from __future__ import annotations

from itertools import combinations
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"
ENV = dict(os.environ, CASCAS_HOST_PRODUCTION="1")

BANNED = ["syntax error", "Bad Argument", "unsupported", "Traceback"]
SYMBOLIC_FINALS = ["s=2*t", "t=(-u + v)/a", "a=-10/t", "t=2*s/(u + v)"]
EQ_LINES = ["v=u+at", "s=ut+1/2at^2", "v^2=u^2+2as", "s=(u+v)t/2"]

WORLDS = [
    {"s": "32", "u": "2", "v": "14", "a": "3", "t": "4"},
    {"s": "6", "u": "-5", "v": "7", "a": "2", "t": "6"},
    {"s": "2", "u": "3/2", "v": "-1/2", "a": "-1/2", "t": "4"},
    {"s": "12", "u": "sin(pi/6)", "v": "7/2", "a": "cos(pi/3)", "t": "6"},
]

EDGE_CASES = [
    "suvat()",
    "suvat( )",
    "suvat(u=,a=3,t=4)",
    "suvat(u=2,a=,t=4)",
    "suvat(u=2,a=3,t=)",
    "suvat(U=2,A=3,T=4)",
    "suvat( u = 2 , a = 3 , t = 4 )",
    "suvat(d=10,start=2,acc=3)",
    "suvat(x=10,start=2,acc=3)",
    "suvat(u=sqrt(2),a=1,t=3)",
    "suvat(u=pi,a=0,t=2)",
    "suvat(u=p,a=q,t=r)",
    "suvat(s=s0,u=u0,a=a0)",
    "suvat(u=v0,a=a0,t=t0)",
    "suvat(u=var_v,a=acc_a,t=time_t)",
    "suvat(u=speed,a=accel,t=time)",
    "suvat(s=dist,u=speed,a=accel)",
    "suvat(u=2,a=0,v=2)",
    "suvat(s=0,u=0,v=0)",
    "suvat(s=0,u=5,v=-5)",
    "suvat(s=10,u=2,a=3,foo=bar)",
    "suvat(s=10,u=2,a=3,extra)",
]


def run(expr: str) -> str:
    return subprocess.check_output([str(RUNNER), expr], cwd=ROOT, text=True, timeout=10, env=ENV)


def require_common(expr: str, out: str) -> None:
    for needle in ["SUVAT", "eq:"] + EQ_LINES:
        if needle not in out:
            raise AssertionError(f"{expr}: missing {needle!r}\n{out}")
    for bad in BANNED:
        if bad in out:
            raise AssertionError(f"{expr}: banned {bad!r}\n{out}")


def main() -> int:
    cases = 0
    for world in WORLDS:
        for size in (3, 4, 5):
            for keys in combinations("suvat", size):
                expr = "suvat(" + ",".join(f"{key}={world[key]}" for key in keys) + ")"
                out = run(expr)
                require_common(expr, out)
                if "No solution" in out or "Some values unresolved." in out:
                    raise AssertionError(f"{expr}: failed consistent solve\n{out}")
                for key in "suvat":
                    if f"{key}=" not in out:
                        raise AssertionError(f"{expr}: missing {key}= final value\n{out}")
                cases += 1

    for expr in EDGE_CASES:
        out = run(expr)
        require_common(expr, out)
        for bad in SYMBOLIC_FINALS:
            if bad in out:
                raise AssertionError(f"{expr}: symbolic relation shown as final value\n{out}")
        if expr == "suvat(s=0,u=0,v=0)":
            for bad in ("\na=0\n", "\nt=0\n"):
                if bad in out:
                    raise AssertionError(f"{expr}: branch-only value shown as final value\n{out}")
        if expr == "suvat(s=s0,u=u0,a=a0)":
            for needle in ("s=s0", "u=u0", "a=a0", "sqrt(2*a0*s0 + u0^2)"):
                if needle not in out:
                    raise AssertionError(f"{expr}: missing symbolic parameter {needle!r}\n{out}")
            for bad in ("\ns=0\n", "\nu=0\n", "\na=0\n"):
                if bad in out:
                    raise AssertionError(f"{expr}: digit-suffixed parameter collapsed to zero\n{out}")
        if expr == "suvat(u=v0,a=a0,t=t0)":
            for needle in ("u=v0", "a=a0", "t=t0", "v=a0*t0 + v0"):
                if needle not in out:
                    raise AssertionError(f"{expr}: missing symbolic parameter {needle!r}\n{out}")
            for bad in ("\nu=0\n", "\na=0\n", "\nt=0\n"):
                if bad in out:
                    raise AssertionError(f"{expr}: digit-suffixed parameter collapsed to zero\n{out}")
        if expr == "suvat(u=speed,a=accel,t=time)":
            for needle in ("u=speed", "a=accel", "t=time", "v=accel*time + speed"):
                if needle not in out:
                    raise AssertionError(f"{expr}: missing word-symbol parameter {needle!r}\n{out}")
            for bad in ("\nu=0\n", "\na=0\n", "\nt=0\n"):
                if bad in out:
                    raise AssertionError(f"{expr}: word-symbol parameter collapsed to zero\n{out}")
        if expr == "suvat(s=dist,u=speed,a=accel)":
            for needle in ("s=dist", "u=speed", "a=accel", "sqrt(2*accel*dist + speed^2)"):
                if needle not in out:
                    raise AssertionError(f"{expr}: missing word-symbol parameter {needle!r}\n{out}")
            for bad in ("\ns=0\n", "\nu=0\n", "\na=0\n"):
                if bad in out:
                    raise AssertionError(f"{expr}: word-symbol parameter collapsed to zero\n{out}")
        cases += 1

    print(f"OK suvat stress cases={cases}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

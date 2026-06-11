#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
HOST = Path("/tmp/p3_engine_host")


def build():
    subprocess.check_call([
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Itools",
        "tools/p3_engine.cpp", "tools/p3_engine_host.cpp",
        "-o", str(HOST),
    ], cwd=ROOT)


def run(expr):
    return subprocess.check_output([str(HOST), expr], cwd=ROOT, text=True)


def require(expr, needles):
    out = run(expr)
    for needle in needles:
        if needle not in out:
            raise SystemExit(f"{expr!r} missing {needle!r}\n{out}")
    for bad in ["Verified", "not checked", "syntax error", "Bad Argument"]:
        if bad in out:
            raise SystemExit(f"{expr!r} contains banned {bad!r}\n{out}")


def main():
    build()
    require("suvat(u=2,a=3,t=4)", ["SUVAT", "v = u + at", "v = 14"])
    require("suvat(u=2,a=3,s=10)", ["v^2 = u^2 + 2as", "v = 8"])
    require("projectile(20,30)", ["Resolve", "u_x", "range"])
    require("force(12,3)", ["Newton", "36 N"])
    require("weight(5)", ["Weight", "49 N"])
    require("friction(0.4,25)", ["F = mu R", "10 N"])
    require("moment(30,2.5)", ["perpendicular", "75 Nm"])
    require("incline(5,30,0.2)", ["parallel = mg sin", "friction", "net down plane"])
    require("connected(2,3,10)", ["one system", "a = 2", "tension"])
    require("pulley(2,3)", ["same acceleration", "T ="])
    require("impulse(0.5,4,-2)", ["change in momentum", "-3 Ns"])
    require("work(12,5)", ["Work done", "60 J"])
    require("power(120,3)", ["Power", "40 W"])
    require("varacc(2,3,4,5)", ["integrate", "v =", "s ="])
    require("normal(65,50,10)", ["Standardise", "1.5"])
    require("binom(10,0.3,4)", ["X ~ B", "10C4"])
    require("binomcdf(10,0.3,4)", ["P(X<=4)", "0.849"])
    require("hypbinom(20,0.4,4,0.05,-1)", ["H0", "Compare", "context"])
    require("cond(0.18,0.3)", ["P(A|B)", "0.6"])
    require("probor(0.4,0.5,0.2)", ["P(A or B)", "0.7"])
    require("poisson(3,2)", ["Po(lambda)", "P(X=2)"])
    require("regress(2,0.5,10)", ["regression line", "y=7"])
    print("OK p3 engine")


if __name__ == "__main__":
    main()

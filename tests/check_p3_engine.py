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
    require("proj(20,30)", ["Resolve", "u_x", "range"])
    require("projectile speed 20 angle 30", ["Resolve", "u_x", "range"])
    require("force(12,3)", ["Newton", "36 N"])
    require("newton(12,3)", ["Newton", "36 N"])
    require("force mass 12 acceleration 3", ["Newton", "36 N"])
    require("weight(5)", ["Weight", "49 N"])
    require("friction(0.4,25)", ["F = mu R", "10 N"])
    require("moment(30,2.5)", ["perpendicular", "75 Nm"])
    require("incline(5,30,0.2)", ["parallel = mg sin", "friction", "net down plane"])
    require("plane(5,30,0.2)", ["parallel = mg sin", "friction"])
    require("connected(2,3,10)", ["one system", "a = 2", "tension"])
    require("pulley(2,3)", ["same acceleration", "T ="])
    require("impulse(0.5,4,-2)", ["change in momentum", "-3 Ns"])
    require("work(12,5)", ["Work done", "60 J"])
    require("power(120,3)", ["Power", "40 W"])
    require("power when work done is 120 and time is 3", ["Power", "40 W"])
    require("restitution(5,-2,1,3)", ["restitution", "speed of separation"])
    require("collision restitution u1 5 u2 -2 v1 1 v2 3", ["restitution", "speed of separation"])
    require("vector(3,4)", ["sqrt", "|R|", "5"])
    require("varacc(2,3,4,5)", ["integrate", "v =", "s ="])
    require("normal(65,50,10)", ["Standardise", "1.5"])
    require("zscore(65,50,10)", ["Standardise", "1.5"])
    require("standardise 65 mean 50 sd 10", ["Standardise", "1.5"])
    require("binom(10,0.3,4)", ["X ~ B", "10C4"])
    require("binomial(10,0.3,4)", ["X ~ B", "10C4"])
    require("binomcdf(10,0.3,4)", ["P(X<=4)", "0.849"])
    require("critbinom(20,0.4,0.05,-1)", ["critical region", "alpha"])
    require("binomial critical region n 20 p 0.4 alpha 0.05 lower", ["critical region", "alpha"])
    require("hypbinom(20,0.4,4,0.05,-1)", ["H0", "Compare", "context"])
    require("normalprob(40,60,50,10)", ["standardise", "NormalCD"])
    require("normal distribution between 40 and 60 mean 50 sd 10", ["standardise", "NormalCD"])
    require("cond(0.18,0.3)", ["P(A|B)", "0.6"])
    require("probor(0.4,0.5,0.2)", ["P(A or B)", "0.7"])
    require("poisson(3,2)", ["Po(lambda)", "P(X=2)"])
    require("regress(2,0.5,10)", ["regression line", "y=7"])
    print("OK p3 engine")


if __name__ == "__main__":
    main()

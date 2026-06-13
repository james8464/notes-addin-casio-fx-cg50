#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
HOST = Path("/tmp/p3_engine_host")


def build():
    subprocess.check_call([
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Wno-unused-function", "-Itools",
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
    require("projectile(20,30)", ["Resolve", "u_x", "range"])
    require("projectileh(20,30,5)", ["vertical motion", "range = u_x t"])
    require("projectileat(20,30,10,0)", ["horizontal motion", "height at x=10"])
    require("projectileangle(20,30,5,0)", ["trajectory equation", "theta ="])
    require("force(12,3)", ["Newton", "36 N"])
    require("weight(6)", ["Weight", "58.8 N"])
    require("friction(0.4,25)", ["F = mu R", "10 N"])
    require("inclineacc(5,30,0.2)", ["Resolve along and perpendicular", "a = F/m"])
    require("connected(2,3,10)", ["Connected particles", "a = 2", "tension"])
    require("pulley(3,5)", ["same acceleration", "T ="])
    require("beam(10,30,4,20)", ["moments about the left support", "R_B = 22", "R_A = 28"])
    require("ladder(5,100,60)", ["smooth wall, rough floor", "limiting friction"])
    require("varacct(6,-4,0,3,5)", ["Variable acceleration", "v = u + integral"])
    require("varaccx(4,3,2,0,5)", ["a = v dv/dx", "v ="])
    require("impulse(0.5,4,-2)", ["change in momentum", "-3 Ns"])
    require("momentum(2,5,3,-1,1)", ["conservation of linear momentum", "v2 ="])
    require("work(12,5)", ["Work done", "60 J"])
    require("power(120,3)", ["Power", "40 W"])
    require("energy(2,5,3)", ["KE = 1/2", "GPE"])
    require("restitution(5,-2,1,3)", ["restitution", "speed of separation"])
    require("vectorkin(0,0,3,4,0,-9.8,2)", ["component by component", "position"])
    require("resolve(20,30)", ["Resolve", "horizontal"])
    require("normalprob(40,60,50,10)", ["standardise", "NormalCD"])
    require("normalprobvar(40,60,50,100)", ["Convert variance", "NormalCD"])
    require("normaltail(65,50,10,1)", ["upper-tail", "NormalCD"])
    require("normalcond(34,26,30,4,-1)", ["conditional probability", "P(A|B)"])
    require("invnormal(0.95,50,10)", ["inverse normal", "InvNorm"])
    require("normalcrit(0.95,50,10)", ["inverse normal", "InvNorm"])
    require("samplemean(50,10,25,45,55)", ["For a sample mean", "NormalCD"])
    require("samplemeantail(50,10,25,55,1)", ["For a sample mean", "P(Xbar>=55)"])
    require("binom(10,0.4,3)", ["Let X ~ B", "P(X=3)"])
    require("binomcdf(10,0.4,3)", ["P(X<=3)", "="])
    require("binomtail(10,0.4,3,1)", ["P(X>=3)", "sum"])
    require("binomstats(20,0.35)", ["E(X)=np", "Var(X)"])
    require("binomnorm(100,0.4,35,45)", ["normal approximation", "continuity correction"])
    require("binomcrit(20,0.4,0.05,-1)", ["Lower tail", "critical region"])
    require("hypbinom(20,0.4,4,0.05,-1)", ["H0", "tail probability", "Compare"])
    require("poisson(3,2)", ["Po(lambda)", "P(X=2)"])
    require("poissontail(3,2,-1)", ["X~Po(3)", "P(X<=2)"])
    require("critpoisson(3,0.05,1)", ["Po", "critical"])
    require("hyppoisson(3,2,0.05,-1)", ["H0", "tail probability"])
    require("cond(0.2,0.5)", ["P(A|B)", "0.4"])
    require("probor(0.4,0.3,0.1)", ["P(A or B)", "0.6"])
    require("bayes(0.8,0.3,0.5)", ["Bayes", "0.48"])
    require("independent(0.4,0.5,0.2)", ["independence", "are independent"])
    require("regresscalc(5,20,30,100,220,140)", ["least-squares", "regression line"])
    require("pmcc(5,20,30,100,220,140)", ["product moment", "r ="])
    require("spearman(12,8)", ["Spearman", "r_s"])
    require("discrete(1,0.2,2,0.5,3,0.3)", ["E(X)", "Var(X)"])
    require("groupmean(5,12,15,30,25,18)", ["grouped data", "mean"])
    require("groupmedian(20,14,18,10,60)", ["interpolation", "median"])
    require("histdensity(24,6)", ["frequency density", "4"])
    require("code(25,10,5,2)", ["coded", "mean Y"])
    require("constant acceleration u=2 a=3 t=4", ["Supported:"])
    print("OK p3 command engine")


if __name__ == "__main__":
    main()

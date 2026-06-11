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
    require("normal(65,50,10)", ["Standardise", "1.5"])
    require("binom(10,0.3,4)", ["X ~ B", "10C4"])
    require("binomcdf(10,0.3,4)", ["P(X<=4)", "0.849"])
    require("hypbinom(20,0.4,4,0.05,-1)", ["H0", "Compare", "context"])
    print("OK p3 engine")


if __name__ == "__main__":
    main()

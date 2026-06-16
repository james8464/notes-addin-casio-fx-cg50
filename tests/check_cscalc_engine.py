#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
HOST = Path("/tmp/cscalc_engine_host")


def build():
    subprocess.check_call([
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Wno-unused-function", "-Itools",
        "tools/cscalc_engine.cpp", "tools/cscalc_engine_host.cpp",
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
    require("help()", ["CSCalc Boolean help.", "bool_simplify nandform", "norform boolprove"])
    require("bool_simplify(A+A)", ["1. A + A", "2. A    (Tautology)", "Result: A"])
    require("bool_simplify(A+A.B)", ["1. A + A.B", "2. A    (Absorption)", "Result: A"])
    require("bool_simplify(A.B+A,.C+B.C)", ["1. A.B + A,.C + B.C", "2. A.B + A,.C    (Consensus theorem)", "Result: A.B + A,.C"])
    require("bool_simplify(A.(B+C))", ["2. B.A + C.A    (Expansion of brackets)", "Result: B.A + C.A"])
    require("bool_simplify(((B,.A),.B,),+A.B)", ["De Morgan's law", "Double complement", "Result: B,.A + B"])
    require("nandform(A.B)", ["1. A.B", "2. NAND form: A, + B,"])
    require("norform(A+B)", ["1. A + B", "2. NOR form: A,.B,"])
    require("boolprove(A.(B+C),A.B+A.C)", ["3. Simplify LHS:", "7. Simplify RHS:", "11. Same output rows"])
    require("boolprove(A,B)", ["3. LHS final: A", "5. RHS final: B", "8. RHS final: B"])
    require("convert(45,10,2)", ["Supported:", "bool_simplify nandform norform", "boolprove"])
    require("truth(A+B)", ["Supported:", "bool_simplify nandform norform"])
    print("OK cscalc Boolean engine")


if __name__ == "__main__":
    main()

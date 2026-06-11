#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
HOST = Path("/tmp/cscalc_engine_host")


def build():
    subprocess.check_call([
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Itools",
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
    banned = ["Verified", "not checked", "syntax error", "Bad Argument"]
    for bad in banned:
        if bad in out:
            raise SystemExit(f"{expr!r} contains banned {bad!r}\n{out}")


def main():
    build()
    require("bin(45)", ["Repeated division", "45_10"])
    require("den(101101,2)", ["place value", "45_10"])
    require("twos(-5,8)", ["8-bit", "11111011"])
    require("twosdec(11111011)", ["subtract 2^8", "-5"])
    require("fixed(101.101)", ["place values", "5.625"])
    require("floatdec(0101100,11101)", ["mantissa", "exponent", "0.0859375"])
    require("normal(0101100)", ["Normalised", "is normalised"])
    require("image(1920,1080,24)", ["width * height", "49766400 bits"])
    require("sound(44100,60,16,2)", ["sample rate", "84672000 bits"])
    require("compress(1200,300)", ["ratio = 4", "saving = 75"])
    require("chars(120,8)", ["characters", "960 bits"])
    require("bool(A+B')", ["truth table", "minterms", "simplified"])
    print("OK cscalc engine")


if __name__ == "__main__":
    main()

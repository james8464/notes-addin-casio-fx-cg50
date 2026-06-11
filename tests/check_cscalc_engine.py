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
    require("binary(45)", ["Repeated division", "45_10"])
    require("convert 45 to binary", ["Repeated division", "45_10"])
    require("den(101101,2)", ["place value", "45_10"])
    require("convert 101101 binary to denary", ["place value", "45_10"])
    require("101101", ["Bare 0/1 input", "45_10"])
    require("twos(-5,8)", ["8-bit", "11111011"])
    require("twosdec(11111011)", ["subtract 2^8", "-5"])
    require("unsignedrange(8)", ["unsigned range", "0 to 255"])
    require("8-bit two's complement range", ["two's complement range", "-128 to 127"])
    require("decode 11111011 as two's complement", ["subtract 2^8", "-5"])
    require("twosadd(01111111,00000001)", ["127+1=128", "overflow"])
    require("twossub(00000101,00000011)", ["Subtraction", "5-3=2"])
    require("binadd(1011,0110,4)", ["carrying", "0001", "overflow"])
    require("shift(00101100,left,2)", ["left shift", "10110000"])
    require("fixed(101.101)", ["place values", "5.625"])
    require("fixed point 101.101", ["place values", "5.625"])
    require("fixedtc(111.01)", ["two's complement", "-0.75"])
    require("fixed two's complement 111.01", ["two's complement", "-0.75"])
    require("floatdec(0101100,11101)", ["mantissa", "exponent", "0.0859375"])
    require("floatenc(12.75,8,4)", ["Normalise", "mantissa", "exponent"])
    require("floatdecode(0101100,11101)", ["mantissa", "exponent"])
    require("mantissa 0101100 exponent 11101", ["mantissa", "exponent", "0.0859375"])
    require("floatrange(8,4)", ["exponent range", "mantissa step", "largest positive"])
    require("floating point range with 8 bit mantissa and 4 bit exponent", ["exponent range", "largest positive"])
    require("normal(0101100)", ["Normalised", "is normalised"])
    require("image(1920,1080,24)", ["width * height", "49766400 bits", "MB"])
    require("bitmap image 1920 x 1080 with 24 bit colour", ["width * height", "49766400 bits"])
    require("bitmap image width=1920 height=1080 depth=24", ["width * height", "49766400 bits"])
    require("imagecolors(100,50,16)", ["ceil(log2(16)) = 4", "20000 bits"])
    require("bitmap image 100 x 50 with 16 colours", ["ceil(log2(16)) = 4", "20000 bits"])
    require("bitmap image width=100 height=50 colours=16", ["ceil(log2(16)) = 4", "20000 bits"])
    require("sound(44100,60,16,2)", ["sample rate", "84672000 bits", "MB"])
    require("transfer time for 8000 bits at 2000 bit/s", ["Transfer time", "4 s"])
    require("compress(1200,300)", ["ratio = 4", "saving = 75"])
    require("rle(12,8,4)", ["Run-length bits", "144 bits"])
    require("run length encoding 12 runs 8 symbol bits 4 count bits", ["Run-length bits", "144 bits"])
    require("records(120,32)", ["records * bytes", "3840 bytes"])
    require("database file 120 records 32 bytes per record", ["records * bytes", "3840 bytes"])
    require("chars(120,8)", ["characters", "960 bits", "MB"])
    require("bool(A+B')", ["truth table", "minterms", "simplified"])
    require("boolean(AB+C)", ["truth table", "simplified"])
    require("bool(A and not B)", ["truth table", "simplified = AB'"])
    require("A and not B", ["truth table", "simplified = AB'"])
    require("simplify A and not B", ["truth table", "simplified = AB'"])
    require("bool(A nand B)", ["truth table", "simplified = A'+B'"])
    print("OK cscalc engine")


if __name__ == "__main__":
    main()

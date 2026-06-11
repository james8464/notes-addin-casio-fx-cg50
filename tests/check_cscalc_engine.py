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
    require("parity(1011001,even)", ["Even parity", "has 4 one-bits", "parity bit = 0"])
    require("odd parity bit for 1011001", ["Odd parity", "has 4 one-bits", "parity bit = 1"])
    require("xorbits(10101100,11001010)", ["XOR gives 1", "result = 01100110"])
    require("xor 10101100 11001010", ["XOR gives 1", "01100110"])
    require("andbits(10101100,11001010)", ["AND gives 1", "result = 10001000"])
    require("orbits(10101100,11001010)", ["OR gives 1", "result = 11101110"])
    require("notbits(10101100)", ["NOT flips", "01010011"])
    require("invert bits 10101100", ["NOT flips", "01010011"])
    require("hamming(101101,111001)", ["Hamming distance", "different positions", "distance = 2"])
    require("hamming distance between 101101 and 111001", ["Hamming distance", "distance = 2"])
    require("checksum(8,00110011,01010101)", ["Checksum", "decimal sum = 136", "10001000"])
    require("checksum for 00110011 01010101", ["Checksum", "10001000"])
    require("checkdigit(12345,11,6,5,4,3,2)", ["weighted modulo", "sum = 50", "remainder", "mod 11 = 5"])
    require("check digit for 12345 weights 6 5 4 3 2 mod 11", ["weighted modulo", "sum = 50", "mod 11 = 5"])
    require("fixed(101.101)", ["place values", "5.625"])
    require("fixed point 101.101", ["place values", "5.625"])
    require("fixedtc(111.01)", ["two's complement", "-0.75"])
    require("fixed two's complement 111.01", ["two's complement", "-0.75"])
    require("fixedenc(5.625,3,3)", ["scaling by 2^fraction bits", "fixed point = 101.101"])
    require("fixed point encode 5.625 with 3 whole bits and 3 fractional bits", ["scaled integer", "fixed point = 101.101"])
    require("fixedtcenc(-0.75,3,2)", ["two's-complement scaled integer", "fixed point = 111.01"])
    require("signed fixed point encode -0.75 with 3 whole bits and 2 fractional bits", ["two's-complement scaled integer", "fixed point = 111.01"])
    require("floatdec(0101100,11101)", ["mantissa", "exponent", "0.0859375"])
    require("floatenc(12.75,8,4)", ["Normalise", "mantissa", "exponent"])
    require("floatdecode(0101100,11101)", ["mantissa", "exponent"])
    require("mantissa 0101100 exponent 11101", ["mantissa", "exponent", "0.0859375"])
    require("floatnorm(00011010,0110)", ["first two mantissa bits differ", "shift mantissa left 2", "mantissa = 01101000", "exponent = 0100"])
    require("normalise mantissa 00011010 exponent 0110", ["shift mantissa left 2", "mantissa = 01101000"])
    require("floating point encode -608 with 7 bit mantissa and 5 bit exponent", ["Normalise", "mantissa (7 bits)", "exponent (5 bits)"])
    require("floatprecision(8,4)", ["last mantissa bit", "2^(4-(8-1))", "0.125"])
    require("floating point precision mantissa 8 exponent 4", ["last mantissa bit", "0.125"])
    require("floatnearest(12.7,8,4)", ["closest representable", "step at this exponent", "nearest multiple = 12.75", "mantissa (8 bits)", "exponent (4 bits)"])
    require("closest representable floating point value 12.7 with 8 bit mantissa and 4 bit exponent", ["closest representable", "nearest multiple = 12.75"])
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
    require("transfermb(10,2)", ["10 MB = 80 Mbit", "time = 80/2 = 40 s"])
    require("download time for 10 megabytes at 2 megabits per second", ["Convert megabytes", "10 MB = 80 Mbit", "40 s"])
    require("compress(1200,300)", ["ratio = 4", "saving = 75"])
    require("rle(12,8,4)", ["Run-length bits", "144 bits"])
    require("run length encoding 12 runs 8 symbol bits 4 count bits", ["Run-length bits", "144 bits"])
    require("rletext(aaabcccc,8,4)", ["consecutive repeated", "runs: ax3,bx1,cx4", "original bits = 8*8", "encoded bits = 3*(8+4)"])
    require("run length encode aaabcccc with 8 symbol bits and 4 count bits", ["consecutive repeated", "runs: ax3,bx1,cx4", "encoded bits = 3*(8+4)"])
    require("records(120,32)", ["records * bytes", "3840 bytes"])
    require("database file 120 records 32 bytes per record", ["records * bytes", "3840 bytes"])
    require("hashmod(10,27,18,29,37)", ["Hash address = key mod table size", "27 mod 10 = 7", "37 mod 10 = 7, collision"])
    require("hash table size 10 keys 27 18 29 37", ["Hash address = key mod table size", "29 mod 10 = 9", "collision"])
    require("chars(120,8)", ["characters", "960 bits", "MB"])
    require("charset(120,128)", ["ceil(log2(128)) = 7", "text bits = 120*7"])
    require("store 120 characters from character set of 128 symbols", ["ceil(log2(128)) = 7", "840 bits"])
    require("bool(A+B')", ["truth table", "minterms", "simplified"])
    require("boolean(AB+C)", ["truth table", "simplified"])
    require("bool(A and not B)", ["truth table", "simplified = AB'"])
    require("A and not B", ["truth table", "simplified = AB'"])
    require("simplify A and not B", ["truth table", "simplified = AB'"])
    require("simplify boolean expression A and not B", ["truth table", "simplified = AB'"])
    require("bool(A+A)", ["Boolean algebra", "Idempotent law", "simplified = A"])
    require("simplify A + A B", ["Boolean algebra", "Absorption law", "simplified = A"])
    require("bool(A+A')", ["Boolean algebra", "Complement law", "simplified = 1"])
    require("bool(not(A and B))", ["Boolean algebra", "De Morgan", "simplified"])
    require("bool(A B + A C)", ["Boolean algebra", "Distributive law", "a&(b+c)", "simplified"])
    require("bool((A+B)(A+C))", ["Boolean algebra", "Distributive law", "a+b&c", "simplified"])
    require("bool(A xor B)", ["Boolean algebra", "XOR identity", "a'&b+a&b'", "simplified"])
    require("bool(A^B)", ["Boolean algebra", "XOR identity", "a'&b+a&b'", "simplified"])
    require("bool(A nand B)", ["truth table", "simplified = A'+B'"])
    require("nandform(A+B)", ["Use NAND", "NAND form", "A NAND A"])
    require("nand form A or B", ["Use NAND", "NAND form", "A NAND A"])
    require("norform(A*B)", ["Use NOR", "NOR form", "A NOR A"])
    require("nor form A and B", ["Use NOR", "NOR form", "A NOR A"])
    require("boolprove(A+B,B+A)", ["truth tables", "LHS output-1 rows", "Same output rows"])
    require("prove A+B = B+A", ["truth tables", "LHS output-1 rows", "Same output rows"])
    require("show that A and not B = A*B'", ["truth tables", "Same output rows"])
    print("OK cscalc engine")


if __name__ == "__main__":
    main()

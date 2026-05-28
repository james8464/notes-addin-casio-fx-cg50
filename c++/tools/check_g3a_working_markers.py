#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
from pathlib import Path


MARKERS = [
    b"LHS - RHS = 0",
    b"Valid:",
    b"y'(...) = ...",
    b"dy/dx = (dy/d",
    b"Area = Int",
    b"v = u+at",
    b"(a+b)^n = sum",
    b"Coeff:",
    b"I = Int[",
    b"P(X<=r)",
    b"coeff(LHS) = coeff(RHS)",
    b"constants =",
    b"ax^2+bx+c = a(",
    b"IF=e^Int(Pdx)",
    b"I = Int[f(x)] dx",
    b"I = u*v - Int(v du)",
    b"PF: A/(x-a)",
    b"num = A*q",
    b"[cos(x+h)-cos(x)]/h",
    b"1 + cos(2x) = 2cos(x)^2",
    b"alpha = atan(5/12)",
    b"d/dx(sin(x)) = cos(x)",
    b"d/dx(-cos(x)) = sin(x)",
    b"d/dx(ln(abs(x))) = 1/x",
    b"d/dx(tan(x)) = sec(x)^2",
    b"d/dx(sec(x)) = sec(x)tan(x)",
    b"tan(x)^2 = sec(x)^2 - 1",
    b"d/dx(-cot(x)) = cosec(x)^2",
    b"d/dx(-cosec(x)) = cosec(x)cot(x)",
]

FORBIDDEN = [
    b"Ans: ",
    b"I=uv-Int(vdu)",
    b"dy/dx=(dy/d",
    b"Area=Int",
    b"use tabvar",
    b"comb(",
    b"normald(",
    b"normald_cdf(",
    b"normald_icdf(",
    b"randnormald(",
    b"mean(",
    b"median(",
    b"stdev(",
    b"stddev(",
    b"correlation(",
    b"covariance(",
    b"linear_regression(",
    b"plot(",
    b"plotarea(",
    b"plotfunc(",
    b"plotcontour(",
    b"plotfield(",
    b"plotlist(",
    b"plotode(",
    b"paramplot(",
    b"plotparam(",
    b"plotpolar(",
    b"plotseq(",
    b"areaplot(",
    b"disque(",
    b"rationalise(",
    b"rationalize(",
    b"tabular(",
    b"weierstrass(",
    b"symmetry(",
    b"mean_value(",
    b"volume_x(",
    b"volume_y(",
    b"area_between(",
    b"param_area(",
    b"param_area_y(",
    b"param_volume_x(",
    b"param_volume_y(",
    b"ztest(",
    b"spark(",
    b"sinh(",
    b"cosh(",
    b"tanh(",
    b"sech(",
    b"csch(",
    b"coth(",
    b"asinh(",
    b"acosh(",
    b"atanh(",
]

MAGIC = b"CCP1"


def read_pack(path: Path) -> bytes:
    data = path.read_bytes()
    if len(data) < 6 or data[:4] != MAGIC:
        raise ValueError("bad PAK magic")
    count = struct.unpack_from(">H", data, 4)[0]
    pos = 6
    body = bytearray()
    for _ in range(count):
        if pos + 4 > len(data):
            raise ValueError("truncated PAK header")
        nlen, blen = struct.unpack_from(">HH", data, pos)
        pos += 4
        if pos + nlen + blen > len(data):
            raise ValueError("truncated PAK body")
        body += data[pos : pos + nlen]
        pos += nlen
        body += b"\n"
        body += data[pos : pos + blen]
        pos += blen
        body += b"\n"
    if pos != len(data):
        raise ValueError("trailing PAK bytes")
    return bytes(body)


def main() -> int:
    ap = argparse.ArgumentParser(description="Verify source-built CasioCAS keeps working text in external pack.")
    ap.add_argument("g3a", type=Path)
    ap.add_argument("--pack", type=Path, default=Path("calculator_files/CASIOCAS.PAK"))
    args = ap.parse_args()
    data = args.g3a.read_bytes()
    embedded = [m.decode("ascii") for m in MARKERS if m in data]
    if embedded:
        print("FAIL working text embedded in g3a: " + ", ".join(embedded))
        return 1
    forbidden = [m.decode("ascii") for m in FORBIDDEN if m in data]
    if forbidden:
        print("FAIL g3a forbidden answer prefixes present: " + ", ".join(forbidden))
        return 1
    try:
        pack = read_pack(args.pack)
    except Exception as exc:
        print("FAIL external working pack unreadable: " + str(exc))
        return 1
    missing = [m.decode("ascii") for m in MARKERS if m not in pack]
    if missing:
        print("FAIL external working markers missing: " + ", ".join(missing))
        return 1
    print("OK external working markers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

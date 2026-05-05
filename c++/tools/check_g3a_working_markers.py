#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
from pathlib import Path


MARKERS = [
    b"2. = ",
    b"Std form.",
    b"Valid:",
    b"Pick rule.",
    b"Collect y' terms",
    b"dy/dx=(dy/d",
    b"Area=Int",
    b"Pick SUVAT",
    b"Binom theorem",
    b"Coeff:",
    b"I = Int[",
    b"Prob/crit",
    b"1. ",
    b"Eq coeffs",
    b"Solve consts",
    b"ax2+bx+c ->",
    b"Int sides.",
    b"Classify.",
    b"IBP:",
    b"Use PF",
    b"Clear den; eq",
    b"FP quotient",
    b"1+cos(2x)=2cos(x)^2",
    b"alpha=atan(5/12)",
]

FORBIDDEN = [
    b"Ans: ",
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

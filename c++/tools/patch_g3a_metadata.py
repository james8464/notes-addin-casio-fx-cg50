#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


NAME_BASIC = 0x40
NAME_BASIC_LEN = 0x1C
NAME_INTERNAL = 0x60
NAME_INTERNAL_LEN = 0x0B
LC_NAMES = 0x6B
LC_NAME_LEN = 0x18
LC_COUNT = 8
FILENAME = 0xEBC
FILENAME_LEN = 0x144
CHECKSUM = 0x20


def put_ascii(buf: bytearray, offset: int, size: int, text: str) -> None:
    raw = text.encode("ascii")
    if len(raw) >= size:
        raise ValueError(f"{text!r} too long for {size}-byte G3A field")
    buf[offset : offset + size] = b"\0" * size
    buf[offset : offset + len(raw)] = raw


def put_fixed(buf: bytearray, old: bytes, new: str) -> int:
    raw = new.encode("ascii")
    if len(raw) > len(old):
        raise ValueError(f"{new!r} longer than target {old!r}")
    count = 0
    start = 0
    while True:
        idx = buf.find(old, start)
        if idx < 0:
            return count
        buf[idx : idx + len(old)] = raw + b" " * (len(old) - len(raw))
        count += 1
        start = idx + len(old)


def canonical_internal_name(internal_name: str) -> str:
    name = "@" + internal_name.upper().lstrip("@")
    if len(name) > 8:
        raise ValueError(f"{name!r} too long for Casio menu-safe internal name")
    if any(ch != "@" and not ("A" <= ch <= "Z") for ch in name):
        raise ValueError(f"{name!r} must use only @ and A-Z")
    return name


def checksum(buf: bytearray) -> int:
    tmp = bytearray(buf)
    tmp[CHECKSUM : CHECKSUM + 4] = b"\0\0\0\0"
    tmp[-4:] = b"\0\0\0\0"
    return sum(tmp) & 0xFFFFFFFF


def patch(path: Path, app_name: str, internal_name: str, filename: str) -> None:
    buf = bytearray(path.read_bytes())
    if len(buf) < 0x7004:
        raise ValueError(f"{path} is too small to be a valid .g3a")
    internal = canonical_internal_name(internal_name)

    put_ascii(buf, NAME_BASIC, NAME_BASIC_LEN, app_name)
    put_ascii(buf, NAME_INTERNAL, NAME_INTERNAL_LEN, internal)
    for i in range(LC_COUNT):
        put_ascii(buf, LC_NAMES + i * LC_NAME_LEN, LC_NAME_LEN, app_name)
    put_ascii(buf, FILENAME, FILENAME_LEN, filename)

    # Cosmetic visible strings in the copied upstream binary.
    put_fixed(buf, b"Khicas Help", app_name)
    put_fixed(buf, b"Khicasen", app_name)
    put_fixed(buf, b"@KHICASEN", internal)
    put_fixed(buf, b"khicasen.g3a", filename)
    put_fixed(buf, b"\0 Xcas \0", "\0 UK   \0")

    cksum = checksum(buf)
    raw = cksum.to_bytes(4, "big")
    buf[CHECKSUM : CHECKSUM + 4] = raw
    buf[-4:] = raw
    path.write_bytes(buf)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("g3a", type=Path)
    ap.add_argument("--name", default="CasioCAS")
    ap.add_argument("--internal", default="CASCAS")
    ap.add_argument("--filename", default="CasioCAS.g3a")
    args = ap.parse_args()
    patch(args.g3a, args.name, args.internal, args.filename)
    print(f"patched metadata: {args.g3a}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


FIELDS = {
    "basic": (0x40, 0x1C),
    "internal": (0x60, 0x0B),
    "filename": (0xEBC, 0x144),
}


def field(buf: bytes, name: str) -> str:
    off, size = FIELDS[name]
    return buf[off : off + size].split(b"\0", 1)[0].decode("ascii", "replace")


def checksum(buf: bytes) -> int:
    tmp = bytearray(buf)
    tmp[0x20:0x24] = b"\0\0\0\0"
    tmp[-4:] = b"\0\0\0\0"
    return sum(tmp) & 0xFFFFFFFF


def check_internal_name(name: str) -> str | None:
    if len(name) > 8:
        return "internal name must be <=8 chars including @"
    if not name.startswith("@"):
        return "internal name must start with @"
    if any(ch != "@" and not ("A" <= ch <= "Z") for ch in name):
        return "internal name must use only @ and A-Z"
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("g3a", type=Path)
    ap.add_argument("--name", default="CasioCAS")
    ap.add_argument("--internal", default="@CASCAS")
    ap.add_argument("--filename", default="/CasioCAS.g3a")
    args = ap.parse_args()

    buf = args.g3a.read_bytes()
    expected = {
        "basic": args.name,
        "internal": args.internal,
        "filename": args.filename,
    }
    bad = []
    for key, want in expected.items():
        got = field(buf, key)
        print(f"{key}: {got}")
        if got != want:
            bad.append(f"{key}={got!r}, want {want!r}")
        if key == "internal":
            reason = check_internal_name(got)
            if reason:
                bad.append(reason)

    stored = int.from_bytes(buf[0x20:0x24], "big")
    stored2 = int.from_bytes(buf[-4:], "big")
    actual = checksum(buf)
    print(f"checksum: header={stored:08x} tail={stored2:08x} actual={actual:08x}")
    if stored != actual or stored2 != actual:
        bad.append("checksum mismatch")

    if b"Khicasen" in buf[:0x1000] or b"@KHICASEN" in buf[:0x1000]:
        bad.append("old Khicasen metadata remains in header")
    if b"\0 Xcas \0" in buf:
        bad.append("old Xcas status label remains")

    if bad:
        print("FAIL " + "; ".join(bad))
        return 1
    print("OK g3a metadata")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import struct
import sys
from pathlib import Path

MAGIC = b"CCP1"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def read_pack(path: Path) -> dict[str, str]:
    data = path.read_bytes()
    if len(data) < 6 or data[:4] != MAGIC:
        raise ValueError("bad magic")
    count = struct.unpack_from(">H", data, 4)[0]
    pos = 6
    records: dict[str, str] = {}
    for _ in range(count):
        if pos + 4 > len(data):
            raise ValueError("truncated record header")
        nlen, blen = struct.unpack_from(">HH", data, pos)
        pos += 4
        if pos + nlen + blen > len(data):
            raise ValueError("truncated record body")
        name = data[pos : pos + nlen].decode("utf-8")
        pos += nlen
        body = data[pos : pos + blen].decode("utf-8")
        pos += blen
        records[name] = body
    if pos != len(data):
        raise ValueError("trailing bytes")
    return records


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_external_pack.py calculator_files/CASIOCAS.PAK", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    if not path.exists():
        return fail(f"missing {path}")
    try:
        records = read_pack(path)
    except Exception as exc:
        return fail(str(exc))
    required = [
        "integrate(f,x,[a,b,method,u])",
        "diff(f,var,[n,method])",
        "solve_trig(eq,[var,lo,hi,max,method])",
        "t141",
        "t140",
        "t107",
        "t079",
    ]
    missing = [name for name in required if name not in records]
    if missing:
        return fail("missing records: " + ", ".join(missing))
    if any(line.startswith(("F2:", "F3:")) for body in records.values() for line in body.splitlines()):
        return fail("insert-only examples leaked into F6 help pack")
    size = path.stat().st_size
    print(f"OK external pack records={len(records)} bytes={size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

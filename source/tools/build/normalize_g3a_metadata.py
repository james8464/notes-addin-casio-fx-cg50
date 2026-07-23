#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path


STAMP_RE = re.compile(rb"20[0-9]{2}\.[0-9]{4}\.[0-9]{4}")


def checksum(buf: bytes | bytearray) -> int:
    tmp = bytearray(buf)
    tmp[0x20:0x24] = b"\0\0\0\0"
    tmp[-4:] = b"\0\0\0\0"
    return sum(tmp) & 0xFFFFFFFF


def normalize(path: Path, stamp: bytes) -> None:
    if not re.fullmatch(rb"[0-9]{4}\.[0-9]{4}\.[0-9]{4}", stamp):
        raise SystemExit("timestamp must be YYYY.MMDD.HHMM")
    buf = bytearray(path.read_bytes())
    matches = list(STAMP_RE.finditer(buf))
    if len(matches) != 1:
        raise SystemExit(f"{path}: expected one g3a timestamp, found {len(matches)}")
    match = matches[0]
    buf[match.start() : match.end()] = stamp
    value = checksum(buf).to_bytes(4, "big")
    buf[0x20:0x24] = value
    buf[-4:] = value
    path.write_bytes(buf)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("g3a", nargs="+", type=Path)
    parser.add_argument(
        "--timestamp",
        default="2000.0101.0000",
        help="deterministic g3a timestamp, format YYYY.MMDD.HHMM",
    )
    args = parser.parse_args()
    stamp = args.timestamp.encode("ascii")
    for path in args.g3a:
        normalize(path, stamp)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

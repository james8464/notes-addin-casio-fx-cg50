#!/usr/bin/env python3
from __future__ import annotations

import struct
import sys
from pathlib import Path

MAGIC = b"CCP1"


def parse_help(src: Path) -> list[tuple[bytes, bytes]]:
    preserve = src.suffix.upper() == ".TPL"
    records: list[tuple[bytes, bytes]] = []
    name: str | None = None
    body: list[str] = []
    for raw in src.read_text(encoding="utf-8", errors="ignore").splitlines():
        if raw.startswith("@END"):
            if name:
                text = "\n".join(body)
                if not preserve:
                    text = text.strip()
                text += "\n"
                records.append((name.encode("utf-8"), text.encode("utf-8")))
            name = None
            body = []
            continue
        if raw.startswith("@"):
            name = raw[1:].strip()
            body = []
            continue
        if not name:
            continue
        # F2/F3 examples are insertion hints, not F6 help text.
        if not preserve and (raw.startswith("F2:") or raw.startswith("F3:")):
            continue
        line = raw if preserve else " ".join(raw.rstrip().split())
        if line or preserve:
            body.append(line)
    return records


def build_pack(records: list[tuple[bytes, bytes]]) -> bytes:
    records = sorted(records, key=lambda r: 0 if r[0].startswith(b"t") and r[0][1:].isdigit() else 1)
    out = bytearray(MAGIC)
    out += struct.pack(">H", len(records))
    for name, body in records:
        if len(name) > 0xFFFF or len(body) > 0xFFFF:
            raise ValueError("record too large")
        out += struct.pack(">HH", len(name), len(body))
        out += name
        out += body
    return bytes(out)


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: build_external_pack.py CASIOCAS.HLP [CASIOCAS.TPL ...] CASIOCAS.PAK", file=sys.stderr)
        return 2
    dst = Path(sys.argv[-1])
    records: list[tuple[bytes, bytes]] = []
    for arg in sys.argv[1:-1]:
        records.extend(parse_help(Path(arg)))
    if not records:
        print("FAIL no help records", file=sys.stderr)
        return 1
    data = build_pack(records)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    print(f"PAK {dst}: {len(records)} records, {len(data)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

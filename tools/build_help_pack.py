#!/usr/bin/env python3
from __future__ import annotations

import struct
import sys
from pathlib import Path

MAGIC = b"CASPAK1\0"


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: tools/build_help_pack.py help/functions output.PAK", file=sys.stderr)
        return 2
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    if not src.is_dir():
        print(f"missing help dir: {src}", file=sys.stderr)
        return 2

    records: list[tuple[bytes, bytes]] = []
    for path in sorted(src.glob("*.txt")):
        name = path.stem.encode("ascii")
        text = path.read_text(encoding="utf-8").replace("\r\n", "\n").replace("\r", "\n")
        data = text.encode("utf-8")
        if not data.endswith(b"\n"):
            data += b"\n"
        if len(name) > 255:
            raise SystemExit(f"help name too long: {path.name}")
        records.append((name, data))

    index_len = 2 + sum(1 + len(name) + 8 for name, _ in records)
    offset = len(MAGIC) + index_len
    index = bytearray(struct.pack(">H", len(records)))
    payload = bytearray()
    for name, data in records:
        index.append(len(name))
        index.extend(name)
        index.extend(struct.pack(">II", offset, len(data)))
        payload.extend(data)
        offset += len(data)

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(MAGIC + bytes(index) + bytes(payload))
    print(f"{out.name}: records={len(records)} bytes={out.stat().st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

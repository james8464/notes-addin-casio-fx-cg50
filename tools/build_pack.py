#!/usr/bin/env python3
from __future__ import annotations

import struct
import sys
from pathlib import Path

MAGIC = b"CCP1"


def records_from_file(path: Path) -> list[tuple[bytes, bytes]]:
    text = path.read_text(encoding="utf-8", errors="ignore").strip()
    if not text:
        return []
    if text.startswith("@"):
        out: list[tuple[bytes, bytes]] = []
        name: str | None = None
        body: list[str] = []
        for raw in text.splitlines():
            if raw.startswith("@END"):
                if name:
                    out.append((name.encode(), ("\n".join(body).strip() + "\n").encode()))
                name = None
                body = []
            elif raw.startswith("@"):
                name = raw[1:].strip()
                body = []
            elif name:
                body.append(raw.rstrip())
        if name:
            out.append((name.encode(), ("\n".join(body).strip() + "\n").encode()))
        return out
    return [(path.stem.encode(), (text + "\n").encode())]


def build(records: list[tuple[bytes, bytes]]) -> bytes:
    out = bytearray(MAGIC)
    out += struct.pack(">H", len(records))
    for name, body in sorted(records):
        out += struct.pack(">HH", len(name), len(body))
        out += name
        out += body
    return bytes(out)


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: tools/build_pack.py help_dir out.pak", file=sys.stderr)
        return 2
    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    records: list[tuple[bytes, bytes]] = []
    for path in sorted(src.rglob("*")):
        if path.is_file() and path.suffix.lower() in {".txt", ".hlp", ".tpl", ".md"}:
            records.extend(records_from_file(path))
    if not records:
        records.append((b"index", b"CasioCAS help pack.\n"))
    dst.parent.mkdir(parents=True, exist_ok=True)
    data = build(records)
    dst.write_bytes(data)
    print(f"PAK {dst}: {len(records)} records, {len(data)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

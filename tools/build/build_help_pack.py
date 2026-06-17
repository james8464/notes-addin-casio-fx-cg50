#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

MAGIC = b"CASPAK4\0"


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: tools/build/build_help_pack.py help/functions output.PAK", file=sys.stderr)
        return 2
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    if not src.is_dir():
        print(f"missing help dir: {src}", file=sys.stderr)
        return 2

    root = src.parents[1]
    catalog = root / "khicas" / "upstream" / "giac90_1addin" / "catalogen.cpp"
    names: list[str] = []
    if catalog.exists():
        block = catalog.read_text(errors="ignore").split("const catalogFunc completeCat[]", 1)[1].split("\n};", 1)[0]
        for line in block.splitlines():
            if '{"' in line:
                label = line.split('"', 2)[1]
                names.append(label.split("(", 1)[0].split("[", 1)[0])
    if not names:
        names = [p.stem for p in sorted(src.glob("*.txt"))]

    records: list[bytes] = []
    for name in names:
        path = src / f"{name}.txt"
        text = path.read_text(encoding="utf-8").replace("\r\n", "\n").replace("\r", "\n")
        data = text.encode("utf-8")
        if not data.endswith(b"\n"):
            data += b"\n"
        records.append(data)

    payload = bytearray(MAGIC)
    payload.extend(len(records).to_bytes(2, "little"))
    offset = 0
    offsets = [0]
    for data in records:
        offset += len(data)
        offsets.append(offset)
    for off in offsets:
        payload.extend(off.to_bytes(4, "little"))
    for data in records:
        payload.extend(data)

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(bytes(payload))
    print(f"{out.name}: records={len(records)} bytes={out.stat().st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

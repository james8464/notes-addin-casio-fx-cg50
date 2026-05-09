#!/usr/bin/env python3
from __future__ import annotations

import struct
import sys
from pathlib import Path

MAGIC = b"CCP1"
BAD_HELP = ("PReq", "Req/Opt", "Req before []. Opt inside []", "Req before []", "Opt inside []")


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


def pack_names(path: Path) -> list[str]:
    data = path.read_bytes()
    count = struct.unpack_from(">H", data, 4)[0]
    pos = 6
    names: list[str] = []
    for _ in range(count):
        nlen, blen = struct.unpack_from(">HH", data, pos)
        pos += 4
        names.append(data[pos : pos + nlen].decode("utf-8"))
        pos += nlen + blen
    return names


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
        "solve(equation,x,[method])",
        "t141",
        "t140",
        "t107",
        "t079",
        "fmenu.standard",
    ]
    missing = [name for name in required if name not in records]
    if missing:
        return fail("missing records: " + ", ".join(missing))
    if any(line.startswith(("F2:", "F3:")) for body in records.values() for line in body.splitlines()):
        return fail("insert-only examples leaked into F6 help pack")
    unclear = [name for name, body in records.items() if any(phrase in body for phrase in BAD_HELP)]
    if unclear:
        return fail("unclear help wording: " + ", ".join(unclear[:12]))
    for name in required[:3]:
        body = records[name]
        if "Req:" not in body or "Opt:" not in body or "Ex F" not in body:
            return fail(f"incomplete command help: {name}")
    inline_tpl = {
        "t047", "t053", "t055", "t060", "t061", "t070", "t071",
        "t072", "t073", "t075", "t078", "t081", "t082", "t085", "t107",
    }
    bad_tpl = [
        name for name in sorted(inline_tpl)
        if records.get(name, "").endswith(("\n", "\r")) or "\n" in records.get(name, "")
    ]
    if bad_tpl:
        return fail("inline working templates contain newlines: " + ", ".join(bad_tpl))
    names = pack_names(path)
    first_tpl = next((i for i, name in enumerate(names) if name.startswith("t") and name[1:].isdigit()), 9999)
    if first_tpl > 8:
        return fail("working templates are too late in PAK; runtime lookup is slow")
    size = path.stat().st_size
    print(f"OK external pack records={len(records)} bytes={size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

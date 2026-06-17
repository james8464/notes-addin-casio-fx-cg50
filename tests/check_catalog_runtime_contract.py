#!/usr/bin/env python3
from __future__ import annotations

import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "khicas/upstream/giac90_1addin"
CATALOG = SRC / "catalogen.cpp"
MAIN = SRC / "main.cc"
PAK = ROOT / "calculator_files/CAS.PAK"

PROBE = [
    "suvat",
    "xform",
    "range",
    "friction",
    "projectile",
    "texpand",
    "tcollect",
    "sec",
]


def catalog_names() -> list[str]:
    text = CATALOG.read_text(errors="ignore")
    block = text.split("const catalogFunc completeCat[]", 1)[1].split("\n};", 1)[0]
    return [
        line.split('"', 2)[1].split("(", 1)[0].split("[", 1)[0]
        for line in block.splitlines()
        if '{"' in line
    ]


def pak_records() -> list[str]:
    data = PAK.read_bytes()
    if data[:8] != b"CASPAK4\0":
        raise SystemExit("FAIL catalog runtime: CAS.PAK magic")
    count = data[8] | (data[9] << 8)
    base = 10 + 4 * (count + 1)
    offsets = [struct.unpack_from("<I", data, 10 + 4 * i)[0] for i in range(count + 1)]
    return [
        data[base + offsets[i] : base + offsets[i + 1]].decode("utf-8", "replace").split("\n", 1)[0].strip()
        for i in range(count)
    ]


def main() -> int:
    names = catalog_names()
    missing = [name for name in PROBE if name not in names]
    if missing:
        raise SystemExit("FAIL catalog runtime missing names: " + ", ".join(missing))

    main_src = MAIN.read_text(errors="ignore")
    cat_src = CATALOG.read_text(errors="ignore")
    required_source = [
        "catalog_has_command(buf)",
        "bool folders=category==CAT_CATEGORY_ALL && !cmdname",
        "category==CAT_CATEGORY_ALL || completeCat[cur].category==category",
        "completeCat[cur].name[cmdl]==0",
        "if(!folders && sres == KEY_CTRL_F6)",
    ]
    absent = [s for s in required_source if s not in main_src + cat_src]
    if absent:
        raise SystemExit("FAIL catalog runtime source contract: " + ", ".join(absent))

    records = pak_records()
    if records != names:
        for i, (a, b) in enumerate(zip(names, records)):
            if a != b:
                raise SystemExit(f"FAIL catalog runtime pak order {i}: {a} != {b}")
        raise SystemExit("FAIL catalog runtime pak count")

    print(f"OK catalog runtime contract commands={len(names)} probes={len(PROBE)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "c++/prizm/build"
LIMIT = 2 * 1024 * 1024
WARN_HEADROOM = 8 * 1024
BASELINE = BUILD / "size_report_baseline.json"


def file_size(path: Path) -> int:
    return path.stat().st_size if path.exists() else 0


def parse_top_symbols(path: Path, limit: int = 20) -> list[tuple[int, str]]:
    if not path.exists():
        return []
    rows: list[tuple[int, str]] = []
    for line in path.read_text(errors="ignore").splitlines():
        parts = line.split()
        if len(parts) < 5:
            continue
        try:
            size = int(parts[4], 16)
        except ValueError:
            continue
        if size >= 512:
            rows.append((size, line[-140:]))
    return sorted(rows, reverse=True)[:limit]


def parse_sections(path: Path) -> list[str]:
    if not path.exists():
        return []
    keep: list[str] = []
    for line in path.read_text(errors="ignore").splitlines():
        s = line.strip()
        if not s.startswith((".text ", ".rodata ", ".data ", ".bss ")):
            continue
        parts = s.split()
        if len(parts) >= 3 and parts[1] != "0x0000000000000000" and parts[2] != "0x0":
            keep.append(s[:160])
    return keep[:40]


def main() -> int:
    ap = argparse.ArgumentParser(description="Report CasioCAS add-in size and linked symbol hotspots.")
    ap.add_argument("--baseline", default="", help="Use 'current' for c++/prizm/build/size_report_baseline.json")
    ap.add_argument("--write-baseline", action="store_true")
    args = ap.parse_args()

    g3a = BUILD / "CasioCAS.g3a"
    data = {
        "g3a": file_size(g3a),
        "bin": file_size(BUILD / "CasioCAS.bin"),
        "elf": file_size(BUILD / "CasioCAS.elf"),
        "pak": file_size(BUILD / "CASIOCAS.PAK"),
    }
    print(json.dumps(data, sort_keys=True))
    if args.baseline == "current" and BASELINE.exists():
        old = json.loads(BASELINE.read_text())
        print(f"delta_g3a={data['g3a'] - int(old.get('g3a', 0))}")
    elif args.baseline:
        print("baseline=none")
    if args.write_baseline:
        BASELINE.write_text(json.dumps(data, sort_keys=True) + "\n")

    headroom = LIMIT - data["g3a"]
    print(f"headroom={headroom} bytes")
    if data["g3a"] > LIMIT:
        print("FAIL .g3a exceeds 2 MiB")
        return 1
    if headroom < WARN_HEADROOM:
        print("WARN .g3a headroom below 8 KiB")

    sections = parse_sections(BUILD / "CasioCAS.map")
    if sections:
        print("sections:")
        for s in sections:
            print("  " + s)
    symbols = parse_top_symbols(BUILD / "dumpen_t")
    if symbols:
        print("top_symbols:")
        for size, line in symbols:
            print(f"  {size:7d} {line}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

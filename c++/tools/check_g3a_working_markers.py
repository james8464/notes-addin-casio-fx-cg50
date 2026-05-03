#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


MARKERS = [
    b"Alias rewrite:",
    b"Fallback search",
    b"Answer: ",
    b"Parse derivative",
    b"Parse integral",
    b"Matrix setup",
    b"Stats setup",
]


def main() -> int:
    ap = argparse.ArgumentParser(description="Verify source-built CasioCAS .g3a includes working-line hooks.")
    ap.add_argument("g3a", type=Path)
    args = ap.parse_args()
    data = args.g3a.read_bytes()
    missing = [m.decode("ascii") for m in MARKERS if m not in data]
    if missing:
        print("FAIL g3a working markers missing: " + ", ".join(missing))
        return 1
    print("OK g3a working markers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

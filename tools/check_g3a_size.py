#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

HARD_PACKAGE_LIMIT = 2 * 1024 * 1024
TARGET_SIZE = 2_000_000


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: tools/check_g3a_size.py path/to/file.g3a", file=sys.stderr)
        return 2
    p = Path(sys.argv[1])
    if not p.exists():
        print(f"missing: {p}", file=sys.stderr)
        return 2
    size = p.stat().st_size
    print(f"{p.name}: {size} bytes ({size / (1024 * 1024):.3f} MiB)")
    if size > HARD_PACKAGE_LIMIT:
        print("FAIL: exceeds 2 MiB fx-CG add-in package limit", file=sys.stderr)
        return 1
    if size > TARGET_SIZE:
        print(f"WARN: above target {TARGET_SIZE} bytes; pruning required", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

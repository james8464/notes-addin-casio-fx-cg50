#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path

HARD_PACKAGE_LIMIT = int(os.environ.get("CASIO_G3A_HARD_LIMIT", "2097152"))
TARGET_SIZE = int(os.environ.get("CASIO_G3A_TARGET_SIZE", str(HARD_PACKAGE_LIMIT)))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: tools/checks/check_g3a_size.py path/to/file.g3a", file=sys.stderr)
        return 2
    p = Path(sys.argv[1])
    if not p.exists():
        print(f"missing: {p}", file=sys.stderr)
        return 2
    size = p.stat().st_size
    print(f"{p.name}: {size} bytes ({size / (1024 * 1024):.3f} MiB)")
    if size > HARD_PACKAGE_LIMIT:
        print(f"FAIL: exceeds configured add-in package limit {HARD_PACKAGE_LIMIT} bytes", file=sys.stderr)
        return 1
    if size > TARGET_SIZE:
        print(f"WARN: above target {TARGET_SIZE} bytes; pruning required", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

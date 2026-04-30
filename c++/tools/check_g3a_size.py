#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: tools/check_g3a_size.py path/to/file.g3a", file=sys.stderr)
        return 2
    p = Path(sys.argv[1])
    if not p.exists():
        print(f"missing: {p}", file=sys.stderr)
        return 2
    size = p.stat().st_size
    mib = size / (1024 * 1024)
    print(f"{p.name}: {size} bytes ({mib:.3f} MiB)")
    if mib > 2.0:
        print("FAIL: exceeds ~2 MiB CG50 add-in size guidance", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


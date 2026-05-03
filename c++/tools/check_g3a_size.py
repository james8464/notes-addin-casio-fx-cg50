#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

SOFT_GUIDANCE = 2 * 1024 * 1024
HARD_PACKAGE_LIMIT = int(2.05 * 1024 * 1024)


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
    if size > HARD_PACKAGE_LIMIT:
        print("FAIL: exceeds 2.05 MiB package safety limit", file=sys.stderr)
        return 1
    if size > SOFT_GUIDANCE:
        print("WARN: package is above 2 MiB; linker ROM usage is the hard add-in guard")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

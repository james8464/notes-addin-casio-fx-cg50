#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

HARD_PACKAGE_LIMIT = 2 * 1024 * 1024
WARN_HEADROOM = 8 * 1024


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
        print("FAIL: exceeds 2 MiB fx-CG add-in package limit", file=sys.stderr)
        return 1
    headroom = HARD_PACKAGE_LIMIT - size
    if headroom < WARN_HEADROOM:
        print(f"WARN: only {headroom} bytes below 2 MiB; keep code changes size-neutral", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

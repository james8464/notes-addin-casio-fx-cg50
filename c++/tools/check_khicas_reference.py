#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
REFERENCE = REPO / "c++/khicas/upstream/reference/khicasen.g3a"
EXPECTED_SHA256 = "e926c9a8e3111c51786e444f2b6f59104362c960925fd0704668f73a8987bfdf"
MIN_SIZE = 1_900_000


def main() -> int:
    if not REFERENCE.exists():
        print(f"FAIL missing {REFERENCE}")
        return 1
    data = REFERENCE.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    print(f"{REFERENCE.name}: {len(data)} bytes sha256={sha}")
    if sha != EXPECTED_SHA256:
        print("FAIL upstream KhiCAS reference SHA mismatch")
        return 1
    if len(data) < MIN_SIZE:
        print("FAIL upstream KhiCAS reference is unexpectedly small")
        return 1
    print("OK upstream KhiCAS reference")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


MARKERS = [
    b"Rw:",
    b"Fallback:",
    b"Valid:",
    b"Ans: ",
    b"Br: real branch.",
    b"Diff:",
    b"Impl:",
    b"Param:",
    b"Area:",
    b"Fn:",
    b"SUVAT:",
    b"Binom:",
    b"Coeff:",
    b"Int:",
    b"Mat:",
    b"Stats:",
    b"Trig solve:",
    b"Const:",
    b"Match:",
    b"Comp sq:",
    b"DE:",
    b"King:",
    b"Weier:",
    b"Sophie:",
    b"Sym:",
    b"Ref tri:",
    b"DI:",
    b"PF:",
    b"Hard int:",
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

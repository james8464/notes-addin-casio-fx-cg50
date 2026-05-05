#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "c++/khicas/upstream/giac90_1addin"
MAKEFILE = SRC / "Makefile"

# Only remove an object after this script proves the edited source still links.
PRUNE_CANDIDATES = [
    "kplot.o",
    "kprog.o",
    "krpn.o",
    "kmoyal.o",
    "kmaple.o",
    "kti89.o",
]


def restore_makefile(original: str) -> None:
    MAKEFILE.write_text(original)


def remove_candidate(makefile: str, candidate: str) -> str:
    token = candidate.strip()
    if not token.endswith(".o"):
        token += ".o"
    if token not in makefile:
        raise ValueError(f"{token} not in Makefile")
    return makefile.replace(" " + token, "", 1)


def link_test(candidate: str, keep: bool) -> int:
    original = MAKEFILE.read_text()
    try:
        MAKEFILE.write_text(remove_candidate(original, candidate))
        result = subprocess.run(
            [str(ROOT / "compile")],
            cwd=ROOT,
            text=True,
        )
        if result.returncode == 0:
            print(f"SAFE {candidate}: full compile/link passed")
            if keep:
                return 0
        else:
            print(f"KEEP {candidate}: compile/link failed")
        return result.returncode
    finally:
        if not keep:
            restore_makefile(original)


def main() -> int:
    parser = argparse.ArgumentParser(description="Prove a KhiCAS object can be pruned before removing it.")
    parser.add_argument("--candidate", action="append", choices=PRUNE_CANDIDATES)
    parser.add_argument("--keep", action="store_true", help="keep the Makefile edit if the candidate passes")
    args = parser.parse_args()

    candidates = args.candidate or PRUNE_CANDIDATES
    rc = 0
    for candidate in candidates:
        rc = link_test(candidate, args.keep) or rc
    return rc


if __name__ == "__main__":
    raise SystemExit(main())

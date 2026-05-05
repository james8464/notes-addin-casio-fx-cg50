#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "c++/khicas/upstream/giac90_1addin"
MAKEFILE = SRC / "Makefile"

# Seed list only. The script discovers the authoritative CAS_OBJS list from Makefile.
PRUNE_CANDIDATES = ["kplot.o", "kprog.o", "krpn.o", "kmoyal.o", "kmaple.o", "kti89.o"]


def discover_makefile_objects(makefile: str) -> list[str]:
    match = re.search(r"^CAS_OBJS\s*=\s*(.*)$", makefile, re.M)
    if not match:
        raise ValueError("CAS_OBJS not found")
    line = match.group(1).split("#", 1)[0]
    return [x for x in line.split() if x.endswith(".o")]


def restore_makefile(original: str) -> None:
    MAKEFILE.write_text(original)


def remove_candidate(makefile: str, candidate: str) -> str:
    token = candidate.strip()
    if not token.endswith(".o"):
        token += ".o"
    valid = set(discover_makefile_objects(makefile))
    if token not in valid:
        raise ValueError(f"{token} not in CAS_OBJS")
    if token not in makefile:
        raise ValueError(f"{token} not in Makefile")
    return makefile.replace(" " + token, "", 1)


def run_gate(cmd: list[str]) -> int:
    print("+ " + " ".join(cmd))
    return subprocess.run(cmd, cwd=ROOT, text=True).returncode


def link_test(candidate: str, keep: bool, full_tests: bool) -> int:
    original = MAKEFILE.read_text()
    try:
        MAKEFILE.write_text(remove_candidate(original, candidate))
        result = run_gate([str(ROOT / "compile")])
        if result != 0:
            print(f"KEEP {candidate}: compile/link failed")
            return result
        gates = [
            ["python3", "c++/tools/check_g3a_metadata.py", "c++/prizm/build/CasioCAS.g3a"],
            ["python3", "c++/tools/check_g3a_working_markers.py", "c++/prizm/build/CasioCAS.g3a"],
            ["python3", "c++/tools/check_external_pack.py", "calculator_files/CASIOCAS.PAK"],
            ["python3", "c++/tools/check_build_packaging.py"],
        ]
        if full_tests:
            gates.append(["python3", "c++/tools/run_tests_cpp.py"])
        for gate in gates:
            rc = run_gate(gate)
            if rc != 0:
                print(f"KEEP {candidate}: gate failed")
                return rc
        print(f"SAFE {candidate}: compile/link/tests passed")
        return 0
    finally:
        if not keep:
            restore_makefile(original)


def main() -> int:
    parser = argparse.ArgumentParser(description="Prove a KhiCAS object can be pruned before removing it.")
    parser.add_argument("--candidate", action="append")
    parser.add_argument("--keep", action="store_true", help="keep the Makefile edit if the candidate passes")
    parser.add_argument("--list", action="store_true", help="list CAS_OBJS and exit")
    parser.add_argument("--link-only", action="store_true", help="skip the expensive full run_tests_cpp.py gate")
    args = parser.parse_args()

    objects = discover_makefile_objects(MAKEFILE.read_text())
    if args.list:
        for obj in objects:
            print(obj)
        return 0
    candidates = args.candidate or PRUNE_CANDIDATES
    rc = 0
    for candidate in candidates:
        rc = link_test(candidate, args.keep, not args.link_only) or rc
    return rc


if __name__ == "__main__":
    raise SystemExit(main())

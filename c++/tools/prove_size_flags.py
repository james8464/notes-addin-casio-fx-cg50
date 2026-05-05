#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAKEFILE = ROOT / "c++/khicas/upstream/giac90_1addin/Makefile"

FLAG_CANDIDATES = {
    "whole-program": ("CXXFLAGS", "-fwhole-program"),
    "strip-link": ("LDFLAGS", "-Wl,-s"),
}


def restore_makefile(original: str) -> None:
    MAKEFILE.write_text(original)


def add_flag(makefile: str, variable: str, flag: str) -> str:
    lines = makefile.splitlines()
    for i, line in enumerate(lines):
        if line.startswith(variable + " ="):
            if flag not in line:
                lines[i] = line + " " + flag
            return "\n".join(lines) + "\n"
    raise ValueError(variable + " not found")


def run(cmd: list[str]) -> int:
    print("+ " + " ".join(cmd))
    return subprocess.run(cmd, cwd=ROOT, text=True).returncode


def prove(name: str, keep: bool, link_only: bool) -> int:
    variable, flag = FLAG_CANDIDATES[name]
    original = MAKEFILE.read_text()
    try:
        MAKEFILE.write_text(add_flag(original, variable, flag))
        rc = run([str(ROOT / "compile")])
        if rc != 0:
            print(f"KEEP baseline: {name} compile failed")
            return rc
        gates = [
            ["python3", "c++/tools/check_g3a_size.py", "c++/prizm/build/CasioCAS.g3a"],
            ["python3", "c++/tools/check_external_pack.py", "calculator_files/CASIOCAS.PAK"],
            ["python3", "c++/tools/size_report.py", "--baseline", "current"],
        ]
        if not link_only:
            gates.append(["python3", "c++/tools/run_tests_cpp.py"])
        for gate in gates:
            rc = run(gate)
            if rc != 0:
                print(f"KEEP baseline: {name} gate failed")
                return rc
        print(f"SAFE flag {name}: {flag}")
        return 0
    finally:
        if not keep:
            restore_makefile(original)


def main() -> int:
    ap = argparse.ArgumentParser(description="Temporarily prove size flags before keeping them.")
    ap.add_argument("--flag", choices=sorted(FLAG_CANDIDATES), action="append")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--link-only", action="store_true")
    ap.add_argument("--list", action="store_true")
    args = ap.parse_args()
    if args.list:
        for name, (_, flag) in FLAG_CANDIDATES.items():
            print(f"{name}: {flag}")
        return 0
    names = args.flag or sorted(FLAG_CANDIDATES)
    rc = 0
    for name in names:
        rc = prove(name, args.keep, args.link_only) or rc
    return rc


if __name__ == "__main__":
    raise SystemExit(main())

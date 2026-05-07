#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / "c++/addin/host/build/casio_host"
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def main() -> int:
    if not HOST.exists():
        return fail("missing host build; run c++/tools/build_host.sh")
    cases = [
        ["--alg", "solve_trig("],
        ["--alg", "solve("],
        ["--trig", "solve_trig("],
    ]
    for args in cases:
        proc = subprocess.run([str(HOST), *args], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout + proc.stderr).strip()
        if "Answer:" in out or out.startswith("1.") or "\n1." in out or "\n2." in out:
            return fail("invalid input produced working: " + " ".join(args) + "\n" + out)
        if "ERR:" not in out and "Err:" not in out:
            return fail("invalid input did not produce compact error: " + " ".join(args) + "\n" + out)
    main_cc = MAIN.read_text(errors="ignore")
    for marker in ["cascas_syntax_complete", "cascas_show_working_for"]:
        if marker not in main_cc:
            return fail("calculator invalid-input guard missing: " + marker)
    proc = subprocess.run(
        [str(HOST), "--alg", "solve(2*sin(x)+1=0,x=0..2*pi,method=cast)"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    trig_out = proc.stdout + proc.stderr
    for marker in ["Base angle", "x=[7*pi/6, 11*pi/6]"]:
        if marker not in trig_out:
            return fail("solve() did not route trig solve working:\n" + trig_out)
    print("OK invalid input suppresses working")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

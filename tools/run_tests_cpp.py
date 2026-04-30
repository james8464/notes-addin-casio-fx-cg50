#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]


def run(cmd: list[str]) -> int:
    p = subprocess.run(cmd, cwd=str(REPO))
    return p.returncode


def main() -> int:
    if "--tui" in sys.argv:
        # Launch Textual TUI runner (drives C++ host via --stdin-program).
        return run([sys.executable, "tools/tests_cpp/run_tests_tui.py"])
    if "--fuzz" in sys.argv:
        # Randomized oracle-vs-host miner (writes JSONL on failures).
        return run([sys.executable, "tools/fuzz/fuzz_triage.py"])

    # Build host first (uses pip --user cmake when needed).
    rc = run(["./tools/build_host.sh"])
    if rc != 0:
        print("FAIL: build_host", file=sys.stderr)
        return rc

    checks: list[tuple[str, list[str]]] = [
        ("host_smoke", [sys.executable, "tools/golden/check_host_smoke.py"]),
        ("expr_format", [sys.executable, "tools/golden/compare_expr_format.py"]),
        ("suvat", [sys.executable, "tools/golden/compare_suvat.py"]),
        ("integrate_basic", [sys.executable, "tools/golden/compare_int_basic.py"]),
        ("integrate_more", [sys.executable, "tools/golden/compare_int_more.py"]),
        ("derive_basic", [sys.executable, "tools/golden/compare_derive_basic.py"]),
    ]

    bad = 0
    for name, cmd in checks:
        print(f"== {name} ==")
        rc = run(cmd)
        if rc != 0:
            bad += 1
            print(f"FAIL {name} (exit={rc})", file=sys.stderr)

    if bad:
        print(f"FAILED: {bad} checks", file=sys.stderr)
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


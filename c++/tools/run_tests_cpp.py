#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]


def run(cmd: list[str]) -> int:
    p = subprocess.run(cmd, cwd=str(REPO))
    return p.returncode


def main() -> int:
    lowered = [a.lower() for a in sys.argv[1:]]
    if lowered and lowered[-1] == "compile":
        return run(["./compile"])
    if "--fuzz" in sys.argv:
        return run([sys.executable, "c++/tools/fuzz/check_regressions.py"])

    # Build host first (uses pip --user cmake when needed).
    rc = run(["./c++/tools/build_host.sh"])
    if rc != 0:
        print("FAIL: build_host", file=sys.stderr)
        return rc

    checks: list[tuple[str, list[str]]] = [
        ("khicas_reference", [sys.executable, "c++/tools/check_khicas_reference.py"]),
        ("khicas_catalog", [sys.executable, "c++/tools/check_khicas_catalog.py"]),
        ("khicas_working", [sys.executable, "c++/tools/check_khicas_working.py"]),
        ("feature_parity", [sys.executable, "c++/tools/check_feature_parity.py"]),
        ("host_smoke", [sys.executable, "c++/tools/golden/check_host_smoke.py"]),
        ("tests_txt_cases", [sys.executable, "c++/tools/golden/check_tests_txt_cases.py"]),
        ("expr_format", [sys.executable, "c++/tools/golden/compare_expr_format.py"]),
        ("suvat", [sys.executable, "c++/tools/golden/compare_suvat.py"]),
        ("integrate_basic", [sys.executable, "c++/tools/golden/compare_int_basic.py"]),
        ("integrate_more", [sys.executable, "c++/tools/golden/compare_int_more.py"]),
        ("exam_working", [sys.executable, "c++/tools/golden/check_exam_working.py"]),
        ("integration_centurion", [sys.executable, "c++/tools/golden/check_integration_centurion.py"]),
        ("derive_basic", [sys.executable, "c++/tools/golden/compare_derive_basic.py"]),
        ("trig_basic", [sys.executable, "c++/tools/golden/compare_trig_basic.py"]),
        ("algebra_basic", [sys.executable, "c++/tools/golden/compare_algebra_basic.py"]),
        ("fuzz_regressions", [sys.executable, "c++/tools/fuzz/check_regressions.py"]),
    ]
    prizm_g3a = REPO / "c++/prizm/build/CasioCAS.g3a"
    if prizm_g3a.exists():
        checks.insert(2, ("g3a_metadata", [sys.executable, "c++/tools/check_g3a_metadata.py", str(prizm_g3a)]))
        checks.insert(3, ("g3a_working_markers", [sys.executable, "c++/tools/check_g3a_working_markers.py", str(prizm_g3a)]))

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

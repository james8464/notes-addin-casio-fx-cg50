#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PY = sys.executable


def run_cpp(host: Path, expr: str) -> str:
    p = subprocess.run([str(host), "--alg", expr], text=True, capture_output=True, cwd=str(REPO))
    return p.stdout


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    cases = [
        ("domain_sqrt", "sqrt(x-1)"),
        ("domain_log", "ln(x)"),
        ("domain_div", "1/(x-2)"),
        ("range_quad", "x^2+6*x+5"),
    ]

    bad = 0
    for label, expr in cases:
        out = run_cpp(host, expr).lower()
        ok = ("answer:" in out)
        if label.startswith("domain"):
            ok = ok and ("domain:" in out)
        if label == "range_quad":
            ok = ok and ("range:" in out)
        if not ok:
            bad += 1
            print("MISMATCH", label, expr)
            print(out)
        else:
            print("OK", label)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())


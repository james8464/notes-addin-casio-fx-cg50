#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
MAIN = REPO / "c++" / "khicas" / "upstream" / "giac90_1addin" / "main.cc"
CATALOG = REPO / "c++" / "khicas" / "upstream" / "giac90_1addin" / "catalogen.cpp"


CASES = [
    ("int", "x*exp(x),method=parts", ["u=x", "dv=e^x dx"]),
    ("int", "x*cos(x^2),method=sub,u=x^2", ["u=x^2", "du=2*x dx"]),
    ("int", "x^2,method=badmethod", ["Invalid method"]),
    ("derive", "x^4,method=second", ["Differentiate once", "d2y/dx2"]),
    ("trig", "2+sec(x-pi/3)=0, x, 0, 2*pi,method=bounded", ["cos(A) = -1/2", "x = [pi, 5*pi/3]"]),
    ("alg", "x^2-5*x+6=0,method=factor", ["x^2 - 5*x + 6 = 0"]),
    ("stats", "1,2,3,4,method=summary", ["mean =", "Sxx ="]),
    ("suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=suvat", ["s = 25"]),
]


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")

    src = MAIN.read_text(errors="ignore") + "\n" + CATALOG.read_text(errors="ignore")
    markers = [
        "cascas_extract_method",
        "cascas_strip_method_args",
        "cascas_rewrite_by_call",
        "integrate_by(f,x,method,[u])",
        "diff_by(f,var,method)",
        "solve_by(equation,x,method)",
        "solve_trig_by(eq,var,method)",
    ]
    missing = [m for m in markers if m not in src]
    if missing:
        print("FAIL missing source markers: " + ", ".join(missing))
        return 1

    bad = 0
    for flag, expr, needles in CASES:
        proc = subprocess.run(
            [str(HOST), "--" + flag, expr],
            cwd=str(REPO),
            text=True,
            capture_output=True,
            timeout=12,
        )
        out = proc.stdout + proc.stderr
        expected_error = "Invalid method" in needles
        ok = (proc.returncode == 0 or expected_error) and all(markers_present(out, [n]) for n in needles)
        if ok:
            print("OK", flag, expr)
            continue
        bad += 1
        print("FAIL", flag, expr)
        print(out)

    print("done mismatches", bad, "/", len(CASES))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

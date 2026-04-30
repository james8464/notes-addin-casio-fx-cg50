#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PY = sys.executable


def run_python(expr: str) -> str:
    # intProgram: mode=1 (int), then f:, then Met=1 (auto)
    stdin = f"1\n{expr}\n1\n"
    p = subprocess.run(
        [PY, "python/src/Math/intProgram.py"],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=20,
    )
    return p.stdout


def run_cpp(host: Path, expr: str) -> str:
    p = subprocess.run([str(host), "--int", expr], text=True, capture_output=True, cwd=str(REPO))
    return p.stdout


def extract_answer(text: str) -> str | None:
    for ln in text.splitlines():
        m = re.search(r"\banswer:\s*(.+)", ln, flags=re.I)
        if m:
            return m.group(1).strip()
    return None


def norm(s: str) -> str:
    s = (s or "").strip().lower()
    s = s.replace("π", "pi")
    s = re.sub(r"\s+", "", s)
    s = s.replace("*", "")
    s = s.replace("ln", "log")
    s = s.replace("|", "abs")
    return s


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    cases = [
        "x^2",
        "1/x",
        "sin(3x+2)",
        "cos(4x)",
        "exp(5x)",
        "1/(5x+7)",
        "sec(x)^2",
        "sec(x)*tan(x)",
        "cosec(x)^2",
        "cosec(x)*cot(x)",
        "tan(x)^2",
    ]

    bad = 0
    for expr in cases:
        py_out = run_python(expr)
        cpp_out = run_cpp(host, expr)
        py_ans = extract_answer(py_out)
        cpp_ans = extract_answer(cpp_out)
        ok = (py_ans is not None) and (cpp_ans is not None) and (norm(py_ans) == norm(cpp_ans))
        if not ok:
            bad += 1
            print("MISMATCH", expr)
            print("  PY :", py_ans)
            print("  C++:", cpp_ans)
            print("")
        else:
            print("OK", expr, "->", cpp_ans)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())


#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
PY = sys.executable


def run_python(mode: str, payload_lines: list[str]) -> str:
    stdin = mode + "\n" + "\n".join(payload_lines) + "\n"
    p = subprocess.run(
        [PY, "python/src/Math/deriveProgram.py"],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=20,
    )
    return p.stdout


def run_cpp(host: Path, mode: int, expr: str) -> str:
    p = subprocess.run([str(host), "--derive", f"mode:{mode},{expr}"], text=True, capture_output=True, cwd=str(REPO))
    return p.stdout


def extract_answer(text: str) -> str | None:
    # Prefer explicit Answer: lines, otherwise fall back to the last derivative label.
    last_dy = None
    last_d2 = None
    for ln in text.splitlines():
        s = ln.strip()
        m = re.search(r"\banswer:\s*(.+)", s, flags=re.I)
        if m:
            return m.group(1).strip()
        m3 = re.search(r"\bd2y/dx2\s*=\s*(.+)", s, flags=re.I)
        if m3 and not s.lower().startswith(("1.", "2.", "3.", "4.", "5.", "6.", "7.", "8.", "9.")):
            last_d2 = "d2y/dx2 = " + m3.group(1).strip()
        m2 = re.search(r"\bdy/dx\s*=\s*(.+)", s, flags=re.I)
        if m2 and not s.lower().startswith(("1.", "2.", "3.", "4.", "5.", "6.", "7.", "8.", "9.")):
            last_dy = "dy/dx = " + m2.group(1).strip()
    return last_d2 or last_dy


def norm(s: str) -> str:
    s = (s or "").strip().lower()
    s = s.replace("π", "pi")
    s = re.sub(r"\s+", "", s)
    s = s.replace("*", "")
    s = s.replace("ln", "log")
    s = s.replace("|", "abs")
    s = re.sub(r"[\[\]\(\),]", "", s)
    return s


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "addin/host/build/casio_host"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    cases = [
        # (mode, python_mode_str, python_payload_lines, cpp_expr)
        (1, "1", ["x^3"], "x^3"),
        (1, "1", ["sin(x)"], "sin(x)"),
        (1, "1", ["ln(x)"], "ln(x)"),
        (4, "4", ["x^4"], "x^4"),
        (2, "2", ["x^2+y^2=1"], "x^2+y^2=1"),
        (3, "3", ["t^2", "t^3"], "t^2,t^3,t"),
    ]

    bad = 0
    for mode, pymode, pylines, cpp_expr in cases:
        py_out = run_python(pymode, pylines)
        cpp_out = run_cpp(host, mode, cpp_expr)
        py_ans = extract_answer(py_out)
        cpp_ans = extract_answer(cpp_out)
        ok = (py_ans is not None) and (cpp_ans is not None) and (norm(py_ans) == norm(cpp_ans))
        if not ok:
            bad += 1
            print("MISMATCH", mode, cpp_expr)
            print("  PY :", py_ans)
            print("  C++:", cpp_ans)
            print("")
        else:
            print("OK", mode, cpp_expr, "->", cpp_ans)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())


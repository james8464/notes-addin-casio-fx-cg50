#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import math
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PY = sys.executable


def run_python(expr: str) -> str:
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


def _safe_eval(expr: str, x: float) -> float | None:
    expr = (expr or "").strip()
    if not expr:
        return None
    expr = expr.replace("^", "**").replace("π", "pi")
    expr = expr.replace("ln(", "log(")
    # Handle ln|x| and |x| style pipes.
    expr = re.sub(r"ln\|([^|]+)\|", r"log(abs(\1))", expr)
    expr = re.sub(r"\|([^|]+)\|", r"abs(\1)", expr)
    env = {
        "__builtins__": {},
        "pi": math.pi,
        "e": math.e,
        "sin": math.sin,
        "cos": math.cos,
        "tan": math.tan,
        "asin": math.asin,
        "acos": math.acos,
        "atan": math.atan,
        "sqrt": math.sqrt,
        "abs": abs,
        "log": math.log,
        "exp": math.exp,
        "x": x,
    }
    try:
        v = float(eval(expr, env, {}))
        if math.isfinite(v):
            return v
        return None
    except Exception:
        return None


def numeric_equiv(lhs: str, rhs: str) -> bool:
    samples = [0.2, 0.7, 1.3]
    hits = 0
    for x in samples:
        a = _safe_eval(lhs, x)
        b = _safe_eval(rhs, x)
        if a is None or b is None:
            continue
        hits += 1
        if abs(a - b) > 1e-6:
            return False
    return hits >= 1


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    cases = [
        "(3*x^2-2*x+2)/x",
        "sin(x)^2",
        "cos(x)^2",
    ]

    bad = 0
    for expr in cases:
        py_out = run_python(expr)
        cpp_out = run_cpp(host, expr)
        py_ans = extract_answer(py_out)
        cpp_ans = extract_answer(cpp_out)
        ok = (py_ans is not None) and (cpp_ans is not None) and (norm(py_ans) == norm(cpp_ans))
        if not ok and py_ans and cpp_ans:
            # Compare expressions ignoring trailing +C by numeric check.
            py_rhs = py_ans.replace("+ C", "").strip()
            cpp_rhs = cpp_ans.replace("+ C", "").strip()
            ok = numeric_equiv(py_rhs, cpp_rhs)
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


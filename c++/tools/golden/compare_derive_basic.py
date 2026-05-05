#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import math
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


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


def _safe_eval(expr: str, x: float) -> float | None:
    expr = (expr or "").strip()
    if not expr:
        return None
    expr = expr.replace("^", "**").replace("π", "pi")
    # basic canonicalization
    expr = expr.replace("ln(", "log(")
    expr = re.sub(r"ln\|([^|]+)\|", r"log(abs(\1))", expr)
    expr = re.sub(r"\|([^|]+)\|", r"abs(\1)", expr)
    # Allowed math names
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
        v = eval(expr, env, {})
        v = float(v)
        if math.isfinite(v):
            return v
        return None
    except Exception:
        return None


def numeric_equiv(lhs: str, rhs: str) -> bool:
    # Compare RHS expressions at a few safe positive points.
    # This is only a fallback for formatting differences.
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
        # (mode, cpp_expr, expected)
        (1, "x^3", "dy/dx = 3*x^2"),
        (1, "sin(x)", "dy/dx = cos(x)"),
        (1, "ln(x)", "dy/dx = 1/x"),
        (1, "x^x", "dy/dx = x^x*(log(x) + 1)"),
        (1, "sin(x)^x", "dy/dx = sin(x)^x*(log(sin(x)) + x*cos(x)/sin(x))"),
        (1, "asin(x)", "dy/dx = 1/sqrt(- x^2 + 1)"),
        (4, "x^4", "d2y/dx2 = 12*x^2"),
        (2, "x^2+y^2=1", "dy/dx = -x/y"),
        (3, "t^2,t^3,t", "dy/dx = 3*t/2"),
    ]

    bad = 0
    for mode, cpp_expr, expected in cases:
        cpp_out = run_cpp(host, mode, cpp_expr)
        cpp_ans = extract_answer(cpp_out)
        ok = (cpp_ans is not None) and (norm(expected) == norm(cpp_ans))
        if not ok and cpp_ans and ("=" in expected) and ("=" in cpp_ans):
            try:
                py_rhs = expected.split("=", 1)[1].strip()
                cpp_rhs = cpp_ans.split("=", 1)[1].strip()
                ok = numeric_equiv(py_rhs, cpp_rhs)
            except Exception:
                ok = False
        if not ok:
            bad += 1
            print("MISMATCH", mode, cpp_expr)
            print("  WANT:", expected)
            print("  C++:", cpp_ans)
            print("")
        elif "^x" in cpp_expr or "x^x" in cpp_expr or "sin(x)^x" in cpp_expr:
            low = cpp_out.lower()
            if "logdiff" not in low and "log diff" not in low and "var power" not in low:
                bad += 1
                print("WEAK WORKING", mode, cpp_expr)
                print("  missing variable-power/logdiff route")
                print(cpp_out)
        else:
            print("OK", mode, cpp_expr, "->", cpp_ans)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

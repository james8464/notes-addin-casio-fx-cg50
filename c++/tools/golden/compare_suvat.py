#!/usr/bin/env python3
"""
SUVAT compare: Python oracle vs C++ host.

We compare the *exact* answer line: "<target> = <expr>" (first occurrence).
"""

from __future__ import annotations

import argparse
import math
import re
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PY = sys.executable


def run_python(s: str, u: str, v: str, a: str, t: str) -> str:
    stdin = "s: \n"
    # actual program expects no labels, just values per prompt
    stdin = f"{s}\n{u}\n{v}\n{a}\n{t}\n"
    p = subprocess.run(
        [PY, "python/src/Math/SUVATprogram.py"],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=20,
    )
    return p.stdout


def run_cpp(host_bin: Path, s: str, u: str, v: str, a: str, t: str, target: str) -> str:
    # Host CLI uses comma-separated k=v pairs, so don't use "," markers inside values.
    kv = f"s={s},u={u},v={v},a={a},t={t},target={target}"
    p = subprocess.run([str(host_bin), "--suvat", kv], capture_output=True, text=True, cwd=str(REPO))
    return p.stdout


def extract_exact_answer(text: str, target: str) -> str | None:
    # Match lines like: "v = 10" or "t = ( ... )"
    unit_re = re.compile(r"\s+(m/s\^2|m/s|m|s)\s*$")
    for ln in text.splitlines():
        m = re.match(rf"^\s*{re.escape(target)}\s*=\s*(.+?)\s*$", ln)
        if m:
            val = m.group(1).strip()
            # Skip formula helper lines that contain ±.
            if "±" in val:
                continue
            val = unit_re.sub("", val).strip()
            return val
    return None


def numeric_value(expr: str) -> float | None:
    expr = expr.strip().replace("^", "**")
    try:
        return float(eval(expr, {"__builtins__": {}}, {"sqrt": math.sqrt, "pi": math.pi}))
    except Exception:
        return None


def equivalent(expected: str | None, actual: str | None) -> bool:
    if expected == actual:
        return True
    if expected is None or actual is None:
        return False
    ev = numeric_value(expected)
    av = numeric_value(actual)
    if ev is None or av is None:
        return False
    return math.isclose(ev, av, rel_tol=5e-3, abs_tol=5e-3)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    args = ap.parse_args()
    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host}")

    cases = [
        # (s,u,v,a,t,target,expected_override)
        ("10", "0", "", "2", "5", "v", None),
        ("", "0", "10", "2", "5", "s", None),
        ("10", "0", "", "2", "", "v", "sqrt(40) or -sqrt(40)"),
        ("10", "0", "", "2", "", "t", "sqrt(40)/2"),
    ]

    bad = 0
    for s,u,v,a,t,target,expected in cases:
        py_out = run_python(s,u,v,a,t)
        cpp_out = run_cpp(host, s,u,v,a,t,target)
        py_ans = extract_exact_answer(py_out, target)
        cpp_ans = extract_exact_answer(cpp_out, target) or extract_exact_answer(cpp_out, "Answer: "+target)
        ok = equivalent(expected, cpp_ans) if expected is not None else equivalent(py_ans, cpp_ans)
        if not ok:
            bad += 1
            print("MISMATCH", (s,u,v,a,t,target))
            print("PY:", py_ans)
            print("C :", cpp_ans)
        else:
            print("OK", target, "=", py_ans)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

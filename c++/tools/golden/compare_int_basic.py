#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


def run_cpp(host: Path, expr: str) -> str:
    p = subprocess.run([str(host), "--int", expr], text=True, capture_output=True, cwd=str(REPO))
    return p.stdout


def extract_answer(text: str) -> str | None:
    lines = [ln.strip() for ln in text.splitlines() if ln.strip()]
    for ln in text.splitlines():
        m = re.search(r"\banswer:\s*(.+)", ln, flags=re.I)
        if m:
            return m.group(1).strip()
    if lines:
        return lines[-1]
    return None


def norm(s: str) -> str:
    s = (s or "").strip().lower()
    s = s.replace("π", "pi")
    s = re.sub(r"ln\|([^|]+)\|", r"log(abs(\1))", s)
    s = re.sub(r"\s+", "", s)
    s = s.replace("*", "")
    s = s.replace("ln", "log")
    s = re.sub(r"logabs([^+/-]+)", r"log(abs(\1))", s)
    s = s.replace("|", "")
    return s


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    cases = [
        ("x^2", "x^3/3 + C"),
        ("1/x", "log(abs(x)) + C"),
        ("sin(3x+2)", "-1/3*cos(3*x + 2) + C"),
        ("cos(4x)", "sin(4*x)/4 + C"),
        ("exp(5x)", "e^(5*x)/5 + C"),
        ("1/(5x+7)", "log(abs(5*x + 7))/5 + C"),
        ("sec(x)^2", "tan(x) + C"),
        ("sec(x)*tan(x)", "sec(x) + C"),
        ("cosec(x)^2", "-cot(x) + C"),
        ("cosec(x)*cot(x)", "-cosec(x) + C"),
        ("tan(x)^2", "tan(x) - x + C"),
    ]

    bad = 0
    for expr, expected in cases:
        cpp_out = run_cpp(host, expr)
        cpp_ans = extract_answer(cpp_out)
        ok = (cpp_ans is not None) and (norm(expected) == norm(cpp_ans))
        if not ok:
            bad += 1
            print("MISMATCH", expr)
            print("  WANT:", expected)
            print("  C++:", cpp_ans)
            print("")
        else:
            print("OK", expr, "->", cpp_ans)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
PY = sys.executable


def run_python(script_relpath: str, stdin: str) -> str:
    p = subprocess.run(
        [PY, f"python/{script_relpath}"],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=20,
    )
    return p.stdout


def run_cpp(host: Path, mode: str, expr: str) -> str:
    flag = {"int": "--int", "alg": "--alg", "trig": "--trig"}[mode]
    p = subprocess.run([str(host), flag, expr], text=True, capture_output=True, cwd=str(REPO))
    return p.stdout


def extract_answer_lines(text: str) -> list[str]:
    out: list[str] = []
    for ln in text.splitlines():
        s = ln.strip()
        if s.lower().startswith("answer:"):
            out.append(s)
    return out


def extract_alg_solutions(text: str) -> list[str]:
    sols: list[str] = []
    for ln in text.splitlines():
        s = ln.strip()
        if re.match(r"^x\s*=\s*.+", s):
            sols.append(re.sub(r"\s+", " ", s))
    return sols


def _safe_eval_number(expr: str) -> float | None:
    expr = (expr or "").strip()
    if not expr:
        return None
    expr = expr.replace("^", "**").replace("π", "pi")
    try:
        import math

        return float(eval(expr, {"__builtins__": {}}, {"sqrt": math.sqrt, "pi": math.pi}))
    except Exception:
        return None


def extract_alg_roots_numeric(text: str) -> list[float] | None:
    # Match "Answer: x = [a, b]" anywhere, or fall back to collecting "x = <expr>" lines.
    m = re.search(r"answer:\s*x\s*=\s*\[(.+?)\]", text, flags=re.I | re.S)
    inside = None
    if m:
        inside = m.group(1).strip()
        parts: list[str] = []
        cur = ""
        depth = 0
        for ch in inside:
            if ch in "([":  # nested
                depth += 1
            elif ch in ")]":
                depth = max(0, depth - 1)
            if ch == "," and depth == 0:
                parts.append(cur.strip())
                cur = ""
            else:
                cur += ch
        if cur.strip():
            parts.append(cur.strip())
        vals: list[float] = []
        for p in parts:
            v = _safe_eval_number(p)
            if v is None:
                return None
            vals.append(v)
        vals.sort()
        return vals

    vals2: list[float] = []
    for ln in text.splitlines():
        s = ln.strip()
        m2 = re.match(r"^x\s*=\s*(.+?)\s*$", s)
        if m2:
            v = _safe_eval_number(m2.group(1))
            if v is None:
                return None
            vals2.append(v)
    if not vals2:
        return None
    vals2.sort()
    return vals2


def map_fixture_to_host(script_relpath: str, stdin: str) -> tuple[str, str] | None:
    """
    Convert the python program stdin payload to our host CLI input.
    Only handles the modes we currently implemented.
    """
    lines = [ln.rstrip("\n") for ln in (stdin or "").splitlines()]
    if script_relpath.endswith("Math/intProgram.py"):
        if not lines:
            return None
        if lines[0].strip() != "1":
            return None  # only indefinite integration for now
        if len(lines) < 2:
            return None
        expr = lines[1].strip()
        if not re.fullmatch(r"[0-9a-zA-Z_πpiPI\\+\\-\\*/\\^\\(\\)\\.\\s]+", expr):
            return None
        if " " in expr:
            return None
        return ("int", expr)

    if script_relpath.endswith("Math/algebraProgram.py"):
        # Our port currently supports direct solve equations (not other menu modes).
        # The python program uses mode "6" for solve equations.
        if not lines:
            return None
        if lines[0].strip() != "6":
            return None
        if len(lines) < 2:
            return None
        expr = lines[1].strip()
        if " " in expr:
            return None
        return ("alg", expr)

    if script_relpath.endswith("Math/trigProgram.py"):
        if not lines:
            return None
        if lines[0].strip() != "3":
            return None  # only solve mode for now
        if len(lines) < 2:
            return None
        expr = lines[1].strip()
        if " " in expr:
            return None
        return ("trig", expr)

    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cases", required=True, help="JSONL cases from generate_fixtures.py")
    ap.add_argument("--host", default=str(REPO / "addin/host/build/casio_host"))
    ap.add_argument("--limit", type=int, default=200)
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    bad = 0
    tried = 0

    with Path(args.cases).open("r", encoding="utf-8") as f:
        for line in f:
            if args.limit is not None and tried >= args.limit:
                break
            row = json.loads(line)
            script = row.get("script_relpath", "")
            stdin = row.get("stdin", "")
            mapped = map_fixture_to_host(script, stdin)
            if not mapped:
                continue
            mode, expr = mapped

            tried += 1
            py_out = run_python(script, stdin)
            cpp_out = run_cpp(host, mode, expr)

            if mode == "alg":
                py_roots = extract_alg_roots_numeric(py_out)
                cpp_roots = extract_alg_roots_numeric(cpp_out)
                py_ans = extract_alg_solutions(py_out)
                cpp_ans = extract_alg_solutions(cpp_out)
            else:
                py_ans = extract_answer_lines(py_out)
                cpp_ans = extract_answer_lines(cpp_out)

            # Compare first answer line only for now.
            if mode == "alg":
                ok = False
                if py_roots is not None and cpp_roots is not None and len(py_roots) == len(cpp_roots):
                    ok = all(abs(a - b) < 1e-6 for a, b in zip(py_roots, cpp_roots))
                if not ok and py_ans:
                    ok = (sorted(py_ans) == sorted(cpp_ans))
            else:
                ok = (py_ans[:1] == cpp_ans[:1]) and bool(py_ans)
            if not ok:
                bad += 1
                print("MISMATCH", mode, expr)
                print("  PY :", (py_ans[:3] or ["<no answer>"])[0])
                print("  C++:", (cpp_ans[:3] or ["<no answer>"])[0])
                print("")
            else:
                if mode == "alg":
                    msg = py_ans[0] if py_ans else "roots ok"
                    print("OK", mode, expr, "->", msg)
                else:
                    print("OK", mode, expr, "->", py_ans[0])

    print(f"Done. tried={tried} mismatches={bad}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())


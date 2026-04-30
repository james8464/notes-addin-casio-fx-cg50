#!/usr/bin/env python3
from __future__ import annotations

"""
Randomized failure miner: Python oracle vs C++ host.

Generates program stdin payloads (derive/int/algebra/trig), runs:
  - Python: python/src/Math/<Program>.py
  - C++:   addin/host/build/casio_host --stdin-program <Program>.py

Writes JSONL of failures (parse errors, mismatched answers, missing working).
"""

import argparse
import json
import os
import random
import re
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
PY = sys.executable
HOST_DEFAULT = REPO / "addin" / "host" / "build" / "casio_host"

WORK_MARKERS = ("method:", "use ", "let ", "solve", "implicit", "parametric", "rewrite", "simplify")


def _norm_compact(s: str) -> str:
    s = (s or "").strip().lower()
    s = s.replace("π", "pi")
    s = re.sub(r"\s+", "", s)
    s = s.replace("*", "")
    s = s.replace("ln", "log")
    s = s.replace("|", "abs")
    return s


def _extract_answer_lines(text: str) -> list[str]:
    out: list[str] = []
    for ln in (text or "").splitlines():
        m = re.search(r"\banswer:\s*.+", ln.strip(), flags=re.I)
        if m:
            out.append(m.group(0).strip())
    return out


def _has_working(text: str) -> bool:
    t = (text or "").lower()
    return any(m in t for m in WORK_MARKERS) and ("answer:" in t)


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


def _extract_answer_list_numeric(text: str) -> list[float] | None:
    m = re.search(r"answer:\s*[a-zA-Zθ]+\s*=\s*\[(.+?)\]", text, flags=re.I | re.S)
    if not m:
        return None
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


def _extract_alg_roots_numeric(text: str) -> list[float] | None:
    m = re.search(r"answer:\s*x\s*=\s*\[(.+?)\]", text, flags=re.I | re.S)
    if not m:
        return None
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


def run_python(program_py: str, stdin: str, timeout_s: float) -> tuple[int, str, str]:
    p = subprocess.run(
        [PY, f"python/src/Math/{program_py}"],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=timeout_s,
    )
    return p.returncode, p.stdout, p.stderr


def run_cpp(host: Path, program_py: str, stdin: str, timeout_s: float) -> tuple[int, str, str]:
    p = subprocess.run(
        [str(host), "--stdin-program", program_py],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=timeout_s,
    )
    return p.returncode, p.stdout, p.stderr


def rand_int(lo: int, hi: int) -> int:
    return random.randint(lo, hi)


def rand_rat() -> str:
    if random.random() < 0.7:
        return str(rand_int(-9, 9) or 1)
    a = rand_int(-9, 9) or 1
    b = rand_int(1, 9)
    return f"{a}/{b}"


FUNS = ["sin", "cos", "tan", "sqrt", "ln", "exp", "abs"]


def rand_atom(var: str = "x") -> str:
    r = random.random()
    if r < 0.35:
        return var
    if r < 0.7:
        return rand_rat()
    if r < 0.8:
        return "pi"
    f = random.choice(FUNS)
    return f"{f}({rand_expr(var, depth=1)})"


def rand_expr(var: str = "x", depth: int = 0) -> str:
    if depth > 3:
        return rand_atom(var)
    r = random.random()
    if r < 0.25:
        return rand_atom(var)
    if r < 0.55:
        op = random.choice(["+", "-", "*", "/"])
        return f"({rand_expr(var, depth+1)}{op}{rand_expr(var, depth+1)})"
    if r < 0.75:
        base = rand_atom(var)
        exp = random.choice(["2", "3", rand_rat()])
        return f"({base})^{exp}"
    # small polynomial-like
    a = rand_int(-5, 5) or 1
    b = rand_int(-9, 9)
    c = rand_int(-9, 9)
    return f"({a}*{var}^2+{b}*{var}+{c})"


def rand_equation(var: str = "x") -> str:
    # Keep solvable-ish: quadratic or rational quadratic sometimes.
    if random.random() < 0.75:
        a = rand_int(-5, 5) or 1
        b = rand_int(-9, 9)
        c = rand_int(-9, 9)
        rhs = rand_int(-9, 9)
        return f"{a}*{var}^2+{b}*{var}+{c}={rhs}"
    # rational: (ax+b)/(cx+d) = k
    a = rand_int(-5, 5) or 1
    b = rand_int(-9, 9)
    c = rand_int(-5, 5) or 1
    d = rand_int(-9, 9)
    k = rand_rat()
    return f"({a}*{var}+{b})/({c}*{var}+{d})={k}"


@dataclass
class Failure:
    ts: float
    kind: str  # crash | mismatch | missing_working
    program: str
    stdin: str
    py_rc: int
    cpp_rc: int
    py_answer: str | None
    cpp_answer: str | None
    py_stdout: str
    cpp_stdout: str
    py_stderr: str
    cpp_stderr: str


def compare(program: str, py_out: str, cpp_out: str) -> bool:
    if program == "algebraProgram.py":
        py_roots = _extract_alg_roots_numeric(py_out)
        cpp_roots = _extract_alg_roots_numeric(cpp_out)
        if py_roots is not None and cpp_roots is not None and len(py_roots) == len(cpp_roots):
            return all(abs(a - b) < 1e-6 for a, b in zip(py_roots, cpp_roots))
    if program == "trigProgram.py":
        py_list = _extract_answer_list_numeric(py_out)
        cpp_list = _extract_answer_list_numeric(cpp_out)
        if py_list is not None and cpp_list is not None and len(py_list) == len(cpp_list):
            return all(abs(a - b) < 1e-6 for a, b in zip(py_list, cpp_list))
    py_ans = _extract_answer_lines(py_out)
    cpp_ans = _extract_answer_lines(cpp_out)
    if not py_ans or not cpp_ans:
        return False
    return _norm_compact(py_ans[0]) == _norm_compact(cpp_ans[0])


def gen_case(program: str) -> str:
    if program == "intProgram.py":
        expr = rand_expr("x")
        return f"1\n{expr}\n1\n"
    if program == "deriveProgram.py":
        expr = rand_expr("x")
        return f"1\n{expr}\n"
    if program == "algebraProgram.py":
        eq = rand_equation("x")
        return f"6\n{eq}\n"
    if program == "trigProgram.py":
        # trig solve input: eq,var,lo,hi
        # keep simple trig eq = c
        fn = random.choice(["sin", "cos", "tan"])
        c = random.choice(["0", "1/2", "-1/2", "1", "-1"])
        lo, hi = random.choice([("0", "360"), ("0", "2*pi")])
        expr = f"{fn}(x)={c},x,{lo},{hi}"
        return f"3\n{expr}\n"
    raise ValueError(program)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(HOST_DEFAULT))
    ap.add_argument("--out", default=str(REPO / "tools/fuzz/fuzz_failures.jsonl"))
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--n", type=int, default=200)
    ap.add_argument("--timeout-s", type=float, default=float(os.environ.get("CASIO_FUZZ_TIMEOUT", "8")))
    ap.add_argument("--programs", default="int,alg,trig,derive", help="comma: int,alg,trig,derive")
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host binary: {host} (build with ./tools/build_host.sh)")

    random.seed(args.seed)
    prog_map = {"int": "intProgram.py", "alg": "algebraProgram.py", "trig": "trigProgram.py", "derive": "deriveProgram.py"}
    progs = [prog_map[p.strip()] for p in args.programs.split(",") if p.strip() in prog_map]
    if not progs:
        raise SystemExit("No programs selected.")

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    bad = 0
    with out_path.open("w", encoding="utf-8") as f:
        for _ in range(args.n):
            program = random.choice(progs)
            stdin = gen_case(program)

            try:
                py_rc, py_stdout, py_stderr = run_python(program, stdin, args.timeout_s)
            except subprocess.TimeoutExpired as e:
                py_rc, py_stdout, py_stderr = 124, e.stdout or "", f"{e.stderr or ''}\nTimeout"
            try:
                cpp_rc, cpp_stdout, cpp_stderr = run_cpp(host, program, stdin, args.timeout_s)
            except subprocess.TimeoutExpired as e:
                cpp_rc, cpp_stdout, cpp_stderr = 124, e.stdout or "", f"{e.stderr or ''}\nTimeout"

            ok = (py_rc == 0 and cpp_rc == 0 and compare(program, py_stdout, cpp_stdout))
            missing_work = (cpp_rc == 0 and not _has_working(cpp_stdout))

            if (not ok) or missing_work:
                bad += 1
                py_ans = (_extract_answer_lines(py_stdout) or [None])[0]
                cpp_ans = (_extract_answer_lines(cpp_stdout) or [None])[0]
                kind = "missing_working" if missing_work and ok else ("crash" if (cpp_rc != 0 or py_rc != 0) else "mismatch")
                row = Failure(
                    ts=time.time(),
                    kind=kind,
                    program=program,
                    stdin=stdin,
                    py_rc=py_rc,
                    cpp_rc=cpp_rc,
                    py_answer=py_ans,
                    cpp_answer=cpp_ans,
                    py_stdout=py_stdout[-8000:],
                    cpp_stdout=cpp_stdout[-8000:],
                    py_stderr=py_stderr[-2000:],
                    cpp_stderr=cpp_stderr[-2000:],
                )
                f.write(json.dumps(asdict(row), ensure_ascii=False) + "\n")

    print(f"Wrote {bad} failures to {out_path}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())


#!/usr/bin/env python3
"""
Run the legacy/exam-style question bank stored in Tests.rtf.

Strategy:
- Convert Tests.rtf -> plain text using macOS textutil (available on Darwin).
- Parse the plain text into blocks starting with "Question ... — <Program>".
- Map each block to a program+mode+input payload when possible.
- Otherwise, mark the block as "unsupported mapping" (this is distinct from the
  underlying program being unable to solve something).

This runner is intentionally conservative: it will only claim PASS when it can
extract an expected answer and verify it appears in the output (or matches
numerically when the expected answer is a number).
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, List, Tuple


ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parent
TESTS_RTF = ROOT / "Tests.rtf"
TOP_TESTS_TXT = REPO / "Tests.txt"


QUESTION_RE = re.compile(r"^Question\s+(.+?)\s+—\s+(.+?)(?:\s+\((.+)\))?\s*$")


@dataclass
class QuestionBlock:
    header_line: str
    qid: str
    program: str
    subtopic: str
    lines: List[str]

    expected_answer_text: Optional[str] = None


def _run(cmd: List[str], *, cwd: Path, timeout_s: int = 15, input_text: str = "") -> Tuple[int, str]:
    p = subprocess.run(
        cmd,
        input=(input_text or "").encode("utf-8"),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(cwd),
        timeout=timeout_s,
    )
    out = p.stdout.decode("utf-8", errors="replace")
    return p.returncode, out


def _cpp_host() -> Path:
    configured = os.environ.get("CASIO_CPP_HOST", "").strip()
    if configured:
        return Path(configured).expanduser()
    return REPO / "c++" / "addin" / "host" / "build" / "casio_host"


def _ensure_cpp_host(timeout_s: int) -> Optional[str]:
    host = _cpp_host()
    if host.exists():
        return None
    build = REPO / "c++" / "tools" / "build_host.sh"
    if not build.exists():
        return f"C++ host missing and build script not found: {build}"
    code, out = _run([str(build)], cwd=REPO, timeout_s=max(timeout_s, 60))
    if code != 0 or not host.exists():
        return "C++ host build failed:\n" + out
    return None


def _run_mapped_program(script: str, payload: str, timeout_s: int) -> Tuple[int, str]:
    backend = os.environ.get("CASIO_BACKEND", "").strip().lower()
    if backend in ("c", "cpp", "c++"):
        err = _ensure_cpp_host(timeout_s)
        if err:
            return 2, err
        return _run(
            [str(_cpp_host()), "--stdin-program", Path(script).name],
            cwd=REPO,
            timeout_s=timeout_s,
            input_text=payload,
        )
    return _run([sys.executable, str(ROOT / script)], cwd=ROOT, timeout_s=timeout_s, input_text=payload)


def _safe_eval_number(expr: str) -> Optional[float]:
    expr = (expr or "").strip()
    if not expr:
        return None
    expr = expr.replace("^", "**").replace("π", "pi")
    try:
        import math
        return float(eval(expr, {"__builtins__": {}}, {"sqrt": math.sqrt, "pi": math.pi}))
    except Exception:
        return None


def _extract_answer_roots(output: str) -> Optional[List[float]]:
    # Expect something like: "Answer: x = [a, b]"
    m = re.search(r"answer:\s*x\s*=\s*\[(.+?)\]", output, flags=re.I | re.S)
    if not m:
        return None
    inside = m.group(1).strip()
    parts = []
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
    vals = []
    for p in parts:
        v = _safe_eval_number(p)
        if v is None:
            return None
        vals.append(v)
    vals.sort()
    return vals


def convert_rtf_to_txt(rtf_path: Path, txt_path: Path) -> None:
    if sys.platform != "darwin":
        raise RuntimeError("This converter expects macOS textutil (darwin).")
    cmd = ["textutil", "-convert", "txt", str(rtf_path), "-output", str(txt_path)]
    code, out = _run(cmd, cwd=rtf_path.parent, timeout_s=30)
    if code != 0:
        raise RuntimeError("textutil failed:\n" + out)


def split_blocks(lines: List[str]) -> List[QuestionBlock]:
    blocks: List[QuestionBlock] = []
    cur: Optional[QuestionBlock] = None

    def flush():
        nonlocal cur
        if cur is not None:
            blocks.append(cur)
        cur = None

    for line in lines:
        m = QUESTION_RE.match(line.strip())
        if m:
            flush()
            qid = m.group(1).strip()
            program = m.group(2).strip()
            subtopic = (m.group(3) or "").strip()
            cur = QuestionBlock(header_line=line.strip(), qid=qid, program=program, subtopic=subtopic, lines=[line.rstrip("\n")])
        else:
            if cur is not None:
                cur.lines.append(line.rstrip("\n"))
    flush()
    return blocks


def extract_expected_answer(block: QuestionBlock) -> Optional[str]:
    # Prefer explicit "Answer:" lines.
    for ln in block.lines:
        s = ln.strip()
        if s.lower().startswith("answer:"):
            return s.split(":", 1)[1].strip()
    # Then "∴" lines.
    for ln in block.lines:
        s = ln.strip()
        if s.startswith("∴"):
            return s[1:].strip()
    return None


def _eval_newton_consistency(eq_expr: str, expected_x: float) -> Optional[float]:
    """
    Quick numeric check: evaluate eq_expr at expected_x.
    eq_expr should be an expression equal to 0.
    Returns f(x) value or None if unsafe.
    """
    # Very small, intentionally limited evaluator.
    allowed = {"x": expected_x}
    try:
        expr = eq_expr.replace("^", "**")
        # Basic implicit multiplication fixes for simple Newton bank entries.
        expr = re.sub(r"(\d)\s*x\b", r"\1*x", expr)
        expr = re.sub(r"\)\s*x\b", r")*x", expr)
        return float(eval(expr, {"__builtins__": {}}, allowed))
    except Exception:
        return None


def map_to_program_input(block: QuestionBlock) -> Optional[Tuple[str, str]]:
    """
    Returns (script_relpath, stdin_payload) for program invocation.
    """
    prog = block.program.lower()
    topic = (block.subtopic or "").lower()

    # Algebra Newton-Raphson
    if "algebra" in prog and ("newton" in topic or "newton-raphson" in topic):
        # Expect a line like: "Solve x^10 + 5x = 449 ..."
        eq = None
        x0 = None
        steps = 2
        for ln in block.lines:
            m = re.search(r"solve\s+(.+?)\s*=\s*(.+?)\s+using\s+newton", ln, flags=re.I)
            if m:
                left = m.group(1).strip()
                right = m.group(2).strip()
                eq = f"{left} - ({right})"
            m2 = re.search(r"starting\s+with\s+x0\s*=\s*([0-9.]+)", ln, flags=re.I)
            if m2:
                x0 = m2.group(1)
            m3 = re.search(r"using\s+newton-raphson\s+(\w+)\s+times", ln, flags=re.I)
            if m3:
                try:
                    steps = int(m3.group(1))
                except Exception:
                    pass
        if eq and x0:
            payload = "12\n{eq}, {x0}, {steps}\n".format(eq=eq, x0=x0, steps=steps)
            return "src/Math/algebraProgram.py", payload

    # Algebra inverse functions
    if "algebra" in prog and "inverse" in topic:
        # Lines often include "Given f(x)=..., x<2."
        f = None
        for ln in block.lines:
            m = re.search(r"given\s+f\(x\)\s*=\s*(.+?)(?:,\s*(.+))?\.?$", ln, flags=re.I)
            if m:
                f = m.group(1).strip()
                dom = (m.group(2) or "").strip()
                if dom:
                    f = f + ", " + dom
                break
        if f:
            payload = "8\n{f}\n\n".format(f=f)
            return "src/Math/algebraProgram.py", payload

    # Algebra solve equations
    if "algebra" in prog:
        expected = (block.expected_answer_text or "").lower().replace(" ", "")
        want_x = "x=" in expected
        # Avoid stealing trig equations; those belong in trigProgram.
        blob = "\n".join(block.lines[:20]).lower()
        has_trig = any(fn in blob for fn in ("sin", "cos", "tan", "sec", "cosec", "cot"))
        # Do not try to "solve" worked series/GP narratives; not a supported algebra mode.
        if any(key in blob for key in ("g.p", "gp", "geometric progression", "convergent", "...")):
            want_x = False
        if want_x and not has_trig:
            candidates = []
            for ln in block.lines:
                s = ln.strip()
                low = s.lower()
                if "=" not in s:
                    continue
                if "..." in s:
                    continue
                if "∴" in s:
                    continue
                if "dy/dx" in low:
                    continue
                if low.startswith(("answer:", "working:", "domain", "range")):
                    continue
                # De-prioritize function definitions like f(x)=...
                score = 0
                if "f(" in low:
                    score -= 3
                if "=0" in low or ")=0" in low:
                    score += 3
                if low.startswith(("solve", "hence", "therefore")):
                    score -= 1
                # Prefer shorter, solve-like lines.
                score -= min(len(s), 120) / 120.0
                candidates.append((score, s))
            candidates.sort(reverse=True)
            eq_line = candidates[0][1] if candidates else None
            if eq_line:
                payload = "6\n{eq}\n".format(eq=eq_line)
                return "src/Math/algebraProgram.py", payload
    if "algebra" in prog and ("solve" in topic or "solve equation" in prog):
        eq_line = None
        for ln in block.lines:
            s = ln.strip()
            if "=" in s and "dy/dx" not in s.lower() and not s.lower().startswith(("answer:", "working:", "domain", "range")):
                eq_line = s
                break
        if eq_line:
            payload = "6\n{eq}\n".format(eq=eq_line)
            return "src/Math/algebraProgram.py", payload

    # Implicit differentiation
    if "implicit differentiation" in prog or "implicit" in topic:
        eq_line = None
        point = None
        for ln in block.lines:
            s = ln.strip()
            if eq_line is None and "=" in s and "dy/dx" not in s.lower():
                eq_line = s
            m = re.search(r"at\s*\(?\s*([0-9.+-]+)\s*,\s*([0-9.+-]+)\s*\)?\s*:", s, flags=re.I)
            if m:
                point = (m.group(1), m.group(2))
        if eq_line:
            # deriveProgram implicit mode expects:
            # 2\n<equation>\n<point or blank>\n
            pt_line = ""
            if point is not None:
                pt_line = "{},{}\n".format(point[0], point[1])
            payload = "2\n{eq}\n{pt}".format(eq=eq_line, pt=pt_line)
            return "src/Math/deriveProgram.py", payload

    # Differential equations (integration program mode 2)
    if "differential equations" in prog or prog.strip().lower() == "differential equations":
        # Expect a line like: "Solve (1+x)dy/dx = y(1-x), y=1 when x=0."
        rhs = None
        bc = None
        for ln in block.lines:
            m = re.search(r"solve\s+(.+?)\s*,\s*(.+?)\.?$", ln, flags=re.I)
            if m and "dy/dx" in m.group(1):
                eq = m.group(1).strip()
                ic = m.group(2).strip()
                # Convert "A*dy/dx = B" -> dy/dx = B/A for common simple patterns.
                eq2 = eq.replace(" ", "")
                # Try to split at '='
                def fix_mul(s: str) -> str:
                    # minimal implicit-multiplication fixer for DE RHS strings
                    s = re.sub(r"([a-zA-Z])\(", r"\1*(", s)
                    s = s.replace(")(", ")*(")
                    return s

                if "=" in eq2:
                    left, right = eq2.split("=", 1)
                    if "dy/dx" in left:
                        mult = left.replace("dy/dx", "")
                        mult = mult.strip("*")
                        if mult == "":
                            rhs = fix_mul(right)
                        else:
                            rhs = fix_mul(f"({right})/({mult})")
                # Boundary condition: "y=1 when x=0" -> "x=0,y=1"
                m2 = re.search(r"y\s*=\s*([^\s]+)\s*when\s*x\s*=\s*([^\s]+)", ic, flags=re.I)
                if m2:
                    yv = m2.group(1).strip()
                    xv = m2.group(2).strip()
                    bc = f"x={xv},y={yv}"
                else:
                    bc = ic
                break
        if rhs:
            # intProgram expects:
            # 2\n<dy/dx RHS>\n<BC>\n
            payload = "2\n{rhs}\n{bc}\n".format(rhs=rhs, bc=(bc or ""))
            return "src/Math/intProgram.py", payload

    # Integration (indefinite) - try to find an integrand
    if "integration" in prog and "parametric" not in prog and "area" not in prog:
        integrand = None
        for ln in block.lines:
            s = ln.strip()
            m = re.search(r"integrate\s+(.+?)\s*dx\.?$", s, flags=re.I)
            if m:
                integrand = m.group(1).strip()
                break
            m2 = re.search(r"integrate[:\s]+(.+)$", s, flags=re.I)
            if m2 and "dx" not in s.lower():
                integrand = m2.group(1).strip()
                break
        if integrand:
            payload = "1\n{f}\n1\n".format(f=integrand)
            return "src/Math/intProgram.py", payload

    # Integration parametric area: map only when x=... and y=... are provided
    if "integration" in prog and "parametric area" in topic:
        xexpr = None
        yexpr = None
        for ln in block.lines:
            s = ln.strip()
            mx = re.search(r"^x\s*=\s*(.+)$", s, flags=re.I)
            if mx:
                xexpr = mx.group(1).strip()
            my = re.search(r"^y\s*=\s*(.+)$", s, flags=re.I)
            if my:
                yexpr = my.group(1).strip()
        if xexpr and yexpr:
            payload = "3\n{x}\n{y}\n\n".format(x=xexpr, y=yexpr)
            return "src/Math/intProgram.py", payload

    # Trigonometry solve - detect common interval lines when present
    if "trigonometry" in prog and ("equation" in topic or "solve" in topic or topic == "" or "solve + sketch" in topic):
        expected = block.expected_answer_text or ""
        low_expected = expected.lower().replace(" ", "")
        # Only map when the expected answer is clearly a solution set for one variable
        # (e.g. "x=..." or "θ=..."). Worked multi-variable derivations like "tany=..."
        # or ordered pairs should remain unmapped.
        if not ("x=" in low_expected or "θ=" in low_expected or "theta=" in low_expected):
            return None
        eq = None
        var = "x"
        lower = None
        upper = None
        for ln in block.lines:
            s = ln.strip()
            if not s:
                continue
            m_int = re.search(r"0\s*≤\s*([a-zA-Z]+)\s*≤\s*2\s*π", s)
            if m_int:
                var = m_int.group(1)
                lower = "0"
                upper = "2*pi"
            m_deg = re.search(r"0\s*≤\s*([a-zA-Z]+)\s*<\s*360", s)
            if m_deg:
                var = m_deg.group(1)
                lower = "0"
                upper = "360"
            if eq is None and ("=" in s):
                low = s.lower()
                if "dy/dx" in low:
                    continue
                if low.startswith(("answer:", "∴")):
                    continue
                if any(fn in low for fn in ("sin", "cos", "tan", "sec", "cosec", "cot")):
                    eq = s
        if eq:
            solve_text = eq
            if lower is not None and upper is not None:
                solve_text = "{eq}, {var}, {lo}, {hi}".format(eq=eq, var=var, lo=lower, hi=upper)
            payload = "3\n{t}\n".format(t=solve_text)
            return "src/Math/trigProgram.py", payload

    # Trigonometry identity proof (trig program prove mode)
    if "trigonometry" in prog and "identity" in topic:
        # Only map when the block actually provides two sides to compare.
        # Many papers store just the final relationship (result of a solve),
        # which is not a "prove identity" input.
        lhs = None
        rhs = None
        for ln in block.lines:
            s = ln.strip()
            if "=" in s and not s.lower().startswith(("answer:", "∴")):
                parts = s.split("=", 1)
                lhs = parts[0].strip()
                rhs = parts[1].strip()
                break
        if lhs and rhs:
            # trigProgram prove mode expects:
            # 1\n<E1>\n<E2>\n<route>\n
            payload = "1\n{lhs}\n{rhs}\n1\n".format(lhs=lhs, rhs=rhs)
            return "src/Math/trigProgram.py", payload

    return None


def normalize(text: str) -> str:
    return re.sub(r"\s+", " ", (text or "")).strip().lower()


def normalize_compact(text: str) -> str:
    # For answer substring matching: ignore whitespace and common unicode spacing.
    compact = re.sub(r"\s+", "", (text or "")).strip().lower()
    compact = compact.replace("*", "")
    compact = compact.replace("π", "pi")
    compact = compact.replace("theta", "θ")
    # Drop list/tuple punctuation so "x=pi,5pi/3" matches "x=[pi,(5*pi)/3]".
    compact = re.sub(r"[\[\]\(\),]", "", compact)
    return compact


def main() -> None:
    default_txt = TOP_TESTS_TXT if TOP_TESTS_TXT.exists() else Path("/tmp/Tests.txt")
    txt_path = Path(os.environ.get("CASIO_TESTS_TXT", str(default_txt)))
    if not txt_path.exists():
        convert_rtf_to_txt(TESTS_RTF, txt_path)

    raw = txt_path.read_text(encoding="utf-8", errors="replace")
    lines = raw.splitlines()
    blocks = split_blocks(lines)
    if not blocks:
        print("No question blocks found.")
        raise SystemExit(2)

    # Attach expected answers.
    for b in blocks:
        b.expected_answer_text = extract_expected_answer(b)

    timeout_s = int(os.environ.get("CASIO_RTF_TIMEOUT", "15"))
    limit = os.environ.get("CASIO_RTF_LIMIT", "").strip()
    max_blocks = int(limit) if limit.isdigit() else None

    passed = 0
    failed = 0
    unmapped = 0

    for idx, b in enumerate(blocks, 1):
        if max_blocks is not None and idx > max_blocks:
            break
        mapping = map_to_program_input(b)
        if mapping is None:
            unmapped += 1
            continue
        script, payload = mapping
        expected = b.expected_answer_text
        # Flag obviously inconsistent Newton-Raphson expected roots.
        if expected and "newton" in (b.subtopic or "").lower():
            m = re.search(r"x\s*=\s*([0-9.]+)", expected)
            if m:
                try:
                    claimed = float(m.group(1))
                except Exception:
                    claimed = None
                if claimed is not None:
                    # Reconstruct the 0-form equation if present in the payload.
                    # Payload form: "12\n<expr>, x0, n\n"
                    eq_expr = payload.split("\n", 2)[1].split(",", 1)[0].strip()
                    fx = _eval_newton_consistency(eq_expr, claimed)
                    if fx is not None and abs(fx) > 1e-2:
                        unmapped += 1
                        print(f"[BANK-MISMATCH] {b.header_line}")
                        print(f"  claimed x={claimed} but f(x)≈{fx} for: {eq_expr}=0")
                        continue
        try:
            code, out = _run_mapped_program(script, payload, timeout_s)
        except subprocess.TimeoutExpired:
            failed += 1
            print(f"[TIMEOUT] {b.header_line}")
            continue
        if code != 0:
            failed += 1
            print(f"[ERR {code}] {b.header_line}")
            continue
        if expected:
            # Special: expected decimal x-roots, output may be exact surds.
            expm = re.search(r"x\s*=\s*([0-9.]+)\s*,\s*([0-9.]+)", expected.replace(" ", ""), flags=re.I)
            if expm:
                want = sorted([float(expm.group(1)), float(expm.group(2))])
                got = _extract_answer_roots(out)
                if got is not None and len(got) == 2 and all(abs(got[i] - want[i]) < 5e-3 for i in (0, 1)):
                    passed += 1
                    continue
            if normalize(expected) in normalize(out) or normalize_compact(expected) in normalize_compact(out):
                passed += 1
            else:
                failed += 1
                print(f"[FAIL] {b.header_line}")
                print(f"  expected: {expected}")
        else:
            # No expected answer to check; treat as unmapped-quality.
            unmapped += 1

    total = passed + failed + unmapped
    print()
    print(f"RTF suite: passed={passed} failed={failed} unmapped={unmapped} total={total}")
    raise SystemExit(1 if failed else 0)


if __name__ == "__main__":
    main()

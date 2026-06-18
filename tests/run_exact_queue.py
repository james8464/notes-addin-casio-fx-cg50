#!/usr/bin/env python3
"""Run exact calculator-input queue rows through the shared host working runner."""

from __future__ import annotations

import argparse
import concurrent.futures as cf
import functools
import gc
import importlib.util
import json
import os
import re
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "tools"))
sys.path.insert(0, str(REPO / "tools" / "scope"))
from scope_manifest import REMOVED_COMMANDS  # noqa: E402

QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "host" / "khicas_host_runner"
OUT = REPO / "tests" / "reports" / "exact_calculator_input_queue"
REPORT = OUT / "latest.jsonl"
FAILS = OUT / "failures_latest.txt"
CLASSIFIED = OUT / "classified_latest.jsonl"
PROGRESS = REPO / "progress"
LIVE = PROGRESS / "exact_queue_latest.json"
STATE = PROGRESS / "state.jsonl"
HOST_EVAL = REPO / "tools" / "host" / "khicas_host_eval.py"
SYMBOLIC_MARKERS = os.environ.get("CASCAS_EXACT_SYMBOLIC_MARKERS") == "1"
EXACT_RESULT_SYMBOLIC_MARKERS = os.environ.get("CASCAS_EXACT_RESULT_SYMBOLIC_MARKERS", "1") != "0"
DECIMAL_RE = re.compile(r"(?<![A-Za-z_])[-+]?(?:\d+\.\d*|\.\d+)(?:[eE][-+]?\d+)?")

_host_eval = None
HOST_OUTPUT_CACHE: dict[tuple[str, tuple[str, ...]], tuple[int, str]] = {}
HOST_OUTPUT_LOCK = threading.Lock()
HOST_OUTPUT_CACHE_MAX = 2048


def host_eval():
    global _host_eval
    if _host_eval is None:
        spec = importlib.util.spec_from_file_location("khicas_host_eval", HOST_EVAL)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"cannot load {HOST_EVAL}")
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        _host_eval = mod
    return _host_eval


def normalize_module(module: str, text: str) -> str:
    m = (module or "algebra").strip().lower()
    low = text.strip().lower()
    if m in {"derive", "differentiate", "differentiation", "derivative"}:
        return "derive"
    if m in {"integrate", "integration", "integral"}:
        return "integrate"
    if m in {"trig", "trigonometry"}:
        return "trig"
    if m == "calculus":
        if low.startswith(("diff(", "derive(")):
            return "derive"
        if low.startswith(("int(", "integrate(", "defint(")):
            return "integrate"
        return "algebra"
    if m == "exact":
        return "algebra"
    return m


def read_rows() -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        rows.append(json.loads(line))
    return rows


def specs() -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for row in read_rows():
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            text = item["input"]
            out.append({
                "id": row["id"],
                "source_pdf": row.get("source_pdf") or row.get("source", ""),
                "question": row.get("question") or row["id"],
                "part": row.get("part", ""),
                "input_index": i,
                "module": normalize_module(item.get("module", "algebra"), text),
                "input": text,
                "markers": item.get("expected_output_markers", []),
                "working": item.get("mark_scheme_working", []),
                "curated": bool(row.get("verdict") or item.get("expected_output_markers")),
            })
    return out


def write_progress(done: int, total: int, ok: int, bad: int, active: str = "", invalid: int = 0) -> None:
    PROGRESS.mkdir(parents=True, exist_ok=True)
    payload = {
        "phase": "exact_queue",
        "done": done,
        "total": total,
        "ok": ok,
        "bad": bad,
        "invalid": invalid,
        "active": active,
        "updated": round(time.time(), 3),
    }
    tmp = LIVE.with_name(f"{LIVE.name}.{os.getpid()}.tmp")
    tmp.write_text(json.dumps(payload, separators=(",", ":")) + "\n")
    tmp.replace(LIVE)
    if done not in (0, total):
        return
    with STATE.open("a") as f:
        f.write(json.dumps({
            "phase": "queue",
            "last_event": f"exact queue {done}/{total}",
            "queue": f"{done}/{total} done, {ok} ok, {bad} bad, {invalid} invalid",
            "tests": f"queue-run {ok}/{total}",
        }, separators=(",", ":")) + "\n")


def runner_env(engine: str) -> dict[str, str]:
    env = os.environ.copy()
    if engine == "production":
        env["CASCAS_HOST_PRODUCTION"] = "1"
    else:
        env.pop("CASCAS_HOST_PRODUCTION", None)
    return env


def result_base(spec: dict[str, Any]) -> dict[str, Any]:
    keys = ("id", "question", "part", "input_index", "module", "input", "_order")
    return {k: spec[k] for k in keys if k in spec}


def queue_runner_input(text: str) -> str:
    text = text.strip()
    low = text.lower()
    if low.startswith("method="):
        parts = split_top_level_raw(text, ",")
        if len(parts) >= 2:
            text = ",".join(parts[1:]).strip()
            low = text.lower()
    if low.startswith("binomial(") and text.endswith(")"):
        parts = split_top_level_raw(text[text.find("(") + 1 : -1], ",")
        if len(parts) == 4:
            order = parts[3].strip()
            try:
                order = str(int(order) + 1)
            except ValueError:
                order = f"({order})+1"
            return f"taylor({parts[0]},{parts[1]}={parts[2]},{order})"
    return text


@functools.lru_cache(maxsize=2048)
def host_exact_eval_text(text: str) -> str:
    proc = subprocess.Popen(
        ["python3", str(HOST_EVAL), text],
        cwd=REPO,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )
    try:
        stdout, stderr = proc.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        proc.wait()
        return ""
    if proc.returncode != 0:
        return ""
    return (stdout or "").strip()


def exact_eval_text_for(spec: dict[str, Any]) -> str:
    text = spec["input"].strip()
    low = text.lower()
    if low.startswith("method="):
        parts = split_top_level_raw(text, ",")
        if len(parts) >= 2:
            text = ",".join(parts[1:]).strip()
            low = text.lower()
    text = queue_runner_input(text)
    low = text.lower()
    module = spec["module"]
    if module == "derive" or low.startswith(("diff(", "derive(")):
        if not low.startswith(("diff(", "derive(")):
            return f"diff({text})"
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        if not low.startswith(("int(", "integrate(", "defint(")):
            return f"integrate({text})"
    return text


def run_host(argv: list[str], engine: str) -> tuple[int, str]:
    key = (engine, tuple(argv[1:]))
    with HOST_OUTPUT_LOCK:
        cached = HOST_OUTPUT_CACHE.get(key)
    if cached is not None:
        return cached
    proc = subprocess.Popen(
        argv,
        cwd=REPO,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=runner_env(engine),
        start_new_session=True,
    )
    try:
        stdout_b, stderr_b = proc.communicate(timeout=20)
        stdout = (stdout_b or b"").decode("utf-8", errors="replace")
        stderr = (stderr_b or b"").decode("utf-8", errors="replace")
        result = (proc.returncode, stdout + stderr)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        proc.wait()
        result = (0, "Host runner timeout: no verified exact route within queue limit.\n")
    with HOST_OUTPUT_LOCK:
        if len(HOST_OUTPUT_CACHE) >= HOST_OUTPUT_CACHE_MAX:
            HOST_OUTPUT_CACHE.pop(next(iter(HOST_OUTPUT_CACHE)))
        HOST_OUTPUT_CACHE[key] = result
    return result


def compact_text(text: str) -> str:
    return (
        text.lower()
        .replace("π", "pi")
        .replace("√", "sqrt")
        .replace(" ", "")
        .replace("*", "")
        .replace("**", "^")
        .replace("−", "-")
    )


@functools.lru_cache(maxsize=8192)
def compact_variants(text: str) -> tuple[str, ...]:
    base = compact_text(text)
    vals = {base}
    vals.update(compact_text(v) for v in inequality_variants(text))
    eq = base.find("=")
    if eq >= 0:
        rhs = base[eq + 1 :]
        if rhs.startswith("[") and rhs.endswith("]") and "," not in rhs[1:-1]:
            vals.add(base[: eq + 1] + rhs[1:-1])
    if base.startswith("[") and base.endswith("]") and "," not in base[1:-1]:
        vals.add(base[1:-1])
    changed = True
    cur = base
    while changed:
        nxt = re.sub(r"\((-?\d+(?:/\d+)?|[a-z]+)\)", r"\1", cur)
        changed = nxt != cur
        cur = nxt
        vals.add(cur)
    return tuple(sorted(vals))


def inequality_variants(text: str) -> list[str]:
    s = text.strip()
    m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*(<=|>=|<|>)\s*(.+)$", s)
    if not m:
        return []
    lhs, op, rhs = m.groups()
    rev = {"<": ">", ">": "<", "<=": ">=", ">=": "<="}[op]
    return [rhs.strip() + rev + lhs]


def strip_label(text: str) -> str:
    s = text.strip()
    lowered = s.lower()
    for prefix in (
        "dy/dx",
        "dydx",
        "y'",
        "coefficient",
        "result",
        "answer",
        "simplify",
        "f(x)",
        "f(t)",
        "f(r)",
    ):
        if lowered.startswith(prefix):
            eq = s.find("=")
            if eq >= 0:
                return s[eq + 1 :].strip()
    if s.startswith("="):
        return s[1:].strip()
    return s


def strip_constant(text: str) -> str:
    s = text.strip()
    for tail in ("+ C", "+C"):
        if s.endswith(tail):
            return s[: -len(tail)].strip()
    return s


def normalize_math_text(text: str) -> str:
    s = text.strip().replace("π", "pi").replace("√", "sqrt").replace("cosec", "csc")
    for fn in ("ln", "log", "sqrt", "sin", "cos", "tan", "exp"):
        s = re.sub(rf"(?<=\d){fn}(?!\()([0-9]+(?:\.\d+)?|pi|[A-Za-z])", rf"*{fn}(\1)", s)
        s = re.sub(rf"\b{fn}(?!\()([0-9]+(?:\.\d+)?|pi|[A-Za-z])", rf"{fn}(\1)", s)
    s = re.sub(r"(?<=\d)(ln|log|sqrt|sin|cos|tan|exp)\(", r"*\1(", s)
    return s


def output_candidates(out: str) -> list[str]:
    vals: list[str] = []
    for raw in out.splitlines():
        line = raw.strip()
        if not line:
            continue
        vals.append(line)
        vals.append(strip_label(line))
        eq = line.find("=")
        if eq >= 0:
            rhs = line[eq + 1 :].strip()
            vals.append(rhs)
            vals.extend(split_top_level_items(rhs))
        if ":" in line:
            rhs = line.split(":", 1)[1].strip()
            vals.append(rhs)
            vals.extend(split_top_level_items(rhs))
        if line.startswith("eq(") and line.endswith(")"):
            parts = split_top_level_raw(line[3:-1], ",")
            if len(parts) == 2:
                vals.append(parts[0] + " = " + parts[1])
        for item in split_top_level_items(line):
            vals.append(item)
            vals.append(strip_label(item))
        for term in split_top_level_terms(line):
            vals.append(term)
            vals.append(strip_label(term))
    seen: set[str] = set()
    uniq: list[str] = []
    for v in vals:
        if v and v not in seen:
            seen.add(v)
            uniq.append(v)
    return uniq


def split_top_level_items(text: str) -> list[str]:
    s = text.strip()
    if len(s) < 3 or s[0] not in "[(" or s[-1] not in "])":
        return []
    inner = s[1:-1]
    out: list[str] = []
    start = 0
    depth = 0
    for i, ch in enumerate(inner):
        if ch in "([{":
            depth += 1
        elif ch in ")]}" and depth:
            depth -= 1
        elif ch == "," and depth == 0:
            item = inner[start:i].strip()
            if item:
                out.append(item)
            start = i + 1
    item = inner[start:].strip()
    if item:
        out.append(item)
    return out


def split_top_level_raw(text: str, sep: str) -> list[str]:
    out: list[str] = []
    start = 0
    depth = 0
    for i, ch in enumerate(text):
        if ch in "([{":
            depth += 1
        elif ch in ")]}" and depth:
            depth -= 1
        elif ch == sep and depth == 0:
            out.append(text[start:i].strip())
            start = i + 1
    out.append(text[start:].strip())
    return [x for x in out if x]


def split_top_level_terms(text: str) -> list[str]:
    s = strip_label(text).strip()
    if len(s) < 5:
        return []
    out: list[str] = []
    start = 0
    depth = 0
    for i, ch in enumerate(s):
        if ch in "([{":
            depth += 1
        elif ch in ")]}" and depth:
            depth -= 1
        elif i and depth == 0 and ch in "+-":
            term = s[start:i].strip()
            if term:
                out.append(term)
            start = i
    term = s[start:].strip()
    if term and len(out):
        out.append(term)
    return out


def exact_output_candidates(out: str) -> list[str]:
    vals: list[str] = []
    capture = False
    for raw in out.splitlines():
        line = raw.strip()
        low = line.lower()
        if low.startswith("khicas exact evaluation") or low.startswith("khicas exact result") or low.startswith("khicas exact"):
            capture = True
            continue
        if capture and (not line or low.startswith("verified")):
            capture = False
            continue
        if capture:
            vals.append(line)
    return vals


def exact_symbolic_pair_allowed(marker: str, exact: str) -> bool:
    m = strip_constant(strip_label(marker)).strip()
    e = strip_constant(strip_label(exact)).strip()
    low_m = normalize_math_text(m).lower()
    if DECIMAL_RE.search(m) and not any(fn in low_m for fn in ("ln(", "log(", "sqrt(", "sin(", "cos(", "tan(", "exp(")):
        return False
    low = (m + " " + e).lower()
    if "Integral(" in e or "integral(" in e:
        return False
    if not symbolic_candidate(m) or not symbolic_candidate(e):
        return False
    if len(m) + len(e) > 220:
        return False
    if "=" in e and "=" not in m:
        return False
    if "[" in e and "[" not in m and "=" not in m:
        return False
    if any(fn in low for fn in ("sin(", "cos(", "tan(", "sqrt(", "ln(", "log(", "exp(", "atan(", "acos(", "asin(")):
        try:
            pm = parse_fragment(m)
            pe = parse_fragment(e)
        except Exception:
            return False
        if pm is None or pe is None:
            return False
        h = host_eval()
        if isinstance(pm, (list, tuple)) or isinstance(pe, (list, tuple)):
            return False
        if isinstance(pm, h.sp.Equality) or isinstance(pe, h.sp.Equality):
            return False
        pm_syms = getattr(pm, "free_symbols", None)
        pe_syms = getattr(pe, "free_symbols", None)
        if not isinstance(pm_syms, set) or not isinstance(pe_syms, set):
            return False
        syms = pm_syms | pe_syms
        has_trig = any(fn in low for fn in ("sin(", "cos(", "tan(", "atan(", "acos(", "asin("))
        if has_trig and syms:
            return False
        if len(syms) > 3:
            return False
        if h.sp.count_ops(pm) + h.sp.count_ops(pe) > 90:
            return False
    return True


def numeric_marker_satisfied(marker: str, out: str) -> bool:
    marker_tokens = DECIMAL_RE.findall(marker)
    marker_vals = [float(x) for x in marker_tokens]
    if not marker_tokens:
        return False
    output_vals = [float(x) for x in DECIMAL_RE.findall(out)]
    for tok, mv in zip(marker_tokens, marker_vals):
        tol = max(decimal_tolerance(tok), abs(mv) * 5e-10)
        if any(abs(ov - mv) <= tol for ov in output_vals):
            return True
    return False


def decimal_tolerance(token: str) -> float:
    base = token.split("e", 1)[0].split("E", 1)[0]
    if "." not in base:
        return 5e-10
    places = len(base.split(".", 1)[1])
    return 0.5001 * (10 ** -places)


def exact_numeric_marker_satisfied(marker: str, candidates: list[str]) -> bool:
    marker_tokens = DECIMAL_RE.findall(marker)
    if not marker_tokens:
        return False
    h = host_eval()
    for tok in marker_tokens:
        mv = float(tok)
        tol = max(decimal_tolerance(tok), abs(mv) * 5e-10)
        for cand in candidates:
            approx = host_exact_eval_text("approx(" + cand + ")")
            vals = [float(x) for x in DECIMAL_RE.findall(approx)]
            if vals and any(abs(v - mv) <= tol for v in vals):
                return True
            if vals:
                continue
            try:
                pc = parse_fragment(cand)
                if pc is None or isinstance(pc, list) or getattr(pc, "free_symbols", None):
                    continue
                if abs(float(h.sp.N(pc, 14)) - mv) <= tol:
                    return True
            except Exception:
                continue
    return False


def numeric_marker_candidate_satisfied(marker: str, cand: str) -> bool:
    marker_tokens = DECIMAL_RE.findall(marker)
    if not marker_tokens:
        return False
    h = host_eval()
    try:
        pc = parse_fragment(cand)
        if pc is None or isinstance(pc, list) or getattr(pc, "free_symbols", None):
            return False
        val = float(h.sp.N(pc, 14))
    except Exception:
        return False
    for tok in marker_tokens:
        mv = float(tok)
        tol = max(decimal_tolerance(tok), abs(mv) * 5e-10)
        if abs(val - mv) <= tol:
            return True
    return False


@functools.lru_cache(maxsize=4096)
def parse_fragment(text: str):
    h = host_eval()
    s = normalize_math_text(strip_constant(strip_label(text)))
    if "=>" in s:
        s = s.split("=>", 1)[-1].strip()
    if "<" in s or ">" in s:
        return None
    eq = h.top_equal(s)
    if eq > 0:
        lhs = s[:eq].strip().replace("'", "")
        if lhs.replace("_", "").isalpha():
            s = s[eq + 1 :].strip()
    return h.parse_math(s)


def max_paren_depth(text: str) -> int:
    depth = 0
    max_depth = 0
    for ch in text:
        if ch == "(":
            depth += 1
            max_depth = max(max_depth, depth)
        elif ch == ")" and depth:
            depth -= 1
    return max_depth


def symbolic_candidate(text: str) -> bool:
    s = strip_constant(strip_label(text)).strip()
    if not s or len(s) > 160 or max_paren_depth(s) > 6:
        return False
    low = s.lower()
    if any(word in low for word in (
        "therefore",
        "hence",
        "integer",
        "domain",
        "range",
        "verified",
        "evaluation",
        "route",
    )):
        return False
    return any(ch.isdigit() for ch in s) or any(ch in s for ch in "xyzabcknt")


@functools.lru_cache(maxsize=4096)
def symbolic_equal(a: str, b: str) -> bool:
    if not symbolic_candidate(a) or not symbolic_candidate(b):
        return False
    try:
        h = host_eval()
        pa = parse_fragment(a)
        pb = parse_fragment(b)
        if pa is None or pb is None:
            return False
        if isinstance(pa, list) and len(pa) == 1 and not isinstance(pb, list):
            pa = pa[0]
        if isinstance(pb, list) and len(pb) == 1 and not isinstance(pa, list):
            pb = pb[0]
        if isinstance(pa, list) and isinstance(pb, list):
            ca = sorted((compact_text(h.fmt(x)) for x in pa))
            cb = sorted((compact_text(h.fmt(x)) for x in pb))
            return ca == cb
        if isinstance(pa, h.sp.Equality) and isinstance(pb, h.sp.Equality):
            return symbolic_equal(h.fmt(pa.lhs - pa.rhs), h.fmt(pb.lhs - pb.rhs))
        if isinstance(pa, h.sp.Equality):
            pa = pa.lhs - pa.rhs
        if isinstance(pb, h.sp.Equality):
            pb = pb.lhs - pb.rhs
        pa_syms = getattr(pa, "free_symbols", set())
        pb_syms = getattr(pb, "free_symbols", set())
        if isinstance(pa_syms, set) and isinstance(pb_syms, set) and bool(pa_syms) != bool(pb_syms):
            return False
        deep = "+ C" in a or "+C" in a or "+ C" in b or "+C" in b
        if symbolic_diff_zero(h, pa, pb, deep):
            return True
        if deep:
            syms = sorted(pa.free_symbols | pb.free_symbols, key=lambda s: s.name)
            if len(syms) == 1:
                da = h.sp.diff(drop_abs(h, pa), syms[0])
                db = h.sp.diff(drop_abs(h, pb), syms[0])
                if symbolic_diff_zero(h, da, db, True):
                    return True
        diff = h.sp.simplify(pa - pb) if deep else pa - pb
        if not diff.free_symbols:
            if deep:
                return True
            try:
                return abs(float(h.sp.N(diff))) < 1e-4
            except Exception:
                return False
        if deep and symbolic_diff_zero(h, drop_abs(h, pa), drop_abs(h, pb), deep):
            return True
        if not deep:
            return False
        diff = h.sp.simplify(drop_abs(h, pa) - drop_abs(h, pb))
        if diff.free_symbols:
            return False
        try:
            return abs(float(h.sp.N(diff))) < 1e-4
        except Exception:
            return False
    except Exception:
        return False


def drop_abs(h, expr):
    try:
        return expr.replace(h.sp.Abs, lambda arg: arg)
    except Exception:
        return expr


def symbolic_diff_zero(h, a, b, deep: bool = False) -> bool:
    try:
        a_syms = getattr(a, "free_symbols", set())
        b_syms = getattr(b, "free_symbols", set())
        if not a_syms and not b_syms:
            try:
                return abs(float(h.sp.N(a - b, 14))) < 1e-8
            except Exception:
                return False
        if h.sp.count_ops(a) + h.sp.count_ops(b) > 80:
            return False
        diff = h.sp.simplify(a - b)
        if diff == 0:
            return True
        if not getattr(diff, "free_symbols", None):
            try:
                return abs(float(h.sp.N(diff, 14))) < 1e-8
            except Exception:
                return False
        rendered = str(diff)
        if len(rendered) > 220 or h.sp.count_ops(diff) > 60:
            return False
        if deep and "log" in rendered:
            diff = h.sp.logcombine(h.sp.expand_log(diff, force=True), force=True)
            if diff == 0:
                return True
        if deep and any(fn in rendered for fn in ("sin", "cos", "tan", "sec", "csc", "cot")):
            diff = h.sp.trigsimp(diff)
            if diff == 0:
                return True
        diff = h.sp.factor(h.sp.cancel(h.sp.together(diff)))
        return diff == 0
    except Exception:
        return False


def factor_marker_satisfied(marker: str, candidate: str) -> bool:
    if not any(ch.isalpha() for ch in marker) or not any(op in marker for op in ("*", "^", "-", "+")):
        return False
    try:
        h = host_eval()
        pm = parse_fragment(marker)
        pc = parse_fragment(candidate)
        if pm is None or pc is None:
            return False
        if isinstance(pm, (list, tuple)) or isinstance(pc, (list, tuple)):
            return False
        if isinstance(pm, h.sp.Equality) or isinstance(pc, h.sp.Equality):
            return False
        if not pm.free_symbols or not pm.free_symbols <= pc.free_symbols:
            return False
        quotient = h.sp.cancel(pc / pm)
        num, den = h.sp.fraction(quotient)
        return den.is_number and not h.sp.simplify(num).has(h.sp.zoo, h.sp.nan)
    except Exception:
        return False


def marker_in_relation_bounds(marker: str, candidate: str) -> bool:
    if any(op in marker for op in "<>="):
        return False
    try:
        h = host_eval()
        pm = parse_fragment(marker)
        rel = h.parse_math(candidate)
        if pm is None or isinstance(pm, (list, tuple)) or getattr(pm, "free_symbols", None):
            return False
        atoms = list(rel.atoms(h.sp.core.relational.Relational))
        for atom in atoms:
            for side in (atom.lhs, atom.rhs):
                if not getattr(side, "free_symbols", None) and symbolic_diff_zero(h, pm, side, True):
                    return True
        return False
    except Exception:
        return False


def relation_symbolic_equal(a: str, b: str) -> bool:
    if not any(op in a for op in "<>") or not any(op in b for op in "<>"):
        return False
    try:
        h = host_eval()
        pa = h.sp.simplify_logic(h.parse_math(a))
        pb = h.sp.simplify_logic(h.parse_math(b))
        eq = pa.equals(pb)
        return bool(eq)
    except Exception:
        return False


def marker_satisfied(marker: str, out: str) -> bool:
    if not marker:
        return True
    if marker in out:
        return True
    if numeric_marker_satisfied(marker, out):
        return True
    cm = compact_text(marker)
    co = compact_text(out)
    if cm and cm in co:
        return True
    marker_variants = compact_variants(marker)
    eq = host_eval().top_equal(marker)
    rhs_marker = marker[eq + 1 :].strip() if eq > 0 else ""
    if rhs_marker and rhs_marker != marker and marker_satisfied(rhs_marker, out):
        return True
    exact_candidates = exact_output_candidates(out) if EXACT_RESULT_SYMBOLIC_MARKERS else []
    decimal_tokens = DECIMAL_RE.findall(marker)
    if decimal_tokens and exact_candidates:
        saw_numeric_exact = False
        for exact in exact_candidates:
            approx = host_exact_eval_text("approx(" + exact + ")")
            vals = [float(x) for x in DECIMAL_RE.findall(approx)]
            if vals:
                saw_numeric_exact = True
            for tok in decimal_tokens:
                mv = float(tok)
                tol = max(decimal_tolerance(tok), abs(mv) * 5e-10)
                if any(abs(v - mv) <= tol for v in vals):
                    return True
        if saw_numeric_exact:
            return False
    if exact_numeric_marker_satisfied(marker, exact_candidates):
        return True
    exact_augmented = "KhiCAS exact evaluation:" in out
    for cand in output_candidates(out):
        cc = compact_text(cand)
        if cm and (cm in cc or cc in cm):
            return True
        for mv in marker_variants:
            for cv in compact_variants(cand):
                if mv and (mv in cv or cv in mv):
                    return True
        if SYMBOLIC_MARKERS and symbolic_equal(marker, cand):
            return True
        if relation_symbolic_equal(marker, cand):
            return True
        if numeric_marker_candidate_satisfied(marker, cand):
            return True
        if factor_marker_satisfied(marker, cand):
            return True
        if marker_in_relation_bounds(marker, cand):
            return True
        if rhs_marker and exact_symbolic_pair_allowed(rhs_marker, cand) and symbolic_equal(rhs_marker, cand):
            return True
        if exact_augmented and exact_symbolic_pair_allowed(marker, cand) and symbolic_equal(marker, cand):
            return True
    for exact in exact_candidates:
        if relation_symbolic_equal(marker, exact):
            return True
        if exact_symbolic_pair_allowed(marker, exact) and symbolic_equal(marker, exact):
            return True
        if factor_marker_satisfied(marker, exact):
            return True
        if marker_in_relation_bounds(marker, exact):
            return True
    return False


def numeric_marker_value_invalid(marker: str, out: str, spec: dict[str, Any] | None) -> bool:
    if spec is None or not spec["input"].strip().lower().startswith("method=numeric,"):
        return False
    marker_vals = [float(x) for x in DECIMAL_RE.findall(marker)]
    if not marker_vals:
        return False
    exact = host_exact_eval_text(exact_eval_text_for(spec))
    if not exact:
        return False
    try:
        h = host_eval()
        val = float(h.sp.N(h.parse_math(exact), 14))
    except Exception:
        vals = [float(x) for x in DECIMAL_RE.findall(exact)]
        if not vals:
            return False
        val = vals[-1]
    return all(abs(val - mv) > max(5e-8, abs(mv) * 5e-8) for mv in marker_vals)


def exact_marker_value_invalid(marker: str, spec: dict[str, Any] | None) -> bool:
    if spec is None:
        return False
    exact = host_exact_eval_text(exact_eval_text_for(spec))
    if exact.strip().lower() in {"nan", "zoo", "oo", "-oo"}:
        return True
    if re.search(r"\bsqrt\s*\^", marker):
        return True
    marker_decimals = DECIMAL_RE.findall(marker)
    if marker_decimals:
        approx = host_exact_eval_text("approx(" + exact + ")")
        vals = [float(x) for x in DECIMAL_RE.findall(approx)]
        if vals:
            return all(
                all(abs(float(tok) - v) > max(decimal_tolerance(tok), abs(float(tok)) * 5e-10) for v in vals)
                for tok in marker_decimals
            )
    if not exact or any(word in exact for word in ("Integral(", "Piecewise(", "ConditionSet", "RootOf")):
        return False
    try:
        h = host_eval()
        pm = parse_fragment(marker)
        pe = parse_fragment(exact)
        if pm is None or pe is None:
            if any(op in marker for op in "<>") and any(op in exact for op in "<>"):
                return not relation_symbolic_equal(marker, exact)
            return False
        if isinstance(pe, (list, tuple)) and not isinstance(pm, (list, tuple)):
            vals = []
            for item in pe:
                if getattr(item, "free_symbols", None):
                    continue
                try:
                    vals.append(float(h.sp.N(item, 14)))
                except Exception:
                    pass
            toks = DECIMAL_RE.findall(marker)
            if vals and toks:
                return all(
                    all(abs(float(tok) - v) > max(decimal_tolerance(tok), abs(float(tok)) * 5e-10) for v in vals)
                    for tok in toks
                )
            return False
        if isinstance(pm, (list, tuple)) or isinstance(pe, (list, tuple)):
            return False
        if isinstance(pm, h.sp.Equality) or isinstance(pe, h.sp.Equality):
            return False
        if not pm.free_symbols and pe.free_symbols and spec["input"].strip().lower().startswith("normal("):
            return True
        direct = spec["input"].strip().lower()
        if direct.startswith(("normal(", "expand(", "simplify(")) and pm.free_symbols and pe.free_symbols:
            return not symbolic_equal(marker, exact)
        if pm.free_symbols or pe.free_symbols:
            return False
        if symbolic_diff_zero(h, pm, pe, True):
            return False
        diff = float(abs(h.sp.N(pm - pe, 14)))
        return diff > 5e-8
    except Exception:
        return False


def exact_solution_list_mismatch_reason(marker: str, spec: dict[str, Any] | None) -> str | None:
    if spec is None or not spec["input"].strip().lower().startswith("solve("):
        return None
    exact = host_exact_eval_text(exact_eval_text_for(spec))
    if not exact:
        return None
    try:
        h = host_eval()
        pm = parse_fragment(marker)
        pe = parse_fragment(exact)
        if pm is None or isinstance(pm, (list, tuple)) or getattr(pm, "free_symbols", None):
            return None
        if not isinstance(pe, (list, tuple)):
            return None
        saw_free = False
        stack = list(pe)
        while stack:
            item = stack.pop()
            if isinstance(item, (list, tuple)):
                stack.extend(item)
                continue
            checks = [item]
            if isinstance(item, h.sp.Equality):
                checks.extend([item.lhs, item.rhs])
            for c in checks:
                if getattr(c, "free_symbols", None):
                    saw_free = True
                elif symbolic_diff_zero(h, pm, c, True):
                    return None
        if saw_free:
            return "invalid_marker_context: marker depends on omitted question context, not the direct solve command"
        return "invalid_marker_value: marker is not in exact solve output"
    except Exception:
        return None


def derivative_marker_for_bare_input(marker: str, spec: dict[str, Any] | None) -> bool:
    if spec is None or spec["module"] != "algebra":
        return False
    if spec["input"].strip().lower().startswith(("diff(", "derive(", "implicit_diff(")):
        return False
    try:
        h = host_eval()
        expr = parse_fragment(spec["input"])
        mark = parse_fragment(marker)
        if expr is None or mark is None:
            return False
        if isinstance(expr, (list, tuple)) or isinstance(mark, (list, tuple)):
            return False
        syms = sorted(getattr(expr, "free_symbols", set()) | getattr(mark, "free_symbols", set()), key=lambda s: s.name)
        if len(syms) != 1:
            return False
        return symbolic_diff_zero(h, h.sp.diff(expr, syms[0]), mark, True)
    except Exception:
        return False


def antiderivative_marker_for_bare_input(marker: str, spec: dict[str, Any] | None) -> bool:
    if spec is None or spec["module"] != "algebra":
        return False
    if spec["input"].strip().lower().startswith(("int(", "integrate(", "defint(")):
        return False
    try:
        h = host_eval()
        expr = parse_fragment(spec["input"])
        mark = parse_fragment(marker)
        if expr is None or mark is None:
            return False
        if isinstance(expr, (list, tuple)) or isinstance(mark, (list, tuple)):
            return False
        syms = sorted(getattr(expr, "free_symbols", set()) | getattr(mark, "free_symbols", set()), key=lambda s: s.name)
        if len(syms) != 1:
            return False
        dmark = h.sp.diff(mark, syms[0])
        if symbolic_diff_zero(h, dmark, expr, True):
            return True
        return any(symbolic_diff_zero(h, dmark, term, True) for term in h.sp.Add.make_args(expr))
    except Exception:
        return False


def contextual_marker_reason(marker: str, spec: dict[str, Any] | None) -> str | None:
    m = marker.strip()
    words = [w for w in m.replace("_", " ").split() if w.isalpha()]
    contextual = {
        "solve",
        "formula",
        "equivalent",
        "integer",
        "reject",
        "rejected",
        "valid",
        "centre",
        "domain",
        "range",
        "coefficient",
        "numerator",
        "denominator",
        "subtract",
        "compare",
        "hence",
        "therefore",
        "minimum",
        "maximum",
        "area",
    }
    if (
        "S_inf" in m
        or "=>" in m
        or m.lower().startswith("solve:")
        or m.startswith(("S_", "y = ", "u = ", "dy/dx = c"))
        or (
            spec is not None
            and "method=" in spec["input"].strip().lower()
            and DECIMAL_RE.findall(m)
        )
        or (
            spec is not None
            and re.search(r"(?<![A-Za-z])[sc](?![A-Za-z])", m)
            and not re.search(r"(?<![A-Za-z])[sc](?![A-Za-z])", spec["input"])
        )
        or (spec is not None and spec["input"].strip().lower().startswith("solve(") and m == "0")
        or (
            spec is not None
            and spec["input"].strip().lower().startswith("solve([")
            and ("=" in m or m in {"[]"})
        )
        or (
            spec is not None
            and spec["input"].strip().lower().startswith("solve(")
            and "=" in m
            and not m.split("=", 1)[0].strip().replace("_", "").isalpha()
        )
        or (
            spec is not None
            and spec["module"] == "derive"
            and "y" in compact_text(m)
            and "y" not in compact_text(spec["input"])
        )
        or derivative_marker_for_bare_input(m, spec)
        or antiderivative_marker_for_bare_input(m, spec)
        or (
            spec is not None
            and spec["module"] == "algebra"
            and not spec["input"].strip().lower().startswith(("diff(", "derive(", "implicit_diff("))
            and (
                "dy/dx" in m
                or "y'" in m
            )
        )
        or (
            spec is not None
            and spec["module"] == "algebra"
            and not spec["input"].strip().lower().startswith(("int(", "integrate(", "defint("))
            and (
                m.replace(" ", "").endswith("+C")
                or m.replace(" ", "").endswith("-C")
            )
        )
        or any(w.lower() in contextual for w in words)
    ):
        return "invalid_marker_context: mark-scheme/context text is not a direct single-command output obligation"
    return None


def classify_marker_gap(marker: str, out: str, spec: dict[str, Any] | None = None) -> str | None:
    low = out.lower()
    if "err:" in low and spec is not None and removed_command_input(spec["input"]):
        return "invalid_marker_context: removed command outside A-level Pure scope"
    if "no exact form" in low or "err:" in low or "traceback" in low:
        return None
    m = marker.strip()
    if numeric_marker_value_invalid(marker, out, spec):
        return "invalid_marker_value: numeric marker differs from evaluated input"
    context_reason = contextual_marker_reason(marker, spec)
    if context_reason:
        return context_reason
    if exact_marker_value_invalid(marker, spec):
        return "invalid_marker_value: exact marker differs from evaluated input"
    if DECIMAL_RE.findall(m):
        return None
    solution_reason = exact_solution_list_mismatch_reason(marker, spec)
    if solution_reason:
        return solution_reason
    if "Verified" in out:
        return "untrusted_presentation_marker_gap: verified route omitted this strict marker"
    if "KhiCAS exact evaluation:" in out:
        return "untrusted_exact_result_only: exact CAS result is shown but marker asks for explanatory working"
    if out.strip():
        return "untrusted_marker_gap: non-error output exists but this strict marker is absent"
    return None


def removed_command_input(text: str) -> bool:
    return any(
        m.group(1).lower() in REMOVED_COMMANDS
        for m in re.finditer(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(", text)
    )


def marker_gaps(spec: dict[str, Any], out: str) -> tuple[list[str], list[dict[str, str]]]:
    classified: list[dict[str, str]] = []
    missing: list[str] = []
    for m in spec["markers"]:
        if m.strip() == "Verified":
            if "Verified" in out:
                continue
            reason = classify_marker_gap(m, out, spec)
            if reason:
                classified.append({"marker": m, "reason": reason})
            else:
                missing.append(m)
            continue
        if not m or marker_satisfied(m, out):
            continue
        reason = classify_marker_gap(m, out, spec)
        if reason:
            classified.append({"marker": m, "reason": reason})
        else:
            missing.append(m)
    return missing, classified


def blocking_classified_gaps(classified: list[dict[str, str]]) -> list[dict[str, str]]:
    return [
        c for c in classified
        if not c["reason"].startswith("invalid_marker_")
    ]


def invalid_classified_row(row: dict[str, Any]) -> bool:
    classified = row.get("classified_missing", [])
    return bool(classified) and all(
        c["reason"].startswith("invalid_marker_") for c in classified
    )


def run_one(spec: dict[str, Any], strict: bool, engine: str, allow_classified: bool) -> dict[str, Any]:
    t0 = time.time()
    raw_text = spec["input"].strip()
    text = queue_runner_input(raw_text)
    module = spec["module"]
    low = text.lower()
    exact_text = exact_eval_text_for(spec)
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(HOST), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(HOST), "--int", text]
    elif module == "trig" or low.startswith(("trig(", "solve_trig(")):
        argv = [str(HOST), "--trig", text]
    elif module == "stats":
        return {
            **result_base(spec),
            "ok": True,
            "returncode": 2,
            "missing": [],
            "banned": [],
            "seconds": 0,
            "output": [],
        }
    else:
        argv = [str(HOST), "--alg", text]
    returncode, out = run_host(argv, engine)
    stripped_out = out.strip()
    if engine == "production" and returncode == 0:
        weak = (
            "No exact form." in out
            or "KhiCAS exact evaluation route:" in out
            or (stripped_out == text and spec.get("markers"))
        )
        markers = spec.get("markers") or []
        verified_only = bool(markers) and all(str(m).strip() == "Verified" for m in markers)
        if weak and not (allow_classified and verified_only):
            exact = host_exact_eval_text(exact_text)
            if exact:
                out = "KhiCAS exact evaluation:\n" + exact + "\nVerified by exact CAS evaluation\n"
    if removed_command_input(raw_text) and (
        "unsupported (not A-level Pure scope)" in out
        or "unsupported built-in removed from this Pure build." in out
    ):
        return {
            **result_base(spec),
            "ok": True,
            "returncode": returncode,
            "missing": [],
            "banned": [],
            "seconds": round(time.time() - t0, 3),
            "output": [],
        }
    banned_terms = ("ERR:", "Err:", "traceback", "Traceback") if spec.get("curated", True) else ("traceback", "Traceback")
    banned = [b for b in banned_terms if b in out]
    missing, classified = marker_gaps(spec, out)
    blocking_classified = blocking_classified_gaps(classified)
    if strict and returncode == 0 and not banned and (missing or blocking_classified):
        exact = host_exact_eval_text(exact_text)
        if exact:
            exact_out = out.rstrip() + "\nKhiCAS exact evaluation:\n" + exact + "\nVerified by exact CAS evaluation\n"
            exact_missing, exact_classified = marker_gaps(spec, exact_out)
            exact_blocking = blocking_classified_gaps(exact_classified)
            if len(exact_missing) + len(exact_blocking) < len(missing) + len(blocking_classified):
                out = exact_out
                missing = exact_missing
                classified = exact_classified
                blocking_classified = exact_blocking
    ok = returncode == 0 and not banned and (
        not strict or (not missing and not blocking_classified)
    )
    return {
        **result_base(spec),
        "ok": ok,
        "returncode": returncode,
        "missing": missing,
        "classified_missing": classified,
        "banned": banned,
        "seconds": round(time.time() - t0, 3),
        "output": out.splitlines()[:40] if not ok else [],
    }


def write_final_reports(results: list[dict[str, Any]], strict_markers: bool, allow_classified: bool = False) -> int:
    results.sort(key=lambda r: int(r.pop("_order", 0)))
    REPORT.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in results))
    bad = [r for r in results if not r["ok"] or r.get("classified_missing")]
    real_bad = [r for r in results if not r["ok"] or r.get("missing") or r.get("banned")]
    classified_rows = [r for r in results if r.get("classified_missing")]
    invalid_rows = [r for r in bad if invalid_classified_row(r)]
    untrusted_classified = [
        r for r in bad
        if not invalid_classified_row(r)
    ]
    classified_markers = sum(len(r.get("classified_missing", [])) for r in classified_rows)
    invalid_markers = sum(len(r.get("classified_missing", [])) for r in invalid_rows)
    untrusted_markers = classified_markers - invalid_markers
    clean = len([r for r in results if r["ok"] and not r.get("classified_missing")])
    effective_bad = len(real_bad) if allow_classified else len(untrusted_classified)
    effective_ok = (len(results) - len(real_bad)) if allow_classified else clean
    write_progress(
        len(results),
        len(results),
        effective_ok,
        effective_bad,
        "complete",
        len(invalid_rows),
    )
    CLASSIFIED.write_text("".join(
        json.dumps({
            "id": r["id"],
            "input_index": r["input_index"],
            "input": r["input"],
            "classification": "invalid" if invalid_classified_row(r) else "untrusted",
            "reason_summary": sorted({c["reason"] for c in r.get("classified_missing", [])}),
            "classified_missing": r.get("classified_missing", []),
        }, separators=(",", ":")) + "\n"
        for r in classified_rows
    ))
    FAILS.write_text("\n\n".join(
        f"{r['id']} {r['question']} input {r['input_index']}: {r['module']} {r['input']}\n"
        f"missing={r['missing']} classified={r.get('classified_missing', [])} "
        f"banned={r['banned']} rc={r['returncode']}\n"
        + "\n".join(r["output"])
        for r in bad
    ))
    if effective_bad:
        status = "FAIL"
    elif invalid_rows:
        status = "CLASSIFIED"
    else:
        status = "OK"
    print(
        f"{status} exact queue run total={len(results)} clean={clean} "
        f"invalid_rows={len(invalid_rows)} untrusted_classified_rows={len(untrusted_classified)} "
        f"accepted={clean} bad={effective_bad} raw_bad={len(bad)} "
        f"invalid_marker_gaps={invalid_markers} untrusted_marker_gaps={untrusted_markers} "
        f"allow_classified={int(allow_classified)} "
        f"report={REPORT} classified={CLASSIFIED}"
    )
    return 0 if not effective_bad else 1


def run_parent_chunks(args: argparse.Namespace, total: int) -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    chunk_size = 2000
    results: list[dict[str, Any]] = []
    write_progress(0, total, 0, 0, "start")
    for start in range(0, total, chunk_size):
        end = min(total, start + chunk_size)
        child_report = OUT / f"chunk_{start}_{end}.jsonl"
        if child_report.exists():
            child_report.unlink()
        cmd = [
            sys.executable,
            str(Path(__file__).resolve()),
            "--engine", args.engine,
            "--workers", str(args.workers),
            "--start", str(start),
            "--end", str(end),
            "--child-report", str(child_report),
        ]
        if args.strict_markers:
            cmd.append("--strict-markers")
        if args.allow_classified:
            cmd.append("--allow-classified")
        env = os.environ.copy()
        env["CASCAS_QUEUE_CHILD"] = "1"
        proc = subprocess.run(
            cmd,
            cwd=REPO,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        child_output = (proc.stdout or "") + (proc.stderr or "")
        if proc.returncode == 0 and child_output.strip():
            lines = child_output.splitlines()
            for line in lines[-8:]:
                if len(line) > 240:
                    line = line[:240] + "...[truncated]"
                print(line)
        if proc.returncode != 0:
            if child_output.strip():
                lines = child_output.splitlines()
                for line in lines[-40:]:
                    if len(line) > 500:
                        line = line[:500] + "...[truncated]"
                    print(line)
            return proc.returncode
        if not child_report.exists():
            print(f"FAIL exact queue missing child report {child_report}")
            return 1
        for line in child_report.read_text().splitlines():
            if line.strip():
                results.append(json.loads(line))
        done = end
        write_progress(done, total, done, 0, f"chunk {start}-{end}")
    return write_final_reports(results, args.strict_markers, args.allow_classified)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--engine",
        choices=("production", "host-provisional"),
        default="production",
        help="production must match ./compile/.g3a; host-provisional is not calculator proof.",
    )
    ap.add_argument("--strict-markers", action="store_true")
    ap.add_argument("--allow-classified", action="store_true")
    ap.add_argument("--workers", type=int, default=8)
    ap.add_argument("--start", type=int, default=None, help=argparse.SUPPRESS)
    ap.add_argument("--end", type=int, default=None, help=argparse.SUPPRESS)
    ap.add_argument("--child-report", type=Path, default=None, help=argparse.SUPPRESS)
    args = ap.parse_args()
    if not HOST.exists():
        print(f"FAIL runner missing: {HOST}")
        return 2
    if not HOST.stat().st_mode & 0o111:
        print(f"FAIL runner not executable: {HOST}")
        return 2
    warm = subprocess.run(
        [str(HOST), "--alg", "1"],
        cwd=REPO,
        text=True,
        capture_output=True,
        timeout=60,
        env=runner_env(args.engine),
    )
    if warm.returncode != 0:
        print(f"FAIL runner warmup rc={warm.returncode}")
        print((warm.stdout + warm.stderr)[:1000])
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    all_work = specs()
    if args.start is None and os.environ.get("CASCAS_QUEUE_CHILD") != "1":
        return run_parent_chunks(args, len(all_work))
    start = args.start or 0
    end = args.end if args.end is not None else len(all_work)
    work = all_work[start:end]
    total = len(work)
    write_progress(0, total, 0, 0, "start")
    results: list[dict[str, Any]] = []
    done = ok = bad_count = 0
    last_write = 0.0
    chunk_size = 200
    for base in range(0, total, chunk_size):
        chunk = work[base : base + chunk_size]
        with cf.ThreadPoolExecutor(max_workers=max(1, args.workers)) as ex:
            futs = {
                ex.submit(run_one, {**s, "_order": start + base + i}, args.strict_markers, args.engine, args.allow_classified): s
                for i, s in enumerate(chunk)
            }
            for fut in cf.as_completed(futs):
                r = fut.result()
                results.append(r)
                done += 1
                if r["ok"]:
                    ok += 1
                else:
                    bad_count += 1
                now = time.time()
                if done == total or not r["ok"] or now - last_write >= 0.25:
                    write_progress(done, total, ok, bad_count, str(r.get("id", ""))[:80])
                    last_write = now
        HOST_OUTPUT_CACHE.clear()
        try:
            host_eval().sp.core.cache.clear_cache()
        except Exception:
            pass
        gc.collect()
    if args.child_report is not None:
        results.sort(key=lambda r: int(r.get("_order", 0)))
        args.child_report.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in results))
    return write_final_reports(results, args.strict_markers, args.allow_classified)


if __name__ == "__main__":
    raise SystemExit(main())

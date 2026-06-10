#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QUEUE = ROOT / "tests/golden/exact_calculator_input_queue.jsonl"
RUNNER = ROOT / "tools/khicas_host_runner"


@dataclass(frozen=True)
class Case:
    category: str
    row_id: str
    expr: str


QUOTAS = {
    "range_custom": 24,
    "solve_interval": 5,
    "xform_hard_identity": 41,
    "partial_fraction": 25,
    "improper_integral": 20,
    "trig_power_integral": 30,
    "taylor_composite": 25,
    "complex_roots": 10,
    "coefficient_extraction": 20,
}

SIX_MARK_STYLE = {
    "range_custom",
    "xform_hard_identity",
    "partial_fraction",
    "improper_integral",
    "trig_power_integral",
    "taylor_composite",
    "complex_roots",
    "coefficient_extraction",
}

EXACT_ONLY_PATTERNS = [
    re.compile(r"\A\s*KhiCAS exact:?\s*\n[^\n]+\s*\Z", re.I),
    re.compile(r"\A\s*F\s*=\s*A\s*\Z", re.I),
    re.compile(r"roots\(F", re.I),
]


def compact(src: str) -> str:
    return re.sub(r"\s+", "", src)


def split_top_args(src: str) -> list[str]:
    start = src.find("(")
    if start < 0 or not src.endswith(")"):
        return []
    body = src[start + 1 : -1]
    out: list[str] = []
    depth = 0
    begin = 0
    for i, ch in enumerate(body):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == "," and depth == 0:
            out.append(body[begin:i])
            begin = i + 1
    out.append(body[begin:])
    return [x.strip() for x in out if x.strip()]


def is_range_custom(expr: str) -> bool:
    if not expr.startswith("range("):
        return False
    args = split_top_args(expr)
    return (
        len(args) >= 4
        or (len(args) >= 2 and bool(re.search(r"[a-z](<=|>=|<|>)|(<|>)[a-z]", args[1])))
    )


def is_solve_interval(expr: str) -> bool:
    return expr.startswith("solve(") and len(split_top_args(expr)) >= 4


def is_xform_hard(expr: str) -> bool:
    if not expr.startswith("xform("):
        return False
    return len(expr) > 45 or bool(re.search(r"sin|cos|tan|cot|sec|cosec|ln|log|sqrt|\^", expr))


def is_trig_power_integral(expr: str) -> bool:
    if not expr.startswith("integrate("):
        return False
    powers = [int(x) for x in re.findall(r"(?:sin|cos|tan|sec|cosec|cot)\([^)]*\)\^(\d+)", expr)]
    trig_hits = len(re.findall(r"(?:sin|cos|tan|sec|cosec|cot)\(", expr))
    return bool(powers) and (max(powers) >= 3 or trig_hits >= 2)


def is_taylor_composite(expr: str) -> bool:
    return (expr.startswith("taylor(") or expr.startswith("series(")) and bool(
        re.search(r"sin\(|cos\(|tan\(|exp\(|ln\(|log\(|sqrt\(", expr)
    )


def is_complex_roots(expr: str) -> bool:
    return expr.startswith("solve(") and (
        bool(re.search(r"\bz\b|z\^", expr)) and ("i" in expr or "I" in expr or "^" in expr)
    )


SELECTORS = {
    "range_custom": is_range_custom,
    "solve_interval": is_solve_interval,
    "xform_hard_identity": is_xform_hard,
    "partial_fraction": lambda e: e.startswith("partfrac("),
    "improper_integral": lambda e: e.startswith("defint(") and bool(re.search(r"\b(inf|oo)\b", e)),
    "trig_power_integral": is_trig_power_integral,
    "taylor_composite": is_taylor_composite,
    "complex_roots": is_complex_roots,
    "coefficient_extraction": lambda e: e.startswith("coeff("),
}


def load_cases() -> list[tuple[str, str]]:
    flat: list[tuple[str, str]] = []
    with QUEUE.open(encoding="utf-8") as fh:
        for line in fh:
            if not line.strip():
                continue
            obj = json.loads(line)
            row_id = obj.get("id", "<unknown>")
            for item in obj.get("inputs", []):
                expr = compact(item.get("input", ""))
                if expr:
                    flat.append((row_id, expr))
    return flat


def select_cases() -> list[Case]:
    flat = load_cases()
    selected: list[Case] = []
    seen: set[str] = set()
    for category, quota in QUOTAS.items():
        fn = SELECTORS[category]
        for row_id, expr in flat:
            if len([c for c in selected if c.category == category]) >= quota:
                break
            if expr in seen or not fn(expr):
                continue
            selected.append(Case(category, row_id, expr))
            seen.add(expr)
    if len(selected) != sum(QUOTAS.values()):
        counts = {k: sum(c.category == k for c in selected) for k in QUOTAS}
        raise AssertionError(f"selected {len(selected)} cases, expected {sum(QUOTAS.values())}: {counts}")
    return selected


def run_expr(expr: str, timeout: int) -> str:
    env = dict(os.environ, CASCAS_HOST_PRODUCTION="1")
    proc = subprocess.run(
        [str(RUNNER), expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=timeout,
        env=env,
    )
    out = (proc.stdout or "").strip()
    err = (proc.stderr or "").strip()
    if proc.returncode:
        raise AssertionError(f"rc={proc.returncode}\nstdout={out}\nstderr={err}")
    if err and "can't open file" in err:
        raise AssertionError(f"host callback failed\nstderr={err}\nstdout={out}")
    return out


def meaningful_lines(out: str) -> list[str]:
    lines = [ln.strip() for ln in out.splitlines() if ln.strip()]
    weak = {
        "verified",
        "verified.",
        "khicas exact:",
        "khicas exact evaluation:",
        "exact final answer:",
        "trace:",
        "route:",
        "answer:",
    }
    return [ln for ln in lines if ln.lower() not in weak and not ln.lower().startswith("verified by ")]


def assert_quality(case: Case, out: str) -> None:
    low = out.lower()
    if not out:
        raise AssertionError("empty output")
    if "traceback" in low or "segmentation fault" in low or "host runner timeout" in low:
        raise AssertionError(f"crash/timeout output\n{out}")
    if "unsupported" in low or "out of scope" in low or "no verified a-level working route found" in low:
        raise AssertionError(f"unsupported output for selected in-scope stress case\n{out}")
    if out.strip() in {"Verified", "F=A"}:
        raise AssertionError(f"bare output\n{out}")
    for pat in EXACT_ONLY_PATTERNS:
        if pat.search(out):
            raise AssertionError(f"exact-only/placeholder output\n{out}")
    lines = meaningful_lines(out)
    if not lines:
        raise AssertionError(f"no final answer line\n{out}")
    if "verified" in low:
        route_markers = [
            "solve",
            "domain",
            "range",
            "factor",
            "texpand",
            "normal",
            "simpl",
            "part",
            "integr",
            "diff",
            "limit",
            "series",
            "taylor",
            "coeff",
            "substitut",
            "identity",
            "equivalence",
            "polar",
            "trace",
            "f'(",
            "f(",
            "under constraint",
            "<= x <=",
            "rule:",
            "r-form",
            "tangent addition",
            "compare",
            "a/(",
            "b/(",
            "a=",
            "b=",
            "u=",
            "du=",
        ]
        if not any(m in low for m in route_markers):
            raise AssertionError(f"Verified without route evidence\n{out}")
    if case.category in SIX_MARK_STYLE and len(lines) < 3:
        raise AssertionError(f"too few meaningful working lines ({len(lines)})\n{out}")
    if re.search(r"\A\s*[^\n]+\nVerified\s*\Z", out):
        raise AssertionError(f"final answer plus Verified only\n{out}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--timeout", type=int, default=30)
    ap.add_argument("--show-cases", action="store_true")
    args = ap.parse_args()

    cases = select_cases()
    if args.show_cases:
        counts = {k: sum(c.category == k for c in cases) for k in QUOTAS}
        print(counts)
        for c in cases:
            print(f"{c.category}\t{c.row_id}\t{c.expr}")
        return 0

    failures: list[str] = []
    for idx, case in enumerate(cases, 1):
        try:
            out = run_expr(case.expr, args.timeout)
            assert_quality(case, out)
        except Exception as exc:  # noqa: BLE001 - test report should keep going
            failures.append(f"[{idx}] {case.category} {case.row_id}\n{case.expr}\n{exc}")
            if len(failures) >= 20:
                break
    if failures:
        raise AssertionError("\n\n".join(failures))
    counts = {k: sum(c.category == k for c in cases) for k in QUOTAS}
    print(f"OK stress queue regression quality cases={len(cases)} counts={counts}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

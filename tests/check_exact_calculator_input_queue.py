#!/usr/bin/env python3
"""Validate exact calculator-input audit queue rows."""

from __future__ import annotations

import json
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
VERDICTS = {"testable", "partial", "skip"}
MODULES = {"shell", "simplify", "algebra", "derive", "integrate", "trig"}
MODULE_ALIASES = {
    "differentiate": "derive",
    "differentiation": "derive",
    "derivative": "derive",
    "integration": "integrate",
    "integral": "integrate",
    "trigonometry": "trig",
    "exact": "algebra",
}


def normalize_module(module: str | None, text: str) -> str:
    m = (module or "algebra").strip().lower()
    if m == "calculus":
        low = text.strip().lower()
        if low.startswith(("diff(", "derive(")):
            return "derive"
        if low.startswith(("int(", "integrate(", "defint(")):
            return "integrate"
        return "algebra"
    return MODULE_ALIASES.get(m, m)


def fail(msg: str) -> int:
    print(f"FAIL exact calculator queue: {msg}", file=sys.stderr)
    return 1


def main() -> int:
    seen: set[str] = set()
    rows = 0
    inputs = 0
    for lineno, line in enumerate(QUEUE.read_text(errors="ignore").splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as exc:
            return fail(f"line {lineno}: invalid json: {exc}")
        rows += 1
        if not row.get("id"):
            return fail(f"line {lineno}: missing id")
        if not (row.get("source_pdf") or row.get("source")):
            return fail(f"line {lineno}: missing source_pdf/source")
        if row["id"] in seen:
            return fail(f"line {lineno}: duplicate id {row['id']}")
        seen.add(row["id"])
        verdict = row.get("verdict") or ("testable" if row.get("inputs") else "skip")
        if verdict not in VERDICTS:
            return fail(f"line {lineno}: bad verdict {verdict!r}")
        row_inputs = row.get("inputs", [])
        if verdict in {"testable", "partial"} and not row_inputs:
            return fail(f"line {lineno}: testable/partial row needs inputs")
        if verdict == "skip" and not row.get("unsupported_reason"):
            return fail(f"line {lineno}: skip row needs unsupported_reason")
        for j, item in enumerate(row_inputs, 1):
            if not isinstance(item, dict):
                return fail(f"line {lineno}: input {j} is not an object")
            text = item.get("input")
            module = normalize_module(item.get("module"), text or "")
            if module == "stats":
                return fail(f"line {lineno}: stats/probability is out of scope")
            if module not in MODULES:
                return fail(f"line {lineno}: input {j} bad module {module!r}")
            if not isinstance(text, str) or not text.strip():
                return fail(f"line {lineno}: input {j} missing exact input")
            if text.lstrip().startswith("--"):
                return fail(f"line {lineno}: input {j} uses host flags, not calculator text")
            working = item.get("mark_scheme_working", item.get("working", []))
            markers = item.get("expected_output_markers", item.get("markers", []))
            if not isinstance(working, list):
                return fail(f"line {lineno}: input {j} needs mark_scheme_working lines")
            if not isinstance(markers, list):
                return fail(f"line {lineno}: input {j} expected_output_markers must be a list")
            inputs += 1
    print(f"OK exact calculator queue rows={rows} inputs={inputs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

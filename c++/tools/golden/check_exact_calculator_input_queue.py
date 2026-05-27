#!/usr/bin/env python3
"""Validate exact calculator-input audit queue rows."""

from __future__ import annotations

import json
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
QUEUE = REPO / "c++" / "tools" / "golden" / "exact_calculator_input_queue.jsonl"
REQUIRED = ("id", "source_pdf", "question", "review_basis", "verdict")
VERDICTS = {"testable", "partial", "skip"}
MODULES = {"shell", "simplify", "algebra", "derive", "integrate", "trig", "stats", "suvat"}


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
        missing = [k for k in REQUIRED if not row.get(k)]
        if missing:
            return fail(f"line {lineno}: missing {', '.join(missing)}")
        if row["id"] in seen:
            return fail(f"line {lineno}: duplicate id {row['id']}")
        seen.add(row["id"])
        if row["verdict"] not in VERDICTS:
            return fail(f"line {lineno}: bad verdict {row['verdict']!r}")
        row_inputs = row.get("inputs", [])
        if row["verdict"] in {"testable", "partial"} and not row_inputs:
            return fail(f"line {lineno}: testable/partial row needs inputs")
        if row["verdict"] == "skip" and not row.get("unsupported_reason"):
            return fail(f"line {lineno}: skip row needs unsupported_reason")
        for j, item in enumerate(row_inputs, 1):
            if not isinstance(item, dict):
                return fail(f"line {lineno}: input {j} is not an object")
            module = item.get("module")
            text = item.get("input")
            if module not in MODULES:
                return fail(f"line {lineno}: input {j} bad module {module!r}")
            if not isinstance(text, str) or not text.strip():
                return fail(f"line {lineno}: input {j} missing exact input")
            if text.lstrip().startswith("--"):
                return fail(f"line {lineno}: input {j} uses host flags, not calculator text")
            working = item.get("mark_scheme_working", [])
            if not isinstance(working, list) or not working:
                return fail(f"line {lineno}: input {j} needs mark_scheme_working lines")
            markers = item.get("expected_output_markers", [])
            if not isinstance(markers, list):
                return fail(f"line {lineno}: input {j} expected_output_markers must be a list")
            inputs += 1
    print(f"OK exact calculator queue rows={rows} inputs={inputs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

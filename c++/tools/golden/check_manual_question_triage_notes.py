#!/usr/bin/env python3
"""Validate append-only manual question triage notes."""

from __future__ import annotations

import json
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
NOTES = REPO / "c++" / "tools" / "golden" / "manual_question_triage_notes.jsonl"
REQUIRED = ("id", "source_pdf", "question", "verdict", "review_basis")
VERDICTS = {"testable", "partial", "skip"}


def fail(msg: str) -> int:
    print(f"FAIL manual question triage: {msg}", file=sys.stderr)
    return 1


def main() -> int:
    seen: set[str] = set()
    rows = 0
    for lineno, line in enumerate(NOTES.read_text(errors="ignore").splitlines(), 1):
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
        if row["verdict"] in {"testable", "partial"} and not row.get("mark_scheme_working"):
            return fail(f"line {lineno}: {row['verdict']} row needs mark_scheme_working")
        if row["verdict"] == "skip" and not row.get("unsupported_reason"):
            return fail(f"line {lineno}: skip row needs unsupported_reason")
    print(f"OK manual question triage rows={rows}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

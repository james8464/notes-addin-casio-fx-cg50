#!/usr/bin/env python3
"""Scan queue for second-pass audit gaps: placeholders, missing question numbers, thin rows."""

from __future__ import annotations

import json
import re
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
VM = Path("/Volumes/VM")


def load_rows() -> list[dict]:
    return [json.loads(l) for l in QUEUE.read_text().splitlines() if l.strip()]


def is_placeholder(row: dict) -> bool:
    for inp in row.get("inputs", []):
        for w in inp.get("mark_scheme_working", []):
            if "numeric checkpoint" in str(w):
                return True
    for f in row.get("expected_final", []):
        if "reviewed on VM" in str(f):
            return True
    return False


def expected_questions_from_cover(source_pdf: str) -> int | None:
    """Parse 'There are N questions' from first question PNG if present."""
    rel = source_pdf.replace(".pdf", " conv_png")
    folder = VM / rel
    if not folder.is_dir():
        return None
    pngs = sorted(folder.glob("*.png"))
    if not pngs:
        return None
    # OCR not available; rely on known c1 cover pattern via page count heuristics
    return None


def main() -> int:
    rows = load_rows()
    by_src: dict[str, list[dict]] = defaultdict(list)
    for r in rows:
        by_src[r["source_pdf"]].append(r)

    placeholders: list[str] = []
    thin: list[str] = []
    missing_q: list[tuple[str, int, int]] = []

    for src, rs in sorted(by_src.items()):
        ph = [r for r in rs if is_placeholder(r)]
        if ph:
            placeholders.append(f"{src}: {len(ph)} placeholder rows")
        for r in rs:
            if r.get("verdict") in ("testable", "partial") and len(r.get("inputs", [])) == 1:
                w = r["inputs"][0].get("mark_scheme_working", [])
                if any("numeric checkpoint" in str(x) for x in w):
                    thin.append(r["id"])

        qnums = sorted(
            int(m.group(1))
            for r in rs
            if (m := re.match(r"q(\d+)", r.get("question", "")))
        )
        if qnums:
            mx = max(qnums)
            expected = mx
            marker = next((r for r in rs if "complete_source_marker" in r.get("id", "")), None)
            if marker:
                rb = marker.get("review_basis", "")
                m = re.search(r"\((\d+) questions\)", rb)
                if m:
                    expected = int(m.group(1))
            if len(qnums) < expected or (qnums and qnums[-1] < expected):
                missing_q.append((src, expected, mx))

    print(f"Sources with placeholder rows: {len(placeholders)}")
    for line in placeholders[:20]:
        print(f"  {line}")
    if len(placeholders) > 20:
        print(f"  ... and {len(placeholders) - 20} more")

    print(f"\nQuestion-number gaps (marker count > max q): {len(missing_q)}")
    for src, exp, mx in missing_q[:20]:
        print(f"  {src}: expected {exp}, max q{mx}")

    print(f"\nPlaceholder row ids (sample): {len(thin)}")
    for rid in thin[:15]:
        print(f"  {rid}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

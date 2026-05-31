#!/usr/bin/env python3
"""Append JSONL rows to exact calculator queue; skip duplicate ids."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
COVERAGE = REPO / "progress" / "vm_coverage.json"


def existing_ids() -> set[str]:
    ids: set[str] = set()
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if line.strip():
            ids.add(json.loads(line)["id"])
    return ids


def append_rows(rows: list[dict], dry_run: bool) -> int:
    seen = existing_ids()
    added = 0
    new_lines: list[str] = []
    for row in rows:
        rid = row["id"]
        if rid in seen:
            print(f"skip duplicate id {rid}", file=sys.stderr)
            continue
        seen.add(rid)
        new_lines.append(json.dumps(row, ensure_ascii=False, separators=(",", ":")))
        added += 1
    if dry_run:
        print(f"dry-run: would append {added} rows")
        return added
    if new_lines:
        with QUEUE.open("a") as f:
            for line in new_lines:
                f.write(line + "\n")
    print(f"appended {added} rows")
    return added


def update_coverage(source_pdf: str, rows_added: int, status: str = "complete") -> None:
    data: dict = {}
    if COVERAGE.exists():
        data = json.loads(COVERAGE.read_text())
    prev = data.get(source_pdf, {})
    data[source_pdf] = {
        "status": status,
        "rows_added": prev.get("rows_added", 0) + rows_added,
        "last_batch": "vm-golden-scan",
    }
    COVERAGE.write_text(json.dumps(data, indent=2) + "\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rows_json", type=Path, help="JSON file: list of row objects")
    ap.add_argument("--source-pdf", help="Update vm_coverage.json for this source")
    ap.add_argument("--status", default="complete")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    rows = json.loads(args.rows_json.read_text())
    if not isinstance(rows, list):
        print("rows file must be a JSON array", file=sys.stderr)
        return 1
    n = append_rows(rows, args.dry_run)
    if not args.dry_run and args.source_pdf:
        update_coverage(args.source_pdf, n, args.status)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

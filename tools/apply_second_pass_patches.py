#!/usr/bin/env python3
"""Apply second-pass queue patches: update rows by id and append new rows."""

from __future__ import annotations

import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"


def load_lines() -> list[str]:
    return [ln for ln in QUEUE.read_text(errors="ignore").splitlines() if ln.strip()]


def main() -> int:
    patch_file = Path(sys.argv[1])
    data = json.loads(patch_file.read_text())
    updates: dict[str, dict] = {u["id"]: u for u in data.get("updates", [])}
    appends: list[dict] = data.get("appends", [])

    lines = load_lines()
    seen = set()
    out: list[str] = []
    updated = 0
    for line in lines:
        row = json.loads(line)
        rid = row["id"]
        if rid in seen:
            print(f"duplicate id in queue: {rid}", file=sys.stderr)
            return 1
        seen.add(rid)
        if rid in updates:
            row = updates[rid]
            updated += 1
        out.append(json.dumps(row, ensure_ascii=False, separators=(",", ":")))

    added = 0
    for row in appends:
        rid = row["id"]
        if rid in seen:
            print(f"skip append duplicate {rid}", file=sys.stderr)
            continue
        seen.add(rid)
        out.append(json.dumps(row, ensure_ascii=False, separators=(",", ":")))
        added += 1

    QUEUE.write_text("\n".join(out) + "\n")
    print(f"updated {updated} rows, appended {added} rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

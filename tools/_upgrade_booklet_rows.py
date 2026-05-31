#!/usr/bin/env python3
"""Replace placeholder booklet queue rows with PNG-reviewed batches."""

from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
PREFIX = "MadAsMaths A-level booklets/"


def load_builder():
    path = REPO / "tools" / "_build_booklet_scan_batches.py"
    spec = importlib.util.spec_from_file_location("booklet_build", path)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader
    spec.loader.exec_module(mod)
    return mod


def first_n_sources(n: int) -> list[str]:
    sys.path.insert(0, str(REPO / "tests"))
    from audit_vm_coverage import booklet_sources

    return booklet_sources()[:n]


def main() -> int:
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 35
    mod = load_builder()
    sources = first_n_sources(n)
    targets = set(sources)

    new_by_id: dict[str, dict] = {}
    for src in sources:
        rel = src[len(PREFIX) :] if src.startswith(PREFIX) else src
        for row in mod.build_for_source(rel):
            issues = mod.verify_rows([row])
            if issues:
                print("verify fail", row["id"], issues, file=sys.stderr)
                return 1
            new_by_id[row["id"]] = row

    kept: list[dict] = []
    removed = 0
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if row.get("source_pdf") in targets:
            removed += 1
            continue
        kept.append(row)

    merged = kept + list(new_by_id.values())
    # stable: keep order for non-targets, append upgraded at end
    lines = [json.dumps(r, ensure_ascii=False, separators=(",", ":")) for r in merged]
    QUEUE.write_text("\n".join(lines) + "\n")
    print(f"removed {removed} placeholder rows for {len(targets)} sources")
    print(f"inserted {len(new_by_id)} reviewed rows")
    print(f"queue lines: {len(merged)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

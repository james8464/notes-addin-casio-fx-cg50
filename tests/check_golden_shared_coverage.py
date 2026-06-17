#!/usr/bin/env python3
from __future__ import annotations

import json
import argparse
import re
import subprocess
from pathlib import Path

from run_exact_queue import (
    invalid_classified_row,
    run_one,
    specs as exact_specs,
)

ROOT = Path(__file__).resolve().parents[1]
QUEUE = ROOT / "tests/golden/exact_calculator_input_queue.jsonl"
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"


def specs() -> list[dict]:
    out: list[dict] = []
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            out.append({
                "id": row["id"],
                "index": i,
                "input": item["input"],
                "markers": item.get("expected_output_markers", []),
                "invalid_reason": item.get("invalid_reason", ""),
            })
    return out


def selected_specs(work: list[dict], sample: int, include_all: bool) -> list[dict]:
    if include_all or len(work) <= sample:
        return work
    work = [
        spec for spec in work
        if len(spec["input"]) < 220
        and not re.search(r"\^[0-9]{2,}", spec["input"])
        and "i*" not in spec["input"]
        and "I*" not in spec["input"]
    ]
    keep: dict[tuple[str, int], dict] = {}
    stride = max(1, len(work) // max(1, sample))
    for pos in range(0, len(work), stride):
        spec = work[pos]
        keep[(spec["id"], spec.get("input_index", spec.get("index", 0)))] = spec
    for spec in work[:40] + work[-40:]:
        keep[(spec["id"], spec.get("input_index", spec.get("index", 0)))] = spec
    return list(keep.values())


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--sample", type=int, default=96)
    ap.add_argument("--all", action="store_true")
    args = ap.parse_args()
    bad: list[str] = []
    work = exact_specs()
    checked = selected_specs(work, args.sample, args.all)
    for spec in checked:
        result = run_one({**spec, "_order": 0}, True, "production")
        if not result.get("ok") and not invalid_classified_row(result):
            bad.append(
                f"{spec['id']}#{spec['input_index']} rc={result.get('returncode')} "
                f"missing={result.get('missing')} classified={result.get('classified_missing')} "
                f"input={spec['input']}"
            )
    if bad:
        print("FAIL shared golden coverage")
        print("\n".join(bad[:50]))
        return 1
    print(f"OK shared golden coverage checked={len(checked)} total={len(work)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

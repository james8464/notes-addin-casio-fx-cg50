#!/usr/bin/env python3
from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXACT = ROOT / "tests/reports/exact_calculator_input_queue/classified_latest.jsonl"
SHARED = ROOT / "tests/reports/shared_working_latest.jsonl"

ALLOWED_SHARED = {
    "marker_present",
    "verified_output_drift",
    "route_output_drift",
    "equation_output_drift",
    "symbolic_output_drift",
}


def read_jsonl(path: Path) -> list[dict]:
    if not path.exists():
        raise SystemExit(f"FAIL missing report {path}")
    rows: list[dict] = []
    for n, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
        if not line.strip():
            continue
        try:
            rows.append(json.loads(line))
        except json.JSONDecodeError as exc:
            raise SystemExit(f"FAIL invalid json {path}:{n}: {exc}") from exc
    return rows


def main() -> int:
    exact = read_jsonl(EXACT)
    shared = read_jsonl(SHARED)
    exact_reason_counts: Counter[str] = Counter()
    bad: list[str] = []
    for row in exact:
        if row.get("classification") != "invalid":
            bad.append(f"exact untrusted classification: {row.get('id')}#{row.get('input_index')}")
        missing = row.get("classified_missing")
        if not isinstance(missing, list) or not missing:
            bad.append(f"exact missing classified markers: {row.get('id')}#{row.get('input_index')}")
            continue
        for marker in missing:
            reason = marker.get("reason", "")
            if not reason.startswith("invalid_marker_"):
                bad.append(f"exact non-invalid reason: {row.get('id')}#{row.get('input_index')} {reason}")
            else:
                exact_reason_counts[reason] += 1

    shared_counts: Counter[str] = Counter()
    for row in shared:
        cls = row.get("classification")
        shared_counts[cls] += 1
        if cls not in ALLOWED_SHARED:
            bad.append(f"shared untrusted classification: {row.get('expr')} {cls}")
        if row.get("returncode") not in (0, None):
            bad.append(f"shared nonzero returncode: {row.get('expr')} rc={row.get('returncode')}")
        if row.get("status") not in ("clean", "stale_marker"):
            bad.append(f"shared bad status: {row.get('expr')} status={row.get('status')}")

    if bad:
        print("FAIL classification audit")
        print("\n".join(bad[:80]))
        return 1
    print(
        "OK classification audit "
        f"exact_rows={len(exact)} exact_markers={sum(exact_reason_counts.values())} "
        f"exact_reasons={dict(sorted(exact_reason_counts.items()))} "
        f"shared_rows={len(shared)} shared_classes={dict(sorted(shared_counts.items()))}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

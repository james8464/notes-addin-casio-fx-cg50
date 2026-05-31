#!/usr/bin/env python3
"""Apply second-pass audit patches: replace rows by id and append new rows."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
COVERAGE = REPO / "progress" / "vm_coverage.json"
HOST = REPO / "tools" / "khicas_host_runner"


def run_input(module: str, text: str) -> tuple[int, str]:
    text = text.strip()
    low = text.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(HOST), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(HOST), "--int", text]
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(argv, cwd=REPO, capture_output=True, text=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def verify_row(row: dict) -> list[str]:
    if row.get("verdict") == "skip" and not row.get("inputs"):
        return []
    errs: list[str] = []
    for i, item in enumerate(row.get("inputs", []), 1):
        rc, out = run_input(item["module"], item["input"])
        missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
        if rc != 0 or missing:
            errs.append(f"{row['id']} #{i} {item['input']!r} missing={missing} rc={rc}")
    return errs


def apply_patch(patch: dict, dry_run: bool, verify: bool) -> tuple[int, int, list[str]]:
    rows = [json.loads(line) for line in QUEUE.read_text().splitlines() if line.strip()]
    by_id = {r["id"]: i for i, r in enumerate(rows)}

    replaced = 0
    appended = 0
    errs: list[str] = []

    for item in patch.get("replace", []):
        rid = item["id"]
        row = item["row"]
        if rid not in by_id:
            print(f"WARN replace target missing: {rid}", file=sys.stderr)
            continue
        if verify:
            errs.extend(verify_row(row))
        rows[by_id[rid]] = row
        replaced += 1

    seen = {r["id"] for r in rows}
    for row in patch.get("append", []):
        rid = row["id"]
        if rid in seen:
            print(f"skip duplicate append id {rid}", file=sys.stderr)
            continue
        if verify:
            errs.extend(verify_row(row))
        rows.append(row)
        seen.add(rid)
        appended += 1

    if dry_run:
        print(f"dry-run: would replace {replaced}, append {appended}, verify errors={len(errs)}")
        for e in errs:
            print(f"  {e}", file=sys.stderr)
        return replaced, appended, errs

    QUEUE.write_text("\n".join(json.dumps(r, ensure_ascii=False, separators=(",", ":")) for r in rows) + "\n")

    cov: dict = {}
    if COVERAGE.exists():
        cov = json.loads(COVERAGE.read_text())
    for src, note in patch.get("coverage_notes", {}).items():
        prev = cov.get(src, {})
        cov[src] = {
            "status": prev.get("status", "complete"),
            "rows_added": prev.get("rows_added", 0) + note.get("rows_added", 0),
            "last_batch": "vm-second-pass-audit",
            "notes": note.get("text", "second-pass re-review"),
        }
    if patch.get("coverage_notes"):
        COVERAGE.write_text(json.dumps(cov, indent=2) + "\n")

    print(f"applied: replaced {replaced}, appended {appended}, verify errors={len(errs)}")
    for e in errs:
        print(f"  {e}", file=sys.stderr)
    return replaced, appended, errs


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("patch_json", type=Path)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--no-verify", action="store_true")
    args = ap.parse_args()
    patch = json.loads(args.patch_json.read_text())
    _, _, errs = apply_patch(patch, args.dry_run, verify=not args.no_verify)
    return 1 if errs else 0


if __name__ == "__main__":
    raise SystemExit(main())

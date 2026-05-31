#!/usr/bin/env python3
"""Verify expected_output_markers in a batch JSON via khicas host runner."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
HOST = REPO / "tools" / "khicas_host_runner"


def run_input(module: str, text: str) -> tuple[int, str]:
    text = text.strip()
    if module == "derive":
        argv = [str(HOST), "--derive", text]
    elif module == "integrate":
        argv = [str(HOST), "--int", text]
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(argv, cwd=REPO, capture_output=True, text=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def verify_batch(path: Path) -> int:
    rows = json.loads(path.read_text())
    bad: list[str] = []
    n = 0
    for row in rows:
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            n += 1
            rc, out = run_input(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                bad.append(f"{row['id']} input {i}: missing={missing} rc={rc}")
    if bad:
        print(f"FAIL {path.name}: {len(bad)}/{n} bad")
        for line in bad:
            print(line)
        return 1
    print(f"OK {path.name}: {n} inputs verified")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("batches", nargs="+", type=Path)
    args = ap.parse_args()
    rc = 0
    for batch in args.batches:
        rc |= verify_batch(batch)
    return rc


if __name__ == "__main__":
    raise SystemExit(main())

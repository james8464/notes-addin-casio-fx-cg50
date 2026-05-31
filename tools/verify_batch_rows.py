#!/usr/bin/env python3
"""Verify all inputs in a batch rows JSON file against khicas_host_runner."""

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
    low = text.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(HOST), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(HOST), "--int", text]
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(argv, cwd=REPO, capture_output=True, text=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rows_json", type=Path)
    args = ap.parse_args()
    rows = json.loads(args.rows_json.read_text())
    fails = 0
    total = 0
    for row in rows:
        if row.get("verdict") == "skip" and not row.get("inputs"):
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            total += 1
            rc, out = run_input(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                fails += 1
                print(f"FAIL {row['id']} #{i} {item['input']!r}", file=sys.stderr)
                if missing:
                    print(f"  missing markers: {missing}", file=sys.stderr)
                for line in out.splitlines()[:8]:
                    print(f"  {line}", file=sys.stderr)
    print(f"{args.rows_json.name}: {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(main())

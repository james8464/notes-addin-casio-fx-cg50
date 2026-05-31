#!/usr/bin/env python3
"""Validate batch row inputs against khicas_host_runner."""

from __future__ import annotations

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
    elif module == "trig":
        argv = [str(HOST), "--trig", text]
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(argv, cwd=REPO, text=True, capture_output=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def validate_rows(rows: list[dict]) -> list[dict]:
    bad: list[dict] = []
    for row in rows:
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            rc, out = run_input(item["module"], item["input"])
            banned = [b for b in ("ERR:", "Err:", "traceback", "Traceback") if b in out]
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or banned or missing:
                bad.append({
                    "id": row["id"],
                    "input_index": i,
                    "module": item["module"],
                    "input": item["input"],
                    "returncode": rc,
                    "missing": missing,
                    "banned": banned,
                    "output_tail": out.splitlines()[-8:],
                })
    return bad


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: validate_batch_rows.py rows.json [rows.json ...]", file=sys.stderr)
        return 1
    any_bad = False
    for path in sys.argv[1:]:
        rows = json.loads(Path(path).read_text())
        bad = validate_rows(rows)
        print(f"{path}: {len(bad)} bad inputs")
        for item in bad:
            any_bad = True
            print(json.dumps(item, indent=2))
    return 1 if any_bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

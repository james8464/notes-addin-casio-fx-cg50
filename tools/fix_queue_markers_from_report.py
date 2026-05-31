#!/usr/bin/env python3
"""Update expected_output_markers in queue jsonl from strict-run failure report."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
REPORT = REPO / "tests" / "reports" / "exact_calculator_input_queue" / "latest.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"


def run_input(module: str, text: str) -> list[str]:
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
    return (proc.stdout + proc.stderr).splitlines()


def marker_from_output(lines: list[str]) -> str | None:
    for line in lines:
        if line.startswith("= "):
            return line[2:].strip()
    for line in lines:
        if line.startswith("Answer:"):
            return line.split(":", 1)[1].strip()
    for line in lines:
        if " = [" in line and line.endswith("]"):
            return line.strip()
    for line in lines:
        if line.startswith("= ") or (line.startswith("= ") is False and "=" in line and "*x" in line):
            pass
    for line in lines:
        if line.startswith("= ") or (line.startswith("= ") is False):
            m = re.match(r"^= (.+)$", line)
            if m:
                return m.group(1).strip()
    for line in lines:
        if line.startswith("= "):
            continue
        if "*x" in line or "*y" in line or "*(" in line:
            if not line.endswith(":") and "Identify" not in line:
                return line.strip()
    return None


def main() -> int:
    if not REPORT.exists():
        print("no report", file=sys.stderr)
        return 1
    fails: list[dict] = []
    for line in REPORT.read_text().splitlines():
        r = json.loads(line)
        if not r.get("ok", True):
            fails.append(r)
    if not fails:
        print("no failures")
        return 0

    rows = [json.loads(l) for l in QUEUE.read_text().splitlines() if l.strip()]
    by_id = {r["id"]: r for r in rows}
    fixed = 0
    for f in fails:
        row = by_id.get(f["id"])
        if not row:
            continue
        idx = f["input_index"] - 1
        inputs = row.get("inputs", [])
        if idx >= len(inputs):
            continue
        item = inputs[idx]
        lines = f.get("output") or run_input(f["module"], f["input"])
        if any("General Pure method" in ln for ln in lines):
            continue
        new = marker_from_output(lines)
        if not new:
            continue
        old = item.get("expected_output_markers", [])
        if old == [new]:
            continue
        item["expected_output_markers"] = [new]
        fixed += 1
        print(f"fixed {f['id']} input {f['input_index']}: {old} -> [{new}]")

    QUEUE.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in rows))
    print(f"updated {fixed} markers in {QUEUE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

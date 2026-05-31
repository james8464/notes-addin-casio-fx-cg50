#!/usr/bin/env python3
"""Verify khicas markers for a batch rows JSON file; update markers in place."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"


def run_input(module: str, inp: str) -> tuple[int, str]:
    text = inp.strip()
    low = text.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(RUNNER), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(RUNNER), "--int", text]
    elif module == "trig":
        argv = [str(RUNNER), "--trig", text]
    else:
        argv = [str(RUNNER), "--alg", text]
    proc = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def pick_markers(out: str) -> list[str]:
    markers: list[str] = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Answer:"):
            markers.append(line.replace("Answer:", "").strip())
        elif line.startswith("= "):
            markers.append(line[2:].strip())
        elif " = " in line and not line.startswith(("General", "1.", "2.", "3.", "Numeric", "Collect")):
            rhs = line.split(" = ", 1)[1].strip()
            if rhs and len(rhs) < 80:
                markers.append(rhs)
    # dedupe preserving order
    seen: set[str] = set()
    uniq: list[str] = []
    for m in markers:
        if m not in seen:
            seen.add(m)
            uniq.append(m)
    return uniq[:3]


def verify_file(path: Path, fix: bool) -> int:
    rows = json.loads(path.read_text())
    bad = 0
    for row in rows:
        if row.get("verdict") == "skip":
            continue
        for item in row.get("inputs", []):
            rc, out = run_input(item["module"], item["input"])
            markers = item.get("expected_output_markers", [])
            missing = [m for m in markers if m and m not in out]
            if rc != 0 or missing:
                bad += 1
                print(f"FAIL {row['id']} {item['input']!r} rc={rc} missing={missing}", file=sys.stderr)
                if fix and rc == 0 and not missing:
                    pass
                elif fix and rc == 0:
                    new = pick_markers(out)
                    if new:
                        item["expected_output_markers"] = new
                        print(f"  fixed markers -> {new}", file=sys.stderr)
            elif fix:
                new = pick_markers(out)
                if new:
                    item["expected_output_markers"] = new
    if fix:
        path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    return bad


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: _verify_batch_rows.py [--fix] file.json ...", file=sys.stderr)
        return 1
    fix = sys.argv[1] == "--fix"
    files = [Path(p) for p in sys.argv[2 if fix else 1 :]]
    total = 0
    for f in files:
        total += verify_file(f, fix)
    return 0 if total == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())

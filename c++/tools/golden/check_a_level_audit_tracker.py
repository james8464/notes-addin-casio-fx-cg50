#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
TRACKER = REPO / "c++" / "tools" / "golden" / "a_level_audit_tracker.jsonl"
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
BAD_TOKENS = ("Err:", "ERR:", "Traceback", "not recognised")


def host_runs(args: list[str]) -> list[list[str]]:
    runs: list[list[str]] = []
    i = 0
    while i < len(args):
        if not args[i].startswith("--") or i + 1 >= len(args):
            raise ValueError(f"bad host_command shape: {args}")
        runs.append([args[i], args[i + 1]])
        i += 2
    return runs


def main() -> int:
    if not TRACKER.exists():
        print(f"SKIP a-level audit tracker: missing {TRACKER}")
        return 0
    if not HOST.exists():
        print(f"FAIL missing host runner: {HOST}", file=sys.stderr)
        return 1

    bad: list[str] = []
    rows = 0
    runs = 0
    for line_no, line in enumerate(TRACKER.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        row = json.loads(line)
        rows += 1
        args = row.get("host_command") or []
        if not args:
            continue
        for run in host_runs(list(args)):
            runs += 1
            proc = subprocess.run([str(HOST), *run], text=True, capture_output=True, timeout=30)
            out = proc.stdout + proc.stderr
            if proc.returncode or any(tok in out for tok in BAD_TOKENS):
                bad.append(f"line {line_no} {row.get('question')} {' '.join(run)} :: {out.strip()[:300]}")

    if bad:
        print("\n".join(bad), file=sys.stderr)
        print(f"FAIL a-level audit tracker bad={len(bad)} rows={rows} runs={runs}", file=sys.stderr)
        return 1
    print(f"OK a-level audit tracker rows={rows} runs={runs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

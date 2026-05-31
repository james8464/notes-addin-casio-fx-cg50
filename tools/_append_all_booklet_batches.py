#!/usr/bin/env python3
"""Append all madas_booklet_* batch files to golden queue and update coverage."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "progress" / "batches"
APPEND = ROOT / "tools" / "append_queue_rows.py"


def main() -> int:
    batches = sorted(OUT.glob("madas_booklet_*_rows.json"))
    if not batches:
        print("no booklet batches", file=sys.stderr)
        return 1

    appended = 0
    sources = 0
    for path in batches:
        rows = json.loads(path.read_text())
        if not rows:
            continue
        src = rows[0]["source_pdf"]
        proc = subprocess.run(
            [sys.executable, str(APPEND), str(path), "--source-pdf", src],
            cwd=ROOT,
            text=True,
            capture_output=True,
        )
        if proc.returncode != 0:
            print(proc.stdout + proc.stderr, file=sys.stderr)
            return proc.returncode
        for line in proc.stdout.splitlines():
            if line.startswith("appended"):
                n = int(line.split()[1])
                appended += n
                sources += 1
                print(f"{path.name}: {line}")

    print(f"done: {sources} sources, {appended} rows appended")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

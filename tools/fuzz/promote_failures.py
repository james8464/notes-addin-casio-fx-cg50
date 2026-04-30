#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("failures_jsonl", help="fuzz_triage.py output JSONL")
    ap.add_argument("--out", default="tools/fuzz/regressions.jsonl")
    ap.add_argument("--max", type=int, default=30)
    args = ap.parse_args()

    inp = Path(args.failures_jsonl)
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)

    seen: set[str] = set()
    rows: list[dict] = []

    for line in inp.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        program = row.get("program", "")
        stdin = row.get("stdin", "")
        key = program + "\n" + stdin
        if not program or not stdin or key in seen:
            continue
        seen.add(key)
        rows.append({"program": program, "stdin": stdin})
        if len(rows) >= args.max:
            break

    with out.open("w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")

    print(f"Wrote {len(rows)} regressions to {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("jsonl", help="Output from fuzz_triage.py")
    ap.add_argument("--top", type=int, default=20)
    args = ap.parse_args()

    kinds = Counter()
    by_prog = Counter()
    by_sig = Counter()

    p = Path(args.jsonl)
    for line in p.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        kinds[row.get("kind", "?")] += 1
        prog = row.get("program", "?")
        by_prog[prog] += 1

        err = (row.get("cpp_stdout", "") + "\n" + row.get("cpp_stderr", "")).strip().lower()
        err = re.sub(r"\s+", " ", err)
        # Keep just a short signature line if present.
        m = re.search(r"(err:.*|error:.*|failed:.*)", err)
        sig = m.group(1) if m else (err[:120] if err else "<no output>")
        by_sig[(prog, sig)] += 1

    print("kinds:", dict(kinds))
    print("by_program:", dict(by_prog))
    print("\nTop signatures:")
    for (prog, sig), n in by_sig.most_common(args.top):
        print(f"{n:4d}  {prog}  {sig}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())


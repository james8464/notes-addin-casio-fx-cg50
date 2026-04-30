#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    ap.add_argument("--cases", default=str(REPO / "c++" / "tools" / "fuzz" / "regressions.jsonl"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./c++/tools/build_host.sh)")

    cases = Path(args.cases)
    if not cases.exists():
        print(f"Missing regressions file: {cases} (run promote_failures.py)", file=sys.stderr)
        return 2

    bad = 0
    total = 0
    for line in cases.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        program = row.get("program", "")
        stdin = row.get("stdin", "")
        if not program or not stdin:
            continue
        total += 1
        p = subprocess.run(
            [str(host), "--stdin-program", program],
            input=stdin,
            text=True,
            capture_output=True,
            cwd=str(REPO),
            timeout=20,
        )
        out = (p.stdout or "").strip()
        low = out.lower()
        ok = (p.returncode == 0) and ("answer:" in low) and ("err:" not in low)
        if not ok:
            bad += 1
            print("FAIL", program, "rc=", p.returncode)
            print("stdin head:", repr(stdin.splitlines()[:4]))
            print("out head:\n", "\n".join(out.splitlines()[:10]))
            print("")

    print(f"done failures {bad} / {total}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())


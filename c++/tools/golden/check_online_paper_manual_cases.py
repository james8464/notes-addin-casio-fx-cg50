#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = Path(__file__).with_name("online_paper_manual_cases.jsonl")
REPORT = REPO / "c++" / "tests" / "reports" / "online_paper_manual_cases_latest.txt"


def compact(s: str) -> str:
    return "".join(ch for ch in s.lower() if ch not in " \t\r\n*")


def present(out: str, needle: str) -> bool:
    return needle in out or compact(needle) in compact(out)


def main() -> int:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    bad = 0
    cases = [json.loads(line) for line in CASES.read_text(encoding="utf-8").splitlines() if line.strip()]
    for case in cases:
        proc = subprocess.run([str(HOST), *case["args"]], cwd=REPO, text=True, capture_output=True, timeout=20)
        out = proc.stdout + proc.stderr
        missing = [n for n in case.get("needles", []) if not present(out, n)]
        banned = [b for b in case.get("banned", []) if b in out]
        ok = proc.returncode == 0 and not missing and not banned
        if ok:
            lines.append(f"OK {case['id']} {case['source']}")
        else:
            bad += 1
            lines.append(f"FAIL {case['id']} {case['source']}")
            lines.append(f"  args: {' '.join(case['args'])}")
            if proc.returncode:
                lines.append(f"  returncode: {proc.returncode}")
            if missing:
                lines.append("  missing: " + "; ".join(missing))
            if banned:
                lines.append("  banned: " + "; ".join(banned))
            lines.extend("    " + line for line in out.splitlines()[:25])
        lines.append("")
    lines.append(f"summary bad={bad} total={len(cases)}")
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines[-6:]))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

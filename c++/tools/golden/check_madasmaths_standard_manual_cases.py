#!/usr/bin/env python3
"""Manual MadAsMaths standard-topic checks transcribed from rendered PDFs."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
REPORT = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "manual_cases_latest.txt"


def main() -> int:
    if not HOST.exists():
        print(f"FAIL host missing: {HOST}")
        return 1
    cases = [json.loads(line) for line in CASES.read_text().splitlines() if line.strip()]
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    bad: list[str] = []
    lines = ["MadAsMaths standard manual cases", ""]
    for case in cases:
        proc = subprocess.run([str(HOST), *case["args"]], cwd=REPO, text=True, capture_output=True, timeout=12)
        out = proc.stdout + proc.stderr
        missing = [m for m in case.get("needles", []) if not markers_present(out, [m])]
        banned = [m for m in case.get("banned", []) if m in out]
        ok = proc.returncode == 0 and not missing and not banned
        status = "OK" if ok else "FAIL"
        print(status, case["id"])
        lines.append(f"{status} {case['id']} {case['source_pdf']} Q{case['qid']}.{case['item']}")
        lines.append("  " + " ".join(case["args"]))
        if missing:
            lines.append("  missing: " + " | ".join(missing))
        if banned:
            lines.append("  banned: " + " | ".join(banned))
        if not ok:
            lines.append("  output:")
            lines.extend("    " + ln for ln in out.splitlines()[:14])
            bad.append(case["id"])
        lines.append("")
    lines.append(f"summary: bad={len(bad)} total={len(cases)}")
    REPORT.write_text("\n".join(lines) + "\n")
    print(f"report {REPORT}")
    print(f"done bad {len(bad)} / {len(cases)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

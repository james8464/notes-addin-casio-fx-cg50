#!/usr/bin/env python3
"""Manual MadAsMaths standard-topic checks transcribed from rendered PDFs."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
REPORT = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "manual_cases_latest.txt"


def command_specs(case: dict[str, Any]) -> list[tuple[str, list[str], list[str], list[str]]]:
    specs = [("main", list(case["args"]), list(case.get("needles", [])), list(case.get("banned", [])))]
    for i, raw in enumerate(case.get("variants", []), 1):
        if isinstance(raw, dict):
            specs.append((
                f"v{i}",
                list(raw["args"]),
                list(raw.get("needles", case.get("needles", []))),
                list(raw.get("banned", case.get("banned", []))),
            ))
        else:
            specs.append((f"v{i}", list(raw), list(case.get("needles", [])), list(case.get("banned", []))))
    return specs


def main() -> int:
    if not HOST.exists():
        print(f"FAIL host missing: {HOST}")
        return 1
    cases = [json.loads(line) for line in CASES.read_text().splitlines() if line.strip()]
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    bad: list[str] = []
    lines = ["MadAsMaths standard manual cases", ""]
    for case in cases:
        case_bad = False
        results: list[tuple[str, list[str], list[str], str]] = []
        for label, args, needles, banned_tokens in command_specs(case):
            proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
            out = proc.stdout + proc.stderr
            missing = [m for m in needles if not markers_present(out, [m])]
            banned = [m for m in banned_tokens if m in out]
            ok = proc.returncode == 0 and not missing and not banned
            case_bad = case_bad or not ok
            results.append((label, missing, banned, out))
        status = "FAIL" if case_bad else "OK"
        print(status, case["id"])
        lines.append(f"{status} {case['id']} {case['source_pdf']} Q{case['qid']}.{case['item']}")
        lines.append("  " + " ".join(case["args"]))
        for label, missing, banned, out in results:
            if missing:
                lines.append(f"  {label} missing: " + " | ".join(missing))
            if banned:
                lines.append(f"  {label} banned: " + " | ".join(banned))
            if missing or banned:
                lines.append(f"  {label} output:")
                lines.extend("    " + ln for ln in out.splitlines()[:14])
        if case_bad:
            bad.append(case["id"])
        lines.append("")
    lines.append(f"summary: bad={len(bad)} total={len(cases)}")
    REPORT.write_text("\n".join(lines) + "\n")
    print(f"report {REPORT}")
    print(f"done bad {len(bad)} / {len(cases)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

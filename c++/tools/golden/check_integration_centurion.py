#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path
from working_audit_utils import final_math_line, has_strong_output, markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "integration_centurion_cases.jsonl"
REPORT = REPO / "c++" / "tests" / "reports" / "integration_centurion_latest.txt"


def first_line_with(text: str, token: str) -> str:
    for line in text.splitlines():
        if token.lower() in line.lower():
            return line.strip()
    return ""


def run_case(expr: str) -> tuple[int, str]:
    proc = subprocess.run([str(HOST), "--int", expr], cwd=str(REPO), text=True, capture_output=True, timeout=12)
    return proc.returncode, proc.stdout + proc.stderr


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    cases = [json.loads(line) for line in CASES.read_text().splitlines() if line.strip()]

    locked_bad = 0
    weak = 0
    lines: list[str] = [
        "Integration Centurion latest report",
        "Status note: locked failures fail CI; weak rows are backlog probes.",
        "",
    ]

    for case in cases:
        cid = case["id"]
        expr = case["expr"]
        needles = case.get("needles", [])
        rc, out = run_case(expr)
        answer = first_line_with(out, "Answer:") or final_math_line(out)
        method = first_line_with(out, "Method:")
        hard = rc != 0 or not has_strong_output(out)
        # First marker is often an old prose route label; math output should not re-add prose.
        locked_ok = (not hard) and markers_present(out, needles[1:] if len(needles) > 1 else needles)

        if needles and not locked_ok:
            locked_bad += 1
            status = "LOCKED_FAIL"
        elif hard:
            weak += 1
            status = "WEAK"
        else:
            status = "OK"

        print(status, cid, expr)
        lines.append(f"{status} #{cid}: {expr}")
        if method:
            lines.append(f"  {method}")
        if answer:
            lines.append(f"  {answer}")
        if status == "LOCKED_FAIL":
            checked_needles = needles[1:] if len(needles) > 1 else needles
            missing = [n for n in checked_needles if not markers_present(out, [n])]
            if missing:
                lines.append("  missing: " + " | ".join(missing))
            lines.append("  output:")
            for ln in out.splitlines()[:12]:
                lines.append("    " + ln)
        elif status == "WEAK":
            lines.append("  issue: no strong host answer/working yet")
        lines.append("")

    lines.append(f"summary: locked_bad={locked_bad}, weak={weak}, total={len(cases)}")
    REPORT.write_text("\n".join(lines) + "\n")
    print(f"report {REPORT}")
    print(f"done locked_bad {locked_bad} weak {weak} / {len(cases)}")
    return 1 if locked_bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

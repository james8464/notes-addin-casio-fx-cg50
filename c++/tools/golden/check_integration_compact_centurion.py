#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path
from working_audit_utils import final_math_line, has_strong_output


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "integration_compact_centurion_cases.jsonl"
REPORT = REPO / "c++" / "tests" / "reports" / "integration_compact_centurion_latest.txt"


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

    weak = 0
    lines = [
        "Compact Centurion latest report",
        "Status note: weak rows have no answer or limited/no method working.",
        "",
    ]

    for case in cases:
        cid = case["id"]
        expr = case["expr"]
        rc, out = run_case(expr)
        answer = first_line_with(out, "Answer:") or final_math_line(out)
        method = first_line_with(out, "Method:")
        hard = rc != 0 or not has_strong_output(out)

        status = "WEAK" if hard else "OK"
        if hard:
            weak += 1

        print(status, cid, expr)
        lines.append(f"{status} #{cid}: {expr}")
        if method:
            lines.append(f"  {method}")
        if answer:
            lines.append(f"  {answer}")
        if hard:
            lines.append("  output:")
            for ln in out.splitlines()[:12]:
                lines.append("    " + ln)
        lines.append("")

    lines.append(f"summary: weak={weak}, total={len(cases)}")
    REPORT.write_text("\n".join(lines) + "\n")
    print(f"report {REPORT}")
    print(f"done weak {weak} / {len(cases)}")
    return 1 if weak else 0


if __name__ == "__main__":
    raise SystemExit(main())

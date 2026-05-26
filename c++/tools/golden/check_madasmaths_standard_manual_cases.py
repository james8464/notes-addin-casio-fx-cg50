#!/usr/bin/env python3
"""Manual MadAsMaths standard-topic checks transcribed from rendered PDFs."""

from __future__ import annotations

import json
import argparse
import subprocess
import sys
from pathlib import Path
from typing import Any

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
REPORT = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "manual_cases_latest.txt"
FILTERED_REPORT = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "manual_cases_filtered_latest.txt"
REMOVED_FEATURE_MARKERS = (
    "mean_value(", "volume_x(", "volume_y(", "area_between(",
    "param_area(", "param_area_y(", "param_volume",
    "ztest(", "covariance(", "correlation(", "linear_regression(",
    "median(", "mean(", "quartiles(", "stddev(", "stdev(",
    "method=summary", "method=weierstrass", "method=tabular",
    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "arcosh",
    "taylor(", "maclaurin(", "method=third", "method=param_second", "mode:5",
    "third_derivative", "fourth_derivative", "higher_derivative", "d3y", "d4y",
)


def removed_case(case: dict[str, Any]) -> bool:
    text = " ".join(str(x) for x in case.get("args", []))
    text += " " + str(case.get("id", "")) + " " + str(case.get("item", ""))
    text += " " + " ".join(str(x) for x in case.get("needles", []))
    for raw in case.get("variants", []):
        if isinstance(raw, dict):
            text += " " + " ".join(str(x) for x in raw.get("args", []))
            text += " " + " ".join(str(x) for x in raw.get("needles", []))
        else:
            text += " " + " ".join(str(x) for x in raw)
    lo = text.lower()
    return any(marker.lower() in lo for marker in REMOVED_FEATURE_MARKERS)


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


def parse_args() -> argparse.Namespace:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", action="append", default=[], help="Only check rows whose source_pdf contains this text.")
    ap.add_argument("--id-prefix", action="append", default=[], help="Only check rows whose id starts with this prefix.")
    ap.add_argument("--last", type=int, default=0, help="Only check the last N JSONL rows after other filters.")
    return ap.parse_args()


def select_cases(cases: list[dict[str, Any]], args: argparse.Namespace) -> list[dict[str, Any]]:
    out = cases
    if args.source:
        needles = [s.lower() for s in args.source]
        out = [c for c in out if any(n in str(c.get("source_pdf", "")).lower() for n in needles)]
    if args.id_prefix:
        prefixes = tuple(args.id_prefix)
        out = [c for c in out if str(c.get("id", "")).startswith(prefixes)]
    if args.last:
        out = out[-args.last:]
    return out


def main() -> int:
    args = parse_args()
    if not HOST.exists():
        print(f"FAIL host missing: {HOST}")
        return 1
    all_cases = [json.loads(line) for line in CASES.read_text().splitlines() if line.strip()]
    cases = select_cases(all_cases, args)
    report_path = FILTERED_REPORT if (args.source or args.id_prefix or args.last) else REPORT
    report_path.parent.mkdir(parents=True, exist_ok=True)
    bad: list[str] = []
    lines = ["MadAsMaths standard manual cases", f"selected={len(cases)} total={len(all_cases)}", ""]
    if not cases:
        print("FAIL no cases selected")
        report_path.write_text("\n".join(lines + ["summary: bad=1 total=0"]) + "\n")
        return 1
    for case in cases:
        if removed_case(case):
            print("SKIP removed", case["id"])
            lines.append(f"SKIP removed {case['id']} {case['source_pdf']} Q{case['qid']}.{case['item']}")
            lines.append("")
            continue
        if case.get("status") == "unsupported-ok":
            print("OK", case["id"])
            lines.append(f"OK {case['id']} {case['source_pdf']} Q{case['qid']}.{case['item']}")
            lines.append("  unsupported-ok: " + str(case.get("notes", "reviewed; no unique host command")))
            lines.append("")
            continue
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
    report_path.write_text("\n".join(lines) + "\n")
    print(f"report {report_path}")
    print(f"done bad {len(bad)} / {len(cases)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

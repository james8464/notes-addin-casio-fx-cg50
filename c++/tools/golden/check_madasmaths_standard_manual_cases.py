#!/usr/bin/env python3
"""Manual MadAsMaths standard-topic checks transcribed from rendered PDFs."""

from __future__ import annotations

import json
import argparse
import concurrent.futures as cf
import os
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
    ap.add_argument("--workers", type=int, default=int(os.environ.get("CASIO_AUDIT_WORKERS", str(os.cpu_count() or 1))))
    ap.add_argument("--quiet", action="store_true", help="Only print failures and summary; full details still go to report.")
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


def run_command(task: tuple[int, str, list[str], list[str], list[str]]) -> tuple[int, str, list[str], list[str], str]:
    case_i, label, args, needles, banned_tokens = task
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    missing = [m for m in needles if not markers_present(out, [m])]
    banned = [m for m in banned_tokens if m in out]
    if proc.returncode:
        missing.append(f"returncode={proc.returncode}")
    return case_i, label, missing, banned, out


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
    case_status: list[str] = ["pending"] * len(cases)
    case_lines: list[list[str]] = [[] for _ in cases]
    tasks: list[tuple[int, str, list[str], list[str], list[str]]] = []
    for case_i, case in enumerate(cases):
        if removed_case(case):
            if not args.quiet:
                print("SKIP removed", case["id"])
            case_status[case_i] = "skip"
            case_lines[case_i] = [
                f"SKIP removed {case['id']} {case['source_pdf']} Q{case['qid']}.{case['item']}",
                "",
            ]
            continue
        if case.get("status") == "unsupported-ok":
            if not args.quiet:
                print("OK", case["id"])
            case_status[case_i] = "ok"
            case_lines[case_i] = [
                f"OK {case['id']} {case['source_pdf']} Q{case['qid']}.{case['item']}",
                "  unsupported-ok: " + str(case.get("notes", "reviewed; no unique host command")),
                "",
            ]
            continue
        for label, cmd_args, needles, banned_tokens in command_specs(case):
            tasks.append((case_i, label, cmd_args, needles, banned_tokens))

    results_by_case: list[list[tuple[str, list[str], list[str], str]]] = [[] for _ in cases]
    with cf.ThreadPoolExecutor(max_workers=max(1, args.workers)) as pool:
        for case_i, label, missing, banned, out in pool.map(run_command, tasks):
            results_by_case[case_i].append((label, missing, banned, out))

    for case_i, case in enumerate(cases):
        if case_status[case_i] != "pending":
            lines.extend(case_lines[case_i])
            continue
        results = results_by_case[case_i]
        case_bad = any(missing or banned for _label, missing, banned, _out in results)
        status = "FAIL" if case_bad else "OK"
        if case_bad or not args.quiet:
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

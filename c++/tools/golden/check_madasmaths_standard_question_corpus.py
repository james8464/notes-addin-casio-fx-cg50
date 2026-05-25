#!/usr/bin/env python3
"""Check the generated MadAsMaths standard-topic Markdown corpus."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any

import check_madasmaths_standard_topics_audit as audit
from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
LEDGER = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "ledger_latest.jsonl"
CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
CORPUS = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_question_corpus.md"
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
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


def fail(msg: str) -> int:
    print("FAIL " + msg, file=sys.stderr)
    return 1


def load_jsonl(path: Path) -> list[dict[str, Any]]:
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def source_key(row: dict[str, Any]) -> str:
    return f"{row['topic_page']}/{row['pdf']}"


def row_marker(row: dict[str, Any]) -> str:
    part = row.get("part") or ""
    return f"{source_key(row)}|ord={row['ordinal']}|qid={row['qid']}|part={part}"


def case_key(case: dict[str, Any]) -> tuple[str, str, str, str]:
    return (
        str(case.get("source_pdf")),
        str(case.get("qid")),
        str(case.get("ordinal", "")),
        str(case.get("item", "")),
    )


def case_matches(row: dict[str, Any], case: dict[str, Any]) -> bool:
    source = case.get("source_pdf")
    canonical = audit.canonical_source(str(row["topic_page"]), str(row["pdf"]))
    if source not in {source_key(row), canonical}:
        return False
    case_ordinal = str(case.get("ordinal", ""))
    row_ordinal = str(row["ordinal"])
    if case_ordinal and case_ordinal == row_ordinal:
        return True
    if case_ordinal and case_ordinal != row_ordinal:
        return False
    case_qid = re.sub(r"(?i)^hard", "", str(case.get("qid")))
    if case_qid != str(row["question"]):
        return False
    part = str(row.get("part") or "")
    item = str(case.get("item") or "")
    return not part or not item or part == item


def run_host(case: dict[str, Any]) -> list[str]:
    proc = subprocess.run([str(HOST), *case["args"]], cwd=REPO, text=True, capture_output=True, timeout=12)
    output = proc.stdout + proc.stderr
    bad = [needle for needle in case.get("needles", []) if not markers_present(output, [needle])]
    bad.extend(f"forbidden:{token}" for token in case.get("banned", []) if token in output)
    if proc.returncode:
        bad.append(f"returncode={proc.returncode}")
    return bad


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


def outside_standard_corpus(case: dict[str, Any]) -> bool:
    src = str(case.get("source_pdf", ""))
    return src.startswith("MadAsMaths A-level booklets/")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--skip-host", action="store_true")
    args = ap.parse_args()

    if not LEDGER.exists():
        print(f"SKIP madasmaths standard question corpus: missing ledger {LEDGER}")
        return 0
    if not CORPUS.exists():
        print(f"SKIP madasmaths standard question corpus: generate with {CORPUS.name} builder")
        return 0
    if not CASES.exists():
        return fail(f"missing manual cases: {CASES}")
    if not args.skip_host and not HOST.exists():
        return fail(f"missing host: {HOST}")

    rows = load_jsonl(LEDGER)
    cases = load_jsonl(CASES)
    text = CORPUS.read_text(encoding="utf-8")
    markers = re.findall(r"<!-- madas-row: ([^>]+) -->", text)
    marker_counts = Counter(markers)
    errors: list[str] = []

    if len(markers) != len(rows):
        errors.append(f"marker_count {len(markers)} != ledger_rows {len(rows)}")

    for row in rows:
        marker = row_marker(row)
        if marker_counts[marker] != 1:
            errors.append(f"bad marker count {marker_counts[marker]} for {marker}")
        title = f"### Q{row['qid']}"
        if title not in text:
            errors.append(f"missing title {title}")

    for case in cases:
        if removed_case(case):
            continue
        if outside_standard_corpus(case):
            continue
        args_list = case.get("args")
        if not args_list:
            continue
        command = " ".join(str(x) for x in args_list)
        if command not in text:
            errors.append(f"case command missing from corpus: {case.get('id', case_key(case))}")

    done_rows = [row for row in rows if row["status"] == "done"]
    for row in done_rows:
        if row.get("command"):
            continue
        if not any(case_matches(row, case) for case in cases):
            errors.append(f"done row has no matching manual case: {row_marker(row)}")

    if not args.skip_host:
        for case in cases:
            if removed_case(case):
                continue
            if not case.get("args"):
                continue
            bad = run_host(case)
            if bad:
                errors.append(f"host failed {case.get('id', case_key(case))}: {bad}")

    if errors:
        for line in errors[:80]:
            print(line, file=sys.stderr)
        return fail(f"corpus check failed errors={len(errors)}")

    counts = Counter(str(row["status"]) for row in rows)
    print(
        "OK madasmaths standard question corpus "
        f"rows={len(rows)} cases={len(cases)} "
        + " ".join(f"{key}={counts[key]}" for key in sorted(counts))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

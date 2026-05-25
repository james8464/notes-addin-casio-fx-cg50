#!/usr/bin/env python3
"""Report downloaded MadAsMaths source coverage against manual audit trackers."""

from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
MANIFEST = REPO / "c++" / "tests" / "reports" / "a_level_source_downloads" / "manifest_latest.jsonl"
STANDARD_LEDGER = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "ledger_latest.jsonl"
FULL_LEDGER = REPO / "c++" / "tests" / "reports" / "madasmaths_full_audit" / "ledger_latest.jsonl"
MANUAL_CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "madasmaths_download_coverage"
REPORT = REPORT_DIR / "summary_latest.md"


def load_jsonl(path: Path) -> list[dict[str, object]]:
    if not path.exists():
        return []
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def is_question_pdf(name: str) -> bool:
    low = name.lower()
    return low.endswith(".pdf") and not any(
        token in low
        for token in ("marks", "solutions", "student_version", "student-version", "_condense")
    )


def manual_refs() -> tuple[set[str], set[str]]:
    refs: set[str] = set()
    booklet_refs: set[str] = set()
    for row in load_jsonl(MANUAL_CASES):
        if str(row.get("coverage", "")) == "partial":
            continue
        src = str(row.get("source_pdf", ""))
        if src.startswith(("MadAsMaths papers/", "legacy/")):
            refs.add(Path(src).name.lower())
        if src.startswith("MadAsMaths A-level booklets/"):
            parts = Path(src).parts
            if len(parts) >= 3:
                booklet_refs.add(f"{parts[-2].lower()}/{parts[-1].lower()}")
    for row in load_jsonl(FULL_LEDGER):
        paper = str(row.get("paper", ""))
        if paper.startswith("mp2_") and row.get("verdict") == "PASS":
            refs.add(paper + ".pdf")
    return refs, booklet_refs


def standard_refs() -> set[str]:
    refs: set[str] = set()
    for row in load_jsonl(STANDARD_LEDGER):
        if str(row.get("status", "")) in {"done", "unsupported-ok", "host-pass"}:
            refs.add(str(row.get("pdf", "")).lower())
    return refs


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--strict", action="store_true", help="fail if any downloaded MadAsMaths question PDF lacks manual coverage")
    args = ap.parse_args()

    if not MANIFEST.exists():
        print(f"SKIP madasmaths download coverage: missing {MANIFEST}")
        return 0

    manifest = load_jsonl(MANIFEST)
    manual, booklet_manual = manual_refs()
    standard = standard_refs()
    rows: list[dict[str, object]] = []
    counts: Counter[str] = Counter()
    by_page: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)

    for item in manifest:
        family = str(item.get("family", ""))
        if not family.startswith("madas_"):
            continue
        name = Path(str(item.get("name", ""))).name
        if not is_question_pdf(name):
            continue
        page = str(item.get("page", ""))
        status = "gap"
        reason = "not in manual audit trackers"
        if family == "madas_standard" and name.lower() in standard:
            status = "covered"
            reason = "standard-topic strict ledger"
        elif family == "madas_booklets_a_level" and name.lower() in standard:
            status = "covered"
            reason = "duplicate of standard-topic strict ledger"
        elif family == "madas_booklets_a_level" and f"{page.lower()}/{name.lower()}" in booklet_manual:
            status = "covered"
            reason = "manual booklet case ledger"
        elif family == "madas_iygb" and name.lower() in manual:
            status = "covered"
            reason = "manual paper case ledger"
        rows.append({"family": family, "page": page, "name": name, "status": status, "reason": reason})
        counts[status] += 1
        by_page[(family, page)][status] += 1

    gaps = [row for row in rows if row["status"] == "gap"]
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    lines = [
        "# MadAsMaths Download Coverage",
        "",
        f"- downloaded question PDFs: {len(rows)}",
        f"- covered: {counts['covered']}",
        f"- gaps: {counts['gap']}",
        "",
        "| family | page | covered | gap |",
        "|---|---|---:|---:|",
    ]
    for (family, page), counter in sorted(by_page.items()):
        lines.append(f"| {family} | {page} | {counter['covered']} | {counter['gap']} |")
    if gaps:
        lines.extend(["", "## First Gaps", ""])
        for row in gaps[:80]:
            lines.append(f"- `{row['family']}/{row['page']}/{row['name']}`")
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")

    msg = f"madasmaths download coverage downloaded={len(rows)} covered={counts['covered']} gaps={counts['gap']} report={REPORT}"
    if args.strict and gaps:
        print("FAIL " + msg)
        return 1
    print("OK " + msg)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Verify official Pearson 9MA0 question papers have tracker row coverage."""

from __future__ import annotations

from collections import Counter, defaultdict
import json
from pathlib import Path
import re
import subprocess

from check_edexcel_public_paper_corpus import kind, valid_pdf
from download_a_level_audit_sources import PEARSON_9MA0_PUBLIC


REPO = Path(__file__).resolve().parents[3]
PDF_DIR = Path.home() / "Downloads" / "Edexcel A Level Maths past papers"
TRACKER = REPO / "c++" / "tools" / "golden" / "a_level_audit_tracker.jsonl"
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "edexcel_public_paper_corpus"
REPORT = REPORT_DIR / "question_coverage_latest.txt"
OK_STATUS = {"host-pass", "unsupported-ok"}


def pdf_text(path: Path) -> str:
    try:
        proc = subprocess.run(
            ["pdftotext", "-layout", str(path), "-"],
            text=True,
            capture_output=True,
            timeout=30,
        )
    except FileNotFoundError as exc:
        raise RuntimeError("pdftotext missing") from exc
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError(f"pdftotext timed out: {path.name}") from exc
    if proc.returncode:
        detail = (proc.stderr or proc.stdout).strip().replace("\n", " ")[:160]
        raise RuntimeError(f"pdftotext failed: {path.name}: {detail}")
    return proc.stdout


def question_count_from_paper(text: str) -> tuple[int, str]:
    for pattern in (
        r"There\s+are\s+(\d{1,2})\s+questions?\s+in\s+this\s+question\s+paper",
        r"There\s+are\s+(\d{1,2})\s+questions?\.",
        r"There\s+are\s+(\d{1,2})\s+questions?",
    ):
        match = re.search(pattern, text, re.IGNORECASE)
        if match:
            n = int(match.group(1))
            if 1 <= n <= 20:
                return n, "question-front"
    nums = [
        int(m.group(1))
        for m in re.finditer(r"Total\s+for\s+Question\s+(\d{1,2})\b", text, re.IGNORECASE)
    ]
    nums = [n for n in nums if 1 <= n <= 20]
    if nums:
        return max(nums), "question-total"
    return 0, "missing"


def component(name: str) -> str:
    match = re.search(r"9ma0[-_](\d{2})", name.lower())
    return match.group(1) if match else ""


def year(name: str) -> str:
    match = re.search(r"(20\d{2}|2018)", name.lower())
    return match.group(1) if match else ""


def mark_scheme_for(question_name: str) -> Path | None:
    comp = component(question_name)
    yr = year(question_name)
    for name in PEARSON_9MA0_PUBLIC:
        if kind(name) == "mark_scheme" and component(name) == comp and year(name) == yr:
            path = PDF_DIR / name
            if path.exists():
                return path
    return None


def question_count_from_mark_scheme(text: str) -> tuple[int, str]:
    nums = [
        int(m.group(1))
        for m in re.finditer(r"(?m)^\s*(\d{1,2})\s*\([a-z]\)", text, re.IGNORECASE)
    ]
    nums = [n for n in nums if 1 <= n <= 20]
    if nums:
        return max(nums), "mark-scheme-part"
    nums = [
        int(m.group(1))
        for m in re.finditer(r"(?mi)^\s*Question\s+(\d{1,2})\b", text)
    ]
    nums = [n for n in nums if 1 <= n <= 20]
    if nums:
        return max(nums), "mark-scheme-question"
    return 0, "missing"


def expected_question_count(question_path: Path) -> tuple[int, str]:
    n, source = question_count_from_paper(pdf_text(question_path))
    if n:
        return n, source
    mark_scheme = mark_scheme_for(question_path.name)
    if mark_scheme is None:
        return 0, "missing-mark-scheme"
    n, source = question_count_from_mark_scheme(pdf_text(mark_scheme))
    if n:
        return n, source
    return 0, "missing"


def row_question(row: dict[str, object]) -> int | None:
    match = re.search(r"\d{1,2}", str(row.get("question", "")))
    if not match:
        return None
    n = int(match.group(0))
    return n if 1 <= n <= 20 else None


def tracker_rows_by_pdf() -> dict[str, list[tuple[int | None, str, int]]]:
    rows: dict[str, list[tuple[int | None, str, int]]] = defaultdict(list)
    if not TRACKER.exists():
        return rows
    for line_no, line in enumerate(TRACKER.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        row = json.loads(line)
        basename = Path(str(row.get("source_pdf", ""))).name.lower()
        rows[basename].append((row_question(row), str(row.get("status", "")), line_no))
    return rows


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    question_names = [name for name in PEARSON_9MA0_PUBLIC if kind(name) == "question"]
    rows_by_pdf = tracker_rows_by_pdf()
    counts = Counter()
    bad: list[str] = []
    lines: list[str] = []

    for name in question_names:
        path = PDF_DIR / name
        if not valid_pdf(path):
            bad.append(f"{name}: missing/invalid PDF")
            continue
        try:
            n, source = expected_question_count(path)
        except RuntimeError as exc:
            bad.append(f"{name}: {exc}")
            continue
        if not n:
            bad.append(f"{name}: cannot infer question count")
            continue
        rows = rows_by_pdf.get(name.lower(), [])
        covered = {q for q, _, _ in rows if q is not None}
        expected = set(range(1, n + 1))
        missing = sorted(expected - covered)
        extra = sorted(q for q in covered if q not in expected)
        malformed = [f"line {line_no} status={status}" for q, status, line_no in rows if q is None]
        bad_status = [
            f"line {line_no} q{q} status={status}"
            for q, status, line_no in rows
            if status not in OK_STATUS
        ]
        counts[source] += 1
        lines.append(
            f"{name}: questions={n} via={source} tracked={len(covered & expected)} missing={missing} extra={extra}"
        )
        if missing:
            bad.append(f"{name}: missing tracker rows q{missing}")
        if extra:
            bad.append(f"{name}: extra tracker q{extra}")
        if malformed:
            bad.append(f"{name}: malformed question rows {'; '.join(malformed)}")
        if bad_status:
            bad.append(f"{name}: bad status {'; '.join(bad_status)}")

    header = [
        f"official_question_papers={len(question_names)} bad={len(bad)}",
        "count_sources: " + " ".join(f"{k}={counts[k]}" for k in sorted(counts)),
    ]
    if bad:
        header.append("bad:")
        header.extend(bad)
    REPORT.write_text("\n".join(header + lines) + "\n", encoding="utf-8")

    if bad:
        print("FAIL edexcel question audit coverage")
        print(REPORT.read_text(encoding="utf-8"), end="")
        return 1
    print(f"OK edexcel question audit coverage papers={len(question_names)}")
    print(f"report {REPORT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

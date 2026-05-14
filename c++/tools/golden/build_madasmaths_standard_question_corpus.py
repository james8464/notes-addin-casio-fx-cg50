#!/usr/bin/env python3
"""Build a Markdown corpus for local MadAsMaths standard-topic questions."""

from __future__ import annotations

import argparse
import bisect
import json
import re
import shutil
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

import check_madasmaths_standard_topics_audit as audit


REPO = Path(__file__).resolve().parents[3]
PDF_DIR = Path.home() / "Downloads" / "MadAsMaths standard topics"
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit"
RENDER_DIR = REPORT_DIR / "rendered_standard_pages"
LEDGER = REPORT_DIR / "ledger_latest.jsonl"
MANUAL_CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
CORPUS = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_question_corpus.md"
DRAFT_SOURCES = (REPO / "cas_io", REPO)


def fail(msg: str) -> int:
    print("FAIL " + msg, file=sys.stderr)
    return 1


def run(cmd: list[str], timeout: int = 60) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=REPO, text=True, capture_output=True, timeout=timeout)


def pdf_text(path: Path) -> str:
    proc = run(["pdftotext", "-layout", str(path), "-"], timeout=60)
    if proc.returncode:
        raise RuntimeError(proc.stderr.strip() or f"pdftotext failed: {path}")
    return proc.stdout


def pdf_page_count(path: Path) -> int:
    proc = run(["pdfinfo", str(path)], timeout=30)
    if proc.returncode:
        raise RuntimeError(proc.stderr.strip() or f"pdfinfo failed: {path}")
    for line in proc.stdout.splitlines():
        if line.startswith("Pages:"):
            return int(line.split(":", 1)[1].strip())
    raise RuntimeError(f"pdfinfo did not report page count: {path}")


def render_pdf(path: Path, out_dir: Path, force: bool) -> list[Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    existing = sorted(out_dir.glob("page-*.png"))
    if existing and not force:
        return existing
    for old in existing:
        old.unlink()
    for page in range(1, pdf_page_count(path) + 1):
        prefix = out_dir / f"page-{page:03d}"
        proc = run(
            ["pdftoppm", "-png", "-r", "120", "-f", str(page), "-l", str(page), str(path), str(prefix)],
            timeout=45,
        )
        if proc.returncode:
            raise RuntimeError(proc.stderr.strip() or f"pdftoppm failed: {path} page {page}")
        generated = sorted(out_dir.glob(f"page-{page:03d}-*.png"))
        if generated:
            generated[0].rename(out_dir / f"page-{page:03d}.png")
    return sorted(out_dir.glob("page-*.png"))


def normalize_text(text: str) -> str:
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    lines = [line.rstrip() for line in text.splitlines()]
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    return "\n".join(lines)


def fence_text(text: str) -> str:
    text = normalize_text(text)
    if "```" in text:
        text = text.replace("```", "'''")
    return text or "(no extracted text)"


def split_questions_with_pages(text: str) -> list[dict[str, Any]]:
    page_breaks = [m.start() for m in re.finditer("\f", text)]
    question_hits = list(re.finditer(r"(?im)^Question\s+(\d+)\b", text))
    item_hits = list(re.finditer(r"(?m)^\s*(\d{1,3})\.\s*(?:\S|$)", text))
    hits = question_hits
    prefix = ""
    if (not question_hits or len(question_hits) <= 2) and len(item_hits) > 5:
        hits = item_hits
        prefix = question_hits[0].group(1) + "." if question_hits else ""
    out: list[dict[str, Any]] = []
    for i, hit in enumerate(hits):
        end = hits[i + 1].start() if i + 1 < len(hits) else len(text)
        page = bisect.bisect_right(page_breaks, hit.start()) + 1
        out.append({
            "qid": prefix + hit.group(1),
            "page": page,
            "text": text[hit.start():end].replace("\f", "\n"),
        })
    return out


def load_ledger() -> list[dict[str, Any]]:
    return [json.loads(line) for line in LEDGER.read_text().splitlines() if line.strip()]


def load_cases() -> list[dict[str, Any]]:
    if not MANUAL_CASES.exists():
        return []
    return [json.loads(line) for line in MANUAL_CASES.read_text().splitlines() if line.strip()]


def load_draft_notes() -> dict[str, str]:
    notes: dict[str, str] = {}
    for root in DRAFT_SOURCES:
        if not root.exists():
            continue
        for path in sorted(root.glob("*.json")):
            if path.name == "compile_commands.json":
                continue
            try:
                raw = json.loads(path.read_text())
            except json.JSONDecodeError:
                continue
            if isinstance(raw, dict):
                items = raw.items()
            elif isinstance(raw, list):
                items = ((str(item.get("qid", "")), item) for item in raw if isinstance(item, dict))
            else:
                continue
            for qid, value in items:
                match = re.search(r"\d+", str(qid))
                if not match:
                    continue
                key = match.group(0)
                notes[key] = json.dumps(value, ensure_ascii=False)
    return notes


def source_key(row: dict[str, Any]) -> str:
    return f"{row['topic_page']}/{row['pdf']}"


def row_marker(row: dict[str, Any]) -> str:
    part = row.get("part") or ""
    return f"{source_key(row)}|ord={row['ordinal']}|qid={row['qid']}|part={part}"


def case_matches(row: dict[str, Any], case: dict[str, Any]) -> bool:
    source = case.get("source_pdf")
    canonical = audit.canonical_source(str(row["topic_page"]), str(row["pdf"]))
    if source not in {source_key(row), canonical}:
        return False
    if str(case.get("qid")) != str(row["question"]):
        return False
    if str(case.get("ordinal", "")) not in {"", str(row["ordinal"])}:
        return False
    part = str(row.get("part") or "")
    item = str(case.get("item") or "")
    return not part or not item or part == item


def decision_for(row: dict[str, Any]) -> str:
    if row["status"] == "done":
        return "host-verified"
    if row["status"] == "unsupported-ok":
        return "unsupported"
    if row["status"] in {"needs-fix", "host-pass"}:
        return str(row["status"])
    return "needs-route"


def render_link(row: dict[str, Any], page: int, rendered: dict[str, list[Path]]) -> str:
    pages = rendered.get(source_key(row), [])
    if page <= 0 or page > len(pages):
        return ""
    return str(pages[page - 1].relative_to(REPO))


def markdown_for_case(case: dict[str, Any]) -> list[str]:
    lines = [
        f"- ID: `{case.get('id', '')}`",
        "- Input: `" + " ".join(str(x) for x in case.get("args", [])) + "`",
    ]
    needles = list(case.get("needles", []))
    if needles:
        lines.append("- Expected answer markers:")
        lines.extend(f"  - `{needle}`" for needle in needles)
        lines.append("- Suggested working needles:")
        lines.extend(f"  - `{needle}`" for needle in needles)
    banned = list(case.get("banned", []))
    if banned:
        lines.append("- Banned markers:")
        lines.extend(f"  - `{needle}`" for needle in banned)
    return lines


def build_corpus(rows: list[dict[str, Any]], render: bool, force_render: bool) -> str:
    cases = load_cases()
    draft_notes = load_draft_notes()
    cases_by_row: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        for case in cases:
            if case_matches(row, case):
                cases_by_row[row_marker(row)].append(case)

    text_blocks: dict[tuple[str, int], dict[str, Any]] = {}
    rendered: dict[str, list[Path]] = {}
    for pdf in sorted(PDF_DIR.glob("*/*.pdf")):
        source = f"{pdf.parent.name}/{pdf.name}"
        text = pdf_text(pdf)
        for ordinal, block in enumerate(split_questions_with_pages(text), 1):
            text_blocks[(source, ordinal)] = block
        if render:
            out_dir = RENDER_DIR / pdf.parent.name / pdf.stem
            rendered[source] = render_pdf(pdf, out_dir, force_render)

    counts = Counter(str(row["status"]) for row in rows)
    topic_counts = Counter()
    for row in rows:
        topic_counts.update(str(topic) for topic in row.get("topics", []))

    lines = [
        "# MadAsMaths Standard Question Corpus",
        "",
        "Canonical Markdown index for local MadAsMaths standard-topic questions.",
        "",
        "## Summary",
        f"- PDFs: {len({source_key(row) for row in rows})}",
        f"- Questions/rows: {len(rows)}",
        "- Status: " + " ".join(f"{key}:{counts[key]}" for key in sorted(counts)),
        "- Topics: " + " ".join(f"{key}:{topic_counts[key]}" for key in sorted(topic_counts)),
        f"- Executable golden source: `{MANUAL_CASES.relative_to(REPO)}`",
        "",
    ]

    by_pdf: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        by_pdf[source_key(row)].append(row)

    for source in sorted(by_pdf):
        lines.extend([f"## {source}", ""])
        for row in by_pdf[source]:
            marker = row_marker(row)
            block = text_blocks.get((source, int(row["ordinal"])), {})
            page = int(block.get("page", 0) or 0)
            image = render_link(row, page, rendered)
            decision = decision_for(row)
            title_part = f".{row['part']}" if row.get("part") else ""
            lines.extend([
                f"### Q{row['qid']}{title_part} (ordinal {row['ordinal']})",
                f"<!-- madas-row: {marker} -->",
                f"- Page: {page if page else 'unknown'}",
                f"- Topics: {', '.join(str(t) for t in row.get('topics', [])) or 'none'}",
                f"- Ledger status: `{row['status']}`",
                f"- Testability: `{decision}`",
                f"- Notes: {row.get('notes', '')}",
            ])
            if image:
                lines.append(f"- Rendered page: `{image}`")
            lines.extend(["", "#### Extracted Question Text", "```text", fence_text(str(block.get("text", ""))), "```", ""])
            row_cases = cases_by_row.get(marker, [])
            if row_cases:
                lines.append("#### Program Test")
                for i, case in enumerate(row_cases, 1):
                    if len(row_cases) > 1:
                        lines.append(f"- Case {i}")
                    lines.extend(markdown_for_case(case))
                lines.append("")
            elif decision == "unsupported":
                lines.extend([
                    "#### Program Test",
                    "- Not applicable: non-calculator, graph/proof-only, or no unique CAS command.",
                    "",
                ])
            else:
                lines.extend([
                    "#### Program Test",
                    "- Input: `TODO`",
                    "- Expected answer markers: `TODO`",
                    "- Suggested working needles: `TODO`",
                    "",
                ])
            draft_note = draft_notes.get(str(row["question"]))
            if draft_note:
                lines.extend(["#### Draft Notes", "```json", draft_note, "```", ""])
            lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--refresh-ledger", action="store_true")
    ap.add_argument("--render", action="store_true")
    ap.add_argument("--force-render", action="store_true")
    args = ap.parse_args()

    if not shutil.which("pdftotext"):
        return fail("pdftotext missing")
    if not shutil.which("pdfinfo"):
        return fail("pdfinfo missing")
    if args.render and not shutil.which("pdftoppm"):
        return fail("pdftoppm missing")
    if not PDF_DIR.exists():
        return fail(f"local PDFs missing: {PDF_DIR}")
    if args.refresh_ledger or not LEDGER.exists():
        rc = audit.main()
        if rc:
            return rc
    rows = load_ledger()
    if not rows:
        return fail(f"empty ledger: {LEDGER}")
    CORPUS.write_text(build_corpus(rows, args.render, args.force_render), encoding="utf-8")
    print(f"OK wrote {CORPUS} rows={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

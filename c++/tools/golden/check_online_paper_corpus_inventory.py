#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
CORPUS = REPO / "c++" / "tests" / "reports" / "online_paper_corpus"
REPORT_JSON = CORPUS / "inventory_latest.json"
REPORT_MD = CORPUS / "inventory_latest.md"
MANIFEST = CORPUS / "manifest_latest.jsonl"


def classify_text(s: str) -> str:
    s = s.lower()
    if re.search(r"mark[-_ ]?scheme|(?<![a-z0-9])ms(?=\.|_|-|\(|$)|rms|msc", s):
        return "ms"
    if any(t in s for t in ("question-paper", "question_paper", "_qp", "-qp", " que_", " qp.", "past_paper")):
        return "qp"
    if any(t in s for t in ("solution", "worked", "answer")):
        return "solution"
    return "other"


def classify(path: Path) -> str:
    return classify_text(path.name)


def load_manifest() -> list[dict[str, object]]:
    if not MANIFEST.exists():
        return []
    rows: list[dict[str, object]] = []
    for lineno, line in enumerate(MANIFEST.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            item = json.loads(line)
        except json.JSONDecodeError as exc:
            print(f"FAIL invalid manifest JSONL {MANIFEST}:{lineno}: {exc.msg}", file=sys.stderr)
            raise SystemExit(1)
        if not isinstance(item, dict):
            print(f"FAIL manifest row is not object {MANIFEST}:{lineno}", file=sys.stderr)
            raise SystemExit(1)
        rows.append(item)
    return rows


def count_questions(txt: Path) -> int:
    try:
        text = txt.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return 0
    # Conservative OCR signal only. Not used as proof of audited coverage.
    return len(set(re.findall(r"(?mi)^\s*(?:question\s*)?([0-9]{1,2})[\).]\s+", text)))


def main() -> int:
    if not CORPUS.exists():
        print(f"SKIP online corpus inventory: missing corpus dir {CORPUS}")
        return 0

    manifest_rows = load_manifest()
    manifest_by_source: dict[str, list[dict[str, object]]] = {}
    for item in manifest_rows:
        manifest_by_source.setdefault(str(item.get("source", "")), []).append(item)

    sources: list[dict[str, object]] = []
    totals = {"pdf": 0, "pdf_files": 0, "txt": 0, "qp": 0, "ms": 0, "solution": 0, "other": 0, "question_markers": 0}
    source_names = sorted({p.name for p in CORPUS.iterdir() if p.is_dir()} | set(manifest_by_source))
    for source_name in source_names:
        source_dir = CORPUS / source_name
        pdfs = sorted((source_dir / "pdfs").glob("*.pdf"))
        txts = sorted((source_dir / "txt").glob("*.txt"))
        mrows = [m for m in manifest_by_source.get(source_name, []) if m.get("status") == "download"]
        kinds = {"qp": 0, "ms": 0, "solution": 0, "other": 0}
        if pdfs:
            for p in pdfs:
                kinds[classify(p)] += 1
        elif mrows:
            for item in mrows:
                kinds[classify_text(" ".join(str(item.get(k, "")) for k in ("pdf", "url", "label", "kind")))] += 1
        else:
            for p in txts:
                kinds[classify(p)] += 1
        pdf_count = len(pdfs) if pdfs else len(mrows)
        q_markers = sum(count_questions(t) for t in txts)
        row = {
            "source": source_name,
            "pdf": pdf_count,
            "pdf_files": len(pdfs),
            "txt": len(txts),
            "qp": kinds["qp"],
            "ms": kinds["ms"],
            "solution": kinds["solution"],
            "other": kinds["other"],
            "question_markers": q_markers,
        }
        sources.append(row)
        totals["pdf"] += pdf_count
        totals["pdf_files"] += len(pdfs)
        totals["txt"] += len(txts)
        totals["question_markers"] += q_markers
        for k in ("qp", "ms", "solution", "other"):
            totals[k] += kinds[k]

    manifest = len(manifest_rows)
    payload = {"totals": totals, "manifest_rows": manifest, "sources": sources}
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "# Online Paper Corpus Inventory",
        "",
        f"- pdf: {totals['pdf']}",
        f"- pdf file cache: {totals['pdf_files']}",
        f"- txt: {totals['txt']}",
        f"- manifest rows: {manifest}",
        f"- question-marker hits: {totals['question_markers']}",
        "",
        "| source | pdf | txt | qp | ms | sol | other | q-markers |",
        "|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for row in sources:
        lines.append(
            f"| {row['source']} | {row['pdf']} | {row['txt']} | {row['qp']} | {row['ms']} | "
            f"{row['solution']} | {row['other']} | {row['question_markers']} |"
        )
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")

    if totals["pdf"] <= 0 or totals["txt"] <= 0:
        print("FAIL online paper corpus is empty", file=sys.stderr)
        return 1
    if totals["qp"] <= 0 or totals["ms"] <= 0:
        print("FAIL online paper corpus lacks qp/ms files", file=sys.stderr)
        return 1

    drift = f" manifest={manifest}" if manifest != totals["pdf"] else ""
    print(
        f"OK online corpus inventory pdf={totals['pdf']} txt={totals['txt']} "
        f"qp={totals['qp']} ms={totals['ms']} qmarkers={totals['question_markers']}{drift}"
    )
    print(f"report {REPORT_MD}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

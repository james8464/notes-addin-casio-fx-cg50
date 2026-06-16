#!/usr/bin/env python3
from __future__ import annotations

import os
import re
from pathlib import Path

try:
    from docx import Document
except ModuleNotFoundError:
    print("SKIP notes docx coverage: python-docx unavailable")
    raise SystemExit(0)

ROOT = Path(__file__).resolve().parents[1]
NOTES = ROOT / "calculator_files" / "NOTES"
DOCX_DIR = Path(os.environ.get("NOTES_DOCX_DIR", "/Users/james/Downloads"))
DOCX_NAMES = [
    "Big Data.docx",
    "Computer organisation.docx",
    "Computer systems.docx",
    "Data representation.docx",
    "Databases.docx",
    "Functional programming.docx",
    "Networking.docx",
]


def normalise(text: str) -> str:
    text = (
        text.replace("\u00a0", " ")
        .replace("\u2018", "'")
        .replace("\u2019", "'")
        .replace("\u201c", '"')
        .replace("\u201d", '"')
        .replace("\u2013", "-")
        .replace("\u2014", "-")
        .replace("\u2192", "->")
        .replace("\u00d7", "x")
        .replace("\u211d", "R")
    )
    text = text.replace(" the have ", " they have ").replace(" nd returns ", " and returns ")
    return re.sub(r"[^a-z0-9]+", " ", text.lower()).strip()


def note_detail_cells(row) -> list[str]:
    cells = [cell.text.strip() for cell in row.cells]
    if not any(cells):
        return []
    if len(cells) >= 3 and cells[2].strip():
        return [cells[2]]
    if len(cells) >= 2:
        return [cells[1]]
    return cells


def main() -> int:
    paths = [DOCX_DIR / name for name in DOCX_NAMES if (DOCX_DIR / name).exists()]
    if not paths:
        print(f"SKIP notes docx coverage: no source docs in {DOCX_DIR}")
        return 0
    notes_text = "\n".join(path.read_text(errors="ignore") for path in NOTES.rglob("*.txt"))
    notes_norm = normalise(notes_text)
    ignored = {
        "notes details of vocab and knowledge",
        "syllabus content",
        "syll ref",
    }
    missing: list[str] = []
    for path in paths:
        doc = Document(path)
        for table in doc.tables:
            for row in table.rows:
                for cell in note_detail_cells(row):
                    for raw in cell.splitlines():
                        line = raw.strip()
                        norm = normalise(line)
                        if len(norm) < 8 or norm in ignored:
                            continue
                        if norm not in notes_norm:
                            missing.append(f"{path.name}: {line}")
    if missing:
        raise AssertionError("\n".join(missing[:80]))
    print(f"OK notes docx coverage: {len(paths)} source docs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

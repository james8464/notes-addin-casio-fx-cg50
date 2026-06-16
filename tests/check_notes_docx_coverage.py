#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

try:
    from docx import Document
    from docx.table import Table
except ModuleNotFoundError:
    print("SKIP notes docx coverage: python-docx unavailable")
    raise SystemExit(0)

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.regroup_notes_by_docx_table import (  # noqa: E402
    DOC_TABLE_NAMES,
    iter_block_items,
    markdown_table,
    slug_safe,
    table_detail_column,
)

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


def table_cells(line: str) -> list[str]:
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    return [normalise(cell) for cell in s.split("|")]


def table_separator(cells: list[str]) -> bool:
    return bool(cells) and all(c and set(c) <= set("-: ") for c in cells)


def note_table_rows(text: str) -> list[list[str]]:
    rows: list[list[str]] = []
    for line in text.splitlines():
        if not line.lstrip().startswith("|"):
            continue
        cells = table_cells(line)
        if cells and not table_separator(cells):
            rows.append(cells)
    return rows


def cell_covers(source: str, candidate: str) -> bool:
    source = normalise(source)
    candidate = normalise(candidate)
    if not source:
        return True
    if source == candidate or source in candidate or candidate in source:
        return True
    source_tokens = set(source.split())
    candidate_tokens = set(candidate.split())
    return bool(source_tokens) and (
        source_tokens <= candidate_tokens or candidate_tokens <= source_tokens
    )


def row_present(source_cells: list[str], candidate_rows: list[list[str]]) -> bool:
    for row in candidate_rows:
        if len(row) < len(source_cells):
            continue
        if all(cell_covers(cell, row[i]) for i, cell in enumerate(source_cells) if normalise(cell)):
            return True
    return False


def nested_tables(cell) -> list[Table]:
    return [block for block in iter_block_items(cell) if isinstance(block, Table)]


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
        expected_names = DOC_TABLE_NAMES.get(path.stem, [])
        if expected_names and len(doc.tables) != len(expected_names):
            missing.append(f"{path.name}: expected {len(expected_names)} source tables, found {len(doc.tables)}")
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
        for table, table_name in zip(doc.tables, expected_names):
            note_path = NOTES / path.stem / f"{slug_safe(table_name)}.txt"
            detail_col = table_detail_column(table)
            rows_with_nested = []
            for row_index, row in enumerate(table.rows[1:], 2):
                if detail_col >= len(row.cells):
                    continue
                blocks = nested_tables(row.cells[detail_col])
                if blocks:
                    rows_with_nested.append((row_index, blocks))
            if not rows_with_nested:
                continue
            if not note_path.exists():
                missing.append(f"{path.name}:{table_name}: note file missing for nested table content")
                continue
            table_rows = note_table_rows(note_path.read_text(errors="ignore"))
            for row_index, nested_blocks in rows_with_nested:
                for nested_index, nested in enumerate(nested_blocks, 1):
                    for raw in markdown_table(nested):
                        cells = table_cells(raw)
                        if not cells or table_separator(cells):
                            continue
                        if not row_present(cells, table_rows):
                            missing.append(
                                f"{path.name}:{table_name}: nested table row missing "
                                f"(source row {row_index}, nested table {nested_index}): {raw}"
                            )
    if missing:
        raise AssertionError("\n".join(missing[:80]))
    print(f"OK notes docx coverage: {len(paths)} source docs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

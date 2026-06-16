#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NOTES = ROOT / "calculator_files" / "NOTES"
FILE_BUF_SIZE = 16384
MAX_TABLE_COLS = 6
MAX_TABLE_ROWS = 16
TABLE_CELL_CAP = 416
READABLE_LINE_CAP = 360
READABLE_TABLE_CELL_SEMICOLON_CAP = 2
READABLE_TABLE_CELL_CAP = 180
READABLE_TABLE_ROW_CAP = 360
RAW_DEFINITION_RE = re.compile(r"^[A-Za-z][A-Za-z0-9 /()+.-]{0,60} = ")
RAW_LINE_ALLOW_PREFIXES = ("ghci>", "True", "False")
WEAK_LABEL_BULLETS = {"- Example:", "- Examples:", "- Note:", "- Accepted wording:", "- Do not only say:", "- then:"}


def table_cells(line: str) -> list[str]:
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    return [c.strip() for c in s.split("|")]


def table_separator(line: str) -> bool:
    cells = table_cells(line)
    return bool(cells) and all(c and "-" in c and set(c) <= set("-: ") for c in cells)


def check_table(path: Path, start: int, block: list[tuple[int, str]]) -> list[str]:
    errors: list[str] = []
    widths = [len(table_cells(line)) for _, line in block]
    if len(block) < 2:
        return [f"{path}:{start}: table block is too short"]
    if len(block) > MAX_TABLE_ROWS:
        errors.append(f"{path}:{start}: table has {len(block)} rows, renderer supports {MAX_TABLE_ROWS}")
    if not table_separator(block[1][1]):
        errors.append(f"{path}:{block[1][0]}: missing markdown separator row")
    if len(set(widths)) != 1:
        errors.append(f"{path}:{start}: inconsistent table columns {widths}")
    if widths[0] > MAX_TABLE_COLS:
        errors.append(f"{path}:{start}: table has {widths[0]} columns, renderer supports {MAX_TABLE_COLS}")
    for line_no, line in block:
        if len(line) > READABLE_TABLE_ROW_CAP:
            errors.append(f"{path}:{line_no}: table row has {len(line)} chars, split it for calculator readability")
        if table_separator(line):
            continue
        for cell in table_cells(line):
            if len(cell) > READABLE_TABLE_CELL_CAP:
                errors.append(f"{path}:{line_no}: table cell has {len(cell)} chars, split it for calculator readability")
            if cell.count(";") > READABLE_TABLE_CELL_SEMICOLON_CAP:
                errors.append(f"{path}:{line_no}: table cell has too many semicolon-separated facts")
            if len(cell) >= TABLE_CELL_CAP:
                errors.append(
                    f"{path}:{line_no}: table cell has {len(cell)} chars, renderer cap is {TABLE_CELL_CAP - 1}"
                )
    return errors


def main() -> int:
    files = sorted(NOTES.rglob("*.txt"))
    if not files:
        raise AssertionError(f"{NOTES}: no .txt files")
    errors: list[str] = []
    for path in files:
        data = path.read_bytes()
        if len(data) >= FILE_BUF_SIZE:
            errors.append(f"{path}: file exceeds viewer buffer")
        if b"\0" in data:
            errors.append(f"{path}: contains NUL byte")
        try:
            text = data.decode("ascii")
        except UnicodeDecodeError as exc:
            errors.append(f"{path}: non-ASCII byte at {exc.start}")
            text = data.decode("utf-8", errors="ignore")
        if "Source:" in text or "Syllabus content" in text or "Notes / details of vocab and knowledge" in text:
            errors.append(f"{path}: contains conversion metadata or syllabus-column heading")
        lines = text.splitlines()
        for line_no, line in enumerate(lines, 1):
            if len(line) > READABLE_LINE_CAP:
                errors.append(f"{path}:{line_no}: line has {len(line)} chars, split it for calculator readability")
            if "\t" in line:
                errors.append(f"{path}:{line_no}: tab characters are unsafe; use spaces or markdown tables")
            stripped = line.strip()
            if stripped.startswith("= "):
                errors.append(f"{path}:{line_no}: raw definition line must use '- Definition:'")
            if line == stripped and stripped.startswith("NOTE:"):
                errors.append(f"{path}:{line_no}: note line must use '- Note:'")
            if line == stripped and RAW_DEFINITION_RE.match(stripped):
                errors.append(f"{path}:{line_no}: raw definition line must be a bullet or code block")
            if (
                stripped
                and line == stripped
                and not stripped.startswith(("#", "-", "|", ">"))
                and not stripped[0].isdigit()
                and not stripped.startswith(RAW_LINE_ALLOW_PREFIXES)
            ):
                errors.append(f"{path}:{line_no}: plain note line must be a heading, bullet, table, quote, or code block")
            if stripped in WEAK_LABEL_BULLETS:
                errors.append(f"{path}:{line_no}: label bullet should be a heading or specific sentence")
            if stripped.lower() in {"## below...", "### below..."}:
                errors.append(f"{path}:{line_no}: placeholder heading must be replaced")
        first = next((line for line in lines if line.strip()), "")
        if not first.startswith("# "):
            errors.append(f"{path}: first content line must be a level-1 heading")
        elif first[2:].strip() != path.stem:
            errors.append(f"{path}: first heading {first[2:].strip()!r} must match filename {path.stem!r}")
        i = 0
        while i < len(lines):
            if not lines[i].lstrip().startswith("|"):
                i += 1
                continue
            start = i + 1
            block: list[tuple[int, str]] = []
            while i < len(lines) and lines[i].lstrip().startswith("|"):
                block.append((i + 1, lines[i]))
                i += 1
            errors.extend(check_table(path, start, block))
    if errors:
        raise AssertionError("\n".join(errors[:80]))
    print(f"OK notes integrity: {len(files)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

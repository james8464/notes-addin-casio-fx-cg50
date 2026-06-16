#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NOTES = ROOT / "calculator_files" / "NOTES"
APP = ROOT / "tools" / "notes_app.cc"


def const_int(source: str, name: str) -> int:
    m = re.search(rf"static const int {name} = ([^;]+);", source)
    if not m:
        raise AssertionError(f"missing renderer constant {name}")
    expr = m.group(1).strip()
    values = {
        "MAX_VIEW_LINES": globals().get("MAX_VIEW_LINES"),
        "LINE_CAP": globals().get("LINE_CAP"),
        "MAX_TABLE_COLS": globals().get("MAX_TABLE_COLS"),
        "MAX_TABLE_ROWS": globals().get("MAX_TABLE_ROWS"),
        "TABLE_CELL_CAP": globals().get("TABLE_CELL_CAP"),
        "VIEW_TOP": globals().get("VIEW_TOP"),
        "VIEW_ROW_H": globals().get("VIEW_ROW_H"),
        "PAGE_LINES": globals().get("PAGE_LINES"),
    }
    if expr.isdigit():
        return int(expr)
    for key, value in values.items():
        if value is not None:
            expr = re.sub(rf"\b{key}\b", str(value), expr)
    if not re.fullmatch(r"[0-9+\-*/ ()]+", expr):
        raise AssertionError(f"unsupported constant expression for {name}: {m.group(1)}")
    return int(eval(expr, {"__builtins__": {}}, {}))


APP_SOURCE = APP.read_text()
MAX_VIEW_LINES = const_int(APP_SOURCE, "MAX_VIEW_LINES")
LINE_CAP = const_int(APP_SOURCE, "LINE_CAP")
MAX_TABLE_COLS = const_int(APP_SOURCE, "MAX_TABLE_COLS")
MAX_TABLE_ROWS = const_int(APP_SOURCE, "MAX_TABLE_ROWS")
TABLE_CELL_CAP = const_int(APP_SOURCE, "TABLE_CELL_CAP")
TABLE_MAX_CHARS = const_int(APP_SOURCE, "TABLE_MAX_CHARS")
VIEW_TOP = const_int(APP_SOURCE, "VIEW_TOP")
VIEW_ROW_H = const_int(APP_SOURCE, "VIEW_ROW_H")
PAGE_LINES = const_int(APP_SOURCE, "PAGE_LINES")


def table_like(line: str) -> bool:
    s = line.lstrip(" ")
    if "\t" in line:
        return True
    if s.startswith("|"):
        return line.count("|") >= 2
    if s.startswith("+"):
        return line.count("+") >= 2 and line.count("-") >= 3
    return False


def clean_cell(text: str) -> str:
    text = text.strip().replace("\t", " ")
    text = re.sub(r"<br>", "; ", text, flags=re.I)
    text = "".join(ch for ch in text if 32 <= ord(ch) <= 126)
    return text.strip()


def clean_inline(text: str) -> str:
    text = text.replace("\t", " ")
    text = re.sub(r"<br>", "; ", text, flags=re.I)
    return "".join(ch for ch in text if 32 <= ord(ch) <= 126)


def table_cells(line: str) -> list[str]:
    s = line.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|"):
        s = s[:-1]
    cells = [clean_cell(c) for c in re.split(r"[|\t]", s)]
    while cells and not cells[-1]:
        cells.pop()
    return cells


def separator(cells: list[str]) -> bool:
    return bool(cells) and all(c and "-" in c and set(c) <= set("-: ") for c in cells)


def table_col_cap(cols: int) -> int:
    if cols <= 2:
        return 30
    if cols == 3:
        return 22
    if cols == 4:
        return 16
    return 12


def table_total_chars(widths: list[int]) -> int:
    return 1 + sum(w + 3 for w in widths)


def table_widths(rows: list[list[str]], cols: int) -> list[int]:
    cap = table_col_cap(cols)
    widths = [4 if cols >= 5 else 5 for _ in range(cols)]
    for row in rows:
        for c in range(cols):
            n = min(len(row[c]) if c < len(row) else 0, cap)
            widths[c] = max(widths[c], n)
    if cols <= 3:
        minw = 4 if cols >= 5 else 5
        while table_total_chars(widths) > TABLE_MAX_CHARS:
            widest = max((i for i, w in enumerate(widths) if w > minw), key=lambda i: widths[i], default=-1)
            if widest < 0:
                break
            widths[widest] -= 1
    return widths


def table_segments(cell: str, width: int) -> list[str]:
    if not cell:
        return [""]
    out: list[str] = []
    pos = 0
    while pos < len(cell):
        while pos < len(cell) and cell[pos] == " ":
            pos += 1
        if pos >= len(cell):
            break
        cut = min(len(cell), pos + width)
        last_space = cell.rfind(" ", pos, cut)
        if cut < len(cell) and last_space > pos:
            cut = last_space
        if cut <= pos:
            cut = min(len(cell), pos + width)
        out.append(clean_cell(cell[pos:cut]))
        pos = cut
    return out or [""]


def norm(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def markdown_skip_style(line: str) -> tuple[int, str]:
    p = len(line) - len(line.lstrip(" "))
    s = line[p:]
    if s.startswith("#"):
        hashes = len(s) - len(s.lstrip("#"))
        if hashes < len(s) and s[hashes:hashes + 1] == " ":
            return p + hashes + 1, "heading"
    if len(s) > 2 and s[0] in "-*" and s[1] == " ":
        return 0, "bullet"
    if re.match(r"\d{1,3}[.)] ", s):
        return 0, "ordered"
    if s.startswith("> "):
        return 0, "quote"
    return 0, "text"


def wrapped_non_table_count(line: str) -> int:
    return len(wrapped_non_table_segments(line))


def display_source(line: str) -> str:
    skip, style = markdown_skip_style(line)
    if style == "bullet":
        return line
    return line[skip:]


def wrapped_non_table_segments(line: str) -> list[str]:
    if not line:
        return [""]
    text = display_source(line)
    # Conservative lower width than the app's pixel wrapping, so this overestimates.
    width = 16
    out = [clean_inline(text[i:i + width]) for i in range(0, max(1, len(text)), width)]
    return out or [""]


def audit_file(path: Path) -> list[str]:
    errors: list[str] = []
    src_lines = path.read_text().splitlines()
    rendered = 0
    i = 0
    while i < len(src_lines):
        line = src_lines[i]
        if table_like(line):
            start = i + 1
            block: list[tuple[int, list[str]]] = []
            while i < len(src_lines) and table_like(src_lines[i]):
                cells = table_cells(src_lines[i])
                if cells and not separator(cells):
                    block.append((i + 1, cells))
                i += 1
            if len(block) > MAX_TABLE_ROWS:
                errors.append(f"{path}:{start}: table exceeds row cap")
                continue
            cols = max((len(cells) for _line, cells in block), default=0)
            if cols > MAX_TABLE_COLS:
                errors.append(f"{path}:{start}: table exceeds column cap")
                continue
            rows = [cells + [""] * (cols - len(cells)) for _line, cells in block]
            widths = table_widths(rows, cols)
            for line_no, cells in block:
                row_parts = 1
                row_segments: list[list[str]] = []
                for c, cell in enumerate(cells + [""] * (cols - len(cells))):
                    if len(cell) >= TABLE_CELL_CAP:
                        errors.append(f"{path}:{line_no}: cell exceeds renderer cap")
                    segs = table_segments(cell, widths[c])
                    row_segments.append(segs)
                    row_parts = max(row_parts, len(segs))
                    if norm(" ".join(segs)) != norm(cell):
                        errors.append(f"{path}:{line_no}: table cell segmentation loses text")
                for part in range(row_parts):
                    line_len = sum(len(segs[part]) if part < len(segs) else 0 for segs in row_segments)
                    line_len += max(0, cols - 1)
                    if line_len >= LINE_CAP:
                        errors.append(f"{path}:{line_no}: rendered table row exceeds line_store cap")
                    rendered += 1
            if i < len(src_lines) and src_lines[i].strip():
                rendered += 1
            continue
        segs = wrapped_non_table_segments(line)
        if norm("".join(segs)) != norm(display_source(line)):
            errors.append(f"{path}:{i + 1}: wrapped note line reconstruction loses text")
        for seg in segs:
            if len(seg) >= LINE_CAP:
                errors.append(f"{path}:{i + 1}: wrapped note line exceeds line_store cap")
        rendered += wrapped_non_table_count(line)
        i += 1
    if rendered >= MAX_VIEW_LINES:
        errors.append(f"{path}: estimated rendered lines {rendered} exceed cap {MAX_VIEW_LINES}")
    return errors


def main() -> int:
    errors: list[str] = []
    if "min_int(src_len, LINE_CAP - 3)" in APP_SOURCE:
        errors.append("top-level bullet rendering still has LINE_CAP pre-truncation")
    if "    char cells[MAX_TABLE_COLS][TABLE_CELL_CAP];" in APP_SOURCE:
        errors.append("table parse scratch cells must not be allocated on the fx-CG stack")
    if "      char segs[MAX_TABLE_COLS][TABLE_CELL_CAP];" in APP_SOURCE:
        errors.append("table segment scratch cells must not be allocated on the fx-CG stack")
    softkey_y = 58 * 3
    last_table_bottom = VIEW_TOP + (PAGE_LINES - 1) * VIEW_ROW_H + VIEW_ROW_H - 2
    if last_table_bottom >= softkey_y:
        errors.append("note page lines can overlap the softkey row")
    if "while (n > 0 && mini_width(s, n) > avail) --n;" not in APP_SOURCE:
        errors.append("table/limited text must shrink before PrintMini clipping")
    if "if (table_like(s + p, len - p))" in APP_SOURCE:
        errors.append("markdown fallback must not mark table-like lines as NOTE_TABLE without table metadata")
    if "int scroll = len > visible ? len - 1 : 0;" in APP_SOURCE:
        errors.append("wide-line horizontal scroll must be based on displayed text, not raw source text")
    if "int scroll = len - visible;" in APP_SOURCE:
        errors.append("wide-line horizontal scroll must not stop before the final source character")
    if "int start = min_int(hscroll, max_int(0, len - 1));" not in APP_SOURCE:
        errors.append("wide-line rendering must clamp to the final source character, not stop early")
    if "fit_visible_chars(s + start, len - start, 0, xpad)" not in APP_SOURCE:
        errors.append("wide-line rendering must pixel-fit the visible suffix after horizontal scroll")
    if "fit_suffix_chars" not in APP_SOURCE or "line_end_hscroll(src, src_len, style)" not in APP_SOURCE:
        errors.append("wide-line horizontal scroll must use the pixel-fitted displayed suffix")
    if "style_x_offset(style)" not in APP_SOURCE or "NOTE_X_LIMIT - VIEW_X - xpad" not in APP_SOURCE:
        errors.append("wide-line fitting must account for heading/quote x offsets")
    if "int visible = fit_visible_chars(file_buf + pos, len, 0);" in APP_SOURCE:
        errors.append("wide-line scroll must not use raw source length for markdown-formatted lines")
    if "int cap_cut = pos + max_int(1, LINE_CAP - indent - 1);" not in APP_SOURCE:
        errors.append("wrapped lines must cap copied text before line_store writes")
    if "return over > 0 ? (over + TABLE_CHAR_PX - 1) / TABLE_CHAR_PX : 0;" not in APP_SOURCE:
        errors.append("table horizontal scroll must round up to reveal the final table edge")
    if "note_fill_clip" in APP_SOURCE[APP_SOURCE.find("static void notes_print_with_matches_limit"):APP_SOURCE.find("static void notes_print_with_matches(")]:
        errors.append("search matches must use text colour only, not fill/background highlights")
    if 'if (!jump_to_match(&sp, start_source, 1, &top, &hscroll, &lines, max_line)) {\n          search_prepare(&sp, "");\n          active_search = 0;' not in APP_SOURCE:
        errors.append("failed in-file search must reset search mode so NEXT/PREV keep paging")
    if "NOTE_H1 : (hashes == 2 ? NOTE_H2 : (hashes == 3 ? NOTE_H3 : NOTE_H4))" not in APP_SOURCE:
        errors.append("markdown headings must map H1/H2/H3/H4 to distinct renderer styles")
    for marker in ("if (style == NOTE_H1)", "else if (style == NOTE_H2)", "else if (style == NOTE_H3)", "else if (style == NOTE_H4)"):
        if marker not in APP_SOURCE:
            errors.append(f"renderer missing distinct heading branch: {marker}")
    for indent in ("x += 4;", "x += 8;", "x += 12;"):
        if indent not in APP_SOURCE:
            errors.append(f"subheading renderer missing indentation: {indent}")
    for path in sorted(NOTES.rglob("*.txt")):
        errors.extend(audit_file(path))
    if errors:
        raise AssertionError("\n".join(errors[:80]))
    print("OK notes render layout")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

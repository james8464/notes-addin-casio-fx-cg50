#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, needles: list[str]) -> None:
    text = (ROOT / path).read_text(errors="ignore")
    missing = [n for n in needles if n not in text]
    if missing:
        raise AssertionError(f"{path}: missing {missing}")


def main() -> int:
    require(
        "tools/casio_suite_ui.hpp",
        ["ui_status()", "ui_menu_keys", "ui_softkeys", "ui_print_mini_grid(0, 58, menu, 4)", "ui_khicas_fkeys", "| view | cmds | A<>a | File ", "ui_console_input", "ui_console_wait_page", "ui_mprintxy", "TScrollbar", "PREV", "NEXT"],
    )
    require("tools/khicas_suite_bridge.cpp", ["suite_eval_with_working", "Console_Output", "Console_NewLine", "suite_console_fmenu_config", "suite_register_lexer_symbols", "lexer_functions_register", "p3_eval", "cscalc_eval", "SUITE_APP_P3", "SUITE_APP_CS"])
    require("tools/khicas_suite_catalog.py", ["suvat(", "projectile(", "hypbinom(", "normalprob(", "Mechanics", "Statistics", "Data/prob", "Command help", "doTextArea", "KEY_CTRL_F6"])
    require("tools/p3_engine.cpp", ["suvat(", "projectile(", "hypbinom(", "binomcdf(", "incline(", "pulley(", "poisson("])
    require("tools/patch_khicas_suite_main.py", ["suite_eval_with_working((const char *)expr)", "suite_register_lexer_symbols();", "suite_console_fmenu_config()", "menu_f1,menu_f2,menu_f3,menu_f4,menu_f5,menu_f6", "cascas::eval_with_working", "Err: unsupported"])
    require("tools/khicas_suite_catalog.py", ["Computer Science", "bool_simplify(expression)", "nandform(expression)", "norform(expression)", "boolprove(lhs,rhs)"])
    require("tools/cscalc_engine.cpp", ["eval_bool_simplify_old_style", "OldBoolEngine", "nandform(", "norform(", "boolprove("])
    require("tools/notes_app.cc", ["Bfile_FindFirst_NON_SMEM", ".txt", "load_file_buf", "Find all text", "Find text", "notes_input", "search_all_rec", "file_text_matches", "SearchPattern", "search_prepare", "search_step", "search_results_menu", "ui_menu_keys", "hidden_system_folder", ".Trashes", ".fseventsd", "SAVE-F", "NOTES_ROOT", r"\\\\fls0\\NOTES\\"])
    notes_src = (ROOT / "tools/notes_app.cc").read_text(errors="ignore")
    for forbidden in ["ui_input(", "ui_wait_page(", "ui_chrome(", "ui_status(", "ui_page(", "ui_menu(", "ui_khicas_fkeys(", "SAF_SETUP_"]:
        if forbidden in notes_src:
            raise AssertionError(f"notes app must not use shared status chrome: {forbidden}")
    if notes_src.count("DefineStatusAreaFlags") != 1 or notes_src.count("DisplayStatusArea") != 1:
        raise AssertionError("notes status bar must be owned only by notes_chrome")
    for required in ["DisplayMBString2", "EditMBStringCtrl2", "EditMBStringChar", "Cursor_SetFlashOff"]:
        if required not in notes_src:
            raise AssertionError(f"notes search input must keep KhiCAS edit cursor path: {required}")
    if "y - 1, w, STYLE_BAND_H, UI_HILITE" in notes_src:
        raise AssertionError("notes search highlight must not paint over glyphs")
    if "NOTE_CHAR_PX, LCD_WIDTH_PX - hx" in notes_src or "y + STYLE_BAND_H - 2" in notes_src:
        raise AssertionError("notes search highlight must not draw under/over matched glyphs")
    if "UI_HILITE" in notes_src or "ui_fill(0, y - 1, LCD_WIDTH_PX, STYLE_BAND_H, UI_HILITE)" in notes_src:
        raise AssertionError("notes search result must use coloured matched text, not a highlight band")
    for required in ["UI_MATCH_TEXT", "match_at_ci", "notes_print_with_matches", "notes_print_span"]:
        if required not in notes_src:
            raise AssertionError(f"notes search result must colour only matching text: {required}")
    if "TEXT%d" in notes_src or "TEXT<number>" in (ROOT / "docs/NOTES_README.md").read_text(errors="ignore"):
        raise AssertionError("notes search results must show file names, not old TEXT/NAME prefixes")
    if "static int find_line(" in notes_src:
        raise AssertionError("notes file-view search must use source-line matching, not stale wrapped-line search")
    if "min_int(src_len, LINE_CAP - 3)" in notes_src:
        raise AssertionError("notes top-level bullet lines must wrap from source, not a capped temporary buffer")
    if "    char cells[MAX_TABLE_COLS][TABLE_CELL_CAP];" in notes_src or "      char segs[MAX_TABLE_COLS][TABLE_CELL_CAP];" in notes_src:
        raise AssertionError("notes table scratch buffers must stay static to avoid fx-CG stack corruption")
    if "static const unsigned NOTE_X_LIMIT = LCD_WIDTH_PX - 18;" not in notes_src:
        raise AssertionError("notes text must leave a safe right margin on physical fx-CG screens")
    for required in ["NOTE_H3", "NOTE_H4", "hashes <= 1", "hashes == 2", "hashes == 3"]:
        if required not in notes_src:
            raise AssertionError(f"notes renderer must support nested markdown headings: {required}")
    for required in ["NOTE_ORDERED", "NOTE_QUOTE", "NOTE_RULE", "ordered_list_marker", "rule_like"]:
        if required not in notes_src:
            raise AssertionError(f"notes renderer must support basic markdown reading styles: {required}")
    for required in ["fence_like", "html_break_len", "(line[p] == '-' || line[p] == '*' || line[p] == '+')"]:
        if required not in notes_src:
            raise AssertionError(f"notes renderer must support generic markdown text features: {required}")
    if "static int starts_with_ci" in notes_src or "static int code_like" in notes_src:
        raise AssertionError("notes renderer must keep one simple source_code_like path")
    if "*skip = p;\n    *style = NOTE_ORDERED" in notes_src or "*skip = p;\n    *style = NOTE_QUOTE" in notes_src:
        raise AssertionError("nested ordered lists and block quotes must preserve source indentation")
    for required in ["style == NOTE_ORDERED && pos != base_indent", "style == NOTE_QUOTE && pos != base_indent"]:
        if required not in notes_src:
            raise AssertionError(f"notes wrapped continuations must align nested markdown content: {required}")
    if "push_table_block(&line, rows, row_count, cols, hscroll)" not in notes_src:
        raise AssertionError("notes table horizontal scroll must use the table renderer")
    for required in ["notes_print_with_matches_limit", "notes_print_span_limit", "xlimit", "push_blank_line(&line, next_source)", "table_max_hscroll(widths, cols)", "local_hscroll"]:
        if required not in notes_src:
            raise AssertionError(f"notes table cells must clip text and reserve spacing: {required}")
    if "return tab || bars >= 2" in notes_src:
        raise AssertionError("notes table detection must not treat prose containing || as a table")
    if "if (tab) return 1;" in notes_src:
        raise AssertionError("notes table detection must not treat every tab-indented line as a table")
    if "tab_separated_cells" not in notes_src:
        raise AssertionError("notes table detection must require real tab-separated cells")
    if "if (bars)" in notes_src:
        raise AssertionError("notes pipe table detection must require a separator row")
    if "static int table_start_like_at" not in notes_src or "table_separator_row(table_parse_cells, cols)" not in notes_src:
        raise AssertionError("notes renderer must support no-leading-pipe markdown tables only with a separator row")
    if "setext_underline_style" not in notes_src or "setext_heading_style_at" not in notes_src:
        raise AssertionError("notes renderer must support markdown setext headings")
    if "source_code_like" not in notes_src:
        raise AssertionError("notes renderer must detect code-like rows before table detection")
    if "if (c == '\\r')" not in notes_src or "clean[out++] = '\\n';" not in notes_src:
        raise AssertionError("notes viewer must normalize CR/CRLF input to LF")
    if "source_code_like(file_buf + next_pos, next_end - next_pos)" not in notes_src:
        raise AssertionError("notes long tables must not absorb following code-like rows")
    if "lower_char((unsigned char)out[len - 3]) == 't'" not in notes_src:
        raise AssertionError("notes labels must strip .txt extension case-insensitively")
    if "static const int MAX_TABLE_COLS = 8;" not in notes_src:
        raise AssertionError("notes table renderer must support wider markdown tables")
    if "cols == MAX_TABLE_COLS - 1" not in notes_src:
        raise AssertionError("notes table overflow columns must be preserved in final column")
    if "*hscroll = (style == NOTE_TABLE" in notes_src:
        raise AssertionError("notes search jumps must not horizontally offset wrapped table rows")
    if "hscroll > 0 || style == NOTE_CODE" not in notes_src:
        raise AssertionError("notes horizontal scroll must work for non-table wide lines")
    for required in ["mini_width", "segment_fits_screen", "fit_visible_chars", "max_file_line_scroll", "NOTE_X_LIMIT"]:
        if required not in notes_src:
            raise AssertionError(f"notes wrapping must use PrintMini pixel preview: {required}")
    if "int scroll = len - visible;" in notes_src:
        raise AssertionError("notes horizontal scroll must not cap before the final source character")
    for required in ["view_source_line", "source_line_bounds", "find_source_match", "view_line_for_source_match", "jump_to_match(&sp"]:
        if required not in notes_src:
            raise AssertionError(f"notes search jump must track source lines: {required}")
    if "STYLE_BAND_H" in notes_src or "NOTE_CHAR_PX" in notes_src or "UI_MATCH_ON_BLUE" in notes_src:
        raise AssertionError("notes renderer must not keep stale highlight/character-width constants")
    if "ui_print_mini_px(360, 27" in notes_src or "ui_print_mini_px(372, 27" in notes_src:
        raise AssertionError("notes horizontal-scroll markers must not overlap the first text row")
    if "hscroll = min_int(max_line, hscroll + 8);" not in notes_src:
        raise AssertionError("notes horizontal scroll must advance to the final reachable offset")
    if "static const int VIEW_ROW_H = 17;" not in notes_src or "static const int PAGE_LINES = 8;" not in notes_src:
        raise AssertionError("notes text rows must use safe PrintMini spacing")
    if "static const int MAX_VIEW_LINES = 768;" not in notes_src:
        raise AssertionError("notes viewer must keep the larger 768-line display cache")
    if 'contains_text_len(s + p, len - p, "::")' in notes_src or 'contains_text_len(s + p, len - p, "->")' in notes_src:
        raise AssertionError("notes prose containing :: or -> must not be forced to grey code style")
    if "commas >= 3" in notes_src or "gaps >= 2" in notes_src:
        raise AssertionError("notes prose with commas/spaces must not be forced to table style")
    require("tools/build_g3a.sh", ["CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "build_suite_app CASP3", "build_suite_app CSCALC", "khicas_suite_catalog.py", "patch_khicas_suite_main.py"])
    require("docs/aqa_cs_calc_scope.md", ["AQA 7517", "floating-point", "two's complement"])
    require("docs/CAS_README.md", ["diff(expression)", "integrate(expression,x)", "xform(start_expression,target_expression)"])
    require("docs/CASP3_README.md", ["suvat(known=value", "projectile(u,angle)", "hypbinom(n,p,x,alpha,tail)", "samplemean(mu,sigma,n,lo,hi)", "normalcrit(p,mu,sigma)"])
    require("docs/CSCALC_README.md", ["Same console shell, function keys, and mini menus as KhiCAS", "Computer Science", "bool_simplify(expression)", "nandform(expression)", "boolprove(lhs,rhs)"])
    require("docs/NOTES_README.md", ["Knuth-Morris-Pratt", "O(total path characters + total text bytes + m)", "does not open images", "fx-CG add-in text rendering inspiration", "wide-line horizontal scroll is capped"])
    notes_dir = ROOT / "calculator_files" / "NOTES"
    if notes_dir.exists():
        note_files = list(notes_dir.rglob("*.txt"))
        if not note_files:
            raise AssertionError(f"{notes_dir}: no .txt files found")
        too_big = [p for p in note_files if p.stat().st_size >= 16384]
        if too_big:
            raise AssertionError(f"notes files exceed viewer buffer: {[str(p) for p in too_big[:5]]}")
        nul_files = [p for p in note_files if b"\0" in p.read_bytes()]
        if nul_files:
            raise AssertionError(f"notes files contain NUL bytes: {[str(p) for p in nul_files[:5]]}")
    for name in ["CAS.g3a", "CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "RUNMAT.g3a", "CAS.PAK"]:
        path = ROOT / "calculator_files" / name
        if not path.exists() or path.stat().st_size <= 0:
            raise AssertionError(f"missing built calculator file: {path}")
    print("OK multi app suite source checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

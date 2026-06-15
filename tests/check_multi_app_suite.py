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
    require("tools/khicas_suite_catalog.py", ["convert(", "twosdec(", "floatdec(", "image(", "binarysearch(", "Number", "Float/fixed", "Boolean"])
    require("tools/cscalc_engine.cpp", ["twosdec(", "twossub(", "floatdec(", "floatenc(", "bool(", "nand", "image("])
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
    if "static const int WRAP_COLS = 34;" not in notes_src:
        raise AssertionError("notes text width should use the wider 34-column viewport")
    if "hscroll > 0 || style == NOTE_TABLE || style == NOTE_CODE" not in notes_src:
        raise AssertionError("notes horizontal scroll must work beyond table-only lines")
    if "max_file_line_len" not in notes_src or "if (hscroll + 8 < max_line) hscroll += 8;" not in notes_src:
        raise AssertionError("notes horizontal scroll must be capped by the longest source line")
    if 'contains_text_len(s + p, len - p, "::")' in notes_src or 'contains_text_len(s + p, len - p, "->")' in notes_src:
        raise AssertionError("notes prose containing :: or -> must not be forced to grey code style")
    if "commas >= 3" in notes_src or "gaps >= 2" in notes_src:
        raise AssertionError("notes prose with commas/spaces must not be forced to table style")
    require("tools/build_g3a.sh", ["CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "build_suite_app CASP3", "build_suite_app CSCALC", "khicas_suite_catalog.py", "patch_khicas_suite_main.py"])
    require("docs/aqa_cs_calc_scope.md", ["AQA 7517", "floating-point", "two's complement"])
    require("docs/CAS_README.md", ["diff(expression)", "integrate(expression,x)", "xform(start_expression,target_expression)"])
    require("docs/CASP3_README.md", ["suvat(known=value", "projectile(u,angle)", "hypbinom(n,p,x,alpha,tail)", "samplemean(mu,sigma,n,lo,hi)", "normalcrit(p,mu,sigma)"])
    require("docs/CSCALC_README.md", ["convert(value,from_base,to_base)", "floatdec(mantissa,exponent)", "binarysearch(target,list...)"])
    require("docs/NOTES_README.md", ["Knuth-Morris-Pratt", "O(total path characters + total text bytes + m)", "does not open images", "fx-CG add-in text rendering inspiration", "wide-line horizontal scroll is capped"])
    for name in ["CAS.g3a", "CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "RUNMAT.g3a", "CAS.PAK"]:
        path = ROOT / "calculator_files" / name
        if not path.exists() or path.stat().st_size <= 0:
            raise AssertionError(f"missing built calculator file: {path}")
    print("OK multi app suite source checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

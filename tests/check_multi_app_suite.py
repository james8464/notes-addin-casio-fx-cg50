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
    require("tools/khicas_suite_bridge.cpp", ["suite_eval_with_working", "Console_Output", "Console_NewLine", "p3_eval", "cscalc_eval", "SUITE_APP_P3", "SUITE_APP_CS"])
    require("tools/khicas_suite_catalog.py", ["suvat(", "projectile(", "hypbinom(", "normalprob(", "Mechanics", "Statistics", "Data/prob"])
    require("tools/p3_engine.cpp", ["suvat(", "projectile(", "hypbinom(", "binomcdf(", "incline(", "pulley(", "poisson("])
    require("tools/patch_khicas_suite_main.py", ["suite_eval_with_working((const char *)expr)", "cascas::eval_with_working", "Err: unsupported"])
    require("tools/khicas_suite_catalog.py", ["convert(", "twosdec(", "floatdec(", "image(", "binarysearch(", "Number", "Float/fixed", "Boolean"])
    require("tools/cscalc_engine.cpp", ["twosdec(", "twossub(", "floatdec(", "floatenc(", "bool(", "nand", "image("])
    require("tools/notes_app.cc", ["Bfile_FindFirst_NON_SMEM", ".txt", "load_file_buf", "Find all text", "Find text", "search_all_rec", "file_text_matches", "SearchPattern", "search_prepare", "search_step", "search_results_menu", "ui_menu_keys", "hidden_system_folder", ".Trashes", ".fseventsd", "SAVE-F", "NOTES_ROOT", r"\\\\fls0\\NOTES\\"])
    require("tools/build_g3a.sh", ["CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "build_suite_app CASP3", "build_suite_app CSCALC", "khicas_suite_catalog.py", "patch_khicas_suite_main.py"])
    require("docs/aqa_cs_calc_scope.md", ["AQA 7517", "floating-point", "two's complement"])
    require("docs/CAS_README.md", ["diff(expression)", "integrate(expression,x)", "xform(start_expression,target_expression)"])
    require("docs/CASP3_README.md", ["suvat(known=value", "projectile(u,angle)", "hypbinom(n,p,x,alpha,tail)", "samplemean(mu,sigma,n,lo,hi)", "normalcrit(p,mu,sigma)"])
    require("docs/CSCALC_README.md", ["convert(value,from_base,to_base)", "floatdec(mantissa,exponent)", "binarysearch(target,list...)"])
    require("docs/NOTES_README.md", ["Knuth-Morris-Pratt", "O(total path characters + total text bytes + m)", "does not open images"])
    for name in ["CAS.g3a", "CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "RUNMAT.g3a", "CAS.PAK"]:
        path = ROOT / "calculator_files" / name
        if not path.exists() or path.stat().st_size <= 0:
            raise AssertionError(f"missing built calculator file: {path}")
    print("OK multi app suite source checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

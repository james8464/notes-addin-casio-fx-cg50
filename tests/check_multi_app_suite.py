#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, needles: list[str]) -> None:
    text = (ROOT / path).read_text(errors="ignore")
    missing = [n for n in needles if n not in text]
    if missing:
        raise AssertionError(f"{path}: missing {missing}")


def forbid(path: str, needles: list[str]) -> None:
    text = (ROOT / path).read_text(errors="ignore")
    found = [n for n in needles if n in text]
    if found:
        raise AssertionError(f"{path}: stale content {found}")


def main() -> int:
    require(
        "tools/build_g3a.sh",
        ["Usage: ./compile", "casp3", "notes", "runmat", "khicas"],
    )
    require(
        "compile",
        ['"${ROOT_DIR}/tools/build_g3a.sh" "$@"'],
    )
    require(
        "tools/khicas_suite_bridge.cpp",
        ["suite_eval_with_working", "p3_eval", "SUITE_APP_P3"],
    )
    require(
        "tools/khicas_suite_catalog.py",
        ["suvat(", "projectile(", "hypbinom(", "normalprob(", "Mechanics", "Statistics"],
    )
    require(
        "tools/p3_engine.cpp",
        ["suvat(", "projectile(", "hypbinom(", "binomcdf(", "incline(", "pulley(", "poisson("],
    )
    require(
        "tools/notes_app.cc",
        ["NOTES_ROOT", r"\\\\fls0\\NOTES\\", "Bfile_FindFirst_NON_SMEM", ".txt", "KEY_CTRL_LEFT", "KEY_CTRL_RIGHT", "search_all_rec"],
    )
    require(
        "tools/generate_runmat_icons.py",
        ["runmat_icon.png", '"casp3":', '"notes":', 'f"{name}_icon.png"'],
    )
    require(
        "docs/CAS_README.md",
        ["diff(expression)", "integrate(expression,x)", "xform(start_expression,target_expression)"],
    )
    require(
        "docs/CASP3_README.md",
        ["suvat(known=value", "projectile(u,angle)", "hypbinom(n,p,x,alpha,tail)"],
    )
    require(
        "docs/NOTES_README.md",
        ["does not open images", "search all text files", "wide-line horizontal scroll"],
    )

    for rel in [
        "tools/khicas_suite_bridge.cpp",
        "tools/khicas_suite_catalog.py",
        "tools/generate_runmat_icons.py",
        "README.md",
    ]:
        forbid(rel, ["CSCALC", "cscalc", "SUITE_APP_CS", "bool_simplify", "booleanProgram"])

    forbidden_files = [
        "tools/cscalc_engine.cpp",
        "tools/cscalc_engine.hpp",
        "tools/cscalc_engine_host.cpp",
        "tests/check_cscalc_engine.py",
        "docs/CSCALC_README.md",
        "docs/aqa_cs_calc_scope.md",
        "calculator_files/boolean.py",
        "calculator_files/booleanProgram.mpy",
    ]
    existing = [p for p in forbidden_files if (ROOT / p).exists()]
    if existing:
        raise AssertionError(f"CS/boolean files should be removed: {existing}")

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

    print("OK multi app suite source checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

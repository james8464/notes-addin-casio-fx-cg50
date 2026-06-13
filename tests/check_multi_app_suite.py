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
    require("tools/p3_app.cc", ["Command input", "command_templates", "p3_eval", "SUVAT", "Projectiles", "Hypothesis tests", "Normal dist", "Regression"])
    require("tools/p3_engine.cpp", ["suvat(", "projectile(", "hypbinom(", "binomcdf(", "incline(", "pulley(", "poisson("])
    require("tools/cscalc_app.cc", ["Command input", "command_templates", "cscalc_eval", "Two's complement", "Floating decode", "Image storage", "Compression"])
    require("tools/cscalc_engine.cpp", ["twosdec(", "twossub(", "floatdec(", "floatenc(", "bool(", "nand", "image("])
    require("tools/notes_app.cc", ["Bfile_FindFirst_NON_SMEM", ".txt", ".bmp", "load_text"])
    require("tools/build_g3a.sh", ["CASP3.g3a", "CSCALC.g3a", "NOTES.g3a"])
    require("docs/aqa_cs_calc_scope.md", ["AQA 7517", "floating-point", "two's complement"])
    for name in ["CAS.g3a", "CASP3.g3a", "CSCALC.g3a", "NOTES.g3a", "RUNMAT.g3a", "CAS.PAK"]:
        path = ROOT / "calculator_files" / name
        if not path.exists() or path.stat().st_size <= 0:
            raise AssertionError(f"missing built calculator file: {path}")
    print("OK multi app suite source checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

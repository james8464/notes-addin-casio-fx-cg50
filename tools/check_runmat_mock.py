#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "khicas/upstream/giac90_1addin"


def require(text: str, marker: str, label: str) -> None:
    if marker not in text:
        raise SystemExit(f"FAIL runmat {label}: missing {marker!r}")


def main() -> int:
    source = (ROOT / "tools/runmat_mock.cc").read_text(errors="ignore")
    makefile = (SRC / "Makefile").read_text(errors="ignore")
    build = (ROOT / "tools/build_g3a.sh").read_text(errors="ignore")

    for label in ("Math", "Deg", "Norm1", "d/c", "Real", "JUMP", "DELETE", "MAT/VCT", "MATH"):
        require(source, f'"{label}"', label)
    require(source, "static const unsigned short kPink = 0xf81f;", "pink")
    require(source, "fill_rect(0, 0, 7, LCD_HEIGHT_PX, kPink);", "left border")
    require(source, "fill_rect(LCD_WIDTH_PX - 7, 0, 7, LCD_HEIGHT_PX, kPink);", "right border")
    require(source, "fill_rect(0, LCD_HEIGHT_PX - 7, LCD_WIDTH_PX, 7, kPink);", "bottom border")
    require(source, "draw_r_indicator", "R indicator")
    require(source, "kRBlinkPeriodTicks = 12", "R 3s period")
    require(source, "kRVisibleTicks = 8", "R 2s visible window")
    require(source, "fill_rect(339, 1, 21, 22, visible ? kBlue : kWhite);", "R blue highlight")
    require(source, 'PrintXY(343, 4, (char *)"R"', "R glyph")
    require(source, "r_ticks = (r_ticks + 1) % kRBlinkPeriodTicks;", "R blink loop")
    require(source, "draw_r_indicator(r_ticks < kRVisibleTicks);", "R blink duty cycle")

    require(makefile, "RUNMAT.g3a: runmat_mock.bin", "make target")
    require(makefile, "-n basic:RunMat -n internal:RUNMAT", "metadata")
    require(makefile, "RUNMAT_LIBS =", "standalone libs")
    if "RUNMAT_LIBS = " in makefile and ("-lcas" in makefile.split("RUNMAT_LIBS =", 1)[1].split("\n", 1)[0]):
        raise SystemExit("FAIL runmat standalone libs include -lcas")

    require(build, "RUNMAT_TARGET", "build variable")
    require(build, 'cp "${ROOT_DIR}/tools/runmat_mock.cc" "${SRC_DIR}/runmat_mock.cc"', "source install")
    require(build, 'cp "${OUT_DIR}/${RUNMAT_TARGET}" "${TRANSFER_DIR}/${RUNMAT_TARGET}"', "transfer copy")
    require(build, "--name RunMat", "metadata check")
    print("OK runmat mock")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

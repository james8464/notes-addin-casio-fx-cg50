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

    require(source, "SAF_BATTERY", "battery status flag")
    for setup_flag in (
        "SAF_SETUP_INPUT_OUTPUT",
        "SAF_SETUP_ANGLE",
        "SAF_SETUP_DISPLAY",
        "SAF_SETUP_FRAC_RESULT",
        "SAF_SETUP_COMPLEX_MODE",
    ):
        require(source, setup_flag, setup_flag)
    require(source, "DisplayStatusArea();", "OS status draw")
    if "status_box(" in source or "draw_status_labels" in source:
        raise SystemExit("FAIL runmat custom status text present")
    require(source, "extern \"C\" void DirectDrawRectangle", "direct border syscall")
    require(source, "const unsigned short kCasioCasPink = 0xF81F;", "pink")
    require(source, "DirectDrawRectangle(0, 0, 5, 223, kCasioCasPink);", "left border")
    require(source, "DirectDrawRectangle(390, 0, 395, 223, kCasioCasPink);", "right border")
    require(source, "DirectDrawRectangle(0, 217, 395, 223, kCasioCasPink);", "bottom border")
    require(source, "Bdisp_PutDisp_DD();\n  drawCasioCasBorder();", "border after flush")
    require(source, "GetFKeyPtr", "OS fkey bitmap lookup")
    require(source, "FKey_Display", "OS fkey display")
    require(source, "kFKeyJump = 508", "JUMP fkey bitmap")
    require(source, "kFKeyDelete = 0x38", "DELETE fkey bitmap")
    require(source, "draw_r_indicator", "R indicator")
    require(source, "kRBlinkPeriodTicks = 384", "R 3s period")
    require(source, "kRVisibleTicks = 256", "R 2s visible window")
    require(source, "RTC_GetTicks", "RTC blink clock")
    require(source, "KEYWAIT_HALTOFF_TIMEROFF", "nonblocking key poll")
    require(source, "GetKeyWait_OS(&col, &row, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &keycode)", "nonblocking key call")
    require(source, "OS_InnerWait_ms(40)", "loop pacing")
    require(source, "fill_rect(339, 0, 21, 24, visible ? COLOR_BLUE : kWhite);", "R blue highlight")
    require(source, 'PrintCXY(342, 1, "R", 0x40', "status-area R glyph")
    require(source, "kFKeyBlackTemplate = 0x0190", "OS black fkey template")
    require(source, "display_fkey(slot, kFKeyBlackTemplate);", "OS fkey background")
    require(source, "PrintMini(&x, &y, (unsigned char *)text", "OS mini fkey text")
    require(source, 'draw_os_text_fkey(2, "MAT/VCT", 4);', "MAT/VCT fkey")
    require(source, 'draw_os_text_fkey(3, "MATH", 9);', "MATH fkey")
    require(source, '"MAT/VCT"', "MAT/VCT fkey")
    require(source, '"MATH"', "MATH fkey")
    if "PrintCXY(340" in source:
        raise SystemExit("FAIL runmat clipped R PrintCXY present")
    if "draw_custom_fkey_text" in source or "Bdisp_MMPrint(x + 4, 196, text" in source:
        raise SystemExit("FAIL runmat custom fkey box renderer present")
    if "static unsigned char glyph" in source or "draw_pixel_text" in source:
        raise SystemExit("FAIL runmat manual pixel glyph renderer present")

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

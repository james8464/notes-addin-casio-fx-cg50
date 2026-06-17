#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "khicas/upstream/giac90_1addin"


def require(text: str, marker: str, label: str) -> None:
    if marker not in text:
        raise SystemExit(f"FAIL runmat {label}: missing {marker!r}")


def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"FAIL runmat icon is not PNG: {path.name}")
    return struct.unpack(">II", data[16:24])


def main() -> int:
    source = (ROOT / "apps/runmat/runmat_mock.cc").read_text(errors="ignore")
    makefile = (SRC / "Makefile").read_text(errors="ignore")
    build = (ROOT / "tools/build/build_g3a.sh").read_text(errors="ignore")
    for icon_name in ("runmat_icon.png", "runmat_icon_selected.png"):
        icon_path = SRC / icon_name
        if not icon_path.exists():
            raise SystemExit(f"FAIL runmat icon missing: {icon_name}")
        if png_size(icon_path) != (92, 64):
            raise SystemExit(f"FAIL runmat icon size: {icon_name}")

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
    require(source, "fill_rect(339, 0, 21, 23, COLOR_BLUE);", "R blue highlight")
    require(source, "fill_rect(339, 0, 21, 23, kWhite);", "R off-state clear")
    require(source, "fill_rect(339, 23, 21, 2, kWhite);", "R lower cleanup")
    require(source, "hline(339, 359, 23, kBlack);", "R divider restore")
    require(source, 'PrintCXY(342, 1, "R", 0x40', "status-area R glyph")
    require(source, "display_fkey(slot, base_id);", "OS fkey base")
    require(source, "fill_rect(slot * 64 + 2, 198, 60, 16, kBlack);", "fkey text cover")
    require(source, "Bdisp_MMPrint(slot * 64 + xoffset, 196, text, 0x40, 0xffffffff", "OS fkey text")
    require(source, 'draw_os_text_fkey(2, kFKeyDelete, "MAT/VCT", 4);', "MAT/VCT fkey")
    require(source, 'draw_os_text_fkey(3, kFKeyDelete, "MATH", 9);', "MATH fkey")
    require(source, '"MAT/VCT"', "MAT/VCT fkey")
    require(source, '"MATH"', "MATH fkey")
    require(source, "draw_input_box", "input box helper")
    require(source, "fill_rect(13, 31, 14, 17, kWhite);", "input box clear")
    require(source, "rect_outline(13, 31, 14, 17, kBlack);", "input box outline")
    require(source, "fill_rect(13, 31, 2, 17, kBlack);", "input box left accent")
    if "PrintCXY(340" in source:
        raise SystemExit("FAIL runmat clipped R PrintCXY present")
    if "hline(6, 389, 24" in source:
        raise SystemExit("FAIL runmat duplicate status divider present")
    if "draw_custom_fkey_text" in source or "PrintMini(&x, &y, (unsigned char *)text" in source:
        raise SystemExit("FAIL runmat custom fkey box renderer present")
    if "fill_rect(339, 0, 21, 23, visible ? COLOR_BLUE : kWhite);" in source:
        raise SystemExit("FAIL runmat R off-state leaves blue residue")
    if "kFKeyBlackTemplate = 0x0190" in source:
        raise SystemExit("FAIL runmat wave fkey template present")
    if "static unsigned char glyph" in source or "draw_pixel_text" in source:
        raise SystemExit("FAIL runmat manual pixel glyph renderer present")

    require(makefile, "RUNMAT.g3a: runmat_mock.bin runmat_icon.png runmat_icon_selected.png", "make target")
    require(makefile, "-n basic:RunMat -n internal:RUNMAT", "metadata")
    require(makefile, "-i uns:runmat_icon.png -i sel:runmat_icon_selected.png", "RUNMAT icons")
    if "RUNMAT.g3a: runmat_mock.bin\n\tmkg3a" in makefile:
        raise SystemExit("FAIL runmat still using shared Khicas icons")
    require(makefile, "RUNMAT_LIBS =", "standalone libs")
    if "RUNMAT_LIBS = " in makefile and ("-lcas" in makefile.split("RUNMAT_LIBS =", 1)[1].split("\n", 1)[0]):
        raise SystemExit("FAIL runmat standalone libs include -lcas")

    require(build, 'python3 "${ROOT_DIR}/tools/build/generate_runmat_icons.py"', "RUNMAT icon generation")
    require(build, 'cp "${ROOT_DIR}/apps/runmat/runmat_mock.cc" "${SRC_DIR}/runmat_mock.cc"', "source install")
    require(build, "copy_and_check RUNMAT.g3a RunMat @RUNMAT", "transfer copy")
    require(build, "copy_and_check RUNMAT.g3a RunMat @RUNMAT", "metadata check")
    print("OK runmat mock")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

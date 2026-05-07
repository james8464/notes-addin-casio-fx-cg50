#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "c++/khicas/upstream/giac90_1addin"


def require(text: str, marker: str, label: str) -> None:
    if marker not in text:
        raise SystemExit(f"FAIL {label}: missing {marker!r}")


def main() -> int:
    gfx_h = (SRC / "graphicsProvider.hpp").read_text()
    gfx = (SRC / "graphicsProvider.cpp").read_text()
    console = (SRC / "console.cc").read_text()
    catalog = (SRC / "catalogen.cpp").read_text()
    dconsole = (SRC / "dConsole.cpp").read_text()

    require(gfx_h, "void drawCasioCasBorder();", "border declaration")
    require(gfx, "const unsigned short kCasioCasPink = 0xF81F;", "pink color")
    require(gfx, "drawRectangle(0, 0, 6, LCD_HEIGHT_PX, kCasioCasPink);", "left safe border")
    require(gfx, "drawRectangle(LCD_WIDTH_PX-6, 0, 6, LCD_HEIGHT_PX, kCasioCasPink);", "right safe border")
    require(gfx, "drawRectangle(0, LCD_HEIGHT_PX-7, LCD_WIDTH_PX, 7, kCasioCasPink);", "bottom safe border")
    for unsafe in ("390, 0, 395, 223", "0, 217, 395, 223", "0, 0, 5, 223"):
        if unsafe in gfx:
            raise SystemExit(f"FAIL calculator border: unsafe physical-frame draw {unsafe}")
    if "DirectDrawRectangle(LCD_WIDTH_PX+6" in dconsole:
        raise SystemExit("FAIL calculator border: unsafe scroll indicator outside LCD_WIDTH_PX")
    require(dconsole, "drawRectangle(LCD_WIDTH_PX-6, 24, 5, LCD_HEIGHT_PX-30, COLOR_WHITE);", "safe scroll clear")
    require(dconsole, "drawRectangle(LCD_WIDTH_PX-6, 24+starty, 5, 8, COLOR_BLACK);", "safe scroll thumb")
    require(console, "drawCasioCasBorder();\n  startCasioCasStatusR();\n  Bdisp_PutDisp_DD();", "console border before flush")
    require(catalog, "drawCasioCasBorder();\n    startCasioCasStatusR();\n    Bdisp_PutDisp_DD();", "catalog border before flush")
    print("OK calculator border")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

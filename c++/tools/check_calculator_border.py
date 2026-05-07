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

    require(gfx_h, "void drawCasioCasBorder();", "border declaration")
    require(gfx, "const unsigned short kCasioCasPink = 0xF81F;", "pink color")
    require(gfx, "drawRectangle(0, 0, 6, LCD_HEIGHT_PX, kCasioCasPink);", "left border")
    require(gfx, "drawRectangle(LCD_WIDTH_PX-6, 0, 6, LCD_HEIGHT_PX, kCasioCasPink);", "right border")
    require(gfx, "drawRectangle(0, LCD_HEIGHT_PX-7, LCD_WIDTH_PX, 7, kCasioCasPink);", "bottom border")
    require(console, "drawCasioCasBorder();\n  Bdisp_PutDisp_DD();", "console present")
    require(catalog, "drawCasioCasBorder();\n    Bdisp_PutDisp_DD();", "catalog present")
    print("OK calculator border")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

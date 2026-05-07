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
    require(gfx, "DirectDrawRectangle(0, 0, 5, 223, kCasioCasPink);", "left DD frame border")
    require(gfx, "DirectDrawRectangle(390, 0, 395, 223, kCasioCasPink);", "right DD frame border")
    require(gfx, "DirectDrawRectangle(0, 217, 395, 223, kCasioCasPink);", "bottom DD frame border")
    require(console, "Bdisp_PutDisp_DD();\n  drawCasioCasBorder();", "console present after flush")
    require(catalog, "Bdisp_PutDisp_DD();\n    drawCasioCasBorder();", "catalog present after flush")
    print("OK calculator border")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

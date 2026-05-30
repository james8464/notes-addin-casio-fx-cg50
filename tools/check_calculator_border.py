#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "khicas/upstream/giac90_1addin"


def require(text: str, marker: str, label: str) -> None:
    if marker not in text:
        raise SystemExit(f"FAIL border {label}: missing {marker!r}")


def main() -> int:
    gfx_h = (SRC / "graphicsProvider.hpp").read_text(errors="ignore")
    gfx = (SRC / "graphicsProvider.cpp").read_text(errors="ignore")
    console = (SRC / "console.cc").read_text(errors="ignore")
    catalog = (SRC / "catalogen.cpp").read_text(errors="ignore")

    require(gfx_h, "void drawCasioCasBorder();", "declaration")
    require(gfx, "const unsigned short kCasioCasPink = 0xF81F;", "pink")
    require(gfx, "DirectDrawRectangle(0, 0, 5, 223, kCasioCasPink);", "left")
    require(gfx, "DirectDrawRectangle(390, 0, 395, 223, kCasioCasPink);", "right")
    require(gfx, "DirectDrawRectangle(0, 217, 395, 223, kCasioCasPink);", "bottom")
    require(console, "Bdisp_PutDisp_DD();\n  drawCasioCasBorder();", "console flush")
    require(catalog, "Bdisp_PutDisp_DD();\n    drawCasioCasBorder();", "catalog flush")
    print("OK calculator border")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

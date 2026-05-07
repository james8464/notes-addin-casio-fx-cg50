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
    main_cc = (SRC / "main.cc").read_text()
    catalog = (SRC / "catalogen.cpp").read_text()

    require(gfx_h, "void startCasioCasStatusR();", "status R timer declaration")
    require(gfx, "static void drawCasioCasStatusR()", "private status R drawer")
    require(gfx, "status_r_phase=(status_r_phase+1)%12", "3s blink cycle")
    require(gfx, "status_r_phase<8", "2s visible phase")
    require(gfx, "Timer_Install(0,status_r_tick,250)", "status R refresh timer")
    require(gfx, "DirectDrawRectangle(x-1,y-1,x+13,y+15,COLOR_WHITE);", "status R clear")
    require(gfx, 'PrintMiniMini(&tx,&ty,(unsigned char*)"R",0,TEXT_COLOR_BLUE,0);', "status R glyph draw")
    require(console, "drawCasioCasBorder();\n  startCasioCasStatusR();", "console status R draw")
    require(main_cc, "Bdisp_PutDisp_DD ();\n    startCasioCasStatusR();", "key loop status R draw")
    require(catalog, "drawCasioCasBorder();\n    startCasioCasStatusR();", "catalog status R draw")
    print("OK status R")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

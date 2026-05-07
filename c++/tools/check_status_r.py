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
    require(gfx, "RTC_GetTicks()/128", "status R tick source")
    require(gfx, "%3)==2", "2s visible / 1s hidden phase")
    if "Timer_Install(0,status_r_tick" in gfx or "drawCasioCasStatusR();" in gfx.partition("static void status_r_tick")[2]:
        raise SystemExit("FAIL status R: LCD drawing must not happen from timer callbacks")
    if "DirectDrawRectangle(" in gfx:
        raise SystemExit("FAIL status R: use safe VRAM drawing, not DirectDrawRectangle")
    require(gfx, "drawRectangle(x-1,y-1,14,16,COLOR_WHITE);", "status R clear")
    require(gfx, 'PrintMiniMini(&tx,&ty,(unsigned char*)"R",0,TEXT_COLOR_BLUE,0);', "status R glyph draw")
    require(console, "startCasioCasStatusR();\n  Bdisp_PutDisp_DD();", "console status R draw before DD")
    if "Bdisp_PutDisp_DD ();\n    startCasioCasStatusR();" in main_cc:
        raise SystemExit("FAIL status R: do not draw from ck_getkey before key wait")
    require(catalog, "startCasioCasStatusR();\n    Bdisp_PutDisp_DD();", "catalog status R draw before DD")
    print("OK status R")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "c++/khicas/upstream/giac90_1addin"


def require(text: str, marker: str, label: str) -> None:
    if marker not in text:
        raise SystemExit(f"FAIL {label}: missing {marker!r}")


def main() -> int:
    gfx = (SRC / "graphicsProvider.cpp").read_text()
    console = (SRC / "console.cc").read_text()
    main_cc = (SRC / "main.cc").read_text()
    catalog = (SRC / "catalogen.cpp").read_text()

    if "static void drawCasioCasStatusR()" in gfx or 'PrintMiniMini(&tx,&ty,(unsigned char*)"R"' in gfx:
        raise SystemExit("FAIL status R: launch path must not draw custom status glyphs")
    if "Timer_Install(0,status_r_tick" in gfx:
        raise SystemExit("FAIL status R: do not install status timers")
    if "DirectDrawRectangle(" in gfx:
        raise SystemExit("FAIL status R: use safe VRAM drawing, not DirectDrawRectangle")
    if "startCasioCasStatusR();\n  Bdisp_PutDisp_DD();" in console:
        raise SystemExit("FAIL status R: no custom status draw during console flush")
    if "Bdisp_PutDisp_DD ();\n    startCasioCasStatusR();" in main_cc:
        raise SystemExit("FAIL status R: no custom status draw from key wait")
    if "startCasioCasStatusR();\n    Bdisp_PutDisp_DD();" in catalog:
        raise SystemExit("FAIL status R: no custom status draw during catalog flush")
    print("OK status R")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

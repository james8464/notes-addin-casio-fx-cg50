#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "c++/khicas/upstream/giac90_1addin"


def main() -> int:
    bad: list[str] = []
    for path in SRC.glob("*.[ch]*"):
        text = path.read_text(errors="ignore")
        if "unsigned short VRAMbuffer[" in text:
            bad.append(f"{path.name}: large VRAMbuffer on stack")
        for marker in ("char BUF[L+4]", "char bufscript[L+1]", "char buf[l+1]"):
            if marker in text:
                bad.append(f"{path.name}: unbounded session VLA {marker}")
        if "DirectDrawRectangle(LCD_WIDTH_PX+" in text:
            bad.append(f"{path.name}: DirectDrawRectangle outside LCD_WIDTH_PX")
        if path.name == "graphicsProvider.cpp" and "DirectDrawRectangle(" in text:
            bad.append(f"{path.name}: direct LCD rectangle in UI helper")
        for marker in ("390, 0, 395, 223", "0, 217, 395, 223", "0, 0, 5, 223"):
            if marker in text:
                bad.append(f"{path.name}: physical-frame draw {marker}")
    restore_text = (SRC / "console.cc").read_text(errors="ignore") + (SRC / "dConsole.cpp").read_text(errors="ignore")
    if "SESSION_RESTORE_MAX_BLOCK" not in restore_text or "SESSION_RESTORE_MAX_LINE" not in restore_text:
        bad.append("session restore: missing restore caps")
    if bad:
        raise SystemExit("FAIL calculator runtime safety:\n" + "\n".join(bad))
    print("OK calculator runtime safety")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

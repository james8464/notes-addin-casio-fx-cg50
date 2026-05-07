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
        if "DirectDrawRectangle(LCD_WIDTH_PX+" in text:
            bad.append(f"{path.name}: DirectDrawRectangle outside LCD_WIDTH_PX")
        for marker in ("390, 0, 395, 223", "0, 217, 395, 223", "0, 0, 5, 223"):
            if marker in text:
                bad.append(f"{path.name}: physical-frame draw {marker}")
    if bad:
        raise SystemExit("FAIL calculator runtime safety:\n" + "\n".join(bad))
    print("OK calculator runtime safety")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

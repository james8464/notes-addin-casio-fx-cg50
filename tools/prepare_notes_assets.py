#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
from PIL import Image


def convert_one(src: Path, out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    dst = out_dir / (src.stem + ".bmp")
    img = Image.open(src).convert("RGB")
    img.thumbnail((384, 192))
    img.save(dst, format="BMP")
    return dst


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert note images to calculator-friendly BMP files.")
    ap.add_argument("inputs", nargs="+", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=Path("calculator_notes"))
    ns = ap.parse_args()
    for item in ns.inputs:
        if item.is_dir():
            for src in item.rglob("*"):
                if src.suffix.lower() in {".png", ".jpg", ".jpeg", ".bmp"}:
                    print(convert_one(src, ns.out / src.relative_to(item).parent))
        elif item.suffix.lower() in {".png", ".jpg", ".jpeg", ".bmp"}:
            print(convert_one(item, ns.out))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

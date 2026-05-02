#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path


APP_NAME = "fx-CG Manager PLUS Subscription for fx-CG50series"
DEFAULT_SD_REL = Path("Library") / "Application Support" / "CASIO" / APP_NAME / "fx-CG50" / "EmulatorData" / "SDCard"


def default_sdcard_dir() -> Path:
    return Path.home() / DEFAULT_SD_REL


def find_sdcard_dir(configured: str | None = None) -> Path | None:
    if configured:
        path = Path(configured).expanduser()
        return path if path.exists() else None

    default = default_sdcard_dir()
    if default.exists():
        return default

    support = Path.home() / "Library" / "Application Support" / "CASIO"
    if support.exists():
        matches = sorted(support.glob("*/fx-CG50/EmulatorData/SDCard"))
        if matches:
            return matches[0]

    return None


def install(g3a_paths: list[Path], sdcard_dir: Path) -> list[Path]:
    sdcard_dir.mkdir(parents=True, exist_ok=True)
    copied = []
    for src in g3a_paths:
        if not src.exists() or src.suffix.lower() != ".g3a":
            raise FileNotFoundError(f"not a .g3a file: {src}")
        dst = sdcard_dir / src.name
        tmp = dst.with_suffix(dst.suffix + ".tmp")
        shutil.copy2(src, tmp)
        tmp.replace(dst)
        copied.append(dst)
    return copied


def main() -> int:
    parser = argparse.ArgumentParser(description="Copy .g3a files into the CASIO fx-CG50 emulator SD card folder.")
    parser.add_argument("g3a", nargs="+", type=Path, help=".g3a file(s) to copy")
    parser.add_argument("--sdcard-dir", default=os.environ.get("CASIO_EMULATOR_SDCARD_DIR", ""), help="override emulator SDCard directory")
    args = parser.parse_args()

    sdcard = find_sdcard_dir(args.sdcard_dir or None)
    if sdcard is None:
        print("CASIO emulator SDCard folder not found.", file=sys.stderr)
        print(f"Expected: {default_sdcard_dir()}", file=sys.stderr)
        return 2

    copied = install([p.expanduser().resolve() for p in args.g3a], sdcard)
    for path in copied:
        print(f"copied {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

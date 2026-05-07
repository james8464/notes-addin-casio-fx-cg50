#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
from pathlib import Path


FILES = ("CasioCAS.g3a", "CASIOCAS.PAK")
NAME_RE = re.compile(r"(casio|fx.?cg|cg50|prizm)", re.I)


def fail(msg: str) -> int:
    print("FAIL " + msg, file=sys.stderr)
    return 1


def candidate_roots(args_roots: list[str]) -> list[tuple[Path, bool]]:
    """Return (path, explicit). Explicit paths sync even if files are absent."""
    out: list[tuple[Path, bool]] = []
    explicit_env = os.environ.get("CASIO_CALCULATOR_VOLUME", "").strip()
    if explicit_env:
        out.append((Path(explicit_env).expanduser(), True))
    for raw in args_roots:
        out.append((Path(raw).expanduser(), True))
    if out:
        return out

    roots = [Path("/Volumes")]
    user = os.environ.get("USER") or os.environ.get("LOGNAME") or ""
    if user:
        roots += [Path("/media") / user, Path("/run/media") / user]
    for root in roots:
        if not root.is_dir():
            continue
        try:
            for child in sorted(root.iterdir()):
                if child.is_dir():
                    out.append((child, False))
        except OSError:
            continue
    return out


def is_calculator_volume(path: Path, explicit: bool) -> tuple[bool, str]:
    if explicit:
        return True, "explicit"
    existing = [name for name in FILES if (path / name).exists()]
    if existing:
        return True, "existing " + ",".join(existing)
    if NAME_RE.search(path.name):
        return True, "volume-name"
    return False, ""


def sync_volume(transfer: Path, volume: Path, dry_run: bool) -> None:
    for name in FILES:
        src = transfer / name
        dst = volume / name
        if not src.is_file():
            raise FileNotFoundError(src)
        if dry_run:
            print(f"Would replace {dst} <- {src}")
            continue
        if dst.exists() or dst.is_symlink():
            dst.unlink()
        shutil.copy2(src, dst)
        if dst.stat().st_size != src.stat().st_size:
            raise OSError(f"size mismatch after copy: {dst}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Replace CasioCAS transfer files on a mounted calculator volume.")
    ap.add_argument("transfer_dir", type=Path, help="folder containing CasioCAS.g3a and CASIOCAS.PAK")
    ap.add_argument("--volume", action="append", default=[], help="explicit calculator volume path")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    if os.environ.get("CASIO_AUTO_SYNC", "1").lower() in ("0", "false", "no", "off"):
        print("Calculator auto-sync disabled (CASIO_AUTO_SYNC=0).")
        return 0

    transfer = args.transfer_dir.resolve()
    missing = [name for name in FILES if not (transfer / name).is_file()]
    if missing:
        return fail("transfer files missing: " + ", ".join(missing))

    synced = 0
    for path, explicit in candidate_roots(args.volume):
        try:
            volume = path.resolve()
        except OSError:
            continue
        if not volume.is_dir() or volume == transfer:
            continue
        ok, reason = is_calculator_volume(volume, explicit)
        if not ok:
            continue
        try:
            sync_volume(transfer, volume, args.dry_run)
        except OSError as err:
            return fail(f"could not sync {volume}: {err}")
        synced += 1
        action = "Would sync" if args.dry_run else "Synced"
        print(f"{action} calculator volume: {volume} ({reason})")

    if synced == 0:
        print(f"No calculator volume found; transfer files ready in {transfer}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

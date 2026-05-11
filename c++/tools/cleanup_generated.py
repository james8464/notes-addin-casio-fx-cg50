#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
from pathlib import Path


REL_TARGETS = (
    ".DS_Store",
    "c++/.DS_Store",
    "c++/tests/reports",
    "c++/tools/tests_cpp/__pycache__",
)


def _children_or_self(path: Path) -> list[Path]:
    if not path.exists():
        return []
    if path.is_dir() and path.name == "reports":
        return sorted(path.iterdir()) + [path]
    return [path]


def cleanup(root: Path, dry_run: bool = True) -> list[Path]:
    root = Path(root)
    targets: list[Path] = []
    for rel in REL_TARGETS:
        targets.extend(_children_or_self(root / rel))

    found = sorted({p for p in targets if p.exists()}, key=lambda p: p.as_posix())
    if dry_run:
        return found
    for path in sorted(found, key=lambda p: (-len(p.parts), p.as_posix())):
        if not path.exists():
            continue
        if path.is_dir():
            if path.name == "reports":
                try:
                    path.rmdir()
                except OSError:
                    continue
            else:
                shutil.rmtree(path)
        else:
            path.unlink()
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description="Remove ignored generated CASIO workspace files.")
    ap.add_argument("--apply", action="store_true", help="Delete files; default is dry-run.")
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = ap.parse_args()

    removed = cleanup(args.root, dry_run=not args.apply)
    prefix = "would remove" if not args.apply else "removed"
    for path in removed:
        print(f"{prefix}: {path.relative_to(args.root)}")
    print(f"{prefix}: {len(removed)} paths")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
from pathlib import Path


REL_TARGETS = (
    "c++/tests/reports",
    "c++/tools/fuzz/catalogue_manifest_latest.txt",
    "c++/tools/fuzz/random_exploration_graph.json",
    "c++/tools/fuzz/tui_failures.jsonl",
)

GLOB_TARGETS = (
    "**/.DS_Store",
    "**/__pycache__",
    "**/*.pyc",
    "**/*.pyo",
)


def _children_or_self(path: Path) -> list[Path]:
    if not path.exists():
        return []
    if path.is_dir() and path.name == "reports":
        return sorted(path.rglob("*")) + [path]
    return [path]


def cleanup(root: Path, dry_run: bool = True) -> list[Path]:
    root = Path(root)
    targets: list[Path] = []
    for rel in REL_TARGETS:
        targets.extend(_children_or_self(root / rel))
    for pattern in GLOB_TARGETS:
        for path in root.glob(pattern):
            if ".git" not in path.parts:
                targets.append(path)

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

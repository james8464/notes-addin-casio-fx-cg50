#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

GENERATED_PATHS = [
    ".idea",
    "calculator_files",
    "c++/addin/host/build",
    "c++/prizm/build",
    "c++/tools/fuzz/random_exploration_graph.json",
    "c++/tools/fuzz/tui_failures.jsonl",
    "c++/tools/fuzz/catalogue_manifest_latest.txt",
    "paper1_audit",
    "paper2_audit",
    "graphify-out",
    "c++/graphify-out",
    "c++/tools/tests_cpp/reports",
    "c++/addin/src/device/cas_lib",
]


def remove_path(path: Path, dry_run: bool) -> bool:
    if not path.exists() and not path.is_symlink():
        return False
    rel = path.relative_to(ROOT)
    print(("would remove " if dry_run else "removed ") + str(rel))
    if dry_run:
        return True
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path)
    else:
        path.unlink()
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Remove generated/local project artifacts.")
    parser.add_argument("--apply", action="store_true", help="Delete files; default is dry-run.")
    parser.add_argument("--reports", action="store_true", help="Also delete c++/tests/reports; default preserves reports except empty dirs.")
    args = parser.parse_args()
    dry_run = not args.apply

    removed = 0
    paths = list(GENERATED_PATHS)
    if args.reports:
        paths.append("c++/tests/reports")
    for rel in paths:
        removed += int(remove_path(ROOT / rel, dry_run))

    for pattern in ("**/__pycache__", "**/*.pyc", "**/*.pyo", "**/.DS_Store"):
        for path in sorted(ROOT.glob(pattern)):
            if ".git" in path.parts:
                continue
            removed += int(remove_path(path, dry_run))

    for rel in ("c++/tests",):
        path = ROOT / rel
        if path.is_dir() and not any(path.iterdir()):
            removed += int(remove_path(path, dry_run))
    reports = ROOT / "c++/tests/reports"
    if reports.is_dir() and not args.reports:
        for path in sorted((p for p in reports.rglob("*") if p.is_dir() and not any(p.iterdir())), reverse=True):
            removed += int(remove_path(path, dry_run))

    print(("would remove total " if dry_run else "removed total ") + str(removed))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

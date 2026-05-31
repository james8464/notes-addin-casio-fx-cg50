#!/usr/bin/env python3
"""Diff VM inventory vs golden queue source_pdf and progress/vm_coverage.json."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]
VM_ROOT = Path("/Volumes/VM")
COVERAGE = REPO / "progress" / "vm_coverage.json"
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"

PURE_MADAS = re.compile(r"^(c[134]_|c2_|mp[12]_|syn_)[a-z0-9]+$")
SKIP_MADAS = re.compile(r"^(mms|sp|spx)_[a-z0-9]+$")
PRIMARY_MADAS_DIR = re.compile(
    r"^(c[134]_|c2_|mp[12]_|syn_|mms_|sp_|spx_)[a-z0-9]+ conv_png$"
)


def load_coverage() -> dict[str, dict]:
    if not COVERAGE.exists():
        return {}
    return json.loads(COVERAGE.read_text())


def queue_rows() -> list[dict]:
    rows: list[dict] = []
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if line.strip():
            rows.append(json.loads(line))
    return rows


def source_has_complete_marker(rows: list[dict], source_pdf: str) -> bool:
    for row in rows:
        if row.get("source_pdf") == source_pdf and "complete_source_marker" in row.get("id", ""):
            return True
        if row.get("source_pdf") == source_pdf and row.get("verdict") == "skip" and row.get("coverage") == "complete":
            return True
    return False


def madas_primary_sources() -> list[str]:
    root = VM_ROOT / "MadAsMaths papers"
    if not root.is_dir():
        return []
    seen: set[str] = set()
    for name in sorted(root.iterdir()):
        if not name.is_dir() or not PRIMARY_MADAS_DIR.match(name.name):
            continue
        base = name.name.replace(" conv_png", "")
        if base.endswith("_marks") or base.endswith("_solutions"):
            continue
        seen.add(f"MadAsMaths papers/{base}.pdf")
    return sorted(seen)


def madas_pure_sources() -> list[str]:
    return [s for s in madas_primary_sources() if PURE_MADAS.match(Path(s).stem)]


def madas_skip_sources() -> list[str]:
    return [s for s in madas_primary_sources() if SKIP_MADAS.match(Path(s).stem)]


def edexcel_folder_sources() -> list[str]:
    root = VM_ROOT / "Edexcel A Level Maths past papers"
    if not root.is_dir():
        return []
    out: list[str] = []
    for folder in sorted(root.iterdir()):
        if not folder.is_dir() or "conv_png" not in folder.name.lower():
            continue
        stem = folder.name.replace(" conv_png", "").strip()
        out.append(f"Edexcel A Level Maths past papers/{stem}.pdf")
    return out


def edexcel_pure_sources() -> list[str]:
    out: list[str] = []
    for src in edexcel_folder_sources():
        low = src.lower()
        if re.search(r"(?:^|[^0-9])01(?:[^0-9]|$).*que|que.*(?:^|[^0-9])01(?:[^0-9]|$)", low):
            if "rms" not in low and "-ms-" not in low:
                out.append(src)
    return out


def edexcel_skip_sources() -> list[str]:
    pure = set(edexcel_pure_sources())
    return [s for s in edexcel_folder_sources() if s not in pure]


def booklet_sources() -> list[str]:
    root = VM_ROOT / "MadAsMaths A-level booklets"
    if not root.is_dir():
        return []
    out: list[str] = []
    for folder in sorted(root.rglob("* conv_png")):
        if not folder.is_dir():
            continue
        rel = folder.relative_to(VM_ROOT)
        stem = folder.name.replace(" conv_png", "")
        parent = rel.parent
        out.append(f"{parent}/{stem}.pdf".replace("\\", "/"))
    return sorted(set(out))


def standard_topic_sources() -> list[str]:
    root = VM_ROOT / "MadAsMaths standard topics"
    if not root.is_dir():
        return []
    out: list[str] = []
    for folder in sorted(root.rglob("* conv_png")):
        if not folder.is_dir():
            continue
        rel = folder.relative_to(VM_ROOT)
        stem = folder.name.replace(" conv_png", "")
        parent = rel.parent
        out.append(f"{parent}/{stem}.pdf".replace("\\", "/"))
    return sorted(set(out))


def support_material_sources() -> list[str]:
    root = VM_ROOT / "Edexcel A Level Maths support materials"
    if not root.is_dir():
        return []
    out: list[str] = []
    for folder in sorted(root.rglob("* conv_png")):
        if not folder.is_dir():
            continue
        rel = folder.relative_to(VM_ROOT)
        stem = folder.name.replace(" conv_png", "")
        parent = rel.parent
        out.append(f"{parent}/{stem}.pdf".replace("\\", "/"))
    return sorted(set(out))


def inventory(phase: str) -> list[str]:
    if phase == "madas_c2":
        return [s for s in madas_pure_sources() if "MadAsMaths papers/c2_" in s]
    if phase == "madas_pure":
        return madas_pure_sources()
    if phase == "madas_skip":
        return madas_skip_sources()
    if phase == "edexcel_pure":
        return edexcel_pure_sources()
    if phase == "edexcel_skip":
        return edexcel_skip_sources()
    if phase == "booklets":
        return booklet_sources()
    if phase == "topics":
        return standard_topic_sources()
    if phase == "support":
        return support_material_sources()
    # all
    items: list[str] = []
    items.extend(madas_pure_sources())
    items.extend(madas_skip_sources())
    items.extend(edexcel_folder_sources())
    items.extend(support_material_sources())
    items.extend(booklet_sources())
    items.extend(standard_topic_sources())
    return sorted(set(items))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--pending-only", action="store_true")
    ap.add_argument(
        "--phase",
        choices=(
            "madas_c2",
            "madas_pure",
            "madas_skip",
            "madas_all",
            "edexcel_pure",
            "edexcel_skip",
            "booklets",
            "topics",
            "support",
            "all",
        ),
        default="all",
    )
    args = ap.parse_args()

    if not VM_ROOT.is_dir():
        print(f"BLOCKER: VM not mounted at {VM_ROOT}", file=sys.stderr)
        return 2

    phase = args.phase
    if phase == "madas_all":
        inv = madas_pure_sources() + madas_skip_sources()
    else:
        inv = inventory(phase)

    cov = load_coverage()
    rows = queue_rows()
    qsrc = {r["source_pdf"] for r in rows}

    pending: list[str] = []
    for src in inv:
        if cov.get(src, {}).get("status") == "complete":
            continue
        if source_has_complete_marker(rows, src):
            continue
        pending.append(src)

    complete = len(inv) - len(pending)

    print(f"VM inventory (phase={phase}): {len(inv)} sources")
    print(f"Coverage file entries: {len(cov)}")
    print(f"Queue distinct source_pdf: {len(qsrc)}")
    if inv:
        print(f"Complete: {complete}/{len(inv)} ({100 * complete / len(inv):.1f}%)")

    if args.pending_only:
        for s in pending:
            print(s)
    else:
        print(f"Pending ({len(pending)}):")
        for s in pending[:50]:
            print(f"  {s}")
        if len(pending) > 50:
            print(f"  ... and {len(pending) - 50} more")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

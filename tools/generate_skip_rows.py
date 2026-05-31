#!/usr/bin/env python3
"""Generate skip/source-complete rows for out-of-scope VM sources."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]
VM_ROOT = Path("/Volumes/VM")
OUT_DIR = REPO / "progress" / "batches"

SKIP_MADAS = re.compile(r"^(mms|sp|spx)_([a-z0-9]+)$")
REASON_MADAS = {
    "mms": "mechanics out of A-level Pure CAS scope",
    "sp": "statistics/probability out of A-level Pure CAS scope",
    "spx": "statistics/probability out of A-level Pure CAS scope",
}


def edexcel_skip_reason(stem: str) -> str:
    low = stem.lower()
    if re.search(r"\b02\b|stats", low):
        return "statistics/probability out of A-level Pure CAS scope"
    if re.search(r"\b03\b|mech", low):
        return "mechanics out of A-level Pure CAS scope"
    if re.search(r"\b31\b|\b32\b|fm", low):
        return "further maths out of A-level Pure CAS scope"
    if "rms" in low or "-ms-" in low:
        return "mark scheme reference only; question paper queued separately where applicable"
    return "out of A-level Pure CAS scope"


def madas_skip_row(stem: str) -> dict:
    m = SKIP_MADAS.match(stem)
    if not m:
        raise ValueError(stem)
    series, letter = m.group(1), m.group(2)
    source = f"MadAsMaths papers/{stem}.pdf"
    return {
        "id": f"madas_{series}_{letter}_complete_source_marker",
        "source_pdf": source,
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual inventory: {stem} is {series.upper()} series on VM",
        "unsupported_reason": REASON_MADAS[series],
    }


def edexcel_skip_row(source_pdf: str) -> dict:
    stem = Path(source_pdf).stem
    safe = re.sub(r"[^a-zA-Z0-9]+", "_", stem).strip("_").lower()
    return {
        "id": f"edexcel_{safe}_complete_source_marker",
        "source_pdf": source_pdf,
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual inventory: Edexcel folder {stem}",
        "unsupported_reason": edexcel_skip_reason(stem),
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--phase", choices=("madas_skip", "edexcel_skip"), required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    sys.path.insert(0, str(REPO / "tests"))
    from audit_vm_coverage import edexcel_skip_sources, madas_skip_sources, inventory

    if args.phase == "madas_skip":
        sources = madas_skip_sources()
        rows = [madas_skip_row(Path(s).stem) for s in sources]
        batch = OUT_DIR / "madas_skip_all.json"
    else:
        sources = edexcel_skip_sources()
        rows = [edexcel_skip_row(s) for s in sources]
        batch = OUT_DIR / "edexcel_skip_all.json"

    batch.write_text(json.dumps(rows, indent=2) + "\n")
    print(f"wrote {len(rows)} skip rows to {batch}")

    if args.dry_run:
        return 0

    import subprocess

    proc = subprocess.run(
        [
            sys.executable,
            str(REPO / "tools" / "append_queue_rows.py"),
            str(batch),
        ],
        cwd=REPO,
        text=True,
    )
    if proc.returncode != 0:
        return proc.returncode

    cov_path = REPO / "progress" / "vm_coverage.json"
    cov: dict = {}
    if cov_path.exists():
        cov = json.loads(cov_path.read_text())
    for row in rows:
        src = row["source_pdf"]
        prev = cov.get(src, {})
        cov[src] = {
            "status": "complete",
            "rows_added": prev.get("rows_added", 0) + 1,
            "last_batch": "generate_skip_rows",
        }
    cov_path.write_text(json.dumps(cov, indent=2) + "\n")
    print(f"coverage updated for {len(rows)} sources")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Fast inventory for the paper-audit queue."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[2]
CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
DOWNLOAD_ROOTS = (
    Path("/Users/james/Downloads/MadAsMaths A-level booklets"),
    Path("/Users/james/Downloads/Edexcel A Level Maths support materials"),
)


def rel_source(pathish: str) -> str:
    p = pathish.replace("\\", "/")
    for marker in ("MadAsMaths A-level booklets/", "Edexcel A Level Maths support materials/"):
        if marker in p:
            return p.split(marker, 1)[1]
    return Path(p).name


def local_pdfs() -> list[Path]:
    out: list[Path] = []
    for root in DOWNLOAD_ROOTS:
        if root.exists():
            out.extend(root.rglob("*.pdf"))
    return sorted(set(out), key=lambda p: str(p).lower())


def rows() -> list[dict[str, Any]]:
    return [json.loads(line) for line in CASES.read_text().splitlines() if line.strip()]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--next", type=int, default=20, help="Show next N pending unique documents.")
    ap.add_argument("--emit-duplicate-jsonl", action="store_true", help="Emit complete-marker JSONL for byte-identical pending duplicates.")
    args = ap.parse_args()

    case_rows = rows()
    complete = {
        rel_source(str(r.get("source_pdf", "")))
        for r in case_rows
        if r.get("source_pdf") and (r.get("coverage") == "complete" or r.get("item") == "source_complete_marker")
    }
    max_ord = max((int(r.get("ordinal", 0)) for r in case_rows), default=0)
    pdfs = local_pdfs()
    hashed: dict[str, str] = {}
    by_hash: dict[str, list[Path]] = defaultdict(list)
    for p in pdfs:
        h = sha256(p)
        hashed[rel_source(str(p))] = h
        by_hash[h].append(p)

    complete_hashes = {hashed[src] for src in complete if src in hashed}
    pending = [p for p in pdfs if rel_source(str(p)) not in complete]
    duplicate_pending = [
        p for p in pending
        if hashed[rel_source(str(p))] in complete_hashes
    ]
    seen_hashes: set[str] = set()
    unique_pending: list[Path] = []
    for p in pending:
        h = hashed[rel_source(str(p))]
        if h in seen_hashes or h in complete_hashes:
            continue
        seen_hashes.add(h)
        unique_pending.append(p)

    if args.emit_duplicate_jsonl:
        for i, p in enumerate(duplicate_pending, 1):
            src = rel_source(str(p))
            donor = next((rel_source(str(q)) for q in by_hash[hashed[src]] if rel_source(str(q)) in complete), "")
            rec = {
                "id": "duplicate_complete_" + src.lower().replace("/", "_").replace(".pdf", "").replace(" ", "_"),
                "source_pdf": src,
                "coverage": "complete",
                "qid": "all_questions",
                "ordinal": max_ord + i,
                "item": "byte_identical_duplicate_source_marker",
                "status": "unsupported-ok",
                "notes": f"Byte-identical duplicate of already reviewed source: {donor}. No new questions; no fake working added.",
            }
            print(json.dumps(rec, separators=(",", ":")))
        return 0

    print(f"local_pdfs={len(pdfs)} complete_local={len([p for p in pdfs if rel_source(str(p)) in complete])}")
    print(f"pending_local={len(pending)} unique_pending={len(unique_pending)} duplicate_pending_complete={len(duplicate_pending)}")
    print(f"tracked_rows={len(case_rows)} complete_sources={len(complete)}")
    print("")
    print("next_unique:")
    for p in unique_pending[: args.next]:
        print("  " + rel_source(str(p)))
    if duplicate_pending:
        print("")
        print("duplicate_pending_complete:")
        for p in duplicate_pending[: args.next]:
            print("  " + rel_source(str(p)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

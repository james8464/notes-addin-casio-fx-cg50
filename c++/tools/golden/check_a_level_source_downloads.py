#!/usr/bin/env python3
"""Verify the local A-level audit source download manifest."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

from download_a_level_audit_sources import PEARSON_9MA0_PUBLIC, PEARSON_9MA0_2025_PROBES, PEARSON_SUPPORT


REPO = Path(__file__).resolve().parents[3]
MANIFEST = REPO / "c++" / "tests" / "reports" / "a_level_source_downloads" / "manifest_latest.jsonl"

REQUIRED_FAMILY_COUNTS = {
    "pearson_9ma0_public": len(PEARSON_9MA0_PUBLIC),
    "pearson_9ma0_support": len(PEARSON_SUPPORT),
    "pearson_9ma0_probe": len(PEARSON_9MA0_2025_PROBES),
}

OPTIONAL_ALL_SCOPE_COUNTS = {
    "madas_standard": 62,
    "madas_booklets_a_level": 171,
    "madas_iygb": 624,
}


def valid_pdf(path: str) -> bool:
    if not path:
        return True
    p = Path(path)
    try:
        return p.stat().st_size > 1000 and p.read_bytes()[:4] == b"%PDF"
    except OSError:
        return False


def main() -> int:
    if not MANIFEST.exists():
        print(f"SKIP source downloads: missing {MANIFEST}")
        return 0
    rows = [json.loads(line) for line in MANIFEST.read_text(encoding="utf-8").splitlines() if line.strip()]
    counts = Counter(str(row.get("family", "")) for row in rows)
    bad = [
        f"{row.get('family')}/{row.get('page')}/{row.get('name')}: {row.get('status')} {row.get('error')}"
        for row in rows
        if str(row.get("status", "")).startswith(("failed", "bad"))
    ]
    invalid = [
        f"{row.get('family')}/{row.get('name')} -> {row.get('path')}"
        for row in rows
        if row.get("status") in {"downloaded", "exists"} and not valid_pdf(str(row.get("path", "")))
    ]
    missing_families = [f"{family}:{counts[family]}/{need}" for family, need in REQUIRED_FAMILY_COUNTS.items() if counts[family] < need]
    for family, need in OPTIONAL_ALL_SCOPE_COUNTS.items():
        if counts[family] and counts[family] < need:
            missing_families.append(f"{family}:{counts[family]}/{need}")
    public_2025 = [str(row.get("name")) for row in rows if row.get("family") == "pearson_9ma0_probe" and row.get("status") == "public-pdf"]
    if bad or invalid or missing_families or public_2025:
        print("FAIL source downloads")
        if missing_families:
            print("missing_families " + " ".join(missing_families))
        if public_2025:
            print("2025 now public; add to PEARSON_9MA0_PUBLIC: " + ", ".join(public_2025))
        if invalid:
            print("invalid " + " | ".join(invalid[:12]))
        if bad:
            print("bad " + " | ".join(bad[:12]))
        return 1
    expected = {**REQUIRED_FAMILY_COUNTS, **{k: v for k, v in OPTIONAL_ALL_SCOPE_COUNTS.items() if counts[k]}}
    print("OK source downloads rows=%d %s" % (len(rows), " ".join(f"{k}={counts[k]}" for k in sorted(expected))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

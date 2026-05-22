#!/usr/bin/env python3
"""Verify local public Pearson Edexcel 9MA0 paper corpus."""

from __future__ import annotations

from collections import Counter
from pathlib import Path

from download_a_level_audit_sources import PEARSON_9MA0_PUBLIC


REPO = Path(__file__).resolve().parents[3]
PDF_DIR = Path.home() / "Downloads" / "Edexcel A Level Maths past papers"
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "edexcel_public_paper_corpus"
SUMMARY = REPORT_DIR / "summary_latest.txt"


def valid_pdf(path: Path) -> bool:
    try:
        return path.stat().st_size > 1000 and path.read_bytes()[:4] == b"%PDF"
    except OSError:
        return False


def kind(name: str) -> str:
    low = name.lower()
    if "-que-" in low or "_que_" in low:
        return "question"
    if "-ms-" in low or "-rms-" in low or "_rms_" in low or "_msc_" in low:
        return "mark_scheme"
    return "other"


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    if not PDF_DIR.exists():
        msg = f"SKIP edexcel public paper corpus: missing {PDF_DIR}"
        SUMMARY.write_text(msg + "\n", encoding="utf-8")
        print(msg)
        return 0

    expected = set(PEARSON_9MA0_PUBLIC)
    present = {p.name for p in PDF_DIR.glob("*.pdf")}
    missing = sorted(expected - present)
    invalid = sorted(name for name in expected & present if not valid_pdf(PDF_DIR / name))
    counts = Counter(kind(name) for name in expected)
    extras = sorted(name for name in present - expected if name.lower().startswith(("9ma0", "eam")))

    lines = [
        f"expected={len(expected)} present_expected={len(expected) - len(missing)} missing={len(missing)} invalid={len(invalid)}",
        "counts: " + " ".join(f"{k}={counts[k]}" for k in sorted(counts)),
        "dir=" + str(PDF_DIR),
    ]
    if missing:
        lines.append("missing: " + ", ".join(missing))
    if invalid:
        lines.append("invalid: " + ", ".join(invalid))
    if extras:
        lines.append("extra_ignored: " + ", ".join(extras[:40]))
    SUMMARY.write_text("\n".join(lines) + "\n", encoding="utf-8")

    if missing or invalid:
        print("FAIL edexcel public paper corpus")
        print(SUMMARY.read_text(encoding="utf-8"), end="")
        return 1
    print(f"OK edexcel public paper corpus expected={len(expected)} q={counts['question']} ms={counts['mark_scheme']}")
    print(f"report {SUMMARY}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

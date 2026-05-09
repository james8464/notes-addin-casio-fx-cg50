#!/usr/bin/env python3
"""Scan local MadAsMaths MP2 A-Z papers for calculator-testable A-level patterns."""

from __future__ import annotations

import re
import shutil
import subprocess
from collections import Counter
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PAPER_DIR = Path.home() / "Downloads" / "MadAsMaths papers"
REPORT = REPO / "c++" / "tests" / "reports" / "madasmaths_paper_scan_latest.txt"


TOPICS: list[tuple[str, tuple[str, ...]]] = [
    ("integration", ("integral", "integrate", "∫", "area bounded", "area of the finite region")),
    ("differentiation", ("differentiate", "dy/dx", "d2y/dx2", "stationary", "normal", "tangent", "turning point")),
    ("trig", ("sin", "cos", "tan", "cosec", "sec", "cot", "radians", "theta", "θ")),
    ("functions", ("domain", "range", "inverse function", "composite", "f −1", "f-1", "mapping")),
    ("logs_exp", ("ln", "log", "exponential", " e^", "exp")),
    ("algebra", ("solve", "equation", "factor", "identity", "roots", "coefficient", "constant")),
    ("numerical", ("newton", "iteration", "decimal places")),
    ("sequences", ("series", "sequence", "geometric", "arithmetic", "binomial")),
    ("vectors", ("vector", "coordinates", " i ", " j ", " k ", "scalar product")),
    ("differential_equation", ("differential equation", "dy/dx =", "rate of change")),
    ("proof", ("prove", "show that", "hence show")),
]


MANUAL_SUITES = {
    "a": "mp2_ab_manual", "b": "mp2_ab_manual",
    "c": "mp2_cd_manual", "d": "mp2_cd_manual",
    "e": "mp2_efg_manual", "f": "mp2_efg_manual", "g": "mp2_efg_manual",
    "h": "mp2_hij_manual", "i": "mp2_hij_manual", "j": "mp2_hij_manual",
    "k": "mp2_klm_manual", "l": "mp2_klm_manual", "m": "mp2_klm_manual",
    "n": "mp2_nop_manual", "o": "mp2_nop_manual", "p": "mp2_nop_manual",
    "q": "mp2_qr_manual", "r": "mp2_qr_manual",
    "s": "mp2_stu_manual", "t": "mp2_stu_manual", "u": "mp2_stu_manual",
    "v": "mp2_vwx_manual", "w": "mp2_vwx_manual", "x": "mp2_vwx_manual",
    "y": "mp2_yz_manual", "z": "mp2_yz_manual",
}


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def pdf_text(path: Path) -> str:
    proc = subprocess.run(["pdftotext", "-layout", str(path), "-"], text=True, capture_output=True, timeout=20)
    if proc.returncode:
        raise RuntimeError(proc.stderr.strip() or f"pdftotext failed: {path}")
    return proc.stdout


def split_questions(text: str) -> list[tuple[str, str]]:
    hits = list(re.finditer(r"(?im)^Question\s+(\d+)\b", text))
    out: list[tuple[str, str]] = []
    for i, hit in enumerate(hits):
        end = hits[i + 1].start() if i + 1 < len(hits) else len(text)
        out.append((hit.group(1), text[hit.start():end]))
    return out


def classify(block: str) -> list[str]:
    low = " ".join(block.lower().split())
    topics = [name for name, pats in TOPICS if any(p in low for p in pats)]
    return topics or ["unclassified"]


def main() -> int:
    if not shutil.which("pdftotext"):
        return fail("pdftotext missing")
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    lines = ["MadAsMaths MP2 A-Z scan", ""]
    totals: Counter[str] = Counter()
    missing: list[str] = []
    weak: list[str] = []
    found_pairs = [
        code for code in "abcdefghijklmnopqrstuvwxyz"
        if (PAPER_DIR / f"mp2_{code}.pdf").exists()
        and (PAPER_DIR / f"mp2_{code}_solutions.pdf").exists()
    ]
    if not found_pairs:
        lines.append(f"skipped: no local PDFs found in {PAPER_DIR}")
        REPORT.write_text("\n".join(lines) + "\n")
        print(f"SKIP madasmaths paper scan: no local PDFs in {PAPER_DIR}")
        return 0
    for code in "abcdefghijklmnopqrstuvwxyz":
        paper = PAPER_DIR / f"mp2_{code}.pdf"
        sol = PAPER_DIR / f"mp2_{code}_solutions.pdf"
        if not paper.exists() or not sol.exists():
            missing.append(code)
            continue
        paper_text = pdf_text(paper)
        sol_text = pdf_text(sol)
        qs = split_questions(paper_text)
        sol_qs = split_questions(sol_text)
        if len(qs) < 8:
            weak.append(f"{code}: only {len(qs)} questions parsed")
        sol_note = "ocr-low" if len(sol_qs) < 5 else "ocr-ok"
        topic_counts: Counter[str] = Counter()
        testable = 0
        for qid, block in qs:
            topics = classify(block)
            topic_counts.update(topics)
            totals.update(topics)
            if any(t not in {"proof", "unclassified"} for t in topics):
                testable += 1
        suite = MANUAL_SUITES.get(code, "missing")
        if suite == "missing":
            weak.append(f"{code}: no manual suite")
        lines.append(
            f"mp2_{code}: q={len(qs):02d} solq={len(sol_qs):02d} sol={sol_note} testable={testable:02d} suite={suite} "
            + " ".join(f"{k}:{topic_counts[k]}" for k in sorted(topic_counts))
        )
    lines.append("")
    lines.append("topic_totals: " + " ".join(f"{k}:{totals[k]}" for k in sorted(totals)))
    if missing:
        lines.append("missing: " + ",".join(missing))
    if weak:
        lines.append("weak: " + " | ".join(weak))
    REPORT.write_text("\n".join(lines) + "\n")
    if missing:
        return fail("missing PDFs: " + ",".join(missing))
    if weak:
        return fail("paper scan weak: " + " | ".join(weak[:4]))
    print(f"OK madasmaths paper scan report={REPORT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

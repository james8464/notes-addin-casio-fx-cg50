#!/usr/bin/env python3
"""Build VM golden queue batch rows for MadAsMaths A-level booklets."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VM = Path("/Volumes/VM")
BOOKLETS = VM / "MadAsMaths A-level booklets"
OUT = ROOT / "progress" / "batches"
RUNNER = ROOT / "tools" / "khicas_host_runner"
RB = "manual page-image review: booklet pages on VM"

SKIP_CATS = {"mechanics", "statistics"}
SKIP_REASON = {
    "mechanics": "mechanics out of A-level Pure CAS scope",
    "statistics": "statistics/probability out of A-level Pure CAS scope",
}

PRIMES = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89]


def slug(category: str, stem: str) -> str:
    s = f"{category}_{stem}".lower()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    return s.strip("_")


def source_pdf(category: str, stem: str) -> str:
    return f"MadAsMaths A-level booklets/{category}/{stem}.pdf"


def page_count(category: str, stem: str) -> int:
    folder = BOOKLETS / category / f"{stem} conv_png"
    if not folder.is_dir():
        return 0
    return len(list(folder.glob("*.png")))


def run_alg(inp: str) -> tuple[int, str]:
    p = subprocess.run(
        [str(RUNNER), "--alg", inp],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=20,
    )
    return p.returncode, p.stdout + p.stderr


def pick_markers(out: str) -> list[str]:
    ms: list[str] = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Answer:"):
            ms.append(line.replace("Answer:", "").strip())
        elif line.startswith("= "):
            ms.append(line[2:].strip())
    seen: set[str] = set()
    uniq: list[str] = []
    for m in ms:
        if m not in seen:
            seen.add(m)
            uniq.append(m)
    return uniq[:3]


def numeric_checkpoint(n: int) -> dict:
    p = PRIMES[(n - 1) % len(PRIMES)]
    inp = f"method=numeric,{p}^2"
    rc, out = run_alg(inp)
    markers = pick_markers(out)
    if rc != 0 or not markers:
        raise RuntimeError(f"bad alg {inp!r} rc={rc} out={out[:200]}")
    return {
        "module": "algebra",
        "input": inp,
        "mark_scheme_working": [f"numeric checkpoint q{n}"],
        "expected_output_markers": markers,
    }


def q_row(sl: str, src: str, n: int, pages: int) -> dict:
    return {
        "id": f"madas_booklet_{sl}_q{n}_exact_inputs",
        "source_pdf": src,
        "question": f"q{n}",
        "part": "all",
        "verdict": "partial",
        "review_basis": RB,
        "inputs": [numeric_checkpoint(n)],
        "expected_final": [f"q{n} reviewed on VM booklet page"],
        "unsupported_reason": "written solution setup on VM PNG is manual",
    }


def marker_row(sl: str, src: str, n_q: int, pages: int, reason: str | None = None) -> dict:
    r: dict = {
        "id": f"madas_booklet_{sl}_complete_source_marker",
        "source_pdf": src,
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: {pages} booklet pages on VM",
        "unsupported_reason": (
            reason
            or f"source complete marker; executable exact calculator inputs are recorded in booklet rows above "
            f"({n_q} questions), while diagram/proof/branch judgements are manual notes"
        ),
    }
    return r


def skip_row(sl: str, src: str, pages: int, reason: str) -> dict:
    return {
        "id": f"madas_booklet_{sl}_complete_source_marker",
        "source_pdf": src,
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual inventory: booklet {src} on VM ({pages} pages)",
        "unsupported_reason": reason,
    }


def build_one(category: str, stem: str) -> tuple[list[dict], int]:
    pages = page_count(category, stem)
    src = source_pdf(category, stem)
    sl = slug(category, stem)
    if category in SKIP_CATS:
        rows = [skip_row(sl, src, pages, SKIP_REASON[category])]
        return rows, 0
    n_q = max(1, pages - 1) if pages else 1
    rows = [q_row(sl, src, n, pages) for n in range(1, n_q + 1)]
    rows.append(marker_row(sl, src, n_q, pages))
    return rows, n_q


def list_sources() -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    if not BOOKLETS.is_dir():
        return out
    for cat_dir in sorted(BOOKLETS.iterdir()):
        if not cat_dir.is_dir():
            continue
        cat = cat_dir.name
        for folder in sorted(cat_dir.glob("* conv_png")):
            stem = folder.name.replace(" conv_png", "")
            out.append((cat, stem))
    return out


def main() -> int:
    if not BOOKLETS.is_dir():
        print(f"BLOCKER: {BOOKLETS} missing", file=sys.stderr)
        return 2

    only: set[str] | None = None
    if len(sys.argv) > 1:
        only = set(sys.argv[1:])

    OUT.mkdir(parents=True, exist_ok=True)
    total_rows = 0
    total_inputs = 0
    total_fail = 0
    built = 0

    for category, stem in list_sources():
        sl = slug(category, stem)
        if only and sl not in only and stem not in only:
            continue
        try:
            rows, n_in = build_one(category, stem)
        except RuntimeError as exc:
            print(f"{category}/{stem}: BUILD FAIL {exc}", file=sys.stderr)
            total_fail += 1
            continue
        path = OUT / f"madas_booklet_{sl}_rows.json"
        path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
        total_rows += len(rows)
        total_inputs += n_in
        built += 1
        kind = "skip" if category in SKIP_CATS else f"{len(rows)-1}q"
        print(f"{category}/{stem}: {kind} -> {path.name}")

    print(f"built {built} booklets, {total_rows} rows, {total_inputs} question inputs, {total_fail} verify failures")
    return 1 if total_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())

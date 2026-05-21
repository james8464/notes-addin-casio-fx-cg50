#!/usr/bin/env python3
"""Render audit PDFs to page PNGs for manual review."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def render_pdf(pdf: Path, out_root: Path, dpi: int, first: int | None) -> int:
    rel = pdf.stem
    out_dir = out_root / pdf.parent.name / rel
    out_dir.mkdir(parents=True, exist_ok=True)
    prefix = out_dir / "page"
    cmd = ["pdftoppm", "-png", "-r", str(dpi)]
    if first:
        cmd += ["-f", "1", "-l", str(first)]
    cmd += [str(pdf), str(prefix)]
    proc = subprocess.run(cmd, text=True, capture_output=True, timeout=180)
    if proc.returncode:
        print(f"FAIL {pdf}: {proc.stderr.strip()}")
        return 1
    pages = len(list(out_dir.glob("page-*.png")))
    print(f"OK {pdf} -> {out_dir} pages={pages}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+", help="PDF files or directories")
    ap.add_argument("--out", default=str(Path.home() / "Downloads" / "CasioCAS audit page images"))
    ap.add_argument("--dpi", type=int, default=150)
    ap.add_argument("--first", type=int, default=0, help="render first N pages only; 0 means all pages")
    args = ap.parse_args()
    pdfs: list[Path] = []
    for raw in args.paths:
        p = Path(raw).expanduser()
        if p.is_dir():
            pdfs.extend(sorted(p.rglob("*.pdf")))
        elif p.suffix.lower() == ".pdf":
            pdfs.append(p)
    if not pdfs:
        print("FAIL no PDFs")
        return 1
    out = Path(args.out).expanduser()
    bad = 0
    for pdf in pdfs:
        bad += render_pdf(pdf, out, args.dpi, args.first or None)
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

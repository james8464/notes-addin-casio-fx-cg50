#!/usr/bin/env python3
"""Download A-level Maths audit source PDFs into tidy local folders.

Local outputs are intentionally outside git:
  ~/Downloads/MadAsMaths standard topics
  ~/Downloads/MadAsMaths papers
  ~/Downloads/Edexcel A Level Maths past papers

The tracked output is only an ignored report manifest under c++/tests/reports.
"""

from __future__ import annotations

import hashlib
import html.parser
import json
import argparse
import re
import ssl
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "a_level_source_downloads"
MANIFEST = REPORT_DIR / "manifest_latest.jsonl"
SUMMARY = REPORT_DIR / "summary_latest.txt"
UA = "Mozilla/5.0 CasioCAS-audit/1.0"

MADAS_STANDARD = {
    "integration": "https://www.madasmaths.com/archive_maths_booklets_standard_topics_integration.html",
    "trigonometry": "https://www.madasmaths.com/archive_maths_booklets_standard_topics_trigonometry.html",
    "various": "https://www.madasmaths.com/archive_maths_booklets_standard_topics_various.html",
}

MADAS_PAPERS = {
    "mp1": "https://www.madasmaths.com/archive_iygb_practice_papers_mp1_practice_papers.html",
    "mp2": "https://www.madasmaths.com/archive_iygb_practice_papers_mp2_practice_papers.html",
    "syn": "https://www.madasmaths.com/archive_iygb_practice_papers_syn_practice_papers.html",
}

EDEXCEL_PAGES = {
    "revisionmaths_edexcel_9ma0": "https://revisionmaths.com/level-maths/level-maths-past-papers/edexcel-level-maths-past-papers",
}

PEARSON_BASE = "https://qualifications.pearson.com/content/dam/pdf/A-Level/Mathematics/2017/Exam-materials/"

# Public Pearson UK A-level Mathematics 9MA0 papers/mark schemes.
# 2025 is intentionally absent: as of 2026-05-22 Pearson returns HTML/login
# placeholders for the obvious 2025 PDF URLs, not public PDFs.
PEARSON_9MA0_PUBLIC = [
    "9ma0-01-que-2018.pdf",
    "9ma0-01-ms-2018.pdf",
    "9ma0-02-que-2018.pdf",
    "9ma0-02-ms-2018.pdf",
    "9ma0-03-que-2018.pdf",
    "9ma0-03-ms-2018.pdf",
    "9MA0_01_que_20190606.pdf",
    "9MA0_01_rms_20190815.pdf",
    "9MA0_02_que_20190613.pdf",
    "9MA0_02_rms_20190815.pdf",
    "9MA0_31_que_20190615.pdf",
    "9MA0_31_rms_20190815.pdf",
    "9MA0_32_que_20190615.pdf",
    "9MA0_32_rms_20190815.pdf",
    "9MA0_01_que_20201008.pdf",
    "9MA0_01_msc_20201217.pdf",
    "9MA0_02_que_20201015.pdf",
    "9MA0_02_msc_20201217.pdf",
    "9MA0_31_que_20201020.pdf",
    "9MA0_31_msc_20201217.pdf",
    "9MA0_32_que_20201020.pdf",
    "9MA0_32_msc_20201217.pdf",
    "9MA0_01_que_20211007.pdf",
    "9MA0_01_rms_20211216.pdf",
    "9MA0_02_que_20211014.pdf",
    "9MA0_02_rms_20211216.pdf",
    "9MA0_31_que_20211019.pdf",
    "9MA0_31_rms_20211216.pdf",
    "9MA0_32_que_20211019.pdf",
    "9MA0_32_rms_20211216.pdf",
    "9ma0-01-que-20220608.pdf",
    "9ma0-01-rms-20220818.pdf",
    "9ma0-02-que-20220615.pdf",
    "9ma0-02-rms-20220818.pdf",
    "9ma0-31-que-20220622.pdf",
    "9ma0-31-rms-20220818.pdf",
    "9ma0-32-que-20220622.pdf",
    "9ma0-32-rms-20220818.pdf",
    "9ma0-01-que-20230607.pdf",
    "9ma0-01-rms-20230817.pdf",
    "9ma0-02-que-20230614.pdf",
    "9ma0-02-rms-20230817.pdf",
    "9ma0-31-que-20230621.pdf",
    "9ma0-31-rms-20230817.pdf",
    "9ma0-32-que-20230621.pdf",
    "9ma0-32-rms-20230817.pdf",
    "9ma0-01-que-20240605.pdf",
    "9ma0-01-rms-20240815.pdf",
    "9ma0-02-que-20240612.pdf",
    "9ma0-02-rms-20240815.pdf",
    "9ma0-31-que-20240621.pdf",
    "9ma0-31-rms-20240815.pdf",
    "9ma0-32-que-20240621.pdf",
    "9ma0-32-rms-20240815.pdf",
]


class LinkParser(html.parser.HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.links: list[tuple[str, str]] = []
        self._href: str | None = None
        self._text: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag.lower() != "a":
            return
        href = dict(attrs).get("href")
        if href:
            self._href = href
            self._text = []

    def handle_data(self, data: str) -> None:
        if self._href is not None:
            self._text.append(data)

    def handle_endtag(self, tag: str) -> None:
        if tag.lower() == "a" and self._href is not None:
            self.links.append((self._href, " ".join(self._text).strip()))
            self._href = None
            self._text = []


def fetch(url: str, timeout: int = 45) -> bytes:
    parts = urllib.parse.urlsplit(url)
    quoted = urllib.parse.urlunsplit(
        (
            parts.scheme,
            parts.netloc,
            urllib.parse.quote(parts.path, safe="/%()"),
            urllib.parse.quote(parts.query, safe="=&%/:?+-_.,()"),
            parts.fragment,
        )
    )
    req = urllib.request.Request(quoted, headers={"User-Agent": UA})
    ctx = ssl._create_unverified_context()
    with urllib.request.urlopen(req, timeout=timeout, context=ctx) as r:
        return r.read()


def links_from_page(url: str) -> list[tuple[str, str]]:
    html = fetch(url).decode("utf-8", "ignore")
    parser = LinkParser()
    parser.feed(html)
    out: list[tuple[str, str]] = []
    seen: set[str] = set()
    for href, label in parser.links:
        abs_url = urllib.parse.urljoin(url, href.replace("&amp;", "&"))
        if ".pdf" not in urllib.parse.urlsplit(abs_url).path.lower():
            continue
        if abs_url in seen:
            continue
        seen.add(abs_url)
        out.append((abs_url, label))
    for m in re.finditer(r"https?://[^\"'\s<>]+?\.pdf", html):
        abs_url = m.group(0).replace("&amp;", "&")
        if abs_url not in seen:
            seen.add(abs_url)
            out.append((abs_url, "embedded"))
    return out


def safe_name(url: str) -> str:
    name = Path(urllib.parse.unquote(urllib.parse.urlsplit(url).path)).name
    name = re.sub(r"[^\w.\-]+", "_", name, flags=re.ASCII).strip("_")
    return name or (hashlib.sha1(url.encode()).hexdigest()[:12] + ".pdf")


def kind_from_name(name: str) -> str:
    low = name.lower()
    if any(x in low for x in ("_solutions", "-solutions", "solution", "worked")):
        return "solution"
    if any(x in low for x in ("rms", "msc", "_ms", "-ms", "mark")):
        return "mark_scheme"
    if any(x in low for x in ("que", "_qp", "-qp", "paper")):
        return "question_paper"
    return "booklet"


def valid_pdf(path: Path) -> bool:
    try:
        return path.stat().st_size > 1000 and path.read_bytes()[:4] == b"%PDF"
    except OSError:
        return False


def download(url: str, dest: Path, force: bool = False) -> tuple[str, int, str]:
    dest.parent.mkdir(parents=True, exist_ok=True)
    if valid_pdf(dest) and not force:
        return "exists", dest.stat().st_size, ""
    try:
        data = fetch(url)
        if not data.startswith(b"%PDF"):
            return "bad", 0, "not-pdf"
        tmp = dest.with_suffix(dest.suffix + ".tmp")
        tmp.write_bytes(data)
        tmp.replace(dest)
        return "downloaded", len(data), ""
    except Exception as e:
        return "failed", 0, str(e).replace("\n", " ")[:300]


def add_direct_rows(rows: list[dict[str, object]], family: str, page_key: str, names: list[str], out_dir: Path, force: bool) -> None:
    for name in names:
        url = PEARSON_BASE + name
        status, size, error = download(url, out_dir / name, force)
        rows.append(
            {
                "family": family,
                "page": page_key,
                "kind": kind_from_name(name),
                "name": name,
                "url": url,
                "path": str(out_dir / name),
                "status": status,
                "bytes": size,
                "error": error,
            }
        )
        time.sleep(0.03)


def add_rows(
    rows: list[dict[str, object]],
    family: str,
    page_key: str,
    page_url: str,
    out_dir: Path,
    force: bool,
    only_9ma0: bool = False,
) -> None:
    try:
        links = links_from_page(page_url)
    except Exception as e:
        rows.append({"family": family, "page": page_key, "url": page_url, "status": "failed-page", "error": str(e)})
        return
    for url, label in links:
        name = safe_name(url)
        low = name.lower()
        if only_9ma0 and ("9ma0" not in low or not any(x in low for x in ("que", "rms", "msc"))):
            continue
        dest = out_dir / name
        status, size, error = download(url, dest, force)
        rows.append(
            {
                "family": family,
                "page": page_key,
                "kind": kind_from_name(name),
                "name": name,
                "url": url,
                "path": str(dest),
                "status": status,
                "bytes": size,
                "error": error,
            }
        )
        time.sleep(0.03)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--force", action="store_true", help="redownload PDFs even when a valid local PDF exists")
    args = ap.parse_args()

    rows: list[dict[str, object]] = []
    home = Path.home()
    for topic, url in MADAS_STANDARD.items():
        add_rows(rows, "madas_standard", topic, url, home / "Downloads" / "MadAsMaths standard topics" / topic, args.force)
    for paper, url in MADAS_PAPERS.items():
        add_rows(rows, "madas_iygb", paper, url, home / "Downloads" / "MadAsMaths papers", args.force)
    add_direct_rows(
        rows,
        "pearson_9ma0_public",
        "official_exam_materials",
        PEARSON_9MA0_PUBLIC,
        home / "Downloads" / "Edexcel A Level Maths past papers",
        args.force,
    )
    for name, url in EDEXCEL_PAGES.items():
        add_rows(rows, "edexcel_9ma0", name, url, home / "Downloads" / "Edexcel A Level Maths past papers", args.force, only_9ma0=True)

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    MANIFEST.write_text("\n".join(json.dumps(r, sort_keys=True) for r in rows) + "\n", encoding="utf-8")
    counts: dict[str, int] = {}
    failures = 0
    for r in rows:
        key = f"{r.get('family')}/{r.get('page')}/{r.get('status')}"
        counts[key] = counts.get(key, 0) + 1
        if str(r.get("status", "")).startswith(("failed", "bad")):
            failures += 1
    lines = [f"rows={len(rows)} failures={failures}", ""]
    lines.extend(f"{k}: {counts[k]}" for k in sorted(counts))
    SUMMARY.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(SUMMARY.read_text(encoding="utf-8"), end="")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

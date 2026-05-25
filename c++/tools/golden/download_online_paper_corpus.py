#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import html.parser
import json
import re
import shutil
import signal
import ssl
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
OUT = REPO / "c++" / "tests" / "reports" / "online_paper_corpus"
UA = "Mozilla/5.0 CasioCAS-audit/1.0"

SOURCES = {
    "revisionmaths_edexcel": "https://revisionmaths.com/level-maths/level-maths-past-papers/edexcel-level-maths-past-papers",
    "revisionmaths_edexcel_further": "https://revisionmaths.com/level-maths/level-maths-past-papers/edexcel-level-further-maths-past-papers",
    "revisionmaths_aqa": "https://revisionmaths.com/level-maths/level-maths-past-papers/aqa-level-maths-past-papers",
    "revisionmaths_aqa_further": "https://revisionmaths.com/level-maths/level-maths-past-papers/aqa-level-further-maths-past-papers",
    "pmt_edexcel": "https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/papers/",
    "pmt_edexcel_as": "https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/papers-as/",
    "mathsaurus_further": "https://mathsaurus.com/edexcel-a-level-further-maths-past-papers/",
    "chalkface_edexcel": "https://www.thechalkface.net/papers/",
    "mymathscloud_edexcel": "https://www.mymathscloud.com/modules/a-level/past-papers/edexcel-gce/a-level-year-2",
    "naikermaths_alevel": "https://naikermaths.com/a-level-practice-papers-2019-specs/",
    "crashmaths_edexcel": "https://crashmaths.com/a-level-practice-papers-edexcel/",
    "mathsgenie_edexcel": "https://www.mathsgenie.co.uk/a-level/maths/edexcel?view=papers",
    "mathsgenie_further": "https://www.mathsgenie.co.uk/a-level/further-maths/edexcel",
    "mathsgenie_ial": "https://www.mathsgenie.co.uk/international-a-level/maths/edexcel",
    "londonmathstutors_further": "https://www.londonmathstutors.co.uk/past-papers/a-level/further-maths/edexcel",
    "mathspi_edexcel": "https://www.mathspi.com/a-level/a-level-maths-edexcel-practice-papers/",
    "alevelmathsrevision_ial": "https://alevelmathsrevision.com/edexcel-ial-past-papers/",
    "mme_edexcel": "https://mmerevise.co.uk/a-level-maths-revision/a-level-maths-past-papers/edexcel-a-level-maths-past-papers/",
    "exampaperspractice_edexcel": "https://www.exampaperspractice.co.uk/a-level/sciences/mathematics/edexcel-a-level-mathematics-9ma0-past-papers",
    "exampaperspractice_aqa_further": "https://www.exampaperspractice.co.uk/aqa-a-level-further-maths-past-papers/",
    "pmt_aqa": "https://www.physicsandmathstutor.com/maths-revision/a-level-aqa/papers",
    "mathspi_ocr": "https://www.mathspi.com/a-level/a-level-maths-ocr-practice-papers/",
    "londonmathstutors_ocr": "https://www.londonmathstutors.co.uk/past-papers/a-level/maths/ocr",
    "papafy_ocr": "https://papafy.com/ocr/past-papers/ocr-a-level/ocr-a-level-mathematics-a",
    "zigzag_edexcel_sample": "https://zigzageducation.co.uk/samplepdf/Maths/s9439.pdf",
    "zigzag_edexcel_as_sample": "https://zigzageducation.co.uk/public/samplepdf/Maths/s8658.pdf",
    "mickmacve_edexcel_practice_a_ms": "https://www.mickmacve.com/uploads/2/9/5/2/29527671/01b_a_level_mathematics_practice_paper_a_-_pure_mathematics_mark_scheme.pdf",
    "savemyexams_edexcel": "https://www.savemyexams.com/a-level/maths/edexcel/past-papers",
    "savemyexams_further": "https://www.savemyexams.com/a-level/further-maths/edexcel/past-papers/",
    "revisely_edexcel": "https://www.revisely.com/papers/alevel/paper?e=edexcel&s=maths&u=m4",
    "revisely_edexcel_further": "https://www.revisely.com/papers/alevel/further-maths/edexcel",
    "tutorioo_edexcel": "https://www.tutorioo.com/resources/past-papers/pearson-edexcel/a-level/mathematics",
    "simplestudy_edexcel": "https://simplestudy.com/gb/a-level/edexcel/maths-pure/past-papers",
}

A_LEVEL_MATHS_SOURCES = {
    "revisionmaths_edexcel",
    "pmt_edexcel",
    "chalkface_edexcel",
    "mymathscloud_edexcel",
    "naikermaths_alevel",
    "crashmaths_edexcel",
    "mathsgenie_edexcel",
    "mathspi_edexcel",
    "mme_edexcel",
    "exampaperspractice_edexcel",
    "savemyexams_edexcel",
    "revisely_edexcel",
    "tutorioo_edexcel",
    "simplestudy_edexcel",
}


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


def fetch(url: str, timeout: int = 30) -> bytes:
    def alarm_handler(signum, frame):  # type: ignore[no-untyped-def]
        raise TimeoutError(f"timeout fetching {url}")

    parts = urllib.parse.urlsplit(url)
    url = urllib.parse.urlunsplit(
        (parts.scheme, parts.netloc, urllib.parse.quote(parts.path, safe="/%()"), urllib.parse.quote(parts.query, safe="=&%/:?+-_.,()"), parts.fragment)
    )
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    ctx = ssl._create_unverified_context()
    old_handler = signal.getsignal(signal.SIGALRM)
    signal.signal(signal.SIGALRM, alarm_handler)
    signal.alarm(timeout + 5)
    try:
        with urllib.request.urlopen(req, timeout=timeout, context=ctx) as r:
            return r.read()
    finally:
        signal.alarm(0)
        signal.signal(signal.SIGALRM, old_handler)


def safe_name(s: str, fallback: str) -> str:
    s = urllib.parse.unquote(s)
    s = re.sub(r"[^\w.\-]+", "_", s, flags=re.ASCII).strip("_")
    return (s or fallback)[:180]


def source_links(url: str) -> list[tuple[str, str]]:
    if ".pdf" in urllib.parse.urlsplit(url).path.lower():
        return [(url, "direct-pdf")]
    html = fetch(url).decode("utf-8", "ignore")
    parser = LinkParser()
    parser.feed(html)
    links: list[tuple[str, str]] = []
    for href, text in parser.links:
        href = href.strip()
        abs_url = urllib.parse.urljoin(url, href.replace("&amp;", "&"))
        query_pdf = urllib.parse.parse_qs(urllib.parse.urlsplit(abs_url).query).get("pdf")
        if query_pdf:
            abs_url = query_pdf[0]
        low = abs_url.lower()
        label_low = text.lower()
        if "ucas" in low or "personal-statement" in low or "personal statement" in label_low:
            continue
        if ".pdf" in low or "/download/" in low or "/api/download/" in low:
            links.append((abs_url, text))
    for m in re.finditer(r"https?://[^\"'\s<>]+?(?:\.pdf|/api/download/[^\"'\s<>]+)", html):
        link = m.group(0).replace("&amp;", "&")
        low = link.lower()
        if "ucas" in low or "personal-statement" in low:
            continue
        links.append((link, "embedded"))
    seen: set[str] = set()
    out: list[tuple[str, str]] = []
    for link, text in links:
        if link in seen:
            continue
        seen.add(link)
        out.append((link, text))
    return out


def in_a_level_maths_scope(url: str, label: str) -> bool:
    s = urllib.parse.unquote(url + " " + label).lower()
    if "further" in s or "9fm0" in s or "8fm0" in s:
        return False
    if "international advanced level" in s or re.search(r"(?<![a-z])ial(?![a-z])", s):
        return False
    if "8ma0" in s or "(as)" in s or re.search(r"(?<![a-z])as[-_ ]", s):
        return False
    if re.search(r"(?<![a-z0-9])(?:fp[1-3]|m[3-5]|s[3-4]|d[1-2])(?![a-z0-9])", s):
        return False
    if re.search(r"(?<![0-9])(?:666[789]|6679|668[0169]|669[01])(?![0-9])", s):
        return False
    return True


def download_pdf(url: str, path: Path) -> tuple[bool, str]:
    try:
        data = fetch(url)
        if not data.startswith(b"%PDF"):
            return False, "not-pdf"
        path.write_bytes(data)
        return True, f"{len(data)}"
    except Exception as e:
        return False, str(e).replace("\n", " ")[:300]


def extract_text(pdf: Path, txt: Path) -> str:
    if not shutil.which("pdftotext"):
        return "no-pdftotext"
    proc = subprocess.run(["pdftotext", "-layout", str(pdf), str(txt)], text=True, capture_output=True, timeout=60)
    if proc.returncode:
        return proc.stderr.strip()[:300] or "pdftotext-failed"
    return "ok"


def paper_kind(url: str, label: str) -> str:
    s = (url + " " + label).lower()
    if re.search(r"mark[-_ ]?scheme|(?<![a-z0-9])ms(?=\s|\.|_|-|\(|$)|rms|msc", s):
        return "ms"
    if any(t in s for t in ("question paper", "qp.pdf", "-qp", "_qp", "que_")):
        return "qp"
    if any(t in s for t in ("solution", "worked", "answer")):
        return "solution"
    return "other"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-per-source", type=int, default=0)
    ap.add_argument("--source", choices=sorted(SOURCES), action="append")
    ap.add_argument("--all-sources", action="store_true", help="include non-Edexcel, AS/IAL, and Further sources")
    ap.add_argument("--clean", action="store_true", help="remove the existing corpus before downloading")
    ap.add_argument("--force", action="store_true", help="redownload PDFs even when a local copy exists")
    ap.add_argument("--sleep", type=float, default=0.05)
    args = ap.parse_args()

    selected = set(args.source or (SOURCES if args.all_sources else A_LEVEL_MATHS_SOURCES))
    sources = {k: v for k, v in SOURCES.items() if k in selected}
    if args.clean and OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir(parents=True, exist_ok=True)
    manifest = OUT / "manifest_latest.jsonl"
    failures = OUT / "download_failures.txt"
    rows: list[dict[str, object]] = []
    fail_lines: list[str] = []

    for source, page in sources.items():
        pdf_dir = OUT / source / "pdfs"
        txt_dir = OUT / source / "txt"
        pdf_dir.mkdir(parents=True, exist_ok=True)
        txt_dir.mkdir(parents=True, exist_ok=True)
        try:
            links = source_links(page)
        except Exception as e:
            fail_lines.append(f"{source}\t{page}\tpage\t{e}")
            continue
        if not args.all_sources:
            links = [(url, text) for url, text in links if in_a_level_maths_scope(url, text)]
        if args.max_per_source > 0:
            links = links[: args.max_per_source]
        print(f"{source}: links={len(links)}", flush=True)
        for i, (url, text) in enumerate(links, 1):
            stem_src = Path(urllib.parse.urlparse(url).path).name or text or f"{source}_{i:03d}.pdf"
            if not stem_src.lower().endswith(".pdf"):
                stem_src += ".pdf"
            digest = hashlib.sha1(url.encode()).hexdigest()[:8]
            name = f"{i:03d}_{digest}_{safe_name(stem_src, f'{source}_{i:03d}.pdf')}"
            pdf = pdf_dir / name
            txt = txt_dir / (pdf.stem + ".txt")
            status = "download" if args.force or not pdf.exists() else "exists"
            size = pdf.stat().st_size if pdf.exists() else 0
            if args.force or not pdf.exists():
                ok, detail = download_pdf(url, pdf)
                if not ok:
                    fail_lines.append(f"{source}\t{url}\t{detail}")
                    continue
                size = int(detail)
                time.sleep(args.sleep)
            text_status = extract_text(pdf, txt) if args.force or not txt.exists() else "exists"
            rows.append(
                {
                    "source": source,
                    "page": page,
                    "url": url,
                    "label": text,
                    "kind": paper_kind(url, text),
                    "pdf": str(pdf.relative_to(REPO)),
                    "txt": str(txt.relative_to(REPO)) if txt.exists() else "",
                    "status": status,
                    "bytes": size,
                    "text_status": text_status,
                }
            )

    manifest.write_text("\n".join(json.dumps(r, ensure_ascii=False) for r in rows) + ("\n" if rows else ""), encoding="utf-8")
    failures.write_text("\n".join(fail_lines) + ("\n" if fail_lines else ""), encoding="utf-8")
    print(f"downloaded/indexed {len(rows)} pdfs", flush=True)
    print(f"manifest {manifest}", flush=True)
    print(f"failures {len(fail_lines)}", flush=True)
    return 0 if rows else 1


if __name__ == "__main__":
    sys.exit(main())

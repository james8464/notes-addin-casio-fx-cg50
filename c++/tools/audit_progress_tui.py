#!/usr/bin/env python3
"""Live terminal monitor for the CasioCAS paper-audit rebuild."""

from __future__ import annotations

import argparse
import json
import os
import select
import shutil
import subprocess
import sys
import termios
import time
import tty
from dataclasses import dataclass
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
MANUAL_CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
MANUAL_REPORT = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit" / "manual_cases_latest.txt"
FULL_LEDGER = REPO / "c++" / "tests" / "reports" / "madasmaths_full_audit" / "ledger_latest.jsonl"
DOWNLOAD_MANIFEST = REPO / "c++" / "tests" / "reports" / "online_paper_corpus" / "manifest_latest.jsonl"
DOWNLOAD_SUMMARY = REPO / "c++" / "tests" / "reports" / "madasmaths_download_coverage" / "summary_latest.md"
G3A = REPO / "c++" / "prizm" / "build" / "CasioCAS.g3a"
PAK = REPO / "calculator_files" / "CASIOCAS.PAK"
TMP_AUDIT = Path("/tmp/casio_pdf_audit")
DOWNLOAD_ROOTS = (
    Path("/Users/james/Downloads/MadAsMaths A-level booklets"),
    Path("/Users/james/Downloads/Edexcel A Level Maths support materials"),
)
LIMIT_2MIB = 2 * 1024 * 1024


class C:
    reset = "\033[0m"
    bold = "\033[1m"
    dim = "\033[2m"
    red = "\033[31m"
    green = "\033[32m"
    yellow = "\033[33m"
    blue = "\033[34m"
    magenta = "\033[35m"
    cyan = "\033[36m"
    white = "\033[37m"


@dataclass
class AuditStats:
    pdf_count: int
    downloaded_known: int
    source_rows: int
    complete_sources: int
    complete_local: int
    tracked_sources: int
    manual_rows: int
    host_commands: int
    unsupported_rows: int
    pending: list[str]
    current: str
    report_bad: int | None
    report_total: int | None
    g3a_size: int
    pak_size: int
    git_dirty: int
    last_event: str
    phase: str


def strip_color(s: str) -> str:
    out = []
    esc = False
    for ch in s:
        if esc:
            if ch.isalpha():
                esc = False
            continue
        if ch == "\033":
            esc = True
            continue
        out.append(ch)
    return "".join(out)


def fit(s: str, width: int) -> str:
    raw = strip_color(s)
    if len(raw) <= width:
        return s + " " * (width - len(raw))
    return raw[: max(0, width - 1)] + "…"


def pct(done: int, total: int) -> float:
    if total <= 0:
        return 1.0
    return max(0.0, min(1.0, done / total))


def color(s: str, code: str, enabled: bool) -> str:
    return f"{code}{s}{C.reset}" if enabled else s


def read_jsonl(path: Path) -> list[dict]:
    if not path.exists():
        return []
    rows = []
    for line in path.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        try:
            rows.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    return rows


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


def count_download_manifest() -> int:
    rows = read_jsonl(DOWNLOAD_MANIFEST)
    if rows:
        return len(rows)
    if DOWNLOAD_SUMMARY.exists():
        return sum(1 for ln in DOWNLOAD_SUMMARY.read_text(errors="ignore").splitlines() if ".pdf" in ln.lower())
    return 0


def parse_manual_report() -> tuple[int | None, int | None]:
    if not MANUAL_REPORT.exists():
        return None, None
    text = MANUAL_REPORT.read_text(errors="ignore")
    marker = "summary: bad="
    i = text.rfind(marker)
    if i < 0:
        return None, None
    tail = text[i + len(marker):].strip().split()
    try:
        bad = int(tail[0])
        total = int(tail[1].split("=")[-1])
        return bad, total
    except Exception:
        return None, None


def latest_path(paths: list[Path]) -> Path | None:
    existing = [p for p in paths if p.exists()]
    if not existing:
        return None
    return max(existing, key=lambda p: p.stat().st_mtime)


def latest_tmp_event() -> str:
    paths = []
    if TMP_AUDIT.exists():
        paths.extend(p for p in TMP_AUDIT.rglob("*") if p.is_file())
    for p in (MANUAL_CASES, MANUAL_REPORT, FULL_LEDGER, G3A, PAK):
        if p.exists():
            paths.append(p)
    latest = latest_path(paths)
    if not latest:
        return "no audit artifacts yet"
    age = max(0, int(time.time() - latest.stat().st_mtime))
    name = latest.name
    if latest.parent == TMP_AUDIT or TMP_AUDIT in latest.parents:
        name = str(latest.relative_to(TMP_AUDIT))
    return f"{name} · {age}s ago"


def git_dirty_count() -> int:
    try:
        p = subprocess.run(
            ["git", "status", "--short"],
            cwd=REPO,
            text=True,
            capture_output=True,
            timeout=2,
            check=False,
        )
        return sum(1 for ln in p.stdout.splitlines() if ln.strip())
    except Exception:
        return -1


def collect_stats() -> AuditStats:
    pdfs = local_pdfs()
    pdf_names = {rel_source(str(p)) for p in pdfs}
    cases = read_jsonl(MANUAL_CASES)
    source_rows = {rel_source(str(r.get("source_pdf", ""))) for r in cases if r.get("source_pdf")}
    complete_rows = {
        rel_source(str(r.get("source_pdf", "")))
        for r in cases
        if r.get("source_pdf") and (r.get("coverage") == "complete" or r.get("item") == "source_complete_marker")
    }
    unsupported = sum(1 for r in cases if r.get("status") == "unsupported-ok")
    host_commands = 0
    for r in cases:
        if r.get("args"):
            host_commands += 1
        host_commands += sum(1 for v in r.get("variants", []) if isinstance(v, dict) and v.get("args"))
    complete_local = len(pdf_names & complete_rows)
    pending = sorted(pdf_names - complete_rows)
    known_total = max(len(pdfs), count_download_manifest(), len(complete_rows), len(source_rows))
    current = "none"
    tmp = latest_path([p for p in TMP_AUDIT.rglob("*")] if TMP_AUDIT.exists() else [])
    if tmp:
        current = tmp.stem
    elif pending:
        current = pending[0]
    bad, total = parse_manual_report()
    g3a_size = G3A.stat().st_size if G3A.exists() else 0
    pak_size = PAK.stat().st_size if PAK.exists() else 0
    dirty = git_dirty_count()
    phase = "fixing" if dirty > 0 else "auditing"
    if bad:
        phase = "fix failing manual cases"
    if not pending and bad == 0:
        phase = "all local complete"
    return AuditStats(
        pdf_count=len(pdfs),
        downloaded_known=known_total,
        source_rows=len(source_rows),
        complete_sources=len(complete_rows),
        complete_local=complete_local,
        tracked_sources=len(source_rows),
        manual_rows=len(cases),
        host_commands=host_commands,
        unsupported_rows=unsupported,
        pending=pending,
        current=current,
        report_bad=bad,
        report_total=total,
        g3a_size=g3a_size,
        pak_size=pak_size,
        git_dirty=dirty,
        last_event=latest_tmp_event(),
        phase=phase,
    )


def bar(done: int, total: int, width: int, enabled: bool) -> str:
    frac = pct(done, total)
    fill = int(round(frac * width))
    body = "█" * fill + "░" * max(0, width - fill)
    code = C.green if frac >= 1 else C.cyan if frac >= 0.5 else C.yellow
    return color(body, code, enabled) + f" {frac * 100:5.1f}%"


def live_bar(done: int, total: int, width: int, frame: int, enabled: bool) -> str:
    frac = pct(done, total)
    fill = int(round(frac * width))
    cells = ["█"] * fill + ["░"] * max(0, width - fill)
    if 0 < fill < width:
        cells[(frame // 2) % fill] = "▓"
    elif fill == 0:
        cells[frame % width] = "▒"
    body = "".join(cells)
    code = C.green if frac >= 1 else C.cyan if frac >= 0.5 else C.yellow
    return color(body, code, enabled) + f" {frac * 100:5.1f}%"


def size_line(label: str, size: int, limit: int, width: int, enabled: bool) -> str:
    headroom = limit - size
    status = "OK" if headroom >= 0 else "OVER"
    code = C.green if headroom >= 1024 else C.yellow if headroom >= 0 else C.red
    mib = size / (1024 * 1024)
    return f"{label:<11} {bar(size, limit, width, enabled)} {mib:.3f} MiB · {status} · headroom {headroom:,} B"


def row(label: str, value: str, width: int, enabled: bool) -> str:
    return fit(f"{color(label, C.dim, enabled):<18} {value}", width)


def render(stats: AuditStats, frame: int, width: int, height: int, enabled: bool) -> str:
    width = max(72, width)
    bar_w = max(16, min(42, width - 45))
    title = color("CasioCAS Audit", C.bold + C.cyan, enabled)
    spin = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏"[frame % 10] if enabled else "|/-\\"[frame % 4]
    clock = time.strftime("%H:%M:%S")
    report = "n/a"
    if stats.report_bad is not None:
        code = C.green if stats.report_bad == 0 else C.red
        report = color(f"bad {stats.report_bad} / {stats.report_total}", code, enabled)
    dirty_code = C.green if stats.git_dirty == 0 else C.yellow if stats.git_dirty > 0 else C.red
    pending_count = max(0, stats.pdf_count - stats.complete_local)
    corpus_pending = max(0, stats.downloaded_known - stats.complete_sources)
    lines = [
        fit(f"{title} {spin}  {stats.phase}  live {clock}", width),
        fit("─" * width, width),
        row("current", stats.current, width, enabled),
        row("last event", stats.last_event, width, enabled),
        row("report", f"{report}   git {color(str(stats.git_dirty), dirty_code, enabled)} dirty   {stats.host_commands} host cases", width, enabled),
        "",
        fit(color("Progress", C.bold, enabled), width),
        fit(f"corpus     {live_bar(stats.complete_sources, stats.downloaded_known or stats.complete_sources, bar_w, frame, enabled)} {stats.complete_sources}/{stats.downloaded_known} done · {corpus_pending} left", width),
        fit(f"local pdfs {bar(stats.pdf_count, stats.downloaded_known or stats.pdf_count, bar_w, enabled)} {stats.pdf_count}/{stats.downloaded_known} present on disk", width),
        fit(f"local audit{live_bar(stats.complete_local, stats.pdf_count or stats.complete_local, bar_w, frame, enabled)} {stats.complete_local}/{stats.pdf_count} done · {pending_count} pending", width),
        fit(f"tracked    {bar(stats.tracked_sources, stats.downloaded_known or stats.tracked_sources, bar_w, enabled)} {stats.tracked_sources}/{stats.downloaded_known} sources", width),
        fit(f"manual     {stats.manual_rows} rows · {stats.unsupported_rows} unsupported-ok/context", width),
        "",
        fit(color("Artifacts", C.bold, enabled), width),
        fit(size_line("g3a", stats.g3a_size, LIMIT_2MIB, bar_w, enabled), width),
        fit(f"pak        {stats.pak_size:,} B", width),
        "",
        fit(color("Pending downloaded PDFs", C.bold, enabled), width),
    ]
    max_pending = max(0, height - len(lines) - 2)
    shown = stats.pending[:max_pending]
    if shown:
        lines.extend(fit(f"  {i+1:02d}. {p}", width) for i, p in enumerate(shown))
        if len(stats.pending) > len(shown):
            lines.append(fit(f"  … {len(stats.pending) - len(shown)} more", width))
    else:
        lines.append(fit("  none", width))
    lines.append(fit(color("q", C.bold, enabled) + "/ctrl-c quit · --once prints one frame", width))
    return "\n".join(lines[:height])


def run_loop(fps: float, no_color: bool) -> int:
    delay = 1.0 / max(1.0, fps)
    use_color = not no_color and sys.stdout.isatty()
    old_tty = termios.tcgetattr(sys.stdin) if sys.stdin.isatty() else None
    sys.stdout.write("\033[?1049h\033[?25l")
    sys.stdout.flush()
    if old_tty:
        tty.setcbreak(sys.stdin.fileno())
    frame = 0
    try:
        while True:
            stats = collect_stats()
            size = shutil.get_terminal_size((100, 34))
            sys.stdout.write("\033[H\033[2J" + render(stats, frame, size.columns, size.lines, use_color))
            sys.stdout.flush()
            frame += 1
            if old_tty and select.select([sys.stdin], [], [], delay)[0]:
                if sys.stdin.read(1).lower() == "q":
                    return 0
                continue
            time.sleep(delay)
    except KeyboardInterrupt:
        return 0
    finally:
        if old_tty:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_tty)
        sys.stdout.write("\033[?25h\033[?1049l")
        sys.stdout.flush()


def main() -> int:
    ap = argparse.ArgumentParser(description="Live FPS monitor for CasioCAS paper-audit progress.")
    ap.add_argument("--fps", type=float, default=float(os.environ.get("CASIO_AUDIT_TUI_FPS", "8")))
    ap.add_argument("--once", action="store_true", help="print one non-interactive frame and exit")
    ap.add_argument("--no-color", action="store_true")
    args = ap.parse_args()
    if args.once:
        stats = collect_stats()
        size = shutil.get_terminal_size((100, 34))
        print(render(stats, 0, size.columns, size.lines, not args.no_color))
        return 0
    return run_loop(args.fps, args.no_color)


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import termios
import time
import tty
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / "progress" / "state.jsonl"
GRAPH = ROOT / "docs" / "GRAPH.md"
G3A = ROOT / "calculator_files" / "CAS.g3a"
QUEUE_FILE = ROOT / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
QUEUE_LIVE = ROOT / "progress" / "exact_queue_latest.json"
QUEUE_REPORT = ROOT / "tests" / "reports" / "exact_calculator_input_queue" / "latest.jsonl"
QUEUE_FAILS = ROOT / "tests" / "reports" / "exact_calculator_input_queue" / "failures_latest.txt"
LIMIT = 2 * 1024 * 1024
TARGET = 2_000_000


class C:
    reset = "\033[0m"
    bold = "\033[1m"
    dim = "\033[2m"
    red = "\033[31m"
    green = "\033[32m"
    yellow = "\033[33m"
    cyan = "\033[36m"


@dataclass
class Stat:
    phase: str
    last: str
    tests: str
    queue: str
    unsupported: str
    branch: str
    commit: str
    tag: str
    dirty: int
    g3a: int
    g3a_age: str
    g3a_hash: str
    graph_age: str
    report_age: str
    queue_rows: int
    queue_inputs: int
    checkpoint: str
    failures: list[str]
    ratios: list[tuple[str, int, int]]
    dirty_files: list[str]
    events: list[str]


def color(text: str, code: str, enabled: bool) -> str:
    return f"{code}{text}{C.reset}" if enabled else text


def run(args: list[str]) -> str:
    try:
        return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.DEVNULL, timeout=2).rstrip()
    except Exception:
        return "n/a"


def latest_state() -> dict:
    last: dict = {}
    if not STATE.exists():
        return last
    for line in STATE.read_text(errors="ignore").splitlines():
        try:
            last = json.loads(line)
        except json.JSONDecodeError:
            pass
    return last


def latest_checkpoint() -> dict:
    last: dict = {}
    if not STATE.exists():
        return last
    for line in STATE.read_text(errors="ignore").splitlines():
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        if "g3a_size" in row or row.get("phase") == "release candidate":
            last = row
    return last


def file_size(path: Path) -> int:
    return path.stat().st_size if path.exists() else 0


def short_hash(path: Path) -> str:
    if not path.exists():
        return "missing"
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()[:12]


def dirty_count() -> int:
    out = run(["git", "status", "--short"])
    if out == "n/a" or not out:
        return 0
    return len([ln for ln in out.splitlines() if ln.strip()])


def dirty_files(limit: int = 5) -> list[str]:
    out = run(["git", "status", "--short"])
    if out == "n/a" or not out:
        return []
    files: list[str] = []
    for line in out.splitlines():
        if line.strip():
            files.append(line[:2] + " " + line[3:])
            if len(files) >= limit:
                break
    return files


def graph_age() -> str:
    if not GRAPH.exists():
        return "missing"
    age = max(0, int(time.time() - GRAPH.stat().st_mtime))
    if age < 60:
        return f"{age}s ago"
    if age < 3600:
        return f"{age // 60}m ago"
    return f"{age // 3600}h ago"


def age_s(path: Path) -> str:
    if not path.exists():
        return "missing"
    age = max(0, int(time.time() - path.stat().st_mtime))
    if age < 60:
        return f"{age}s"
    if age < 3600:
        return f"{age // 60}m"
    return f"{age // 3600}h"


def recent_events(limit: int = 4) -> list[str]:
    if not STATE.exists():
        return []
    rows: list[str] = []
    for line in STATE.read_text(errors="ignore").splitlines()[-80:]:
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        phase = str(row.get("phase", "n/a"))
        event = str(row.get("last_event", row.get("queue", "n/a")))
        if event and event != "n/a":
            rows.append(f"{phase}: {event}")
    return rows[-limit:]


def queue_counts() -> tuple[int, int]:
    rows = inputs = 0
    if not QUEUE_FILE.exists():
        return rows, inputs
    for line in QUEUE_FILE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        rows += 1
        try:
            data = json.loads(line)
            inputs += len(data.get("inputs") or [])
        except json.JSONDecodeError:
            pass
    return rows, inputs


def failure_samples(limit: int = 3) -> list[str]:
    if not QUEUE_FAILS.exists():
        return []
    samples: list[str] = []
    for line in QUEUE_FAILS.read_text(errors="ignore").splitlines():
        if " input " not in line or ":" not in line:
            continue
        samples.append(line.strip())
        if len(samples) >= limit:
            break
    return samples


def ratios_from(text: str) -> list[tuple[str, int, int]]:
    out: list[tuple[str, int, int]] = []
    for label in ("help-examples", "working-quality", "shared-working", "queue-run", "removed"):
        m = re.search(rf"{re.escape(label)}\s+(\d+)/(\d+)", text)
        if m:
            out.append((label, int(m.group(1)), int(m.group(2))))
    return out


def queue_progress() -> dict[str, int | str]:
    if QUEUE_LIVE.exists() and (
        not QUEUE_REPORT.exists() or QUEUE_LIVE.stat().st_mtime >= QUEUE_REPORT.stat().st_mtime
    ):
        try:
            data = json.loads(QUEUE_LIVE.read_text(errors="ignore"))
            if int(data.get("total", 0)) > 0:
                return data
        except (json.JSONDecodeError, TypeError, ValueError):
            pass
    if not QUEUE_REPORT.exists():
        return {}
    total = ok = bad = 0
    for line in QUEUE_REPORT.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        total += 1
        if row.get("ok"):
            ok += 1
        else:
            bad += 1
    if total <= 0:
        return {}
    return {"phase": "exact_queue", "done": total, "total": total, "ok": ok, "bad": bad, "active": "latest report"}


def collect() -> Stat:
    s = latest_state()
    checkpoint = latest_checkpoint()
    tests = str(s.get("tests", "n/a"))
    queue = str(s.get("queue", "n/a"))
    ratios = ratios_from(tests)
    q = queue_progress()
    if q:
        done = int(q.get("done", 0))
        total = int(q.get("total", 0))
        ok = int(q.get("ok", 0))
        bad = int(q.get("bad", 0))
        queue = f"{done}/{total} done, {ok} ok, {bad} bad"
        ratios = [("queue-done", done, total), ("queue-right", ok, total), *ratios]
    rows, inputs = queue_counts()
    return Stat(
        phase=str(s.get("phase", "n/a")),
        last=str(s.get("last_event", "n/a")),
        tests=tests if tests != "n/a" else str(checkpoint.get("tests", "n/a")),
        queue=queue,
        unsupported=str(s.get("unsupported", checkpoint.get("unsupported", "n/a"))),
        branch=run(["git", "branch", "--show-current"]),
        commit=run(["git", "rev-parse", "--short", "HEAD"]),
        tag=run(["git", "describe", "--tags", "--exact-match", "HEAD"]),
        dirty=dirty_count(),
        g3a=file_size(G3A),
        g3a_age=age_s(G3A),
        g3a_hash=short_hash(G3A),
        graph_age=graph_age(),
        report_age=age_s(QUEUE_REPORT),
        queue_rows=rows,
        queue_inputs=inputs,
        checkpoint=str(checkpoint.get("last_event", "n/a")),
        failures=failure_samples(),
        ratios=ratios,
        dirty_files=dirty_files(),
        events=recent_events(),
    )


def pct(done: int, total: int) -> float:
    if total <= 0:
        return 0.0
    return max(0.0, min(1.0, done / total))


def bar(done: int, total: int, width: int, frame: int, enabled: bool) -> str:
    frac = pct(done, total)
    fill = int(frac * width)
    cells = ["#"] * fill + ["-"] * max(0, width - fill)
    if fill < width:
        cells[(fill + frame) % width] = ">"
    body = "".join(cells)
    code = C.green if frac >= 1 else C.cyan if frac >= 0.5 else C.yellow
    return color(body, code, enabled) + f" {frac * 100:5.1f}%"


def trim(text: str, width: int) -> str:
    raw = re.sub(r"\033\[[0-9;]*m", "", text)
    if len(raw) <= width:
        return text + " " * (width - len(raw))
    return raw[: max(0, width - 3)] + "..."


def size_row(label: str, size: int, frame: int, width: int, enabled: bool) -> str:
    mib = size / (1024 * 1024)
    headroom = LIMIT - size
    status = "OK" if size and headroom >= 0 else "MISSING" if not size else "OVER"
    code = C.green if status == "OK" else C.red
    target_delta = TARGET - size
    return trim(
        f"{label:<14} {bar(size, LIMIT, width, frame, enabled)} {mib:.3f} MiB "
        f"{color(status, code, enabled)} hard {headroom:,} B target {target_delta:,} B",
        120,
    )


def rate(done: int, total: int) -> str:
    return "n/a" if total <= 0 else f"{done / total * 100:.2f}%"


def badge(text: str, code: str, enabled: bool) -> str:
    return color(f"[{text}]", code, enabled)


def kv(key: str, val: str, width: int, enabled: bool) -> str:
    return trim(f"{color(key, C.dim, enabled):<18} {val}", width)


def panel(title: str, rows: list[str], width: int, enabled: bool) -> list[str]:
    inner = max(20, width - 4)
    head = f"+-- {title} "
    top = trim(head + "-" * max(0, width - len(head) - 1) + "+", width)
    out = [color(top, C.dim, enabled)]
    for row in rows:
        raw = trim(row, inner)
        out.append(color("| ", C.dim, enabled) + raw + color(" |", C.dim, enabled))
    out.append(color("+" + "-" * (width - 2) + "+", C.dim, enabled))
    return out


def render_compact(st: Stat, frame: int, width: int, enabled: bool) -> str:
    width = max(78, width)
    bar_w = max(18, min(34, width - 48))
    spin = "|/-\\"[frame % 4]
    pulse = ("...." + ("#" * (frame % 5))).ljust(9, ".")[:9]
    headroom = LIMIT - st.g3a
    status = "OK" if st.g3a and headroom >= 0 else "OVER" if st.g3a else "MISSING"
    status_code = C.green if status == "OK" else C.red
    dirty_code = C.green if st.dirty == 0 else C.yellow
    lines = [
        trim("=" * width, width),
        trim(
            f"{color('CAS LIVE AUDIT', C.bold + C.cyan, enabled)} {spin} "
            f"{time.strftime('%H:%M:%S')} scan {pulse} "
            f"{badge(status, status_code, enabled)} {badge('dirty ' + str(st.dirty), dirty_code, enabled)}",
            width,
        ),
        trim("=" * width, width),
        kv("repo", f"{st.branch} @ {st.commit}  graph {st.graph_age}", width, enabled),
        kv("phase", st.phase, width, enabled),
        kv("event", st.last, width, enabled),
        size_row("CAS.g3a", st.g3a, frame, bar_w, enabled),
        kv("artifact", f"age {st.g3a_age}  sha256 {st.g3a_hash}  headroom {headroom:,} B", width, enabled),
        kv("queue", f"{st.queue_rows:,} rows / {st.queue_inputs:,} inputs", width, enabled),
        kv("latest", f"{st.queue}  report age {st.report_age}", width, enabled),
        trim(color("progress", C.bold, enabled), width),
    ]
    for label, done, total in st.ratios[:4]:
        lines.append(trim(f"{label:<14} {bar(done, total, bar_w, frame, enabled)} {done}/{total}", width))
    if not st.ratios:
        lines.append(trim("no ratio data yet", width))
    lines.append(kv("dirty files", "; ".join(st.dirty_files) if st.dirty_files else "clean", width, enabled))
    quality = f"unsupported {st.unsupported}"
    if st.failures:
        quality += "  first gap " + st.failures[0]
    else:
        quality += "  strict gaps none reported"
    lines.append(kv("quality", quality, width, enabled))
    if st.events:
        lines.append(kv("recent", st.events[-1], width, enabled))
    lines.extend(
        [
            trim("-" * width, width),
            trim(f"run: python3 {ROOT / 'tools' / 'audit_progress_tui.py'} --fps 12", width),
            trim("q/ctrl-c quit  --once one frame  --fps N animation rate", width),
        ]
    )
    return "\n".join(lines)


def render(st: Stat, frame: int, width: int, height: int, enabled: bool) -> str:
    if height <= 32:
        return render_compact(st, frame, width, enabled)
    width = max(78, width)
    bar_w = max(18, min(44, width - 54))
    spin = "|/-\\"[frame % 4]
    pulse = ("...." + ("#" * (frame % 5))).ljust(9, ".")[:9]
    dirty_color = C.green if st.dirty == 0 else C.yellow
    tag = st.tag if st.tag != "n/a" else "untagged"
    headroom = LIMIT - st.g3a
    artifact_status = "OK" if st.g3a and headroom >= 0 else "OVER" if st.g3a else "MISSING"
    artifact_code = C.green if artifact_status == "OK" else C.red
    header = (
        f"{color('CAS LIVE AUDIT', C.bold + C.cyan, enabled)} {spin} "
        f"{time.strftime('%H:%M:%S')} scan {pulse} "
        f"{badge(artifact_status, artifact_code, enabled)} "
        f"{badge('dirty ' + str(st.dirty), dirty_color, enabled)}"
    )
    lines = [trim("=" * width, width), trim(header, width), trim("=" * width, width)]
    lines += panel(
        "repo",
        [
            f"branch {st.branch} @ {st.commit}  tag {tag}",
            f"phase  {st.phase}",
            f"event  {st.last}",
            f"graph  {st.graph_age}  checkpoint {st.checkpoint}",
        ],
        width,
        enabled,
    )
    lines.append("")
    lines += panel(
        "artifact",
        [
            size_row("CAS.g3a", st.g3a, frame, bar_w, enabled),
            f"age {st.g3a_age}  sha256 {st.g3a_hash}",
            f"hard headroom {headroom:,} B  target delta {TARGET - st.g3a:,} B",
        ],
        width,
        enabled,
    )
    lines.append("")
    lines += panel(
        "queue",
        [
            f"golden {st.queue_rows:,} rows / {st.queue_inputs:,} inputs",
            f"latest {st.queue}  report age {st.report_age}",
        ],
        width,
        enabled,
    )
    lines.append("")
    lines += panel("progress", [], width, enabled)[:1]
    if st.ratios:
        for label, done, total in st.ratios:
            lines.append(trim(f"{label:<14} {bar(done, total, bar_w, frame, enabled)} {done}/{total} {rate(done,total)}", width))
    else:
        lines.append(trim("no ratio data yet", width))
    lines.append(color("+" + "-" * (width - 2) + "+", C.dim, enabled))
    lines.append("")
    lines += panel("dirty files", st.dirty_files or ["clean"], width, enabled)
    lines.append("")
    gap_rows = [f"unsupported {st.unsupported}", "strict gaps:"]
    if st.failures:
        for sample in st.failures:
            gap_rows.append(" - " + sample)
    else:
        gap_rows.append(" - none reported")
    lines += panel("quality", gap_rows, width, enabled)
    lines.append("")
    lines += panel("recent", st.events or ["no state events yet"], width, enabled)
    lines.append("")
    run_panel = panel(
        "run",
        [
            f"python3 {ROOT / 'tools' / 'audit_progress_tui.py'} --fps 12",
            "q/ctrl-c quit  --once one frame  --fps N animation rate",
        ],
        width,
        enabled,
    )
    lines += run_panel
    if len(lines) > height:
        keep = max(0, height - len(run_panel) - 1)
        lines = lines[:keep] + [""] + run_panel
    return "\n".join(lines[:height])


def live(fps: float, scan_interval: float, no_color: bool) -> int:
    delay = 1.0 / max(1.0, fps)
    enabled = (not no_color) and sys.stdout.isatty()
    old_tty = termios.tcgetattr(sys.stdin) if sys.stdin.isatty() else None
    if old_tty:
        tty.setcbreak(sys.stdin.fileno())
    sys.stdout.write("\033[?1049h\033[?25l")
    sys.stdout.flush()
    frame = 0
    last_scan = 0.0
    st = collect()
    try:
        while True:
            now = time.time()
            if now - last_scan >= scan_interval:
                st = collect()
                last_scan = now
            size = shutil.get_terminal_size((110, 28))
            sys.stdout.write("\033[H\033[2J" + render(st, frame, size.columns, size.lines, enabled))
            sys.stdout.flush()
            frame += 1
            end = time.time() + delay
            while old_tty and time.time() < end:
                import select

                if select.select([sys.stdin], [], [], max(0.0, end - time.time()))[0]:
                    if sys.stdin.read(1).lower() == "q":
                        return 0
                else:
                    break
            if not old_tty:
                time.sleep(delay)
    except KeyboardInterrupt:
        return 0
    finally:
        if old_tty:
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_tty)
        sys.stdout.write("\033[?25h\033[?1049l")
        sys.stdout.flush()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--once", action="store_true")
    ap.add_argument("--fps", type=float, default=float(os.environ.get("CASIO_AUDIT_TUI_FPS", "8")))
    ap.add_argument("--scan-interval", type=float, default=float(os.environ.get("CASIO_AUDIT_TUI_SCAN_INTERVAL", "4")))
    ap.add_argument("--no-color", action="store_true")
    args = ap.parse_args()
    if args.once:
        size = shutil.get_terminal_size((110, 28))
        print(render(collect(), 0, size.columns, size.lines, not args.no_color))
        return 0
    return live(args.fps, args.scan_interval, args.no_color)


if __name__ == "__main__":
    raise SystemExit(main())

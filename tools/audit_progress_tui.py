#!/usr/bin/env python3
from __future__ import annotations

import argparse
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
G3A = ROOT / "calculator_files" / "khicasen.g3a"
LIMIT = 2 * 1024 * 1024


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
    dirty: int
    g3a: int
    graph_age: str
    ratios: list[tuple[str, int, int]]


def color(text: str, code: str, enabled: bool) -> str:
    return f"{code}{text}{C.reset}" if enabled else text


def run(args: list[str]) -> str:
    try:
        return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.DEVNULL, timeout=2).strip()
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


def file_size(path: Path) -> int:
    return path.stat().st_size if path.exists() else 0


def dirty_count() -> int:
    out = run(["git", "status", "--short"])
    if out == "n/a" or not out:
        return 0
    return len([ln for ln in out.splitlines() if ln.strip()])


def graph_age() -> str:
    if not GRAPH.exists():
        return "missing"
    age = max(0, int(time.time() - GRAPH.stat().st_mtime))
    if age < 60:
        return f"{age}s ago"
    if age < 3600:
        return f"{age // 60}m ago"
    return f"{age // 3600}h ago"


def ratios_from(text: str) -> list[tuple[str, int, int]]:
    out: list[tuple[str, int, int]] = []
    for label in ("help-examples", "working-quality", "shared-working", "queue-run", "removed"):
        m = re.search(rf"{re.escape(label)}\s+(\d+)/(\d+)", text)
        if m:
            out.append((label, int(m.group(1)), int(m.group(2))))
    return out


def collect() -> Stat:
    s = latest_state()
    tests = str(s.get("tests", "n/a"))
    return Stat(
        phase=str(s.get("phase", "n/a")),
        last=str(s.get("last_event", "n/a")),
        tests=tests,
        queue=str(s.get("queue", "n/a")),
        unsupported=str(s.get("unsupported", "n/a")),
        branch=run(["git", "branch", "--show-current"]),
        commit=run(["git", "rev-parse", "--short", "HEAD"]),
        dirty=dirty_count(),
        g3a=file_size(G3A),
        graph_age=graph_age(),
        ratios=ratios_from(tests),
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
    return trim(
        f"{label:<14} {bar(size, LIMIT, width, frame, enabled)} {mib:.3f} MiB "
        f"{color(status, code, enabled)} headroom {headroom:,} B",
        120,
    )


def render(st: Stat, frame: int, width: int, height: int, enabled: bool) -> str:
    width = max(78, width)
    bar_w = max(18, min(44, width - 54))
    spin = "|/-\\"[frame % 4]
    dirty_color = C.green if st.dirty == 0 else C.yellow
    lines = [
        trim(f"{color('KhiCASen audit', C.bold + C.cyan, enabled)} {spin} {time.strftime('%H:%M:%S')} fps live", width),
        trim("-" * width, width),
        trim(f"branch {st.branch}  commit {st.commit}  dirty {color(str(st.dirty), dirty_color, enabled)}  graph {st.graph_age}", width),
        trim(f"phase  {st.phase}", width),
        trim(f"last   {st.last}", width),
        trim(f"queue  {st.queue}", width),
        trim(f"tests  {st.tests}", width),
        "",
        trim(color("artifacts", C.bold, enabled), width),
        size_row("khicasen.g3a", st.g3a, frame, bar_w, enabled),
        "",
        trim(color("checks", C.bold, enabled), width),
    ]
    if st.ratios:
        for label, done, total in st.ratios:
            lines.append(trim(f"{label:<14} {bar(done, total, bar_w, frame, enabled)} {done}/{total}", width))
    else:
        lines.append(trim("no ratio data yet", width))
    lines.extend(
        [
            "",
            trim(f"unsupported     {st.unsupported}", width),
            trim("q/ctrl-c quit  --once one frame  --fps N animation rate", width),
        ]
    )
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

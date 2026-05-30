#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / "progress" / "state.jsonl"
G3A = ROOT / "calculator_files" / "CasioCAS.g3a"
PAK = ROOT / "calculator_files" / "CASIOCAS.PAK"
LIMIT = 2 * 1024 * 1024


def sh(args: list[str]) -> str:
    try:
        return subprocess.check_output(args, cwd=ROOT, text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return "n/a"


def latest_state() -> dict:
    if not STATE.exists():
        return {}
    last = {}
    for line in STATE.read_text(errors="ignore").splitlines():
        try:
            last = json.loads(line)
        except json.JSONDecodeError:
            pass
    return last


def size_line(path: Path) -> str:
    if not path.exists():
        return "missing"
    size = path.stat().st_size
    return f"{size} bytes; headroom {LIMIT - size}"


def render() -> str:
    s = latest_state()
    graph = ROOT / "docs" / "GRAPH.md"
    lines = [
        "CasioCAS rebuild progress",
        f"branch: {sh(['git', 'branch', '--show-current'])}",
        f"commit: {sh(['git', 'rev-parse', '--short', 'HEAD'])}",
        f"dirty: {sh(['git', 'status', '--short']) or 'clean'}",
        f"phase: {s.get('phase', 'n/a')}",
        f"last: {s.get('last_event', 'n/a')}",
        f"tests: {s.get('tests', 'n/a')}",
        f"queue: {s.get('queue', 'n/a')}",
        f"unsupported: {s.get('unsupported', 'n/a')}",
        f"g3a: {size_line(G3A)}",
        f"pak: {size_line(PAK)}",
        f"graph_mtime: {time.ctime(graph.stat().st_mtime) if graph.exists() else 'missing'}",
    ]
    return "\n".join(lines)


def main() -> int:
    once = "--once" in os.sys.argv
    while True:
        print("\033[2J\033[H" + render())
        if once:
            return 0
        time.sleep(2)


if __name__ == "__main__":
    raise SystemExit(main())

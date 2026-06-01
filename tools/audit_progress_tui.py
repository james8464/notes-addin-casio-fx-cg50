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
BUILD_G3A = ROOT / "build" / "CAS.g3a"
QUEUE_FILE = ROOT / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
QUEUE_LIVE = ROOT / "progress" / "exact_queue_latest.json"
QUEUE_REPORT = ROOT / "tests" / "reports" / "exact_calculator_input_queue" / "latest.jsonl"
QUEUE_FAILS = ROOT / "tests" / "reports" / "exact_calculator_input_queue" / "failures_latest.txt"
AUDIT_PATH = ROOT / "tools" / "audit_progress_tui.py"
VM_ROOT = Path("/Volumes/VM")
VM_COVERAGE = ROOT / "progress" / "vm_coverage.json"
LIMIT = 2 * 1024 * 1024
TARGET = 2_000_000
_QUEUE_RATE = {"done": -1, "at": 0.0, "rate": 0.0}


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


@dataclass
class Stat:
    phase: str
    last: str
    tests: str
    queue: str
    unsupported: str
    branch: str
    commit: str
    subject: str
    tag: str
    upstream: str
    dirty: int
    staged: int
    unstaged: int
    untracked: int
    g3a: int
    build_g3a: int
    g3a_age: str
    build_age: str
    g3a_hash: str
    artifact_note: str
    graph_age: str
    state_age: str
    state_rows: int
    report_age: str
    queue_rows: int
    queue_inputs: int
    checkpoint: str
    failures: list[str]
    fail_clusters: list[tuple[str, int]]
    ratios: list[tuple[str, int, int]]
    ratio_trend: str
    dirty_files: list[str]
    events: list[str]
    next_action: str
    queue_active: str
    queue_done: int
    queue_total: int
    queue_ok: int
    queue_bad: int
    queue_rate: str
    queue_eta: str
    queue_live_age: str
    tool_count: int
    test_count: int
    ignored_items: int
    ignored_bytes: int
    cleanup_items: int
    cleanup_bytes: int
    docs_done: int
    docs_total: int
    docs_pending: int
    docs_label: str


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


def state_stats() -> tuple[str, int]:
    if not STATE.exists():
        return "missing", 0
    rows = sum(1 for line in STATE.read_text(errors="ignore").splitlines() if line.strip())
    return age_s(STATE), rows


def file_size(path: Path) -> int:
    return path.stat().st_size if path.exists() else 0


def tree_size(path: Path, limit: int = 4_000) -> int:
    if not path.exists():
        return 0
    if path.is_file():
        return file_size(path)
    total = 0
    seen = 0
    for item in path.rglob("*"):
        if item.is_file():
            total += item.stat().st_size
            seen += 1
            if seen >= limit:
                break
    return total


def tree_stat(path: Path, limit: int = 8_000) -> tuple[int, int]:
    if not path.exists():
        return 0, 0
    if path.is_file():
        return 1, file_size(path)
    count = total = 0
    for item in path.rglob("*"):
        if item.is_file():
            count += 1
            total += item.stat().st_size
            if count >= limit:
                break
    return count, total


def human_size(size: int) -> str:
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.1f}M"
    if size >= 1024:
        return f"{size / 1024:.1f}K"
    return f"{size}B"


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


def change_counts() -> tuple[int, int, int]:
    out = run(["git", "status", "--short"])
    if out == "n/a" or not out:
        return 0, 0, 0
    staged = unstaged = untracked = 0
    for line in out.splitlines():
        if not line.strip():
            continue
        if line.startswith("??"):
            untracked += 1
            continue
        if len(line) >= 2 and line[0] != " ":
            staged += 1
        if len(line) >= 2 and line[1] != " ":
            unstaged += 1
    return staged, unstaged, untracked


def dirty_files(limit: int = 3) -> list[str]:
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


def active_counts() -> tuple[int, int]:
    tools = [p for p in (ROOT / "tools").rglob("*") if p.is_file() and "__pycache__" not in p.parts]
    tests = [p for p in (ROOT / "tests").rglob("*.py") if p.is_file() and "__pycache__" not in p.parts]
    return len(tools), len(tests)


def upstream_status() -> str:
    out = run(["git", "rev-list", "--left-right", "--count", "@{u}...HEAD"])
    if out == "n/a":
        return "no upstream"
    parts = out.split()
    if len(parts) != 2:
        return "n/a"
    behind, ahead = parts
    if behind == "0" and ahead == "0":
        return "synced"
    return f"behind {behind} / ahead {ahead}"


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


def age_from_ts(ts: float) -> str:
    if ts <= 0:
        return "n/a"
    age = max(0, int(time.time() - ts))
    if age < 60:
        return f"{age}s"
    if age < 3600:
        return f"{age // 60}m"
    return f"{age // 3600}h"


def duration_s(seconds: float) -> str:
    if seconds <= 0:
        return "n/a"
    seconds = int(seconds)
    if seconds < 60:
        return f"{seconds}s"
    if seconds < 3600:
        return f"{seconds // 60}m {seconds % 60}s"
    return f"{seconds // 3600}h {(seconds % 3600) // 60}m"


def queue_rate(done: int, total: int, updated: float) -> tuple[str, str]:
    global _QUEUE_RATE
    if total <= 0 or done >= total or updated <= 0:
        _QUEUE_RATE = {"done": done, "at": updated, "rate": 0.0}
        return ("idle", "n/a")
    prev_done = int(_QUEUE_RATE["done"])
    prev_at = float(_QUEUE_RATE["at"])
    if prev_done >= 0 and done > prev_done and updated > prev_at:
        rate = (done - prev_done) / max(0.001, updated - prev_at)
        _QUEUE_RATE = {"done": done, "at": updated, "rate": rate}
    elif prev_done < 0 or done < prev_done:
        _QUEUE_RATE = {"done": done, "at": updated, "rate": 0.0}
    rate = float(_QUEUE_RATE["rate"])
    if rate <= 0:
        return ("warming", "n/a")
    return (f"{rate:.1f}/s", duration_s((total - done) / rate))


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


def queue_sources() -> set[str]:
    sources: set[str] = set()
    if not QUEUE_FILE.exists():
        return sources
    for line in QUEUE_FILE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        src = row.get("source_pdf")
        if isinstance(src, str) and src:
            sources.add(src)
    return sources


def vm_doc_sources() -> list[str]:
    if not VM_ROOT.is_dir():
        return []
    out: set[str] = set()
    for folder in sorted(VM_ROOT.rglob("* conv_png")):
        if not folder.is_dir():
            continue
        rel = folder.relative_to(VM_ROOT)
        stem = folder.name.replace(" conv_png", "")
        out.add(f"{rel.parent}/{stem}.pdf".replace("\\", "/"))
    return sorted(out)


def coverage_complete_sources() -> set[str]:
    if not VM_COVERAGE.exists():
        return set()
    try:
        data = json.loads(VM_COVERAGE.read_text(errors="ignore"))
    except json.JSONDecodeError:
        return set()
    return {src for src, meta in data.items() if isinstance(meta, dict) and meta.get("status") == "complete"}


def complete_marker_sources() -> set[str]:
    sources: set[str] = set()
    if not QUEUE_FILE.exists():
        return sources
    for line in QUEUE_FILE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        src = row.get("source_pdf")
        if not isinstance(src, str) or not src:
            continue
        if "complete_source_marker" in str(row.get("id", "")):
            sources.add(src)
        elif row.get("verdict") == "skip" and row.get("coverage") == "complete":
            sources.add(src)
    return sources


def doc_progress() -> tuple[int, int, int, str]:
    docs = set(vm_doc_sources())
    if not docs:
        return 0, 0, 0, "VM missing"
    done = docs & (queue_sources() | coverage_complete_sources() | complete_marker_sources())
    pending = len(docs) - len(done)
    return len(done), len(docs), pending, str(VM_ROOT)


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


def failure_clusters(limit: int = 5) -> list[tuple[str, int]]:
    if not QUEUE_FAILS.exists():
        return []
    counts: dict[str, int] = {}
    for line in QUEUE_FAILS.read_text(errors="ignore").splitlines():
        if " input " not in line:
            continue
        m = re.search(r":\s+([a-z_]+)\s+", line)
        if not m:
            continue
        key = m.group(1)
        counts[key] = counts.get(key, 0) + 1
    return sorted(counts.items(), key=lambda item: (-item[1], item[0]))[:limit]


def failure_cluster_rows(st: Stat, limit: int = 5) -> list[str]:
    rows: list[str] = []
    if st.fail_clusters:
        rows.append("clusters " + "  ".join(f"{k}:{v}" for k, v in st.fail_clusters[:limit]))
    else:
        rows.append("clusters none")
    if st.failures:
        rows.extend("gap " + item for item in st.failures[:2])
    else:
        rows.append("strict gaps none reported")
    return rows


def ignored_rows(limit: int = 4) -> list[str]:
    paths = [
        ROOT / "build",
        ROOT / "tests" / "reports",
        ROOT / "progress" / "exact_queue_latest.json",
        ROOT / "calculator_files" / "CAS.g3a",
    ]
    rows = []
    for path in paths:
        size = tree_size(path)
        if size:
            rows.append(f"{path.relative_to(ROOT)} {human_size(size)}")
    return rows[:limit]


def source_artifacts() -> list[Path]:
    src = ROOT / "khicas" / "upstream" / "giac90_1addin"
    globs = ("*.o", "*.a", "*.elf", "*.bin", "*.map", "*.g3a", "*.ac2", "*.882", "dump*", "khicasio*.png")
    out: list[Path] = []
    for pattern in globs:
        out.extend(src.glob(pattern))
    return sorted({p for p in out if p.exists()})


def ignored_targets() -> list[Path]:
    return [
        ROOT / "build",
        ROOT / "tests" / "reports",
        ROOT / "progress" / "exact_queue_latest.json",
        ROOT / "calculator_files" / "CAS.g3a",
        *source_artifacts(),
    ]


def cleanup_targets() -> list[Path]:
    targets = [
        ROOT / "build",
        *source_artifacts(),
    ]
    targets.extend(ROOT.rglob("__pycache__"))
    targets.extend(ROOT.rglob(".DS_Store"))
    targets.extend(ROOT.rglob("*.pyc"))
    return top_paths(sorted({p for p in targets if p.exists()}))


def top_paths(paths: list[Path]) -> list[Path]:
    out: list[Path] = []
    seen: set[Path] = set()
    for path in paths:
        resolved = path.resolve()
        if any(resolved == old or old in resolved.parents for old in seen):
            continue
        seen.add(resolved)
        out.append(path)
    return out


def stat_paths(paths: list[Path]) -> tuple[int, int]:
    seen: set[Path] = set()
    count = total = 0
    for path in paths:
        if not path.exists():
            continue
        resolved = path.resolve()
        if any(resolved == old or old in resolved.parents for old in seen):
            continue
        seen.add(resolved)
        c, s = tree_stat(path)
        count += c
        total += s
    return count, total


def cleanup_rows(limit: int = 5) -> list[str]:
    paths = cleanup_targets()
    rows: list[str] = []
    count, total = stat_paths(paths)
    if count:
        rows.append(f"reclaim {human_size(total)} across {count} file(s)")
    for path in paths:
        size = tree_size(path)
        if size:
            rows.append(f"rm -rf {path.relative_to(ROOT)}  ({human_size(size)})")
    return rows[:limit] or ["no cleanup candidates"]


def cleanup_command_rows() -> list[str]:
    return [
        f"python3 {AUDIT_PATH} --cleanup",
        "removes build/, source objects, __pycache__, .pyc, .DS_Store",
        "keeps calculator_files/CAS.g3a and test reports",
    ]


def perform_cleanup() -> tuple[int, int]:
    paths = cleanup_targets()
    count, total = stat_paths(paths)
    for path in paths:
        if not path.exists():
            continue
        if path.is_dir():
            shutil.rmtree(path)
        else:
            path.unlink()
    return count, total


def ratio_history(label: str = "queue-run", limit: int = 18) -> str:
    vals: list[float] = []
    if STATE.exists():
        for line in STATE.read_text(errors="ignore").splitlines()[-250:]:
            try:
                row = json.loads(line)
            except json.JSONDecodeError:
                continue
            for found, done, total in ratios_from(str(row.get("tests", ""))):
                if found == label and total:
                    vals.append(done / total)
    if not vals:
        return "n/a"
    vals = vals[-limit:]
    marks = ".:-=+*#%@"
    return "".join(marks[min(len(marks) - 1, int(v * (len(marks) - 1)))] for v in vals)


def ratios_from(text: str) -> list[tuple[str, int, int]]:
    out: list[tuple[str, int, int]] = []
    labels = (
        "help-examples",
        "working-quality",
        "shared-working",
        "queue-schema",
        "queue-run",
        "strict-markers",
        "removed",
    )
    for label in labels:
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


def artifact_note(g3a_size: int, build_size: int) -> str:
    if g3a_size:
        return "transfer ready"
    if build_size:
        headroom = LIMIT - build_size
        if headroom < 0:
            return f"last build over hard limit by {-headroom:,} B"
        return "build exists, transfer missing"
    return "no artifact"


def collect() -> Stat:
    s = latest_state()
    checkpoint = latest_checkpoint()
    tests = str(s.get("tests", "n/a"))
    queue = str(s.get("queue", "n/a"))
    ratios = ratios_from(tests)
    q = queue_progress()
    q_done = q_total = q_ok = q_bad = 0
    q_active = "n/a"
    q_updated = 0.0
    q_rate = "idle"
    q_eta = "n/a"
    q_age = "n/a"
    if q:
        q_done = int(q.get("done", 0))
        q_total = int(q.get("total", 0))
        q_ok = int(q.get("ok", 0))
        q_bad = int(q.get("bad", 0))
        q_active = str(q.get("active", "n/a"))
        q_updated = float(q.get("updated", 0) or 0)
        q_age = age_from_ts(q_updated) if q_updated else age_s(QUEUE_REPORT)
        q_rate, q_eta = queue_rate(q_done, q_total, q_updated)
        queue = f"{q_done}/{q_total} done, {q_ok} ok, {q_bad} bad"
        ratios = [("queue-done", q_done, q_total), ("queue-right", q_ok, q_total), *ratios]
    rows, inputs = queue_counts()
    staged, unstaged, untracked = change_counts()
    tools, tests_n = active_counts()
    st_age, st_rows = state_stats()
    ignored_items, ignored_bytes = stat_paths(ignored_targets())
    cleanup_items, cleanup_bytes = stat_paths(cleanup_targets())
    docs_done, docs_total, docs_pending, docs_label = doc_progress()
    return Stat(
        phase=str(s.get("phase", "n/a")),
        last=str(s.get("last_event", "n/a")),
        tests=tests if tests != "n/a" else str(checkpoint.get("tests", "n/a")),
        queue=queue,
        unsupported=str(s.get("unsupported", checkpoint.get("unsupported", "n/a"))),
        branch=run(["git", "branch", "--show-current"]),
        commit=run(["git", "rev-parse", "--short", "HEAD"]),
        subject=run(["git", "log", "-1", "--pretty=%s"]),
        tag=run(["git", "describe", "--tags", "--exact-match", "HEAD"]),
        upstream=upstream_status(),
        dirty=dirty_count(),
        staged=staged,
        unstaged=unstaged,
        untracked=untracked,
        g3a=file_size(G3A),
        build_g3a=file_size(BUILD_G3A),
        g3a_age=age_s(G3A),
        build_age=age_s(BUILD_G3A),
        g3a_hash=short_hash(G3A),
        artifact_note=artifact_note(file_size(G3A), file_size(BUILD_G3A)),
        graph_age=graph_age(),
        state_age=st_age,
        state_rows=st_rows,
        report_age=age_s(QUEUE_REPORT),
        queue_rows=rows,
        queue_inputs=inputs,
        checkpoint=str(checkpoint.get("last_event", "n/a")),
        failures=failure_samples(),
        fail_clusters=failure_clusters(),
        ratios=ratios,
        ratio_trend=ratio_history(),
        dirty_files=dirty_files(),
        events=recent_events(),
        next_action=str(s.get("eta", checkpoint.get("eta", "n/a"))),
        queue_active=q_active,
        queue_done=q_done,
        queue_total=q_total,
        queue_ok=q_ok,
        queue_bad=q_bad,
        queue_rate=q_rate,
        queue_eta=q_eta,
        queue_live_age=q_age,
        tool_count=tools,
        test_count=tests_n,
        ignored_items=ignored_items,
        ignored_bytes=ignored_bytes,
        cleanup_items=cleanup_items,
        cleanup_bytes=cleanup_bytes,
        docs_done=docs_done,
        docs_total=docs_total,
        docs_pending=docs_pending,
        docs_label=docs_label,
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


def split_bar(ok: int, bad: int, total: int, width: int, frame: int, enabled: bool) -> str:
    if total <= 0:
        return color("-" * width, C.dim, enabled) + " n/a"
    ok_w = int(width * pct(ok, total))
    bad_w = int(width * pct(bad, total))
    idle_w = max(0, width - ok_w - bad_w)
    idle = ["-"] * idle_w
    if idle:
        idle[(frame // 2) % idle_w] = ">"
    body = (
        color("#" * ok_w, C.green, enabled)
        + color("!" * bad_w, C.red, enabled)
        + color("".join(idle), C.dim, enabled)
    )
    return body + f" {ok / total * 100:5.1f}% ok"


def sweep(width: int, frame: int, enabled: bool) -> str:
    width = max(12, width)
    cells = ["."] * width
    for off in range(3):
        cells[(frame + off) % width] = "#"
    return color("".join(cells), C.magenta, enabled)


def meter(width: int, frame: int, enabled: bool) -> str:
    cells = [" "] * max(12, width)
    pos = frame % len(cells)
    for off, char in ((0, "#"), (1, "="), (2, "-")):
        cells[(pos + off) % len(cells)] = char
    return color("[" + "".join(cells) + "]", C.blue, enabled)


def wave(width: int, frame: int, enabled: bool) -> str:
    marks = "-=+#"
    cells = [marks[(i + frame) % len(marks)] for i in range(max(12, width))]
    return color("".join(cells), C.cyan, enabled)


def ticker(text: str, width: int, frame: int) -> str:
    if width <= 0:
        return ""
    clean = re.sub(r"\s+", " ", text).strip()
    if len(clean) <= width:
        return clean + " " * (width - len(clean))
    loop = clean + "   "
    start = frame % len(loop)
    doubled = loop + loop
    return doubled[start : start + width]


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


def size_hint(st: Stat, frame: int, width: int, enabled: bool) -> list[str]:
    rows = [size_row("CAS.g3a", st.g3a, frame, width, enabled)]
    if not st.g3a and st.build_g3a:
        rows.append(size_row("build/CAS.g3a", st.build_g3a, frame, width, enabled))
    rows.append(f"note {st.artifact_note}")
    rows.append(f"path {G3A}")
    return rows


def queue_health(st: Stat, frame: int, width: int, enabled: bool) -> list[str]:
    if st.queue_total <= 0:
        return ["no queue report yet"]
    active = st.queue_done < st.queue_total and st.queue_rate not in ("idle", "n/a")
    state = "running" if active else "complete" if st.queue_done == st.queue_total else "stale"
    state_code = C.cyan if active else C.green if state == "complete" else C.yellow
    rows = [
        f"{badge(state, state_code, enabled)} active {st.queue_active}",
        f"age {st.queue_live_age}  rate {st.queue_rate}  eta {st.queue_eta}",
        f"pass {st.queue_ok:,}/{st.queue_total:,}  fail {st.queue_bad:,}",
        split_bar(st.queue_ok, st.queue_bad, st.queue_total, max(18, width), frame, enabled),
        f"scan {wave(max(18, width), frame, enabled)}",
    ]
    return rows


def doc_scan_rows(st: Stat, frame: int, width: int, enabled: bool) -> list[str]:
    if st.docs_total <= 0:
        return [f"docs {st.docs_label}"]
    return [
        f"docs {st.docs_done:,}/{st.docs_total:,} done  pending {st.docs_pending:,}",
        bar(st.docs_done, st.docs_total, max(18, width), frame, enabled),
        f"root {st.docs_label}",
    ]


def cluster_bars(st: Stat, width: int, frame: int, enabled: bool) -> list[str]:
    if not st.fail_clusters:
        return ["strict gap map none"]
    max_count = max(count for _, count in st.fail_clusters) or 1
    bar_w = max(12, min(28, width - 18))
    rows = ["strict gap map"]
    for name, count in st.fail_clusters[:5]:
        fill = max(1, int(bar_w * count / max_count))
        cells = ["#"] * fill + ["-"] * max(0, bar_w - fill)
        if fill < bar_w:
            cells[(fill + frame) % bar_w] = ">"
        rows.append(f"{name:<10} {color(''.join(cells), C.yellow, enabled)} {count}")
    return rows


def freshness_rows(st: Stat) -> list[str]:
    return [
        f"artifact {st.g3a_age}  build {st.build_age}",
        f"graph {st.graph_age}  state {st.state_age} ({st.state_rows} rows)",
        f"queue report {st.report_age}  live {st.queue_live_age}",
        f"last event {st.last}",
    ]


def rate(done: int, total: int) -> str:
    return "n/a" if total <= 0 else f"{done / total * 100:.2f}%"


def health_score(st: Stat) -> tuple[int, str]:
    score = 0
    headroom = LIMIT - st.g3a
    if st.g3a and headroom >= 0:
        score += 25
    if st.upstream == "synced":
        score += 15
    if st.dirty == 0:
        score += 15
    q = ratio_value(st, "queue-done")
    if q and q[1]:
        score += int(20 * pct(q[0], q[1]))
    strict = ratio_value(st, "queue-right")
    if strict and strict[1]:
        score += int(25 * pct(strict[0], strict[1]))
    label = "ship" if score >= 95 else "good" if score >= 80 else "work" if score >= 60 else "risk"
    return score, label


def badge(text: str, code: str, enabled: bool) -> str:
    return color(f"[{text}]", code, enabled)


def ratio_value(st: Stat, label: str) -> tuple[int, int] | None:
    for found, done, total in st.ratios:
        if found == label:
            return done, total
    return None


def test_items(st: Stat, limit: int = 12) -> list[tuple[str, str, str]]:
    if not st.tests or st.tests == "n/a":
        return []
    rows: list[tuple[str, str, str]] = []
    for raw in [part.strip() for part in st.tests.split(",") if part.strip()]:
        status = "WARN"
        detail = ""
        if "failing" in raw or " fail" in raw:
            status = "FAIL"
        elif raw.endswith(" ok") or " ok" in raw or "exit1 expected" in raw:
            status = "OK"
        m = re.search(r"(\d+)/(\d+)", raw)
        if m:
            done, total = int(m.group(1)), int(m.group(2))
            detail = f"{done}/{total}"
            if done == total:
                status = "OK"
            elif "strict-markers" in raw or ("queue-run" in raw and st.queue_bad > 0):
                status = "WARN"
            elif done < total:
                status = "FAIL"
        name = raw
        for suffix in (" ok", " exit1 expected"):
            name = name.replace(suffix, "")
        name = re.sub(r"\s+\d+/\d+.*", "", name).strip()
        if name == "queue-run" and st.queue_bad > 0:
            name = "strict-markers"
        rows.append((name, status, detail))
        if len(rows) >= limit:
            break
    return rows


def gate_mark(status: str, frame: int, enabled: bool) -> str:
    if status == "OK":
        return color("[OK]", C.green, enabled)
    if status == "FAIL":
        return color("[!!]", C.red, enabled)
    pulse = ">>" if frame % 2 else "--"
    return color(f"[{pulse}]", C.yellow, enabled)


def gate_rows(st: Stat, frame: int, enabled: bool) -> list[str]:
    rows: list[tuple[str, str, str]] = []
    headroom = LIMIT - st.g3a
    rows.append(("artifact", "OK" if st.g3a and headroom >= 0 else "FAIL", f"headroom {headroom:,} B"))
    rows.append(("git sync", "OK" if st.upstream == "synced" else "WARN", st.upstream))
    rows.append(("workspace", "OK" if st.dirty == 0 else "WARN", f"{st.dirty} dirty"))
    rows.append(("graph", "OK" if st.graph_age != "missing" else "FAIL", st.graph_age))
    q = ratio_value(st, "queue-done")
    if q:
        rows.append(("queue", "OK" if q[0] == q[1] else "WARN", f"{q[0]}/{q[1]}"))
    strict = ratio_value(st, "queue-right")
    if strict:
        rows.append(("strict", "OK" if strict[0] == strict[1] else "WARN", f"{strict[0]}/{strict[1]}"))
    rows.extend(test_items(st, 8))
    out: list[str] = []
    for name, status, detail in rows[:12]:
        tail = f" {detail}" if detail else ""
        out.append(f"{gate_mark(status, frame, enabled)} {name}{tail}")
    return out or ["no gates yet"]


def phase_lanes(st: Stat, frame: int, enabled: bool) -> str:
    q = ratio_value(st, "queue-done")
    strict = ratio_value(st, "queue-right")
    stages = [
        ("BUILD", bool(st.g3a)),
        ("SIZE", bool(st.g3a and LIMIT - st.g3a >= 0)),
        ("QUEUE", bool(q and q[0] == q[1])),
        ("STRICT", bool(strict and strict[0] == strict[1])),
        ("COMMIT", st.dirty == 0),
        ("SYNC", st.upstream == "synced"),
    ]
    active = next((i for i, (_, ok) in enumerate(stages) if not ok), len(stages) - 1)
    rows: list[str] = []
    pulse = "*" if frame % 2 else "+"
    for i, (name, ok) in enumerate(stages):
        if ok:
            rows.append(color(f"[{name}]", C.green, enabled))
        elif i == active:
            rows.append(color(f"[{pulse}{name}{pulse}]", C.yellow, enabled))
        else:
            rows.append(color(f"[{name}]", C.dim, enabled))
    return " -> ".join(rows)


def topline_metrics(st: Stat, enabled: bool) -> str:
    headroom = LIMIT - st.g3a
    strict = ratio_value(st, "queue-right")
    gap = "n/a" if not strict else f"{strict[1] - strict[0]:,}"
    score, label = health_score(st)
    score_code = C.green if score >= 90 else C.yellow if score >= 70 else C.red
    return "  ".join(
        [
            f"score {color(str(score) + '/100 ' + label, score_code, enabled)}",
            f"headroom {headroom:,} B",
            f"strict gaps {gap}",
            f"sha {st.g3a_hash}",
        ]
    )


def status_strip(st: Stat, enabled: bool) -> str:
    headroom = LIMIT - st.g3a
    artifact = "artifact OK" if st.g3a and headroom >= 0 else "artifact MISS" if not st.g3a else "artifact OVER"
    artifact_code = C.green if st.g3a and headroom >= 0 else C.red
    sync_code = C.green if st.upstream == "synced" else C.yellow
    dirty_code = C.green if st.dirty == 0 else C.yellow
    strict = ratio_value(st, "queue-right")
    if strict:
        done, total = strict
        gap = total - done
        strict_text = f"strict {done}/{total}"
        strict_code = C.green if gap == 0 else C.yellow
    else:
        strict_text = "strict n/a"
        strict_code = C.yellow
    q = ratio_value(st, "queue-done")
    queue_text = "queue n/a" if not q else f"queue {q[0]}/{q[1]}"
    queue_code = C.green if q and q[0] == q[1] else C.cyan
    return " ".join(
        [
            badge(artifact, artifact_code, enabled),
            badge(st.upstream, sync_code, enabled),
            badge(queue_text, queue_code, enabled),
            badge(strict_text, strict_code, enabled),
            badge(f"dirty {st.dirty}", dirty_code, enabled),
        ]
    )


def risk_rows(st: Stat) -> list[str]:
    rows: list[str] = []
    headroom = LIMIT - st.g3a
    if st.g3a and headroom < 0:
        rows.append(f"g3a over hard cap by {-headroom:,} B")
    elif st.g3a and headroom < 256:
        rows.append(f"g3a hard headroom only {headroom:,} B")
    if st.dirty:
        rows.append(f"{st.dirty} dirty tracked file(s)")
    for label, done, total in st.ratios:
        if label == "queue-right" and done < total:
            rows.append(f"strict gaps {total - done:,}")
            break
    if st.report_age == "missing":
        rows.append("no exact queue report")
    if st.graph_age == "missing":
        rows.append("graph missing")
    if st.next_action not in ("", "n/a"):
        rows.append(st.next_action)
    return rows or ["none"]


def hygiene_rows(st: Stat, enabled: bool) -> list[str]:
    clean = st.dirty == 0 and st.cleanup_items == 0
    state = color("clean", C.green, enabled) if clean else color("needs attention", C.yellow, enabled)
    rows = [
        f"workspace {state}",
        f"tracked staged {st.staged}  unstaged {st.unstaged}  untracked {st.untracked}",
        f"ignored generated {st.ignored_items} file(s), {human_size(st.ignored_bytes)}",
        f"cleanup candidates {st.cleanup_items} file(s), {human_size(st.cleanup_bytes)}",
        f"tracked tools {st.tool_count}; test scripts {st.test_count}; delete candidates 0",
        f"audit path {AUDIT_PATH}",
    ]
    return rows


def tool_rows(st: Stat) -> list[str]:
    return [
        f"audit {AUDIT_PATH}",
        "build ./compile -> tools/build_g3a.sh -> tools/docker/Dockerfile.khicas-source",
        "host tools/khicas_host_runner",
        "checks size, metadata, border, catalog, help",
        f"tracked tool files {st.tool_count}; reviewed delete candidates 0",
    ]


def test_rows(st: Stat) -> list[str]:
    if not st.tests or st.tests == "n/a":
        return ["no test checkpoint yet"]
    parts = [part.strip() for part in st.tests.split(",") if part.strip()]
    return parts[:8] or [st.tests]


def strict_gap(st: Stat) -> int | None:
    strict = ratio_value(st, "queue-right")
    if not strict:
        return None
    return strict[1] - strict[0]


def readiness_rows(st: Stat) -> list[str]:
    rows: list[str] = []
    headroom = LIMIT - st.g3a
    if not st.g3a:
        rows.append("build artifact missing")
    elif headroom < 0:
        rows.append(f"artifact over hard cap by {-headroom:,} B")
    if st.upstream != "synced":
        rows.append(f"git not synced: {st.upstream}")
    if st.dirty:
        rows.append(f"tracked tree dirty: {st.dirty}")
    q = ratio_value(st, "queue-done")
    if not q:
        rows.append("exact queue not run")
    elif q[0] < q[1]:
        rows.append(f"queue incomplete: {q[0]:,}/{q[1]:,}")
    gap = strict_gap(st)
    if gap is None:
        rows.append("strict marker report missing")
    elif gap:
        rows.append(f"strict marker gaps: {gap:,}")
    if st.graph_age == "missing":
        rows.append("graph missing")
    return rows or ["release checklist clear"]


def command_rows() -> list[str]:
    return [
        f"python3 {AUDIT_PATH} --fps 12",
        "./compile",
        "python3 tests/run_exact_queue.py --engine production --workers 8",
        "python3 tests/run_exact_queue.py --engine production --workers 8 --strict-markers",
        f"transfer {ROOT / 'calculator_files' / 'CAS.g3a'}",
    ]


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


def columns(left: list[str], right: list[str], width: int) -> list[str]:
    if width < 112:
        return left + [""] + right
    gap = "  "
    l_width = (width - len(gap)) // 2
    r_width = width - len(gap) - l_width
    height = max(len(left), len(right))
    left = left + [" " * l_width] * (height - len(left))
    right = right + [" " * r_width] * (height - len(right))
    return [trim(left[i], l_width) + gap + trim(right[i], r_width) for i in range(height)]


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
        kv("commit", ticker(st.subject, max(12, width - 20), frame), width, enabled),
        kv("status", status_strip(st, enabled), width, enabled),
        kv("run path", str(AUDIT_PATH), width, enabled),
        kv("lane", phase_lanes(st, frame, enabled), width, enabled),
        kv("metrics", topline_metrics(st, enabled), width, enabled),
        kv("scanner", meter(max(12, min(28, width - 22)), frame, enabled), width, enabled),
        kv("gates", "  ".join(gate_rows(st, frame, enabled)[:3]), width, enabled),
        kv("sync", st.upstream, width, enabled),
        kv("state", f"{st.state_rows} events  age {st.state_age}", width, enabled),
        kv("phase", st.phase, width, enabled),
        kv("event", ticker(st.last, max(12, width - 20), frame), width, enabled),
        *[trim(r, width) for r in size_hint(st, frame, bar_w, enabled)],
        kv("artifact", f"age {st.g3a_age}  sha256 {st.g3a_hash}  headroom {headroom:,} B", width, enabled),
        kv("queue", f"{st.queue_rows:,} rows / {st.queue_inputs:,} inputs", width, enabled),
        kv("latest", f"{st.queue}  report age {st.report_age}", width, enabled),
        kv("live", f"{st.queue_active}  age {st.queue_live_age}  rate {st.queue_rate}  eta {st.queue_eta}", width, enabled),
        kv("vm docs", f"{bar(st.docs_done, st.docs_total, bar_w, frame, enabled)} {st.docs_done}/{st.docs_total} pending {st.docs_pending}", width, enabled),
        kv("accuracy", split_bar(st.queue_ok, st.queue_bad, st.queue_total, bar_w, frame, enabled), width, enabled),
        trim(color("progress", C.bold, enabled), width),
        trim(f"trend          {st.ratio_trend}  {wave(18, frame, enabled)}", width),
    ]
    for label, done, total in st.ratios[:4]:
        lines.append(trim(f"{label:<14} {bar(done, total, bar_w, frame, enabled)} {done}/{total}", width))
    if not st.ratios:
        lines.append(trim("no ratio data yet", width))
    for row in gate_rows(st, frame, enabled)[3:8]:
        lines.append(kv("gate", row, width, enabled))
    lines.append(kv("changes", f"staged {st.staged}  unstaged {st.unstaged}  untracked {st.untracked}", width, enabled))
    lines.append(kv("dirty files", "; ".join(st.dirty_files) if st.dirty_files else "clean", width, enabled))
    lines.append(kv("risk", "; ".join(risk_rows(st)[:3]), width, enabled))
    if st.next_action not in ("", "n/a"):
        lines.append(kv("next", st.next_action, width, enabled))
    lines.append(kv("release", "; ".join(readiness_rows(st)[:3]), width, enabled))
    ignored = ignored_rows()
    if ignored:
        lines.append(kv("ignored", "; ".join(ignored), width, enabled))
    lines.append(kv("hygiene", f"ignored {st.ignored_items} files/{human_size(st.ignored_bytes)}  cleanup {st.cleanup_items} files/{human_size(st.cleanup_bytes)}", width, enabled))
    lines.append(kv("cleanup", "; ".join(cleanup_rows(3)), width, enabled))
    lines.append(kv("clean cmd", cleanup_command_rows()[0], width, enabled))
    lines.append(kv("audit path", str(AUDIT_PATH), width, enabled))
    lines.append(kv("active", f"tools {st.tool_count}  test scripts {st.test_count}  delete candidates 0", width, enabled))
    quality = f"unsupported {st.unsupported}"
    quality += "  " + "; ".join(failure_cluster_rows(st, 3)[:2])
    lines.append(kv("quality", quality, width, enabled))
    for row in cluster_bars(st, bar_w + 14, frame, enabled)[:4]:
        lines.append(kv("gap map", row, width, enabled))
    lines.append(kv("freshness", "  ".join(freshness_rows(st)[:2]), width, enabled))
    if st.events:
        lines.append(kv("recent", st.events[-1], width, enabled))
    lines.extend(
        [
            trim("-" * width, width),
            trim("run: " + "  |  ".join(command_rows()[:3]), width),
            trim("q/ctrl-c quit  --once one frame  --fps N animation rate  --scan-interval N refresh rate", width),
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
    repo_rows = [
        f"branch {st.branch} @ {st.commit}  tag {tag}  {st.upstream}",
        f"last   {st.subject}",
        f"phase  {st.phase}",
        f"event  {st.last}",
        f"graph  {st.graph_age}  state {st.state_rows} rows / age {st.state_age}",
        f"checkpoint {st.checkpoint}",
        f"audit {AUDIT_PATH}",
    ]
    status_rows = [
        status_strip(st, enabled),
        phase_lanes(st, frame, enabled),
        topline_metrics(st, enabled),
        f"changes staged {st.staged}  unstaged {st.unstaged}  untracked {st.untracked}",
        f"active tooling {st.tool_count} files  test scripts {st.test_count}",
        f"scanner {meter(24, frame, enabled)}",
    ]
    if width >= 112:
        left_w = (width - 2) // 2
        right_w = width - 2 - left_w
        lines += columns(
            panel("repo", repo_rows, left_w, enabled),
            panel("status", status_rows, right_w, enabled),
            width,
        )
    else:
        lines += panel("repo", repo_rows, width, enabled)
        lines.append("")
        lines += panel("status", status_rows, width, enabled)
    lines.append("")
    artifact_rows = [
        *size_hint(st, frame, bar_w, enabled),
        f"age {st.g3a_age}  sha256 {st.g3a_hash}",
        f"last build age {st.build_age}",
        f"hard headroom {headroom:,} B  target delta {TARGET - st.g3a:,} B",
    ]
    queue_rows_ui = [
        f"golden {st.queue_rows:,} rows / {st.queue_inputs:,} inputs",
        f"latest {st.queue}  report age {st.report_age}",
        *queue_health(st, frame, bar_w, enabled),
    ]
    doc_rows_ui = doc_scan_rows(st, frame, bar_w, enabled)
    if width >= 112:
        left_w = (width - 2) // 2
        right_w = width - 2 - left_w
        lines += columns(
            panel("artifact", artifact_rows, left_w, enabled),
            panel("queue", queue_rows_ui, right_w, enabled),
            width,
        )
        lines += columns(
            panel("vm docs", doc_rows_ui, left_w, enabled),
            panel("progress", [f"trend {st.ratio_trend}", sweep(24, frame, enabled)], right_w, enabled),
            width,
        )
    else:
        lines += panel("artifact", artifact_rows, width, enabled)
        lines.append("")
        lines += panel("queue", queue_rows_ui, width, enabled)
        lines.append("")
        lines += panel("vm docs", doc_rows_ui, width, enabled)
    lines.append("")
    lines += panel("progress", [], width, enabled)[:1]
    lines.append(trim(f"trend          {st.ratio_trend}  {sweep(24, frame, enabled)}", width))
    if st.ratios:
        for label, done, total in st.ratios:
            lines.append(trim(f"{label:<14} {bar(done, total, bar_w, frame, enabled)} {done}/{total} {rate(done,total)}", width))
    else:
        lines.append(trim("no ratio data yet", width))
    lines.append(color("+" + "-" * (width - 2) + "+", C.dim, enabled))
    lines.append("")
    workspace_rows = [
        f"changes staged {st.staged}  unstaged {st.unstaged}  untracked {st.untracked}",
        *(st.dirty_files or ["clean"]),
    ]
    risk = risk_rows(st)
    if width >= 112:
        left_w = (width - 2) // 2
        right_w = width - 2 - left_w
        lines += columns(
            panel("workspace", workspace_rows, left_w, enabled),
            panel("risk", risk, right_w, enabled),
            width,
        )
    else:
        lines += panel("workspace", workspace_rows, width, enabled)
        lines.append("")
        lines += panel("risk", risk, width, enabled)
    lines += panel("project hygiene", hygiene_rows(st, enabled), width, enabled)
    lines += panel("tooling", tool_rows(st), width, enabled)
    lines += panel("cleanup command", cleanup_command_rows(), width, enabled)
    lines += panel("freshness", freshness_rows(st), width, enabled)
    lines += panel("tests", test_rows(st), width, enabled)
    lines += panel("gates", gate_rows(st, frame, enabled), width, enabled)
    lines += panel("release", readiness_rows(st), width, enabled)
    gap_rows = [f"unsupported {st.unsupported}", *failure_cluster_rows(st)]
    gap_rows.extend(cluster_bars(st, bar_w + 14, frame, enabled))
    lines += panel("quality", gap_rows, width, enabled)
    ignored = ignored_rows()
    if ignored:
        lines.append("")
        lines += panel("ignored workspace", ignored, width, enabled)
    lines.append("")
    lines += panel("cleanup candidates", cleanup_rows(), width, enabled)
    lines.append("")
    lines += panel("recent", st.events or ["no state events yet"], width, enabled)
    lines.append("")
    run_panel = panel(
        "run",
        [*command_rows(), "q/ctrl-c quit  --once one frame  --fps N animation rate"],
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
    ap.add_argument("--cleanup", action="store_true", help="remove generated cleanup candidates")
    ap.add_argument("--fps", type=float, default=float(os.environ.get("CASIO_AUDIT_TUI_FPS", "8")))
    ap.add_argument("--scan-interval", type=float, default=float(os.environ.get("CASIO_AUDIT_TUI_SCAN_INTERVAL", "4")))
    ap.add_argument("--no-color", action="store_true")
    args = ap.parse_args()
    if args.cleanup:
        count, total = perform_cleanup()
        print(f"removed {count} generated file(s), reclaimed {human_size(total)}")
        return 0
    if args.once:
        size = shutil.get_terminal_size((110, 28))
        print(render(collect(), 0, size.columns, size.lines, not args.no_color))
        return 0
    return live(args.fps, args.scan_interval, args.no_color)


if __name__ == "__main__":
    raise SystemExit(main())

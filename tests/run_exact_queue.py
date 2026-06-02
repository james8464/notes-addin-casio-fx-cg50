#!/usr/bin/env python3
"""Run exact calculator-input queue rows through the shared host working runner."""

from __future__ import annotations

import argparse
import concurrent.futures as cf
import json
import subprocess
import time
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"
OUT = REPO / "tests" / "reports" / "exact_calculator_input_queue"
REPORT = OUT / "latest.jsonl"
FAILS = OUT / "failures_latest.txt"
PROGRESS = REPO / "progress"
LIVE = PROGRESS / "exact_queue_latest.json"
STATE = PROGRESS / "state.jsonl"


def normalize_module(module: str, text: str) -> str:
    m = (module or "algebra").strip().lower()
    low = text.strip().lower()
    if m in {"derive", "differentiate", "differentiation", "derivative"}:
        return "derive"
    if m in {"integrate", "integration", "integral"}:
        return "integrate"
    if m in {"trig", "trigonometry"}:
        return "trig"
    if m == "calculus":
        if low.startswith(("diff(", "derive(")):
            return "derive"
        if low.startswith(("int(", "integrate(", "defint(")):
            return "integrate"
        return "algebra"
    if m == "exact":
        return "algebra"
    return m


def read_rows() -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        rows.append(json.loads(line))
    return rows


def specs() -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for row in read_rows():
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            text = item["input"]
            out.append({
                "id": row["id"],
                "source_pdf": row.get("source_pdf") or row.get("source", ""),
                "question": row.get("question") or row["id"],
                "part": row.get("part", ""),
                "input_index": i,
                "module": normalize_module(item.get("module", "algebra"), text),
                "input": text,
                "markers": item.get("expected_output_markers", []),
                "working": item.get("mark_scheme_working", []),
                "curated": bool(row.get("verdict") or item.get("expected_output_markers")),
            })
    return out


def write_progress(done: int, total: int, ok: int, bad: int, active: str = "") -> None:
    PROGRESS.mkdir(parents=True, exist_ok=True)
    payload = {
        "phase": "exact_queue",
        "done": done,
        "total": total,
        "ok": ok,
        "bad": bad,
        "active": active,
        "updated": round(time.time(), 3),
    }
    tmp = LIVE.with_suffix(".tmp")
    tmp.write_text(json.dumps(payload, separators=(",", ":")) + "\n")
    tmp.replace(LIVE)
    if done not in (0, total):
        return
    with STATE.open("a") as f:
        f.write(json.dumps({
            "phase": "queue",
            "last_event": f"exact queue {done}/{total}",
            "queue": f"{done}/{total} done, {ok} ok, {bad} bad",
            "tests": f"queue-run {ok}/{total}",
        }, separators=(",", ":")) + "\n")


def run_one(spec: dict[str, Any], strict: bool) -> dict[str, Any]:
    t0 = time.time()
    text = spec["input"].strip()
    module = spec["module"]
    low = text.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(HOST), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(HOST), "--int", text]
    elif module == "trig" or low.startswith(("trig(", "solve_trig(")):
        argv = [str(HOST), "--trig", text]
    elif module == "stats":
        return {
            **spec,
            "ok": False,
            "returncode": 2,
            "missing": [],
            "banned": ["stats/probability out of scope"],
            "seconds": 0,
            "output": ["stats/probability out of scope"],
        }
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(
        argv,
        cwd=REPO,
        text=True,
        capture_output=True,
        timeout=20,
    )
    out = proc.stdout + proc.stderr
    banned_terms = ("ERR:", "Err:", "traceback", "Traceback") if spec.get("curated", True) else ("traceback", "Traceback")
    banned = [b for b in banned_terms if b in out]
    missing = [m for m in spec["markers"] if m and m not in out]
    ok = proc.returncode == 0 and not banned and (not strict or not missing)
    return {
        **spec,
        "ok": ok,
        "returncode": proc.returncode,
        "missing": missing,
        "banned": banned,
        "seconds": round(time.time() - t0, 3),
        "output": out.splitlines()[:40] if not ok else [],
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--engine",
        choices=("production", "host-provisional"),
        default="production",
        help="production must match ./compile/.g3a; host-provisional is not calculator proof.",
    )
    ap.add_argument("--strict-markers", action="store_true")
    ap.add_argument("--workers", type=int, default=8)
    args = ap.parse_args()
    if not HOST.exists():
        print(f"FAIL runner missing: {HOST}")
        return 2
    if not HOST.stat().st_mode & 0o111:
        print(f"FAIL runner not executable: {HOST}")
        return 2
    warm = subprocess.run([str(HOST), "--alg", "1"], cwd=REPO, text=True, capture_output=True, timeout=60)
    if warm.returncode != 0:
        print(f"FAIL runner warmup rc={warm.returncode}")
        print((warm.stdout + warm.stderr)[:1000])
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    work = specs()
    total = len(work)
    write_progress(0, total, 0, 0, "start")
    results: list[dict[str, Any]] = []
    done = ok = bad_count = 0
    last_write = 0.0
    with cf.ThreadPoolExecutor(max_workers=max(1, args.workers)) as ex:
        futs = {ex.submit(run_one, {**s, "_order": i}, args.strict_markers): s for i, s in enumerate(work)}
        for fut in cf.as_completed(futs):
            r = fut.result()
            results.append(r)
            done += 1
            if r["ok"]:
                ok += 1
            else:
                bad_count += 1
            now = time.time()
            if done == total or not r["ok"] or now - last_write >= 0.25:
                write_progress(done, total, ok, bad_count, str(r.get("id", ""))[:80])
                last_write = now
    results.sort(key=lambda r: int(r.pop("_order", 0)))
    REPORT.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in results))
    bad = [r for r in results if not r["ok"]]
    write_progress(len(results), len(results), len(results) - len(bad), len(bad), "complete")
    FAILS.write_text("\n\n".join(
        f"{r['id']} {r['question']} input {r['input_index']}: {r['module']} {r['input']}\n"
        f"missing={r['missing']} banned={r['banned']} rc={r['returncode']}\n"
        + "\n".join(r["output"])
        for r in bad
    ))
    print(f"OK exact queue run {len(results) - len(bad)}/{len(results)} ok bad={len(bad)} report={REPORT}")
    return 0 if not bad else 1


if __name__ == "__main__":
    raise SystemExit(main())

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
            out.append({
                "id": row["id"],
                "source_pdf": row["source_pdf"],
                "question": row["question"],
                "part": row.get("part", ""),
                "input_index": i,
                "module": item["module"],
                "input": item["input"],
                "markers": item.get("expected_output_markers", []),
                "working": item.get("mark_scheme_working", []),
            })
    return out


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
    elif module == "suvat" or low.startswith("suvat("):
        argv = [str(HOST), "--suvat", text]
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
    banned = [b for b in ("ERR:", "Err:", "traceback", "Traceback") if b in out]
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
    with cf.ThreadPoolExecutor(max_workers=max(1, args.workers)) as ex:
        results = list(ex.map(lambda s: run_one(s, args.strict_markers), work))
    REPORT.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in results))
    bad = [r for r in results if not r["ok"]]
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

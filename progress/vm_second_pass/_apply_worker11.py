#!/usr/bin/env python3
"""Second-pass worker 11: patch mp2_a..mp2_m queue rows (extend inputs)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"

REPLACEMENTS: dict[str, dict[int, dict]] = {
    "madas_iygb_mp2_b_q4_exact_inputs": {
        1: {
            "module": "algebra",
            "input": "method=numeric,1+4-2",
            "mark_scheme_working": ["D_x component: a_x+c_x-b_x=3"],
            "expected_output_markers": ["3"],
        }
    }
}

ADDITIONS: dict[str, list[dict]] = {
    "madas_iygb_mp2_a_q12_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,(1/16)*(-1+2*ln(2))",
            "mark_scheme_working": ["substitution u=2x+1 gives exact integral 1/16(-1+2ln2)"],
            "expected_output_markers": ["0.02414339757"],
        }
    ],
    "madas_iygb_mp2_b_q4_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,1+0-1",
            "mark_scheme_working": ["D_y component: a_y+c_y-b_y=0"],
            "expected_output_markers": ["0"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,2+1-5",
            "mark_scheme_working": ["D_z component: a_z+c_z-b_z=-2"],
            "expected_output_markers": ["-2"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,1-1",
            "mark_scheme_working": ["F_x from A translated by BE vector: 1-1=0"],
            "expected_output_markers": ["0"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,1+1",
            "mark_scheme_working": ["F_y: 1+1=2"],
            "expected_output_markers": ["2"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,2+7",
            "mark_scheme_working": ["F_z: 2+7=9 gives F(0,2,9)"],
            "expected_output_markers": ["9"],
        },
    ],
    "madas_iygb_mp2_h_q8_exact_inputs": [
        {
            "module": "algebra",
            "input": "factor(x^4-3*x^2-4)",
            "mark_scheme_working": ["f(f(x))=-47 gives x^4-3x^2-4=0"],
            "expected_output_markers": ["(x + 2)*(x - 2)*(x^2 + 1)"],
        }
    ],
    "madas_iygb_mp2_j_q12_exact_inputs": [
        {
            "module": "algebra",
            "input": "factor(20*x^2-41*x+2)",
            "mark_scheme_working": ["binomial approximation gives 20x^2-41x+2=0"],
            "expected_output_markers": ["(20*x - 1)*(x - 2)"],
        }
    ],
}


def run_input(module: str, text: str) -> tuple[int, str]:
    text = text.strip()
    low = text.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(HOST), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(HOST), "--int", text]
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(argv, cwd=REPO, text=True, capture_output=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def verify_row(row: dict) -> list[str]:
    issues: list[str] = []
    for i, item in enumerate(row.get("inputs", []), 1):
        rc, out = run_input(item["module"], item["input"])
        missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
        if rc != 0 or missing:
            issues.append(f"{row['id']} #{i} {item['input']!r} missing={missing}")
    return issues


def main() -> int:
    rows = [json.loads(line) for line in QUEUE.read_text().splitlines() if line.strip()]
    by_id = {r["id"]: r for r in rows}
    touched: list[str] = []

    for rid, idx_map in REPLACEMENTS.items():
        row = by_id[rid]
        for idx, new_item in idx_map.items():
            row["inputs"][idx - 1] = new_item
        touched.append(rid)

    for rid, new_items in ADDITIONS.items():
        row = by_id[rid]
        row["inputs"].extend(new_items)
        touched.append(rid)

    issues: list[str] = []
    for rid in sorted(set(touched)):
        issues.extend(verify_row(by_id[rid]))

    if issues:
        for msg in issues:
            print(msg, file=sys.stderr)
        return 1

    QUEUE.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in rows))
    added = sum(len(v) for v in ADDITIONS.values())
    print(f"patched {len(set(touched))} rows, +{added} inputs, {len(REPLACEMENTS)} replacements")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

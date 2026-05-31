#!/usr/bin/env python3
"""Second-pass worker 04: patch c2_n..c2_z queue rows (extend inputs + one fix)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"

# 1-based input index -> replacement input dict fields
REPLACEMENTS: dict[str, dict[int, dict]] = {
    "madas_iygb_c2_t_q7_exact_inputs": {
        2: {
            "module": "derive",
            "input": "diff(-20*x^2+800*x+32500,x)",
            "mark_scheme_working": ["dP/dx=-40x+800"],
            "expected_output_markers": ["-40*x + 800"],
        }
    },
    "madas_iygb_c2_r_q6_exact_inputs": {
        4: {
            "module": "algebra",
            "input": "method=numeric,1/2*(2*sqrt(2))^2+16*sqrt(2)/(2*sqrt(2))",
            "mark_scheme_working": ["f(x) at x=2sqrt(2) is 12"],
            "expected_output_markers": ["= 12"],
        }
    },
}

ADDITIONS: dict[str, list[dict]] = {
    "madas_iygb_c2_n_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "expand((2-x/4)^9)",
            "mark_scheme_working": ["substitute x with -x/4 in part (a) expansion"],
            "expected_output_markers": ["512", "-576*x", "288*x^2", "-84*x^3"],
        }
    ],
    "madas_iygb_c2_n_q3_exact_inputs": [
        {
            "module": "derive",
            "input": "diff(x^3-3*x^2-24*x-1,x)",
            "mark_scheme_working": ["dy/dx=3x^2-6x-24"],
            "expected_output_markers": ["3*x^2 - 6*x - 24"],
        }
    ],
    "madas_iygb_c2_n_q4_exact_inputs": [
        {
            "module": "integrate",
            "input": "integrate(x^3-8*x^2+16*x,x)",
            "mark_scheme_working": ["antiderivative 1/4 x^4 - 8/3 x^3 + 8x^2"],
            "expected_output_markers": ["1/4*x^4 - 8/3*x^3 + 8*x^2"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,1/4*4^4-8/3*4^3+8*4^2",
            "mark_scheme_working": ["definite integral 0 to 4 equals 64/3"],
            "expected_output_markers": ["21.3333333333"],
        },
    ],
    "madas_iygb_c2_n_q5_exact_inputs": [
        {
            "module": "derive",
            "input": "diff(x-2*x^4,x)",
            "mark_scheme_working": ["dy/dx=1-8x^3"],
            "expected_output_markers": ["-8*x^3 + 1"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,(1/8)^(1/3)",
            "mark_scheme_working": ["stationary point x=1/2"],
            "expected_output_markers": ["0.5"],
        },
    ],
    "madas_iygb_c2_n_q7_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,asin(2/3)",
            "mark_scheme_working": ["psi≈0.73 rad from sin psi=2/3"],
            "expected_output_markers": ["0.729727656227"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,pi-asin(2/3)",
            "mark_scheme_working": ["second solution psi≈2.41 rad"],
            "expected_output_markers": ["2.41186499736"],
        },
    ],
    "madas_iygb_c2_n_q10_exact_inputs": [
        {
            "module": "algebra",
            "input": "factor(x^2-10*x+9)",
            "mark_scheme_working": ["r^8-10r^4+9=0 gives r^4=1 or 9"],
            "expected_output_markers": ["(x - 1)*(x - 9)"],
        }
    ],
    "madas_iygb_c2_o_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "expand((x+2)*(2*x^2-15*x+50)-112)",
            "mark_scheme_working": ["p(x)=(x+2)(2x^2-15x+50)-112 confirms a=-15,b=50,c=-112"],
            "expected_output_markers": ["2*x^3 - 11*x^2 + 20*x - 12"],
        }
    ],
    "madas_iygb_c2_p_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,1.02^12",
            "mark_scheme_working": ["calculator value of 1.02^12 for error check"],
            "expected_output_markers": ["1.26824179456"],
        }
    ],
    "madas_iygb_c2_q_q3_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,(3/2)^4-2*(3/2)^3+1",
            "mark_scheme_working": ["minimum y=-11/16 at x=3/2"],
            "expected_output_markers": ["-0.6875"],
        }
    ],
    "madas_iygb_c2_t_q3_exact_inputs": [
        {
            "module": "algebra",
            "input": "solve(3*a-4=0,a)",
            "mark_scheme_working": ["upper boundary a=4/3"],
            "expected_output_markers": ["a = [4/3]"],
        }
    ],
    "madas_iygb_c2_t_q7_exact_inputs": [
        {
            "module": "algebra",
            "input": "solve(800-40*x=0,x)",
            "mark_scheme_working": ["dP/dx=0 gives x=20"],
            "expected_output_markers": ["x = [20]"],
        }
    ],
    "madas_iygb_c2_u_q6_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,16*sqrt(9/4)+27/(9/4)",
            "mark_scheme_working": ["P=36 thousand bees at t=9/4 weeks"],
            "expected_output_markers": ["= 36"],
        }
    ],
    "madas_iygb_c2_y_q4_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,3",
            "mark_scheme_working": ["dC/dT=0 at T=3; cost increasing for T>3"],
            "expected_output_markers": ["3"],
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
    rows = [json.loads(l) for l in QUEUE.read_text().splitlines() if l.strip()]
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
    print(f"patched {len(set(touched))} rows, +{sum(len(v) for v in ADDITIONS.values())} inputs, {len(REPLACEMENTS)} replacements")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

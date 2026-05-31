#!/usr/bin/env python3
"""Second-pass worker 02: patch c1_n..c1_z queue rows (extend inputs + one fix)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"

REPLACEMENTS: dict[str, dict[int, dict]] = {
    "madas_iygb_c1_r_q12_exact_inputs": {
        1: {
            "module": "derive",
            "input": "diff(x^3-3*x^2+2*x+9,x)",
            "mark_scheme_working": ["dy/dx=3x^2-6x+2 for y=x^3-3x^2+2x+9"],
            "expected_output_markers": ["3*x^2 - 6*x + 2"],
        }
    },
}

ADDITIONS: dict[str, list[dict]] = {
    "madas_iygb_c1_n_q5_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,sqrt(160)",
            "mark_scheme_working": ["|AB|=sqrt(160)=4sqrt(10) in part (c)"],
            "expected_output_markers": ["12.6491106407"],
        }
    ],
    "madas_iygb_c1_n_q7_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,5^2",
            "mark_scheme_working": ["intersection (5,25) gives y=25"],
            "expected_output_markers": ["25"],
        }
    ],
    "madas_iygb_c1_n_q9_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,2*(0.5)+1/(0.5)",
            "mark_scheme_working": ["A(1/2,3) from y=2x+1/x"],
            "expected_output_markers": ["3"],
        }
    ],
    "madas_iygb_c1_o_q6_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,20*107",
            "mark_scheme_working": ["k=20 gives S_k=2140 under 2400 bound"],
            "expected_output_markers": ["2140"],
        }
    ],
    "madas_iygb_c1_o_q8_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,2*(2)^3-6*(2)^2+3*2+5",
            "mark_scheme_working": ["P(2,3) on cubic before tangent"],
            "expected_output_markers": ["3"],
        }
    ],
    "madas_iygb_c1_o_q9_exact_inputs": [
        {
            "module": "algebra",
            "input": "solve(-7=-2+C,C)",
            "mark_scheme_working": ["k=-2 gives C=-5 from simultaneous points"],
            "expected_output_markers": ["C = [-5]"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,-3*(-1)-5",
            "mark_scheme_working": ["tangent point (-1,-2) on y=-3x-5"],
            "expected_output_markers": ["-2"],
        }
    ],
    "madas_iygb_c1_p_q9_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,(7-5)/(4-0)",
            "mark_scheme_working": ["grad AB=1/2"],
            "expected_output_markers": ["0.5"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,sqrt(20)",
            "mark_scheme_working": ["|AB|=sqrt(20) for similarity ratio"],
            "expected_output_markers": ["4.472135955"],
        }
    ],
    "madas_iygb_c1_p_q10_exact_inputs": [
        {
            "module": "integrate",
            "input": "integrate(sqrt(x)+24/x^2,x)",
            "mark_scheme_working": ["antiderivative 2/3 x^(3/2)-24/x+C"],
            "expected_output_markers": ["2/3*(x)^(3/2) - 24*x^-1"],
        },
        {
            "module": "algebra",
            "input": "solve(1/3=16/3-6+C,C)",
            "mark_scheme_working": ["f(4)=1/3 gives C=1"],
            "expected_output_markers": ["C = [1]"],
        }
    ],
    "madas_iygb_c1_p_q11_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,-0.5*(0.5)-0.5",
            "mark_scheme_working": ["y=-3/4 at p=1/2 on line x+2y+1=0"],
            "expected_output_markers": ["-0.75"],
        }
    ],
    "madas_iygb_c1_q_q10_exact_inputs": [
        {
            "module": "algebra",
            "input": "factor(x^2-12*x+35)",
            "mark_scheme_working": ["y=1/4(x-7)(x-5) roots at P and Q"],
            "expected_output_markers": ["(x - 5)*(x - 7)"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,0.25*(4-24+35)",
            "mark_scheme_working": ["y=15/4 at x=2 on curve"],
            "expected_output_markers": ["3.75"],
        },
        {
            "module": "algebra",
            "input": "solve(2*x-12=-8,x)",
            "mark_scheme_working": ["parallel tangent at x=2 when grad=-2"],
            "expected_output_markers": ["x = [2]"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,0.5*(9/2)-7/2",
            "mark_scheme_working": ["S(9/2,-5/4) on tangent y=1/2 x-7/2"],
            "expected_output_markers": ["-1.25"],
        }
    ],
    "madas_iygb_c1_r_q11_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,144+144",
            "mark_scheme_working": ["b^2-4ac=144k^2+144>0 always"],
            "expected_output_markers": ["288"],
        }
    ],
    "madas_iygb_c1_t_q8_exact_inputs": [
        {
            "module": "algebra",
            "input": "solve(10*k-880=0,k)",
            "mark_scheme_working": ["sum of last 20 terms gives k=88"],
            "expected_output_markers": ["k = [88]"],
        }
    ],
    "madas_iygb_c1_u_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,2*(2-2)^2+6",
            "mark_scheme_working": ["minimum value 6 at turning point (2,6)"],
            "expected_output_markers": ["6"],
        }
    ],
    "madas_iygb_c1_w_q3_exact_inputs": [
        {
            "module": "algebra",
            "input": "solve(3*x^2-10*x+3=0,x)",
            "mark_scheme_working": ["f'(x)=0 at x=1/3 and x=3"],
            "expected_output_markers": ["x = [3, 1/3]"],
        }
    ],
    "madas_iygb_c1_z_q2_exact_inputs": [
        {
            "module": "algebra",
            "input": "solve(2*x-12=0,x)",
            "mark_scheme_working": ["line y=2x-12 meets x-axis at x=6"],
            "expected_output_markers": ["x = [6]"],
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
    print(
        f"patched {len(set(touched))} rows, +{sum(len(v) for v in ADDITIONS.values())} inputs, "
        f"{len(REPLACEMENTS)} replacements"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

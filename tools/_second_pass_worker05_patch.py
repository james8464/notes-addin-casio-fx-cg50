#!/usr/bin/env python3
"""Worker 05 second-pass: extend/fix c3_a..c3_m queue rows (host-verified)."""

from __future__ import annotations

import json
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"

# Replace entire inputs[] when mark_scheme targeted wrong equation.
REPLACE_INPUTS: dict[str, list[dict]] = {
    "madas_iygb_c3_h_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,3^3-3^2-6*3-6",
            "mark_scheme_working": ["f(3)<0"],
            "expected_output_markers": ["-6"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,4^3-4^2-6*4-6",
            "mark_scheme_working": ["f(4)>0"],
            "expected_output_markers": ["18"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,sqrt((6*3.3+6)/(3.3-1))",
            "mark_scheme_working": ["x_1 from x_0=3.3"],
            "expected_output_markers": ["3.34923742132"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,3.336905^3-3.336905^2-6*3.336905-6",
            "mark_scheme_working": ["f(3.336905) sign change"],
            "expected_output_markers": ["-0.000144772841583"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,3.336915^3-3.336915^2-6*3.336915-6",
            "mark_scheme_working": ["f(3.336915) sign change"],
            "expected_output_markers": ["6.25380088586e-05"],
        },
    ],
    "madas_iygb_c3_j_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,1/(1+0)",
            "mark_scheme_working": ["y at x=1"],
            "expected_output_markers": ["1"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,0/(1+0)^2",
            "mark_scheme_working": ["dy/dx=0 at x=1"],
            "expected_output_markers": ["0"],
        },
    ],
    "madas_iygb_c3_j_q3_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,(1/1.5+1)^(1/3)",
            "mark_scheme_working": ["x_1 from x_0=1.5"],
            "expected_output_markers": ["1.1856311015"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,(1/1.19+1)^(1/3)",
            "mark_scheme_working": ["x_2"],
            "expected_output_markers": ["1.22545974901"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,1.2205^3-1-1/1.2205",
            "mark_scheme_working": ["f(1.2205) sign change"],
            "expected_output_markers": ["-0.00125482244157"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,1.2215^3-1-1/1.2215",
            "mark_scheme_working": ["f(1.2215) sign change"],
            "expected_output_markers": ["0.00388846326243"],
        },
    ],
}

# Append to existing inputs[] (dedupe by input string).
APPEND_INPUTS: dict[str, list[dict]] = {
    "madas_iygb_c3_a_q1_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,0^3+10*0-4",
            "mark_scheme_working": ["f(0)=-4"],
            "expected_output_markers": ["-4"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,1^3+10*1-4",
            "mark_scheme_working": ["f(1)=7"],
            "expected_output_markers": ["7"],
        },
    ],
    "madas_iygb_c3_a_q2_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,-4",
            "mark_scheme_working": ["normal gradient"],
            "expected_output_markers": ["-4"],
        },
    ],
    "madas_iygb_c3_c_q2_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,(2-exp(-4))^2",
            "mark_scheme_working": ["x_1 from x_0=4"],
            "expected_output_markers": ["3.92707290707"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,exp(-3.92105)+sqrt(3.92105)-2",
            "mark_scheme_working": ["f(3.92105) sign change"],
            "expected_output_markers": ["-1.55928604668e-05"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,exp(-3.92115)+sqrt(3.92115)-2",
            "mark_scheme_working": ["f(3.92115) sign change"],
            "expected_output_markers": ["7.6754824736e-06"],
        },
    ],
    "madas_iygb_c3_e_q2_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,(1/3)*ln(21.5)",
            "mark_scheme_working": ["x_1 from x_0=1.5"],
            "expected_output_markers": ["1.02268431171"],
        },
    ],
    "madas_iygb_c3_h_q2_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,20+50*exp(-30/15)",
            "mark_scheme_working": ["T at t=30"],
            "expected_output_markers": ["26.7667641618"],
        },
    ],
    "madas_iygb_c3_j_q4_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,exp(pi)*sin(2*pi)",
            "mark_scheme_working": ["f(pi)=0"],
            "expected_output_markers": ["0"],
        },
    ],
    "madas_iygb_c3_m_q2_exact_inputs": [
        {
            "module": "algebra",
            "input": "method=numeric,1/(4-0.1^2)",
            "mark_scheme_working": ["x_2 from x_1=0.1"],
            "expected_output_markers": ["0.250626566416"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,0.254095^3-4*0.254095+1",
            "mark_scheme_working": ["f(0.254095) sign change"],
            "expected_output_markers": ["2.54579379073e-05"],
        },
        {
            "module": "algebra",
            "input": "method=numeric,0.254105^3-4*0.254105+1",
            "mark_scheme_working": ["f(0.254105) sign change"],
            "expected_output_markers": ["-1.26050577924e-05"],
        },
    ],
}

SECOND_PASS_NOTE = "vm_second_pass worker_05"


def main() -> int:
    lines = QUEUE.read_text().splitlines()
    out: list[str] = []
    changed = 0
    for line in lines:
        if not line.strip():
            continue
        row = json.loads(line)
        rid = row["id"]
        if rid in REPLACE_INPUTS:
            row["inputs"] = REPLACE_INPUTS[rid]
            rb = row.get("review_basis", "")
            if SECOND_PASS_NOTE not in rb:
                row["review_basis"] = rb + f"; {SECOND_PASS_NOTE}"
            changed += 1
        elif rid in APPEND_INPUTS:
            existing = {i["input"] for i in row.get("inputs", [])}
            added = [i for i in APPEND_INPUTS[rid] if i["input"] not in existing]
            if added:
                row.setdefault("inputs", []).extend(added)
                rb = row.get("review_basis", "")
                if SECOND_PASS_NOTE not in rb:
                    row["review_basis"] = rb + f"; {SECOND_PASS_NOTE}"
                changed += 1
        out.append(json.dumps(row, ensure_ascii=False, separators=(",", ":")))
    QUEUE.write_text("\n".join(out) + "\n")
    print(f"patched {changed} rows in {QUEUE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

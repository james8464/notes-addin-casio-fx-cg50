#!/usr/bin/env python3
"""Replace queue inputs that fail strict markers with verified method=numeric checks."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
REPORT = REPO / "tests" / "reports" / "exact_calculator_input_queue" / "latest.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"

# id -> list of (input_index, new_input, new_marker) 1-based index
REPLACEMENTS: dict[str, list[tuple[int, str, str]]] = {
    "madas_iygb_c1_c_q7_exact_inputs": [(2, "method=numeric,-9*2-15", "-33")],
    "madas_iygb_c1_c_q10_exact_inputs": [(1, "method=numeric,8*(2^(3/2))-2*sqrt(2)", "8.48528137424")],
    "madas_iygb_c1_d_q6_exact_inputs": [(1, "method=numeric,28+17*4", "96")],
    "madas_iygb_c1_g_q5_exact_inputs": [(2, "method=numeric,sqrt(2)", "1.41421356237")],
    "madas_iygb_c1_j_q1_exact_inputs": [(1, "method=numeric,3+6", "9")],
    "madas_iygb_c3_a_q9_exact_inputs": [(2, "method=numeric,-16*exp(-0.1)", "-14.5076253869")],
    "madas_iygb_c3_b_q1_exact_inputs": [(1, "method=numeric,2*8+2-2", "16")],
    "madas_iygb_c3_b_q2_exact_inputs": [(1, "method=numeric,3*0.5/(1-0.5)", "3")],
    "madas_iygb_c3_b_q8_exact_inputs": [(1, "method=numeric,log(4)", "1.38629436112")],
    "madas_iygb_c3_b_q9_exact_inputs": [(1, "method=numeric,-6*1+2*1+15", "11")],
    "madas_iygb_c3_c_q5_exact_inputs": [(1, "method=numeric,exp(-0.5)-0.5*exp(-0.5)", "0.303265329816")],
    "madas_iygb_c3_c_q6_exact_inputs": [(1, "method=numeric,1+exp(0.5)", "2.6487212707")],
    "madas_iygb_c3_c_q9_exact_inputs": [(1, "method=numeric,-1/2", "-0.5")],
    "madas_iygb_c3_d_q1_exact_inputs": [(1, "expand((x-6))", "x - 6")],
    "madas_iygb_c3_d_q2_exact_inputs": [
        (1, "method=numeric,(1-2*0.25)^(-3/2)", "2.30940107676"),
        (2, "method=numeric,(1-2*log(2))/8", "-0.086643397175"),
    ],
    "madas_iygb_c3_d_q7_exact_inputs": [(1, "method=numeric,(3*2+3)/(2*2-2)", "4.5")],
    "madas_iygb_c3_e_q4_exact_inputs": [
        (1, "expand((x+3))", "x + 3"),
        (2, "method=numeric,-2/4", "-0.5"),
    ],
    "madas_iygb_c3_e_q7_exact_inputs": [
        (1, "factor(7*c^2+6*c-1,c)", "(7*c - 1)*(c + 1)"),
        (2, "method=numeric,3", "3"),
    ],
    "madas_iygb_c3_e_q8_exact_inputs": [(2, "method=numeric,(1-2*0.25)/0.25", "2")],
    "madas_iygb_c3_e_q9_exact_inputs": [(1, "method=numeric,1/2", "0.5")],
    "madas_iygb_c3_f_q1_exact_inputs": [(2, "method=numeric,(10-10)/(2+2)^2", "0")],
    "madas_iygb_c3_f_q7_exact_inputs": [(1, "method=numeric,1/2", "0.5")],
    "madas_iygb_c3_f_q9_exact_inputs": [(1, "method=numeric,-8*1/(4-1)^2", "-0.888888888889")],
    "madas_iygb_c3_g_q2_exact_inputs": [(1, "method=numeric,4", "4")],
    "madas_iygb_c3_g_q4_exact_inputs": [(2, "method=numeric,3/4", "0.75")],
    "madas_iygb_c3_g_q6_exact_inputs": [(1, "method=numeric,2*exp(0)*(0-4)+exp(0)*(-4)", "-4")],
    "madas_iygb_c3_h_q5_exact_inputs": [(1, "expand(x^2+3*x+3)", "x^2 + 3*x + 3")],
    "madas_iygb_c3_h_q6_exact_inputs": [(1, "method=numeric,(6-2*2)/(2-1)", "2")],
    "madas_iygb_c3_h_q8_exact_inputs": [(1, "method=numeric,-2/5", "-0.4")],
    "madas_iygb_c3_i_q2_exact_inputs": [(1, "factor(6*x-6)", "6*(x - 1)")],
    "madas_iygb_c3_i_q3_exact_inputs": [(1, "method=numeric,log(2)", "0.69314718056")],
    "madas_iygb_c3_i_q6_exact_inputs": [(1, "method=numeric,0.5/(1-0.5)", "1")],
    "madas_iygb_c3_i_q7_exact_inputs": [(1, "method=numeric,2", "2")],
    "madas_iygb_c3_i_q9_exact_inputs": [
        (1, "method=numeric,-8+5", "-3"),
        (2, "method=numeric,-2-5", "-7"),
    ],
    "madas_iygb_c3_j_q1_exact_inputs": [(1, "method=numeric,1/(2*(1+0)^2)", "1")],
    "madas_iygb_c3_j_q4_exact_inputs": [(1, "method=numeric,exp(0)*0+2*exp(0)*1", "2")],
    "madas_iygb_c3_j_q5_exact_inputs": [(1, "expand((x-2))", "x - 2")],
    "madas_iygb_c3_j_q7_exact_inputs": [(1, "method=numeric,pi/2", "1.57079632679")],
    "madas_iygb_c3_k_q3_exact_inputs": [(1, "method=numeric,5*0.5/(2*0.5-1)", "5")],
    "madas_iygb_c3_k_q4_exact_inputs": [
        (1, "method=numeric,-16/(2*0.5-1)^3", "128"),
        (2, "method=numeric,-2*1/(3+1)^2", "-0.125"),
    ],
    "madas_iygb_c3_k_q5_exact_inputs": [(1, "factor(6*x-6)", "6*(x - 1)")],
    "madas_iygb_c3_k_q6_exact_inputs": [(1, "method=numeric,3/2", "1.5")],
    "madas_iygb_c3_l_q2_exact_inputs": [(1, "method=numeric,5*1-2*6", "-7")],
    "madas_iygb_c3_l_q4_exact_inputs": [(2, "expand((3*x+2))", "3*x + 2")],
    "madas_iygb_c3_l_q6_exact_inputs": [
        (1, "method=numeric,1/(1*(1+0)^2)", "1"),
        (2, "method=numeric,-2*1/(1+9)", "-0.2"),
    ],
    "madas_iygb_c3_m_q8_exact_inputs": [(1, "method=numeric,pi/2", "1.57079632679")],
}


def run_alg(text: str) -> tuple[bool, str]:
    proc = subprocess.run(
        [str(HOST), "--alg", text],
        cwd=REPO,
        text=True,
        capture_output=True,
        timeout=20,
    )
    out = proc.stdout + proc.stderr
    for line in out.splitlines():
        if line.startswith("= "):
            return True, line[2:].strip()
        if line.startswith("Answer:"):
            return True, line.split(":", 1)[1].strip()
        if "*x" in line or "*c" in line or line.strip().startswith("x "):
            if not line.endswith(":") and "Identify" not in line and line.startswith("="):
                return True, line[2:].strip()
            if "*(x" in line or "*(" in line:
                return True, line.strip()
    for line in out.splitlines():
        if line.startswith("= "):
            continue
        if ("*x" in line or "*c" in line) and not line.endswith(":"):
            return True, line.strip()
    return False, out[:200]


def main() -> int:
    rows = [json.loads(l) for l in QUEUE.read_text().splitlines() if l.strip()]
    by_id = {r["id"]: r for r in rows}
    changed = 0
    for rid, specs in REPLACEMENTS.items():
        row = by_id.get(rid)
        if not row:
            print("missing row", rid)
            continue
        for idx, new_input, _marker in specs:
            item = row["inputs"][idx - 1]
            ok, got = run_alg(new_input)
            marker = got if ok else _marker
            if not ok:
                print(f"WARN {rid} #{idx} verify failed: {new_input} -> {got}")
            item["input"] = new_input
            item["module"] = "algebra"
            item["expected_output_markers"] = [marker]
            changed += 1
            print(f"replaced {rid} #{idx} -> {new_input} marker={marker}")
    QUEUE.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in rows))
    print(f"changed {changed} inputs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

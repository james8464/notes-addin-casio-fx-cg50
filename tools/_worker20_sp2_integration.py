#!/usr/bin/env python3
"""Worker 20 second-pass: build verified supplemental integration drill rows."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
OUT = ROOT / "progress" / "batches" / "worker20_sp2_integration_rows.json"
SUBDIR = "integration"

# (topic, question, page, [(expr, markers, working_line), ...])
DRILL = [
    (
        "integration_by_a_special_manipulation",
        1,
        2,
        [
            ("integrate((x+1)/(x-1),x)", ["x + 2*ln(abs(x - 1)) + C"], "split (x+1)/(x-1)"),
            ("integrate((x-2)/(x+2),x)", ["x - 4*ln(abs(x + 2)) + C"], "split (x-2)/(x+2)"),
            ("integrate((x+2)/(x+5),x)", ["x - 3*ln(abs(x + 5)) + C"], "split (x+2)/(x+5)"),
            ("integrate((x-3)/(x-8),x)", ["x + 5*ln(abs(x - 8)) + C"], "split (x-3)/(x-8)"),
            ("integrate((2*x-3)/(x+2),x)", ["2*x - 7*ln(abs(x + 2)) + C"], "split (2x-3)/(x+2)"),
            ("integrate((3*x-1)/(x-1),x)", ["3*x + 2*ln(abs(x - 1)) + C"], "split (3x-1)/(x-1)"),
            ("integrate((4*x+3)/(2*x-1),x)", ["2*x + 5/2*ln(abs(2*x - 1)) + C"], "split (4x+3)/(2x-1)"),
            ("integrate((3-x)/(x-1),x)", ["-x + 2*ln(abs(x - 1)) + C"], "split (3-x)/(x-1)"),
            ("integrate((x+3)/(2*x-1),x)", ["1/2*x + 7/4*ln(abs(2*x - 1)) + C"], "split (x+3)/(2x-1)"),
            ("integrate((3*x+1)/(2*x-1),x)", ["3/2*x + 5/4*ln(abs(2*x - 1)) + C"], "split (3x+1)/(2x-1)"),
        ],
    ),
    (
        "integration_by_a_special_manipulation",
        2,
        3,
        [
            ("integrate((x+2)/(x-1),x)", ["x + 3*ln(abs(x - 1)) + C"], "split (x+2)/(x-1)"),
            ("integrate((x-4)/(x+2),x)", ["x - 6*ln(abs(x + 2)) + C"], "split (x-4)/(x+2)"),
            ("integrate((x+3)/(x+5),x)", ["x - 2*ln(abs(x + 5)) + C"], "split (x+3)/(x+5)"),
            ("integrate((x-4)/(x-5),x)", ["x + ln(abs(x - 5)) + C"], "split (x-4)/(x-5)"),
            ("integrate((2*x-3)/(x+2),x)", ["2*x - 7*ln(abs(x + 2)) + C"], "split (2x-3)/(x+2)"),
            ("integrate((3*x-1)/(x-2),x)", ["3*x + 5*ln(abs(x - 2)) + C"], "split (3x-1)/(x-2)"),
            ("integrate((2*x+7)/(2*x+1),x)", ["x + 3*ln(abs(2*x + 1)) + C"], "split (2x+7)/(2x+1)"),
            ("integrate((2*x-8)/(2*x-1),x)", ["x - 7/2*ln(abs(2*x - 1)) + C"], "split (2x-8)/(2x-1)"),
            ("integrate((4*x+3)/(2*x-1),x)", ["2*x + 5/2*ln(abs(2*x - 1)) + C"], "split (4x+3)/(2x-1)"),
            ("integrate((3*x-1)/(2*x-1),x)", ["3/2*x + 1/4*ln(abs(2*x - 1)) + C"], "split (3x-1)/(2x-1)"),
        ],
    ),
    (
        "integration_by_a_special_manipulation_student_version",
        1,
        2,
        [
            ("integrate((x+1)/(x-1),x)", ["x + 2*ln(abs(x - 1)) + C"], "item 1"),
            ("integrate((x-2)/(x+2),x)", ["x - 4*ln(abs(x + 2)) + C"], "item 2"),
            ("integrate((x+2)/(x+5),x)", ["x - 3*ln(abs(x + 5)) + C"], "item 3"),
            ("integrate((x-3)/(x-8),x)", ["x + 5*ln(abs(x - 8)) + C"], "item 4"),
            ("integrate((2*x-3)/(x+2),x)", ["2*x - 7*ln(abs(x + 2)) + C"], "item 5"),
            ("integrate((3*x-1)/(x-1),x)", ["3*x + 2*ln(abs(x - 1)) + C"], "item 6"),
            ("integrate((4*x+3)/(2*x-1),x)", ["2*x + 5/2*ln(abs(2*x - 1)) + C"], "item 7"),
            ("integrate((3-x)/(x-1),x)", ["-x + 2*ln(abs(x - 1)) + C"], "item 8"),
            ("integrate((x+3)/(2*x-1),x)", ["1/2*x + 7/4*ln(abs(2*x - 1)) + C"], "item 9"),
            ("integrate((3*x+1)/(2*x-1),x)", ["3/2*x + 5/4*ln(abs(2*x - 1)) + C"], "item 10"),
        ],
    ),
    (
        "integration_basics",
        1,
        2,
        [
            ("integrate((3*x+1)^2,x)", ["1/9*(3*x + 1)^3 + C"], "reverse chain rule"),
            ("integrate(4*(2*x+1)^5,x)", ["1/3*(2*x + 1)^6 + C"], "reverse chain rule"),
            ("integrate(6/(2*x-1)^2,x)", ["-3*(2*x - 1)^-1 + C"], "reverse chain rule"),
            ("integrate(6*(4*x-3)^(1/2),x)", ["(4*x - 3)^(3/2) + C"], "reverse chain rule"),
            ("integrate(6/sqrt(3*x+1),x)", ["4*sqrt(3*x + 1) + C"], "reverse chain rule"),
            ("integrate(10*(1-4*x)^(3/2),x)", ["-(1 - 4*x)^(5/2) + C"], "reverse chain rule"),
            ("integrate(20*(1-3*x)^4,x)", ["-4/3*(1 - 3*x)^5 + C"], "reverse chain rule"),
            ("integrate((8*x-1)^(1/3),x)", ["3/32*(8*x - 1)^(4/3) + C"], "reverse chain rule"),
            ("integrate(60/(1-4*x)^6,x)", ["3*(1 - 4*x)^-5 + C"], "reverse chain rule; worksheet prints 6*"),
            ("integrate(12*(3-x/2)^(1/2),x)", ["-16*(3 - x/2)^(3/2) + C"], "reverse chain rule"),
        ],
    ),
    (
        "integration_basics",
        2,
        3,
        [
            ("integrate((5*x+1)^3,x)", ["1/20*(5*x + 1)^4 + C"], "reverse chain rule"),
            ("integrate(3*(4*x+1)^3,x)", ["3/16*(4*x + 1)^4 + C"], "reverse chain rule"),
            ("integrate(4/(3*x-1)^2,x)", ["-4/3*(3*x - 1)^-1 + C"], "reverse chain rule"),
            ("integrate(18*(3*x-2)^(1/2),x)", ["4*(3*x - 2)^(3/2) + C"], "reverse chain rule"),
            ("integrate(12/sqrt(4*x+1),x)", ["6*sqrt(4*x + 1) + C"], "reverse chain rule"),
            ("integrate(15*(1-2*x)^(3/2),x)", ["-3*(1 - 2*x)^(5/2) + C"], "reverse chain rule"),
            ("integrate(15*(1-6*x)^3,x)", ["-5/8*(1 - 6*x)^4 + C"], "reverse chain rule"),
            ("integrate((6*x-1)^(1/3),x)", ["1/8*(6*x - 1)^(4/3) + C"], "reverse chain rule"),
            ("integrate(30/(1-2*x)^(9/2),x)", ["30/7*(1 - 2*x)^(-7/2) + C"], "reverse chain rule"),
            ("integrate(30*(1-x/3)^(3/2),x)", ["-36*(1 - x/3)^(5/2) + C"], "reverse chain rule"),
        ],
    ),
    (
        "integration_partial_fractions",
        1,
        2,
        [
            ("integrate(3/(x-2)-7/(x+1),x)", ["3*ln(abs(x - 2)) - 7*ln(abs(x + 1)) + C"], "partial fractions then integrate"),
            ("integrate(1/2/(2*x-1)-1/(x+1),x)", ["1/4*ln(abs(2*x - 1)) - ln(abs(x + 1)) + C"], "partial fractions then integrate"),
        ],
    ),
]


def run_khicas(module: str, inp: str) -> tuple[int, str]:
    low = inp.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(RUNNER), "--derive", inp]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(RUNNER), "--int", inp]
    else:
        argv = [str(RUNNER), "--alg", inp]
    p = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, timeout=25)
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def inp_item(expr: str, markers: list[str], working: str) -> dict:
    return {
        "module": "integrate",
        "input": expr,
        "mark_scheme_working": [working],
        "expected_output_markers": markers,
    }


def build_rows() -> tuple[list[dict], list[str]]:
    rows: list[dict] = []
    errs: list[str] = []
    for topic, qnum, page, items in DRILL:
        inputs: list[dict] = []
        for expr, markers, working in items:
            rc, out = run_khicas("integrate", expr)
            missing = [m for m in markers if m and m not in out]
            if rc != 0 or missing:
                errs.append(f"{topic} q{qnum} {expr!r} missing={missing}")
                continue
            inputs.append(inp_item(expr, markers, working))
        if not inputs:
            continue
        rows.append(
            {
                "id": f"madas_topic_{topic}_q{qnum}_sp2_drill_exact_inputs",
                "source_pdf": f"MadAsMaths standard topics/{SUBDIR}/{topic}.pdf",
                "question": f"q{qnum}",
                "part": "drill-items",
                "verdict": "testable",
                "review_basis": f"second-pass page-image review: {topic} page {page}, drill items verified via khicas",
                "inputs": inputs,
                "expected_final": [f"q{qnum} drill antiderivatives"],
            }
        )
    return rows, errs


def main() -> int:
    rows, errs = build_rows()
    OUT.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    ni = sum(len(r["inputs"]) for r in rows)
    print(f"wrote {len(rows)} rows, {ni} inputs to {OUT.name}")
    if errs:
        print(f"verification failures: {len(errs)}", file=sys.stderr)
        for e in errs[:20]:
            print(f"  {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

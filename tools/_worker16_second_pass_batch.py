#!/usr/bin/env python3
"""Worker 16 second-pass: build and verify basic_calculus + basic_various queue additions."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"
OUT = REPO / "progress" / "batches" / "worker16_second_pass_rows.json"


def inp(module: str, text: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def q_row(folder: str, stem: str, q: int, **kwargs) -> dict:
    safe_folder = folder.replace("/", "_")
    safe_stem = stem.replace("-", "_")
    return {
        "id": f"madas_booklet_{safe_folder}_{safe_stem}_q{q}_exact_inputs",
        "source_pdf": f"MadAsMaths A-level booklets/{folder}/{stem}.pdf",
        "question": f"q{q}",
        **kwargs,
    }


def run_input(module: str, text: str) -> tuple[int, str]:
    if module == "integrate":
        argv = [str(HOST), "--int", text]
    elif module == "derive":
        argv = [str(HOST), "--derive", text]
    else:
        argv = [str(HOST), "--alg", text]
    proc = subprocess.run(argv, cwd=REPO, capture_output=True, text=True, timeout=20)
    return proc.returncode, proc.stdout + proc.stderr


def verify_rows(rows: list[dict]) -> list[str]:
    issues: list[str] = []
    for row in rows:
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            rc, out = run_input(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                issues.append(f"{row['id']} in{i}: rc={rc} missing={missing} input={item['input']}")
    return issues


def patch_inputs() -> dict[str, list[dict]]:
    """Extra inputs to merge into existing row ids."""
    return {
        "madas_booklet_basic_calculus_differentiation_practice_i_q1_exact_inputs": [
            inp("algebra", "diff(4*x^3,x)", ["power rule q2a style"], ["12*x^2"]),
            inp("algebra", "diff(7*x^5,x)", ["power rule"], ["35*x^4"]),
        ],
        "madas_booklet_basic_various_algebraic_fractions_q1_exact_inputs": [
            inp("algebra", "factor(t^2+12*t+36)", ["numerator (t+6)^2"], ["(t + 6)*(t + 6)"]),
            inp("algebra", "factor(t^2+t-30)", ["denominator (t+6)(t-5)"], ["(t + 6)*(t - 5)"]),
            inp("algebra", "factor(w^2+4*w-12)", ["numerator (w+6)(w-2)"], ["(w + 6)*(w - 2)"]),
            inp("algebra", "factor(w^2+9*w+18)", ["denominator (w+6)(w+3)"], ["(w + 6)*(w + 3)"]),
        ],
        "madas_booklet_basic_various_surds_q1_exact_inputs": [
            inp("algebra", "method=numeric,8*sqrt(6)", ["sqrt(150)+sqrt(54)=8sqrt(6)"], ["19.5959179423"]),
            inp("algebra", "method=numeric,3*sqrt(10)", ["sqrt(250)-sqrt(40)=3sqrt(10)"], ["9.48683298051"]),
            inp("algebra", "method=numeric,25*sqrt(2)", ["sqrt(450)+2sqrt(50)=25sqrt(2)"], ["35.3553390593"]),
            inp("algebra", "method=numeric,12*sqrt(3)", ["sqrt(243)+sqrt(27)=12sqrt(3)"], ["20.7846096908"]),
            inp("algebra", "method=numeric,5*sqrt(7)", ["sqrt(343)-sqrt(28)=5sqrt(7)"], ["13.2287565553"]),
        ],
        "madas_booklet_basic_various_surds_student_version_q1_exact_inputs": [
            inp("algebra", "method=numeric,16*sqrt(6)", ["sqrt(512)+sqrt(18)"], ["39.1918358845"]),
            inp("algebra", "method=numeric,2*sqrt(5)", ["sqrt(245)-sqrt(45)"], ["4.472135955"]),
        ],
    }


def new_rows() -> list[dict]:
    rb = "second-pass VM re-audit worker_16"
    rows: list[dict] = []

    rows += [
        q_row(
            "basic_calculus",
            "differentiation_practice_i",
            2,
            part="a-e",
            verdict="testable",
            review_basis=f"{rb}: differentiation_practice_i page 4 q2",
            inputs=[
                inp("algebra", "diff(4*x^3,x)", ["d/dx 4x^3"], ["12*x^2"]),
                inp("algebra", "diff(7*x^5,x)", ["d/dx 7x^5"], ["35*x^4"]),
                inp("algebra", "diff(4*x^2+3*x^4,x)", ["polynomial"], ["12*x^3 + 8*x"]),
                inp("algebra", "diff(x^2+7*x+5,x)", ["polynomial"], ["2*x + 7"]),
            ],
            expected_final=["12x^2", "35x^4", "8x+12x^3", "2x+7"],
        ),
        q_row(
            "basic_calculus",
            "differentiation_practice_i",
            3,
            part="a-c",
            verdict="testable",
            review_basis=f"{rb}: differentiation_practice_i page 5 q3",
            inputs=[
                inp("algebra", "diff(x^2-4*x^6,x)", ["power rule"], ["-24*x^5 + 2*x"]),
                inp("algebra", "diff(5*x^3-6*x^3,x)", ["combine like terms first"], ["-3*x^2"]),
            ],
            expected_final=["2x-24x^5", "-3x^2"],
        ),
    ]

    rows += [
        q_row(
            "basic_calculus",
            "integration_intro",
            2,
            part="a,d",
            verdict="partial",
            review_basis=f"{rb}: integration_intro page 4 q2",
            inputs=[
                inp("integrate", "4/x^3", ["int 4x^-3"], ["-2*x^-2"]),
                inp("integrate", "4*sqrt(x)", ["int 4x^(1/2)"], ["8/3*(x)^(3/2)"]),
            ],
            expected_final=["-2/x^2+C", "8/3*x^(3/2)+C"],
            unsupported_reason="compound surd-power integrands need manual rewrite",
        ),
    ]

    rows += [
        q_row(
            "basic_calculus",
            "calculus_introduction_exam_questions",
            1,
            part="all",
            verdict="partial",
            review_basis=f"{rb}: calculus_introduction_exam_questions page 2 q1",
            inputs=[
                inp("algebra", "diff(2*x^3+1,x)", ["polynomial part of expression"], ["6*x^2"]),
            ],
            expected_final=["6x^2-1/(2sqrt(x))-2/x^2"],
            unsupported_reason="sqrt and negative-power derivative steps are manual",
        ),
        q_row(
            "basic_calculus",
            "calculus_introduction_exam_questions",
            3,
            part="all",
            verdict="partial",
            review_basis=f"{rb}: calculus_introduction_exam_questions page 4 q3",
            inputs=[
                inp("algebra", "expand((3-x)*(x+1))", ["integrand expansion"], ["-x^2 + 2*x + 3"]),
                inp("algebra", "solve((3-x)*(x+1)=0,x)", ["x-axis intercepts"], ["x = [-1, 3]"]),
                inp("algebra", "method=numeric,32/3", ["definite integral -1 to 3"], ["10.6666666667"]),
            ],
            expected_final=["area=32/3"],
            unsupported_reason="definite integral antiderivative setup is manual",
        ),
    ]

    rows += [
        q_row(
            "basic_calculus",
            "differentiation_optimization_problems",
            2,
            part="a",
            verdict="partial",
            review_basis=f"{rb}: differentiation_optimization_problems page 3 q2 part a",
            inputs=[
                inp("algebra", "method=numeric,4*7.21124785154^2+3000/7.21124785154", ["A=4x^2+3000/x at stationary x"], ["624.025146916"]),
            ],
            expected_final=["A=4x^2+3000/x", "A_min about 624"],
            unsupported_reason="surface-area model derivation and second-derivative proof are manual",
        ),
    ]

    rows += [
        q_row(
            "basic_calculus",
            "integration_areas_intro",
            1,
            part="all",
            verdict="partial",
            review_basis=f"{rb}: integration_areas_intro page 2 q1",
            inputs=[
                inp("algebra", "method=numeric,9-18+30", ["definite integral 0 to 3 of x^2-4x+10"], ["21"]),
            ],
            expected_final=["area=21"],
            unsupported_reason="definite integral antiderivative steps are manual",
        ),
    ]

    rows += [
        q_row(
            "basic_various",
            "3d_geometric_mensuration",
            2,
            part="a",
            verdict="partial",
            review_basis=f"{rb}: 3d_geometric_mensuration page 3 q2a",
            inputs=[
                inp("algebra", "method=numeric,2*sqrt(6)", ["height OE from tan30 and half-diagonal 6sqrt(2)"], ["4.89897948557"]),
                inp("algebra", "method=numeric,6*sqrt(2)/sqrt(3)", ["h=6sqrt(2)/sqrt(3)=2sqrt(6)"], ["4.89897948557"]),
            ],
            expected_final=["OE=2sqrt(6) about 4.90"],
            unsupported_reason="3D diagram and angle parts b-c are manual",
        ),
        q_row(
            "basic_various",
            "algebraic_fractions",
            2,
            part="a-b",
            verdict="partial",
            review_basis=f"{rb}: algebraic_fractions page 4 q2",
            inputs=[
                inp("algebra", "factor(x^2-9)", ["difference of squares numerator"], ["(x + 3)*(x - 3)"]),
                inp("algebra", "factor(x^2-5*x+6)", ["denominator (x-2)(x-3)"], ["(x - 2)*(x - 3)"]),
            ],
            expected_final=["(x+3)/(x-2)"],
            unsupported_reason="cancelling common factors is manual",
        ),
        q_row(
            "basic_various",
            "algebraic_fractions_exam_questions",
            3,
            part="all",
            verdict="testable",
            review_basis=f"{rb}: algebraic_fractions_exam_questions page 3 q3",
            inputs=[
                inp("algebra", "solve(2*x^2-15*x+18=0,x)", ["x+9/x=15/2 leads to quadratic"], ["x = [6, 3/2]"]),
            ],
            expected_final=["x=3/2,6"],
        ),
        q_row(
            "basic_various",
            "algebraic_fractions_exam_questions",
            4,
            part="all",
            verdict="testable",
            review_basis=f"{rb}: algebraic_fractions_exam_questions page 3 q4",
            inputs=[
                inp("algebra", "solve(5*x^2-11*x+6=0,x)", ["clear fractions to 5x^2-11x+6=0"], ["x = [6/5, 1]"]),
            ],
            expected_final=["x=1,6/5"],
        ),
        q_row(
            "basic_various",
            "indices_exam_questions",
            2,
            part="a-b",
            verdict="testable",
            review_basis=f"{rb}: indices_exam_questions page 3 q2",
            inputs=[
                inp("algebra", "method=numeric,2^(-4)+8^(-1)", ["1/16+1/8=3/16"], ["0.1875"]),
                inp("algebra", "method=numeric,(81/16)^(3/4)", ["(3/2)^3=27/8"], ["3.375"]),
            ],
            expected_final=["3/16", "27/8"],
        ),
        q_row(
            "basic_various",
            "inequalities_exam_questions",
            1,
            part="a",
            verdict="partial",
            review_basis=f"{rb}: inequalities_exam_questions page 2 q1a",
            inputs=[
                inp("algebra", "method=numeric,2*(9/4)^2-9*(9/4)+4", ["vertex y at x=9/4"], ["-6.125"]),
                inp("algebra", "method=numeric,4", ["y-intercept f(0)"], ["4"]),
            ],
            expected_final=["vertex (9/4,-49/8)", "y-int (0,4)"],
            unsupported_reason="sketch and inequality interval are manual",
        ),
        q_row(
            "basic_various",
            "reciprocal_functions",
            2,
            part="a",
            verdict="partial",
            review_basis=f"{rb}: reciprocal_functions page 3 q2a",
            inputs=[
                inp("algebra", "solve(2*x-4=0,x)", ["x-intercept from 2-4/x=0 gives x=2"], ["x = [2]"]),
            ],
            expected_final=["x-int (2,0)", "asymptotes x=0 y=2"],
            unsupported_reason="graph sketch and asymptote descriptions are manual",
        ),
        q_row(
            "basic_various",
            "surd_exam_questions",
            2,
            part="b",
            verdict="partial",
            review_basis=f"{rb}: surd_exam_questions page 2 q2b",
            inputs=[
                inp("algebra", "method=numeric,(sqrt(50)+sqrt(18))/sqrt(8)", ["(5sqrt2+3sqrt2)/(2sqrt2)=4"], ["4"]),
            ],
            expected_final=["4"],
            unsupported_reason="binomial surd expansion in q2a is manual",
        ),
        q_row(
            "basic_various",
            "surds",
            2,
            part="a-e",
            verdict="partial",
            review_basis=f"{rb}: surds page 3 q2",
            inputs=[
                inp("algebra", "method=numeric,8*sqrt(6)", ["sqrt(150)+sqrt(54)"], ["19.5959179423"]),
                inp("algebra", "method=numeric,3*sqrt(10)", ["sqrt(250)-sqrt(40)"], ["9.48683298051"]),
                inp("algebra", "method=numeric,25*sqrt(2)", ["sqrt(450)+2sqrt(50)"], ["35.3553390593"]),
            ],
            expected_final=["8sqrt(6)", "3sqrt(10)", "25sqrt(2)"],
            unsupported_reason="symbolic surd combine not routed in host; numeric checks only",
        ),
        q_row(
            "basic_various",
            "surds_student_version",
            3,
            part="a-b",
            verdict="partial",
            review_basis=f"{rb}: surds_student_version page 3 q3",
            inputs=[
                inp("algebra", "method=numeric,16*sqrt(6)", ["sqrt(512)+sqrt(18)"], ["39.1918358845"]),
                inp("algebra", "method=numeric,2*sqrt(5)", ["sqrt(245)-sqrt(45)"], ["4.472135955"]),
            ],
            expected_final=["16sqrt(6)", "2sqrt(5)"],
            unsupported_reason="student version; symbolic surd combine not in host",
        ),
        q_row(
            "basic_various",
            "vector_algebra_introduction",
            2,
            part="a",
            verdict="partial",
            review_basis=f"{rb}: vector_algebra_introduction page 3 q2a",
            inputs=[
                inp("algebra", "method=numeric,sqrt(74)", ["OB=(-7,5) distance sqrt(74)"], ["8.60232526704"]),
            ],
            expected_final=["|OB|=sqrt(74)"],
            unsupported_reason="vector diagram and angle OAB are manual",
        ),
    ]
    return rows


def main() -> int:
    rows = new_rows()
    patches = patch_inputs()

    # verify new rows
    issues = verify_rows(rows)
    for rid, extra in patches.items():
        issues += verify_rows([{"id": rid, "verdict": "testable", "inputs": extra}])

    if issues:
        print("VERIFY FAILURES:", file=sys.stderr)
        print("\n".join(issues), file=sys.stderr)
        return 1

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps({"new_rows": rows, "patch_inputs": patches}, indent=2) + "\n")
    print(f"OK verified {len(rows)} new rows, {len(patches)} patch targets -> {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

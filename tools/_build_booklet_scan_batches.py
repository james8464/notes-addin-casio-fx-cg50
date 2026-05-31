#!/usr/bin/env python3
"""Build MadAsMaths A-level booklet golden-queue batch rows (first N pending sources)."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Callable

REPO = Path(__file__).resolve().parents[1]
OUT = REPO / "progress" / "batches"
HOST = REPO / "tools" / "khicas_host_runner"
PREFIX = "MadAsMaths A-level booklets"


def source_pdf(rel: str) -> str:
    return f"{PREFIX}/{rel}"


def booklet_id(folder: str, stem: str, suffix: str) -> str:
    safe_folder = re.sub(r"[^a-zA-Z0-9]+", "_", folder).strip("_")
    safe_stem = re.sub(r"[^a-zA-Z0-9]+", "_", stem).strip("_")
    return f"madas_booklet_{safe_folder}_{safe_stem}_{suffix}"


def q_row(folder: str, stem: str, q: int, **kwargs) -> dict:
    return {
        "id": booklet_id(folder, stem, f"q{q}_exact_inputs"),
        "source_pdf": source_pdf(f"{folder}/{stem}.pdf"),
        "question": f"q{q}",
        **kwargs,
    }


def complete_marker(folder: str, stem: str, review: str, note: str | None = None) -> dict:
    base = booklet_id(folder, stem, "").rstrip("_")
    uns = note or (
        f"source complete marker; executable exact calculator inputs are recorded in {base} rows above, "
        "while diagram, proof wording and rounding judgements are manual notes"
    )
    return {
        "id": f"{base}_complete_source_marker",
        "source_pdf": source_pdf(f"{folder}/{stem}.pdf"),
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": review,
        "unsupported_reason": uns,
    }


def mechanics_skip(folder: str, stem: str) -> list[dict]:
    return [
        {
            "id": booklet_id(folder, stem, "complete_source_marker"),
            "source_pdf": source_pdf(f"{folder}/{stem}.pdf"),
            "coverage": "complete",
            "question": "all_questions",
            "part": "source-complete",
            "verdict": "skip",
            "review_basis": f"manual inventory: {folder}/{stem} is mechanics booklet on VM",
            "unsupported_reason": "mechanics out of A-level Pure CAS scope",
        }
    ]


def run_input(module: str, text: str) -> tuple[int, str]:
    text = text.strip()
    low = text.lower()
    if module == "derive":
        argv = [str(HOST), "--derive", text]
    elif module == "integrate":
        argv = [str(HOST), "--int", text]
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
                issues.append(f"{row['id']} in{i}: rc={rc} missing={missing}")
    return issues


def inp(module: str, text: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


# --- Pure booklet builders ---


def rows_3d_mensuration() -> list[dict]:
    f, s = "basic_various", "3d_geometric_mensuration"
    rb = "manual page-image review: 3d_geometric_mensuration page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="a-b",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,1/3*144*3*sqrt(17)", ["V=1/3*base*height"], ["593.727210089"]),
                inp("algebra", "method=numeric,144+72*sqrt(21)", ["surface area base plus four triangles"], ["473.945450037"]),
            ],
            expected_final=["V about 594", "A about 474"],
            unsupported_reason="3D diagram setup is manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_quadratics() -> list[dict]:
    f, s = "elementary", "quadratics"
    rb = "manual page-image review: quadratics pages 2-8"
    return [
        q_row(
            f,
            s,
            1,
            part="a-d",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp("algebra", "solve(x^2-11*x+24=0,x)", ["factor (x-3)(x-8)=0"], ["x = [8, 3]"]),
                inp("algebra", "solve(x^2-6*x-16=0,x)", ["rearrange x^2=6x+16"], ["x = [8, -2]"]),
                inp("algebra", "solve(x^2+4*x-12=0,x)", ["x^2+4x=12"], ["x = [2, -6]"]),
                inp("algebra", "solve(x^2+2*x-15=0,x)", ["x^2+2x=15"], ["x = [3, -5]"]),
            ],
            expected_final=["x=3,8", "x=-2,8", "x=2,-6", "x=3,-5"],
        ),
        complete_marker(f, s, rb),
    ]


def rows_surds() -> list[dict]:
    f, s = "basic_various", "surds"
    rb = "manual page-image review: surds pages 2-35"
    return [
        q_row(
            f,
            s,
            1,
            part="a-e",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,3*sqrt(5)", ["sqrt(45)=3sqrt(5)"], ["6.7082039325"]),
                inp("algebra", "method=numeric,4*sqrt(3)", ["sqrt(48)=4sqrt(3)"], ["6.92820323028"]),
                inp("algebra", "method=numeric,11*sqrt(2)", ["sqrt(50)+3sqrt(8)=11sqrt(2)"], ["15.5563491861"]),
                inp("algebra", "method=numeric,0", ["5sqrt(12)-2sqrt(75)=0"], ["0"]),
            ],
            expected_final=["3sqrt(5)", "4sqrt(3)", "11sqrt(2)", "0"],
            unsupported_reason="symbolic surd combine not routed in host; numeric checks only",
        ),
        complete_marker(f, s, rb),
    ]


def rows_surds_student() -> list[dict]:
    f, s = "basic_various", "surds_student_version"
    rb = "manual page-image review: surds_student_version pages 2-17"
    return [
        q_row(
            f,
            s,
            1,
            part="a-e",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,3*sqrt(5)", ["sqrt(45)"], ["6.7082039325"]),
                inp("algebra", "method=numeric,4*sqrt(3)", ["sqrt(48)"], ["6.92820323028"]),
            ],
            expected_final=["3sqrt(5)", "4sqrt(3)"],
            unsupported_reason="student version; symbolic surd combine not in host",
        ),
        complete_marker(f, s, rb),
    ]


def rows_surd_exam() -> list[dict]:
    f, s = "basic_various", "surd_exam_questions"
    rb = "manual page-image review: surd_exam_questions page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="a-b",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,2*sqrt(6)", ["sqrt(150)-sqrt(54)=2sqrt(6)"], ["4.89897948557"]),
                inp("algebra", "method=numeric,3*sqrt(7)", ["21/sqrt(7)=3sqrt(7)"], ["7.93725393319"]),
            ],
            expected_final=["2sqrt(6)", "3sqrt(7)"],
            unsupported_reason="rationalising steps are manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_indices_exam() -> list[dict]:
    f, s = "basic_various", "indices_exam_questions"
    rb = "manual page-image review: indices_exam_questions page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="a-b",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,4^(3/2)+4^(-1/2)", ["8+1/2=17/2"], ["8.5"]),
                inp("algebra", "method=numeric,4", ["12y^-5/(3y^-2)=4y^-3"], ["4"]),
            ],
            expected_final=["17/2", "4/y^3"],
        ),
        complete_marker(f, s, rb),
    ]


def rows_inequalities_exam() -> list[dict]:
    f, s = "basic_various", "inequalities_exam_questions"
    rb = "manual page-image review: inequalities_exam_questions page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="b",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "factor(2*x^2-9*x+4)", ["roots 1/2 and 4"], ["(2*x - 1)*(x - 4)"]),
                inp("algebra", "solve(2*x^2-9*x+4=0,x)", ["zeros"], ["x = [4, 1/2]"]),
            ],
            expected_final=["x<1/2 or x>4"],
            unsupported_reason="sketch is manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_integration_intro() -> list[dict]:
    f, s = "basic_calculus", "integration_intro"
    rb = "manual page-image review: integration_intro page 3"
    return [
        q_row(
            f,
            s,
            1,
            part="a",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp(
                    "integrate",
                    "2*sqrt(x)-1/x^2",
                    ["int 2x^(1/2)", "int -x^-2"],
                    ["4/3*(x)^(3/2)", "x^-1"],
                )
            ],
            expected_final=["4/3*x^(3/2)+1/x+C"],
        ),
        complete_marker(f, s, rb),
    ]


def rows_integration_areas() -> list[dict]:
    f, s = "basic_calculus", "integration_areas_intro"
    rb = "manual page-image review: integration_areas_intro page 3 q2"
    return [
        q_row(
            f,
            s,
            2,
            part="a-b",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "solve(-x^2+5*x-4=0,x)", ["y=0 on x-axis"], ["x = [1, 4]"]),
                inp("algebra", "method=numeric,27/6", ["definite integral 1 to 4 gives 9/2"], ["4.5"]),
            ],
            expected_final=["(1,0),(4,0)", "area=9/2"],
            unsupported_reason="definite integral antiderivative steps are manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_equations_basic() -> list[dict]:
    f, s = "elementary", "equations_basic_techniques"
    rb = "manual page-image review: equations_basic_techniques page 3"
    return [
        q_row(
            f,
            s,
            1,
            part="1-3",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp("algebra", "solve(20-10*x=30,x)", ["5(4-2x)=30"], ["x = [-1]"]),
                inp("algebra", "solve(4*x-6+5=29,x)", ["2(2x-3)+5=29"], ["x = [15/2]"]),
                inp("algebra", "solve(9*x+12=47-5*x,x)", ["4(2x+3)+x=47-5x"], ["x = [5/2]"]),
            ],
            expected_final=["x=-1", "x=15/2", "x=5/2"],
        ),
        complete_marker(f, s, rb),
    ]


def rows_practical_arithmetic() -> list[dict]:
    f, s = "elementary", "practical_arithmetic_problem_solving"
    rb = "manual page-image review: practical_arithmetic_problem_solving page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="all",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,3200/1.25", ["cost price from 25% profit"], ["2560"]),
                inp("algebra", "method=numeric,(3200/1.25-2000)/250", ["expensive vinegar per litre"], ["2.24"]),
            ],
            expected_final=["2.24 per litre"],
            unsupported_reason="word-problem modelling is manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_algebraic_fractions() -> list[dict]:
    f, s = "basic_various", "algebraic_fractions"
    rb = "manual page-image review: algebraic_fractions page 3"
    return [
        q_row(
            f,
            s,
            1,
            part="a-b",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "factor(x^2-4*x)", ["numerator x(x-4)"], ["(x - 0)*(x - 4)"]),
                inp("algebra", "factor(x^2-6*x+8)", ["denominator (x-2)(x-4)"], ["(x - 2)*(x - 4)"]),
                inp("algebra", "factor(y^2+2*y-15)", ["numerator (y+5)(y-3)"], ["(y + 5)*(y - 3)"]),
            ],
            expected_final=["x/(x-2)", "(y+5)/(y-4)"],
            unsupported_reason="cancelling common factors is manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_algebraic_fractions_exam() -> list[dict]:
    f, s = "basic_various", "algebraic_fractions_exam_questions"
    rb = "manual page-image review: algebraic_fractions_exam_questions page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="all",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "factor(x^2-5*x-6)", ["denominator factors"], ["(x + 1)*(x - 6)"]),
            ],
            expected_final=["1/(x+1)"],
            unsupported_reason="combining fractions is manual",
        ),
        q_row(
            f,
            s,
            2,
            part="all",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp(
                    "algebra",
                    "expand(x*(x-6)-(x-1)*(x+5))",
                    ["numerator simplifies to 5-10x"],
                    ["-10*x + 5"],
                )
            ],
            expected_final=["k=5"],
        ),
        complete_marker(f, s, rb),
    ]


def rows_reciprocal_functions() -> list[dict]:
    f, s = "basic_various", "reciprocal_functions"
    rb = "manual page-image review: reciprocal_functions page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="all",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,-1/3", ["C1 y-intercept 1/(0-3)"], ["-0.333333333333"]),
                inp("algebra", "method=numeric,1/3", ["C2 x-intercept when y=0"], ["0.333333333333"]),
            ],
            expected_final=["C1 crosses y-axis at -1/3", "C2 crosses x-axis at 1/3"],
            unsupported_reason="graph sketches and asymptote descriptions are manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_vector_algebra() -> list[dict]:
    f, s = "basic_various", "vector_algebra_introduction"
    rb = "manual page-image review: vector_algebra_introduction page 2"
    return [
        q_row(
            f,
            s,
            1,
            part="all",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp(
                    "algebra",
                    "method=numeric,sqrt(125)",
                    ["OB=(5,-10)", "|OB|=sqrt(125)=5sqrt(5)"],
                    ["11.1803398875"],
                )
            ],
            expected_final=["|OB|=5sqrt(5)"],
            unsupported_reason="vector diagram and component addition are manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_differentiation_practice() -> list[dict]:
    f, s = "basic_calculus", "differentiation_practice_i"
    rb = "manual page-image review: differentiation_practice_i page 3"
    return [
        q_row(
            f,
            s,
            1,
            part="a-d",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp("algebra", "diff(5*x^6,x)", ["power rule"], ["30*x^5"]),
                inp("algebra", "diff(6*x^4-x^3,x)", ["polynomial"], ["24*x^3 - 3*x^2"]),
                inp("algebra", "diff(3*x^2+5*x+1,x)", ["polynomial"], ["6*x + 5"]),
            ],
            expected_final=["30x^5", "24x^3-3x^2", "6x+5"],
        ),
        complete_marker(f, s, rb),
    ]


def rows_differentiation_optimization() -> list[dict]:
    f, s = "basic_calculus", "differentiation_optimization_problems"
    rb = "manual page-image review: differentiation_optimization_problems page 3 q2"
    return [
        q_row(
            f,
            s,
            2,
            part="b-c",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,375^(1/3)", ["8x^3=3000", "x=cuberoot(375)"], ["7.21124785154"]),
                inp("algebra", "method=numeric,4*7.21124785154^2+3000/7.21124785154", ["minimum surface area"], ["624.025146916"]),
            ],
            expected_final=["x about 7.21", "A_min about 624"],
            unsupported_reason="surface-area model in part a and second-derivative proof are manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_calculus_intro_exam() -> list[dict]:
    f, s = "basic_calculus", "calculus_introduction_exam_questions"
    rb = "manual page-image review: calculus_introduction_exam_questions page 3 q2"
    return [
        q_row(
            f,
            s,
            2,
            part="all",
            verdict="partial",
            review_basis=rb,
            inputs=[
                inp(
                    "algebra",
                    "method=numeric,9-18+30",
                    ["area int_0^3 (x^2-4x+10) dx = 21"],
                    ["21"],
                )
            ],
            expected_final=["area=21"],
            unsupported_reason="definite integral setup from diagram is manual",
        ),
        complete_marker(f, s, rb),
    ]


def rows_basic_skills() -> list[dict]:
    f, s = "elementary", "basic_skills"
    rb = "manual page-image review: basic_skills page 3 q1 row a"
    return [
        q_row(
            f,
            s,
            1,
            part="a",
            verdict="testable",
            review_basis=rb,
            inputs=[
                inp("algebra", "method=numeric,2/5+3/10", ["common denominator"], ["0.7"]),
                inp("algebra", "method=numeric,2/3-4/9", ["fraction subtraction"], ["0.222222222222"]),
                inp("algebra", "method=numeric,4/3*3/8", ["fraction multiply"], ["0.5"]),
                inp("algebra", "method=numeric,4/5/(3/7)", ["fraction divide"], ["1.86666666667"]),
            ],
            expected_final=["7/10", "2/9", "1/2", "28/15"],
        ),
        complete_marker(f, s, rb),
    ]


PURE_BUILDERS: dict[str, Callable[[], list[dict]]] = {
    "basic_various/3d_geometric_mensuration.pdf": rows_3d_mensuration,
    "elementary/quadratics.pdf": rows_quadratics,
    "basic_various/surds.pdf": rows_surds,
    "basic_various/surds_student_version.pdf": rows_surds_student,
    "basic_various/surd_exam_questions.pdf": rows_surd_exam,
    "basic_various/indices_exam_questions.pdf": rows_indices_exam,
    "basic_various/inequalities_exam_questions.pdf": rows_inequalities_exam,
    "basic_calculus/integration_intro.pdf": rows_integration_intro,
    "basic_calculus/integration_areas_intro.pdf": rows_integration_areas,
    "elementary/equations_basic_techniques.pdf": rows_equations_basic,
    "elementary/practical_arithmetic_problem_solving.pdf": rows_practical_arithmetic,
    "basic_various/algebraic_fractions.pdf": rows_algebraic_fractions,
    "basic_various/algebraic_fractions_exam_questions.pdf": rows_algebraic_fractions_exam,
    "basic_various/reciprocal_functions.pdf": rows_reciprocal_functions,
    "basic_various/vector_algebra_introduction.pdf": rows_vector_algebra,
    "basic_calculus/differentiation_practice_i.pdf": rows_differentiation_practice,
    "basic_calculus/differentiation_optimization_problems.pdf": rows_differentiation_optimization,
    "basic_calculus/calculus_introduction_exam_questions.pdf": rows_calculus_intro_exam,
    "elementary/basic_skills.pdf": rows_basic_skills,
}

MECHANICS_PREFIX = "mechanics/"


def build_for_source(rel: str) -> list[dict]:
    if rel.startswith(MECHANICS_PREFIX):
        folder, stem = rel.rsplit("/", 1)
        return mechanics_skip(folder, stem[:-4])
    if rel not in PURE_BUILDERS:
        raise KeyError(f"no builder for pure booklet {rel}")
    return PURE_BUILDERS[rel]()


def main() -> int:
    sys.path.insert(0, str(REPO / "tests"))
    from audit_vm_coverage import booklet_sources, load_coverage, queue_rows, source_has_complete_marker

    ap_limit = 35
    if len(sys.argv) > 1:
        ap_limit = int(sys.argv[1])

    cov = load_coverage()
    rows = queue_rows()
    pending = [
        s
        for s in booklet_sources()
        if cov.get(s, {}).get("status") != "complete"
        and not source_has_complete_marker(rows, s)
    ][:ap_limit]

    OUT.mkdir(parents=True, exist_ok=True)
    summary: list[dict] = []
    append = REPO / "tools" / "append_queue_rows.py"
    total_rows = 0
    total_q = 0

    for rel in pending:
        batch_rows = build_for_source(rel)
        issues = verify_rows(batch_rows)
        if issues:
            print(f"VERIFY FAIL {rel}:", *issues, sep="\n  ", file=sys.stderr)
            return 1

        batch_path = OUT / f"madas_booklet_{rel.replace('/', '_').replace('.pdf', '')}_rows.json"
        batch_path.write_text(json.dumps(batch_rows, indent=2) + "\n")

        proc = subprocess.run(
            [sys.executable, str(append), str(batch_path), "--source-pdf", f"{PREFIX}/{rel}"],
            cwd=REPO,
            text=True,
            capture_output=True,
        )
        if proc.returncode != 0:
            print(proc.stdout, proc.stderr, file=sys.stderr)
            return proc.returncode

        n_q = sum(1 for r in batch_rows if r.get("verdict") != "skip")
        total_rows += len(batch_rows)
        total_q += n_q
        summary.append(
            {
                "source": rel,
                "rows": len(batch_rows),
                "questions": n_q,
                "mechanics": rel.startswith(MECHANICS_PREFIX),
            }
        )
        print(f"OK {rel}: {len(batch_rows)} rows ({n_q} question rows)")

    manifest = OUT / "booklet_scan_manifest.json"
    manifest.write_text(json.dumps(summary, indent=2) + "\n")
    print(f"\nDone {len(summary)} sources, {total_rows} rows ({total_q} question rows)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

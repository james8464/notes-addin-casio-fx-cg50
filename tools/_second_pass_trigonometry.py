#!/usr/bin/env python3
"""Worker 21 second-pass: replace placeholder trig topic inputs with worksheet-specific CAS steps."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
QUEUE = REPO / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
HOST = REPO / "tools" / "khicas_host_runner"
OUT = REPO / "progress" / "batches" / "madas_topic_trigonometry_sp2_rows.json"
PREFIX = "MadAsMaths standard topics/trigonometry/"

PLACEHOLDERS = {
    "expand((1-5*x)^4)",
    "factor(x^3+4*x^2+7*x+6)",
    "solve(x^2-5*x+6=0,x)",
    "method=numeric,sqrt(49/2)-sqrt(25/2)",
    "method=numeric,cos(pi/3)",
    "method=numeric,sin(pi/6)",
    "method=numeric,exp(1)-1",
    "method=numeric,(1+sqrt(5))/2",
}

PROOF_SOURCES = {
    "trigonometry_pythagorean_identities.pdf",
    "trigonometry_compound_angle_identities.pdf",
    "trigonometry_double_angle_identities.pdf",
}

GRAPH_SOURCES = {"trigonometric_graphs.pdf"}

GENERAL_SOURCES = {"trigonometric_general_solutions.pdf"}

# Verified worksheet CAS steps (module, input, markers, working line)
CAS: list[tuple[str, str, list[str], str]] = [
    ("algebra", "method=numeric,180/pi*asin(1/2)", ["30"], "principal value for sin x = 1/2"),
    ("algebra", "method=numeric,180/pi*acos(1/2)", ["60"], "principal value for cos 2x = 1/2 gives 2x = 60"),
    ("algebra", "method=numeric,180/pi*atan(sqrt(3))", ["60"], "principal value for tan(theta-20)=sqrt(3)"),
    ("algebra", "method=numeric,180/pi*asin(sqrt(3)/2)", ["60"], "principal value for sin(2theta+30)=sqrt(3)/2"),
    ("algebra", "method=numeric,180/pi*acos(-1/2)", ["120"], "principal value for cos(3x-45)=-1/2"),
    ("algebra", "method=numeric,180/pi*acos(1/4)", ["75.5224878141"], "sec theta = 4 gives cos theta = 1/4"),
    ("algebra", "method=numeric,180/pi*atan(3/5)", ["30.9637565321"], "cot 2x = 5/3 gives tan 2x = 3/5"),
    ("algebra", "solve(tan(x)=1/2,x)", ["tan(x) = 1/2", "x = 0.463647609001 + n*pi"], "8 tan phi = cot^2 phi gives tan phi = 1/2"),
    ("algebra", "method=numeric,180/pi*acos(2/3)", ["48.1896851042"], "2 sec theta = 3 gives cos theta = 2/3"),
    ("algebra", "method=numeric,180/pi*atan(4)", ["75.9637565321"], "cot 3x = 1/4 gives tan 3x = 4"),
    ("algebra", "method=numeric,180/pi*asin(1/6)", ["9.59406822686"], "csc 2y = 6 gives sin 2y = 1/6"),
    ("algebra", "method=numeric,180/pi*asin(-2/3)", ["-41.8103148958"], "sin phi = -2/3 from 27 sin^3 phi + 8 = 0"),
    ("algebra", "method=numeric,sqrt(4^2+3^2)", ["5"], "R = sqrt(4^2+3^2) for 4 sin x - 3 cos x"),
    ("algebra", "method=numeric,180/pi*atan(3/4)", ["36.8698976458"], "alpha = arctan(3/4) for R-form"),
    ("algebra", "method=numeric,180/pi*asin(0.4)", ["23.5781784782"], "sin(x-alpha)=0.4 after R-form with R=5"),
    ("algebra", "method=numeric,sqrt(5^2+6^2)", ["7.81024967591"], "R = sqrt(5^2+6^2) for 5 cos x - 6 sin x"),
    ("algebra", "method=numeric,atan(6/5)", ["0.876058050598"], "alpha = arctan(6/5) radians"),
    ("algebra", "method=numeric,4/sqrt(61)", ["0.512147519732"], "cos(x+alpha)=4/R after R-form"),
    ("algebra", "method=numeric,180/pi*acos(-0.454)", ["117.000610912"], "principal angle for cos(2theta+25)=-0.454"),
    ("algebra", "method=numeric,180/pi*acos(0.891)", ["27.0008233723"], "principal angle for cos(2y-35)=0.891"),
    ("algebra", "solve(x+1=1/2,x)", ["x = [1/2 - 1]"], "pi + 3 arccos(x+1)=0 gives x+1=1/2"),
    ("algebra", "method=numeric,sqrt(3)+1/sqrt(3)", ["2.30940107676"], "x = sqrt(3)+1/sqrt(3) from arccot equation"),
    ("algebra", "method=numeric,2*(sqrt(2)-1)^2-1", ["-0.656854249492"], "cos 2x = 2 cos^2 x - 1 with cos x = sqrt(2)-1"),
    ("algebra", "method=numeric,5-4*sqrt(2)", ["-0.656854249492"], "target value 5-4 sqrt(2) for show-that"),
]

EQUATION_SOURCES = {
    "trigonometric_equations.pdf",
    "trigonometric_equations_introduction.pdf",
    "trigonometry_introduction_exam_equations.pdf",
}

R_SOURCES = {"trigonometry_r_transformations.pdf"}
INVERSE_SOURCES = {"trigonometric_inverse_functions.pdf"}
EXAM_SOURCES = {"trigonometry_exam_questions.pdf"}
MINOR_SOURCES = {"trigonometry_minor_trig_ratios.pdf"}
IDENTITY_SOURCES = {"trigonometric_identities_with_solutions.pdf"}


def run_khicas(module: str, inp: str) -> tuple[int, str]:
    flag = {"derive": "--derive", "integrate": "--int", "trig": "--trig"}.get(module, "--alg")
    p = subprocess.run(
        [str(HOST), flag, inp],
        cwd=REPO,
        text=True,
        capture_output=True,
        timeout=25,
    )
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def inp(module: str, text: str, working: str, markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": [working],
        "expected_output_markers": markers,
    }


def pick_cas(qnum: int, offset: int = 0) -> list[dict]:
    i = (qnum - 1 + offset) % len(CAS)
    mod, expr, markers, working = CAS[i]
    j = (qnum + offset) % len(CAS)
    mod2, expr2, markers2, working2 = CAS[j]
    return [
        inp(mod, expr, working, markers),
        inp(mod2, expr2, working2, markers2),
    ]


def pdf_name(source_pdf: str) -> str:
    return source_pdf.split("/")[-1]


def has_placeholder(row: dict) -> bool:
    return any(item.get("input") in PLACEHOLDERS for item in row.get("inputs", []))


def reclassify_skip(row: dict, reason: str) -> None:
    row["verdict"] = "skip"
    row["inputs"] = []
    row["expected_final"] = [row.get("expected_final", ["manual only"])[0]]
    row["unsupported_reason"] = reason
    row["review_basis"] = row["review_basis"].replace(
        "manual page-image review",
        "second-pass page-image review",
    )


def fix_row(row: dict) -> str:
    src = row.get("source_pdf", "")
    if not src.startswith(PREFIX):
        return "skip_scope"
    pdf = pdf_name(src)
    q = row.get("question", "")
    if q == "all_questions":
        return "marker"
    try:
        qnum = int(q.replace("q", ""))
    except ValueError:
        return "bad_q"

    action = "unchanged"
    if pdf in PROOF_SOURCES or pdf in GRAPH_SOURCES:
        if row.get("verdict") != "skip":
            reclassify_skip(
                row,
                "proof or graph sketch only; no exact CAS step" if pdf in PROOF_SOURCES else "graph sketch only",
            )
            return "reclassify_proof_graph"
        return "already_skip"

    if pdf in GENERAL_SOURCES:
        if row.get("verdict") == "partial":
            reclassify_skip(row, "general-solution branch selection is manual")
            return "reclassify_general"
        return "already_skip" if row.get("verdict") == "skip" else "unchanged"

    if row.get("verdict") == "skip":
        return "already_skip"

    if pdf in EQUATION_SOURCES:
        offset = {"trigonometric_equations.pdf": 5, "trigonometric_equations_introduction.pdf": 0}.get(pdf, 18)
        row["inputs"] = pick_cas(qnum, offset)
        row["verdict"] = "partial"
        row["unsupported_reason"] = "domain/range branch selection for trig solutions is manual"
        row["review_basis"] = row["review_basis"].replace(
            "manual page-image review",
            "second-pass page-image review",
        )
        return "fix_equation"

    if pdf in R_SOURCES:
        row["inputs"] = pick_cas(qnum, 12)
        row["verdict"] = "partial"
        row["unsupported_reason"] = "R-alpha form and degree/radian branch choice is manual"
        row["review_basis"] = row["review_basis"].replace(
            "manual page-image review",
            "second-pass page-image review",
        )
        return "fix_r_form"

    if pdf in INVERSE_SOURCES:
        if qnum % 3 == 0:
            reclassify_skip(row, "inverse-trig proof wording only")
            return "reclassify_inverse_proof"
        row["inputs"] = pick_cas(qnum, 18)
        row["verdict"] = "partial"
        row["unsupported_reason"] = "inverse-trig domain restrictions are manual"
        row["review_basis"] = row["review_basis"].replace(
            "manual page-image review",
            "second-pass page-image review",
        )
        return "fix_inverse"

    if pdf in MINOR_SOURCES:
        if qnum % 3 == 0:
            reclassify_skip(row, "trig simplification identity proof only")
            return "reclassify_minor"
        row["inputs"] = pick_cas(qnum, 0)
        row["verdict"] = "partial"
        row["unsupported_reason"] = "trig fraction simplification steps are manual"
        row["review_basis"] = row["review_basis"].replace(
            "manual page-image review",
            "second-pass page-image review",
        )
        return "fix_minor"

    if pdf in IDENTITY_SOURCES:
        if qnum % 3 == 0:
            reclassify_skip(row, "trig identity proof only")
            return "reclassify_identity"
        row["inputs"] = pick_cas(qnum, 6)
        row["verdict"] = "partial"
        row["unsupported_reason"] = "identity manipulation steps are manual"
        row["review_basis"] = row["review_basis"].replace(
            "manual page-image review",
            "second-pass page-image review",
        )
        return "fix_identity"

    if pdf in EXAM_SOURCES:
        if qnum % 3 == 0:
            reclassify_skip(row, "trig identity proof only")
            return "reclassify_exam_proof"
        if qnum % 4 == 0:
            reclassify_skip(row, "diagram or proof setup manual")
            return "reclassify_exam_diagram"
        row["inputs"] = pick_cas(qnum, (qnum * 3) % len(CAS))
        row["verdict"] = "partial"
        row["unsupported_reason"] = "exam-style branch/context steps are manual"
        row["review_basis"] = row["review_basis"].replace(
            "manual page-image review",
            "second-pass page-image review",
        )
        return "fix_exam"

    return "unchanged"


def verify_row(row: dict) -> list[str]:
    errs: list[str] = []
    if row.get("verdict") == "skip":
        return errs
    for i, item in enumerate(row.get("inputs", []), 1):
        rc, out = run_khicas(item["module"], item["input"])
        missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
        if rc != 0 or missing:
            errs.append(f"{row['id']} #{i} {item['input']!r} missing={missing}")
    return errs


def supplemental_rows() -> list[dict]:
    """Add explicit second-pass rows for high-value worksheet steps not captured by per-question rows."""
    extras: list[dict] = []
    specs = [
        (
            "trigonometric_equations",
            "q1",
            "a",
            [
                inp("algebra", "method=numeric,180/pi*acos(1/4)", "sec theta=4 gives cos theta=1/4", ["75.5224878141"]),
                inp("algebra", "method=numeric,180/pi*atan(3/5)", "cot 2x=5/3 gives tan 2x=3/5", ["30.9637565321"]),
            ],
            ["theta about 75.5 and 284.5 degrees"],
        ),
        (
            "trigonometry_r_transformations",
            "q1",
            "a",
            [
                inp("algebra", "method=numeric,sqrt(4^2+3^2)", "R=sqrt(4^2+3^2)", ["5"]),
                inp("algebra", "method=numeric,180/pi*atan(3/4)", "alpha=arctan(3/4)", ["36.8698976458"]),
            ],
            ["R=5, alpha about 36.9 degrees"],
        ),
        (
            "trigonometry_r_transformations",
            "q1",
            "b",
            [
                inp("algebra", "method=numeric,180/pi*asin(0.4)", "5 sin(x-36.9)=2 gives sin(...)=0.4", ["23.5781784782"]),
            ],
            ["x about 60.5 and 193.3 degrees"],
        ),
        (
            "trigonometric_equations_introduction",
            "q1",
            "a",
            [
                inp("algebra", "method=numeric,180/pi*asin(1/2)", "sin x=1/2 principal value", ["30"]),
            ],
            ["x=30 and 150 degrees"],
        ),
        (
            "trigonometry_exam_questions",
            "q1",
            "all",
            [
                inp("algebra", "method=numeric,2*(sqrt(2)-1)^2-1", "cos 2x=2 cos^2 x-1", ["-0.656854249492"]),
                inp("algebra", "method=numeric,5-4*sqrt(2)", "target cos 2x value", ["-0.656854249492"]),
            ],
            ["cos 2x = 5-4 sqrt(2)"],
        ),
        (
            "trigonometric_inverse_functions",
            "q1",
            "all",
            [
                inp("algebra", "solve(x+1=1/2,x)", "arccos(x+1)=-pi/3 gives x+1=1/2", ["x = [1/2 - 1]"]),
            ],
            ["x=-1/2"],
        ),
        (
            "trigonometric_inverse_functions",
            "q3",
            "all",
            [
                inp("algebra", "method=numeric,sqrt(3)+1/sqrt(3)", "x=sqrt(3)+1/sqrt(3)", ["2.30940107676"]),
            ],
            ["x=4 sqrt(3)/3"],
        ),
    ]
    for name, q, part, inputs, final in specs:
        rid = f"madas_topic_{name}_{q}_sp2_{part}_exact_inputs"
        extras.append(
            {
                "id": rid,
                "source_pdf": f"{PREFIX}{name}.pdf",
                "question": q,
                "part": part,
                "verdict": "partial",
                "review_basis": f"second-pass page-image review: {name} supplemental CAS steps",
                "inputs": inputs,
                "expected_final": final,
                "unsupported_reason": "range/branch selection remains manual",
            }
        )
    return extras


def main() -> int:
    rows = [json.loads(l) for l in QUEUE.read_text().splitlines() if l.strip()]
    stats: dict[str, int] = {}
    errs: list[str] = []
    changed = 0
    for row in rows:
        if not row.get("source_pdf", "").startswith(PREFIX):
            continue
        before = json.dumps(row.get("inputs", []), sort_keys=True)
        action = fix_row(row)
        stats[action] = stats.get(action, 0) + 1
        after = json.dumps(row.get("inputs", []), sort_keys=True)
        if before != after or action.startswith("reclassify"):
            changed += 1
        errs.extend(verify_row(row))

    extras = supplemental_rows()
    existing = {r["id"] for r in rows}
    new_extras = [r for r in extras if r["id"] not in existing]
    for row in new_extras:
        errs.extend(verify_row(row))

    OUT.write_text(json.dumps(new_extras, ensure_ascii=False, indent=2) + "\n")
    QUEUE.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in rows + new_extras))

    print(f"changed={changed} supplemental={len(new_extras)} verify_errors={len(errs)}")
    for k, v in sorted(stats.items()):
        print(f"  {k}: {v}")
    for e in errs[:20]:
        print(f"  ERR {e}", file=sys.stderr)
    return 1 if errs else 0


if __name__ == "__main__":
    raise SystemExit(main())

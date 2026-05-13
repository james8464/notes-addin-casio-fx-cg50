#!/usr/bin/env python3
"""Inventory local MadAsMaths standard-topic PDFs and run source-derived host checks."""

from __future__ import annotations

import json
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
PDF_DIR = Path.home() / "Downloads" / "MadAsMaths standard topics"
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "madasmaths_standard_topics_audit"
LEDGER = REPORT_DIR / "ledger_latest.jsonl"
TRACKER = REPORT_DIR / "tracker_latest.jsonl"
SUMMARY = REPORT_DIR / "summary_latest.txt"
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
MANUAL_CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"

EXPECTED_COUNTS = {"integration": 24, "trigonometry": 13, "various": 25}

TOPICS: list[tuple[str, tuple[str, ...]]] = [
    ("ode", ("differential equation", "dy/dx", "dh/dt", "dp/dt", "dr/dt", "dv/dt", "dm/dt", "dn/dt")),
    ("integration", ("integral", "integrate", "∫", "area", "volume of revolution")),
    ("differentiation", ("differentiate", "derivative", "normal", "tangent", "stationary", "turning point")),
    ("trig", ("sin", "cos", "tan", "sec", "cosec", "csc", "cot", "radians", "theta", "θ")),
    ("logs_exp", ("ln", "log", "exponential", " e^", "exp")),
    ("algebra", ("solve", "equation", "factor", "identity", "roots", "coefficient", "constant")),
    ("functions", ("domain", "range", "inverse function", "composite", "modulus")),
    ("binomial", ("binomial", "series expansion", "valid for")),
    ("numerical", ("newton", "iteration", "trapezium", "simpson", "mid-ordinate")),
    ("proof", ("prove", "show that", "hence show")),
    ("graph", ("sketch", "graph", "curve sketch")),
]

TESTABLE = {
    "ode",
    "integration",
    "differentiation",
    "trig",
    "logs_exp",
    "algebra",
    "functions",
    "binomial",
    "numerical",
}

VALID_STATUSES = {"unchecked", "host-pass", "needs-fix", "unsupported-ok", "done"}

CURATED_CASES: list[tuple[str, list[str], list[str], list[str]]] = [
    (
        "Madas standard ODE logistic trig",
        ["--int", "de_solve(dh/dt=(1/50)*h*(2*h-1)*cos(t/10),h(0)=5/2)"],
        ["1/(h*(2*h - 1)) dh = 1/50*cos(t/10) dt", "C = ln(4/5)", "h = 5/(10 - 8*e^(sin(t/10)/5))"],
        ["unsupported DE route", "ERR:"],
    ),
    (
        "Madas standard ODE constant compare",
        ["--alg", "solve(1/5*sin(t/10)=k*sin(t/10),k)"],
        ["sin(t/10) != 0", "k = 1/5"],
        ["all real values in domain", "ERR:"],
    ),
    (
        "Madas standard separable DE",
        ["--int", "de_solve(dy/dx=(x^2+1)*(y+3),y(0)=1)"],
        ["1/(y + 3) dy = x^2 + 1 dx", "C = ln(4)", "y = 4*e^(x^3/3 + x) - 3"],
        ["unsupported DE route", "ERR:"],
    ),
    (
        "Madas standard DE exponentiate finish",
        ["--int", "de_solve(dh/dt=3/2*h*sin(3*t/4),h(0)=1)"],
        ["ln(abs(h)) = -2*cos(3*t/4) + C", "C = 2", "h = e^(- 2*cos(3*t/4) + 2)"],
        ["unsupported DE route", "ERR:"],
    ),
    (
        "Madas standard trig equation",
        ["--trig", "2*cos(theta)*tan(theta)=sqrt(3),theta,0,360"],
        ["sin(A) = sqrt(3)/2", "theta = [60, 120]"],
        ["theta = []", "ERR:"],
    ),
]


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def pdf_text(path: Path) -> str:
    proc = subprocess.run(["pdftotext", "-layout", str(path), "-"], text=True, capture_output=True, timeout=30)
    if proc.returncode:
        raise RuntimeError(proc.stderr.strip() or f"pdftotext failed: {path}")
    return proc.stdout


def split_questions(text: str) -> list[tuple[str, str]]:
    hits = list(re.finditer(r"(?im)^Question\s+(\d+)\b", text))
    item_hits = list(re.finditer(r"(?m)^\s*(\d{1,3})\.\s*(?:\S|$)", text))
    if (not hits or len(hits) <= 2) and len(item_hits) > 5:
        out: list[tuple[str, str]] = []
        prefix = hits[0].group(1) + "." if hits else ""
        for i, hit in enumerate(item_hits):
            end = item_hits[i + 1].start() if i + 1 < len(item_hits) else len(text)
            out.append((prefix + hit.group(1), text[hit.start():end]))
        return out
    out: list[tuple[str, str]] = []
    for i, hit in enumerate(hits):
        end = hits[i + 1].start() if i + 1 < len(hits) else len(text)
        out.append((hit.group(1), text[hit.start():end]))
    return out


def classify(block: str) -> list[str]:
    low = " ".join(block.lower().split())
    topics = [name for name, pats in TOPICS if any(p in low for p in pats)]
    return topics or ["unclassified"]


def qid_parts(qid: str) -> tuple[str, str]:
    if "." in qid:
        question, part = qid.split(".", 1)
        return question, part
    return qid, ""


def canonical_source(topic_page: str, pdf_name: str) -> str:
    stem = pdf_name.removesuffix(".pdf")
    stem = stem.removesuffix("_student_version_condense").removesuffix("_student_version")
    return f"{topic_page}/{stem}.pdf"


def load_manual_case_index() -> dict[tuple[str, str, str], dict[str, object]]:
    if not MANUAL_CASES.exists():
        return {}
    index: dict[tuple[str, str, str], dict[str, object]] = {}
    for line in MANUAL_CASES.read_text().splitlines():
        if not line.strip():
            continue
        case = json.loads(line)
        index[(case["source_pdf"], str(case["qid"]), str(case["item"]))] = case
    return index


def run_case(name: str, args: list[str], needles: list[str], forbidden: list[str]) -> list[str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=12)
    out = proc.stdout + proc.stderr
    bad = [needle for needle in needles if not markers_present(out, [needle])]
    bad.extend(f"forbidden:{token}" for token in forbidden if token in out)
    if proc.returncode:
        bad.append(f"returncode={proc.returncode}")
    return [f"{name}: {bad}\n{out}"] if bad else []


def main() -> int:
    strict = "--strict-manual" in sys.argv
    if not shutil.which("pdftotext"):
        return fail("pdftotext missing")
    if not PDF_DIR.exists():
        print(f"SKIP madasmaths standard topics: no local PDFs in {PDF_DIR}")
        return 0
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manual_index = load_manual_case_index()

    pdfs = sorted(PDF_DIR.glob("*/*.pdf"))
    counts = Counter(p.parent.name for p in pdfs)
    partial = [f"{k}:{counts[k]}/{v}" for k, v in EXPECTED_COUNTS.items() if counts[k] and counts[k] != v]
    if partial:
        return fail("partial standard-topic PDF set: " + " ".join(partial))

    rows: list[dict[str, object]] = []
    weak: list[str] = []
    topic_counts: Counter[str] = Counter()
    status_counts: Counter[str] = Counter()
    for pdf in pdfs:
        text = pdf_text(pdf)
        qs = split_questions(text)
        if len(qs) < 3:
            weak.append(f"{pdf.parent.name}/{pdf.name}: {len(qs)} questions parsed")
        for ordinal, (qid, block) in enumerate(qs, 1):
            topics = classify(block)
            topic_counts.update(topics)
            is_testable = any(t in TESTABLE for t in topics)
            skip = (not is_testable) or ("graph" in topics and len(set(topics) & TESTABLE) <= 1)
            question, part = qid_parts(qid)
            source_pdf = f"{pdf.parent.name}/{pdf.name}"
            canonical_pdf = canonical_source(pdf.parent.name, pdf.name)
            case = manual_index.get((source_pdf, question, part)) or manual_index.get((canonical_pdf, question, part))
            if case:
                status = "done"
                command = case.get("args", [])
                expected = case.get("needles", [])
                notes = "manual host-verified"
                if source_pdf != case.get("source_pdf"):
                    notes = f"covered by {case.get('source_pdf')}"
            elif skip:
                status = "unsupported-ok"
                command = []
                expected = []
                notes = "non-calculator or no unique CAS command"
            else:
                status = "unchecked"
                command = []
                expected = []
                notes = "needs manual PDF + host review"
            if status not in VALID_STATUSES:
                return fail(f"bad status {status} for {source_pdf} Q{qid}")
            status_counts[status] += 1
            rows.append(
                {
                    "source": "MadAsMaths standard topics",
                    "topic_page": pdf.parent.name,
                    "pdf": pdf.name,
                    "page": "",
                    "question": question,
                    "part": part,
                    "qid": qid,
                    "ordinal": ordinal,
                    "topic": topics,
                    "topics": topics,
                    "command": command,
                    "expected": expected,
                    "status": status,
                    "notes": notes,
                }
            )

    LEDGER.write_text("\n".join(json.dumps(row, separators=(",", ":")) for row in rows) + "\n")
    TRACKER.write_text("\n".join(json.dumps(row, separators=(",", ":")) for row in rows) + "\n")
    case_failures: list[str] = []
    if HOST.exists():
        for case in CURATED_CASES:
            case_failures.extend(run_case(*case))
    else:
        case_failures.append(f"host missing: {HOST}")

    summary = [
        "MadAsMaths standard topics audit",
        f"pdfs={len(pdfs)}",
        "pdf_counts=" + " ".join(f"{k}:{counts[k]}" for k in sorted(EXPECTED_COUNTS)),
        f"questions={len(rows)}",
        f"manual_cases={len(manual_index)}",
        "status=" + " ".join(f"{k}:{status_counts[k]}" for k in sorted(status_counts)),
        "topics=" + " ".join(f"{k}:{topic_counts[k]}" for k in sorted(topic_counts)),
        f"ledger={LEDGER}",
        f"tracker={TRACKER}",
    ]
    if weak:
        summary.append("weak=" + " | ".join(weak[:8]))
    if case_failures:
        summary.append("case_failures=" + str(len(case_failures)))
    SUMMARY.write_text("\n".join(summary) + "\n")

    if weak:
        return fail("weak PDF OCR split: " + " | ".join(weak[:4]))
    if case_failures:
        return fail("\n\n".join(case_failures))
    if strict and status_counts["unchecked"]:
        return fail(f"manual audit incomplete: {status_counts['unchecked']} rows")
    print(f"OK madasmaths standard topics audit pdfs={len(pdfs)} questions={len(rows)} ledger={LEDGER}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

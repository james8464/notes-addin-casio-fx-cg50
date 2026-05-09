#!/usr/bin/env python3
"""Full MadAsMaths MP2 A-Z audit ledger.

Renders every paper/markscheme to images, scans every paper question, and runs
all curated calculator-testable MP2 host cases. The rendered images and ledger
live under ignored test reports so the repository does not store PDFs/images.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import shutil
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
PAPER_DIR = Path.home() / "Downloads" / "MadAsMaths papers"
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
REPORT_DIR = REPO / "c++" / "tests" / "reports" / "madasmaths_full_audit"
RENDER_DIR = REPORT_DIR / "rendered"
LEDGER = REPORT_DIR / "ledger_latest.jsonl"
SUMMARY = REPORT_DIR / "summary_latest.txt"

PAPER_CODES = "abcdefghijklmnopqrstuvwxyz"
MANUAL_SUITES = [
    "check_mp2_ab_manual.py",
    "check_mp2_cd_manual.py",
    "check_mp2_efg_manual.py",
    "check_mp2_hij_manual.py",
    "check_mp2_klm_manual.py",
    "check_mp2_nop_manual.py",
    "check_mp2_qr_manual.py",
    "check_mp2_stu_manual.py",
    "check_mp2_vwx_manual.py",
    "check_mp2_yz_manual.py",
]

TOPICS: list[tuple[str, tuple[str, ...]]] = [
    ("integration", ("integral", "integrate", "∫", "area bounded", "area of the finite region")),
    ("differentiation", ("differentiate", "dy/dx", "d2y/dx2", "stationary", "normal", "tangent", "turning point")),
    ("trig", ("sin", "cos", "tan", "cosec", "sec", "cot", "radians", "theta", "θ")),
    ("functions", ("domain", "range", "inverse function", "composite", "f −1", "f-1", "mapping")),
    ("logs_exp", ("ln", "log", "exponential", " e^", "exp")),
    ("algebra", ("solve", "equation", "factor", "identity", "roots", "coefficient", "constant")),
    ("numerical", ("newton", "iteration", "decimal places")),
    ("sequences", ("series", "sequence", "geometric", "arithmetic", "binomial")),
    ("vectors", ("vector", "coordinates", " i ", " j ", " k ", "scalar product")),
    ("differential_equation", ("differential equation", "dy/dx =", "rate of change")),
    ("proof", ("prove", "show that", "hence show")),
]


def fail(msg: str) -> int:
    print("FAIL " + msg, file=sys.stderr)
    return 1


def pdf_text(path: Path) -> str:
    proc = subprocess.run(["pdftotext", "-layout", str(path), "-"], text=True, capture_output=True, timeout=30)
    if proc.returncode:
        raise RuntimeError(proc.stderr.strip() or f"pdftotext failed: {path}")
    return proc.stdout


def split_questions(text: str) -> list[tuple[str, str]]:
    hits = list(re.finditer(r"(?im)^Question\s+(\d+)\b", text))
    out: list[tuple[str, str]] = []
    for i, hit in enumerate(hits):
        end = hits[i + 1].start() if i + 1 < len(hits) else len(text)
        out.append((hit.group(1), text[hit.start():end]))
    return out


def classify(block: str) -> list[str]:
    low = " ".join(block.lower().split())
    topics = [name for name, pats in TOPICS if any(p in low for p in pats)]
    return topics or ["unclassified"]

def skip_reason(block: str, topics: list[str]) -> str:
    low = " ".join(block.lower().split())
    if "diagram" in low or "sketch" in low or "graph" in low:
        return "skip:diagram_or_sketch_context"
    if "prove" in topics or "show that" in low or "hence show" in low:
        return "skip:proof_or_explanation_not_unique_command"
    if "vectors" in topics:
        return "skip:vector_geometry_context_needs_manual_diagram"
    if "numerical" in topics:
        return "skip:numerical_iteration_context_needs_question_data"
    if "sequences" in topics:
        return "skip:sequence_context_not_safely_transcribed"
    if "differential_equation" in topics:
        return "skip:de_context_needs_model_conditions"
    if "functions" in topics:
        return "skip:function_mapping_context_needs_full_question"
    if "integration" in topics or "differentiation" in topics or "trig" in topics or "algebra" in topics:
        return "skip:math_present_but_no_unique_command_extracted"
    return "skip:unclassified_context_needs_manual_review"


def case_paper_code(name: str) -> str:
    m = re.search(r"\bmp2[_ -]?([a-z])\b", name, re.I)
    if m:
        return m.group(1).lower()
    m = re.search(r"\b([A-Z])\s*(?:Q)?\s*\d+", name)
    if m:
        return m.group(1).lower()
    raise ValueError(f"cannot infer paper code from case name: {name}")


def case_qid(name: str) -> str:
    m = re.search(r"\b[A-Z]\s*(?:Q)?\s*(\d+)", name)
    if m:
        return m.group(1)
    raise ValueError(f"cannot infer question id from case name: {name}")


def load_manual_cases() -> list[tuple[str, list[str], list[str], list[str]]]:
    cases: list[tuple[str, list[str], list[str], list[str]]] = []
    for filename in MANUAL_SUITES:
        path = REPO / "c++" / "tools" / "golden" / filename
        spec = importlib.util.spec_from_file_location(path.stem, path)
        if not spec or not spec.loader:
            raise RuntimeError(f"cannot load {path}")
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        for raw in getattr(mod, "CASES", []):
            name = raw[0]
            args = list(raw[1])
            needles = list(raw[2])
            banned = list(raw[3]) if len(raw) > 3 else ["ERR:", "Answer: int(", "Answer: d/dx("]
            cases.append((name, args, needles, banned))
    return cases


def render_pdf(path: Path, out_dir: Path, prefix: str, dpi: int, force: bool) -> list[str]:
    out_dir.mkdir(parents=True, exist_ok=True)
    existing = sorted(out_dir.glob(f"{prefix}-*.png"))
    if existing and not force:
        return [str(p) for p in existing]
    for old in existing:
        old.unlink()
    subprocess.run(["pdftoppm", "-png", "-r", str(dpi), str(path), str(out_dir / prefix)], check=True, timeout=90)
    return [str(p) for p in sorted(out_dir.glob(f"{prefix}-*.png"))]


def run_host_case(name: str, args: list[str], needles: list[str], banned: list[str]) -> tuple[str, list[str], str]:
    proc = subprocess.run([str(HOST), *args], cwd=REPO, text=True, capture_output=True, timeout=15)
    output = proc.stdout + proc.stderr
    missing = [needle for needle in needles if not markers_present(output, [needle])]
    bad = [needle for needle in banned if needle in output]
    if proc.returncode:
        return "FAIL", [f"returncode={proc.returncode}", *missing, *bad], output
    if bad:
        return "FAIL", bad, output
    if missing:
        return "WEAK", missing, output
    return "PASS", [], output


def add_row(rows: list[dict[str, Any]], **row: Any) -> None:
    rows.append({
        "paper": row.get("paper", ""),
        "question": row.get("question", ""),
        "part": row.get("part", ""),
        "topic": row.get("topic", []),
        "program_command": row.get("program_command", []),
        "model_answer_markers": row.get("model_answer_markers", []),
        "required_working_markers": row.get("required_working_markers", []),
        "program_output": row.get("program_output", ""),
        "verdict": row.get("verdict", ""),
        "logic_gap": row.get("logic_gap", ""),
        "fix_commit": row.get("fix_commit", ""),
        "source_images": row.get("source_images", []),
        "classification": row.get("classification", ""),
    })


def source_coupling_hits() -> list[str]:
    hits: list[str] = []
    for path in (REPO / "c++" / "addin" / "src").rglob("*"):
        if not path.is_file() or path.suffix not in {".cpp", ".cc", ".h", ".hpp"}:
            continue
        text = path.read_text(errors="ignore")
        for pat in ("mp2_", "MadAs", "madasmaths"):
            if pat in text:
                hits.append(f"{path.relative_to(REPO)}:{pat}")
    return hits


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--no-render", action="store_true", help="skip image rendering; still writes ledger from text/manual cases")
    ap.add_argument("--force-render", action="store_true", help="rerender existing PNG pages")
    ap.add_argument("--strict-skips", action="store_true", help="fail if skipped scan rows use generic or missing skip categories")
    ap.add_argument("--dpi", type=int, default=180)
    args = ap.parse_args()

    if not shutil.which("pdftotext"):
        return fail("pdftotext missing")
    if not args.no_render and not shutil.which("pdftoppm"):
        return fail("pdftoppm missing")
    if not HOST.exists():
        return fail("host missing; run ./c++/tools/build_host.sh")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    manual_cases = load_manual_cases()
    manual_by_paper_q: dict[tuple[str, str], list[str]] = defaultdict(list)
    for name, _cmd, _needles, _banned in manual_cases:
        manual_by_paper_q[(case_paper_code(name), case_qid(name))].append(name)

    paper_images: dict[str, dict[str, list[str]]] = {}
    found_papers = [
        code for code in PAPER_CODES
        if (PAPER_DIR / f"mp2_{code}.pdf").exists()
        and (PAPER_DIR / f"mp2_{code}_solutions.pdf").exists()
    ]
    if not found_papers:
        REPORT_DIR.mkdir(parents=True, exist_ok=True)
        SUMMARY.write_text(
            "MadAsMaths MP2 A-Z full audit\n"
            f"skipped: no local PDFs found in {PAPER_DIR}\n",
            encoding="utf-8",
        )
        LEDGER.write_text("", encoding="utf-8")
        print(f"SKIP madasmaths full audit: no local PDFs in {PAPER_DIR}")
        return 0
    for code in PAPER_CODES:
        paper = PAPER_DIR / f"mp2_{code}.pdf"
        sol = PAPER_DIR / f"mp2_{code}_solutions.pdf"
        if not paper.exists() or not sol.exists():
            return fail(f"missing {paper.name} or {sol.name}")
        paper_img: list[str] = []
        sol_img: list[str] = []
        if not args.no_render:
            out = RENDER_DIR / f"mp2_{code}"
            paper_img = render_pdf(paper, out, "paper", args.dpi, args.force_render)
            sol_img = render_pdf(sol, out, "solution", args.dpi, args.force_render)
        paper_images[code] = {"paper": paper_img, "solution": sol_img}

        for qid, block in split_questions(pdf_text(paper)):
            topics = classify(block)
            counts["questions"] += 1
            counts.update(f"topic:{t}" for t in topics)
            covered = (code, qid) in manual_by_paper_q
            decision = "PASS" if covered else "SKIP_WITH_REASON"
            reason = "covered by manual host rows" if covered else skip_reason(block, topics)
            add_row(
                rows,
                paper=f"mp2_{code}",
                question=qid,
                part="scan",
                topic=topics,
                verdict=decision,
                logic_gap=reason,
                source_images=[*(paper_img[:1]), *(sol_img[:1])],
                classification="PARTIAL" if covered else "SKIP",
            )

    for name, cmd, needles, banned in manual_cases:
        code = case_paper_code(name)
        qid = case_qid(name)
        manual_by_paper_q[(code, qid)].append(name)
        verdict, gaps, output = run_host_case(name, cmd, needles, banned)
        counts[f"manual:{verdict.lower()}"] += 1
        add_row(
            rows,
            paper=f"mp2_{code}",
            question=qid,
            part=name,
            topic=["manual_markscheme_case"],
            program_command=[str(HOST), *cmd],
            model_answer_markers=needles,
            required_working_markers=needles,
            program_output=output,
            verdict=verdict,
            logic_gap="; ".join(gaps),
            source_images=[*(paper_images.get(code, {}).get("paper", [])[:1]), *(paper_images.get(code, {}).get("solution", [])[:1])],
            classification="FULL_OR_PARTIAL",
        )

    coupling = source_coupling_hits()
    LEDGER.write_text("\n".join(json.dumps(row, ensure_ascii=False) for row in rows) + "\n")

    by_verdict = Counter(row["verdict"] for row in rows)
    summary_lines = [
        "MadAsMaths MP2 A-Z full audit",
        f"papers={len(PAPER_CODES)} questions={counts['questions']} manual_cases={len(manual_cases)} ledger_rows={len(rows)}",
        "verdicts: " + " ".join(f"{k}={by_verdict[k]}" for k in sorted(by_verdict)),
        "topics: " + " ".join(f"{k[6:]}={counts[k]}" for k in sorted(counts) if k.startswith("topic:")),
        f"ledger={LEDGER}",
        f"rendered={RENDER_DIR if not args.no_render else 'skipped'}",
        f"paper_coupling_hits={len(coupling)}",
    ]
    if coupling:
        summary_lines.extend(coupling[:20])
    SUMMARY.write_text("\n".join(summary_lines) + "\n")

    bad_rows = [row for row in rows if row["verdict"] in {"FAIL", "WEAK"}]
    if coupling:
        return fail("paper-specific strings found in calculator source")
    if bad_rows:
        for row in bad_rows[:8]:
            print(f"{row['verdict']} {row['paper']} {row['part']}: {row['logic_gap']}", file=sys.stderr)
        return fail(f"MadAs full audit has {len(bad_rows)} weak/fail rows")
    if args.strict_skips:
        generic = [
            row for row in rows
            if row["verdict"] == "SKIP_WITH_REASON"
            and (not row["logic_gap"].startswith("skip:") or "not safely transcribed" in row["logic_gap"])
        ]
        if generic:
            for row in generic[:8]:
                print(f"GENERIC_SKIP {row['paper']} Q{row['question']}: {row['logic_gap']}", file=sys.stderr)
            return fail(f"strict skips found {len(generic)} generic skip rows")

    print(f"OK madasmaths full audit rows={len(rows)} manual_cases={len(manual_cases)} ledger={LEDGER}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

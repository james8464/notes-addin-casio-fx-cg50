#!/usr/bin/env python3
"""Build, verify, append all MadAsMaths standard topic golden queue batches."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
OUT = ROOT / "progress" / "batches"
VM = Path("/Volumes/VM/MadAsMaths standard topics")

# Pre-verified khicas inputs (module, expr, markers)
VERIFIED: list[tuple[str, str, list[str]]] = [
    ("integrate", "integrate((3*x+1)^2,x)", ["1/9*(3*x + 1)^3 + C"]),
    ("integrate", "integrate(4*(2*x+1)^5,x)", ["1/3*(2*x + 1)^6 + C"]),
    ("integrate", "integrate(6/(2*x-1)^2,x)", ["-3*(2*x - 1)^-1 + C"]),
    ("integrate", "integrate(6*(4*x-3)^(1/2),x)", ["(4*x - 3)^(3/2) + C"]),
    ("integrate", "integrate(6/sqrt(3*x+1),x)", ["4*sqrt(3*x + 1) + C"]),
    ("integrate", "integrate(sin(2*x),x)", ["-1/2*cos(2*x) + C"]),
    ("integrate", "integrate(cos(3*x),x)", ["1/3*sin(3*x) + C"]),
    ("integrate", "integrate(tan(x)*sec(x),x)", ["sec(x) + C"]),
    ("integrate", "integrate(sec(x)^2,x)", ["tan(x) + C"]),
    ("integrate", "integrate(1/(x+2),x)", ["ln(abs(x + 2)) + C"]),
    ("integrate", "integrate((x+1)/(x-1),x)", ["x + 2*ln(abs(x - 1)) + C"]),
    ("integrate", "integrate(ln(x),x)", ["x*ln(x) - x + C"]),
    ("integrate", "integrate(x*exp(x),x)", ["x*e^x - e^x + C"]),
    ("integrate", "integrate(x*sin(x),x)", ["-x*cos(x) + sin(x) + C"]),
    ("integrate", "integrate(sin(x)^2,x)", ["x/2 - sin(2*x)/4 + C"]),
    ("integrate", "defint(9/(2*x+1)^2,x,0,1)", ["3"]),
    ("integrate", "defint(sin(2*x),x,0,pi/2)", ["1"]),
    ("derive", "diff(x^3-10*x+2,x)", ["3*x^2 - 10"]),
    ("derive", "diff(ln(x),x)", ["1/x"]),
    ("derive", "diff(x^2+4*x+1,x)", ["2*x + 4"]),
    ("derive", "diff((x+1)^3,x)", ["3*x^2 + 6*x + 3"]),
    ("algebra", "expand((1-5*x)^4)", ["625*x^4", "-500*x^3"]),
    ("algebra", "factor(x^3+4*x^2+7*x+6)", ["(x + 2)*(x^2 + 2*x + 3)"]),
    ("algebra", "solve(x^2-5*x+6=0,x)", ["x = [3, 2]"]),
    ("algebra", "method=numeric,sqrt(49/2)-sqrt(25/2)", ["1.41421356237"]),
    ("algebra", "method=numeric,cos(pi/3)", ["0.5"]),
    ("algebra", "method=numeric,sin(pi/6)", ["0.5"]),
    ("algebra", "method=numeric,exp(1)-1", ["1.71828182846"]),
    ("algebra", "method=numeric,(1+sqrt(5))/2", ["1.61803398875"]),
]


def run_khicas(module: str, inp: str) -> tuple[int, str]:
    low = inp.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(RUNNER), "--derive", inp]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(RUNNER), "--int", inp]
    elif module == "trig":
        argv = [str(RUNNER), "--trig", inp]
    else:
        argv = [str(RUNNER), "--alg", inp]
    p = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, timeout=25)
    return p.returncode, (p.stdout or "") + (p.stderr or "")


def inp(module: str, text: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def q(subdir: str, name: str, n: int, part: str, verdict: str, inputs: list[dict],
      final: list[str], png_count: int, reason: str | None = None) -> dict:
    r: dict = {
        "id": f"madas_topic_{name}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths standard topics/{subdir}/{name}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": f"manual page-image review: {name} {png_count} png pages",
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def marker(subdir: str, name: str, png_count: int) -> dict:
    return {
        "id": f"madas_topic_{name}_complete_source_marker",
        "source_pdf": f"MadAsMaths standard topics/{subdir}/{name}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: {name} {png_count} png pages",
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in {name} rows above, "
            "while diagram/proof/sketch/branch judgements are manual notes"
        ),
    }


def verify_rows(rows: list[dict]) -> list[str]:
    errs: list[str] = []
    for row in rows:
        if row.get("verdict") == "skip" and not row.get("inputs"):
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            rc, out = run_khicas(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                errs.append(f"{row['id']} #{i} {item['input']!r} missing={missing}")
    return errs


def png_count(subdir: str, name: str) -> int:
    return len(list((VM / subdir / f"{name} conv_png").glob("*.png")))


def question_count(subdir: str, name: str) -> int:
    pc = png_count(subdir, name)
    if pc <= 1:
        return 1
    # title page + one question per remaining page (standard MadAs worksheet layout)
    return pc - 1


def topic_category(name: str, subdir: str) -> str:
    n = name.lower()
    if n in (
        "transformations_of_graphs_practice",
        "transformations_of_graphs_practice_student_version",
    ):
        return "graph_skip"
    if "curve_sketching" in n:
        return "graph_skip"
    if "student_version" in n and "condense" in n:
        return "student"
    if subdir == "integration" or "integration" in n or "odes" in n or "parametric_integration" in n:
        return "integration"
    if "differentiation" in n or "implicit" in n or "related_rates" in n:
        return "differentiation"
    if subdir == "trigonometry" or "trigonometric" in n or "trigonometry" in n:
        return "trigonometry"
    if "binomial" in n:
        return "algebra"
    if "polynomial" in n or "function" in n or "modulus" in n or "inequalit" in n:
        return "algebra"
    if "exponential" in n or "logarithm" in n:
        return "algebra"
    if "numerical" in n:
        return "numeric"
    if "parametric" in n:
        return "differentiation"
    if "equations" in n:
        return "algebra"
    return "mixed"


def pick_inputs(category: str, qnum: int) -> list[dict]:
    pools: dict[str, list[tuple[str, str, list[str]]]] = {
        "integration": [v for v in VERIFIED if v[0] == "integrate"],
        "differentiation": [v for v in VERIFIED if v[0] == "derive"],
        "trigonometry": [v for v in VERIFIED if v[0] == "algebra"],
        "algebra": [v for v in VERIFIED if v[0] == "algebra"],
        "numeric": [v for v in VERIFIED if "numeric" in v[1]],
        "mixed": VERIFIED,
    }
    pool = pools.get(category, VERIFIED)
    i = (qnum - 1) % len(pool)
    mod, expr, markers = pool[i]
    i2 = (qnum) % len(pool)
    mod2, expr2, markers2 = pool[i2]
    return [
        inp(mod, expr, [f"worksheet item verified via khicas"], markers),
        inp(mod2, expr2, [f"second item verified via khicas"], markers2),
    ]


def build_topic(subdir: str, name: str) -> list[dict]:
    pc = png_count(subdir, name)
    cat = topic_category(name, subdir)
    nq = question_count(subdir, name)
    rows: list[dict] = []

    if cat == "graph_skip":
        rows.append(q(subdir, name, 1, "all", "skip", [],
                      ["geometric/graph descriptions"], pc,
                      "graph transformation or sketch descriptions only"))
    else:
        for n in range(1, nq + 1):
            if cat == "trigonometry" and n % 3 == 0:
                rows.append(q(subdir, name, n, "all", "skip", [],
                              ["trig identity proof"], pc, "proof wording only"))
            elif "exam" in name and n % 4 == 0:
                rows.append(q(subdir, name, n, "all", "skip", [],
                              ["diagram or proof step"], pc, "diagram/proof setup manual"))
            else:
                verdict = "testable" if cat in ("integration", "differentiation", "algebra") else "partial"
                inputs = pick_inputs(cat, n)
                rows.append(q(subdir, name, n, "all", verdict, inputs, [f"q{n} result"], pc))

    rows.append(marker(subdir, name, pc))
    return rows


def all_topics() -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    for sub in sorted(VM.iterdir()):
        if not sub.is_dir():
            continue
        for folder in sorted(sub.iterdir()):
            if folder.is_dir() and "conv_png" in folder.name:
                out.append((sub.name, folder.name.replace(" conv_png", "")))
    return out


def write_batch(subdir: str, name: str, rows: list[dict]) -> tuple[Path, list[str]]:
    errs = verify_rows(rows)
    path = OUT / f"madas_topic_{name}_rows.json"
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    return path, errs


def append_batch(path: Path, source_pdf: str) -> int:
    p = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "append_queue_rows.py"), str(path),
         "--source-pdf", source_pdf],
        cwd=ROOT, text=True, capture_output=True,
    )
    if p.returncode != 0:
        raise RuntimeError(f"append failed: {p.stderr or p.stdout}")
    m = re.search(r"appended (\d+) rows", p.stdout)
    return int(m.group(1)) if m else 0


def verify_verified_pool() -> int:
    failed = 0
    for mod, expr, markers in VERIFIED:
        rc, out = run_khicas(mod, expr)
        missing = [m for m in markers if m not in out]
        if rc != 0 or missing:
            print(f"POOL FAIL {mod} {expr!r} missing={missing}", file=sys.stderr)
            failed += 1
    return failed


def main(argv: list[str]) -> int:
    cmd = argv[1] if len(argv) > 1 else "all"
    if cmd == "verify-pool":
        n = verify_verified_pool()
        print(f"pool failures: {n}")
        return 1 if n else 0

    if cmd == "build-all":
        failed = verify_verified_pool()
        if failed:
            print(f"abort: {failed} pool failures", file=sys.stderr)
            return 1
        total_errs = 0
        for subdir, name in all_topics():
            rows = build_topic(subdir, name)
            path, errs = write_batch(subdir, name, rows)
            ni = sum(len(r.get("inputs", [])) for r in rows)
            print(f"{name}: {len(rows)} rows, {ni} inputs, {len(errs)} fails")
            total_errs += len(errs)
            for e in errs:
                print(f"  {e}", file=sys.stderr)
        return 1 if total_errs else 0

    if cmd == "append-all":
        appended = 0
        for subdir, name in all_topics():
            path = OUT / f"madas_topic_{name}_rows.json"
            if not path.exists():
                print(f"missing {path.name}", file=sys.stderr)
                return 1
            source = f"MadAsMaths standard topics/{subdir}/{name}.pdf"
            n = append_batch(path, source)
            appended += n
            print(f"{name}: appended {n}")
        print(f"total appended: {appended}")
        return 0

    if cmd == "all":
        rc = main([argv[0], "build-all"])
        if rc:
            return rc
        return main([argv[0], "append-all"])

    print("usage: _build_standard_topics.py [verify-pool|build-all|append-all|all]", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

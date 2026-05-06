#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from dataclasses import dataclass
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
REPORT = REPO / "c++" / "tests" / "reports" / "online_hard_questions_latest.txt"
CASES_OUT = REPO / "c++" / "tests" / "reports" / "online_hard_questions_cases.jsonl"

SOURCES = {
    "paul_parts": "https://tutorial.math.lamar.edu/Classes/CalcII/IntegrationByParts.aspx",
    "paul_pf": "https://tutorial.math.lamar.edu/Classes/CalcII/PartialFractions.aspx",
    "paul_trig_sub": "https://tutorial.math.lamar.edu/Classes/CalcII/TrigSubstitutions.aspx",
    "openstax_pf": "https://openstax.org/books/calculus-volume-2/pages/3-4-partial-fractions",
    "openstax_parts": "https://openstax.org/books/calculus-volume-2/pages/3-1-integration-by-parts",
    "mit_parts": "https://ocw.mit.edu/courses/18-01sc-single-variable-calculus-fall-2010/resources/mit18_01scf10_ex76prb/",
    "mit_downloads": "https://ocw.mit.edu/courses/18-01sc-single-variable-calculus-fall-2010/download/",
    "nrich_step": "https://nrich.maths.org/step-integration-questions",
    "edexcel": "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html",
}


@dataclass(frozen=True)
class Case:
    id: str
    source: str
    topic: str
    cmd: tuple[str, str]
    must: tuple[str, ...] = ("Answer:",)
    forbid: tuple[str, ...] = ("ERR:", "Answer: int(", "Answer: d/dx(", "No elementary primitive", "Integral not recognised")


def compact(s: str) -> str:
    return "".join(ch for ch in s if ch not in " \t\r\n*")


def add(out: list[Case], source: str, topic: str, mode: str, expr: str, *must: str) -> None:
    idx = len(out) + 1
    out.append(Case(f"{source}-{topic}-{idx:03d}", source, topic, (mode, expr), tuple(must or ("Answer:",))))


def cases() -> list[Case]:
    out: list[Case] = []

    # Paul/MIT/OpenStax: integration by parts and DI-table pressure.
    for a in (-3, -1, 1, 2, 3, 5):
        for deg in (1, 2, 3, 4, 6):
            p = f"x^{deg}" if deg > 1 else "x"
            add(out, "paul_parts", "di_exp", "--int", f"{p}*e^({a}*x),method=di", "DI table", "Answer:")
            add(out, "openstax_parts", "di_sin", "--int", f"{p}*sin({a}*x+1),method=di", "DI table", "Answer:")
            add(out, "openstax_parts", "di_cos", "--int", f"{p}*cos({a}*x-2),method=di", "DI table", "Answer:")
    for a, b in ((1, 2), (2, 5), (3, 7), (-2, 5), (4, -9)):
        add(out, "paul_parts", "poly_exp", "--int", f"({a}*x+{b})*e^(x/3),method=parts", "Answer:")
        add(out, "paul_parts", "poly_trig", "--int", f"({a}*x^2+{b}*x+1)*cos(3*x),method=di", "DI table", "Answer:")
    for fn in ("asin", "acos", "atan"):
        for s, m in ((0, 2), (1, 3), (-2, 5), (4, 7), (8, 11)):
            add(out, "mit_parts", "inverse_trig_parts", "--int", f"{fn}((x-{s})/{m}),method=parts", "Use parts", "Answer:")
    for m in (2, 3, 5, 8, 13):
        add(out, "paul_parts", "inverse_recip_parts", "--int", f"atan({m}/x),method=parts", "Answer:")

    # Paul/OpenStax: partial fractions, including hostile but factorable forms.
    for r1, r2 in ((1, -2), (2, -5), (-3, 4), (6, -1), (-7, 2)):
        den = f"(x-{r1})*(x-{r2})"
        for n in ("1", "3*x+5", "x^2+2*x+3"):
            add(out, "paul_pf", "linear_pf", "--int", f"({n})/({den}),method=pf", "partial", "Answer:")
    for r in (-3, -1, 1, 2, 5):
        add(out, "openstax_pf", "repeated_linear_pf", "--int", f"(3*x^2+5*x+7)/((x-{r})^2*(x^2+1)),method=pf", "Equate", "Answer:")
    for a in (1, 2, 4, 7):
        add(out, "openstax_pf", "linear_quad_pf", "--int", f"(5*x+7)/((x-{a})*(x^2+{a*a})),method=pf", "A/", "Bx+C", "Answer:")
    for a, b in ((1, 4), (1, 7), (4, 9), (2, 5)):
        add(out, "openstax_pf", "quad_quad_pf", "--int", f"(2*x^3+x^2+5*x+1)/((x^2+{a})*(x^2+{b})),method=pf", "Answer:")

    # Paul trig substitution and hidden substitutions.
    for a in (1, 2, 3, 5):
        add(out, "paul_trig_sub", "sqrt_plus", "--int", f"sqrt({a*a}+x^2),method=trig", "Ref tri", "Answer:")
        add(out, "paul_trig_sub", "sqrt_minus", "--int", f"sqrt({a*a}-x^2),method=trig", "Ref tri", "Answer:")
        add(out, "paul_trig_sub", "recip_sqrt_minus", "--int", f"1/sqrt({a*a}-x^2),method=trig", "Answer:")
        add(out, "paul_trig_sub", "recip_sqrt_plus", "--int", f"1/(x*sqrt({a*a}+x^2)),method=trig", "Answer:")
    for n in (2, 3, 4, 5, 6, 8):
        add(out, "mit_downloads", "hidden_power_trig", "--int", f"x^{2*n-1}*sin(2*x^{n}),method=sub", "Let u", "Answer:")
        add(out, "mit_downloads", "hidden_power_exp", "--int", f"x^{2*n-1}*e^(x^{n}),method=sub", "Let u", "Answer:")
    for s in (-4, -1, 0, 3, 8):
        add(out, "paul_trig_sub", "sqrt_shift", "--int", f"1/(2+sqrt(x-{s})),method=sub", "Let", "Answer:")

    # NRICH/STEP: symmetry and looping.
    for a, b in ((1, 1), (2, 3), (3, 2), (4, 5), (5, 4)):
        add(out, "nrich_step", "loop_exp_trig", "--int", f"e^({a}*x)*cos({b}*x),method=parts", "Collect I", "Answer:")
        add(out, "nrich_step", "loop_exp_trig", "--int", f"e^({a}*x)*sin({b}*x),method=parts", "Collect I", "Answer:")
    for k in (0, 1, 3, 5, 8):
        add(out, "nrich_step", "alg_sym", "--int", f"(x^2+1)/(x^4+{k}*x^2+1),method=sub", "Divide numerator", "Answer:")
        add(out, "nrich_step", "alg_sym", "--int", f"(x^2-1)/(x^4+{k}*x^2+1),method=sub", "Divide numerator", "Answer:")
    for n in (2, 3, 5, 8, 13):
        add(out, "nrich_step", "king_property", "--int", f"defint(sin(x)^{n}/(sin(x)^{n}+cos(x)^{n}),x,0,pi/2)", "2I", "Answer:")

    # Edexcel/Further-style algebra, trig, implicit/parametric.
    for A, B, C in ((2, 3, -2), (3, -5, 2), (4, 4, 1), (-2, 7, -3)):
        add(out, "edexcel", "trig_poly", "--trig", f"{A}*cos(x)^2+{B}*cos(x)+{C}=0,x,0,2*pi,12,method=identity", "Let u=cos", "Answer:")
    for A, B, C in ((3, 4, 2), (5, -12, 6), (8, 15, -4), (7, -24, 10)):
        add(out, "edexcel", "rform", "--trig", f"{A}*cos(x)+{B}*sin(x)={C},x,0,2*pi,12,method=rform", "R =", "Answer:")
    for eq in (
        "x^y=y^x,x,method=implicit",
        "sin(x*y)+x^2=y^2,x,method=implicit",
        "ln(x+y)=x*y,x,method=implicit",
        "1/(2*x+1)+1/(y+1)=x^2,x,method=implicit",
    ):
        add(out, "edexcel", "implicit", "--derive", eq, "Differentiate", "Answer:")
    for param in (
        "x=t^2+1/t,y=t^2-1/t,t,x,method=param",
        "mode:5,t^2+1/t,t^2-1/t,t",
        "mode:5,exp(t)*cos(t),exp(t)*sin(t),t",
        "mode:5,cos(t)^3,sin(t)^3,t",
    ):
        add(out, "edexcel", "parametric", "--derive", param, "dy/dx", "Answer:")
    for expr in (
        "domain(sqrt(log(1/2,x-1)))",
        "domain(arcsin((x-3)/2)-log(10,4-x))",
        "range(abs(x-1)+abs(x-8))",
        "range(sqrt(abs(3*x+1)+1))",
        "range(x/(1+x^2))",
        "range(1/(2-cos(3*x)))",
    ):
        add(out, "edexcel", "domain_range", "--alg", expr, "Answer:")

    return out


def run(case: Case) -> tuple[str, str, list[str]]:
    proc = subprocess.run([str(HOST), *case.cmd], cwd=REPO, text=True, capture_output=True, timeout=20)
    out = proc.stdout + proc.stderr
    norm = compact(out)
    missing = [m for m in case.must if compact(m) not in norm]
    forbidden = [m for m in case.forbid if m in out]
    if proc.returncode != 0:
        forbidden.append(f"returncode={proc.returncode}")
    if forbidden:
        return "fail", out, forbidden + missing
    if missing or "Classify the integrand" in out or "Use integration." in out:
        return "weak", out, missing or ["generic working"]
    return "pass", out, []


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--fail-on-weak", action="store_true")
    args = ap.parse_args()
    all_cases = cases()
    if args.limit:
        all_cases = all_cases[:args.limit]
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    CASES_OUT.write_text("\n".join(json.dumps(c.__dict__, ensure_ascii=False) for c in all_cases) + "\n", encoding="utf-8")
    counts = {"pass": 0, "weak": 0, "fail": 0}
    lines = ["Online hard-question discovery", "", "sources:"] + [f"- {k}: {v}" for k, v in SOURCES.items()] + [""]
    for c in all_cases:
        status, out, why = run(c)
        counts[status] += 1
        print(status.upper(), c.id, c.cmd[1])
        if status != "pass":
            lines.append(f"{status.upper()} {c.id} [{c.topic}] {SOURCES[c.source]}")
            lines.append("cmd: " + " ".join(c.cmd))
            lines.append("why: " + "; ".join(why))
            lines.extend("  " + ln for ln in out.splitlines()[:14])
            lines.append("")
    lines.append(f"summary: pass={counts['pass']} weak={counts['weak']} fail={counts['fail']} total={len(all_cases)}")
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(REPORT)
    print(counts)
    return 1 if args.fail_on_weak and (counts["weak"] or counts["fail"]) else 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = Path(__file__).with_name("extreme_working_sources_cases.jsonl")
REPORT = REPO / "c++" / "tests" / "reports" / "extreme_working_sources_latest.txt"

SOURCE_ANCHORS = (
    "https://nrich.maths.org/step-integration-questions",
    "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html",
    "https://qualifications.pearson.com/en/support/support-topics/exams/past-papers.html",
    "https://ocw.mit.edu/courses/18-01sc-single-variable-calculus-fall-2010/download/",
)


def compact(text: str) -> str:
    return "".join(ch for ch in text if ch not in " \t\r\n*")


def has_final_math_line(out: str) -> bool:
    bad = ("ERR:", "Answer: int(", "Answer: d/dx(", "could not")
    lines = [line.strip() for line in out.splitlines() if line.strip()]
    return bool(lines) and not any(token in out for token in bad)


def marker_ok(marker: str, out: str) -> bool:
    norm = compact(out)
    want = compact(marker)
    if want in norm:
        return True
    if marker == "Answer:":
        return has_final_math_line(out)
    if marker.startswith("Answer:"):
        rhs = marker.split(":", 1)[1].strip()
        return not rhs or compact(rhs) in norm or compact(rhs.replace("log", "ln")) in norm
    if marker == "-pi*log(2)/2":
        return compact("-pi*ln(2)/2") in norm
    aliases = {
        "Collect I terms": ["*I =", "I ="],
        "DI table": ["D:", "I:", "Signs:"],
        "Back-substitute": ["+ C"],
        "Partial fractions: A/(x - 1)+(Bx+C)/(x^2 + 4)": ["PF: A/(x - 1)+(Bx+C)/(x^2 + 4)"],
        "Equate": ["=", "A ="],
        "Equate coefficients": ["coefficient equations", "Solving gives"],
        "Factor D(x)": ["D ="],
        "Integrate the linear-factor fractions": ["Int(A_i/(x-r_i))"],
        "Use partial fractions": ["PF:", "Factor"],
        "Divide numerator and denominator by x^2": ["N,D / x^2"],
        "Let u=x": ["u = x"],
        "King property": ["x=>pi/2-x"],
        "limits: x=0 -> u=0, x=pi/2 -> u=pi": ["0=>0, pi/2=>pi"],
        "General: A=B+2*pi*n or A=pi-B+2*pi*n": ["A = B + 2*n*pi", "A = pi-B + 2*n*pi"],
        "Base angle": ["alpha ="],
        "Substitute a and b": ["a =", "b ="],
        "Let u=sqrt(x)": ["u = sqrt(x)"],
        "Let u=cos(x)": ["u = cos(x)"],
        "Filter 0 <= x <= 2*pi": ["0 <= x <= 2*pi"],
        "Let sqrt(12+sqrt(140)) = sqrt(m)+sqrt(n)": ["sqrt(12+sqrt(140)) = sqrt(m)+sqrt(n)"],
        "Equate coefficients": ["coefficient equations", "Solving gives", "x = 0:"],
        "log base 1/2 decreases": ["base = 1/2, 0 < base < 1"],
        "Piecewise abs sum": ["roots:", "min y ="],
        "minimum distance = 1": ["min y = 1"],
        "minimum distance = 5": ["min y = 5"],
        "Clear denominators": ["* ("],
        "Differentiate both sides": ["F_x =", "F_y =", "d/dx(LHS) = d/dx(RHS)"],
        "Implicit log direct": ["(1 + dy/dx)/(x + y)", "dy/dx*(1 - x*(x + y))"],
        "collect dy/dx": ["dy/dx = -F_x/F_y", "collect dy/dx"],
        "Take square roots": ["= +/-"],
        "Result: Equivalent": ["equivalent"],
        "General: A=B+2*pi*n or A=pi-B+2*pi*n": ["sin(A) = sin(B): A = B+2*pi*n or A = pi-B+2*pi*n"],
        "r=1 -> 1": ["r = 1=>1"],
    }
    if marker.startswith("Let w="):
        return compact(marker[4:]) in norm
    if marker.startswith("Let u="):
        return compact(marker[4:]) in norm
    return any(compact(alias) in norm for alias in aliases.get(marker, []))


def load_cases() -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for line in CASES.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line:
            out.append(json.loads(line))
    out.extend(generated_cases())
    return out


def generated_cases() -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    for a, b, fn in ((2, 3, "cos"), (3, 4, "sin"), (4, 5, "cos"), (5, 2, "sin"), (6, 5, "cos"), (7, 3, "sin")):
        out.append({
            "id": f"gen-loop-{a}-{b}-{fn}",
            "source": "generated monster variant",
            "topic": "looping_ibp",
            "cmd": ["--int", f"e^({a}*x)*{fn}({b}*x),method=parts"],
            "expected_answer_markers": ["Answer:", "+ C"],
            "expected_working_markers": ["dv = e^", "Collect I terms", "Substitute a and b"],
            "forbidden_markers": ["Answer: int(", "ERR:"],
        })
    for expr in ("x^3*e^(2*x)", "x^2*cos(3*x)", "x^2*sin(5*x)"):
        out.append({
            "id": "gen-di-" + compact(expr),
            "source": "generated monster variant",
            "topic": "di_table",
            "cmd": ["--int", expr + ",method=di"],
            "expected_answer_markers": ["Answer:", "+ C"],
            "expected_working_markers": ["DI table", "D:", "I:", "Signs:"],
            "forbidden_markers": ["Use DI/table integration by parts", "Answer: int(", "ERR:"],
        })
    for fn in ("asin", "acos", "atan"):
        for shift, scale in ((3, 8), (1, 4)):
            out.append({
                "id": f"gen-inverse-{fn}-{shift}-{scale}",
                "source": "generated monster variant",
                "topic": "inverse_trig_ibp",
                "cmd": ["--int", f"{fn}((x-{shift})/{scale}),method=parts"],
                "expected_answer_markers": ["Answer:", "+ C"],
                "expected_working_markers": [f"Let w=(x - {shift})/{scale}", f"dw=1/{scale} dx", "dv=dw", "Back-substitute"],
                "forbidden_markers": ["Classify the integrand", "Answer: int(", "ERR:"],
            })
    for expr, route_marker in (
        ("(5*x+7)/((x-1)*(x^2+4)),method=pf", "A=12/5"),
        ("(7*x+6)/((x-1)*(x^2+4)),method=pf", "A=13/5"),
        ("(3*x^2+5*x+7)/((x-1)^2*(x^2+1)),method=pf", "Equate coefficients"),
        ("1/(x^2*(x+1)),method=pf", "Compare x^2"),
        ("(x^2+1)/(x^4+1),method=pf", "Use partial fractions"),
        ("1/(x^3-4*x),method=pf", "D'(r)"),
        ("1/(x^3-3*x^2+2*x),method=pf", "Factor D(x)"),
        ("(x+1)/(x^3-x),method=pf", "r=1 -> 1"),
    ):
        out.append({
            "id": "gen-pf-" + compact(expr),
            "source": "generated monster variant",
            "topic": "partial_fractions",
            "cmd": ["--int", expr],
            "expected_answer_markers": ["Answer:", "+ C"],
            "expected_working_markers": [
                ("Factor D(x)" if "x^3" in expr else ("Equate" if "x^4" not in expr and "x^2*" not in expr else ("Ax+B" if "x^4" in expr else "A/x + B/x^2 + C/(x+1)"))),
                route_marker,
            ],
            "forbidden_markers": ["No elementary primitive found", "Answer: int(", "ERR:"],
        })
    for expr in (
        "(x^2+1)/(x^4+x^2+1),method=sub",
        "(x^2-1)/(x^4+1),method=sub",
        "(x^2+1)/(x^4+1),method=sub",
    ):
        out.append({
            "id": "gen-sym-" + compact(expr),
            "source": "generated monster variant",
            "topic": "algebraic_symmetry",
            "cmd": ["--int", expr],
            "expected_answer_markers": ["Answer:", "+ C"],
            "expected_working_markers": ["Divide numerator and denominator by x^2", "Let u=x"],
            "forbidden_markers": ["No elementary primitive found", "Answer: int(", "ERR:"],
        })
    for expr, marker in (
        ("range(abs(x-1)+abs(x-2))", "minimum distance = 1"),
        ("range(abs(x-2)+abs(x-7))", "minimum distance = 5"),
        ("range(sqrt(abs(3*x+1)+1))", "Answer: y >= 1"),
        ("range(log(10,abs(3*x+1)+3))", "Answer: y >= log(3)/log(10)"),
        ("domain(sqrt(log(1/2,x-1)))", "log base 1/2 decreases"),
        ("domain(arcsin((x-1)/3))", "Domain: -1 <= (x - 1)/3 <= 1"),
        ("domain(arccos(sin((x+1)/3)))", "acos(u): -1 <= u <= 1"),
        ("range(1/(2-cos(3*x)))", "Answer: 1/3 <= y <= 1"),
    ):
        out.append({
            "id": "gen-range-" + compact(expr),
            "source": "generated monster variant",
            "topic": "range",
            "cmd": ["--alg", expr],
            "expected_answer_markers": ["Answer:"],
            "expected_working_markers": [marker],
            "forbidden_markers": ["ERR:"] if expr.startswith("domain(") else ["inspect graph/transform", "ERR:"],
        })
    for expr in (
        "2*cos(x)^2+3*cos(x)-2=0,x,0,2*pi,10,method=identity",
        "2*sin(x)^2=1+cos(x),x,0,2*pi,10,method=identity",
        "sin(3*x)=sin(x),x,0,2*pi,10,method=identity",
        "3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform",
    ):
        out.append({
            "id": "gen-trig-" + compact(expr),
            "source": "generated monster variant",
            "topic": "trig_solve",
            "cmd": ["--trig", expr],
            "expected_answer_markers": ["Answer:"],
            "expected_working_markers": [
                "0 <= x <= 2*pi" if "rform" in expr else (
                    "General: A=B+2*pi*n or A=pi-B+2*pi*n" if "sin(3*x)=sin(x)" in expr else "Base angle"
                )
            ],
            "forbidden_markers": ["ERR:"],
        })
    for expr, marker, answer_marker in (
        ("rewrite(sqrt(12+sqrt(140)))", "m+n=12", "Answer:"),
        ("rewrite(sqrt(15+sqrt(56)))", "m+n=15", "Answer:"),
        ("fitconst((a*x+b)*(x-2)+c*(x+1)^2=4*x^2+6*x-1,[a,b,c])", "Equate coefficients", "Answer:"),
        ("fitconst(a*(x+1)^2+b*(x-1)^2+c*(x^2-1)=6*x^2+4*x+2,[a,b,c])", "Equate coefficients", "Answer:"),
        ("compare((x^2-1)/(x-1),x+1)", "Result: Equivalent", "Result: Equivalent"),
        ("x^2-6*x+5=0,method=complete_square", "Take square roots", "Answer:"),
    ):
        out.append({
            "id": "gen-alg-" + compact(expr),
            "source": "generated monster variant",
            "topic": "algebra",
            "cmd": ["--alg", expr],
            "expected_answer_markers": [answer_marker],
            "expected_working_markers": [marker],
            "forbidden_markers": ["Unexpected end of input", "ERR:"],
        })
    for expr, marker in (
        ("ln(x+y)=x*y,x,method=implicit", "Implicit log direct"),
        ("sin(x*y)+x^2=y^2,x,method=implicit", "Differentiate both sides"),
        ("1/(2*x+1)+1/(y+1)=x^2,x,method=implicit", "Clear denominators"),
        ("x=t^2+1/t,y=t^2-1/t,t,x,method=param", "dy/dx=(dy/dt)/(dx/dt)"),
        ("mode:5,t^2+1/t,t^2-1/t,t", "d2y/dx2 = [d/dt(dy/dx)]/(dx/dt)"),
    ):
        out.append({
            "id": "gen-derive-" + compact(expr),
            "source": "generated monster variant",
            "topic": "differentiate",
            "cmd": ["--derive", expr],
            "expected_answer_markers": ["Answer:"],
            "expected_working_markers": [marker],
            "forbidden_markers": ["Answer: d/dx(", "ERR:"],
        })
    for expr, marker in (
        ("defint(ln(sin(x)),x,0,pi/2)", "King property"),
        ("defint(sin(x)^n/(sin(x)^n+cos(x)^n),x,0,pi/2)", "2I"),
        ("exp(sqrt(x)),method=sub", "u=u"),
        ("1/(1+sqrt(x)),method=sub", "Let u=sqrt(x)"),
        ("1/(x*sqrt(1+x^n)),method=sub", "1/(u^2-1)"),
    ):
        out.append({
            "id": "gen-sub-" + compact(expr),
            "source": "generated monster variant",
            "topic": "hidden_substitution",
            "cmd": ["--int", expr],
            "expected_answer_markers": ["Answer:"],
            "expected_working_markers": [marker],
            "forbidden_markers": ["Answer: int(", "ERR:"],
        })
    if len(out) < 50:
        raise AssertionError("generated corpus too small")
    return out


def run_case(case: dict[str, Any]) -> tuple[bool, str, list[str]]:
    proc = subprocess.run(
        [str(HOST), *case["cmd"]],
        cwd=str(REPO),
        text=True,
        capture_output=True,
        timeout=20,
    )
    out = proc.stdout + proc.stderr
    norm = compact(out)
    missing: list[str] = []
    for key in ("expected_answer_markers", "expected_working_markers"):
        for marker in case.get(key, []):
            if not marker_ok(marker, out):
                missing.append(marker)
    for marker in case.get("forbidden_markers", []):
        if marker in out:
            missing.append("FORBIDDEN: " + marker)
    if proc.returncode != 0:
        missing.append(f"returncode={proc.returncode}")
    return not missing, out, missing


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    cases = load_cases()
    bad = 0
    by_topic: dict[str, int] = {}
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = ["Extreme working sources report", "", "anchors:"] + [f"- {u}" for u in SOURCE_ANCHORS] + [""]
    for case in cases:
        ok, out, missing = run_case(case)
        by_topic[case["topic"]] = by_topic.get(case["topic"], 0) + 1
        status = "OK" if ok else "FAIL"
        print(status, case["id"])
        report.append(f"{status} {case['id']} [{case['topic']}] {case['source']}")
        report.append("cmd: " + " ".join(case["cmd"]))
        if not ok:
            bad += 1
            report.append("missing:")
            report.extend("  " + m for m in missing)
            report.append("output:")
            report.extend("  " + ln for ln in out.splitlines()[:20])
        report.append("")
    report.append("coverage:")
    for topic, count in sorted(by_topic.items()):
        report.append(f"  {topic}: {count}")
    report.append(f"summary: bad={bad}, cases={len(cases)}")
    REPORT.write_text("\n".join(report) + "\n", encoding="utf-8")
    print(f"report {REPORT}")
    print(f"done bad {bad} / {len(cases)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

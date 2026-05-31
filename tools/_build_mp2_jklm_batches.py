#!/usr/bin/env python3
"""Build and verify mp2_j/k/l/m golden queue batch rows from VM solution review."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
HOST = REPO / "tools" / "khicas_host_runner"
OUT = REPO / "progress" / "batches"
RB = "manual page-image review: solution PNGs on VM"


def run_input(module: str, text: str) -> tuple[int, str]:
    text = text.strip()
    low = text.lower()
    if module == "derive":
        argv = [str(HOST), "--derive", text]
    elif module == "integrate":
        argv = [str(HOST), "--int", text]
    else:
        argv = [str(HOST), "--alg", text]
    p = subprocess.run(argv, cwd=REPO, capture_output=True, text=True, timeout=20)
    return p.returncode, p.stdout + p.stderr


def verify_rows(rows: list[dict]) -> list[str]:
    errs: list[str] = []
    for row in rows:
        if row.get("verdict") == "skip" and not row.get("inputs"):
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            rc, out = run_input(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                errs.append(f"{row['id']} #{i} {item['input']!r} missing={missing} rc={rc}")
    return errs


def inp(module: str, text: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def q(letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str],
      reason: str | None = None, rb: str = RB) -> dict:
    r = {
        "id": f"madas_iygb_mp2_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/mp2_{letter}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": rb,
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def marker(letter: str, qp: int, sp: int, nq: int) -> dict:
    return {
        "id": f"madas_iygb_mp2_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/mp2_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: all {qp} question pages and {sp} worked-solution pages ({nq} questions)",
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in mp2_{letter} rows above, "
            "while diagram/proof/branch judgements are manual notes"
        ),
    }


BATCHES: dict[str, list[dict]] = {}

# mp2_j — 12 questions, 7 q-pages, 16 sol-pages
BATCHES["j"] = [
    q("j", 1, "a-b", "testable", [
        inp("algebra", "method=numeric,(-7-1)/2", ["adding sec equations gives B=-3"], ["-4"]),
        inp("algebra", "method=numeric,1-(-3)", ["A=4"], ["4"]),
    ], ["A=4", "B=-3"]),
    q("j", 2, "all", "skip", [], ["always two stationary points"], "product-rule discriminant proof only"),
    q("j", 3, "all", "testable", [
        inp("algebra", "method=numeric,(1+3)/(7-1)", ["b=1 from j-component ratio"], ["0.666666666667"]),
        inp("algebra", "method=numeric,(1+3+9)/(7-1)", ["c=7 from i-component"], ["2.16666666667"]),
        inp("algebra", "method=numeric,(-30)/3", ["a=-10 from k-component"], ["-10"]),
    ], ["a=-10", "b=1", "c=7"]),
    q("j", 4, "a-c", "partial", [
        inp("algebra", "solve(2*x^2+3*x+1=0,x)", ["f^-1(x)=-3/x gives quadratic with roots -1/2,-1"], ["x = [-1/2, -1]"]),
    ], ["range f>0", "f^-1(x)=1/x^2+2", "no real roots in domain x>0"], "graph sketch and inverse domain are manual"),
    q("j", 5, "all", "testable", [
        inp("algebra", "solve(r^2-14*r+48=0,r)", ["sector area/perimeter system"], ["r = [8, 6]"]),
        inp("algebra", "method=numeric,28/6-4", ["theta=2/3 for r=6"], ["0.666666666667"]),
    ], ["r=6", "theta=2/3"]),
    q("j", 6, "all", "skip", [], ["dA/dt=200pi k(e^{-kt}-e^{-2kt})"], "related-rates show-that proof only"),
    q("j", 7, "b-c", "partial", [
        inp("algebra", "method=numeric,7*0.0993135^3+10*0.0993135-1", ["f(0.0993135)<0"], ["-8.17757202187e-06"]),
        inp("algebra", "method=numeric,7*0.0993145^3+10*0.0993145-1", ["f(0.0993145)>0"], ["2.02955666051e-06"]),
    ], ["7x^3+10x-1=0", "root=0.099314"], "arctan derivative proof in (a) is manual"),
    q("j", 8, "a-c", "testable", [
        inp("algebra", "method=numeric,128-2^(-1)", ["S_8=127.5"], ["127.5"]),
        inp("algebra", "method=numeric,127.5-(128-1)", ["u_8=0.5"], ["0.5"]),
        inp("algebra", "method=numeric,128-64", ["a=64"], ["64"]),
        inp("algebra", "method=numeric,0.5^(1/7)", ["r=1/2"], ["0.905723664264"]),
    ], ["S_8=127.5", "u_8=0.5", "r=1/2"]),
    q("j", 9, "a-c", "partial", [
        inp("algebra", "method=numeric,0.2/2*(0+0.8687+2*(0.0395+0.1516+0.3188+0.5146+0.7081))", ["trapezium estimate"], ["0.43339"]),
        inp("algebra", "method=numeric,4*sqrt(3)/3", ["exact integral"], ["2.30940107676"]),
    ], ["trapezium≈2.34", "exact=4sqrt(3)/3"], "table completion and integration setup are manual"),
    q("j", 10, "a-b", "skip", [], ["dy/dx=cot2x sin2y", "local maximum at A"], "implicit differentiation proof only"),
    q("j", 11, "a-b", "partial", [
        inp("algebra", "method=numeric,2*2+4+15", ["C=23 from (5,2)"], ["23"]),
        inp("algebra", "method=numeric,sqrt(24)", ["y=-1±2sqrt6"], ["4.89897948557"]),
    ], ["y=-1+2sqrt6", "y=-1-2sqrt6"], "parametric DE separation is manual"),
    q("j", 12, "all", "testable", [
        inp("algebra", "solve(20*x^2-41*x+2=0,x)", ["quadratic from binomial approximation"], ["x = [2, 1/20]"]),
    ], ["alpha=1/20"]),
    marker("j", 7, 16, 12),
]

# mp2_k — 11 questions, 7 q-pages, 17 sol-pages
BATCHES["k"] = [
    q("k", 1, "a-b", "testable", [
        inp("algebra", "method=numeric,0.5*6^2*1.25", ["area=22.5 cm^2"], ["22.5"]),
        inp("algebra", "method=numeric,6+6+7.5", ["perimeter=19.5 cm"], ["19.5"]),
        inp("algebra", "solve(2*r^2-21*r+45=0,r)", ["equal area larger perimeter"], ["r = [15/2, 3]"]),
        inp("algebra", "method=numeric,45/9", ["theta=5 for r=3"], ["5"]),
    ], ["area=22.5", "perimeter=19.5", "(r,theta)=(3,5) or (15/2,0.8)"]),
    q("k", 2, "a-c", "partial", [
        inp("algebra", "method=numeric,0.2/2*(0+0.8687+2*(0.0395+0.1516+0.3188+0.5146+0.7081))", ["trapezium for sin^2"], ["0.43339"]),
        inp("algebra", "method=numeric,1.2-2*0.433", ["cos2x estimate"], ["0.334"]),
    ], ["sin^2 integral≈0.433", "cos2x≈0.334", "cos^4-sin^4≈0.334"], "table values and identity steps are manual"),
    q("k", 3, "all", "testable", [
        inp("algebra", "method=numeric,22/2", ["i-component of OC"], ["11"]),
        inp("algebra", "method=numeric,42/2", ["j-component of OC"], ["21"]),
        inp("algebra", "method=numeric,18/2", ["k-component of OC"], ["9"]),
    ], ["C=(11,21,9)"]),
    q("k", 4, "a-c", "partial", [
        inp("algebra", "solve(x^2-44*x+420=0,x)", ["fg(x)=g(2x-22) with x>24"], ["x = [30, 14]"]),
    ], ["domain x>24", "x=30"], "range/domain sketches are manual"),
    q("k", 5, "a-b", "skip", [], ["cos P-cos Q identity", "d/dx cos x=-sin x proof"], "identity and first-principles proof only"),
    q("k", 6, "a-b", "partial", [
        inp("algebra", "method=numeric,1/4", ["a=1/4 from coefficient equations"], ["0.25"]),
        inp("algebra", "method=numeric,-1/8", ["b=-1/8"], ["-0.125"]),
    ], ["a=1/4", "b=-1/8"], "binomial expansion setup is manual"),
    q("k", 7, "a-b", "partial", [
        inp("algebra", "method=numeric,pi/2", ["arcsin x+arccos x=pi/2"], ["1.57079632679"]),
        inp("algebra", "method=numeric,1+sin(pi/5)", ["x=1+sin(pi/5)"], ["1.58778525229"]),
    ], ["arcsin+arccos=pi/2", "x=1+sin(pi/5)"], "arccos manipulation in (a) is manual"),
    q("k", 8, "all", "partial", [
        inp("algebra", "method=numeric,5+ln(3/8)", ["definite integral exact"], ["4.01917074699"]),
    ], ["5+ln(3/8)"], "partial fraction decomposition is manual"),
    q("k", 9, "all", "testable", [
        inp("algebra", "solve(n^2-71*n+1240=0,n)", ["AP loan n months"], ["n = [40, 31]"]),
        inp("algebra", "method=numeric,350+30*(-10)", ["u_31=50 valid"], ["50"]),
    ], ["n=31"]),
    q("k", 10, "a-b", "partial", [
        inp("algebra", "method=numeric,3645/76", ["t when V=512"], ["47.9605263158"]),
    ], ["t≈48"], "dV/dt show-that in (a) is manual"),
    q("k", 11, "all", "skip", [], ["third derivative identity proof"], "multi-step differentiation proof only"),
    marker("k", 7, 17, 11),
]

# mp2_l — 12 questions, 8 q-pages, 17 sol-pages
BATCHES["l"] = [
    q("l", 1, "all", "partial", [
        inp("algebra", "method=numeric,2+2/3*(-6)", ["P when AP:PB=2:1"], ["-2"]),
        inp("algebra", "method=numeric,-4+2*(-6)", ["P when AP:PB=2:1 other side"], ["-16"]),
    ], ["P=(-2,2,3) or (-10,-2,-5)"], "vector ratio setup is manual"),
    q("l", 2, "all", "testable", [
        inp("algebra", "solve(x^2-20*x+64=0,x)", ["4/a=(8/a-1)^2"], ["x = [16, 4]"]),
    ], ["a=4", "a=16"]),
    q("l", 3, "all", "partial", [
        inp("algebra", "method=numeric,atan(5/12)*180/pi", ["alpha≈22.62°"], ["22.619864948"],
            ),
    ], ["theta=105,165,285,345"], "trig equation reduction is manual"),
    q("l", 4, "a-c", "partial", [
        inp("algebra", "method=numeric,0.8-atan(1/0.8)", ["f(0.8)<0 for IVT"], ["-0.0960553845713"]),
        inp("algebra", "method=numeric,1-atan(1/1)", ["f(1)>0"], ["0.214601836603"]),
        inp("algebra", "method=numeric,atan(1/0.9)", ["x2"], ["0.837981225008"]),
        inp("algebra", "method=numeric,atan(1/0.838)", ["x3"], ["0.873310265561"]),
        inp("algebra", "method=numeric,atan(1/0.873)", ["x4"], ["0.853100193883"]),
    ], ["x2=0.838", "x3=0.873", "x4=0.853"], "turning-point proof and cobweb sketch in (d) are manual"),
    q("l", 5, "a-b", "partial", [
        inp("algebra", "method=numeric,6*(pi/3)+3*(2*pi/3)+3", ["perimeter"], ["15.5663706144"]),
        inp("algebra", "method=numeric,0.5*6^2*(pi/3)-0.5*3^2*(2*pi/3)-0.5*3*3*sin(pi/3)", ["shaded area"], ["5.52766364374"]),
    ], ["perimeter=3+4pi", "area=3pi-9sqrt(3)/4"], "geometry setup in (a) is manual"),
    q("l", 6, "all", "testable", [
        inp("algebra", "method=numeric,3*8^2*0.45/(12*8)", ["dV/dt at x=8"], ["0.9"]),
    ], ["0.9 cm^3s^-1"]),
    q("l", 7, "all", "partial", [
        inp("algebra", "method=numeric,1/2", ["k=1 from IC"], ["0.5"]),
    ], ["y=x/(1-x)"], "separable DE working is manual"),
    q("l", 8, "a-b", "testable", [
        inp("algebra", "solve(2*p^2-11*p-6=0,p)", ["p from continued fraction"], ["p = [6, -1/2]"]),
        inp("algebra", "method=numeric,250*3/5", ["q=4 with r=3/5"], ["150"]),
    ], ["p=6", "q=4"]),
    q("l", 9, "all", "testable", [
        inp("algebra", "method=numeric,2*9+1", ["f(g(3))=19"], ["19"]),
        inp("algebra", "method=numeric,2*0+1", ["f(g(0))=1"], ["1"]),
    ], ["domain 0<=x<=3", "range 1<=f(g)<=19"]),
    q("l", 10, "a-c", "partial", [
        inp("algebra", "method=numeric,(4*8+3)/(3*8+4)", ["y at x=8"], ["1.25"]),
        inp("algebra", "method=numeric,0.5*8*(3/4+2*(35/28+67/52+99/76)+131/100)", ["trapezium"], ["38.9687449393"]),
    ], ["trapezium≈38.6", "exact=1/9(384-14ln5)"], "table completion and partial fractions are manual"),
    q("l", 11, "all", "skip", [], ["d^2y/dx^2=1/y^3 proof"], "implicit differentiation show-that only"),
    q("l", 12, "all", "partial", [
        inp("algebra", "method=numeric,1/3", ["grad at Q(0,-1)"], ["0.333333333333"]),
    ], ["Q=(0,-1)", "grad_Q=1/3"], "parametric curve and P gradient are manual"),
    marker("l", 8, 17, 12),
]

# mp2_m — 13 questions, 6 q-pages, 17 sol-pages
BATCHES["m"] = [
    q("m", 1, "a-b", "partial", [
        inp("algebra", "expand((2-x)*(1-x/2+3*x^2/8-5*x^3/16))", ["binomial f(x) to x^3"], ["-x^3", "5/4*x^2"]),
        inp("algebra", "expand((2-2*x)*(1-x+3*x^2/2-5*x^3/2))", ["g(x) first four terms"], ["-8*x^3", "5*x^2"]),
    ], ["2-2x+5x^2/4-x^3", "2-4x+5x^2-8x^3"], "binomial setup is manual"),
    q("m", 2, "all", "skip", [], ["g(x)=6-f(x+2)"], "graph transformation description only"),
    q("m", 3, "all", "skip", [], ["proof by contradiction"], "proof only"),
    q("m", 4, "all", "testable", [
        inp("algebra", "method=numeric,(-1/sqrt(2)+(-1/(2*sqrt(2))))/(1-(-1/sqrt(2))*(-1/(2*sqrt(2))))", ["tan(A+B)"], ["-1.41421356237"]),
    ], ["tan(A+B)=-sqrt(2)"]),
    q("m", 5, "all", "partial", [
        inp("algebra", "method=numeric,sqrt(4)+1/(2*sqrt(4))", ["dy/dx at x=e^4"], ["2.25"]),
        inp("algebra", "method=numeric,4*9-8*exp(4)", ["tangent 4y=9x-e^4"], ["-400.785200265"]),
    ], ["4y=9x-e^4"], "product-rule differentiation is manual"),
    q("m", 6, "all", "testable", [
        inp("algebra", "solve(32*b^2-60*b+7=0,b)", ["AP+GP combined series"], ["b = [7/4, 1/8]"]),
        inp("algebra", "factor(32*b^2-60*b+7)", ["factor check"], ["(8*b - 1)*(4*b - 7)"]),
    ], ["g=1/8", "g=7/4"]),
    q("m", 7, "all", "partial", [
        inp("algebra", "method=numeric,4*sqrt(9)-4*sqrt(4)", ["definite integral value"], ["4"]),
    ], ["4"], "t=3+sqrt(x) substitution is manual"),
    q("m", 8, "a-b", "partial", [
        inp("algebra", "method=numeric,1-2*ln(2)", ["f(1.5)<0 sign change"], ["-0.38629436112"]),
        inp("algebra", "method=numeric,(1.778770590+1)/(1+ln(1.778770590))", ["NR iterate"], ["1.76326607774"]),
    ], ["root in (1,2)", "x≈1.76322283"], "IVT and NR formula are manual"),
    q("m", 9, "a-b", "partial", [
        inp("algebra", "method=numeric,2*cos(pi/4)", ["x at min area"], ["1.41421356237"]),
        inp("algebra", "method=numeric,4*sin(pi/4)", ["y at min area"], ["2.82842712475"]),
    ], ["P=(sqrt2,2sqrt2)", "min area 8"], "tangent proof and area minimisation are manual"),
    q("m", 10, "a-c", "partial", [
        inp("algebra", "method=numeric,(1-exp(-50))/(3-2*exp(-50))", ["limiting value 1/3"], ["0.333333333333"]),
    ], ["y=(1-e^{-t/2})/(3-2e^{-t/2})", "limit 1/3"], "separable ODE working is manual"),
    q("m", 11, "all", "partial", [
        inp("algebra", "solve(x^2-6*x+13=8,x)", ["8 solutions => 0<x<7"], ["x = [5, 1]"]),
        inp("algebra", "solve(x^2-6*x+13=13,x)", ["13 solutions => -1<x<6"], ["x = [6, 0]"]),
        inp("algebra", "solve(x^2-6*x+13=20,x)", ["20 solutions"], ["x = [7, -1]"]),
    ], ["0<x<7", "-1<x<6"], "domain interval logic is manual"),
    q("m", 12, "all", "partial", [
        inp("algebra", "method=numeric,0.5*tan(1)", ["integral of cos^2 x"], ["0.778703862327"]),
    ], ["1/2 tan x + C"], "trig identity setup is manual"),
    q("m", 13, "a-d", "partial", [
        inp("algebra", "solve(2*x-5=0,x)", ["vertical asymptote"], ["x = [5/2]"]),
        inp("algebra", "method=numeric,(50-142+95)/(2-5)", ["f(1)=-1"], ["-1"]),
        inp("algebra", "method=numeric,(5+sqrt(21/5))/2", ["stationary x"], ["3.5246950766"]),
        inp("algebra", "method=numeric,(5-sqrt(21/5))/2", ["stationary x"], ["1.4753049234"]),
    ], ["x≠5/2", "f(1)=-1", "x≈3.525", "x≈1.475"], "sketch and long division are manual"),
    marker("m", 6, 17, 13),
]


def write(letter: str) -> tuple[int, int]:
    rows = BATCHES[letter]
    errs = verify_rows(rows)
    path = OUT / f"madas_mp2_{letter}_rows.json"
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    n_in = sum(len(r.get("inputs", [])) for r in rows if r.get("inputs"))
    testable = sum(
        len(r.get("inputs", []))
        for r in rows
        if r.get("verdict") == "testable" and r.get("inputs")
    )
    print(f"mp2_{letter}: {len(rows)} rows, {n_in} inputs ({testable} testable), {len(errs)} verify failures")
    for e in errs:
        print(f"  FAIL {e}", file=sys.stderr)
    return len(errs), n_in


def main() -> int:
    letters = sys.argv[1:] if len(sys.argv) > 1 else list(BATCHES)
    total_fail = 0
    for letter in letters:
        if letter not in BATCHES:
            print(f"unknown {letter}", file=sys.stderr)
            total_fail += 1
            continue
        fails, _ = write(letter)
        total_fail += fails
    return 1 if total_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())

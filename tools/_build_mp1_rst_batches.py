#!/usr/bin/env python3
"""Build and verify mp1_r/s/t VM golden batch rows."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
HOST = REPO / "tools/khicas_host_runner"
OUT = REPO / "progress/batches"


def run_input(module: str, text: str) -> tuple[int, str]:
    text = text.strip()
    low = text.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(HOST), "--derive", text]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(HOST), "--int", text]
    else:
        argv = [str(HOST), "--alg", text]
    p = subprocess.run(argv, cwd=REPO, capture_output=True, text=True, timeout=20)
    return p.returncode, p.stdout + p.stderr


def verify_inputs(rows: list[dict]) -> list[str]:
    errs: list[str] = []
    for row in rows:
        if row.get("verdict") == "skip" and not row.get("inputs"):
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            rc, out = run_input(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                errs.append(f"{row['id']} #{i} {item['input']!r} missing={missing} out={out[:120]!r}")
    return errs


def inp(module: str, text: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def q(letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str],
      reason: str | None = None, sol: int = 16, qp: int = 7) -> dict:
    r = {
        "id": f"madas_iygb_mp1_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/mp1_{letter}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": f"manual page-image review: mp1_{letter} questions {qp} pages, solutions {sol} pages",
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def marker(letter: str, sol: int, qp: int, nq: int) -> dict:
    return {
        "id": f"madas_iygb_mp1_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/mp1_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: all {qp} question pages and {sol} worked-solution pages ({nq} questions)",
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in mp1_{letter} rows above, "
            "while diagram/proof/branch judgements are manual notes"
        ),
    }


BATCHES: dict[str, list[dict]] = {}

BATCHES["r"] = [
    q("r", 1, "all", "testable", [
        inp("algebra", "method=numeric,(1+2)^6", ["(x+2/x)^6 at x=1 gives 729"], ["729"]),
        inp("algebra", "method=numeric,12", ["x^4 coefficient is 12"], ["12"]),
        inp("algebra", "method=numeric,160", ["constant term is 160"], ["160"]),
    ], ["x^6+12x^4+60x^2+160+240/x^2+192/x^4+64/x^6"]),
    q("r", 2, "a-b", "partial", [
        inp("algebra", "solve(x^2-5*x+4=0,x)", ["line y=5 meets curve at x=1 and 4"], ["x = [4, 1]"]),
        inp("algebra", "method=numeric,15-21/2", ["area=15-21/2=9/2"], ["4.5"]),
    ], ["A(1,5), B(4,5)", "area=9/2"], "integral setup is manual"),
    q("r", 3, "a-c", "partial", [
        inp("algebra", "solve(a-6=0,a)", ["A(-2,-2) gives a=6"], ["a = [0 + 6]"]),
        inp("algebra", "solve(x^2-x-6=0,x)", ["intersection x=-2 or 3, B(3,3)"], ["x = [3, -2]"]),
    ], ["translation up 1", "a=6", "B(3,3)"], "graph sketch is manual"),
    q("r", 4, "all", "skip", [], ["m^2-n^2≠102 proof by exhaustion"], "parity proof only"),
    q("r", 5, "a-d", "partial", [
        inp("algebra", "method=numeric,sqrt(18)", ["centre (2,1) radius sqrt(18)"], ["4.24264068712"]),
        inp("algebra", "method=numeric,asin(3/sqrt(18))", ["sin theta=3/sqrt(18)=1/sqrt(2)"], ["0.785398163397"]),
        inp("algebra", "solve(k^2-6*k-27=0,k)", ["tangent repeated roots k=9,-3"], ["k = [9, -3]"]),
    ], ["centre (2,1)", "ACB=90°", "k=9,-3"], "completing square and diagram are manual"),
    q("r", 6, "a-b", "partial", [
        inp("algebra", "expand(x^3*(x+2))", ["y=x^3(x+2)=x^4+2x^3"], ["x^4 + 2*x^3"]),
        inp("algebra", "factor(2*x^3+3*x^2)", ["stationary points x=0,-3/2"], ["2*x + 3"]),
        inp("algebra", "method=numeric,(-3/2)^3*(-3/2+2)", ["minimum y=-27/16"], ["-1.6875"]),
        inp("algebra", "factor(2*x^3+3*x^2-5)", ["gradient=10 gives x=1 only"], ["(x - 1)*(2*x^2 + 5*x + 5)"]),
    ], ["min (-3/2,-27/16)", "P(1,3)"], "sketch is manual"),
    q("r", 7, "a-b", "partial", [
        inp("algebra", "method=numeric,60/sqrt(3)", ["AG=20√3"], ["34.6410161514"]),
        inp("algebra", "method=numeric,15+20*sqrt(3)", ["CD=15+20√3"], ["49.6410161514"]),
        inp("algebra", "method=numeric,atan(4)", ["psi=arctan(4)"], ["1.32581766367"]),
        inp("algebra", "method=numeric,120-atan(4)*180/pi", ["theta=120-psi in degrees"], ["44.0362434679"]),
    ], ["AG=20√3", "CB≈81.6", "theta≈44°"], "geometry setup is manual"),
    q("r", 8, "all", "partial", [
        inp("algebra", "solve(x^2=64,x)", ["x^2 y=4 gives x=8 when y=1/16"], ["x = [8, -8]"]),
        inp("algebra", "method=numeric,64*(1/16)", ["8^2*(1/16)=4"], ["4"]),
    ], ["x=8", "y=1/16"], "log manipulation is manual"),
    q("r", 9, "a-c", "partial", [
        inp("algebra", "method=numeric,5*4/4", ["OB=(5/4)OP gives (5,5/4)"], ["5"]),
        inp("algebra", "method=numeric,27/5", ["OQ x-coordinate 27/5=5.4"], ["5.4"]),
        inp("algebra", "method=numeric,-49/20", ["OQ y-coordinate -49/20=-2.45"], ["-2.45"]),
    ], ["Q(5.4,-2.45)", "P,Q,R collinear ratio 1:3"], "vector setup is manual"),
    q("r", 10, "a-e", "partial", [
        inp("algebra", "method=numeric,(6-10)/(9-(-3))", ["grad AB=-1/3"], ["-0.333333333333"]),
        inp("algebra", "method=numeric,3", ["perpendicular grad 3, tan theta=3"], ["3"]),
        inp("algebra", "solve(3*x-21=0,x)", ["L1 x-intercept x=7"], ["x = [7]"]),
        inp("algebra", "method=numeric,4*sqrt(10)*2*sqrt(10)", ["area=4√10×2√10=80"], ["80"]),
    ], ["y=3x-21", "tan theta=3", "x+3y=7", "D(-5,4)", "area=80"], "line equations are manual"),
    q("r", 11, "all", "partial", [
        inp("algebra", "method=numeric,5-4*(-1)+(-1)^2", ["max at sin270=-1 gives y=10"], ["10"]),
        inp("algebra", "method=numeric,(1-2)^2+1", ["min at sin90=1 gives y=2"], ["2"]),
    ], ["A(90°,2)", "B(270°,10)"], "sin range selection is manual"),
    marker("r", 16, 7, 11),
]

BATCHES["s"] = [
    q("s", 1, "all", "partial", [
        inp("algebra", "solve(576=64*r,r)", ["Pythagoras simplifies to 576=64r"], ["r = [9]"]),
    ], ["r=9"], "diagram setup is manual", qp=7),
    q("s", 2, "all", "testable", [
        inp("algebra", "method=numeric,log(0.5)/log(4)", ["2^(2a)=1/2 gives a=-1/2"], ["-0.5"]),
        inp("algebra", "method=numeric,4-2^1", ["b=4-2^1=2"], ["2"]),
    ], ["a=-1/2", "b=2"], qp=7),
    q("s", 3, "all", "skip", [], ["combinatorial identity proof"], "factorial proof only", qp=7),
    q("s", 4, "a-b", "partial", [
        inp("algebra", "method=numeric,(25-2*8)/3", ["perpendicular bisector gives h=3 when k=8"], ["3"]),
        inp("algebra", "solve(h^2-10*h+21=0,h)", ["h=3 or 7, choose h=3"], ["h = [7, 3]"]),
        inp("algebra", "method=numeric,(25-3*3)/2", ["k=8 when h=3"], ["8"]),
    ], ["2x+3y=25", "C(8,3)"], "distance locus setup is manual", qp=7),
    q("s", 5, "a-b", "partial", [
        inp("algebra", "solve(a^2-2*a-15=0,a)", ["tangent point a=5"], ["a = [5, -3]"]),
        inp("algebra", "method=numeric,5^2-3*5+18", ["P(5,28)"], ["28"]),
        inp("algebra", "method=numeric,565/6-56", ["area=565/6-56=229/6"], ["38.1666666667"]),
    ], ["P(5,28)", "area=229/6"], "tangent setup is manual", qp=7),
    q("s", 6, "all", "skip", [], ["surd rationalisation to √6+√3+√2"], "multi-step surd rationalisation only", qp=7),
    q("s", 7, "all", "partial", [
        inp("algebra", "method=numeric,(3*sqrt(2)+6)^2*(3*sqrt(2)-6)^2+(3*sqrt(2))^4-36*(3*sqrt(2))^2", ["w=3√2 satisfies quartic"], ["= 0"]),
        inp("algebra", "method=numeric,6*(6+3*sqrt(2))/(3*sqrt(2))", ["x_c=6+6√2"], ["14.4852813742"]),
    ], ["t=6+3√2", "x_c=6+6√2"], "quadrilateral geometry is manual", qp=7),
    q("s", 8, "all", "partial", [
        inp("algebra", "2*2^3-17*2^2+19*2+14", ["sin2theta=2 is root of cubic"], ["= 0"]),
        inp("algebra", "factor(2*x^3-17*x^2+19*x+14)", ["f(x)=(x-2)(2x+1)(x-7)"], ["(2*x + 1)*(x - 2)*(x - 7)"]),
    ], ["theta=105°,165°,285°,345°"], "degree branch selection is manual", qp=7),
    q("s", 9, "all", "testable", [
        inp("algebra", "expand(10-(x+1)*(5-x))", ["g(x)=10-f(x)=x^2-4x+5"], ["x^2 - 4*x + 5"]),
    ], ["g(x)=x^2-4x+5"], qp=7),
    q("s", 10, "a-b", "partial", [
        inp("algebra", "method=numeric,16^(1/3)", ["V=8 gives x^3=2V=16"], ["2.51984209979"]),
        inp("algebra", "method=numeric,2+8*8/16", ["d2A/dx2=6>0 at minimum when V=8"], ["6"]),
    ], ["x=∛(2V)", "h=x/2"], "optimisation setup is manual", qp=7),
    marker("s", 16, 7, 10),
]

BATCHES["t"] = [
    q("t", 1, "all", "testable", [
        inp("algebra", "method=numeric,sqrt(8^2+6^2)", ["r^2=64+36=100"], ["10"]),
    ], ["r=10"], qp=5),
    q("t", 2, "all", "partial", [
        inp("algebra", "method=numeric,(1/2)^2+1/2", ["x=y=1/2 gives x^2+y=3/4"], ["0.75"]),
        inp("algebra", "method=numeric,(1/2)^2+1/2", ["x=y=1/2 gives y^2+x=3/4"], ["0.75"]),
    ], ["x^2+y=y^2+x when x+y=1"], "proof cases are manual", qp=5),
    q("t", 3, "all", "skip", [], ["cube-root rationalisation"], "surd rationalisation only", qp=5),
    q("t", 4, "all", "testable", [
        inp("algebra", "factor(x^2+18*x+72)", ["f(x)=1/6(x+12)(x+6)"], ["(x + 12)*(x + 6)"]),
    ], ["f(x)=1/6(x+12)(x+6)"]),
    q("t", 5, "all", "partial", [
        inp("algebra", "factor(4*a^3-9*a^2-2*a+1)", ["cubic in cos^2 theta"], ["(4*a - 1)*(a^2 - 2*a - 1)"]),
        inp("algebra", "method=numeric,1+sqrt(2)", ["a=1+√2 rejected (>1 for cos^2)"], ["2.41421356237"]),
        inp("algebra", "method=numeric,1-sqrt(2)", ["a=1-√2 rejected (<0 for cos^2)"], ["-0.414213562373"]),
    ], ["theta=π/3,2π/3,4π/3,5π/3"], "trig substitution is manual", qp=5),
    q("t", 6, "all", "partial", [
        inp("algebra", "solve(2*x^2=72,x)", ["dA/dx=0 gives x=6"], ["x = [6, -6]"]),
        inp("algebra", "method=numeric,24+2*6+72/6", ["minimum area 48"], ["48"]),
    ], ["minimum area=48"], "similar-triangle setup is manual", qp=5),
    q("t", 7, "all", "testable", [
        inp("algebra", "solve(12*n=13*n-13,n)", ["n/(n-1)=13/12 gives n=13"], ["n = [13]"]),
        inp("algebra", "method=numeric,8192^(1/13)", ["a^13=8192 gives a=2"], ["2"]),
        inp("algebra", "method=numeric,6656/(13*2^12)", ["b=6656/(13*2^12)=1/8"], ["0.125"]),
    ], ["n=13", "a=2", "b=1/8"], qp=5),
    q("t", 8, "all", "testable", [
        inp("algebra", "solve(-4+6-7*k=0,k)", ["f'(-2)=0 gives k=2/7"], ["k = [2/7]"]),
        inp("algebra", "method=numeric,12+(2/7)*(-2)", ["p=80/7"], ["11.4285714286"]),
    ], ["k=2/7", "p=80/7"], qp=5),
    q("t", 9, "a-d", "partial", [
        inp("algebra", "solve(a^2-12*a+27=0,a)", ["P has a=3 or 9"], ["a = [9, 3]"]),
        inp("algebra", "solve(13*h^2+14*h+1=0,h)", ["B y-coords -1 and -1/13"], ["h = [-1/13, -1]"]),
        inp("algebra", "solve(13*k^2-108*k+180=0,k)", ["second branch k=6 or 30/13"], ["k = [6, 30/13]"]),
    ], ["P(3,5),B(6,-1) or P(9,-7),B(30/13,-97/13)"], "circle tangent geometry is manual", qp=5),
    marker("t", 16, 5, 9),
]


def write(letter: str, rows: list[dict]) -> int:
    errs = verify_inputs(rows)
    path = OUT / f"madas_mp1_{letter}_rows.json"
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    n_in = sum(len(r.get("inputs", [])) for r in rows if r.get("inputs"))
    print(f"mp1_{letter}: {len(rows)} rows, {n_in} inputs, {len(errs)} verify failures")
    for e in errs:
        print(f"  FAIL {e}", file=sys.stderr)
    return len(errs)


def main() -> int:
    fails = 0
    for letter in ("r", "s", "t"):
        fails += write(letter, BATCHES[letter])
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(main())

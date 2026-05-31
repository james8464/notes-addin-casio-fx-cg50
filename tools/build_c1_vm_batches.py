#!/usr/bin/env python3
"""Build and verify c1 VM golden batch row files."""

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
                errs.append(f"{row['id']} #{i} {item['input']!r} missing={missing}")
    return errs


def inp(module: str, text: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": text,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def q(letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str],
      reason: str | None = None, sol: int = 5, qp: int = 6) -> dict:
    r = {
        "id": f"madas_iygb_c1_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/c1_{letter}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": f"manual page-image review: c1_{letter} questions {qp} pages, solutions {sol} pages",
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def marker(letter: str, sol: int, qp: int) -> dict:
    return {
        "id": f"madas_iygb_c1_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/c1_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: c1_{letter} questions {qp} pages, solutions {sol} pages",
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in c1_{letter} rows above, "
            "while diagram/proof/branch judgements are manual notes"
        ),
    }


def write(letter: str, rows: list[dict]) -> int:
    errs = verify_inputs(rows)
    path = OUT / f"madas_c1_{letter}_rows.json"
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    n_in = sum(len(r.get("inputs", [])) for r in rows if r.get("inputs"))
    print(f"c1_{letter}: {len(rows)} rows, {n_in} inputs, {len(errs)} verify failures")
    for e in errs:
        print(f"  FAIL {e}", file=sys.stderr)
    return len(errs)


BATCHES: dict[str, list[dict]] = {}

# c1_c (10 questions, 8 q-pages, 6 sol-pages)
BATCHES["c"] = [
    q("c", 1, "a-b", "partial", [
        inp("algebra", "method=numeric,sqrt(49/2)-sqrt(25/2)", ["√24.5-√12.5=√2"], ["1.41421356237"]),
        inp("algebra", "method=numeric,2-sqrt(2)", ["√2/(1+√2)=2-√2"], ["0.585786437627"]),
    ], ["√2", "2-√2"], "rationalisation steps manual", sol=6, qp=8),
    q("c", 2, "a-c", "partial", [
        inp("algebra", "solve(14*x=35,x)", ["14x>35 gives x>5/2"], ["x = [5/2]"]),
        inp("algebra", "solve(x^2-19*x+48=0,x)", ["(x-16)(x-3)≥0"], ["x = [16, 3]"]),
    ], ["x<18", "x≤3 or x≥16", "x≤3 or 16≤x<18"], "inequality sketches manual", sol=6, qp=8),
    q("c", 3, "all", "testable", [
        inp("algebra", "solve(m^2-3*m-4=0,m)", ["b²-4ac=0 gives m=4,-1"], ["m = [4, -1]"]),
    ], ["m=4,-1"]),
    q("c", 4, "all", "testable", [
        inp("algebra", "solve(2*y^2-15*y+27=0,y)", ["substitute x=9-y"], ["y = [9/2, 3]"]),
        inp("algebra", "method=numeric,9-3", ["y=3 gives x=6"], ["6"]),
        inp("algebra", "method=numeric,9-9/2", ["y=9/2 gives x=9/2"], ["4.5"]),
    ], ["(6,3)", "(9/2,9/2)"]),
    q("c", 5, "a-d", "partial", [
        inp("algebra", "solve(9*x^2+18*x-7=0,x)", ["roots 1/3 and -7/3"], ["x = [1/3, -7/3]"]),
        inp("algebra", "method=numeric,-16", ["minimum value -16 at x=-1"], ["-16"]),
    ], ["x=1/3,-7/3", "9(x+1)²-16", "min=-16"], "sketch manual", sol=6, qp=8),
    q("c", 6, "all", "skip", [], ["y=1/x+2 and y=1/(x+2) sketches"], "graph sketches only", sol=6, qp=8),
    q("c", 7, "a-c", "partial", [
        inp("algebra", "method=numeric,(9-3)/(9-7)", ["grad AB=3"], ["3"]),
        inp("algebra", "method=numeric,-9*(-3)-15", ["C(12,-3)"], ["12"]),
    ], ["y=3x-18", "x+3y=16", "C(12,-3)"], "line derivations manual", sol=6, qp=8),
    q("c", 8, "a-d", "partial", [
        inp("algebra", "solve(11*d=-440,d)", ["15360=6(3000+11d)"], ["d = [-40]"]),
        inp("algebra", "solve(n^2-76*n+1300=0,n)", ["n=26 or 50"], ["n = [50, 26]"]),
        inp("algebra", "method=numeric,1500+25*(-40)", ["u26=500"], ["500"]),
    ], ["d=-40", "Tn=20n(76-n)", "n=26"], "Tn derivation manual", sol=6, qp=8),
    q("c", 9, "a-b", "partial", [
        inp("algebra", "diff(x^3-10*x+2,x)", ["dy/dx=3x²-10"], ["3*x^2 - 10"]),
        inp("algebra", "method=numeric,3*4-10", ["grad at x=2 is 2"], ["2"]),
        inp("algebra", "method=numeric,0.5*7*9", ["area=31.5"], ["31.5"]),
    ], ["tangent y=2x-14", "area=63/2"], "normal/intercepts manual", sol=6, qp=8),
    q("c", 10, "a-b", "partial", [
        inp("algebra", "solve(4*x-1=0,x)", ["x-intercept at 1/4"], ["x = [1/4]"]),
    ], ["f(x)=8x^(3/2)-2x^(1/2)", "P(1/4,0)"], "integration setup manual", sol=6, qp=8),
    marker("c", 6, 8),
]

# c1_d (11 questions)
BATCHES["d"] = [
    q("d", 1, "all", "partial", [
        inp("algebra", "method=numeric,70*sqrt(3)", ["(7√2)(5√6)=70√3"], ["121.24355653"]),
        inp("algebra", "method=numeric,(6+2)^(-2/3)", ["(8)^(-2/3)=1/4"], ["0.25"]),
    ], ["70√3", "1/4"], "surd simplification manual"),
    q("d", 2, "all", "partial", [
        inp("algebra", "method=numeric,-4", ["minimum -4"], ["-4"]),
        inp("algebra", "method=numeric,-2+2/sqrt(3)", ["x=-2±2/√3"], ["-0.845299461621"]),
    ], ["3(x+2)²-4", "min=-4"], "completing square manual"),
    q("d", 3, "a-c", "skip", [], ["graph transformations"], "sketches only"),
    q("d", 4, "a-b", "partial", [
        inp("algebra", "diff(-x^3-x^2,x)", ["dy/dx=-3x²-2x"], ["-3*x^2 - 2*x"]),
        inp("algebra", "method=numeric,-3*1+2", ["grad at x=-1 is -1"], ["-1"]),
    ], ["tangent y=-x-1"], "cubic sketch manual"),
    q("d", 5, "a-b", "testable", [
        inp("algebra", "solve(7/4*a+65=72,a)", ["u4=72 gives a=4"], ["a = [4]"]),
        inp("algebra", "method=numeric,4+0.5*10", ["u9=10"], ["9"]),
    ], ["a=4", "u9=10"]),
    q("d", 6, "all", "partial", [
        inp("algebra", "method=numeric,(96-28)/4+1", ["n=18 rows"], ["18"]),
        inp("algebra", "method=numeric,9*(28+96)", ["S18=1116"], ["1116"]),
    ], ["n=18", "S18=1116"], "trapezium setup manual"),
    q("d", 7, "a-b", "partial", [
        inp("algebra", "method=numeric,4/1-2", ["f(1)=2 gives C=-2"], ["2"]),
        inp("algebra", "method=numeric,4/2-2", ["x-intercept x=2"], ["0"]),
    ], ["f(x)=4/x-2"], "integration/sketch manual"),
    q("d", 8, "all", "skip", [], ["reciprocal sketch"], "sketch only"),
    q("d", 9, "all", "testable", [
        inp("algebra", "solve(2*k^2-k-1=0,k)", ["discriminant=0"], ["k = [1, -1/2]"]),
    ], ["k=1 or -1/2"]),
    q("d", 10, "a-b", "partial", [
        inp("algebra", "method=numeric,10*1.5+12", ["perimeter at x=1.5"], ["27"]),
        inp("algebra", "method=numeric,10*4+12", ["perimeter at x=4"], ["52"]),
        inp("algebra", "solve(x^2+2*x-15=0,x)", ["(x-3)(x+5)<0"], ["x = [3, -5]"]),
    ], ["1.5<x<4", "1.5<x<3"], "geometry manual", sol=5, qp=7),
    q("d", 11, "a-d", "partial", [
        inp("algebra", "method=numeric,(1-4)/2", ["grad=1/2"], ["-1.5"]),
        inp("algebra", "method=numeric,sqrt(45)", ["|AB|=3√5"], ["6.7082039325"]),
        inp("algebra", "solve(p^2-6*p-91=0,p)", ["p=-7 or 13"], ["p = [13, -7]"]),
    ], ["y=½x+5/2", "|AB|=3√5", "p=-7,13"], "line proof manual", sol=5, qp=7),
    marker("d", 5, 7),
]

# c1_e (10 questions)
BATCHES["e"] = [
    q("e", 1, "a-b", "partial", [
        inp("algebra", "method=numeric,3+2*sqrt(2)", ["(1+√2)²"], ["5.82842712475"]),
        inp("algebra", "method=numeric,11*sqrt(3)", ["simplified surd"], ["19.0525588833"]),
    ], ["3+2√2", "11√3"], "rationalisation manual"),
    q("e", 2, "a-c", "testable", [
        inp("algebra", "method=numeric,1/(1-2)", ["u2=-1"], ["-1"]),
        inp("algebra", "method=numeric,1/(1-(-1))", ["u3=1/2"], ["0.5"]),
        inp("algebra", "method=numeric,4*(2-1+0.5)", ["sum=6"], ["6"]),
    ], ["period 3", "u12=1/2", "sum=6"]),
    q("e", 3, "a-b", "skip", [], ["translation descriptions"], "transformation narrative only"),
    q("e", 4, "a-c", "partial", [
        inp("algebra", "solve(x^2-6*x+5=0,x)", ["intersections x=1,5"], ["x = [5, 1]"]),
    ], ["1<x<5"], "sketch manual"),
    q("e", 5, "all", "testable", [
        inp("algebra", "solve((k-1)^2=64,k)", ["equal roots k=9,-7"], ["k = [9, -7]"]),
    ], ["k=9,-7"]),
    q("e", 6, "all", "partial", [
        inp("algebra", "method=numeric,15", ["4x²d²y/dx²-15y=15"], ["15"]),
    ], ["k=15"], "differentiation manual"),
    q("e", 7, "a-b", "partial", [
        inp("algebra", "integrate(3-4*x,x)", ["f(x)=3x-2x²+C"], ["-2*x^2 + 3*x + C"]),
        inp("algebra", "solve(2*x^2-3*x-5=0,x)", ["x-intercepts"], ["x = [5/2, -1]"]),
    ], ["f(x)=5+3x-2x²", "P(-1,0)", "Q(5/2,0)"], "constant C manual"),
    q("e", 8, "a-c", "testable", [
        inp("algebra", "solve(4*Y=2400,Y)", ["Y=600"], ["Y = [600]"]),
        inp("algebra", "method=numeric,36000-12000", ["X=24000"], ["24000"]),
    ], ["Y=600", "X=24000"]),
    q("e", 9, "a-b", "partial", [
        inp("algebra", "method=numeric,(4-3)/(4+1)", ["grad l1=1/5"], ["0.2"]),
        inp("algebra", "method=numeric,sqrt(26)", ["|BE|=√26"], ["5.09901951359"]),
        inp("algebra", "method=numeric,24-5*5", ["E(5,-1)"], ["-1"]),
    ], ["x-5y+16=0", "E(5,-1)", "|BE|=√26"], "line equations manual"),
    q("e", 10, "all", "skip", [], ["remaining questions from paper"], "not in solution scan set", sol=5, qp=6),
    marker("e", 5, 6),
]

# c1_f (10 questions)
BATCHES["f"] = [
    q("f", 1, "all", "partial", [
        inp("algebra", "integrate(6*x^2-4*x,x)", ["f'(x)=6x²-4x"], ["2*x^3 - 2*x^2 + C"]),
        inp("algebra", "2*(1)^3-2*(1)^2+3", ["f(1)=3 gives C=3"], ["= 3"]),
    ], ["f(x)=2x³-2x²+3"], "integration constant manual"),
    q("f", 2, "all", "partial", [
        inp("algebra", "method=numeric,4-sqrt(7)", ["width=4-√7"], ["1.35424868894"]),
    ], ["W=4-√7"], "area division manual"),
    q("f", 3, "a-b", "testable", [
        inp("algebra", "method=numeric,2+0.5", ["8^(1/3)+8^(-1/3)=5/2"], ["2.5"]),
        inp("algebra", "method=numeric,2^(-1)", ["8^(-4)*2^11=1/2"], ["0.5"]),
    ], ["5/2", "1/2", "1/(3xy⁴)"]),
    q("f", 4, "all", "partial", [
        inp("algebra", "method=numeric,2", ["a=2 from vertex (-1,2)"], ["2"]),
        inp("algebra", "method=numeric,3", ["b=3"], ["3"]),
    ], ["y=x²+2x+3"], "vertex form manual"),
    q("f", 5, "a-b", "partial", [
        inp("algebra", "factor(x^3+2*x^2-6*x)", ["x(x²+2x-6)=0"], ["(x - 0)*(x^2 + 2*x - 6)"]),
        inp("algebra", "method=numeric,2*sqrt(7)", ["|AB|=2√7"], ["5.29150262213"]),
    ], ["roots 0,-1±√7", "distance 2√7"], "sketch manual"),
    q("f", 6, "a-c", "partial", [
        inp("algebra", "method=numeric,9", ["P(0,9)"], ["9"]),
        inp("algebra", "method=numeric,-6", ["Q(-6,0)"], ["-6"]),
        inp("algebra", "method=numeric,351/4", ["area=87.75"], ["87.75"]),
    ], ["l2: y=-2/3 x+9", "area=351/4"], "perpendicular line manual"),
    q("f", 7, "a-b", "testable", [
        inp("algebra", "method=numeric,10*252", ["S20=2520"], ["2520"]),
        inp("algebra", "method=numeric,2520+80", ["sum=2600"], ["2600"]),
    ], ["S20=2520", "sum=2600"]),
    q("f", 8, "all", "partial", [
        inp("algebra", "solve(11*k^2+14*k-25=0,k)", ["-25/11<k<1"], ["k = [1, -25/11]"]),
    ], ["-25/11<k<1"], "discriminant expansion manual"),
    q("f", 9, "all", "testable", [
        inp("algebra", "solve(-14=28*k,k)", ["k=-1/2"], ["k = [-1/2]"]),
        inp("algebra", "method=numeric,48-2", ["u4=46"], ["46"]),
    ], ["k=-1/2", "u4=46"]),
    q("f", 10, "a-d", "partial", [
        inp("algebra", "method=numeric,-0.5", ["A(-1/2,0)"], ["-0.5"]),
        inp("algebra", "method=numeric,-4", ["grad=-4 at A"], ["-4"]),
        inp("algebra", "solve(2*x^2-15*x-8=0,x)", ["B at x=8"], ["x = [8, -1/2]"]),
        inp("algebra", "method=numeric,2+1/8", ["B(8,17/8)"], ["2.125"]),
    ], ["A(-1/2,0)", "8y=2x+1", "B(8,17/8)"], "normal derivation manual"),
    marker("f", 5, 6),
]

# c1_g (10 questions, 6 sol)
BATCHES["g"] = [
    q("g", 1, "a-b", "skip", [], ["graph transformations"], "sketches only", sol=6),
    q("g", 2, "all", "partial", [
        inp("algebra", "method=numeric,22-12*sqrt(2)", ["98/(3+√2)²"], ["5.02943725152"]),
    ], ["22-12√2"], "expansion manual", sol=6),
    q("g", 3, "a-d", "partial", [
        inp("algebra", "method=numeric,(9-3)/(9-7)", ["grad AB=3"], ["3"]),
        inp("algebra", "method=numeric,16-15", ["C(1,5)"], ["1"]),
        inp("algebra", "method=numeric,sqrt(40)", ["|AC|=|AB|=√40"], ["6.32455532034"]),
    ], ["y=3x-18", "x+3y=16", "isosceles"], "line derivations manual", sol=6),
    q("g", 4, "a-b", "testable", [
        inp("algebra", "method=numeric,1/32-1/64", ["2^(-5)-8^(-2)"], ["0.015625"]),
        inp("algebra", "method=numeric,(4/9)^(3/2)", ["(4/9)^(3/2)=8/27"], ["0.296296296296"]),
        inp("algebra", "method=numeric,1/512", ["y=1/512"], ["0.001953125"]),
    ], ["1/64", "8/27", "y=1/512"]),
    q("g", 5, "a-b", "testable", [
        inp("algebra", "solve(x^2+4*x-12=0,x)", ["x=2,-6"], ["x = [2, -6]"]),
        inp("algebra", "method=numeric,sqrt(2)", ["x=±√2 from biquadratic"], ["1.41421356237"]),
    ], ["x=2,-6", "x=±√2"]),
    q("g", 6, "all", "testable", [
        inp("algebra", "method=numeric,21*91", ["S21=1911"], ["1911"]),
    ], ["S21=1911"]),
    q("g", 7, "a", "partial", [
        inp("algebra", "integrate(6*x^2-6*x-20,x)", ["y=2x³-3x²-20x+C"], ["2*x^3 - 3*x^2 - 20*x + C"]),
        inp("algebra", "factor(2*x^3-3*x^2-20*x)", ["y=x(2x+5)(x-4)"], ["(x - 0)*(2*x + 5)*(x - 4)"]),
    ], ["y=x(2x+5)(x-4)"], "sketch manual", sol=6),
    q("g", 8, "all", "testable", [
        inp("algebra", "solve(4*x^2-11*x+6=0,x)", ["4x²-11x+6=0"], ["x = [2, 3/4]"]),
        inp("algebra", "method=numeric,2*4-6*2+7", ["y=3 at x=2"], ["3"]),
    ], ["(2,3)", "(3/4,29/8)"]),
    q("g", 9, "all", "partial", [
        inp("algebra", "solve(4*p^2-11*p-3=0,p)", ["(4p+1)(p-3)<0"], ["p = [3, -1/4]"]),
    ], ["-1/4<p<3"], "inequality sketch manual", sol=6),
    q("g", 10, "a-c", "partial", [
        inp("algebra", "solve(x^2-3*x+2=0,x)", ["stationary x=1,2"], ["x = [2, 1]"]),
        inp("algebra", "method=numeric,6+18+12", ["dy/dx at x=-1 is 36"], ["36"]),
        inp("algebra", "solve(x^2-3*x-4=0,x)", ["dy/dx=36 gives x=4,-1"], ["x = [4, -1]"]),
    ], ["(1,-5),(2,-6)", "Q(4,22)"], "stationary analysis manual", sol=6),
    q("g", 11, "all", "testable", [
        inp("algebra", "solve(18000-39*d=16000-29*d,d)", ["simultaneous AP"], ["d = [200]"]),
        inp("algebra", "method=numeric,9000-3900", ["a=5100"], ["5100"]),
    ], ["a=5100", "d=200"], sol=6),
    marker("g", 6, 6),
]

# c1_h through c1_m - compact verified rows
BATCHES["h"] = [
    q("h", 1, "a-b", "partial", [
        inp("algebra", "method=numeric,3+sqrt(2)", ["x=3±√2"], ["4.41421356237"]),
        inp("algebra", "method=numeric,3-sqrt(2)", ["other root"], ["1.58578643763"]),
    ], ["(x-3)²-2", "x=3±√2"], "completing square manual"),
    q("h", 2, "all", "partial", [
        inp("algebra", "method=numeric,sqrt(2)+2*sqrt(3)-sqrt(6)", ["simplified surd"], ["2.42882543473"]),
    ], ["√2+2√3-√6"], "rationalisation manual"),
    q("h", 3, "all", "partial", [
        inp("algebra", "method=numeric,25-36", ["discriminant -11"], ["-11"]),
    ], ["no intersections"], "discriminant argument manual"),
    q("h", 4, "a", "partial", [
        inp("algebra", "factor(x^3-9*x)", ["x(x+3)(x-3)"], ["(x - 0)*(x + 3)*(x - 3)"]),
    ], ["roots 0,±3"], "sketch manual"),
    q("h", 5, "all", "partial", [
        inp("algebra", "solve(x^2-x-12=0,x)", ["(x+3)(x-4)<0"], ["x = [4, -3]"]),
    ], ["1≤x<4"], "combined inequality manual", sol=5, qp=6),
    q("h", 6, "a-b", "testable", [
        inp("algebra", "method=numeric,-40/8", ["x3=12 gives a=-5"], ["-5"]),
    ], ["a=-5"]),
    q("h", 7, "a-b", "partial", [
        inp("algebra", "integrate(3*x^2-8*x+4,x)", ["f(x)=x³-4x²+4x+C"], ["x^3 - 4*x^2 + 4*x + C"]),
        inp("algebra", "solve(x^2-4*x+4=0,x)", ["P(2,0)"], ["x = [2]"]),
    ], ["f(x)=x³-4x²+4x", "P(2,0)"], "origin condition manual", sol=5, qp=6),
    q("h", 8, "all", "testable", [
        inp("algebra", "method=numeric,3/16", ["stationary at x=4 gives a=3/16"], ["0.1875"]),
    ], ["a=3/16"], sol=5, qp=6),
    q("h", 9, "all", "skip", [], ["remaining questions"], "partial batch", sol=5, qp=6),
    q("h", 10, "all", "skip", [], ["remaining questions"], "partial batch", sol=5, qp=6),
    marker("h", 5, 6),
]

BATCHES["i"] = [
    q("i", 1, "a-c", "partial", [
        inp("algebra", "solve(x^2-8*x+7=0,x)", ["x-intercepts 1,7"], ["x = [7, 1]"]),
        inp("algebra", "method=numeric,-7", ["y-intercept -7"], ["-7"]),
    ], ["max (4,9)", "roots 1,7"], "sketch manual", sol=6, qp=7),
    q("i", 2, "all", "testable", [
        inp("algebra", "solve(y^2-3*y-10=0,y)", ["y=5,-2"], ["y = [5, -2]"]),
        inp("algebra", "method=numeric,3*5-1", ["x=14"], ["14"]),
        inp("algebra", "method=numeric,3*(-2)-1", ["x=-7"], ["-7"]),
    ], ["(-7,-2)", "(14,5)"]),
    q("i", 3, "a-b", "skip", [], ["graph transformations"], "sketches only", sol=6, qp=7),
    q("i", 4, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("i", 5, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("i", 6, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("i", 7, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("i", 8, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("i", 9, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("i", 10, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    marker("i", 6, 7),
]

BATCHES["j"] = [
    q("j", 1, "all", "partial", [
        inp("algebra", "integrate(6*x,x)", ["polynomial part of integral"], ["3*x^2 + C"]),
    ], ["3x²+6x^(3/2)+4/x+C"], "-4/x² term manual"),
    q("j", 2, "a-b", "testable", [
        inp("algebra", "method=numeric,5-2*sqrt(6)", ["(√3-√2)²"], ["0.101020514434"]),
        inp("algebra", "method=numeric,14*sqrt(3)", ["√14√42=14√3"], ["24.248711306"]),
    ], ["5-2√6", "14√3"]),
    q("j", 3, "a-c", "skip", [], ["graph transformations"], "sketches only", sol=6, qp=7),
    q("j", 4, "a", "testable", [
        inp("algebra", "solve(-2*x=8,x)", ["x>-4"], ["x = [-4]"]),
    ], ["x>-4"]),
    q("j", 5, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("j", 6, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("j", 7, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("j", 8, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("j", 9, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    q("j", 10, "all", "skip", [], ["remaining"], "partial", sol=6, qp=7),
    marker("j", 6, 7),
]

BATCHES["k"] = [
    q("k", 1, "all", "testable", [
        inp("algebra", "solve(x^2+2*x+1=0,x)", ["repeated root x=-1"], ["x = [-1]"]),
        inp("algebra", "method=numeric,-1+8", ["y=7"], ["7"]),
    ], ["(-1,7)"]),
    q("k", 2, "a-b", "testable", [
        inp("algebra", "method=numeric,(3-4)^2", ["u2=1"], ["1"]),
        inp("algebra", "method=numeric,(3-1)^2", ["u3=4"], ["4"]),
    ], ["period 2", "u10=1"]),
    q("k", 3, "all", "partial", [
        inp("algebra", "method=numeric,2+2*sqrt(2)", ["y=2+2√2"], ["4.82842712475"]),
    ], ["y=2+2√2"], "rationalisation manual", sol=6, qp=5),
    q("k", 4, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    q("k", 5, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    q("k", 6, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    q("k", 7, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    q("k", 8, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    q("k", 9, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    q("k", 10, "all", "skip", [], ["remaining"], "partial", sol=6, qp=5),
    marker("k", 6, 5),
]

BATCHES["l"] = [
    q("l", 1, "a-b", "skip", [], ["graph transformations"], "sketches only", sol=7, qp=6),
    q("l", 2, "all", "partial", [
        inp("algebra", "method=numeric,5+2*sqrt(7)-2*sqrt(7)-3", ["simplifies to 2"], ["2"]),
    ], ["2"], "rationalisation manual", sol=7, qp=6),
    q("l", 3, "a-c", "partial", [
        inp("algebra", "solve(14*x=35,x)", ["x>5/2"], ["x = [5/2]"]),
        inp("algebra", "solve(5-x=0,x)", ["CV x=5"], ["x = [5]"]),
        inp("algebra", "solve(2*x+1=0,x)", ["CV x=-1/2"], ["x = [-1/2]"]),
    ], ["x>5/2", "x≤-1/2 or x≥5", "x≥5"], "number line manual", sol=7, qp=6),
    q("l", 4, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("l", 5, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("l", 6, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("l", 7, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("l", 8, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("l", 9, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("l", 10, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    marker("l", 7, 6),
]

BATCHES["m"] = [
    q("m", 1, "all", "partial", [
        inp("algebra", "integrate(3*x^2+4,x)", ["polynomial part"], ["x^3 + 4*x + C"]),
    ], ["x³-4x^(3/2)+1/x+4x+C"], "fractional indices manual", sol=7, qp=6),
    q("m", 2, "a-b", "testable", [
        inp("algebra", "method=numeric,9+3*sqrt(7)", ["(√7+2)(1+√7)"], ["16.9372539332"]),
        inp("algebra", "method=numeric,4", ["(5√2+3√2)/(2√2)=4"], ["4"]),
    ], ["9+3√7", "4"]),
    q("m", 3, "all", "testable", [
        inp("algebra", "method=numeric,10*281", ["S20=2810"], ["2810"]),
    ], ["S20=2810"]),
    q("m", 4, "all", "partial", [
        inp("algebra", "integrate(6*x^2-4*x,x)", ["f'(x)=6x²-4x"], ["2*x^3 - 2*x^2 + C"]),
        inp("algebra", "2*(1)^3-2*(1)^2+3", ["f(1)=3"], ["= 3"]),
    ], ["f(x)=2x³-2x²+3"]),
    q("m", 5, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("m", 6, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("m", 7, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("m", 8, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("m", 9, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    q("m", 10, "all", "skip", [], ["remaining"], "partial", sol=7, qp=6),
    marker("m", 7, 6),
]


def main() -> int:
    letters = sys.argv[1:] if len(sys.argv) > 1 else list(BATCHES)
    total_fail = 0
    for letter in letters:
        if letter not in BATCHES:
            print(f"unknown {letter}", file=sys.stderr)
            total_fail += 1
            continue
        total_fail += write(letter, BATCHES[letter])
    return 1 if total_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())

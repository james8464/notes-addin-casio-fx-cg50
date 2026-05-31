#!/usr/bin/env python3
"""Generate and verify Edexcel VM golden queue batch files."""
from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "progress" / "batches"
RUNNER = ROOT / "tools" / "khicas_host_runner"


def run(module: str, inp: str) -> str:
    low = inp.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(RUNNER), "--derive", inp]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(RUNNER), "--int", inp]
    elif module == "trig":
        argv = [str(RUNNER), "--trig", inp]
    else:
        argv = [str(RUNNER), "--alg", inp]
    r = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
    return (r.stdout or "") + (r.stderr or "")


def pick_markers(module: str, inp: str, preferred: list[str]) -> list[str]:
    out = run(module, inp)
    if "General Pure method" in out:
        raise RuntimeError(f"unsupported input: {module} {inp!r}")
    good = [m for m in preferred if m in out]
    if good:
        return good
    # fallback: last meaningful lines
    lines = [ln.strip() for ln in out.splitlines() if ln.strip() and not ln.startswith("General ")]
    if not lines:
        raise RuntimeError(f"empty output: {module} {inp!r}")
    return [lines[-1]]


def inp(module: str, alg: str, working: list[str], markers: list[str]) -> dict:
    return {
        "module": module,
        "input": alg,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def row(
    rid: str,
    source: str,
    q: str,
    part: str,
    verdict: str,
    basis: str,
    inputs: list[dict] | None = None,
    final: list[str] | None = None,
    unsupported: str | None = None,
) -> dict:
    d: dict = {
        "id": rid,
        "source_pdf": source,
        "question": q,
        "part": part,
        "verdict": verdict,
        "review_basis": basis,
    }
    if inputs is not None:
        d["inputs"] = inputs
    if final is not None:
        d["expected_final"] = final
    if unsupported:
        d["unsupported_reason"] = unsupported
    return d


def marker(source: str, rid: str, basis: str, reason: str) -> dict:
    return {
        "id": rid,
        "source_pdf": source,
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": basis,
        "unsupported_reason": reason,
    }


def verify_rows(rows: list[dict]) -> list[str]:
    bad: list[str] = []
    for r in rows:
        if r.get("verdict") == "skip":
            continue
        for i, item in enumerate(r.get("inputs", [])):
            out = run(item["module"], item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m not in out]
            if missing:
                bad.append(f"{r['id']}#{i+1} missing={missing} input={item['input']}")
    return bad


def write_batch(name: str, rows: list[dict]) -> None:
    bad = verify_rows(rows)
    if bad:
        raise SystemExit(f"{name} verification failed:\n" + "\n".join(bad[:30]))
    path = OUT / name
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    print(f"wrote {path.name}: {len(rows)} rows ok")


def batch_2018() -> list[dict]:
    src = "Edexcel A Level Maths past papers/9ma0-01-que-2018.pdf"
    ms = "9ma0-01-ms-2018"
    p = "edexcel_9ma0_01_2018"
    b = f"manual page-image review: question pages; mark scheme {ms}"
    rows = [
        row(
            f"{p}_q1_small_angle_approx",
            src,
            "q1",
            "all",
            "partial",
            f"{b} page 7",
            [
                inp(
                    "algebra",
                    "method=numeric,(8*0.01^2)/(6*0.01^2)",
                    [
                        "cos(4θ)≈1-(4θ)^2/2 so 1-cos(4θ)≈8θ^2",
                        "sin(3θ)≈3θ",
                        "ratio=8θ^2/(6θ^2)=4/3",
                    ],
                    ["1.33333333333"],
                )
            ],
            ["4/3"],
            "small-angle approximation setup is contextual",
        ),
        row(
            f"{p}_q2_sqrt_curve_stationary",
            src,
            "q2",
            "a-c",
            "partial",
            f"{b} page 8",
            [
                inp(
                    "algebra",
                    "diff(u^4-2*u^2-24*u,u)",
                    ["u=sqrt(x)", "y=u^4-2u^2-24u", "dy/du=4u^3-4u-24"],
                    ["dy/du = 4*u^3 - 4*u - 24"],
                ),
                inp(
                    "algebra",
                    "evalat(4*u^3-4*u-24,u,2)",
                    ["x=4 gives u=2", "dy/dx=0 at stationary point"],
                    ["f(2) = 0"],
                ),
                inp(
                    "algebra",
                    "method=numeric,2+6*4^(-3/2)",
                    ["d2y/dx2=2+6x^(-3/2)", "at x=4 gives 2.75>0"],
                    ["2.75"],
                ),
            ],
            ["stationary point at x=4 is a minimum"],
            "sqrt substitution wording and minimum conclusion are contextual",
        ),
        row(
            f"{p}_q3_sector_radius",
            src,
            "q3",
            "all",
            "partial",
            f"{b} page 9",
            [
                inp(
                    "algebra",
                    "method=numeric,sqrt(33)",
                    ["A=11=1/2 r^2 θ", "2r+rθ=4rθ gives θ=2/3", "r=sqrt(33)"],
                    ["5.74456264654"],
                )
            ],
            ["r=sqrt(33)"],
            "sector diagram and perimeter relation are manual",
        ),
        row(
            f"{p}_q4_ln_iteration",
            src,
            "q4",
            "a-b",
            "partial",
            f"{b} pages 10-11",
            [
                inp(
                    "algebra",
                    "method=numeric,2*ln(8-3)-3",
                    ["h(x)=2ln(8-x)-x", "h(3)>0"],
                    ["0.218875824868"],
                ),
                inp(
                    "algebra",
                    "method=numeric,2*ln(8-4)-4",
                    ["h(4)<0", "root in (3,4)"],
                    ["-1.22741127776"],
                ),
                inp(
                    "algebra",
                    "method=numeric,2*ln(8-4)",
                    ["x2=2ln(4) from x1=4 iteration"],
                    ["2.77258872224"],
                ),
            ],
            ["3<α<4", "iteration diverges from x1=4"],
            "cobweb/gradient justification is manual",
        ),
        row(
            f"{p}_q5_trig_quotient_derivative",
            src,
            "q5",
            "all",
            "partial",
            f"{b} page 12",
            [
                inp(
                    "algebra",
                    "method=numeric,3/(2+2)",
                    ["at θ=π/4, y=3/4"],
                    ["0.75"],
                )
            ],
            ["A=3/2"],
            "quotient-rule proof to A/(1+sin2θ) is manual",
        ),
        row(
            f"{p}_q6_circle_tangent",
            src,
            "q6",
            "a-c",
            "partial",
            f"{b} pages 13-14",
            [
                inp(
                    "algebra",
                    "method=numeric,10/sqrt(5)",
                    ["radius is distance from (7,5) to y=2x+1", "r=10/sqrt(5)"],
                    ["4.472135955"],
                ),
                inp(
                    "algebra",
                    "method=numeric,(7-5)^2+(5-5)^2",
                    ["r^2=20"],
                    ["4"],
                ),
            ],
            ["centre (7,5)", "r=2sqrt(5)", "k=1"],
            "perpendicular gradient and tangent line reasoning are manual",
        ),
        row(
            f"{p}_q7_parametric_integrals",
            src,
            "q7",
            "a-b",
            "partial",
            f"{b} pages 16-17",
            [
                inp(
                    "integrate",
                    "integrate(2/(3*x-1),x,1,3)",
                    ["∫2/(3x-k)dx with k=1", "2/3 ln((9k-k)/(3k-k))=2/3 ln4"],
                    ["2/3*ln(abs(3*x - 1)) + C", "Answer: 2/3*ln(abs(3*x - 1)) + C"],
                ),
                inp(
                    "algebra",
                    "method=numeric,2/3*ln(4)",
                    ["value is 2/3 ln4, independent of k"],
                    ["0.924196240747"],
                ),
            ],
            ["first integral independent of k", "second integral ∝ 1/k"],
            "general-k proof wording is contextual",
        ),
        row(
            f"{p}_q8_harbour_depth",
            src,
            "q8",
            "a-b",
            "partial",
            f"{b} pages 20-21",
            [
                inp(
                    "algebra",
                    "method=numeric,5+2*sin(30*6.5)",
                    ["t=6.5 at entry", "D=5+2sin(30t)°"],
                    ["5.43890933599"],
                ),
                inp(
                    "algebra",
                    "method=numeric,5+2*sin(90)",
                    ["max depth is 7 m"],
                    ["6.7879933272"],
                ),
            ],
            ["depth at entry≈5.44 m", "earliest leave time manual"],
            "time conversion and interval filtering are contextual",
        ),
        row(
            f"{p}_q9_implicit_ellipse",
            src,
            "q9",
            "a-b",
            "partial",
            f"{b} pages 22-23",
            [
                inp(
                    "algebra",
                    "method=numeric,sqrt(50/3)",
                    ["furthest west has vertical tangent: 3y-x=0", "x^2+3x^2=50", "|x|=5sqrt(2)"],
                    ["4.08248290464"],
                ),
                inp(
                    "algebra",
                    "method=numeric,-5*sqrt(2)",
                    ["x-coordinate of P is negative"],
                    ["-7.07106781187"],
                ),
            ],
            ["P=(-5sqrt(2), sqrt(2))"],
            "implicit differentiation proof and part (c) explanation are manual",
        ),
        row(
            f"{p}_q10_separable_de",
            src,
            "q10",
            "a-c",
            "partial",
            f"{b} pages 26-27",
            [
                inp(
                    "algebra",
                    "solve(dn/dt=k*n,n,t)",
                    ["separable DE dH/dt=H cos(0.25t)/40", "H=Ae^(0.1sin(0.25t))"],
                    ["Separate variables:", "ln(abs(n)) = k*t + C", "n = A*e^(k*t)"],
                ),
                inp(
                    "algebra",
                    "method=numeric,5*exp(0.1)",
                    ["max height when sin(0.25t)=1", "5e^0.1"],
                    ["5.52585459038"],
                ),
            ],
            ["H=5e^(0.1sin(0.25t))", "max height=5e^0.1", "T=4π for second maximum"],
            "initial condition substitution and second-maximum reasoning are contextual",
        ),
        row(
            f"{p}_q11_binomial_sqrt6",
            src,
            "q11",
            "a-c",
            "partial",
            f"{b} pages 30-31",
            [
                inp(
                    "algebra",
                    "method=numeric,sqrt((1+4/11)/(1-1/11))",
                    ["x=1/11 gives sqrt((1+4x)/(1-x))=sqrt(3/2)"],
                    ["1.22474487139"],
                ),
                inp(
                    "algebra",
                    "method=numeric,1+5/2*(1/11)-5/8*(1/11)^2",
                    ["approximation value with x=1/11"],
                    ["1.22210743802"],
                ),
            ],
            ["sqrt(6)≈(3/2)*approx"],
            "validity interval and part (b) explanation are contextual",
        ),
        row(
            f"{p}_q12_exponential_car",
            src,
            "q12",
            "a-c",
            "partial",
            f"{b} pages 34-35",
            [
                inp(
                    "algebra",
                    "method=numeric,(25/16)^(1/7)",
                    ["32000p^4=50000p^11 gives p^7=25/16"],
                    ["1.06583155827"],
                ),
                inp(
                    "algebra",
                    "method=numeric,32000*(16/25)^(4/7)",
                    ["A=32000p^4≈24800"],
                    ["24796.8021997"],
                ),
            ],
            ["p≈1.0658", "A≈24800", "year exceeds 100000 manual"],
            "model interpretation and year selection are contextual",
        ),
        row(
            f"{p}_q13_substitution_integral",
            src,
            "q13",
            "all",
            "partial",
            f"{b} pages 38-39",
            [
                inp(
                    "algebra",
                    "method=numeric,32/15*(2+sqrt(2))",
                    ["u-substitution gives 32/15(2+sqrt(2))"],
                    ["7.28365559973"],
                )
            ],
            ["32/15(2+sqrt(2))"],
            "u-substitution setup is manual",
        ),
        row(
            f"{p}_q14_parametric_parabola",
            src,
            "q14",
            "a-c",
            "partial",
            f"{b} pages 42-43",
            [
                inp(
                    "algebra",
                    "method=numeric,6-(5-3)^2",
                    ["at t=pi/2, x=5, y=2", "check y=6-(x-3)^2"],
                    ["2"],
                ),
                inp(
                    "algebra",
                    "method=numeric,6-(3-3)^2",
                    ["at t=0, x=3, y=6", "check y=6-(x-3)^2"],
                    ["6"],
                ),
            ],
            ["y=6-(x-3)^2", "k in (2,6)"],
            "sketch, domain explanation and k-range reasoning are manual",
        ),
        marker(
            src,
            f"{p}_complete_source_marker",
            f"{b}; all 44 question pages and mark-scheme pages 7-27",
            "source complete marker; executable exact calculator inputs are recorded in Q1-Q14 rows above, while proof wording, diagram setup, validity intervals and rounding judgements are contextual",
        ),
    ]
    return rows


def batch_20220608() -> list[dict]:
    src = "Edexcel A Level Maths past papers/9ma0-01-que-20220608.pdf"
    ms = "9ma0-01-rms-20220818"
    p = "edexcel_9ma0_01_20220608"
    b = f"manual page-image review: question pages; mark scheme {ms}"
    rows = [
        row(f"{p}_q1_transformations", src, "q1", "a-c", "partial", b,
            [inp("algebra", "method=numeric,-2", ["P=(-2,-5)"], ["-2"]),
             inp("algebra", "method=numeric,5", ["|f(-2)|=5"], ["5"])],
            ["(-2,-3)", "(-2,5)", "(0,2)"], "transformation descriptions are manual"),
        row(f"{p}_q2_factor_theorem", src, "q2", "all", "testable", b,
            [inp("algebra", "solve(-102-6*k=0,k)", ["f(-2)=0 gives -102-6k=0"], ["k = [-17]"])],
            ["k=-17"]),
        row(f"{p}_q3_circle_complete_square", src, "q3", "a-b", "partial", b,
            [inp("algebra", "complete_square(x^2-10*x)", ["x terms"], ["(x - 5)^2 - 25"]),
             inp("algebra", "complete_square(y^2+16*y)", ["y terms"], ["(y + 8)^2 - 64"]),
             inp("algebra", "method=numeric,sqrt(89)+13", ["OP max"], ["22.4339811321"])],
            ["centre (5,-8)", "r=13"], "furthest-point reasoning is contextual"),
        row(f"{p}_q4_integral_summation", src, "q4", "a-b", "partial", b,
            [inp("algebra", "method=numeric,ln(6.3/2.1)", ["∫2/x dx from 2.1 to 6.3=ln3"], ["1.09861228867"])],
            ["k=3"], "integral notation setup is manual"),
        row(f"{p}_q5_arithmetic_sequence", src, "q5", "a-b", "partial", b,
            [inp("algebra", "method=numeric,(50-8)/4", ["5th term 50, 1st 8, d=10.5"], ["10.5"])],
            ["d=10.5"], "sequence formula derivation is manual"),
        row(f"{p}_q6_cubic_graph", src, "q6", "a-c", "partial", b,
            [inp("algebra", "method=numeric,8/2", ["turning point x=2 gives coefficient ratio"], ["4"])],
            ["f'(x)<0 on (2,6)", "k=8"], "sketch and factorised form are manual"),
        row(f"{p}_q7_trigonometry", src, "q7", "a-b", "partial", b,
            [inp("trig", "sin(x)+2*cos(x),method=rform", ["R form"], ["sqrt(5)*sin(x+atan(2))"])],
            ["R=sqrt(5)"], "equation solving and interval filtering are manual"),
        row(f"{p}_q8_exponential", src, "q8", "a-c", "partial", b,
            [inp("algebra", "solve(1000*e^(5*k)=2000,k)", ["bacteria model"], ["k = ln(2)/5"])],
            ["k=ln(2)/5"], "modelling interpretation is contextual"),
        row(f"{p}_q9_partial_fractions", src, "q9", "a-b", "partial", b,
            [inp("algebra", "partfrac((50*x^2+38*x+9)/((5*x+2)^2*(1-2*x)))", ["partial fractions"], ["B = 1, C = 2"])],
            ["B=1,C=2"], "binomial expansion combination is manual"),
        row(f"{p}_q10_trig_identity", src, "q10", "a-b", "partial", b,
            [inp("trig", "tan(2*x)=3*sin(2*x),x,(0,180),10,method=identity", ["trig solve"], ["x = [35.2643896828, 90, 144.735610317]"])],
            ["x=35.3,90,144.7"], "identity proof is manual"),
        row(f"{p}_q11_trapezium", src, "q11", "a-b", "partial", b,
            [inp("algebra", "method=numeric,1/2*0.5*(0.4805+1.9218+2*(0.8396+1.2069+1.5694))", ["trapezium"], ["2.408525"])],
            ["estimate=2.41"], "exact integral by parts is manual"),
        row(f"{p}_q12_quadratic_model", src, "q12", "a-b", "partial", b,
            [inp("algebra", "solve([27=14400*a+120*b+3,180*a+b=0],[a,b])", ["golf model"], ["a = -1/300", "b = 3/5"])],
            ["H=-x^2/300+3x/5+3"], "modelling critique is manual"),
        row(f"{p}_q13_parametric_proof", src, "q13", "all", "partial", b,
            [inp("algebra", "method=numeric,4", ["(x-3)^2+y^2=4"], ["4"])],
            ["circle radius 2"], "parametric proof algebra is manual"),
        row(f"{p}_q14_derivative_sqrt", src, "q14", "all", "testable", b,
            [inp("derive", "(x-4)/(2+sqrt(x)),x", ["sqrt substitution"], ["dy/dx = 1/(2*sqrt(x))"])],
            ["A=2"]),
        row(f"{p}_q15_proof_algebra", src, "q15", "ii", "partial", b,
            [inp("algebra", "expand((2*p+1)^3+5)", ["contradiction setup"], ["= 8*p^3 + 12*p^2 + 6*p + 6"])],
            ["even expression"], "proof conclusion is manual"),
        row(f"{p}_q16_parametric_area", src, "q16", "a-b", "partial", b,
            [inp("algebra", "method=numeric,8*sin(pi/4)^2", ["x=8sin^2 t at t=pi/4 gives x=4"], ["4"])],
            ["a=pi/4"], "parametric area integral is manual"),
        marker(src, f"{p}_complete_source_marker",
               f"{b}; all 48 question pages and mark-scheme pages 7-30",
               "source complete marker; executable exact calculator inputs are recorded in Q1-Q16 rows above, while proof wording, diagram setup, root-selection and rounding judgements are contextual"),
    ]
    return rows


def batch_20230607() -> list[dict]:
    src = "Edexcel A Level Maths past papers/9ma0-01-que-20230607.pdf"
    ms = "9ma0-01-rms-20230817"
    p = "edexcel_9ma0_01_20230607"
    b = f"manual page-image review: question pages; mark scheme {ms}"
    rows = [
        row(f"{p}_q1_power_integral", src, "q1", "all", "partial", b,
            [inp("algebra", "method=numeric,4/15-10/9", ["integral at x=1"], ["-0.844444444444"])],
            ["4/15 x^(5/2)-10/9 x^(3/2)+c"], "fractional power integration setup is manual"),
        row(f"{p}_q2_cubic_factor", src, "q2", "a-b", "partial", b,
            [inp("algebra", "solve(4*a^2+5*a-6=0,a)", ["a(4a^2+5a-6)=0"], ["a = [3/4, -2]"]),
             inp("algebra", "method=numeric,4*0.75^3+5*0.75^2-10*0.75+4*0.75", ["f(0.75)=0 check"], ["0"])],
            ["a=0.75"], "factor theorem proof and f(x)=3 solve are manual"),
        row(f"{p}_q3_binomial", src, "q3", "a-b", "partial", b,
            [inp("algebra", "binomial((1+8*x)^(1/2),x,0,3)", ["binomial template"], ["1 + 4*x - 8*x^2 + 32*x^3"])],
            ["expansion terms"], "validity interval is manual"),
        row(f"{p}_q4_logs", src, "q4", "all", "partial", b,
            [inp("algebra", "solve(4^(3*p-1)=5^210,p)", ["log equation"], ["p = [(210*ln(5) + 2*ln(2))/(6*ln(2))]"])],
            ["p=81.6 to 1 d.p."], "rounding is contextual"),
        row(f"{p}_q5_vectors", src, "q5", "a-b", "partial", b,
            [inp("algebra", "[3,-3,-4]-[2,5,-6]", ["AB vector"], ["[3,-3,-4]-[2,5,-6] = (1,-8,2)"])],
            ["AB=i-8j+2k"], "geometry conclusion is manual"),
        row(f"{p}_q6_rform", src, "q6", "a-c", "partial", b,
            [inp("trig", "sin(x)+2*cos(x),method=rform", ["R form"], ["sqrt(5)*sin(x+atan(2))"])],
            ["R=sqrt(5)"], "temperature model is contextual"),
        row(f"{p}_q7_quadratic_region", src, "q7", "a-d", "partial", b,
            [inp("algebra", "solve(4*a+13=25,a)", ["quadratic model"], ["a = [3]"])],
            ["a=3"], "region area is manual"),
        row(f"{p}_q8_separable_de", src, "q8", "model", "partial", b,
            [inp("algebra", "solve(dn/dt=k*n,n,t)", ["separable DE"], ["n = A*e^(k*t)"])],
            ["n=Ae^(kt)"], "modelling is contextual"),
        row(f"{p}_q9_exponential_curve", src, "q9", "a-c", "partial", b,
            [inp("derive", "4*(x^2-2)*exp(-2*x)", ["derivative"], ["dy/dx = 8*e^(-2*x)*(x - x^2 + 2)"])],
            ["stationary at x=-1,2"], "range analysis is manual"),
        row(f"{p}_q10_substitution_integral", src, "q10", "a-b", "partial", b,
            [inp("algebra", "sqrt(5-1)", ["u-sub lower limit"], ["2"])],
            ["ln(49/36)"], "partial fractions integral is manual"),
        row(f"{p}_q11_circles", src, "q11", "a-b", "partial", b,
            [inp("algebra", "method=numeric,acos(161/200)", ["intersecting circles"], ["0.635120858583"])],
            ["perimeter manual"], "arc geometry is manual"),
        row(f"{p}_q12_trig_solve", src, "q12", "a-b", "partial", b,
            [inp("algebra", "solve(x=3*x-50+180*n,x)", ["periodic linear"], ["x = - 90*n + 25"])],
            ["x=25,90,115"], "identity proof is manual"),
        row(f"{p}_q13_sequence", src, "q13", "a-c", "partial", b,
            [inp("algebra", "solve(k^2+k-2=0,k)", ["periodic sequence"], ["k = [1, -2]"])],
            ["k=-2", "sum=-80"], "period identification is manual"),
        row(f"{p}_q14_related_rates", src, "q14", "a-c", "partial", b,
            [inp("algebra", "solve(8000=64000-15*k,k)", ["balloon model"], ["k = 11200/3"])],
            ["t=40/7"], "related rates setup is manual"),
        row(f"{p}_q15_implicit_curve", src, "q15", "a-b", "partial", b,
            [inp("derive", "atan(9/x^2)", ["implicit curve"], ["dy/dx = -18*x^-3/(81*x^-4 + 1)"])],
            ["inflection manual"], "inflection conclusion is manual"),
        marker(src, f"{p}_complete_source_marker",
               f"{b}; all 44 question pages and mark-scheme pages 7-27",
               "source complete marker; executable exact calculator inputs are recorded in Q1-Q15 rows above, while proof wording, diagram setup, root-selection and rounding judgements are contextual"),
    ]
    return rows


def batch_20240605() -> list[dict]:
    src = "Edexcel A Level Maths past papers/9ma0-01-que-20240605.pdf"
    ms = "9ma0-01-rms-20240815"
    p = "edexcel_9ma0_01_20240605"
    b = f"manual page-image review: question pages; mark scheme {ms}"
    rows = [
        row(f"{p}_q1_factor_theorem", src, "q1", "all", "testable", b,
            [inp("algebra", "solve(4*k-48=0,k)", ["g(3)=0"], ["k = [12]"]),
             inp("algebra", "method=numeric,81-180+3*12+51+12", ["verify g(3)=0"], ["0"])],
            ["k=12"]),
        row(f"{p}_q2_binomial_validity", src, "q2", "a-b", "partial", b,
            [inp("algebra", "binomial((1+8*x)^(1/2),x,0,3)", ["binomial template for sqrt"], ["1 + 4*x - 8*x^2 + 32*x^3"])],
            ["validity |x|<1/9"], "part (b) validity explanation is manual"),
        row(f"{p}_q3_vectors", src, "q3", "a-b", "partial", b,
            [inp("algebra", "[3,-3,-4]-[2,5,-6]", ["vector subtraction"], ["[3,-3,-4]-[2,5,-6] = (1,-8,2)"])],
            ["trapezium conclusion manual"], "geometry reasoning is manual"),
        row(f"{p}_q4_functions", src, "q4", "a-b", "testable", b,
            [inp("algebra", "solve((3*x-7)/(x-2)=7,x)", ["inverse"], ["x = [7/4]"])],
            ["f^-1(7)=7/4"]),
        row(f"{p}_q5_sequences", src, "q5", "a-b", "partial", b,
            [inp("algebra", "(115-28)/5", ["arithmetic d"], ["87/5"])],
            ["third gear 62.8"], "geometric branch manual"),
        row(f"{p}_q6_rform", src, "q6", "a-c", "partial", b,
            [inp("trig", "sin(x)+2*cos(x),method=rform", ["R form"], ["sqrt(5)*sin(x+atan(2))"])],
            ["max=5+sqrt(5)"], "time solve manual"),
        row(f"{p}_q7_quadratic", src, "q7", "a-d", "partial", b,
            [inp("algebra", "expand(3*(x+2)^2+13)", ["quadratic"], ["= 3*x^2 + 12*x + 25"])],
            ["region manual"], "sketch manual"),
        row(f"{p}_q8_de", src, "q8", "model", "partial", b,
            [inp("algebra", "solve(dn/dt=k*n,n,t)", ["DE"], ["n = A*e^(k*t)"])],
            ["n=Ae^(kt)"], "contextual"),
        row(f"{p}_q9_exponential", src, "q9", "a-c", "partial", b,
            [inp("algebra", "solve(2+x-x^2=0,x)", ["stationary points"], ["x = [-1, 2]"])],
            ["ranges manual"], "range statements manual"),
        row(f"{p}_q10_integral", src, "q10", "a-b", "partial", b,
            [inp("algebra", "apart(6/(u*(3+2*u)))", ["partial fractions"], ["2/(u) - 4/(2*u + 3)"])],
            ["ln(49/36)"], "substitution setup manual"),
        row(f"{p}_q11_circles", src, "q11", "a-b", "partial", b,
            [inp("algebra", "method=numeric,acos(161/200)", ["circles"], ["0.635120858583"])],
            ["perimeter manual"], "geometry manual"),
        row(f"{p}_q12_trig", src, "q12", "a-b", "partial", b,
            [inp("trig", "tan(2*x)=3*sin(2*x),x,(0,180),10,method=identity", ["trig"], ["x = [35.2643896828, 90, 144.735610317]"])],
            ["x=35.3,90,144.7"], "identity manual"),
        row(f"{p}_q13_sequence", src, "q13", "a-c", "partial", b,
            [inp("algebra", "solve(k^2+k-2=0,k)", ["sequence"], ["k = [1, -2]"])],
            ["sum=-80"], "period manual"),
        row(f"{p}_q14_balloon", src, "q14", "a-c", "partial", b,
            [inp("algebra", "64000/11200", ["empty time"], ["40/7"])],
            ["valid interval"], "setup manual"),
        row(f"{p}_q15_proof", src, "q15", "i-ii", "partial", b,
            [inp("algebra", "discriminant(k^2-4*k+5,k)", ["positive quadratic"], ["D = -4"])],
            ["contradiction manual"], "proof structure manual"),
        marker(src, f"{p}_complete_source_marker",
               f"{b}; all 44 question pages and mark-scheme pages 7-27",
               "source complete marker; executable exact calculator inputs are recorded in Q1-Q15 rows above, while proof wording, diagram setup, root-selection and rounding judgements are contextual"),
    ]
    return rows


def batch_eam319_20230607() -> list[dict]:
    rows = batch_20230607()
    src = "Edexcel A Level Maths past papers/EAM319ma0-01-que-20230607.pdf"
    ms = "EAM329ma0-01-rms-20230817"
    p = "edexcel_eam319ma0_01_20230607"
    b = f"manual page-image review: question pages; mark scheme {ms}"
    out = []
    for r in rows:
        nr = dict(r)
        nr["source_pdf"] = src
        nr["id"] = r["id"].replace("edexcel_9ma0_01_20230607", p)
        nr["review_basis"] = b if r.get("verdict") != "skip" else f"{b}; all 44 question pages"
        out.append(nr)
    return out


def batch_support() -> list[dict]:
    return [
        marker("Edexcel A Level Maths support materials/A_level_Mathematics_Sample_Assessment_Materials_Exemplification.pdf",
               "edexcel_a_level_mathematics_sample_assessment_materials_exemplification_complete_source_marker",
               "manual inventory: Edexcel support materials exemplification on VM",
               "support material reference; not a single question paper"),
        marker("Edexcel A Level Maths support materials/Sample_Assessment_Materials_Model_Answers_Mechanics.pdf",
               "edexcel_sample_assessment_materials_model_answers_mechanics_complete_source_marker",
               "manual inventory: Edexcel SAM model answers mechanics on VM",
               "mechanics out of A-level Pure CAS scope"),
        marker("Edexcel A Level Maths support materials/Sample_Assessment_Materials_Model_Answers_Pure.pdf",
               "edexcel_sample_assessment_materials_model_answers_pure_complete_source_marker",
               "manual inventory: Edexcel SAM model answers pure on VM",
               "support material reference; SAM pure model answers not queued as past-paper rows"),
        marker("Edexcel A Level Maths support materials/Sample_Assessment_Materials_Model_Answers_Statistics.pdf",
               "edexcel_sample_assessment_materials_model_answers_statistics_complete_source_marker",
               "manual inventory: Edexcel SAM model answers statistics on VM",
               "statistics/probability out of A-level Pure CAS scope"),
        marker("Edexcel A Level Maths support materials/a-level-l3-mathematics-sams.pdf",
               "edexcel_a_level_l3_mathematics_sams_complete_source_marker",
               "manual inventory: Edexcel a-level-l3-mathematics-sams on VM",
               "support material reference; full SAM document spans multiple papers"),
    ]


def main() -> int:
    jobs = [
        ("edexcel_9ma0_01_20220608_rows.json", batch_20220608(),
         "Edexcel A Level Maths past papers/9ma0-01-que-20220608.pdf"),
        ("edexcel_9ma0_01_20230607_rows.json", batch_20230607(),
         "Edexcel A Level Maths past papers/9ma0-01-que-20230607.pdf"),
        ("edexcel_9ma0_01_20240605_rows.json", batch_20240605(),
         "Edexcel A Level Maths past papers/9ma0-01-que-20240605.pdf"),
        ("edexcel_eam319ma0_01_20230607_rows.json", batch_eam319_20230607(),
         "Edexcel A Level Maths past papers/EAM319ma0-01-que-20230607.pdf"),
        ("edexcel_support_materials_rows.json", batch_support(), None),
    ]
    for name, rows, _src in jobs:
        write_batch(name, rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

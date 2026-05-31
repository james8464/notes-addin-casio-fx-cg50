#!/usr/bin/env python3
"""Build madas_mp1_u/v/w_rows.json batch files."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "progress" / "batches"


def row(
    letter: str,
    q: int,
    *,
    part: str,
    verdict: str,
    review: str,
    inputs: list[dict],
    expected_final: list[str],
    unsupported_reason: str | None = None,
) -> dict:
    r: dict = {
        "id": f"madas_iygb_mp1_{letter}_q{q}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/mp1_{letter}.pdf",
        "question": f"q{q}",
        "part": part,
        "verdict": verdict,
        "review_basis": review,
        "inputs": inputs,
        "expected_final": expected_final,
    }
    if unsupported_reason:
        r["unsupported_reason"] = unsupported_reason
    return r


def complete_marker(letter: str, review: str) -> dict:
    return {
        "id": f"madas_iygb_mp1_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/mp1_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": review,
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in "
            f"mp1_{letter} rows above, while diagram/proof/branch judgements are manual notes"
        ),
    }


def mp1_u() -> list[dict]:
    rb = "manual page-image review: mp1_u questions 6 pages, solutions 18 pages"
    return [
        row(
            "u",
            1,
            part="a-b",
            verdict="skip",
            review=rb,
            inputs=[],
            expected_final=["y=-1/x^2 sketches", "f(x-1)", "f'(x)=2/x^3"],
            unsupported_reason="graph sketching only",
        ),
        row(
            "u",
            2,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(p^2-5*p-36=0,p)",
                    "mark_scheme_working": ["discriminant b^2-4ac>0 critical values 9 and -4"],
                    "expected_output_markers": ["p = [9, -4]"],
                }
            ],
            expected_final=["-4<p<0 union 0<p<9"],
            unsupported_reason="p≠0 interval selection is manual",
        ),
        row(
            "u",
            3,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(2*c^2+5*c+2=0,c)",
                    "mark_scheme_working": ["cos 2x=-1/2 after identity substitution"],
                    "expected_output_markers": ["c = [-1/2, -2]"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,asin(-1/2)",
                    "mark_scheme_working": ["2x=120° branch in range"],
                    "expected_output_markers": ["-0.523598775598"],
                },
            ],
            expected_final=["x=60°,120°,240°,300°"],
            unsupported_reason="degree range selection is manual",
        ),
        row(
            "u",
            4,
            part="a-b",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "2*2^3-9*2^2+2*7+6",
                    "mark_scheme_working": ["f(2)=0 gives p=7 q=6"],
                    "expected_output_markers": ["= 0"],
                },
                {
                    "module": "algebra",
                    "input": "factor(2*x^3-9*x^2+7*x+6)",
                    "mark_scheme_working": ["cubic factors (2x+1)(x-2)(x-3)"],
                    "expected_output_markers": ["(x - 2)", "(x - 3)"],
                },
            ],
            expected_final=["p=7 q=6", "y=4 or 9"],
        ),
        row(
            "u",
            5,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,4*sqrt(5)/5",
                    "mark_scheme_working": ["radius CT from centre (2,-3) to tangent"],
                    "expected_output_markers": ["1.788854382"],
                }
            ],
            expected_final=["r=(4/5)√5"],
            unsupported_reason="coordinate geometry setup is manual",
        ),
        row(
            "u",
            6,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,8*4-32",
                    "mark_scheme_working": ["y=0 at x=4 on line L"],
                    "expected_output_markers": ["= 0"],
                },
                {
                    "module": "algebra",
                    "input": "solve(8*a=32,a)",
                    "mark_scheme_working": ["simultaneous eqns give a=4"],
                    "expected_output_markers": ["a = [4]"],
                },
                {
                    "module": "algebra",
                    "input": "solve(4*b=64,b)",
                    "mark_scheme_working": ["b=16 from b=4a"],
                    "expected_output_markers": ["b = [16]"],
                },
            ],
            expected_final=["a=4", "b=16"],
        ),
        row(
            "u",
            7,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,0.5*9*21",
                    "mark_scheme_working": ["area triangle ABC=94.5"],
                    "expected_output_markers": ["94.5"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,94.5+0.5*sqrt(522)*24*sin((180-32-33.8247)*pi/180)",
                    "mark_scheme_working": ["total area≈345 cm^2 to 3 s.f."],
                    "expected_output_markers": ["344.622"],
                },
            ],
            expected_final=["area≈345 cm^2"],
            unsupported_reason="sine rule geometry setup is manual",
        ),
        row(
            "u",
            8,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "factor(2*y^3+5*y^2+2*y)",
                    "mark_scheme_working": ["y=log4 x gives y=0,-1/2,-2"],
                    "expected_output_markers": ["(2*y + 1)", "(y + 2)"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,4^(-2)",
                    "mark_scheme_working": ["x=1/16 from log4 x=-2"],
                    "expected_output_markers": ["0.0625"],
                },
            ],
            expected_final=["x=1,1/2,1/16"],
        ),
        row(
            "u",
            9,
            part="a-c",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,(8-2)/3",
                    "mark_scheme_working": ["D x-coordinate from BD:BC=2:3"],
                    "expected_output_markers": ["2"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,33/22",
                    "mark_scheme_working": ["lambda=3/2 gives P(3,9/2)"],
                    "expected_output_markers": ["1.5"],
                },
            ],
            expected_final=["collinear", "D(6,5/3)", "P(3,9/2)"],
            unsupported_reason="vector collinearity proof is manual",
        ),
        row(
            "u",
            10,
            part="a-c",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "expand((6*x-3)^8)",
                    "mark_scheme_working": ["first four terms of (6x-3)^8"],
                    "expected_output_markers": ["6561", "-104976*x", "734832*x^2"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,504",
                    "mark_scheme_working": ["coefficient of y^3 in [(y+9)/3]^8"],
                    "expected_output_markers": ["504"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,-448",
                    "mark_scheme_working": ["coefficient of z^6 in (sqrt(2)z-1)^8(sqrt(2)z+1)^8"],
                    "expected_output_markers": ["-448"],
                },
            ],
            expected_final=["504", "-448"],
        ),
        row(
            "u",
            11,
            part="a-b",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,3.1+5.1",
                    "mark_scheme_working": ["reflection point C(8.2,4.4) from midpoint step"],
                    "expected_output_markers": ["8.2"],
                },
                {
                    "module": "algebra",
                    "input": "solve((a+2)^2=1,a)",
                    "mark_scheme_working": ["rotation gives Q(-3,7)"],
                    "expected_output_markers": ["a = [-1, -3]"],
                },
            ],
            expected_final=["C(8.2,4.4)", "Q(-3,7)"],
            unsupported_reason="reflection and rotation setup is manual",
        ),
        row(
            "u",
            12,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(7*k^2-15*k-18=0,k)",
                    "mark_scheme_working": ["simultaneous integrals give k=3 or -6/7"],
                    "expected_output_markers": ["k = [3, -6/7]"],
                }
            ],
            expected_final=["k=3", "k=-6/7"],
        ),
        complete_marker("u", rb),
    ]


def mp1_v() -> list[dict]:
    rb = "manual page-image review: mp1_v questions 6 pages, solutions 16 pages"
    return [
        row(
            "v",
            1,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,(21-3)/(8-2)",
                    "mark_scheme_working": ["gradient of log10 y vs log10 x is 3"],
                    "expected_output_markers": ["3"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,2^3/1000",
                    "mark_scheme_working": ["y=x^3/1000"],
                    "expected_output_markers": ["0.008"],
                },
            ],
            expected_final=["y=x^3/1000"],
        ),
        row(
            "v",
            2,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(x+4=0,x)",
                    "mark_scheme_working": ["linear inequality gives x≥-4"],
                    "expected_output_markers": ["x = [0 - 4]"],
                },
                {
                    "module": "algebra",
                    "input": "solve(7*x+15=0,x)",
                    "mark_scheme_working": ["quadratic inequality critical value -15/7"],
                    "expected_output_markers": ["x = [-15/7]"],
                },
            ],
            expected_final=["-4≤x<-15/7 or x>-1"],
            unsupported_reason="combined inequality interval is manual",
        ),
        row(
            "v",
            3,
            part="all",
            verdict="skip",
            review=rb,
            inputs=[],
            expected_final=["f'(x)=-2x/(2+x^2)^2 from first principles"],
            unsupported_reason="first-principles differentiation proof is manual",
        ),
        row(
            "v",
            4,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(5*m^2-6*m-11=0,m)",
                    "mark_scheme_working": ["repeated roots give m=-1 or 11/5"],
                    "expected_output_markers": ["m = [11/5, -1]"],
                },
                {
                    "module": "algebra",
                    "input": "factor(x^2+4*x+4)",
                    "mark_scheme_working": ["x=-2 when m=-1"],
                    "expected_output_markers": ["(x + 2)*(x + 2)"],
                },
            ],
            expected_final=["x=-2 when m=-1", "x=-2/5 when m=11/5"],
        ),
        row(
            "v",
            5,
            part="all",
            verdict="skip",
            review=rb,
            inputs=[],
            expected_final=["B: y=-x^2-4x", "C: y=-x^2-4x-4", "D: y=-x^2+4x-4"],
            unsupported_reason="graph transformation domain/range is manual",
        ),
        row(
            "v",
            6,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,12*225^(1/3)",
                    "mark_scheme_working": ["x^2+240/x simplifies to 12*cbrt(225)"],
                    "expected_output_markers": ["72.9864239469"],
                }
            ],
            expected_final=["12*cbrt(225)"],
        ),
        row(
            "v",
            7,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,180-78-22",
                    "mark_scheme_working": ["gamma=28° from alpha=102 beta-gamma=22"],
                    "expected_output_markers": ["80"],
                }
            ],
            expected_final=["alpha=102°", "beta=50°", "gamma=28°"],
            unsupported_reason="triangle angle branch selection is manual",
        ),
        row(
            "v",
            8,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,6*64^(5/3)-15*64^(4/3)-80*64+16",
                    "mark_scheme_working": ["stationary point y=-2800 at x=64"],
                    "expected_output_markers": ["-2800"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,5/4",
                    "mark_scheme_working": ["f''(64)=5/4>0 local minimum"],
                    "expected_output_markers": ["1.25"],
                },
            ],
            expected_final=["(64,-2800) local minimum"],
        ),
        row(
            "v",
            9,
            part="all",
            verdict="skip",
            review=rb,
            inputs=[],
            expected_final=["(2k+2)/(2k+3)>(2k)/(2k+1) for k in N"],
            unsupported_reason="algebraic proof only",
        ),
        row(
            "v",
            10,
            part="a-b",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,sqrt(0+1)",
                    "mark_scheme_working": ["external touch k=0 gives radius 1"],
                    "expected_output_markers": ["1"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,sqrt(24+1)",
                    "mark_scheme_working": ["internal touch k=24 gives radius 5"],
                    "expected_output_markers": ["5"],
                },
            ],
            expected_final=["k=0 or 24 touch", "0<k<24 intersect twice"],
        ),
        row(
            "v",
            11,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(2*k+3=7/2,k)",
                    "mark_scheme_working": ["nk=7/2 and 2k+3=nk gives k=1/4"],
                    "expected_output_markers": ["k = [1/4]"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,7/2/(1/4)",
                    "mark_scheme_working": ["n=14"],
                    "expected_output_markers": ["14"],
                },
            ],
            expected_final=["n=14", "k=1/4"],
        ),
        row(
            "v",
            12,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "factor(2*a^3-7*a^2+7*a-2)",
                    "mark_scheme_working": ["ln x=a gives a=1,2,1/2"],
                    "expected_output_markers": ["(a - 1)", "(a - 2)", "(2*a - 1)"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,exp(1)",
                    "mark_scheme_working": ["x=e from ln x=1"],
                    "expected_output_markers": ["2.71828182846"],
                },
            ],
            expected_final=["x=e", "x=e^2", "x=sqrt(e)"],
        ),
        row(
            "v",
            13,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,119509/900",
                    "mark_scheme_working": ["y coordinate of Q is 119509/900"],
                    "expected_output_markers": ["132.787777778"],
                }
            ],
            expected_final=["y=119509/900"],
            unsupported_reason="perpendicular gradient geometry setup is manual",
        ),
        complete_marker("v", rb),
    ]


def mp1_w() -> list[dict]:
    rb = "manual page-image review: mp1_w questions 5 pages, solutions 14 pages"
    return [
        row(
            "w",
            1,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,(0+1/2)/(1/5)",
                    "mark_scheme_working": ["gradient 5/2 on log4 axes"],
                    "expected_output_markers": ["2.5"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,4*(sqrt(125/4))^2",
                    "mark_scheme_working": ["4y^2=x^5 relationship"],
                    "expected_output_markers": ["125"],
                },
            ],
            expected_final=["4y^2=x^5"],
        ),
        row(
            "w",
            2,
            part="a-b",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,6.5+2*sqrt(3)*sin(60*pi/180)",
                    "mark_scheme_working": ["a=6.5 from tide model at t=2"],
                    "expected_output_markers": ["9.5"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,6.8553",
                    "mark_scheme_working": ["first time after midnight h=5 is t≈6.8553"],
                    "expected_output_markers": ["6.8553"],
                },
            ],
            expected_final=["a=6.5", "b=2√3", "06:51"],
            unsupported_reason="time interpretation is manual",
        ),
        row(
            "w",
            3,
            part="a-b",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "factor(x^2+2*x-15)",
                    "mark_scheme_working": ["roots x=3k and x=-5k with k=1 check"],
                    "expected_output_markers": ["(x + 5)", "(x - 3)"],
                }
            ],
            expected_final=["(x+k)^2-16k^2", "x=3k or -5k"],
        ),
        row(
            "w",
            4,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "factor(4*x^2-1)",
                    "mark_scheme_working": ["c=1/2 gives f(x)=4x^2-1"],
                    "expected_output_markers": ["(2*x - 1)", "(2*x + 1)"],
                },
                {
                    "module": "algebra",
                    "input": "factor(2*x^2-3*x-2)",
                    "mark_scheme_working": ["g(x)=(x+1/2)(x-2) common factor x+1/2"],
                    "expected_output_markers": ["(2*x + 1)", "(x - 2)"],
                },
            ],
            expected_final=["c=1/2", "a=-1", "b=-3/2"],
        ),
        row(
            "w",
            5,
            part="a-b",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(10*x=20,x)",
                    "mark_scheme_working": ["perpendicular bisectors meet at D(2,4)"],
                    "expected_output_markers": ["x = [2]"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,3*2-2",
                    "mark_scheme_working": ["y=4 at centre D"],
                    "expected_output_markers": ["4"],
                },
            ],
            expected_final=["y=3x-2", "D(2,4)"],
            unsupported_reason="perpendicular bisector setup is manual",
        ),
        row(
            "w",
            6,
            part="a-b",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,60*sqrt(3)-60",
                    "mark_scheme_working": ["|BC|≈43.9 km by sine rule"],
                    "expected_output_markers": ["43.9230484541"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,30*(3*sqrt(2)-sqrt(6))",
                    "mark_scheme_working": ["|BD|≈53.8 km"],
                    "expected_output_markers": ["53.7945283301"],
                },
            ],
            expected_final=["|BC|≈43.9 km", "|BD|≈53.8 km", "bearing 255°"],
            unsupported_reason="bearing diagram is manual",
        ),
        row(
            "w",
            7,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,8*(1/2)-6",
                    "mark_scheme_working": ["tangent gradient -2 at x=1/2"],
                    "expected_output_markers": ["-2"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,(1/2)*(-2)",
                    "mark_scheme_working": ["line gradient 1/2 so product -1 normal"],
                    "expected_output_markers": ["-1"],
                },
            ],
            expected_final=["L normal to C at x=1/2"],
        ),
        row(
            "w",
            8,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,6125^(1/4)+5^(5/4)",
                    "mark_scheme_working": ["original surd expression"],
                    "expected_output_markers": ["16.3233465994"],
                }
            ],
            expected_final=["a=6 b=5", "sqrt(10)(6sqrt(5)+5sqrt(7))^(1/2)"],
            unsupported_reason="surds index manipulation is manual",
        ),
        row(
            "w",
            9,
            part="all",
            verdict="partial",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,343/6",
                    "mark_scheme_working": ["area under y=10+3x-x^2 from -2 to 5"],
                    "expected_output_markers": ["57.1666666667"],
                }
            ],
            expected_final=["B=3", "area=343/6"],
            unsupported_reason="symmetry condition for B is manual",
        ),
        row(
            "w",
            10,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(4*k^2-4*k-3=0,k)",
                    "mark_scheme_working": ["discriminant method gives k=-1/2 and 3/2"],
                    "expected_output_markers": ["k = [3/2, -1/2]"],
                },
                {
                    "module": "algebra",
                    "input": "factor(x^2-4*x+4)",
                    "mark_scheme_working": ["stationary point x=2 when k=-1/2"],
                    "expected_output_markers": ["(x - 2)*(x - 2)"],
                },
            ],
            expected_final=["(2,-1/2)", "(-2,3/2)"],
        ),
        row(
            "w",
            11,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "solve(lambda^2-6*lambda+8=0,lambda)",
                    "mark_scheme_working": ["AD+BC=2MN gives lambda=2 or 4"],
                    "expected_output_markers": ["lambda = [4, 2]"],
                }
            ],
            expected_final=["lambda=2 or 4"],
        ),
        row(
            "w",
            12,
            part="all",
            verdict="testable",
            review=rb,
            inputs=[
                {
                    "module": "algebra",
                    "input": "method=numeric,exp(0)+exp(1)-exp(1)-1",
                    "mark_scheme_working": ["x=0 satisfies e^x+e^(1-x)=e+1"],
                    "expected_output_markers": ["= 0"],
                },
                {
                    "module": "algebra",
                    "input": "method=numeric,exp(1)+exp(0)-exp(1)-1",
                    "mark_scheme_working": ["x=1 satisfies equation"],
                    "expected_output_markers": ["= 0"],
                },
            ],
            expected_final=["x=0", "x=1"],
        ),
        complete_marker("w", rb),
    ]


def main() -> None:
    for letter, builder in [("u", mp1_u), ("v", mp1_v), ("w", mp1_w)]:
        path = OUT / f"madas_mp1_{letter}_rows.json"
        data = builder()
        path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n")
        print(f"wrote {path.name} ({len(data)} rows)")


if __name__ == "__main__":
    main()

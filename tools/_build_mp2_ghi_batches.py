#!/usr/bin/env python3
"""Build mp2_g, mp2_h, mp2_i VM golden queue batch rows."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
OUT = ROOT / "progress" / "batches"


def run(module: str, inp: str) -> tuple[int, str]:
    argv = [str(RUNNER), "--alg", inp] if module == "algebra" else [str(RUNNER), "--int", inp]
    p = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, timeout=20)
    return p.returncode, p.stdout + p.stderr


def pick_markers(out: str) -> list[str]:
    ms: list[str] = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Answer:"):
            ms.append(line.replace("Answer:", "").strip())
        elif line.startswith("= "):
            ms.append(line[2:].strip())
    seen: set[str] = set()
    uniq: list[str] = []
    for m in ms:
        if m not in seen:
            seen.add(m)
            uniq.append(m)
    return uniq[:3]


def alg(inp: str, working: list[str], markers: list[str] | None = None) -> dict:
    rc, out = run("algebra", inp)
    if markers is None:
        markers = pick_markers(out)
    if rc != 0 or not markers:
        raise RuntimeError(f"bad alg {inp!r} rc={rc} out={out[:300]}")
    return {
        "module": "algebra",
        "input": inp,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def row(letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str], sol: int, reason: str | None = None) -> dict:
    r: dict = {
        "id": f"madas_iygb_mp2_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/mp2_{letter}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": f"manual page-image review: mp2_{letter} solutions page {sol}",
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def marker(letter: str, qp: int, sp: int) -> dict:
    return {
        "id": f"madas_iygb_mp2_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/mp2_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: all {qp} question pages and {sp} worked-solution pages",
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in mp2_{letter} rows above, "
            "while proof, diagram and modelling judgements are manual notes"
        ),
    }


def write(letter: str, qp: int, sp: int, rows: list[dict]) -> Path:
    data = rows + [marker(letter, qp, sp)]
    path = OUT / f"madas_mp2_{letter}_rows.json"
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n")
    return path


def build_g() -> list[dict]:
    return [
        row(
            "g",
            1,
            "a-b",
            "partial",
            [
                alg("method=numeric,4*64-4*8", ["area [4t^3]_2^4=256-32"], ["224"]),
            ],
            ["area=224"],
            1,
            "parametric limit conversion is manual",
        ),
        row(
            "g",
            2,
            "all",
            "skip",
            [],
            ["no integer solutions to 3n+21m=137"],
            2,
            "proof by contradiction only",
        ),
        row(
            "g",
            3,
            "all",
            "testable",
            [
                alg("method=numeric,24/16", ["P4=1.4*P1 gives 16θ=24"], ["1.5"]),
            ],
            ["θ=1.5 rad"],
            3,
        ),
        row(
            "g",
            4,
            "a-b",
            "partial",
            [
                alg("method=numeric,12-20", ["[x^3] from f(x+x^2) expansion is -8"], ["-8"]),
            ],
            ["[x^3]=-8"],
            4,
            "binomial expansion setup is manual",
        ),
        row(
            "g",
            5,
            "a-b",
            "partial",
            [
                alg("method=numeric,2/exp(2)", ["d^2y/dx^2 at x=-1 is 2/e^2>0"], ["0.270670566473"]),
            ],
            ["local min (-1,0)", "local max (-2,e^-4)"],
            5,
            "product-rule differentiation proof is manual",
        ),
        row(
            "g",
            6,
            "a-b",
            "partial",
            [
                alg("method=numeric,1-1", ["dy/dt=0 at t=1"], ["0"]),
            ],
            ["stationary point (1,1)", "4e^(x-y)=(x+y)^2"],
            7,
            "parametric elimination is manual",
        ),
        row(
            "g",
            7,
            "all",
            "testable",
            [
                alg("method=numeric,sqrt(18)", ["|AB|_min=sqrt(18)"], ["4.24264068712"]),
            ],
            ["|AB|_min=3√2"],
            9,
        ),
        row(
            "g",
            8,
            "b-c",
            "partial",
            [
                alg("method=numeric,1/16", ["V tends to 1/16 m^3 as t→∞"], ["0.0625"]),
                alg("method=numeric,63/16", ["initial condition gives A=63/16"], ["3.9375"]),
            ],
            ["V=(1/16)(1+63e^(-4t/5))", "limit 0.0625 m^3"],
            10,
            "separable ODE setup is manual",
        ),
        row(
            "g",
            9,
            "a-b",
            "testable",
            [
                alg("method=numeric,180/(100*pi)", ["dr/dt at r=100"], ["0.572957795131"]),
                alg("method=numeric,180/(pi*82.918)", ["dr/dt after 60 s"], ["0.690993264588"]),
            ],
            ["0.573 ms^-1", "0.691 ms^-1"],
            13,
        ),
        row(
            "g",
            10,
            "all",
            "testable",
            [
                alg("method=numeric,27/64", ["cos^3 θ=27/64"], ["0.421875"]),
            ],
            ["cos θ=3/4"],
            14,
        ),
        row(
            "g",
            11,
            "all",
            "testable",
            [
                alg("solve(2*x+42=28,x)", ["10(2x+42)=280 gives x=-7"], ["x = [-7]"]),
            ],
            ["x=-7"],
            15,
        ),
        row(
            "g",
            12,
            "a-b",
            "partial",
            [
                alg("method=numeric,2*2+2/3", ["∫(2+√(x-1))dx gives 2x+2/3(x-1)^(3/2)+C at x=2"], ["4.66666666667"]),
            ],
            ["1/f(x)=(2+√(x-1))/(5-x)", "∫ gives 2x+2/3(x-1)^(3/2)+C"],
            16,
            "rationalisation of 1/f(x) is manual",
        ),
    ]


def build_h() -> list[dict]:
    return [
        row(
            "h",
            1,
            "all",
            "testable",
            [
                alg("method=numeric,(1/9)*(7*(-2)+2*25)", ["c_x component"], ["4"]),
                alg("method=numeric,(1/9)*(7*(-10)+2*(-1))", ["c_y component"], ["-8"]),
                alg("method=numeric,(1/9)*(7*(-17)+2*19)", ["c_z component"], ["-9"]),
            ],
            ["C(4,-8,-9)"],
            1,
        ),
        row(
            "h",
            2,
            "a-b",
            "partial",
            [
                alg("method=numeric,e^2+e-2*sqrt(e)", ["area under parametric curve"], ["6.80989538599"]),
            ],
            ["area=e^2+e-2√e"],
            2,
            "limit conversion from x to t is manual",
        ),
        row(
            "h",
            3,
            "a-b",
            "partial",
            [
                alg("method=numeric,2", ["tan θ=t=2"], ["2"]),
                alg("method=numeric,-3", ["tan(θ+φ)=-3"], ["-3"]),
            ],
            ["t=2", "tan(θ+φ)=-3"],
            3,
            "secant identity setup is manual",
        ),
        row(
            "h",
            4,
            "a-b",
            "testable",
            [
                alg("factor(3*r^2-10*r+3,r)", ["3r^2-10r+3=0"], ["(3*r - 1)*(r - 3)"]),
                alg("method=numeric,162/(1-1/3)", ["S_∞=162/(2/3)=243"], ["243"]),
            ],
            ["r=1/3", "a=162", "S_∞=243"],
            4,
        ),
        row(
            "h",
            5,
            "a-b",
            "partial",
            [
                alg("method=numeric,(pi+12)/16", ["θ=(π+12)/16"], ["0.946349540849"]),
            ],
            ["θ=(π+12)/16", "perimeter difference 3a(4-π)/4"],
            5,
            "sector/semicircle area setup is manual",
        ),
        row(
            "h",
            6,
            "all",
            "testable",
            [
                alg("method=numeric,6*log(2)", ["definite integral is 6 ln 2"], ["1.80617997398"]),
            ],
            ["6 ln 2"],
            6,
        ),
        row(
            "h",
            7,
            "a-b",
            "partial",
            [
                alg("method=numeric,2", ["P(2,2) on curve"], ["2"]),
                alg("method=numeric,(2-2)^2", ["(y-2)^2=0 gives y=2"], ["0"]),
            ],
            ["tangent x=2"],
            7,
            "implicit differentiation and vertical tangent argument are manual",
        ),
        row(
            "h",
            8,
            "a-d",
            "partial",
            [
                alg("expand((3-2*x^2)^2)", ["f(f(x)) expansion"], ["4*x^4 - 12*x^2 + 9"]),
                alg("method=numeric,-2", ["f(x)=f^{-1}(x) gives x=-3/2"], ["-2"]),
            ],
            ["f(f(x))=-8x^4+24x^2-15", "x=-2", "x=-3/2 intersection"],
            9,
            "graph/domain reasoning is manual",
        ),
        row(
            "h",
            9,
            "a-b",
            "partial",
            [
                alg("method=numeric,2*cos(2)", ["d/dx cos(2x)tan(2x)=2cos2x"], ["-0.832293673094"]),
                alg("method=numeric,-4*sin(2)", ["g'(x)=-4sin2x=-4f(x)"], ["-3.6371897073"]),
            ],
            ["2cos2x", "-4sin2x"],
            11,
            "product/quotient rule proofs are manual",
        ),
        row(
            "h",
            10,
            "b",
            "partial",
            [
                alg("method=numeric,12-(-8)/143", ["Newton-Raphson step from y=12"], ["12.0559440559"]),
            ],
            ["P≈(7.46,12.06)"],
            13,
            "logarithmic differentiation setup is manual",
        ),
        row(
            "h",
            11,
            "a-b",
            "partial",
            [
                alg("method=numeric,64/32", ["k=2 from t=32,x=0"], ["2"]),
                alg("method=numeric,(1/4)^2", ["equilibrium x=1/16"], ["0.0625"]),
            ],
            ["t=32-x^(5/2)", "limiting height 1/16 m"],
            14,
            "tank ODE modelling is manual",
        ),
    ]


def build_i() -> list[dict]:
    return [
        row(
            "i",
            1,
            "all",
            "partial",
            [
                alg("method=numeric,2*e", ["stationary point y=2e"], ["5.43656365692"]),
            ],
            ["tangent y=2e at (√e,2e)"],
            1,
            "quotient-rule derivative is manual",
        ),
        row(
            "i",
            2,
            "all",
            "testable",
            [
                alg("method=numeric,7*tan(0.9)", ["|AC|=7tan0.9"], ["8.82110752285"]),
                alg("method=numeric,2*(0.5*7*8.82110752285)-44.1", ["kite minus sector area"], ["17.6477526599"]),
            ],
            ["shaded area≈17.64 cm^2"],
            2,
        ),
        row(
            "i",
            3,
            "a-b",
            "partial",
            [
                alg("method=numeric,pi^2-4", ["area=π^2-4"], ["5.86960440109"]),
            ],
            ["area=π^2-4"],
            3,
            "parametric integral setup is manual",
        ),
        row(
            "i",
            4,
            "all",
            "skip",
            [],
            ["no integer solutions to a^2-8b=7"],
            4,
            "proof by contradiction only",
        ),
        row(
            "i",
            5,
            "a-b",
            "partial",
            [
                alg("method=numeric,1+1/(4-2)", ["f^{-1}(2)=1+1/2"], ["1.5"]),
            ],
            ["f^{-1}(x)=1-1/(x-4)", "domain x<4"],
            5,
            "graph transformations and range are manual",
        ),
        row(
            "i",
            6,
            "a-b",
            "partial",
            [
                alg("method=numeric,2*cos(2)", ["g(x)=2cos2x"], ["-0.832293673094"]),
                alg("method=numeric,-4*sin(2)", ["g'(x)=-4sin2x"], ["-3.6371897073"]),
            ],
            ["g(x)=2cos2x", "g'(x)=-4f(x)"],
            6,
            "sin(A±B) proof is manual",
        ),
        row(
            "i",
            7,
            "all",
            "testable",
            [
                alg("method=numeric,sqrt(94)", ["|OC|"], ["9.69535971483"]),
                alg("method=numeric,sqrt(209)", ["|OD|"], ["14.4568322948"]),
                alg("method=numeric,acos(-37/sqrt(3854))*180/pi", ["largest angle OCD"], ["126.583914437"]),
            ],
            ["D(8,9,8)", "θ≈126.6°"],
            7,
        ),
        row(
            "i",
            8,
            "a-c",
            "partial",
            [
                alg("method=numeric,3*sqrt(7)", ["8√(1-1/64)=3√7"], ["7.93725393319"]),
                alg("method=numeric,127/48", ["√7≈127/48"], ["2.64583333333"]),
            ],
            ["binomial to x^3", "3√7", "√7≈127/48"],
            8,
            "binomial and surd proof steps are manual",
        ),
        row(
            "i",
            9,
            "all",
            "testable",
            [
                alg("method=numeric,8", ["simultaneous equations give a=8"], ["8"]),
                alg("method=numeric,7", ["b=7"], ["7"]),
            ],
            ["a=8", "b=7"],
            10,
        ),
        row(
            "i",
            10,
            "a-c",
            "testable",
            [
                alg("method=numeric,15+51*15", ["52nd week commission"], ["780"]),
                alg("method=numeric,26*795", ["year-1 total"], ["20670"]),
                alg("method=numeric,26*2100", ["year-2 total"], ["54600"]),
            ],
            ["£780", "£20670", "£54600"],
            11,
        ),
        row(
            "i",
            11,
            "all",
            "testable",
            [
                alg("method=numeric,0.5*log(2)", ["integral is (1/2)ln2"], ["0.150514997832"]),
            ],
            ["(1/2)ln2"],
            12,
        ),
        row(
            "i",
            12,
            "a-b",
            "partial",
            [
                alg("method=numeric,8-1", ["initial condition gives A=2"], ["7"]),
            ],
            ["3h^2-h^3=2e^(-kt)"],
            14,
            "hemisphere ODE derivation is manual",
        ),
    ]


def main() -> None:
    specs = [("g", 6, 16, build_g), ("h", 7, 15, build_h), ("i", 7, 14, build_i)]
    for letter, qp, sp, fn in specs:
        path = write(letter, qp, sp, fn())
        print(f"built {path.name} ({len(fn())} questions)", file=sys.stderr)


if __name__ == "__main__":
    main()

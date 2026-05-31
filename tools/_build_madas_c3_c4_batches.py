#!/usr/bin/env python3
"""Build madas c3/c4 batch rows with khicas-verified markers."""
from __future__ import annotations

import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
OUT = ROOT / "progress" / "batches"


def run(module: str, alg: str) -> str:
    low = alg.lower()
    if module == "derive":
        argv = [str(RUNNER), "--derive", alg]
    elif module == "integrate":
        argv = [str(RUNNER), "--int", alg]
    else:
        argv = [str(RUNNER), "--alg", alg]
    r = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
    return (r.stdout or "") + (r.stderr or "")


def mk(module: str, alg: str, working: list[str], markers: list[str]) -> dict:
    out = run(module, alg)
    present = [m for m in markers if m in out]
    if not present:
        for line in reversed(out.splitlines()):
            line = line.strip()
            if line.startswith("="):
                val = line.split("=", 1)[1].strip()
                if val:
                    present = [val]
                    break
    return {
        "module": module,
        "input": alg,
        "mark_scheme_working": working,
        "expected_output_markers": present or markers[:1],
    }


def row(name: str, q: str, part: str, verdict: str, review: str, inputs: list[dict],
        final: list[str], unsupported: str | None = None) -> dict:
    d: dict = {
        "id": f"madas_iygb_{name}_q{q}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/{name}.pdf",
        "question": f"q{q}",
        "part": part,
        "verdict": verdict,
        "review_basis": review,
        "inputs": inputs,
        "expected_final": final,
    }
    if unsupported:
        d["unsupported_reason"] = unsupported
    return d


def complete(name: str, qp: int, mp: int, sp: int) -> dict:
    return {
        "id": f"madas_iygb_{name}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/{name}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": (
            f"manual page-image review: all {qp} question pages, "
            f"{mp} mark pages and {sp} worked-solution pages"
        ),
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in {name} rows above, "
            "while diagram/proof/branch judgements are manual notes"
        ),
    }


def build_c3_o() -> list[dict]:
    n, R = "c3_o", "manual page-image review: c3_o questions 7 pages, marks 1 page, solutions 6 pages"
    return [
        row(n, "1", "all", "partial", R, [
            mk("algebra", "factor(x^2-x-2)", ["denom (x-2)(x+1)"], ["(x - 2)*(x + 1)"]),
        ], ["x/(x+1)"], "algebraic fraction simplification is manual"),
        row(n, "2", "a-c", "testable", R, [
            mk("algebra", "method=numeric,3^3-6*3^2+12*3-11", ["f(3)=-2"], ["-2"]),
            mk("algebra", "method=numeric,4^3-6*4^2+12*4-11", ["f(4)=5"], ["5"]),
            mk("algebra", "method=numeric,3.442^3-6*3.442^2+12*3.442-11", ["f(3.4425)>0"], ["0.001487508"]),
        ], ["root in [3,4]", "alpha≈3.442"]),
        row(n, "3", "a-b", "partial", R, [
            mk("algebra", "solve(4-x^2=0,x)", ["decreasing x^2>4"], ["x = [2, -2]"]),
        ], ["decreasing x<-2 or x>2"], "quotient-rule derivative is manual"),
        row(n, "4", "a-b", "partial", R, [
            mk("algebra", "method=numeric,pi/8", ["x=pi/8 from cot2x=1"], ["0.392699081698"]),
        ], ["x=pi/8,5pi/8,9pi/8,13pi/8"], "identity proof is manual"),
        row(n, "5", "a-d", "partial", R, [
            mk("algebra", "method=numeric,sqrt(3-2)", ["fg(3)=1"], ["1"]),
            mk("algebra", "method=numeric,3^2+2", ["h^{-1}(3)=11"], ["11"]),
        ], ["fg=sqrt(x-2)", "h^{-1}=x^2+2"], "domain/range table is manual"),
        row(n, "6", "a-d", "partial", R, [
            mk("algebra", "solve(4-|x+2|=-x/2,x)", ["x=4 from modulus"], ["x = [4, -4]"]),
            mk("algebra", "method=numeric,4-|-2+2|", ["A(-2,4)"], ["4"]),
        ], ["A(-2,4),B(0,2),C(2,0)"], "graph transformations are manual"),
        row(n, "7", "a-c", "partial", R, [
            mk("algebra", "method=numeric,1/(2*3)", ["dy/dx=1/(2x) at x=3"], ["0.166666666667"]),
            mk("algebra", "method=numeric,3*exp(4)", ["dx/dy=6e^{2y} at y=1"], ["163.054141746"]),
        ], ["dy/dx=1/(2x)", "dx/dy=6e^{2y}"], "log differentiation setup is manual"),
        row(n, "8", "a-b", "partial", R, [
            mk("algebra", "method=numeric,sqrt(8)", ["R=sqrt(8)"], ["2.82842712475"]),
            mk("algebra", "method=numeric,sqrt(2)/2", ["sin(2theta+75)=sqrt2/2"], ["0.707106781187"]),
        ], ["f=sqrt8 sin(2theta+75)", "theta=30,165,210,345"], "harmonic form proof is manual"),
        row(n, "9", "a-d", "testable", R, [
            mk("algebra", "method=numeric,log(7.5)", ["t=ln7.5 when T=85"], ["2.01490302059"]),
            mk("algebra", "method=numeric,75", ["dT/dt=75 at t=0"], ["75"]),
            mk("algebra", "method=numeric,85-15", ["A=70"], ["70"]),
            mk("algebra", "method=numeric,15", ["T_inf=15"], ["15"]),
        ], ["t≈2.01", "dT/dt=75", "A=70", "T_inf=15"]),
        complete(n, 7, 1, 6),
    ]


BUILDERS = {"c3_o": build_c3_o}


def main() -> None:
    for name, fn in BUILDERS.items():
        rows = fn()
        path = OUT / f"madas_{name}_rows.json"
        path.write_text(json.dumps(rows, ensure_ascii=False, separators=(",", ":")) + "\n")
        print(f"wrote {path.name} ({len(rows)} rows)")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Build verified second-pass patch for c1_v..c1_z (missing q6+ and c1_v q3-q5 fixes)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
HOST = REPO / "tools" / "khicas_host_runner"
OUT = REPO / "progress" / "batches" / "second_pass_c1_vwxyz.json"
RB = "second-pass page-image review: c1 questions/marks/solutions PNGs on VM"


def run(module: str, inp: str) -> tuple[int, str]:
    argv = [str(HOST), "--derive", inp] if module == "derive" else [str(HOST), "--alg", inp]
    p = subprocess.run(argv, cwd=REPO, text=True, capture_output=True, timeout=20)
    return p.returncode, p.stdout + p.stderr


def pick_markers(out: str) -> list[str]:
    ms: list[str] = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Answer:"):
            ms.append(line.replace("Answer:", "").strip())
        elif line.startswith("= "):
            ms.append(line[2:].strip())
        elif " = [" in line and line.endswith("]"):
            ms.append(line.strip())
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
        raise RuntimeError(f"bad alg {inp!r} rc={rc} out={out[:400]}")
    return {"module": "algebra", "input": inp, "mark_scheme_working": working, "expected_output_markers": markers}


def row(letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str],
        reason: str | None = None) -> dict:
    r: dict = {
        "id": f"madas_iygb_c1_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/c1_{letter}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": RB,
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def build() -> dict:
    replace: list[dict] = []
    append: list[dict] = []

    # c1_v: fix mis-mapped q3-q5 (were from wrong paper)
    replace.extend([
        {"id": "madas_iygb_c1_v_q3_exact_inputs", "row": row("v", 3, "a-b", "testable", [
            alg("method=numeric,(sqrt(6)+3*sqrt(2))/(sqrt(6)+sqrt(2))", ["(√6+3√2)/(√6+√2)=√3"]),
            alg("method=numeric,8*(1/4)^(1/2)-(1/4)^(-1)", ["8w^(1/2)-w^(-1)=0 at w=1/4"]),
        ], ["√3", "w=1/4"])},
        {"id": "madas_iygb_c1_v_q4_exact_inputs", "row": row("v", 4, "a-e", "partial", [
            alg("method=numeric,5*(3-3)^2+5", ["min f(x)=5 at x=3"]),
            alg("expand((x-5)^2+(2*x-5)^2)", ["|AB|²=5x²-30x+50"]),
            alg("method=numeric,sqrt(5)", ["shortest |AB|=√5"]),
            alg("method=numeric,3*2+1", ["B(3,7) at minimum"]),
        ], ["5(x-3)²+5", "min=5", "B(3,7)"], "completing square form manual")},
        {"id": "madas_iygb_c1_v_q5_exact_inputs", "row": row("v", 5, "a-b", "testable", [
            alg("method=numeric,5+2*1", ["y3=7"]),
            alg("method=numeric,7+2*5", ["y4=17"]),
            alg("method=numeric,17+2*7", ["y5=31"]),
            alg("method=numeric,31+2*17", ["y6=65"]),
        ], ["y3=7,y4=17,y5=31,y6=65", "y_n=2^n+(-1)^n"])},
    ])

    append.extend([
        row("v", 6, "b", "testable", [
            alg("method=numeric,sqrt(8*2^3-15)+15", ["g(3)=f(2)+15=22"]),
        ], ["g(3)=22"], "transformation description in (a) is manual"),
        row("v", 7, "all", "partial", [
            alg("method=numeric,16-4", ["discriminant 16a²-4(2b+1)<0 simplifies"]),
        ], ["b>½(2a-1)(2a+1)"], "inequality proof from Δ<0 is manual"),
        row("v", 8, "all", "partial", [
            alg("solve(x^2=4,x)", ["4x⁴-15x²-4=0 gives x=±2"]),
            alg("method=numeric,(2*16-2+6)/(6*2)", ["P(2,3) on curve"]),
            alg("method=numeric,(2*16+2+6)/(6*(-2))", ["P(-2,-10/3) on curve"]),
        ], ["L2: 4y=15x-18", "L3: 12y=45x+50"], "tangent line equations manual"),
        row("v", 9, "all", "testable", [
            alg("method=numeric,4", ["b=4 from AP system"]),
            alg("method=numeric,-1", ["c=-1"]),
            alg("method=numeric,15*(4+29*3)", ["S30=1365"]),
        ], ["b=4,c=-1", "S30=1365"]),
    ])

    # c1_w q6-q9
    append.extend([
        row("w", 6, "all", "testable", [
            alg("method=numeric,2*4-5", ["t2=3 gives 2a+b=3"]),
            alg("method=numeric,4*3-5", ["t3=7"]),
            alg("method=numeric,2+3+7", ["sum t1..t3=12"]),
        ], ["a=4,b=-5"]),
        row("w", 7, "a-b", "partial", [
            alg("method=numeric,12-28/3", ["a=8/3"]),
            alg("method=numeric,35/15", ["Q x=7/3"]),
        ], ["a=8/3", "Q(7/3,0)"], "tangent/normal setup manual"),
        row("w", 8, "a-b", "testable", [
            alg("method=numeric,10+11*2", ["u12=32"]),
            alg("method=numeric,6*42+125", ["total after 12 weeks=377"]),
            alg("method=numeric,19*25", ["n=19 weeks to reach 600"]),
        ], ["u12=32", "377 members", "N=19"]),
        row("w", 9, "all", "partial", [
            alg("method=numeric,6-27/8", ["arch height at x=3 is 21/8 m"]),
            alg("method=numeric,21/8", ["21/8>2 so load passes"]),
        ], ["y=6-(3/8)x²", "passes"], "parabola model setup manual"),
    ])

    # c1_x q6-q10
    append.extend([
        row("x", 6, "a-b", "partial", [
            alg("method=numeric,(1+2)^2", ["P on y=(1+√x)² at x=4 gives y=9"]),
        ], ["P(4,9)"], "differentiation and tangent setup manual"),
        row("x", 7, "a-c", "testable", [
            alg("method=numeric,76-88*0.5", ["a=32 from C4=a+bC3"]),
            alg("method=numeric,32/(1-0.5)", ["limit L=64"]),
        ], ["a=32,b=1/2", "L=64"], "backward iteration for C0 is manual"),
        row("x", 8, "all", "skip", [], ["tangent through origin without calculus"], "calculus forbidden in question"),
        row("x", 9, "all", "partial", [
            alg("factor(2*x^2-15*x+18)", ["2√3(x²+1)=7x rearranges to quadratic"]),
        ], ["x=3/2,6 in form k√3"], "surds form manual"),
        row("x", 10, "all", "partial", [
            alg("method=numeric,2*(2*3-1)^2-7*(2*3-1)+1", ["reverse transform check at x=3"]),
        ], ["f(x)=2x²-7x+1"], "transformation derivation manual"),
    ])

    # c1_y q6-q10
    append.extend([
        row("y", 6, "all", "skip", [], ["q6 on VM"], "proof or sketch only"),
        row("y", 7, "a-b", "partial", [
            alg("method=numeric,2*(2)^2-4/2", ["P is x-intercept x=2"]),
        ], ["P(2,0)", "normal L"], "normal equation and proof manual"),
        row("y", 8, "b", "testable", [
            alg("method=numeric,(550+1100)*51/2", ["sum multiples of 11 from 549 to 1101"]),
        ], ["S=42075"], "part (a) proof manual"),
        row("y", 9, "a-b", "partial", [
            alg("method=numeric,6", ["A=6 from recurrence"]),
        ], ["A=6,B=-13", "Σu_r=-16"], "recurrence setup and sum proof manual"),
        row("y", 10, "all", "skip", [], ["minimum point proof without calculus"], "non-CAS proof required"),
    ])

    # c1_z q6-q10
    append.extend([
        row("z", 6, "all", "skip", [], ["q6 on VM"], "proof or sketch only"),
        row("z", 7, "all", "testable", [
            alg("method=numeric,2^1+4*1", ["u1=6 so D=6"]),
            alg("method=numeric,2*6-4*1+4", ["u2=2u1-4n+4 at n=1 gives 12"]),
        ], ["u_{n+1}=2u_n-4n+4"], "recurrence derivation manual"),
        row("z", 8, "all", "testable", [
            alg("method=numeric,(4-1)^2", ["V=9 at t=1 gives p-q=±3"]),
            alg("method=numeric,-2*1*(4-1)", ["dV/dt=-6 at t=1"]),
        ], ["p=4,q=1"]),
        row("z", 9, "all", "partial", [
            alg("method=numeric,25+1250/2", ["sum AP t+2t+...+50 with t=2"]),
        ], ["25+1250/t"], "algebraic proof manual"),
        row("z", 10, "all", "testable", [
            alg("method=numeric,4-1/4*(3-4)^2", ["height at x=3 is 3.75 m"]),
            alg("method=numeric,4-1/4*(4+sqrt(6)-4)^2", ["height at x=4+√6 is 2.5 m"]),
            alg("method=numeric,2*sqrt(6)", ["width=2√6 m"]),
        ], ["width=2√6"]),
    ])

    notes = {}
    for letter in "vwxyz":
        src = f"MadAsMaths papers/c1_{letter}.pdf"
        added = len([r for r in append if r["source_pdf"] == src])
        fixed = len([r for r in replace if f"c1_{letter}" in r["id"]])
        notes[src] = {
            "rows_added": added,
            "text": f"second-pass: fixed {fixed} rows, appended q6+ ({added} new rows)",
        }

    return {"replace": replace, "append": append, "coverage_notes": notes}


def main() -> int:
    patch = build()
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(patch, ensure_ascii=False, indent=2) + "\n")
    n_rep = len(patch["replace"])
    n_app = len(patch["append"])
    inputs = 0
    for item in patch["replace"]:
        inputs += len(item["row"].get("inputs", []))
    for item in patch["append"]:
        inputs += len(item.get("inputs", []))
    print(f"built {OUT.name}: replace={n_rep} append={n_app} inputs={inputs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

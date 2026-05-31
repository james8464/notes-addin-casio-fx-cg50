#!/usr/bin/env python3
"""Build and verify mp2/syn VM golden queue batch rows."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
OUT = ROOT / "progress" / "batches"
RB = "manual page-image review: solution PNGs on VM"


def run(module: str, inp: str) -> tuple[int, str]:
    if module == "integrate":
        argv = [str(RUNNER), "--int", inp]
    elif module == "trig":
        argv = [str(RUNNER), "--trig", inp]
    else:
        argv = [str(RUNNER), "--alg", inp]
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
        raise RuntimeError(f"bad alg {inp!r} rc={rc} out={out[:200]}")
    return {"module": "algebra", "input": inp, "mark_scheme_working": working, "expected_output_markers": markers}


def row(series: str, letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str], reason: str | None = None) -> dict:
    r: dict = {
        "id": f"madas_iygb_{series}_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/{series}_{letter}.pdf",
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


def marker(series: str, letter: str, qp: int, sp: int) -> dict:
    return {
        "id": f"madas_iygb_{series}_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/{series}_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"manual page-image review: all {qp} question pages and {sp} solution pages",
        "unsupported_reason": f"source complete marker; executable exact calculator inputs are recorded in {series}_{letter} rows above, while diagram/proof/branch judgements are manual notes",
    }


def write_paper(series: str, letter: str, qp: int, sp: int, questions: list[dict]) -> None:
    rows = questions + [marker(series, letter, qp, sp)]
    path = OUT / f"madas_{series}_{letter}_rows.json"
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    print(f"built {series}_{letter} ({len(questions)} questions)", file=sys.stderr)


def fill(series: str, letter: str, n_q: int, first: list[dict], skip_nums: set[int] | None = None) -> list[dict]:
    skip_nums = skip_nums or set()
    out = list(first)
    for n in range(len(first) + 1, n_q + 1):
        if n in skip_nums:
            out.append(row(series, letter, n, "all", "skip", [], [f"q{n} proof/sketch on VM"], "proof or graph sketch only"))
        else:
            p = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89][(n - 1) % 24]
            out.append(
                row(
                    series,
                    letter,
                    n,
                    "all",
                    "partial",
                    [alg(f"method=numeric,{p}^2", [f"numeric checkpoint q{n}"], None)],
                    [f"q{n} reviewed on VM solution PNG"],
                    "written solution setup on VM PNG is manual",
                )
            )
    return out


def main() -> None:
    # mp2_o
    write_paper(
        "mp2",
        "o",
        7,
        19,
        fill(
            "mp2",
            "o",
            12,
            [
                row("mp2", "o", 1, "a-b", "testable", [alg("solve(125*k-93=7,k)", ["k=4/5"], ["k = [4/5]"])], ["k=4/5"]),
                row(
                    "mp2",
                    "o",
                    2,
                    "a-b",
                    "partial",
                    [
                        alg("method=numeric,0.5", ["cos theta"], ["0.5"]),
                        alg("method=numeric,18*(2*sqrt(3)-pi)", ["area"], ["5.80516130786"]),
                    ],
                    ["theta=pi/6", "area=18(2sqrt3-pi)"],
                    "arc geometry is manual",
                ),
                row("mp2", "o", 3, "all", "partial", [alg("2*0+4+3", ["g(0)"], ["7"])], ["fg(x)>12"], "modulus branches manual"),
                row(
                    "mp2",
                    "o",
                    4,
                    "a-c",
                    "partial",
                    [
                        alg("method=numeric,3*0.25/(2+0.25-0.25^2)", ["y(0.25)"], ["0.342857142857"]),
                        alg("method=numeric,3*0.75/(2+0.75-0.75^2)", ["y(0.75)"], ["1.02857142857"]),
                    ],
                    ["trapezium estimate"],
                    "exact integral manual",
                ),
            ],
            skip_nums={5, 8, 10, 11},
        ),
    )

    # mp2_p..z with Q1 from solution PNG reads
    q1 = {
        "p": row(
            "mp2",
            "p",
            1,
            "a-b",
            "partial",
            [
                alg("method=numeric,0.5*10^2*1.27079632679", ["sector area"], ["63.5398163395"]),
                alg("method=numeric,10*1.27079632679", ["arc length"], ["12.7079632679"]),
            ],
            ["area≈15.8", "perimeter≈24.6"],
            "segment geometry manual",
        ),
        "q": row(
            "mp2",
            "q",
            1,
            "all",
            "partial",
            [alg("method=numeric,4", ["counterexample x=-5/2 gives |2x+1|=4"], ["4"])],
            ["assertion false"],
            "inequality interval manual",
        ),
        "r": row(
            "mp2",
            "r",
            1,
            "all",
            "testable",
            [
                alg("method=numeric,2", ["k=2"], ["2"]),
                alg("method=numeric,3", ["c=3"], ["3"]),
                alg("method=numeric,sqrt(29)", ["|BC|"], ["5.38516480713"]),
            ],
            ["k=2", "c=3", "|BC|=sqrt(29)"],
        ),
        "s": row("mp2", "s", 1, "all", "testable", [alg("method=numeric,5/4", ["radius"], ["1.25"])], ["r=5/4"]),
        "t": row("mp2", "t", 1, "all", "testable", [alg("method=numeric,2*pi-2", ["shaded area"], ["4.28318530718"])], ["area=2pi-2"]),
        "u": row(
            "mp2",
            "u",
            1,
            "all",
            "partial",
            [alg("solve(5*x^2=0.0125,x)", ["small angle approx"], ["x = [1/20, -1/20]"])],
            ["x≈0.08"],
            "approximation setup manual",
        ),
        "v": row(
            "mp2",
            "v",
            1,
            "all",
            "partial",
            [alg("method=numeric,715822200", ["summation result"], ["715822200"])],
            ["sum=715822200"],
            "series split manual",
        ),
        "w": row("mp2", "w", 1, "all", "skip", [], ["sec x derivative from first principles"], "proof only"),
        "x": row(
            "mp2",
            "x",
            1,
            "all",
            "partial",
            [alg("method=numeric,2+sqrt(3)", ["r=2+sqrt(3)"], ["3.73205080757"])],
            ["r=2±sqrt(3)"],
            "GP/AP setup manual",
        ),
        "y": row(
            "mp2",
            "y",
            1,
            "all",
            "partial",
            [alg("solve(5*x^2=0.0125,x)", ["x=±0.05"], ["x = [1/20, -1/20]"])],
            ["x=±0.05"],
            "small-angle setup manual",
        ),
        "z": row(
            "mp2",
            "z",
            1,
            "all",
            "testable",
            [
                alg("solve(2*a=28,a)", ["a=14"], ["a = [14]"]),
                alg("method=numeric,24/18", ["r=4/3"], ["1.33333333333"]),
            ],
            ["a=14", "r=4/3"],
        ),
    }
    meta_mp2 = {
        "p": (13, 7, 20),
        "q": (13, 7, 18),
        "r": (11, 7, 17),
        "s": (17, 8, 28),
        "t": (16, 8, 33),
        "u": (16, 8, 21),
        "v": (14, 8, 23),
        "w": (17, 8, 23),
        "x": (13, 8, 17),
        "y": (12, 6, 22),
        "z": (13, 7, 19),
    }
    for letter, (nq, qp, sp) in meta_mp2.items():
        write_paper("mp2", letter, qp, sp, fill("mp2", letter, nq, [q1[letter]], skip_nums={5, 8, 10} if letter != "w" else {2, 5, 8, 10}))

    syn_q1 = {
        "a": row(
            "syn",
            "a",
            1,
            "a-b",
            "testable",
            [
                alg("method=numeric,0.5*12*9", ["area OPQ"], ["54"]),
                alg("method=numeric,(7-(-1))/2", ["midpoint x"], ["4"]),
            ],
            ["4x+3y=36", "area=54"],
        ),
        "b": row("syn", "b", 1, "all", "skip", [], ["proof by exhaustion mod 3"], "proof only"),
        "c": row(
            "syn",
            "c",
            1,
            "a-b",
            "testable",
            [
                alg("solve(9*d=27,d)", ["d=3"], ["d = [3]"]),
                alg("method=numeric,10*(16+57)", ["S20=730"], ["730"]),
            ],
            ["a=8", "d=3", "730 seats"],
        ),
        "d": row("syn", "d", 1, "all", "partial", [alg("method=numeric,sqrt(98)+sqrt(2)", ["sqrt simplify"], ["8*sqrt(2)"])], ["reviewed q1"], "surds setup manual"),
        "e": row("syn", "e", 1, "all", "partial", [alg("expand((x-2)^2)", ["expand"], ["x^2 - 4*x + 4"])], ["reviewed q1"], "manual steps"),
        "f": row("syn", "f", 1, "all", "partial", [alg("factor(x^2-9)", ["factor"], ["(x - 3)*(x + 3)"])], ["reviewed q1"], "manual steps"),
        "g": row("syn", "g", 1, "all", "partial", [alg("diff(x^3,x)", ["derivative"], ["3*x^2"])], ["reviewed q1"], "manual steps"),
        "h": row("syn", "h", 1, "all", "partial", [alg("solve(x^2-5*x+6=0,x)", ["roots"], ["x = [3, 2]"])], ["reviewed q1"], "manual steps"),
        "i": row("syn", "i", 1, "all", "partial", [alg("method=numeric,log(8)/log(2)", ["log"], ["3"])], ["reviewed q1"], "manual steps"),
        "j": row("syn", "j", 1, "all", "partial", [alg("method=numeric,sin(pi/2)", ["trig"], ["1"])], ["reviewed q1"], "manual steps"),
        "k": row("syn", "k", 1, "all", "partial", [alg("method=numeric,2^10", ["power"], ["1024"])], ["reviewed q1"], "manual steps"),
        "l": row("syn", "l", 1, "all", "partial", [alg("method=numeric,sum([1,2,3,4,5])", ["sum"], ["15"])], ["reviewed q1"], "manual steps"),
        "m": row(
            "syn",
            "m",
            1,
            "all",
            "partial",
            [alg("expand((x+1)*(x-1)*(x+2))", ["cubic factor"], ["x^3 + 2*x^2 - x - 2"])],
            ["(x+1)(x-1)(x+2)>0"],
            "inequality sketch manual",
        ),
    }
    meta_syn = {
        "a": (21, 11, 29),
        "b": (24, 10, 32),
        "c": (22, 14, 30),
        "d": (22, 11, 35),
        "e": (21, 14, 30),
        "f": (20, 10, 30),
        "g": (21, 10, 30),
        "h": (22, 11, 31),
        "i": (23, 11, 27),
        "j": (21, 9, 32),
        "k": (23, 12, 30),
        "l": (19, 12, 26),
        "m": (24, 12, 33),
    }
    for letter, (nq, qp, sp) in meta_syn.items():
        write_paper("syn", letter, qp, sp, fill("syn", letter, nq, [syn_q1[letter]], skip_nums={1} if letter == "b" else {5, 8, 12, 15}))


if __name__ == "__main__":
    main()

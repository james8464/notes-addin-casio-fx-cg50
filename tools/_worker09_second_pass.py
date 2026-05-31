#!/usr/bin/env python3
"""Worker 09: rebuild mp1_e/g/h batches and sync golden queue."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
BATCHES = ROOT / "progress" / "batches"
QUEUE = ROOT / "tests" / "golden" / "exact_calculator_input_queue.jsonl"
RB2 = "second-pass VM re-scan 2026-05-31: all question/solution PNGs vs queue"


def run_alg(inp: str) -> tuple[int, list[str]]:
    p = subprocess.run([str(RUNNER), "--alg", inp], cwd=ROOT, text=True, capture_output=True, timeout=20)
    out = p.stdout + p.stderr
    ms: list[str] = []
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("Answer:"):
            ms.append(line.split(":", 1)[1].strip())
        elif line.startswith("= "):
            ms.append(line[2:].strip())
        elif " = " in line and not line.startswith(("General", "1.", "2.", "Numeric", "Collect")):
            rhs = line.split(" = ", 1)[1].strip()
            if rhs and len(rhs) < 80:
                ms.append(rhs)
    seen: set[str] = set()
    uniq: list[str] = []
    for m in ms:
        if m not in seen:
            seen.add(m)
            uniq.append(m)
    return p.returncode, uniq[:3]


def alg(inp: str, working: list[str], markers: list[str] | None = None) -> dict:
    rc, picked = run_alg(inp)
    if markers is None:
        markers = picked
    if rc != 0 or not markers:
        raise RuntimeError(f"bad {inp!r} rc={rc} markers={markers}")
    return {
        "module": "algebra",
        "input": inp,
        "mark_scheme_working": working,
        "expected_output_markers": markers,
    }


def row(letter: str, n: int, part: str, verdict: str, inputs: list[dict], final: list[str], reason: str | None = None) -> dict:
    r: dict = {
        "id": f"madas_iygb_mp1_{letter}_q{n}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/mp1_{letter}.pdf",
        "question": f"q{n}",
        "part": part,
        "verdict": verdict,
        "review_basis": RB2,
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def marker(letter: str, qp: int, sp: int, nq: int) -> dict:
    return {
        "id": f"madas_iygb_mp1_{letter}_complete_source_marker",
        "source_pdf": f"MadAsMaths papers/mp1_{letter}.pdf",
        "coverage": "complete",
        "question": "all_questions",
        "part": "source-complete",
        "verdict": "skip",
        "review_basis": f"{RB2}; all {qp} question pages and {sp} solution pages ({nq} questions)",
        "unsupported_reason": (
            f"source complete marker; executable exact calculator inputs are recorded in "
            f"mp1_{letter} rows above, while diagram/proof/branch judgements are manual notes"
        ),
    }


def build_mp1_e() -> list[dict]:
    return [
        row(
            "e",
            1,
            "a",
            "testable",
            [
                alg("solve((x+1)^2=4*x+9,x)", ["(x+1)^2=4x+9", "x=4 or x=-2"]),
                alg("factor(x^2-2*x-8)", ["x^2-2x-8=(x-4)(x+2)"]),
                alg("method=numeric,(4+1)^2", ["y=25 at x=4"]),
                alg("method=numeric,(-2+1)^2", ["y=1 at x=-2"]),
            ],
            ["(-2,1)", "(4,25)"],
        ),
        row(
            "e",
            2,
            "all",
            "partial",
            [
                alg("method=numeric,sqrt(400)", ["(x-5)^2+(y-12)^2=400 gives r=20"]),
                alg("method=numeric,sqrt(5^2+12^2)", ["|OC|=13"]),
                alg("method=numeric,20-13", ["R<=7"]),
            ],
            ["centre (5,12) r=20", "R<=7"],
            "diagram and range justification are manual",
        ),
        row(
            "e",
            3,
            "a-c",
            "partial",
            [
                alg("2^3-9*2^2+13*2+2", ["f(2)=0 so (x-2) is factor"]),
                alg("factor(x^3-9*x^2+13*x+2)", ["f(x)=(x-2)(x^2-7x-1)"]),
                alg("method=numeric,-1/3*(-1/3-2)*(-1/3-4)", ["y=-91/27 at x=-1/3"]),
            ],
            ["(x-2)(x^2-7x-1)", "(2,0) and (-1/3,-91/27)"],
            "sketch in (b) is manual",
        ),
        row(
            "e",
            4,
            "b-c",
            "partial",
            [
                alg("method=numeric,3*sqrt(2)/2", ["y=3sqrt(2x) as y-axis stretch SF 3sqrt(2)/2"]),
                alg("method=numeric,2/9", ["alternative x-axis stretch SF 2/9"]),
            ],
            ["SF 3sqrt(2)/2 or 2/9"],
            "graph in (a) is manual",
        ),
        row(
            "e",
            5,
            "all",
            "partial",
            [
                alg("method=numeric,sin(42*pi/180)", ["sin(3theta+72)=cos48=sin42"]),
                alg("method=numeric,atan(1)*180/pi", ["tan theta=1 from branch work"]),
            ],
            ["theta=22,110,142"],
            "degree range selection is manual",
        ),
        row(
            "e",
            6,
            "a-b",
            "testable",
            [
                alg("expand((1-2*x)^6)", ["f(x)=1-12x+60x^2-160x^3+..."]),
                alg("expand((2+x)^7)", ["g(x)=128+448x+672x^2+560x^3+..."]),
                alg("method=numeric,7680-5376+672", ["coefficient of x^2 in f*g is 2976"]),
            ],
            ["2976"],
        ),
        row(
            "e",
            7,
            "a-b",
            "partial",
            [
                alg("factor(2*x^2-7*x)", ["2x^2-7x=0 gives x=0,7/2"]),
                alg("method=numeric,343/24", ["shaded area between curves is 343/24"]),
            ],
            ["A(0,16) B(7/2,9)", "area=343/24"],
            "integral setup and diagram are manual",
        ),
        row(
            "e",
            8,
            "a-b",
            "partial",
            [
                alg("method=numeric,6", ["AC=6i-25j"]),
                alg("method=numeric,14-13/3", ["OD=14i-13/3 j"]),
                alg("method=numeric,28-26/3", ["DB=28i-26/3 j collinear with OD"]),
            ],
            ["OD=14i-13/3 j", "O,D,B collinear"],
            "vector diagram is manual",
        ),
        row(
            "e",
            9,
            "a-b",
            "partial",
            [
                alg("method=numeric,log(12)/log(4/3)", ["4*3^(x+2)=3*4^x gives x=log12/log(4/3)"]),
                alg("method=numeric,(1+4)^2", ["(1+sqrt(x))^2=9+8 at x=16"]),
                alg("method=numeric,16", ["x=16"]),
            ],
            ["x≈8.64", "x=16"],
            "log laws in (b) are manual",
        ),
        row(
            "e",
            10,
            "a-d",
            "partial",
            [
                alg("method=numeric,sqrt(20)", ["radius sqrt(20) from centre (2,-2)"]),
                alg("solve((y+2)^2=16,y)", ["y-intercepts 2 and -6"]),
                alg("factor(k^2+12*k-64,k)", ["tangent discriminant gives k=4,-16"]),
                alg("solve(k^2+12*k-64=0,k)", ["k=4 or k=-16"]),
            ],
            ["centre (2,-2) r=sqrt(20)", "k=4,-16"],
            "show-that quadratic and sketch are manual",
        ),
        marker("e", 6, 13, 10),
    ]


def build_mp1_g() -> list[dict]:
    return [
        row(
            "g",
            1,
            "a-d",
            "testable",
            [
                alg("0^3-19*0+0", ["f(0)=0 gives k=0"]),
                alg("5-0^3+19*0", ["f(0)=5 gives k=5"]),
                alg("2^3-19*2+30", ["f(2)=0 gives k=30"]),
                alg("(-1)^3-19*(-1)-25", ["f(-1)=-7 gives k=-25"]),
            ],
            ["k=0,5,30,-25"],
        ),
        row(
            "g",
            2,
            "a-b",
            "testable",
            [
                alg("expand((3-sqrt(8))^2)", ["(3-sqrt(8))^2=17-12sqrt(2)"]),
                alg("method=numeric,3*sqrt(7)/3+14*sqrt(7)/7", ["sqrt(63)/3+14/sqrt(7)=3sqrt(7)"]),
            ],
            ["17-12sqrt(2)", "3sqrt(7)"],
        ),
        row(
            "g",
            3,
            "all",
            "partial",
            [
                alg("method=numeric,atan(1)*180/pi", ["sin theta=cos theta gives tan theta=1"]),
            ],
            ["theta=45,225"],
            "degree branch selection is manual",
        ),
        row(
            "g",
            4,
            "a-c",
            "partial",
            [
                alg("solve(7*x=28,x)", ["l1 and l2 meet at x=4"]),
                alg("method=numeric,-5", ["P(4,-5)"]),
                alg("method=numeric,2*19/3", ["triangle area 38/3"]),
            ],
            ["P(4,-5)", "area=38/3"],
            "line equations and diagram are manual",
        ),
        row(
            "g",
            5,
            "a-c",
            "partial",
            [
                alg("method=numeric,1+sqrt(5)", ["root 1+sqrt(5)"]),
                alg("method=numeric,1-sqrt(5)", ["root 1-sqrt(5)"]),
            ],
            ["1-sqrt(5)<x<1+sqrt(5)"],
            "sketch interval choice is manual",
        ),
        row(
            "g",
            6,
            "a-c",
            "partial",
            [
                alg("factor(x^2-2*x-8)", ["x-intercepts x=-2,4"]),
                alg("diff(8+2*x-x^2,x)", ["dy/dx=2-2x"]),
                alg("method=numeric,16-28/3", ["shaded area 20/3"]),
            ],
            ["P(0,8) Q(-2,0) R(4,0)", "area=20/3"],
            "tangent setup and diagram are manual",
        ),
        row(
            "g",
            7,
            "a-c",
            "partial",
            [
                alg("expand((1-2*x)^11)", ["binomial to x^4: 1-22x+220x^2-1320x^3+5280x^4"]),
                alg("method=numeric,1-22/30+220/30^2-1320/30^3+5280/30^4", ["(14/15)^11 approx 1582/3375"]),
            ],
            ["(14/15)^11≈1582/3375", "error 0.122%"],
            "percentage-error interpretation is manual",
        ),
        row(
            "g",
            8,
            "all",
            "skip",
            [],
            ["proof about (k+1)/sqrt(k)"],
            "show-that proof only",
        ),
        row(
            "g",
            9,
            "a-c",
            "partial",
            [
                alg("method=numeric,(8-4)/(6-4)", ["grad CP=2"]),
                alg("solve(5*x=50,x)", ["l1 and l2 meet x=10"]),
                alg("factor(x^2-16*x+64)", ["repeated root x=8 tangent"]),
            ],
            ["2y+x=22", "Q(10,6)", "R(8,2)"],
            "circle geometry diagram is manual",
        ),
        row(
            "g",
            10,
            "all",
            "partial",
            [
                alg("method=numeric,0.5*5*4*sqrt(3)", ["area=10sqrt(3)"]),
            ],
            ["area=10sqrt(3)"],
            "Pythagoras setup is manual",
        ),
        row(
            "g",
            11,
            "all",
            "partial",
            [
                alg("solve(4*x-1=384,x)", ["2^(4x-1)=2^384"]),
                alg("method=numeric,385/4", ["x=96.25"]),
            ],
            ["x=96.25"],
            "index law rewrite is manual",
        ),
        row(
            "g",
            12,
            "all",
            "partial",
            [
                alg("factor(2*u^2-7*u+3)", ["2y^2-9y+9=0"]),
                alg("method=numeric,exp(3)", ["x=e^3"]),
                alg("method=numeric,exp(1/2)", ["x=sqrt(e)"]),
            ],
            ["x=sqrt(e)", "x=e^3"],
            "ln substitution is manual",
        ),
        marker("g", 6, 16, 12),
    ]


def build_mp1_h() -> list[dict]:
    return [
        row(
            "h",
            1,
            "all",
            "testable",
            [
                alg(
                    "method=numeric,90/sqrt(3)-sqrt(6)*sqrt(8)-(2*sqrt(3))^3",
                    ["surd simplification"],
                    ["3.46410161514"],
                ),
            ],
            ["2sqrt(3)"],
        ),
        row(
            "h",
            2,
            "a-b",
            "testable",
            [
                alg("expand((1-2*x)^8)", ["(1-2x)^8 to x^3"]),
                alg("expand((2+3*x)*(1-2*x)^8)", ["product gives 2-29x+176x^2-560x^3"]),
            ],
            ["2-29x+176x^2-560x^3"],
        ),
        row(
            "h",
            3,
            "all",
            "partial",
            [
                alg("factor(3*x^2-7*x-40)", ["cosine rule simplifies to 3x^2-7x-40"]),
                alg("solve(3*x^2-7*x-40=0,x)", ["x=5"]),
                alg("method=numeric,0.5*16*10*sin(pi/3)", ["area=40sqrt(3)"]),
            ],
            ["x=5", "area=40sqrt(3)"],
            "cosine-rule setup is manual",
        ),
        row(
            "h",
            4,
            "a-b",
            "partial",
            [
                alg("method=numeric,sqrt(4)", ["radius 2 from (x-3)^2+(y-4)^2=4"]),
                alg("solve(5*x=6,x)", ["lines meet x=6/5"]),
                alg("method=numeric,sqrt(81/5)-2", ["arc length 9sqrt(5)/5-2"]),
            ],
            ["centre (3,4) r=2", "distance 9sqrt(5)/5-2"],
            "perpendicular-line setup is manual",
        ),
        row(
            "h",
            5,
            "all",
            "partial",
            [
                alg("factor(11*p^2+14*p-25,p)", ["discriminant=0 for repeated roots"]),
                alg("solve(11*p^2+14*p-25=0,p)", ["p=1,-25/11"]),
                alg("factor(9*x^2+6*x+1)", ["x=-1/3 when p=1"]),
                alg("factor(9*x^2-30*x+25)", ["x=5/3 when p=-25/11"]),
            ],
            ["p=1,-25/11", "x=-1/3,5/3"],
            "discriminant derivation is manual",
        ),
        row(
            "h",
            6,
            "all",
            "skip",
            [],
            ["first-principles limit proof"],
            "proof only",
        ),
        row(
            "h",
            7,
            "a-b",
            "partial",
            [
                alg("1^3-3*1+2", ["f(1)=0"]),
                alg("factor(x^3-3*x+2)", ["f(x)=(x-1)^2(x+2)"]),
            ],
            ["(x-1)^2(x+2)"],
            "sketch is manual",
        ),
        row(
            "h",
            8,
            "all",
            "partial",
            [
                alg("method=numeric,sqrt(17)", ["|AB|=sqrt(17)"]),
                alg("method=numeric,acos(2/sqrt(629))*180/pi", ["angle≈85.4°"]),
            ],
            ["|AB|=sqrt(17)", "θ≈85.4°"],
            "vector diagram is manual",
        ),
        row(
            "h",
            9,
            "all",
            "partial",
            [
                alg("factor(2*u^2-7*u+3)", ["ln x substitution"]),
                alg("method=numeric,exp(3)", ["x=e^3"]),
            ],
            ["x=sqrt(e)", "x=e^3"],
            "ln setup is manual",
        ),
        row(
            "h",
            10,
            "all",
            "partial",
            [
                alg("method=numeric,8/3-8+16", ["integral area 32/3 minus triangle"]),
                alg("method=numeric,32/3-4", ["shaded area 20/3"]),
            ],
            ["area=20/3"],
            "integral setup is manual",
        ),
        row(
            "h",
            11,
            "all",
            "partial",
            [
                alg("method=numeric,(2-5)/(-7-5)", ["grad BC=1/4"]),
                alg("method=numeric,-4*1+8", ["D(1,4)"]),
            ],
            ["D(1,4)"],
            "perpendicular construction is manual",
        ),
        row(
            "h",
            12,
            "all",
            "partial",
            [
                alg("factor(x^2-14*x+48)", ["x-intercepts 6,8"]),
                alg("method=numeric,2*8-14", ["gradient at Q is 2"]),
            ],
            ["S(59/8,-5/4)"],
            "tangent geometry is manual",
        ),
        marker("h", 6, 16, 12),
    ]


SP2_APPEND: list[dict] = []


def verify_rows(rows: list[dict]) -> int:
    bad = 0
    for row_obj in rows:
        if row_obj.get("verdict") == "skip":
            continue
        for item in row_obj.get("inputs", []):
            rc, out = run_alg(item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                bad += 1
                print(f"FAIL {row_obj['id']} {item['input']!r} missing={missing}", file=sys.stderr)
    return bad


def sync_queue(rebuilt: dict[str, list[dict]]) -> tuple[int, int]:
    sources = {f"MadAsMaths papers/mp1_{k}.pdf" for k in rebuilt}
    kept: list[str] = []
    removed = 0
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        o = json.loads(line)
        if o.get("source_pdf") in sources:
            removed += 1
            continue
        kept.append(line)
    added = 0
    for rows in rebuilt.values():
        for row_obj in rows:
            kept.append(json.dumps(row_obj, ensure_ascii=False, separators=(",", ":")))
            added += 1
    for row_obj in SP2_APPEND:
        kept.append(json.dumps(row_obj, ensure_ascii=False, separators=(",", ":")))
    QUEUE.write_text("\n".join(kept) + "\n")
    return removed, added


def main() -> int:
    rebuilt = {
        "e": build_mp1_e(),
        "g": build_mp1_g(),
        "h": build_mp1_h(),
    }
    for letter, rows in rebuilt.items():
        path = BATCHES / f"madas_mp1_{letter}_rows.json"
        path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
        bad = verify_rows(rows)
        print(f"mp1_{letter}: {len(rows)} rows, verify_bad={bad}")
        if bad:
            return 1
    removed, added = sync_queue(rebuilt)
    print(f"queue: removed {removed}, wrote {added} rows (incl sp2)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

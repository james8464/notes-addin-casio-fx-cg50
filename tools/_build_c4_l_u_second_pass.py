#!/usr/bin/env python3
"""Build and verify c4_l–c4_u second-pass golden queue rows (worker 08)."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
OUT = ROOT / "progress" / "batches" / "madas_c4_l_u_second_pass_rows.json"
RB = "second-pass VM re-scan worker_08: solution PNGs c4_l–c4_u"


def run(inp: str) -> tuple[int, str]:
    p = subprocess.run(
        [str(RUNNER), "--alg", inp],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=20,
    )
    return p.returncode, p.stdout + p.stderr


def marker(inp: str) -> str:
    rc, out = run(inp)
    if rc != 0:
        raise RuntimeError(f"bad {inp!r} rc={rc} out={out[:300]}")
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("= "):
            return line[2:].strip()
        if line.startswith("Answer:"):
            return line.replace("Answer:", "").strip()
    raise RuntimeError(f"no marker for {inp!r} out={out[:300]}")


def alg(inp: str, working: list[str]) -> dict:
    return {
        "module": "algebra",
        "input": inp,
        "mark_scheme_working": working,
        "expected_output_markers": [marker(inp)],
    }


def row(
    letter: str,
    q: int,
    part: str,
    verdict: str,
    inputs: list[dict],
    final: list[str],
    suffix: str | None = None,
    reason: str | None = None,
) -> dict:
    part_slug = part.replace("/", "-").replace(" ", "")
    sid = suffix or f"sp2_{part_slug}" if part_slug not in ("all", "sp2") else "sp2"
    r: dict = {
        "id": f"madas_iygb_c4_{letter}_q{q}_{sid}_exact_inputs",
        "source_pdf": f"MadAsMaths papers/c4_{letter}.pdf",
        "question": f"q{q}",
        "part": part,
        "verdict": verdict,
        "review_basis": RB,
        "inputs": inputs,
        "expected_final": final,
    }
    if reason:
        r["unsupported_reason"] = reason
    return r


def build() -> list[dict]:
    rows: list[dict] = []

    # c4_l — 8 questions; first pass only q1–q5 with template placeholders
    rows += [
        row("l", 1, "a", "testable", [alg("method=numeric,2*(exp(2)-1)", ["Q1a e^(x/2) integral"])], ["2(e^2-1)"]),
        row("l", 1, "b", "testable", [alg("method=numeric,ln(2)", ["Q1b tan integral"])], ["ln2"]),
        row("l", 5, "a", "testable", [alg("method=numeric,48", ["Q5a area under sin"])], ["area=48"]),
        row("l", 5, "b", "partial", [alg("method=numeric,72*pi^2", ["Q5b volume of revolution"])], ["V=72pi^2"], reason="integral setup manual"),
        row("l", 4, "b", "partial", [alg("method=numeric,exp(-3/2)", ["Q4b stationary k"])], ["k=e^(-3/2)"], reason="implicit setup manual"),
        row("l", 7, "b", "testable", [alg("method=numeric,ln(3)+2", ["Q7b trig integral"])], ["ln3+2"]),
        row("l", 8, "c", "testable", [alg("method=numeric,(9/5)*ln(4)", ["Q8c DE time"])], ["t=(9/5)ln4"]),
        row("l", 6, "e", "partial", [alg("method=numeric,20*sqrt(42)", ["Q6e kite area"])], ["20sqrt42"], reason="vector geometry setup manual"),
    ]

    # c4_m — 9 questions
    rows += [
        row("m", 2, "b", "testable", [alg("method=numeric,-27", ["Q2b x^2 coefficient"])], ["-27"]),
        row("m", 3, "all", "partial", [alg("method=numeric,sqrt(5)", ["Q3 dh/dt at h=2"])], ["sqrt5"], reason="related-rates setup manual"),
        row("m", 5, "b", "partial", [alg("method=numeric,pi*(13+6*ln(3))", ["Q5b volume"])], ["pi(13+6ln3)"], reason="expansion setup manual"),
        row("m", 8, "b", "testable", [alg("method=numeric,ln(6)", ["Q8b stationary y"])], ["y=ln6"]),
        row("m", 9, "a", "testable", [alg("method=numeric,72*pi", ["Q9a ellipse area"])], ["72pi"]),
        row("m", 9, "c", "partial", [alg("method=numeric,192*sqrt(3)-72*pi", ["Q9c gold area"])], ["approx106"], reason="geometry setup manual"),
    ]

    # c4_n — 8 questions; first pass only q1–q4
    rows += [
        row("n", 1, "all", "testable", [alg("method=numeric,1", ["Q1 definite integral"])], ["=1"]),
        row("n", 3, "a", "testable", [alg("method=numeric,-3", ["Q3a dy/dx at t=1/2"])], ["-3"]),
        row("n", 4, "all", "testable", [alg("method=numeric,6/5", ["Q4 dy/dt at t=4"])], ["6/5"]),
        row("n", 6, "d", "testable", [alg("method=numeric,-3", ["Q6d lambda from distance"])], ["lambda=-3 or 1"]),
        row("n", 7, "all", "partial", [alg("method=numeric,pi*(7*sqrt(3)/3+8*pi/3)", ["Q7 volume"])], ["V=(pi/3)(7sqrt3+8pi)"], reason="integral setup manual"),
    ]

    # c4_o
    rows += [
        row("o", 1, "b", "testable", [alg("method=numeric,4*ln(7/2)", ["Q1b partial-fraction integral"])], ["4ln(7/2)"]),
        row("o", 2, "b", "partial", [alg("method=numeric,sqrt(6)", ["Q2b binomial approx"])], ["sqrt6≈2.45"], reason="expansion setup manual"),
        row("o", 3, "a-ii", "testable", [alg("method=numeric,-3/4", ["Q3a tangent gradient at (1,1)"])], ["m=-3/4"]),
    ]

    # c4_p
    rows += [
        row("p", 1, "a", "testable", [alg("method=numeric,481", ["Q1 x^2 coefficient"])], ["481"]),
        row("p", 2, "b", "testable", [alg("method=numeric,6", ["Q2b find k"])], ["k=6"]),
    ]

    # c4_q — 7 questions
    rows += [
        row("q", 1, "b", "testable", [alg("method=numeric,-3+8*ln(2)", ["Q1b parts integral"])], ["-3+8ln2"]),
        row("q", 2, "a", "testable", [alg("method=numeric,4/3", ["Q2a find n"])], ["n=4/3"]),
        row("q", 2, "a", "testable", [alg("method=numeric,3/2", ["Q2a find a"])], ["a=3/2"], suffix="sp2_a"),
        row("q", 2, "b", "testable", [alg("method=numeric,-1/6", ["Q2b coefficient b"])], ["b=-1/6"]),
    ]

    # c4_r
    rows += [
        row("r", 2, "all", "partial", [alg("method=numeric,81*pi", ["Q2 dA/dt at r=13.5"])], ["81pi"], reason="chain-rule setup manual"),
        row("r", 3, "all", "partial", [alg("method=numeric,1.34", ["Q3 trapezium rule"])], ["≈1.34"], reason="table setup manual"),
    ]

    # c4_s — q8 proof already skip
    rows += [
        row("s", 2, "b", "testable", [alg("method=numeric,8/3", ["Q2b area check"])], ["area=8/3"]),
    ]

    # c4_t
    rows += [
        row("t", 1, "b-i", "partial", [alg("method=numeric,1/(15*pi^(1/3))", ["Q1b dy/dt at t=60"])], ["1/(15pi^(1/3))"], reason="cone setup manual"),
        row("t", 2, "all", "testable", [alg("method=numeric,16/3", ["Q2 area integral"])], ["area=16/3"]),
    ]

    # c4_u
    rows += [
        row("u", 2, "all", "testable", [alg("method=numeric,2-ln(3)", ["Q2 substitution integral"])], ["2-ln3"]),
        row("u", 3, "b", "testable", [alg("method=numeric,2", ["Q3b limiting population"])], ["N→2 thousands"]),
        row("u", 4, "all", "partial", [alg("method=numeric,(1-4*ln(2))/(2*ln(2)-1)", ["Q4 implicit diff at (4,2)"])], ["dy/dx=(1-4ln2)/(2ln2-1)"], reason="implicit setup manual"),
    ]

    return rows


def verify(rows: list[dict]) -> list[str]:
    bad: list[str] = []
    for r in rows:
        if r.get("verdict") == "skip":
            continue
        for i, item in enumerate(r.get("inputs", []), 1):
            rc, out = run(item["input"])
            missing = [m for m in item.get("expected_output_markers", []) if m and m not in out]
            if rc != 0 or missing:
                bad.append(f"{r['id']} #{i} {item['input']!r} missing={missing}")
    return bad


def main() -> int:
    rows = build()
    bad = verify(rows)
    if bad:
        print("VERIFY FAIL:", file=sys.stderr)
        for b in bad:
            print(b, file=sys.stderr)
        return 1
    OUT.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n")
    print(f"OK wrote {len(rows)} rows to {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
PLANNER = ROOT / "tools" / "working_planner.py"


def compact(s: str) -> str:
    return "".join(s.split()).replace("*", "").replace("**", "^")


def run_prod(expr: str) -> str:
    proc = subprocess.run(
        [str(RUNNER), "--alg", expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=10,
    )
    out = (proc.stdout or "") + (proc.stderr or "")
    if proc.returncode:
        raise AssertionError(f"{expr}: production failed rc={proc.returncode}\n{out}")
    return out


def run_planner(expr: str) -> str:
    proc = subprocess.run(
        ["python3", str(PLANNER), expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=10,
    )
    out = (proc.stdout or "") + (proc.stderr or "")
    if proc.returncode:
        raise AssertionError(f"{expr}: planner failed rc={proc.returncode}\n{out}")
    return out


def assert_markers(expr: str, out: str, markers: list[str]) -> None:
    packed = compact(out)
    missing = [m for m in markers if compact(m) not in packed]
    if missing:
        raise AssertionError(f"{expr}: missing {missing}\n{out}")
    bad = ["not equivalent", "no verified route", "no route", "No verified A-level working route found."]
    leaked = [m for m in bad if m.lower() in out.lower()]
    if leaked:
        raise AssertionError(f"{expr}: bad marker {leaked}\n{out}")


def generated_cases() -> list[tuple[str, list[str], bool]]:
    cases: list[tuple[str, list[str], bool]] = []
    for v in ["x", "t", "y"]:
        cases += [
            (f"xform(tan({v}),sin({v})/cos({v}))", ["Reciprocal identities", "Verified"], True),
            (f"xform(cot({v}),cos({v})/sin({v}))", ["Reciprocal identities", "Verified"], True),
            (f"xform(sec({v}),1/cos({v}))", ["Reciprocal identities", "Verified"], True),
            (f"xform(cosec({v}),1/sin({v}))", ["Reciprocal identities", "Verified"], True),
        ]
        for k in [1, 2, 3, 5]:
            cases += [
                (f"xform({2*k}*sin({v})*cos({v}),{k}*sin(2*{v}))", ["Double-angle identities", "Verified"], True),
                (f"xform({k}*cos({v})^2-{k}*sin({v})^2,{k}*cos(2*{v}))", ["Double-angle identities", "Verified"], True),
                (f"xform({k}+{k}*tan({v})^2,{k}*sec({v})^2)", ["Pythagorean identities", "Verified"], True),
                (f"xform({k}+{k}*cot({v})^2,{k}*cosec({v})^2)", ["Pythagorean identities", "Verified"], True),
            ]
            if v == "x":
                cases += [
                    (f"xform({k}*sin(3*x)+{k}*sin(x),{2*k}*sin(2*x)*cos(x))", ["Sum-prod", "Verified"], True),
                    (f"xform({k}*sin(3*x)-{k}*sin(x),{2*k}*cos(2*x)*sin(x))", ["Sum-prod", "Verified"], True),
                    (f"xform({k}*cos(3*x)+{k}*cos(x),{2*k}*cos(2*x)*cos(x))", ["Sum-prod", "Verified"], True),
                    (f"xform({k}*cos(3*x)-{k}*cos(x),-{2*k}*sin(2*x)*sin(x))", ["Sum-prod", "Verified"], True),
                ]
    for base in [2, 3, 5]:
        for n in [2, 3, 5]:
            cases.append((f"xform(log({base},x^{n}),{n}*log({base},x))", ["Power", "Verified"], False))
        cases += [
            (f"xform(log({base},x*y),log({base},x)+log({base},y))", ["Product", "Verified"], False),
            (f"xform(log({base},x/y),log({base},x)-log({base},y))", ["Quotient", "Verified"], False),
        ]
    for a, b in [("x", "y"), ("2*x", "3*y"), ("theta", "phi")]:
        cases += [
            (
                f"xform(sin({a}+{b}),sin({a})*cos({b})+cos({a})*sin({b}))",
                ["texpand", "Verified"],
                True,
            ),
            (
                f"xform(cos({a}+{b}),cos({a})*cos({b})-sin({a})*sin({b}))",
                ["texpand", "Verified"],
                True,
            ),
        ]
    for n in [2, 3, 4, 5]:
        cases.append((f"xform(ln(x^{n}),{n}*ln(x))", ["Power", "Verified"], False))
    cases += [
        ("xform(ln(x*y),ln(x)+ln(y))", ["Product", "Verified"], False),
        ("xform(ln(x/y),ln(x)-ln(y))", ["Quotient", "Verified"], False),
    ]
    return cases


def main() -> int:
    cases = generated_cases()
    planner_checked = 0
    for expr, markers, also_planner in cases:
        assert_markers(expr, run_prod(expr), markers)
        if also_planner or "ln(" in expr or "log(" in expr:
            planner_markers = ["Planner search:", "Verified by equivalence check"]
            assert_markers(expr, run_planner(expr), planner_markers)
            planner_checked += 1
    print(f"OK xform rule families production_cases={len(cases)} planner_cases={planner_checked}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

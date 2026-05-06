#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


def run(args: list[str], stdin: str | None = None) -> str:
    p = subprocess.run(
        [str(HOST), *args],
        input=stdin,
        text=True,
        cwd=REPO,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return p.stdout


def require(name: str, out: str, must: tuple[str, ...], forbid: tuple[str, ...] = ()) -> int:
    low = out.lower()
    bad = [x for x in must if x.lower() not in low]
    bad += [f"forbidden:{x}" for x in forbid if x.lower() in low]
    if bad:
        print(f"FAIL {name}: {bad}", file=sys.stderr)
        print(out, file=sys.stderr)
        return 1
    return 0


def main() -> int:
    bad = 0
    bad += require(
        "cosh_asinh_integral",
        run(["--int", "cosh(asinh(x+5)),method=auto"]),
        ("cosh(asinh(u))", "sqrt", "asinh(x + 5)", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "symbolic_expand_noop",
        run(["--alg", "x^2+a*x+b,method=expand"]),
        ("Route: expand", "Answer:", "x^2"),
        ("only polynomial expansion supported", "ERR:", "Unexpected token"),
    )
    bad += require(
        "cartesian_wrapped_trig",
        run(["--stdin-program", "algebraProgram.py"], "11\nsin(t)\n(cos((t)))\n(t)\n"),
        ("sin", "cos", "Answer: x^2 + y^2 = 1"),
        ("supports linear", "ERR:"),
    )
    bool_out = run(["--stdin-program", "ComputerScience/booleanProgram.py"], "1\nA.(B+C)+A.B\n")
    bad += require(
        "boolean_no_loop",
        bool_out,
        ("Result:",),
        ("50.", "49.", "48."),
    )
    if len([ln for ln in bool_out.splitlines() if ln.strip()]) > 12:
        print("FAIL boolean_no_loop: too many repeated lines", file=sys.stderr)
        print(bool_out, file=sys.stderr)
        bad += 1
    bad += require(
        "poly_division_integral",
        run(["--int", "(x^4+5*x^2+4)/(x^2+1)"]),
        ("Cancel common factor", "power rule", "Answer: 4*x + 1/3*x^3 + C"),
        ("ERR:",),
    )
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

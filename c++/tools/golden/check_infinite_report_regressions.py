#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from working_audit_utils import final_math_line, has_strong_output, markers_present

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
    effective = [x for x in must if x != "Answer:"]
    bad = [x for x in effective if not markers_present(out, [x])]
    bad += [f"forbidden:{x}" for x in forbid if x.lower() in low]
    if "Answer:" in must and not final_math_line(out):
        bad.append("missing:final answer")
    if not has_strong_output(out):
        bad.append("weak-output")
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
        "sinh_acosh_integral",
        run(["--int", "sinh(acosh(x+5)),method=auto"]),
        ("sinh(acosh(u))", "sqrt", "acosh(x + 5)", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "symbolic_expand_noop",
        run(["--alg", "x^2+a*x+b,method=expand"]),
        ("Answer:", "x^2"),
        ("only polynomial expansion supported", "ERR:", "Unexpected token"),
    )
    bad += require(
        "alg_function_method_strip",
        run(["--alg", "factor((x+1)^2,method=factor)"]),
        ("(x + 1)^2",),
        ("Expected )", "ERR:", "method=factor"),
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
        ("N/D = x^2 + 4", "I = Int(x^2 + 4)", "4*x + 1/3*x^3 + C"),
        ("ERR:",),
    )
    bad += require(
        "inverse_trig_parts_simplified",
        run(["--int", "asin((x-2)/6),method=parts"]),
        ("u = asin", "dv = dw", "sqrt(- (x - 2)^2 + 36)"),
        ("ERR:", "Answer: int("),
    )
    bad += require(
        "trig_square_interval_working",
        run(["--trig", "sin(x)^2=1/4,x,0,2*pi,method=square_then_check"]),
        ("u=sin(x)", "sin(x)=1/2", "sin(x)=-1/2", "0 <= x <= 2*pi", "x = [pi/6, 5*pi/6, 7*pi/6, 11*pi/6]"),
        ("ERR:",),
    )
    bad += require(
        "online_poly_exp_parts",
        run(["--int", "(2*x+5)*e^(x/3),method=parts"]),
        ("DI table", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_poly_trig_di",
        run(["--int", "(2*x^2+5*x+1)*cos(3*x),method=di"]),
        ("DI table", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_atan_recip_parts",
        run(["--int", "atan(5/x),method=parts"]),
        ("u=atan", "dv=dx", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_quad_quad_pf",
        run(["--int", "(2*x^3+x^2+5*x+1)/((x^2+1)*(x^2+4)),method=pf"]),
        ("(Ax+B)/(x^2 + 1)+(Cx+D)/(x^2 + 4)", "A=", "D="),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_repeated_quad_pf",
        run(["--int", "(x^3+1)/((x-1)*(x^2+1)^2),method=pf"]),
        ("quadratic^2", "A=", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_hidden_power_sub",
        run(["--int", "x^5*e^(x^3),method=sub"]),
        ("Let u=x^3", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_hidden_power_high_sub",
        run(["--int", "x^15*sin(2*x^8),method=sub"]),
        ("Let u=x^8", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_sqrt_shift_sub",
        run(["--int", "1/(2+sqrt(x-3)),method=sub"]),
        ("Let u=sqrt", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_alg_sym_wide",
        run(["--int", "(x^2+1)/(x^4+8*x^2+1),method=sub"]),
        ("N,D / x^2", "u=x-1/x", "du = (1+1/x^2) dx"),
        ("No elementary primitive found", "ERR:"),
    )
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

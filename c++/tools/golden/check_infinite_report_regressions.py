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


def require_compact(name: str, out: str, must: tuple[str, ...], final_must: tuple[str, ...], forbid: tuple[str, ...] = ()) -> int:
    bad = [x for x in must if not markers_present(out, [x])]
    bad += [f"forbidden:{x}" for x in forbid if x.lower() in out.lower()]
    lines = [ln.strip() for ln in out.splitlines() if ln.strip()]
    final = lines[-1] if lines else ""
    bad += [f"final:{x}" for x in final_must if x not in final]
    if "ERR:" in out or len(lines) > 6:
        bad.append("not compact")
    if bad:
        print(f"FAIL {name}: {bad}", file=sys.stderr)
        print(out, file=sys.stderr)
        return 1
    return 0


def main() -> int:
    bad = 0
    bad += require_compact(
        "cosh_asinh_removed",
        run(["--int", "cosh(asinh(x+5)),method=auto"]),
        ("Err: unsupported function",),
        (),
        ("Step 2:", "Use cosh"),
    )
    bad += require_compact(
        "sinh_acosh_removed",
        run(["--int", "sinh(acosh(x+5)),method=auto"]),
        ("Err: unsupported function",),
        (),
        ("Step 2:", "Use sinh"),
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
    bad += require_compact(
        "boolean_removed",
        run(["--stdin-program", "ComputerScience/booleanProgram.py"], "1\nA.(B+C)+A.B\n"),
        ("Err: unsupported program",),
        (),
        ("Result:", "proved", "NAND", "NOR"),
    )
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
        ("I1 = Int(5*e^(x/3))dx", "I2 = Int(2*x*e^(x/3))dx", "I = I1+I2", "Answer:"),
        ("No elementary primitive found", "ERR:"),
    )
    bad += require(
        "online_poly_trig_di",
        run(["--int", "(2*x^2+5*x+1)*cos(3*x),method=di"]),
        ("I1 = Int(cos(3*x))dx", "I2 = Int(5*x*cos(3*x))dx", "I3 = Int(2*x^2*cos(3*x))dx", "I = I1+I2+I3", "Answer:"),
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
    bad += require(
        "fuzz_wrapped_defint_power_spaces",
        run(["--int", "(defint(sin(x)^  4  / (sin((x))  **  4  +cos(x)^  4),x,0,pi /  2))"]),
        ("King property", "2I = Integral_0^(pi/2) 1 dx = pi/2", "pi/4"),
        ("ERR:", "Expected )"),
    )
    bad += require(
        "fuzz_wrapped_defint_variable",
        run(["--int", "(defint(ln((x))/(1 +(x)**2),(x),0,inf))"]),
        ("Split I = Integral_0^1", "Tail becomes", "0"),
        ("ERR:", "Expected )"),
    )
    bad += require_compact(
        "labelled_parametric_second_removed",
        run(["--derive", "(x=t^2 + 1/t),(y=t**2-1/t),t,x,method=param_second"]),
        ("Err: invalid method",),
        (),
        ("d2y/dx2", "Unexpected token"),
    )
    param_e = run(["--derive", "(x)=exp((t))*cos((t)),y=exp(t)sin(t),t,x,method=param_second"])
    bad += require_compact(
        "labelled_parametric_second_exp_removed",
        param_e,
        ("Err: invalid method",),
        (),
        ("d2y/dx2", "Unexpected token", "Divide by"),
    )
    bad += require(
        "nested_surd_rewrite_wrapped",
        run(["--alg", "(rewrite(sqrt((15)+sqrt((224)))))"]),
        ("m+n = 15", "4*m*n = 224", "2*sqrt(2)+sqrt(7)"),
        ("ERR:", "sqrt(sqrt(224) + 15)"),
    )
    bad += require(
        "algebra_de_solve_routes_general_engine",
        run(["--alg", "de_solve(dh/dt=(1/50)*h*(2*h-1)*cos(t/10),h(0)=5/2)"]),
        ("1/(h*(2*h - 1)) dh = 1/50*cos(t/10) dt", "C = ln(8/5)", "h = 5/(10 - 8*e^(sin(t/10)/5))"),
        ("solve(de_solve", "ERR:"),
    )
    bad += require(
        "rform_interval_filter_line",
        run(["--trig", "(3*cos(x)+4*sin(x)=2),x,0,2*pi,10,method=rform"]),
        ("x-alpha = arccos(2/5)", "0 <= arctan(4/3)+arccos(2/5) <= 2*pi", "x = [arctan(4/3) + arccos(2/5)"),
        ("ERR:",),
    )
    bad += require(
        "quotient_derivative_substitution_final",
        run(["--derive", "(x^2+1)/(x-1),x,method=quotient"]),
        ("u = x^2 + 1", "y' = (u'v-u*v')/v^2", "dy/dx = [(2*x)*(x - 1) - (x^2 + 1)*(1)]/(x - 1)^2"),
        ("(x - 1)^-2", "ERR:"),
    )
    bad += require(
        "first_principles_scaled_trig_math_lines",
        run(["--derive", "cos(3*x),x,method=first_principles"]),
        ("[cos(3*(x+h))-cos(3*x)]/h = [cos(3*x+3*h)-cos(3*x)]/h", "h->0: sin(3*h/2)/(h/2)->3", "d/dx cos(3*x) = -3*sin(3*x)"),
        ("This is", "Divide by", "ERR:"),
    )
    bad += require(
        "interval_domain_final",
        run(["--stdin-program", "algebraProgram.py"], "10\n(t)^2+1,t,(0),(4)\n"),
        ("Domain used: (0) <= t <= (4)", "Range: 1 <= y <= 17", "(0) <= t <= (4); 1 <= y <= 17"),
        ("all real t; 1 <= y <= 17", "ERR:"),
    )
    bad += require(
        "binomial_outside_support",
        run(["--stats", "binomial(2,0.4,6)"]),
        ("6 is outside 0 <= r <= 2", "P(X = 6) = 0"),
        ("2C6", "0.6^(2-6)", "ERR:"),
    )
    bad += require_compact(
        "plot_removed",
        run(["--stats", "plot(x^2-4,-6,6,9)"]),
        ("Err: unsupported stats function",),
        ("Err: unsupported",),
        ("x-intercepts", "-1.88888889", "1.88888889"),
    )
    bad += require(
        "substitution_integral_keeps_factor",
        run(["--int", "3*x^2*cos(x^3+1),method=sub"]),
        ("I = 3*J", "J = 1/3*Int(cos(u)) du", "sin(x^3 + 1) + C"),
        ("No elementary primitive found", "ERR:"),
    )
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

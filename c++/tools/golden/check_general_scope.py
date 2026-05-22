#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


def run_host(*args: str) -> str:
    proc = subprocess.run([str(HOST), *args], cwd=str(REPO), text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode != 0:
        raise AssertionError(proc.stderr or proc.stdout)
    return proc.stdout


def run_host_err_ok(*args: str) -> str:
    proc = subprocess.run([str(HOST), *args], cwd=str(REPO), text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out = proc.stdout or proc.stderr
    if proc.returncode != 0 and not out.startswith("Err:"):
        raise AssertionError(out)
    return out


def require(name: str, out: str, needles: tuple[str, ...], forbidden: tuple[str, ...] = ()) -> None:
    low = out.lower()
    compact_low = "".join(low.split())
    missing = [n for n in needles if n.lower() not in low and "".join(n.lower().split()) not in compact_low]
    bad = [n for n in forbidden if n.lower() in low]
    if missing or bad:
        raise AssertionError(f"{name}: missing={missing} forbidden={bad}\n{out}")


def main() -> int:
    require(
        "hyperbolic_removed",
        run_host("--derive", "sinh(x^2)+atanh(x/3),x,method=chain"),
        ("Err: unsupported function",),
        ("dy/dx =", "atan(h)"),
    )
    require(
        "non_elementary_integral",
        run_host("--int", "exp(-x^2),method=auto"),
        ("No elementary primitive found",),
        ("A-level primitive",),
    )
    require(
        "general_nested_hyperbolic_removed",
        run_host("--alg", "exp(log(abs(x)+2))+sinh(asinh(x))"),
        ("Err: unsupported function",),
        ("simplify*", "atan(h)", "abs(x) + 2"),
    )
    require(
        "rationalise_removed",
        run_host("--alg", "rationalise(1/(sqrt(2)+1))"),
        ("Err: unsupported function",),
        ("rationalise*", "Answer:"),
    )
    require(
        "comb_removed",
        run_host("--alg", "comb(5,2)"),
        ("Err: unsupported function",),
        ("comb*", "10"),
    )
    require(
        "hyperbolic_reciprocal_removed",
        run_host("--int", "sech(x)^2+coth(x),method=auto"),
        ("Err: unsupported function",),
        ("tanh(x)", "ln(abs(sinh(x)))"),
    )
    require(
        "removed_surface_derive_guard",
        run_host("--derive", "mean(x)+weierstrass(x),x"),
        ("Err: unsupported function",),
        ("dy/dx =", "mean*x", "weierstrass*x"),
    )
    require(
        "removed_surface_integrate_guard",
        run_host("--int", "param_area(t,t^2,t,0,1)"),
        ("Err: unsupported function",),
        ("A = Int", "dx/dt"),
    )
    require(
        "boolean_removed",
        run_host("--alg", "prove_bool(A+A'=1)"),
        ("Err: unsupported function",),
        ("NAND", "NOR", "prove"),
    )
    for flag in ("--bool", "--nand", "--nor"):
        require(
            "boolean_host_" + flag[2:],
            run_host(flag, "A+B"),
            ("Err: unsupported function",),
            ("BOOL:", "NAND", "NOR", "Result:"),
        )
    require(
        "boolean_host_prove",
        run_host("--prove", "A.(B+C)", "A.B+A.C"),
        ("Err: unsupported function",),
        ("proved", "Both sides", "Result:"),
    )
    removed_alg = [
        "normald(0,1,0)", "mean([1,2,3])", "median([1,2,3])", "stdev([1,2,3])",
        "correlation([1,2],[3,4])", "covariance([1,2],[3,4])",
        "linear_regression([1,2],[3,4])", "plot(x)", "plotcontour(x)",
        "plotfield(x)", "plotlist([1,2])", "plotode(x)", "plotparam(x,y)",
        "plotpolar(sin(x))", "plotseq(x)", "disque(x)", "tabular(x^2)",
        "symmetry(x^2)", "mean_value(x,x,0,1)", "volume_x(x,x,0,1)",
        "volume_y(x,x,0,1)", "area_between(x,x^2,x,0,1)",
        "param_area(t,t^2,t,0,1)", "param_area_y(t,t^2,t,0,1)",
        "param_volume_x(t,t^2,t,0,1)", "param_volume_y(t,t^2,t,0,1)",
        "ztest(5,4,1,10,0.05,gt)", "spark([1,2,3])",
    ]
    for expr in removed_alg:
        require("removed_alg_" + expr.split("(")[0], run_host("--alg", expr), ("Err: unsupported function",))
    require(
        "weierstrass_method_removed",
        run_host_err_ok("--int", "1/(2*sin(x)-cos(x)+5),method=weierstrass"),
        ("Err: invalid method",),
        ("u = tan",),
    )
    require(
        "tabular_method_removed",
        run_host_err_ok("--int", "x^2*e^x,method=tabular"),
        ("Err: invalid method",),
        ("I:",),
    )
    require(
        "ztest_stats_removed",
        run_host("--stats", "ztest(5,4,1,10,0.05,gt)"),
        ("Err: unsupported stats function",),
        ("z =", "p ="),
    )
    print("general_scope OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

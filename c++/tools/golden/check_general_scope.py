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
        "linear_regression([1,2],[3,4])", "plot(x)", "plotarea(x,x=0..1)", "plotfunc(x,x)",
        "plotcontour(x)",
        "plotfield(x)", "plotlist([1,2])", "plotode(x)", "plotparam(x,y)",
        "plotpolar(sin(x))", "plotseq(x)", "disque(x)", "tabular(x^2)", "tabvar(x^2)",
        "symmetry(x^2)", "mean_value(x,x,0,1)", "volume_x(x,x,0,1)",
        "volume_y(x,x,0,1)", "area_between(x,x^2,x,0,1)",
        "param_area(t,t^2,t,0,1)", "param_area_y(t,t^2,t,0,1)",
        "param_volume_x(t,t^2,t,0,1)", "param_volume_y(t,t^2,t,0,1)",
        "ztest(5,4,1,10,0.05,gt)", "spark([1,2,3])",
        "csolve(x^2+1=0,x)", "cfactor(x^2+1)", "cpartfrac(1/(x^2+1))",
        "complex(1,2)", "arg(1+i)", "re(1+i)", "im(1+i)", "conj(1+i)",
        "matrix(2,2,1)", "det(matrix(2,2,1))", "rref(matrix(2,2,1))",
        "jordan(matrix(2,2,1))", "svd(matrix(2,2,1))", "inv([[1,2],[3,4]])",
        "linsolve([x+y=1],[x,y])",
        "polar2rectangular(1,pi/3)", "rectangular2polar(1+i)",
        "laplace(sin(x),x,s)", "ilaplace(1/(s^2+1),s,x)",
        "fourier_an(sin(x),x,2*pi,1,0)", "taylor(ln(x),x,1,2)",
        "normald_cdf(0)", "normald_icdf(0.5)", "exponentiald(2,3)", "uniformd(0,1,0.5)",
        "quartiles([1,2,3])",
        "erf(x)", "erfc(x)", "poisson(3,2)", "poisson_cdf(3,2)",
        "geometric(0.4,3)", "negbinomial(3,0.2,4)", "studentd(10,0)",
        "fisher(2,3,1)", "weibulld(1,2,3)", "exponential_regression([1,2],[3,4])",
        "logarithmic_regression([1,2],[3,4])", "power_regression([1,2],[3,4])",
        "rsolve(u(n)=u(n-1)+1,u(n),u(0)=0)", "odesolve(y,x,0,1)",
        "newton(x^2-2,x,1)", "matrix_norm([[1,2],[3,4]])",
        "randmatrix(2,2)", "normalvariate(0,1)", "expovariate(1)",
    ]
    for expr in removed_alg:
        require("removed_alg_" + expr.split("(")[0], run_host("--alg", expr), ("Err: unsupported function",))
    require("vector_dot_supported", run_host("--alg", "dot([1,2],[3,4])"), ("= 11",))
    require("vector_cross_supported", run_host("--alg", "cross([1,2,3],[4,5,6])"), ("(-3,6,-3)",))
    require("vector_norm_supported", run_host("--alg", "norm([3,4])"), ("5",))
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
        "third_derivative_method_removed",
        run_host_err_ok("--derive", "x^4,x,method=third"),
        ("Err: invalid method",),
        ("d3y",),
    )
    require(
        "param_second_method_removed",
        run_host_err_ok("--derive", "t^2+1/t,t^2-1/t,t,method=param_second"),
        ("Err: invalid method",),
        ("d2y/dx2",),
    )
    require(
        "third_mode_removed",
        run_host("--derive", "mode:7,e^(atan(x)),x"),
        ("Err: unsupported function",),
        ("d3y",),
    )
    require(
        "fourth_mode_removed",
        run_host("--derive", "mode:8,e^(3*x)*sin(x),x"),
        ("Err: unsupported function",),
        ("d4y", "Leibnitz"),
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

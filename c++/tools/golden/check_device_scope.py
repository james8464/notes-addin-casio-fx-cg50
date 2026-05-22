#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
DEVICE = REPO / "c++" / "addin" / "host" / "build" / "device_solver_smoke"


@dataclass(frozen=True)
class Case:
    name: str
    module: str
    expr: str
    must: tuple[str, ...]
    allow_unsupported: bool = False
    min_lines: int = 2


BAD = ("exact result from full CAS path", "Traceback", "Answer: int(", "Answer: d/dx(")


CASES: tuple[Case, ...] = (
    Case("shell_simplify", "shell", "simplify(2x+3-x+4)", ("Answer: x + 7",)),
    Case("shell_solve_linear", "shell", "solve(2x+3=7)", ("2x", "Answer: x = 2")),
    Case("shell_solve_quadratic", "shell", "solve((x+1)^2=9)", ("Answer: x = -4 or x = 2",)),
    Case("shell_diff_chain_log", "shell", "diff((2x+ln(x))^3)", ("2 + 1/x", "dy/dx")),
    Case("shell_integrate_poly", "shell", "int((x+1)^2)", ("x^3/3 + x^2 + x + C",)),
    Case("shell_trig_exact", "shell", "trig(cos(5*pi/3))", ("Answer: 1/2",)),
    Case("shell_trig_solve", "shell", "solve_trig(2sin(x)+1=0)", ("x = 210 + 360n",)),
    Case("shell_suvat", "shell", "suvat(s=10,u=0,v=?,a=2,t=5)", ("Answer: v = 10",)),
    Case("shell_binom", "shell", "binom(4,1/2,2)", ("Answer: 3/8",)),
    Case("shell_number_theory", "shell", "gcd(84,126,210)", ("Answer: gcd = 42",)),
    Case("shell_factor", "shell", "factor(x^2-5x+6)", ("Answer: (x - 2)(x - 3)",)),
    Case("shell_compare", "shell", "compare(2(x+1),2x+2)", ("Answer: equivalent.",)),
    Case("shell_compose", "shell", "compose(2x+1,x^2)", ("Answer: f(g(x)) = 2x^2 + 1",)),
    Case("shell_inverse", "shell", "inverse(2x+1)", ("Answer: f^-1(x) = (x - 1)/2",)),
    Case("shell_rewrite", "shell", "rewrite(x^2+2x+3,x+1)", ("Answer: u^2 + 2",)),
    Case("shell_range", "shell", "domain(x^2-4x+1)", ("Range: y >= -3.",)),
    Case("direct_simplify", "simplify", "2x+3-x+4", ("Answer: x + 7",)),
    Case("direct_algebra", "algebra", "x^2-5x+6=0", ("Answer: x = 2 or x = 3",)),
    Case("direct_derive", "derive", "3x^2+2x+1", ("Answer: dy/dx = 6x + 2",)),
    Case("direct_integrate", "integrate", "3x^2+2x+1", ("Answer: x^3 + x^2 + x + C",)),
    Case("direct_trig", "trig", "sin^2(x)+cos^2(x)", ("Answer: 1",)),
    Case("direct_stats_binom", "stats", "binom(4,1/2,2)", ("Answer:", "3/8")),
    Case("direct_suvat", "suvat", "s=10,u=0,v=?,a=2,t=5", ("Answer: v = 10",)),
    Case("unsupported_cartesian", "shell", "cartesian(x=t,y=t^2)", ("Err: unsupported form.",), True, 1),
    Case("unsupported_fitconst", "shell", "fitconst(a*x+b=2*x+3,[a,b])", ("unsupported on CG50 route",), True, 1),
    Case("removed_bool", "shell", "prove_bool(A+A'=1)", ("Err: unsupported function.",), True, 1),
    Case("removed_hyperbolic", "shell", "diff(sinh(x))", ("Err: unsupported function.",), True, 1),
    Case("removed_mean", "shell", "mean([1,2,3])", ("Err: unsupported function.",), True, 1),
    Case("removed_plot", "shell", "plot(x)", ("Err: unsupported function.",), True, 1),
    Case("removed_complex", "shell", "csolve(x^2+1=0,x)", ("Err: unsupported function.",), True, 1),
    Case("removed_matrix", "shell", "det(matrix(2,2,1))", ("Err: unsupported function.",), True, 1),
    Case("removed_matrix_inv", "shell", "inv([[1,2],[3,4]])", ("Err: unsupported function.",), True, 1),
    Case("removed_vector_dot", "shell", "dot([1,2],[3,4])", ("Err: unsupported function.",), True, 1),
    Case("removed_laplace", "shell", "laplace(sin(x),x,s)", ("Err: unsupported function.",), True, 1),
    Case("removed_taylor", "shell", "taylor(ln(x),x,1,2)", ("Err: unsupported function.",), True, 1),
    Case("removed_uniformd", "shell", "uniformd(0,1,0.5)", ("Err: unsupported function.",), True, 1),
)


def run(case: Case) -> tuple[str, list[str], str]:
    proc = subprocess.run(
        [str(DEVICE), case.module, case.expr],
        cwd=REPO,
        text=True,
        capture_output=True,
        timeout=10,
    )
    out = proc.stdout + proc.stderr
    why: list[str] = []
    if proc.returncode:
        why.append(f"returncode={proc.returncode}")
    for marker in case.must:
        if marker not in out:
            why.append(f"missing:{marker}")
    lines = [ln for ln in out.splitlines() if ln.strip()]
    if len(lines) < case.min_lines:
        why.append("too-few-lines")
    if not case.allow_unsupported and "unsupported" in out.lower():
        why.append("unexpected-unsupported")
    for marker in BAD:
        if marker in out:
            why.append(f"bad:{marker}")
    return ("pass" if not why else "fail"), why, out


def main() -> int:
    bad = 0
    for case in CASES:
        status, why, out = run(case)
        print(f"{status.upper()} {case.name}: {case.module} {case.expr}")
        if status != "pass":
            bad += 1
            print("why:", "; ".join(why))
            print(out)
    if bad:
        print(f"device_scope FAIL {bad}/{len(CASES)}")
        return 1
    print(f"device_scope OK {len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

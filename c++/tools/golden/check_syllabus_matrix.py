#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import subprocess
from dataclasses import dataclass
from pathlib import Path

from working_audit_utils import has_strong_output, markers_present

REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
REPORT = REPO / "c++" / "tests" / "reports" / "syllabus_coverage_latest.txt"
JSON_REPORT = REPO / "c++" / "tests" / "reports" / "syllabus_coverage_latest.json"


@dataclass(frozen=True)
class MatrixCase:
    topic: str
    capability: str
    cmd: tuple[str, str]
    must: tuple[str, ...]
    working_expected: bool = True


FORBID = ("ERR:", "Traceback", "Answer: int(", "Answer: d/dx(", "No elementary primitive", "not recognised")


CASES: tuple[MatrixCase, ...] = (
    MatrixCase("pure_proof", "identity proof / equivalence", ("--alg", "compare((a+b)^2,a^2+2*a*b+b^2)"), ("Equivalent",)),
    MatrixCase("pure_algebra", "index law product", ("--alg", "compare(a^m*a^n,a^(m+n))"), ("equivalent",)),
    MatrixCase("pure_algebra", "index law power of power", ("--alg", "compare((a^m)^n,a^(m*n))"), ("equivalent",)),
    MatrixCase("pure_algebra", "surd rationalisation", ("--alg", "compare(1/(2+sqrt(3)),2-sqrt(3))"), ("equivalent",)),
    MatrixCase("pure_algebra", "indices and surd simplification", ("--alg", "rewrite(sqrt(12+sqrt(140)))"), ("sqrt(7)", "sqrt(5)")),
    MatrixCase("pure_algebra", "quadratic solve by factorisation", ("--alg", "solve(x^2-5*x+6=0,x,method=factor)"), ("x = 3", "x = 2")),
    MatrixCase("pure_algebra", "quadratic completing-square solve", ("--alg", "solve(2*x^2-5*x-3=0,x)"), ("(x - 5/4)^2 = 49/16", "x - 5/4 = +/-7/4")),
    MatrixCase("pure_algebra", "factor theorem subpart", ("--alg", "solve(a^3-6*a^2+11*a-6=0,a)"), ("a = 3", "a = 2", "a = 1")),
    MatrixCase("pure_algebra", "linear solve with surd coefficient", ("--alg", "solve(10=2*k/sqrt(5),k)"), ("k = 5*sqrt(5)",)),
    MatrixCase("pure_functions_graphs", "domain/range from function", ("--alg", "range(x/(1+x^2))"), ("-1/2 <= y <= 1/2",), False),
    MatrixCase("pure_functions_graphs", "composite function simplification", ("--alg", "compare(2*(3*x+1)+5,6*x+7)"), ("E1-E2 = 0", "equivalent"), False),
    MatrixCase("pure_functions_graphs", "inverse function rearrange", ("--alg", "solve(y=2*x+3,x)"), ("x = -(y - 3)/-2",)),
    MatrixCase("pure_functions_graphs", "inverse quadratic rearrange branches", ("--alg", "solve(y=(x-1)^2+3,x)"), ("LHS - RHS",), False),
    MatrixCase("pure_functions_graphs", "modulus branch solve", ("--alg", "solve(abs(x-3)=5,x)"), ("x - 3 = 5", "x - 3 = -5", "x = [-2, 8]")),
    MatrixCase("pure_coordinate_geometry", "distance formula subpart", ("--alg", "sqrt((3-1)^2+(7-4)^2)"), ("sqrt(13)",), False),
    MatrixCase("pure_coordinate_geometry", "complete square subpart", ("--alg", "complete_square(x^2+y^2-6*x+10*y+34)"), ("(x - 3)^2",)),
    MatrixCase("pure_coordinate_geometry", "point-gradient line rearrange", ("--alg", "solve(y-4=3*(x-2),y)"), ("y = -(- 3*(x - 2) - 4)",)),
    MatrixCase("pure_coordinate_geometry", "point-slope formula rearrange", ("--alg", "solve(y-y1=m*(x-x1),y)"), ("y = -(- y1 - m*(x - x1))",)),
    MatrixCase("pure_coordinate_geometry", "perpendicular gradient relation", ("--alg", "solve(m1*m2=-1,m2)"), ("m2 = -1/m1",)),
    MatrixCase("pure_sequences", "arithmetic sequence formula", ("--alg", "solve(5=2+(4-1)*d,d)"), ("-3*d = -3", "d = 1")),
    MatrixCase("pure_sequences", "arithmetic series formula", ("--alg", "solve(35=5/2*(2*3+(5-1)*d),d)"), ("-10*d = -20", "d = 2")),
    MatrixCase("pure_sequences", "geometric infinite sum formula", ("--alg", "solve(20=5/(1-r),r)"), ("Domain:", "r = 3/4")),
    MatrixCase("pure_sequences", "binomial expansion with validity", ("--alg", "binomial((1+x)^5,x,0,5)"), ("Valid for", "x^5")),
    MatrixCase("pure_sequences", "negative binomial validity", ("--alg", "binomial((1+x)^(-1/2),x,0,4)"), ("C(n,2)", "Valid for abs(x) < 1")),
    MatrixCase("pure_trigonometry", "area formula with sine", ("--trig", "30=1/2*6*10*sin(C),C,0,pi,8"), ("sin(C) = 1", "C = [pi/2]")),
    MatrixCase("pure_trigonometry", "sine rule rearrange", ("--alg", "solve(a/sin(A)=b/sin(B),a)"), ("Domain: sin(A) != 0", "a = b/sin(B)/(1/sin(A))")),
    MatrixCase("pure_trigonometry", "R-form trig solve", ("--trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,10,method=rform"), ("R =", "arccos(2/5)")),
    MatrixCase("pure_trigonometry", "radian solve with numeric bounds", ("--trig", "2-cos(3*t/4)=3,t,0,20,10,method=rad"), ("3*t/4 = alpha + 2*pi*n", "t = [4*pi/3, 4*pi]")),
    MatrixCase("pure_trigonometry", "double-angle sine identity", ("--alg", "compare(sin(2*x),2*sin(x)*cos(x))"), ("equivalent",)),
    MatrixCase("pure_trigonometry", "double-angle cosine identity", ("--alg", "compare(cos(2*x),1-2*sin(x)^2)"), ("equivalent",)),
    MatrixCase("pure_trigonometry", "sector arc formula subpart", ("--alg", "solve(s=r*theta,theta)"), ("theta = s/r",)),
    MatrixCase("pure_explogs", "log laws and domain", ("--alg", "solve(log(2,x-1)+log(2,x+3)=3,x,method=log_exp)"), ("Domain:", "(x - 1)*(x + 3) - 8 = 0", "rejected by domain")),
    MatrixCase("pure_explogs", "change of base", ("--alg", "compare(log(a,b),ln(b)/ln(a))"), ("equivalent",)),
    MatrixCase("pure_explogs", "exact exponential log coefficient", ("--alg", "solve(4=2*e^(3*k*ln(16)),k)"), ("3*k*ln(16) = ln(2)", "k = 1/12")),
    MatrixCase("pure_differentiation", "chain rule", ("--derive", "sin((x+1)^2),x,method=chain"), ("u = (x + 1)^2", "du/dx = 2*(x + 1)", "dy/dx = cos(u)*du/dx")),
    MatrixCase("pure_differentiation", "symbolic power rule", ("--derive", "x^n,x"), ("d/dx(x^n) = n*x^(n-1)", "dy/dx = n*x^(n - 1)")),
    MatrixCase("pure_differentiation", "tan derivative", ("--derive", "tan(x),x"), ("dy/dx = sec(x)^2",)),
    MatrixCase("pure_differentiation", "ln derivative", ("--derive", "ln(x),x"), ("dy/dx = 1/x",)),
    MatrixCase("pure_differentiation", "second derivative test subpart", ("--derive", "x^3-3*x,x,method=second"), ("Differentiate once", "d2y/dx2 = 6*x")),
    MatrixCase("pure_differentiation", "parametric derivative", ("--derive", "t^2+1,t^3-3*t,t,x,method=param"), ("dx/dt = 2*t", "dy/dx = (3*t^2 - 3)/(2*t)")),
    MatrixCase("pure_integration", "DI/table integration", ("--int", "x^2*e^(2*x),method=di"), ("D:", "I:", "Signs:")),
    MatrixCase("pure_integration", "reciprocal integral", ("--int", "1/x"), ("Int 1/x dx = ln(abs(x)) + C",)),
    MatrixCase("pure_integration", "basic sine integral", ("--int", "sin(x)"), ("-cos(x) + C",)),
    MatrixCase("pure_integration", "definite integral evaluation", ("--int", "defint(x^2,x,1,3)"), ("F(3) - F(1)", "26/3")),
    MatrixCase("pure_integration", "area between curves setup", ("--int", "area_between(x,x^2,x,0,1)"), ("A = Int(upper-lower) dx", "1/6")),
    MatrixCase("pure_integration", "volume of revolution setup", ("--int", "volume_x(4-x^2,x,-2,2)"), ("V = pi*Int(y^2) dx", "512*pi/15")),
    MatrixCase("pure_numerical_methods", "Newton-Raphson iteration", ("--alg", "newton(x^2-2,x,1,3)"), ("x_(n+1) = x_n - f(x_n)/f'(x_n)", "x = 1.414")),
    MatrixCase("pure_numerical_methods", "non-polynomial Newton iteration", ("--alg", "newton(cos(x)-x,x,1,4)"), ("x_(n+1) = x_n - f(x_n)/f'(x_n)", "x = 0.739")),
    MatrixCase("pure_vectors", "vector magnitude subpart", ("--alg", "sqrt(1^2+2^2+3^2)"), ("sqrt(14)",), False),
    MatrixCase("stats_sampling_data", "summary statistics", ("--stats", "stats(1,2,2,3,5,8)"), ("mean", "Sxx"), False),
    MatrixCase("stats_sampling_data", "variance and sd formula lines", ("--stats", "stats(2,4,4,10)"), ("var = Sxx/n", "sd = sqrt(var)")),
    MatrixCase("stats_probability", "binomial probability", ("--stats", "binom(10,.5,4)"), ("X ~ B", "P(X = 4)")),
    MatrixCase("stats_probability", "addition rule rearrange", ("--alg", "solve(PAorB=PA+PB-PAandB,PAandB)"), ("PAandB = -(PAorB - (PA + PB))",)),
    MatrixCase("stats_probability", "conditional probability rearrange", ("--alg", "solve(pagivenb=pab/pb,pab)"), ("Domain: pb != 0", "pab = -pagivenb/(-1/pb)")),
    MatrixCase("stats_probability", "independent probability rearrange", ("--alg", "solve(pab=pa*pb,pa)"), ("pa = pab/pb",)),
    MatrixCase("stats_distributions", "normal distribution probability", ("--stats", "normalcdf(0,1,-1,1)"), ("z1 =", "Phi")),
    MatrixCase("stats_distributions", "normal standardisation", ("--stats", "normalcdf(100,15,85,115)"), ("Z = (X-100)/15", "Phi(1) - Phi(-1)")),
    MatrixCase("stats_hypothesis", "z-test hypothesis subpart", ("--stats", "ztest(5.4,5,1.2,36,0.05,gt)"), ("H0:", "p = P(Z >")),
    MatrixCase("mechanics_kinematics", "SUVAT calculation", ("--suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=suvat"), ("s = 25",)),
    MatrixCase("mechanics_kinematics", "SUVAT formula rearrange", ("--alg", "solve(v^2=u^2+2*a*s,s)"), ("s = -(v^2 - u^2)/(-2*a)",)),
    MatrixCase("mechanics_forces", "Newton second law algebra subpart", ("--alg", "solve(3*a=12-6,a,method=linear)"), ("a = 2",)),
    MatrixCase("mechanics_forces", "Newton second law rearrange", ("--alg", "solve(F=m*a,a)"), ("a = F/m",)),
    MatrixCase("mechanics_moments", "moment equilibrium algebra subpart", ("--alg", "solve(2*R=3*5,R,method=linear)"), ("R = 15/2",)),
    MatrixCase("mechanics_energy_power", "kinetic energy algebra subpart", ("--alg", "solve(1/2*2*v^2=100,v)"), ("v = +/-10", "v = [10, -10]")),
    MatrixCase("mechanics_energy_power", "power formula rearrange", ("--alg", "solve(P=F*v,v)"), ("v = P/F",)),
    MatrixCase("mechanics_energy_power", "impulse momentum rearrange", ("--alg", "solve(I=m*v-m*u,v)"), ("v = (I + m*u)/m",)),
)


def has(text: str, marker: str) -> bool:
    return markers_present(text, [marker])


def run(case: MatrixCase) -> tuple[str, str, list[str]]:
    proc = subprocess.run([str(HOST), *case.cmd], cwd=REPO, text=True, capture_output=True, timeout=20)
    out = proc.stdout + proc.stderr
    why: list[str] = []
    if proc.returncode:
        why.append(f"returncode={proc.returncode}")
    why.extend(f"missing:{m}" for m in case.must if m != "Answer:" and not has(out, m))
    why.extend(f"forbidden:{m}" for m in FORBID if m.lower() in out.lower())
    if case.working_expected and not has_strong_output(out):
        why.append("weak-output")
    if case.working_expected and len([ln for ln in out.splitlines() if ln.strip()]) < 2:
        why.append("answer-only")
    return ("pass" if not why else "fail"), out, why


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--fail-on-gap", action="store_true")
    ap.add_argument("--report-json", action="store_true")
    args = ap.parse_args()

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    counts = {"pass": 0, "fail": 0}
    rows: list[dict] = []
    lines = ["A-level syllabus capability matrix", ""]
    for case in CASES:
        status, out, why = run(case)
        counts[status] += 1
        rows.append({"topic": case.topic, "capability": case.capability, "status": status, "cmd": list(case.cmd), "why": why})
        print(status.upper(), case.topic, case.cmd[1])
        if status != "pass":
            lines.append(f"FAIL {case.topic}: {case.capability}")
            lines.append("cmd: " + " ".join(case.cmd))
            lines.append("why: " + "; ".join(why))
            lines.extend("  " + ln for ln in out.splitlines()[:12])
            lines.append("")
    topics = {c.topic for c in CASES}
    lines.append(f"topics={len(topics)} cases={len(CASES)} pass={counts['pass']} fail={counts['fail']}")
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    if args.report_json:
        JSON_REPORT.write_text(json.dumps({"counts": counts, "cases": rows}, indent=2) + "\n", encoding="utf-8")
    print(REPORT)
    print(counts)
    return 1 if args.fail_on_gap and counts["fail"] else 0


if __name__ == "__main__":
    raise SystemExit(main())

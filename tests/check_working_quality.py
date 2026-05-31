#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

QUALITY_CASES = [
    (
        "int(2*x+3,x)",
        4,
        [
            "Integrate term by term:",
            "Use int(a*x) dx = a*x^2/2",
            "Answer: x^2 + 3*x + C",
        ],
    ),
    (
        "integrate(2*x+3,x)",
        4,
        [
            "Integrate term by term:",
            "Use int(a*x) dx = a*x^2/2",
            "Answer: x^2 + 3*x + C",
        ],
    ),
    (
        "int(cos(x),x)",
        4,
        [
            "Use formula: int(cos(x)) dx = sin(x) + C",
            "d/dx sin(x)=cos(x)",
            "Answer: sin(x) + C",
        ],
    ),
    (
        "int(sec(x)^2,x)",
        4,
        [
            "Use formula: int(sec(x)^2) dx = tan(x) + C",
            "d/dx tan(x)=sec(x)^2",
            "Answer: tan(x) + C",
        ],
    ),
    (
        "int(tan(3*x),x)",
        5,
        [
            "Use formula: int(tan(k*x)) dx = 1/k*ln(abs(sec(k*x))) + C",
            "k = 3",
            "int(tan(3*x)) dx = 1/3*ln(abs(sec(3*x))) + C",
            "Answer: 1/3*ln(abs(sec(3*x))) + C",
        ],
    ),
    (
        "int(cosec(2*x),x)",
        5,
        [
            "Use formula: int(cosec(k*x)) dx = -1/k*ln(abs(cosec(k*x)+cot(k*x))) + C",
            "k = 2",
            "int(cosec(2*x)) dx = -1/2*ln(abs(cosec(2*x)+cot(2*x))) + C",
            "Answer: -1/2*ln(abs(cosec(2*x)+cot(2*x))) + C",
        ],
    ),
    (
        "integrate(exp(3*x),x)",
        5,
        [
            "Use formula: int(e^(k*x)) dx = 1/k*e^(k*x) + C",
            "k = 3",
            "int(e^(3*x)) dx = 1/3*e^(3*x) + C",
            "Answer: 1/3*e^(3*x) + C",
        ],
    ),
    (
        "integrate(x*exp(x),x)",
        6,
        [
            "Use integration by parts",
            "Let u=x, v'=e^x",
            "int(x*e^x) dx = x*e^x - int(e^x) dx",
            "Answer: x*e^x - e^x + C",
        ],
    ),
    (
        "integrate(2*x*cos(x^2),x)",
        5,
        [
            "Substitute u = x^2",
            "du/dx = 2*x",
            "int(2*x*cos(x^2)) dx = int(cos(u)) du",
            "Answer: sin(x^2) + C",
        ],
    ),
    (
        "integrate(sin(x),x,0,pi)",
        5,
        [
            "Antiderivative of sin(x) is -cos(x)",
            "F(pi) = -cos(pi) = 1",
            "F(0) = -cos(0) = -1",
            "Answer: 2",
        ],
    ),
    (
        "int(1/x,x)",
        4,
        [
            "x != 0",
            "d/dx ln(abs(x))=1/x",
            "Answer: ln(abs(x)) + C",
        ],
    ),
    (
        "range(x^2+4*x+7)",
        4,
        [
            "vertex",
            "x = -2",
            "minimum y = 3",
            "Answer: y >= 3",
        ],
    ),
    (
        "diff((x^2)*tan(y)=9,x)",
        4,
        [
            "2*x*tan(y)+x^2*sec(y)^2*(dy)/(dx)=0",
            "tan(y)=9/x^2 and sec(y)^2=1+tan(y)^2",
            "(dy)/(dx)=(-18x)/(x^4+81)",
        ],
    ),
    (
        "diff([x=t^2,y=t^3],t)",
        6,
        [
            "Parametric differentiation:",
            "dx/dt = 2*t",
            "dy/dt = 3*t^2",
            "(dy)/(dx) = (dy/dt)/(dx/dt)",
            "(dy)/(dx) = 3*t/2",
        ],
    ),
    (
        "diff([x=t^2,y=t^3],t,2)",
        7,
        [
            "dx/dt = 2*t",
            "dy/dt = 3*t^2",
            "(dy)/(dx) = 3*t/2",
            "At t = 2, (dy)/(dx) = 3",
        ],
    ),
    (
        "solve(k*(k+3)/(k+1)=2,k)",
        6,
        [
            "Domain: k + 1 != 0 => k != -1",
            "Multiply by k + 1",
            "(k+2)(k-1)=0",
            "k = [1, -2]",
        ],
    ),
    (
        "solve(10^(3*k)=2,k)",
        5,
        [
            "Take ln of both sides.",
            "3*k*ln(10) = ln(2)",
            "Answer: k = [ln(2)/(3*ln(10))]",
        ],
    ),
    (
        "solve(log(2,x)=3,x)",
        5,
        [
            "Use definition: log_a(x)=b means x=a^b",
            "x = 2^3",
            "Answer: x = [8]",
        ],
    ),
    (
        "solve(dn/dt=k*n,n,t)",
        8,
        [
            "Separate variables:",
            "1/n dn = k dt",
            "ln(abs(n)) = k*t + C",
            "n = A*e^(k*t)",
        ],
    ),
    (
        "range(x/(x^2+4),x)",
        5,
        [
            "Let y=x/(x^2+4)",
            "y*x^2 - x + 4*y = 0",
            "Discriminant >= 0: 1 - 16*y^2 >= 0",
            "-1/4 <= y <= 1/4",
        ],
    ),
    (
        "range(sqrt(9-x^2),x)",
        4,
        [
            "Square-root output is non-negative",
            "Inside root has maximum 9 at x = 0",
            "0 <= y <= 3",
        ],
    ),
    (
        "range(x^2-4*x+5,x,0,5)",
        6,
        [
            "Interval: 0 <= x <= 5",
            "f(0) = 5",
            "f(5) = 10",
            "vertex x = 2 is inside the interval",
            "Answer: 1 <= y <= 10",
        ],
    ),
    (
        "domain(log(10,4-x),x)",
        4,
        [
            "Log argument must be positive",
            "4 - x > 0",
            "Answer: x < 4",
        ],
    ),
    (
        "domain(1/(x^2-1),x)",
        5,
        [
            "Denominator must be non-zero",
            "x^2 - 1 != 0",
            "(x-1)(x+1) != 0",
            "Answer: x != -1 and x != 1",
        ],
    ),
    (
        "limit((x^2-1)/(x-1),x=1)",
        6,
        [
            "Factor numerator: x^2-1 = (x-1)(x+1)",
            "Cancel x-1",
            "Limit becomes x+1",
            "Substitute x=1",
            "Answer: 2",
        ],
    ),
    (
        "xform(log(2,x),ln(x)/ln(2))",
        5,
        [
            "Use change of base: log_a(x)=ln(x)/ln(a)",
            "Here a = 2",
            "So log_a(x) becomes ln(x)/ln(2)",
            "Answer: ln(x)/ln(2)",
        ],
    ),
    (
        "xform(log(2,x^2),2*log(2,x))",
        5,
        [
            "Use log power law: log_a(u^n)=n*log_a(u)",
            "Here a = 2",
            "u = x, n = 2",
            "x > 0",
            "Answer: 2*log(2,x)",
        ],
    ),
    (
        "xform(3*x^2+12*x+25,3*(x+2)^2+13)",
        4,
        [
            "Complete the square:",
            "3*x^2 + 12*x + 25",
            "3*(x + 2)^2 + 13",
            "Answer: 3*(x + 2)^2 + 13",
        ],
    ),
    (
        "xform((sin(x)-cos(x)+1)/(sin(x)+cos(x)-1),sec(x)+tan(x))",
        6,
        [
            "Put t=tan(x/2)",
            "sin(x)=2*t/(1+t^2), cos(x)=(1-t^2)/(1+t^2)",
            "(sin(x)-cos(x)+1)/(sin(x)+cos(x)-1) = (1+t)/(1-t)",
            "sec(x)+tan(x) = (1+t)/(1-t)",
        ],
    ),
    (
        "xform(sin(x)+2*cos(x),R*sin(x+alpha))",
        7,
        [
            "Expand target: R*sin(x+alpha)=R*sin(x)*cos(alpha)+R*cos(x)*sin(alpha)",
            "Compare coefficients:",
            "R*cos(alpha)=1",
            "R*sin(alpha)=2",
            "R = sqrt(1^2+2^2) = sqrt(5)",
            "alpha = atan(2)",
            "Answer: sqrt(5)*sin(x+atan(2))",
        ],
    ),
    (
        "series(cos(theta),theta=0,3)",
        5,
        [
            "Small-angle approximation:",
            "theta must be in radians and close to 0",
            "cos(theta) = 1 - theta^2/2 + ...",
            "Answer: 1 - theta^2/2",
        ],
    ),
    (
        "binomial((1+8*x)^(1/2),x,0,3)",
        5,
        [
            "(1+u)^n = 1 + n*u + n*(n-1)*u^2/2! + n*(n-1)*(n-2)*u^3/3!",
            "u = 8*x, n = 1/2",
            "Simplified terms: T0 = 1, T1 = 4*x, T2 = -8*x^2, T3 = 32*x^3",
            "Answer: 1 + 4*x - 8*x^2 + 32*x^3",
        ],
    ),
    (
        "partfrac((50*x^2+38*x+9)/((5*x+2)^2*(1-2*x)))",
        5,
        [
            "A/(5*x + 2)+B/(5*x + 2)^2+C/(- 2*x + 1)",
            "B = 1, C = 2",
            "Compare x^2 coefficients: A = 0",
            "1/(5*x + 2)^2 + 2/(- 2*x + 1)",
        ],
    ),
    (
        "partfrac((3*x+5)/(x^2+x-2),x)",
        6,
        [
            "x^2+x-2 = (x+2)(x-1)",
            "A/(x+2)+B/(x-1)",
            "3*x+5 = A*(x-1)+B*(x+2)",
            "A = 1/3, B = 8/3",
            "Answer: 1/(3*(x+2)) + 8/(3*(x-1))",
        ],
    ),
]


def nonblank_lines(text: str) -> list[str]:
    return [line.strip() for line in text.splitlines() if line.strip()]


def main() -> int:
    bad = []
    for expr, min_lines, markers in QUALITY_CASES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        lines = nonblank_lines(out)
        missing = [marker for marker in markers if marker not in out]
        if proc.returncode or len(lines) < min_lines or missing:
            bad.append((expr, proc.returncode, len(lines), missing, out))
    if bad:
        for expr, code, line_count, missing, out in bad:
            print(f"FAIL {expr}: rc={code} lines={line_count} missing={missing}")
            print(out)
        return 1
    print(f"OK working quality cases={len(QUALITY_CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

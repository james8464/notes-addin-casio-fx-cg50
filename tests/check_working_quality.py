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
        "suvat(v=20,a=2,s=48,target=u)",
        5,
        [
            "v^2 = u^2 + 2as",
            "u^2 = 20^2 - 2*2*48 = 208",
            "u = sqrt(208) or -sqrt(208)",
        ],
    ),
    (
        "suvat(s=45,v=20,a=5,target=t)",
        5,
        [
            "s = vt - 1/2at^2",
            "t = (v +/- sqrt(v^2 - 2as))/a",
            "v^2 - 2as = 20^2 - 2*5*45 = -50",
            "t = (no positive root)",
        ],
    ),
    (
        "suvat(u=5,a=3,t=4,target=v)",
        4,
        [
            "v = u + at",
            "v = 5 + 3*4",
            "Answer: 17",
        ],
    ),
    (
        "suvat(u=4,v=16,t=3,target=a)",
        5,
        [
            "v = u + at",
            "a = (v-u)/t",
            "a = (16 - 4)/3",
            "Answer: 4",
        ],
    ),
    (
        "suvat(u=4,v=16,t=3,target=s)",
        4,
        [
            "s = (u+v)t/2",
            "s = (4 + 16)*3/2",
            "Answer: 30",
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

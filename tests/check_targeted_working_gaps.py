#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("integrate(2*x+3,x)", ["Integrate term by term:", "Use int(a*x^n)", "Answer: x^2 + 3*x + C"]),
    ("integrate(9x)", ["Use int(a*x^n)", "int(9x) dx", "Answer: 9/2*x^2 + C"]),
    ("integrate(x*exp(x),x)", ["Use integration by parts", "Let u=x, dv=e^x dx", "du=dx, v=e^x"]),
    ("integrate(x*cos(x),x)", ["Use integration by parts", "Let u=x, dv=cos(x) dx", "du=dx, v=sin(x)"]),
    ("integrate((ln(x))^2)", ["Let u = ln(x)^2, dv = dx", "Let u = ln(x), dv = dx"]),
    ("integrate(2*x*cos(x^2),x)", ["Sub u=x^2", "du=2*x dx", "Answer: sin(x^2) + C"]),
    ("integrate(2*x*sin(x^2),x)", ["Sub u=x^2", "Answer: -cos(x^2) + C"]),
    ("integrate(x*(3+x^2)^4,x)", ["Sub u=3 + x^2", "scale 1/2", "Answer: 1/10*(3 + x^2)^5 + C"]),
    ("integrate((2*x+1)*cos(x^2+x),x)", ["Sub u=x^2 + x", "du=(2*x + 1) dx", "Answer: sin(x^2 + x) + C"]),
    ("integrate((6*x-5)*exp(3*x^2-5*x),x)", ["Sub u=3x^2 - 5x", "du=(6*x - 5) dx", "Answer: exp(3x^2 - 5x) + C"]),
    ("simplify((x^2+3*x+2)/(x+1))", ["Factorise numerator and denominator:", "x^2 + 3*x + 2 = (x + 1)*(x + 2)", "Cancel common factor (x + 1)", "Answer: x + 2"]),
    ("simplify((x^2-4)/(x^2-2*x))", ["x^2 - 4 = (x - 2)*(x + 2)", "x^2 - 2*x = (x - 2)*(x)", "Cancel common factor (x - 2)", "Answer: (x + 2)/(x)"]),
    ("simplify((2*x^2+6*x+4)/(2*x+2))", ["Cancel common factor (x + 1)", "Answer: x + 2"]),
    ("simplify((x^2-1)/(1-x))", ["Cancel common factor (x - 1)", "Answer: -(x + 1)"]),
    ("simplify((x^2+2*x+1)/(x+1)^2)", ["(x + 1)*(x + 1)", "Cancel common factor (x + 1)", "Answer: 1"]),
    ("simplify((x^2-1)/(x+1)^2)", ["x^2 - 1 = (x + 1)*(x - 1)", "Cancel common factor (x + 1)", "Answer: (x - 1)/(x + 1)"]),
]


def main() -> int:
    bad = []
    for expr, markers in CASES:
        p = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (p.stdout or "") + (p.stderr or "")
        missing = [m for m in markers if m not in out]
        if p.returncode or missing:
            bad.append((expr, p.returncode, missing, out[:600]))
    for expr, code, missing, out in bad:
        print(f"FAIL {expr!r} code={code} missing={missing}\n{out}")
    if bad:
        return 1
    print(f"OK targeted working gaps={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

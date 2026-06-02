#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("integrate(2*x+3,x)", ["Integrate term by term:", "Use int(a*x^n)", "x^2 + 3*x + C"]),
    ("integrate(9x)", ["Use int(a*x^n)", "int(9x) dx", "9/2*x^2 + C"]),
    ("integrate((x)+(3),x)", ["Integrate term by term:", "1/2*x^2 + 3*x + C"]),
    ("integrate(((x)^2)+(3*x)+(2),x)", ["Termwise:", "1/3*x^3 + 3/2*x^2 + 2*x + C"]),
    ("integrate(x*exp(x),x)", ["Use integration by parts", "Let u=x, dv=e^x dx", "du=dx, v=e^x"]),
    ("integrate(x*cos(x),x)", ["Use integration by parts", "Let u=x, dv=cos(x) dx", "du=dx, v=sin(x)"]),
    ("integrate((ln(x))^2)", ["Let u = ln(x)^2, dv = dx", "Let u = ln(x), dv = dx"]),
    ("integrate(2*x*cos(x^2),x)", ["Sub u=x^2", "du=2*x dx", "sin(x^2) + C"]),
    ("integrate(2*x*sin(x^2),x)", ["Sub u=x^2", "-cos(x^2) + C"]),
    ("integrate(x*(3+x^2)^4,x)", ["Sub u=3 + x^2", "scale 1/2", "1/10*(3 + x^2)^5 + C"]),
    ("integrate((2*x+1)*cos(x^2+x),x)", ["Sub u=x^2 + x", "du=(2*x + 1) dx", "sin(x^2 + x) + C"]),
    ("integrate((6*x-5)*exp(3*x^2-5*x),x)", ["Sub u=3x^2 - 5x", "du=(6*x - 5) dx", "exp(3x^2 - 5x) + C"]),
    ("integrate(tan(x))", ["tan(x)=sin(x)/cos(x)", "Sub u=cos(x)", "ln(abs(sec(x))) + C"]),
    ("integrate(x^2*ln(x),x)", ["Use integration by parts:", "u=ln(x), dv=x^2 dx", "v=1/3*x^3", "1/3*x^3*ln(x) - 1/9*x^3 + C"]),
    ("integrate(3*x^2*ln(x),x)", ["Use integration by parts:", "u=ln(x), dv=3*x^2 dx", "v=x^3", "x^3*ln(x) - 1/3*x^3 + C"]),
    ("integrate((x+1)/(x^2+2*x+3)^3,x)", ["Sub u=x^2 + 2*x + 3", "x + 1 = 1/2*(2*x + 2)", "-1/4*(x^2 + 2*x + 3)^-2 + C"]),
    ("integrate(x^2+sin(2*x)+exp(3*x),x)", ["Integrate term by term:", "int(x^2) dx = 1/3*x^3", "int(sin(2*x)) dx = -1/2*cos(2*x)", "int(exp(3*x)) dx = 1/3*exp(3*x)", "1/3*x^3 - 1/2*cos(2*x) + 1/3*exp(3*x) + C"]),
    ("integrate((2*x+7)+cos(6*x),x)", ["Integrate term by term:", "int(2*x+7) dx = x^2 + 7*x", "int(cos(6*x)) dx = 1/6*sin(6*x)", "x^2 + 7*x + 1/6*sin(6*x) + C"]),
    ("integrate(exp(ln(x)),x,3)", ["exp(ln(x)) = x", "1/2*x^2 + C"]),
    ("integrate(exp(-4))", ["no x: int(a)=a*x+C", "0.01831563889*x + C"]),
    ("integrate((x)*(((x)^2)^5))", ["Expand into powers of x:", "x^10*(x) = x^11", "1/12*x^12 + C"]),
    ("integrate(((x)^2)((4)^2))", ["Combine powers of x:", "16*x^2", "16/3*x^3 + C"]),
    ("integrate(x^-1,x,3)", ["Rewrite using reciprocal form:", "x^-1 = 1/x", "Use int(1/x)", "ln(abs(x)) + C"]),
    ("range(-9)", ["Range:", "constant", "y = -9"]),
    ("range(2*x+3)", ["Range:", "linear", "all real"]),
    ("range((x)(1))", ["Range:", "linear", "all real y"]),
    ("range((x)^3)", ["Range:", "Odd power", "all real y"]),
    ("range(x^2+4*x+7)", ["vertex x = -2", "minimum y = 3", "y >= 3"]),
    ("range(-x^2+4*x+1)", ["vertex x = 2", "maximum y = 5", "y <= 5"]),
    ("range(exp(x))", ["exp(x) > 0", "y > 0"]),
    ("range(exp(3))", ["constant y = 20.08553692319"]),
    ("range(ln(x))", ["x > 0", "all real y"]),
    ("range(sqrt(x))", ["sqrt(x) >= 0", "y >= 0"]),
    ("range(cos(x)+8)", ["cos(x) is between -1 and 1", "Shift by 8", "7 <= y <= 9"]),
    ("range(2*sin(3*x-1)-5)", ["sin(3*x - 1) is between -1 and 1", "Scale by 2", "Shift by -5", "-7 <= y <= -3"]),
    ("range(x^2-4*x+5,x,0,5)", ["Interval: 0 <= x <= 5", "f(0) = 5", "f(5) = 10", "vertex x = 2 is inside", "1 <= y <= 10"]),
    ("simplify((x^2+3*x+2)/(x+1))", ["Factorise:", "x^2 + 3*x + 2 = (x + 1)*(x + 2)", "Cancel common factor (x + 1)", "x + 2"]),
    ("simplify((x^2-4)/(x^2-2*x))", ["x^2 - 4 = (x - 2)*(x + 2)", "x^2 - 2*x = (x - 2)*(x)", "Cancel common factor (x - 2)", "(x + 2)/(x)"]),
    ("simplify((2*x^2+6*x+4)/(2*x+2))", ["Cancel common factor (x + 1)", "x + 2"]),
    ("simplify((x^2-1)/(1-x))", ["Cancel common factor (x - 1)", "-(x + 1)"]),
    ("simplify((x^2+2*x+1)/(x+1)^2)", ["(x + 1)*(x + 1)", "Cancel common factor (x + 1)", "1"]),
    ("simplify((x^2-1)/(x+1)^2)", ["x^2 - 1 = (x + 1)*(x - 1)", "Cancel common factor (x + 1)", "(x - 1)/(x + 1)"]),
    ("solve(0=(10-0.4x)*ln(x+1),x)", ["Solve by zero-product rule:", "10 - 2/5*x = 0", "ln(x + 1) = 0", "Domain: x + 1 > 0", "x = [0, 25]"]),
    ("solve((3-0.5*x)*ln(2*x-1)=0,x)", ["Solve by zero-product rule:", "3 - 1/2*x = 0", "ln(2*x - 1) = 0", "Domain: 2*x - 1 > 0", "x = [1, 6]"]),
    ("solve(x=x+6,x)", ["Collect like terms:", "0 = 6", "x = []"]),
    ("solve(x=-2.7,x)", ["x = [-27/10]"]),
    ("solve(5=(x)^2,x)", ["x^2 = 5", "x = [-sqrt(5), sqrt(5)]"]),
    ("solve(x=(((-5)/7)-sqrt(9))^3,x)", ["x = [-51.24198250729]"]),
    ("solve(x=x,x)", ["Collect like terms:", "0 = 0", "x = all real"]),
    ("solve(tan(-6)=-5,x)", ["No x term appears.", "x = []"]),
    ("solve(tan(x)=4/5,x)", ["tan(x) = 4/5", "x = atan(4/5) + n*pi"]),
    ("diff((10-0.4x)*ln(x+1))", ["Product rule:", "u = 10 - 2/5*x", "v = ln(x + 1)", "du/dx = -2/5", "dv/dx = 1/(x + 1)", "-2/5*ln(x + 1) + (10 - 2/5*x)/(x + 1)"]),
    ("diff((2*x+1)*ln(3*x-2),x)", ["Product rule:", "u = 2*x + 1", "v = ln(3*x - 2)", "du/dx = 2", "dv/dx = 3/(3*x - 2)", "2*ln(3*x - 2) + (2*x + 1)*(3/(3*x - 2))"]),
    ("diff((x^2+1)/(x-1),x)", ["Quotient rule:", "u = x^2 + 1", "v = x - 1", "du/dx = 2*x", "dv/dx = 1", "((2*x)*(x - 1) - (x^2 + 1))/(x - 1)^2"]),
    ("diff((((x)^-1)/(x))^2,x)", ["Chain rule:", "Let u = x^-1/x", "du/dx = ((-x^-2)*(x) - (x^-1))/(x)^2", "2*(x^-1/x)*(((-x^-2)*(x) - (x^-1))/(x)^2)"]),
    ("diff(9/x,x)", ["Rewrite using negative powers:", "9/x = 9*x^-1", "Use d/dx(a*x^n)=a*n*x^(n-1)", "-9*x^-2"]),
    ("diff(sin(x)+x^2,x)", ["Differentiate term by term:", "d/dx(sin(x)) = cos(x)", "d/dx(x^2) = 2*x", "cos(x) + 2*x"]),
    ("diff(cos(x+3)-x,x)", ["Differentiate term by term:", "d/dx(cos(x + 3)) = -sin(x + 3)", "d/dx(-x) = -1", "-sin(x + 3) - 1"]),
    ("diff((sin(x))^3,x)", ["Chain rule:", "Let u = sin(x)", "du/dx = cos(x)", "3*(sin(x))^2*cos(x)"]),
    ("diff(ln(((x)-(6))*((x)^2)),x)", ["Use log law:", "d/dx ln(x - 6) = 1/(x - 6)", "d/dx ln(x^2) = 2*x/(x^2)", "1/(x - 6) + 2*x/(x^2)"]),
    ("diff((sin(x))*(7-x)-x*x,x)", ["Differentiate term by term:", "d/dx(sin(x)*(7 - x))", "(7 - x)*cos(x) - sin(x)", "-2*x"]),
    ("diff(ln((x+1)^2),x)", ["Chain rule:", "d/dx ln((x + 1)^2)", "inner derivative = 2*(x + 1)", "2/(x + 1)"]),
    ("diff(sin((x+1)^2),x)", ["Chain rule:", "Let u = (x + 1)^2", "du/dx = 2*(x + 1)", "2*(x + 1)*cos((x + 1)^2)"]),
    ("diff(cos(3*x^2-2*x),x)", ["Chain rule:", "Let u = 3*x^2 - 2*x", "du/dx = 6*x - 2", "-(6*x - 2)*sin(3*x^2 - 2*x)"]),
    ("diff(exp((2*x-1)^3),x)", ["Chain rule:", "Let u = (2*x - 1)^3", "du/dx = 6*(2*x - 1)^2", "6*(2*x - 1)^2*exp((2*x - 1)^3)"]),
    ("diff((x^2+1)*sin(3*x-2),x)", ["Product rule:", "u = x^2 + 1", "v = sin(3*x - 2)", "du/dx = 2*x", "dv/dx = 3*cos(3*x - 2)", "2*x*sin(3*x - 2) + (x^2 + 1)*3*cos(3*x - 2)"]),
    ("diff((x^2-1)*cos((x+1)^2),x)", ["Product rule:", "u = x^2 - 1", "v = cos((x + 1)^2)", "dv/dx = -2*(x + 1)*sin((x + 1)^2)", "2*x*cos((x + 1)^2) - (x^2 - 1)*2*(x + 1)*sin((x + 1)^2)"]),
    ("diff((x+1)(x-2)ln(x),x)", ["Product rule:", "f1 = x + 1", "f2 = x - 2", "f3 = ln(x)", "(x - 2)*ln(x) + (x + 1)*ln(x) + (x + 1)*(x - 2)/(x)"]),
    ("diff(x^2*ln(x)*sin(x),x)", ["Product rule:", "f1 = x^2", "f2 = ln(x)", "f3 = sin(x)", "f1' = 2*x", "f2' = 1/(x)", "f3' = cos(x)", "2*x*ln(x)*sin(x) + x^2*sin(x)/(x) + x^2*ln(x)*cos(x)"]),
    ("diff((x+1)*(x-2)*ln(x),x)", ["Product rule:", "f1 = x + 1", "f2 = x - 2", "f3 = ln(x)", "f1' = 1", "f2' = 1", "f3' = 1/(x)", "(x - 2)*ln(x) + (x + 1)*ln(x) + (x + 1)*(x - 2)/(x)"]),
    ("integrate((x+1)/(x^2+2*x+3),x)", ["Sub u=x^2 + 2*x + 3", "du=2*x + 2 dx", "x + 1 = 1/2*(2*x + 2)", "1/2*ln(abs(x^2 + 2*x + 3)) + C"]),
    ("integrate((x^2+2*x+1)/(x+1),x)", ["Simplify before integrating:", "x^2 + 2*x + 1 = (x + 1)*(x + 1)", "Cancel common factor (x + 1)", "1/2*x^2 + x + C"]),
    ("integrate((x^2-1)/(x+1),x)", ["Simplify before integrating:", "x^2 - 1 = (x + 1)*(x - 1)", "Cancel common factor (x + 1)", "1/2*x^2 - x + C"]),
    ("expand((3*x-2)*(x+5))", ["3*x^2 + 13*x - 10"]),
    ("expand((2*x+3)^2)", ["4*x^2 + 12*x + 9"]),
    ("apart(6/(u(3+2u)))", ["A/(u)+B/(2*u + 3)", "A = 6/3 = 2", "B = 6/-3/2 = -4", "2/(u) - 4/(2*u + 3)"]),
    ("binomial((1-6*x)^(1/2))", ["u = -6*x", "C0=1,C1=1/2,C2=-1/8,C3=1/16", "Valid for abs(x) < 1/6", "1 - 3*x - 9/2*x^2 - 27/2*x^3"]),
    ("diff((x)+(3),x)", ["dy/dx = 1"]),
    ("diff(6,x)", ["dy/dx = 0"]),
    ("diff(exp(3),x)", ["dy/dx = 0"]),
    ("simplify(((8)))", ["8"]),
    ("simplify(exp(-4))", ["0.01831563889"]),
    ("simplify((x)^2)", ["x^2"]),
    ("simplify(sin(x))", ["sin(x)"]),
    ("simplify(ln(ln(exp(x))))", ["ln(ln(exp(x)))"]),
    ("xform(-7,expand(-7))", ["-7"]),
    ("xform(x,factor(x))", ["x"]),
    ("xform(2*sin(x-60)=cos(x-30),tan(x)=3*sqrt(3))", ["Use angle formulae:", "sin(x-a)=sin(x)*cos(a)-cos(x)*sin(a)", "cos(x-a)=cos(x)*cos(a)+sin(x)*sin(a)", "tan(x)=3*sqrt(3)"]),
    ("xform(2*sin(x-60)-cos(x-30)=0,tan(x)=3*sqrt(3))", ["Use angle formulae:", "2sin(x-60)-cos(x-30) = 0", "tan(x)=3*sqrt(3)"]),
    ("xform(tan(x)^2+1,sec(x)^2)", ["sec(x)^2 = 1 + tan(x)^2", "sec(x)^2"]),
    ("xform(cos(x)^2+sin(x)^2,1)", ["sin(x)^2+cos(x)^2=1", "1"]),
    ("xform(ln(x^3),3*ln(x))", ["ln(u^n)=n*ln(u)", "3*ln(x)"]),
    ("xform(x^2+4*x+4,(x+2)^2)", ["Factorise:", "x^2 + 4*x + 4 = (x + 2)^2", "(x + 2)^2"]),
    ("xform(sin(x)+2cos(x),sqrt(5)*sin(x+atan(2)))", ["R-form:", "R = sqrt(1^2 + 2^2) = sqrt(5)", "alpha = atan(2)", "sqrt(5)*sin(x+atan(2))"]),
]


def main() -> int:
    bad = []
    for expr, markers in CASES:
        p = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (p.stdout or "") + (p.stderr or "")
        missing = [m for m in markers if m not in out]
        lines = [line.strip() for line in out.splitlines() if line.strip()]
        if "Exact:" in out:
            missing.append("no Exact: fallback label")
        if lines and lines[-1].startswith("Answer:"):
            missing.append("final line must be math only")
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

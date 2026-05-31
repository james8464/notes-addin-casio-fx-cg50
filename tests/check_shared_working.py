#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("diff((x^2)*tan(y)=9,x)", "(dy)/(dx)=(-18x)/(x^4+81)"),
    ("diff(sin(x),x)", "Differentiate: sin(x)"),
    ("diff(x^3,x)", "Power rule: d/dx x^n = n*x^(n-1)"),
    ("diff(x^2*sin(x),x)", "Product rule: d(uv)/dx = u*v' + v*u'"),
    ("diff(sin(3*x+1),x)", "Chain rule: u=3*x+1"),
    ("diff(sin(x)/x,x)", "Quotient rule: d(u/v)/dx=(v*u'-u*v')/v^2"),
    ("diff((x-4)/(2+sqrt(x)),x)", "u = sqrt(x); x = u^2; du/dx = 1/(2*sqrt(x))"),
    ("diff(sqrt(x)-2,x)", "d/dx(sqrt(x)) = 1/(2*sqrt(x))"),
    ("diff(4*(x^2-2)*exp(-2*x),x)", "dy/dx = 8*e^(-2*x)*(x - x^2 + 2)"),
    ("diff(tan(3*x),x)", "dy/dx = 3*sec(3*x)^2"),
    ("diff(sec(x),x)", "d/dx sec(x)=sec(x)*tan(x)"),
    ("diff(cosec(x),x)", "d/dx cosec(x)=-cosec(x)*cot(x)"),
    ("int(2*x+3,x)", "Integrate term by term"),
    ("int(sin(x)^2,x)", "sin(x)^2 = (1-cos(2*x))/2"),
    ("int(cos(x),x)", "int(cos(x)) dx = sin(x) + C"),
    ("int(sec(x)^2,x)", "int(sec(x)^2) dx = tan(x) + C"),
    ("int(1/x,x)", "int(1/x) dx = ln(abs(x)) + C"),
    ("int((x^2+6)/x^4,x)", "(x^2+6)/x^4 = x^-2 + 6*x^-4"),
    ("int(9-9/x^2,x)", "Answer: 9*x + 9*x^-1 + C"),
    ("int(x^2-1,x)", "Answer: x^3/3 - x + C"),
    ("defint(ln(x)^2,x,2,4),method=parts", "F(4) = 4*ln(4)^2 - 8*ln(4) + 8"),
    ("integrate(2*x+3,x)", "Integrate term by term"),
    ("solve(x/(x-4)=4,x)", "Answer: x = [16/3]"),
    ("solve(3*k^2-58*k+240=0,k)", "Answer: k = [40/3, 6]"),
    ("solve(3*k^2-58*k+240=0,k,k integer)", "Answer: k = [6]"),
    ("solve(-1/300*x^2+3/5*x+3=0,x)", "x = (3/5 + sqrt(2/5))/(1/150)"),
    ("solve(-1/300*x^2+3/5*x+3=0,x,x>0)", "Answer: x = 90 + 150*sqrt(2/5)"),
    ("solve(k^2+k-2=0,k)", "Answer: k = [1, -2]"),
    ("solve(k^2+k-2=0,k,k!=1)", "Answer: k = [-2]"),
    ("solve(10+3*k=-2,k)", "Answer: k = [-4]"),
    ("solve(24*k^2=12*32*k,k)", "k = 0 or k = 16"),
    ("solve(10*(1.2)^(n-1)>1000,n)", "n integer => n >= 27"),
    ("solve(10^(3*k)=2,k)", "Answer: k = [ln(2)/(3*ln(10))]"),
    ("solve(4^(3*p-1)=5^210,p)", "Answer: p = [(210*ln(5) + 2*ln(2))/(6*ln(2))]"),
    ("solve(1/4-1/x^2>0,x)", "x < -2 or x > 2"),
    ("solve(tan(x)=1/2,x)", "x = 0.463647609001 + n*pi"),
    ("solve([x^2+y^2=100,(x-15)^2+y^2=40],[x,y])", "y = -sqrt(39)/2"),
    ("solve(x^2-1=9*(1-1/x^2),x)", "Answer: x = [-3, -1, 1, 3]"),
    ("solve(54*(x^4-27)/(x^4+81)^2=0,x)", "Answer: x = [-27^(1/4), 27^(1/4)]"),
    ("solve(2+x-x^2=0,x)", "Answer: x = [-1, 2]"),
    ("solve((3*x-7)/(x-2)=7,x)", "Answer: x = [7/4]"),
    ("solve(4*a+13=25,a)", "Answer: a = [3]"),
    ("solve(64000-11200*t=0,t)", "Answer: t = [40/7]"),
    ("range(x^2+4*x+7)", "y >= 3"),
    ("range(x/(x^2+4),x)", "1 - 16*y^2 >= 0"),
    ("range(log(2*x+3),x)", "non-constant linear input covers all positive log arguments"),
    ("range((x^2-1)/(x^2+1))", "-1 <= y < 1"),
    ("range(abs(x-3)+2)", "y >= 2"),
    ("range(sqrt(x-1)+3)", "y >= 3"),
    ("range((x+1)/(x^2+1))", "(1-sqrt(2))/2 <= y <= (1+sqrt(2))/2"),
    ("range(1/(x^2+4*x+5))", "0 < y <= 1"),
    ("diff(log(1/(sqrt(x^2+1)-x)),x)", "Rationalise: 1/(sqrt(x^2+1)-x) = sqrt(x^2+1)+x"),
    ("xform((x+1)^2,x^2+2*x+1)", "normal(((x+1)^2)-(x^2+2*x+1)) = 0"),
    ("xform((sin(x)-cos(x)+1)/(sin(x)+cos(x)-1),sec(x)+tan(x))", "t=tan(x/2)"),
    ("xform(sin(x)^2+cos(x)^2,1)", "sin(x)^2+cos(x)^2 = 1"),
    ("xform(log(2,x),ln(x)/ln(2))", "change of base"),
    ("xform(sec(x)^2,1+tan(x)^2)", "sec(x)^2 = 1 + tan(x)^2"),
    ("xform(cosec(x)^2,1+cot(x)^2)", "cosec(x)^2 = 1 + cot(x)^2"),
    ("xform(cot(x),cos(x)/sin(x))", "cot(x)=cos(x)/sin(x)"),
    ("xform(2*sin(x)*cos(x),sin(2*x))", "sin(2*x)=2*sin(x)*cos(x)"),
    ("sin(x)+2*cos(x),method=rform", "sqrt(5)*sin(x+atan(2))"),
    ("coeff((9/(2*x)-2*x^2/3)^13,x,11)", "Coefficient = 92664"),
    ("binomial((1+8*x)^(1/2),x,0,3)", "1 + 4*x - 8*x^2 + 32*x^3"),
    ("log(2,8)", "Answer: 3"),
    ("compare(4*ln(4)^2 - 2*ln(2)^2 + 4 + 6*ln(1/4),14*ln(2)^2-12*ln(2)+4)", "E1-E2 = 0"),
    ("method=numeric,200*ln(2)*2^(8/5)", "To 2 significant figures: 420"),
    ("method=numeric,1/2*0.5*(0.4805+1.9218+2*(0.8396+1.2069+1.5694))", "To 3 significant figures: 2.41"),
    ("method=numeric,(-atan(2)+pi/2+3)/(pi/12)", "Nearest minute: 13:14"),
    ("method=numeric,2*0.3415^3-4*0.3415^2+7*0.3415-2", "h(0.3415)=0.00366399675 > 0"),
    ("method=numeric,2*0.3405^3-4*0.3405^2+7*0.3405-2", "root rounds to 0.341"),
    ("method=numeric,(3/5+sqrt(2/5))/(1/150)", "Nearest metre: 185"),
    ("[-3,-4,-5]+[1,1,4]", "[-3,-4,-5]+[1,1,4] = (-2,-3,-1)"),
    ("[3,-3,-4]-[2,5,-6]", "[3,-3,-4]-[2,5,-6] = (1,-8,2)"),
    ("2*[1,-8,2]", "2*[1,-8,2] = (2,-16,4)"),
    ("dot([3,4,5],[1,1,4])", "(3,4,5).(1,1,4) = 3*1 + 4*1 + 5*4"),
    ("norm([3,4,5])^2", "norm([3,4,5])^2 = (5*sqrt(2))^2"),
    ("norm([1,1,4])^2", "norm([1,1,4])^2 = (3*sqrt(2))^2"),
    ("suvat(3,2,5)", "s = u*t + 1/2*a*t^2"),
    ("suvat(v=20,a=2,s=48,target=u)", "u = sqrt(208) or -sqrt(208)"),
    ("suvat(s=25,v=0,a=-10,target=t)", "t = 2.23607 s"),
    ("suvat(s=45,v=20,a=5,target=t)", "t = (no positive root)"),
]

ARGV_CASES = [
    ([str(RUNNER), "--suvat", "v=20,a=2,s=48,target=u"], "u = sqrt(208) or -sqrt(208)"),
    ([str(RUNNER), "--suvat", "s=25,v=0,a=-10,target=t"], "t = 2.24 s"),
    ([str(RUNNER), "--suvat", "s=45,v=20,a=5,target=t"], "t = (no positive root)"),
]


def main() -> int:
    bad = []
    for expr, marker in CASES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        if proc.returncode != 0 or marker not in out:
            bad.append((expr, marker, proc.returncode, out[:500]))
    for argv, marker in ARGV_CASES:
        proc = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        if proc.returncode != 0 or marker not in out:
            bad.append((" ".join(argv), marker, proc.returncode, out[:500]))
    if bad:
        for expr, marker, code, out in bad:
            print(f"FAIL {expr!r} code={code} missing={marker!r}\n{out}")
        return 1
    print(f"OK shared working cases={len(CASES) + len(ARGV_CASES)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

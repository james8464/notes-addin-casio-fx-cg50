#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


CASES: list[tuple[str, str, list[str], list[str]]] = [
    (
        "int",
        "acos((x-1)/3)",
        ["Use parts", "w=(x - 1)/3", "Answer:"],
        ["Answer: int(", "Classify the integrand", "ERR:", "Unexpected token"],
    ),
    (
        "int",
        "asin((x-1)/3)",
        ["Use parts", "w=(x - 1)/3", "Answer:"],
        ["Answer: int(", "Classify the integrand", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(sqrt(abs(3*x+1)+1))",
        ["abs(3*x + 1) >= 0", "Answer: y >= 1"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "ln(x+y)=x*y,x,method=implicit",
        ["Differentiate both sides", "dy/dx"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "arccos(cos(2*x+pi/6)),x,method=auto",
        ["Use chain rule", "d/dx arccos(u)", "dy/dx"],
        ["Use quotient rule", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "log(abs(3*x+1)+2),x,method=auto",
        ["Use chain rule", "d(abs(u))/dx", "dy/dx"],
        ["Use product rule", "Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "derive",
        "x=t^2+1/t,y=t^2-1/t,t,x,method=param",
        ["dx/dt", "dy/dx"],
        ["Answer: d/dx(", "Unexpected token", "ERR:"],
    ),
    (
        "alg",
        "x^2-5*x+6,method=factor",
        ["Answer:", "(x - 2)", "(x - 3)"],
        ["Range:", "Answer: x^2 - 5*x + 6", "ERR:"],
    ),
    (
        "alg",
        "domain(csc(2*x+pi/6)^2-cot(2*x+pi/6)^2)",
        ["sin(2*x + pi/6) != 0", "Answer:"],
        ["Answer: all real x", "ERR:"],
    ),
    (
        "alg",
        "range(cot(x^2-pi/4)^2+1)",
        ["Answer: y >= 1"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "int",
        "atan(tan(x^2-pi/4))",
        ["branch"],
        ["Answer: int(", "Classify the integrand", "ERR:"],
    ),
    (
        "int",
        "x*ln(5*x)",
        ["Integration by parts", "Answer: 1/2*x^2*log(5*x) - 1/4*x^2 + C"],
        ["Integral not recognised", "Answer: int(", "ERR:"],
    ),
    (
        "int",
        "tan(3*x)",
        ["Use Integral(tan u)", "Answer: -log(abs(cos(3*x)))/3 + C"],
        ["Integral not recognised", "Answer: int(", "ERR:"],
    ),
    (
        "alg",
        "range(log(10,abs(3*x+1)+3))",
        ["Answer: y >= log(3)/log(10)"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "sin(x)^2-sin(x)=0,method=factor",
        ["Let u = sin(x)", "u*(u - 1) = 0", "sin(x) = 0 or sin(x) = 1"],
        ["Unexpected token", "Answer: solve(", "ERR:"],
    ),
    (
        "alg",
        "x^2+a*x+b,method=factor",
        ["not factorable with numeric roots in this lightweight route", "Answer:"],
        ["Err:", "Unexpected token"],
    ),
    (
        "alg",
        "cot(x^2-pi/4),method=auto",
        ["Use identity cot(u) = cos(u)/sin(u)", "Domain: sin(x^2 - pi/4) != 0", "Answer: cos(x^2 - pi/4)/sin(x^2 - pi/4)"],
        ["Err:", "Unexpected token"],
    ),
    (
        "trig",
        "tan(2*x)=sqrt(3),x,0,2*pi,8,method=cast",
        ["A = pi/3 + n*pi", "x = pi/6 + n*pi/2", "Answer: x = [pi/6, 2*pi/3, 7*pi/6, 5*pi/3]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cot(x)^2+1,method=auto",
        ["Use identity 1 + cot(u)^2 = cosec(u)^2", "Domain: sin(x) != 0", "Answer: cosec(x)^2"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "sqrt((3*x+1)^2),method=auto",
        ["Let u = 3*x + 1", "sqrt(u^2)=abs(u)", "Answer: abs(3*x + 1)"],
        ["ERR:", "Unexpected token", "Answer: sqrt((3*x + 1)^2)"],
    ),
    (
        "derive",
        "mode:5,exp(t)*cos(t),exp(t)*sin(t),t",
        ["d/dt(dy/dx) = 2/(cos(t)-sin(t))^2", "Divide by dx/dt = e^t(cos(t)-sin(t))", "d2y/dx2 = 2/[e^t(cos(t)-sin(t))^3]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "int",
        "sqrt((2*x-3)^2)",
        ["sqrt(u^2)=abs(u)", "Split at u=0, so x=3/2", "For x >= 3/2", "For x < 3/2", "Answer: (2*x - 3)*abs(2*x - 3)/4 + C"],
        ["ERR:", "Unexpected token", "Answer: int("],
    ),
    (
        "alg",
        "arcsin(sin(2*x+pi/6)),method=complete_square",
        ["Complete square is for quadratics", "inverse-trig expression", "not applicable"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "asin(sin(2*x+pi/6)),method=auto",
        ["asin(sin(u))=u only for -pi/2 <= u <= pi/2", "branch", "Answer: asin(sin(2*x + pi/6))"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "atan(2*x-3),method=pf",
        ["Partial fractions need rational P(x)/Q(x)", "not applicable"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "sin(x+y),method=compound_angle",
        ["sin(A+B)=sin(A)cos(B)+cos(A)sin(B)", "Answer: sin(x)*cos(y)+cos(x)*sin(y)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "trig",
        "tan(2*x+pi/6),method=auto",
        ["tan(A+B)", "Let N = tan(2*x)+1/sqrt(3)", "Let D = 1-tan(2*x)*1/sqrt(3)", "Answer: N/D"],
        ["Answer: (tan(", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "arctan(2*x-3)=0,method=auto",
        ["arctan has domain all real", "arctan(A)=0 => A=0", "2*x - 3 = 0", "Answer: x = 3/2"],
        ["Answer: 2*x - 3 = 0", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cosec((x)^2-pi/4)^2-cot((x)^2-pi/4)^2,method=auto",
        ["Use identity cosec(u)^2 - cot(u)^2 = 1", "Answer: 1"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "range(arccos((x-1)/3))",
        ["Domain: -1 <= (x - 1)/3 <= 1", "Range: 0 <= y <= pi", "Answer: 0 <= y <= pi"],
        ["inspect graph/transform", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "cosec((x+1)/3),method=auto",
        ["Use identity cosec(u) = 1/sin(u)", "Domain: (x + 1)/3 != n*pi", "Domain: x != 3*n*pi - 1", "Answer: 1/sin((x + 1)/3)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "sec((x+1)/3),method=auto",
        ["Use identity sec(u) = 1/cos(u)", "Domain: (x + 1)/3 != pi/2 + n*pi", "Answer: 1/cos((x + 1)/3)"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "mode:5,t^2+1/t,t^2-1/t,t",
        ["dy/dx = (2*t^3 + 1)/(2*t^3 - 1)", "d/dt(dy/dx) = -12*t^2/(2*t^3-1)^2", "Answer: d2y/dx2 = -12*t^4/(2*t^3 - 1)^3"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "stats",
        "covariance([1,1,2,3,5,8],[2,5,7,11])",
        ["lengths must match", "Answer: no covariance"],
        ["Err:", "Traceback"],
    ),
    (
        "int",
        "sin((x)^2-pi/4)^2+cos((x)^2-pi/4)^2",
        ["Use identity sin(u)^2 + cos(u)^2 = 1", "Answer: x + C"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "x^2-6*x+5=0,method=complete_square",
        ["Complete square: (x - 3)^2 - 4 = 0", "Take square roots: x - 3 = +/-2", "Answer: x = [5, 1]"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "derive",
        "mode:3,exp(t)*cos(t),exp(t)*sin(t),t",
        ["dy/dx = e^t(sin(t)+cos(t))/[e^t(cos(t)-sin(t))]", "Cancel e^t"],
        ["ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(sin(2*x+pi/6)^2+cos(2*x+pi/6)^2)",
        ["Use sin(u)^2 + cos(u)^2 = 1", "Domain: all real x", "Answer: all real x"],
        ["Start with 1", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "domain(cosec((x)^2-pi/4)^2-cot((x)^2-pi/4)^2)",
        ["Use identity cosec(u)^2 - cot(u)^2 = 1", "Domain: sin(x^2 - pi/4) != 0", "Range: y = 1", "Answer: sin(x^2 - pi/4) != 0"],
        ["Start with 1", "ERR:", "Unexpected token"],
    ),
    (
        "alg",
        "arcsin(sin((x)^2-pi/4))=0,method=auto",
        ["arcsin(A)=0 => A=0", "sin(u)=0 => u=n*pi", "x = +/-sqrt(pi/4 + n*pi)"],
        ["x = []", "No real solution", "ERR:"],
    ),
    (
        "alg",
        "cosec(2*x+pi/6)^2-cot(2*x+pi/6)^2=0,method=auto",
        ["Domain: sin(2*x + pi/6) != 0", "Use identity cosec(u)^2 - cot(u)^2 = 1", "Answer: x = []"],
        ["log args", "denoms !=0", "ERR:"],
    ),
    (
        "alg",
        "cos(x)=0,method=auto",
        ["cos(x)=0 => x=pi/2 + n*pi", "Answer: x = pi/2 + n*pi, integer n"],
        ["Answer (3 d.p.)", "-98.", "ERR:"],
    ),
    (
        "int",
        "cot((x)^2-pi/4)^2+1,method=auto",
        ["Use identity 1 + cot(u)^2 = cosec(u)^2", "du/dx=2*x", "direct reverse chain does not apply"],
        ["Use the general integration route", "Answer: int", "ERR:"],
    ),
    (
        "alg",
        "range(x/(1+x^2))",
        ["Range: -1/2 <= y <= 1/2", "Answer: -1/2 <= y <= 1/2"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(1/(2-cos(3*x)))",
        ["cos(3*x) in [-1,1]", "Answer: 1/3 <= y <= 1"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "range(cot((x+1)/3))",
        ["Range: all real y", "Answer: all real y"],
        ["inspect graph/transform", "ERR:"],
    ),
    (
        "alg",
        "domain(arccos(sin((x+1)/3)))",
        ["sin input is always in [-1,1]", "Answer: all real x"],
        ["Answer: -1 <= sin", "inspect graph/transform", "ERR:"],
    ),
    (
        "trig",
        "sqrt(1-cos(x))=sin(x),x,0,2*pi,8,method=square_then_check",
        ["Square both sides", "cos(x)*(cos(x) - 1) = 0", "Check in original", "Answer: x = [0, pi/2"],
        ["Answer: x = []", "ERR:"],
    ),
]


def compact(text: str) -> str:
    return "".join(ch for ch in text if ch not in " \t\r\n*")


def run(flag: str, expr: str) -> str:
    proc = subprocess.run(
        [str(HOST), "--" + flag, expr],
        cwd=str(REPO),
        text=True,
        capture_output=True,
        timeout=15,
    )
    return proc.stdout + proc.stderr


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    bad = 0
    for flag, expr, needles, forbidden in CASES:
        out = run(flag, expr)
        norm = compact(out)
        ok = all(compact(n) in norm for n in needles)
        ok = ok and not any(f in out for f in forbidden)
        if ok:
            print("OK", flag, expr)
            continue
        bad += 1
        print("FAIL", flag, expr)
        print(out)
    print("done mismatches", bad, "/", len(CASES))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / "c++" / "addin" / "host" / "build" / "casio_host"

CASES = [
    (
        "chain sum",
        ["--derive", "sinh(x^2)+atanh(x/3),x,method=chain"],
        ("d/dx(sinh(x^2)) =", "d/dx(atanh(x/3)) =", "dy/dx ="),
    ),
    (
        "product",
        ["--derive", "(x^2+1)*sin(x),x"],
        ("f1 = x^2 + 1", "f1' = 2*x", "f2 = sin(x)", "f2' = cos(x)", "y' = f1'*f2 + f1*f2'"),
    ),
    (
        "single chain",
        ["--derive", "sin((x+1)^2),x,method=chain"],
        ("u = (x + 1)^2", "du/dx = 2*(x + 1)", "dy/dx = cos(u)*du/dx"),
    ),
    (
        "quotient",
        ["--derive", "x^2/(3*x-1),x"],
        ("u = x^2", "u' = 2*x", "v = 3*x - 1", "v' = 3", "y' = (u'v-u*v')/v^2"),
    ),
    (
        "param generic",
        ["--derive", "x=3+2*cos(theta),y=-3+2*sin(theta),theta,x,method=param"],
        ("dx/dt = -2*sin(theta)", "dy/dt = 2*cos(theta)", "dy/dx=(dy/dt)/(dx/dt)", "dy/dx = (2*cos(theta))/(-2*sin(theta))"),
    ),
    (
        "param no duplicate final",
        ["--derive", "t^5+4,t^5-5*t,t,method=param"],
        ("dx/dt = 5*t^4", "dy/dt = 5*t^4 - 5", "dy/dx = (5*t^4 - 5)/(5*t^4)"),
        ("dy/dx = (5*t^4 - 5)/(5*t^4)\ndy/dx = (5*t^4 - 5)/(5*t^4)",),
    ),
    (
        "exam chain final",
        ["--derive", "(2*x+ln(x))^3,x"],
        ("y = (2*x + ln(x))^3", "u = 2*x + ln(x)", "du/dx = 1/x + 2", "dy/dx = 3*(2*x + ln(x))^2*(1/x + 2)"),
    ),
    (
        "looping parts final",
        ["--int", "e^(2*x)*cos(3*x),method=parts"],
        ("I = Int(e^(2*x)*cos(3*x)) dx", "u = cos(3*x)", "dv = e^(2*x) dx", "J = Int(e^(2*x)*sin(3*x)) dx", "e^(2*x)*(2*cos(3*x) + 3*sin(3*x))/13 + C"),
    ),
    (
        "rform final",
        ["--trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,8,method=rform"],
        ("R = sqrt(3^2 + 4^2) = 5", "3*cos(x)+4*sin(x)=5*cos(x-alpha)", "x = [arctan(4/3) + arccos(2/5), 2*pi + arctan(4/3) - arccos(2/5)]"),
    ),
    (
        "sin same fn family",
        ["--trig", "sin(8*x)=sin(5*x),x,0,2*pi,26,method=identity"],
        ("sin(A) = sin(B)", "8*x = 5*x+2*pi*n => x = 2*pi*n/3", "8*x = pi-5*x+2*pi*n => x = pi*(2*n+1)/13", "x = ["),
    ),
    (
        "nested surd rewrite",
        ["--alg", "rewrite(sqrt(12+sqrt(140)))"],
        ("sqrt(12+sqrt(140)) = sqrt(m)+sqrt(n)", "m+n=12", "sqrt(7)+sqrt(5)"),
    ),
    (
        "log solve exact",
        ["--alg", "solve(log(2,x^2+4*x+3)=4+log(2,x^2+x),x)"],
        ("log(2,(x^2 + 4*x + 3)/(x^2 + x)) = 4", "5*x^2 + 4*x - 1 = 0", "x = [1/5]"),
    ),
    (
        "rational kept root domain",
        ["--alg", "(x^2-16)/(x-4)=14"],
        ("Domain: x - 4 != 0", "10 != 4", "x = [10]"),
    ),
    (
        "square equation shows plus minus",
        ["--alg", "solve((x+4)^2=16,x)"],
        ("(x + 4)^2 = 16", "x + 4 = +/-4", "x = [0, -8]"),
        ("Simplify:",),
    ),
    (
        "contradiction maths only",
        ["--alg", "solve((2*x+5)^2=4*x^2+20*x+30,x)"],
        ("-5 != 0", "x = []"),
        ("Simplify:", "No solution"),
    ),
    (
        "radical solve no generic",
        ["--alg", "sqrt(x+5)-sqrt(x-3)=2,method=auto"],
        ("Domain:", "sqrt(x + 5) = 2 + sqrt(x - 3)", "(x + 5) - (x - 3) - 4 = 2*2*sqrt(x - 3)", "x = [4]"),
    ),
    (
        "exp substitution no generic",
        ["--alg", "2^(2*x)-5*2^x+4=0,method=log_exp"],
        ("u=a^x", "x = [0, 2]"),
    ),
    (
        "sin reverse chain concise",
        ["--int", "sin(x),method=reverse_chain"],
        ("I = Int(sin(x)) dx", "-cos(x) + C"),
    ),
    (
        "tan square integral line",
        ["--int", "tan(x)^2"],
        ("tan(x)^2 = sec(x)^2 - 1", "I = Int(sec(x)^2 - 1) dx", "tan(x) - x + C"),
    ),
    (
        "defint wrapped variable",
        ["--int", "defint(e^(2*x)/(1+e^(2*x)),(x),0,ln(6))"],
        ("I = Int(e^(2*x)/(e^(2*x) + 1)) dx", "u = 1+e^(2*x)", "F(ln(6)) - F(0)", "ln(37)/2 - ln(2)/2"),
        ("ERR:",),
    ),
    (
        "defint outer wrapped",
        ["--int", "(defint(2*x/sqrt(2*x+4),x,(0),(7)))"],
        ("u^2 = 2*x + 4", "x = 0 => u = 2, x = 7 => u = sqrt(18)", "I = 2*2/4"),
        ("ERR:",),
    ),
    (
        "cos reverse chain concise",
        ["--int", "cos(x),method=reverse_chain"],
        ("I = Int(cos(x)) dx", "sin(x) + C"),
    ),
    (
        "linear division concise",
        ["--int", "x/(x+1),method=div"],
        ("N/D = Q + R/D", "Q = 1", "R = -1", "x - ln(abs(x + 1)) + C"),
    ),
    (
        "substitution spacing",
        ["--int", "sin(3*x+2)"],
        ("u = 3*x+2; du = 3 dx; dx = du/3", "I = 1/3*Int(sin(u)) du"),
        ("u=3*x+2", "du=3 dx", "dx=du/3", "I=1/3"),
    ),
    (
        "sec power no prose",
        ["--int", "sec(x)^4"],
        ("sec(x)^4 = sec(x)^2*sec(x)^2", "u = tan(x); du = sec(x)^2 dx", "I = Int(1+u^2) du"),
        ("Write ", "u=tan", "du=sec", "I=Int"),
    ),
    (
        "parts no prose",
        ["--int", "x*sin(x),method=parts"],
        ("u=x", "dv=sin(x) dx", "du=dx", "v=-cos(x)", "sin(x) - x*cos(x) + C"),
    ),
    (
        "invalid derive no fake working",
        ["--derive", "diff("],
        ("Err:",),
    ),
    (
        "invalid int method no auto working",
        ["--int", "x^2,method=not_a_method"],
        ("Err: invalid method",),
    ),
    (
        "sign derivative branch",
        ["--derive", "sign(sin(x)),x"],
        ("u = sin(x)", "u != 0 => dy/dx = 0", "u = 0 => dy/dx undefined"),
    ),
    (
        "sign branch in sum",
        ["--derive", "sin(x)+sign(x^2-1),x"],
        ("d/dx(sin(x)) = cos(x)", "u = x^2 - 1", "u != 0 => dy/dx = 0", "u = 0 => dy/dx undefined", "dy/dx = cos(x)"),
    ),
    (
        "trig fallback no fake working",
        ["--trig", "cos(x)+cos(2*x)+cos(3*x)=0,x,0,2*pi,10,method=identity"],
        ("x = []",),
    ),
    (
        "trig tautology interval",
        ["--trig", "sin(8*x)=sin(8*x),x,0,2*pi,32,method=identity"],
        ("A = 8*x, B = 8*x", "A = B", "0 <= x <= 2*pi"),
        ("x = [",),
    ),
]

TEXT_WORDS = (
    "Use ",
    "Start with",
    "Differentiate",
    "Simplify",
    "Apply",
    "Let ",
    "Since ",
    "Here ",
    "Then ",
    "Hence ",
    "Therefore",
    "looping integration",
    "Substitute ",
    "Collect ",
    "Inside ",
    "For ",
)

GLOBAL_TEXT_BANS = (
    "Method:",
    "Route:",
    "Answer:",
    "Auto result:",
    "Answer (3 d.p.):",
    "Chk:",
    "Done",
    "pick rule",
    "chain/prod",
    "Std form",
    "Rule/sub/id",
    "Verify",
    "Rearrange to lhs-rhs=0",
    "Use log/exp laws",
    "Exponentiate, solve",
    "State radical domain",
    "Solve the resulting equation",
    "Solve the polynomial in u",
    "substitution/factorisation/rearrangement",
    "u=inner",
    "alpha=base angle",
    "Apply the matching derivative rule set",
    "reverse chain sine rule",
    "reverse chain cosine rule",
    "constant times a polynomial part",
    "ln(abs(...)) for the reciprocal-linear term",
    "the split terms back",
    "Divide polynomial numerator",
)

STANDALONE_LINE_BANS = {
    "y =",
    "u =",
    "v =",
    "w =",
    "I =",
    "dy/d",
    "dx/d",
    "du/dx =",
    "dx/du =",
}

PACK_BANS = (
    "combine logs",
    "solve each case",
    "factor/rearrange",
    "std integral",
    "u-sub/IBP/PF if needed",
    "valid syntax",
    "d/dx rule -> simplify",
    "isolate trig fn",
    "CAST/base angles",
    "method route",
)


def numbered_lines(out: str):
    return [line for line in out.splitlines() if line[:1].isdigit() and ". " in line]


def compact_equals(s: str) -> str:
    return s.replace(" = ", "=").replace("= ", "=").replace(" =", "=")


def run_case(name, args, required, forbidden=()):
    proc = subprocess.run([str(HOST), *args], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout
    bad = []
    expects_error = any(marker.startswith("Err") for marker in required)
    if proc.returncode and not expects_error:
        bad.append(f"exit {proc.returncode}")
    for word in GLOBAL_TEXT_BANS:
        if word in out:
            bad.append(f"user-facing header: {word}")
    for line in (ln.strip() for ln in out.splitlines()):
        if line in STANDALONE_LINE_BANS:
            bad.append(f"standalone fragment: {line}")
    for word in forbidden:
        if word in out:
            bad.append(f"bad formatting: {word}")
    for line in numbered_lines(out):
        for word in TEXT_WORDS:
            if word in line:
                bad.append(f"text in working: {line}")
                break
    compact_out = compact_equals(out)
    for marker in required:
        if compact_equals(marker) not in compact_out:
            bad.append(f"missing: {marker}")
    if bad:
        print(f"FAIL {name}: " + "; ".join(bad))
        print(out)
        return False
    print(f"PASS {name}")
    return True


def main():
    ok = True
    for case in CASES:
        ok = run_case(*case) and ok
    pack = "\n".join(
        p.read_text(errors="replace")
        for p in sorted((ROOT / "c++" / "prizm" / "help").glob("CASIOCAS*.TPL"))
    )
    for marker in PACK_BANS:
        if marker in pack:
            print(f"FAIL pack generic: {marker}")
            ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

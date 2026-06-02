#!/usr/bin/env python3
import argparse
import json
import random
import signal
import string
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
PROGRESS = ROOT / "progress" / "state.jsonl"

OPS = ["+", "-", "*", "/"]
FUNCS = ["sin", "cos", "tan", "ln", "sqrt", "exp"]
VISIBLE_COMMANDS = [
    "abs", "acos", "approx", "asin", "atan", "ceil", "ceiling", "coeff",
    "collect", "cos", "cot", "degree", "diff", "exact", "expand", "factor",
    "floor", "fsolve", "gcd", "integrate", "implicit_diff", "lcm", "limit",
    "ln", "log", "partfrac", "pcoeff", "product", "proot", "range", "round",
    "rewrite", "sec", "series", "simplify", "sin", "solve", "sqrt", "subst", "sum",
    "tan", "taylor", "tcollect", "texpand", "xform",
]
COMMANDS = [
    "diff", "integrate", "simplify", "solve", "range",
    "rewrite", "rewrite", "xform", "xform", "xform",
    "log", "expand", "binomial", "apart", "defint", "trig",
]
WEIRD_CHARS = string.ascii_letters + string.digits + "+-*/^=(),[]{} ._"
CHAOS_VARS = ["x", "y", "z", "t", "u", "v", "a", "b", "k", "n"]
CHAOS_CONSTS = ["pi", "e"]

TEMPLATES = {
    "diff": [
        "diff((x+1)(x-2)ln(x),x)",
        "diff(ln(((x)-(6))*((x)^2)),x)",
        "diff((sin(x))^3,x)",
        "diff((sin(x))*(7-x)-x*x,x)",
        "diff((x^2+1)*sin(3*x-2),x)",
        "diff((x^2+1)/(x-1),x)",
    ],
    "integrate": [
        "integrate((ln(x))^2,x,2)",
        "integrate(x^2*ln(x),x)",
        "integrate(3*x^2*ln(x),x)",
        "integrate((x+1)/(x^2+2*x+3)^3,x)",
        "integrate(x^2+sin(2*x)+exp(3*x),x)",
        "integrate(exp(ln(x)),x,3)",
        "integrate(tan(x),x)",
        "integrate(((x)^2)((4)^2))",
    ],
    "simplify": [
        "simplify((x^2+3*x+2)/(x+1))",
        "simplify((x^2-1)/(1-x))",
        "simplify((x^2+2*x+1)/(x+1)^2)",
        "simplify(sin(x))",
    ],
    "solve": [
        "solve(0=(10-0.4x)*ln(x+1),x)",
        "solve(x=-2.7,x)",
        "solve(x=x+6,x)",
        "solve(tan(x)=1/2,x)",
        "solve(dy/dx=y,y)",
    ],
    "range": [
        "range(2*x+3)",
        "range(x^2+4*x+7)",
        "range(2*sin(3*x-1)-5)",
        "range(exp(x))",
        "range(ln(x))",
    ],
    "xform": [
        "xform(2*sin(x-60)=cos(x-30),tan(x)=3*sqrt(3))",
        "xform(2*sin(x-60)-cos(x-30)=0,tan(x)=3*sqrt(3))",
        "xform(1+tan(x)^2,sec(x)^2)",
        "xform(1+cot(x)^2,cosec(x)^2)",
        "xform(sin(x)^2+cos(x)^2,1)",
        "xform(log(a,x),ln(x)/ln(a))",
        "xform(log(2,x^3),3log(2,x))",
        "xform((x+2)^2,x^2+4*x+4)",
        "xform(sin(x)+2cos(x),sqrt(5)*sin(x+atan(2)))",
    ],
    "rewrite": [
        "rewrite(log(2,sqrt(x)),[log(2,x),log(2,x+8)])",
        "rewrite(log(2,sqrt(x)),[a=log(2,x),b=log(2,x+8)])",
        "rewrite(log(2,x^2+8*x),[a=log(2,x),b=log(2,x+8)])",
        "rewrite(log(2,8+64/x),[a=log(2,x),b=log(2,x+8)])",
        "rewrite((x+1)^2+sin(x),[u=(x+1)^2,v=sin(x)])",
    ],
    "log": ["log(2,x)", "log(5,x^2)"],
    "expand": ["expand((3*x-2)*(x+5))", "expand((2*x+3)^2)"],
    "binomial": ["binomial((1+8x)^(1/2))", "binomial((1-2x)^(-1))"],
    "apart": ["apart(6/(u(3+2u)))"],
    "defint": ["defint(sin(2x),x,0,pi/2)", "defint(9/(2x+1)^2,x,0,1)"],
    "trig": ["xform(1+tan(x)^2,sec(x)^2)"],
}


def maybe_space(rng: random.Random) -> str:
    return " " * rng.randrange(0, 3)


def maybe_wrap(rng: random.Random, s: str, p: float = 0.35) -> str:
    while rng.random() < p:
        s = f"({s})"
        p *= 0.45
    return s


def messy(rng: random.Random, s: str) -> str:
    out = []
    for ch in s:
        if ch in "+-*/=,":
            out.append(maybe_space(rng))
            out.append(ch)
            out.append(maybe_space(rng))
        else:
            out.append(ch)
    return "".join(out)


def mul(rng: random.Random, a: str, b: str) -> str:
    if rng.random() < 0.4:
        return f"({a})({b})"
    return f"({a})*({b})"


def var_or_num(rng: random.Random) -> str:
    if rng.random() < 0.45:
        return "x"
    if rng.random() < 0.25:
        return f"{rng.choice(['', '-'])}{rng.randrange(1, 10)}.{rng.randrange(0, 10)}"
    return str(rng.randrange(-9, 10))


def power(rng: random.Random, depth: int) -> str:
    base = expr(rng, depth - 1)
    p = rng.choice(["2", "3", "-1", "1/2", str(rng.randrange(2, 6))])
    return f"({base})^{p}"


def func(rng: random.Random, depth: int) -> str:
    name = rng.choice(FUNCS)
    return f"{name}({expr(rng, depth - 1)})"


def product(rng: random.Random, depth: int) -> str:
    left = expr(rng, depth - 1)
    right = expr(rng, depth - 1)
    op = rng.choice(OPS)
    if op == "*" and rng.random() < 0.35:
        return f"({left})({right})"
    return f"({left}){maybe_space(rng)}{op}{maybe_space(rng)}({right})"


def expr(rng: random.Random, depth: int) -> str:
    if depth <= 0:
        return var_or_num(rng)
    choice = rng.random()
    if choice < 0.22:
        return var_or_num(rng)
    if choice < 0.42:
        return func(rng, depth)
    if choice < 0.57:
        return power(rng, depth)
    return product(rng, depth)


def equation(rng: random.Random, depth: int) -> str:
    return f"{expr(rng, depth)}={expr(rng, depth)}"


def chaos_number(rng: random.Random) -> str:
    case = rng.randrange(5)
    if case == 0:
        return str(rng.randrange(-10**6, 10**6 + 1))
    if case == 1:
        return f"{rng.randrange(-5000,5001)}/{nz(rng,1,97)}"
    if case == 2:
        return f"{rng.choice(['', '-'])}{rng.randrange(0, 1000)}.{rng.randrange(0, 100000):05d}"
    if case == 3:
        return rng.choice(CHAOS_CONSTS)
    return f"sqrt({rng.randrange(2, 200)})"


def chaos_atom(rng: random.Random, numeric: bool = False) -> str:
    if not numeric and rng.random() < 0.45:
        return rng.choice(CHAOS_VARS)
    return chaos_number(rng)


def chaos_expr(rng: random.Random, depth: int, numeric: bool = False) -> str:
    if depth <= 0 or rng.random() < 0.12:
        return maybe_wrap(rng, chaos_atom(rng, numeric), 0.2)
    case = rng.randrange(9)
    if case == 0:
        f = rng.choice(["sin", "cos", "tan", "sec", "cot", "ln", "log", "sqrt", "abs", "exp"])
        if f == "log" and rng.random() < 0.75:
            return f"log({chaos_expr(rng, depth-1, numeric)},{chaos_expr(rng, depth-1, numeric)})"
        return f"{f}({chaos_expr(rng, depth-1, numeric)})"
    if case == 1:
        return f"({chaos_expr(rng, depth-1, numeric)})^{rng.choice(['-3','-2','-1','1/2','2/3','3/2','2','3','4'])}"
    if case == 2:
        return f"({chaos_expr(rng, depth-1, numeric)}){rng.choice(OPS)}({chaos_expr(rng, depth-1, numeric)})"
    if case == 3:
        return f"({chaos_expr(rng, depth-1, numeric)})*({chaos_expr(rng, depth-1, numeric)})"
    if case == 4:
        return f"{rng.choice(['sin','cos','tan','exp','ln','sqrt'])}({chaos_expr(rng, depth-1, numeric)})"
    if case == 5:
        return f"(({chaos_expr(rng, depth-1, numeric)})+({chaos_expr(rng, depth-1, numeric)}))"
    if case == 6:
        return f"(({chaos_expr(rng, depth-1, numeric)})-({chaos_expr(rng, depth-1, numeric)}))"
    if case == 7:
        return f"{rng.choice(['+', '-'])}{chaos_expr(rng, depth-1, numeric)}"
    return maybe_wrap(rng, chaos_expr(rng, depth-1, numeric), 0.55)


def chaos_equation(rng: random.Random, depth: int) -> str:
    return f"{chaos_expr(rng, depth)}={chaos_expr(rng, depth)}"


def chaos_args(rng: random.Random, cmd: str, depth: int) -> list[str]:
    v = rng.choice(CHAOS_VARS)
    numeric = rng.random() < 0.42
    e = lambda d=depth, n=numeric: chaos_expr(rng, max(1, d), n)
    eq = lambda d=depth: chaos_equation(rng, max(1, d))
    if rng.random() < 0.05:
        return [e(depth-1) for _ in range(rng.randrange(0, 5))]
    if cmd in {"abs", "acos", "approx", "asin", "atan", "ceil", "ceiling", "cos", "cot", "exact", "expand", "floor", "ln", "round", "sec", "simplify", "sin", "sqrt", "tan", "tcollect", "texpand"}:
        return [e()]
    if cmd in {"coeff"}:
        return [e(), v, str(rng.randrange(-3, 8))]
    if cmd in {"collect", "degree", "factor", "partfrac", "gcd", "lcm"}:
        return [e(), rng.choice([v, e(depth-1)])]
    if cmd in {"diff", "implicit_diff"}:
        args = [rng.choice([e(), eq()]), v]
        if rng.random() < 0.35:
            args.append(str(rng.randrange(1, 4)))
        return args
    if cmd == "integrate":
        args = [e(), v]
        r = rng.random()
        if r < 0.25:
            args += [str(rng.randrange(-5, 6)), str(rng.randrange(6, 13))]
        elif r < 0.5:
            args.append(str(rng.randrange(1, 6)))
        elif r < 0.65:
            args += [str(rng.randrange(1, 4)), f"u={e(depth-1)}"]
        return args
    if cmd == "fsolve":
        return [eq(), f"{v}={rng.randrange(-10,0)}..{rng.randrange(1,11)}"]
    if cmd == "limit":
        return [e(), f"{v}={rng.choice(['0','1','-1','inf'])}"]
    if cmd == "log":
        return [rng.choice([str(rng.randrange(2, 13)), e(depth-1)]), e()]
    if cmd == "pcoeff":
        return [f"[{e(depth-1, False)},{e(depth-1, False)},{e(depth-1, False)}]"]
    if cmd in {"product", "sum"}:
        return [e(), rng.choice(["k", v]), str(rng.randrange(-3, 4)), str(rng.randrange(5, 15))]
    if cmd in {"proot"}:
        return [random_poly(rng, rng.randrange(2, 7))]
    if cmd == "range":
        args = [e()]
        if rng.random() < 0.5:
            args += [v, str(rng.randrange(-10, 0)), str(rng.randrange(1, 10))]
        return args
    if cmd == "rewrite":
        if rng.random() < 0.55:
            k = rng.randrange(1, 10)
            base = rng.randrange(2, 6)
            form = rng.choice([
                f"log({base},sqrt(x))",
                f"log({base},x^2+{k}*x)",
                f"log({base},{base**3}+{base**3*k}/x)",
            ])
            return [form, f"[a=log({base},x),b=log({base},x+{k})]"]
        t1 = chaos_expr(rng, max(1, depth - 1), numeric)
        t2 = chaos_expr(rng, max(1, depth - 1), numeric)
        start = rng.choice([f"({t1})+({t2})", f"({t1})*({t2})", f"{rng.choice(['sin','cos','ln'])}({t1})"])
        return [start, f"[a={t1},b={t2}]"]
    if cmd in {"series", "taylor"}:
        return [e(), f"{v}={rng.randrange(-3, 4)}", str(rng.randrange(2, 8))]
    if cmd == "solve":
        return [eq(), v]
    if cmd == "subst":
        return [e(), f"{v}={e(depth-1)}"]
    if cmd == "xform":
        return [rng.choice([e(), eq()]), rng.choice([e(), eq()])]
    return [e()]


def chaos_case(rng: random.Random, depth: int, index: int, commands: list[str] | None = None) -> tuple[str, str]:
    if rng.random() < 0.08:
        return noisy_input(rng)
    if rng.random() < 0.10:
        return "chaos:raw", messy(rng, chaos_expr(rng, depth, rng.random() < 0.5))
    choices = commands or VISIBLE_COMMANDS
    cmd = choices[index % len(choices)]
    args = ",".join(chaos_args(rng, cmd, depth))
    return f"chaos:{cmd}", messy(rng, f"{cmd}({args})")


def nz(rng: random.Random, lo: int = -7, hi: int = 7) -> int:
    n = 0
    while n == 0:
        n = rng.randrange(lo, hi + 1)
    return n


def lin(rng: random.Random) -> str:
    a = nz(rng, -6, 6)
    b = rng.randrange(-9, 10)
    if b == 0:
        return f"{a}*x"
    return f"{a}*x{'+' if b > 0 else ''}{b}"


def pos_lin(rng: random.Random) -> str:
    a = nz(rng, 1, 6)
    b = rng.randrange(1, 10)
    return f"{a}*x+{b}"


def affine_x(rng: random.Random, *, nonzero_b: bool = False) -> str:
    a = nz(rng, -6, 6)
    b = rng.randrange(-9, 10)
    if nonzero_b:
        while b == 0:
            b = rng.randrange(-9, 10)
    if b == 0:
        return f"{a}*x"
    return f"{a}*x{b:+d}"


def quad_from_roots(rng: random.Random) -> tuple[str, int, int]:
    r1, r2 = rng.randrange(-6, 7), rng.randrange(-6, 7)
    b = -(r1 + r2)
    c = r1 * r2
    parts = ["x^2"]
    if b:
        parts.append(f"{b:+d}*x")
    if c:
        parts.append(f"{c:+d}")
    return "".join(parts), r1, r2


def factorable_ratio(rng: random.Random) -> tuple[str, str, str]:
    a = rng.randrange(1, 8)
    b = rng.randrange(1, 8)
    den = f"x+{a}"
    num = f"({den})*(x{b:+d})"
    ans = f"x{b:+d}"
    return num, den, ans


def random_poly(rng: random.Random, degree: int = 3) -> str:
    parts = []
    for p in range(degree, -1, -1):
        c = rng.randrange(-7, 8)
        if p == degree and c == 0:
            c = nz(rng, -7, 7)
        if c == 0:
            continue
        if p == 0:
            term = str(abs(c))
        elif p == 1:
            term = "x" if abs(c) == 1 else f"{abs(c)}*x"
        else:
            term = f"x^{p}" if abs(c) == 1 else f"{abs(c)}*x^{p}"
        parts.append(("+" if c > 0 else "-", term))
    if not parts:
        return "0"
    out = parts[0][1] if parts[0][0] == "+" else "-" + parts[0][1]
    for sign, term in parts[1:]:
        out += sign + term
    return out


def rand_diff(rng: random.Random) -> str:
    case = rng.randrange(9)
    if case == 0:
        return messy(rng, f"diff(({lin(rng)})^{rng.randrange(2,5)},x)")
    if case == 1:
        return messy(rng, f"diff({rng.choice(['sin','cos','exp'])}(({lin(rng)})^2),x)")
    if case == 2:
        return messy(rng, f"diff({mul(rng, lin(rng), 'ln('+pos_lin(rng)+')')},x)")
    if case == 3:
        return messy(rng, f"diff((x^2+{rng.randrange(1,8)})/(x{rng.choice(['-','+'])}{rng.randrange(1,6)}),x)")
    if case == 4:
        return messy(rng, f"diff({mul(rng, mul(rng, 'x+'+str(rng.randrange(1,6)), 'x-'+str(rng.randrange(1,6))), 'ln(x)')},x)")
    if case == 5:
        return messy(rng, f"diff({random_poly(rng, rng.randrange(2,5))}+sin({lin(rng)}),x)")
    if case == 6:
        return messy(rng, f"diff(ln(({pos_lin(rng)})^2),x)")
    if case == 7:
        return messy(rng, f"diff(({pos_lin(rng)})^2,x)")
    return messy(rng, f"diff(({lin(rng)})+{rng.choice(['sin','cos'])}({lin(rng)}),x)")


def rand_integrate(rng: random.Random) -> str:
    case = rng.randrange(11)
    if case == 0:
        a, p = nz(rng, -8, 8), rng.randrange(1, 6)
        return messy(rng, f"integrate({a}*x^{p}+{rng.randrange(-9,10)},x)")
    if case == 1:
        k = nz(rng, 1, 6)
        return messy(rng, f"integrate(sin({k}*x),x)")
    if case == 2:
        k = nz(rng, 1, 6)
        return messy(rng, f"integrate(exp({k}*x),x)")
    if case == 3:
        p = rng.randrange(1, 5)
        return messy(rng, f"integrate({mul(rng, f'x^{p}', 'ln(x)')},x)")
    if case == 4:
        a, b, p = nz(rng, 1, 5), rng.randrange(-5, 6), rng.randrange(2, 5)
        return messy(rng, f"integrate({a}*x*({b}+x^2)^{p},x)")
    if case == 5:
        c = rng.randrange(2, 8)
        return messy(rng, f"integrate({mul(rng, '((x)^2)', f'(({c})^2)')})")
    if case == 6:
        return messy(rng, f"integrate(({lin(rng)})+cos({nz(rng,1,6)}*x),x)")
    if case == 7:
        k = rng.randrange(-5, 6)
        return messy(rng, f"integrate((2*x+{k})*cos(x^2+{k}*x),x)")
    if case == 8:
        k, c = rng.randrange(1, 6), rng.randrange(2, 9)
        return messy(rng, f"integrate((2*x+{k})/(x^2+{k}*x+{c}),x)")
    if case == 9:
        return messy(rng, f"integrate((ln(x))^{rng.randrange(1,3)},x,{rng.choice([1,2])})")
    return messy(rng, f"integrate({random_poly(rng, rng.randrange(1,4))},x)")


def rand_solve(rng: random.Random) -> str:
    case = rng.randrange(12)
    if case == 0:
        a, b, r = nz(rng, -7, 7), rng.randrange(-9, 10), rng.randrange(-9, 10)
        return messy(rng, f"solve({a}*x+{b}={r},x)")
    if case == 1:
        q, r1, r2 = quad_from_roots(rng)
        return messy(rng, f"solve({q}=0,x)")
    if case == 2:
        n = rng.randrange(2, 10)
        return messy(rng, f"solve({n}=(x)^2,x)")
    if case == 3:
        a, b = nz(rng, 1, 6), rng.randrange(-4, 5)
        arg = rng.choice(["x+1", pos_lin(rng)])
        eq = f"({a}*x+{b})*ln({arg})=0"
        if rng.random() < 0.5:
            eq = "0=" + eq[:-2]
        return messy(rng, f"solve({eq},x)")
    if case == 4:
        q, _, _ = quad_from_roots(rng)
        return messy(rng, f"solve(0=({q})*ln(x+1),x)")
    if case == 5:
        return messy(rng, f"solve(tan(x)={rng.randrange(1,6)}/{rng.randrange(2,7)},x)")
    if case == 6:
        return messy(rng, f"solve({lin(rng)}={lin(rng)},x)")
    if case == 7:
        a, b, c = nz(rng, 1, 6), nz(rng, -8, 8), rng.randrange(-5, 6)
        return messy(rng, f"solve({a}*x{b:+d}/x={c},x)")
    if case == 8:
        a, b, c = nz(rng, 1, 6), nz(rng, 1, 8), rng.randrange(2, 8)
        return messy(rng, f"solve(({a}*x+{b})/x=sqrt({c}),x)")
    if case == 9:
        n = rng.randrange(2, 8)
        k = nz(rng, 1, 5)
        c = rng.randrange(1, 6)
        return messy(rng, f"solve({k}*sqrt({n})*x-{c}*sqrt({n})=x+sqrt({n}),x)")
    if case == 10:
        base = rng.randrange(2, 7)
        p = rng.randrange(2, 5)
        return messy(rng, f"solve(log({base},x+1)={p},x)")
    q, _, _ = quad_from_roots(rng)
    return messy(rng, f"solve({q}=0,x)")


def rand_range(rng: random.Random) -> str:
    case = rng.randrange(8)
    if case == 0:
        return messy(rng, f"range({lin(rng)})")
    if case == 1:
        q, _, _ = quad_from_roots(rng)
        return messy(rng, f"range({q})")
    if case == 2:
        return messy(rng, f"range((x)^{rng.choice([2,3,4,5])})")
    if case == 3:
        a, b, c = nz(rng, 1, 5), nz(rng, 1, 5), rng.randrange(-8, 9)
        return messy(rng, f"range({a}*{rng.choice(['sin','cos'])}({b}*x{c:+d}){rng.randrange(-8,9):+d})")
    if case == 4:
        q, _, _ = quad_from_roots(rng)
        return messy(rng, f"range({q},x,{rng.randrange(-5,2)},{rng.randrange(3,9)})")
    if case == 5:
        return messy(rng, f"range({rng.choice(['exp','ln','sqrt'])}({maybe_wrap(rng, 'x')}))")
    if case == 6:
        return messy(rng, f"range({rng.randrange(-9,10)})")
    q, _, _ = quad_from_roots(rng)
    return messy(rng, f"range({q})")


def rand_simplify(rng: random.Random) -> str:
    case = rng.randrange(7)
    if case == 0:
        a = rng.randrange(1, 7)
        return messy(rng, f"simplify((x^2+{2*a}*x+{a*a})/(x+{a})^2)")
    if case == 1:
        a = rng.randrange(1, 7)
        return messy(rng, f"simplify((x^2-{a*a})/(x-{a}))")
    if case == 2:
        return messy(rng, f"simplify(((x)^{rng.randrange(2,6)}))")
    if case == 3:
        num, den, _ = factorable_ratio(rng)
        return messy(rng, f"simplify(({num})/({den}))")
    if case == 4:
        a = rng.randrange(1, 7)
        return messy(rng, f"simplify(((x-{a})*(x+{a}))/(x-{a}))")
    if case == 5:
        return messy(rng, f"simplify(exp(ln({pos_lin(rng)})))")
    return messy(rng, f"simplify({expr(rng, 3)})")


def rand_xform(rng: random.Random) -> str:
    case = rng.randrange(17)
    if case == 0:
        terms = ["1", "tan(x)^2"]
        rng.shuffle(terms)
        return messy(rng, f"xform({terms[0]}+{terms[1]},sec(x)^2)")
    if case == 1:
        terms = ["sin(x)^2", "cos(x)^2"]
        rng.shuffle(terms)
        return messy(rng, f"xform({terms[0]}+{terms[1]},1)")
    if case == 2:
        terms = ["1", "cot(x)^2"]
        rng.shuffle(terms)
        return messy(rng, f"xform({terms[0]}+{terms[1]},cosec(x)^2)")
    if case == 3:
        n = rng.randrange(2, 7)
        return messy(rng, f"xform(ln(x^{n}),{n}*ln(x))")
    if case == 4:
        base, n = rng.randrange(2, 8), rng.randrange(2, 6)
        return messy(rng, f"xform(log({base},x^{n}),{n}*log({base},x))")
    if case == 5:
        a = rng.randrange(1, 6)
        return messy(rng, f"xform((x+{a})^2,x^2+{2*a}*x+{a*a})")
    if case == 6:
        a = rng.randrange(1, 6)
        return messy(rng, f"xform(x^2+{2*a}*x+{a*a},(x+{a})^2)")
    if case == 7:
        a, b = rng.randrange(1, 6), rng.randrange(1, 6)
        while b == a:
            b = rng.randrange(1, 6)
        return messy(rng, f"xform({a}*sin(x-60)={b}*cos(x-30),tan(x))")
    if case == 8:
        a, b = rng.randrange(1, 6), rng.randrange(1, 6)
        while b == a:
            b = rng.randrange(1, 6)
        return messy(rng, f"xform({a}*sin(x-60)-{b}*cos(x-30)=0,tan(x))")
    if case == 9:
        a, b = rng.randrange(1, 6), rng.randrange(1, 6)
        return messy(rng, f"xform({a}*sin(x)+{b}cos(x),sqrt({a*a+b*b})*sin(x+atan({b}/{a})))")
    if case == 10:
        a, b = rng.randrange(1, 6), rng.randrange(1, 6)
        return messy(rng, f"xform({a}*cos(x)+{b}sin(x),sqrt({a*a+b*b})*cos(x-atan({b}/{a})))")
    if case == 11:
        num, den, ans = factorable_ratio(rng)
        return messy(rng, f"xform(({num})/({den}),{ans})")
    if case == 12:
        a = rng.randrange(1, 7)
        return messy(rng, f"xform((x^2-{a*a})/(x-{a}),x+{a})")
    if case == 13:
        inner = rng.choice(["x+1", pos_lin(rng), f"(x+{rng.randrange(1,6)})^2"])
        return messy(rng, f"xform(exp(ln({inner})),{inner})")
    if case == 14:
        inner = rng.choice(["x", pos_lin(rng), f"x+{rng.randrange(1,6)}"])
        return messy(rng, f"xform(sqrt({inner})^2,{inner})")
    if case == 15:
        inner = rng.choice(["x+1", pos_lin(rng)])
        return messy(rng, f"xform(ln(exp({inner})),{inner})")
    num, den, ans = factorable_ratio(rng)
    return messy(rng, f"xform(({num})/({den}),{ans})")


def rand_rewrite(rng: random.Random) -> str:
    case = rng.randrange(10)
    base = rng.randrange(2, 6)
    k = rng.randrange(1, 10)
    if case == 0:
        return messy(rng, f"rewrite(log({base},sqrt(x)),[log({base},x),log({base},x+{k})])")
    if case == 1:
        return messy(rng, f"rewrite(log({base},sqrt(x)),[a=log({base},x),b=log({base},x+{k})])")
    if case == 2:
        return messy(rng, f"rewrite(log({base},x^2+{k}*x),[a=log({base},x),b=log({base},x+{k})])")
    if case == 3:
        return messy(rng, f"rewrite(log({base},{base**3}+{base**3*k}/x),[a=log({base},x),b=log({base},x+{k})])")
    if case == 4:
        return messy(rng, f"rewrite((x+{k})^2+sin(x),[u=(x+{k})^2,v=sin(x)])")
    if case == 5:
        return messy(rng, f"rewrite(({pos_lin(rng)})/({pos_lin(rng)}),[a=x,b=ln(x),c=sin(x)])")
    if case == 6:
        return messy(rng, f"rewrite(ln((x+{k})^2),[a=ln(x+{k})])")
    if case == 7:
        return messy(rng, f"rewrite(log({base},(x+{k})^3/x),[a=log({base},x),b=log({base},x+{k})])")
    if case == 8:
        return messy(rng, f"rewrite(sin(x)+cos(x)+x,[a=sin(x),b=cos(x),c=x])")
    return messy(rng, f"rewrite({chaos_expr(rng,3)},[a={chaos_expr(rng,2)},b={chaos_expr(rng,2)}])")


def rand_log(rng: random.Random) -> str:
    base = rng.randrange(2, 10)
    arg = rng.choice(["x", f"x^{rng.randrange(2, 6)}", f"({pos_lin(rng)})", f"sqrt({pos_lin(rng)})"])
    return messy(rng, f"log({base},{arg})")


def rand_expand(rng: random.Random) -> str:
    case = rng.randrange(3)
    if case == 0:
        return messy(rng, f"expand(({affine_x(rng)})^2)")
    if case == 1:
        return messy(rng, f"expand({mul(rng, affine_x(rng, nonzero_b=True), affine_x(rng, nonzero_b=True))})")
    a = rng.randrange(1, 7)
    b = rng.randrange(1, 7)
    return messy(rng, f"expand({mul(rng, f'x+{a}', f'x-{b}')})")


def rand_binomial(rng: random.Random) -> str:
    sign = rng.choice(["+", "-"])
    a = rng.randrange(1, 9)
    p = rng.choice(["1/2", "-1", "-1/2", "2/3", "3/2"])
    n = rng.choice(["", f",x,0,{rng.randrange(2,5)}"])
    return messy(rng, f"binomial((1{sign}{a}x)^({p}){n})")


def rand_apart(rng: random.Random) -> str:
    s = maybe_space
    return f"apart({s(rng)}6{s(rng)}/{s(rng)}({s(rng)}u{s(rng)}({s(rng)}3{s(rng)}+{s(rng)}2u{s(rng)}){s(rng)}))"


def rand_defint(rng: random.Random) -> str:
    case = rng.randrange(6)
    if case == 0:
        k = rng.randrange(1, 6)
        return messy(rng, f"integrate(sin({k}x),x,0,pi/{k})")
    if case == 1:
        p = rng.randrange(1, 5)
        return messy(rng, f"integrate(x^{p},x,{rng.randrange(0,3)},{rng.randrange(4,9)})")
    if case == 2:
        k = rng.randrange(1, 6)
        return messy(rng, f"integrate(exp({k}x),x,0,1)")
    if case == 3:
        return messy(rng, f"integrate(1/(x+{rng.randrange(1,6)}),x,0,{rng.randrange(1,6)})")
    if case == 4:
        return messy(rng, f"integrate(({lin(rng)}),x,{rng.randrange(-3,2)},{rng.randrange(3,9)})")
    return messy(rng, f"integrate(cos({rng.randrange(1,6)}x),x,0,pi/{rng.randrange(1,5)})")


def rand_trig(rng: random.Random) -> str:
    case = rng.randrange(6)
    if case == 0:
        a = rng.randrange(1, 6)
        b = rng.randrange(1, 6)
        return messy(rng, f"xform({a}*sin(x)+{b}cos(x),sqrt({a*a+b*b})*sin(x+atan({b}/{a})))")
    if case == 1:
        terms = ["1", "tan(x)^2"]
        rng.shuffle(terms)
        return messy(rng, f"xform({terms[0]}+{terms[1]},sec(x)^2)")
    if case == 2:
        terms = ["1", "cot(x)^2"]
        rng.shuffle(terms)
        return messy(rng, f"xform({terms[0]}+{terms[1]},cosec(x)^2)")
    if case == 3:
        terms = ["sin(x)^2", "cos(x)^2"]
        rng.shuffle(terms)
        return messy(rng, f"xform({terms[0]}+{terms[1]},1)")
    if case == 4:
        return messy(rng, f"xform(2*sin(x-60)=cos(x-30),tan(x)=3*sqrt(3))")
    a, b = rng.randrange(1, 6), rng.randrange(1, 6)
    while b == a:
        b = rng.randrange(1, 6)
    return messy(rng, f"xform({a}*sin(x-60)-{b}*cos(x-30)=0,tan(x))")


def command_input(rng: random.Random, depth: int, template_rate: float, commands: list[str] | None = None) -> tuple[str, str]:
    cmd = rng.choice(commands or COMMANDS)
    if rng.random() < template_rate and cmd in TEMPLATES:
        return cmd, rng.choice(TEMPLATES[cmd])
    if cmd == "diff":
        return cmd, rand_diff(rng)
    if cmd == "integrate":
        return cmd, rand_integrate(rng)
    if cmd == "simplify":
        return cmd, rand_simplify(rng)
    if cmd == "solve":
        return cmd, rand_solve(rng)
    if cmd == "range":
        return cmd, rand_range(rng)
    if cmd == "rewrite":
        return cmd, rand_rewrite(rng)
    if cmd == "xform":
        return cmd, rand_xform(rng)
    if cmd == "log":
        return cmd, rand_log(rng)
    if cmd == "expand":
        return cmd, rand_expand(rng)
    if cmd == "binomial":
        return cmd, rand_binomial(rng)
    if cmd == "apart":
        return cmd, rand_apart(rng)
    if cmd == "defint":
        return cmd, rand_defint(rng)
    if cmd == "trig":
        return cmd, rand_trig(rng)
    start = expr(rng, depth)
    target = rng.choice([f"expand({start})", f"factor({start})", expr(rng, depth), "tan(x)=3*sqrt(3)"])
    return cmd, f"xform({start},{target})"


def noisy_input(rng: random.Random) -> tuple[str, str]:
    n = rng.randrange(1, 80)
    return "noise", "".join(rng.choice(WEIRD_CHARS) for _ in range(n)).strip()


def make_case(
    rng: random.Random,
    depth: int,
    noise_rate: float,
    template_rate: float,
    commands: list[str] | None = None,
    chaos: bool = False,
    index: int = 0,
) -> tuple[str, str]:
    if chaos:
        return chaos_case(rng, depth, index, commands)
    if rng.random() < noise_rate:
        return noisy_input(rng)
    return command_input(rng, depth, template_rate, commands)


def classify(kind: str, src: str, out: str, code: int, elapsed: float, timeout: bool) -> str:
    stripped = out.strip()
    if timeout:
        return "timeout"
    if code:
        return "crash"
    if not stripped:
        return "empty"
    lines = [line.strip() for line in stripped.splitlines() if line.strip()]
    if "Exact:" in stripped:
        return "exact-label"
    if lines and lines[-1].startswith("Answer:"):
        return "answer-label-final"
    lowered = stripped.lower()
    if "nan" in lowered:
        return "nan"
    if any(x in lowered for x in ["segmentation", "abort", "system error", "reboot", "traceback"]):
        return "fatal-text"
    if kind.startswith("chaos") and stripped == src.strip():
        return "symbolic"
    if kind != "noise" and stripped == src.strip():
        return "echo-only"
    if kind != "noise" and ("A-level route:" in stripped or "KhiCAS exact result:" in stripped):
        return "weak-fallback"
    if elapsed > 2.5:
        return "slow"
    return "ok"


def append_progress(done: int, bad: int, status: str) -> None:
    PROGRESS.parent.mkdir(parents=True, exist_ok=True)
    event = {
        "phase": "fuzz",
        "last_event": f"random fuzz {done} done, {bad} bad",
        "queue": f"fuzz {done} cases",
        "tests": status,
    }
    with PROGRESS.open("a", encoding="utf-8") as f:
        f.write(json.dumps(event, separators=(",", ":")) + "\n")


def run_case(src: str, timeout_s: float) -> tuple[int, str, float, bool]:
    start = time.monotonic()
    try:
        p = subprocess.run(
            [str(RUNNER), src],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=timeout_s,
        )
        return p.returncode, (p.stdout or "") + (p.stderr or ""), time.monotonic() - start, False
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or "") + (exc.stderr or "")
        return 124, out, time.monotonic() - start, True


def write_transcript_case(f, rec: dict, out: str) -> None:
    f.write("=" * 72 + "\n")
    f.write(f"#{rec['i']} kind={rec['kind']} verdict={rec['verdict']} ")
    f.write(f"returncode={rec['returncode']} elapsed={rec['elapsed']}s\n")
    f.write("INPUT:\n")
    f.write(rec["input"] + "\n")
    f.write("OUTPUT:\n")
    f.write(out.rstrip() + "\n\n")


def main() -> int:
    ap = argparse.ArgumentParser(description="Random CAS working-output fuzzer.")
    ap.add_argument("--count", type=int, default=200, help="number of tests; ignored with --forever")
    ap.add_argument("--forever", action="store_true", help="run until interrupted")
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--depth", type=int, default=4)
    ap.add_argument("--timeout", type=float, default=4.0)
    ap.add_argument("--noise-rate", type=float, default=0.08)
    ap.add_argument("--template-rate", type=float, default=0.0, help="optional fixed regression seed rate; default is fully random")
    ap.add_argument("--only", default="", help="comma-separated command names to generate")
    ap.add_argument("--chaos", action="store_true", help="ignore templates and generate unpredictable catalog-wide tests")
    ap.add_argument("--stop-on-fail", action="store_true")
    ap.add_argument("--strict", action="store_true", help="count weak fallback as failure")
    ap.add_argument("--jsonl", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.jsonl")
    ap.add_argument("--transcript", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.txt")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    only = [x.strip() for x in args.only.split(",") if x.strip()] or None
    args.jsonl.parent.mkdir(parents=True, exist_ok=True)
    args.transcript.parent.mkdir(parents=True, exist_ok=True)
    done = bad = 0
    append_progress(0, 0, "random-fuzz running")

    def interrupted(_signum, _frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGINT, interrupted)
    with args.jsonl.open("w", encoding="utf-8") as report, args.transcript.open("w", encoding="utf-8") as transcript:
        transcript.write("CAS random working fuzzer transcript\n")
        transcript.write(f"seed={args.seed} count={args.count} forever={args.forever} chaos={args.chaos} depth={args.depth}\n")
        transcript.write(f"commands={','.join(only or (VISIBLE_COMMANDS if args.chaos else COMMANDS))}\n\n")
        try:
            while args.forever or done < args.count:
                kind, src = make_case(
                    rng,
                    args.depth,
                    args.noise_rate,
                    args.template_rate,
                    only,
                    args.chaos,
                    done,
                )
                code, out, elapsed, timeout = run_case(src, args.timeout)
                verdict = classify(kind, src, out, code, elapsed, timeout)
                fail = verdict not in {"ok", "slow", "symbolic"} and (args.strict or verdict != "weak-fallback")
                rec = {
                    "i": done,
                    "kind": kind,
                    "input": src,
                    "verdict": verdict,
                    "returncode": code,
                    "elapsed": round(elapsed, 4),
                    "output": out[:1200],
                }
                report.write(json.dumps(rec, ensure_ascii=False) + "\n")
                write_transcript_case(transcript, rec, out)
                if fail:
                    bad += 1
                    print(json.dumps(rec, ensure_ascii=False))
                    done += 1
                    if args.stop_on_fail:
                        break
                    continue
                done += 1
                if done % 25 == 0:
                    append_progress(done, bad, f"random-fuzz {done} done")
        except KeyboardInterrupt:
            append_progress(done, bad, "random-fuzz interrupted")
            print(f"INTERRUPTED random fuzz done={done} bad={bad} report={args.jsonl}")
            return 130 if bad else 0

    append_progress(done, bad, "random-fuzz complete")
    status = "OK" if bad == 0 else "DONE"
    print(f"{status} random fuzz done={done} bad={bad} report={args.jsonl}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

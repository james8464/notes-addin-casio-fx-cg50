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
CHAOS_FUNCS = ["sin", "cos", "tan", "sec", "cot", "ln", "sqrt", "exp", "abs"]
VISIBLE_COMMANDS = [
    "abs", "acos", "approx", "asin", "atan", "ceil", "ceiling", "coeff",
    "collect", "cos", "cot", "degree", "diff", "exact", "expand", "factor",
    "floor", "fsolve", "gcd", "integrate", "implicit_diff", "lcm", "limit",
    "ln", "log", "partfrac", "pcoeff", "product", "proot", "range", "round",
    "rewrite", "sec", "series", "simplify", "sin", "solve", "sqrt", "subst", "sum",
    "tan", "taylor", "tcollect", "texpand", "xform",
]
OPTIONAL_ARG_COMMANDS = {
    "coeff", "collect", "diff", "factor", "fsolve", "integrate", "limit",
    "log", "partfrac", "product", "range", "series", "solve", "sum",
    "taylor",
}
COMMANDS = [
    "diff", "integrate", "simplify", "solve", "range",
    "rewrite", "rewrite", "xform", "xform", "xform",
    "log", "expand", "binomial", "apart", "defint", "trig",
]
WEIRD_CHARS = string.ascii_letters + string.digits + "+-*/^=(),[]{} ._"
CHAOS_VARS = ["x", "y", "z", "t", "u", "v", "a", "b", "k", "n"]
CHAOS_CONSTS = ["pi", "e"]


def chance(rng: random.Random, numerator: int, denominator: int) -> bool:
    return rng.randrange(denominator) < numerator


def pick(rng: random.Random, values):
    return values[rng.randrange(len(values))]


class ChaosCommandScheduler:
    def __init__(self, rng: random.Random, commands: list[str] | None = None):
        self.rng = rng
        self.commands = list(commands or VISIBLE_COMMANDS)
        self.pool: list[str] = []
        self.counts: dict[str, int] = {}

    def next(self) -> tuple[str, int]:
        if not self.pool:
            self.pool = self.commands[:]
            self.rng.shuffle(self.pool)
        cmd = self.pool.pop()
        variant = self.counts.get(cmd, 0)
        self.counts[cmd] = variant + 1
        return cmd, variant


def maybe_space(rng: random.Random) -> str:
    return " " * rng.randrange(0, 3)


def maybe_wrap(rng: random.Random, s: str, p: float = 0.35) -> str:
    while rng.randrange(1000) < int(p * 1000):
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
    if chance(rng, 40, 100):
        return f"({a})({b})"
    return f"({a})*({b})"


def var_or_num(rng: random.Random) -> str:
    if chance(rng, 45, 100):
        return "x"
    if chance(rng, 25, 100):
        return f"{pick(rng, ['', '-'])}{rng.randrange(1, 10)}.{rng.randrange(0, 10)}"
    return str(rng.randrange(-9, 10))


def power(rng: random.Random, depth: int) -> str:
    base = expr(rng, depth - 1)
    p = pick(rng, ["2", "3", "-1", "1/2", str(rng.randrange(2, 6))])
    return f"({base})^{p}"


def func(rng: random.Random, depth: int) -> str:
    name = pick(rng, FUNCS)
    return f"{name}({expr(rng, depth - 1)})"


def product(rng: random.Random, depth: int) -> str:
    left = expr(rng, depth - 1)
    right = expr(rng, depth - 1)
    op = pick(rng, OPS)
    if op == "*" and chance(rng, 35, 100):
        return f"({left})({right})"
    return f"({left}){maybe_space(rng)}{op}{maybe_space(rng)}({right})"


def expr(rng: random.Random, depth: int) -> str:
    if depth <= 0:
        return var_or_num(rng)
    choice = rng.randrange(100)
    if choice < 22:
        return var_or_num(rng)
    if choice < 42:
        return func(rng, depth)
    if choice < 57:
        return power(rng, depth)
    return product(rng, depth)


def equation(rng: random.Random, depth: int) -> str:
    return f"{expr(rng, depth)}={expr(rng, depth)}"


def chaos_number(rng: random.Random, huge: bool = False) -> str:
    case = rng.randrange(6)
    scale = 10**12 if huge else 10**6
    if case == 0:
        return str(rng.randrange(-scale, scale + 1))
    if case == 1:
        return f"{rng.randrange(-500000 if huge else -5000, 500001 if huge else 5001)}/{nz(rng,1,997 if huge else 97)}"
    if case == 2:
        return f"{pick(rng, ['', '-'])}{rng.randrange(0, 10**6 if huge else 1000)}.{rng.randrange(0, 10**8 if huge else 100000):05d}"
    if case == 3:
        return pick(rng, CHAOS_CONSTS)
    if case == 4:
        return f"sqrt({rng.randrange(2, 10**6 if huge else 200)})"
    return f"({rng.randrange(-999,1000)}/{nz(rng,2,999)})*sqrt({rng.randrange(2,999)})"


def chaos_atom(rng: random.Random, numeric: bool = False, huge: bool = False) -> str:
    if not numeric and chance(rng, 45, 100):
        return pick(rng, CHAOS_VARS)
    return chaos_number(rng, huge)


def chaos_poly(rng: random.Random, min_terms: int = 35, max_terms: int = 130, var: str | None = None) -> str:
    v = var or pick(rng, CHAOS_VARS[:5])
    terms = rng.randrange(min_terms, max_terms + 1)
    start_power = rng.randrange(terms + 5, terms + 80)
    out = []
    for i in range(terms):
        p = max(0, start_power - i)
        c = rng.randrange(-9999, 10000)
        if c == 0:
            c = nz(rng, -9999, 9999)
        sign = "+" if c > 0 else "-"
        a = abs(c)
        if p == 0:
            term = str(a)
        elif p == 1:
            term = f"{a}*{v}"
        else:
            term = f"{a}*{v}^{p}"
        if not out:
            out.append(term if sign == "+" else "-" + term)
        else:
            out.append(sign + term)
    return "".join(out)


def chaos_poly_expr(rng: random.Random, depth: int) -> str:
    p = chaos_poly(rng, rng.randrange(35, 80), rng.randrange(90, 140))
    if depth <= 1 or chance(rng, 35, 100):
        return p
    op = pick(rng, ["+", "-", "*", "/"])
    small = chaos_expr(rng, depth - 2)
    return f"({p}){op}({small})"


def chaos_nested_tower(rng: random.Random, depth: int, numeric: bool = False) -> str:
    inner = chaos_atom(rng, numeric, huge=True)
    funcs = rng.sample(CHAOS_FUNCS, k=rng.randrange(5, min(8, len(CHAOS_FUNCS)) + 1))
    if chance(rng, 45, 100):
        inner = chaos_poly(rng, 10, 28)
    for f in funcs:
        if f == "ln" and chance(rng, 35, 100):
            inner = f"ln(abs({inner})+1)"
        elif f == "sqrt" and chance(rng, 35, 100):
            inner = f"sqrt(({inner})^2+1)"
        else:
            inner = f"{f}({inner})"
    if depth > 4 and chance(rng, 40, 100):
        inner = f"{inner}{pick(rng, OPS)}({chaos_expr(rng, depth-3, numeric)})"
    return inner


def chaos_log_base(rng: random.Random, depth: int) -> str:
    base_case = rng.randrange(6)
    if base_case == 0:
        base = f"sqrt({chaos_poly(rng, 18, 55)})"
    elif base_case == 1:
        base = f"ln(abs({chaos_poly(rng, 12, 45)})+2)"
    elif base_case == 2:
        base = f"exp(sqrt(abs({chaos_expr(rng, max(1, depth-2))})))"
    elif base_case == 3:
        base = f"{pick(rng, ['sin','cos','tan'])}(sqrt(abs({chaos_poly(rng, 8, 25)})))+{rng.randrange(2,9)}"
    elif base_case == 4:
        base = f"({chaos_expr(rng, max(1, depth-1))})^2+{rng.randrange(2,11)}"
    else:
        base = str(rng.randrange(2, 20))
    arg = chaos_expr(rng, max(1, depth - 1))
    return f"log({base},{arg})"


def chaos_expr(rng: random.Random, depth: int, numeric: bool = False) -> str:
    if depth <= 0 or chance(rng, 6, 100):
        return maybe_wrap(rng, chaos_atom(rng, numeric, huge=depth > 4), 0.2)
    if depth >= 5 and chance(rng, 18, 100):
        return chaos_poly_expr(rng, depth)
    if depth >= 5 and chance(rng, 18, 100):
        return chaos_nested_tower(rng, depth, numeric)
    if depth >= 4 and chance(rng, 16, 100):
        return chaos_log_base(rng, depth)
    case = rng.randrange(14)
    if case == 0:
        f = pick(rng, CHAOS_FUNCS + ["log"])
        if f == "log" and chance(rng, 75, 100):
            return chaos_log_base(rng, depth)
        return f"{f}({chaos_expr(rng, depth-1, numeric)})"
    if case == 1:
        return f"({chaos_expr(rng, depth-1, numeric)})^{pick(rng, ['-5','-4','-3','-2','-1','1/3','1/2','2/3','3/2','5/2','2','3','4','7'])}"
    if case == 2:
        return f"({chaos_expr(rng, depth-1, numeric)}){pick(rng, OPS)}({chaos_expr(rng, depth-1, numeric)})"
    if case == 3:
        return f"({chaos_expr(rng, depth-1, numeric)})*({chaos_expr(rng, depth-1, numeric)})"
    if case == 4:
        return f"{pick(rng, ['sin','cos','tan','exp','ln','sqrt'])}({chaos_expr(rng, depth-1, numeric)})"
    if case == 5:
        return f"(({chaos_expr(rng, depth-1, numeric)})+({chaos_expr(rng, depth-1, numeric)}))"
    if case == 6:
        return f"(({chaos_expr(rng, depth-1, numeric)})-({chaos_expr(rng, depth-1, numeric)}))"
    if case == 7:
        return f"{pick(rng, ['+', '-'])}{chaos_expr(rng, depth-1, numeric)}"
    if case == 8:
        return chaos_poly_expr(rng, depth)
    if case == 9:
        return chaos_nested_tower(rng, depth, numeric)
    if case == 10:
        return chaos_log_base(rng, depth)
    if case == 11:
        pieces = [chaos_expr(rng, depth-2, numeric) for _ in range(rng.randrange(3, 7))]
        return "(" + pick(rng, ["+", "-", "*"]).join(f"({p})" for p in pieces) + ")"
    if case == 12:
        return f"sqrt(abs({chaos_expr(rng, depth-1, numeric)})+({chaos_expr(rng, depth-2, numeric)})^2)"
    return maybe_wrap(rng, chaos_expr(rng, depth-1, numeric), 0.55)


def chaos_equation(rng: random.Random, depth: int) -> str:
    return f"{chaos_expr(rng, depth)}={chaos_expr(rng, depth)}"


def chaos_xform_pair(rng: random.Random, depth: int) -> list[str]:
    base = rng.randrange(2, 13)
    n = rng.randrange(2, 8)
    k = rng.randrange(1, 10)
    a = nz(rng, -6, 6)
    b = rng.randrange(-9, 10)
    if chance(rng, 20, 100):
        e = chaos_expr(rng, max(1, depth - 4))
    else:
        e = pick(rng, ["x", f"x+{k}", f"{a}*x+{b}", "sin(x)", "cos(x)", "tan(x)"])
    cases = [
        ("1+tan(x)^2", "sec(x)^2"),
        ("sec(x)^2", "1+tan(x)^2"),
        ("1+cot(x)^2", "cosec(x)^2"),
        ("sin(x)^2+cos(x)^2", "1"),
        ("tan(x)^2", "sec(x)^2-1"),
        ("cot(x)^2", "cosec(x)^2-1"),
        ("2sin(x)cos(x)", "sin(2x)"),
        (f"log({base},{e})", f"ln({e})/ln({base})"),
        (f"log({base},x^{n})", f"{n}log({base},x)"),
        (f"ln(x^{n})", f"{n}ln(x)"),
        (f"exp(ln({e}))", e),
        (f"ln(exp({e}))", e),
    ]
    if abs(a) > 0:
        aa = a * a
        bb = 2 * a * b
        cc = b * b
        cases.append((f"({a}*x+{b})^2", f"{aa}*x^2+{bb}*x+{cc}"))
    if chance(rng, 50, 100):
        src, target = pick(rng, cases)
    else:
        target, src = pick(rng, cases)
    return [src, target]


def chaos_args(rng: random.Random, cmd: str, depth: int, variant: int | None = None) -> list[str]:
    v = pick(rng, CHAOS_VARS)
    numeric = chance(rng, 42, 100)
    e = lambda d=depth, n=numeric: chaos_expr(rng, max(1, d), n)
    eq = lambda d=depth: chaos_equation(rng, max(1, d))
    if chance(rng, 1, 100):
        return [e(depth-1) for _ in range(rng.randrange(0, 5))]
    if cmd in {"abs", "acos", "approx", "asin", "atan", "ceil", "ceiling", "cos", "cot", "exact", "expand", "floor", "ln", "round", "sec", "simplify", "sin", "sqrt", "tan", "tcollect", "texpand"}:
        return [e()]
    if cmd in {"coeff"}:
        args = [e(), pick(rng, [v, e(depth-2, False)])]
        if (variant is None and chance(rng, 75, 100)) or (variant is not None and variant % 2):
            args.append(str(rng.randrange(-8, 18)))
        return args
    if cmd in {"collect", "factor", "partfrac"}:
        args = [e()]
        if (variant is None and chance(rng, 80, 100)) or (variant is not None and variant % 2):
            args.append(pick(rng, [v, e(depth-1)]))
        return args
    if cmd in {"degree", "gcd", "lcm"}:
        return [e(), pick(rng, [v, e(depth-1)])]
    if cmd in {"diff", "implicit_diff"}:
        args = [pick(rng, [e(), eq()])]
        if (variant is None and chance(rng, 90, 100)) or (variant is not None and variant % 3 in {1, 2}):
            args.append(v)
        if len(args) == 2 and ((variant is None and chance(rng, 45, 100)) or (variant is not None and variant % 3 == 2)):
            args.append(str(rng.randrange(1, 4)))
        return args
    if cmd == "integrate":
        args = [e()]
        if (variant is None and chance(rng, 92, 100)) or (variant is not None and variant % 4 != 0):
            args.append(v)
        r = rng.randrange(100)
        mode = variant % 4 if variant is not None else -1
        if len(args) == 2 and (r < 25 or mode == 1):
            args += [str(rng.randrange(-5, 6)), str(rng.randrange(6, 13))]
        elif len(args) == 2 and (r < 50 or mode == 2):
            args.append(str(rng.randrange(1, 6)))
        elif len(args) == 2 and (r < 70 or mode == 3):
            args += [str(rng.randrange(1, 4)), f"u={e(depth-1)}"]
        return args
    if cmd == "fsolve":
        args = [eq()]
        if (variant is None and chance(rng, 85, 100)) or (variant is not None and variant % 2):
            args.append(f"{v}={rng.randrange(-10,0)}..{rng.randrange(1,11)}")
        return args
    if cmd == "limit":
        args = [e(), f"{v}={pick(rng, ['0','1','-1','inf'])}"]
        if (variant is None and chance(rng, 45, 100)) or (variant is not None and variant % 2):
            args.append(pick(rng, ["1", "-1", "+", "-"]))
        return args
    if cmd == "log":
        if (variant is None and chance(rng, 20, 100)) or (variant is not None and variant % 2 == 0):
            return [e()]
        return [pick(rng, [str(rng.randrange(2, 13)), chaos_log_base(rng, max(2, depth - 1)), e(depth-1)]), e()]
    if cmd == "pcoeff":
        return [f"[{e(depth-1, False)},{e(depth-1, False)},{e(depth-1, False)}]"]
    if cmd in {"product", "sum"}:
        args = [e(), pick(rng, ["k", v])]
        if (variant is None and chance(rng, 85, 100)) or (variant is not None and variant % 2):
            args += [str(rng.randrange(-8, 4)), str(rng.randrange(5, 30))]
        return args
    if cmd in {"proot"}:
        return [random_poly(rng, rng.randrange(2, 7))]
    if cmd == "range":
        args = [e()]
        if (variant is None and chance(rng, 65, 100)) or (variant is not None and variant % 2):
            args += [v, str(rng.randrange(-10, 0)), str(rng.randrange(1, 10))]
        return args
    if cmd == "rewrite":
        if chance(rng, 25, 100):
            k = rng.randrange(1, 10)
            base = rng.randrange(2, 6)
            form = pick(rng, [
                f"log({base},sqrt(x))",
                f"log({base},x^2+{k}*x)",
                f"log({base},{base**3}+{base**3*k}/x)",
            ])
            return [form, f"[a=log({base},x),b=log({base},x+{k})]"]
        terms = [chaos_expr(rng, max(2, depth - rng.randrange(1, 4)), numeric) for _ in range(rng.randrange(2, 6))]
        start = chaos_expr(rng, depth, numeric)
        if chance(rng, 50, 100):
            start = pick(rng, ["+", "-", "*"]).join(f"({t})" for t in terms) + pick(rng, ["", f"+({start})"])
        labels = [f"{chr(97+i)}={term}" if chance(rng, 75, 100) else term for i, term in enumerate(terms)]
        return [start, "[" + ",".join(labels) + "]"]
    if cmd in {"series", "taylor"}:
        if (variant is None and chance(rng, 25, 100)) or (variant is not None and variant % 2 == 0):
            return [e()]
        return [e(), f"{v}={rng.randrange(-3, 4)}", str(rng.randrange(2, 12))]
    if cmd == "solve":
        if (variant is None and chance(rng, 18, 100)) or (variant is not None and variant % 3 == 0):
            return [eq()]
        if (variant is None and chance(rng, 22, 100)) or (variant is not None and variant % 3 == 1):
            return [f"[{eq(depth-1)},{eq(depth-1)}]", f"[{v},{pick(rng, [c for c in CHAOS_VARS if c != v])}]"]
        return [eq(), v]
    if cmd == "subst":
        if (variant is None and chance(rng, 35, 100)) or (variant is not None and variant % 2):
            return [e(), f"[{v}={e(depth-1)},{pick(rng, [c for c in CHAOS_VARS if c != v])}={e(depth-2)}]"]
        return [e(), f"{v}={e(depth-1)}"]
    if cmd == "xform":
        if chance(rng, 55, 100):
            return chaos_xform_pair(rng, depth)
        return [pick(rng, [e(), eq()]), pick(rng, [e(), eq()])]
    return [e()]


def chaos_case(
    rng: random.Random,
    depth: int,
    index: int,
    commands: list[str] | None = None,
    scheduler: ChaosCommandScheduler | None = None,
) -> tuple[str, str]:
    if scheduler is None and chance(rng, 4, 100):
        return noisy_input(rng)
    if scheduler is None and chance(rng, 6, 100):
        return "chaos:raw", messy(rng, chaos_expr(rng, depth, chance(rng, 50, 100)))
    if scheduler is not None:
        cmd, variant = scheduler.next()
    else:
        cmd, variant = pick(rng, commands or VISIBLE_COMMANDS), None
    args = ",".join(chaos_args(rng, cmd, depth, variant))
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
        return messy(rng, f"diff({pick(rng, ['sin','cos','exp'])}(({lin(rng)})^2),x)")
    if case == 2:
        return messy(rng, f"diff({mul(rng, lin(rng), 'ln('+pos_lin(rng)+')')},x)")
    if case == 3:
        return messy(rng, f"diff((x^2+{rng.randrange(1,8)})/(x{pick(rng, ['-','+'])}{rng.randrange(1,6)}),x)")
    if case == 4:
        return messy(rng, f"diff({mul(rng, mul(rng, 'x+'+str(rng.randrange(1,6)), 'x-'+str(rng.randrange(1,6))), 'ln(x)')},x)")
    if case == 5:
        return messy(rng, f"diff({random_poly(rng, rng.randrange(2,5))}+sin({lin(rng)}),x)")
    if case == 6:
        return messy(rng, f"diff(ln(({pos_lin(rng)})^2),x)")
    if case == 7:
        return messy(rng, f"diff(({pos_lin(rng)})^2,x)")
    return messy(rng, f"diff(({lin(rng)})+{pick(rng, ['sin','cos'])}({lin(rng)}),x)")


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
        return messy(rng, f"integrate((ln(x))^{rng.randrange(1,3)},x,{pick(rng, [1,2])})")
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
        arg = pick(rng, ["x+1", pos_lin(rng)])
        eq = f"({a}*x+{b})*ln({arg})=0"
        if chance(rng, 50, 100):
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
        return messy(rng, f"range((x)^{pick(rng, [2,3,4,5])})")
    if case == 3:
        a, b, c = nz(rng, 1, 5), nz(rng, 1, 5), rng.randrange(-8, 9)
        return messy(rng, f"range({a}*{pick(rng, ['sin','cos'])}({b}*x{c:+d}){rng.randrange(-8,9):+d})")
    if case == 4:
        q, _, _ = quad_from_roots(rng)
        return messy(rng, f"range({q},x,{rng.randrange(-5,2)},{rng.randrange(3,9)})")
    if case == 5:
        return messy(rng, f"range({pick(rng, ['exp','ln','sqrt'])}({maybe_wrap(rng, 'x')}))")
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
        inner = pick(rng, ["x+1", pos_lin(rng), f"(x+{rng.randrange(1,6)})^2"])
        return messy(rng, f"xform(exp(ln({inner})),{inner})")
    if case == 14:
        inner = pick(rng, ["x", pos_lin(rng), f"x+{rng.randrange(1,6)}"])
        return messy(rng, f"xform(sqrt({inner})^2,{inner})")
    if case == 15:
        inner = pick(rng, ["x+1", pos_lin(rng)])
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
    arg = pick(rng, ["x", f"x^{rng.randrange(2, 6)}", f"({pos_lin(rng)})", f"sqrt({pos_lin(rng)})"])
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
    sign = pick(rng, ["+", "-"])
    a = rng.randrange(1, 9)
    p = pick(rng, ["1/2", "-1", "-1/2", "2/3", "3/2"])
    n = pick(rng, ["", f",x,0,{rng.randrange(2,5)}"])
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
    _ = template_rate
    cmd = pick(rng, commands or COMMANDS)
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
    target = pick(rng, [f"expand({start})", f"factor({start})", expr(rng, depth), "tan(x)=3*sqrt(3)"])
    return cmd, f"xform({start},{target})"


def noisy_input(rng: random.Random) -> tuple[str, str]:
    n = rng.randrange(1, 80)
    return "noise", "".join(pick(rng, WEIRD_CHARS) for _ in range(n)).strip()


def make_case(
    rng: random.Random,
    depth: int,
    noise_rate: float,
    template_rate: float,
    commands: list[str] | None = None,
    chaos: bool = False,
    index: int = 0,
    scheduler: ChaosCommandScheduler | None = None,
) -> tuple[str, str]:
    if chaos:
        return chaos_case(rng, depth, index, commands, scheduler)
    if rng.randrange(1000) < int(noise_rate * 1000):
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
    ap.add_argument("--template-rate", type=float, default=0.0, help="deprecated; fixed templates were removed")
    ap.add_argument("--only", default="", help="comma-separated command names to generate")
    ap.add_argument("--chaos", action="store_true", help="compatibility flag; chaos is now the default")
    ap.add_argument("--structured", action="store_true", help="use the older smaller structured generator")
    ap.add_argument("--stop-on-fail", action="store_true")
    ap.add_argument("--strict", action="store_true", help="count weak fallback as failure")
    ap.add_argument("--jsonl", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.jsonl")
    ap.add_argument("--transcript", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.txt")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    only = [x.strip() for x in args.only.split(",") if x.strip()] or None
    chaos = args.chaos or not args.structured
    scheduler = ChaosCommandScheduler(rng, only or VISIBLE_COMMANDS) if chaos else None
    args.jsonl.parent.mkdir(parents=True, exist_ok=True)
    args.transcript.parent.mkdir(parents=True, exist_ok=True)
    done = bad = 0
    append_progress(0, 0, "random-fuzz running")

    def interrupted(_signum, _frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGINT, interrupted)
    with args.jsonl.open("w", encoding="utf-8") as report, args.transcript.open("w", encoding="utf-8") as transcript:
        transcript.write("CAS random working fuzzer transcript\n")
        transcript.write(f"seed={args.seed} count={args.count} forever={args.forever} chaos={chaos} depth={args.depth}\n")
        transcript.write(f"commands={','.join(only or (VISIBLE_COMMANDS if chaos else COMMANDS))}\n\n")
        try:
            while args.forever or done < args.count:
                kind, src = make_case(
                    rng,
                    args.depth,
                    args.noise_rate,
                    args.template_rate,
                    only,
                    chaos,
                    done,
                    scheduler,
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

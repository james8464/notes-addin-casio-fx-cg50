#!/usr/bin/env python3
import argparse
import importlib.util
import json
import os
import random
import re
import signal
import string
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
HOST_EVAL = ROOT / "tools" / "khicas_host_eval.py"
PROGRESS = ROOT / "progress" / "state.jsonl"

OPS = ["+", "-", "*", "/"]
FUNCS = ["sin", "cos", "tan", "atan", "ln", "sqrt", "exp"]
CHAOS_FUNCS = ["sin", "cos", "tan", "sec", "cot", "atan", "ln", "sqrt", "exp", "abs"]
VISIBLE_COMMANDS = [
    "abs", "acos", "approx", "asin", "atan", "ceil", "coeff",
    "cos", "cot", "degree", "diff", "domain", "exact", "factor",
    "floor", "fsolve", "gcd", "integrate", "implicit_diff", "lcm", "limit",
    "ln", "log", "partfrac", "product", "range", "round",
    "rewrite", "sec", "series", "simplify", "sin", "solve", "sqrt", "subst", "sum",
    "tan", "taylor", "tcollect", "texpand", "xform",
]
OPTIONAL_ARG_COMMANDS = {
    "coeff", "diff", "domain", "factor", "fsolve", "integrate", "limit",
    "log", "partfrac", "product", "range", "series", "solve", "sum",
    "taylor",
}
WORKING_OUTPUT_COMMANDS = {
    "diff", "domain", "integrate", "log", "partfrac", "range", "rewrite",
    "series", "solve", "taylor", "xform",
}
COMMANDS = [
    "diff", "integrate", "simplify", "solve", "range",
    "rewrite", "rewrite", "xform", "xform", "xform",
    "log", "texpand", "apart", "defint", "trig",
]
WEIRD_CHARS = string.ascii_letters + string.digits + "+-*/^=(),[]{} ._"
CHAOS_VARS = ["x", "y", "z", "t", "u", "v", "a", "b", "k", "n"]
CHAOS_CONSTS = ["pi", "e"]
_HOST_EVAL_MOD = None


def host_eval():
    global _HOST_EVAL_MOD
    if _HOST_EVAL_MOD is None:
        spec = importlib.util.spec_from_file_location("khicas_host_eval", HOST_EVAL)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"cannot load {HOST_EVAL}")
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        _HOST_EVAL_MOD = mod
    return _HOST_EVAL_MOD


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
    scale = 1_000_000_000_000 if huge else 1_000_000
    if case == 0:
        return str(rng.randrange(-scale, scale + 1))
    if case == 1:
        return f"{rng.randrange(-500000 if huge else -5000, 500001 if huge else 5001)}/{nz(rng,1,997 if huge else 97)}"
    if case == 2:
        return f"{pick(rng, ['', '-'])}{rng.randrange(0, 1_000_000 if huge else 1000)}.{rng.randrange(0, 100_000_000 if huge else 100000):05d}"
    if case == 3:
        return pick(rng, CHAOS_CONSTS)
    if case == 4:
        return f"sqrt({rng.randrange(2, 1_000_000 if huge else 200)})"
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
    if cmd in {"abs", "acos", "approx", "asin", "atan", "ceil", "cos", "cot", "exact", "floor", "ln", "round", "sec", "simplify", "sin", "sqrt", "tan", "tcollect", "texpand"}:
        return [e()]
    if cmd in {"coeff"}:
        args = [e(), pick(rng, [v, e(depth-2, False)])]
        if (variant is None and chance(rng, 75, 100)) or (variant is not None and variant % 2):
            args.append(str(rng.randrange(-8, 18)))
        return args
    if cmd in {"factor", "partfrac"}:
        args = [e()]
        if (variant is None and chance(rng, 80, 100)) or (variant is not None and variant % 2):
            args.append(pick(rng, [v, e(depth-1)]))
        return args
    if cmd == "degree":
        return [e(), v]
    if cmd in {"gcd", "lcm"}:
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
            args += [str(rng.randrange(1, 4)), e(depth-1)]
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
    if cmd in {"product", "sum"}:
        args = [e(), pick(rng, ["k", v])]
        if (variant is None and chance(rng, 85, 100)) or (variant is not None and variant % 2):
            args += [str(rng.randrange(-8, 4)), str(rng.randrange(5, 30))]
        return args
    if cmd == "domain":
        form = pick(rng, [
            f"sqrt({affine_x(rng)})",
            f"ln({pos_lin(rng)})",
            f"log({rng.randrange(2, 11)},{pos_lin(rng)})",
            f"1/({quad_from_roots(rng)[0]})",
            f"sqrt({affine_x(rng)})+ln({pos_lin(rng)})",
        ])
        return [form, "x"] if ((variant is None and chance(rng, 80, 100)) or variant) else [form]
    if cmd == "range":
        args = [e()]
        if (variant is None and chance(rng, 65, 100)) or (variant is not None and variant % 2):
            args += [v, str(rng.randrange(-10, 0)), str(rng.randrange(1, 10))]
        return args
    if cmd == "rewrite":
        if chance(rng, 25, 100):
            k = rng.randrange(1, 10)
            base = rng.randrange(2, 6)
            base_cubed = base * base * base
            form = pick(rng, [
                f"log({base},sqrt(x))",
                f"log({base},x^2+{k}*x)",
                f"log({base},{base_cubed}+{base_cubed*k}/x)",
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
            q, _, _ = quad_from_roots(rng)
            return [f"{q}=0"]
        if (variant is None and chance(rng, 22, 100)) or (variant is not None and variant % 3 == 1):
            w = pick(rng, [c for c in CHAOS_VARS if c != v])
            a, b, c, d = nz(rng, -5, 5), nz(rng, -5, 5), rng.randrange(-8, 9), rng.randrange(-8, 9)
            return [f"[{a}*{v}+{b}*{w}={c},{b}*{v}-{a}*{w}={d}]", f"[{v},{w}]"]
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
    case = rng.randrange(12)
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
    if case == 10:
        return messy(rng, f"integrate(cot({nz(rng,1,6)}*x),x)")
    return messy(rng, f"integrate({random_poly(rng, rng.randrange(1,4))},x)")


def rand_solve(rng: random.Random) -> str:
    case = rng.randrange(18)
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
    if case == 11:
        v = pick(rng, ["y", "u", "k", "t"])
        r1 = rng.randrange(1, 8)
        r2 = -rng.randrange(1, 8)
        b = -(r1 + r2)
        c = r1 * r2
        return messy(rng, f"solve({v}^2{b:+d}*{v}{c:+d}=0,{v})")
    if case == 12:
        v = pick(rng, ["x", "y", "u", "k", "t"])
        r = rng.randrange(1, 12)
        return messy(rng, f"solve({v}*({v}-{r})=0,{v})")
    if case == 13:
        v = pick(rng, ["x", "y", "u", "k", "t"])
        a = rng.randrange(1, 6)
        b = rng.randrange(1, 6)
        if b == a:
            b = 6
        s = a * a + b * b
        p = a * a * b * b
        return messy(rng, f"solve({v}^4-{s}*{v}^2+{p}=0,{v})")
    if case == 14:
        return messy(rng, "solve(cos(x)=0,x,0,pi)")
    if case == 15:
        return messy(rng, "solve(tan(x)=1,x,0,pi)")
    if case == 16:
        return messy(rng, f"solve(x^2-{rng.randrange(1,6)}^2=0,x,0,{rng.randrange(6,12)})")
    q, _, _ = quad_from_roots(rng)
    return messy(rng, f"solve({q}=0,x)")


def rand_range(rng: random.Random) -> str:
    case = rng.randrange(13)
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
    if case == 7:
        a, b, c = nz(rng, 1, 6), nz(rng, 1, 6), rng.randrange(-8, 9)
        m, d = nz(rng, 1, 5), rng.randrange(-8, 9)
        return messy(rng, f"range({a}*sin({m}*x{d:+d})+{b}*cos({m}*x{d:+d}){c:+d},x)")
    if case == 8:
        return messy(rng, "range(ln(x),x,1,inf)")
    if case == 9:
        return messy(rng, f"range(sqrt(x),x,{rng.randrange(0,3)},{rng.randrange(4,10)})")
    if case == 10:
        return messy(rng, "range(tan(x),x,-pi/4,pi/4)")
    if case == 11:
        return messy(rng, "range(x^2,x>0)")
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
    case = rng.randrange(18)
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
    if case == 16:
        v = pick(rng, ["x", "t", "y"])
        k = rng.randrange(1, 6)
        return messy(rng, f"xform({2*k}*sin({v})*cos({v}),{k}*sin(2*{v}))")
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
        base_cubed = base * base * base
        return messy(rng, f"rewrite(log({base},{base_cubed}+{base_cubed*k}/x),[a=log({base},x),b=log({base},x+{k})])")
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


def rand_texpand(rng: random.Random) -> str:
    case = rng.randrange(3)
    if case == 0:
        return messy(rng, f"texpand(({affine_x(rng)})^2)")
    if case == 1:
        return messy(rng, f"texpand({mul(rng, affine_x(rng, nonzero_b=True), affine_x(rng, nonzero_b=True))})")
    a = rng.randrange(1, 7)
    b = rng.randrange(1, 7)
    return messy(rng, f"texpand({mul(rng, f'x+{a}', f'x-{b}')})")


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
    if cmd == "texpand":
        return cmd, rand_texpand(rng)
    if cmd == "apart":
        return cmd, rand_apart(rng)
    if cmd == "defint":
        return cmd, rand_defint(rng)
    if cmd == "trig":
        return cmd, rand_trig(rng)
    start = expr(rng, depth)
    target = pick(rng, [f"texpand({start})", f"factor({start})", expr(rng, depth), "tan(x)=3*sqrt(3)"])
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
    if "input exceeds host fuzz complexity guard" in stripped:
        return "guarded-too-large"
    if "Exact:" in stripped:
        return "exact-label"
    if "proot(" in stripped or "min(f(r)" in stripped or "max(f(r)" in stripped:
        return "symbolic-skeleton"
    if lines and lines[-1].startswith("Answer:"):
        return "answer-label-final"
    if any(line in {
        "Integral:", "Solve:", "Range:", "xform:", "rewrite:",
        "Rewrite:", "Transform:", "Series form:", "Partial fractions:",
        "Differentiate:",
    } for line in lines):
        return "weak-section-label"
    lowered = stripped.lower()
    if "nan" in lowered:
        return "nan"
    if any(x in lowered for x in ["segmentation", "abort", "system error", "reboot", "traceback"]):
        return "fatal-text"
    if kind.startswith("chaos") and stripped == src.strip():
        cmd = kind.split(":", 1)[1] if ":" in kind else ""
        if cmd in WORKING_OUTPUT_COMMANDS:
            return "echo-only"
        return "symbolic"
    if kind != "noise" and stripped == src.strip():
        return "echo-only"
    if kind != "noise" and ("A-level route:" in stripped or "KhiCAS exact result:" in stripped):
        return "weak-fallback"
    cmd = kind.split(":", 1)[1] if kind.startswith("chaos:") else kind
    if cmd in WORKING_OUTPUT_COMMANDS:
        if cmd in {"solve", "fsolve"} and len(lines) <= 2:
            return "thin-solve"
        if cmd in {"xform", "rewrite"} and len(lines) <= 2:
            return "thin-transform"
        if cmd == "integrate" and len(lines) <= 2 and any(line.startswith("int(") for line in lines):
            return "thin-integral"
    if elapsed > 2.5:
        return "slow"
    return "ok"


def strip_constant(text: str) -> str:
    s = text.strip()
    for suffix in ("+ C", "+C"):
        if s.endswith(suffix):
            return s[: -len(suffix)].strip()
    return s


def final_math_line(out: str) -> str:
    skip_prefixes = (
        "planner search",
        "rule:",
        "texpand expression",
        "check equivalence",
        "difference simplifies",
        "checked",
        "power:",
        "product:",
        "quotient:",
        "sub ",
        "terms:",
        "integrate term",
    )
    for raw in reversed(out.splitlines()):
        line = raw.strip()
        if not line:
            continue
        if line.lower().startswith(skip_prefixes):
            continue
        if line.startswith("="):
            line = line[1:].strip()
        return strip_constant(line)
    return ""


def math_result_lines(out: str) -> list[str]:
    skip_prefixes = (
        "planner search",
        "rule:",
        "check equivalence",
        "difference simplifies",
        "checked",
        "power:",
        "product:",
        "quotient:",
        "sub ",
        "terms:",
        "integrate term",
        "u=",
        "du=",
        "dv=",
    )
    vals: list[str] = []
    for raw in out.splitlines():
        line = raw.strip()
        if not line or line.lower().startswith(skip_prefixes):
            continue
        vals.append(strip_constant(line[1:].strip() if line.startswith("=") else line))
        if "=" in line and "==" not in line:
            rhs = line.split("=", 1)[1].strip()
            if rhs:
                vals.append(strip_constant(rhs))
    return vals


def symbolic_zero(expr) -> bool:
    h = host_eval()
    candidates = [expr]
    for build in (
        lambda e: h.sp.trigsimp(e),
        lambda e: h.sp.trigsimp(e, method="fu"),
        lambda e: h.sp.simplify(e),
        lambda e: h.sp.trigsimp(h.sp.simplify(e), method="fu"),
        lambda e: h.sp.factor(h.sp.cancel(h.sp.together(h.sp.simplify(e)))),
        lambda e: h.sp.trigsimp(h.sp.expand_trig(e)),
    ):
        try:
            candidates.append(build(expr))
        except Exception:
            pass
    for z in candidates:
        try:
            if bool(z == 0 or z.equals(0)):
                return True
        except Exception:
            pass
    return False


def symbolic_equal(left: str, right: str) -> bool:
    h = host_eval()
    try:
        return symbolic_zero(h.parse_math(left) - h.parse_math(right))
    except Exception:
        return False


def antiderivative_matches(result: str, integrand: str, var_name: str = "x") -> bool:
    h = host_eval()
    try:
        var = h.NAMES.get(var_name) or h.sp.Symbol(var_name)
        if "abs(" in result.lower():
            if antiderivative_matches(re.sub(r"\babs\(", "(", result, flags=re.IGNORECASE), integrand, var_name):
                return True
        diff_expr = h.sp.diff(h.parse_math(result), var) - h.parse_math(integrand)
        if symbolic_zero(diff_expr):
            return True
        good = 0
        for pt in (0.3, 0.7, 1.1, 1.7):
            try:
                val = complex(h.sp.N(diff_expr.subs(var, pt), 30))
            except Exception:
                continue
            if abs(val) < 1e-7:
                good += 1
        return good >= 2
    except Exception:
        return False


def property_cases(commands: list[str] | None = None) -> list[dict[str, str]]:
    selected = set(commands or [])

    def allow(cmd: str) -> bool:
        return not selected or cmd in selected

    cases: list[dict[str, str]] = []
    if allow("factor"):
        for a, b, c, d in [(2, -3, 1, 5), (3, 2, 2, -7), (-2, 5, 4, 1)]:
            cases.append({
                "kind": "property:factor:linear_product",
                "cmd": "factor",
                "input": f"factor(({a}*x{b:+d})*({c}*x{d:+d}))",
                "mode": "eval_equiv",
            })
        for src, markers in [
            ("factor(10*x^3-21*x^2-x+6)", ["(x - 2)", "(2*x + 1)", "(5*x - 3)"]),
            ("factor(2*x^3-5*x^2-34*x-48)", ["(x - 6)", "(2*x^2 + 7*x + 8)"]),
        ]:
            cases.append({
                "kind": "property:factor:exact_backend",
                "cmd": "factor",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("partfrac"):
        for a, b, c, d in [(3, 5, 1, 2), (5, -1, 2, -3), (-4, 7, -2, 5)]:
            cases.append({
                "kind": "property:partfrac:two_linear",
                "cmd": "partfrac",
                "input": f"partfrac(({a}*x{b:+d})/((x{c:+d})*(x{d:+d})),x)",
                "mode": "eval_equiv",
            })
    if allow("diff"):
        for expr in [
            "(x^2+1)*sin(3*x-2)",
            "(2*x-5)/(x^2+3)",
            "ln((3*x+2)^2)",
            "sqrt(5*x-1)*exp(x)",
            "log(1/(sqrt(x^2+1)-x))",
        ]:
            cases.append({
                "kind": "property:diff:rules",
                "cmd": "diff",
                "input": f"diff({expr},x)",
                "mode": "derivative",
                "expr": expr,
                "var": "x",
            })
        for src, markers in [
            (
                "diff((x^2)*tan(y)=9,x)",
                ["Use original equation:", "tan(y) = 9/x^2", "x^4 + 81"],
            ),
            (
                "diff(3*x*tan(y)=6,x)",
                ["Use original equation:", "tan(y) = 2/x", "x^2 + 4"],
            ),
            (
                "diff(x^2*exp(y)=9,x)",
                ["Use original equation:", "exp(y) = 9/x^2", "(dy)/(dx)=-2/x"],
            ),
        ]:
            cases.append({
                "kind": "property:diff:implicit_dep_substitution",
                "cmd": "diff",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("solve"):
        for eq, var in [
            ("3*x+5=17", "x"),
            ("(x-2)*(x+3)=0", "x"),
            ("ln(2*x+1)=ln(7)", "x"),
            ("y^2-5*y+6=0", "y"),
        ]:
            cases.append({
                "kind": "property:solve:exact_output",
                "cmd": "solve",
                "input": f"solve({eq},{var})",
                "mode": "exact_output_contains",
            })
        for src, markers in [
            ("solve(3*k^2-58*k+240=0,k,k integer)", ["Apply condition:", "k integer", "k = [6]"]),
            ("solve(k^2+k-2=0,k,k!=1)", ["Apply condition:", "k!=1", "k = [-2]"]),
            ("solve(-1/300*x^2+3/5*x+3=0,x,x>0)", ["Apply condition:", "x>0", "x = [90 + 30*sqrt(10)]"]),
            (
                "solve(4^(3*p-1)=5^210,p)",
                ["Take ln of both sides.", "p = [(ln(5^210) + ln(4))/(3*ln(4))]"],
            ),
        ]:
            forbidden = ["p = []"] if "4^(3*p-1)" in src else []
            cases.append({
                "kind": "property:solve:condition_filter",
                "cmd": "solve",
                "input": src,
                "mode": "markers",
                "markers": markers,
                "forbidden": forbidden,
            })
    if allow("integrate"):
        for q, a, b in [
            (1, 1, 0),
            (1, 2, 1),
            (2, 3, -2),
            (3, 5, 4),
            (5, 4, -3),
            (7, 2, -5),
        ]:
            arg = f"{a}*x{b:+d}"
            cases.append({
                "kind": "property:integrate:sin_square",
                "cmd": "integrate",
                "input": f"integrate({q}*sin({arg})^2,x)",
                "mode": "antiderivative",
                "integrand": f"{q}*sin({arg})^2",
                "var": "x",
            })
        for expr in [
            "x*exp(2*x)",
            "(2*x+3)/(x^2+3*x+7)",
            "cos(x)*(6*sin(x)-2*sin(3*x))^(2/3)",
            "sqrt(18*cos(x)*sin(2*x))",
        ]:
            cases.append({
                "kind": "property:integrate:mixed_rules",
                "cmd": "integrate",
                "input": f"integrate({expr},x)",
                "mode": "antiderivative",
                "integrand": expr,
                "var": "x",
            })
    if allow("texpand"):
        for a, b in [(2, 3), (3, -5), (-4, 1), (5, 0), (-2, -7)]:
            cases.append({
                "kind": "property:texpand:affine_square",
                "cmd": "texpand",
                "input": f"texpand(({a}*x{b:+d})^2)",
                "mode": "eval_equiv",
            })
    if allow("texpand"):
        for a, b in [("x", "y"), ("2*x", "t"), ("u+1", "v")]:
            cases += [
                {
                    "kind": "property:texpand:compound_sin",
                    "cmd": "texpand",
                    "input": f"texpand(sin({a}+{b}))",
                    "mode": "markers",
                    "markers": ["sin(", "cos("],
                },
                {
                    "kind": "property:texpand:compound_cos",
                    "cmd": "texpand",
                    "input": f"texpand(cos({a}-{b}))",
                    "mode": "markers",
                    "markers": ["cos(", "sin("],
                },
                {
                    "kind": "property:texpand:compound_tan",
                    "cmd": "texpand",
                    "input": f"texpand(tan({a}+{b}))",
                    "mode": "markers",
                    "markers": ["tan("],
                },
            ]
    if allow("xform"):
        for start, target in [
            ("(x+3)^2", "x^2+6*x+9"),
            ("1/(sqrt(x)+1)", "(sqrt(x)-1)/(x-1)"),
            ("1/(sqrt(x)+2)", "(sqrt(x)-2)/(x-4)"),
            ("exp(ln(x+4))", "x+4"),
            ("(2*x+3)/(4*x+6)", "1/2"),
        ]:
            cases.append({
                "kind": "property:xform:algebraic_rewrite",
                "cmd": "xform",
                "input": f"xform({start},{target})",
                "mode": "target_equiv",
            })
        for v in ["x", "t"]:
            for k in [1, 2, 5]:
                cases += [
                    {
                        "kind": "property:xform:pythagorean",
                        "cmd": "xform",
                        "input": f"xform({k}+{k}*tan({v})^2,{k}*sec({v})^2)",
                        "mode": "target_equiv",
                    },
                    {
                        "kind": "property:xform:double_angle",
                        "cmd": "xform",
                        "input": f"xform({2*k}*sin({v})*cos({v}),{k}*sin(2*{v}))",
                        "mode": "target_equiv",
                    },
                ]
        for base in [2, 3, 5]:
            for n in [2, 4]:
                cases.append({
                    "kind": "property:xform:log_power",
                    "cmd": "xform",
                    "input": f"xform(log({base},(x+1)^{n}),{n}*log({base},x+1))",
                    "mode": "target_equiv",
                })
    if allow("log"):
        for src, result in [("log(2,8)", "3"), ("log(3,81)", "4")]:
            cases.append({
                "kind": "property:log:exact_backend",
                "cmd": "log",
                "input": src,
                "mode": "markers",
                "markers": ["Change of base", "= "+result],
            })
    if allow("sum"):
        for a, b, lo, hi in [(3, 2, 1, 5), (-2, 7, 0, 6), (5, -1, 2, 8)]:
            cases.append({
                "kind": "property:sum:linear",
                "cmd": "sum",
                "input": f"sum({a}*k{b:+d},k,{lo},{hi})",
                "mode": "exact_output_contains",
            })
    if allow("limit"):
        for src, markers in [
            ("limit(sin(x)/x,x=0)", ["limit method:", "Answer:", "1"]),
            ("limit((x^2-1)/(x-1),x=1)", ["limit method:", "trace:", "Answer:", "2"]),
            ("limit((1-cos(x))/x^2,x=0)", ["limit method:", "trace:", "Answer:", "1/2"]),
        ]:
            cases.append({
                "kind": "property:limit:exact_backend",
                "cmd": "limit",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("numeric"):
        for src, markers in [
            ("method=numeric,200*ln(2)*2^(8/5)", ["Sign: > 0", "To 2 significant figures: 420", "Nearest integer: 420"]),
            ("method=numeric,1/2*0.5*(0.4805+1.9218+2*(0.8396+1.2069+1.5694))", ["To 3 significant figures: 2.41"]),
            ("method=numeric,(3/5+sqrt(2/5))/(1/150)", ["Nearest integer: 185"]),
            ("method=numeric,2*0.3405^3-4*0.3405^2+7*0.3405-2", ["Sign: < 0", "Nearest integer: 0"]),
            ("method=numeric,0.0000000000001-0.0000000000001", ["Sign: = 0", "Nearest integer: 0"]),
        ]:
            cases.append({
                "kind": "property:numeric:exam_rounding",
                "cmd": "numeric",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("evalat"):
        for src, markers in [
            ("evalat(x^2-4*x+5,x,0)", ["Sub x=0", "f(0) = 5"]),
            ("evalat(3*x^2+12*x+25,x,-2)", ["Sub x=-2", "f(-2) = 13"]),
            ("evalat(1000*(ln(2)/5)*e^(ln(2)/5*t),t,8)", ["Sub t=8", "f(8) =", "ln(2)"]),
        ]:
            cases.append({
                "kind": "property:evalat:value_line",
                "cmd": "evalat",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("domain"):
        for src, markers in [
            ("domain(sqrt(2*x-3),x)", ["x >= 3/2"]),
            ("domain(ln(5-x),x)", ["x < 5"]),
            ("domain(1/(x^2-9),x)", ["x != -3", "x != 3"]),
        ]:
            cases.append({
                "kind": "property:domain:inequality_rules",
                "cmd": "domain",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("range"):
        for src, markers in [
            ("range((2*x-3)^2+5,x)", ["y >= 5"]),
            ("range(-3*(x+2)^2+7,x)", ["y <= 7"]),
            ("range(abs(x-3)+2)", ["abs(u) >= 0", "y >= 2"]),
            ("range(sqrt(x-1)+3)", ["sqrt(u) >= 0", "y >= 3"]),
            ("range(x/(x^2+4),x)", ["D>=0", "-16*y^2 + 1 >= 0", "Solve D>=0"]),
            ("range((x^2-1)/(x^2+1),x)", ["D>=0", "-4*y^2 + 4 >= 0", "Solve D>=0", "Exclude degenerate y: 1"]),
            ("range((x+1)/(x^2+1),x)", ["D>=0", "-4*y^2 + 4*y + 1 >= 0", "Solve D>=0"]),
            ("range(1/(x^2+4*x+5),x)", ["D>=0", "-4*y^2 + 4*y >= 0", "Solve D>=0", "Exclude degenerate y: 0"]),
            ("range((x^2-2)*exp(-x),x)", ["f'(x)=", "as x -> -infinity", "as x -> +infinity", "y >="]),
            ("range((x+1)*exp(-x),x)", ["f'(x)=", "f(0) = 1", "max y =", "y <="]),
            ("range(x^2*exp(-x),x)", ["f'(x)=", "f(0) = 0", "min y = 0", "y >= 0"]),
            ("range(sin(x)+cos(x),x)", ["R-form range", "R=sqrt(1^2+1^2)=sqrt(2)", "-sqrt(2) <= y <= sqrt(2)"]),
            ("range(3*sin(2*x+1)+4*cos(2*x+1)-5,x)", ["R-form range", "R=sqrt(3^2+4^2)=5", "-10 <= y <= 0"]),
        ]:
            cases.append({
                "kind": "property:range:quadratic_vertex",
                "cmd": "range",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("series"):
        for src, markers in [
            ("series(exp(x),x=0,4)", ["series method:", "trace:", "Answer:", "x^4/24", "x^3/6"]),
            ("series(sin(theta),theta=0,3)", ["series method:", "trace:", "Answer:", "theta^3/6", "theta"]),
            ("series(tan(theta),theta=0,3)", ["series method:", "trace:", "Answer:", "theta^3/3", "theta"]),
            ("series((1+2*x)^(-1),x=0,5)", ["series method:", "trace:", "Answer:", "16*x^4", "- 8*x^3"]),
        ]:
            cases.append({
                "kind": "property:series:exact_backend",
                "cmd": "series",
                "input": src,
                "mode": "markers",
                "markers": markers,
            })
    if allow("taylor"):
        for src in [
            "taylor((1-4*x)^(-1/2),x,0,5)",
            "taylor(ln(1+x),x,0,5)",
        ]:
            cases.append({
                "kind": "property:taylor:exact_backend",
                "cmd": "taylor",
                "input": src,
                "mode": "exact_output_contains",
            })
    if allow("product"):
        for a, b in [(1, 4), (2, 5), (3, 6)]:
            cases.append({
                "kind": "property:product:linear",
                "cmd": "product",
                "input": f"product(k+{a},k,1,{b})",
                "mode": "exact_output_contains",
            })
    return cases


def verify_property(case: dict[str, str], out: str, code: int, timeout: bool) -> str:
    if timeout:
        return "property-timeout"
    if code:
        return "property-crash"
    lowered = out.lower()
    if "no route" in lowered or "no a-level working route found" in lowered:
        return "property-no-route"
    if "not equivalent" in lowered:
        return "property-not-equivalent"
    mode = case["mode"]
    if mode == "markers":
        packed_out = "".join(out.split()).replace("*", "").replace("**", "^")
        markers = case.get("markers", [])
        if isinstance(markers, str):
            markers = [markers]
        for marker in markers:
            packed_marker = "".join(str(marker).split()).replace("*", "").replace("**", "^")
            if str(marker) not in out and packed_marker not in packed_out:
                return "property-marker-missing"
        forbidden = case.get("forbidden", [])
        if isinstance(forbidden, str):
            forbidden = [forbidden]
        for marker in forbidden:
            packed_marker = "".join(str(marker).split()).replace("*", "").replace("**", "^")
            if str(marker) in out or packed_marker in packed_out:
                return "property-forbidden-marker"
        return "ok"
    if "checked" not in lowered:
        return "property-unchecked"
    result_lines = math_result_lines(out)
    result = final_math_line(out)
    if result:
        result_lines.append(result)
    if not result_lines:
        return "property-no-result"
    if mode == "derivative":
        h = host_eval()
        try:
            var = h.NAMES.get(case.get("var", "x")) or h.sp.Symbol(case.get("var", "x"))
            expected = h.fmt(h.sp.diff(h.parse_math(case["expr"]), var))
        except Exception:
            return "property-eval-error"
        if any(symbolic_equal(candidate, expected) for candidate in result_lines):
            return "ok"
        return "property-bad-derivative"
    if mode == "antiderivative":
        if any(antiderivative_matches(candidate, case["integrand"], case.get("var", "x"))
               for candidate in result_lines):
            return "ok"
        return "property-bad-antiderivative"
    if mode == "eval_equiv":
        h = host_eval()
        try:
            expected = h.fmt(h.evaluate(case["input"]))
        except Exception:
            return "property-eval-error"
        if any(symbolic_equal(candidate, expected) for candidate in result_lines):
            return "ok"
        return "property-bad-result"
    if mode == "target_equiv":
        h = host_eval()
        parsed = h.call(case["input"])
        if not parsed or len(parsed[1]) < 2:
            return "property-bad-input"
        target = parsed[1][1]
        packed_out = "".join(out.split()).replace("*", "").replace("**", "^")
        packed_target = "".join(target.split()).replace("*", "").replace("**", "^")
        if packed_target in packed_out or any(symbolic_equal(candidate, target) for candidate in result_lines):
            return "ok"
        return "property-target-mismatch"
    if mode == "exact_output_contains":
        h = host_eval()
        try:
            expected = h.fmt(h.evaluate(case["input"]))
        except Exception:
            return "property-eval-error"
        packed_out = "".join(out.split()).replace("*", "").replace("**", "^")
        packed_expected = "".join(expected.split()).replace("*", "").replace("**", "^")
        if packed_expected in packed_out or any(symbolic_equal(candidate, expected) for candidate in result_lines):
            return "ok"
        return "property-exact-output-mismatch"
    return "property-unknown-mode"


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
    proc = subprocess.Popen(
        [str(RUNNER), src],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )
    try:
        stdout, stderr = proc.communicate(timeout=timeout_s)
    except subprocess.TimeoutExpired as exc:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        stdout, stderr = proc.communicate()
        out = (exc.stdout or "") + (exc.stderr or "") + (stdout or "") + (stderr or "")
        return 124, out, time.monotonic() - start, True
    return proc.returncode, (stdout or "") + (stderr or ""), time.monotonic() - start, False


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
    ap.add_argument("--per-function", type=int, default=0, help="run this many tests for each selected visible function")
    ap.add_argument("--forever", action="store_true", help="run until interrupted")
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--depth", type=int, default=4)
    ap.add_argument("--timeout", type=float, default=8.0)
    ap.add_argument("--noise-rate", type=float, default=0.08)
    ap.add_argument("--template-rate", type=float, default=0.0, help="deprecated; fixed templates were removed")
    ap.add_argument("--only", default="", help="comma-separated command names to generate")
    ap.add_argument("--chaos", action="store_true", help="compatibility flag; chaos is now the default")
    ap.add_argument("--structured", action="store_true", help="use the older smaller structured generator")
    ap.add_argument("--stop-on-fail", action="store_true")
    ap.add_argument("--strict", action="store_true", help="count weak fallback as failure")
    ap.add_argument("--no-property-families", action="store_true", help="skip generated semantic property families")
    ap.add_argument("--print-failures", action="store_true", help="print failed records to stdout")
    ap.add_argument("--jsonl", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.jsonl")
    ap.add_argument("--transcript", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.txt")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    only = [x.strip() for x in args.only.split(",") if x.strip()] or None
    chaos = args.chaos or not args.structured
    scheduler = ChaosCommandScheduler(rng, only or VISIBLE_COMMANDS) if chaos else None
    if args.per_function > 0 and not args.forever:
        args.count = args.per_function * len(only or (VISIBLE_COMMANDS if chaos else COMMANDS))
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
                fail = verdict not in {"ok", "slow", "symbolic", "guarded-too-large"} and (
                    args.strict or verdict != "weak-fallback"
                )
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
                    if args.print_failures:
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

        if not args.no_property_families:
            for case in property_cases(only):
                src = case["input"]
                code, out, elapsed, timeout = run_case(src, args.timeout)
                verdict = verify_property(case, out, code, timeout)
                fail = verdict != "ok"
                rec = {
                    "i": done,
                    "kind": case["kind"],
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
                    if args.print_failures:
                        print(json.dumps(rec, ensure_ascii=False))
                    done += 1
                    if args.stop_on_fail:
                        break
                    continue
                done += 1
                if done % 25 == 0:
                    append_progress(done, bad, f"random-fuzz {done} done")

    append_progress(done, bad, "random-fuzz complete")
    status = "OK" if bad == 0 else "DONE"
    print(f"{status} random fuzz done={done} bad={bad} report={args.jsonl}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

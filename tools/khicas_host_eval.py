#!/usr/bin/env python3
"""Small host-side evaluator for production working-route tests.

The calculator build uses KhiCAS through cascas_khicas_eval(). The local host
runner cannot link the full calculator CAS, so strict queue tests use this
bounded SymPy evaluator as the callback for the same route shape.
"""

from __future__ import annotations

import sys
import signal
from functools import reduce
from math import prod
import re

import sympy as sp
from sympy.calculus.util import continuous_domain, function_range
from sympy.core.relational import Relational
from sympy.parsing.sympy_parser import (
    convert_xor,
    implicit_multiplication_application,
    parse_expr,
    rationalize,
    standard_transformations,
)


TRANSFORMS = standard_transformations + (
    implicit_multiplication_application,
    convert_xor,
    rationalize,
)

NAMES: dict[str, object] = {
    "pi": sp.pi,
    "e": sp.E,
    "E": sp.E,
    "ln": sp.log,
    "sqrt": sp.sqrt,
    "exp": sp.exp,
    "sin": sp.sin,
    "cos": sp.cos,
    "tan": sp.tan,
    "sec": sp.sec,
    "cosec": sp.csc,
    "csc": sp.csc,
    "cot": sp.cot,
    "asin": sp.asin,
    "acos": sp.acos,
    "atan": sp.atan,
    "abs": sp.Abs,
    "Abs": sp.Abs,
}


def cas_log(base_or_arg, arg=None):
    if arg is None:
        return sp.log(base_or_arg)
    return sp.log(arg, base_or_arg)


NAMES["log"] = cas_log

for _name in [
    "x",
    "y",
    "z",
    "t",
    "u",
    "v",
    "a",
    "b",
    "c",
    "d",
    "h",
    "k",
    "m",
    "n",
    "p",
    "q",
    "r",
    "theta",
    "alpha",
    "beta",
]:
    NAMES[_name] = sp.Symbol(_name)
    if len(_name) == 1:
        NAMES[_name.upper()] = NAMES[_name]

NAMES["lambda_var"] = sp.Symbol("lambda")


def _diff(expr, var=None, order=1):
    var = var or NAMES["x"]
    if getattr(order, "is_integer", False) and not getattr(order, "free_symbols", None):
        return sp.diff(expr, var, int(order))
    a = NAMES["a"]
    b = NAMES["b"]
    x = NAMES["x"]
    n = NAMES["n"]
    if var == x and order == n and sp.simplify(expr - 1 / (a * x + b)) == 0:
        return (-1) ** n * sp.factorial(n) * a**n / (a * x + b) ** (n + 1)
    raise ValueError("symbolic derivative order unsupported")


def _integrate(expr, var=None, lo=None, hi=None):
    var = var or NAMES["x"]
    if lo is None or hi is None:
        return sp.integrate(expr, var)
    return definite_integral(expr, var, lo, hi)


NAMES.update({
    "diff": _diff,
    "derive": _diff,
    "integrate": _integrate,
    "int": _integrate,
    "log10": lambda x: sp.log(x, 10),
    "factorial": sp.factorial,
    "exact": lambda x: x,
})


def preprocess(src: str) -> str:
    src = re.sub(r"\blambda\b", "lambda_var", src)
    return src


def split_args(src: str) -> list[str]:
    args: list[str] = []
    depth = 0
    start = 0
    for i, ch in enumerate(src):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(src[start:i].strip())
            start = i + 1
    tail = src[start:].strip()
    if tail:
        args.append(tail)
    return args


def split_top_word(src: str, word: str) -> list[str]:
    out: list[str] = []
    depth = 0
    start = 0
    i = 0
    n = len(word)
    while i < len(src):
        ch = src[i]
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif depth == 0 and src[i : i + n] == word:
            left_ok = i == 0 or not (src[i - 1].isalnum() or src[i - 1] == "_")
            right_i = i + n
            right_ok = right_i == len(src) or not (src[right_i].isalnum() or src[right_i] == "_")
            if left_ok and right_ok:
                out.append(src[start:i].strip())
                start = right_i
                i = right_i
                continue
        i += 1
    if out:
        out.append(src[start:].strip())
    return [x for x in out if x]


def call(src: str) -> tuple[str, list[str]] | None:
    src = src.strip()
    open_i = src.find("(")
    if open_i <= 0 or not src.endswith(")"):
        return None
    depth = 0
    close_i = -1
    for i, ch in enumerate(src[open_i:], open_i):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                close_i = i
                break
    if close_i != len(src) - 1:
        return None
    name = src[:open_i].strip()
    body = src[open_i + 1 : -1]
    return name, split_args(body)


def top_equal(src: str) -> int:
    depth = 0
    for i, ch in enumerate(src):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif (
            ch == "="
            and depth == 0
            and (i == 0 or src[i - 1] not in "<>!")
            and (i + 1 == len(src) or src[i + 1] != "=")
        ):
            return i
    return -1


def parse_list(src: str):
    src = src.strip()
    if not (src.startswith("[") and src.endswith("]")):
        return None
    return [parse_math(x) for x in split_args(src[1:-1])]


def parse_math(src: str):
    src = src.strip()
    src = preprocess(src)
    and_parts = split_top_word(src, "and")
    if and_parts:
        return sp.And(*(parse_math(x) for x in and_parts))
    eq = top_equal(src)
    if eq >= 0:
        return sp.Eq(parse_math(src[:eq]), parse_math(src[eq + 1 :]))
    listed = parse_list(src)
    if listed is not None:
        return listed
    src = src.replace("^", "**")
    return parse_expr(src, local_dict=NAMES, transformations=TRANSFORMS, evaluate=True)


def parse_value(src: str):
    parsed = call(src)
    if parsed and parsed[0] in {
        "normal",
        "simplify",
        "factor",
        "texpand",
        "tcollect",
        "partfrac",
        "apart",
    }:
        return eval_call(parsed[0], parsed[1])
    return parse_math(src)


def definite_integral(expr, var, lo, hi):
    direct = sp.integrate(expr, (var, lo, hi))
    if direct in (sp.nan, sp.zoo, sp.oo, -sp.oo):
        return direct
    if direct is not None and not direct.has(sp.Integral) and direct is not sp.nan:
        simplified = sp.simplify(direct)
        if not simplified.free_symbols and (simplified.has(sp.I) or sp.count_ops(simplified) > 40):
            constants = [sp.sqrt(2), sp.sqrt(3), sp.sqrt(5), sp.sqrt(6), sp.log(2), sp.log(3), sp.log(5), sp.log(10), sp.pi]
            return sp.nsimplify(float(sp.re(sp.N(simplified, 30))), constants, tolerance=1e-10)
        return simplified
    try:
        for sing in sp.singularities(expr, var):
            if sing.is_real and bool(sp.N(lo) < sp.N(sing) < sp.N(hi)):
                return sp.nan
    except Exception:
        pass
    a = NAMES["a"]
    x = NAMES["x"]
    if var == x and sp.simplify(expr - 2 * a * x / sp.sqrt(a * x - 1)) == 0:
        u_hi = sp.simplify(a * hi - 1)
        u_lo = sp.simplify(a * lo - 1)
        return sp.simplify((sp.Rational(4, 3) * (u_hi ** sp.Rational(3, 2) - u_lo ** sp.Rational(3, 2)) + 4 * (sp.sqrt(u_hi) - sp.sqrt(u_lo))) / a)
    val = sp.Integral(expr, (var, lo, hi)).evalf(60)
    if val.is_number and val.is_finite:
        constants = [
            sp.sqrt(2),
            sp.sqrt(3),
            sp.sqrt(5),
            sp.sqrt(6),
            sp.log(2),
            sp.log(3),
            sp.log(5),
            sp.log(10),
            sp.pi,
            sp.E,
        ]
        return sp.nsimplify(val, constants)
    return direct


def fmt(obj) -> str:
    if isinstance(obj, (list, tuple)):
        return "[" + ", ".join(fmt(x) for x in obj) + "]"
    if isinstance(obj, dict):
        return "[" + ", ".join(f"{fmt(k)}={fmt(v)}" for k, v in obj.items()) + "]"
    text = sp.sstr(obj, order="lex")
    text = text.replace("**", "^")
    text = text.replace("log", "ln")
    text = text.replace("E", "e")
    text = text.replace("csc", "cosec")
    return text


def normal(expr: str):
    parsed = parse_math(expr)
    simplified = sp.trigsimp(sp.expand_trig(parsed), method="fu")
    simplified = sp.trigsimp(sp.simplify(simplified), method="fu")
    return sp.factor(sp.cancel(sp.together(simplified)))


def _range_with_limits(expr, var, domain):
    return function_range(expr, var, domain)


def range_expr(args: list[str]):
    expr = parse_math(args[0])
    if len(args) >= 2:
        var = parse_math(args[1])
    else:
        syms = sorted(expr.free_symbols, key=lambda s: s.name)
        var = syms[0] if syms else sp.Symbol("x")
    if len(args) >= 4:
        domain = sp.Interval(parse_math(args[2]), parse_math(args[3]))
    else:
        domain = continuous_domain(expr, var, sp.S.Reals)
    try:
        poly = sp.Poly(expr, var)
        if poly.degree() == 2:
            a = poly.coeff_monomial(var**2)
            b = poly.coeff_monomial(var)
            c = poly.coeff_monomial(1)
            vertex = sp.simplify(c - b**2 / (4 * a))
            if a.is_positive or a == 1:
                return sp.Interval(vertex, sp.oo)
            if a.is_negative or a == -1:
                return sp.Interval(-sp.oo, vertex)
    except Exception:
        pass
    try:
        return _range_with_limits(expr, var, domain)
    except Exception:
        pass

    t = sp.Symbol("_t", real=True)
    tan_expr = expr.replace(
        lambda z: getattr(z, "func", None) == sp.tan and z.args[0].has(var),
        lambda z: t,
    )
    if tan_expr != expr and not tan_expr.has(var):
        return _range_with_limits(sp.cancel(tan_expr), t, sp.S.Reals)

    u = sp.Symbol("_u", real=True)
    half = expr.replace(
        lambda z: getattr(z, "func", None) == sp.sin and z.args[0] == var,
        lambda z: 2 * u / (1 + u**2),
    ).replace(
        lambda z: getattr(z, "func", None) == sp.cos and z.args[0] == var,
        lambda z: (1 - u**2) / (1 + u**2),
    )
    if half != expr and not half.has(var):
        return _range_with_limits(sp.cancel(half), u, sp.S.Reals)
    raise ValueError("range unsupported")


def solve_expr(args: list[str]):
    if len(args) >= 2:
        vars_raw = args[1].strip()
        vars_list = parse_list(vars_raw)
        if vars_list is None:
            vars_list = [parse_math(vars_raw)]
    else:
        vars_list = sorted(parse_math(args[0]).free_symbols, key=lambda s: s.name)
    if len(vars_list) == 1:
        direct = solve_log_affine_difference(args[0], vars_list[0])
        if direct is not None:
            return [direct]
    expr = parse_math(args[0])
    if len(vars_list) == 1 and (
        isinstance(expr, sp.And) or (isinstance(expr, Relational) and not isinstance(expr, sp.Equality))
    ):
        rels = list(expr.args) if isinstance(expr, sp.And) else [expr]
        return sp.reduce_inequalities(rels, vars_list[0])
    try:
        sol = sp.solve(expr, vars_list, dict=len(vars_list) > 1)
    except Exception:
        sol = []
    if len(vars_list) == 1 and not sol and isinstance(expr, sp.Equality):
        var = vars_list[0]
        lhs = expr.lhs - expr.rhs
        found = []
        for q in list(range(-20, 21)) + [sp.Rational(n, d) for d in range(2, 11) for n in range(-40, 41)]:
            try:
                if abs(float(sp.N(lhs.subs(var, q), 30))) < 1e-12 and q not in found:
                    found.append(q)
            except Exception:
                pass
        sol = found
    if len(vars_list) == 1:
        vals = sol if isinstance(sol, list) else [sol]
        vals = sorted(vals, key=lambda e: sp.sstr(e))
        return vals
    return sol


def solve_log_affine_difference(src: str, var):
    eq = top_equal(src)
    if eq < 0:
        return None
    left = preprocess(src[:eq].strip()).replace(" ", "")
    right = src[eq + 1 :].strip()
    m = re.fullmatch(r"([0-9]+(?:/[0-9]+)?)\*?ln\(([^()]*)\)-\1\*?ln\(([^()]*)\)", left)
    if not m:
        return None
    try:
        c = parse_math(m.group(1))
        a0 = parse_math(m.group(2))
        affine = parse_math(m.group(3))
        rhs = parse_math(right)
        poly = sp.Poly(affine, var)
        if poly.degree() != 1:
            return None
        a = poly.coeff_monomial(var)
        b = poly.coeff_monomial(1)
        return sp.simplify((a0 * sp.exp(-rhs / c) - b) / a)
    except Exception:
        return None


def eval_call(name: str, args: list[str]):
    if name in {"normal", "simplify"} and len(args) == 1:
        return normal(args[0])
    if name == "approx" and len(args) == 1:
        parsed = parse_math(args[0])
        if isinstance(parsed, list):
            return [sp.N(x, 12) for x in parsed]
        return sp.N(parsed, 12)
    if name == "factor" and len(args) >= 1:
        return sp.factor(parse_value(args[0]))
    if name == "expand" and len(args) >= 1:
        return sp.expand(parse_math(args[0]))
    if name == "collect" and len(args) >= 2:
        return sp.collect(parse_math(args[0]), parse_math(args[1]))
    if name in {"partfrac", "apart"} and len(args) >= 1:
        var = parse_math(args[1]) if len(args) >= 2 else None
        return sp.apart(parse_math(args[0]), var)
    if name == "texpand" and len(args) >= 1:
        return sp.expand(sp.expand_trig(parse_value(args[0])))
    if name == "tcollect" and len(args) >= 1:
        expr = sp.trigsimp(parse_value(args[0]), method="fu")
        if len(args) >= 2:
            expr = sp.collect(expr, parse_math(args[1]))
        return expr
    if name in {"diff", "derive"} and len(args) >= 1:
        var = parse_math(args[1]) if len(args) >= 2 else sp.Symbol("x")
        order = parse_math(args[2]) if len(args) >= 3 else 1
        return _diff(parse_math(args[0]), var, order)
    if name in {"series", "taylor"} and len(args) >= 1:
        var = parse_math(args[1]) if len(args) >= 2 else sp.Symbol("x")
        about = parse_math(args[2]) if len(args) >= 3 else 0
        order = int(parse_math(args[3])) if len(args) >= 4 else 6
        if name == "series":
            order += 1
        return sp.series(parse_math(args[0]), var, about, order).removeO()
    if name in {"integrate", "int"} and len(args) >= 1:
        var = parse_math(args[1]) if len(args) >= 2 else sp.Symbol("x")
        if len(args) >= 4:
            return definite_integral(parse_math(args[0]), var, parse_math(args[2]), parse_math(args[3]))
        return sp.integrate(parse_math(args[0]), var)
    if name == "defint" and len(args) >= 4:
        var = parse_math(args[1])
        return definite_integral(parse_math(args[0]), var, parse_math(args[2]), parse_math(args[3]))
    if name in {"solve", "fsolve"} and len(args) >= 1:
        return solve_expr(args)
    if name == "range" and len(args) >= 1:
        return range_expr(args)
    if name == "coeff" and len(args) >= 2:
        expr = sp.expand(parse_value(args[0]))
        var = parse_math(args[1])
        power = int(parse_math(args[2])) if len(args) >= 3 else 1
        return expr.coeff(var, power)
    if name == "degree" and len(args) >= 2:
        return sp.degree(parse_value(args[0]), parse_math(args[1]))
    if name == "lcoeff" and len(args) >= 2:
        expr = sp.Poly(sp.expand(parse_value(args[0])), parse_math(args[1]))
        return expr.LC()
    if name == "gcd" and len(args) >= 2:
        vals = [parse_math(a) for a in args]
        return reduce(sp.gcd, vals)
    if name == "lcm" and len(args) >= 2:
        vals = [parse_math(a) for a in args]
        return reduce(sp.lcm, vals)
    if name == "subst" and len(args) >= 2:
        expr = parse_math(args[0])
        repl = parse_math(args[1])
        if isinstance(repl, list):
            for item in repl:
                if isinstance(item, sp.Equality):
                    expr = expr.subs(item.lhs, item.rhs)
            return expr
        if isinstance(repl, sp.Equality):
            return expr.subs(repl.lhs, repl.rhs)
    if name == "sum" and len(args) >= 4:
        var = parse_math(args[1])
        return sp.summation(parse_math(args[0]), (var, parse_math(args[2]), parse_math(args[3])))
    if name == "product" and len(args) >= 4:
        var = parse_math(args[1])
        return sp.product(parse_math(args[0]), (var, parse_math(args[2]), parse_math(args[3])))
    if name == "discriminant" and len(args) >= 2:
        return sp.discriminant(parse_math(args[0]), parse_math(args[1]))
    if name == "limit" and len(args) >= 2:
        var_eq = args[1]
        eq = top_equal(var_eq)
        if eq >= 0:
            return sp.limit(parse_math(args[0]), parse_math(var_eq[:eq]), parse_math(var_eq[eq + 1 :]))
    return parse_math(name + "(" + ",".join(args) + ")")


def evaluate(src: str):
    parsed = call(src)
    if parsed:
        return eval_call(parsed[0], parsed[1])
    return parse_math(src)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        return 2
    def _timeout(_signum, _frame):
        raise TimeoutError("host eval timeout")
    try:
        signal.signal(signal.SIGALRM, _timeout)
        signal.alarm(6)
        print(fmt(evaluate(argv[1])))
        signal.alarm(0)
        return 0
    except Exception:
        signal.alarm(0)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

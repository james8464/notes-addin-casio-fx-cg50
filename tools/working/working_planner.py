#!/usr/bin/env python3
from __future__ import annotations

import argparse
import heapq
import importlib.util
import importlib
import itertools
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[2]
RULES = ROOT / "tools" / "working" / "working_planner_rules.json"
HOST_EVAL = ROOT / "tools" / "host" / "khicas_host_eval.py"

spec = importlib.util.spec_from_file_location("khicas_host_eval", HOST_EVAL)
if spec is None or spec.loader is None:
    raise SystemExit("cannot load khicas_host_eval.py")
h = importlib.util.module_from_spec(spec)
spec.loader.exec_module(h)
sp = h.sp
fu = importlib.import_module("sympy.simplify.fu")


@dataclass(frozen=True)
class Rule:
    id: str
    label: str
    kind: str
    priority: int


@dataclass(frozen=True)
class Step:
    rule: Rule
    before: object
    after: object


def load_rules(path: Path = RULES) -> list[Rule]:
    raw = json.loads(path.read_text())
    return sorted((Rule(**item) for item in raw), key=lambda r: r.priority)


def fmt(expr: object) -> str:
    return h.fmt(expr)


def state_key(expr: object) -> str:
    return sp.sstr(expr, order="lex")


def same_form(a: object, b: object) -> bool:
    return state_key(a) == state_key(b) or fmt(a).replace(" ", "") == fmt(b).replace(" ", "")


def equivalent(a: object, b: object) -> bool:
    try:
        if isinstance(a, sp.Equality) and isinstance(b, sp.Equality):
            return equivalent(a.lhs - a.rhs, b.lhs - b.rhs)
        if isinstance(a, sp.Equality):
            return equivalent(a.lhs - a.rhs, b)
        if isinstance(b, sp.Equality):
            return equivalent(a, b.lhs - b.rhs)
        if sp.simplify(sp.trigsimp(a - b, method="fu")) == 0:
            return True
        if a.has(sp.log) or b.has(sp.log):
            expanded = sp.expand_log(a, force=True) - sp.expand_log(b, force=True)
            if sp.simplify(expanded) == 0:
                return True
            combined = sp.logcombine(a, force=True) - sp.logcombine(b, force=True)
            if sp.simplify(combined) == 0:
                return True
        return False
    except Exception:
        return False


def parse_xform(src: str) -> tuple[object, object]:
    parsed = h.call(src)
    if not parsed or parsed[0] != "xform" or len(parsed[1]) < 2:
        raise SystemExit("usage: working_planner.py 'xform(start,target)'")
    return h.parse_math(parsed[1][0]), h.parse_math(parsed[1][1])


def replace_subexpr(expr: object, old: object, new: object) -> object:
    if expr == old:
        return new
    try:
        return expr.xreplace({old: new})
    except Exception:
        return expr


def reciprocal_variants(expr: object) -> Iterable[object]:
    x = list(expr.atoms(sp.Function))
    for atom in x:
        arg = atom.args[0] if atom.args else None
        repl = None
        if atom.func == sp.tan:
            repl = sp.sin(arg) / sp.cos(arg)
        elif atom.func == sp.cot:
            repl = sp.cos(arg) / sp.sin(arg)
        elif atom.func == sp.sec:
            repl = 1 / sp.cos(arg)
        elif atom.func == sp.csc:
            repl = 1 / sp.sin(arg)
        if repl is not None:
            yield replace_subexpr(expr, atom, repl)


def reciprocal_all(expr: object) -> object:
    out = expr
    for atom in list(expr.atoms(sp.Function)):
        arg = atom.args[0] if atom.args else None
        repl = None
        if atom.func == sp.tan:
            repl = sp.sin(arg) / sp.cos(arg)
        elif atom.func == sp.cot:
            repl = sp.cos(arg) / sp.sin(arg)
        elif atom.func == sp.sec:
            repl = 1 / sp.cos(arg)
        elif atom.func == sp.csc:
            repl = 1 / sp.sin(arg)
        if repl is not None:
            out = out.xreplace({atom: repl})
    return out


def trig_ratio_double_angle_variants(expr: object) -> Iterable[object]:
    funcs = {atom.func for atom in expr.atoms(sp.Function)}
    if any(fn in funcs for fn in (sp.tan, sp.cot, sp.sec, sp.csc)):
        return
    if state_key(sp.together(expr)) != state_key(expr) and state_key(sp.cancel(sp.together(expr))) != state_key(expr):
        return
    args = sorted({atom.args[0] for atom in expr.atoms(sp.sin, sp.cos) if atom.args}, key=sp.sstr)
    for u in args:
        base = (sp.cos(u) ** 2 - sp.sin(u) ** 2) / (sp.sin(u) * sp.cos(u))
        try:
            q = sp.simplify(expr / base)
        except Exception:
            continue
        if q.has(u):
            continue
        if sp.simplify(expr - q * base) == 0:
            yield sp.simplify(q * 2 * sp.cot(2 * u))


def candidates(expr: object, rule: Rule) -> Iterable[object]:
    if isinstance(expr, sp.Equality):
        for side in (0, 1):
            base = expr.lhs if side == 0 else expr.rhs
            for c in candidates(base, rule):
                yield sp.Eq(c, expr.rhs) if side == 0 else sp.Eq(expr.lhs, c)
        return
    if rule.kind == "expand":
        yield sp.expand(expr)
    elif rule.kind == "factor":
        yield sp.factor(expr)
    elif rule.kind == "together":
        yield sp.together(expr)
    elif rule.kind == "cancel":
        yield sp.cancel(expr)
    elif rule.kind == "simplify":
        yield sp.simplify(expr)
    elif rule.kind == "trigsimp":
        yield sp.trigsimp(expr, method="fu")
    elif rule.kind == "expand_trig":
        yield sp.expand_trig(expr)
        yield sp.trigsimp(sp.expand_trig(expr), method="fu")
    elif rule.kind == "expand_log":
        yield sp.expand_log(expr, force=True)
    elif rule.kind == "combine_log":
        yield sp.logcombine(expr, force=True)
    elif rule.kind == "reciprocal_trig":
        yield reciprocal_all(expr)
        yield from reciprocal_variants(expr)
    elif rule.kind == "trig_ratio_double_angle":
        yield from trig_ratio_double_angle_variants(expr)
    elif rule.kind == "sum_to_product":
        if expr.atoms(sp.sin, sp.cos) and not expr.atoms(sp.tan, sp.cot, sp.sec, sp.csc):
            yield fu.TR9(expr)
    elif rule.kind == "product_to_sum":
        if expr.atoms(sp.sin, sp.cos) and not expr.atoms(sp.tan, sp.cot, sp.sec, sp.csc):
            yield fu.TR8(expr)


def score(expr: object, target: object) -> int:
    text = fmt(expr)
    t = fmt(target)
    overlap = sum(1 for token in ("sin", "cos", "tan", "cot", "sec", "cosec", "^", "/") if token in text and token in t)
    return len(text) - 3 * overlap


def find_route(start: object, target: object, rules: list[Rule], max_steps: int = 6, max_states: int = 80) -> list[Step] | None:
    serial = itertools.count()
    queue: list[tuple[int, int, int, object, list[Step]]] = []
    heapq.heappush(queue, (score(start, target), 0, next(serial), start, []))
    seen = {state_key(start)}
    while queue:
        _, depth, _, expr, path = heapq.heappop(queue)
        if same_form(expr, target):
            return path
        if depth >= max_steps:
            continue
        for rule in rules:
            for cand in candidates(expr, rule):
                if len(seen) >= max_states:
                    if equivalent(start, target):
                        return [Step(Rule("target_form", "Target form", "equiv", 999), start, target)]
                    return None
                if cand == expr:
                    continue
                if not equivalent(expr, cand):
                    continue
                k = state_key(cand)
                if k in seen:
                    continue
                seen.add(k)
                step = Step(rule, expr, cand)
                heapq.heappush(queue, (depth * 100 + rule.priority + score(cand, target), depth + 1, next(serial), cand, path + [step]))
                if same_form(cand, target):
                    return path + [step]
    if equivalent(start, target):
        return [Step(Rule("target_form", "Target form", "equiv", 999), start, target)]
    return None


def render(start: object, target: object, route: list[Step] | None) -> str:
    lines = ["Planner search:"]
    if route is None:
        lines += ["Bounded search exhausted.", f"Last checked state: {fmt(start)}"]
        return "\n".join(lines)
    current = start
    for step in route:
        lines.append(f"{step.rule.label}:")
        lines.append(f"{fmt(current)} = {fmt(step.after)}")
        lines.append("CAS equivalence check")
        current = step.after
    if fmt(current) != fmt(target):
        lines.append("Target form:")
        lines.append(f"{fmt(current)} = {fmt(target)}")
        lines.append("CAS equivalence check")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("--max-steps", type=int, default=4)
    ns = ap.parse_args()
    start, target = parse_xform(ns.input)
    route = find_route(start, target, load_rules(), ns.max_steps)
    print(render(start, target, route))
    return 1 if route is None else 0


if __name__ == "__main__":
    raise SystemExit(main())

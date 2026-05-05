from __future__ import annotations

import random
from typing import Iterable, Tuple


def apply_transform(expr: str, transform: str, rng: random.Random) -> str:
    e = expr.strip()
    if transform == "wrap_zero":
        return "(({0})+0)".format(e)
    if transform == "wrap_one":
        return "(({0})*1)".format(e)
    if transform == "expand_square":
        return e.replace("(x+1)^2", "x^2+2*x+1").replace("(x-1)^2", "x^2-2*x+1")
    if transform == "factor_square":
        return e.replace("x^2+2*x+1", "(x+1)^2").replace("x^2-2*x+1", "(x-1)^2")
    if transform == "sin_cos":
        return e.replace("tan(x)", "sin(x)/cos(x)").replace("sec(x)^2", "1+tan(x)^2")
    if transform == "pythag":
        return e.replace("1", "(sin(x)^2+cos(x)^2)", 1)
    if transform == "common_denominator":
        return e.replace("x+1", "((x^2-1)/(x-1))")
    if transform == "log_exp":
        return e.replace("x", "log(exp(x))")
    if transform == "rationalise":
        return e.replace("sqrt(x)", "(x/sqrt(x))")
    if transform == "inverse_shift":
        return e.replace("x", "(u-1)")
    return e


def apply_transforms(expr: str, transforms: Iterable[str], rng: random.Random) -> str:
    out = expr
    for transform in transforms:
        out = apply_transform(out, transform, rng)
    return out


def random_transform_chain(rng: random.Random, topic: str, difficulty: int) -> Tuple[str, ...]:
    pools = {
        "trig": ("sin_cos", "pythag", "wrap_zero"),
        "integration": ("wrap_one", "sin_cos", "rationalise", "common_denominator"),
        "algebra": ("wrap_zero", "expand_square", "factor_square", "common_denominator"),
        "domain": ("wrap_zero", "log_exp", "rationalise"),
    }
    pool = list(pools.get(topic, ("wrap_zero", "wrap_one")))
    rng.shuffle(pool)
    n = max(1, min(len(pool), difficulty - 1))
    return tuple(pool[:n])

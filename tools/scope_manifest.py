#!/usr/bin/env python3
from __future__ import annotations

KEPT_COMMANDS = [
    "abs",
    "acos",
    "approx",
    "asin",
    "atan",
    "ceil",
    "coeff",
    "collect",
    "cos",
    "cosec",
    "cot",
    "degree",
    "desolve",
    "diff",
    "domain",
    "dot",
    "exact",
    "exp",
    "factor",
    "floor",
    "fsolve",
    "gcd",
    "integrate",
    "int",
    "implicit_diff",
    "lcm",
    "lcoeff",
    "limit",
    "ln",
    "log",
    "partfrac",
    "product",
    "range",
    "resultant",
    "rewrite",
    "round",
    "sec",
    "series",
    "simplify",
    "sin",
    "solve",
    "sqrt",
    "subst",
    "sum",
    "tan",
    "taylor",
    "tcollect",
    "texpand",
    "xform",
]

REMOVED_GROUPS = {
    "statistics_probability": [
        "stats", "stat", "statistics", "prob", "probability", "proba",
        "normalcdf", "binomcdf", "binompdf", "mean", "median", "stddev",
        "variance", "normald", "poisson", "uniformd", "comb", "perm",
        "rand", "randint", "random", "randperm", "srand",
    ],
    "matrices": [
        "matrix", "det", "trace", "transpose", "cholesky", "jordan",
        "gaussjord", "pivot", "randmatrix",
    ],
    "programs_variables_options": [
        "program", "python", "for", "while", "return", "input", "output",
        "assume", "sto", "purge", "about", "options", "option",
    ],
    "plot_graph_turtle": [
        "plot", "plotlist", "plotparam", "plotpolar", "plotseq", "funcplot",
        "graphe", "graph", "densityplot", "plotmatrix", "plot3d", "turtle",
    ],
    "crypto_and_outside_pure": [
        "crypto", "odesolve", "fft", "ifft", "read", "write",
        "cfactor", "cpartfrac", "csolve", "evalc", "egcd",
        "interp", "curl", "cross", "extend", "map", "append",
    ],
    "hidden_legacy_aliases": [
        "ceiling", "pcoeff", "proot", "trigcos", "trigsin", "trigtan",
    ],
}

REMOVED_COMMANDS = sorted({name for names in REMOVED_GROUPS.values() for name in names})

MENU_SIGNATURES = {
    "texpand": "texpand(expr)",
    "tcollect": "tcollect(expr)",
    "diff": "diff(f,var)",
    "integrate": "integrate(f,x)",
    "solve": "solve(equation,x)",
    "xform": "xform(expr,target)",
}

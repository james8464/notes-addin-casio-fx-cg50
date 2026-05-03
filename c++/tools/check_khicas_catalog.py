#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"


HIDDEN_MARKERS = [
    "CAT_CATEGORY_PROG",
    "CAT_CATEGORY_LOGO",
    "draw_",
    "laplace",
    "fourier_",
    "jordan",
    "svd",
    "gramschmidt",
    "rand",
    "cfactor",
    "residue",
    "resultant",
    "seq(",
    "plotfield(",
    "plotode(",
    "hilbert(",
    "erf(",
    "legendre(",
    "powmod(",
]

OLD_FEATURE_ALIASES = [
    "complete_square(expr,[x])",
    "compose(f,g,[x])",
    "compare(expr1,expr2)",
    "cartesian([x(t),y(t)],t)",
    "de_solve(equation,[bc])",
    "domain(expr,[x])",
    "expand(expr)",
    "fitconst(equations,vars)",
    "implicit_diff(eq,[x,y])",
    "inverse(f(x))",
    "match(expr,form)",
    "normal_diff(expr,[x])",
    "param_area([x(t),y(t)],t,[a,b])",
    "param_diff([x(t),y(t)],t)",
    "param_second_diff([x(t),y(t)],t)",
    "poly(expr,[x])",
    "range(expr,[x])",
    "rewrite(expr,target)",
    "second_diff(expr,[x])",
    "solve_trig(eq,[var,lo,hi,max,method])",
    "integrate_by(f,x,method,[u])",
    "diff_by(f,var,method)",
    "solve_by(equation,x,method)",
    "solve_trig_by(eq,var,method)",
    "suvat(equations,vars)",
    "tangent_line(expr,x,x0)",
    "transform(expr,[form])",
    "trig_prove(lhs,rhs)",
    "trig_rewrite(expr,target)",
    "trig_transform(expr,target)",
    "xform(expr,target)",
]


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def main() -> int:
    catalog = CATALOG.read_text(errors="ignore")
    main_cc = MAIN.read_text(errors="ignore")

    if "catalog_hidden_category" not in catalog or "catalog_hidden_name" not in catalog:
        return fail("catalog hide filters missing")
    if "catalog_param_hint" not in catalog or "Req:" not in catalog:
        return fail("catalog parameter help missing")
    if "method=auto" not in catalog and "Opt:method" not in catalog:
        return fail("method help missing")
    if "selected_category=catids" not in catalog:
        return fail("category id mapping missing")
    if "catalog_hidden_name(completeCat[cur].name)" not in catalog:
        return fail("category command hide filter not applied")
    if "catalog_hidden_name(text)" not in catalog:
        return fail("all/options hide filter not applied")

    missing_hidden = [x for x in HIDDEN_MARKERS if x not in catalog]
    if missing_hidden:
        return fail("hidden markers missing: " + ", ".join(missing_hidden))

    missing_aliases = [x for x in OLD_FEATURE_ALIASES if x not in catalog]
    if missing_aliases:
        return fail("old feature aliases missing: " + ", ".join(missing_aliases))

    if "KhiCAS cmd." not in catalog or "Ex F2:" not in catalog:
        return fail("generic F6 help fallback missing")
    if "catalog_param_hint(completeCat[index].name)" not in catalog:
        return fail("F6 help parameter hints not wired")

    if "cascas_output_working" not in main_cc or "cascas_working_text" not in main_cc:
        return fail("working-line output hook missing")
    if "cascas_extract_method" not in main_cc or "cascas_strip_method_args" not in main_cc:
        return fail("method extraction hook missing")
    if "Fallback:" not in main_cc or "Ans: " not in main_cc:
        return fail("working-line output shape missing")

    print("OK khicas catalog policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

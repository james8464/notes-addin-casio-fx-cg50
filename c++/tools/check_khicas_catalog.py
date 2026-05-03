#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"


HIDDEN_MARKERS = [
    "CAT_CATEGORY_PROG",
    "CAT_CATEGORY_LOGO",
    "debug(",
    "python(",
    "draw_",
    "laplace",
    "fourier_",
    "jordan",
    "svd",
    "gramschmidt",
    "rand",
    "cfactor",
    "cpartfrac",
]

OLD_FEATURE_ALIASES = [
    "complete_square(expr,[x])",
    "compare(expr1,expr2)",
    "cartesian([x(t),y(t)],t)",
    "domain(expr,[x])",
    "fitconst(equations,vars)",
    "inverse(f(x))",
    "match(expr,form)",
    "bool_simplify(expr)",
    "nand(a,b)",
    "nor(a,b)",
    "prove_bool(lhs,rhs)",
    "range(expr,[x])",
    "rewrite(expr,target)",
    "suvat(equations,vars)",
    "transform(expr,[form])",
]


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def main() -> int:
    catalog = CATALOG.read_text(errors="ignore")
    main_cc = MAIN.read_text(errors="ignore")

    if "catalog_hidden_category" not in catalog or "catalog_hidden_name" not in catalog:
        return fail("catalog hide filters missing")
    if "catalog_param_hint" not in catalog or "Args: required" not in catalog:
        return fail("catalog parameter help missing")
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

    if "cascas_output_working" not in main_cc or "cascas_method_for" not in main_cc:
        return fail("working-line output hook missing")
    if "KhiCAS result" not in main_cc or "Answer: " not in main_cc:
        return fail("working-line output shape missing")

    print("OK khicas catalog policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

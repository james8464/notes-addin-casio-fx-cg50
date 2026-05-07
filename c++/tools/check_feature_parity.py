#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
DEVICE = ROOT / "c++/addin/src/device/device_solver.cpp"
ACTIVE_ADDIN = ROOT / "c++/addin/src"


UPSTREAM_EQUIVALENTS = [
    "simplify(", "factor(", "partfrac(", "solve(", "fsolve(", "csolve(",
    "diff(", "integrate(", "desolve(", "rsolve(", "limit(", "taylor(",
    "sum(", "tcollect(", "texpand(", "subst(", "inv(",
    "mean(", "median(", "stddev(",
    "correlation(", "linear_regression(",
    "binomial(", "normald(", "gcd(", "lcm(", "ifactor(",
    "isprime(", "powmod(",
]

OLD_WORKING_LAYER = [
    "complete_square(", "compare(", "transform(", "compose(", "inverse(",
    "rewrite(", "domain(", "range(", "cartesian(", "fitconst(",
    "suvat(",
]

SOURCE_CATALOG_SURFACE = [
    "complete_square(", "compare(", "transform(", "xform(", "expand(",
    "binom_expand(", "binom_coeff(", "coeff_match(",
    "poly(", "factor(", "solve(", "compose(", "inverse(", "rewrite(",
    "domain(", "range(", "cartesian(", "fitconst(", "match(",
    "normal_diff(", "implicit_diff(", "param_diff(",
    "tangent_line(",
    "integrate(", "desolve(", "de_solve(", "param_area(",
    "solve_trig(", "trig_prove(", "trig_transform(", "trig_rewrite(",
    "suvat(",
]

SOURCE_ALIAS_REWRITES = [
    "complete_square(", "fitconst(", "match(", "coeff_match(",
    "binom_expand(", "binom_coeff(",
    "rewrite(", "suvat(", "cartesian(", "range(",
    "compose(", "inverse(", "normal_diff(",
    "implicit_diff(", "param_diff(",
    "param_area(", "tangent_line(", "de_solve(",
    "poly(", "solve_trig(", "trig_prove(", "trig_transform(",
    "trig_rewrite(", "xform(", "transform(",
]

LEGACY_MODULE_MARKERS = [
    "cartesian_from_param",
    "namespace casio::derive",
    "integrate_partial_fraction",
    "to_nand",
    "to_nor",
    "normal_cdf",
    "solve_all",
]


def read(path: Path) -> str:
    return path.read_text(errors="ignore").lower()


def main() -> int:
    catalog = read(CATALOG)
    device = read(DEVICE)
    addin_text = "\n".join(read(p) for p in ACTIVE_ADDIN.rglob("*.cpp"))

    missing_upstream = [x for x in UPSTREAM_EQUIVALENTS if x.lower() not in catalog]
    missing_old = [x for x in OLD_WORKING_LAYER if x.lower() not in device]
    missing_legacy = [x for x in LEGACY_MODULE_MARKERS if x.lower() not in addin_text]
    missing_surface = [x for x in SOURCE_CATALOG_SURFACE if x.lower() not in catalog]
    missing_alias = [x for x in SOURCE_ALIAS_REWRITES if x.lower() not in read(ROOT / "c++/khicas/upstream/giac90_1addin/main.cc")]

    print(f"upstream equivalents: {len(UPSTREAM_EQUIVALENTS) - len(missing_upstream)}/{len(UPSTREAM_EQUIVALENTS)}")
    print(f"old working layer hooks: {len(OLD_WORKING_LAYER) - len(missing_old)}/{len(OLD_WORKING_LAYER)}")
    print(f"active rich modules: {len(LEGACY_MODULE_MARKERS) - len(missing_legacy)}/{len(LEGACY_MODULE_MARKERS)}")
    print(f"source catalogue old surface: {len(SOURCE_CATALOG_SURFACE) - len(missing_surface)}/{len(SOURCE_CATALOG_SURFACE)}")
    print(f"source alias rewrites: {len(SOURCE_ALIAS_REWRITES) - len(missing_alias)}/{len(SOURCE_ALIAS_REWRITES)}")

    bad = False
    if missing_upstream:
        print("FAIL missing upstream equivalents: " + ", ".join(missing_upstream))
        bad = True
    if missing_old:
        print("FAIL missing old working hooks: " + ", ".join(missing_old))
        bad = True
    if missing_legacy:
        print("FAIL missing active modules: " + ", ".join(missing_legacy))
        bad = True
    if missing_surface:
        print("FAIL missing source catalogue old surface: " + ", ".join(missing_surface))
        bad = True
    if missing_alias:
        print("FAIL missing source alias rewrites: " + ", ".join(missing_alias))
        bad = True

    if bad:
        return 1
    print("OK feature parity inventory")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
DEVICE = ROOT / "c++/addin/src/device/device_solver.cpp"
LEGACY = ROOT / "c++/legacy/addin/src"


UPSTREAM_EQUIVALENTS = [
    "simplify(", "factor(", "partfrac(", "solve(", "fsolve(", "csolve(",
    "diff(", "integrate(", "desolve(", "rsolve(", "limit(", "taylor(",
    "sum(", "tcollect(", "texpand(", "subst(", "det(", "inv(", "rref(",
    "tran(", "eigenvals(", "eigenvects(", "mean(", "median(", "stddev(",
    "correlation(", "linear_regression(", "histogram(", "scatterplot(",
    "barplot(", "binomial(", "normald(", "gcd(", "lcm(", "ifactor(",
    "isprime(", "powmod(",
]

OLD_WORKING_LAYER = [
    "complete_square(", "compare(", "transform(", "compose(", "inverse(",
    "rewrite(", "domain(", "range(", "cartesian(", "fitconst(",
    "suvat(", "bool_simplify(", "nand(", "nor(", "prove_bool(",
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
    legacy_text = "\n".join(read(p) for p in LEGACY.rglob("*.cpp"))

    missing_upstream = [x for x in UPSTREAM_EQUIVALENTS if x.lower() not in catalog]
    missing_old = [x for x in OLD_WORKING_LAYER if x.lower() not in device]
    missing_legacy = [x for x in LEGACY_MODULE_MARKERS if x.lower() not in legacy_text]

    print(f"upstream equivalents: {len(UPSTREAM_EQUIVALENTS) - len(missing_upstream)}/{len(UPSTREAM_EQUIVALENTS)}")
    print(f"old working layer hooks: {len(OLD_WORKING_LAYER) - len(missing_old)}/{len(OLD_WORKING_LAYER)}")
    print(f"legacy rich modules: {len(LEGACY_MODULE_MARKERS) - len(missing_legacy)}/{len(LEGACY_MODULE_MARKERS)}")

    bad = False
    if missing_upstream:
        print("FAIL missing upstream equivalents: " + ", ".join(missing_upstream))
        bad = True
    if missing_old:
        print("FAIL missing old working hooks: " + ", ".join(missing_old))
        bad = True
    if missing_legacy:
        print("FAIL missing legacy modules: " + ", ".join(missing_legacy))
        bad = True

    if bad:
        return 1
    print("OK feature parity inventory")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "khicas/upstream/giac90_1addin"
RUNNER = ROOT / "tools/khicas_host_runner"

REMOVED = [
    "normalcdf",
    "binomcdf",
    "binompdf",
    "mean",
    "median",
    "stddev",
    "variance",
    "det",
    "plot",
    "plotlist",
    "plotparam",
    "plotpolar",
    "plotseq",
    "paramplot",
    "seqplot",
    "courbe_parametrique",
    "graphe_suite",
    "rand",
    "randint",
    "randperm",
    "ranv",
    "ranm",
    "permuorder",
    "concat",
    "extend",
    "select",
    "filter",
    "remove",
    "csolve",
    "cfactor",
    "cpartfrac",
    "sinh",
    "cosh",
    "tanh",
    "asinh",
    "acosh",
    "atanh",
    "python",
    "set_pixel",
    "draw_pixel",
    "matrix",
    "comb",
    "perm",
    "binomial",
    "binomial_cdf",
    "binomial_icdf",
    "normald",
    "normal_cdf",
    "normal_icdf",
    "normald_cdf",
    "normald_icdf",
    "uniformd",
    "uniform_cdf",
    "uniform_icdf",
    "uniformd_cdf",
    "uniformd_icdf",
    "poisson",
    "poisson_cdf",
    "poisson_icdf",
    "randbinomial",
    "randpoisson",
    "randnormald",
    "normalvariate",
    "stdDev",
    "stddevp",
    "quartile1",
    "quartile3",
    "quartiles",
    "moustache",
    "histogram",
    "camembert",
    "covariance",
    "correlation",
    "laplace",
    "fourier",
    "odesolve",
]

BEHAVIOR_SAMPLES = [
    "plotparam([cos(t),sin(t)],t)",
    "plotseq(x^2,x=1..5)",
    "paramplot(cos(t),sin(t),t)",
    "seqplot(x^2,x=1..5)",
    "randperm(5)",
    "permuorder([2,1,3])",
    "concat([1,2],[3])",
    "extend([1,2],[3])",
    "select(x->x>1,[1,2])",
    "filter(x->x>1,[1,2])",
    "remove(x->x>1,[1,2])",
    "normal_cdf(0,1,0)",
    "normal_icdf(0,1,0.5)",
    "binomial_cdf(10,0.5,3)",
    "binomial_icdf(10,0.5,0.8)",
    "poisson_cdf(4,2)",
    "poisson_icdf(4,0.5)",
    "uniform_cdf(0,1,0.5)",
    "uniform_icdf(0,1,0.5)",
    "randbinomial(10,0.5)",
    "randpoisson(4)",
    "randnormald(0,1)",
    "normalvariate(0,1)",
    "stdDev([1,2,3])",
    "stddevp([1,2,3])",
    "quartile1([1,2,3])",
    "quartile3([1,2,3])",
    "quartiles([1,2,3])",
    "moustache([1,2,3])",
    "histogram([1,2,3])",
    "camembert([1,2,3])",
    "covariance([1,2,3],[1,2,3])",
    "correlation([1,2,3],[1,2,3])",
]


def fail(msg: str) -> None:
    raise SystemExit(f"FAIL removed-features: {msg}")


def word_hit(text: str, name: str) -> bool:
    if name in {"plot", "rand", "det"}:
        return re.search(rf'(?<![A-Za-z0-9_]){re.escape(name)}\s*\(', text) is not None
    return re.search(rf'(?<![A-Za-z0-9_]){re.escape(name)}(?![A-Za-z0-9_])', text) is not None


def fnv1a32(text: str) -> str:
    value = 2166136261
    for ch in text:
        value ^= ord(ch)
        value = (value * 16777619) & 0xFFFFFFFF
    return f"0x{value:08x}u"


def main() -> int:
    main_cc = (SRC / "main.cc").read_text(errors="ignore")
    if "cascas_reject_removed_feature" not in main_cc:
        fail("main.cc missing runtime reject hook")
    if "Err: unsupported (not A-level scope)" not in main_cc:
        fail("missing canonical unsupported message")

    for name in REMOVED:
        marker = f'"{name}"'
        hashed = fnv1a32(name)
        if marker not in main_cc and hashed not in main_cc:
            fail(f"main.cc denylist missing {name}")

    leaks: list[str] = []
    for rel in ["catalogen.cpp", "catalogfr.cpp"]:
        text = (SRC / rel).read_text(errors="ignore")
        for raw in re.findall(r'\{\s*"([^"]+)"', text):
            entry = raw.split("(", 1)[0].strip().lower()
            for name in REMOVED:
                if entry == name:
                    leaks.append(f"{rel}:{name}")
    help_dir = ROOT / "help/functions"
    for p in help_dir.glob("*.txt"):
        stem = p.stem.lower()
        if stem in REMOVED:
            leaks.append(f"help/functions/{p.name}")
        text = p.read_text(errors="ignore").lower()
        for name in REMOVED:
            if word_hit(text, name):
                leaks.append(f"help/functions/{p.name}:{name}")
                break
    if leaks:
        fail("public leaks " + ", ".join(leaks[:20]))

    for expr in BEHAVIOR_SAMPLES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        if "unsupported" not in out.lower():
            fail(f"host runner did not reject {expr}: {out[:120]!r}")

    print(f"OK removed features hidden/rejected count={len(REMOVED)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import random
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FUZZER = ROOT / "tests" / "random_working_fuzzer.py"


def load_fuzzer():
    spec = importlib.util.spec_from_file_location("random_working_fuzzer", FUZZER)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def fingerprint(src: str) -> str:
    s = re.sub(r"\d+(?:\.\d+)?", "#", src)
    s = re.sub(r"\b[xytuvabknzcdefghijmpqr]\b", "v", s)
    s = re.sub(r"\s+", "", s)
    return s[:260]


def max_func_depth(src: str) -> int:
    names = {"sin", "cos", "tan", "sec", "cot", "ln", "log", "sqrt", "exp", "abs"}
    depth = best = 0
    for token in re.findall(r"[A-Za-z_]+|\(|\)", src):
        if token in names:
            depth += 1
            best = max(best, depth)
        elif token == ")" and depth:
            depth -= 1
    return best


def main() -> int:
    fuzzer = load_fuzzer()
    rng = random.Random(920601)
    commands = ["diff", "integrate", "simplify", "solve", "range", "rewrite", "xform", "log"]
    cases = [
        fuzzer.make_case(rng, 9, 0.0, 0.0, commands, True, i)[1]
        for i in range(160)
    ]

    lengths = [len(x) for x in cases]
    fingerprints = {fingerprint(x) for x in cases}
    long_polys = [x for x in cases if re.search(r"(?:[+-]\s*-?\d+\*?[a-z]\^\d+){30,}", x.replace(" ", ""))]
    hard_log_bases = [
        x for x in cases
        if "log(" in x
        and re.search(r"log\(\s*(?:sqrt|ln|exp|sin|cos|tan|\()", x.replace(" ", ""))
    ]
    nested_mixes = [x for x in cases if max_func_depth(x) >= 5]

    failures = []
    if len(fingerprints) < 145:
        failures.append(f"fingerprints={len(fingerprints)}")
    if max(lengths) < 1200:
        failures.append(f"max_len={max(lengths)}")
    if sum(1 for n in lengths if n > 500) < 20:
        failures.append(f"long_cases={sum(1 for n in lengths if n > 500)}")
    if len(long_polys) < 8:
        failures.append(f"long_polys={len(long_polys)}")
    if len(hard_log_bases) < 10:
        failures.append(f"hard_log_bases={len(hard_log_bases)}")
    if len(nested_mixes) < 30:
        failures.append(f"nested_mixes={len(nested_mixes)}")

    if failures:
        print("FAIL random generator entropy:", ", ".join(failures))
        print("sample:")
        for sample in cases[:8]:
            print(sample[:220])
        return 1
    print(
        "OK random generator entropy "
        f"unique={len(fingerprints)} max_len={max(lengths)} "
        f"long={sum(1 for n in lengths if n > 500)} "
        f"long_polys={len(long_polys)} hard_log_bases={len(hard_log_bases)} "
        f"nested={len(nested_mixes)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

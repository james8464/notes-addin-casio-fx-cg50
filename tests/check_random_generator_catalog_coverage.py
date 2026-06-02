#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import random
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FUZZER = ROOT / "tests" / "random_working_fuzzer.py"


def load_fuzzer():
    spec = importlib.util.spec_from_file_location("random_working_fuzzer", FUZZER)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod


def top_command(src: str) -> str:
    m = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(", src)
    return m.group(1) if m else "raw"


def split_top_args(src: str) -> list[str]:
    start = src.find("(")
    end = src.rfind(")")
    if start < 0 or end <= start:
        return []
    body = src[start + 1:end]
    args: list[str] = []
    depth = 0
    cur: list[str] = []
    for ch in body:
        if ch == "," and depth == 0:
            args.append("".join(cur).strip())
            cur = []
            continue
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth = max(0, depth - 1)
        cur.append(ch)
    if cur or body.strip():
        args.append("".join(cur).strip())
    return args


def main() -> int:
    fuzzer = load_fuzzer()
    rng = random.Random(771337)
    scheduler = fuzzer.ChaosCommandScheduler(rng, fuzzer.VISIBLE_COMMANDS)
    cases = [
        fuzzer.make_case(rng, 10, 0.0, 0.0, None, True, i, scheduler)[1]
        for i in range(len(fuzzer.VISIBLE_COMMANDS) * 5)
    ]

    by_cmd: dict[str, list[list[str]]] = defaultdict(list)
    for src in cases:
        cmd = top_command(src)
        if cmd != "raw":
            by_cmd[cmd].append(split_top_args(src))

    missing = sorted(set(fuzzer.VISIBLE_COMMANDS) - set(by_cmd))
    too_single = sorted(
        cmd for cmd, rows in by_cmd.items()
        if len({len(r) for r in rows}) < 2 and cmd in fuzzer.OPTIONAL_ARG_COMMANDS
    )
    no_lists = []
    if not any("[" in src and "]" in src for src in cases if top_command(src) == "rewrite"):
        no_lists.append("rewrite")
    if not any("[" in src and "]" in src for src in cases if top_command(src) == "subst"):
        no_lists.append("subst")
    no_eqs = sorted(
        cmd for cmd in {"solve", "xform", "implicit_diff"}
        if not any("=" in src for src in cases if top_command(src) == cmd)
    )
    no_hard_expr = sorted(
        cmd for cmd, rows in by_cmd.items()
        if not any(any(len(arg) > 200 and any(fn in arg for fn in ("log(", "sqrt(", "sin(", "exp(", "ln(")) for arg in args) for args in rows)
    )

    failures = []
    if missing:
        failures.append("missing=" + ",".join(missing))
    if too_single:
        failures.append("single_arity=" + ",".join(too_single))
    if no_lists:
        failures.append("missing_list_params=" + ",".join(no_lists))
    if no_eqs:
        failures.append("missing_equations=" + ",".join(no_eqs))
    if len(no_hard_expr) > len(fuzzer.VISIBLE_COMMANDS) // 3:
        failures.append(f"too_many_without_hard_expr={len(no_hard_expr)}")

    if failures:
        print("FAIL random generator catalog coverage:", "; ".join(failures))
        for sample in cases[:20]:
            print(sample[:260])
        return 1
    print(
        "OK random generator catalog coverage "
        f"commands={len(by_cmd)} cases={len(cases)} "
        f"optional_multi={len(fuzzer.OPTIONAL_ARG_COMMANDS)-len(too_single)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"


REQUIRED_MARKERS = [
    "cascas_show_working_screen",
    "cascas_working_text",
    "doTextArea(&text)",
    "text.allowEXE=true",
    "check_do_graph(ge,do_logo_graph_eqw & ~1)",
    "if (do_logo_graph_eqw % 2)",
    "cascas_output_working(s,eval_s,s_)",
    "Alias rewrite:",
    "Fallback search",
    "Verify:",
    "Answer: ",
]

METHOD_MARKERS = [
    "Parse derivative",
    "Parse integral",
    "Rearrange: move each equation",
    "Matrix setup",
    "Stats setup",
    "Trig setup",
    "Graph setup",
    "Local form",
]

ALIASES = [
    "complete_square(",
    "fitconst(",
    "match(",
    "bool_simplify(",
    "prove_bool(",
    "rewrite(",
    "suvat(",
    "transform(",
    "cartesian(",
    "nand(",
    "nor(",
]


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def main() -> int:
    main_cc = MAIN.read_text(errors="ignore")
    missing = [x for x in REQUIRED_MARKERS if x not in main_cc]
    if missing:
        return fail("working screen markers missing: " + ", ".join(missing))

    missing = [x for x in METHOD_MARKERS if x not in main_cc]
    if missing:
        return fail("method working markers missing: " + ", ".join(missing))

    missing = [x for x in ALIASES if x not in main_cc]
    if missing:
        return fail("alias rewrites missing: " + ", ".join(missing))

    print("OK khicas working policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

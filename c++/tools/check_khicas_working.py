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
    "Rw:",
    "Fallback:",
    "Met:",
    "Chk:",
    "Ans: ",
]

METHOD_MARKERS = [
    "Diff:",
    "Impl:",
    "Param:",
    "Area:",
    "Int:",
    "Trig solve:",
    "Const:",
    "Match:",
    "Comp sq:",
    "DE:",
    "King:",
    "Weier:",
    "Sophie:",
    "Sym:",
    "Ref tri:",
    "DI:",
    "PF:",
    "Hard int:",
    "Move to lhs-rhs=0",
    "Mat:",
    "Stats:",
    "Trig:",
    "Fn:",
    "SUVAT:",
    "Binom:",
    "Coeff:",
    "Graph:",
    "Local:",
]

ALIASES = [
    "complete_square(",
    "compose(",
    "inverse(",
    "normal_diff(",
    "implicit_diff(",
    "param_diff(",
    "param_second_diff(",
    "second_diff(",
    "param_area(",
    "tangent_line(",
    "de_solve(",
    "poly(",
    "solve_trig(",
    "trig_prove(",
    "trig_transform(",
    "trig_rewrite(",
    "xform(",
    "fitconst(",
    "match(",
    "coeff_match(",
    "binom_expand(",
    "binom_coeff(",
    "rewrite(",
    "suvat(",
    "transform(",
    "cartesian(",
    "integrate_by(",
    "diff_by(",
    "solve_by(",
    "solve_trig_by(",
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

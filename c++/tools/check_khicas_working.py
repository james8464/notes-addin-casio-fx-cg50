#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"
CONSOLE = ROOT / "c++/khicas/upstream/giac90_1addin/console.cc"
TPL = ROOT / "c++/prizm/help/CASIOCAS.TPL"


REQUIRED_MARKERS = [
    "cascas_show_working_screen",
    "cascas_working_text",
    "cascas_append_specific_lines",
    "cascas_clean_answer_text",
    "cascas_strip_one_sign_factor",
    "cascas_answer_cleanup_allowed",
    "cascas_binom_validity",
    "doTextArea(&text)",
    "text.allowEXE=true",
    "check_do_graph(ge,do_logo_graph_eqw & ~1)",
    "if (do_logo_graph_eqw % 2)",
    "cascas_output_working(s,eval_s,s_)",
    "2. = ",
    "Std form.",
    "IBP:",
    "Valid:",
    "Valid: all x",
    "Valid: |x|<1",
    "Use PF",
    "Move left:",
    "Base =",
    "CASCAS_WORK_MAX_CHARS",
    "cascas_working_sink",
    "cascas_old_python_scope_working_call",
    "cascas_append_guard_lines",
    "tgt=",
    "Clear den; eq",
    "du=u'dx; v=Int dv.",
    "cascas_try_domain_range_command",
    "cascas_domain_range_poly2",
    "cascas_rewrite_fitconst_call",
    "coeff(expand((",
    "Range:",
    "Domain:",
]

CONSOLE_MARKERS = [
    "cascas_extract_work_expr",
    "Line[l].type==LINE_TYPE_OUTPUT",
    "strchr(p,':')",
    "memmove(buf,p,strlen(p)+1)",
]

METHOD_MARKERS = [
    "Pick rule.",
    "Collect y' terms",
    "dy/dx=(dy/d",
    "Area=Int",
    "I = Int[",
    "1. ",
    "Eq coeffs",
    "Solve consts",
    "ax2+bx+c ->",
    "Int sides.",
    "Classify.",
    "PF",
    "Move left",
    "Prob/crit",
    "trig ids",
    "Use ",
    "Ids:",
    "Expand; collect; factor.",
    "Pick SUVAT",
    "Binom theorem",
    "Coeff:",
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
    "trigcos(",
    "trigsin(",
    "trigtan(",
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
    console_cc = CONSOLE.read_text(errors="ignore")
    template_text = TPL.read_text(errors="ignore")
    working_surface = main_cc + "\n" + template_text
    missing = [x for x in REQUIRED_MARKERS if x not in working_surface]
    if missing:
        return fail("working screen markers missing: " + ", ".join(missing))
    if 'out += "Ans: "' in main_cc:
        return fail("final answer still has Ans prefix")
    if '{"range(","tabvar("}' in main_cc:
        return fail("range() still aliases to tabvar()")
    try:
        policy = main_cc.split("static bool cascas_old_python_scope_working_call", 1)[1]
        policy = policy.split("static bool cascas_show_working_for", 1)[0]
    except IndexError:
        return fail("working allowlist policy missing")
    forbidden_policy = ['"texpand("', '"tcollect("', '"tlin("']
    leaked = [x.strip('"') for x in forbidden_policy if x in policy]
    if leaked:
        return fail("KhiCAS-native trig transforms in working allowlist: " + ", ".join(leaked))

    missing = [x for x in CONSOLE_MARKERS if x not in console_cc]
    if missing:
        return fail("console working pretty-view markers missing: " + ", ".join(missing))

    missing = [x for x in METHOD_MARKERS if x not in working_surface]
    if missing:
        return fail("method working markers missing: " + ", ".join(missing))

    missing = [x for x in ALIASES if x not in main_cc]
    if missing:
        return fail("alias rewrites missing: " + ", ".join(missing))

    print("OK khicas working policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"
HELP = ROOT / "c++/prizm/help/CASIOCAS.HLP"
TPL_DIR = ROOT / "c++/prizm/help"
STATIC_LEXER = ROOT / "c++/khicas/upstream/giac90_1addin/static_lexer.h"
STATIC_LEXER_FULL = ROOT / "c++/khicas/upstream/giac90_1addin/static_lexer_full.h"
DCONSOLE = ROOT / "c++/khicas/upstream/giac90_1addin/dConsole.cpp"
DMAIN = ROOT / "c++/khicas/upstream/giac90_1addin/dmain.cpp"


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
    "binom_expand(expr)",
    "binom_coeff(expr,x,k)",
    "compose(f,g,[x])",
    "compare(expr1,expr2)",
    "cartesian([x(t),y(t)],t)",
    "de_solve(equation,[bc])",
    "domain(expr,[x,lo,hi])",
    "expand(expr)",
    "fitconst(equations,vars)",
    "implicit_diff(eq,[x,y])",
    "inverse(f(x))",
    "match(expr,form)",
    "coeff_match(expr,form,vars,[x])",
    "normal_diff(expr,[x])",
    "param_area([x(t),y(t)],t,[a,b])",
    "param_diff([x(t),y(t)],t)",
    "poly(expr,[x])",
    "range(expr,[x,lo,hi])",
    "rewrite(expr,target)",
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

REMOVED_SURFACES = [
    "conj(z)",
    "det(A)",
    "eigenvals(A)",
    "eigenvects(A)",
    "idn(n)",
    "im(z)",
    "mult_c_conjugate",
    "re(z)",
    "rref(A)",
    "tran(A)",
    "param_second_diff([x(t),y(t)],t)",
    "second_diff(expr,[x])",
]

REMOVED_LEXER_NAMES = [
    "conj",
    "det",
    "det_minor",
    "egv",
    "egvl",
    "eigenvals",
    "eigenvalues",
    "eigenvectors",
    "eigenvects",
    "idn",
    "im",
    "imag",
    "mult_c_conjugate",
    "re",
    "real",
    "rref",
    "tran",
]

REQUIRED_PROGCMD_SURFACES = [
    '{"<", "<"',
    '{"<=", "<="',
    '{">", ">"',
    '{">=", ">="',
    '{"a and b", " and "',
    '{"a or b", " or "',
    '{"not(x)"',
]


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def without_if0(text: str) -> str:
    out = []
    skip = 0
    for line in text.splitlines():
        stripped = line.strip()
        if stripped == "#if 0":
            skip += 1
            continue
        if stripped == "#endif" and skip:
            skip -= 1
            continue
        if not skip:
            out.append(line)
    return "\n".join(out)


def main() -> int:
    catalog = CATALOG.read_text(errors="ignore")
    active_catalog = without_if0(catalog)
    main_cc = MAIN.read_text(errors="ignore")
    help_text = HELP.read_text(errors="ignore") if HELP.exists() else ""
    template_text = "\n".join(p.read_text(errors="ignore") for p in sorted(TPL_DIR.glob("CASIOCAS*.TPL")))
    lexer_text = STATIC_LEXER.read_text(errors="ignore")
    lexer_full_text = STATIC_LEXER_FULL.read_text(errors="ignore")
    dconsole = DCONSOLE.read_text(errors="ignore")
    dmain = DMAIN.read_text(errors="ignore")

    if "catalog_hidden_category" not in catalog or "catalog_hidden_name" not in catalog:
        return fail("catalog hide filters missing")
    if "Args:" not in help_text:
        return fail("external catalog parameter help missing")
    if "method=auto" not in catalog and "method=auto" not in help_text:
        return fail("method help missing")
    for unclear in ["Req/Opt", "PReq", "KhiCAS cmd.\\nReq", "Req:f,x"]:
        if unclear in catalog:
            return fail("unclear help wording still present: " + unclear)
    if "selected_category=catids" not in catalog:
        return fail("category id mapping missing")
    if 'ADD_CAT(CAT_CATEGORY_ALL,"All")' not in catalog:
        return fail("All catalogue category missing")
    if 'ADD_CAT(CAT_CATEGORY_PROGCMD,"Program cmds")' not in catalog:
        return fail("Program cmds category missing")
    for marker in ["dconsole_catalog_prefix", 'doCatalogMenu(buf,(char *)"Index",0,prefix)', "len>4"]:
        if marker not in dconsole:
            return fail("shell catalogue prefix search missing: " + marker)
    for marker in ['!strcmp(buf,"log")', '!strcmp(buf,"sec")', '!strcmp(buf,"csc")', '!strcmp(buf,"cosec")', '!strcmp(buf,"cot")']:
        if marker not in dmain:
            return fail("syntax colour alias missing: " + marker)
    if "solve_trig(eq,[var,lo,hi,max,method])" in active_catalog:
        return fail("solve_trig still active in catalogue; use solve() surface")
    if "catalog_hidden_name(completeCat[cur].name)" not in catalog:
        return fail("category command hide filter not applied")
    if "catalog_hidden_name(text)" not in catalog:
        return fail("all/options hide filter not applied")
    if "hidden_exact" not in catalog:
        return fail("exact hidden-name filter missing")
    hidden_prefix_block = catalog.split("hidden_prefix", 1)[1].split("hidden_exact", 1)[0]
    for protected_prefix in ['"line"', '"nor"']:
        if protected_prefix in hidden_prefix_block:
            return fail(f"over-broad hidden prefix: {protected_prefix}")

    missing_hidden = [x for x in HIDDEN_MARKERS if x not in catalog]
    if missing_hidden:
        return fail("hidden markers missing: " + ", ".join(missing_hidden))

    missing_aliases = [x for x in OLD_FEATURE_ALIASES if x not in catalog]
    if missing_aliases:
        return fail("old feature aliases missing: " + ", ".join(missing_aliases))

    missing_progcmd = [x for x in REQUIRED_PROGCMD_SURFACES if x not in catalog]
    if missing_progcmd:
        return fail("program command surfaces missing: " + ", ".join(missing_progcmd))

    still_catalogued = [x for x in REMOVED_SURFACES if x in catalog]
    if still_catalogued:
        return fail("removed surfaces still in catalogue: " + ", ".join(still_catalogued))
    still_helped = ["@" + x for x in REMOVED_SURFACES if "@" + x in help_text]
    if still_helped:
        return fail("removed surfaces still in help: " + ", ".join(still_helped))
    still_lexed = [x for x in REMOVED_LEXER_NAMES if '{"' + x + '"' in lexer_text]
    if still_lexed:
        return fail("removed surfaces still in active lexer: " + ", ".join(still_lexed))
    still_full_lexed = [x for x in REMOVED_LEXER_NAMES if '{"' + x + '"' in lexer_full_text]
    if still_full_lexed:
        return fail("removed surfaces still in full lexer: " + ", ".join(still_full_lexed))

    if "Copy CASIOCAS.PAK to storage root." not in catalog:
        return fail("compact F6 help fallback missing")
    if "CASCAS_HELP_FILE" not in catalog or "catalog_read_help_record" not in catalog:
        return fail("external F6 help loader missing")
    if "@integrate(f,x,[a,b,method,u])" not in help_text or "F2:" not in help_text:
        return fail("external help pack missing integrate examples")
    if "Bfile_OpenFile_OS" not in catalog or "CCP1" not in catalog:
        return fail("external help scanner not wired")
    for marker in [
        "catalog_make_calculus_insert",
        "Diff method",
        "Int method",
        "u expr blank=auto",
        "integrate(method=",
        "diff(method=",
    ]:
        if marker not in catalog:
            return fail("calculus method picker missing: " + marker)

    if "cascas_output_working" not in main_cc or "cascas_working_text" not in main_cc:
        return fail("working-line output hook missing")
    if "cascas_extract_method" not in main_cc or "cascas_strip_method_args" not in main_cc:
        return fail("method extraction hook missing")
    if "u = " not in template_text or "v = Int dv" not in template_text:
        return fail("parts/sub method u hint missing")
    if "LHS - RHS = 0" not in template_text or "cascas_append_final_answer(out,shown_answer)" not in main_cc:
        return fail("working-line output shape missing")
    if 'out += "Ans: "' in main_cc:
        return fail("final answer still has Ans prefix")

    print("OK khicas catalog policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

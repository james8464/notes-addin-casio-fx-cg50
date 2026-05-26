#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
CATALOG_FR = ROOT / "c++/khicas/upstream/giac90_1addin/catalogfr.cpp"
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"
SCOPE_GUARD = ROOT / "c++/addin/src/core/scope_guard.hpp"
HELP = ROOT / "c++/prizm/help/CASIOCAS.HLP"
TPL_DIR = ROOT / "c++/prizm/help"
STATIC_LEXER = ROOT / "c++/khicas/upstream/giac90_1addin/static_lexer.h"
STATIC_LEXER_PTR = ROOT / "c++/khicas/upstream/giac90_1addin/static_lexer_.h"
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
    "comb(n,k)",
    "normald([mu,sigma],x)",
    "mean(l)",
    "median(l)",
    "stddev(l)",
    "correlation(l1,l2)",
    "covariance(l1,l2)",
    "linear_regression(Xlist,Ylist)",
    "linear_regression_plot(Xlist,Ylist)",
    "plot(expr,x)",
    "plotarea(expr,x=a..b,[n,meth])",
    "plotcontour(expr,[x=xm..xM,y=ym..yM],levels)",
    "plotfield(f(t,y),[t=tmin..tmax,y=ymin..ymax])",
    "plotlist(list)",
    "plotode(f(t,y),[t=tmin..tmax,y],[t0,y0])",
    "plotparam([x,y],t)",
    "plotpolar(r,theta)",
    "plotseq(f(x),x=[u0,m,M],n)",
    "param_area([x(t),y(t)],t,[a,b])",
    "param_area_y(",
    "param_volume_x(",
    "param_volume_y(",
    "area_between(",
    "volume_x(",
    "volume_y(",
    "mean_value(",
    "rationalise(",
    "tabular(",
    "weierstrass(",
    "symmetry(",
    "ztest(",
    "stdev(",
    "disque(",
    "sinh(",
    "cosh(",
    "tanh(",
    "asinh(",
    "acosh(",
    "atanh(",
    "spark(",
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
    "acosh",
    "ACOSH",
    "acoth",
    "asinh",
    "ASINH",
    "atanh",
    "ATANH",
    "comb",
    "correlation",
    "cosh",
    "coth",
    "csch",
    "covariance",
    "covariance_correlation",
    "disque",
    "disque_centre",
    "erf",
    "erfc",
    "erfs",
    "betad",
    "betad_cdf",
    "betad_icdf",
    "cauchyd",
    "cauchyd_cdf",
    "cauchyd_icdf",
    "chisquared",
    "chisquared_cdf",
    "chisquared_icdf",
    "expovariate",
    "exponentiald_cdf",
    "exponentiald_icdf",
    "linear_regression",
    "linear_regression_plot",
    "exponential_regression",
    "logarithmic_regression",
    "polynomial_regression",
    "power_regression",
    "fisher",
    "fisher_cdf",
    "fisher_icdf",
    "fisherd",
    "fisherd_cdf",
    "fisherd_icdf",
    "gammad",
    "gammad_cdf",
    "gammad_icdf",
    "geometric",
    "geometric_cdf",
    "geometric_icdf",
    "indets",
    "is_matrix",
    "mean",
    "median",
    "median_line",
    "matrix_norm",
    "frobenius_norm",
    "l1norm",
    "l2norm",
    "linfnorm",
    "maxnorm",
    "colnorm",
    "rownorm",
    "negbinomial",
    "negbinomial_cdf",
    "negbinomial_icdf",
    "newton",
    "normald",
    "NORMALD",
    "normald_cdf",
    "normald_icdf",
    "normalvariate",
    "odesolve",
    "poisson",
    "poisson_cdf",
    "poisson_icdf",
    "randNorm",
    "randbinomial",
    "randchisquare",
    "randchisquared",
    "randexp",
    "randfisher",
    "randfisherd",
    "randgeometric",
    "randmatrix",
    "randmultinomial",
    "randnormald",
    "randpoisson",
    "randpoly",
    "randrange",
    "randstudentd",
    "randvector",
    "reverse_rsolve",
    "rsolve",
    "snedecord",
    "snedecord_cdf",
    "snedecord_icdf",
    "studentd",
    "studentd_cdf",
    "studentd_icdf",
    "area_between",
    "plot",
    "areaplot",
    "bar_plot",
    "barplot",
    "cdfplot",
    "contourplot",
    "courbe_parametrique",
    "courbe_polaire",
    "densityplot",
    "exponential_regression_plot",
    "fieldplot",
    "funcplot",
    "histogram",
    "implicitplot",
    "implicitplot3d",
    "inequationplot",
    "interactive_odeplot",
    "interactive_plotode",
    "listplot",
    "logarithmic_regression_plot",
    "logistic_regression_plot",
    "odeplot",
    "paramplot",
    "plotarea",
    "plot3d",
    "plot_style",
    "plotcdf",
    "plotcontour",
    "plotdensity",
    "plotfield",
    "plotfunc",
    "plotimplicit",
    "plotinequation",
    "plotlist",
    "plotmatrix",
    "plotode",
    "plotparam",
    "plotpolar",
    "plotseq",
    "polarplot",
    "polygonplot",
    "polygonscatterplot",
    "polynomial_regression_plot",
    "power_regression_plot",
    "scatterplot",
    "seqplot",
    "sech",
    "sinh",
    "spark",
    "stdDev",
    "stddev",
    "stddevp",
    "symmetry",
    "tabular",
    "tabvar",
    "tanh",
    "rationalise",
    "weierstrass",
    "mean_value",
    "param_area",
    "param_area_y",
    "param_volume_x",
    "param_volume_y",
    "volume_x",
    "volume_y",
    "ztest",
    "conj",
    "arg",
    "cfactor",
    "complex",
    "cpartfrac",
    "csolve",
    "det",
    "det_minor",
    "egv",
    "egvl",
    "eigenvals",
    "eigenvalues",
    "eigenvectors",
    "eigenvects",
    "evalc",
    "exponentiald",
    "fourier_an",
    "fourier_bn",
    "fourier_cn",
    "gramschmidt",
    "idn",
    "ilaplace",
    "im",
    "imag",
    "inv",
    "invlaplace",
    "jordan",
    "laplace",
    "linsolve",
    "lu",
    "matpow",
    "matrix",
    "mult_c_conjugate",
    "polar2rectangular",
    "polar_complex",
    "qr",
    "quartile1",
    "quartile3",
    "quartiles",
    "rank",
    "ranm",
    "ranv",
    "re",
    "real",
    "rectangular2polar",
    "rref",
    "svd",
    "taylor",
    "trace",
    "tran",
    "transpose",
    "uniform",
    "uniform_cdf",
    "uniform_icdf",
    "uniformd",
    "uniformd_cdf",
    "uniformd_icdf",
    "variance",
    "weibull",
    "weibull_cdf",
    "weibull_icdf",
    "weibulld",
    "weibulld_cdf",
    "weibulld_icdf",
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

FM_ONLY_INACTIVE_CATALOG_SURFACES = [
    "erf(x)",
    "erfc(x)",
    "euler(n)",
    "exponentiald(lambda,x)",
    "halftan(expr)",
    "hermite(n)",
    "iabcuv(a,b,c)",
    "ichinrem([a,m],[b,n])",
    "idivis(n)",
    "iegcd(a,b)",
    "interp(X,Y)",
    "laguerre(n,a,x)",
    "legendre(n)",
    "odesolve(f(t,y),[t,y],[t0,y0],t1)",
    "powmod(a,n,p)",
    "proot(p)",
    "resultant(p,q,x)",
    "revert(p[,x])",
    "rsolve(equation,u(n),[init])",
    "tabvar(f,[x=a..b])",
    "taylor(f,x=a,n,[polynom])",
    "tchebyshev1(n)",
    "tchebyshev2(n)",
    "trig2exp(expr)",
    "uniformd(a,b,x)",
]

FM_ONLY_HIDDEN_PREFIXES = ["fourier_", "laplace", "ilaplace"]

LEXER_POINTER_ALIASES = {
    "%": "PERCENT",
    "%CHANGE": "PERCENTCHANGE",
    "%TOTAL": "PERCENTTOTAL",
    ".": "struct_dot",
    "assert": "giac_assert",
    "affichage": "display",
    "bin": "binprint",
    "coordonnees": "coordinates",
    "draw_pixel": "set_pixel",
    "efface": "efface_logo",
    "expovariate": "randexp",
    "get_key": "getKey",
    "hex": "hexprint",
    "linetan": "LineTan",
    "normalvariate": "randNorm",
    "oct": "octprint",
    "regroup": "regrouper",
    "snedecord_cdf": "fisher_cdf",
}


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def command_name(surface: str) -> str:
    name = surface.split("(", 1)[0].strip()
    if not name:
        return ""
    return name.split()[0].lower()


def removed_catalog_names() -> set[str]:
    return {name for name in (command_name(x) for x in REMOVED_SURFACES) if name}


def catalog_command_names(text: str) -> set[str]:
    names: set[str] = set()
    for match in re.finditer(r'\{\s*"([^"]+)"', text):
        name = command_name(match.group(1))
        if name:
            names.add(name)
    return names


def lexer_names(text: str) -> list[str]:
    return [m.group(1) for m in re.finditer(r'\{\s*"([^"]+)"', without_if0(text))]


def lexer_pointer_names(text: str) -> list[str]:
    names: list[str] = []
    for line in without_if0(text).splitlines():
        name = line.strip().rstrip(",")
        if name.startswith("at_"):
            names.append(name[3:])
    return names


def expected_lexer_pointer(name: str) -> str:
    return LEXER_POINTER_ALIASES.get(name, name)


def removed_fnv1a(name: str) -> int:
    h = 2166136261
    for b in name.lower().encode("ascii"):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def removed_identifier_hash_names() -> dict[int, str]:
    out: dict[int, str] = {}
    for name in REMOVED_LEXER_NAMES:
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            out.setdefault(removed_fnv1a(name), name)
    return out


def switch_case_hashes(text: str, marker: str) -> set[int]:
    start = text.index(marker)
    end = text.index("return true;", start)
    return {int(m.group(1), 16) for m in re.finditer(r"case 0x([0-9a-fA-F]+)u:", text[start:end])}


def without_if0(text: str) -> str:
    out = []
    skip = 0
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("//"):
            continue
        if skip:
            if stripped.startswith("#if"):
                skip += 1
            elif stripped == "#endif":
                skip -= 1
            continue
        if stripped == "#if 0":
            skip = 1
            continue
        out.append(line)
    return "\n".join(out)


def main() -> int:
    catalog = CATALOG.read_text(errors="ignore")
    active_catalog = without_if0(catalog)
    catalog_fr = CATALOG_FR.read_text(errors="ignore")
    active_catalog_fr = without_if0(catalog_fr)
    main_cc = MAIN.read_text(errors="ignore")
    scope_guard = SCOPE_GUARD.read_text(errors="ignore")
    help_text = HELP.read_text(errors="ignore") if HELP.exists() else ""
    template_text = "\n".join(p.read_text(errors="ignore") for p in sorted(TPL_DIR.glob("CASIOCAS*.TPL")))
    lexer_text = STATIC_LEXER.read_text(errors="ignore")
    lexer_ptr_text = STATIC_LEXER_PTR.read_text(errors="ignore")
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
    for marker in ['!strcmp(buf,"log")', '!strcmp(buf,"log10")', '!strcmp(buf,"ln")', '!strcmp(buf,"sec")', '!strcmp(buf,"csc")', '!strcmp(buf,"cosec")', '!strcmp(buf,"cot")']:
        if marker not in dmain or marker not in main_cc:
            return fail("syntax colour alias missing from calculator/main shell: " + marker)
    if '{"normal_cdf",0,0,9,13}' not in lexer_text:
        return fail("source-built normal_cdf lexer entry missing")
    lexer_name_list = lexer_names(lexer_text)
    lexer_ptr_list = lexer_pointer_names(lexer_ptr_text)
    if len(lexer_name_list) != len(lexer_ptr_list):
        return fail(f"source-built lexer pointer count mismatch: {len(lexer_name_list)} names vs {len(lexer_ptr_list)} ptrs")
    for i, (name, ptr) in enumerate(zip(lexer_name_list, lexer_ptr_list)):
        expected = expected_lexer_pointer(name)
        if expected.lower() != ptr.lower():
            return fail(f"source-built lexer pointer mismatch at {i}: {name} -> at_{ptr}, expected at_{expected}")
    for marker in ['cascas_rewrite_normalcdf_call', '"normalcdf("', '"evalf(normal_cdf("']:
        if marker not in main_cc:
            return fail("source-built normalcdf wrapper missing: " + marker)
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
    active_fm_only = [x for x in FM_ONLY_INACTIVE_CATALOG_SURFACES if x in active_catalog]
    if active_fm_only:
        return fail("FM-only catalog surface active: " + ", ".join(active_fm_only))
    hidden_impl = catalog.split("static bool catalog_hidden_name", 1)[1].split("static const char *CASCAS_HELP_FILE", 1)[0]
    missing_fm_prefix = [x for x in FM_ONLY_HIDDEN_PREFIXES if x not in hidden_impl]
    if missing_fm_prefix:
        return fail("FM-only hidden prefix missing: " + ", ".join(missing_fm_prefix))
    if "catalog_visible_category" not in catalog or "isall && !catalog_visible_category(completeCat[cur].category)" not in catalog:
        return fail("hidden-category commands still leak into All")

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
    removed_names = removed_catalog_names()
    active_catalogued = sorted(removed_names & catalog_command_names(active_catalog))
    if active_catalogued:
        return fail("removed names still active in catalogue: " + ", ".join(active_catalogued))
    active_catalogued_fr = sorted(removed_names & catalog_command_names(active_catalog_fr))
    if active_catalogued_fr:
        return fail("removed names still active in FR catalogue: " + ", ".join(active_catalogued_fr))
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
    if "cascas_contains_removed_function" not in main_cc or "Err: unsupported function." not in main_cc:
        return fail("removed/Further-only runtime guard missing")
    removed_hash_names = removed_identifier_hash_names()
    required_removed_hashes = set(removed_hash_names)
    scope_hashes = switch_case_hashes(scope_guard, "is_removed_function_hash")
    main_hashes = switch_case_hashes(main_cc, "cascas_removed_function_hash")
    missing_scope_hashes = sorted(required_removed_hashes - scope_hashes)
    if missing_scope_hashes:
        return fail("removed addin hashes missing: " + ", ".join(removed_hash_names[h] for h in missing_scope_hashes[:12]))
    missing_main_hashes = sorted(required_removed_hashes - main_hashes)
    if missing_main_hashes:
        return fail("removed source-built hashes missing: " + ", ".join(removed_hash_names[h] for h in missing_main_hashes[:12]))
    if scope_hashes != main_hashes:
        return fail("removed hash guard mismatch between addin and source-built main")
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

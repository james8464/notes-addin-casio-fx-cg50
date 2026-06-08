#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

SOURCE_MARKERS = {
    "khicas/upstream/giac90_1addin/cascas_working.h": [
        "typedef bool (*khicas_eval_callback)",
        "void set_khicas_eval_callback",
    ],
    "khicas/upstream/giac90_1addin/main.cc": [
        "static bool cascas_khicas_eval",
        "cascas::set_khicas_eval_callback(cascas_khicas_eval)",
    ],
    "khicas/upstream/giac90_1addin/cascas_working.cc": [
        "static khicas_eval_callback g_khicas_eval=0",
        "g_khicas_eval=cb",
        "try_khicas_exact_route",
        "production_exact_command",
        "khicas_zero",
        "khicas_equiv",
        "try_xform_rewrite_planner",
        "struct RewriteRule",
        "static const RewriteRule rules[]",
        "production_exact_command(working_string(rules[i].cmd)+\"(\"+cur+\")\"",
        "try_xform_trig_direct(compact(cur),compact(target),trig)",
        "try_xform_trig_direct",
        "try_xform_quad_template",
        "parse_quad_template",
        "exact_simplify_scalar",
        "try_xform_isolate_parameter",
        "try_xform_linear_parameter_ratio",
        "xform_numeric_parameter_ok",
        "Verified by substitution",
        "Verified by equivalence check",
        "Verified",
        "KhiCAS exact evaluation",
    ],
}

FORBIDDEN_SOURCE_SNIPPETS = {
    "khicas/upstream/giac90_1addin/Makefile": [
        "CASCAS_PRODUCTION_CALLBACK_ONLY",
    ],
    "tools/khicas_host_runner": [
        "CASCAS_PRODUCTION_CALLBACK_ONLY",
    ],
    "khicas/upstream/giac90_1addin/cascas_working.cc": [
        "CASCAS_PRODUCTION_CALLBACK_ONLY",
        "diff(x*e^x,x)",
        "diff(7xe^x/sqrt",
        "tan(x)^2sec(x)^2",
        'e=="sec(x)^4"',
        "3x^2(4-2x^3)",
        "(cos(x)+sin(x))(cosec(x)-sec(x))=kcot(2x)",
        "try_xform_recip_product_isolate",
        "try_xform_exact_solve",
        "Rule family: exact solve + substitution",
        "parse_recip_product_to_cot_coeff",
        "parse_sin_cos_linear_factor",
        "Use AC+BD=0 and AD=-BC",
        "try_solve_paper_system_special",
        "Sub x=20 into H=A*x*(40-x)",
        'ceq=="tan(x)=1/2"',
        'ceq=="10^(3k)=2"',
        'e=="6/(u(3+2u))"',
        'e=="(3x+5)/(x^2+x-2)"',
    ],
}

VERIFIED_HOST_POLICY_CASES = [
    "xform(sin(x)^2+cos(x)^2,1)",
    "xform(cosec(x)^2-1,cot(x)^2)",
    "xform(1+tan(x)^2,sec(x)^2)",
    "diff(x*e^x,x)",
    "integrate(2*x+3,x)",
    "solve(10^(3*k)=2,k)",
]

TRIG_XFORM_POLICY_CASES = [
    (
        "xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)",
        [
            "Planner search:",
            "Parameter isolation:",
            "k = 2",
            "Verified by substitution",
        ],
    ),
    (
        "xform(cot(t)-tan(t),2*cot(2*t))",
        [
            "Reciprocal identities:",
            "Common denominator:",
            "Double-angle identities:",
            "2cot(2t)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(3*cot(y)-3*tan(y),6*cot(2*y))",
        [
            "Reciprocal identities:",
            "Common denominator:",
            "Double-angle identities:",
            "6cot(2y)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(cos(t)^2-sin(t)^2,cos(2*t))",
        [
            "Double-angle identities:",
            "cos(2t)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(4*cos(y)^2-4*sin(y)^2,4*cos(2*y))",
        [
            "Double-angle identities:",
            "4cos(2y)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(2*sin(t)*cos(t),sin(2*t))",
        [
            "Double-angle identities:",
            "sin(2t)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(6*sin(y)*cos(y),3*sin(2*y))",
        [
            "Double-angle identities:",
            "3sin(2y)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(1+tan(y)^2,sec(y)^2)",
        [
            "Pythagorean identities:",
            "sec(y)^2",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(5+5*cot(t)^2,5*cosec(t)^2)",
        [
            "Pythagorean identities:",
            "5cosec(t)^2",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(3*sec(y)^2-3,3*tan(y)^2)",
        [
            "Pythagorean identities:",
            "3tan(y)^2",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(4*sin(t)^2+4*cos(t)^2,4)",
        [
            "Pythagorean identities:",
            "4",
            "Verified by equivalence check",
        ],
    ),
]

LINEAR_PARAMETER_XFORM_CASES = [
    (
        "xform(a*x+b=k*(x+1),k)",
        [
            "Planner search:",
            "Parameter isolation:",
            "k = (a*x + b)/(x + 1)",
            "Verified by substitution",
        ],
    ),
    (
        "xform(3*sin(t)=m*cos(t),m)",
        [
            "Planner search:",
            "Parameter isolation:",
            "m = 3*tan(t)",
            "Verified by substitution",
        ],
    ),
]

SOLVE_POLICY_CASES = [
    (
        "solve(y^2-y-2=0,y)",
        [
            "y^2 - y - 2 = 0",
            "y = -1 or y = 2",
            "y = [-1, 2]",
            "Verified",
        ],
    ),
    (
        "solve(u^2=4,u)",
        [
            "u^2 - 4 = 0",
            "u = -2 or u = 2",
            "u = [-2, 2]",
            "Verified",
        ],
    ),
    (
        "solve(k^2+k-2=0,k)",
        [
            "k^2 + k - 2 = 0",
            "k = -2 or k = 1",
            "k = [-2, 1]",
            "Verified",
        ],
    ),
    (
        "solve(y*(y-4)=0,y)",
        [
            "Zero product:",
            "y = 0 or y = 4",
            "y = [0, 4]",
            "Verified",
        ],
    ),
    (
        "solve(16*x^4+40*x^2-11=0,x)",
        [
            "u = x^2",
            "16*u^2 + 40*u - 11 = 0",
            "u = -11/4 or u = 1/4",
            "-11/4 < 0, reject for real x",
            "x = [-1/2, 1/2]",
            "Verified",
        ],
    ),
]

SOLVE_FALLBACK_CASES = [
    (
        "solve([n(0)=500,n(2)=1000,dn/dt=k*n],[a,k])",
        [
            "Planner search:",
            "last verified state:",
            "KhiCAS exact evaluation:",
        ],
    ),
    (
        "solve(s^2*(2*s^2+3*s+1)=0,s)",
        [
            "KhiCAS exact evaluation:",
            "[-1, -1/2, 0]",
            "Verified by exact CAS evaluation",
        ],
    ),
    (
        "solve(21504/k^2=21,k)",
        [
            "KhiCAS exact evaluation:",
            "[-32, 32]",
            "Verified by exact CAS evaluation",
        ],
    ),
    (
        "solve(log(3,2*x-1)-log(3,x+1)=1,x)",
        [
            "domain: log args>0;base!=1",
            "KhiCAS exact evaluation:",
            "[-4]",
            "Reject x=-4: log domain fails",
            "x = []",
            "Verified by domain check",
        ],
    ),
    (
        "solve(log(2,x+3)+log(2,x-1)=3,x)",
        [
            "domain: log args>0;base!=1",
            "KhiCAS exact evaluation:",
            "[-1 + 2*sqrt(3)]",
            "Verified by exact CAS evaluation",
        ],
    ),
]

INTEGRAL_POLICY_CASES = [
    (
        "integrate(cot(2*x),x)",
        [
            "cot=cos/sin",
            "1/2*ln(abs(sin(2*x))) + C",
            "Verified",
        ],
    ),
]

PARTFRAC_POLICY_CASES = [
    (
        "partfrac((2*x+3)/(x^2-1),x)",
        [
            "A/(x + 1)+B/(x - 1)",
            "-1/(2*(x + 1)) + 5/(2*(x - 1))",
        ],
    ),
    (
        "partfrac((5*x+13)/((2*x+1)*(x+4)),x)",
        [
            "A/(2*x + 1)+B/(x + 4)",
            "3/(2*x + 1) + 1/(x + 4)",
        ],
    ),
    (
        "partfrac(30/((x+3)*(9-2*x)),x)",
        [
            "A/(x + 3)+B/(9 - 2*x)",
            "2/(x + 3) + 4/(9 - 2*x)",
        ],
    ),
    (
        "apart(6/(u*(3+2*u)))",
        [
            "A/(u)+B/(2*u + 3)",
            "A = 6/3 = 2",
            "B = 6/-3/2 = -4",
            "2/(u) - 4/(2*u + 3)",
        ],
    ),
]

DOMAIN_POLICY_CASES = [
    ("domain(sqrt(x-2),x)", ["Domain", "x - 2 >= 0", "x >= 2"]),
    ("domain(log(10,4-x),x)", ["Domain", "4 - x > 0", "x < 4"]),
    ("domain(log(2,x^2-1),x)", ["Domain", "x^2 - 1 > 0", "x < -1 or x > 1"]),
    ("domain(1/(x^2-4),x)", ["Domain", "x^2 - 4 != 0", "x != -2 and x != 2"]),
    ("domain(sqrt(4-x)+ln(x-1),x)", ["Domain", "4 - x >= 0", "x - 1 > 0", "1 < x <= 4"]),
]

RANGE_POLICY_CASES = [
    (
        "range(2*x+1,x,-1,3)",
        [
            "linear interval",
            "f(-1) = -1",
            "f(3) = 7",
            "-1 <= y <= 7",
        ],
    ),
    (
        "range(exp(x),x,0,1)",
        [
            "f'(x)=",
            "f(0) = 1",
            "f(1) = e",
            "1 <= y <= e",
            "Verified",
        ],
    ),
    (
        "range(sin(x),x,0,pi/2)",
        [
            "f'(x)=",
            "f(0) = 0",
            "f(pi/2) = 1",
            "0 <= y <= 1",
            "Verified",
        ],
    ),
    (
        "range(sin(x)+cos(x),x)",
        [
            "R-form range",
            "R=sqrt(1^2+1^2)=sqrt(2)",
            "-sqrt(2) <= y <= sqrt(2)",
            "Verified",
        ],
    ),
    (
        "range(3*sin(2*x+1)+4*cos(2*x+1)-5,x)",
        [
            "R-form range",
            "R=sqrt(3^2+4^2)=5",
            "-10 <= y <= 0",
            "Verified",
        ],
    ),
    (
        "range(x^3,x,-2,1)",
        [
            "f'(x)=",
            "f(-2) = -8",
            "f(1) = 1",
            "-8 <= y <= 1",
            "Verified",
        ],
    ),
    (
        "range(4*(x^2-2)*exp(-2*x),x)",
        [
            "f'(x)=",
            "f(-1) = -4*exp(2)",
            "f(2) = 8*exp(-4)",
            "as x -> -infinity, y -> +infinity",
            "as x -> +infinity, y -> 0",
            "y >= -4*exp(2)",
            "Verified",
        ],
    ),
    (
        "range(x*exp(-x),x)",
        [
            "f'(x)=",
            "f(1) = 1/e",
            "as x -> -infinity, y -> -infinity",
            "as x -> +infinity, y -> 0",
            "y <= 1/e",
            "Verified",
        ],
    ),
    (
        "range(sqrt(x^2+1),x)",
        [
            "convex quad inside sqrt",
            "min inside=1 at x=0",
            "y >= 1",
            "Verified",
        ],
    ),
    (
        "range(sqrt((x+1)^2+4),x)",
        [
            "convex quad inside sqrt",
            "min inside=4 at x=-1",
            "y >= 2",
            "Verified",
        ],
    ),
    (
        "range(sqrt(x^2-1),x)",
        [
            "convex quad inside sqrt",
            "min inside=-1 at x=0",
            "inside crosses 0",
            "y >= 0",
            "Verified",
        ],
    ),
    (
        "range(2+3*exp(-x),x)",
        [
            "shifted exponential",
            "exp(u) > 0",
            "y > 2",
            "Verified",
        ],
    ),
    (
        "range(2-3*exp(-x),x)",
        [
            "shifted exponential",
            "exp(u) > 0",
            "y < 2",
            "Verified",
        ],
    ),
    (
        "range(1+2*sin(x)^2,x)",
        [
            "sin^2(u) in [0,1]",
            "1 <= y <= 3",
            "Verified",
        ],
    ),
    (
        "range(tan(x)^2,x)",
        [
            "tan^2(u) >= 0",
            "y >= 0",
            "Verified",
        ],
    ),
    (
        "range(1/(sin(x)+2),x)",
        [
            "reciprocal shifted trig",
            "sin(u) in [-1,1]",
            "1 <= D <= 3",
            "1/3 <= y <= 1",
            "Verified",
        ],
    ),
    (
        "range(2/(2-cos(x)),x)",
        [
            "reciprocal shifted trig",
            "cos(u) in [-1,1]",
            "1 <= D <= 3",
            "2/3 <= y <= 2",
            "Verified",
        ],
    ),
    (
        "range(sin(x)/(2+cos(x)),x)",
        [
            "trig rational route",
            "R-form condition",
            "3*y^2 - 1 <= 0",
            "-sqrt(3)/3 <= y",
            "y <= sqrt(3)/3",
            "Verified",
        ],
    ),
    (
        "range((x^2+2*x+3)/(x^2+1),x)",
        [
            "D>=0",
            "-4*y^2 + 16*y - 8 >= 0",
            "-sqrt(2) + 2 <= y",
            "y <= sqrt(2) + 2",
            "Verified",
        ],
    ),
    (
        "range(sin(x)+ln(x),x)",
        [
            "endpoint/critical route",
            "Domain:",
            "x > 0",
            "as x -> 0, y -> -infinity",
            "as x -> +infinity, y -> +infinity",
            "range: all real",
            "Verified",
        ],
    ),
    (
        "range(ln(x)-x,x)",
        [
            "endpoint/critical route",
            "f'(x)=",
            "f(1) = -1",
            "max y = -1",
            "y <= -1",
            "Verified",
        ],
    ),
    (
        "range(ln(x)/x,x)",
        [
            "endpoint/critical route",
            "Domain:",
            "x > 0",
            "f'(x)=",
            "f(e) = 1/e",
            "as x -> 0, y -> -infinity",
            "as x -> +infinity, y -> 0",
            "y <= 1/e",
            "Verified",
        ],
    ),
    (
        "range(ln(x^2+1),x)",
        [
            "log of quadratic",
            "min g=1 at x=0",
            "g >= 1 > 0",
            "y >= 0",
            "Verified",
        ],
    ),
]

IMPLICIT_POLICY_CASES = [
    (
        "implicit_diff(x^2+sin(y)=y,x,y)",
        [
            "Implicit differentiation:",
            "(dy)/(dx)=-(2*x)/(cos(y) - 1)",
            "Verified",
        ],
    ),
]

EQUIVALENCE_POLICY_CASES = [
    (
        "xform(sin(x),cos(x)+7)",
        [
            "Planner search:",
            "Attempted transformations:",
            "Failure reason:",
            "KhiCAS exact evaluation:",
            "Verification status:",
            "not equivalent",
        ],
    ),
    (
        "xform(sin(2*x+1),2*sin(x+1)*cos(x+1))",
        [
            "Planner search:",
            "Attempted transformations:",
            "Failure reason:",
            "KhiCAS exact evaluation:",
            "Verification status:",
            "not equivalent",
        ],
    ),
    (
        "xform(sin(x)+cos(x),banana)",
        [
            "Planner search:",
            "Attempted transformations:",
            "Failure reason:",
            "KhiCAS exact evaluation:",
            "Verification status:",
            "not equivalent",
            "last verified state:",
        ],
    ),
    (
        "xform(sin(x)+cos(x),cos(x))",
        [
            "Planner search:",
            "Attempted transformations:",
            "Failure reason:",
            "KhiCAS exact evaluation:",
            "Verification status:",
            "not equivalent",
            "last verified state:",
        ],
    ),
]

XFORM_PLANNER_CASES = [
    (
        "xform((sin(x)^2+cos(x)^2)*tan(x),sin(x)/cos(x))",
        [
            "Planner search:",
            "Rule: Reciprocal identities:",
            "sin(x)/cos(x)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(cot(x),cos(x)/sin(x))",
        [
            "Planner search:",
            "Rule: Reciprocal identities:",
            "cos(x)/sin(x)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(tan(x),sin(x)/cos(x))",
        [
            "Planner search:",
            "Rule: Reciprocal identities:",
            "sin(x)/cos(x)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(sin(x)/cos(x),tan(x))",
        [
            "Planner search:",
            "Rule: Reciprocal identities:",
            "tan(x)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(sec(t),1/cos(t))",
        [
            "Planner search:",
            "Rule: Reciprocal identities:",
            "1/cos(t)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(cosec(y),1/sin(y))",
        [
            "Planner search:",
            "Rule: Reciprocal identities:",
            "1/sin(y)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(cot(t)-tan(t),2*cot(2*t))",
        [
            "Planner search:",
            "Common denominator:",
            "2*cot(2*t)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(2*sin(t)*cos(t),sin(2*t))",
        [
            "Planner search:",
            "Double-angle identities:",
            "sin(2*t)",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(a*x^2+b*x+c,3*(x+2)^2+13)",
        [
            "Compare coefficients:",
            "Expand target:",
            "a = 3, b = 12, c = 25",
            "Verified by substitution",
        ],
    ),
    (
        "xform(p*x^2+q*x+r,2*(x-3)^2+5)",
        [
            "Compare coefficients:",
            "p = 2, q = -12, r = 23",
            "Verified by substitution",
        ],
    ),
    (
        "xform((x+1)^2+(x-1)^2,2*x^2+2)",
        [
            "Planner search:",
            "Expand expression:",
            "Verified by equivalence check",
        ],
    ),
]

XFORM_EQUIV_FALLBACK_CASES = [
    (
        "xform(sin(x)^2,(1-cos(2*x))/2)",
        [
            "Check equivalence:",
            "Difference simplifies to 0",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(log(2,x*y),log(2,x)+log(2,y))",
        [
            "Product:",
            "log_a(uv)=log_a(u)+log_a(v)",
            "Verified by equivalence check",
        ],
    ),
]

XFORM_LOG_LAW_CASES = [
    (
        "xform(log(2,x/y),log(2,x)-log(2,y))",
        [
            "Quotient:",
            "log_a(u/v)=log_a(u)-log_a(v)",
            "Condition: 2>0, 2!=1; x>0; y>0",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(log(2,x)+log(2,y),log(2,x*y))",
        [
            "Product:",
            "log_a(u)+log_a(v)=log_a(uv)",
            "Condition: 2>0, 2!=1; x>0; y>0",
            "Verified by equivalence check",
        ],
    ),
    (
        "xform(ln(x+1)+ln(x-1),ln(x^2-1))",
        [
            "Product:",
            "log_a(u)+log_a(v)=log_a(uv)",
            "Condition: e>0, e!=1; x+1>0; x-1>0",
            "ln(x^2-1)",
            "Verified by equivalence check",
        ],
    ),
]

ARGUMENT_POLICY_CASES = [
    ("factor()", ["Err: missing arguments", "factor(expr[,var])"]),
    ("diff()", ["Err: missing arguments", "diff(...)"]),
    ("texpand()", ["Err: missing arguments", "texpand(expr)"]),
]

FACTOR_CUBIC_POLICY_CASES = [
    (
        "factor(x^3+4*x^2+7*x+6)",
        ["Factor:", "(x + 2)*(x^2 + 2*x + 3)", "Verified by equivalence check"],
    ),
    (
        "factor(x^3-6*x^2+11*x-6)",
        ["Factor:", "(x - 3)*(x - 2)*(x - 1)", "Verified by equivalence check"],
    ),
    (
        "factor(2*x^3-3*x^2-8*x+12)",
        ["Factor:", "(x - 2)*(x + 2)*(2*x - 3)", "Verified by equivalence check"],
    ),
]

BAD_WORKING_MARKERS = [
    "Exact:",
    "fake",
    "symbolic form",
    "No verified A-level working route found.",
]


def run(expr: str) -> str:
    proc = subprocess.run(
        [str(RUNNER), "--alg", expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=10,
    )
    out = (proc.stdout or "") + (proc.stderr or "")
    if proc.returncode:
        raise AssertionError(f"{expr} exited {proc.returncode}\n{out}")
    return out


def assert_contains(label: str, text: str, markers: list[str]) -> None:
    compact_text = text.replace(" ", "").replace("*", "")
    missing = [
        m for m in markers
        if m not in text and m.replace(" ", "").replace("*", "") not in compact_text
    ]
    if missing:
        raise AssertionError(f"{label}: missing {missing}")


def check_source_policy() -> None:
    for rel, markers in SOURCE_MARKERS.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        assert_contains(rel, text, markers)
        if "No verified A-level working route found." in text:
            raise AssertionError(f"{rel}: old failure string leaked")
    for rel, snippets in FORBIDDEN_SOURCE_SNIPPETS.items():
        text = (ROOT / rel).read_text(encoding="utf-8")
        leaked = [s for s in snippets if s in text]
        if leaked:
            raise AssertionError(f"{rel}: exact-route snippets leaked {leaked}")


def check_host_policy() -> None:
    for expr in VERIFIED_HOST_POLICY_CASES:
        out = run(expr)
        assert_contains(expr, out, ["Verified"])
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in EQUIVALENCE_POLICY_CASES + ARGUMENT_POLICY_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        if expr.startswith("xform(sin(x),cos(x)+7)") and "Verified" in out:
            raise AssertionError(f"{expr}: non-equivalent route marked verified\n{out}")
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in XFORM_PLANNER_CASES + XFORM_EQUIV_FALLBACK_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        if "not equivalent" in out.lower():
            raise AssertionError(f"{expr}: equivalent form marked not equivalent\n{out}")
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in XFORM_LOG_LAW_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        if "not equivalent" in out.lower():
            raise AssertionError(f"{expr}: equivalent log-law form marked not equivalent\n{out}")
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in TRIG_XFORM_POLICY_CASES + LINEAR_PARAMETER_XFORM_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in SOLVE_POLICY_CASES + INTEGRAL_POLICY_CASES + PARTFRAC_POLICY_CASES + DOMAIN_POLICY_CASES + RANGE_POLICY_CASES + IMPLICIT_POLICY_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in SOLVE_FALLBACK_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        leaked = [m for m in ("ln(0)", "roots(F(") if m in out]
        if "last verified state:" in out and "KhiCAS exact evaluation:\n[]" in out and "\nVerified" in out:
            leaked.append("fake Verified on exact [] fallback")
        if leaked:
            raise AssertionError(f"{expr}: fake fallback leaked {leaked}\n{out}")
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")

    for expr, markers in FACTOR_CUBIC_POLICY_CASES:
        out = run(expr)
        assert_contains(expr, out, markers)
        bad = [m for m in BAD_WORKING_MARKERS if m.lower() in out.lower()]
        if bad:
            raise AssertionError(f"{expr}: bad working markers {bad}\n{out}")


def main() -> int:
    check_source_policy()
    check_host_policy()
    print(
        "OK targeted working policy "
        f"source_files={len(SOURCE_MARKERS)} "
        f"verified_cases={len(VERIFIED_HOST_POLICY_CASES)} "
        f"solve_cases={len(SOLVE_POLICY_CASES)} "
        f"solve_fallback_cases={len(SOLVE_FALLBACK_CASES)} "
        f"integral_cases={len(INTEGRAL_POLICY_CASES)} "
        f"partfrac_cases={len(PARTFRAC_POLICY_CASES)} "
        f"domain_cases={len(DOMAIN_POLICY_CASES)} "
        f"range_cases={len(RANGE_POLICY_CASES)} "
        f"implicit_cases={len(IMPLICIT_POLICY_CASES)} "
        f"equivalence_cases={len(EQUIVALENCE_POLICY_CASES)} "
        f"xform_planner_cases={len(XFORM_PLANNER_CASES)} "
        f"xform_equiv_fallback_cases={len(XFORM_EQUIV_FALLBACK_CASES)} "
        f"xform_log_law_cases={len(XFORM_LOG_LAW_CASES)} "
        f"xform_linear_parameter_cases={len(LINEAR_PARAMETER_XFORM_CASES)} "
        f"argument_cases={len(ARGUMENT_POLICY_CASES)} "
        f"factor_cubic_cases={len(FACTOR_CUBIC_POLICY_CASES)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

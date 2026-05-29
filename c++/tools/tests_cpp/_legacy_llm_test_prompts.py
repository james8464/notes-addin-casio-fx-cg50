# Optional LLM case generation (tests/run_tests.py). Output: one line per field, no prose.
# Keep prompts short so small models stay in format.

LLM_GENERATION_PROMPTS = {
    "Algebra": {
        "chaos": """Random ugly eq in x. Mix types (linear, quad, abs, rat). Coeffs roughly -50..50.
Return ONLY (no other text):
EQ: ...=0
ANSWER: solutions, comma sep""",
        "hard": """Hard eq in x. Deg 2-4 poly, or exp/ln/trig style. Coeffs -20..20.
Return ONLY:
EQ: ...=0
ANSWER: solutions, comma sep""",
        "easy": """Simple eq in x (linear or easy quad).
Return ONLY:
EQ: ...=0
ANSWER: solution(s)""",
    },
    "Derive": {
        "chaos": """Random expr in x to d/dx. Products, quotients, chain, nested. sin/cos/tan, exp, ln, poly.
Return ONLY:
EXPR: ...
DERIV: ...""",
        "hard": """Expr for d/dx. Use chain / product / quotient.
Return ONLY:
EXPR: ...
DERIV: ...""",
        "easy": """Simple expr: x^n or a*x+b.
Return ONLY:
EXPR: ...
DERIV: ...""",
    },
    "Integrate": {
        "chaos": """Random integrand w.r.t. x. trig, exp, ln, rat, sqrt, nest ok.
Return ONLY:
EXPR: ...
INT: ...+ C""",
        "hard": """Integrand for u-sub or parts.
Return ONLY:
EXPR: ...
INT: ...+ C""",
        "easy": """Simple x^n.
Return ONLY:
EXPR: ...
INT: ...+ C""",
    },
    "Trigonometry": {
        "chaos": """Random trig eq in x. sin/cos/tan/cosec etc. rad or deg ok.
Return ONLY:
EQ: ...
SOL: ...""",
        "hard": """Challenging trig eq.
Return ONLY:
EQ: ...
SOL: ...""",
        "easy": """e.g. sin(x)=1/2 style.
Return ONLY:
EQ: ...
SOL: ...""",
    },
    "Matrix": {
        "chaos": """2x2 or 3x3. +, *, det, or inverse.
Return ONLY:
OP: ...
MATRIX: ...
ANSWER: ...""",
        "hard": """Matrix op.
Return ONLY:
OP: ...
MATRIX: ...
ANSWER: ...""",
        "easy": """Simple matrix op.
Return ONLY:
OP: ...
MATRIX: ...
ANSWER: ...""",
    },
    "Complex": {
        "chaos": """Random complex # problem: +,-,*,/, polar, De Moivre.
Return ONLY:
EXPR: ...
ANSWER: ...""",
        "hard": """Complex # problem.
Return ONLY:
EXPR: ...
ANSWER: ...""",
        "easy": """Simple a+bi style.
Return ONLY:
EXPR: ...
ANSWER: ...""",
    },
    "Solve": {
        "chaos": """Random eq. poly, rat, rad, exp, ln, trig ok.
Return ONLY:
EQ: ...
SOL: ...""",
        "hard": """Challenging eq.
Return ONLY:
EQ: ...
SOL: ...""",
        "easy": """Simple eq.
Return ONLY:
EQ: ...
SOL: ...""",
    },
    "Calculate": {
        "chaos": """Num eval: n!, C(n,k), big arith, trig, ln, etc.
Return ONLY:
EXPR: ...
ANSWER: number""",
        "hard": """Num computation.
Return ONLY:
EXPR: ...
ANSWER: ...""",
        "easy": """Simple arith.
Return ONLY:
EXPR: ...
ANSWER: ...""",
    },
}

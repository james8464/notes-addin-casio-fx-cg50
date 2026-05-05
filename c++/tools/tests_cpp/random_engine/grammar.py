from __future__ import annotations

import random


SUPPORTED_TOKENS = (
    "sin",
    "cos",
    "tan",
    "sec",
    "cot",
    "cosec",
    "abs",
    "sqrt",
    "ln",
    "log",
    "exp",
    "asin",
    "acos",
    "atan",
    "!",
)


class ExpressionGrammar:
    def __init__(self, rng: random.Random | None = None):
        self.rng = rng or random.Random()

    def affine(self, var: str = "x") -> str:
        a = self.rng.choice([2, 3, 4, 5, -2, -3])
        b = self.rng.choice([-5, -3, -1, 1, 4, 7])
        sign = "+" if b >= 0 else "-"
        return "{0}*{1}{2}{3}".format(a, var, sign, abs(b))

    def quadratic(self, var: str = "x") -> str:
        r1 = self.rng.choice([-5, -3, -1, 2, 4])
        r2 = self.rng.choice([-4, -2, 1, 3, 5])
        return "({0}-{1})*({0}-{2})".format(var, r1, r2)

    def hard_trig_identity(self) -> str:
        return self.rng.choice(
            [
                "(sin(x)+cos(x))^2-2*sin(x)*cos(x)",
                "sec(2*x+pi/6)^2-tan(2*x+pi/6)^2",
                "cosec(x)^2-cot(x)^2",
                "(1-cos(2*x))/sin(x)",
                "(sin(3*x)-sin(x))/(cos(3*x)+cos(x))",
            ]
        )

    def hard_integrand(self, method: str = "auto") -> str:
        a = self.rng.choice([2, 3, 4, 5])
        b = self.rng.choice([1, 2, 3, 6])
        n = self.rng.choice([1, 2, 3, 4, 5])
        pools = {
            "parts": [
                f"exp({a}*x)*cos({b}*x)",
                f"exp({a}*x)*sin({b}*x)",
                f"x^{n}*ln(x)",
                f"x^{n}*exp({a}*x)",
                f"asin((x-{b})/{a + b + 1})",
                f"acos((x-{b})/{a + b + 1})",
            ],
            "sub": [
                "1/(1+sqrt(x))",
                "exp(sqrt(x))",
                f"1/(x*ln(x)^{max(1, b)})",
                f"cos({a}*x)*exp(sin({a}*x))",
                f"x/sqrt({b}+x^2)",
                f"{a}*x/(1+x^2)^{b}",
            ],
            "pf": [
                "1/(x^2*(x+1))",
                "1/(x*(x^2+1))",
                "(x^2+1)/(x^4+1)",
                "1/(x^4+1)",
                f"({a}*x+{b})/((x-{a})*(x^2+{b}))",
                f"1/((x-{a})^2*(x+{b}))",
            ],
            "trig": [
                "sec(x)^3",
                "tan(x)^3",
                "sin(x)^4",
                "1/(1+sin(x)+cos(x))",
                f"sin({a}*x)^{2 * max(1, b)}",
                f"tan({a}*x)^{2*n+1}",
            ],
        }
        return self.rng.choice(pools.get(method, sum(pools.values(), [])))

    def range_expr(self) -> str:
        return self.rng.choice(
            [
                "sqrt(abs(3*x+1)+1)",
                "abs(2*x-5)+3",
                "x/(1+x^2)",
                "(x^2-x+1)/(x^2+x+1)",
                "sin(x)^2-4*sin(x)+3",
            ]
        )

    def domain_expr(self) -> str:
        return self.rng.choice(
            [
                "sqrt(log(0.5,x-1))",
                "ln(sin(x))",
                "1/sqrt(abs(x)-x)",
                "sqrt(x-sqrt(x))",
                "ln(1-x^2)",
            ]
        )

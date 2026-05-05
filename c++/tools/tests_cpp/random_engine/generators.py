from __future__ import annotations

from dataclasses import dataclass
import itertools
import random
from typing import Iterable, List

from .concepts import ConceptNode
from .grammar import ExpressionGrammar
from .transforms import apply_transforms, random_transform_chain


@dataclass
class AdversarialCase:
    label: str
    command_flag: str
    command_expr: str
    concept: ConceptNode
    expects_working: bool = True
    expected_note: str = ""
    source_kernel: str = ""

    @property
    def input_text(self) -> str:
        return "--{0} {1}".format(self.command_flag, self.command_expr)


class AdversarialGenerator:
    def __init__(self, seed: int | None = None):
        self.seed = seed
        self.rng = random.Random(seed)
        self.grammar = ExpressionGrammar(self.rng)

    def generate(self, feature: str, count: int) -> List[AdversarialCase]:
        feature = (feature or "all").lower()
        if feature in ("all", "exammix", "exam"):
            features = ["integrate", "diff", "solve_trig", "domain", "range", "constants", "rewrite"]
        else:
            features = [feature]
        out = []
        seen = set()
        attempts = 0
        limit = max(20, int(count) * 20)
        for name in itertools.cycle(features):
            if len(out) >= max(0, int(count)) or attempts >= limit:
                break
            attempts += 1
            case = self._one(name, len(out) + 1)
            key = case.input_text
            if key in seen:
                continue
            seen.add(key)
            out.append(case)
        return out

    def concepts_for(self, feature: str) -> List[ConceptNode]:
        return [case.concept for case in self.generate(feature, 36)]

    def _one(self, feature: str, index: int) -> AdversarialCase:
        dispatch = {
            "integrate": self._integrate,
            "int": self._integrate,
            "diff": self._diff,
            "derive": self._diff,
            "solve_trig": self._solve_trig,
            "trig": self._solve_trig,
            "domain": self._domain,
            "range": self._range,
            "constants": self._constants,
            "rewrite": self._rewrite,
        }
        return dispatch.get(feature, self._rewrite)(index)

    def _concept(self, function: str, sig: str, method: str, topic: str, transforms, difficulty: int, oracle: str) -> ConceptNode:
        return ConceptNode(function, sig, method, topic, tuple(transforms), difficulty, oracle)

    def _integrate(self, index: int) -> AdversarialCase:
        method = self.rng.choice(["auto", "parts", "sub", "pf", "trig"])
        kernel = self.grammar.hard_integrand("parts" if method == "auto" else method)
        difficulty = self.rng.choice([3, 4, 5])
        transforms = random_transform_chain(self.rng, "integration", difficulty)
        expr = apply_transforms(kernel, transforms, self.rng)
        command = "{0},method={1}".format(expr, method)
        concept = self._concept("integrate", "expr,[method]", method, "integration", transforms, difficulty, "sympy+llm")
        return AdversarialCase(
            "Adv integrate {0}: {1}".format(index, method),
            "int",
            command,
            concept,
            True,
            "antiderivative differentiates to integrand; working shows chosen route",
            kernel,
        )

    def _diff(self, index: int) -> AdversarialCase:
        method = self.rng.choice(["auto", "implicit", "param", "logdiff", "chain"])
        difficulty = self.rng.choice([2, 3, 4])
        if method == "implicit":
            expr = self.rng.choice(["x^y=y^x", "sin(x*y)+x^2=y^2", "ln(x+y)=x*y"])
            command = "{0},x,method=implicit".format(expr)
            topic = "implicit"
        elif method == "param":
            expr = self.rng.choice(["x=t^2+1/t,y=t^2-1/t,t,x", "x=exp(t)*cos(t),y=exp(t)*sin(t),t,x"])
            command = "{0},method=param".format(expr)
            topic = "parametric"
        elif method == "logdiff":
            expr = self.rng.choice(["x^x", "x^(sin(x))", "(sin(x))^x"])
            command = "{0},x,method=logdiff".format(expr)
            topic = "logdiff"
        else:
            expr = self.rng.choice(["sin((x+1)^2)", "ln(sqrt(x^2+1))", "(x^2+1)*exp(3*x)"])
            command = "{0},x,method={1}".format(expr, method)
            topic = "standard"
        concept = self._concept("diff", "expr,var,[method]", method, topic, (), difficulty, "sympy+llm")
        return AdversarialCase("Adv diff {0}: {1}".format(index, method), "derive", command, concept, True, "derivative verified; working shows rule", command)

    def _solve_trig(self, index: int) -> AdversarialCase:
        method = self.rng.choice(["auto", "rform", "identity", "bounded", "square_then_check"])
        k = self.rng.choice([2, 3, 4, 5])
        target = self.rng.choice(["0", "1/2", "-1/2", "sqrt(2)/2", "sqrt(3)/2"])
        expr = self.rng.choice(
            [
                "3*cos(x)+4*sin(x)=2",
                "5*sin(x)-12*cos(x)=6",
                "sqrt(3)*cos(x)-sin(x)=1",
                "cos(2*x)+sin(2*x)=1",
                "cos(2*x)+sqrt(3)*sin(2*x)=1",
                f"sin({k}*x)-sin(x)=0",
                f"cos({k}*x)-cos(x)=0",
                f"sin({k}*x)={target}",
                "2*sin(x)^2=1+cos(x)",
                "(2*sin(x)*cos(x))/(cos(x)^2-sin(x)^2)=sin(x)/cos(x)",
            ]
        )
        interval = self.rng.choice(["x,0,2*pi,10", "x,-pi,pi,10", "x,0,360,10"]) if method in ("bounded", "auto", "rform") else "x"
        command = "{0},{1},method={2}".format(expr, interval, method)
        concept = self._concept("solve_trig", "eq,var,[lo,hi,max],[method]", method, "trig", ("identity_rewrite",), 4, "substitution+llm")
        return AdversarialCase("Adv solve_trig {0}: {1}".format(index, method), "trig", command, concept, True, "solutions satisfy equation and working shows identity route", expr)

    def _domain(self, index: int) -> AdversarialCase:
        expr = self.grammar.domain_expr()
        concept = self._concept("domain", "expr,[var,lo,hi]", "auto", "domain", ("branch_guard",), 3, "symbolic+sampling")
        return AdversarialCase("Adv domain {0}".format(index), "alg", "domain({0})".format(expr), concept, False, "domain constraints correct", expr)

    def _range(self, index: int) -> AdversarialCase:
        expr = self.grammar.range_expr()
        concept = self._concept("range", "expr,[var,lo,hi]", "auto", "range", ("branch_guard",), 3, "symbolic+sampling")
        return AdversarialCase("Adv range {0}".format(index), "alg", "range({0})".format(expr), concept, False, "range constraints correct", expr)

    def _constants(self, index: int) -> AdversarialCase:
        expr = self.rng.choice(
            [
                "(a*x+b)*(x-2)+c*(x+1)^2=4*x^2+6*x-1",
                "x^2+a*x+b=(x-2)^2+5",
                "(x+a)^3-(x-a)^3=12*x^2+16",
            ]
        )
        concept = self._concept("fitconst", "identity", "equate_coeffs", "constants", ("expand", "collect"), 4, "substitution+llm")
        return AdversarialCase("Adv constants {0}".format(index), "alg", "fitconst({0})".format(expr), concept, True, "constants verified by substitution", expr)

    def _rewrite(self, index: int) -> AdversarialCase:
        expr = self.rng.choice([self.grammar.hard_trig_identity(), "(x^4-1)/(x^2-1)", "sqrt(12+sqrt(140))"])
        concept = self._concept("rewrite", "expr,[target]", "auto", "rewrite", ("hidden_identity",), 3, "sympy+llm")
        return AdversarialCase("Adv rewrite {0}".format(index), "alg", "rewrite({0})".format(expr), concept, True, "rewritten form equivalent with justified steps", expr)

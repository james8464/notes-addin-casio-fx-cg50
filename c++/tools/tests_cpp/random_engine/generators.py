from __future__ import annotations

from dataclasses import dataclass
import itertools
import random
from typing import Iterable, List

from .concepts import ConceptNode
from .grammar import ExpressionGrammar
from .source_bank import SOURCE_FAMILIES, source_case_data
from .transforms import apply_transforms, random_transform_chain


EXAM_GAP_TOPICS = (
    "looping_parts",
    "di_table",
    "inverse_trig_parts",
    "hidden_substitution",
    "partial_fraction_setup",
    "algebraic_symmetry",
    "trig_power_reduction",
    "definite_symmetry",
    "implicit_collect",
    "parametric_second",
    "interval_trig",
    "trig_poly_reject",
    "log_base_domain",
    "abs_sum_range",
    "hidden_constants",
    "implicit_rational_clear",
    "radical_rewrite",
    "domain_range_edge",
    "formatting_traps",
)

SYLLABUS_TOPICS = (
    "pure_proof",
    "pure_algebra",
    "pure_functions_graphs",
    "pure_coordinate_geometry",
    "pure_sequences",
    "pure_trigonometry",
    "pure_explogs",
    "pure_differentiation",
    "pure_integration",
    "pure_numerical_methods",
    "pure_vectors",
    "stats_sampling_data",
    "stats_probability",
    "stats_distributions",
    "stats_hypothesis",
    "mechanics_kinematics",
    "mechanics_forces",
    "mechanics_moments",
)

ONLINE_TOPICS = tuple(f.topic for f in SOURCE_FAMILIES)

CRASH_TOPICS = (
    "long_parse_guard",
    "nested_rewrite_guard",
    "trig_many_roots",
    "chain_depth_guard",
    "domain_singularity_guard",
    "working_truncation_guard",
)


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
            features = ["exam_gap", "integrate", "diff", "solve_trig", "domain", "range", "constants", "rewrite", "general", "online", "syllabus"]
        elif feature == "general":
            features = ["general"]
        else:
            features = [feature]
        allow_repeats = feature in ("syllabus", "alevel", "online", "sources", "crash", "safety")
        out = []
        seen = set()
        attempts = 0
        limit = max(20, int(count) * 20)
        for name in itertools.cycle(features):
            if len(out) >= max(0, int(count)) or attempts >= limit:
                break
            attempts += 1
            case = self._one(name, attempts)
            key = case.input_text
            if key in seen and not allow_repeats:
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
            "general": self._general,
            "exam_gap": self._exam_gap,
            "full_marks": self._exam_gap,
            "syllabus": self._syllabus,
            "alevel": self._syllabus,
            "online": self._online,
            "sources": self._online,
            "crash": self._crash,
            "safety": self._crash,
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
                "(x+a)^2+(y+b)^2=x^2+y^2-6*x+10*y+34,[a,b]",
            ]
        )
        concept = self._concept("fitconst", "identity", "equate_coeffs", "constants", ("expand", "collect"), 4, "substitution+llm")
        return AdversarialCase("Adv constants {0}".format(index), "alg", "fitconst({0})".format(expr), concept, True, "constants verified by substitution", expr)

    def _rewrite(self, index: int) -> AdversarialCase:
        expr = self.rng.choice([self.grammar.hard_trig_identity(), "(x^4-1)/(x^2-1)", "sqrt(12+sqrt(140))"])
        concept = self._concept("rewrite", "expr,[target]", "auto", "rewrite", ("hidden_identity",), 3, "sympy+llm")
        return AdversarialCase("Adv rewrite {0}".format(index), "alg", "rewrite({0})".format(expr), concept, True, "rewritten form equivalent with justified steps", expr)

    def _general(self, index: int) -> AdversarialCase:
        flag, expr, topic, expects_working = self.grammar.general_expr()
        difficulty = 5 if topic in ("non_elementary", "branch", "special_function") else 4
        transforms = ("nested", "branch_guard") if topic != "non_elementary" else ("special_function",)
        concept = self._concept("general", "expr,[method/options]", "auto", topic, transforms, difficulty, "sympy+llm")
        note = "advanced unrestricted combination; answer must be correct and working must justify special/domain/branch route"
        return AdversarialCase("Adv general {0}: {1}".format(index, topic), flag, expr, concept, expects_working, note, expr)

    def _exam_gap(self, index: int) -> AdversarialCase:
        """Worksheet-style traps where a right answer can still lose method marks."""
        cycle = max(0, (index - 1) // len(EXAM_GAP_TOPICS))
        n = 2 + (cycle % 5)
        a = 2 + (cycle % 6)
        b = 1 + ((2 * cycle + 1) % 7)
        inv = ("asin", "acos", "atan")[cycle % 3]
        trig = ("sin", "cos")[cycle % 2]
        m_sur = 2 + cycle
        n_sur = 1 + (cycle % 8)
        cases = [
            (
                "looping_parts",
                "int",
                f"exp({a}*x)*{trig}({b}*x),method=parts",
                "integrate",
                "expr,method=parts",
                "parts",
                ("ibp_loop", "solve_repeated_integral"),
                "exam: show I/J, u/dv/du/v twice, collect repeated integral, final simplified form",
            ),
            (
                "di_table",
                "int",
                ["x^3*e^(2*x),method=di", "x^2*cos(3*x),method=di", "x^2*sin(5*x),method=di"][cycle % 3],
                "integrate",
                "expr,method=di",
                "di",
                ("di_table", "alternating_signs", "collect_terms"),
                "exam: show D/I columns, alternating signs, then final collected answer",
            ),
            (
                "inverse_trig_parts",
                "int",
                f"{inv}((x-{b})/{a + b + 2}),method=parts",
                "integrate",
                "expr,method=parts",
                "parts",
                ("ibp", "substitution", "back_sub"),
                "exam: show substitution, differential, IBP u/dv/du/v, back-substitution",
            ),
            (
                "hidden_substitution",
                "int",
                [
                    "1/(1+sqrt(x)),method=sub",
                    f"1/(x*sqrt(1+x^{n})),method=sub",
                    "exp(sqrt(x)),method=sub",
                    "x/(sqrt(1+x^2)*(1+sqrt(1+x^2))),method=sub",
                ][cycle % 4],
                "integrate",
                "expr,method=sub",
                "sub",
                ("reverse_chain", "rationalise", "back_sub"),
                "exam: show substitution, dx/du or du/dx, transformed integral, algebra step, back-substitution",
            ),
            (
                "partial_fraction_setup",
                "int",
                [
                    f"({a + 3}*x+{b + 5})/((x-{(cycle % 4) + 1})*(x^2+{(cycle % 6) + 2})),method=pf",
                    f"({a}*x^2+{b}*x+{a + b})/((x-{(cycle % 3) + 1})^2*(x^2+{(cycle % 5) + 1})),method=pf",
                    "1/(x^2*(x+1)),method=pf",
                    "(x^2+1)/(x^4+1),method=pf",
                ][cycle % 4],
                "integrate",
                "expr,method=pf",
                "pf",
                ("pf_setup", "equate_coefficients", "integrate_terms"),
                "exam: show PF form, multiply through, coefficient equations, A/B/C values, integrate terms",
            ),
            (
                "algebraic_symmetry",
                "int",
                [
                    "(x^2+1)/(x^4+x^2+1),method=sub",
                    "(x^2-1)/(x^4+1),method=sub",
                    "(x^2+1)/(x^4+1),method=sub",
                ][cycle % 3],
                "integrate",
                "expr,method=sub",
                "sub",
                ("divide_by_x2", "x_plus_minus_reciprocal", "substitution"),
                "exam: divide by x^2, spot x±1/x derivative, show u-sub and standard result",
            ),
            (
                "trig_power_reduction",
                "int",
                ["sin(x)^4,method=trig", "cos(x)^6,method=trig", "sin(x)^2*cos(x)^4,method=trig"][cycle % 3],
                "integrate",
                "expr,method=trig",
                "trig",
                ("power_reduction", "double_angle", "integrate_terms"),
                "exam: show trig identities used, expand/collect, integrate each term",
            ),
            (
                "definite_symmetry",
                "int",
                [
                    f"defint(sin(x)^{n}/(sin(x)^{n}+cos(x)^{n}),x,0,pi/2)",
                    "defint(ln(sin(x)),x,0,pi/2)",
                    "defint(ln(x)/(1+x^2),x,0,inf)",
                ][cycle % 3],
                "integrate",
                "defint(expr,var,lo,hi)",
                "symmetry",
                ("kings_property", "limit_transform", "combine"),
                "exam: show symmetry/substitution limits, combine integrals, conclude value",
            ),
            (
                "implicit_collect",
                "derive",
                [
                    "x^y=y^x,x,method=implicit",
                    "sin(x*y)+x^2=y^2,x,method=implicit",
                    "ln(x+y)=x*y,x,method=implicit",
                    "1/(2*x+1)+1/(y+1)=x^2,x,method=implicit",
                ][cycle % 4],
                "diff",
                "eq,var,method=implicit",
                "implicit",
                ("differentiate_both_sides", "collect_dy_dx", "isolate"),
                "exam: show differentiate both sides, collect dy/dx terms, factor/isolate",
            ),
            (
                "implicit_rational_clear",
                "derive",
                "1/(2*x+1)+1/(y+1)=x^2,x,method=implicit",
                "diff",
                "eq,var,method=implicit",
                "implicit",
                ("clear_denominators", "differentiate_both_sides", "collect_dy_dx"),
                "exam: state denominator restrictions, clear denominators, differentiate, collect dy/dx",
            ),
            (
                "parametric_second",
                "derive",
                ["x=t^2+1/t,y=t^2-1/t,t,x,method=param_second", "x=exp(t)*cos(t),y=exp(t)*sin(t),t,x,method=param_second"][cycle % 2],
                "diff",
                "x(t),y(t),t,x,method=param_second",
                "param_second",
                ("dxdt_dydt", "dy_dx", "second_derivative"),
                "exam: show dx/dt, dy/dt, dy/dx, then divide d/dt(dy/dx) by dx/dt",
            ),
            (
                "interval_trig",
                "trig",
                [
                    "3*cos(x)+4*sin(x)=2,x,0,2*pi,10,method=rform",
                    "sin(3*x)=sin(x),x,0,2*pi,10,method=identity",
                    "2*sin(x)^2=1+cos(x),x,0,2*pi,10,method=identity",
                ][cycle % 3],
                "solve_trig",
                "eq,var,lo,hi,max,method",
                "rform",
                ("identity", "general_or_cast", "interval_filter"),
                "exam: show identity/R-form, general roots, interval filtering, reject invalid roots",
            ),
            (
                "trig_poly_reject",
                "trig",
                [
                    "2*cos(x)^2+3*cos(x)-2=0,x,0,2*pi,10,method=identity",
                    "2*sin(x)^2+3*sin(x)-2=0,x,0,2*pi,10,method=identity",
                ][cycle % 2],
                "solve_trig",
                "eq,var,lo,hi,max,method",
                "identity",
                ("trig_poly_sub", "reject_invalid_root", "interval_filter"),
                "exam: let u=sin/cos, solve quadratic, reject roots outside [-1,1], filter interval",
            ),
            (
                "radical_rewrite",
                "alg",
                f"rewrite(sqrt({m_sur + n_sur}+sqrt({4 * m_sur * n_sur})))",
                "rewrite",
                "expr",
                "radical_decomposition",
                ("square_both_sides", "match_surds", "solve_mn"),
                "exam: show sqrt(m)+sqrt(n), m+n=a, 4mn=b, solve m,n",
            ),
            (
                "hidden_constants",
                "alg",
                [
                    "fitconst((a*x+b)*(x-2)+c*(x+1)^2=4*x^2+6*x-1,[a,b,c])",
                    "fitconst(a*(x+1)^2+b*(x-1)^2+c*(x^2-1)=6*x^2+4*x+2,[a,b,c])",
                    "fitconst((x+a)^2+(y+b)^2=x^2+y^2-6*x+10*y+34,[a,b])",
                ][cycle % 3],
                "fitconst",
                "identity,vars",
                "equate_coeffs",
                ("expand", "collect", "solve_constants"),
                "exam: expand, collect, equate coefficients, solve constants, verify identity",
            ),
            (
                "log_base_domain",
                "alg",
                ["domain(sqrt(log(1/2,x-1)))", "domain(sqrt(log(2,x-1)))"][cycle % 2],
                "domain",
                "expr",
                "auto",
                ("log_base_inequality", "monotone_base", "sqrt_guard"),
                "exam: handle base<1/base>1 inequality direction and solve the linear guard",
            ),
            (
                "abs_sum_range",
                "alg",
                f"range(abs(x-{a})+abs(x-{a + b}))",
                "range",
                "expr",
                "auto",
                ("piecewise_abs", "minimum_distance"),
                "exam: range is y>=distance between breakpoints",
            ),
            (
                "domain_range_edge",
                "alg",
                [
                    f"range(sqrt(abs({a}*x+{b})+1))",
                    f"range(log(10,abs({a}*x+{b})+3))",
                    f"domain(arcsin((x-{b})/{a + b + 2}))",
                ][cycle % 3],
                "range_domain",
                "expr,[var,lo,hi]",
                "auto",
                ("abs_edge", "inverse_domain", "log_guard"),
                "exam: concise domain/range constraints with no graph-scan fallback",
            ),
            (
                "formatting_traps",
                ["derive", "alg", "int", "derive"][cycle % 4],
                [
                    "(2*x+ln(x))^3,x",
                    "solve(log(2,x^2+4*x+3)=4+log(2,x^2+x),x)",
                    "x^2*cos(3*x),method=di",
                    "(x^2+1)*sin(x),x",
                ][cycle % 4],
                ["diff", "solve", "integrate", "diff"][cycle % 4],
                ["expr,var", "eq,var", "expr,method", "expr,var"][cycle % 4],
                ["chain", "log_exp", "di", "product"][cycle % 4],
                ("math_only_lines", "no_standalone_assignments", "no_generic_scaffold"),
                "exam: output must be copyable maths lines; no Done/pick-rule prose or lone y=/u=/dy/d fragments",
            ),
        ]
        topic, flag, expr, function, sig, method, transforms, note = cases[(index - 1) % len(cases)]
        concept = self._concept(function, sig, method, topic, transforms, 5, "sympy+llm+exam")
        return AdversarialCase("Adv exam-gap {0}: {1}".format(index, topic), flag, expr, concept, True, note, expr)

    def _syllabus(self, index: int) -> AdversarialCase:
        """Edexcel 9MA0 capability matrix pressure cases.

        These are not past-paper clones. They are compact kernels chosen so the
        fuzzer can mutate every normal A-level Maths command surface.
        """
        i = max(0, index - 1)
        cases = [
            ("pure_proof", "alg", "compare((a+b)^2,a^2+2*a*b+b^2)", "compare", "lhs,rhs", "equivalence", ("expand", "equate_terms"), True),
            ("pure_algebra", "alg", "solve(x^2-5*x+6=0,x,method=factor)", "solve", "eq,var,method", "factor", ("factor", "check_roots"), True),
            ("pure_functions_graphs", "alg", "range(x/(1+x^2))", "range", "expr", "auto", ("stationary_points", "bounds"), False),
            ("pure_coordinate_geometry", "alg", "complete_square(x^2+y^2-6*x+10*y+34)", "geometry", "equation", "complete_square", ("complete_square", "centre_radius"), True),
            ("pure_sequences", "alg", "binomial((1+x)^5,x,0,5)", "binomial", "expr,var,point,order", "expand", ("pascal", "validity"), True),
            ("pure_trigonometry", "trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,10,method=rform", "solve_trig", "eq,var,lo,hi,max,method", "rform", ("rform", "interval_filter"), True),
            ("pure_explogs", "alg", "solve(log(2,x-1)+log(2,x+3)=3,x,method=log_exp)", "solve", "eq,var,method", "log_exp", ("domain", "log_laws"), True),
            ("pure_differentiation", "derive", "sin((x+1)^2),x,method=chain", "diff", "expr,var,method", "chain", ("chain_rule",), True),
            ("pure_integration", "int", "x^2*e^(2*x),method=di", "integrate", "expr,method", "di", ("di_table", "collect_terms"), True),
            ("pure_numerical_methods", "alg", "solve(x^2-2=0,x,method=numeric)", "numeric", "eq,var,method", "numeric", ("iteration", "convergence"), True),
            ("pure_vectors", "alg", "sqrt(1^2+2^2+3^2)", "vectors", "components", "magnitude", ("component_square_sum",), False),
            ("stats_sampling_data", "stats", "stats(1,2,2,3,5,8)", "stats", "data", "summary", ("mean", "spread"), False),
            ("stats_probability", "stats", "binom(10,.5,4)", "probability", "n,p,k", "binomial", ("formula", "cdf_or_pmf"), False),
            ("stats_distributions", "stats", "normalcdf(0,1,-1,1)", "distributions", "mu,sigma,lo,hi", "normal", ("standardise",), False),
            ("stats_hypothesis", "stats", "ztest(5.4,5,1.2,36,0.05,gt)", "hypothesis", "xbar,mu,sigma,n,alpha,tail", "ztest", ("tail_prob", "compare_alpha"), True),
            ("mechanics_kinematics", "suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=suvat", "mechanics", "knowns,target", "suvat", ("choose_formula", "substitute"), True),
            ("mechanics_forces", "alg", "solve(3*a=12-6,a,method=linear)", "mechanics", "F=ma", "forces", ("resolve", "newton2"), True),
            ("mechanics_moments", "alg", "solve(2*R=3*5,R,method=linear)", "mechanics", "moments", "moments", ("pivot", "equilibrium"), True),
        ]
        topic, flag, expr, function, sig, method, transforms, expects = cases[i % len(cases)]
        difficulty = 5 if topic in ("pure_integration", "pure_trigonometry", "stats_hypothesis", "mechanics_kinematics") else 4
        concept = self._concept(function, sig, method, topic, transforms, difficulty, "syllabus+oracle+llm")
        return AdversarialCase(
            "Syllabus {0}: {1}".format(index, topic),
            flag,
            expr,
            concept,
            expects,
            "syllabus: Edexcel 9MA0 capability node; require full-mark route where working is expected",
            "syllabus:edexcel-9ma0:{0}".format(topic),
        )

    def _online(self, index: int) -> AdversarialCase:
        """Source-anchored hard cases for adversarial stress.

        Uses public source families as shape seeds only.  We store source URLs
        and method-mark markers, not copied mark schemes.
        """
        fam, expr = source_case_data(self.rng, index)
        concept = self._concept(fam.function, fam.signature, fam.method, fam.topic, fam.transforms, 5 + (index % 2), "source+sympy+llm")
        return AdversarialCase(
            "Online {0}: {1}".format(index, fam.topic),
            fam.flag,
            expr,
            concept,
            True,
            "source/markscheme: {0}; {1}; require full method steps, not only answer".format(fam.source, fam.marker_note),
            "source:{0} {1}".format(fam.source, fam.url),
        )

    def _crash(self, index: int) -> AdversarialCase:
        """Calculator safety edges: should finish compactly, not hang/crash."""
        i = max(0, index - 1)
        deep = "x"
        for _ in range(10 + (i % 8)):
            deep = f"sin({deep})"
        repeated = "+".join(["x^2-1"] * (18 + (i % 10)))
        cases = [
            ("long_parse_guard", "alg", f"factor(({repeated})/(x-1))", "algebra", "expr", "factor", ("long_parse", "division"), False),
            ("nested_rewrite_guard", "alg", f"rewrite({deep})", "rewrite", "expr", "auto", ("nested_fn", "depth_cap"), True),
            ("trig_many_roots", "trig", "sin(18*x)=sin(x),x,0,2*pi,40,method=identity", "solve_trig", "eq,var,lo,hi,max,method", "identity", ("many_roots", "interval_filter"), True),
            ("chain_depth_guard", "derive", f"{deep},x,method=chain", "diff", "expr,var,method", "chain", ("nested_chain", "depth_cap"), True),
            ("domain_singularity_guard", "alg", "domain(1/((x-1)*(x+1)*(x-2)))", "domain", "expr", "auto", ("singularities", "denominator_guard"), False),
            ("working_truncation_guard", "int", "x^5*e^(2*x),method=di", "integrate", "expr,method", "di", ("long_working", "line_cap"), True),
        ]
        topic, flag, expr, function, sig, method, transforms, expects = cases[i % len(cases)]
        concept = self._concept(function, sig, method, topic, transforms, 6, "timeout+oracle")
        return AdversarialCase(
            "Crash {0}: {1}".format(index, topic),
            flag,
            expr,
            concept,
            expects,
            "crash-safety: complete within timeout, bounded output, no parser/memory/debug failure",
            expr,
        )

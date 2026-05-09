from __future__ import annotations

from dataclasses import dataclass
import random


@dataclass(frozen=True)
class SourceFamily:
    topic: str
    flag: str
    function: str
    signature: str
    method: str
    source: str
    url: str
    transforms: tuple[str, ...]
    marker_note: str


SOURCE_FAMILIES: tuple[SourceFamily, ...] = (
    SourceFamily("source_nrich_symmetric_integral", "int", "integrate", "defint(expr,var,lo,hi)", "symmetry", "NRICH STEP integration", "https://nrich.maths.org/step-integration-questions", ("kings_property", "limit_transform", "combine"), "markscheme markers: define I, transform limits, add complementary form, solve for I"),
    SourceFamily("source_mit_looping_ibp", "int", "integrate", "expr,method", "parts", "MIT OCW 18.01SC", "https://ocw.mit.edu/courses/18-01sc-single-variable-calculus-fall-2010/download/", ("ibp_loop", "collect_I"), "markscheme markers: I/J, u,dv,du,v twice, substitute J, collect I"),
    SourceFamily("source_openstax_repeated_pf", "int", "integrate", "expr,method", "pf", "OpenStax Calculus 2 partial fractions", "https://openstax.org/books/calculus-volume-2/pages/3-4-partial-fractions", ("pf_repeated", "equate_coeffs", "integrate_terms"), "markscheme markers: assumed PF form, multiply through, coefficients, integrate each term"),
    SourceFamily("source_pauls_reference_triangle", "int", "integrate", "expr,method", "trig", "Paul's Online Notes trig substitution", "https://tutorial.math.lamar.edu/Classes/CalcII/TrigSubstitutions.aspx", ("reference_triangle", "substitution", "back_sub"), "markscheme markers: trig substitution, dx, reference triangle, back-substitution"),
    SourceFamily("source_edexcel_changed_limits", "int", "integrate", "defint(expr,var,lo,hi)", "sub", "Pearson Edexcel mark schemes", "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html", ("substitution", "changed_limits", "evaluate"), "markscheme markers: substitution, du, changed limits, primitive, evaluate upper-lower"),
    SourceFamily("source_pmt_disguised_substitution", "int", "integrate", "expr,method", "sub", "PMT Edexcel integration by substitution", "https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/integration/", ("hidden_substitution", "rewrite", "back_sub"), "markscheme markers: rewrite hidden factor, substitution, differential, transformed integral"),
    SourceFamily("source_edexcel_rform_interval", "trig", "solve_trig", "eq,var,lo,hi,max,method", "rform", "Pearson Edexcel trig equations", "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html", ("rform", "base_angle", "interval_filter"), "markscheme markers: R-alpha form, base angle, general solutions, interval filtering"),
    SourceFamily("source_edexcel_parametric_second", "derive", "diff", "x(t),y(t),t,x,method", "param_second", "Pearson Edexcel parametric calculus", "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html", ("dxdt_dydt", "dy_dx", "second_derivative"), "markscheme markers: dx/dt, dy/dt, dy/dx, d/dt(dy/dx), divide by dx/dt"),
    SourceFamily("source_edexcel_log_exp_equation", "alg", "solve", "eq,var,method", "log_exp", "Pearson Edexcel logs/exponentials", "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html", ("domain", "log_laws", "reject_invalid"), "markscheme markers: domain, log law/change of base, substitution, reject invalid roots"),
    SourceFamily("source_openstax_trig_powers", "int", "integrate", "expr,method", "trig", "OpenStax Calculus 2 trig integrals", "https://openstax.org/books/calculus-volume-2/pages/3-2-trigonometric-integrals", ("power_reduction", "product_to_sum", "integrate_terms"), "markscheme markers: identity used, expand/reduce, integrate term-by-term"),
    SourceFamily("source_edexcel_binomial_validity", "alg", "binomial", "expr,var,point,order", "expand", "Pearson Edexcel binomial expansion", "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html", ("binomial_series", "validity_range"), "markscheme markers: first terms, coefficients, validity range"),
    SourceFamily("source_edexcel_implicit_collect", "derive", "diff", "eq,var,method", "implicit", "Pearson Edexcel implicit differentiation", "https://qualifications.pearson.com/en/qualifications/edexcel-a-levels/mathematics-2017/useful-links.html", ("differentiate_both_sides", "collect_dy_dx", "isolate"), "markscheme markers: differentiate both sides, collect y' terms, factor/isolate"),
)


def _source_expr(fam: SourceFamily, rng: random.Random, i: int) -> str:
    a = 2 + (i % 5)
    b = 1 + ((2 * i) % 7)
    n = 2 + (i % 5)
    if fam.topic == "source_nrich_symmetric_integral":
        p = 3 + (i % 5)
        return f"defint(sin(x)^{p}/(sin(x)^{p}+cos(x)^{p}),x,0,pi/2)"
    if fam.topic == "source_mit_looping_ibp":
        return rng.choice([f"e^({a}*x)*cos({b}*x),method=parts", f"e^({a}*x)*sin({b}*x),method=parts", f"x^{n}*e^({a}*x),method=di"])
    if fam.topic == "source_openstax_repeated_pf":
        return rng.choice([f"({a}*x+{b})/((x-1)*(x^2+{a + b})),method=pf", f"({a}*x^2+{b}*x+{a+b})/((x-1)^2*(x^2+{a})),method=pf", "1/(x^2*(x+1)),method=pf"])
    if fam.topic == "source_pauls_reference_triangle":
        return rng.choice([f"sqrt({a*a}-x^2),method=trig", f"sqrt(x^2+{a*a}),method=trig", f"1/(x*sqrt(x^2-{a*a})),method=trig"])
    if fam.topic == "source_edexcel_changed_limits":
        return rng.choice([f"defint(e^({a}*x)/(1+e^({a}*x)),x,0,ln({b+3}))", "defint(e^(2*x)/(e^x+1),x,ln(2),ln(8))", f"defint({a}*x/sqrt({a}*x+{b}),x,0,{b+3})"])
    if fam.topic == "source_pmt_disguised_substitution":
        return rng.choice([f"x^{2*n-1}*exp(x^{2*n}),method=sub", f"x/(sqrt(1+x^2)*(1+sqrt(1+x^2))),method=sub", f"1/(x*sqrt(1+x^{n})),method=sub"])
    if fam.topic == "source_edexcel_rform_interval":
        return rng.choice([f"{a}*cos(x)+{b}*sin(x)=2,x,0,2*pi,12,method=rform", f"{a}*sin(x)-{b}*cos(x)=1,x,-pi,pi,12,method=rform"])
    if fam.topic == "source_edexcel_parametric_second":
        return rng.choice(["x=t^2+1/t,y=t^2-1/t,t,x,method=param_second", "x=e^t*cos(t),y=e^t*sin(t),t,x,method=param_second"])
    if fam.topic == "source_edexcel_log_exp_equation":
        return rng.choice(["solve(log(2,x)+log(x,2)=5/2,x,method=log_exp)", f"solve({a}^(2*x)-{a+b}*{a}^x+{a*b}=0,x,method=log_exp)", "solve(ln(x+1)-ln(x-1)=ln(3),x,method=log_exp)"])
    if fam.topic == "source_openstax_trig_powers":
        return rng.choice(["sin(x)^4,method=trig", "cos(x)^6,method=trig", "sin(x)^2*cos(x)^4,method=trig"])
    if fam.topic == "source_edexcel_binomial_validity":
        return rng.choice(["binomial((1+2*x)^(-1/2),x,0,4)", "binomial(sqrt(1-4*x),x,0,4)", "binomial((1+x)^5,x,0,5)"])
    if fam.topic == "source_edexcel_implicit_collect":
        return rng.choice(["x^y=y^x,x,method=implicit", "sin(x*y)+x^2=y^2,x,method=implicit", "1/(2*x+1)+1/(y+1)=x^2,x,method=implicit"])
    return "x^2"


def source_case_data(rng: random.Random, index: int) -> tuple[SourceFamily, str]:
    fam = SOURCE_FAMILIES[(index - 1) % len(SOURCE_FAMILIES)]
    return fam, _source_expr(fam, rng, index)

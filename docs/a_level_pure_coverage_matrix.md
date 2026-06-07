# A-Level Pure Coverage Matrix

Target: Edexcel A-level Pure topic surface. Internal methods may use AEA, Further Maths, or university algebra/CAS techniques when they help solve an in-topic question, but displayed work must prefer exam-style A-level steps and every displayed transformation must be verified where possible.

No claim: all maths, every equation, or Further Maths topic coverage.

## Done Gate

A row is implemented only when all are true:

- visible route is present in the kept menu/catalog/help when in scope
- rule family is general, not example-only
- generated property tests cover unseen forms
- exact queue passes for matching rows
- accepted working lines are quality-checked
- every algebraic/trig/equation step is equivalence-checked where possible
- no out-of-scope command leaks through menu, help, catalog, generator, examples, docs, or direct input

## Matrix

| Area | Kept route | General rules | Property families | Exact queue | Reject when |
|---|---|---|---|---|---|
| Expansion | `texpand`, `expand` only if upstream-visible | distribute products, powers of binomials, collect like monomials after expansion | random polynomial products and binomial powers | required | command renamed or hidden surface mismatch |
| Factorisation | `factor` | common factor, quadratic factor, difference of squares, factor theorem for integer/rational roots | generated expanded factors round-trip to factor form | required | non-Pure special factor domains only |
| Completing square | `rewrite`, `xform`, `solve`, `range` | quadratic `ax^2+bx+c -> a(x+h)^2+k`, vertex extraction | random quadratics with rational coefficients | required | matrix/conic transform command required |
| Rearranging formulae | `solve`, `xform` | isolate target through inverse operations, multiply/divide nonzero expressions with domain note | random linear/rational formula rearrangements | required | removed-feature command only |
| Indices/surds | `simplify`, `xform`, `exact` | exponent laws, surd simplification, nested power normalization | generated equivalent powers/surds | required | complex-only branch required |
| Log laws | `ln`, `log`, `xform`, `solve` | product, quotient, power, change of base, exp/log inverse | generated log identities with positive-domain assumptions | required | probability/stat log commands |
| Rationalising denominators | `simplify`, `xform` | multiply by conjugate for single/binomial surd denominator | generated surd denominator expressions | required | complex conjugate command surface if hidden |
| Rational simplification | `simplify`, `ratnormal`, `partfrac` if kept | common denominator, cancel common factors, domain notes | generated rational equivalents | required | matrix/list rational ops |
| Partial fractions | `partfrac` | linear repeated factors, irreducible quadratic if needed for Pure | generated proper rational functions | required | complex partial fractions if complex hidden |
| Binomial expansion | `series`, `taylor` or existing binomial route if kept | coefficient recurrence, validity interval | generated `(1+kx)^n` integer and rational n | required | probability `binomial` distribution aliases |
| Functions/composite/inverse | `solve`, `subst`, `xform`, `domain`, `range` | compose, invert monotone elementary functions, transform graph algebraically | generated affine/composite/inverse pairs | required | variable storage/program routes |
| Coordinate geometry algebra | `solve`, `range`, `xform` | lines, circles, intersections, tangents via algebra | generated line/circle systems | required | plotting/graphing commands |
| Trig reciprocal identities | `texpand`, `tcollect`, `xform`, `simplify` | `sec=1/cos`, `cosec=1/sin`, `cot=cos/sin` | generated reciprocal rewrites | required | hidden trig-to-plot routes |
| Trig Pythagorean identities | `texpand`, `tcollect`, `xform` | `sin^2+cos^2=1`, `1+tan^2=sec^2`, `1+cot^2=cosec^2` | generated identity mutations | required | no verified identity path |
| Trig double-angle | `texpand`, `tcollect`, `xform` | `sin(2x)`, `cos(2x)`, `tan(2x)` variants | generated double-angle target forms | required | no domain-safe rewrite |
| Trig product/sum | `texpand`, `tcollect`, `xform` | product-to-sum and sum-to-product when useful for Pure transformations | generated product/sum pairs | required | Further-only topic surface |
| Trig equations | `solve`, `xform` | rewrite to base trig, isolate, period/branch handling, exact and numeric angles | generated solvable trig equations | required | probability/stat angle distributions |
| Differentiation basics | `diff` | constant, power, exp, ln, trig, inverse trig where in Pure | generated derivative identities checked by CAS | required | removed-feature command only |
| Product rule | `diff` | split product factors, differentiate each side, combine | generated `u*v` products | required | no verified simplification path |
| Quotient rule | `diff` | numerator/denominator derivative, common denominator simplification | generated `u/v` quotients | required | denominator zero without note |
| Chain rule | `diff` | outer/inner derivative for powers, roots, exp, ln, trig | generated nested elementary functions | required | removed-feature command only |
| Implicit differentiation | `implicit_diff`, `diff` | differentiate both sides, collect `dy/dx`, solve | generated implicit polynomial/trig equations | required | removed-feature command only |
| Parametric differentiation | `diff` | `dy/dx=(dy/dt)/(dx/dt)`, second derivative via divide by `dx/dt` | generated parametric pairs | required | graph plotting requested |
| Integration recognition | `integrate`, `int` | reverse chain, elementary antiderivatives | generated derivatives integrated back | required | non-elementary with no verified route |
| Integration substitution | `integrate`, `int` | choose `u`, transform `du`, substitute back | generated reverse-chain/substitution families | required | invalid substitution domain unhandled |
| Integration by parts | `integrate`, `int` | `u dv = uv - int(v du)`, repeated when bounded | generated polynomial*exp/trig/log forms | required | non-terminating parts loop |
| Integration partial fractions | `integrate`, `int`, `partfrac` | decompose rational then integrate logs/arctan if in scope | generated rational forms | required | complex-only decomposition required |
| Definite integrals | `integrate`, `int`, `defint` if kept | antiderivative, endpoint substitution, exact simplification | generated definite integral identities | required | improper/out-of-scope convergence proof |
| A-level differential equations | `solve`, `integrate` | separable equations, exponential growth/decay | generated separable families | required | general ODE solving beyond A-level |
| Sequences/series | `sum`, `product`, `series`, `taylor` | arithmetic/geometric/sigma/binomial series within Pure | generated finite sums and geometric series | required | stats/probability series commands |
| Numerical methods | keep only if menu keeps route | sign change, iteration, Newton, trapezium with labelled approximations | generated numeric-method worksheets | required if kept | method removed from menu |
| Vectors | kept only for Edexcel-style vector arithmetic | component add/subtract, scalar multiply, dot product, magnitude/unit vectors | generated vector component identities | required if kept | reject cross product/matrix surface |
| Domain/range | `domain`, `range` | inequalities from denominators/roots/logs, quadratic vertex, monotone transforms | generated domain/range transformations | required | no verified interval reasoning |

## Route Search Contract

For kept commands, do not assume an entered question is out of A-level scope just because the first route fails.

Search order:

- apply specific exam-style route families
- apply KhiCAS-backed symbolic, transform, solve, differentiation, and integration routes
- verify transformations by `normal`, derivative checks, antiderivative checks, or substitution where possible
- fall through to `KhiCAS exact evaluation` for kept commands instead of emitting a route-failure message

Never emit vague filler or fake intermediate steps. Removed-feature commands remain rejected by the scope manifest.

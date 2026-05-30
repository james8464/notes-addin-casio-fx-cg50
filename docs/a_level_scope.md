# A-Level Scope

Source documents:

- `/Users/james/Downloads/Maths syllabus.pdf`
- `/Users/james/Downloads/math formula booklet.pdf`

Decision: support Edexcel A-level Pure + Mechanics only. All statistics and probability commands are removed.

## Keep

- Proof support where it is algebraic or calculus based.
- Algebra/functions: indices, surds, quadratics, simultaneous equations, inequalities, polynomial manipulation, factor theorem, rational expressions, functions, inverse/composite functions, transformations, partial fractions.
- Coordinate geometry: straight lines, circles, parametric conversion and differentiation support.
- Sequences/series: arithmetic, geometric, sigma, binomial expansion and validity. No public `nCr`/`comb` command.
- Trigonometry: sin/cos/tan/sec/cosec/cot, inverse trig, radians/degrees, identities, R-form, trig equations, small-angle approximations.
- Logs/exponentials: any base via `log(a,x)`, laws, change of base, exponential models.
- Differentiation: first principles, powers, exp/log/trig, product/quotient/chain, implicit, parametric, tangents/normals, stationary points.
- Integration: powers, exp/log/trig, definite integrals, area, substitution, parts, partial fractions, separable differential equations.
- Numerical methods: sign change, iteration, Newton-Raphson, trapezium rule.
- Vectors: 2D/3D component work, magnitude, unit vectors, position vectors, geometry.
- Mechanics: SUVAT, forces/Newton laws, friction, connected particles, moments.

## Remove

- All statistics/probability: distributions, CDFs, tests, regression, sampling, summary stats, random numbers.
- Complex-number support and complex solve/factor/partial fractions.
- Matrix/list algebra beyond parser internals required by GIAC.
- Plotting, graphs, turtle, pixels, drawing.
- Programming/control flow/Python/custom scripts.
- Session save/load.
- Crypto, Fourier/Laplace, ODE plot fields, special polynomials, resultants, interpolation, EGCD, Jordan, Gram-Schmidt.
- Hyperbolic and inverse hyperbolic functions.

## Formula Pack

Formula and help text belongs in `CASIOCAS.PAK`, not embedded `.g3a` source strings.

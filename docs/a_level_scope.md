# A-Level Scope

Source documents:

- `/Users/james/Downloads/Maths syllabus.pdf`
- `/Users/james/Downloads/math formula booklet.pdf`

Decision: support Edexcel A-level Pure plus Paper 3 mechanics in `CAS.g3a`.
Statistics and probability remain out of scope.

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
- Paper 3 mechanics: SUVAT as the main supported route; compact support for force, weight, friction, connected particles, pulleys, moments, resolving, variable acceleration, work/power/energy/impulse/momentum where already present.

## Remove

- All statistics/probability: distributions, CDFs, tests, regression, sampling, summary stats, random numbers.
- Complex-number support and complex solve/factor/partial fractions.
- Matrix/list algebra beyond parser internals required by GIAC.
- Plotting, graphs, turtle, pixels, drawing.
- Programming/control flow/Python/custom scripts.
- Session save/load.
- Crypto, Fourier/Laplace, ODE plot fields, special polynomials, resultants, interpolation, EGCD, Jordan, Gram-Schmidt.
- Hyperbolic and inverse hyperbolic functions.
- Question-specific mechanics shortcuts such as projectile and incline templates; use SUVAT, resolve, and the core force/energy commands instead.

## Formula Pack

`khicasen.g3a` stays pristine upstream/source output. `CAS.g3a` may carry scoped A-level working routes and uses `CAS.PAK` for command help.

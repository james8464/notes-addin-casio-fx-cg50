# CasioCAS Tutorial

Goal: short exam-style maths lines for fx-CG50. Use exact syntax; use `*` if in doubt.

Build note: the shipped `.g3a` comes from `c++/khicas/upstream/giac90_1addin/`. Host-only changes in `c++/addin/src/modules/` are not calculator changes until ported there.

## Core Inputs

| Task | Function | Parameters | Notes |
|---|---|---|---|
| solve equation | `solve(eq,x,[method])` | equation, variable, optional method | Algebra, logs, exp, radicals, trig intervals. Use `x=0..2*pi` for bounded trig. |
| force solve route | `solve_by(eq,x,method)` | equation, variable, method | Methods: `linear`, `factor`, `quad`, `complete_square`, `sub`, `clear_denoms`, `log_exp`, `identity`, `rform`, `cast`. |
| compare/prove | `compare(lhs,rhs)` | two expressions | Shows equivalence or contradiction route where supported. |
| rewrite | `rewrite(expr,[target])` | expression, optional target | Surds, trig identities, useful forms. |
| complete square | `complete_square(expr,[x])` | quadratic, variable | Shows square form. |
| domain | `domain(expr,[x,lo,hi])` | expression, variable, optional interval | Logs, roots, fractions, trig restrictions. |
| range | `range(expr,[x,lo,hi])` | expression, variable, optional interval | A-level forms; interval final shown. |
| derivative | `diff(f,x,[n,method])` | expression, variable, optional order/method | Methods below. |
| forced derivative | `diff_by(f,x,method)` | expression, variable, method | Same routes as `diff`. |
| integral | `integrate(f,x,[a,b,method,u])` | expression, variable, optional bounds/method/substitution | Methods below. |
| forced integral | `integrate_by(f,x,method,[u])` | expression, variable, method, optional `u` | Use when exam asks a route. |
| differential equation | `de_solve(eq,[bc])` | first-order DE, optional boundary | Separable and linear first-order forms. |
| trig solve | `solve(eq,x=lo..hi,method=cast/rform)` | equation, bounded variable | Prefer `solve`; `solve_trig_by` is compatibility. |
| stats binomial | `binomial(n,p,k)` | trials, probability, value | Gives support check and `P(X=k)`. |
| stats normal | `normalcdf(mu,sigma,lo,hi)` | mean, sd, lower, upper | Shows standardisation. |
| SUVAT | `suvat(s=...,u=...,v=...,a=...,t=...,target=...)` | known values plus target | Solves missing kinematics variable. |

## Formula Coverage

The program should handle A-level formula families as working routes or subparts:

| Area | Covered route |
|---|---|
| indices/surds | simplify/rewrite, rationalise-style surd transforms |
| quadratics | factor, formula, discriminant, complete square, roots |
| coordinate geometry | line/circle algebra, midpoint/distance as subparts |
| functions | inverse, composite, modulus, domain/range |
| sequences | arithmetic/geometric formulae via algebra; binomial expansion with validity |
| trig | identities, exact values, bounded solves, R-form, sine/cosine rule subparts |
| logs/exp | log laws, change of base, growth/decay, DE constants |
| differentiation | chain/product/quotient/implicit/parametric/logdiff/stationary routes |
| integration | direct, substitution, parts, trig identities, partial fractions, definite integrals |
| statistics | binomial and normal probabilities |
| mechanics | SUVAT plus algebraic force/moment/energy/power subparts |

Gate: `python3 c++/tools/golden/check_syllabus_matrix.py --fail-on-gap --report-json`

## Derivative Methods

| Method | Use |
|---|---|
| `auto` | choose route |
| `chain` | composite functions |
| `product` | products |
| `quotient` | fractions |
| `logdiff` | powers like `sin(x)^x` |
| `implicit` | equation in `x,y`, e.g. `diff(x^2+y^2=1,x,method=implicit)` |
| `param` | `diff(x=t^2,y=t^3,t,method=param)` |
| `param_second` | `diff(x=t^2+1/t,y=t^2-1/t,t,method=param_second)` |
| `second` | ordinary second derivative |
| `first_principles` | supported standard first-principles proofs |

## Integral Methods

| Method | Use |
|---|---|
| `auto` | choose route |
| `direct` | standard reverse derivative |
| `reverse_chain` | `f'(x)g(f(x))` |
| `sub` | substitution |
| `parts` | integration by parts |
| `trig` | trig identities/powers |
| `pf` | partial fractions |
| `div` | algebraic division first |

## Differential Equations

Use `de_solve` in the integration workflow:

```text
de_solve(dh/dt=(1/50)*h*(2*h-1)*cos(t/10),h(0)=5/2)
```

Expected flow:

```text
dh/dt = ...
Separate variables: ...
Int(...) dh = Int(...) dt
...
h = ...
```

Supported:
- separable first-order: `dy/dx=f(x)*g(y)`
- linear first-order: `dy/dx+P(x)y=Q(x)`
- boundary values like `y(0)=1`
- variables other than `x,y`, e.g. `h,t`

Unsupported:
- higher-order DEs unless a route is explicitly implemented
- non-elementary answers may return compact honest output

## Help On Calculator

Use catalogue help for full command list. `F2`/`F3` examples insert the shown example, not a generic template. Keep `CASIOCAS.PAK` beside `CasioCAS.g3a`; verbose help lives there.

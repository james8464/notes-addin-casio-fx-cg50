# Maths Program Improvement List

## CAS.g3a

1. Add a compact `method()` wrapper.
   Example: `method(integrate(x*exp(2*x),x))` could force a full method trace without changing the existing command behaviour.

2. Add `paramarea()` / `paramdiff()` wrappers for parametric A-level questions.
   Useful for questions asking for Cartesian coordinates, `dy/dx`, area under a parametric curve, and bounds conversion.

3. Add a more explicit `showthat()` alias for `xform()`.
   This would reduce user confusion for exam prompts that say "show that ... can be written as ...".

4. Add a small `checkdomain()` line to every solve/range route.
   This would make excluded roots and log/square-root restrictions more visible in mark-scheme style.

5. Add optional `degrees` marker for trig solve routes.
   Example: `solve(sin(x)=0.5,x),degrees`.

## CASP3.g3a

Implemented:

- `normalcrit()` alias for inverse normal critical-value work.
- `binomcrit()` alias for binomial critical regions.
- `samplemean(mu,sigma,n,lo,hi)` and `samplemeantail(mu,sigma,n,x,tail)`.
- `largebinomnormal()` alias for normal approximation to binomial.
- `connected()` connected-particle route.
- `ladder()` moments/friction route.

Remaining useful additions:

1. Add `projectilefromheight()` alias.
   Current projectile commands support initial height in parameter forms, but a named alias would help users pick it quickly.

2. Add `vectorforces()` command.
   Useful for resolving forces in `i,j` form and finding acceleration/resultant.

## CSCALC.g3a

1. Add Karnaugh map simplification for 2, 3, and 4 variables.
   Useful if AQA asks Boolean simplification visually.

2. Add trace-table helper.
   Example: `trace(init_i=0,limit=5,step=1)` is probably too broad, but a simple counter-loop template could cover common paper questions.

3. Add `astar()` or `mst()` only if future AQA papers use those algorithms.
   Current `dijkstra()` is the most useful weighted-graph command.

4. Add `normalfloatsteps()` command.
   It would show every shift needed to normalise a mantissa/exponent pair, useful for explanation marks.

5. Add `overflowcheck()` command.
   It should compare unsigned, sign-magnitude, one's complement, and two's complement range for the same bit width.

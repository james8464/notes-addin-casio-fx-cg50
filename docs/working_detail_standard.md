# Working Detail Standard

Minimum acceptable working for visible commands. A final answer alone is not enough when these lines can be inferred.

| Command | Required working |
| --- | --- |
| `diff` | Name product/quotient/chain rule; define `u`, `v`, or inner function; show derivative components; show simplified result; verify. |
| `integrate` | Name method: substitution, parts, partial fractions, trig identity, or complete square; show substitution/constants; show antiderivative; verify by differentiating when route is not obvious. |
| `defint` | Show antiderivative `F(x)`; show `F(b)-F(a)`; for infinite or improper bounds show a limit variable; evaluate the limit fully. |
| `solve` | Show equation rearrangement or exact solve route; show domain/interval constraints; reject invalid roots; verify returned roots by substitution. |
| `range` | Show constraints; split domain when needed; show endpoints, critical points, and one-sided limits; return interval with open/closed endpoints; verify under the same constraints. |
| `domain` | Show every natural-domain condition: denominators non-zero, log arguments positive, even roots non-negative, trig restrictions; simplify to a readable set when available. |
| `xform` | Show successful rewrite steps; use `texpand`/`tcollect`, denominator clearing, trig/log/surd identities as needed; verify `normal(start-target)=0` or equivalent. |
| `taylor` | Show base series; show substitution for composite expressions; state truncation order; show final polynomial. |
| `coeff` | Show expansion or series source; show `[x^n]`; show final coefficient; verify. |
| `partfrac` | Show denominator factorisation, partial-fraction form, solved constants or exact decomposition, then final form; verify. |
| complex roots via `solve` | Show modulus and argument; show nth-root formula; list root family or exact roots; verify. |

Forbidden fallback:
- `KhiCAS exact` with no route.
- `Verified` with no working.
- Placeholder roots such as `roots(F(z))`.
- Domain or interval ignored while still marked verified.

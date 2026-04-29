# Differentiation Program (`deriveProgram.py`)

Symbolic differentiation program with normal, implicit, parametric, and explicit second-derivative menu modes.

## Main Menu (`M`)

- `1 n` - normal derivative
- `2 imp` - implicit differentiation
- `3 par` - parametric differentiation
- `4 2nd` - first + second derivative workflow

## Shared Syntax

- operators: `+ - * / ^ **`
- implicit multiplication supported
- functions: trig, inverse trig, `ln/log`, `sqrt`, `abs`, exponentials
- constants: `pi`, `e`
- compact forms like `sin x`, `sin^2 x`, `e^x` are accepted

## Mode 1: Normal (`n`)

Prompt:

- `y:`

Accepted normal inputs include:

- plain expression
- expression with explicit variable (`expr,x`)
- equation wrappers like `y=expr` or `EQ1=expr`
- second-derivative textual forms can also be recognized in this path (`d2/dx2 ...`, `d^2/dx^2 ...`)
- tangent request forms:
  - `expr,point`
  - `expr,var,point`

When tangent request is used, output includes:

- derivative at point
- gradient
- tangent-line equation

## Mode 2: Implicit (`imp`)

Prompt:

- `Eq:`

Requirements:

- must contain `left=right`
- variable pair is inferred (independent + dependent)
- rearranges to isolate `d(dep)/d(indep)`

Output includes derivative equation steps and final `Answer: d?/d? = ...`.

## Mode 3: Parametric (`par`)

Prompts:

- `x(t):`
- `y(t):`

Behavior:

- computes `dx/dt`, `dy/dt`, then `dy/dx=(dy/dt)/(dx/dt)`
- reports undefined vertical tangent case when `dx/dt = 0`
- also attempts a cartesian relation summary for supported parametric families

## Mode 4: Second Derivative (`2nd`)

Prompt:

- `y:`

Behavior:

- runs normal-derivative flow first (prints compact first-derivative working)
- then differentiates that result again
- prints final second derivative as `Answer: d2y/dx2 = ...` (or equivalent label for non-`x` variable)

## Covered Rule Families

- constants, sums, products, quotients, chain rule
- trig + inverse trig base derivatives
- log/exponential rules
- variable exponents (`x^x`, `x^n`, `x^(f(x))`, `(f(x))^x`)

## Error Handling

- invalid/missing input -> `Err: ...`
- implicit mode without `=` -> rejected
- parser/unsupported structures fail explicitly (no silent guessing)

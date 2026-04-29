# Trigonometry Program (`trigProgram.py`)

Trig manipulation/solve tool with compact calculator-oriented output.

## Main Menu (`M`)

- `1 prove`
- `2 xform`
- `3 solve`
- `4 rw`

## Mode 1: Prove (`prove`)

Prompts:

- `E1:`
- `E2:`
- `R:` route menu
  - `1 auto`
  - `2 lhs`
  - `3 rhs`
  - `4 both`

Important behavior:

- if both entries are plain expressions (no `=`), mode treats them as identity sides
- if either contains `=`, mode compares equation forms (residual form)
- non-identities are rejected with explicit error text
- domain-restricted identities are guarded

## Mode 2: Transform (`xform`)

Prompts:

- `E1:` source
- `E2:` target

Behavior:

- supports expression->expression and equation->equation transforms
- includes direct named routes (reciprocal, pythagorean, angle identities, common denominator routes, half-angle ratio forms)
- ends with explicit `Answer: ...` or mismatch message

## Mode 3: Solve (`solve`)

Prompt:

- `Eq:`

This mode handles both equation solving and extrema inputs.

### Solve input styles

- equation only (no explicit interval)
- `equation,var`
- `equation,var,start,end`
- `equation,var,a<x<b` (also `<=`)
- max/min style prompts (for example comma-delimited kind + variable)

### Interval and angle behavior

- if no interval is given, solver uses a symmetric default window and prints a trimmed sample (nearest roots around `0`: up to 5 below and 5 above)
- interval relation form (`a<x<b`) is parsed explicitly
- degree/radian style is auto-inferred and normalized in numeric solve paths
- mixed angle inputs are normalized to one unit internally

### Solve engine coverage

- direct single trig equations (`sin`, `cos`, `tan`, `sec`, `cosec`, `cot`)
- polynomial-in-trig substitutions (e.g. quadratic in `sin(x)`)
- algebraic rewrite + solve routes when direct solve is not possible
- numeric fallback for harder mixed forms

## Mode 4: Rewrite (`rw`)

Prompts:

- `Rw:`
- repeated `T:` lines (`blank` ends list; first prompt includes helper text)

Behavior:

- rewrites expression/equation using only supplied target terms when supported
- includes direct ratio rewrites such as `sin/cos -> tan` where target terms permit it
- returns explicit refusal if requested term set cannot express source

## Parser and Syntax Coverage

- operators: `+ - * / ^ **`
- compact notation: `sin x`, `sin^2 x`, etc.
- trig: `sin cos tan sec cosec cot`
- inverse trig aliases accepted
- constants: `pi`, `e`
- strict token/nesting guards exist for calculator stability

## Notes

- output lines are auto-shortened for small displays
- repeated/noisy working lines are compacted before printing
- CLI errors use `Err: ...`

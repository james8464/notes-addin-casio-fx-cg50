# Integration Program (`intProgram.py`)

Symbolic integration + first-order DE solver + parametric area helper for CASIO fx-cg50 (MicroPython 1.9.4 compatible).

## Main Menu

`M` prompt:

- `1 int` - Integral mode
- `2 de` - Differential equation mode
- `3 param area` - Parametric area mode

## Mode 1: Integral (`int`)

Prompt sequence:

- `f:` integral request
- `Met:` method selection
- optional `u:` only when `Met = 4` (substitution), to force a specific `u`

### `f:` accepted input forms

- `expr` -> indefinite integral with inferred variable
- `expr,var` -> indefinite integral with explicit variable
- `expr,var,lower,upper` -> definite integral
- `expr,lower,upper` -> definite integral with inferred variable
- equation-style cartesian forms are accepted when they can be rearranged to `y=f(x)`:
  - `y=...`
  - `EQ1=...`
  - `expr=expr` (if linear in `y`)

### `Met:` method menu

- `1 auto` - route chooser (direct, reverse-chain, trig rewrite, substitution, parts/cyclic parts, partial fractions, division)
- `2 dir` - direct/basic pattern integration
- `3 trig` - trig identity/reduction routes
- `4 sub` - substitution route (`u:` optional)
- `5 pts` - integration by parts/cyclic parts
- `6 pf` - partial fractions
- `7 div` - polynomial/rational division

### `u:` (forced substitution) accepted forms

- `u=x^3+x+7`
- `x^3+x+7`
- `u^2=...` / `u**2=...` (treated as `u=sqrt(...)`)

## Mode 2: Differential Equations (`de`)

Prompt sequence:

- `dy/dx:` DE input
- `BC (e.g. y(1)=2 or x=1,y=2):` optional boundary condition

### `dy/dx:` accepted forms

- `dy/dx=...`
- `dy/dx: ...`
- full symbolic derivative variables such as `dY/dX=...`
- plain RHS expression (defaults to dependent `y`, independent `x`)

### What `BC (e.g. y(1)=2 or x=1,y=2):` means

This field is optional. Leave blank for a general `+C` answer.

Accepted BC formats:

- point form: `y(1)=2`
- comma form: `x=1,y=2` or `y=2,x=1`
- `"at"` also works: `y=2 at x=1`

Rules:

- it must include both independent and dependent values
- variable names must match the DE variables (`x`/`y`, or whatever was parsed)
- invalid BC formats are rejected

## Mode 3: Parametric Area (`param area`)

Prompt sequence:

- `x(t):`
- `y(t):`

Computes:

- `dx/dt`
- integrand `y * (dx/dt)`
- integrates with respect to `t`

Output starts with:

- `dx/dt = ...`
- `Use Int[y dx] = Int[y*(dx/dt)] dt`

## Parser and Syntax Coverage

- operators: `+ - * / ^ **`
- implicit multiplication supported (`2x`, `sin x`, `x(x+1)`)
- constants: `pi`, `e`
- inverse trig aliases accepted (`arcsin`, `arccos`, `arctan`)
- compact trig/log forms accepted
- expression variable is auto-detected when omitted

## Practical Notes

- output is intentionally compact for calculator screen width
- definite integrals print `F(upper)-F(lower)` working when relevant
- unsupported/non-elementary families return explicit failure text
- CLI failures use `Err: ...`

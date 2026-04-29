# Algebra Program (`algebraProgram.py`)

Algebra utility program for CASIO fx-cg50 with compact exam-style working lines.

## Main Menu (`M`)

- `1 cmp` - compare two expressions
- `2 xform` - transform expression/equation to target form
- `3 exp` - expand expression
- `4 poly` - factor/test polynomial expression
- `5 comp sq` - complete the square
- `6 solve` - solve equation (optionally with interval)
- `7 comp` - composite function `f(g(x))`
- `8 inv` - inverse function
- `9 rw` - rewrite using target terms
- `10 dom/rng` - domain + range (optional interval)
- `11 cart` - parametric to cartesian

## Shared Syntax

- operators: `+ - * / ^ **`
- implicit multiplication supported (`2x`, `(x+1)(x-1)`)
- trig/log/sqrt functions supported by parser
- constants: `pi`, `e`
- aliases: `ln`, `csc`, `arcsin`, `arccos`, `arctan`

## Mode-by-Mode Prompts and Behavior

### 1) Compare (`cmp`)

Prompts:

- `E1:`
- `E2:`

Behavior:

- simplifies both
- prints step list
- final line is `Result: Equivalent.` or `Result: Not equivalent.`

### 2) Transform (`xform`)

Prompts:

- `E1:` source
- `E2:` target

Behavior:

- supports expression transforms and equation transforms
- validates equivalence before accepting final target
- ends with `Answer: ...` or explicit mismatch message

### 3) Expand (`exp`)

Prompts:

- `Expr:`
- `Max:` optional term cap (integer); blank = default

Behavior:

- expands supported powers/binomial-like forms
- handles some negative-power expansions in series-like form

### 4) Polynomial (`poly`)

Prompt:

- `Poly:`

Behavior (important): this mode is factor-oriented in current codebase.

- tries symbolic factorization
- if not factorable, may show transformed/expanded fallback as `Out = ...`

### 5) Complete Square (`comp sq`)

Prompt:

- `Expression:`

Behavior:

- rewrites quadratic into completed-square form when supported

### 6) Solve (`solve`)

Prompt:

- `Eq (or eq, var, low, high):`

Accepted solve forms:

- `eq`
- `eq,var`
- `eq,low,high`
- `eq,var,low,high`

Notes:

- plain `x^2-4` is treated as `x^2-4=0`
- if only one bound is supplied, input is rejected
- interval bounds are parsed as expressions

### 7) Composite (`comp`)

Prompts:

- `f:`
- `g:`

Behavior:

- computes and prints `f(g(x))`
- blank `f` triggers built-in demo defaults

### 8) Inverse (`inv`)

Prompt:

- `f:`

Behavior:

- solves for inverse when valid on real domain
- reports non-invertible families (constant/even-power/all-real failures)

### 9) Rewrite (`rw`)

Prompts:

- `Rw:`
- `T:` repeated (first prompt includes helper text), blank line ends target list

Behavior:

- single target term: rewrites in that term
- multiple target terms: builds combined target form and transforms toward it

### 10) Domain/Range (`dom/rng`)

Prompt:

- `Expr (or expr, var, low, high):`

Accepted forms:

- `expr`
- `expr,var`
- `expr,low,high`
- `expr,var,low,high`

Behavior:

- prints domain section first, then range section
- supports interval-restricted analysis when bounds provided

### 11) Cartesian (`cart`)

Prompts:

- `x(t):`
- `y(t):`
- `Param:` (blank defaults to `t`)

Behavior:

- eliminates parameter when supported
- prints resulting cartesian relation

## Error/Validation Rules

- bad mode -> `Bad mode.`
- parse/validation failures -> `Err: ...`
- unsupported solve families return explicit no-solution/no-closed-form messages

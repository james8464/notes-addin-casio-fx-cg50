# Boolean Algebra Program (`booleanProgram.py`)

Boolean simplifier/converter/prover with compact calculator output.

## Main Menu (`Mode`)

- `1 simplify`
- `2 nand form`
- `3 nor form`
- `4 prove`

Menu is paged for small screen; `n`/`p` moves pages when needed.

## Syntax (exactly as parser expects)

- constants: `0`, `1`
- variables: letters/identifiers (multi-letter words are expanded as chained terms in parser logic)
- operators:
  - `,` = NOT (postfix), e.g. `A,`
  - `.` = AND
  - `+` = OR
- parentheses `(` `)` supported

Examples:

- `(A.B),`
- `A+B.C`
- `A,,`

## Mode Details

### 1) Simplify

Prompt:

- `Expression:`

Behavior:

- prints starting form as step `1`
- repeatedly applies one simplification law per step (up to 50 printed steps)
- final line: `Result: ...`

If expression is blank, built-in demo is used:

- `((B,.A),.B,),+A.B`

### 2) NAND form

Prompt:

- `Expression:`

Behavior:

- prints original expression
- prints `NAND form: ...`

Blank input demo defaults to `A.B`.

### 3) NOR form

Prompt:

- `Expression:`

Behavior:

- prints original expression
- prints `NOR form: ...`

Blank input demo defaults to `A+B`.

### 4) Prove

Prompts:

- `LHS:`
- `RHS:`

Behavior:

- simplifies LHS and RHS separately
- prints both step trails
- reports final common simplified form if equivalent

Blank defaults:

- `LHS = A.(B+C)`
- `RHS = A.B+A.C`

## Core Laws/Transforms Implemented

- common-sense constant laws
- tautology/complement checks
- double complement
- De Morgan
- absorption
- distributive transforms (including expansion checks)
- consensus theorem route

## Error Handling

- parse/input issues -> `Input error: ...`
- internal fallback -> `Input error: internal error.`
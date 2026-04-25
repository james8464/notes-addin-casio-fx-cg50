# Algebra Calculator

General-purpose algebra solver with 11 modes.

## Menu Modes

### 1. Compare (Mode 1)
Check if two expressions are equivalent.

**Input:**
- E1: First expression
- E2: Second expression

**Output:** Shows steps and whether equivalent.

**Examples:**
```
E1: (x+1)^2        E2: x^2 + 2x + 1       → Equivalent
E1: (x+1)(x-1)    E2: x^2 - 1            → Equivalent
E1: x^2 + 1        E2: (x+1)^2            → Not equivalent
```

### 2. Transform (Mode 2)
Show step-by-step transformation between forms.

**Input:**
- E1: Starting expression
- E2: Target expression

**Examples:**
```
E1: (x+1)^2        E2: x^2 + 2x + 1       → Expand
E1: x^2 - 1        E2: (x+1)(x-1)         → Factor
```

### 3. Expand (Mode 3)
Expand algebraic expressions.

**Input:**
- Expr: Expression to expand
- Max: (optional) Maximum terms in result

**Examples:**
```
Expr: (x+1)^2               → x^2 + 2x + 1
Expr: (x+1)(x-1)           → x^2 - 1
Expr: (x+1)^3              → x^3 + 3x^2 + 3x + 1
Expr: (2x+3)^2             → 4x^2 + 12x + 9
```

### 4. Polynomial (Mode 4)
Factor polynomials, find roots.

**Input:**
- Poly: Polynomial to factor

**Examples:**
```
Poly: x^2 - 1            → (x+1)(x-1)
Poly: x^2 - 5x + 6     → (x-2)(x-3)
Poly: x^3 - 1           → (x-1)(x^2 + x + 1)
```

### 5. Complete Square (Mode 5)
Complete the square for quadratic expressions.

**Input:**
- Expression: Quadratic expression

**Examples:**
```
Expression: x^2 + 4x           → (x+2)^2 - 4
Expression: x^2 + 4x + 1        → (x+2)^2 - 3
Expression: 2x^2 + 8x + 1     → 2(x+2)^2 - 7
```

### 6. Solve (Mode 6)
Solve algebraic equations.

**Input:**
- Eq: Equation with "="

**Examples:**
```
Eq: x^2 - 4 = 0              → x = 2 or x = -2
Eq: x^2 + 5x + 6 = 0       → x = -2 or x = -3
Eq: x + 3 = 0               → x = -3
Eq: ax + b = 0               → x = -b/a
```

### 7. Compose (Mode 7)
Find f(g(x)).

**Input:**
- f: Outer function (default: 2*x+1)
- g: Inner function (default: x^2)

**Examples:**
```
f: 2*x+1        g: x^2        → 2x^2 + 1
f: x^2          g: x+1        → (x+1)^2
f: 1/x         g: x-1        → 1/(x-1)
```

### 8. Inverse (Mode 8)
Find inverse function f⁻¹(x).

**Input:**
- f: Function (default: 2*x+1)

**Examples:**
```
f: 2*x+1                   → (x-1)/2
f: x^2 (x > 0)           → sqrt(x)
f: 1/x                     → 1/x
f: x+3                     → x-3
```

### 9. Rewrite (Mode 9)
Rewrite expression using specific terms.

**Input:**
- Rw: Expression
- T: (repeated) Terms to apply

**Common terms:**
- `expand` - expand products
- `factor` - factor expressions
- `collect` - collect like terms
- `simplify` - simplify fully
- `radsq` - rationalize square root

### 10. Domain/Range (Mode 10)
Find domain and range of function.

**Input:**
- Expr: Function expression

**Examples:**
```
Expr: 1/x                  → Domain: x ≠ 0
Expr: sqrt(x)              → Domain: x ≥ 0
Expr: x^2                 → Range: y ≥ 0
Expr: 1/(x-1)            → Domain: x ≠ 1
```

### 11. Cartesian (Mode 11)
Convert parametric equations to Cartesian form.

**Input:**
- x(t): x in terms of parameter
- y(t): y in terms of parameter
- Param: Parameter name (default: t)

**Examples:**
```
x(t): t        y(t): t^2       Param: t    → y = x^2
x(t): cos(t)    y(t): sin(t)    Param: t    → x^2 + y^2 = 1
x(t): 2t+1    y(t): 3t-2    Param: t    → Eliminate t: t = (x-1)/2, y = 3(x-1)/2 - 2
```

## Input Syntax

### Numbers
- Integers: `10`, `-5`
- Decimals: `9.8`, `0.5`
- Fractions: `1/2`, `3/4`
- Scientific: `6e3`, `1.5e-2`

### Variables
- Single: `x`, `y`, `z`, `t`, `n`
- Multi-letter: `ax`, `by`, `param`

### Operators
- `+`, `-` : addition, subtraction
- `*` or space : multiplication (`2*x` or `2x`)
- `/` : division
- `^` or `**` : power (`x^2` or `x**2`)
- `()` : grouping

### Special Functions
- `sqrt(x)` - square root
- `abs(x)` - absolute value
- `exp(x)` - e^x
- `ln(x)`, `log(x)` - natural log
- `sign(x)` - sign function

### Constants
- `e` - Euler's number
- `pi` - π
- `I` - imaginary unit (for complex)

### Comparison Operators in Equations
- `=` - equals
- `>` - greater than
- `<` - less than
- `>=` - greater or equal
- `<=` - less or equal

### Implicit Multiplication
- `2x` = `2*x`
- `3(4)` = `3*4`
- `xy` = `x*y` (two variables together)
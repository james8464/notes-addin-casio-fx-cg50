# Algebra Program (algebraProgram.py)

A general algebraic manipulation program for the CASIO fx-cg50 calculator (runs MicroPython v1.9.4).

## Features

| Mode | Feature | Description |
|------|---------|-------------|
| 1 | Compare | Compare two expressions to determine if they're equivalent, with step-by-step working |
| 2 | Transform | Transform expressions to match a target form (with unknown constants A, B, etc.) |
| 3 | Expand | Binomial expansion with optional term limit |
| 4 | Polynomial Ops | Add, subtract, or multiply polynomials |
| 5 | Complete Square | Complete the square for quadratic expressions |
| 6 | Solve | Solve linear and quadratic equations |
| 7 | Simultaneous | Solve simultaneous linear equations |
| 8 | Composite | Find composite functions f(g(x)) |
| 9 | Inverse | Find inverse functions f⁻¹(x) |

## Input Syntax

- **Variables**: Single letters (x, y, z, etc.)
- **Numbers**: Integers, decimals, or fractions (e.g., `3/4`, `2.5`)
- **Constants**: `e`, `pi`
- **Operators**: `+`, `-`, `*`, `/`, `^`, `**`
- **Functions**: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`, `exp`, `log`, `ln`, `sqrt`, `abs`, `asin`, `acos`, `atan`
- **Aliases**: `arcsin`, `arccos`, `arctan`, `csc`
- **Grouping**: Parentheses `()`
- **Compact forms**: `sin x`, `sin^2(x)`, `cos^3 x`, `ln x`, `(x+1)(x+2)`, `2x+3x`

## Coverage Notes

- Parser accepts both `^` and `**`
- Implicit multiplication is supported in many calculator-style forms
- Compact trig/log forms are supported without extra imports
- CLI messages now use short labels for the calculator screen

## Error Handling

- Invalid input shows `Err: ...`
- Division by zero is caught and reported
- Malformed expressions are detected during parsing

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Keeps runtime lightweight: no new imports, bounded caches, simple parser logic
- Limited binomial expansion is still used for performance on large inputs
- Some non-polynomial transforms may still be out of scope for exact matching

## Mode 1: Compare Expressions

Compares two algebraic expressions to determine equivalence with step-by-step simplification.

## Mode 1: Compare Expressions

Compares two algebraic expressions to determine equivalence with step-by-step simplification.

### Example 1: Simple Equivalence

```
Mode: 1
Expr1: 2*x+3*x
Expr2: 5*x

Output:
1. expr1 = 2*x+3*x
2. expr2 = 5*x
3. Simplify both...
4. expr1 = 5*x
5. expr2 = 5*x
6. Simplify expr1: 5*x
7. Simplify expr2: 5*x
8. EXPRESSIONS ARE EQUIVALENT
9. RESULT: Equivalent
```

### Example 2: Binomial Equivalence

```
Mode: 1
Expr1: (x+1)^2
Expr2: x^2+2*x+1

Output:
1. expr1 = (x+1)^2
2. expr2 = x^2+2*x+1
3. Simplify both...
4. expr1 = (x+1)^2
5. expr2 = x^2+2*x+1
6. Simplify expr1: x^2+2*x+1
7. Simplify expr2: x^2+2*x+1
8. EXPRESSIONS ARE EQUIVALENT
9. RESULT: Equivalent
```

### Example 3: Logarithm Rules

```
Mode: 1
Expr1: log(x*y)
Expr2: log(x)+log(y)

Output:
1. expr1 = ln(x*y)
2. expr2 = ln(x)+ln(y)
3. Simplify both...
4. expr1 = ln(x*y)
5. expr2 = ln(x)+ln(y)
6. Simplify expr1: ln(x)+ln(y)
7. Simplify expr2: ln(x)+ln(y)
8. EXPRESSIONS ARE EQUIVALENT
9. RESULT: Equivalent
```

### Example 4: Non-Equivalent Expressions

```
Mode: 1
Expr1: x^2+2*x+1
Expr2: (x+1)^3

Output:
1. expr1 = x^2+2*x+1
2. expr2 = (x+1)^3
3. Simplify both...
4. expr1 = x^2+2*x+1
5. expr2 = (x+1)^3
6. Simplify expr1: x^2+2*x+1
7. Simplify expr2: x^3+3*x^2+3*x+1
8. Difference (should be 0): x^2+2*x+1-x^3-3*x^2-3*x-1
9. RESULT: Not equivalent
```

## Mode 2: Transform to Target Form

Transform a source expression to match a target form with unknown constants (A, B, etc.).

### Example 1: Find Coefficients

```
Mode: 2
Source: 3*x+6
Target form (with A,B): A*x+B

Output:
1. Source = 3*x+6
2. Target = A*x+B
3. Original: 3*x+6
4. Simplify: 3*x+6
5. Final = 3*x+6

Result: The source matches A=3, B=6
```

## Mode 3: Binomial Expansion

Expand expressions of the form `(a+b)^n` using the binomial theorem.

### Example 1: Basic Expansion

```
Mode: 3
Expression: (x+1)^2
Max terms (Enter for all): 

Output:
1. Input = (x+1)^2
2. Binomial expansion
3. Terms:
4. Output = x^2+2*x+1
```

### Example 2: Expansion with Different Terms

```
Mode: 3
Expression: (2*x+3)^3
Max terms (Enter for all): 

Output:
1. Input = (2*x+3)^3
2. Binomial expansion
3. Terms:
4. Output = 8*x^3+36*x^2+54*x+27
```

### Example 3: Limited Terms

```
Mode: 3
Expression: (x+1)^4
Max terms (Enter for all): 2

Output:
1. Input = (x+1)^4
2. Binomial expansion
3. Terms:
4. Output = x^4+4*x^3

(Note: Only first 2 terms shown)
```

### Example 4: Cannot Expand

```
Mode: 3
Expression: x^2+2*x+1
Max terms (Enter for all): 

Output:
1. Input = x^2+2*x+1
2. Cannot expand (not binomial)
3. Output = x^2+2*x+1
```

## Mode 4: Polynomial Operations

Basic arithmetic operations on polynomials.

### Example 1: Add Polynomials

```
Mode: 4
Operation: 1
Poly1: 2*x^2+3*x
Poly2: x^2-4*x

Output:
1. p1 = 2*x^2+3*x
2. p2 = x^2-4*x
3. Add: p1 + p2
4. Sum = 3*x^2-x
```

### Example 2: Subtract Polynomials

```
Mode: 4
Operation: 2
Poly1: 5*x^2+3*x
Poly2: 2*x^2+x

Output:
1. p1 = 5*x^2+3*x
2. p2 = 2*x^2+x
3. Subtract: p1 - p2
4. Difference = 3*x^2+2*x
```

### Example 3: Multiply Polynomials

```
Mode: 4
Operation: 3
Poly1: x+2
Poly2: x+3

Output:
1. p1 = x+2
2. p2 = x+3
3. Multiply: p1 * p2
4. Product = x^2+5*x+6
```

## Mode 5: Complete the Square

Convert a quadratic expression to completed square form.

### Example 1: Basic Complete Square

```
Mode: 5
Expression: x^2+4*x+1

Output:
1. Input = x^2+4*x+1
2. Complete square: (x + b/2)^2 = x^2 + bx + (b/2)^2
3. Result = x^2+4*x+4-3
```

### Example 2: Perfect Square

```
Mode: 5
Expression: x^2+6*x+9

Output:
1. Input = x^2+6*x+9
2. x^2 + bx + c = (x + b/2)^2 when c = (b/2)^2
3. Result = (x+3)^2
```

### Example 3: No Linear Term

```
Mode: 5
Expression: x^2+4

Output:
1. Input = x^2+4
2. No x term
3. Result = x^2+4
```

## Mode 6: Solve Equations

Solve linear and quadratic equations.

### Example 1: Linear Equation

```
Mode: 6
Equation (set = 0): 3*x-6=0

Output:
1. Equation = 3*x-6 = 0
2. x = 2
```

### Example 2: Quadratic with Two Roots

```
Mode: 6
Equation (set = 0): x^2-5*x+6=0

Output:
1. Equation = x^2-5*x+6 = 0
2. x = 3 or x = 2
```

### Example 3: Quadratic Difference of Squares

```
Mode: 6
Equation (set = 0): x^2-4=0

Output:
1. Equation = x^2-4 = 0
2. x = 2 or x = -2
```

### Example 4: No Solution

```
Mode: 6
Equation (set = 0): x^2+1=0

Output:
1. Equation = x^2+1 = 0
2. No real solutions
```

### Example 5: Identity

```
Mode: 6
Equation (set = 0): 0=0

Output:
1. Equation = 0 = 0
2. Identity (infinite solutions)
```

## Mode 7: Simultaneous Equations

Solve systems of two linear equations in two variables.

### Example 1: Basic Simultaneous

```
Mode: 7
Type: 1
Eqn1: 2*x+3*y=5
Eqn2: x-y=1

Output:
1. Eqn1: 2*x+3*y=5
2. Eqn2: x-y=1
3. Solution: x=2, y=1
```

## Mode 8: Composite Functions

Find the composite function f(g(x)).

### Example 1: Basic Composite

```
Mode: 8
f(x): 2*x+1
g(x): x^2

Output:
1. f(x) = 2*x+1
2. g(x) = x^2
3. f(g(x)) = 2*x^2+1
```

### Example 2: More Complex

```
Mode: 8
f(x): x^2+3*x
g(x): 2*x-1

Output:
1. f(x) = x^2+3*x
2. g(x) = 2*x-1
3. f(g(x)) = (2*x-1)^2+3*(2*x-1)
4. = 4*x^2-4*x+1+6*x-3
5. = 4*x^2+2*x-2
```

## Mode 9: Inverse Functions

Find the inverse function f⁻¹(x).

### Example 1: Linear Inverse

```
Mode: 9
f(x): 2*x+1

Output:
1. f(x) = 2*x+1
2. Let y = 2*x+1
3. x = (y-1)/2
4. f⁻¹(x) = (x-1)/2
```

### Example 2: Quadratic Inverse

```
Mode: 9
f(x): x^2+1

Output:
1. f(x) = x^2+1
2. Let y = x^2+1
3. x = sqrt(y-1) (principal branch)
4. f⁻¹(x) = sqrt(x-1)
```

## Supported Operations

### Simplification
- Combine like terms (e.g., `2x + 3x → 5x`)
- Simplify coefficients (e.g., `6/2 → 3`)
- Remove zero terms
- Simplify powers (e.g., `x^2 * x^3 → x^5`)

### Logarithm Rules
- `log(a*b) = log(a) + log(b)`
- `log(a/b) = log(a) - log(b)`
- `log(a^n) = n*log(a)`

### Trigonometric Simplification
- `sin^2(x) + cos^2(x) → 1`
- `1 + tan^2(x) → sec^2(x)` (limited support)
- `1 + cot^2(x) → cosec^2(x)` (limited support)

### Division Simplification
- `x^2/x → x`
- `2*x/x → 2`
- `(x*y)/x → y`

## Error Handling

- Invalid input shows "Input error: [message]"
- Division by zero is caught and reported
- Malformed expressions are detected during parsing

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` and `random` imports (both available on calculator)
- No recursion - uses while loops for performance
- Limited to 100 terms in binomial expansion
- Very large expressions may cause memory issues
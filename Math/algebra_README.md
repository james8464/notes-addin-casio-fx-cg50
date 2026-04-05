# Algebra Program (`algebraProgram.py`)

An algebra manipulation program for the CASIO fx-cg50 calculator (MicroPython v1.9.4).

## Features

| Mode | Label shown | Feature | What it does |
|---|---|---|---|
| 1 | `cmp` | Compare | Checks whether two expressions are equivalent and shows simplification steps |
| 2 | `xform` | Transform | Rewrites a source expression into a target form when a supported route exists |
| 3 | `exp` | Expand | Expands supported binomial powers, including limited negative-power series |
| 4 | `poly` | Polynomial ops | Adds, subtracts, or multiplies two expressions |
| 5 | `comp sq` | Complete square | Rewrites a quadratic into completed-square form |
| 6 | `solve` | Solve | Solves linear and quadratic equations in one variable |
| 7 | `comp` | Composite | Finds `f(g(x))` |
| 8 | `inv` | Inverse | Finds inverse functions for supported one-to-one families |
| 9 | `rw` | Rewrite | Rewrites an expression using only the user-supplied target terms |

## Input syntax

- Variables: `x`, `y`, `z`, ...
- Numbers: integers, decimals, fractions
- Constants: `pi`, `e`
- Operators: `+`, `-`, `*`, `/`, `^`, `**`
- Functions: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`, `exp`, `log`, `ln`, `sqrt`, `abs`, `asin`, `acos`, `atan`
- Aliases: `arcsin`, `arccos`, `arctan`, `csc`
- Compact forms: `sin x`, `ln x`, `(x+1)(x+2)`, `2x+3x`

## Notes

- Accepts both `^` and `**`
- Implicit multiplication is supported in many calculator-style inputs
- Transform mode is strongest for algebraic target forms and a few direct trig template routes
- Solve mode treats plain input like `x^2-4` as `x^2-4 = 0`
- Higher-degree polynomial families are reported as unsupported
- CLI errors show `Err: ...`

## Mode 1: Compare (`cmp`)

Enter two expressions and the program simplifies both before deciding whether they are equivalent.

### Example

```text
M: 1
E1: (x+1)^5-(x-1)^5
E2: 10*x^4+20*x^2+2
```

Output:

```text
1. expr1 = (x+1)^5-(x-1)^5
2. expr2 = 10*(x)^4+20*(x)^2+2
3. Simplify both...
4. expr1 = (x+1)^5-(x-1)^5
5. expr2 = 10*(x)^4+20*(x)^2+2
6. Expression 1: (x+1)^5-(x-1)^5
7. Expression 2: 10*(x)^4+20*(x)^2+2
8. Simplify expr1: 20*(x)^2+10*(x)^4+2
9. Simplify expr2: 20*(x)^2+10*(x)^4+2
10. EXPRESSIONS ARE EQUIVALENT
11. RESULT: Equivalent
```

## Mode 2: Transform (`xform`)

Enter a source expression and a target expression. Supported routes include direct trig templates and algebraic target forms.

### Example

```text
M: 2
Src: 1-cos(2*x)
Tgt: 2*sin(x)^2
```

Output:

```text
1. Source = -cos(2*x)+1
2. Target = 2*(sin(x))^2
3. Original: -cos(2*x)+1
4. Use 1-cos(2A) = 2sin^2(A): 2*(sin(x))^2
5. Final = 2*(sin(x))^2
```

## Mode 3: Expand (`exp`)

Expands supported binomial powers.

Supported cases:
- non-negative integer powers such as `(2*x-3)^6`
- some negative integer powers as a finite series display
- some rational-power routes remain unsupported

### Example 1: positive power

```text
M: 3
Expr: (2*x-3)^6
Max:
```

Output:

```text
1. Input = (2*x-3)^6
2. Binomial expansion
3. Expand (2*x-3)^6
4. Term 1: C(6,0) * 2*x^6 * -3^0 = 64*(x)^6
...
11. Output = 729-2916*x+4860*(x)^2-4320*(x)^3+2160*(x)^4-576*(x)^5+64*(x)^6
```

### Example 2: negative power series

```text
M: 3
Expr: (x+1)^(-3)
Max:
```

Output:

```text
1. Input = (x+1)^-3
2. Binomial expansion (negative power)
3. Expand (x+1)^-3
4. Factor out x: (x)*(1 + 1/x)^-3
5. Term 1: 1 * (1/x)^0 = 1
6. Term 2: -3 * (1/x)^1 = -3/x
...
15. Output = (x)^-3*(1+-3/x+6/(x)^2+-10/(x)^3+...)
```

## Mode 4: Polynomial ops (`poly`)

Choose:
- `1` add
- `2` sub
- `3` mul

### Example

```text
M: 4
Op: 1
P1: 2*x^2+3*x
P2: x^2-4*x
```

Output:

```text
1. p1 = 2*(x)^2+3*x
2. p2 = (x)^2-4*x
3. Add: p1 + p2
4. Sum = 3*(x)^2-x
```

## Mode 5: Complete square (`comp sq`)

Rewrites a quadratic into completed-square form when possible.

### Example

```text
M: 5
Expression: 3*x^2+12*x-7
```

Output:

```text
1. Input = 3*(x)^2+12*x-7
2. Complete square: 3*(x+2)^2-19
3. Ans = 3*(x+2)^2-19
```

## Mode 6: Solve (`solve`)

Solves one-variable linear and quadratic equations.

Accepted forms:
- `6*x^2-5*x-6=0`
- `6*x^2-5*x-6`

### Example

```text
M: 6
Eq: 6*x^2-5*x-6=0
```

Output:

```text
1. Equation = 6*(x)^2-5*x-6 = 0
2. x = 3/2 or x = -2/3
```

## Mode 7: Composite (`comp`)

Finds `f(g(x))`.

### Example

```text
M: 7
f: (x^2+1)/(x-1)
g: 2*x+3
```

Output:

```text
1. f(x) = (x^2+1)/(x-1)
2. g(x) = 2*x+3
3. f(x) = (x^2+1)/(x-1)
4. g(x) = 2*x+3
5. f(g(x)) = ((2*x+3)^2+1)/(2*x+2)
```

## Mode 8: Inverse (`inv`)

Finds an inverse when the function family is supported and one-to-one over the reals.

### Example 1: linear inverse

```text
M: 8
f: 3*x-7
```

Output:

```text
1. f(x) = 3*x-7
2. f(x) = 3*x-7
3. Let y = 3*x-7
4. Swap: x = 3*y-7
5. x+7 = 3*y
6. y = (x+7)/3
7. f^-1(x) = (x+7)/3
```

### Example 2: unsupported quadratic inverse

```text
M: 8
f: x^2+1
```

Output:

```text
1. f(x) = x^2+1
2. f(x) = x^2+1
3. Let y = (x)^2+1
4. Swap: x = (y)^2+1
5. x-1 = (y)^2
6. No inverse on all real x for an even power
```

## Mode 9: Rewrite (`rw`)

Rewrites an expression using only the listed target terms.

### Example

```text
M: 9
Rw: x^2-1
T: x-1
T: x+1
T:
```

Output:

```text
1. Start with (x)^2-1
2. Write in terms of x-1, x+1 only.
3. Factor as a difference of squares
4. = (x-1)*(x+1)
5. Final = (x-1)*(x+1)
```

## Error handling

- Invalid input shows `Err: ...`
- Unsupported equation families are reported directly
- Malformed expressions are rejected during parsing

# Differentiation Program (`deriveProgram.py`)

A symbolic differentiation engine for the CASIO fx-cg50 calculator (MicroPython v1.9.4).

## Features

| Mode | Label shown | Feature | What it does |
|---|---|---|---|
| 1 | `n` | Normal | Differentiates an expression with respect to the chosen variable |
| 2 | `imp` | Implicit | Performs implicit differentiation on an equation |
| 3 | `par` | Parametric | Computes `dy/dx` from `x(t)` and `y(t)` |

## Input syntax

- Variables: `x`, `y`, `t`, ...
- Numbers: integers, decimals, fractions
- Constants: `pi`, `e`
- Functions: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`, `asin`, `acos`, `atan`, `exp`, `log`, `log10`, `sqrt`, `abs`, `ln`
- Operators: `+`, `-`, `*`, `/`, `^`, `**`
- Compact forms: `sin x`, `ln x`, `sin^2 x`, `cos^3 x`, `e^x`

## Notes

- Accepts both `^` and `**`
- Normal mode accepts plain expressions, `expr,x`, or `y=expr`
- Implicit mode requires `left=right`
- Parametric mode expects `t` as the parameter
- Working is short and calculator-friendly
- CLI errors show `Err: ...`

## Supported differentiation families

- constants, powers, sums, products, quotients
- chain rule
- trigonometric functions
- exponential and logarithmic functions
- inverse trig basics
- variable-exponent cases such as `x^x`, `(sin(x))^x`, `x^(sin(x))`
- **symbolic exponents** such as `x^n`, `x^a`, `x^(2*n)` (FIXED)

## Python API

```python
import sys
sys.path.insert(0, 'Math')
sys._derive_no_autorun = True
import deriveProgram as dp

# Basic derivative
result = dp.diff(dp.parse('x^2'), 'x', {})
print(dp.show(result))  # 2*x

# Symbolic power rule (NEW)
result = dp.diff(dp.parse('x^n'), 'x', {})
print(dp.show(result))  # n*x^(n-1)

# Chain rule
result = dp.diff(dp.parse('sin(2*x)'), 'x', {})
print(dp.show(result))  # 2*cos(2*x)

# Product rule
result = dp.diff(dp.parse('x*sin(x)'), 'x', {})
print(dp.show(result))  # x*cos(x)+sin(x)

# Quotient rule
result = dp.diff(dp.parse('sin(x)/x'), 'x', {})
print(dp.show(result))  # (x*cos(x)-sin(x))/x^2
```

## Mode 1: Normal (`n`)

Computes the derivative of a normal expression.

### Example 1: variable-exponent / log-differentiate style case

```text
M: 1
y: ((x^2+1)/(x^2-1))^x
```

Output:

```text
1. Log-differentiate style rule
2. y = ((x^2+1)/(x^2-1))^x
3. dy/dx = y*(v*du/u+ln(u)*dv/dx)
4. = (-4*((x^2+1)/(x^2-1))^x*x^2+((x^2+1)/(x^2-1))^x*ln((x^2+1)/(x^2-1))*x^4-((x^2+1)/(x^2-1))^x*ln((x^2+1)/(x^2-1)))/((x^2-1)*(x^2+1))
dy/dx = (-4*((x^2+1)/(x^2-1))^x*x^2+((x^2+1)/(x^2-1))^x*ln((x^2+1)/(x^2-1))*x^4-((x^2+1)/(x^2-1))^x*ln((x^2+1)/(x^2-1)))/((x^2-1)*(x^2+1))
```

### Example 2: simpler chain-rule style input

```text
M: 1
y: sin(2*x)
```

Output:

```text
Method: Differentiate with respect to x
Using chain rule
dy/dx = 2*cos(2*x)
```

## Mode 2: Implicit (`imp`)

Differentiates an equation where `y` is not isolated.

### Example

```text
M: 2
Eq: x^2+x*y+y^2=7
```

Output:

```text
Method: Implicit differentiation
d/dx(LHS)=d/dx(RHS)
2*x + x*dy/dx + y + 2*y*dy/dx = 0
Make dy/dx
(x+2*y)*dy/dx + 2*x+y = 0
dy/dx = (-2*x-y)/(x+2*y)
```

## Mode 3: Parametric (`par`)

Computes `dy/dx = (dy/dt)/(dx/dt)`.

### Example

```text
M: 3
x(t): t^3-3*t
y(t): t^4+t
```

Output:

```text
Method: Parametric differentiation
dx/dt = 3*t^2-3
dy/dt = 4*t^3+1
dy/dx = (4*t^3+1)/(3*(t^2-1))
```

## Error handling

- Invalid input shows `Err: ...`
- Missing `=` in implicit mode is rejected
- `dx/dt = 0` in parametric mode is rejected
- Unsupported forms fail directly rather than guessing

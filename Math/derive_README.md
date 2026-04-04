# Differentiation Program (deriveProgram.py)

A symbolic differentiation engine for the CASIO fx-cg50 calculator (runs MicroPython v1.9.4).

## Features

| Mode | Feature | Description |
|------|---------|-------------|
| 1 | Normal | Standard derivative of y with respect to x |
| 2 | Implicit | Implicit differentiation |
| 3 | Parametric | Parametric differentiation (dy/dx from dy/dt and dx/dt) |

## Input Syntax

- **Variables**: Single letters (x, y, t, etc.)
- **Numbers**: Integers, decimals, or fractions
- **Constants**: `pi`, `e`
- **Trig Functions**: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- **Inverse Trig**: `asin`, `acos`, `atan`, `arcsin`, `arccos`, `arctan`
- **Other Functions**: `exp`, `log`, `log10`, `sqrt`, `abs`, `ln`
- **Operators**: `+`, `-`, `*`, `/`, `^`, `**`
- **Grouping**: Parentheses `()`
- **Compact forms**: `sin x`, `ln x`, `sin^2 x`, `cos^3 x`, `e^x`
- **Normal mode extras**: `expr,x` for an explicit variable, or `y=expr`
- **Implicit mode**: `left=right`
- **Parametric mode**: enter `x(t)` and `y(t)` separately

## Coverage Notes

- Supports compact no-parentheses function input in normal mode
- Handles inverse trig aliases and powers like `sin^2(x)`
- Working is now shorter and closer to markscheme style
- Invalid input now shows `Err: ...` in the CLI
- Implicit mode still expects an equation and parametric mode still uses `t`

## Error Handling

- Invalid input shows `Err: ...`
- Division by zero is caught and reported
- Missing variables in implicit mode are detected
- Unsupported forms fail with a direct parse/solve error instead of guessing

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` import (available on calculator)
- Optimized for calculator-friendly symbolic work
- Shows short step lines for the screen
- Accepts both `^` and `**`

## Differentiation Rules Implemented

### Basic Rules
- **Constant**: d/dx[c] = 0
- **Power**: d/dx[x^n] = n*x^(n-1)
- **Constant Multiple**: d/dx[c*f] = c*f'
- **Sum/Difference**: d/dx[f ± g] = f' ± g'

### Product & Quotient
- **Product Rule**: d/dx[f*g] = f'*g + f*g'
- **Quotient Rule**: d/dx[f/g] = (f'*g - f*g')/g^2

### Chain Rule
- **Composite**: d/dx[f(g(x))] = f'(g(x)) * g'(x)

### Trigonometric
- d/dx[sin(x)] = cos(x)
- d/dx[cos(x)] = -sin(x)
- d/dx[tan(x)] = sec^2(x)
- d/dx[sec(x)] = sec(x)tan(x)
- d/dx[cosec(x)] = -cosec(x)cot(x)
- d/dx[cot(x)] = -cosec^2(x)

### Exponential & Logarithm
- d/dx[e^x] = e^x
- d/dx[a^x] = a^x * ln(a)
- d/dx[ln(x)] = 1/x
- d/dx[log(x)] = 1/(x * ln(10))

### Inverse Trig (limited)
- d/dx[asin(x)] = 1/sqrt(1-x^2)
- d/dx[acos(x)] = -1/sqrt(1-x^2)
- d/dx[atan(x)] = 1/(1+x^2)

## Error Handling

- Invalid input shows "Input error: [message]"
- Division by zero is caught and reported
- Missing variables in implicit mode are detected

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` import (available on calculator)
- Optimized for A-level mathematics
- Can handle complex nested expressions
- Shows step-by-step working for clarity

## Mode 1: Normal Differentiation

Calculate dy/dx for a function y = f(x).

### Example 1: Polynomial

```
Mode: 1
y = 3*x^2+2*x+1

Output:
1. d/dx[3*x^2+2*x+1]
2. = 6*x+2
dy/dx = 6*x+2
```

### Example 2: Power Function

```
Mode: 1
y = x^5

Output:
1. d/dx[x^5]
2. = 5*x^4
dy/dx = 5*x^4
```

### Example 3: Trigonometric

```
Mode: 1
y = sin(x)

Output:
1. d/dx[sin(x)]
2. = cos(x)
dy/dx = cos(x)
```

### Example 4: Chain Rule

```
Mode: 1
y = sin(2*x)

Output:
1. d/dx[sin(2*x)]
2. = cos(2*x)*2
3. = 2*cos(2*x)
dy/dx = 2*cos(2*x)
```

### Example 5: Exponential

```
Mode: 1
y = exp(x)

Output:
1. d/dx[exp(x)]
2. = exp(x)
dy/dx = exp(x)
```

### Example 6: Exponential with Chain Rule

```
Mode: 1
y = exp(3*x)

Output:
1. d/dx[exp(3*x)]
2. = exp(3*x)*3
3. = 3*exp(3*x)
dy/dx = 3*exp(3*x)
```

### Example 7: Logarithm

```
Mode: 1
y = log(x)

Output:
1. d/dx[log(x)]
2. = 1/x
dy/dx = 1/x
```

### Example 8: Product Rule

```
Mode: 1
y = x*sin(x)

Output:
1. d/dx[x*sin(x)]
2. = sin(x)+x*cos(x)
dy/dx = sin(x)+x*cos(x)
```

### Example 9: Quotient Rule

```
Mode: 1
y = sin(x)/x

Output:
1. d/dx[sin(x)/x]
2. = (cos(x)*x-sin(x))/x^2
dy/dx = (cos(x)*x-sin(x))/x^2
```

### Example 10: Multiple Terms

```
Mode: 1
y = 2*x^3+3*x^2-5*x+7

Output:
1. d/dx[2*x^3+3*x^2-5*x+7]
2. = 6*x^2+6*x-5
dy/dx = 6*x^2+6*x-5
```

## Mode 2: Implicit Differentiation

Differentiate equations where y is not isolated.

### Example 1: Simple Implicit

```
Mode: 2
Eqn: x^2+y^2=1

Output:
1. Clear fractions
2. x^2+y^2 = 0
3. d/dx(LHS) = d/dx(RHS)
4. 2*x+2*y*dy/dx = 0
5. 2*x*dy/dx+2*y*dy/dx = 0
6. Make dy/dx the subject
7. 2*x*dy/dx = -2*y
dy/dx = -x/y
```

### Example 2: Implicit with xy term

```
Mode: 2
Eqn: x*y=1

Output:
1. d/dx(LHS) = d/dx(RHS)
2. 1*y+x*dy/dx = 0
3. Make dy/dx the subject
4. x*dy/dx = -y
dy/dx = -y/x
```

### Example 3: More Complex

```
Mode: 2
Eqn: x^2+y^2=4

Output:
1. Clear fractions
2. x^2+y^2-4 = 0
3. d/dx(LHS) = d/dx(RHS)
4. 2*x+2*y*dy/dx = 0
5. Make dy/dx the subject
6. 2*y*dy/dx = -2*x
dy/dx = -x/y
```

## Mode 3: Parametric Differentiation

Find dy/dx when both x and y are functions of t.

### Example 1: Basic Parametric

```
Mode: 3
x(t) = t^2
y(t) = t^3

Output:
dx/dt = 2*t
dy/dt = 3*t^2
dy/dx = (dy/dt)/(dx/dt) = 3*t^2/(2*t)
= 3*t/2
dy/dx = 3*t/2
```

### Example 2: Trig Parametric

```
Mode: 3
x(t) = cos(t)
y(t) = sin(t)

Output:
dx/dt = -sin(t)
dy/dt = cos(t)
dy/dx = (dy/dt)/(dx/dt) = cos(t)/(-sin(t))
= -cot(t)
dy/dx = -cot(t)
```

### Example 3: Exponential Parametric

```
Mode: 3
x(t) = exp(t)
y(t) = t^2

Output:
dx/dt = exp(t)
dy/dt = 2*t
dy/dx = (dy/dt)/(dx/dt) = 2*t/exp(t)
= 2*t*exp(-t)
dy/dx = 2*t*exp(-t)
```

## Differentiation Rules Implemented

### Basic Rules
- **Constant**: d/dx[c] = 0
- **Power**: d/dx[x^n] = n*x^(n-1)
- **Constant Multiple**: d/dx[c*f] = c*f'
- **Sum/Difference**: d/dx[f ± g] = f' ± g'

### Product & Quotient
- **Product Rule**: d/dx[f*g] = f'*g + f*g'
- **Quotient Rule**: d/dx[f/g] = (f'*g - f*g')/g^2

### Chain Rule
- **Composite**: d/dx[f(g(x))] = f'(g(x)) * g'(x)

### Trigonometric
- d/dx[sin(x)] = cos(x)
- d/dx[cos(x)] = -sin(x)
- d/dx[tan(x)] = sec^2(x)
- d/dx[sec(x)] = sec(x)tan(x)
- d/dx[cosec(x)] = -cosec(x)cot(x)
- d/dx[cot(x)] = -cosec^2(x)

### Exponential & Logarithm
- d/dx[e^x] = e^x
- d/dx[a^x] = a^x * ln(a)
- d/dx[ln(x)] = 1/x
- d/dx[log(x)] = 1/(x * ln(10))

### Inverse Trig (limited)
- d/dx[asin(x)] = 1/sqrt(1-x^2)
- d/dx[acos(x)] = -1/sqrt(1-x^2)
- d/dx[atan(x)] = 1/(1+x^2)

## Error Handling

- Invalid input shows "Input error: [message]"
- Division by zero is caught and reported
- Missing variables in implicit mode are detected

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` import (available on calculator)
- Optimized for A-level mathematics
- Can handle complex nested expressions
- Shows step-by-step working for clarity
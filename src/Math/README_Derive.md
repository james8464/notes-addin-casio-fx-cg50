# Derive Calculator

Calculus differentiation tool with 3 modes.

## Menu Modes

### 1. Normal (Mode 1)
Differentiate an expression with respect to a variable.

**Input:**
- y: Expression to differentiate

The program auto-detects the variable to differentiate by (usually x if present).

**Examples:**
```
y: x^2                   → dy/dx = 2x
y: 3x^2 + 2x            → dy/dx = 6x + 2
y: sin(x)                → dy/dx = cos(x)
y: x^3 + 2x^2 + x       → dy/dx = 3x^2 + 4x + 1
y: 1/x                   → dy/dx = -1/x^2
y: sqrt(x)               → dy/dx = 1/(2*sqrt(x))
y: ln(x)                → dy/dx = 1/x
y: exp(x)                → dy/dx = exp(x)
y: sin(2x)              → dy/dx = 2cos(2x)
y: x*sin(x)              → dy/dx = sin(x) + x*cos(x)
y: (x+1)^2             → dy/dx = 2(x+1)
y: 1/x^2                 → dy/dx = -2/x^3
y: x^3 - 3x             → dy/dx = 3x^2 - 3
y: cos(x^2)             → dy/dx = -2x*sin(x^2)
y: e^(2x)               → dy/dx = 2e^(2x)
```

### 2. Implicit (Mode 2)
Differentiate an implicitly-defined equation.

**Input:**
- Eq: Equation in form left=right

Solves for dy/dx (or dz/dx etc.) by implicit differentiation.

**Examples:**
```
Eq: x^2 + y^2 = 25        → dy/dx = -x/y
Eq: x*y = 1               → dy/dx = -y/x
Eq: x^2 + y = x + y^2      → dy/dx = (1 - 2x)/(2y - 1)
Eq: x^3 + y^3 = 2xy       → dy/dx = (2y - 3x^2)/(3y^2 - 2x)
Eq: sin(x+y) = cos(x)      → dy/dx = ?
Eq: x^2 - y^2 = 1         → dy/dx = x/y
Eq: y = x + 1/y           → dy/dx = y^2/(1 - y^2)
Eq: x + y^2 = 4           → dy/dx = -1/(2y)
```

Shows step-by-step:
1. Clear fractions if needed
2. Differentiate both sides
3. Rearrange to isolate dy/dx

### 3. Parametric (Mode 3)
Find derivative dy/dx from parametric equations.

**Input:**
- x(t): x as function of parameter
- y(t): y as function of parameter

Uses dy/dx = (dy/dt)/(dx/dt).

**Examples:**
```
x(t): t^2          y(t): t^3        → dy/dx = (3t^2)/(2t) = 3t/2
x(t): sin(t)       y(t): cos(t)       → dy/dx = -cot(t)
x(t): t           y(t): t^2         → dy/dx = 2t
x(t): 2t+1        y(t): 3t-2       → dy/dx = 3/2
x(t): exp(t)       y(t): exp(2t)    → dy/dx = 2exp(t)
x(t): t^3          y(t): t^2        → dy/dx = (2t)/(3t^2) = 2/(3t)
```

Also shows Cartesian form if derivable.

## Input Syntax

### Numbers
- Integers: `10`, `-5`
- Decimals: `9.8`, `0.5`
- Fractions: `1/2`, `3/4`

### Variables
- Variables: `x`, `y`, `z`, `t`, `u`, `v`
- Parameter for Mode 3: `t` (or use any letter)

### Operators
- `+`, `-` : addition, subtraction
- `*` or space : multiplication
- `/` : division
- `^` or `**` : power

### Functions
- Trigonometric: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- Inverse trig: `asin`, `acos`, `atan`
- Exponential: `exp`, `e^`
- Logarithmic: `ln`, `log`
- Other: `sqrt`, `abs`

### Constants
- `e` - Euler's number
- `pi` - π

## Derivative Rules

### Basic
- d/dx(x^n) = nx^(n-1)
- d/dx(c) = 0 (c is constant)
- d/dx(cx) = c

### Sum/Product/Quotient
- d/dx(f + g) = f' + g'
- d/dx(fg) = f'g + fg'
- d/dx(f/g) = (f'g - fg')/g²

### Chain Rule
- d/dx(f(g(x))) = f'(g(x)) * g'(x)

### Common Derivatives
- d/dx(sin x) = cos x
- d/dx(cos x) = -sin x
- d/dx(tan x) = sec² x
- d/dx(e^x) = e^x
- d/dx(ln x) = 1/x

### Implicit Differentiation
1. Differentiate both sides with respect to x
2. Collect terms with dy/dx on one side
3. Solve for dy/dx
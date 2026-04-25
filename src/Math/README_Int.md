# Integral Calculator

Integration solver with 3 main modes.

## Menu Modes

### 1. Integrate (Mode 1)
Find indefinite integral ∫f(x)dx

**Input:**
- f: Integrand expression
- Met: Integration method (1-7)

**Method Options:**
1. **Auto** - Program chooses best method
2. **Direct** - Basic power rule, exponential rules
3. **Trig** - Trigonometric integration
4. **Sub** - Integration by substitution (u-substitution)
5. **Parts** - Integration by parts
6. **Partial Fractions** - Partial fraction decomposition
7. **Div** - Division for improper fractions

**Input for Method 4 (Sub):**
- u: Substitution expression (e.g., "2x" for u = 2x)

**Examples:**
```
f: x^2                 Met: 1      → x^3/3 + C
f: 1/x                 Met: 1      → ln|x| + C
f: x^3 + 2x           Met: 1      → x^4/4 + x^2 + C
f: sin(x)              Met: 1      → -cos(x) + C
f: exp(x)              Met: 1      → exp(x) + C
f: 1/(x+1)           Met: 1      → ln|x+1| + C
f: (x+1)^2           Met: 1      → (x+1)^3/3 + C
f: x*sqrt(x)           Met: 4      → u = x^2, gives 2/3*x^(3/2)
f: x*exp(x^2)         Met: 4      → u = x^2, gives exp(x^2)/2 + C
f: x/(x+1)            Met: 7      → Division then ln
f: 1/(x^2-1)          Met: 6      → Partial fractions
f: x*sin(x)            Met: 5      → Parts: u=x, dv=sin(x)
f: ln(x)               Met: 5      → Parts: u=ln(x), dv=1
```

### 2. Differential Equation (Mode 2)
Solve first-order differential equations.

**Input:**
- dy/dx: Left side (e.g., "dy/dx" or just expression)
- BC: (optional) Boundary condition

**Boundary condition formats:**
- `y(1)=2` - y = 2 when x = 1
- `x=1,y=2` - same as above
- `y(0)=1` - y = 1 when x = 0

**Equation types supported:**
- Separable: dy/dx = f(x)g(y)
- Linear: dy/dx + P(x)y = Q(x)
- Exact equations
- Homogeneous

**Examples:**
```
dy/dx: y                BC: y(1)=2        → dy/y = dx, ln y = x + C, y = e^x
dy/dx: 2x              BC:                → y = x^2 + C
dy/dx: x               BC: y(0)=1        → y = x^2/2 + 1
dy/dx: y*sin(x)        BC:                → separable
dy/dx: y + x          BC:                → linear ODE
dy/dx: 1 - y^2        BC: y(0)=0        → tanh solution
dy/dx: 3y             BC: y(0)=2        → y = 2e^(3x)
```

### 3. Parametric Area (Mode 3)
Find area under parametric curve.

**Input:**
- x(t): x as function of parameter
- y(t): y as function of parameter

Uses formula: ∫y dx = ∫y(t) x'(t) dt

**Examples:**
```
x(t): t^2         y(t): t          → Area between t=0 and t=1
x(t): cos(t)      y(t): sin(t)      → Area enclosed by x=cos(t), y=sin(t)
x(t): t           y(t): t^2        → Area under parabola from t=0 to t=1
x(t): 2t         y(t): t^2        → Area from t=0 to t=1
x(t): sin(t)      y(t): cos(t)     → Circle: x^2+y^2=1
```

Also outputs dx/dt for reference.

## Input Syntax

### Numbers
- Integers: `10`, `-5`
- Decimals: `9.8`, `0.5`
- Fractions: `1/2`, `3/4`
- Mixed: `1 1/2` (= 3/2)

### Variables
- Integration variable: `x` (Mode 1), `t` (Mode 3)
- Other: `y`, `z`, `u`, `v`

### Operators
- `+`, `-` : addition, subtraction
- `*` or space : multiplication
- `/` : division
- `^` or `**` : power
- `()` : grouping

### Special in Integrals
- `I` or `int` : integral notation (parsed as expression)
- `dx`, `dt` : differential
- `d/dx`, `d/dt` : derivative notation

### Functions
- Trigonometric: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- Inverse: `asin`, `acos`, `atan`
- Exponential: `exp`, `e^`
- Logarithmic: `ln`, `log`
- Other: `sqrt`, `abs`, `sign`

### Constants
- `e` - Euler's number (~2.718)
- `pi` - π (~3.14159)

## Integration Methods

### 1. Power Rule
∫x^n dx = x^(n+1)/(n+1) + C (n ≠ -1)

### 2. Exponential
∫e^x dx = e^x + C
∫a^x dx = a^x/ln(a) + C

### 3. Trigonometric
∫sin(x) dx = -cos(x) + C
∫cos(x) dx = sin(x) + C
∫sec²(x) dx = tan(x) + C

### 4. u-Substitution
∫f(g(x))g'(x) dx = ∫f(u) du where u = g(x)

### 5. Integration by Parts
∫u dv = uv - ∫v du

Choose u = LIATE (Log, Inverse trig, Algebraic, Trig, Exponential)

### 6. Partial Fractions
For rational functions, decompose then integrate each term.

### 7. Division
For improper fractions (degree numerator ≥ degree denominator), divide first.

## Common Integral Examples

| Function | Integral |
|----------|----------|
| x^n | x^(n+1)/(n+1) |
| 1/x | ln\|x\| |
| e^x | e^x |
| sin(x) | -cos(x) |
| cos(x) | sin(x) |
| sec²(x) | tan(x) |
| tan(x) | -ln\|cos(x)\| |
| sec(x) | ln\|sec(x)+tan(x)\| |
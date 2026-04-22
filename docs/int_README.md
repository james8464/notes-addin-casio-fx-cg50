# Integration Program (`intProgram.py`)

A symbolic integration engine for the CASIO fx-cg50 calculator (MicroPython v1.9.4).

## Features

| Mode | Label shown | Feature | What it does |
|---|---|---|---|
| 1 | `int` | Integral | Computes indefinite integrals |
| 2 | `de` | Differential equations | Solves supported first-order differential equations |
| 3 | `param area` | Parametric area | Computes area under parametric curve ∫y dx |

## Integration methods (Mode 1)

| Option | Label shown | Meaning |
|---|---|---|
| 1 | `auto` | Let the engine choose a route |
| 2 | `dir` | Direct integration |
| 3 | `trig` | Trigonometric identities / trig reductions |
| 4 | `sub` | Substitution |
| 5 | `pts` | Integration by parts |
| 6 | `pf` | Partial fractions |
| 7 | `div` | Polynomial division |

## Input syntax

- Variables: `x`, `t`, ...
- Numbers: integers, decimals, fractions
- Constants: `pi`, `e`
- Functions: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`, `exp`, `log`, `log10`, `sqrt`, `abs`, `ln`, `asin`, `acos`, `atan`
- Operators: `+`, `-`, `*`, `/`, `^`, `**`
- Compact forms: `sin x`, `ln x`, `sec x tan x`, `sin^2 x`, `cos^3 x`
- Explicit variable form: `expr,x`
- Equation form: `EQ1=expr` or `y=expr` or `expr=expr`
- DE input: `dy/dx=...` or `dy/dx: ...` with optional boundary condition like `y=3,x=0`

## Notes

- Accepts both `^` and `**`
- DE mode accepts both `dy/dx = ...` and `dy/dx: ...` syntax
- Auto mode already covers many direct, trig, substitution, parts, partial-fraction, and DE families
- Working is compact and calculator-friendly: method, key formula/substitution, working, and answer
- Some non-elementary families still return out-of-scope messages
- CLI errors show `Err: ...`

## Mode 1: Integral (`int`)

Choose the method, then enter the integrand.

### Example 1: auto / exponential-trig special form

```text
M: 1
f: e^(5*x)*sin(7*x)
Met: 1
```

Output:

```text
Method: Int[e^(ax+b)sin(cx+d)]
Use: = e^(ax+b)*(a*sin(cx+d) - c*cos(cx+d))/(a^2+c^2) + C
= e^(5*x)*(5*sin(7*x) - 7*cos(7*x))/74 + C
Answer: e^(5*x)*(5*sin(7*x) - 7*cos(7*x))/74 + C
```

### Example 2: substitution

```text
M: 1
f: (3*x^2+1)/(x^3+x+7)
Met: 4
u: x^3+x+7
```

Output:

```text
Method: Integration by substitution
u = x^3+x+7
= ln|x^3+x+7| + C
Answer: ln|x^3+x+7| + C
```

### Example 3: trig reduction

```text
Input family: sin(x)^4
Method: 3
```

Typical output shape:

```text
Use sin^2 x = (1-cos(2x))/2.
So I = Int[...]
= -1/4*sin(2*x)+1/32*sin(4*x)+3/8*x + C
```

### Example 4: partial fractions

```text
Input family: 1/(x^4-1)
Method: 6
```

Typical output shape:

```text
Use partial fractions.
1/(x^4-1) = A/(x-1)+B/(x+1)+(C*x+D)/(x^2+1)
A = 1/4, B = -1/4, C = 0, D = -1/2
So I = 1/4*ln|1-x|-1/4*ln|x+1|-1/2*atan(x) + C
```

### Example 5: division

```text
Input family: (x^4+1)/(x^2+1)
Method: 7
```

Typical output shape:

```text
Divide the numerator by the denominator.
So I = Int[x^2+2/(x^2+1)-1] dx
= 1/3*x^3+2*atan(x)-x + C
```

## Mode 2: Differential equations (`de`)

Solves supported first-order differential equations with optional boundary conditions.

### Example

```text
M: 2
dy/dx: dy/dx=2*x*y
BC: y=3,x=0
```

Output:

```text
1. Separate variables
2. Int[1/y] dy = Int[2*x] dx
3. ln|y| = x^2 + C
4. Use y = 3 when x = 0.
5. C = ln(3)
6. ln|y| = x^2+ln(3)
7. So y = 3*e^(x^2)
y = 3*e^(x^2)
```

## Mode 3: Parametric area (`param area`)

Computes the area under a parametric curve using ∫y dx = ∫y (dx/dt) dt.

### Example

```text
M: 3
x(t): t^2
y(t): 2*t+1
```

Output:

```text
dx/dt = 2*t
Int[y dx] = Int[(2*t+1)*2*t] dt
Method: Int[polynomial in t]
Use power rule.
= 4/3*t^3+t^2 + C
```

## Python API

```python
import sys
sys.path.insert(0, 'Math')
sys._int_no_autorun = True
import intProgram as ip

# Basic polynomial integration
J, K = ip.parse_input('x^2')
result, reason, steps = ip.solve(J, K, '1')
print('Result exists:', result is not None)

# a^x integration (NEW)
J, K = ip.parse_input('2^x')
result, reason, steps = ip.solve(J, K, '1')
print('2^x integral works:', result is not None)

# ln(x)^n by parts (NEW)
J, K = ip.parse_input('(ln(x))^3')
result, reason, steps = ip.solve(J, K, '1')
print('ln(x)^3 integral works:', result is not None)

# High-power trig (NEW)
J, K = ip.parse_input('sin(x)^6')
result, reason, steps = ip.solve(J, K, '1')
print('sin(x)^6 integral works:', result is not None)
```

## Out-of-scope examples

The engine now reports some hard families more explicitly, for example:
- `1/sqrt(1-x^4)`
- `1/(x^2*ln(x))`
- `atan(x)^2`
- `exp(x^2)`
- `ln(ln(x))`

## Error handling

- Invalid input shows `Err: ...`
- Unsupported questions report a direct out-of-scope message
- Division by zero is caught and reported

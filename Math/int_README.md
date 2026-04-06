# Integration Program (`intProgram.py`)

A symbolic integration engine for the CASIO fx-cg50 calculator (MicroPython v1.9.4).

## Features

| Mode | Label shown | Feature | What it does |
|---|---|---|---|
| 1 | `int` | Integral | Computes indefinite integrals |
| 2 | `de` | Differential equations | Solves supported first-order differential equations |

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
- DE input: `dy/dx=...` with optional boundary condition like `y=3,x=0`

## Notes

- Accepts both `^` and `**`
- Auto mode already covers many direct, trig, substitution, parts, partial-fraction, and DE families
- Working is short and calculator-friendly
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
Met: Integration by parts
1. Use the standard result for Int[e^(ax+b)sin(cx+d)] dx.
2. = 1*e^(5*x)*(5*sin(7*x)-7*cos(7*x))/74 + C
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
Met: Integration by substitution
1. u = x^3+x+7
2. du/dx = 3*x^2+1
3. du = 3*x^2+1 dx
4. I = Int[1/u] du
5. = ln|u| + C
6. = ln|x^3+x+7| + C
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

## Supported method families

### Direct (`dir`)
- power rule and rational powers
- `exp(x)` and simple exponential forms
- **exponential with any base**: `a^x` for any constant `a` (NEW)
- `1/x`, `1/(a*x+b)`, and log-derivative style cases
- basic trig integrals such as `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- `sec^2`, `cosec^2`, `sec*tan`, `cosec*cot`
- `1/(x^2+a^2)` style atan forms

### Trig (`trig`)
- power reduction such as `sin^2`, `sin^4`, `cos^2`
- **high-power trig identities**: `sin^6`, `cos^4`, `tan^3`, `tan^4` (NEW)
- odd trig-power families
- product-to-sum and same-angle rewrites

### Substitution (`sub`)
- standard `u` substitution
- reverse-chain style matches
- rational/log composite forms

### Parts (`pts`)
- polynomial × trig / exponential
- **log powers**: `ln(x)^n` by parts (NEW)
- inverse-trig and log products in supported cases
- cyclic exponential-trig forms

### Partial fractions (`pf`)
- linear factor splits
- selected linear/quadratic decompositions
- some higher-value hard-coded rational families

### Division (`div`)
- polynomial numerator / lower-degree denominator division before integration

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

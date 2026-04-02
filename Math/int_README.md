# Integration Program (intProgram.py)

A symbolic integration engine for the CASIO fx-cg50 calculator (runs MicroPython v1.9.4).

## Features

| Mode | Feature | Description |
|------|---------|-------------|
| 1 | Integral | Calculate indefinite integrals |
| 2 | DE | Solve differential equations |

## Integration Methods (Mode 1)

| Option | Method | Description |
|--------|--------|-------------|
| 1 | auto | Let program choose best method |
| 2 | direct | Direct integration |
| 3 | trig | Trigonometric integration |
| 4 | sub | Substitution |
| 5 | parts | Integration by parts |
| 6 | pf | Partial fractions |
| 7 | divide | Division |

## Input Syntax

- **Variables**: Single letters (x, t, etc.)
- **Numbers**: Integers, decimals, or fractions
- **Constants**: `pi`, `e`
- **Trig Functions**: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- **Other Functions**: `exp`, `log`, `log10`, `sqrt`, `abs`, `atan`
- **Operators**: `+`, `-`, `*`, `/`, `^`
- **Grouping**: Parentheses `()`

## Mode 1: Integrals

### Example 1: Power Rule

```
Mode: 1
f = x^2
Method: 1

Output:
Method: direct
1. Add 1 to power: 2+1 = 3
2. Divide by new power: 1/3
= x^3/3 + C
```

### Example 2: Trigonometric

```
Mode: 1
f = cos(x)
Method: 1

Output:
Method: direct
1. Integral of cos(x)
= sin(x) + C
```

### Example 3: Exponential

```
Mode: 1
f = exp(x)
Method: 1

Output:
Method: direct
1. Integral of e^x
= e^x + C
```

### Example 4: Substitution

```
Mode: 1
f = 2*x*(x^2+1)^2
Method: 4
u = x^2+1

Output:
Method: sub
1. Let u = x^2+1
2. du/dx = 2*x
3. dx = du/(2*x)
4. Substituting...
5. Integral = u^3/3 + C
= (x^2+1)^3/3 + C
```

### Example 5: Integration by Parts

```
Mode: 1
f = x*exp(x)
Method: 5

Output:
Method: parts
1. u = x, dv = e^x dx
2. du = dx, v = e^x
3. ∫u dv = uv - ∫v du
4. = x*e^x - ∫e^x dx
5. = x*e^x - e^x + C
= e^x*(x-1) + C
```

### Example 6: 1/x

```
Mode: 1
f = 1/x
Method: 1

Output:
Method: direct
1. ln|x| + C
= ln(x) + C
```

### Example 7: 1/(x+1)

```
Mode: 1
f = 1/(x+1)
Method: 1

Output:
Method: direct
1. ln|x+1| + C
= ln(x+1) + C
```

### Example 8: sec(x)tan(x)

```
Mode: 1
f = sec(x)*tan(x)
Method: 1

Output:
Method: direct
1. Integral of sec(x)tan(x)
= sec(x) + C
```

### Example 9: sec^2(x)

```
Mode: 1
f = sec(x)^2
Method: 1

Output:
Method: direct
1. Integral of sec^2(x)
= tan(x) + C
```

### Example 10: Partial Fractions

```
Mode: 1
f = 1/(x^2-1)
Method: 6

Output:
Method: pf
1. 1/(x^2-1) = 1/[(x-1)(x+1)]
2. = A/(x-1) + B/(x+1)
3. Solving: A = 1/2, B = -1/2
4. = 1/2*1/(x-1) - 1/2*1/(x+1)
5. Integral = 1/2*ln|x-1| - 1/2*ln|x+1| + C
```

## Mode 2: Differential Equations

Solve first-order differential equations with optional boundary conditions.

### Example 1: Basic DE

```
Mode: 2
dy/dx = 2*x
BC: 

Output:
1. Separate variables
2. dy = 2*x dx
3. Integrate both sides
4. ∫dy = ∫2*x dx
5. y = x^2 + C
```

### Example 2: With Boundary Condition

```
Mode: 2
dy/dx = 3*x^2
BC: y(0)=2

Output:
1. Separate variables
2. dy = 3*x^2 dx
3. Integrate both sides
4. ∫dy = ∫3*x^2 dx
5. y = x^3 + C
6. Using y(0) = 2: C = 2
7. y = x^3 + 2
```

### Example 3: dy/dx = ky

```
Mode: 2
dy/dx = 2*y
BC: 

Output:
1. Separate variables
2. dy/y = 2 dx
3. Integrate both sides
4. ln|y| = 2x + C
5. y = C*e^(2x)
```

## Integration Methods

### Direct Integration
- Power rule: ∫x^n dx = x^(n+1)/(n+1) + C
- ∫e^x dx = e^x + C
- ∫1/x dx = ln|x| + C
- ∫cos(x) dx = sin(x) + C
- ∫sin(x) dx = -cos(x) + C

### Trigonometric
- ∫sin^n(x)cos(x) dx = sin^(n+1)(x)/(n+1) + C
- ∫cos^n(x)sin(x) dx = -cos^(n+1)(x)/(n+1) + C

### Substitution
- u-substitution for composite functions

### Integration by Parts
- ∫u dv = uv - ∫v du
- Choose u using LIATE (Log, Inverse trig, Algebraic, Trig, Exponential)

### Partial Fractions
- Decompose rational functions into simpler fractions
- Then integrate each term

## Error Handling

- Invalid input shows "Input error: [message]"
- Unsupported integrals show "This integral is outside the supported A-level method set."
- Division by zero is caught and reported

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` import (available on calculator)
- Optimized for A-level mathematics
- Supports various integration techniques
- Shows step-by-step working
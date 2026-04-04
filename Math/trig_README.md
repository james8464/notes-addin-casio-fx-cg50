# Trigonometry Program (trigProgram.py)

A comprehensive trigonometric manipulation program for the CASIO fx-cg50 calculator (runs MicroPython v1.9.4).

## Features

| Mode | Feature | Description |
|------|---------|-------------|
| 1 | Prove/Show | Prove trigonometric identities step-by-step |
| 2 | Transform | Transform one expression to match another |
| 3 | Solve | Solve trigonometric equations |

## Input Syntax

- **Variables**: Single letters (x, y, A, B, etc.)
- **Numbers**: Integers, decimals, or fractions
- **Constants**: `pi`, `e`
- **Trig Functions**: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- **Inverse Trig**: `asin`, `acos`, `atan`, `arcsin`, `arccos`, `arctan`
- **Other Functions**: `exp`, `log`, `ln`, `sqrt`, `abs`
- **Operators**: `+`, `-`, `*`, `/`, `^`, `**`
- **Grouping**: Parentheses `()`
- **Compact forms**: `sin x`, `sec x`, `sin^2(x)`, `cos^2(x)`
- **Solve mode**: `equation,var`, `equation,var,start,end`, `equation,var,a<x<b`
- **Extrema mode**: `expr,max,x` or `expr,min,x`

## Coverage Notes

- Parser accepts calculator-style no-parentheses trig input
- Power forms like `sin^2(x)` and `cos^2(x)` are supported
- Route/proof/solve output is compacted for short screens
- CLI errors now show `Err: ...`

## Error Handling

- Invalid input shows `Err: ...`
- Unsupported identities or routes return a short direct message
- Equations with no solutions are reported

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` import (available on calculator)
- Optimized for calculator-friendly trig rewrites and solving
- Accepts both `^` and `**`

## Mode 1: Prove/Show Identity

Proves trigonometric identities with step-by-step working.

## Mode 1: Prove/Show Identity

Proves trigonometric identities with step-by-step working.

### Route Options

- `1` (auto): Let program choose the best approach
- `2` (lhs): Work from the left-hand side
- `3` (rhs): Work from the right-hand side
- `4` (both): Show both directions

### Example 1: Basic Identity - sin² + cos² = 1

```
Mode: 1
Identity: sin(x)^2+cos(x)^2=1
Route: 1

Output:
1. sin(x)^2+cos(x)^2
= (1-cos(2x))/2+(1+cos(2x))/2
= (1-cos(2x)+1+cos(2x))/2
= 2/2
= 1

Proof: sin^2 A + cos^2 A = 1
```

### Example 2: tan = sin/cos

```
Mode: 1
Identity: tan(x)=sin(x)/cos(x)
Route: 1

Output:
1. tan(x)
= sin(x)/cos(x)

Proof: tan A = sin A / cos A
```

### Example 3: sec = 1/cos

```
Mode: 1
Identity: sec(x)=1/cos(x)
Route: 1

Output:
1. sec(x)
= 1/cos(x)

Proof: sec A = 1/cos A
```

### Example 4: 1 + tan² = sec²

```
Mode: 1
Identity: 1+tan(x)^2=sec(x)^2
Route: 1

Output:
1. 1+tan(x)^2
= 1+sin(x)^2/cos(x)^2
= (cos(x)^2+sin(x)^2)/cos(x)^2
= 1/cos(x)^2
= sec(x)^2

Proof: 1 + tan^2 A = sec^2 A
```

### Example 5: Double Angle - sin(2x)

```
Mode: 1
Identity: sin(2*x)=2*sin(x)*cos(x)
Route: 1

Output:
1. sin(2*x)
= 2*sin(x)*cos(x)

Proof: Use sin(2A) = 2sin A cos A
```

### Example 6: Sum Formula - sin(A+B)

```
Mode: 1
Identity: sin(x+y)=sin(x)*cos(y)+cos(x)*sin(y)
Route: 1

Output:
1. sin(x+y)
= sin(x)*cos(y)+cos(x)*sin(y)

Proof: sin(A+B) = sin A cos B + cos A sin B
```

### Example 7: cos(A-B)

```
Mode: 1
Identity: cos(x-y)=cos(x)*cos(y)+sin(x)*sin(y)
Route: 1

Output:
1. cos(x-y)
= cos(x)*cos(y)+sin(x)*sin(y)

Proof: cos(A-B) = cos A cos B + sin A sin B
```

### Example 8: Power Reduction

```
Mode: 1
Identity: sin(x)^2=(1-cos(2*x))/2
Route: 1

Output:
1. sin(x)^2
= (1-cos(2*x))/2

Proof: sin^2 A = (1-cos 2A)/2
```

### Example 9: Sum to Product

```
Mode: 1
Identity: sin(x)+sin(y)=2*sin((x+y)/2)*cos((x-y)/2)
Route: 1

Output:
1. sin(x)+sin(y)
= 2*sin((x+y)/2)*cos((x-y)/2)

Proof: sin A + sin B = 2sin((A+B)/2)cos((A-B)/2)
```

### Example 10: Difference to Product

```
Mode: 1
Identity: cos(x)-cos(y)=-2*sin((x+y)/2)*sin((x-y)/2)
Route: 1

Output:
1. cos(x)-cos(y)
= -2*sin((x+y)/2)*sin((x-y)/2)

Proof: cos A - cos B = -2sin((A+B)/2)sin((A-B)/2)
```

## Mode 2: Transform Expression

Transform one expression to match another, showing the steps.

### Example 1: Transform to tan

```
Mode: 2
Eq 1: sin(x)/cos(x)
Eq 2: tan(x)

Output:
1. sin(x)/cos(x)
= tan(x)

Transformation: Rewrite sin(A)/cos(A) as tan(A).
```

### Example 2: Transform to sec

```
Mode: 2
Eq 1: 1/cos(x)
Eq 2: sec(x)

Output:
1. 1/cos(x)
= sec(x)

Transformation: Rewrite 1/cos(A) as sec(A).
```

### Example 3: Transform cos² - sin² to cos(2x)

```
Mode: 2
Eq 1: cos(x)^2-sin(x)^2
Eq 2: cos(2*x)

Output:
1. cos(x)^2-sin(x)^2
= cos(2*x)

Transformation: Use cos(2A) = cos^2 A - sin^2 A.
```

## Mode 3: Solve Equation

Solve trigonometric equations.

### Example 1: sin(x) = 0

```
Mode: 3
Solve: sin(x)=0,x

Output:
Solutions: x = n*pi
```

### Example 2: tan(x) = 1

```
Mode: 3
Solve: tan(x)=1,x

Output:
Solutions: x = pi/4 + n*pi
```

### Example 3: sin(x) = 1/2

```
Mode: 3
Solve: sin(x)=1/2,x

Output:
Solutions: x = pi/6 + 2*n*pi, 5*pi/6 + 2*n*pi
```

### Example 4: Solve within a range (start, end)

```
Mode: 3
Solve: sin(x)=1/2,x,0,180

Output:
Solutions: x = 30, 150
```

### Example 5: Solve using interval relation

```
Mode: 3
Solve: sin(x)=1/2,x,0<x<180

Output:
Solutions: x = 30, 150
```

### Example 6: Solve without specifying variable

```
Mode: 3
Solve: sin(x)=0

Output:
Solutions: x = n*pi
```

## Interval Syntax

| Syntax | Description | Example |
|--------|-------------|---------|
| `equation,x` | Solve with default span | `sin(x)=1/2,x` |
| `equation,x,start,end` | Solve in range | `sin(x)=1/2,x,0,180` |
| `equation,x,start<x<end` | Solve using interval relation | `sin(x)=1/2,x,0<x<pi` |

- **Degrees**: Use numbers like `0, 90` or `0< x < 90`
- **Radians**: Use `pi` like `0, pi` or `0 < x < pi`

### Example 7: Find Maximum Value

```
Mode: 3
Solve: 3*sin(x)+4*cos(x),max,x

Output:
Maximum value: 5
```

### Example 8: Find Minimum Value

```
Mode: 3
Solve: 3*sin(x)+4*cos(x),min,x

Output:
Minimum value: -5
```

## Supported Identities

### Pythagorean Identities
- `sin²A + cos²A = 1`
- `1 + tan²A = sec²A`
- `1 + cot²A = cosec²A`

### Reciprocal Identities
- `sec A = 1/cos A`
- `cosec A = 1/sin A`
- `cot A = cos A/sin A`
- `tan A = sin A/cos A`

### Double Angle Formulas
- `sin(2A) = 2sin A cos A`
- `cos(2A) = cos²A - sin²A`
- `tan(2A) = 2tan A/(1 - tan²A)`

### Sum/Difference Formulas
- `sin(A+B) = sin A cos B + cos A sin B`
- `sin(A-B) = sin A cos B - cos A sin B`
- `cos(A+B) = cos A cos B - sin A sin B`
- `cos(A-B) = cos A cos B + sin A sin B`
- `tan(A+B) = (tan A + tan B)/(1 - tan A tan B)`
- `tan(A-B) = (tan A - tan B)/(1 + tan A tan B)`

### Sum to Product
- `sin A + sin B = 2sin((A+B)/2)cos((A-B)/2)`
- `sin A - sin B = 2cos((A+B)/2)sin((A-B)/2)`
- `cos A + cos B = 2cos((A+B)/2)cos((A-B)/2)`
- `cos A - cos B = -2sin((A+B)/2)sin((A-B)/2)`

### Power Reduction
- `sin²A = (1 - cos(2A))/2`
- `cos²A = (1 + cos(2A))/2`

### Additional Identities
- `sec²A - tan²A = 1`
- `cosec²A - cot²A = 1`

## Error Handling

- Invalid input shows "Input error: [message]"
- Unsupported identities show "This trigonometric question is outside the supported A-level method set."
- Equations with no solutions are reported

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- Uses only `math` import (available on calculator)
- Optimized for A-level mathematics
- Includes caching for performance
- Can handle complex nested expressions
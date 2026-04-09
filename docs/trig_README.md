# Trigonometry Program (`trigProgram.py`)

A trigonometric manipulation program for the CASIO fx-cg50 calculator (MicroPython v1.9.4).

## Features

| Mode | Label shown | Feature | What it does |
|---|---|---|---|
| 1 | `prove` | Prove / show identity | Shows a supported trig identity route |
| 2 | `xform` | Transform | Transforms one expression or equation into another |
| 3 | `solve` | Solve | Solves trig equations and extrema-style max/min prompts |
| 4 | `rw` | Rewrite | Rewrites an expression or equation using only chosen trig terms |

## Input syntax

- Variables: single letters such as `x`, `y`, `A`
- Numbers: integers, decimals, fractions
- Constants: `pi`, `e`
- Trig functions: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- Inverse trig: `asin`, `acos`, `atan`, `arcsin`, `arccos`, `arctan`
- Other functions: `exp`, `log`, `ln`, `sqrt`, `abs`
- Operators: `+`, `-`, `*`, `/`, `^`, `**`
- Compact forms: `sin x`, `sec x`, `sin^2(x)`, `cos^2(x)`

## Notes

- Accepts both `^` and `**`
- Parser accepts many calculator-style no-parentheses forms
- Output is compacted toward short markscheme-style lines
- Expression-only transform inputs such as `sin(x)/(1+cos(x))` are supported
- CLI errors show `Err: ...`

## Mode 1: Prove / show identity (`prove`)

Enter an identity and a route:
- `1` auto
- `2` lhs
- `3` rhs
- `4` both

### Example

```text
M: 1
Id: sin(x)+sin(y)=2*sin((x+y)/2)*cos((x-y)/2)
R: 1
```

Output:

```text
2*sin((x+y)/2)*cos((x-y)/2)
Use 2sin A cos B = sin(A+B) + sin(A-B).
= sin((x+y)/2+(x-y)/2)+sin(-(x-y)/2+(x+y)/2)
= sin(x)+sin(y)
= LHS
```

## Mode 2: Transform (`xform`)

Transforms an expression or equation into a target form.

### Example 1: half-angle ratio route

```text
M: 2
E1: 3*(1-cos(x))/sin(x)
E2: 3*tan(x/2)
```

Output:

```text
(3*(1-cos(x)))/sin(x)
Use half-angle ratio identities.
= 3*tan(x/2)
```

### Example 2: core named trig route

```text
M: 2
E1: sin(x)/cos(x)
E2: tan(x)
```

Typical output shape:

```text
sin(x)/cos(x)
Use sin A / cos A = tan A.
= tan(x)
```

## Mode 3: Solve (`solve`)

Solves trig equations and also supports extrema prompts.

### Equation syntax

- `equation,var`
- `equation,var,start,end`
- `equation,var,a<x<b`

### Example 1: direct trig solve

```text
M: 3
Eq: sec(x)=2,x,0,2*pi
```

Output:

```text
Start with sec(x) = 2
Move all terms to one side
-2+sec(x) = 0
Solve the direct trig equation.
sec(x) = 2
x = [pi/3, (5*pi)/3]
```

### Example 2: quadratic in trig function

```text
M: 3
Eq: 2*sin(x)^2-3*sin(x)+1,x,0,2*pi
```

Output:

```text
Start with 1-3*sin(x)+2*sin(x)^2 = 0
Move all terms to one side
1-3*sin(x)+2*sin(x)^2 = 0
Let u = sin(x).
2*u^2-3*u+1 = 0
u = [1, 1/2]
sin(x) = 1
x = [pi/2] rad
sin(x) = 1/2
x = [pi/6, pi/2, (5*pi)/6]
```

### Example 3: extrema prompt

```text
M: 3
Eq: 3*sin(x)+4*cos(x),max,x
```

Typical result:

```text
Maximum value: 5
```

## Mode 4: Rewrite (`rw`)

Rewrites an expression or equation using only the listed target terms.

### Example

```text
M: 4
Rw: 1-cos(2*x)
T: sin(x)
T:
```

Output:

```text
Start with 1-cos(2*x)
Write in terms of sin(x) only.
Use 1 - cos(2A) = 2sin^2 A.
= 2*sin(x)^2
Final = 2*sin(x)^2
```

## Supported identity families

- reciprocal identities
- Pythagorean identities
- double-angle formulas
- triple-angle formulas (sin(3x), cos(3x), tan(3x))
- sum / difference formulas
- sum-to-product and product-to-sum routes
- power reduction
- half-angle ratio transforms such as
  - `sin(x)/(1+cos(x)) -> tan(x/2)`
  - `(1-cos(x))/sin(x) -> tan(x/2)`
  - `(1+cos(x))/sin(x) -> cot(x/2)`
  - `sin(x)/(1-cos(x)) -> cot(x/2)`

## Python API

```python
import sys
sys.path.insert(0, 'Math')
sys._trig_no_autorun = True
import trigProgram as tp

# Simplify expressions
result = tp.full_simplify(tp.parse('sin(x)^2+cos(x)^2'))
print(tp.show(result))  # 1

# Prove identities
lhs = tp.parse('sin(x)+sin(y)')
rhs = tp.parse('2*sin((x+y)/2)*cos((x-y)/2)')
result = tp.prove_general(lhs, rhs)
print('Prove works:', result is not None)

# Transform expressions  
result = tp.solve_transform_text('sin(x)/cos(x)', 'tan(x)')
print('Transform works:', result is not None)
```

## Error handling

- Invalid input shows `Err: ...`
- Unsupported routes return a short direct message
- Domain-restricted equalities are not silently treated as unrestricted identities
- No-solution cases are reported directly

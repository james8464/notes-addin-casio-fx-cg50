# SUVAT Program (`SUVATprogram.py`)

A kinematics solver for the CASIO fx-cg50 calculator (MicroPython v1.9.4). Solves SUVAT equations with exact rational arithmetic, symbolic working, and decimal approximations.

## Features

| Feature | Description |
|---|---|
| Exact arithmetic | All calculations use rational numbers — no floating-point rounding |
| Symbolic working | Shows the equation used, substitution, and simplified result |
| Decimal output | Approximate decimal shown alongside the exact answer |
| Surd simplification | `sqrt(8)` → `2*sqrt(2)`, `sqrt(72)` → `6*sqrt(2)`, etc. |
| Quadratic solver | Solves `s = ut + 1/2at^2` for `t` with both roots shown |
| Gravity constant | Enter `g` for 9.8 m/s² |
| Keyword presets | Detects phrases like `dropped`, `max height`, `returns to ground` |
| Multi-variable output | After solving, all five variables (s, u, v, a, t) are computed |
| Significant figures | Decimal output matches the precision of the input values |
| Physical validation | Rejects impossible scenarios (negative discriminant, a=0 with v≠u, etc.) |
| Units | Labels output with m, m/s, m/s², s |

## Input syntax

- Variables: `s`, `u`, `v`, `a`, `t`
- Numbers: integers, decimals, fractions
- Constants: `pi`, `e`, `g` (gravity = 9.8)
- Operators: `+`, `-`, `*`, `/`, `^`, `**`
- Functions: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`, `exp`, `log`, `ln`, `sqrt`, `abs`, `asin`, `acos`, `atan`
- Aliases: `arcsin`, `arccos`, `arctan`, `csc`
- Compact forms: `sin x`, `ln x`, `2x`, `3(x+1)`

## How to use

- Leave a field **blank** to mark it as unknown
- Enter **`,`** to explicitly mark a variable as the target
- Enter **expressions** like `12`, `3/2`, `g`, `sqrt(2)`, `9.8`
- Enter **keywords** anywhere in the input to trigger presets

## Keyword presets

| Keyword | Effect |
|---|---|
| `dropped`, `from rest`, `at rest`, `stationary` | Sets `u = 0` |
| `falls`, `free fall`, `gravity`, `thrown upwards`, `thrown up`, `projectile` | Sets `a = -9.8` |
| `maximum height`, `max height`, `highest point` | Sets `v = 0` |
| `returns to ground`, `back to start` | Sets `s = 0` |
| `comes to rest`, `stops` | Sets `v = 0` |

## SUVAT equations supported

All five standard equations, plus rearrangements:

| Equation | Variables needed |
|---|---|
| `s = ut + 1/2at²` | u, a, t |
| `s = vt - 1/2at²` | v, a, t |
| `s = 1/2(u+v)t` | u, v, t |
| `v = u + at` | u, a, t |
| `v² = u² + 2as` | u, a, s |
| `a = (v-u)/t` | v, u, t |
| `a = (v²-u²)/2s` | v, u, s |
| `t = (v-u)/a` | v, u, a |
| `t = 2s/(u+v)` | s, u, v |
| Quadratic for `t` from `s = ut + 1/2at²` | s, u, a |

## Notes

- Accepts both `^` and `**`
- Implicit multiplication is supported (`2x`, `3(x+1)`)
- At least 2 known values are required
- When solving for `t` with s, u, a the quadratic formula is used — both roots are shown if both are positive
- When solving for `v` with u, a, s the program checks that `u² + 2as ≥ 0`
- Division-by-zero edge cases are detected and reported
- CLI errors show `Err: ...`

## Example 1: basic displacement

```text
SUVAT Solver
Enter values (leave blank for unknown, use , for target)
Keywords: dropped, from rest, gravity, max height, returns to ground
Use g for gravity constant (9.8 m/s^2)

s: ,
u: 0
v:
a: 2
t: 3

= s = ut + 1/2at^2
s = ut + 1/2at^2
= s = 0*3 + 1/2*2*3^2
s = 9

--- All variables ---
s = 9 (9.0 m)
u = 0 (0 m/s)
v = 6 (6.0 m/s)
a = 2 (2.0 m/s^2)
t = 3 (3.0 s)
```

## Example 2: deceleration with keyword

```text
s: 36
u: 12
v: 0
a: ,
t:

= v^2 = u^2 + 2as
a = (v^2-u^2)/2s
= a = (0^2 - 12^2) / (2*36)
a = -2
a = -2.0 m/s^2

--- All variables ---
s = 36 (36.0 m)
u = 12 (12.0 m/s)
v = 0 (0 m/s)
a = -2 (-2.0 m/s^2)
t = 6 (6.0 s)
```

## Example 3: free fall with keyword preset

```text
s: ,
u: dropped
v:
a:
t: 4

= s = ut + 1/2at^2
s = ut + 1/2at^2
= s = 0*4 + 1/2*-9.8*4^2
s = -392/5
s = -78.4 m

--- All variables ---
s = -392/5 (-78.4 m)
u = 0 (0 m/s)
v = -196/5 (-39.2 m/s)
a = -49/5 (-9.8 m/s^2)
t = 4 (4.0 s)
```

## Example 4: quadratic time solution

```text
s: 20
u: 5
v:
a: 2
t: ,

= s = ut + 1/2at^2 (quadratic)
t = 2.5311288741 (positive root)

--- All variables ---
s = 20 (20.0 m)
u = 5 (5.0 m/s)
v = 141/10 (14.1231056256 m/s)
a = 2 (2.0 m/s^2)
t = 2.5311288741 (2.5311288741 s)
```

## Example 5: surd simplification

```text
s: ,
u: 0
v:
a: 3
t: sqrt(2)

= s = ut + 1/2at^2
s = ut + 1/2at^2
= s = 0*sqrt(2) + 1/2*3*sqrt(2)^2
s = 3

--- All variables ---
s = 3 (3.0 m)
u = 0 (0 m/s)
v = 3*sqrt(2) (4.2426406871 m/s)
a = 3 (3.0 m/s^2)
t = sqrt(2) (1.4142135624 s)
```

## Example 6: using g constant

```text
s: ,
u: 20
v: 0
a: -g
t: ,

= v^2 = u^2 + 2as
s = 1/2(u+v)t
= s = 1/2*(20 + 0)*20/g
s = 200/g

--- All variables ---
s = 200/g (20.4081632653 m)
u = 20 (20.0 m/s)
v = 0 (0 m/s)
a = -g (-9.8 m/s^2)
t = 20/g (2.0408163265 s)
```

## Error handling

- **No target specified**: `Error: No target variable specified. Use , to mark the unknown.`
- **Too few knowns**: `Error: At least 2 known values are required.`
- **Insufficient information**: `Error: insufficient information`
- **Negative discriminant**: `Error: No real solution (u^2 + 2as = -4 is negative).`
- **Division by zero**: `Error: No solution: a=0 but v!=u.`
- **Invalid input**: `Err: ...`

## Python API

```python
import sys
sys.path.insert(0, 'Math')
sys._suvat_no_autorun = True
import SUVATprogram as sp

# Parse a value
node = sp.parse('12')
print(sp.show(node))  # 12

# Parse expressions
node = sp.parse('sqrt(2)')
print(sp.show(node))  # sqrt(2)

# Parse a raw value (for numeric inputs)
node = sp.parse_value('3.14')
print(sp.show(node))  # 157/50

# Build a solution directly
result, eq, orig, sub = sp.build_suvat_solution(
    sp.num(36),   # s
    sp.num(12),   # u
    sp.num(0),    # v
    None,         # a (unknown)
    None,         # t (unknown)
    'a'           # target
)
print(eq)   # a = (v^2-u^2)/2s
print(sub)  # a = (0^2 - 12^2) / (2*36)
print(result)  # -2

# Detect keyword presets
presets = sp.detect_presets('dropped from a height')
print(presets)  # {'u': ('num', 0, 1)}

# Surd simplification
print(sp.simplify_surd(72))  # (6, 2)  → 6*sqrt(2)
print(sp.simplify_surd(8))   # (2, 2)  → 2*sqrt(2)

# Decimal formatting
print(sp.format_decimal(3.14159, 3))  # 3.14
print(sp.format_decimal(0.000123, 3))  # 0.000123
```

# SUVAT Program (`SUVATprogram.py`)

Exact-symbolic SUVAT solver for the CASIO menu flow.

## CLI Flow

The solver prints:

- `Use , to mark exactly one target and enter the other known values.`
- then prompts:
  - `s:`
  - `u:`
  - `v:`
  - `a:`
  - `t:`

Rules:

- exactly one unknown target is required
- unknown can be marked by blank, or explicitly by entering `,`
- if multiple unknowns exist and no explicit target is set, input is rejected

## Features

| Feature | Description |
|---|---|
| Exact arithmetic | Rational-first calculations |
| Symbolic working | Prints chosen equation + substitution + answer |
| Decimal companion | Decimal line shown when meaningful |
| Surd handling | Simplifies roots where possible |
| Gravity token | `g` / `G` treated as 9.8 (`49/5`) |
| Keyword presets | Text cues auto-fill common kinematics assumptions |
| Quadratic-time branch | Uses quadratic form for `t` from `s,u,a` |
| Two-root handling | Keeps mathematically valid roots and flags unphysical negative-time cases |
| Units | `m`, `m/s`, `m/s^2`, `s` |
| Significant-figures aware output | Decimal precision follows input precision |

## Input syntax

- Variables: `s`, `u`, `v`, `a`, `t`
- Numbers: integers, decimals, fractions
- Constants: `pi`, `e`, `g` (gravity = 9.8)
- Operators: `+`, `-`, `*`, `/`, `^`, `**`
- Functions: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`, `exp`, `log`, `ln`, `sqrt`, `abs`, `asin`, `acos`, `atan`
- Aliases: `arcsin`, `arccos`, `arctan`, `csc`
- Compact forms: `sin x`, `ln x`, `2x`, `3(x+1)`

## Input Notes

- accepted values include expressions (`12`, `3/2`, `sqrt(2)`, `g`, etc.)
- parser supports implicit multiplication and standard operators
- text across all fields is scanned for keyword presets

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

## Output Behavior Details

- if a full consistent set is available, `--- All variables ---` shows all five values
- if more than one variable remains unknown (besides target), that section is intentionally suppressed with an explanation
- exact and decimal lines are both printed when decimal conversion is valid
- symbolic `g` can be preserved in exact output; decimal shown separately where appropriate

## Notes

- accepts both `^` and `**`
- division-by-zero and impossible-physics branches are explicitly trapped
- CLI errors use `Err: ...`

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

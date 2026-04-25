# SUVAT Solver

Solves SUVAT equations (kinematics for constant acceleration).

## Input Format

Five values: **s** (displacement), **u** (initial velocity), **v** (final velocity), **a** (acceleration), **t** (time)

Use `,` to mark exactly **one variable** as the target to solve for. Leave other known values blank or enter numbers.

### Numbers
- Integers: `10`, `-5`
- Decimals: `9.8`, `0.5`
- Scientific e-notation: `6e3` (= 6000), `1.5e-2` (= 0.015)

### Keywords (auto-detected presets)
Enter anywhere in input to auto-fill known values:
- `gravity`, `g`, `freefall` → a = 9.8
- `stop`, `stopping`, `brake` → v = 0
- `vertical`, `up`, `down` → a = -9.8 or a = 9.8 with appropriate direction

### Example Inputs

```
s: ,         u: 10       v:          a: 9.8     t:          → Find s with u=10, a=9.8, find t first
s: 100       u: 20       v:          a:          t: 5        → Find v with s=100, u=20, t=5
s:           u: 0        v: 0        a: -9.8    t:           → Find t for gravity: u=0, v=0, a=-9.8
s: 0         u: 20       v:          a: -9.8    t:          → Find t for vertical: u=20 up, a=-9.8
```

## Output

Shows exact symbolic answer and decimal approximation. May show multiple solutions (or).

Presenting all five SUVAT variables in a consistent set when exactly one was solved for.

## Formulae Used

```
v = u + at
s = ut + 0.5at²
s = 0.5(u + v)t
v² = u² + 2as
s = vt - 0.5at²
```
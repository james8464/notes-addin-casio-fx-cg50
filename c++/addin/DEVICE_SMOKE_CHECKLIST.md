# On-device smoke checklist (fx-CG50)

Run this after building `CasioCAS.g3a` with `fxsdk build-cg`.

## Preconditions

- Copy the `.g3a` to calculator storage and safely eject.
- Reboot calculator (recommended) before the first run after installing.

## Global checks

- Launch add-in from the main menu.
- Navigate menus with `UP/DOWN`, enter with `EXE`, back with `EXIT`.
- Enter a long expression and verify the input widget:
  - `LEFT/RIGHT` moves cursor
  - `DEL` deletes at cursor
  - insertion works mid-string
  - long strings show ellipsis and cursor indicator
- Run each module at least once; ensure no crash/hang.

## Boolean

- Simplify: `~(p&q)` (or your supported boolean syntax)
- NAND/NOR modes: verify outputs are not empty and no crash

## SUVAT

- Solve for `v` with known `u,a,t`
- Solve-all with one blank (unknown) and verify all variables print
- Error handling: multiple blanks or no target should show a clear error

## Derive

- Mode 1: `x^3` (expect `3x^2`)
- Mode 2: `x^2+y^2=1` (expect `-x/y`)
- Mode 3: `x=t^2, y=t^3` (expect `3t/2`)
- Mode 4: `x^4` (expect second derivative `12x^2`)

## Integrate

- `x^2` (expect `x^3/3 + C`)
- `1/x` (expect `ln|x| + C`)
- `sin(3x+2)` (expect `-1/3 cos(3x+2) + C`)

## Algebra

- Solve: `x^2-5x+6=0` (roots)
- Expand: `(2x+1)^5` (binomial output)
- Complete square: `x^2+6x+5`

## Trig

- Exact values: `sin(30)`, `cos(5*pi/3)`
- Solve: `sin(x)=1/2,x,0,360`

## Performance sanity

- Re-run a few operations repeatedly; verify responsiveness doesn’t degrade.


# On-device smoke checklist (fx-CG50)

Run this after building `CasioCAS.g3a` with `fxsdk build-cg`.

## Preconditions

- Copy the `.g3a` to calculator storage and safely eject.
- Reboot calculator (recommended) before the first run after installing.

## Global checks

- Launch add-in from the main menu.
- Navigate menus with `UP/DOWN`, enter with `EXE`, back with `EXIT`.
- Enter a long expression and verify the input widget:
  - digits/operators append at the end
  - `DEL` deletes the last character
  - `AC/ON` clears the buffer
  - long strings show a clipped prefix marker
- Run each module at least once and ensure no crash/hang.

## Simplify

- Input: `2x+3-x+4`
- Expected: answer line equivalent to `x + 7`

## Algebra

- Input: `2x+3=7`
- Expected working:
  - collect x terms
  - divide both sides
  - answer `x = 2`
- Input: `x^2-5x+6=0`
- Expected working:
  - rearranged quadratic
  - discriminant line
  - answer `x = 2 or x = 3`

## SUVAT

- Input: `s=10,u=0,v=?,a=2,t=5`
- Expected: use `v = u + at`, answer `v = 10`

## Derive

- Input: `3x^2+2x+1`
- Expected: power rule line, answer `6x + 2`

## Integrate

- Input: `3x^2+2x+1`
- Expected: power-rule antiderivative line, answer `x^3 + x^2 + x + C`

## Trig

- Input: `sin(30)`
- Expected: unit-circle table line, answer `1/2`

## Unsupported inputs

- Try `x^6` in Derive.
- Try `1/x` in Integrate.
- Try `sin(17)` in Trig.
- Expected: clear unsupported message, no crash/hang.

## Performance sanity

- Re-run a few operations repeatedly; verify responsiveness doesn’t degrade.

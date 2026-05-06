# On-device smoke checklist (fx-CG50)

Run this after building calculator files with `./compile`.

## Preconditions

- Copy `calculator_files/CasioCAS.g3a` and `calculator_files/CASIOCAS.PAK` to calculator storage root and safely eject.
- If F6 help says `Copy CASIOCAS.PAK`, the support file is missing or not in root.
- Reboot calculator (recommended) before the first run after installing.

## Global checks

- Launch add-in from the main menu.
- Verify the main-menu tile has the `CasioCAS` name and a visible icon, not a blank black square.
- Verify the add-in uses the KhiCAS-style dense UI: status strip, blue section label, black selected row, right scrollbar, and boxed soft keys.
- Navigate menus with `UP/DOWN`, enter with `EXE`, back with `EXIT`.
- From the home screen, verify quick launches: `F1` Shell, `F2` Algebra, `F3` Derive, `F4` Integrate, `F5` Trig, `F6` SUVAT.
- Enter a long expression and verify the input widget:
  - digits/operators append at the end
  - `DEL` deletes the last character
  - `AC/ON` clears the buffer
  - long strings show a clipped prefix marker
- Run a result with a long worked line and verify the viewer wraps it into readable chunks.
- In the output viewer, verify `UP/DOWN` scroll one row and `LEFT/RIGHT` page through the working.
- Run each module at least once and ensure no crash/hang.

## Simplify

- Input: `2x+3-x+4`
- Expected: answer line equivalent to `x + 7`
- Input: `x^2+2x+x^2`
- Expected: answer line equivalent to `2x^2 + 2x`

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
- Input: `x^2-2=0`
- Expected: exact surd-form quadratic roots, not a host-only or unsupported message

## SUVAT

- Input: `s=10,u=0,v=?,a=2,t=5`
- Expected: use `v = u + at`, answer `v = 10`
- Input: `s=10,u=0,v=4,a=?,t=5`
- Expected: rearrange `v = u + at`, answer `a = 4/5`
- Input: `s=10,u=?,v=4,t=5`
- Expected: use average velocity form, answer `u = 0`

## Derive

- Input: `3x^2+2x+1`
- Expected: power rule line, answer `6x + 2`

## Integrate

- Input: `3x^2+2x+1`
- Expected: power-rule antiderivative line, answer `x^3 + x^2 + x + C`

## Trig

- Input: `sin(30)`
- Expected: unit-circle table line, answer `1/2`
- Input: `cos(5*pi/3)`
- Expected: unit-circle table line, answer `1/2`

## Unsupported inputs

- Try malformed input such as `solve(`.
- Try non-elementary syllabus-stress input such as `integrate(exp(-x^2),x)`.
- Try non-elementary syllabus-stress input such as `integrate(sin(x^2),x)`.
- Try `sin(17)` in Trig.
- Expected: clear unsupported or verified special-function message, no crash/hang.

## Performance sanity

- Re-run a few operations repeatedly; verify responsiveness doesn’t degrade.

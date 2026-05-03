# Feature Parity

```mermaid
graph TD
  Py["old Python programs"] --> Work["exam working layer"]
  OldCpp["old C++ native port"] --> Work
  Khi["upstream KhiCAS answer engine"] --> App["current .g3a"]
  Work --> Hook["needed source hook"]
  Hook --> Future["KhiCAS UI + working lines"]
```

## Current State

- Active `.g3a`: upstream KhiCAS UI/engine, patched with Eigenmath-style icons.
- Old working layer: preserved in `c++/addin/src/device/device_solver.cpp` and `c++/legacy/addin/src/modules`.
- Gap: active `.g3a` is a reference binary, so it cannot yet inject old working lines or old custom aliases at runtime.

## Old Feature Coverage

| Old feature group | Current KhiCAS equivalent | Working lines status |
|---|---|---|
| simplify/factor/expand/partial fractions | `simplify`, `factor`, `texpand`, `partfrac` | old layer preserved |
| solve equations | `solve`, `fsolve`, `csolve`, `linsolve` | old layer preserved |
| compare/transform/match/rewrite | old custom layer | needs source hook |
| complete square | old custom layer | needs source hook |
| compose/inverse/domain/range | old custom layer + KhiCAS primitives | needs source hook |
| cartesian/parametric conversion | old custom layer + plotparam | needs source hook |
| fit constants | old custom layer | needs source hook |
| normal/implicit/param/second derivative | `diff` + old layer | needs source hook |
| integration/DE/param area | `integrate`, `desolve`, `rsolve` + old layer | needs source hook |
| trig prove/transform/solve/rewrite | KhiCAS trig functions + old layer | needs source hook |
| SUVAT | old custom layer only | needs source hook |
| boolean simplify/NAND/NOR/prove | old custom layer only | needs source hook |
| stats/probability/regression | KhiCAS stats/proba + old layer | old layer preserved |
| matrices/vectors | KhiCAS matrix/linalg | old layer preserved |
| integer arithmetic | KhiCAS arithmetic | old layer preserved |

## Output Rule

Target behaviour:

```text
1. Normalize/input rewrite
2. Method choice
3. Algebraic steps
Answer: simplest exact form
```

Simplification examples:

- `6*3*5x` -> `90*x`
- duplicate answer-only lines rejected
- exact/factorised form preferred before decimal

## Remove Candidates

Likely unnecessary for A-level/Further Maths:

- Turtle/Logo commands: `avance`, `recule`, `tourne_*`, `crayon`, `efface`, `rectangle_plein`.
- Programming language helpers: `for`, `while`, `if`, `local`, `return`, `debug`, `python`, `python_compat`.
- Low-level drawing commands: `draw_pixel`, `draw_string`, `draw_rectangle`, `draw_polygon`, `rgb`, display colour tokens.
- Advanced university CAS: `laplace`, `ilaplace`, `fourier_an/bn/cn`, `jordan`, `svd`, `gramschmidt`, `cond`, `resultant`.
- Random/data generators: `ranv`, `ranm`, `rand`, `randint` unless stats simulation is wanted.
- Complex-only extras: `residue`, `cfactor`, `cpartfrac` unless complex roots are required.

Keep unless you say otherwise:

- core algebra/solve/calculus/trig
- matrix/vector basics
- stats/probability/regression/plots
- exact arithmetic/number theory basics

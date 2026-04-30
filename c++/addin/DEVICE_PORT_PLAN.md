# Device Port Plan

## Question Being Answered

Would a device-safe rewrite help each part of the program work more reliably on the CG50, and could it still show full exam-mark-scheme-style working?

## Short Answer

Yes, with one important constraint:

- On the calculator, each feature should be designed to work within **explicit limits**:
  - maximum input length
  - maximum parse tree size
  - maximum number of intermediate steps
  - maximum number of output lines
- Within those limits, the program can still produce **full working-out lines** at exam-mark-scheme level of detail.

The key idea is that detailed working does **not** require STL, exceptions, or hosted C++ facilities. It requires:

- a structured representation of each algebraic step
- deterministic rewrite rules
- fixed-capacity storage for expressions, steps, and rendered lines

## Current Implementation Status

The device build now includes the first bounded solver slice:

- `c++/addin/src/device/fixed_string.hpp`
- `c++/addin/src/device/line_buffer.hpp`
- `c++/addin/src/device/device_solver.hpp`
- `c++/addin/src/device/device_solver.cpp`
- `c++/addin/src/ui/text_input_device.cpp`

Supported on-device workflows:

- linear simplification such as `2x+3-x+4`
- linear equation solving such as `2x+3=7`
- polynomial derivatives up to `x^5`
- polynomial integrals up to `x^5`
- exact trig values for common degree angles
- selected SUVAT rearrangements

The host build also compiles and runs `device_solver_smoke` so this bounded device slice is tested without requiring the calculator toolchain for every iteration.

## Why This Change Helps

The current host-oriented implementation assumes a richer C++ runtime than the fxSDK/gint target provides. That creates build and reliability problems on-device.

A device-safe rewrite would give us:

1. **Predictable behaviour on the calculator**
   - no hidden heap-heavy STL behaviour
   - no unsupported exception paths
   - no reliance on hosted-only headers

2. **Bounded memory usage**
   - fixed buffers for text
   - fixed-capacity arrays for tokens, AST nodes, and step lists
   - explicit failure states when a limit is exceeded

3. **Cleaner separation between host and device**
   - host build can stay expressive and convenient
   - device build can stay strict, small, and reliable

4. **More controllable exam output**
   - steps become a first-class data model
   - rendering can be tuned for CG50 screen constraints
   - line pagination becomes deliberate instead of incidental

## What "Works Regardless of Input" Should Mean

It should **not** mean "accept infinite complexity."

On the device, the right goal is:

> For any supported input that fits within declared limits, the program either:
> - solves it and shows working, or
> - returns a clear, user-facing reason why it cannot continue.

Examples of acceptable device outcomes:

- `Solved with full working`
- `Input too long for device mode`
- `Expression too complex for current memory budget`
- `Too many working steps to display safely`
- `Feature not yet supported on calculator`

That is much better than silent truncation, crashes, or toolchain-dependent behaviour.

## Can We Still Show Exam-Mark-Scheme-Level Working?

Yes.

The amount of working shown is a **product design and engine design choice**, not a consequence of using STL.

To preserve high-detail working on-device, we should model each solution as:

1. parsed expression / equation
2. normalized form
3. chosen method
4. ordered step records
5. final answer line

Each step record should contain fields like:

- `rule_id`
- `before_expr`
- `after_expr`
- `reason_text`
- `display_text`

Then the device renderer simply paginates these lines across the CG50 screen.

## Core Design Principle

Keep the **reasoning model** independent from the **storage model**.

- Host side:
  - can use STL freely
  - useful for rapid development, testing, and reference outputs
- Device side:
  - uses fixed-capacity containers and string buffers
  - preserves the same logical solving pipeline where practical

This lets us keep rich working while swapping the underlying memory strategy.

## Proposed Architecture

### 1. Split Host and Device Builds Explicitly

Introduce a clear build split:

- `CASIO_HOST`
  - current richer C++ environment
  - STL allowed
  - convenient for golden tests and iteration
- `CASIO_DEVICE`
  - freestanding-safe
  - no exceptions
  - no hosted-only STL dependencies

### 2. Add Device Utility Types

Create a small device support layer, for example under `c++/addin/src/device/`:

- `fixed_string<N>`
- `fixed_vector<T, N>`
- `status_or<T>` or `result<T>`
- `step_buffer<N>`
- `line_buffer<N>`

These should support only the operations we actually need.

### 3. Introduce a Step Model for Working

Define a compact step type, for example:

- step rule / category
- input node ids
- output node ids
- short rationale text id
- rendered line buffer

This becomes the source of truth for:

- exam working
- shell output
- paged display on-device

### 4. Port by Module, Not All at Once

Move one feature at a time to the device-safe layer:

1. Simplify / normalization
2. Basic algebra solve
3. Derivatives
4. Integrals
5. Trig
6. SUVAT
7. Boolean, if still wanted on-device

### 5. Keep Boolean Disabled Until Justified

Boolean currently brings in extra complexity and is not required for the immediate device milestone.

Recommended approach:

- keep boolean available in host tooling if useful
- leave it out of the device target until the rest of the solver pipeline is stable

## Phased Implementation Plan

### Phase 0: Stabilize the Device Shell

Goal:
- keep a buildable `.g3a`
- keep menu and paging infrastructure simple

Deliverables:
- freestanding-safe home screen
- paged line viewer
- explicit "host-only for now" module placeholders

### Phase 1: Build the Device Support Layer

Goal:
- provide the basic data structures needed by parser and solver code

Deliverables:
- fixed string type
- fixed vector type
- error/status type
- compile-time capacity constants

Acceptance:
- no STL containers/strings required in device-only code paths

### Phase 2: Port Parsing and Rendering Core

Goal:
- parse supported input forms into bounded AST structures
- render expressions back to display lines

Deliverables:
- bounded tokenizer
- bounded parser
- bounded expression formatter
- overflow/error reporting

Acceptance:
- supported expressions parse and render identically enough to host references

### Phase 3: Port Step Recording

Goal:
- make working-out explicit and renderable

Deliverables:
- `StepRecord` type
- fixed-capacity step list
- renderer from step list to paged output lines

Acceptance:
- one solved example can show multiple lines of working on-device

### Phase 4: Port Simplify and Algebra First

Goal:
- ship the most generally useful symbolic workflow first

Deliverables:
- normalize
- simplify
- linear equation solving
- exam-style working for those paths

Acceptance:
- representative algebra cases produce final answers and ordered working lines

### Phase 5: Port Derive and Integrate

Goal:
- extend symbolic support without breaking device constraints

Deliverables:
- derivative rules
- selected integral rules
- step annotations for method and substeps

Acceptance:
- supported examples show rule-by-rule working

### Phase 6: Port Trig and SUVAT

Goal:
- cover the remaining educational workflows

Deliverables:
- bounded trig solving/evaluation paths
- SUVAT form entry and working output

Acceptance:
- each feature fails clearly when input exceeds limits

### Phase 7: Reassess Boolean

Goal:
- decide whether boolean belongs in the calculator product at all

Options:
- keep host-only
- add a trimmed device version
- leave out permanently if usage is low

## Testing Strategy

### Host Reference Tests

Use the host build and Python oracle to generate expected:

- final answers
- working line sequences
- error messages

### Device Compatibility Tests

For every ported module, test:

- shortest valid input
- typical exam input
- near-limit input
- overflow / too-complex input
- unsupported syntax

### Golden Output Checks

For selected examples, compare:

- host final answer vs device final answer
- host working line count vs device working line count
- host wording vs device wording where practical

Exact wording can differ a little. Mathematical intent and ordering should match.

## Product Rules for Exam-Level Working

To keep working detailed enough for study and mark-scheme use, adopt these rules:

1. Never skip directly from input to answer when an intermediate teaching step matters.
2. Record the rule used at each transformation.
3. Prefer more short lines over one dense line.
4. Show substitutions explicitly.
5. Show domain or validity notes where they affect the answer.
6. If output is too long, paginate it; do not silently compress it away.

## Risks

1. **Capacity tuning**
   - limits set too low will reject legitimate school-level inputs
   - limits set too high may hurt memory or performance

2. **Porting drift**
   - device and host logic may diverge if we duplicate rules carelessly

3. **Rendering pressure**
   - exam-detail output can be verbose on a small calculator screen

4. **Scope creep**
   - attempting every module at once will slow progress

## Recommended Next Steps

1. Keep the current buildable `.g3a` as the baseline.
2. Add a small device support layer.
3. Port one narrow vertical slice end-to-end:
   - input
   - parse
   - simplify/algebra
   - step recording
   - paged working display
4. Validate that the calculator can show full working for that slice.
5. Expand module-by-module from there.

## Success Criteria

This plan is successful when:

- `fxsdk build-cg` stays green
- device code does not rely on hosted STL/exceptions
- supported calculator inputs behave deterministically
- unsupported or oversized inputs fail clearly
- calculator output still shows step-by-step working at exam-useful detail

# Trig Calculator

Trigonometric identity solver with prove, transform, solve, and rewrite modes.

## Menu Modes

### 1. Prove (Mode 1)
Prove two expressions are equivalent: E1 = E2

**Input:**
- E1: First expression
- E2: Target expression
- R: Route (auto/lhs/rhs/both), controls which side to transform

**Examples:**
```
E1: sin(2x)         E2: 2sin x cos x        R: 1   → Prove sin(2x) = 2sin x cos x
E1: tan(x)+tan(y)   E2: sin(x+y)/cos x cos y R: 2   → Prove via lhs transformation
E1: cos(2x)         E2: 1-2sin^2(x)        R: 3   → Prove via rhs transformation
```

### 2. Transform (Mode 2)
Show step-by-step transformation from one form to another.

**Input:**
- E1: Starting expression
- E2: Target expression

**Examples:**
```
E1: sin(x)+sin(y)   E2: 2sin((x+y)/2)cos((x-y)/2)  → Sum-to-product
E1: 2sin x cos x    E2: sin(2x)                      → Product-to-sum (reverse)
```

### 3. Solve (Mode 3)
Solve trigonometric equation for x.

**Input:**
- Eq: Equation (should contain "=" for solution, or just expression=0 form)

**Examples:**
```
Eq: sin(x) = 0                    → Find all solutions for sin x = 0
Eq: tan(x) = 1                    → Solve tan x = 1
Eq: 2sin(x)cos(x) = sin(2x)      → Verify identity
```

### 4. Rewrite (Mode 4)
Rewrite an expression using specific terms, one per line.

**Input:**
- Rw: Expression to rewrite
- T: (repeated) Transformation terms/identities to apply

**Common terms:**
- `tan` - express in terms of tan
- `sec` - express in terms of sec
- `half` - use half-angle formulas
- `square` - use sin², cos² forms
- `radian` - convert to radian measure

## Input Syntax

### Numbers
- Integers: `10`, `-5`
- Decimals: `9.8`, `0.5`
- Fractions: `1/2`, `3/4`
- Scientific: `6e3` (= 6000), `1.5e-2` (= 0.015)

### Variables
- Single: `x`, `y`, `z`, `t`, `theta`
- Greek: `alpha`, `beta`, `gamma`, `phi`, `theta`

### Operators
- `+`, `-` : addition, subtraction
- `*` or space: multiplication (both `2*x` and `2x` work)
- `/` : division
- `^` or `**` : power (both `x^2` and `x**2` work)
- `()` : grouping

### Trigonometric Functions
- Primary: `sin`, `cos`, `tan`, `sec`, `cosec`, `cot`
- Inverse: `asin`, `acos`, `atan`, `arcsin`, `arccos`, `arctan`
- Hyperbolic: `sinh`, `cosh`, `tanh`
- Other: `exp`, `ln`, `log`, `sqrt`, `abs`

### Function Aliases
- `csc` → `cosec`
- `ln` → `log`
- `arcsin` → `asin`
- `arccos` → `acos`
- `arctan` → `atan`

### Constants
- `e` - Euler's number (~2.718)
- `pi` - π (~3.14159)

## Expression Examples

### Basic trig
```
sin(x)
cos(2x + pi/3)
tan(x/2)
```

### With powers
```
sin^2(x)        → (sin(x))^2
cos^2(x) + sin^2(x)        → equals 1
tan^2(x) + 1
```

### Compound angles
```
sin(x + y)
cos(x - y)
tan(2x)
```

### With constants
```
sin(pi/2)
cos(2*pi)
tan(pi/4)
exp(x)          → e^x
sqrt(1 + x^2)
```

### Equations
```
sin(x) = 0
cos(x) = 1
tan(x) = sqrt(3)
sin(x) = cos(x)
2*sin(x)*cos(x) = sin(2x)
```

### Complex expressions
```
(1 + tan(x)^2) = sec(x)^2
sin(x + y) = sin(x)cos(y) + cos(x)sin(y)
cos(2x) = 2cos^2(x) - 1 = 1 - 2sin^2(x)
```

## Supported Identities

### Double-angle
- sin(2x) = 2sin x cos x
- cos(2x) = cos²x - sin²x = 2cos²x - 1 = 1 - 2sin²x
- tan(2x) = 2tan x / (1 - tan²x)

### Half-angle
- sin²(x) = (1 - cos(2x))/2
- cos²(x) = (1 + cos(2x))/2
- tan²(x) = (1 - cos(2x))/(1 + cos(2x))

### Sum/difference
- sin(x±y) = sin x cos y ± cos x sin y
- cos(x±y) = cos x cos y ∓ sin x sin y
- tan(x±y) = (tan x ± tan y)/(1 ∓ tan x tan y)

### Product-to-sum
- 2sin x cos y = sin(x+y) + sin(x-y)
- 2cos x cos y = cos(x-y) + cos(x+y)
- 2sin x sin y = cos(x-y) - cos(x+y)

### Pythagorean
- sin²x + cos²x = 1
- 1 + tan²x = sec²x
- 1 + cot²x = csc²x
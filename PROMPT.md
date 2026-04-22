# CASIO Comprehensive Enhancement Prompt

## Overview

This is the FINAL and DEFINITIVE prompt to transform the CASIO fx-cg50 math program into its ultimate form - capable of tackling ANY question using ALL available methods in the most mathematically rigorous and exam-appropriate way.

## Current State Analysis

### Identified Issues

1. **Shortcut Derivatives**: The program uses "cot rule", "sec rule", "cosec rule" shortcuts instead of showing proper derivation using quotient rule after converting to sin/cos. This is NOT exam-appropriate.

2. **Output Echoing**: The program echoes user inputs back in output (e.g., "Source = ...", "Target = ...", "Original: ...") which is redundant and not exam-appropriate.

3. **Transform/Xform Failures**: The transform mode fails to transform rational trig expressions to target forms (e.g., sin/cos to tan/sec).

4. **Unsupported Equation Types**: Many equation types from the 100-question test suite are not supported.

5. **Output Formatting**: Various formatting issues exist (duplicate lines, verbose integration, etc.).

6. **Test Coverage**: Need to achieve 100% on /random 9999 runs.

---

## PART 1: CRITICAL FIXES

### Fix 1.1: Remove Derivative Shortcuts (EXAM QUALITY)

**Location:** `src/Math/deriveProgram.py` around lines 1584-1591

**Problem:** Program shows "Using sec rule", "Using cosec rule", "Using cot rule" instead of proper derivation.

**Required Change:**
- Remove all "Using X rule" shortcuts for sec, cosec, cot
- Instead, always convert to sin/cos form and apply quotient rule
- Show the full working: sec(x) = 1/cos(x), then d/dx[1/cos(x)] = sin(x)/cos²(x)

**Example Output for d/dx[cot(x)]:**
```
Method: Convert to sin/cos
cot(x) = cos(x)/sin(x)
Using quotient rule: d/dx[u/v] = (u'v - uv')/v²
u = cos(x), v = sin(x)
u' = -sin(x), v' = cos(x)
dy/dx = (-sin(x)*sin(x) - cos(x)*cos(x))/sin²(x)
= -(sin²(x) + cos²(x))/sin²(x)
= -1/sin²(x)
= -csc²(x)
Answer: -csc²(x)
```

**Example Output for d/dx[sec(x)]:**
```
Method: Convert to sin/cos
sec(x) = 1/cos(x)
Using quotient rule: d/dx[u/v] = (u'v - uv')/v²
u = 1, v = cos(x)
u' = 0, v' = -sin(x)
dy/dx = (0*cos(x) - 1*(-sin(x)))/cos²(x)
= sin(x)/cos²(x)
= tan(x)*sec(x)
Answer: tan(x)*sec(x)
```

**Example Output for d/dx[cosec(x)]:**
```
Method: Convert to sin/cos
cosec(x) = 1/sin(x)
Using quotient rule: d/dx[u/v] = (u'v - uv')/v²
u = 1, v = sin(x)
u' = 0, v' = cos(x)
dy/dx = (0*sin(x) - 1*cos(x))/sin²(x)
= -cos(x)/sin²(x)
= -cot(x)*cosec(x)
Answer: -cot(x)*cosec(x)
```

---

### Fix 1.2: Remove Echoed User Inputs from Output

**Location:** `src/Math/algebraProgram.py` (xform/transform mode)

**Problem:** When user enters inputs, the program echoes them back in the output. This creates redundant lines like:
```
1. Source = (3*(cos(x))^2 + 3*(sin(x))^2 + 2*sin(x) - 2*cos(x))/((cos(x))^2 + 2*cos(x)*sin(x) + (sin(x))^2)
2. Target = (a*tan(x) + b*sec(x) + c)/(sec(x) + 2*sin(x))
3. Original: (3*(cos(x))^2 + 3*(sin(x))^2 + 2*sin(x) - 2*cos(x))/((cos(x))^2 + 2*cos(x)*sin(x) + (sin(x))^2)
```

**Required Change:**
- Remove all echoed user input lines from the output
- Only show the actual working and result
- For xform/transform mode, show "Transform:" followed by the result, not the entire input echoing

**Example of current bad output:**
```
1 cmp
2 xform
3 exp
4 poly
M 1/3 n/p: 2
Src: (3*(cos(x))^2 + 3*(sin(x))^2 + 2*sin(x) - 2*cos(x))/((cos(x))^2 + 2*cos(x)*sin(x) + (sin(x))^2)
Tgt: (a*tan(x)+b*sec(x)+c)/(sec(x)+2*sin(x))
1. Source = (3*(cos(x))^2 + 3*(sin(x))^2 + 2*sin(x) - 2*cos(x))/...
2. Target = (a*tan(x) + b*sec(x) + c)/(sec(x) + 2*sin(x))
3. Original: (3*(cos(x))^2 + 3*(sin(x))^2 + 2*sin(x) - 2*cos(x))/...
```

**Should be:**
```
Transform: source to target form
Method: Express in tan/sec basis
...
Answer: (3*tan(x) + 1)/(sec(x) + 2*sin(x)) (or whatever the correct form is)
```

---

### Fix 1.3: Fix Transform/Xform Mode Failures

**Location:** `src/Math/algebraProgram.py` (xform mode)

**Problem:** The xform mode fails to transform rational trig expressions to specific target forms.

**Example that failed:**
- Source: `(3*(cos(x))^2 + 3*(sin(x))^2 + 2*sin(x) - 2*cos(x))/((cos(x))^2 + 2*cos(x)*sin(x) + (sin(x))^2)`
- Target: `(a*tan(x)+b*sec(x)+c)/(sec(x)+2*sin(x))`
- The program shows "Target is not equivalent to the source" incorrectly

**Required Changes:**
1. **Simplify trig expressions FIRST** before comparing
   - Use identity: `sin²(x) + cos²(x) = 1`
   - Simplify denominator: `(cos(x))^2 + 2*cos(x)*sin(x) + (sin(x))^2 = (cos(x) + sin(x))^2`
   - This should simplify to something like `(3 + 2*sin(x) - 2*cos(x))/(1 + 2*sin(x)*cos(x))`

2. **Better target form matching**:
   - Recognize that `sec(x) + 2*sin(x) = (1 + 2*sin(x)*cos(x))/cos(x)`
   - Allow algebraic manipulation to show equivalence

3. **Show clear working**:
   - Step 1: Simplify source using trig identities
   - Step 2: Express in terms of tan(x) and sec(x)
   - Step 3: Match coefficients

4. **Handle more transform types**:
   - Any rational function of sin/cos → rational function of tan/sec
   - Any rational function of sin/cos → rational function of sin only (using cos² = 1 - sin²)
   - Polynomial in sin(x), cos(x) → expressions in tan(x/2) (Weierstrass substitution)

---

### Fix 1.4: Remove Redundant Output Lines Throughout

**Locations:** Multiple programs (algebra, trig, derive, int, suvat)

**Problem:** Multiple redundant output patterns exist:
- "Solution:" followed by "Answer:" (duplicate)
- "Original:" followed by "Simplify:" followed by the same expression
- Debug output like "Final = ..."

**Required Changes:**

1. **In algebraProgram.py solve mode** (if applicable):
   - Remove "Solution:" line, keep only "Answer:"

2. **In all transform/xform modes**:
   - Remove echoed inputs (see Fix 1.2)
   - Remove "Original:" if it's just a repeat of input

3. **In intProgram.py**:
   - Reduce integration output to max 4 lines:
     - Method: [technique name]
     - [Key formula or substitution]
     - [Working if needed]
     - Answer: [final result]

4. **In trigProgram.py**:
   - Remove duplicate method headers
   - Show one "Method: ..." line only

5. **General cleanup**:
   - Remove any "Final =", "Result =", debug-style output
   - Remove lines that just echo back user input

---

### Fix 1.5: Fix Domain/Range Mode (Mode 10)

**Location:** `src/Math/algebraProgram.py` function `find_domain_text()`

**Problem:** Shows inequality but doesn't solve it:
```
sqrt(2x-5): Expression inside sqrt must be >= 0: 2*x - 5 >= 0
Domain: all real x except where restrictions apply.
```

**Should solve inequalities:**
```
sqrt(2x-5): 2*x - 5 >= 0
Solve: x >= 5/2
Domain: x >= 5/2
```

---

### Fix 1.6: Fix Any Other Output Echoing Issues

Search for and fix patterns like:
- `print("Source = " + ...)` or similar echo statements
- `lines.append("Original: ...")` that duplicates input
- Any debug-style output that echoes user input

### Algebra - New Solvers Needed

1. **Exponential Equations** (Question 1, 14)
   - `2^(2x+1) - 33(2^(x-1)) + 4 = 0` → substitute u = 2^x
   - `9^x - 4(3^x) + 3 = 0` → substitute u = 3^x
   - Handle: any equation where exponential terms can be unified

2. **Simultaneous Non-Linear** (Question 7)
   - `x^2 + y^2 = 25` and `x + 2y = 10`
   - Solve by substitution

3. **Logarithmic Equations** (Question 9)
   - `log2(x) + log4(x) + log16(x) = 7`
   - Use change of base formula: log_a(x) = ln(x)/ln(a)

4. **Modulus Equations** (Question 6)
   - `|2x - 3| = |x^2 - 3|`
   - Solve by considering cases or squaring

5. **Inequalities** (Question 5)
   - `|x^2 - 7| < 9`
   - Solve: -9 < x² - 7 < 9 → -2 < x² < 16

6. **Series Summation** (Question 10)
   - `ln 2 + ln 4 + ln 8 + ...` (first 50 terms)
   - Recognize as arithmetic series of logs

7. **Polynomial Division/Remainder** (Question 16)
   - `x^100 ÷ (x² - 3x + 2)`
   - Use remainder theorem: evaluate at x=1, x=2

8. **Function Composition** (Question 17)
   - Show f(f(x)) = x for f(x) = (x+1)/(x-1)
   - Simplify step by step

9. **Domain and Range** (Question 18, also Mode 10)
   - `g(x) = 1/sqrt(4 - x²)`
   - Domain: 4 - x² > 0 → -2 < x < 2
   - Range: y > 1/2

10. **Sequence Convergence** (Question 20)
    - `u₁ = √2, u_{n+1} = √(2 + u_n)`
    - Find limit L: L = √(2 + L) → L² = 2 + L → L = 2

### Trigonometry - New Solvers Needed

1. **Identity Proofs** (Question 21)
   - Prove tan(3θ) = (3tanθ - tan³θ)/(1 - 3tan²θ)
   - Use tan(3θ) = tan(2θ + θ) = (tan2θ + tanθ)/(1 - tan2θ*tanθ)

2. **Harmonic Form** (Question 22)
   - `5sin(x) + 12cos(x)` → R sin(x + α)
   - R = √(5² + 12²) = 13, α = arctan(12/5)

3. **Small Angle Approximation** (Question 23)
   - Show limit equals 2 using series expansion

4. **Solve Equation sin5x + sinx = sin3x** (Question 25)
   - Use sum-to-product: sin A + sin B = 2 sin((A+B)/2) cos((A-B)/2)

5. **Inverse Trig** (Question 26)
   - arcsin(x) + arccos(x) = π/2
   - Show using complementary relationship

6. **Sine Rule Ambiguous Case** (Question 27)
   - Given a=5, b=6, A=40°, find B (0, 1, or 2 solutions)

7. **3D Trigonometry** (Question 28)
   - Angle between space diagonal and face of cube
   - cos(θ) = √3/3 or θ = arccos(√3/3)

8. **Exact Values** (Question 29)
   - cos(15°) = (√6 + √2)/4
   - Use half-angle: cos(15°) = cos(30°/2)

9. **General Solutions** (Question 30)
   - tan²θ = 3 → tan θ = ±√3
   - θ = π/3 + nπ/2

10. **Trig Substitution** (Question 31)
    - x = a sec θ, find √(x² - a²) = a tan θ

11. **Solve cos3θ = cosθ** (Question 32)
    - 3θ = ±θ + 2nπ → θ = nπ or θ = nπ/2

12. **Range of Rational Trig** (Question 33)
    - f(θ) = 1/(3 - 2 sin θ)
    - Max: 1/(3 - 2*(-1)) = 1/5, Min: 1/(3 - 2*1) = 1

13. **Area from Two Sides and Angle** (Question 37)
    - Area = ½ab sin C → 10√3 = ½*x*(x+1)*sin(60°)

14. **Sec Equation** (Question 38)
    - 2sec²θ - tanθ = 5 → use sec² = 1 + tan²

15. **Half Angle** (Question 40)
    - cos θ = 1/3, find sin(θ/2) = ±√((1-cosθ)/2)

### Differentiation - New Solvers Needed

1. **First Principles** (Question 41)
   - d/dx[√x] = lim(h→0) (√(x+h) - √x)/h
   - Rationalize numerator

2. **Connected Rates** (Question 42)
   - V = 4/3 πr³, dV/dt = 10, find dA/dt when r=5
   - A = 4πr², dA/dt = dA/dr * dr/dt

3. **Stationary Points** (Question 43)
   - y = x² e^(-x), find where dy/dx = 0, classify with d²y/dx²

4. **Second Derivative** (Question 44)
   - y = ln(sin x), find d²y/dx²
   - First: cot x, Second: -csc² x

5. **Implicit Normals** (Question 45)
   - x²y + y²x = 6 at (1,2)
   - Find dy/dx implicitly, then normal is -1/(dy/dx)

6. **Parametric Gradient** (Question 46)
   - x = a cos³t, y = a sin³t
   - dy/dx = (dy/dt)/(dx/dt)

7. **Optimization** (Question 47)
   - Cylinder in sphere radius R
   - Volume V = πr²h, h = 2√(R² - r²)

8. **Logarithmic Differentiation** (Question 48)
   - y = (x+1)² √(x-2) / (x+3)³
   - Take ln, differentiate, solve for dy/dx

9. **Arctan Derivative** (Question 49)
   - y = arctan(x/a) → dy/dx = 1/(a + x²/a) = a/(a² + x²)

10. **Points of Inflexion** (Question 50)
    - y = x e^x
    - Find where d²y/dx² = 0

11. **Chain Rule Deep** (Question 51)
    - y = sin³(4x²)
    - y = (sin(4x²))³

12. **Small Changes** (Question 53)
    - dV/V ≈ 3 * dr/r for sphere

13. **Tangents from External Point** (Question 54)
    - Tangents from (0, -4) to y = x²
    - Use point-slope form

14. **Hyperbolic** (Question 55)
    - y = arsinh(x) = ln(x + √(x² + 1))
    - dy/dx = 1/√(x² + 1)

15. **Derivative of Inverse** (Question 56)
    - f(x) = x³ + x, g = f⁻¹, find g'(2)
    - g'(y) = 1/f'(x) where y = f(x)

### Integration - New Solvers Needed

1. **Integration by Parts Twice** (Question 61)
   - ∫ e^x sin x dx
   - Use parts twice, then solve for integral

2. **Arctan Integral** (Question 62)
   - ∫ dx/(x² + a²) = (1/a) arctan(x/a)

3. **Area Between Curves** (Question 63)
   - ∫₀^π/4 (cos x - sin x) dx

4. **Volume of Revolution** (Question 64)
   - y = √x, rotate about x-axis: V = π ∫ y² dx

5. **Differential Equations** (Question 65, 72)
   - dy/dx = y/x → dy/y = dx/x → ln y = ln x + C
   - Separable: dy/dt = k(M - y)

6. **Partial Fractions** (Question 66)
   - ∫ dx/(x² - 1) = ∫ [1/(x-1) - 1/(x+1)]/2 dx

7. **Improper Integral** (Question 67)
   - ∫₁^∞ dx/x² = 1

8. **Trig Substitution** (Question 68)
   - ∫ √(a² - x²) dx = a²/2 * (arcsin(x/a) + x√(a²-x²)/a²)

9. **Mean Value** (Question 69)
   - (1/(b-a)) ∫ₐᵇ f(x) dx

10. **Trapezium Rule** (Question 70)
    - Approximate ∫₀¹ e^(-x²) dx

11. **Modulus Function Integral** (Question 71)
    - ∫₀³ |x-1| dx = split at x=1

12. **Power Reduction** (Question 73)
    - ∫ cos²x dx = ∫ (1 + cos 2x)/2 dx

13. **Integral of ln** (Question 74)
    - ∫ ln x dx = x ln x - x

14. **Tricky Limit** (Question 75)
    - ∫₀^π/2 sin x/(sin x + cos x) dx = π/4
    - Use symmetry: I + J = π/2, where J = ∫ cos x/(sin x + cos x)

15. **Arc Length** (Question 76)
    - ∫ √(1 + (dy/dx)²) dx

16. **Surface Area Revolution** (Question 77)
    - 2π ∫ y √(1 + (dy/dx)²) dx

17. **Integral of tan** (Question 78)
    - ∫ tan x dx = -ln|cos x|

18. **Double Log** (Question 79)
    - ∫ dx/(x ln x ln(ln x)) = ln|ln(ln x)| + C

19. **Periodic Absolute** (Question 80)
    - ∫₀^2π |sin x| dx = 4

### SUVAT/Mechanics - New Solvers Needed

1. **Two-Stage Motion** (Question 81)
   - Accelerate at 2 m/s² for t seconds, decelerate at 4 m/s² to rest
   - Total distance 600m

2. **Projectile Range** (Question 82)
   - Show max range = u²/g at 45°

3. **Inclined Plane with Friction** (Question 83)
   - a = g(sin θ - μ cos θ)

4. **Pulley System** (Question 84)
   - Two masses over smooth pulley
   - a = (3m - m)g/(3m + m) = g/2

5. **Variable Acceleration** (Question 85)
   - v = 3t² - 4t, find s from t=1 to t=3

6. **Ladder Equilibrium** (Question 86)
   - Ladder against smooth wall, rough ground
   - Minimum angle: tan⁻¹(1/μ)

7. **Vector Motion** (Question 87)
   - v = 3i + 4j, a = i - j
   - v(2) = v(0) + a*t

8. **Projectile Angle** (Question 88)
   - Range 20m, u = 20 m/s
   - Two angles: θ and 90°-θ

9. **Relative Velocity** (Question 89)
   - v_A rel B = v_A - v_B

10. **Moments** (Question 90)
    - Non-uniform rod, COM at L/4
    - Take moments about each support

11. **Impulse** (Question 91)
    - J = m(v + u) for rebound

12. **Work Done** (Question 92)
    - W = Fd cos θ against gravity

13. **Power on Incline** (Question 93)
    - P = Fv = mgv sin θ

14. **Friction Coefficient** (Question 94)
    - μ = tan θ when on verge of sliding

15. **Lift Reaction** (Question 95)
    - R = m(g + a)

16. **Inelastic Collision** (Question 96)
    - v = (m₁u₁ + m₂u₂)/(m₁ + m₂)

17. **Vertical Circle** (Question 97)
    - Minimum speed at bottom: √(5gL)

18. **Banked Curve** (Question 98)
    - No friction needed: tan θ = v²/(rg)

19. **Spring Work** (Question 99)
    - W = ½kx² from natural length

20. **Projectile Clearance** (Question 100)
    - θ from: D = u² sin 2θ / g, H = u² sin²θ/(2g)

---

## PART 3: OUTPUT FORMAT IMPROVEMENTS

### Compact Integration Output
- Maximum 4 lines per integration
- Show method name, key formula, substitution, answer

### Factorised Answers
- Prefer factored form: (x-1)(x-3) over x² - 4x + 3

### Single Method Header
- Don't repeat method names multiple times
- One clear "Method: ..." line

### Domain Solving
- Solve inequalities to give exact bounds
- x >= 5/2, not "2x - 5 >= 0"

### No Duplicate Lines
- "Solution:" AND "Answer:" → just "Answer:"

---

## PART 4: CODE CLEANUP

### Remove Section Comments
- All lines matching `^# ==+ .* ==+$`

### Combine Duplicate Functions
Move to shared_helpers.py:
- is_num(), is_zero(), is_one(), same(), parse(), show()
- ensure_reasoning_marker(), fn(), neg()

---

## PART 5: TESTING PROTOCOL

### Standard Tests
Run the following test suites and fix failures:

```bash
# Run all question tests
python3 tests/run_tests.py
# Then type /run test_20_questions

# Run random chaos tests
python3 tests/run_tests.py
# Then type /run random 9999

# Repeat until 100% achieved
```

### Required Pass Criteria
1. All 100 exam questions must produce correct exam-style output
2. /random 9999 must achieve 100% (run multiple times, fix any failures)
3. All quality checks pass (has working steps, has solution, exam format)

### 100 Question Test List

#### ALGEBRA (1-20)
1. 2^(2x+1) - 33(2^(x-1)) + 4 = 0
2. Factor theorem polynomial
3. Circle through (0,0), (1,0), tangent to y=x
4. Discriminant of (b-c)x² + (c-a)x + (a-b) = 0
5. |x² - 7| < 9
6. |2x - 3| = |x² - 3|
7. x² + y² = 25, x + 2y = 10
8. (x³ - 8)/(x² - 4) ÷ (x² + 2x + 4)/(x² + 4x + 4)
9. log₂x + log₄x + log₁₆x = 7
10. Sum first 50 terms: ln2 + ln4 + ln8 + ...
11. Partial fractions: (x² + 1)/(x(x-1)²)
12. Binomial: coefficient x² in (1 - 2x)^(1/2)
13. Roots of x³ - px + q: α² + β² + γ²
14. 9^x - 4(3^x) + 3 = 0
15. Minimum of 4x² + 12x + 15
16. Remainder x^100 ÷ (x² - 3x + 2)
17. f(f(x)) = x for f(x) = (x+1)/(x-1)
18. Range of 1/√(4 - x²)
19. Transform y = ln x to y = ln(2x + 4)
20. Sequence: u₁ = √2, u_{n+1} = √(2 + u_n)

#### TRIGONOMETRY (21-40)
21. Prove tan 3θ = (3tanθ - tan³θ)/(1 - 3tan²θ)
22. 5sin x + 12cos x = R sin(x + α)
23. Limit of (1 - cos 2θ)/(θ sin θ) as θ→0
24. Sector perimeter P and area A
25. sin 5x + sin x = sin 3x for 0 < x < π
26. arcsin x + arccos x = π/2
27. Sine rule ambiguous case
28. Angle between cube diagonal and face
29. cos 15° exact value
30. tan²θ = 3 general solution
31. x = a secθ, √(x² - a²) = ?
32. cos 3θ = cos θ for 0 ≤ θ ≤ 2π
33. Max/min of 1/(3 - 2 sin θ)
34. Period of sin(3x) + cos(2x)
35. Polar coords of (-3, 3)
36. csc 2θ - cot 2θ = tan θ
37. Triangle area 10√3, sides x, x+1, angle 60°
38. 2sec²θ - tanθ = 5
39. tan 75° exact value
40. cosθ = 1/3, find sin(θ/2)

#### DIFFERENTIATION (41-60)
41. d/dx[√x] from first principles
42. Sphere: dV/dt = 10, find dA/dt at r=5
43. Stationary points of y = x² e^(-x)
44. Second derivative of y = ln(sin x)
45. Normal to x²y + y²x = 6 at (1,2)
46. Gradient of x = a cos³t, y = a sin³t at t=π/6
47. Max cylinder in sphere radius R
48. y = (x+1)²√(x-2)/(x+3)³
49. d/dx[arctan(x/a)]
50. Point of inflexion of y = x e^x
51. y = sin³(4x²)
52. y = ln x / x
53. dV/V for V = 4/3 πr³, r increases 1%
54. Tangents to y = x² through (0, -4)
55. d/dx[arsinh(x)]
56. g = f⁻¹ of f(x) = x³ + x, find g'(2)
57. Concave down of x⁴ - 6x²
58. d/dx[10^x]
59. d/dx[e^x cos x]
60. lim (sin(x+h) - sin x)/h

#### INTEGRATION (61-80)
61. ∫ e^x sin x dx
62. ∫ dx/(x² + a²)
63. Area between sin x and cos x from 0 to π/4
64. Volume of y = √x rotated about x-axis 0 to 4
65. dy/dx = y/x, y(1) = 2
66. ∫ dx/(x² - 1)
67. ∫₁^∞ dx/x²
68. ∫ √(a² - x²) dx
69. Mean value of sin x on [0, π]
70. Trapezium rule ∫₀¹ e^(-x²) dx
71. ∫₀³ |x - 1| dx
72. dy/dt = k(M - y)
73. ∫ cos²x dx
74. ∫ ln x dx
75. ∫₀^(π/2) sin x/(sin x + cos x) dx
76. Arc length y = 2/3 x^(3/2) from 0 to 3
77. Surface area sphere from y = √(r² - x²)
78. ∫ tan x dx
79. ∫ dx/(x ln x ln(ln x))
80. ∫₀^(2π) |sin x| dx

#### SUVAT/MECHANICS (81-100)
81. Car accelerates 2 m/s² for t, decelerates 4 to rest, 600m total
82. Projectile max range = u²/g
83. Mass m on incline θ, friction μ
84. Pulley: 3m and m, find tension
85. v = 3t² - 4t, displacement t=1 to t=3
86. Ladder length L against smooth wall, rough ground μ
87. v = 3i + 4j, a = i - j, speed after 2s
88. Range 20m, u = 20 m/s, find angles
89. Boat A north 10 km/h, Boat B east 10 km/h, v_A rel B
90. Rod mass M, length L, COM at L/4, find reactions
91. 0.5kg ball hits wall 10 m/s, rebounds 8 m/s, impulse
92. Block 5m up 30° incline, 100N force, work vs gravity
93. Car 1000kg climbs 5° hill at 20 m/s, power
94. Block verge sliding at 25° incline, find μ
95. 70kg man in lift accelerating up 2 m/s², reaction
96. m1 moving u hits m2 at rest, coalesce, find v
97. Vertical circle radius L, min speed at bottom
98. Car 20° banked curve radius 50m, no friction speed
99. Spring k = 200 N/m, 10cm to 20cm, work done
100. Projectile clears wall height H at distance D, find angle

---

## PART 6: MICROPYTHON COMPATIBILITY

- No f-strings
- No type hints
- No dataclasses
- Use str.format() instead

---

## GRAPH REBUILD

After all code changes:
```bash
python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"
```

---

## FILES TO EDIT

1. src/Math/deriveProgram.py - Fix cot/sec/cosec rules, add new solvers
2. src/Math/algebraProgram.py - Add exponential, simultaneous, logs, etc.
3. src/Math/trigProgram.py - Add identity proofs, harmonic form, etc.
4. src/Math/intProgram.py - Add new integration techniques
5. src/Math/SUVATprogram.py - Add mechanics solvers
6. src/Math/shared_helpers.py - Add any new shared functions
7. docs/*.md - Update all documentation files
8. AGENTS.md - Update progress section

---

## PART 7: ADDITIONAL IMPROVEMENTS (All Programs)

### Fix 7.1: Verbose Integration Output Reduction (intProgram.py)

**Location:** Various integration functions throughout intProgram.py

**Problem:** Integration outputs show too many steps (8+ lines for simple integrals).

**Example of current bad output:**
```
Method: Integration by substitution
1. u = 5*x
2. du/dx = 5
3. du = 5 dx
4. I = Int[1/5*e^u*sin(7/5*u)] du
5. = 5*e^u*(sin(7/5*u) - 7/5*cos(7/5*u))/74 + C
6. Substitute back u = 5*x.
7. = 5*e^(5*x)*(sin(7*x) - 7/5*cos(7*x))/74 + C
8. Simplify.
= 5*e^(5*x)*(sin(7*x) - 7/5*cos(7*x))/74 + C
```

**Required Changes:**
- Reduce to max 4 lines for ALL integration methods:
  1. `Method: [technique name - max 30 chars]`
  2. `[Key formula or substitution - one line]`
  3. `[Working if needed - one line]`
  4. `Answer: [final compact result]`
- Remove numbered steps like "1.", "2.", "3."
- Remove "Simplify." lines that just repeat the answer
- Remove "Substitute back..." lines that are redundant

---

### Fix 7.2: Duplicate Method Headers in Trig (trigProgram.py)

**Location:** Multiple solve functions in trigProgram.py

**Problem:** Shows multiple method lines when one is sufficient.

**Current:**
```
1. 1 - cos(2*x) = 0
2. 2*sin(x)^2 = 0
3. Method: Using algebraic manipulation
```

**Should be:**
```
Method: Transform to 2sin²x
[working steps]
Answer: [result]
```

---

### Fix 7.3: Factorised vs Expanded Forms (algebraProgram.py)

**Location:** solve_expr, factor_expr functions

**Problem:** Shows expanded `x^2 - 4x + 3` when factored `(x-1)(x-3)` is more appropriate for exam answers.

**Required Changes:**
- When solving polynomial equations, prefer factored form in answer
- When simplifying, prefer factored form if polynomial has integer roots
- Add `prefer_factored(node)` function that returns factored form when:
  - Polynomial with integer roots
  - Can factor cleanly as (x-a)(x-b)...

---

### Fix 7.4: SUVAT Edge Cases

**Location:** SUVATprogram.py

**Problems:**
1. Zero acceleration edge cases not handled well
2. Zero time edge cases cause division by zero
3. Negative discriminant in quadratic formula causes issues

**Required Changes:**
1. Handle a=0 case (constant velocity): v = u + at → v = u, s = ut + ½at² → s = ut
2. Handle t=0 case: return initial values
3. When discriminant < 0, handle complex roots gracefully (show "no real solution")
4. Handle negative time values gracefully

---

### Fix 7.5: Implicit Multiplication Consistency

**Location:** All programs (parse functions)

**Problem:** Some forms of implicit multiplication work in some contexts but not others.

**Examples that should work:**
- `2x` = `2*x`
- `x(x+1)` = `x*(x+1)`
- `sin x` = `sin(x)` (where x is a variable)
- `(x+1)(x+2)` = `(x+1)*(x+2)`

**Required Changes:**
- Ensure consistent parsing across all modes
- Add tests for all implicit multiplication forms
- Fix any edge cases where implicit mult fails

---

### Fix 7.6: Fraction Display

**Location:** show/display functions in all programs

**Problem:** Complex fractions display poorly.

**Examples:**
- `1/2/3` should simplify to `1/6`
- `(x+1)/(x-1)/(x+2)` should simplify
- Nested fractions should be flattened

**Required Changes:**
- Simplify multiple divisions: a/b/c = a/(b*c)
- Flatten rational expressions where possible
- Display in exam-friendly form (prefer fractions over decimals when exact)

---

### Fix 7.7: Power/Exponent Handling

**Location:** All programs

**Problems:**
1. Negative exponents not always simplified: `x^(-2)` should be `1/x²`
2. Fractional exponents: `x^(1/2)` should display as `√x`
3. Zero exponent: anything^0 = 1 (except 0^0)
4. One exponent: anything^1 = anything

**Required Changes:**
- Simplify negative powers to fractions
- Convert fractional powers to radical form where appropriate
- Handle special cases: 0^0, 0^negative

---

### Fix 7.8: Trigonometric Simplification Consistency

**Location:** trigProgram.py, deriveProgram.py, intProgram.py

**Problems:**
1. sin²x + cos²x = 1 not always applied
2. 1 + tan²x = sec²x not always applied
3. 1 + cot²x = cosec²x not always applied
4. Double angle formulas not always recognized

**Required Changes:**
- Apply pythagorean identities consistently
- Simplify expressions using these identities before showing final answer
- In differentiation: prefer simplified final form (e.g., -csc²x for derivative of cot x)

---

### Fix 7.9: Logarithmic Simplification

**Location:** algebraProgram.py, intProgram.py

**Problems:**
1. log(a) + log(b) not always combined to log(ab)
2. log(a) - log(b) not always combined to log(a/b)
3. n*log(x) not always combined to log(x^n)
4. log(e) not simplified to 1 in appropriate contexts

**Required Changes:**
- Combine logarithms where valid (all logs must have same base)
- Simplify log(e) = 1, log(1) = 0
- Handle change of base: log_a(b) = log(b)/log(a)

---

### Fix 7.10: Error Message Quality

**Location:** All programs

**Problem:** Error messages are sometimes cryptic or not exam-appropriate.

**Required Changes:**
- Replace "Err: ..." with meaningful exam-style messages
- For unsupported: "Method: Not supported - try alternative approach"
- For no solution: "Answer: No solution exists" or "Answer: No real solution"

---

## PART 8: DOCUMENTATION UPDATES

### Update 8.1: README Files

**Files to update:**
- docs/algebra_README.md
- docs/trig_README.md
- docs/derive_README.md
- docs/int_README.md
- docs/suvat_README.md
- docs/boolean_README.md (if exists)

**Updates needed:**
1. Add any new modes or features added
2. Fix any outdated examples
3. Add examples for newly supported equation types
4. Update syntax sections if parsing changed

---

### Update 8.2: AGENTS.md

**File:** AGENTS.md

**Updates needed:**
1. Update the Progress section with new date and changes
2. Remove old test results
3. Add new known issues if any discovered
4. Document any new optimizations added

---

### Update 8.3: GRAPH_REPORT.md

**File:** graphify-out/GRAPH_REPORT.md

**Action:** After all code changes, rebuild the graph and update any sections that changed significantly.

---

## PART 9: MICROPYTHON COMPATIBILITY VERIFICATION

### Verify All Changes Work On:
- MicroPython v1.9.4 (target platform)
- Python 3.8+ (development)

### Check:
- No f-strings (use .format() or %)
- No type hints
- No dataclasses
- No walrus operator (:=)
- No match/case statements
- All imports work on both platforms

---

## PART 10: FINAL TESTING PROTOCOL

### Run Tests In This Order:

1. **Quick validation** (10-50 tests):
   ```bash
   python3 tests/run_tests.py
   /run random 50
   ```

2. **Medium batch** (500-1000 tests):
   ```bash
   /run random 500
   /run random 1000
   ```

3. **Large batch** (9999 tests):
   ```bash
   /run random 9999
   ```

4. **Consistency check** (run 9999 at least 3 times):
   ```bash
   /run random 9999
   /run random 9999
   /run random 9999
   ```

5. **All tests must achieve 100%** before completion

### Quality Checks (All Must Pass):
- Has working steps (method, use, let, solve, answer markers)
- Has solution (answer, ans =, x =, dy/dx, + C)
- Is exam format (not numerical fallback, not "compute at N points")

---

## SUCCESS CRITERIA

1. ✅ All 100 test questions produce correct answers with exam-style working
2. ✅ /random 9999 achieves 100% (run minimum 3 times to confirm consistency)
3. ✅ No "shortcut" derivatives - all use proper sin/cos conversion + quotient rule
4. ✅ Output is compact (max 4 lines for integrations), exam-appropriate
5. ✅ Domain/Range mode solves inequalities properly
6. ✅ Code cleaned (no section comments, duplicate functions consolidated)
7. ✅ No echoed user inputs in output
8. ✅ Transform/xform mode works for trig expressions
9. ✅ All README files updated
10. ✅ AGENTS.md updated
11. ✅ Graph rebuilt after changes

---

## PART 11: CODE ORGANIZATION & STRUCTURE

### 11.1: Consolidate Shared Helpers

**Location:** `src/Math/shared_helpers.py` (create if not exists)

**Purpose:** All programs share utility functions. Consolidate to avoid duplication.

**Required Functions:**
```python
def same(a, b):  # structural equality check
def sim(node):    # simplify node
def show(node):   # format for display
def parse(text):  # parse input text
def depends_on(node, var):  # check variable dependency
def is_one(node):  # check if equals 1
def is_zero(node): # check if equals 0
def num(n):       # create number node
def sym(name):    # create symbol node
def mul(nodes):   # create multiplication
def add(nodes):   # create addition
def power(a, b):  # create power
def div(a, b):    # create division
def sub(a, b):    # create subtraction
```

**Usage in each program:**
```python
# At top of each program file
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[0]))
try:
    from shared_helpers import (
        same, sim, show, parse, depends_on,
        is_one, is_zero, num, sym, mul, add, power, div, sub
    )
except ImportError:
    # Inline fallbacks if import fails
    from helpers import same, sim, show, parse, depends_on, ...
```

### 11.2: Standardize Function Signatures

**Pattern for all mode functions:**
```python
def mode_TEXT(some_input):
    """
    Brief description of what this mode does.

    Args:
        some_input: Description of input

    Returns:
        List of output lines (strings)
    """
    lines = []
    # Implementation
    return compact_duplicate_lines(lines)
```

**Pattern for CLI main:**
```python
def main():
    try:
        mode = paged_menu_input('M', [
            ('1', 'label'),
            ('2', 'label'),
        ], '1')
        begin_user_action()
        if mode == '1':
            # Get inputs
            lines = mode_1_text(input_text)
            print_lines(lines)
        elif mode == '2':
            # ...
    except Exception as err:
        print('Err: ' + str(err))
```

### 11.3: Remove Dead Code

**Before each release, search for:**
- Unused functions (grep for `def <name>` with no calls)
- Duplicate implementations (same function in multiple files)
- Commented-out code blocks
- Old section headers like `# === SECTION ===`

**Command to find dead code:**
```bash
# Find functions defined but not called elsewhere
for func in $(grep -h "^def " src/Math/*.py | sed 's/def \([a-z_]*\).*/\1/'); do
    count=$(grep -r "$func(" src/Math/*.py | grep -v "def $func" | wc -l)
    if [ $count -eq 0 ]; then
        echo "UNUSED: $func"
    fi
done
```

---

## PART 12: MICROPYTHON v1.9.4 COMPATIBILITY

### 12.1: F-String Prohibition

**All string formatting must use % or .format(), NEVER f-strings:**

```python
# WRONG (Python 3.6+ only)
result = f"x = {value}"
lines.append(f"Term {i}: {term}")

# CORRECT (MicroPython compatible)
result = "x = %s" % value
lines.append("Term %d: %s" % (i, term))
# OR
result = "x = {}".format(value)
lines.append("Term {}: {}".format(i, term))
```

### 12.2: Type Checking

**Use TYPE_CHECK for isinstance:**

```python
# WRONG
if type(x) == int:

# CORRECT
if isinstance(x, int):
```

### 12.3: No Walrus Operator

```python
# WRONG
if (x := compute()):
    pass

# CORRECT
x = compute()
if x:
    pass
```

### 12.4: No Positional-Only Arguments

```python
# WRONG
def func(a, /, b):
    pass

# CORRECT
def func(a, b):
    pass
```

### 12.5: No match/case

```python
# WRONG
match value:
    case 1:
        pass
    case _:
        pass

# CORRECT
if value == 1:
    pass
else:
    pass
```

---

## PART 13: COMMENTING STANDARDS

### 13.1: Required Header Block

**Every file must have:**
```python
"""
File: algebraProgram.py
Purpose: Symbolic algebra manipulation for CASIO fx-cg50
Author: [name]
Date: 2024
MicroPython: v1.9.4 compatible
"""

import sys
import math
from pathlib import Path
# ... other imports
```

### 13.2: Function Docstrings

**All exported functions:**
```python
def parse(text):
    """
    Parse input text into expression tree.

    Args:
        text (str): Input expression like "x^2 + 3*x"

    Returns:
        tuple: Expression tree node

    Examples:
        >>> parse("x")
        ('sym', 'x')
        >>> parse("1")
        ('num', 1, 1)
    """
    # Implementation
```

### 13.3: Inline Comments Rules

**When to add comments:**
- Non-obvious algorithm steps
- Mathematical transformations
- Edge case handling
- MicroPython compatibility workarounds

**When NOT to add comments:**
- Obvious code (self-explanatory)
- Section dividers
- TODO placeholders (use actual TODO markers)

```python
# Good comment
result = sim(div(numerator, a))  # Use div for exact rational division

# Bad comment
result = sim(div(numerator, a))  # Divide numerator by a
```

---

## PART 14: OUTPUT OPTIMIZATION RULES

### 14.1: Compactness Hierarchy

**Integration Output (MAX 4 lines):**
```
Method: [short name]
Use: [key formula or substitution]
= [working]
Answer: [final result]
```

**Differentiation Output (MAX 3 lines):**
```
Method: [rule used]
= [simplified result]
Answer: [final result]
```

**Algebra Solve Output (MAX 4 lines):**
```
Expr = [factored form]
[method used]
Answer: x = [solutions]
```

### 14.2: Skip Redundant Lines

**Skip when:**
- Constant factor = 1 (binomial expansion)
- Already in target form (transform)
- Single-step solution (simple cases)
- Duplicate method + answer lines

### 14.3: Reasoning Markers

**Required in working steps:**
- "Method:" or "Using:" - what rule/method
- "Let:" - variable substitution
- "Use:" - identity or formula
- "Answer:" or "=" - final step

---

## PART 15: TESTING PROTOCOL

### 15.1: Per-Feature Testing

**After changing ANY mode:**
```bash
# Test all modes for regressions
python3 tests/run_tests.py /run random 50

# Test specific mode
python3 tests/run_tests.py /filter algebra
python3 tests/run_tests.py /run random 100 --workers 8
```

### 15.2: MadAsMaths I.Y.G.B. Validation

**Run sample questions:**
```bash
python3 tests/test_madasmaths.py
```

**All 38 core questions must pass.**

### 15.3: Consistency Testing

**Run same seed 3 times:**
```bash
# Seed fixed for reproducibility
python3 tests/run_tests.py /run random 9999 --workers 24
python3 tests/run_tests.py /run random 9999 --workers 24
python3 tests/run_tests.py /run random 9999 --workers 24
```

---

## PART 16: FUTURE FEATURE PRIORITIES

### High Priority (Next Sprint)

1. **Simultaneous equations** - 2 equations, 2 unknowns
2. **Parametric differentiation** - Already working, verify edge cases
3. **3D vector operations** - Basic dot/cross product
4. **Complex numbers** - Basic operations

### Medium Priority

5. **Partial differentiation** - f(x,y) with respect to one variable
6. **Multiple integrals** - Double integral areas
7. **Series convergence** - Ratio/root/comparison tests
8. **Matrix operations** - 2x2 determinant, inverse

### Low Priority (Nice to Have)

9. **Polynomial long division**
10. **Binomial coefficient expansion** - Negative/fractional n
11. **Second-order DE** - ay'' + by' + cy = 0
12. **Probability distributions** - Normal, binomial probabilities

---

## APPENDIX: FILE CHECKLIST

### Before Any Commit

- [ ] Run `random 100` tests - 100% pass
- [ ] Run `random 500` tests - 100% pass
- [ ] Test specific changed mode manually
- [ ] Check no f-strings introduced
- [ ] Check no unused imports
- [ ] Verify MicroPython syntax
- [ ] Update PROMPT.md if adding features
- [ ] Update corresponding README.md
- [ ] Rebuild graphify if changing structure

### Before Release

- [ ] Run `random 9999` three times - 100% all runs
- [ ] Verify all MadAsMaths questions pass
- [ ] Test on actual CASIO device (if possible)
- [ ] Final README review
- [ ] Archive old prompts

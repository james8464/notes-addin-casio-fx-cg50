# C++ Symbolic Engine Issues & Fixes

**Document:** Comprehensive analysis of `c++/addin/src/device/device_solver.cpp` and `c++/prizm/src/main_prizm.cpp`

**Last Updated:** 2026 (Analysis of CASIO fx-CG50 CAS port)

**Status:** Most critical integration/differentiation issues now fixed in the modern module (`c++/addin/src/modules/`). Remaining: architecture wiring, SUVAT expansion, working steps.

---

## Quick Summary

| Category | Status | Issue Count |
|----------|--------|-------------|
| Algebra | ⚠️ Limited | 6 |
| Integration | ✅ Fixed | 3 |
| Differentiation | ✅ Fixed | 2 |
| Trigonometry | ⚠️ Partial | 2 |
| SUVAT | ⚠️ Limited | 2 |
| Module Stubs | ❌ Severe | 11 |
| Working Steps | ⚠️ Limited | 3 |

**Remaining: 27 distinct issues**

---

## ARCHITECTURE ISSUE: Wrong Engine Wired In

### The Core Problem

**Location:** `c++/prizm/src/main_prizm.cpp` and `c++/addin/src/device/device_solver.cpp`

**Issue:** The `.g3a` add-in uses a **simplified, 200-line device solver** that only handles:
- Linear & simple quadratic equations
- Polynomial differentiation (no trig/log/exp)
- Polynomial integration (no trig/log/exp)
- Special-angle trig evaluation only

**Meanwhile:** A **full-featured modular engine** exists in `c++/addin/src/modules/` with:
- Proper differentiation (chain, product, quotient rules)
- Real integration (parts, substitution, trig, logs, exponentials)
- Implicit & parametric differentiation
- Proper exam-style working output

**Status:** MODERN MODULE WORKS - needs wiring into the Prizm build.

---

## REMAINING ISSUES

---

# ALGEBRA ISSUES

## Issue ALG-1: Only Linear & Simple Quadratic Equations Supported

**Severity:** 🔴 Critical

**Problem:**
- **Cubic and higher:** Equations like `x³ - 6x² + 11x - 6 = 0` rejected
- **Rational equations:** `(x+1)/(x-2) = 3` → not parsed
- **Radical equations:** `√(x-1) = 3` → not parsed
- **Transcendental:** `e^x = 5`, `ln(x) = 2` → not parsed

**Status:** NOT FIXED - requires new solve patterns.

---

## Issue ALG-2: Quadratic Discriminant Overflow

**Severity:** 🟠 High

**Problem:** Using 32-bit int for discriminant can overflow.

**Status:** NOT FIXED - needs 64-bit arithmetic.

---

## Issue ALG-3: Surd Answers Not Simplified

**Severity:** 🟠 High

**Problem:** `√8` shown instead of `2√2`.

**Status:** NOT FIXED - needs surd simplification.

---

## Issue ALG-4: No Domain/Range Output

**Severity:** 🟠 High

**Status:** NOT FIXED - module stubs remain.

---

## Issue ALG-5: Newton-Raphson Not Implemented

**Severity:** 🟠 High

**Status:** NOT FIXED - iterative solver needed.

---

## Issue ALG-6: Comparison/Transform Modules Are Stubs

**Severity:** 🟠 Medium

**Status:** NOT FIXED - 11+ module stubs remain.

---

# SUVAT ISSUES

## Issue SUVAT-1: Only ~11 Specific Formula Combinations

**Severity:** 🟠 High

**Problem:** Only hardcoded patterns work; `v² = u² + 2as` combinations fail.

**Status:** NOT FIXED - needs systematic solver.

---

## Issue SUVAT-2: No Automatic Quadratic Branch Selection

**Severity:** 🟠 Medium

**Status:** NOT FIXED - needs physics-aware root selection.

---

# MODULE STUB ISSUES

## Issue MOD-1: 11+ Specialized Modules Are Non-Functional Stubs

**Severity:** 🟠 High

**Stubs remaining:**

| Module | Intended Purpose | Current Routing |
|--------|-----------------|-----------------|
| `AlgCompare` | Verify equivalence | `solve_algebra()` ❌ |
| `AlgTransform` | Algebraic transformation | `solve_algebra()` ❌ |
| `AlgExpand` | Expand brackets | `solve_algebra()` ❌ |
| `AlgPoly` | Polynomial operations | `solve_algebra()` ❌ |
| `AlgCompSq` | Complete the square | `solve_algebra()` ❌ |
| `AlgInverse` | Find inverse function | `solve_algebra()` ❌ |
| `AlgRewrite` | Rewrite in target form | `solve_algebra()` ❌ |
| `AlgDomRng` | Domain/range analysis | `solve_algebra()` ❌ |
| `AlgCartesian` | Parametric to Cartesian | `solve_algebra()` ❌ |
| `AlgNewton` | Newton-Raphson | `solve_algebra()` ❌ |
| `TrigProve` | Prove identities | `solve_trig()` ❌ |
| `TrigTransform` | Trig transformation | `solve_trig()` ❌ |
| `TrigRewrite` | Rewrite trig expressions | `solve_trig()` ❌ |

**Status:** NOT FIXED - needs implementation or wiring to modern module.

---

# WORKING STEPS ISSUES

## Issue WORK-1: Algebra Working Steps Are Too Sparse

**Severity:** 🟠 Medium

**Status:** NOT FIXED.

---

## Issue WORK-2: Quadratic Working Doesn't Show Formula Substitution

**Severity:** 🟠 Medium

**Status:** NOT FIXED.

---

## Issue WORK-3: Integration Working Doesn't Show Per-Term Application

**Severity:** 🟠 Medium

**Status:** NOT FIXED.

---

# FIXED ISSUES (for reference)

## ✅ Integration - Fixed in modern module
- x·sin(x), x·cos(x) - integration by parts
- x²·sin(x), x²·cos(x) - integration by parts twice
- x·exp(x), exp(x)·sin(x), exp(x)·cos(x) - polynomial × exponential
- x·log(x), x²·log(x) - polynomial × logarithm
- sec·tan, cosec·cot - trig products

## ✅ Differentiation - Fixed in modern module
- All trig functions (sin, cos, tan, sec, cosec, cot, asin, acos, atan)
- Exponentials, logarithms
- Product, quotient, chain rules
- Implicit differentiation
- Parametric differentiation
- Second derivatives

## ✅ Trigonometry - Fixed in modern module
- Full trig equation solving with interval filtering
- Rewrite rules and identities
- Proof support
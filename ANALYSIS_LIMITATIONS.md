# CASIO Program Limitations Analysis

## Summary
This document identifies where each CASIO program falls short regarding user input, particularly around missing/incomplete working steps that students would need for exam credit.

---

## CRITICAL ISSUES (Programs Fail Entirely)

### 1. same_by_sig Import Mismatch
**Issue**: Programs require `sys.path` to include both `src` and `src/Math` to import shared modules correctly.  
**Impact**: Running programs directly (e.g., `python3 src/Math/trigProgram.py`) fails with lambda argument errors.  
**Required setup**: Must run from project root with `sys.path.insert(0, 'src')` and `sys.path.insert(0, 'src/Math')`  
**Test command that works**:
```bash
cd /Users/james/Developer/Python/CASIO
python3 -c "
import sys
sys.path.insert(0, 'src')
sys.path.insert(0, 'src/Math')
from src.Math import trigProgram
trigProgram.main()
"
```

### 2. Partial Fractions - No Working
**Issue**: Algebra program "poly" mode doesn't do partial fraction decomposition.  
**Input**: `(3x+5)/((x-1)(x+2))`  
**Expected**: `A/(x-1) + B/(x+2)` with A=... B=...  
**Actual**: Simple polynomial expansion only  

### 3. Some Prove Identities Fail
**Issue**: Some provable identities fail with "Equation does not match Equation 1"  
**Failing examples**:
- `tan(x)+tan(y) = sin(x+y)/cos(x)cos(y)` - FAILS
- `sec^2(x) = 1+tan^2(x)` - Output shows nothing

---

## WORKING GAPS - Working Out Missing or Incomplete

### 4. DERIVE: Implicit Differentiation - Missing Rearrangement Steps
**Input**: `x^2 + y^2 = 25`  
**Output**:
```
2*x + 2*y*dy/dx = 0
Rearrange: make dy/dx
Answer: dy/dx = (-x)/y
```
**Missing for exam credit**: Individual steps showing:
- Collecting dy/dx terms on one side
- Explicitly dividing to isolate dy/dx

### 5. DERIVE: Complex Implicit - Just Final Answer
**Input**: `x^3 + y^3 = 2xy`  
**Output**:
```
3. d/dx(LHS)=d/dx(RHS)
4. 3*x^2 + 3*y^2*dy/dx - 2*y - 2*x*dy/dx = 0
5. Rearrange: make dy/dx
6. Answer: dy/dx = (-3*x^2 + 2*y)/(3*y^2 - 2*x)
```
**Missing for exam credit**: Step-by-step algebraic manipulation to collect dy/dx terms explicitly

### 6. TRIG SOLVE: General Solutions Use Degrees Instead of Radians
**Input**: `sin(x) = 0`  
**Output**: `x = [-1710, -1530, ...]` (degree values like -1710°)  
**Issue**: Should show in radians OR degrees depending on context. No option to specify.  
**Better output**: `x = nπ` or `x = 180n°`

### 7. TRIG SOLVE: Not Showing General Solution Form
**Input**: `sin(x) = 1`  
**Output**: `x = [-1710, -1350, ..., 90, 450, ...]` (10 specific solutions)  
**Expected for full marks**: `x = π/2 + 2nπ` (general form with n ∈ ℤ)

### 8. TRIG SOLVE: Complex Equations Skip Steps
**Input**: `sin(2x) = cos(x)`  
**Shows 22 steps** including:
- Rewriting using sin² + cos² = 1
- Factorisation
- Solving cos(x)(2sin(x)-1)=0
- Solving each factor  
**But** the factorisation step shows `-cos(x) + 2*sin(x)*cos(x) = 0` then `Factorise.`  
**Missing**: Explicitly taking cos(x) common factor

### 9. ALGEBRA: Inverse Function - Only Shows Basic Linear Case
**Input**: `2x+1`  
**Output**:
```
f(x) = 2*x + 1
y = 2*y + 1
Method: Solve for x in y = f(x)
Answer: f^-1(x) = (x - 1)/2
```
**Issue**: This works for linear. But for `x^2` or `1/x` shows incorrect or partial results

### 10. ALGEBRA: Domain/Range - Range May Be Wrong
**Input**: `1/x`  
**Output**: `Range: all real y`  
**Issue**: Should show y ≠ 0 (hyperbola, not all real values)

---

## LIMITED COVERAGE - Features Not Supported

### 11. ALGEBRA: Absolute Value Equations Limited
**Input**: `|x| = 1`  
**Output**: `Answer: no closed-form solution found`  
**Expected**: Should solve piecewise: x = 1 or x = -1

### 12. ALGEBRA: Exponential Equations Limited  
**Input**: `2^x = 8`  
**Output**: `Answer: no closed-form solution found`  
**Expected**: Should recognize 2^x = 8 means x = 3

### 13. ALGEBRA: Compare Mode - Basic Only
Input matching is exact - cannot handle equivalent forms that need rewrite first.

### 14. ALGEBRA: General Solutions Not in Simplest Form
Polynomial solutions may show unsimplified factors:
- Shows `x = [1, 0, -1]` when could simplify

---

## INPUT PARSING ISSUES

### 15. Implicit Multiplication Limited
May not recognize `xy` as `x*y` in all contexts

### 16. Variable Names
- Only single letters recognized well: x, y, t
- Multi-letter like "theta" works in trig, but not consistently across programs

### 17. Degree vs Radian Ambiguity
No way to specify degrees in trig solve - all outputs in degrees by default but doesn't say

---

## MISSING MODES/FEATURES FOR EXAM COVERAGE

### 18. Second Derivative (Derive Mode 1)
No option for second derivative d²y/dx² - only first derivative

### 19. Integration Limits
Mode 1 asks for indefinite only - no definite integral option

### 20. Integration by Substitution - No Interactive u Selection
You must enter u - program doesn't guide which substitution to try

### 21. DE solve - Limited to First Order
Only separable and linear first-order. Can't do second-order DE.

### 22. Parametric Area - No Option for Bounds
Just calculates indefinite integral, no option for definite area between bounds

---

## VERIFICATION ISSUES

### 23. Compare Mode - Can Incorrectly Report Equivalent
May report some non-equivalent expressions as equivalent due to signature collisions

### 24. Prove - Can Report False Positives
If both sides simplify to same form, claims true without checking all steps

---

## DATA: What Works Well

Working correctly:
- Basic differentiation (power rule, trig, exp, ln)
- Basic integration (power rule, trig basic, exp)
- Simple equation solving (linear, quadratic)
- Simple prove identities
- SUVAT kinematics
- Domain/range for basic functions
- Composition of functions
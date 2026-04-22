# CASIO Output Enhancements for fx-cg50

## Current Issues Identified

### 1. Domain/Range Mode (Mode 10) - CRITICAL ISSUE

**Current output for `sqrt(2x-5)`:**
```
1. Method: Determine Domain
2. Input = sqrt(2*x - 5)
3. Expression inside sqrt/asin/acos must be >= 0: 2*x - 5 >= 0
4. Domain: all real x except where restrictions apply.
```

**Problems:**
- Shows inequality but doesn't SOLVE it
- Should output: `x >= 5/2` (or `x >= 2.5`)
- Range always says "complex" - should calculate actual range

**Target output:**
```
Method: Determine Domain
sqrt(2x-5) requires: 2x-5 >= 0
Solve: x >= 5/2
Domain: x >= 5/2
```

### 2. Redundant Output Lines

**Algebra solve shows both:**
```
Solution: x = [3, 1]
Answer: x = [3, 1]
```

**Should be ONE line only:**
```
Answer: x = [3, 1]
```

### 3. Verbose Integration Steps

**Current (8+ lines for e^(5x)*sin(7x)):**
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

**Should be compact (4 lines max):**
```
Method: Int[e^(ax+b)sin(cx+d)]
Use standard result: = e^(ax+b)(a*sin(cx+d) - c*cos(cx+d))/(a²+c²) + C
= 5*e^(5x)*(sin(7x) - 7/5*cos(7x))/74 + C
Answer: 5*e^(5x)*(sin(7x) - 7/5*cos(7x))/74 + C
```

### 4. Factorised vs Expanded Forms

**Current:** Shows expanded `x^2 - 4x + 3` as answer
**Should:** Show factored `(x-1)(x-3)` when appropriate

### 5. Multiple Method Headers in Trig

**Current:**
```
1. 1 - cos(2*x) = 0
2. 2*sin(x)^2 = 0
3. Method: Using algebraic manipulation
```

**Should be ONE header:**
```
Method: Transform to 2sin²x
```

---

## Action Items

### Priority 1: Fix Domain/Range Mode 10

**File:** `src/Math/algebraProgram.py`
**Function:** `find_domain_text()` (line 6853)

**Current code (lines 6900-6909):**
```python
for r_type, r_node in restrictions:
    r_show = show(r_node)
    if r_type == 'den':
        lines.append('Denominator cannot be zero: ' + r_show + ' != 0')
    elif r_type == 'sqrt':
        lines.append('Expression inside sqrt/asin/acos must be >= 0: ' + r_show + ' >= 0')
    elif r_type == 'pos':
        lines.append('Expression inside log must be > 0: ' + r_show + ' > 0')

lines.append('Domain: all real ' + var_name + ' except where restrictions apply.')
```

**Change to:** Solve inequalities to give exact bounds:
```
x >= 5/2, x != 2, domain: x >= 5/2
```

### Priority 2: Remove Redundant Output Lines

**Algebra solve:** Remove duplicate "Solution:" line - keep only "Answer:"

**Files to check:**
- `src/Math/algebraProgram.py` (lines 4200-4216)
- `src/Math/trigProgram.py` (lines 16141-16143)

### Priority 3: Compact Integration Output

**File:** `src/Math/intProgram.py`

**Every integration method:** Reduce to max 4 lines:
1. Method header
2. Key formula/technique (one line)
3. Working substitution (if needed)
4. Final answer

**Remove:** Numbered step 1, step 2, step 3... excessive detail

### Priority 4: Factorised Answer Preference

**Add function:** `prefer_factored(node)` that returns factored form when:
- Polynomial with integer roots
- Can factor as (x-a)(x-b)... cleanly

**Use in:**
- Algebra solve output
- Integration of polynomials
- Transform output

### Priority 5: User-Entered Restrictions

**Feature:** Allow user to specify restrictions to MODIFY domain

**Example interaction:**
```
M: 10
Expr: sqrt(2x-5)

Method: Determine Domain
Domain: x >= 5/2

Enter restriction (or Enter for all): x < 10
New domain: 5/2 <= x < 10
```

---

## Exact Changes Required

### Change 1: find_domain_text()

In `algebraProgram.py` around line 6900:

**Replace ALL of:**
```python
for r_type, r_node in restrictions:
    r_show = show(r_node)
    if r_type == 'den':
        lines.append('Denominator cannot be zero: ' + r_show + ' != 0')
    elif r_type == 'sqrt':
        lines.append('Expression inside sqrt/asin/acos must be >= 0: ' + r_show + ' >= 0')
    elif r_type == 'pos':
        lines.append('Expression inside log must be > 0: ' + r_show + ' > 0')

lines.append('Domain: all real ' + var_name + ' except where restrictions apply.')
```

**With solved inequalities:**
```python
# For each inequality type, solve to give exact bounds
if r_type == 'sqrt':
    # Solve: 2*x - 5 >= 0 -> x >= 5/2
    solved = solve_inequality(r_node, '>=')
    if solved:
        lines.append(solved)  # "x >= 5/2"
elif r_type == 'den':
    # Solve: x - 2 != 0 -> x != 2
    lines.append(var_name + ' != ' + show(r_node))  # "x != 2"
elif r_type == 'pos':
    # Solve: x > 0 -> x > 0
    lines.append(var_name + ' > 0')
```

**Final domain line:**
```python
lines.append('Domain: ' + format_domain_bounds(var_name, restrictions))
# Output: "x >= 5/2" or "x >= 0, x != 2"
```

### Change 2: Remove Duplicate Answer Lines

**In algebraProgram.py solve_equation_text():**

Remove lines 4215-4216 (keep only Answer):
```python
# DELETE THESE:
# lines.append('Solution: ' + solution)
# lines.append('Answer: ' + solution)

# KEEP ONLY:
lines.append('Answer: ' + solution)
```

### Change 3: Compact Integration Output

**In intProgram.py, every method function:**

Reduce from 8 steps to 4 max:
```
Method: [technique name - max 30 chars]
[1-line formula if applicable]
[substitution if needed, one line]
Answer: [final compact result]
```

---

## Testing

After changes, verify:
- `sqrt(2x-5)` gives `Domain: x >= 5/2`
- `x^2-4x+3` solve gives ONE answer line
- Integration under 4 lines
- Test suite passes
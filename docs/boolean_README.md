# Boolean Algebra Program (booleanProgram.py)

A Boolean algebra simplification and logic gate program for the CASIO fx-cg50 calculator (runs MicroPython v1.9.4).

## Features

| Mode | Feature | Description |
|------|---------|-------------|
| 1 | Simplify | Simplify Boolean expressions using Boolean algebra laws |
| 2 | NAND form | Convert expression to NAND-only form |
| 3 | NOR form | Convert expression to NOR-only form |
| 4 | Prove identity | Prove two Boolean expressions are equivalent |

## Input Syntax

- **Variables**: Single letters (A, B, C, etc.) or multi-letter (AB = A AND B)
- **Constants**: `0` (false), `1` (true)
- **Operators**:
  - `,` for NOT (e.g., `A,` = NOT A)
  - `.` for AND (e.g., `A.B` = A AND B)
  - `+` for OR (e.g., `A+B` = A OR B)
- **Grouping**: Parentheses `()`

## Mode 1: Simplify

Simplify Boolean expressions using Boolean algebra laws, showing each step.

### Example 1: Basic OR

```
Mode: 1
Expression: A+0
Using: A+0

Output:
1. A+0
= A    (Identity)
Result: A
```

### Example 2: AND with 1

```
Mode: 1
Expression: A.1
Using: A.1

Output:
1. A.1
= A    (Identity)
Result: A
```

### Example 3: Idempotent Law

```
Mode: 1
Expression: A.A
Using: A.A

Output:
1. A.A
= A    (Idempotent)
Result: A
```

### Example 4: Absorption

```
Mode: 1
Expression: A+A.B
Using: A+A.B

Output:
1. A+A.B
= A    (Absorption)
Result: A
```

### Example 5: Distributive

```
Mode: 1
Expression: A.(B+C)
Using: A.(B+C)

Output:
1. A.(B+C)
= A.B+A.C    (Distribute)
Result: A.B+A.C
```

### Example 6: De Morgan

```
Mode: 1
Expression: (A.B),
Using: (A.B),

Output:
1. (A.B),
= A,+B,    (De Morgan)
Result: A,+B,
```

### Example 7: Double Negation

```
Mode: 1
Expression: A,,
Using: A,,

Output:
1. A,,
= A    (Double NOT)
Result: A
```

### Example 8: Complex Simplification

```
Mode: 1
Expression: ((B,.A),.B,),+A.B
Using: ((B,.A),.B,),+A.B

Output:
1. ((B,.A),.B,),+A.B
2. (B,.A), = B,+A,    (De Morgan)
3. (B,+A,).B, = B,.B,+A,.B    (Distribute)
4. B,.B = 0    (Contradiction)
5. 0+A,.B = A,.B    (Identity)
6. A,.B+A.B = (A.+A).B+(A.A,).B    (Distribute)
7. A.+A = 1    (Complement)
8. A.A, = 0    (Contradiction)
9. 1.B+0 = B    (Identity)
Result: B
```

## Mode 2: NAND Form

Convert a Boolean expression to NAND-only form (all operations are NAND gates).

### Example 1: Simple AND to NAND

```
Mode: 2
Expression: A.B
Using: A.B

Output:
1. A.B
2. NAND form: (A.B).(A.B)
```

### Example 2: OR to NAND

```
Mode: 2
Expression: A+B
Using: A+B

Output:
1. A+B
2. NAND form: (A.A)+(B.B)
```

### Example 3: NOT to NAND

```
Mode: 2
Expression: A
Using: A

Output:
1. A
2. NAND form: A.A
```

## Mode 3: NOR Form

Convert a Boolean expression to NOR-only form (all operations are NOR gates).

### Example 1: Simple OR to NOR

```
Mode: 3
Expression: A+B
Using: A+B

Output:
1. A+B
2. NOR form: (A+B)+(A+B)
```

### Example 2: AND to NOR

```
Mode: 3
Expression: A.B
Using: A.B

Output:
1. A.B
2. NOR form: (A,+B,)+(A,+B,)
```

## Mode 4: Prove Identity

Prove that two Boolean expressions are equivalent, showing the simplification of both sides.

### Example 1: Distributive Law

```
Mode: 4
LHS: A.(B+C)
RHS: A.B+A.C
1. LHS = A.(B+C)
2. RHS = A.B+A.C

3. LHS: A.(B+C)
4. = A.B+A.C    (Distribute)
5. 
6. RHS: A.B+A.C
7. 
8. Both simplify to: A.B+A.C
```

### Example 2: De Morgan

```
Mode: 4
LHS: (A.B),
RHS: A,+B,
1. LHS = (A.B),
2. RHS = A,+B,

3. LHS: (A.B),
4. = A,+B,    (De Morgan)
5. 
6. RHS: A,+B,
7. 
8. Both simplify to: A,+B,
```

### Example 3: Absorption

```
Mode: 4
LHS: A+A.B
RHS: A
1. LHS = A+A.B
2. RHS = A

3. LHS: A+A.B
4. = A    (Absorption)
5. 
6. RHS: A
7. 
8. Both simplify to: A
```

## Mode 2: NAND Form

Convert a Boolean expression to NAND-only form (all operations are NAND gates).

### Example 1: Simple AND to NAND

```
Mode: 2
Expression: A.B
Using: A.B

Output:
1. A.B
2. NAND form: (A.B).(A.B)
```

### Example 2: OR to NAND

```
Mode: 2
Expression: A+B
Using: A+B

Output:
1. A+B
2. NAND form: (A.A)+(B.B)
```

### Example 3: NOT to NAND

```
Mode: 2
Expression: A
Using: A

Output:
1. A
2. NAND form: A.A
```

### Example 4: Complex Expression

```
Mode: 2
Expression: A.B+C
Using: A.B+C

Output:
1. A.B+C
2. NAND form: ((A.B).(A.B)).((C.C).(C.C))
```

## Mode 3: NOR Form

Convert a Boolean expression to NOR-only form (all operations are NOR gates).

### Example 1: Simple OR to NOR

```
Mode: 3
Expression: A+B
Using: A+B

Output:
1. A+B
2. NOR form: (A+B)+(A+B)
```

### Example 2: AND to NOR

```
Mode: 3
Expression: A.B
Using: A.B

Output:
1. A.B
2. NOR form: (A,+B,)+(A,+B,)
```

### Example 3: NOT to NOR

```
Mode: 3
Expression: A
Using: A

Output:
1. A
2. NOR form: A+A
```

## Mode 4: Prove Identity

Prove that two Boolean expressions are equivalent, showing the simplification of both sides.

### Example 1: Distributive Law

```
Mode: 4
LHS: A.(B+C)
RHS: A.B+A.C

Output:
LHS: A.(B+C)
1. A.(B+C) = A.B+A.C    (Distribute)
= A.B+A.C

RHS: A.B+A.C
= A.B+A.C

Both sides simplify to: A.B+A.C
IDENTITY PROVEN
```

### Example 2: De Morgan

```
Mode: 4
LHS: (A.B),
RHS: A,+B,

Output:
LHS: (A.B),
1. (A.B), = A,+B,    (De Morgan)
= A,+B,

RHS: A,+B,
= A,+B,

Both sides simplify to: A,+B,
IDENTITY PROVEN
```

### Example 3: Absorption

```
Mode: 4
LHS: A+A.B
RHS: A

Output:
LHS: A+A.B
1. A+A.B = A    (Absorption)
= A

RHS: A
= A

Both sides simplify to: A
IDENTITY PROVEN
```

## Boolean Laws Implemented

### Basic Laws
- **Identity**: A + 0 = A, A · 1 = A
- **Null**: A + 1 = 1, A · 0 = 0
- **Idempotent**: A + A = A, A · A = A
- **Complement**: A + A' = 1, A · A' = 0

### Commutative
- A + B = B + A
- A · B = B · A

### Associative
- A + (B + C) = (A + B) + C
- A · (B · C) = (A · B) · C

### Distributive
- A · (B + C) = A·B + A·C
- A + (B · C) = (A + B) · (A + C)

### Absorption
- A + A·B = A
- A · (A + B) = A

### De Morgan
- (A + B)' = A' · B'
- (A · B)' = A' + B'

### Double Negation
- (A')' = A

## Logic Gates Supported

- **NOT**: Inverter (unary)
- **AND**: Conjunction
- **OR**: Disjunction
- **NAND**: NOT-AND (universal)
- **NOR**: NOT-OR (universal)

## Notes

- Works on MicroPython v1.9.4 (CASIO fx-cg50)
- No imports required (pure Python)
- Shows step-by-step simplification
- Can handle complex expressions with up to 50 simplification steps
- Useful for digital logic design and simplification
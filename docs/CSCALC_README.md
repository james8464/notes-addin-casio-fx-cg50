# CSCALC.g3a README

## Purpose

`CSCALC.g3a` is a KhiCAS-based Boolean algebra helper.

## Files

- Calculator app: `/Users/james/Developer/CASIO/calculator_files/CSCALC.g3a`

## Controls

- Same console shell, function keys, and mini menus as KhiCAS.
- Press `F4` for the Function Catalog.
- The catalog keeps the KhiCAS sections and adds `Computer Science`.

## Computer Science Commands

`bool_simplify(expression)`

Shows the old Python Boolean program's line-by-line simplification:

- `,` means NOT, e.g. `A,`
- `.` means AND, e.g. `A.B`
- `+` means OR, e.g. `A+B`
- adjacent letters expand as AND, e.g. `AB` becomes `A.B`

Examples:

- `bool_simplify(A+A)`
- `bool_simplify(A+A.B)`
- `bool_simplify(A.B+A,.C+B.C)`
- `bool_simplify(((B,.A),.B,),+A.B)`

`nandform(expression)`

Example:

- `nandform(A.B)`

`norform(expression)`

Example:

- `norform(A+B)`

`boolprove(lhs,rhs)`

Example:

- `boolprove(A.(B+C),A.B+A.C)`

## Removed Scope

Number-base, storage, floating-point, algorithm, truth-table, K-map, and minterm helpers are not part of this app.

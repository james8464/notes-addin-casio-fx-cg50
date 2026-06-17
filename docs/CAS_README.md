# CAS.g3a README

## Purpose

`CAS.g3a` is the A-level Maths working add-in. It keeps the original KhiCAS calculation core and adds A-level-style output for common Pure Maths commands and Paper 3 mechanics, with SUVAT as the main mechanics route. If a supported working route is known, it prints the method. If not, it falls back to the underlying KhiCAS exact result where possible.

## Files

- Calculator app: `/Users/james/Developer/CASIO/calculator_files/CAS.g3a`
- Help/menu pack: `/Users/james/Developer/CASIO/calculator_files/CAS.PAK`
- Companion RunMat status mock: `/Users/james/Developer/CASIO/calculator_files/RUNMAT.g3a`

## Input Rules

- Use `exp(x)` for exponentials.
- Use `ln(x)` for natural log.
- Use `log(x)` for base-10 log.
- Use radians unless the question clearly uses degrees and the route says so.
- Prefer explicit multiplication: `3*x`, `2*sin(x)`.
- Working commands should be typed directly or inserted from the command menu.
- For mechanics, choose a positive direction before entering signed values.

## Main Working Commands

### Differentiation

`diff(expression)` or `diff(expression,x)`

Examples:

- `diff(x^3+2*x,x)` shows power rule.
- `diff(x*exp(-2*x),x)` shows product rule and chain rule.
- `diff((x^2+1)/(2*x-3),x)` shows quotient rule.
- `diff((ln(x))^2,x)` shows chain rule.

### Integration

`integrate(expression,x)`

`integrate(expression,x,a,b)`

`defint(expression,x,a,b)`

Optional method marker:

- `integrate(expression,x),method=parts`
- `defint(expression,x,a,b),method=parts`

Examples:

- `integrate(x^2-1,x)` gives polynomial antiderivative.
- `integrate(3/(2*x-1),x)` gives affine reciprocal log route.
- `integrate(x*exp(2*x),x)` shows integration by parts.
- `integrate(3*x^2*(4-2*x^3)^(3/2),x)` shows reverse-chain substitution.
- `integrate(x*sqrt(x+1),x,0,3)` shows substitution and endpoint use.
- `defint(ln(x)^2,x,2,4),method=parts` shows by-parts setup and endpoint substitution.

### Solving

`solve(equation,variable)`

Examples:

- `solve(3*x+2=11,x)` linear equation.
- `solve(x^2-5*x+6=0,x)` quadratic factor route.
- `solve(1/4-1/x^2>0,x)` inequality with critical values.
- `solve(10^(3*k)=2,k)` log route.
- `solve(4-exp(2*x)=2,x)` exponential/log route.
- `solve(dn/dt=k*n,n,t)` separable differential equation.

### Transformation / Show That

`xform(start_expression,target_expression)`

`rewrite(expression,target_form)`

Examples:

- `xform((3*sin(x)-1)^2=2,cos(x)^2=8*sin(x)^2-6*sin(x))`
- `xform(2*cot(x)/(1+cot(x)^2),sin(2*x))`
- `rewrite((cos(x)+sin(x))*(cosec(x)-sec(x)),cot(2*x))`

Output should show valid transformation lines only. It should not print `Verified` as a working line.

### Expansion / Collection

`texpand(expression)`

`tcollect(expression)`

Examples:

- `texpand((1+2*x)^5)`
- `tcollect(sin(x)^2+cos(x)^2)`
- `texpand(cos(2*x))`

These use KhiCAS-style command names. `texpand` and `tcollect` cover algebraic and trig expansion/collection.

### Taylor / Binomial Expansion

`taylor(expression)`

`taylor(expression,x=a)`

Examples:

- `taylor((1+2*x)^5)`
- `taylor((1+x)^(-1))`
- `taylor((1+A*x)^N,x=5)`
- `taylor(exp(x),x=0)`

### Range / Domain

`range(expression)`

`domain(expression)`

Examples:

- `range(exp(2*x)-k)`
- `range((x^2+1)/(x-2))`
- `domain(ln(x-3))`
- `domain(sqrt(5-2*x))`

### Paper 3 Mechanics

`suvat(known=value,known=value,known=value)`

Examples:

- `suvat(u=2,a=3,t=4)` uses KhiCAS exact solve and gives `s`, `u`, `v`, `a`, `t` on separate lines.
- `suvat(s=10,u=2,a=3)` gives both possible times and velocities, plus `physical t` for non-negative time roots.
- `suvat(u=x,a=2,t=3)` keeps algebraic values exact where possible.
- `suvat(displacement=10,initial=2,acc=3)` accepts common aliases for `s`, `u`, and `a`.
- `suvat(u=2,a=3)` lists the known values and says that another named value is needed.
- Unnamed or unknown inputs are ignored with a message instead of producing parametric junk.
- Contradictory inputs return `No solution` instead of a silent CAS error.

Other compact mechanics commands are available from F4 -> Mechanics: `force`, `weight`, `friction`, `connected`, `pulley`, `lift`, `varacc`, `moment`, `work`, `power`, `impulse`, `momentum`, `energy`, `resolve`.

## Normal KhiCAS Commands Kept Without Working

These behave like normal calculator commands and do not need mark-scheme working:

`abs`, `approx`, `ceil`, `coeff`, `degree`, `floor`, `exp`, `gcd`, `lcm`, `lcoeff`, `product`, `resultant`, `round`, `sqrt`, `subst`.

## Menus

The F4 command menu keeps the original KhiCAS-style names for visible commands. Press F6 on a highlighted command to open its help sheet from `CAS.PAK`.

## Build

```bash
./compile
```

The fresh calculator files are copied to:

`/Users/james/Developer/CASIO/calculator_files/`

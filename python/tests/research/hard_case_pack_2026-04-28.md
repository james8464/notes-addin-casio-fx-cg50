# Hard Case Pack (2026-04-28)

Adapted stress cases to keep pushing the calculator-safe engines.
These are not verbatim paper transcriptions; they are compact test prompts
adapted from the source collections below so they fit the CASIO program
interfaces cleanly.

Sources used for the pack:
- STEP trigonometry topic index: https://step.maths.org/taxonomy/term/33?page=1
- STEP 3 calculus assignment: https://step.maths.org/assignments/step-3-calculus
- STEP 3 differential equations: https://step.maths.org/step-3-differential-equations
- MadAsMaths trigonometry booklets: https://madasmaths.com/archive_maths_booklets_standard_topics_trigonometry.html
- MadAsMaths integration booklets: https://madasmaths.com/archive_maths_booklets_standard_topics_integration.html
- MadAsMaths advanced algebra/calculus booklets: https://madasmaths.com/archive_maths_booklets_standard_topics_various.html

## Cases

1. `Algebra solve`
   - Prompt: `x^4 + 4*x^2 - 18 = 0`
   - Focus: hidden quadratic in `x^2`, exact radicals
   - Source family: MadAsMaths advanced equations / STEP algebraic manipulation

2. `Algebra solve`
   - Prompt: `x^4 + 8*x^2 - 784 = 0`
   - Focus: disguised quartic with only two real roots
   - Source family: MadAsMaths advanced equations

3. `Algebra inverse`
   - Prompt: `y = (3*x - 2)/(x + 5)`
   - Focus: inverse of fractional linear function, domain restriction
   - Source family: MadAsMaths function exam questions

4. `Algebra expansion`
   - Prompt: `(1 - 2*x)^(-3/2)` about `x = 0`, terms up to `x^3`
   - Focus: fractional/negative binomial power
   - Source family: MadAsMaths binomial series expansions

5. `Algebra expansion`
   - Prompt: `1/(1 + x)^2`
   - Focus: binomial via repeated factor / negative integer power
   - Source family: MadAsMaths binomial series expansions

6. `Trig solve`
   - Prompt: `sqrt(3)*sin(2*x) + 2*sin(x)^2 = 1, x, 0, 2*pi`
   - Focus: hidden double-angle plus power-reduction
   - Source family: STEP trigonometry / MadAsMaths trigonometry exam sets

7. `Trig solve`
   - Prompt: `tan(2*x) - tan(x) = 0, x, -pi, pi`
   - Focus: interval roots vs accidental general family
   - Source family: STEP trigonometric equations

8. `Trig solve`
   - Prompt: `cosec(x) = 2, x, 0, 2*pi`
   - Focus: reciprocal trig solved through sine family with correct working
   - Source family: MadAsMaths inverse/reciprocal trig

9. `Trig prove`
   - Prompt: `cos(x)/(1 + sin(x)) = sec(x) - tan(x)`
   - Focus: reciprocal identities hidden inside rational trig form
   - Source family: STEP identities / MadAsMaths trig identities

10. `Trig prove`
    - Prompt: `(1 - cos(2*x))/(1 + cos(2*x)) = tan(x)^2`
    - Focus: half-angle identity recognition through rational rearrangement
    - Source family: STEP trigonometry / MadAsMaths double-angle identities

11. `Trig transform`
    - Prompt: `3*cos(x) - 4*sin(x)`
    - Focus: rewrite to `R*cos(x + a)` or equivalent with exact angle
    - Source family: MadAsMaths compound angle identities

12. `Derive normal`
    - Prompt: `((exp((-2*x-7)^5)/(ln|6*x|+7))/(sqrt((9-6*x)^6+8)+4))^3`
    - Focus: chain + quotient + abs/log formatting
    - Source family: STEP 3 calculus style composite differentiation

13. `Derive normal`
    - Prompt: `(log(exp(9*x-7)+8))*(cosec(8*(9*x^2-5*x-6)+1))*(exp(4*x-6)+6*x-5)`
    - Focus: triple product with nested chain rules
    - Source family: MadAsMaths differentiation II / STEP calculus

14. `Derive implicit`
    - Prompt: `x^2 + x*y + y^2 = 7`
    - Focus: clean implicit rearrangement and final `dy/dx`
    - Source family: MadAsMaths implicit differentiation

15. `Derive parametric`
    - Prompt: `x = sin(t)/(1 + cos(t)), y = ln(1 + cos(t))`
    - Focus: parametric differentiation plus hidden half-angle simplification
    - Source family: MadAsMaths parametric equations / STEP trigonometry

16. `Integrate`
    - Prompt: `((1/(x^2)) + (1/(x^3)))*exp(1/x)`
    - Focus: reverse-chain substitution from reciprocal power disguise
    - Source family: STEP 3 calculus / MadAsMaths substitution

17. `Integrate`
    - Prompt: `(x*tan(x))/(tan(x)+sec(x))`
    - Focus: hidden trig conjugate rewrite plus parts
    - Source family: MadAsMaths trigonometric identities in integration

18. `Integrate`
    - Prompt: `(2*x + 3)/(x^2 + 3*x + 2)`
    - Focus: partial fractions / repeated linear factors after factorisation
    - Source family: MadAsMaths partial fractions

19. `Integrate`
    - Prompt: `sin(log(x))/x`
    - Focus: substitution where the inner function is logarithmic
    - Source family: STEP 3 calculus / MadAsMaths substitution

20. `Differential equation`
    - Prompt: `dy/dx = y*(1-y), y(0) = 1/2`
    - Focus: separable equation requiring partial fractions and constant solving
    - Source family: STEP differential equations / MadAsMaths first-order DE sets

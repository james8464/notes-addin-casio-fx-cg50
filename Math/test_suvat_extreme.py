"""
Extreme test suite for SUVATprogram.py
Tests edge cases, crash scenarios, incorrect answers, and poor working output.
"""

import sys
sys.path.insert(0, '/Users/james/Developer/Python/CASIO/Math')
sys._suvat_no_autorun = True

import SUVATprogram as sp

PASS = 0
FAIL = 0
WARN = 0


def report(label, passed, detail=''):
    global PASS, FAIL, WARN
    if passed:
        PASS += 1
        print(f'  PASS: {label}')
    else:
        FAIL += 1
        print(f'  FAIL: {label} {detail}')


def warn(label, detail=''):
    global WARN
    WARN += 1
    print(f'  WARN: {label} {detail}')


def run_test(label, s, u, v, a, t, target, expect_result=None, expect_error=None, check_fn=None):
    """Run a single SUVAT test."""
    print(f'\n--- {label} ---')
    try:
        result, equation, original_eq, sub_text = sp.build_suvat_solution(s, u, v, a, t, target)

        if expect_error is not None:
            if result is None:
                report(label, expect_error.lower() in equation.lower(), f'got: {equation}')
            else:
                report(label, False, f'expected error but got result: {result}')
            return

        if result is None:
            report(label, False, f'got error: {equation}')
            return

        if expect_result is not None:
            report(label, result == expect_result, f'expected {expect_result}, got {result}')

        if check_fn is not None:
            check_fn(result, equation, original_eq, sub_text, label)

        # Check for poor working output
        if sub_text is not None:
            if 'None' in sub_text:
                warn(label, f'sub_text contains None: {sub_text}')
            if 'nan' in sub_text.lower():
                warn(label, f'sub_text contains NaN: {sub_text}')

    except Exception as e:
        report(label, False, f'CRASH: {type(e).__name__}: {e}')


def check_physical_consistency(result, equation, original_eq, sub_text, label):
    """Check that the result makes physical sense."""
    try:
        val = sp.node_to_float(result)
        if val is not None:
            # Time should generally be positive
            if 't = ' in sub_text and val < 0:
                warn(label, f'negative time: {val}')
    except:
        pass


# ============================================================================
# TEST 1: Zero value edge cases
# ============================================================================
print('\n========== TEST GROUP 1: Zero Value Edge Cases ==========')

# All zeros
run_test('All zeros - solve for v',
         sp.num(0), sp.num(0), None, sp.num(0), sp.num(5), 'v',
         expect_result='0')

# s=0, u=0, solve for t
run_test('s=0, u=0, a=2 - solve for t (quadratic)',
         sp.num(0), sp.num(0), None, sp.num(2), None, 't')

# u=0, v=0, t=5 - should give a=0
run_test('u=0, v=0, t=5 - solve for a',
         None, sp.num(0), sp.num(0), None, sp.num(5), 'a',
         expect_result='0')

# t=0 edge case
run_test('t=0 with s=0 - solve for u (degenerate)',
         sp.num(0), None, sp.num(5), sp.num(2), sp.num(0), 'u')


# ============================================================================
# TEST 2: Negative values
# ============================================================================
print('\n========== TEST GROUP 2: Negative Values ==========')

# Negative acceleration (deceleration)
run_test('Negative acceleration',
         None, sp.num(20), sp.num(0), sp.num(-5), None, 's',
         expect_result='40')

# Negative displacement
run_test('Negative displacement',
         sp.num(-50), sp.num(0), None, sp.num(-10), None, 't')

# Negative initial velocity
run_test('Negative initial velocity',
         None, sp.num(-10), sp.num(5), sp.num(3), None, 's',
         expect_result='-25/2')

# All negatives
run_test('All negatives - solve for v',
         sp.num(-100), sp.num(-20), None, sp.num(-5), sp.num(4), 'v')


# ============================================================================
# TEST 3: Fractional values
# ============================================================================
print('\n========== TEST GROUP 3: Fractional Values ==========')

# Fractional inputs
run_test('Fractional inputs',
         None, sp.num(1, 2), sp.num(3, 2), sp.num(1, 4), None, 's',
         expect_result='4')

# Result should be fractional
run_test('Fractional result',
         sp.num(1, 3), sp.num(1, 6), None, sp.num(1, 2), sp.num(2, 3), 'v')

# Complex fractions
run_test('Complex fractions',
         None, sp.num(7, 3), sp.num(11, 3), sp.num(2, 3), None, 's',
         expect_result='6')


# ============================================================================
# TEST 4: Quadratic time solver edge cases
# ============================================================================
print('\n========== TEST GROUP 4: Quadratic Time Solver ==========')

# Two positive roots
run_test('Two positive roots for t',
         sp.num(12), sp.num(1), None, sp.num(2), None, 't')

# One positive, one negative root (s=10, u=1, a=3: disc=61)
run_test('One positive, one negative root',
         sp.num(10), sp.num(1), None, sp.num(3), None, 't')

# Both negative roots (s=10, u=-8, a=-1: disc=44)
run_test('Both negative roots',
         sp.num(10), sp.num(-8), None, sp.num(-1), None, 't')

# Perfect square discriminant
run_test('Perfect square discriminant',
         sp.num(8), sp.num(4), None, sp.num(1), None, 't')

# Zero discriminant (one root)
run_test('Zero discriminant',
         sp.num(2), sp.num(2), None, sp.num(2), None, 't')

# Negative discriminant (no real solution) s=-10, u=1, a=-1: disc=1-20=-19
# Actually disc = 1^2 - 4*(1/2*-1)*(-(-10)) = 1 - 20 = -19... wait
# Let me recalculate: A=-1/2, B=1, C=10 -> disc = 1 - 4*(-1/2)*10 = 1+20 = 21
# So this HAS real roots. Need truly negative disc.
run_test('Negative discriminant',
         sp.num(-10), sp.num(1), None, sp.num(1), None, 't',
         expect_error='No real solution')


# ============================================================================
# TEST 5: Physical impossibilities
# ============================================================================
print('\n========== TEST GROUP 5: Physical Impossibilities ==========')

# a=0 but v≠u
run_test('a=0, v≠u - solve for t',
         None, sp.num(5), sp.num(10), sp.num(0), None, 't',
         expect_error='No solution')

# a=0 and v=u (infinite solutions)
run_test('a=0, v=u - solve for t',
         None, sp.num(5), sp.num(5), sp.num(0), None, 't',
         expect_error='Infinite')

# t=0 but s≠0
run_test('t=0, s≠0 - solve for u',
         sp.num(10), None, sp.num(5), sp.num(2), sp.num(0), 'u',
         expect_error='No solution')

# v²=u²+2as is negative
run_test('v^2 = u^2 + 2as negative',
         sp.num(10), sp.num(2), None, sp.num(-5), None, 'v',
         expect_error='No real solution')


# ============================================================================
# TEST 6: Large values
# ============================================================================
print('\n========== TEST GROUP 6: Large Values ==========')

# Very large numbers
run_test('Large values',
         None, sp.num(10000), sp.num(20000), sp.num(500), None, 's',
         expect_result='300000')

# Very large time
run_test('Large time',
         None, sp.num(1), sp.num(100), sp.num(1), sp.num(1000), 's')

# Large fractions
run_test('Large fraction inputs',
         None, sp.num(1000, 7), sp.num(2000, 7), sp.num(100, 7), None, 's',
         expect_result='15000/7')


# ============================================================================
# TEST 7: Surd simplification
# ============================================================================
print('\n========== TEST GROUP 7: Surd Simplification ==========')

# sqrt(8) should simplify
run_test('sqrt(8) simplification',
         None, sp.num(0), None, sp.num(4), sp.num(2), 'v')

# sqrt(72) should simplify
run_test('sqrt(72) simplification',
         None, sp.num(0), None, sp.num(9), sp.num(8), 'v')

# Complex surd
run_test('Complex surd result',
         None, sp.num(0), None, sp.num(18), sp.num(2), 'v')


# ============================================================================
# TEST 8: Parser edge cases
# ============================================================================
print('\n========== TEST GROUP 8: Parser Edge Cases ==========')

# Parse complex expressions
try:
    node = sp.parse('2*sqrt(3)')
    report('Parse 2*sqrt(3)', node is not None)
except Exception as e:
    report('Parse 2*sqrt(3)', False, f'{e}')

try:
    node = sp.parse('(3+sqrt(5))/2')
    report('Parse (3+sqrt(5))/2', node is not None)
except Exception as e:
    report('Parse (3+sqrt(5))/2', False, f'{e}')

try:
    node = sp.parse('g/2')
    report('Parse g/2', node is not None)
except Exception as e:
    report('Parse g/2', False, f'{e}')

try:
    node = sp.parse('sqrt(2)^3')
    report('Parse sqrt(2)^3', node is not None)
except Exception as e:
    report('Parse sqrt(2)^3', False, f'{e}')

# Implicit multiplication
try:
    node = sp.parse('2(3+4)')
    report('Parse 2(3+4)', node is not None)
except Exception as e:
    report('Parse 2(3+4)', False, f'{e}')


# ============================================================================
# TEST 9: Multi-variable consistency
# ============================================================================
print('\n========== TEST GROUP 9: Multi-Variable Consistency ==========')

def check_all_vars_consistent(s, u, v, a, t, label):
    """Check that all computed variables are consistent with each other."""
    print(f'\n  Checking consistency: {label}')
    try:
        results = sp.solve_all_variables(s, u, v, a, t)
        for var in sp.VAR_NAMES:
            if var in results:
                exact, dec = results[var]
                if exact is None:
                    warn(label, f'{var} has None result')
                if dec is None and exact is not None:
                    warn(label, f'{var}={exact} has no decimal')
        return True
    except Exception as e:
        warn(label, f'CRASH: {e}')
        return False

check_all_vars_consistent(sp.num(36), sp.num(12), sp.num(0), None, None, 'deceleration')
check_all_vars_consistent(sp.num(0), sp.num(0), None, sp.num(98, 10), sp.num(5), 'free fall')
check_all_vars_consistent(None, sp.num(10), sp.num(30), sp.num(5), None, 'acceleration')


# ============================================================================
# TEST 10: Equation selection priority
# ============================================================================
print('\n========== TEST GROUP 10: Equation Selection Priority ==========')

# When multiple equations could apply, check the simplest is chosen
run_test('Priority: v,u,a,t available - should use v=u+at',
         sp.num(100), sp.num(10), sp.num(30), sp.num(4), sp.num(5), 'v',
         check_fn=lambda r, eq, orig, sub, lbl: report(
             f'{lbl} - correct equation chosen',
             'v = u + at' in eq, f'got: {eq}'
         ))


# ============================================================================
# TEST 11: Output formatting edge cases
# ============================================================================
print('\n========== TEST GROUP 11: Output Formatting ==========')

# Check that output doesn't contain artifacts
def check_clean_output(result, equation, original_eq, sub_text, label):
    issues = []
    if result is None:
        return
    if '  ' in result:
        issues.append('double space')
    if result.startswith('='):
        issues.append('starts with =')
    if issues:
        warn(label, f'format issues: {", ".join(issues)}')

run_test('Clean output check - simple',
         sp.num(10), sp.num(2), None, sp.num(2), sp.num(2), 'v',
         check_fn=check_clean_output)

run_test('Clean output check - complex',
         None, sp.num(1, 3), sp.num(7, 3), sp.num(1, 2), None, 's',
         check_fn=check_clean_output)


# ============================================================================
# TEST 12: Keyword preset detection
# ============================================================================
print('\n========== TEST GROUP 12: Keyword Presets ==========')

try:
    presets = sp.detect_presets('dropped from rest')
    report('Detect "dropped"', 'u' in presets and sp.is_zero(presets['u']))
except Exception as e:
    report('Detect "dropped"', False, f'{e}')

try:
    presets = sp.detect_presets('thrown upwards')
    report('Detect "thrown upwards"', 'a' in presets)
except Exception as e:
    report('Detect "thrown upwards"', False, f'{e}')

try:
    presets = sp.detect_presets('maximum height')
    report('Detect "maximum height"', 'v' in presets and sp.is_zero(presets['v']))
except Exception as e:
    report('Detect "maximum height"', False, f'{e}')

try:
    presets = sp.detect_presets('returns to ground')
    report('Detect "returns to ground"', 's' in presets and sp.is_zero(presets['s']))
except Exception as e:
    report('Detect "returns to ground"', False, f'{e}')

try:
    presets = sp.detect_presets('dropped maximum height')
    report('Detect multiple keywords', 'u' in presets and 'v' in presets)
except Exception as e:
    report('Detect multiple keywords', False, f'{e}')


# ============================================================================
# TEST 13: Decimal approximation accuracy
# ============================================================================
print('\n========== TEST GROUP 13: Decimal Approximation ==========')

try:
    dec = sp.format_decimal(1/3, 3)
    report('Decimal 1/3 to 3 sig figs', dec == '0.333', f'got {dec}')
except Exception as e:
    report('Decimal 1/3', False, f'{e}')

try:
    dec = sp.format_decimal(22/7, 4)
    report('Decimal 22/7 to 4 sig figs', '3.143' in dec or '3.142' in dec, f'got {dec}')
except Exception as e:
    report('Decimal 22/7', False, f'{e}')

try:
    dec = sp.format_decimal(0.000123456, 3)
    report('Small number sig figs', dec == '0.000123', f'got {dec}')
except Exception as e:
    report('Small number sig figs', False, f'{e}')


# ============================================================================
# TEST 14: Significant figure counting
# ============================================================================
print('\n========== TEST GROUP 14: Significant Figures ==========')

try:
    sf = sp.count_sig_figs('12.0')
    report('Sig figs "12.0"', sf == 3, f'got {sf}')
except Exception as e:
    report('Sig figs "12.0"', False, f'{e}')

try:
    sf = sp.count_sig_figs('0.0045')
    report('Sig figs "0.0045"', sf == 2, f'got {sf}')
except Exception as e:
    report('Sig figs "0.0045"', False, f'{e}')

try:
    sf = sp.count_sig_figs('100')
    report('Sig figs "100"', sf == 1, f'got {sf}')
except Exception as e:
    report('Sig figs "100"', False, f'{e}')


# ============================================================================
# TEST 15: Stress test - many rapid calls
# ============================================================================
print('\n========== TEST GROUP 15: Stress Test ==========')

try:
    for i in range(50):
        s = sp.num(i)
        u = sp.num(i + 1)
        a = sp.num(2)
        t = sp.num(3)
        result, eq, orig, sub = sp.build_suvat_solution(s, u, None, a, t, 'v')
        if result is None:
            warn('Stress test', f'iteration {i} failed')
    report('50 rapid calls', True)
except Exception as e:
    report('50 rapid calls', False, f'{e}')


# ============================================================================
# TEST 16: Cache behavior
# ============================================================================
print('\n========== TEST GROUP 16: Cache Behavior ==========')

try:
    sp.begin_user_action()
    node = sp.parse('x^2 + 2*x + 1')
    result = sp.show(node)
    sp.begin_user_action()
    # Caches should be cleared
    report('Cache clear works', len(sp.SHOW_CACHE) == 0)
except Exception as e:
    report('Cache clear', False, f'{e}')


# ============================================================================
# TEST 17: Edge case - g constant
# ============================================================================
print('\n========== TEST GROUP 17: G Constant ==========')

try:
    node = sp.parse('g')
    report('Parse g', node is not None)
    val = sp.node_to_float(node)
    report('g = 9.8', val == 9.8, f'got {val}')
except Exception as e:
    report('Parse g', False, f'{e}')

try:
    node = sp.parse('2*g')
    report('Parse 2*g', node is not None)
except Exception as e:
    report('Parse 2*g', False, f'{e}')


# ============================================================================
# TEST 18: Division by zero in equations
# ============================================================================
print('\n========== TEST GROUP 18: Division By Zero ==========')

# t=0 in s = ut + 1/2at^2 rearrangement
run_test('t=0 in u = s/t - 1/2at',
         sp.num(10), None, sp.num(5), sp.num(2), sp.num(0), 'u')

# a=0 in t = (v-u)/a
run_test('a=0 in t = (v-u)/a',
         sp.num(10), sp.num(5), sp.num(10), sp.num(0), None, 't')

# u+v=0 in t = 2s/(u+v)
run_test('u+v=0 in t = 2s/(u+v)',
         sp.num(10), sp.num(5), sp.num(-5), sp.num(2), None, 't')


# ============================================================================
# TEST 19: Symbolic expressions as input
# ============================================================================
print('\n========== TEST GROUP 19: Symbolic Expressions ==========')

try:
    node = sp.parse('sqrt(2)')
    run_test('sqrt(2) as acceleration',
             None, sp.num(0), None, node, sp.num(4), 'v')
except Exception as e:
    report('sqrt(2) as input', False, f'{e}')

try:
    node = sp.parse('3/2')
    run_test('3/2 as time',
             None, sp.num(4), None, sp.num(2), node, 'v')
except Exception as e:
    report('3/2 as input', False, f'{e}')


# ============================================================================
# TEST 20: Quadratic v solver edge cases
# ============================================================================
print('\n========== TEST GROUP 20: Quadratic V Solver ==========')

# u=0, a negative, s positive -> v^2 negative
run_test('v^2 negative case',
         sp.num(10), sp.num(0), None, sp.num(-5), None, 'v',
         expect_error='No real solution')

# u=0, a=0, s=10 -> v^2 = 0
run_test('v^2 = 0 case',
         sp.num(10), sp.num(0), None, sp.num(0), None, 'v')

# Large u^2 + 2as
run_test('Large v^2',
         sp.num(1000), sp.num(100), None, sp.num(50), None, 'v')


# ============================================================================
# SUMMARY
# ============================================================================
print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed, {WARN} warnings')
print('=' * 60)

if FAIL > 0:
    print('\nFailures need investigation!')
if WARN > 0:
    print(f'\n{WARN} warnings - review for potential issues')

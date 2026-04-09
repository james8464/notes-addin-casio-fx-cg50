#!/usr/bin/env python3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
sys.path.insert(0, str(ROOT))
sys._int_no_autorun = True
sys._algebra_no_autorun = True
sys._trig_no_autorun = True
sys._derive_no_autorun = True
sys._suvat_no_autorun = True

import intProgram as ip
import algebraProgram as ap
import trigProgram as tp
import deriveProgram as dp
import SUVATprogram as sp

PASS = 0
FAIL = 0


def report(label, passed, detail=''):
    global PASS, FAIL
    if passed:
        PASS += 1
        print('  PASS:', label)
    else:
        FAIL += 1
        print('  FAIL:', label, detail)


print('\n========== INT CONVOLUTED FAMILY AUDIT ==========')
int_cases = [
    ('int_hidden_atan_exp_1', 'exp(2*x)/(1+exp(4*x))', '1/2*atan(e^(2*x))', "f'(x)/(a^2+f(x)^2)"),
    ('int_hidden_atan_poly_2', 'x/(1+x^4)', '1/2*atan(x^2)', "f'(x)/(a^2+f(x)^2)"),
    ('int_hidden_atan_poly_3', 'x^3/(1+x^8)', '1/4*atan(x^4)', "f'(x)/(a^2+f(x)^2)"),
    ('int_hidden_atan_exp_4', 'exp(x)/(4+exp(2*x))', '1/2*atan(1/2*e^x)', "f'(x)/(a^2+f(x)^2)"),
    ('int_clean_surd_5', '(3*x+2)/sqrt(9*x^2+12*x+10)', '1/3*sqrt(9*x^2+12*x+10)', "f'(x)/sqrt(f(x))"),
]
for label, expr, expected, working_bit in int_cases:
    title, ans, lines = ip.solve(ip.parse(expr), 'x', 'auto')
    ans_text = ip.show(ans) if ans is not None else None
    good_lines = lines is not None and any(working_bit in line for line in lines)
    report(label, ans is not None and ans_text == expected and good_lines, f'got {ans_text}, {lines}')


print('\n========== ALGEBRA INVERSE / REWRITE AUDIT ==========')
inverse_cases = [
    ('alg_inv_recip_sqrt_1', '1/(sqrt(2*x+5))', '(1/x^2-5)/2', True),
    ('alg_inv_log_power_2', 'log(3, (x+2)^3)', 'e^(x*ln(3))/3-2', True),
    ('alg_inv_ln_power_3', 'ln((x+2)^3)', 'e^x/3-2', True),
    ('alg_inv_even_ln_4', 'ln((x+1)^2)', None, False),
    ('alg_inv_even_log_5', 'log(3, (x+1)^2)', None, False),
]
for label, expr, expected, invertible in inverse_cases:
    result, steps = ap.inverse_function(expr)
    if invertible:
        report(label, result == expected and steps[-1].startswith('f^-1(x) = '), f'got {result}, {steps}')
    else:
        report(label, result is None and any('No inverse on all real x' in line for line in steps), f'got {result}, {steps}')

rewrite_cases = [
    ('alg_rw_power_term_6', '(x+1)^6 + 3*(x+1)^3 + 1', ['(x+1)^3'], 'Final = ((x+1)^3)^2+3*(x+1)^3+1'),
    ('alg_rw_power_term_7', '(x-2)^4 + 2*(x-2)^2 + 5', ['(x-2)^2'], 'Final = ((x-2)^2)^2+2*(x-2)^2+5'),
    ('alg_rw_power_term_8', 'sin(x)^4 + cos(x)^4', ['sin(x)^2', 'cos(x)^2'], 'Final = ((sin(x))^2)^2+((cos(x))^2)^2'),
]
for label, expr, terms, expected_final in rewrite_cases:
    lines = ap.solve_rewrite_text(expr, terms)
    report(label, lines[-1] == expected_final and any('Rewrite powers in the requested term' in line for line in lines), f'got {lines}')


print('\n========== TRIG CONVOLUTED AUDIT ==========')
trig_cases = [
    ('trig_prove_half_angle_1', 'sin(x)/(1-cos(x))=(1+cos(x))/sin(x)'),
    ('trig_prove_recip_mix_2', 'tan(x)+cot(x)=1/(sin(x)*cos(x))'),
]
for label, expr in trig_cases:
    lines = tp.solve_prove_text(expr, '1')
    report(label, lines[-1] == 'LHS = RHS', f'got {lines}')

trig_solve_cases = [
    ('trig_solve_mix_3', 'sin(2*x)=sin(x),x,0,2*pi', 5),
    ('trig_solve_quad_4', '2*cos(x)^2-3*cos(x)+1=0,x,0,360', 4),
]
for label, expr, count in trig_solve_cases:
    result, lines = tp.solve_solve_text(expr)
    report(label, isinstance(result, list) and len(result) == count, f'got {result}, {lines[:6]}')


print('\n========== DERIVE CONVOLUTED AUDIT ==========')
derive_cases = [
    ('derive_var_pow_fn_1', 'x^sin(x^2)', 'Log diff'),
    ('derive_fn_pow_var_2', 'sin(x)^x', 'Log diff'),
    ('derive_log_base_3', 'log(3, x^2+1)', 'Quotient rule'),
    ('derive_quotient_mix_4', 'sqrt(1+x^2)/(1+x^3)', 'Quotient rule'),
]
for label, expr, key_step in derive_cases:
    var, steps, final, formatted = dp.solve_normal_mode(expr)
    report(label, final is not None and any(key_step in step for step in steps), f'got {steps}')


print('\n========== SUVAT CONVOLUTED AUDIT ==========')
suvat_cases = [
    ('suvat_target_accel_1', sp.num(45), sp.num(5), None, None, sp.num(5), 'a', '8/5', 'a = 2(s-ut)/t^2'),
    ('suvat_symbolic_g_2', sp.parse('1/2*g*3^2'), None, None, sp.parse('g'), sp.num(3), 'u', 'g', 'u = s/t - 1/2at'),
    ('suvat_negative_displacement_3', sp.num(-20), sp.num(20), sp.num(0), None, None, 'a', '10', '(v^2-u^2)/2s'),
]
for label, s, u, v, a, t, target, expected, equation_bit in suvat_cases:
    result, equation, original_eq, sub_text = sp._build_suvat_solution_data(s, u, v, a, t, target)
    result_text = sp.show(result) if result is not None else None
    report(label, result_text == expected and equation_bit in str(equation), f'got {result_text}, {equation}, {sub_text}')


print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed')
print('=' * 60)
if FAIL > 0:
    raise SystemExit(1)

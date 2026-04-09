import math
import subprocess
import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
PY = '/Users/james/Developer/Python/CASIO/.venv/bin/python'

PASS = 0
FAIL = 0
WARN = 0


def report(label, passed, detail=''):
    global PASS, FAIL
    if passed:
        PASS += 1
        print('  PASS:', label)
    else:
        FAIL += 1
        print('  FAIL:', label, detail)


def warn(label, detail=''):
    global WARN
    WARN += 1
    print('  WARN:', label, detail)


def run_cli(user_input):
    proc = subprocess.run([PY, str(ROOT / 'intProgram.py')], input=user_input, text=True, capture_output=True, check=False)
    return proc.returncode, proc.stdout, proc.stderr


def assert_has(label, text, needle):
    report(label, needle in text, f'missing {needle!r}')


def assert_not_has(label, text, needle):
    report(label, needle not in text, f'unexpected {needle!r}')


def assert_has_any(label, text, needles):
    ok = False
    i = 0
    while i < len(needles):
        if needles[i] in text:
            ok = True
            break
        i += 1
    report(label, ok, f'missing any of {needles!r}')


print('\n========== INT DIRECT / AUTO AUDIT ==========' )
direct_cases = [
    ('int_auto_poly_1', '1\nx^7-3*x^2+4\n1\n', '+ C'),
    ('int_auto_frac_2', '1\n1/x\n1\n', 'ln|x|'),
    ('int_auto_baseexp_3', '1\n2^x\n1\n', 'ln(2)'),
    ('int_auto_exp_4', '1\nexp(3*x-1)\n1\n', '+ C'),
    ('int_auto_logder_5', '1\n(2*x+3)/(x^2+3*x+5)\n1\n', 'ln|'),
    ('int_auto_atan_6', '1\n1/(x^2+9)\n1\n', 'atan'),
    ('int_auto_sqrt_7', '1\nsqrt(2*x+5)\n1\n', '+ C'),
    ('int_auto_sin_8', '1\nsin(x)\n1\n', '+ C'),
    ('int_auto_cos_9', '1\ncos(x)\n1\n', '+ C'),
    ('int_auto_tan_10', '1\ntan(x)\n1\n', '+ C'),
    ('int_auto_sec2_11', '1\nsec(x)^2\n1\n', '+ C'),
    ('int_auto_cosec2_12', '1\ncosec(x)^2\n1\n', '+ C'),
    ('int_auto_sectan_13', '1\nsec(x)*tan(x)\n1\n', '+ C'),
    ('int_auto_coseccot_14', '1\ncosec(x)*cot(x)\n1\n', '+ C'),
    ('int_auto_mix_15', '1\nexp(x)+x^3+1/x\n1\n', '+ C'),
]
for label, user_input, needle in direct_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n========== INT WORKING STYLE AUDIT ==========' )
working_cases = [
    ('int_working_surd_1', '1\nx/(sqrt(x^2-2*x+5))\n1\n', 'Method: Direct integration'),
    ('int_working_surd_2', '1\nx/(sqrt(x^2-2*x+5))\n1\n', 'Complete the square: x^2-2*x+5 = (x-1)^2+4.'),
    ('int_working_surd_3', '1\nx/(sqrt(x^2-2*x+5))\n1\n', 'Rewrite the numerator: x = (x-1) + 1.'),
    ('int_working_surd_4', '1\nx/(sqrt(x^2-2*x+5))\n1\n', 'I = Int[(x-1)/sqrt((x-1)^2+4)] dx + Int[1/sqrt((x-1)^2+4)] dx'),
]
for label, user_input, needle in working_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n========== INT TRIG AUDIT ==========' )
trig_cases = [
    ('int_trig_sin2_1', '1\nsin(x)^2\n3\n', '+ C'),
    ('int_trig_cos2_2', '1\ncos(x)^2\n3\n', '+ C'),
    ('int_trig_sin4_3', '1\nsin(x)^4\n3\n', '+ C'),
    ('int_trig_cos4_4', '1\ncos(x)^4\n3\n', '+ C'),
    ('int_trig_sin6_5', '1\nsin(x)^6\n3\n', '+ C'),
    ('int_trig_tan3_6', '1\ntan(x)^3\n3\n', '+ C'),
    ('int_trig_tan4_7', '1\ntan(x)^4\n3\n', '+ C'),
    ('int_trig_sincos_8', '1\nsin(x)*cos(x)\n3\n', '+ C'),
    ('int_trig_sin3cos_9', '1\nsin(x)^3*cos(x)\n3\n', '+ C'),
    ('int_trig_sin_cos2_10', '1\nsin(x)*cos(x)^2\n3\n', '+ C'),
    ('int_trig_sec4_11', '1\nsec(x)^4\n3\n', '+ C'),
    ('int_trig_cosec4_12', '1\ncosec(x)^4\n3\n', '+ C'),
    ('int_trig_same_angle_13', '1\nsin(x)^2*cos(x)^2\n3\n', '+ C'),
    ('int_trig_double_14', '1\nsin(2*x)*cos(2*x)\n3\n', '+ C'),
    ('int_trig_weighted_15', '1\n3*sin(x)^2+5*cos(x)^2\n3\n', '+ C'),
]
for label, user_input, needle in trig_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n========== INT SUBSTITUTION AUDIT ==========' )
sub_cases = [
    ('int_sub_log_1', '1\n(3*x^2+1)/(x^3+x+7)\n4\nx^3+x+7\n', 'u = x^3+x+7'),
    ('int_sub_exp_2', '1\n(2*x)*exp(x^2)\n4\nx^2\n', 'u = x^2'),
    ('int_sub_sqrt_3', '1\n(2*x+3)/sqrt(x^2+3*x+10)\n4\nx^2+3*x+10\n', 'u = x^2+3*x+10'),
    ('int_sub_trig_4', '1\ncos(3*x+1)\n4\n3*x+1\n', 'u = 3*x+1'),
    ('int_sub_inv_5', '1\n1/(2*x+5)\n4\n2*x+5\n', 'u = 2*x+5'),
    ('int_sub_power_6', '1\n(5*x+1)^4\n4\n5*x+1\n', 'u = 5*x+1'),
    ('int_sub_atan_7', '1\n3/(1+(3*x)^2)\n4\n3*x\n', 'u = 3*x'),
    ('int_sub_asin_8', '1\n1/sqrt(1-(2*x)^2)\n4\n2*x\n', 'u = 2*x'),
    ('int_sub_recip_9', '1\n1/(x*log(x))\n4\nlog(x)\n', 'u = ln(x)'),
    ('int_sub_mix_10', '1\n(6*x-4)*(3*x^2-4*x+9)^5\n4\n3*x^2-4*x+9\n', 'u = 3*x^2-4*x+9'),
    ('int_sub_trigpow_11', '1\n2*sin(x)*cos(x)\n4\nsin(x)^2\n', 'u = sin(x)^2'),
    ('int_sub_ln_12', '1\nexp(log(x))\n4\nlog(x)\n', '1/2*x^2'),
    ('int_sub_fraction_13', '1\n1/(x+1)^2\n4\nx+1\n', 'u = x+1'),
    ('int_sub_nested_14', '1\nexp((x^2+1)^3)*(6*x*(x^2+1)^2)\n4\n(x^2+1)^3\n', 'u = (x^2+1)^3'),
    ('int_sub_manual_15', '1\n1/sqrt(9-x^2)\n4\n9-x^2\n', 'asin(1/3*x)'),
]
for label, user_input, needle in sub_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n========== INT PARTS AUDIT ==========' )
parts_cases = [
    ('int_parts_xsin_1', '1\nx*sin(x)\n5\n', '+ C'),
    ('int_parts_xcos_2', '1\nx*cos(x)\n5\n', '+ C'),
    ('int_parts_xexp_3', '1\nx*exp(x)\n5\n', '+ C'),
    ('int_parts_x2exp_4', '1\nx^2*exp(x)\n5\n', '+ C'),
    ('int_parts_ln_5', '1\nlog(x)\n5\n', '+ C'),
    ('int_parts_ln2_6', '1\nlog(x)^2\n5\n', '+ C'),
    ('int_parts_ln3_7', '1\nlog(x)^3\n5\n', '+ C'),
    ('int_parts_asin_8', '1\nasin(x)\n5\n', '+ C'),
    ('int_parts_acos_9', '1\nacos(x)\n5\n', '+ C'),
    ('int_parts_atan_10', '1\natan(x)\n5\n', '+ C'),
    ('int_parts_xasin_11', '1\nx*asin(x)\n5\n', '+ C'),
    ('int_parts_xacos_12', '1\nx*acos(x)\n5\n', '+ C'),
    ('int_parts_cycle_13', '1\nexp(2*x)*sin(3*x)\n5\n', '+ C'),
    ('int_parts_cycle_14', '1\nexp(2*x)*cos(3*x)\n5\n', '+ C'),
    ('int_parts_mix_15', '1\nx*log(x)\n5\n', '+ C'),
]
for label, user_input, needle in parts_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n========== INT RATIONAL AUDIT ==========' )
rational_cases = [
    ('int_pf_linear_1', '1\n1/(x^2-1)\n6\n', 'partial fractions'),
    ('int_pf_quartic_2', '1\n1/(x^4-1)\n6\n', 'partial fractions'),
    ('int_pf_repeated_3', '1\n1/((x-1)^2)\n6\n', '+ C'),
    ('int_pf_mix_4', '1\n(x+1)/(x^2-1)\n6\n', '+ C'),
    ('int_pf_linear_quad_5', '1\n1/((x-1)*(x^2+1))\n6\n', '+ C'),
    ('int_pf_two_linear_quad_6', '1\n1/((x-1)*(x+2)*(x^2+1))\n6\n', '+ C'),
    ('int_div_poly_7', '1\n(x^4+1)/(x^2+1)\n7\n', 'Divide the numerator'),
    ('int_div_poly_8', '1\n(x^5+x+1)/(x^2+1)\n7\n', 'Divide the numerator'),
    ('int_div_poly_9', '1\n(x^3+2*x^2+5)/(x+1)\n7\n', 'Divide the numerator'),
    ('int_rational_auto_10', '1\n(x+1)/(x-2)\n1\n', '+ C'),
    ('int_rational_auto_11', '1\n1/(x^2+4*x+8)\n1\n', '+ C'),
    ('int_rational_auto_12', '1\n(2*x+3)/(x^2+3*x+2)\n1\n', '+ C'),
    ('int_rational_auto_13', '1\n1/((x+1)*(x+2))\n1\n', '+ C'),
    ('int_rational_auto_14', '1\n(x^2+3*x+1)/(x+1)\n1\n', '+ C'),
    ('int_rational_auto_15', '1\n(3*x+5)/(x^2+5*x+8)\n1\n', '+ C'),
]
for label, user_input, needle in rational_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out.lower(), needle.lower())

print('\n========== INT DE AUDIT ==========' )
de_cases = [
    ('int_de_sep_1', '2\ndy/dx=2*x*y\ny=3,x=0\n', 'y = 3*e^(x^2)'),
    ('int_de_sep_2', '2\ndy/dx=y\n\n', 'y = k*e^x'),
    ('int_de_sep_3', '2\ndy/dx=sin(x)*y\n\n', 'y ='),
    ('int_de_sep_4', '2\ndy/dx=(x+1)*y\n\n', 'y ='),
    ('int_de_sep_5', '2\ndy/dx=(1+x^2)*y\n\n', 'y ='),
    ('int_de_sep_6', '2\ndy/dx=y/(x+1)\n\n', 'y ='),
    ('int_de_linear_7', '2\ndy/dx+y=x\n\n', 'Integrating factor'),
    ('int_de_linear_8', '2\ndy/dx+2*y=3\n\n', 'Separate variables'),
    ('int_de_linear_9', '2\ndy/dx+x*y=x\n\n', 'y ='),
    ('int_de_linear_10', '2\ndy/dx-y=exp(x)\n\n', 'Integrating factor'),
    ('int_de_linear_11', '2\ndy/dx+3*y=sin(x)\n\n', 'Integrating factor'),
    ('int_de_bc_12', '2\ndy/dx+y=0\ny=2,x=0\n', 'y = 2*e^(-x)'),
    ('int_de_bc_13', '2\ndy/dx=2*x*y\ny=1,x=1\n', 'y ='),
    ('int_de_parse_14', '2\ny\n\n', 'Out of scope.'),
    ('int_de_fail_15', '2\ndy/dx=sin(y^2)\n\n', 'Out of scope.'),
]
for label, user_input, needle in de_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n========== INT OUT-OF-SCOPE / EDGE AUDIT ==========' )
edge_cases = [
    ('int_oos_1', '1\ne^(x^2)\n1\n', 'Out of scope: e^(x^2).'),
    ('int_oos_2', '1\natan(x)^2\n1\n', 'Out of scope: atan(x)^2.'),
    ('int_oos_3', '1\n1/sqrt(1-x^4)\n1\n', 'Out of scope'),
    ('int_oos_4', '1\n1/(x^2*ln(x))\n1\n', 'Out of scope'),
    ('int_oos_5', '1\nsin(x)/x\n1\n', 'Out of scope'),
    ('int_blank_6', '1\n\n1\n', 'Err: Enter f.'),
    ('int_bad_mode_7', '9\n', 'Bad mode.'),
    ('int_bad_sub_8', '1\nsin(x)\n4\n\n', '+ C'),
    ('int_weird_base_9', '1\n5^x\n1\n', '+ C'),
    ('int_long_mix_10', '1\nexp(x)+sin(x)^2+1/(x+2)+(2*x+1)/(x^2+x+5)\n1\n', '+ C'),
    ('int_long_mix_11', '1\nsec(x)^2+tan(x)^2+1/x\n1\n', '+ C'),
    ('int_long_mix_12', '1\nlog(x)^2+x*cos(x)\n1\n', '+ C'),
    ('int_long_mix_13', '1\n(x^4+1)/(x^2+1)+sin(x)^4\n1\n', '+ C'),
    ('int_long_mix_14', '1\n(3*x^2+1)/(x^3+x+7)+exp(2*x)\n1\n', '+ C'),
    ('int_long_mix_15', '1\n1/(x^2+9)+cos(x)^4+ln(x)\n1\n', '+ C'),
]
for label, user_input, needle in edge_cases:
    _code, out, _err = run_cli(user_input)
    assert_has(label, out, needle)

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed, {WARN} warnings')
print('=' * 60)
if FAIL > 0:
    print('Failures indicate integration weaknesses or unsupported edge cases worth investigation.')

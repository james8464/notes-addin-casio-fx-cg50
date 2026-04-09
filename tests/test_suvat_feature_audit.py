import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
sys.path.insert(0, str(ROOT))
sys._suvat_no_autorun = True

import SUVATprogram as sp

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


def run_solution(label, s, u, v, a, t, target, expect_error=None, expect_contains=None):
    try:
        result, equation, original_eq, sub_text = sp._build_suvat_solution_data(s, u, v, a, t, target)
        if expect_error is not None:
            report(label, result is None and expect_error.lower() in str(equation).lower(), f'got {result}, {equation}')
            return
        if result is None:
            report(label, False, f'got error {equation}')
            return
        text = sp.show(result) if not isinstance(result, list) else ' or '.join(sp.show(x) for x in result)
        if expect_contains is not None:
            report(label, expect_contains in text or expect_contains in str(equation) or (sub_text is not None and expect_contains in sub_text), f'got result={text}, eq={equation}, sub={sub_text}')
        else:
            report(label, True)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


print('\n========== SUVAT EQUATION / TARGET AUDIT ==========' )
solve_cases = [
    ('suvat_target_s_1', None, sp.num(5), None, sp.num(2), sp.num(3), 's', '24'),
    ('suvat_target_s_2', None, sp.num(0), None, sp.num(4), sp.num(5), 's', '50'),
    ('suvat_target_s_3', None, sp.num(-3), None, sp.num(2), sp.num(4), 's', '4'),
    ('suvat_target_u_4', sp.num(20), None, sp.num(14), sp.num(2), sp.num(4), 'u', '6'),
    ('suvat_target_u_5', sp.num(15), None, sp.num(9), sp.num(-2), sp.num(3), 'u', '15'),
    ('suvat_target_v_6', None, sp.num(7), None, sp.num(3), sp.num(2), 'v', '13'),
    ('suvat_target_v_7', None, sp.num(0), None, sp.num(9), sp.num(2), 'v', '18'),
    ('suvat_target_a_8', None, sp.num(3), sp.num(15), None, sp.num(4), 'a', '3'),
    ('suvat_target_a_9', None, sp.num(-2), sp.num(4), None, sp.num(3), 'a', '2'),
    ('suvat_target_t_10', None, sp.num(5), sp.num(17), sp.num(3), None, 't', '4'),
    ('suvat_target_t_11', sp.num(30), sp.num(5), sp.num(10), None, None, 't', '4'),
    ('suvat_target_vquad_12', sp.num(20), sp.num(2), None, sp.num(3), None, 'v', 'sqrt'),
    ('suvat_target_tquad_13', sp.num(20), sp.num(2), None, sp.num(3), None, 't', '2'),
    ('suvat_target_aquad_14', sp.num(12), sp.num(2), sp.num(10), None, None, 'a', '(v^2-u^2)/2s'),
    ('suvat_target_savg_15', None, sp.num(4), sp.num(10), None, sp.num(3), 's', '21'),
]
for label, s, u, v, a, t, target, expect in solve_cases:
    run_solution(label, s, u, v, a, t, target, expect_contains=expect)

print('\n========== SUVAT QUADRATIC / PHYSICAL AUDIT ==========' )
physical_error_cases = [
    ('suvat_no_real_v_1', sp.num(10), sp.num(0), None, sp.num(-5), None, 'v', 'no real solution'),
    ('suvat_no_real_t_2', sp.num(-10), sp.num(1), None, sp.num(1), None, 't', 'no real solution'),
    ('suvat_a0_vnequ_3', None, sp.num(5), sp.num(10), sp.num(0), None, 't', 'no solution'),
    ('suvat_a0_vequ_4', None, sp.num(5), sp.num(5), sp.num(0), None, 't', 'infinite'),
    ('suvat_t0_sne0_5', sp.num(10), None, sp.num(5), sp.num(2), sp.num(0), 'u', 'no solution'),
    ('suvat_s0_unv_6', sp.num(0), sp.num(4), sp.num(-4), None, None, 'a', 'no solution'),
    ('suvat_neg_roots_7', sp.num(10), sp.num(-8), None, sp.num(-1), None, 't', 'time must be positive'),
    ('suvat_zero_accel_hint_11', sp.num(10), sp.num(5), None, sp.num(0), None, 't', 't = s/u'),
    ('suvat_insufficient_12', sp.num(10), None, None, None, None, 'v', 'insufficient'),
    ('suvat_negative_inside_13', sp.num(50), sp.num(3), None, sp.num(-7), None, 'v', 'no real solution'),
    ('suvat_target_missing_14', None, None, None, None, None, 's', 'insufficient'),
    ('suvat_target_divzero_15', sp.num(10), sp.num(5), sp.num(-5), None, None, 't', 'division'),
]
for label, s, u, v, a, t, target, err in physical_error_cases:
    run_solution(label, s, u, v, a, t, target, expect_error=err)

physical_contains_cases = [
    ('suvat_positive_root_8', sp.num(10), sp.num(1), None, sp.num(3), None, 't', 'positive root'),
    ('suvat_double_root_9', sp.num(1), sp.num(2), None, sp.num(-2), None, 't', 't = 1'),
    ('suvat_zero_disc_10', sp.num(-4), sp.num(-4), None, sp.num(2), None, 't', 't = 2'),
]
for label, s, u, v, a, t, target, expect in physical_contains_cases:
    run_solution(label, s, u, v, a, t, target, expect_contains=expect)

print('\n========== SUVAT PARSER / NUMERIC AUDIT ==========' )
parse_cases = [
    ('suvat_parse_1', 'sqrt(2)'),
    ('suvat_parse_2', 'g'),
    ('suvat_parse_3', '(3+sqrt(5))/2'),
    ('suvat_parse_4', '2*g'),
    ('suvat_parse_5', '3/2'),
    ('suvat_parse_6', '2(3+4)'),
    ('suvat_parse_7', 'sin(pi/6)'),
    ('suvat_parse_8', 'cos(pi/3)'),
    ('suvat_parse_9', 'exp(1)'),
    ('suvat_parse_10', 'ln(e)'),
    ('suvat_parse_11', 'abs(-3/2)'),
    ('suvat_parse_12', 'sqrt(72)'),
    ('suvat_parse_13', '((2+3)/5)^2'),
    ('suvat_parse_14', '7.25'),
    ('suvat_parse_15', '-3/4'),
]
for label, text in parse_cases:
    try:
        node = sp.parse(text)
        report(label, node is not None, text)
    except Exception as err:
        report(label, False, f'{text}: {err}')

print('\n========== SUVAT PRESET / FORMAT AUDIT ==========' )
preset_cases = [
    ('suvat_preset_1', 'dropped from rest', ['u']),
    ('suvat_preset_2', 'free fall', ['a']),
    ('suvat_preset_3', 'gravity', ['a']),
    ('suvat_preset_4', 'maximum height', ['v']),
    ('suvat_preset_5', 'returns to ground', ['s']),
    ('suvat_preset_6', 'comes to rest', ['v']),
    ('suvat_preset_7', 'projectile from rest', ['a', 'u']),
    ('suvat_preset_8', 'thrown upwards', ['a']),
    ('suvat_preset_9', 'back to start', ['s']),
    ('suvat_preset_10', 'stationary', ['u']),
    ('suvat_preset_11', 'dropped maximum height', ['u', 'v']),
    ('suvat_preset_12', 'falls and returns to ground', ['a', 's']),
    ('suvat_preset_13', 'stops', ['v']),
    ('suvat_preset_14', 'at rest and gravity', ['u', 'a']),
    ('suvat_preset_15', 'highest point', ['v']),
]
for label, text, keys in preset_cases:
    try:
        presets = sp.detect_presets(text)
        ok = True
        i = 0
        while i < len(keys):
            if keys[i] not in presets:
                ok = False
                break
            i += 1
        report(label, ok, f'got {sorted(presets.keys())}')
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')

print('\n========== SUVAT SURD / DECIMAL / SF AUDIT ==========' )
calc_cases = [
    ('suvat_surd_1', sp.simplify_surd(8) == (2, 2)),
    ('suvat_surd_2', sp.simplify_surd(72) == (6, 2)),
    ('suvat_surd_3', sp.simplify_surd(18) == (3, 2)),
    ('suvat_surd_4', sp.simplify_surd(50) == (5, 2)),
    ('suvat_surd_5', sp.simplify_surd(1) == (1, 1)),
    ('suvat_dec_6', sp.format_decimal(1/3, 3) == '0.333'),
    ('suvat_dec_7', sp.format_decimal(22/7, 4) in ('3.1429', '3.143')),
    ('suvat_dec_8', sp.format_decimal(0.000123456, 3) == '0.000123'),
    ('suvat_dec_9', sp.format_decimal(12.0, 3) == '12'),
    ('suvat_sf_10', sp.count_sig_figs('12.0') == 3),
    ('suvat_sf_11', sp.count_sig_figs('0.0045') == 2),
    ('suvat_sf_12', sp.count_sig_figs('100') == 1),
    ('suvat_float_13', sp.node_to_float(sp.parse('g')) == 9.8),
    ('suvat_float_14', round(sp.node_to_float(sp.parse('sqrt(2)')), 6) == round(2 ** 0.5, 6)),
    ('suvat_float_15', round(sp.node_to_float(sp.parse('(3+sqrt(5))/2')), 6) == round((3 + 5 ** 0.5) / 2, 6)),
]
for label, passed in calc_cases:
    report(label, passed)

print('\n========== SUVAT SYSTEM / CONSISTENCY AUDIT ==========' )
consistency_cases = [
    ('suvat_allvars_1', (sp.num(36), sp.num(12), sp.num(0), None, None)),
    ('suvat_allvars_2', (sp.num(0), sp.num(0), None, sp.num(98, 10), sp.num(5))),
    ('suvat_allvars_3', (None, sp.num(10), sp.num(30), sp.num(5), None)),
    ('suvat_allvars_4', (sp.num(9), sp.num(0), None, sp.num(2), sp.num(3))),
    ('suvat_allvars_5', (sp.num(20), sp.num(2), None, sp.num(3), None)),
    ('suvat_allvars_6', (None, sp.num(-3), sp.num(5), sp.num(2), None)),
    ('suvat_allvars_7', (sp.num(12), sp.num(2), sp.num(10), None, None)),
    ('suvat_allvars_8', (sp.num(-50), sp.num(0), None, sp.num(-10), None)),
    ('suvat_allvars_9', (None, sp.num(1, 2), sp.num(3, 2), sp.num(1, 4), None)),
    ('suvat_allvars_10', (None, sp.num(10000), sp.num(20000), sp.num(500), None)),
    ('suvat_allvars_11', (None, sp.num(0), None, sp.parse('sqrt(2)'), sp.num(4))),
    ('suvat_allvars_12', (None, sp.num(4), None, sp.num(2), sp.parse('3/2'))),
    ('suvat_allvars_13', (sp.num(0), sp.num(0), None, sp.num(2), None)),
    ('suvat_allvars_14', (sp.num(1000), sp.num(100), None, sp.num(50), None)),
    ('suvat_allvars_15', (sp.num(10), sp.num(2), None, sp.num(2), sp.num(2))),
]
for label, vals in consistency_cases:
    try:
        result = sp.solve_all_variables(*vals)
        report(label, isinstance(result, dict) and len(result) >= 1, f'got {result}')
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed, {WARN} warnings')
print('=' * 60)
if FAIL > 0:
    print('Failures indicate SUVAT weaknesses or fragile edge-case handling.')

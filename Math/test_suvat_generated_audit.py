import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
sys.path.insert(0, str(ROOT))
sys._suvat_no_autorun = True

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


def check_solution(label, s, u, v, a, t, target, expect_exact=None, expect_error=None, equation_contains=None, sub_contains=None):
    try:
        result, equation, original_eq, sub_text = sp.build_suvat_solution(s, u, v, a, t, target)
        if expect_error is not None:
            report(label, result is None and expect_error.lower() in str(equation).lower(), f'got result={result}, equation={equation}')
            return
        if result is None:
            report(label, False, f'got error {equation}')
            return
        ok = True
        if expect_exact is not None and result != expect_exact:
            ok = False
        if equation_contains is not None and equation_contains not in str(equation):
            ok = False
        if sub_contains is not None and (sub_text is None or sub_contains not in sub_text):
            ok = False
        report(label, ok, f'got result={result}, equation={equation}, sub={sub_text}')
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


solve_cases = [
    ('suvat_gen_solve_1', None, sp.num(5), None, sp.num(2), sp.num(3), 's', '24', None, '5*3'),
    ('suvat_gen_solve_2', sp.num(30), None, sp.num(18), sp.num(2), sp.num(4), 'u', '10', None, '18 - 2*4'),
    ('suvat_gen_solve_3', None, sp.num(7), None, sp.num(3), sp.num(2), 'v', '13', 'v = u + at', '7 + 3*2'),
    ('suvat_gen_solve_4', None, sp.num(3), sp.num(15), None, sp.num(4), 'a', '3', 'a = (v-u)/t', '(15 - 3) / (4)'),
    ('suvat_gen_solve_5', None, sp.num(5), sp.num(17), sp.num(3), None, 't', '4', 't = (v-u)/a', '(17 - 5) / (3)'),
    ('suvat_gen_solve_6', sp.num(30), sp.num(5), sp.num(10), None, None, 't', '4', 't = 2s/(u+v)', '2*30 / (5 + 10)'),
    ('suvat_gen_solve_7', sp.num(12), sp.num(2), sp.num(10), None, None, 'a', '4', 'a = (v^2-u^2)/2s', '(100 - 4)'),
    ('suvat_gen_solve_8', None, sp.num(4), sp.num(10), None, sp.num(3), 's', '21', 's = 1/2(u+v)t', '1/2*(4 + 10)*3'),
    ('suvat_gen_solve_9', sp.num(16), sp.num(4), sp.num(12), None, None, 'a', '4', 'a = (v^2-u^2)/2s', '(144 - 16)'),
    ('suvat_gen_solve_10', sp.num(20), sp.num(2), None, sp.num(3), None, 'v', None, 'v^2 = u^2 + 2as', '±sqrt'),
]

quadratic_cases = [
    ('suvat_gen_quad_1', sp.num(20), sp.num(2), None, sp.num(3), None, 't', None, None, 'quadratic', 'positive root'),
    ('suvat_gen_quad_2', sp.num(12), sp.num(1), None, sp.num(2), None, 't', None, None, 'quadratic', 'positive root'),
    ('suvat_gen_quad_3', sp.num(10), sp.num(1), None, sp.num(3), None, 't', None, None, 'quadratic', 'positive root'),
    ('suvat_gen_quad_4', sp.num(2), sp.num(2), None, sp.num(2), None, 't', None, None, 'quadratic', 'positive root'),
    ('suvat_gen_quad_5', sp.num(0), sp.num(0), None, sp.num(2), None, 't', '0', None, 'quadratic', 't = 0'),
    ('suvat_gen_quad_6', sp.num(18), sp.num(6), None, sp.num(2), None, 't', None, None, 'quadratic', 'positive root'),
    ('suvat_gen_quad_7', sp.num(45), sp.num(5), None, sp.num(4), None, 't', None, None, 'quadratic', 'positive root'),
    ('suvat_gen_quad_8', sp.num(-10), sp.num(1), None, sp.num(1), None, 't', None, 'No real solution', None, None),
    ('suvat_gen_quad_9', sp.num(10), sp.num(-8), None, sp.num(-1), None, 't', None, 'time must be positive', None, None),
    ('suvat_gen_quad_10', sp.num(8), sp.num(4), None, sp.num(1), None, 't', None, None, 'quadratic', 'positive root'),
]

physical_cases = [
    ('suvat_gen_phys_1', None, sp.num(5), sp.num(10), sp.num(0), None, 't', 'No solution'),
    ('suvat_gen_phys_2', None, sp.num(5), sp.num(5), sp.num(0), None, 't', 'Infinite'),
    ('suvat_gen_phys_3', sp.num(10), None, sp.num(5), sp.num(2), sp.num(0), 'u', 'No solution'),
    ('suvat_gen_phys_4', sp.num(0), sp.num(4), sp.num(-4), None, None, 'a', 'No solution'),
    ('suvat_gen_phys_5', sp.num(10), sp.num(5), sp.num(-5), None, None, 't', 'division'),
    ('suvat_gen_phys_6', sp.num(10), sp.num(2), None, sp.num(-5), None, 'v', 'No real solution'),
    ('suvat_gen_phys_7', None, None, None, None, None, 's', 'insufficient'),
    ('suvat_gen_phys_8', sp.num(50), sp.num(3), None, sp.num(-7), None, 'v', 'No real solution'),
    ('suvat_gen_phys_9', sp.num(10), None, None, None, None, 'v', 'insufficient'),
    ('suvat_gen_phys_10', None, sp.num(5), sp.num(4), sp.num(0), None, 't', 'No solution'),
]

parser_cases = [
    ('suvat_gen_parse_1', 'sqrt(2)', 'sqrt(2)'),
    ('suvat_gen_parse_2', 'g', 'g'),
    ('suvat_gen_parse_3', '(3+sqrt(5))/2', '(sqrt(5)+3)/2'),
    ('suvat_gen_parse_4', '2*g', '2*g'),
    ('suvat_gen_parse_5', '3/2', '3/2'),
    ('suvat_gen_parse_6', '2(3+4)', '14'),
    ('suvat_gen_parse_7', 'sin(pi/6)', None),
    ('suvat_gen_parse_8', 'cos(pi/3)', None),
    ('suvat_gen_parse_9', 'abs(-3/2)', None),
    ('suvat_gen_parse_10', 'sqrt(72)', '6*sqrt(2)'),
]

preset_cases = [
    ('suvat_gen_preset_1', 'dropped from rest', ['u']),
    ('suvat_gen_preset_2', 'free fall', ['a']),
    ('suvat_gen_preset_3', 'gravity', ['a']),
    ('suvat_gen_preset_4', 'maximum height', ['v']),
    ('suvat_gen_preset_5', 'returns to ground', ['s']),
    ('suvat_gen_preset_6', 'comes to rest', ['v']),
    ('suvat_gen_preset_7', 'projectile from rest', ['a', 'u']),
    ('suvat_gen_preset_8', 'thrown upwards', ['a']),
    ('suvat_gen_preset_9', 'stationary at maximum height', ['u', 'v']),
    ('suvat_gen_preset_10', 'back to start under gravity', ['a', 's']),
]

format_cases = [
    ('suvat_gen_fmt_1', (None, sp.num(5), None, sp.num(2), sp.num(3), 's'), ('s', '24')),
    ('suvat_gen_fmt_2', (sp.num(30), sp.num(5), sp.num(10), None, None, 't'), ('t', '4')),
    ('suvat_gen_fmt_3', (sp.num(20), sp.num(2), None, sp.num(3), None, 'v'), ('v', None)),
    ('suvat_gen_fmt_4', (None, sp.num(0), None, sp.num(3), sp.parse('sqrt(2)'), 'v'), ('v', '3*sqrt(2)')),
    ('suvat_gen_fmt_5', (None, sp.num(20), sp.num(0), sp.neg(sp.parse('g')), None, 't'), ('t', '20/g')),
    ('suvat_gen_fmt_6', (sp.num(36), sp.num(12), sp.num(0), None, None, 'a'), ('a', '-2')),
    ('suvat_gen_fmt_7', (None, sp.num(1, 2), sp.num(3, 2), sp.num(1, 4), None, 's'), ('s', '4')),
    ('suvat_gen_fmt_8', (sp.num(1, 3), sp.num(1, 6), None, sp.num(1, 2), sp.num(2, 3), 'v'), ('v', None)),
    ('suvat_gen_fmt_9', (None, sp.num(0), None, sp.num(3), sp.parse('sqrt(2)'), 's'), ('s', '3')),
    ('suvat_gen_fmt_10', (sp.num(10), sp.num(1), None, sp.num(3), None, 't'), ('t', None)),
]


print('\n========== GENERATED SUVAT SOLVE AUDIT ==========')
for label, s, u, v, a, t, target, expect_exact, equation_contains, sub_contains in solve_cases:
    check_solution(label, s, u, v, a, t, target, expect_exact=expect_exact, equation_contains=equation_contains, sub_contains=sub_contains)

print('\n========== GENERATED SUVAT QUADRATIC AUDIT ==========')
for label, s, u, v, a, t, target, expect_exact, expect_error, equation_contains, sub_contains in quadratic_cases:
    check_solution(label, s, u, v, a, t, target, expect_exact=expect_exact, expect_error=expect_error, equation_contains=equation_contains, sub_contains=sub_contains)

print('\n========== GENERATED SUVAT PHYSICAL AUDIT ==========')
for label, s, u, v, a, t, target, err in physical_cases:
    check_solution(label, s, u, v, a, t, target, expect_error=err)

print('\n========== GENERATED SUVAT PARSER AUDIT ==========')
for label, text, expected in parser_cases:
    try:
        node = sp.parse(text)
        if expected is None:
            report(label, node is not None, text)
        else:
            report(label, sp.show(node) == expected, f'expected {expected}, got {sp.show(node)}')
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')

print('\n========== GENERATED SUVAT PRESET AUDIT ==========')
for label, text, expected_keys in preset_cases:
    presets = sp.detect_presets(text)
    ok = True
    i = 0
    while i < len(expected_keys):
        if expected_keys[i] not in presets:
            ok = False
            break
        i += 1
    report(label, ok, f'got {sorted(presets.keys())}')

print('\n========== GENERATED SUVAT FORMAT AUDIT ==========')
for label, values, expected in format_cases:
    s, u, v, a, t, target = values
    result, equation, original_eq, sub_text = sp._build_suvat_solution_data(s, u, v, a, t, target)
    if result is None:
        report(label, False, f'got error {equation}')
        continue
    exact, dec = sp.format_solution_texts(result, 3)
    target_name, exact_expected = expected
    lines = sp.format_output_with_units(target_name, exact, dec, equation, original_eq, sub_text, sp.VAR_UNITS.get(target_name, ''))
    all_values = sp.format_all_variables((s, u, v, a, t), target, 3)
    ok = len(lines) >= 3 and target_name in all_values
    if exact_expected is not None and exact != exact_expected:
        ok = False
    if sub_text is None or '=' not in lines[0] and len(lines) < 3:
        ok = False
    report(label, ok, f'exact={exact}, dec={dec}, equation={equation}, lines={lines}, all={all_values}')

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed')
print('=' * 60)
if FAIL > 0:
    print('Generated SUVAT stress cases exposed solving or working-style gaps.')

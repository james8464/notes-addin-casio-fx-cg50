import subprocess
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
PY = '/Users/james/Developer/Python/CASIO/.venv/bin/python'

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


def run_cli(user_input):
    proc = subprocess.run([PY, str(ROOT / 'trigProgram.py')], input=user_input, text=True, capture_output=True, check=False)
    return proc.stdout


prove_cases = [
    ('trig_prove_gen_1', '1\nsin(x)^2+cos(x)^2=1\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_2', '1\nsec(x)^2-tan(x)^2=1\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_3', '1\ncosec(x)^2-cot(x)^2=1\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_4', '1\nsin(2*x)=2*sin(x)*cos(x)\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_5', '1\ncos(2*x)=1-2*sin(x)^2\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_6', '1\ncos(2*x)=2*cos(x)^2-1\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_7', '1\nsin(3*x)=3*sin(x)-4*sin(x)^3\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_8', '1\ncos(3*x)=4*cos(x)^3-3*cos(x)\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_9', '1\nsin(x)-sin(y)=2*cos((x+y)/2)*sin((x-y)/2)\n1\n', 'LHS = RHS'),
    ('trig_prove_gen_10', '1\ncos(x)+cos(y)=2*cos((x+y)/2)*cos((x-y)/2)\n1\n', 'LHS = RHS'),
]

transform_cases = [
    ('trig_xform_gen_1', '2\nsin(x)/cos(x)\ntan(x)\n', '= tan(x)'),
    ('trig_xform_gen_2', '2\ncos(x)/sin(x)\ncot(x)\n', '= cot(x)'),
    ('trig_xform_gen_3', '2\n1/cos(x)\nsec(x)\n', '= sec(x)'),
    ('trig_xform_gen_4', '2\n1/sin(x)\ncosec(x)\n', '= cosec(x)'),
    ('trig_xform_gen_5', '2\n1-cos(2*x)\n2*sin(x)^2\n', '= 2*sin(x)^2'),
    ('trig_xform_gen_6', '2\n1+cos(2*x)\n2*cos(x)^2\n', '= 2*cos(x)^2'),
    ('trig_xform_gen_7', '2\n(1-cos(x))/sin(x)\ntan(x/2)\n', '= tan(x/2)'),
    ('trig_xform_gen_8', '2\nsin(x)/(1+cos(x))\ntan(x/2)\n', '= tan(x/2)'),
    ('trig_xform_gen_9', '2\n(1+cos(x))/sin(x)\ncot(x/2)\n', '= cot(x/2)'),
    ('trig_xform_gen_10', '2\nsin(3*x)\n3*sin(x)-4*sin(x)^3\n', '3*sin(x)-4*sin(x)^3'),
]

solve_cases = [
    ('trig_solve_gen_1', '3\nsin(x)=1/2,x,0,360\n', 'x = [30, 150]'),
    ('trig_solve_gen_2', '3\ncos(x)=-1/2,x,0,360\n', 'x = [120, 240]'),
    ('trig_solve_gen_3', '3\ntan(x)=sqrt(3),x,0,360\n', 'x = [60, 240]'),
    ('trig_solve_gen_4', '3\nsin(2*x)=0,x,0,360\n', 'x = [0, 90, 180, 270, 360]'),
    ('trig_solve_gen_5', '3\ncos(2*x)=1/2,x,0,360\n', 'x = [30, 150, 210, 330]'),
    ('trig_solve_gen_6', '3\n2*sin(x)^2-3*sin(x)+1,x,0,360\n', 'sin(x) = 1/2'),
    ('trig_solve_gen_7', '3\nsec(x)=-2,x,0,360\n', 'x = [120, 240]'),
    ('trig_solve_gen_8', '3\ncosec(x)=-2,x,0,360\n', 'x = [210, 330]'),
    ('trig_solve_gen_9', '3\n3*sin(x)+4*cos(x),max,x\n', 'Maximum value: 5'),
    ('trig_solve_gen_10', '3\n5*sin(x)-12*cos(x),min,x\n', 'Minimum value: -13'),
]

rewrite_cases = [
    ('trig_rw_gen_1', '4\n1-cos(2*x)\nsin(x)\n\n', 'Final = 2*sin(x)^2'),
    ('trig_rw_gen_2', '4\n1+cos(2*x)\ncos(x)\n\n', 'Final = 2*cos(x)^2'),
    ('trig_rw_gen_3', '4\nsin(2*x)\nsin(x)\ncos(x)\n\n', 'Final = 2*sin(x)*cos(x)'),
    ('trig_rw_gen_4', '4\nsin(x)^2+cos(x)^2\nsin(x)\ncos(x)\n\n', 'Final = 1'),
    ('trig_rw_gen_5', '4\nsec(x)^2-tan(x)^2\nsec(x)\ntan(x)\n\n', 'Final = 1'),
    ('trig_rw_gen_6', '4\ncosec(x)^2-cot(x)^2\ncosec(x)\ncot(x)\n\n', 'Final = 1'),
    ('trig_rw_gen_7', '4\n(1-cos(x))/sin(x)\ntan(x/2)\n\n', 'Final = tan(x/2)'),
    ('trig_rw_gen_8', '4\nsin(x)/(1+cos(x))\ntan(x/2)\n\n', 'Final = tan(x/2)'),
    ('trig_rw_gen_9', '4\nsin(x)-sin(y)\ncos((x+y)/2)\nsin((x-y)/2)\n\n', 'Final = 2*cos((x+y)/2)*sin((x-y)/2)'),
    ('trig_rw_gen_10', '4\ncos(x)+cos(y)\ncos((x+y)/2)\ncos((x-y)/2)\n\n', 'Final = 2*cos((x+y)/2)*cos((x-y)/2)'),
]

print('\n========== GENERATED TRIG PROVE AUDIT ==========')
for label, user_input, needle in prove_cases:
    out = run_cli(user_input)
    report(label, needle in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED TRIG TRANSFORM AUDIT ==========')
for label, user_input, needle in transform_cases:
    out = run_cli(user_input)
    report(label, needle in out and 'Use' in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED TRIG SOLVE AUDIT ==========')
for label, user_input, needle in solve_cases:
    out = run_cli(user_input)
    report(label, needle in out and ('Start with' in out or 'Maximum value:' in out or 'Minimum value:' in out), f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED TRIG REWRITE AUDIT ==========')
for label, user_input, needle in rewrite_cases:
    out = run_cli(user_input)
    report(label, needle in out and 'Final =' in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed')
print('=' * 60)
if FAIL > 0:
    print('Generated trigonometry stress cases exposed trig or working-style gaps.')

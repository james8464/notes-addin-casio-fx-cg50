import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
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


def run_cli(user_input):
    proc = subprocess.run([PY, str(ROOT / 'trigProgram.py')], input=user_input, text=True, capture_output=True, check=False)
    return proc.returncode, proc.stdout, proc.stderr


def expect_contains(label, out, needle):
    report(label, needle in out, f'missing {needle!r}\n--- OUT ---\n{out}')


print('\n========== TRIG PROVE AUDIT ==========' )
prove_cases = [
    ('trig_prove_1', '1\nsin(x)^2+cos(x)^2=1\n1\n', 'LHS = RHS'),
    ('trig_prove_2', '1\nsec(x)^2-tan(x)^2=1\n1\n', 'LHS = RHS'),
    ('trig_prove_3', '1\ncosec(x)^2-cot(x)^2=1\n1\n', 'LHS = RHS'),
    ('trig_prove_4', '1\nsin(2*x)=2*sin(x)*cos(x)\n1\n', 'LHS = RHS'),
    ('trig_prove_5', '1\ncos(2*x)=cos(x)^2-sin(x)^2\n1\n', 'LHS = RHS'),
    ('trig_prove_6', '1\n1-cos(2*x)=2*sin(x)^2\n1\n', 'LHS = RHS'),
    ('trig_prove_7', '1\n1+cos(2*x)=2*cos(x)^2\n1\n', 'LHS = RHS'),
    ('trig_prove_8', '1\nsin(x)+sin(y)=2*sin((x+y)/2)*cos((x-y)/2)\n1\n', 'LHS = RHS'),
    ('trig_prove_9', '1\ncos(x)+cos(y)=2*cos((x+y)/2)*cos((x-y)/2)\n1\n', 'LHS = RHS'),
    ('trig_prove_10', '1\ncos(x)-cos(y)=-2*sin((x+y)/2)*sin((x-y)/2)\n1\n', 'LHS = RHS'),
    ('trig_prove_11', '1\nsin(x)-sin(y)=2*cos((x+y)/2)*sin((x-y)/2)\n1\n', 'LHS = RHS'),
    ('trig_prove_12', '1\ntan(x)=sin(x)/cos(x)\n1\n', 'LHS = RHS'),
    ('trig_prove_13', '1\ncot(x)=cos(x)/sin(x)\n1\n', 'LHS = RHS'),
    ('trig_prove_14', '1\nsec(x)=1/cos(x)\n1\n', 'LHS = RHS'),
    ('trig_prove_15', '1\ncosec(x)=1/sin(x)\n1\n', 'LHS = RHS'),
]
for label, user_input, needle in prove_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n========== TRIG TRANSFORM AUDIT ==========' )
transform_cases = [
    ('trig_xform_1', '2\nsin(x)/cos(x)\ntan(x)\n', 'tan(x)'),
    ('trig_xform_2', '2\ncos(x)/sin(x)\ncot(x)\n', 'cot(x)'),
    ('trig_xform_3', '2\n1/cos(x)\nsec(x)\n', 'sec(x)'),
    ('trig_xform_4', '2\n1/sin(x)\ncosec(x)\n', 'cosec(x)'),
    ('trig_xform_5', '2\n1-cos(2*x)\n2*sin(x)^2\n', '2*sin(x)^2'),
    ('trig_xform_6', '2\n1+cos(2*x)\n2*cos(x)^2\n', '2*cos(x)^2'),
    ('trig_xform_7', '2\nsin(x)/(1+cos(x))\ntan(x/2)\n', 'tan(x/2)'),
    ('trig_xform_8', '2\n(1-cos(x))/sin(x)\ntan(x/2)\n', 'tan(x/2)'),
    ('trig_xform_9', '2\n(1+cos(x))/sin(x)\ncot(x/2)\n', 'cot(x/2)'),
    ('trig_xform_10', '2\nsin(2*x)\n2*sin(x)*cos(x)\n', '2*sin(x)*cos(x)'),
    ('trig_xform_11', '2\ncos(2*x)\ncos(x)^2-sin(x)^2\n', 'cos(x)^2-sin(x)^2'),
    ('trig_xform_12', '2\nsec(x)^2-tan(x)^2\n1\n', '= 1'),
    ('trig_xform_13', '2\ncosec(x)^2-cot(x)^2\n1\n', '= 1'),
    ('trig_xform_14', '2\nsin(3*x)\n3*sin(x)-4*sin(x)^3\n', '3*sin(x)-4*sin(x)^3'),
    ('trig_xform_15', '2\ncos(3*x)\n4*cos(x)^3-3*cos(x)\n', '4*cos(x)^3-3*cos(x)'),
]
for label, user_input, needle in transform_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n========== TRIG SOLVE AUDIT ==========' )
solve_cases = [
    ('trig_solve_1', '3\nsin(x)=0,x,0,360\n', 'x = [0, 180, 360]'),
    ('trig_solve_2', '3\ncos(x)=0,x,0,360\n', 'x = [90, 270]'),
    ('trig_solve_3', '3\ntan(x)=1,x,0,360\n', 'x = [45, 225]'),
    ('trig_solve_4', '3\nsec(x)=2,x,0,360\n', 'x = [60, 300]'),
    ('trig_solve_5', '3\ncosec(x)=2,x,0,360\n', 'x = [30, 150]'),
    ('trig_solve_6', '3\ncot(x)=1,x,0,360\n', 'x = [45, 225]'),
    ('trig_solve_7', '3\nsin(3*x)=0,x,0,360\n', 'x = [0, 60, 120, 180, 240, 300, 360]'),
    ('trig_solve_8', '3\n2*sin(x)^2-3*sin(x)+1,x,0,2*pi\n', 'sin(x) = 1/2'),
    ('trig_solve_9', '3\ncos(2*x)=1,x,0,360\n', 'x = [0, 180, 360]'),
    ('trig_solve_10', '3\n1/cos(x)=0,x,0,360\n', 'has no real solutions'),
    ('trig_solve_11', '3\n3*sin(x)+4*cos(x),max,x\n', 'Maximum value: 5'),
    ('trig_solve_12', '3\n3*sin(x)+4*cos(x),min,x\n', 'Minimum value: -5'),
    ('trig_solve_13', '3\nsin(x)=1/2,x,0,360\n', 'x = [30, 150]'),
    ('trig_solve_14', '3\ncos(x)=1/2,x,0,360\n', 'x = [60, 300]'),
    ('trig_solve_15', '3\ntan(2*x)=0,x,0,360\n', 'x = [0, 90, 180, 270, 360]'),
]
for label, user_input, needle in solve_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n========== TRIG REWRITE AUDIT ==========' )
rewrite_cases = [
    ('trig_rw_1', '4\n1-cos(2*x)\nsin(x)\n\n', 'Final = 2*sin(x)^2'),
    ('trig_rw_2', '4\n1+cos(2*x)\ncos(x)\n\n', 'Final = 2*cos(x)^2'),
    ('trig_rw_3', '4\nsin(2*x)\nsin(x)\ncos(x)\n\n', 'Final = 2*sin(x)*cos(x)'),
    ('trig_rw_4', '4\ncos(2*x)\nsin(x)\ncos(x)\n\n', 'Final ='),
    ('trig_rw_5', '4\nsin(x)^2+cos(x)^2\nsin(x)\ncos(x)\n\n', 'Final = 1'),
    ('trig_rw_6', '4\ntan(x)+cot(x)\nsin(x)\ncos(x)\n\n', 'Final ='),
    ('trig_rw_7', '4\nsec(x)^2-tan(x)^2\nsec(x)\ntan(x)\n\n', 'Final = 1'),
    ('trig_rw_8', '4\ncosec(x)^2-cot(x)^2\ncosec(x)\ncot(x)\n\n', 'Final = 1'),
    ('trig_rw_9', '4\n(1-cos(x))/sin(x)\ntan(x/2)\n\n', 'Final = tan(x/2)'),
    ('trig_rw_10', '4\nsin(x)/(1+cos(x))\ntan(x/2)\n\n', 'Final = tan(x/2)'),
    ('trig_rw_11', '4\n(1+cos(x))/sin(x)\ncot(x/2)\n\n', 'Final = cot(x/2)'),
    ('trig_rw_12', '4\nsin(x)+sin(y)\nsin((x+y)/2)\ncos((x-y)/2)\n\n', 'Final = 2*sin((x+y)/2)*cos((x-y)/2)'),
    ('trig_rw_13', '4\ncos(x)+cos(y)\ncos((x+y)/2)\ncos((x-y)/2)\n\n', 'Final = 2*cos((x+y)/2)*cos((x-y)/2)'),
    ('trig_rw_14', '4\ncos(x)-cos(y)\nsin((x+y)/2)\nsin((x-y)/2)\n\n', 'Final = -2*sin((x+y)/2)*sin((x-y)/2)'),
    ('trig_rw_15', '4\nsin(x)-sin(y)\ncos((x+y)/2)\nsin((x-y)/2)\n\n', 'Final = 2*cos((x+y)/2)*sin((x-y)/2)'),
]
for label, user_input, needle in rewrite_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed, {WARN} warnings')
print('=' * 60)
if FAIL > 0:
    print('Failures indicate trig weaknesses or unsupported identity routes worth investigation.')

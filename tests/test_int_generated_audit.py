import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
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
    proc = subprocess.run([PY, str(ROOT / 'intProgram.py')], input=user_input, text=True, capture_output=True, check=False)
    return proc.stdout


direct_cases = [
    ('int_dir_gen_1', '1\nx^9-4*x^3+7\n1\n', ['Method:', '+ C']),
    ('int_dir_gen_2', '1\n1/(3*x+5)\n1\n', ['ln|', '+ C']),
    ('int_dir_gen_3', '1\n3^x\n1\n', ['ln(3)', '+ C']),
    ('int_dir_gen_4', '1\nexp(4*x-3)\n1\n', ['Method:', '+ C']),
    ('int_dir_gen_5', '1\n(4*x+1)/(2*x^2+x+5)\n1\n', ['ln|', '+ C']),
    ('int_dir_gen_6', '1\n1/(x^2+16)\n1\n', ['atan', '+ C']),
    ('int_dir_gen_7', '1\nsqrt(5*x+11)\n1\n', ['Method:', '+ C']),
    ('int_dir_gen_8', '1\nsin(3*x+1)\n1\n', ['Method:', '+ C']),
    ('int_dir_gen_9', '1\ncos(4*x-2)\n1\n', ['Method:', '+ C']),
    ('int_dir_gen_10', '1\nexp(x)+x^4+1/x\n1\n', ['Method:', '+ C']),
]

trig_cases = [
    ('int_trig_gen_1', '1\nsin(x)^2\n3\n', ['Method: Using trigonometric identities', '+ C']),
    ('int_trig_gen_2', '1\ncos(x)^2\n3\n', ['Method: Using trigonometric identities', '+ C']),
    ('int_trig_gen_3', '1\nsin(x)^4\n3\n', ['Method: Using trigonometric identities', '+ C']),
    ('int_trig_gen_4', '1\ncos(x)^4\n3\n', ['Method: Using trigonometric identities', '+ C']),
    ('int_trig_gen_5', '1\nsin(x)^6\n3\n', ['+ C']),
    ('int_trig_gen_6', '1\ntan(x)^3\n3\n', ['+ C']),
    ('int_trig_gen_7', '1\ntan(x)^4\n3\n', ['+ C']),
    ('int_trig_gen_8', '1\nsin(x)*cos(x)\n3\n', ['Method: Using trigonometric identities', '+ C']),
    ('int_trig_gen_9', '1\nsin(x)^3*cos(x)\n3\n', ['Method: Using trigonometric identities', '+ C']),
    ('int_trig_gen_10', '1\n3*sin(x)^2+5*cos(x)^2\n3\n', ['Method: Using trigonometric identities', '+ C']),
]

sub_cases = [
    ('int_sub_gen_1', '1\n(5*x^2+2)/(5/3*x^3+2*x+9)\n4\n5/3*x^3+2*x+9\n', ['u =', 'du/dx =', '+ C']),
    ('int_sub_gen_2', '1\n(6*x)*exp(3*x^2)\n4\n3*x^2\n', ['u = 3*x^2', 'du/dx = 6*x', '+ C']),
    ('int_sub_gen_3', '1\n(4*x+7)/sqrt(2*x^2+7*x+10)\n4\n2*x^2+7*x+10\n', ['u =', 'du/dx =', '+ C']),
    ('int_sub_gen_4', '1\ncos(5*x-2)\n4\n5*x-2\n', ['u = 5*x-2', 'du/dx = 5', '+ C']),
    ('int_sub_gen_5', '1\n1/(4*x+9)\n4\n4*x+9\n', ['u = 4*x+9', 'du/dx = 4', '+ C']),
    ('int_sub_gen_6', '1\n(7*x-3)^4\n4\n7*x-3\n', ['u = 7*x-3', 'du/dx = 7', '+ C']),
    ('int_sub_gen_7', '1\n4/(1+(4*x)^2)\n4\n4*x\n', ['u = 4*x', 'du/dx = 4', '+ C']),
    ('int_sub_gen_8', '1\n1/sqrt(1-(3*x)^2)\n4\n3*x\n', ['u = 3*x', 'du/dx = 3', '+ C']),
    ('int_sub_gen_9', '1\n1/(x*log(x))\n4\nlog(x)\n', ['u = ln(x)', 'du/dx = 1/x', '+ C']),
    ('int_sub_gen_10', '1\nexp((x^2+2)^3)*(6*x*(x^2+2)^2)\n4\n(x^2+2)^3\n', ['u = (x^2+2)^3', 'du/dx =', '+ C']),
]

parts_cases = [
    ('int_parts_gen_1', '1\nx*sin(x)\n5\n', ['Method: Integration by parts', 'u = x', '+ C']),
    ('int_parts_gen_2', '1\nx*cos(x)\n5\n', ['Method: Integration by parts', 'u = x', '+ C']),
    ('int_parts_gen_3', '1\nx*exp(x)\n5\n', ['Method: Integration by parts', 'u = x', '+ C']),
    ('int_parts_gen_4', '1\nx^2*exp(x)\n5\n', ['Method: Integration by parts', 'u = x^2', '+ C']),
    ('int_parts_gen_5', '1\nlog(x)\n5\n', ['Method: Integration by parts', 'u = ln(x)', '+ C']),
    ('int_parts_gen_6', '1\nlog(x)^2\n5\n', ['Method: Integration by parts', 'u = ln(x)^2', '+ C']),
    ('int_parts_gen_7', '1\nasin(x)\n5\n', ['Method: Integration by parts', 'u = asin(x)', '+ C']),
    ('int_parts_gen_8', '1\natan(x)\n5\n', ['Method: Integration by parts', 'u = atan(x)', '+ C']),
    ('int_parts_gen_9', '1\nexp(2*x)*sin(3*x)\n5\n', ['Method: Integration by parts', '+ C']),
    ('int_parts_gen_10', '1\nx*log(x)\n5\n', ['Method: Integration by parts', 'u = ln(x)', '+ C']),
]

pf_cases = [
    ('int_pf_gen_1', '1\n1/(x^2-1)\n6\n', ['partial fractions', '+ C']),
    ('int_pf_gen_2', '1\n1/(x^4-1)\n6\n', ['partial fractions', '+ C']),
    ('int_pf_gen_3', '1\n1/((x-1)^2)\n6\n', ['+ C']),
    ('int_pf_gen_4', '1\n(x+1)/(x^2-1)\n6\n', ['Method: Partial fractions', '+ C']),
    ('int_pf_gen_5', '1\n1/((x-1)*(x^2+1))\n6\n', ['Method: Partial fractions', '+ C']),
    ('int_pf_gen_6', '1\n1/((x-1)*(x+2)*(x^2+1))\n6\n', ['Method: Partial fractions', '+ C']),
    ('int_pf_gen_7', '1\n1/(x*(x+1))\n6\n', ['partial fractions', '+ C']),
    ('int_pf_gen_8', '1\n1/((x+1)*(x+2))\n6\n', ['partial fractions', '+ C']),
    ('int_pf_gen_9', '1\n(2*x+3)/(x^2+3*x+2)\n6\n', ['partial fractions', '+ C']),
    ('int_pf_gen_10', '1\n1/((x-2)*(x+3))\n6\n', ['partial fractions', '+ C']),
]

div_cases = [
    ('int_div_gen_1', '1\n(x^4+1)/(x^2+1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_2', '1\n(x^5+x+1)/(x^2+1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_3', '1\n(x^3+2*x^2+5)/(x+1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_4', '1\n(x^6+3*x^2+1)/(x^2+1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_5', '1\n(2*x^4+3*x^2+7)/(x^2+1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_6', '1\n(x^3-1)/(x-1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_7', '1\n(x^5-32)/(x-2)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_8', '1\n(x^4+2*x^3+3)/(x^2+1)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_9', '1\n(3*x^3+2*x+1)/(x^2+2)\n7\n', ['Divide the numerator', '+ C']),
    ('int_div_gen_10', '1\n(x^5+2*x^4+3*x^2+1)/(x^2+1)\n7\n', ['Divide the numerator', '+ C']),
]

de_cases = [
    ('int_de_gen_1', '2\ndy/dx=2*x*y\ny=3,x=0\n', ['Separate variables', 'y = 3*e^(x^2)']),
    ('int_de_gen_2', '2\ndy/dx=(3*x^2+1)*y\n\n', ['Separate variables', 'y =']),
    ('int_de_gen_3', '2\ndy/dx=sin(x)*y\n\n', ['Separate variables', 'y =']),
    ('int_de_gen_4', '2\ndy/dx=y/(x+2)\n\n', ['Separate variables', 'y =']),
    ('int_de_gen_5', '2\ndy/dx=(1+x^2)*y\ny=1,x=0\n', ['Separate variables', 'y =']),
    ('int_de_gen_6', '2\ndy/dx+y=x\n\n', ['Integrating factor', 'y =']),
    ('int_de_gen_7', '2\ndy/dx+2*y=3\n\n', ['Separate variables', 'y =']),
    ('int_de_gen_8', '2\ndy/dx-y=exp(x)\n\n', ['Integrating factor', 'y =']),
    ('int_de_gen_9', '2\ndy/dx+3*y=sin(x)\n\n', ['Integrating factor', 'y =']),
    ('int_de_gen_10', '2\ndy/dx+x*y=x\n\n', ['Separate variables', 'y =']),
]


def check_all_needles(label, out, needles):
    ok = True
    i = 0
    while i < len(needles):
        if needles[i] not in out:
            ok = False
            break
        i += 1
    report(label, ok, f'missing {needles!r}\n--- OUT ---\n{out}')


print('\n========== GENERATED INT DIRECT AUDIT ==========')
for label, user_input, needles in direct_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n========== GENERATED INT TRIG AUDIT ==========')
for label, user_input, needles in trig_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n========== GENERATED INT SUBSTITUTION AUDIT ==========')
for label, user_input, needles in sub_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n========== GENERATED INT PARTS AUDIT ==========')
for label, user_input, needles in parts_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n========== GENERATED INT PARTIAL-FRACTIONS AUDIT ==========')
for label, user_input, needles in pf_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n========== GENERATED INT DIVISION AUDIT ==========')
for label, user_input, needles in div_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n========== GENERATED INT DE AUDIT ==========')
for label, user_input, needles in de_cases:
    check_all_needles(label, run_cli(user_input), needles)

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed')
print('=' * 60)
if FAIL > 0:
    print('Generated integration stress cases exposed integration or working-style gaps.')

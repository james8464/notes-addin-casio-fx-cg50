import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
PY = '/Users/james/Developer/Python/CASIO/.venv/bin/python'


def run_cli(script_name, user_input):
    proc = subprocess.run(
        [PY, str(ROOT / script_name)],
        input=user_input,
        text=True,
        capture_output=True,
        check=False,
    )
    return proc.returncode, proc.stdout, proc.stderr


def assert_has(text, needle):
    if needle not in text:
        raise AssertionError(f'missing {needle!r}\n--- out ---\n{text}')


def assert_not_has(text, needle):
    if needle in text:
        raise AssertionError(f'unexpected {needle!r}\n--- out ---\n{text}')


def test_derive_blank_input_is_clean():
    _code, out, _err = run_cli('deriveProgram.py', '1\n\n')
    assert_has(out, 'Err: Enter an expression.')
    assert_not_has(out, 'Unexpected end.')


def test_int_blank_input_is_clean():
    _code, out, _err = run_cli('intProgram.py', '1\n\n1\n')
    assert_has(out, 'Err: Enter f.')
    assert_not_has(out, 'Unexpected end.')


def test_trig_blank_prove_is_clean():
    _code, out, _err = run_cli('trigProgram.py', '1\n\n1\n')
    assert_has(out, 'Err: Enter an identity.')


def test_trig_blank_transform_is_clean():
    _code, out, _err = run_cli('trigProgram.py', '2\n\n\n')
    assert_has(out, 'Err: Enter E1 and E2.')
    assert_not_has(out, 'Unexpected end.')


def test_trig_blank_solve_is_clean():
    _code, out, _err = run_cli('trigProgram.py', '3\n\n')
    assert_has(out, 'Err: Enter an equation.')


def test_trig_blank_rewrite_is_clean():
    _code, out, _err = run_cli('trigProgram.py', '4\n\n\n')
    assert_has(out, 'Err: Enter an expression.')


def test_trig_bad_variable_message():
    _code, out, _err = run_cli('trigProgram.py', '2\nsin(x)\nsin(y)\n')
    assert_has(out, 'Err: Equation 2 uses a different trig variable.')


def test_trig_no_solution_message():
    _code, out, _err = run_cli('trigProgram.py', '3\n1/cos(x)=0,x,0,360\n')
    assert_has(out, 'has no real solutions')


def test_algebra_blank_inverse_keeps_default():
    _code, out, _err = run_cli('algebraProgram.py', '8\n\n')
    assert_has(out, 'Use: f=2*x+1')
    assert_has(out, 'f^-1(x) = (x-1)/2')


def test_algebra_retry_rational_solve_path():
    _code, out, _err = run_cli('algebraProgram.py', '6\n(x+1)/(x-2)=3\n')
    assert_has(out, 'Clear den')
    assert_has(out, 'x = 7/2')


def test_trig_lines_are_shorter():
    _code, out, _err = run_cli('trigProgram.py', '2\n1-cos(2*x)\n2*sin(x)^2\n')
    assert_has(out, 'Use 1 - cos(2A) = 2sin^2 A')
    assert_not_has(out, 'Rewrite in terms of sin(2x), cos(2x) and constants')


def test_trig_sin3_proof_family():
    _code, out, _err = run_cli('trigProgram.py', '1\nsin(3*x)=3*sin(x)-4*sin(x)^3\n1\n')
    assert_has(out, '= -4*sin(x)^3+3*sin(x)')
    assert_has(out, '= RHS')


def test_trig_cos5_proof_family():
    _code, out, _err = run_cli('trigProgram.py', '1\ncos(5*x)=16*cos(x)^5-20*cos(x)^3+5*cos(x)\n1\n')
    assert_has(out, '= -20*cos(x)^3+16*cos(x)^5+5*cos(x)')
    assert_has(out, '= RHS')


def test_trig_sin3_solve_family():
    _code, out, _err = run_cli('trigProgram.py', '3\nsin(3*x)=0,x,0,360\n')
    assert_has(out, 'x = [0, 60, 120, 180, 240, 300, 360]')


def test_int_non_elementary_message_is_specific():
    _code, out, _err = run_cli('intProgram.py', '1\ne^(x^2)\n1\n')
    assert_has(out, 'Out of scope: e^(x^2).')


def test_int_retry_finds_working_method():
    _code, out, _err = run_cli('intProgram.py', '1\nsin(x)*cos(x)\n2\n')
    assert_not_has(out, 'Out of scope.')
    assert_has(out, '+ C')


def test_derive_hyperbolic_support():
    _code, out, _err = run_cli('deriveProgram.py', '1\nsinh(x)\n')
    assert_has(out, 'sinh rule')
    assert_has(out, 'dy/dx = cosh(x)')


if __name__ == '__main__':
    tests = [
        obj for name, obj in sorted(globals().items())
        if name.startswith('test_') and callable(obj)
    ]
    passed = 0
    for test in tests:
        test()
        print('PASS', test.__name__)
        passed += 1
    print(f'PASS {passed} tests')

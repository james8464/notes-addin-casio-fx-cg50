import subprocess
import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
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


def test_trig_no_spurious_c():
    _code, out, _err = run_cli('trigProgram.py', '2\n(9*cos(x)**2)+sin(x)**2\na+(b*cos(2x))\n')
    assert_has(out, 'a = 5')
    assert_has(out, 'b = 4')
    assert_not_has(out, 'c =')


def test_trig_custom_placeholder_names():
    _code, out, _err = run_cli('trigProgram.py', '2\n(9*cos(x)**2)+sin(x)**2\nm+(n*cos(2x))\n')
    assert_has(out, 'm = 5')
    assert_has(out, 'n = 4')
    assert_not_has(out, 'a = 5')


def test_algebra_inverse_linear_fractional():
    _code, out, _err = run_cli('algebraProgram.py', '8\n(2*x+1)/(3*x+4)\n')
    assert_has(out, 'f^-1(x) = (-4*x+1)/(3*x-2)')


def test_algebra_inverse_recip_power():
    _code, out, _err = run_cli('algebraProgram.py', '8\n1/(x+1)^3\n')
    assert_has(out, 'f^-1(x) = 1/x^(1/3)-1')


def test_algebra_inverse_sqrt_power():
    _code, out, _err = run_cli('algebraProgram.py', '8\nsqrt((2*x+3)^3)\n')
    assert_has(out, 'f^-1(x) = ((x)^(2/3)-3)/2')


def test_algebra_quadratic_solve():
    _code, out, _err = run_cli('algebraProgram.py', '6\nx^2+5*x+6=0\n')
    assert_has(out, 'Solve quad')
    assert_has(out, 'x = [-2, -3]')


def test_algebra_rational_solve():
    _code, out, _err = run_cli('algebraProgram.py', '6\n(x+1)/(x-2)=3\n')
    assert_has(out, 'Clear den')
    assert_has(out, 'x = 7/2')


def test_algebra_fraction_sum_solve():
    _code, out, _err = run_cli('algebraProgram.py', '6\n1/(x+1)+1/(x-2)=0\n')
    assert_has(out, 'Clear den')
    assert_has(out, 'x = 1/2')


def test_algebra_rewrite_in_u():
    _code, out, _err = run_cli('algebraProgram.py', '9\nx^2+5*x+6\n2*x+1\n\n')
    assert_has(out, 'u = 2*x+1')
    assert_has(out, '= (1/4)*(T)^2+2*T+15/4')


def test_algebra_rewrite_in_reciprocal_shift_term():
    _code, out, _err = run_cli('algebraProgram.py', '9\nx\n1/(x+3)+4\n\n')
    assert_has(out, 'u = 1/(x+3)+4')
    assert_has(out, '= 1/(T-4)-3')


def test_derive_log_diff_text():
    _code, out, _err = run_cli('deriveProgram.py', '1\nx^x\n')
    assert_has(out, 'Log diff')
    assert_has(out, "dy/dx = y*(u'/u+ln(u)*v')")
    assert_not_has(out, 'v*du/u')


def test_int_method_line_is_not_doubled():
    _code, out, _err = run_cli('intProgram.py', '1\n1/x\n2\n')
    assert_has(out, 'Met std')
    assert_not_has(out, 'Met: Met:')


def test_suvat_requires_explicit_target_for_multiple_blanks():
    _code, out, _err = run_cli('SUVATprogram.py', '10\n\n\n\n\n')
    assert_has(out, 'Multiple unknowns detected')


def test_suvat_fix_sub_u_text():
    sys.path.insert(0, str(ROOT))
    sys._suvat_no_autorun = True
    import SUVATprogram as sp

    text = sp._sub_u(sp.num(10), None, None, sp.num(2), sp.num(3))
    assert_has(text, '1/2*2*9')
    assert_not_has(text, '1/2*2*3) / (3)')


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

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "src" / "Math"
sys.path.insert(0, str(ROOT))


def assert_true(label, condition, detail=""):
    if not condition:
        raise AssertionError(label + (": " + detail if detail else ""))
    print("PASS", label)


def main():
    sys._int_no_autorun = True
    sys._algebra_no_autorun = True
    sys._trig_no_autorun = True

    from intProgram import parse as int_parse, solve as int_solve, show as int_show, diff as int_diff, sim as int_sim
    from algebraProgram import parse as alg_parse, solve_rewrite_text as alg_rewrite
    from trigProgram import parse as trig_parse, solve_rewrite_text as trig_rewrite, solve_prove_text

    title, ans, lines = int_solve(int_parse("exp(x)/(1+exp(x))"), "x", "4")
    assert_true("int substitution exp-over-one-plus-exp answer", ans is not None, "expected an antiderivative")
    assert_true(
        "int substitution exp-over-one-plus-exp working",
        any("u =" in line for line in lines or []) and any("ln|1+e^x|" in line or "ln|e^x+1|" in line for line in lines or []),
        str(lines),
    )
    assert_true(
        "int substitution exp-over-one-plus-exp value",
        int_show(ans) in ("ln|e^x+1|", "ln|1+e^x|"),
        int_show(ans),
    )
    auto_title, auto_ans, _auto_lines = int_solve(int_parse("1/(2*x+3)"), "x", "auto")
    assert_true("int auto linear-over-linear answer", auto_ans is not None, auto_title)
    auto_diff = int_sim(int_diff(auto_ans, "x"))
    assert_true("int diff handles ln abs antiderivative", int_show(auto_diff) == "1/(2*x+3)", int_show(auto_diff))

    alg_lines = alg_rewrite(alg_parse("sin(x)^2+cos(x)^2"), ["1"])
    assert_true("algebra rewrite accepts parsed expr", any("1" in line for line in alg_lines), str(alg_lines))

    trig_lines = trig_rewrite(trig_parse("sin(x)^2+cos(x)^2"), ["1"])
    assert_true("trig rewrite accepts parsed expr", any("1" in line for line in trig_lines), str(trig_lines))

    prove_lines = solve_prove_text("log(2, sin(x)^2)=2*log(2, sin(x))", "auto")
    assert_true("trig prove log power rule note", any("log_b(A^n)" in line for line in prove_lines), str(prove_lines))
    assert_true("trig prove log power rule finishes", prove_lines[-1] == "LHS = RHS", str(prove_lines))

    print("All cross-program API regressions passed.")


if __name__ == "__main__":
    main()

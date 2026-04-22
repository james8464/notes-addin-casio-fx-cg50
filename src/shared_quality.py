try:
    from src.shared_reasoning_markers import REASONING_MARKERS
except ImportError:
    from shared_reasoning_markers import REASONING_MARKERS


def _normalise(output):
    return (output or "").strip()


def has_working_steps(output):
    text = _normalise(output)
    if not text:
        return False
    lower = text.lower()
    return any(marker.lower() in lower for marker in REASONING_MARKERS)


def has_solution(output):
    lower = _normalise(output).lower()
    return any(
        marker in lower
        for marker in (
            "answer:",
            "ans =",
            "final =",
            "result:",
            "out =",
            "x =",
            "dy/dx",
            "+ c",
            "lhs = rhs",
            "f^-1",
            "f(g(x))",
            "s =",
            "u =",
            "v =",
            "a =",
            "t =",
            "no solution",
            "no sol",
        )
    )


def is_exam_format(output):
    text = _normalise(output)
    if not text:
        return False
    lines = [line for line in text.splitlines() if line.strip()]
    if len(lines) < 2:
        return False
    return has_working_steps(text) and has_solution(text)

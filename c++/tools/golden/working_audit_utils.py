from __future__ import annotations


def compact_math(text: str) -> str:
    s = (text or "").lower().replace("\r", "")
    for a, b in (
        ("\n", ""),
        ("\t", ""),
        (" ", ""),
        ("[", "("),
        ("]", ")"),
        ("1*", ""),
        ("ln(", "log("),
        ("ln|", "log|"),
        ("lnabs", "logabs"),
        ("back-substitutet=", "t="),
        ("dcolumn:", "d:"),
        ("icolumn:", "i:"),
        ("partialfractions", "pf"),
        ("integralbecomes", "i="),
        ("theintegralbecomes", "i="),
        ("differentiate", "diff"),
        ("differentiating", "diff"),
        ("hypotenuse", "hyp"),
        ("adjacent", "adj"),
        ("opposite", "opp"),
        ("integral", "int"),
        ("answer:", ""),
        ("method:", ""),
    ):
        s = s.replace(a, b)
    return s


def final_math_line(text: str) -> str:
    for line in reversed((text or "").splitlines()):
        line = line.strip()
        if line:
            return line
    return ""


def has_strong_output(text: str) -> bool:
    low = (text or "").lower()
    if "err:" in low or "not recognised" in low or "not recognized" in low:
        return False
    if "integral not recognised" in low or "could not get working" in low:
        return False
    if "method: integration (limited)" in low:
        return False
    return bool(final_math_line(text))


def markers_present(text: str, markers: list[str]) -> bool:
    flat = compact_math(text)
    for marker in markers:
        if not marker:
            continue
        forms = [compact_math(marker)]
        low = marker.strip().lower()
        if low.startswith("let "):
            forms.append(compact_math(marker.strip()[4:]))
        if low.startswith("use "):
            forms.append(compact_math(marker.strip()[4:]))
        if not any(form in flat for form in forms):
            return False
    return True

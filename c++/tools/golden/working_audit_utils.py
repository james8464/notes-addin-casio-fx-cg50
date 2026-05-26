from __future__ import annotations

import re


def strip_zero_surd_prefix(s: str) -> str:
    s = re.sub(r"(^|(?<=[=,(]))0\+(?=(?:\d+\*)?sqrt\()", r"\1", s)
    s = re.sub(r"(^|(?<=[=,(]))0-(?=(?:\d+\*)?sqrt\()", r"\1-", s)
    return s


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
    flat_nogroup = flat.replace("(", "").replace(")", "")
    singleton_list = re.compile(r"^([a-z][a-z0-9_]*)=\(([^,()]+(?:\([^)]*\)[^,()]*)*)\)$")
    def flip_zero_poly(form: str) -> str | None:
        if not form.startswith("expand=>") or not form.endswith("=0"):
            return None
        body = form[len("expand=>") : -2]
        out: list[str] = []
        for i, ch in enumerate(body):
            if ch == "+":
                out.append("-")
            elif ch == "-":
                out.append("+")
            elif i == 0:
                out.append("-")
                out.append(ch)
            else:
                out.append(ch)
        flipped = "".join(out)
        if flipped.startswith("+"):
            flipped = flipped[1:]
        return "expand=>" + flipped + "=0"

    def quotient_parts(form: str) -> tuple[str, str] | None:
        if "=" not in form or "/" not in form:
            return None
        rhs = form.split("=", 1)[1]
        slash = rhs.rfind("/")
        if slash <= 0:
            return None
        return rhs[:slash].replace("(", "").replace(")", ""), rhs[slash + 1 :].replace("(", "").replace(")", "")

    for marker in markers:
        if not marker:
            continue
        forms = [compact_math(marker)]
        forms += [strip_zero_surd_prefix(form) for form in list(forms)]
        for form in list(forms):
            m = singleton_list.match(form)
            if m:
                forms.append(f"{m.group(1)}={m.group(2)}")
        low = marker.strip().lower()
        if low.startswith("let "):
            forms.append(compact_math(marker.strip()[4:]))
        if low.startswith("use "):
            forms.append(compact_math(marker.strip()[4:]))
        quotient_form = "dy/d" in low and "*" in low and "/" in low
        if not any(form in flat for form in forms):
            if quotient_form and any(form.replace("(", "").replace(")", "") in flat_nogroup for form in forms):
                continue
            if quotient_form:
                parts = [quotient_parts(form) for form in forms]
                if any(p and p[0] in flat_nogroup and p[1] in flat_nogroup for p in parts):
                    continue
            flipped = [flip_zero_poly(form) for form in forms]
            if any(form and form in flat for form in flipped):
                continue
            return False
    return True

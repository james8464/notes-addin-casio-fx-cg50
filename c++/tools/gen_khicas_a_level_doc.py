#!/usr/bin/env python3
"""Generate fx-CG50 KhiCAS A-level function guide from repo allowlist.

Allowlist policy (giac90_1addin / shipped .g3a):
- Device lexer: static_lexer.h via kglobal.cc builtin_lexer_functions[].
  Never use static_lexer_full.h alone (desktop-only symbols, ~1200 false positives).
- Union names: (static_lexer.h minus REMOVED_LEXER_NAMES) | active F4 catalog |
  F-key menus (textGUI.cpp) | main.cc rewrite/alias hooks | #if 0 catalog surfaces
  that remain in the device lexer (e.g. desolve — F4 shows de_solve instead).
- Appendix only: check_khicas_catalog REMOVED_* runtime blocks, MAIN_BODY_EXCLUDE
  (implicit_diff, plot*, mean, taylor, …), and symbols not in the union above.
"""
from __future__ import annotations

import importlib.util
import re
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
KHICAS = ROOT / "c++/khicas/upstream/giac90_1addin"
OUT = Path("/Users/james/Downloads/khicas-a-level-functions.md")
SRC_MD = OUT  # reuse existing entries when present

# User-verified: not on typical downloaded add-in (omit from tutorials / A-level body)
MAIN_BODY_EXCLUDE = frozenset(
    {
        "implicit_diff",
        "implicitplot",
        "plotfunc",
        "mean",
        "median",
        "stddev",
        "linsolve",
        "taylor",
        "tangent_line",
        "plotparam",
        "plotode",
        "det",
        "inv",
        "rref",
        "eigenvals",
        "newton",
        "rsolve",
    }
)

REWRITE_TARGETS = {
    "de_solve": "desolve",
    "tangent_line": "linetan",
    "fitconst": "solve",
    "suvat": "solve (with working steps)",
    "binom_expand": "expand",
    "rewrite": "canonical_form",
    "match": "solve",
    "poly": "factor",
    "complete_square": "canonical_form",
    "cartesian": "eliminate",
    "normalcdf": "normal_cdf",
    "int": "integrate",
    "defint": "integrate (definite)",
}

REWRITE_ONLY = frozenset(
    {
        "implicit_diff",
        "param_diff",
        "normal_diff",
        "compose",
        "inverse",
        "coeff_match",
        "trig_transform",
        "integrate_by",
        "diff_by",
        "solve_by",
        "solve_trig_by",
        "solve_trig",
    }
)

# Detailed A-level tutorials (full allowlist table is wider).
ALEVEL_PRIORITY = frozenset(
    {
        "solve",
        "fsolve",
        "fitconst",
        "match",
        "suvat",
        "diff",
        "integrate",
        "int",
        "defint",
        "limit",
        "sum",
        "desolve",
        "de_solve",
        "param_diff",
        "expand",
        "factor",
        "simplify",
        "partfrac",
        "coeff",
        "complete_square",
        "binom_expand",
        "binom_coeff",
        "compare",
        "domain",
        "range",
        "rewrite",
        "subst",
        "xform",
        "trig_prove",
        "trig_rewrite",
        "trigcos",
        "trigsin",
        "trigtan",
        "tcollect",
        "texpand",
        "tlin",
        "binomial",
        "binomcdf",
        "normalcdf",
        "normal_cdf",
        "binom",
    }
)


def without_if0(text: str) -> str:
    out: list[str] = []
    skip = 0
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("//"):
            continue
        if skip:
            if s.startswith("#if"):
                skip += 1
            elif s == "#endif":
                skip -= 1
            continue
        if s == "#if 0":
            skip = 1
            continue
        out.append(line)
    return "\n".join(out)


def cmd_name(surface: str) -> str:
    if "(" not in surface:
        return surface.strip().lower()
    return surface.split("(", 1)[0].strip().split()[0].lower()


def load_check_catalog():
    spec = importlib.util.spec_from_file_location(
        "ck", ROOT / "c++/tools/check_khicas_catalog.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def f4_visible() -> list[tuple[str, str]]:
    catalog_text = (KHICAS / "catalogen.cpp").read_text(errors="ignore")
    catalog = without_if0(catalog_text)
    cat_ids = {
        k: int(v)
        for k, v in re.findall(r"#define CAT_CATEGORY_(\w+)\s+(\d+)", catalog_text)
    }
    hidden = {2, 5, 6, 12, 14, 15, 16, 17, 19}
    pat = re.compile(
        r'\{\s*"([^"]+)"[^}]*?,\s*0,\s*0,\s*0,\s*0,\s*([^}]+)\}', re.DOTALL
    )

    def visible(expr: str) -> bool:
        ids = [
            cat_ids[m.group(1)]
            for m in re.finditer(r"CAT_CATEGORY_(\w+)", expr)
            if m.group(1) in cat_ids
        ]
        return bool(ids) and any(i not in hidden for i in ids)

    out: list[tuple[str, str]] = []
    for surf, cat in pat.findall(catalog):
        if "(" in surf and visible(cat):
            out.append((cmd_name(surf), surf))
    return sorted(out, key=lambda x: x[0])


def fkey_menu() -> dict[str, str]:
    tg = (KHICAS / "textGUI.cpp").read_text(errors="ignore")
    m = re.search(r'le_menu \+= "([^"]+)"', tg)
    if not m:
        return {}
    raw = m.group(1).replace("\\n", "\n")
    paths: dict[str, str] = {}
    cur = "F-keys"
    for line in raw.split("\n"):
        if line.startswith("F") and " " in line and "(" not in line:
            cur = line.split()[0] + " " + line.split()[1]
            continue
        mm = re.match(r"([a-z_][a-z0-9_]*)\(", line)
        if mm:
            paths[mm.group(1)] = cur
    return paths


def device_lexer() -> set[str]:
    text = without_if0((KHICAS / "static_lexer.h").read_text(errors="ignore"))
    return {m.group(1).lower() for m in re.finditer(r'\{\s*"([^"]+)"', text)}


def inactive_catalog_surfaces() -> dict[str, str]:
    """catalogen.cpp surfaces inside #if 0 (still may be in device lexer)."""
    text = (KHICAS / "catalogen.cpp").read_text(errors="ignore")
    out: dict[str, str] = {}
    in_if0 = False
    for line in text.splitlines():
        s = line.strip()
        if s == "#if 0":
            in_if0 = True
            continue
        if in_if0 and s == "#endif":
            in_if0 = False
            continue
        if not in_if0:
            continue
        m = re.match(r'\{\s*"([^"]+)"', s)
        if m and "(" in m.group(1):
            surf = m.group(1)
            out[cmd_name(surf)] = surf
    return out


def simple_aliases() -> dict[str, str]:
    main = (KHICAS / "main.cc").read_text(errors="ignore")
    if "simple_aliases" not in main:
        return {}
    block = main.split("simple_aliases", 1)[1][:1200]
    return {
        a.lower(): b.lower()
        for a, b in re.findall(
            r'\{"([a-z_][a-z0-9_]*)\(","([a-z_][a-z0-9_]*)"\}', block
        )
    }


def rewrite_names() -> set[str]:
    main = (KHICAS / "main.cc").read_text(errors="ignore")
    names = set(
        re.findall(r'cascas_rewrite_\w+_call\(input,"([a-z_]+)\(', main)
    )
    names |= set(re.findall(r'cascas_rewrite_param_call\(input,"([a-z_]+)"', main))
    names |= set(
        a
        for a, _ in re.findall(
            r'\{"([a-z_][a-z0-9_]*)\(","([a-z_][a-z0-9_]*)"\}',
            main.split("simple_aliases", 1)[1][:2500]
            if "simple_aliases" in main
            else "",
        )
    )
    names |= {
        "implicit_diff",
        "param_diff",
        "de_solve",
        "complete_square",
        "fitconst",
        "match",
        "binom_expand",
        "binom_coeff",
        "normalcdf",
        "trig_prove",
        "trig_rewrite",
        "trigcos",
        "trigsin",
        "trigtan",
        "xform",
        "rewrite",
        "suvat",
        "solve_trig",
        "integrate_by",
        "diff_by",
        "solve_by",
        "solve_trig_by",
        "normal_diff",
        "compose",
        "inverse",
        "coeff_match",
        "trig_transform",
        "transform",
        "cartesian",
        "tangent_line",
    }
    return names


def build_allowlist():
    ck = load_check_catalog()
    removed_lex = {x.lower() for x in ck.REMOVED_LEXER_NAMES}
    removed_cat = {cmd_name(x) for x in ck.REMOVED_SURFACES}
    removed = removed_lex | removed_cat
    f4_map = dict(f4_visible())
    inactive = inactive_catalog_surfaces()
    menu = fkey_menu()
    lex = device_lexer()
    device_lex = lex - removed_lex
    aliases = simple_aliases()
    alias_targets = set(aliases.values())
    rew = rewrite_names()

    surfaces = {**inactive, **f4_map}
    names = sorted(
        set(f4_map)
        | set(menu)
        | device_lex
        | set(aliases)
        | alias_targets
        | rew
    )

    rows: list[dict] = []
    for n in names:
        if n in removed_lex:
            continue
        if (
            n in removed_cat
            and n not in device_lex
            and n not in menu
            and n not in f4_map
            and n not in rew
            and n not in aliases
        ):
            continue
        tags: list[str] = []
        if n in f4_map:
            tags.append("[F4]")
        elif n in inactive:
            tags.append("[F4 hidden]")
        if n in menu:
            tags.append(f"[{menu[n]}]")
        if n in device_lex:
            tags.append("[lexer]")
        elif n in rew or n in aliases:
            tgt = REWRITE_TARGETS.get(n) or aliases.get(n)
            tags.append(f"[alias→{tgt}]" if tgt else "[alias]")
        elif n in MAIN_BODY_EXCLUDE:
            tags.append("[blocked]")
        else:
            tags.append("[catalog/menu]")
        appendix_only = n in MAIN_BODY_EXCLUDE or (
            n in removed_cat and n not in device_lex
        )
        on_device = not appendix_only and (
            n in device_lex
            or n in menu
            or n in f4_map
            or n in inactive
            or n in rew
            or n in aliases
        )
        surf = surfaces.get(n, f"{n}(...)")
        rows.append(
            {
                "name": n,
                "surface": surf,
                "tags": " ".join(tags),
                "menu": menu.get(n, ""),
                "in_lexer": n in device_lex,
                "rewrite": n in rew or n in aliases,
                "main": on_device,
                "alevel": on_device and n in ALEVEL_PRIORITY,
            }
        )
    main_allow = [r for r in rows if r["main"]]
    alevel_allow = [r for r in main_allow if r["alevel"]]
    appendix = [r for r in rows if not r["main"]]
    return (
        rows,
        main_allow,
        alevel_allow,
        appendix,
        removed,
        f4_map,
        inactive,
        menu,
        lex,
        device_lex,
        rew,
        aliases,
    )


def extract_sections(md: str) -> dict[str, str]:
    pat = re.compile(r"^### `([^`]+)`\s*$", re.MULTILINE)
    matches = list(pat.finditer(md))
    stop = len(md)
    for hdr in (
        "## Registered in giac",
        "## F4 calculator catalog",
        "## Lower priority",
        "## Sources scanned",
    ):
        p = md.find(hdr)
        if 0 <= p < stop:
            stop = p
    sections: dict[str, str] = {}
    for i, m in enumerate(matches):
        if m.start() >= stop:
            break
        name = m.group(1).lower()
        start = m.start()
        end = matches[i + 1].start() if i + 1 < len(matches) else stop
        if end > stop:
            end = stop
        chunk = md[start:end]
        chunk = re.split(r"^### [^`\n].*$", chunk, maxsplit=1, flags=re.MULTILINE)[0]
        sections[name] = chunk.rstrip() + "\n"
    return sections


def strip_tags_blocks(text: str) -> str:
    return re.sub(r"^\*\*Tags:\*\*[^\n]*\n+", "", text, flags=re.MULTILINE)


def clean_section(text: str) -> str:
    drop = (
        "implicit_diff",
        "taylor",
        "TAYLOR",
        "plotfunc",
        "plotparam",
        "plotode",
        "mean",
        "linsolve",
        "newton",
        "rsolve",
    )
    for name in drop:
        text = re.sub(
            rf",?\s*\[`{re.escape(name)}`[^\]]*\]\([^\)]*\)",
            "",
            text,
        )
        text = re.sub(rf",?\s*`{re.escape(name)}`", "", text)
    text = re.sub(r"\*\*Related commands:\*\*\s*$", "", text, flags=re.MULTILINE)
    text = re.sub(r"\*\*Related commands:\*\*\s*,", "**Related commands:**", text)
    return text


def good_section(name: str, sec: str | None) -> str | None:
    if not sec or "**In plain English**" not in sec:
        return None
    head = f"### `{name}`"
    if not sec.lstrip().startswith(head):
        return None
    return sec


def default_entry(name: str, tags: str, blurb: str, example: str) -> str:
    return f"""### `{name}`

**Tags:** {tags}

**In plain English**  
{blurb}

**How to type it in KhiCAS**  
```
{example}
```

**Examples**
- Type: `{example}`
- Press **EXE** and check the result on your calculator.

**Related commands:** see [allowlist](#commands-that-work-on-your-calculator).

"""


def appendix_entry(name: str, reason: str, workaround: str = "") -> str:
    lines = [f"### `{name}`", "", f"**Why not on fx-CG50 KhiCAS:** {reason}", ""]
    if workaround:
        lines += ["**Workaround:**", "```", workaround, "```", ""]
    return "\n".join(lines)


def desolve_section(tags: str, sections: dict[str, str]) -> str:
    sec = good_section("desolve", sections.get("desolve")) or good_section(
        "de_solve", sections.get("de_solve")
    )
    if sec:
        sec = clean_section(sec)
        if not sec.lstrip().startswith("### `desolve`"):
            sec = re.sub(r"^### `de_solve`", "### `desolve`", sec, count=1, flags=re.MULTILINE)
        sec = re.sub(
            r"\*\*Also known as:\*\*[^\n]+\n+",
            "",
            sec,
            count=1,
        )
        sec = strip_tags_blocks(sec)
        sec = sec.replace(
            "**In plain English**",
            f"**Tags:** {tags}\n\n**Also type:** `de_solve(...)` — **F4** / **F8 CALC** menu; rewritten to `desolve` in `main.cc`. Both names execute on the calculator.\n\n**In plain English**",
            1,
        )
        if "desolve(diff" not in sec:
            sec = sec.replace(
                "```\nde_solve(diff",
                "```\ndesolve(diff(y,x)=k*y, [y(0)=1])\ndesolve(diff(y,x)=k*y, x, y)\nde_solve(diff",
                1,
            )
        return sec
    return f"""### `desolve`

**Tags:** {tags}

**Also type:** `de_solve(...)` — **F4** catalog and **F8 CALC** soft key (`de_solve(` → `desolve(` in `main.cc`). Both names work; `desolve` is the native Giac lexer symbol (`static_lexer.h`, `kdesolve.cc`).

**In plain English**  
Solves ordinary differential equations (ODEs) with optional initial or boundary conditions. Finds y(x) when you give y′ (or a higher-order equation) plus conditions such as y(0)=1.

**When you'd use it at A-level**  
Exponential growth/decay (dy/dx = ky); Newton's law of cooling; simple separable DE questions in Further Maths.

**How to type it in KhiCAS**  
```
desolve(diff(y,x)=k*y, x, y)
desolve(diff(y,x)=k*y, [y(0)=1])
de_solve(diff(y,x)=k*y, [y(0)=1])
desolve([y'+y=exp(x), y(0)=1], x, y)
```
- **F4** → `de_solve(equation,[bc])` or type `desolve` directly.
- **F8 CALC** → `de_solve(` soft key.
- Prime notation: **F3** for `'` (e.g. `y'=k*y`).

**Examples**
- `desolve(diff(y,x)=k*y, [y(0)=1])` → `exp(k*x)` *(typical)*
- `de_solve(diff(y,x)=-2*y, [y(0)=5])` → `5*exp(-2*x)` *(typical)*

**Related commands:** [`integrate`](#integrate), [`fsolve`](#fsolve), [`diff`](#diff)

"""


def main() -> None:
    (
        rows,
        main_allow,
        alevel_allow,
        _appendix_rows,
        removed,
        f4_map,
        inactive,
        menu,
        _lex,
        device_lex,
        _rew,
        aliases,
    ) = build_allowlist()
    main_names = {r["name"] for r in main_allow}
    alevel_names = {r["name"] for r in alevel_allow}
    f4_count = len(f4_map)

    old = ""
    if SRC_MD.exists():
        old = SRC_MD.read_text(errors="ignore")
    sections = extract_sections(old)

    today = date.today().isoformat()
    essentials_table = "\n".join(
        f"| `{r['name']}` | {r['surface'][:55]} | {r['tags']} |"
        for r in alevel_allow
    )
    full_table = "\n".join(
        f"| `{r['name']}` | {r['surface'][:48]} | {r['tags']} |"
        for r in main_allow
    )

    # A-level topic order (de_solve merged into desolve section)
    topics: list[tuple[str, list[str]]] = [
        (
            "Calculus",
            [
                "diff",
                "integrate",
                "int",
                "defint",
                "limit",
                "sum",
                "desolve",
                "param_diff",
            ],
        ),
        ("Solving", ["solve", "fsolve", "fitconst", "match", "suvat"]),
        (
            "Algebra & polynomials",
            [
                "expand",
                "factor",
                "simplify",
                "partfrac",
                "coeff",
                "complete_square",
                "binom_expand",
                "binom_coeff",
                "compare",
                "domain",
                "range",
                "rewrite",
                "xform",
                "subst",
            ],
        ),
        (
            "Trigonometry",
            ["trig_prove", "trig_rewrite", "trigcos", "trigsin", "trigtan", "tcollect", "texpand", "tlin"],
        ),
        ("Probability", ["binomial", "binomcdf", "normalcdf", "normal_cdf"]),
    ]

    body_parts: list[str] = []
    moved: list[str] = []
    seen: set[str] = set()
    for _topic, names in topics:
        body_parts.append(f"\n### {_topic}\n\n")
        for n in names:
            if n == "de_solve" or n in seen:
                continue
            if n not in main_names and n not in alevel_names:
                if n in sections or n in f4_map or n in menu or n in device_lex:
                    moved.append(n)
                continue
            if n.isupper() and n.lower() in main_names:
                continue
            seen.add(n)
            tags = next(
                (r["tags"] for r in main_allow if r["name"] == n),
                next((r["tags"] for r in alevel_allow if r["name"] == n), "[lexer]"),
            )
            if n == "desolve":
                body_parts.append(desolve_section(tags, sections))
                continue
            sec = good_section(n, sections.get(n))
            if sec:
                sec = strip_tags_blocks(clean_section(sec))
                sec = sec.replace(
                    "**In plain English**",
                    f"**Tags:** {tags}\n\n**In plain English**",
                    1,
                )
                body_parts.append(sec)
            else:
                stubs = {
                    "binomial": (
                        "Binomial distribution PMF / coefficient C(n,k) p^k (1-p)^(n-k).",
                        "binomial(10,0.3,4)",
                    ),
                    "binomcdf": (
                        "Cumulative binomial probability P(X≤k). Menu name `binomcdf`.",
                        "binomcdf(10,0.3,4)",
                    ),
                    "normalcdf": (
                        "Normal distribution cumulative probability; rewritten to `normal_cdf`.",
                        "normalcdf(-1.96,0,1)",
                    ),
                    "normal_cdf": (
                        "Native lexer name for the normal CDF; menu uses `normalcdf`.",
                        "normal_cdf(-1.96,0,1)",
                    ),
                    "defint": (
                        "Definite integral; same as `integrate(f,x,a,b)`.",
                        "defint(x^2,x,0,1)",
                    ),
                    "binom": (
                        "Soft-key alias for binomial — use `binomial` or `binomcdf` as needed.",
                        "binom(10,0.3,4)",
                    ),
                }
                if n in stubs:
                    b, ex = stubs[n]
                    body_parts.append(default_entry(n, tags, b, ex))
                else:
                    body_parts.append(default_entry(n, tags, f"Command `{n}` on the fx-CG50 allowlist.", f"{n}(...)"))

    appendix_reasons = {
        "implicit_diff": (
            "Listed in `catalogen.cpp` / F8 menu string in source, but **not** in the device "
            "`static_lexer.h` (only in `static_lexer_full.h`). Many shipped `.g3a` builds do not "
            "expose or execute it; users report missing menu entry and parse errors.",
            "normal(-diff((x^2+y^2-25),x)/diff((x^2+y^2-25),y))",
        ),
        "tangent_line": (
            "In F8 menu text but **#if 0** in active `catalogen.cpp`; rewrites to `linetan` which is also catalog-disabled.",
            "Use `diff` for gradient; equation of tangent by hand.",
        ),
        "taylor": "In `REMOVED_LEXER_NAMES` / runtime guard — returns `Err: unsupported function.`",
        "plotfunc": "Runtime-removed plot command — use Casio **Graph** app.",
        "mean": "Runtime-removed — average manually, e.g. `(a+b+c)/3`.",
        "linsolve": "Catalog `#if 0` and runtime-removed.",
        "plotparam": "Plot family removed at runtime.",
        "plotode": "Plot family removed at runtime.",
        "newton": "Not in active F4 catalog; numerical root-finding use `fsolve`.",
        "rsolve": "Catalog `#if 0` — recurrence solving not on this add-in.",
    }

    appendix_parts = [
        "## Registered in giac but NOT on fx-CG50 KhiCAS\n",
        "These names appear in full Giac/Xcas docs or `static_lexer_full.h` but are **hidden, "
        "removed, or not in the device lexer** on the Casio port. Do not use in exam practice "
        "unless you have verified your own `.g3a`.\n",
    ]
    for name, info in appendix_reasons.items():
        if isinstance(info, tuple):
            appendix_parts.append(appendix_entry(name, info[0], info[1]))
        else:
            appendix_parts.append(appendix_entry(name, str(info)))
        moved.append(name)

    doc = f"""# KhiCAS Function Catalog for A-level Students

A beginner-friendly guide to commands that **work on the Casio fx-CG50 KhiCAS / CasioCAS add-in**, focused on UK A-level and Further Mathematics.

| Metric | Count |
|--------|------:|
| **Device allowlist** (`static_lexer.h` ∪ F4 ∪ F-keys, minus runtime block) | {len(main_allow)} |
| **A-level essentials** (tutorials + table below) | {len(alevel_allow)} |
| `static_lexer.h` symbols (not in REMOVED_LEXER_NAMES) | {len(device_lex)} |
| F4 catalog surfaces (active, visible category) | {f4_count} |
| Runtime-removed / blocked names (policy) | {len(removed)} |
| Appendix-only (this revision) | {len(set(moved))} |

---

## Commands that work on your calculator

### A-level essentials (start here)

Tags: **[F4]** catalog; **[F4 hidden]** in `#if 0` catalog but still in device lexer (e.g. `desolve`); **[F7 ALG]** / **[F8 CALC]** soft keys; **[lexer]** = `static_lexer.h`; **[alias→…]** = `main.cc` rewrite before evaluation.

| Command | Catalog surface | How to reach |
|---------|-----------------|--------------|
{essentials_table}

<details>
<summary>Full device allowlist ({len(main_allow)} commands)</summary>

| Command | Surface | Tags |
|---------|---------|------|
{full_table}

</details>

---

## What is KhiCAS?

KhiCAS is a **Computer Algebra System (CAS)** on the fx-CG50 (KhiCAS / CasioCAS add-in), based on **Giac/Xcas**. It solves, differentiates, and integrates **symbolically**. For **graphs**, use the Casio **Graph** app — `plot*` commands are blocked in this add-in.

A **command** is a name plus arguments in brackets, e.g. `solve(x^2-4=0,x)`. Separate commands with **`;`** (**Alpha-F1**). Store with **→** (`=>`); define functions with **:=** (**F1**).

---

## Typing on the fx-CG50 (KhiCAS)

Sources: `dConsole.cpp`, `catalogen.cpp`, `textGUI.cpp`, `main.cc`.

### Everyday symbols

| What you need | How to enter |
|---------------|--------------|
| Multiply `*` | **×** key |
| Power `^` | **x^** or `^` |
| Assign `:=` | **F1** |
| Store `=>` | **→** key |
| `;` separator | **Alpha-F1** |
| `inf` | **INS** |
| `pi` | **π** key |

### Derivatives

| Goal | Type | Keys |
|------|------|------|
| Derivative | `diff(sin(x),x)` | **F4** → Calculus; **F6** → **F8 CALC** → `diff(` |
| 2nd derivative | `diff(x^4,x,2)` | same |
| Prime `y'` in ODE | **F3** inserts `'` | or `diff(y,x)` |
| Integral | `integrate(x^2,x)` or `int(x^2,x)` | **Shift-F3**; **F8 CALC** |
| Definite | `integrate(f,x,a,b)` or `defint(f,x,a,b)` | **F8 CALC** → `defint(` |
| Implicit dy/dx | `normal(-diff(F,x)/diff(F,y))` with `F` the left-hand side set to 0 | **Not** `implicit_diff` on typical builds — see appendix |

### Solve & ODE

| Task | Type | Keys |
|------|------|------|
| Solve | `solve(x^2-4=0,x)` | **F4**; **F7 ALG** |
| Numerical root | `fsolve(cos(x)=x,x=0..2)` | **F4** |
| ODE | `desolve(diff(y,x)=k*y,[y(0)=1])` or `de_solve(...)` | **F4**; **F8 CALC**; both names work (`de_solve` → `desolve`) |

### Removed on this build

`plotfunc`, `mean`, `linsolve`, `taylor`, most `plot*`, `det`, `inv`, hyperbolic inverses, etc. → `Err: unsupported function.` See appendix.

---

## Getting started — seven mini-tutorials

1. `solve(x^2-5*x+6=0,x)` → `[2,3]`
2. `diff(sin(x),x)` → `cos(x)`
3. `integrate(x^2,x,0,1)` → `1/3`
4. `factor(x^2-9,x)` → `(x-3)*(x+3)`
5. Graph **y=x²** in **Graph** app (not `plotfunc`)
6. Mean of `2,4,6,8`: `(2+4+6+8)/4` → `5` (no `mean`)
7. `trig_prove(sin(2*x),2*sin(x)*cos(x))` → `1`

---

## How to read this guide

- **Tags** on each entry match the allowlist table.
- **→** in examples means “returns”.
- `help(name)` needs **CASIOCAS.PAK** on calculator storage.

---

## Table of contents

1. [Commands that work](#commands-that-work-on-your-calculator)
2. [A-level commands](#a-level-priority-commands)
3. [Appendix: not on device](#registered-in-giac-but-not-on-fx-cg50-khicas)
4. [Verification](#verification-notes)

---

## A-level priority commands

Tutorial sections for **A-level essentials** only. The full device list is in the table above; appendix lists names that do **not** work on a typical `.g3a`.

{"".join(body_parts)}

{"".join(appendix_parts)}

---

## F4 calculator catalog (active, visible)

| Surface | Command |
|---------|---------|
"""
    for surf in sorted(f4_map.values(), key=lambda s: cmd_name(s)):
        n = cmd_name(surf)
        note = " *(appendix — not on typical device)*" if n in MAIN_BODY_EXCLUDE else ""
        doc += f"| `{surf}` | `{n}`{note} |\n"
    if "desolve" in inactive:
        doc += f"| `{inactive['desolve']}` | `desolve` *(F4 hidden; use `de_solve` or type `desolve`)* |\n"

    doc += f"""
---

## Sources scanned

- `c++/khicas/upstream/giac90_1addin/kglobal.cc` (`#include "static_lexer.h"` — device builtins)
- `c++/khicas/upstream/giac90_1addin/catalogen.cpp` (active + `#if 0` surfaces)
- `c++/khicas/upstream/giac90_1addin/main.cc` (`simple_aliases`, `cascas_contains_removed_function`)
- `c++/khicas/upstream/giac90_1addin/textGUI.cpp` (F7–F8 soft menus)
- `c++/tools/check_khicas_catalog.py` (`REMOVED_SURFACES`, `REMOVED_LEXER_NAMES`)

### Verification notes

**Allowlist:** {len(main_allow)} commands = `static_lexer.h` − `REMOVED_LEXER_NAMES` ∪ active F4 ∪ F-key menus ∪ rewrite hooks. **Not** `static_lexer_full.h`.

**desolve:** In device lexer and `kdesolve.cc`; F4 catalog shows `de_solve` (`#if 0` hides `desolve` surface); `main.cc` maps `de_solve(` → `desolve(`.

**Checked:** {today}. Re-run `python3 c++/tools/gen_khicas_a_level_doc.py` after catalog changes.

**Appendix:** `implicit_diff` (full lexer only), `taylor`/`plot*`/`mean`/`linsolve` (runtime removed), etc.

**Uncertainties:** Example outputs marked *(typical)* — confirm on your `.g3a`.
"""

    OUT.write_text(doc, encoding="utf-8")
    print(f"Wrote {OUT}")
    print(f"allowlist={len(main_allow)} alevel={len(alevel_allow)} appendix_notes={len(set(moved))}")
    print("desolve:", "desolve" in main_names, "de_solve:", "de_solve" in main_names)
    print("main sample:", ", ".join(sorted(main_names)[:24]), "...")


if __name__ == "__main__":
    main()

Z = ("c", "0")
O = ("c", "1")
SIG_CACHE = {}
SHOW_CACHE = {}
CACHE_LIMIT = 512


def cache_set(cache, key, value):
    if key not in cache and len(cache) >= CACHE_LIMIT:
        try:
            del cache[next(iter(cache))]
        except (KeyError, StopIteration):
            pass
    cache[key] = value
    return value


def mk(kind, items):
    flat = []
    for item in items:
        if item[0] == kind:
            for child in item[1]:
                flat.append(child)
        else:
            flat.append(item)
    if len(flat) == 0:
        if kind == "a":
            return O
        return Z
    if len(flat) == 1:
        return flat[0]
    return (kind, tuple(flat))


def kids(node, kind):
    if node[0] != kind:
        return [node]
    out = []
    for child in node[1]:
        if child[0] == kind:
            for item in kids(child, kind):
                out.append(item)
        else:
            out.append(child)
    return out


def same(a, b):
    return sig(a) == sig(b)


def sig(node):
    cached = SIG_CACHE.get(node)
    if cached is not None:
        return cached
    kind = node[0]
    if kind == "c":
        return cache_set(SIG_CACHE, node, node[1])
    if kind == "v":
        return cache_set(SIG_CACHE, node, node[1])
    if kind == "n":
        return cache_set(SIG_CACHE, node, "n(" + sig(node[1]) + ")")
    parts = []
    for child in kids(node, kind):
        parts.append(sig(child))
    parts.sort()
    if kind == "a":
        return cache_set(SIG_CACHE, node, "a(" + ",".join(parts) + ")")
    return cache_set(SIG_CACHE, node, "o(" + ",".join(parts) + ")")


def comp(a, b):
    return (a[0] == "n" and same(a[1], b)) or (b[0] == "n" and same(b[1], a))


def has(items, target):
    for item in items:
        if same(item, target):
            return True
    return False


def sub(a, b):
    for item in a:
        if not has(b, item):
            return False
    return True


def rem1(items, target):
    out = []
    done = False
    for item in items:
        if not done and same(item, target):
            done = True
        else:
            out.append(item)
    return out


def prio(node):
    if node[0] in ("c", "v"):
        return 4
    if node[0] == "n":
        return 3
    if node[0] == "a":
        return 2
    return 1


def simple_not(node):
    return node[0] in ("c", "v", "n")


def show(node, parent=0):
    key = (node, parent)
    cached = SHOW_CACHE.get(key)
    if cached is not None:
        return cached
    kind = node[0]
    if kind in ("c", "v"):
        text = node[1]
    elif kind == "n":
        child = node[1]
        text = show(child, 0)
        if simple_not(child):
            text = text + ","
        else:
            text = "(" + text + "),"
    else:
        joiner = "."
        if kind == "o":
            joiner = " + "
        parts = []
        for child in kids(node, kind):
            part = show(child, 0)
            if prio(child) < prio(node):
                part = "(" + part + ")"
            parts.append(part)
        text = joiner.join(parts)
    if prio(node) < parent:
        return cache_set(SHOW_CACHE, key, "(" + text + ")")
    return cache_set(SHOW_CACHE, key, text)


def short(node):
    return show(node).replace(" + ", "+")


def expand_vars(token):
    token = token.upper()
    if len(token) <= 1:
        return [token]
    result = []
    current = token[0]
    for ch in token[1:]:
        if "A" <= ch <= "Z":
            result.append(current)
            current = ch
        else:
            current += ch
    result.append(current)
    return result


def parse(text):
    text = text.upper()
    toks = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in " \t\r\n":
            i += 1
        elif ch in "(),+.":
            toks.append(ch)
            i += 1
        elif ch in "01":
            toks.append(ch)
            i += 1
        elif ("A" <= ch <= "Z") or ch == "_":
            j = i + 1
            while j < len(text):
                cj = text[j]
                if ("A" <= cj <= "Z") or ("0" <= cj <= "9") or cj == "_":
                    j += 1
                else:
                    break
            var_tokens = expand_vars(text[i:j])
            for vt in var_tokens[:-1]:
                toks.append(vt)
                toks.append(".")
            toks.append(var_tokens[-1])
            i = j
        else:
            raise ValueError("Unexpected character: " + ch)

    p = 0

    def cur():
        if p >= len(toks):
            return ""
        return toks[p]

    def eat(x):
        nonlocal p
        if cur() != x:
            raise ValueError("Expected '" + x + "' but found '" + cur() + "'.")
        p += 1

    def atom():
        nonlocal p
        t = cur()
        if t == "(":
            eat("(")
            node = expr()
            eat(")")
        elif t == "0":
            p += 1
            node = Z
        elif t == "1":
            p += 1
            node = O
        elif t and t[0] not in "(),+.":
            p += 1
            node = ("v", t)
        else:
            raise ValueError("Unexpected token: " + t)
        while cur() == ",":
            eat(",")
            node = ("n", node)
        return node

    def term():
        node = atom()
        parts = [node]
        while cur() == ".":
            eat(".")
            parts.append(atom())
        return mk("a", parts)

    def expr():
        node = term()
        parts = [node]
        while cur() == "+":
            eat("+")
            parts.append(term())
        return mk("o", parts)

    node = expr()
    if cur():
        raise ValueError("Unexpected token: " + cur())
    return node


def direct(node):
    kind = node[0]
    if kind in ("c", "v"):
        return False
    if kind == "n":
        child = node[1]
        return child[0] in ("c", "n", "a", "o")
    parts = kids(node, kind)
    if kind == "o":
        for item in parts:
            if item == O:
                return True
        if len(parts) > 1:
            for item in parts:
                if item == Z:
                    return True
        i = 0
        while i < len(parts):
            j = i + 1
            while j < len(parts):
                if same(parts[i], parts[j]) or comp(parts[i], parts[j]):
                    return True
                j += 1
            i += 1
        i = 0
        while i < len(parts):
            j = 0
            small = kids(parts[i], "a")
            while j < len(parts):
                if i != j and len(kids(parts[j], "a")) > len(small) and sub(small, kids(parts[j], "a")):
                    return True
                j += 1
            i += 1
        return False
    for item in parts:
        if item == Z:
            return True
    if len(parts) > 1:
        for item in parts:
            if item == O:
                return True
    i = 0
    while i < len(parts):
        j = i + 1
        while j < len(parts):
            if same(parts[i], parts[j]) or comp(parts[i], parts[j]):
                return True
            j += 1
        i += 1
    i = 0
    while i < len(parts):
        j = 0
        small = kids(parts[i], "o")
        while j < len(parts):
            if i != j and len(kids(parts[j], "o")) > len(small) and sub(small, kids(parts[j], "o")):
                return True
            j += 1
        i += 1
    return False


def step(node):
    kind = node[0]
    if kind in ("c", "v"):
        return None

    if kind == "n":
        hit = step(node[1])
        if hit:
            return (("n", hit[0]), hit[1])
        child = node[1]
        if child == Z or child == O:
            return (O if child == Z else Z, "Common sense")
        if child[0] == "n":
            return (child[1], "Double complement")
        if child[0] == "a":
            out = []
            for item in kids(child, "a"):
                out.append(("n", item))
            return (mk("o", out), "De Morgan's law")
        if child[0] == "o":
            out = []
            for item in kids(child, "o"):
                out.append(("n", item))
            return (mk("a", out), "De Morgan's law")
        return None

    parts = kids(node, kind)
    i = 0
    while i < len(parts):
        hit = step(parts[i])
        if hit:
            new = parts[:]
            new[i] = hit[0]
            return (mk(kind, new), hit[1])
        i += 1

    if kind == "o":
        for item in parts:
            if item == O:
                return (O, "Common sense")
        i = 0
        while i < len(parts):
            if parts[i] == Z and len(parts) > 1:
                return (mk("o", parts[:i] + parts[i + 1:]), "Common sense")
            i += 1
    else:
        for item in parts:
            if item == Z:
                return (Z, "Common sense")
        i = 0
        while i < len(parts):
            if parts[i] == O and len(parts) > 1:
                return (mk("a", parts[:i] + parts[i + 1:]), "Common sense")
            i += 1

    i = 0
    while i < len(parts):
        j = i + 1
        while j < len(parts):
            if same(parts[i], parts[j]):
                return (mk(kind, parts[:j] + parts[j + 1:]), "Tautology")
            if comp(parts[i], parts[j]):
                if len(parts) == 2:
                    return ((O if kind == "o" else Z), "Tautology")
                new = parts[:]
                new[i] = O if kind == "o" else Z
                del new[j]
                return (mk(kind, new), "Tautology")
            j += 1
        i += 1

    if kind == "o":
        i = 0
        while i < len(parts):
            small = kids(parts[i], "a")
            j = 0
            while j < len(parts):
                if i != j:
                    big = kids(parts[j], "a")
                    if len(big) > len(small) and sub(small, big):
                        return (mk("o", parts[:j] + parts[j + 1:]), "Absorption")
                j += 1
            i += 1
    else:
        i = 0
        while i < len(parts):
            small = kids(parts[i], "o")
            j = 0
            while j < len(parts):
                if i != j:
                    big = kids(parts[j], "o")
                    if len(big) > len(small) and sub(small, big):
                        return (mk("a", parts[:j] + parts[j + 1:]), "Absorption")
                j += 1
            i += 1

    if kind == "o":
        i = 0
        while i < len(parts):
            left = kids(parts[i], "a")
            j = i + 1
            while j < len(parts):
                right = kids(parts[j], "a")
                for li in left:
                    for rj in right:
                        if comp(li, rj):
                            rest_l = rem1(left[:], li)
                            rest_r = rem1(right[:], rj)
                            if rest_l and rest_r:
                                consensus = mk("a", rest_l + rest_r)
                                k = 0
                                while k < len(parts):
                                    if k != i and k != j and same(parts[k], consensus):
                                        new = parts[:k] + parts[k + 1:]
                                        return (mk("o", new), "Consensus theorem")
                                    k += 1
                j += 1
            i += 1

    i = 0
    while i < len(parts):
        left = kids(parts[i], "a" if kind == "o" else "o")
        j = i + 1
        while j < len(parts):
            right = kids(parts[j], "a" if kind == "o" else "o")
            common = []
            for item in left:
                if has(right, item) and not has(common, item):
                    common.append(item)
            if common:
                a = left[:]
                b = right[:]
                for item in common:
                    a = rem1(a, item)
                    b = rem1(b, item)
                if a and b:
                    if kind == "o":
                        rep = mk("a", [mk("a", common), mk("o", [mk("a", a), mk("a", b)])])
                    else:
                        rep = mk("o", [mk("o", common), mk("a", [mk("o", a), mk("o", b)])])
                    new = []
                    k = 0
                    while k < len(parts):
                        if k == i:
                            new.append(rep)
                        elif k != j:
                            new.append(parts[k])
                        k += 1
                    if len(short(mk(kind, new))) <= len(short(mk(kind, parts))):
                        return (mk(kind, new), "Distributive law")
            j += 1
        i += 1

    if kind == "a":
        i = 0
        while i < len(parts):
            if parts[i][0] == "o":
                rest = []
                j = 0
                while j < len(parts):
                    if j != i:
                        rest.append(parts[j])
                    j += 1
                out = []
                for item in kids(parts[i], "o"):
                    out.append(mk("a", [item] + rest))
                cand = mk("o", out)
                if len(short(cand)) <= len(short(mk(kind, parts))):
                    return (cand, "Expansion of brackets")
            i += 1
    else:
        i = 0
        while i < len(parts):
            if parts[i][0] == "a":
                rest = []
                j = 0
                while j < len(parts):
                    if j != i:
                        rest.append(parts[j])
                    j += 1
                out = []
                for item in kids(parts[i], "a"):
                    out.append(mk("o", rest + [item]))
                cand = mk("a", out)
                if len(short(cand)) <= len(short(mk(kind, parts))):
                    return (cand, "Expansion of brackets")
            i += 1

    return None


def to_nand(node):
    kind = node[0]
    if kind == "c":
        return node
    if kind == "v":
        return node
    if kind == "n":
        child = node[1]
        if child == Z or child == O:
            return node
        return ("n", to_nand(child))
    if kind == "a":
        parts = kids(node, "a")
        nanded = []
        for item in parts:
            nanded.append(("n", to_nand(item)))
        return mk("o", nanded)
    if kind == "o":
        parts = kids(node, "o")
        converted = []
        for item in parts:
            converted.append(to_nand(item))
        inner = mk("a", converted)
        return ("n", inner)
    return node


def to_nor(node):
    kind = node[0]
    if kind == "c":
        return node
    if kind == "v":
        return node
    if kind == "n":
        child = node[1]
        if child == Z or child == O:
            return node
        return ("n", to_nor(child))
    if kind == "o":
        parts = kids(node, "o")
        nored = []
        for item in parts:
            nored.append(("n", to_nor(item)))
        return mk("a", nored)
    if kind == "a":
        parts = kids(node, "a")
        converted = []
        for item in parts:
            converted.append(to_nor(item))
        inner = mk("o", converted)
        return ("n", inner)
    return node


def normalise(node):
    kind = node[0]
    if kind in ("c", "v"):
        return node
    if kind == "n":
        return ("n", normalise(node[1]))
    if kind == "a":
        return mk("a", [normalise(x) for x in kids(node, "a")])
    if kind == "o":
        return mk("o", [normalise(x) for x in kids(node, "o")])
    return node


def prove_both(node, max_steps=30):
    steps = []
    cur = node
    n = 1
    while n <= max_steps:
        hit = step(cur)
        if not hit:
            break
        cur = hit[0]
        steps.append((show(cur), hit[1]))
        n += 1
    return cur, steps


def prove(lhs_text, rhs_text):
    try:
        lhs = parse(lhs_text)
        rhs = parse(rhs_text)
    except ValueError as err:
        return [], "Parse error: " + str(err)
    
    lhs = normalise(lhs)
    rhs = normalise(rhs)
    
    if same(lhs, rhs):
        return ["LHS = RHS (already equal)"], None
    
    lhs_final, lhs_steps = prove_both(lhs)
    rhs_final, rhs_steps = prove_both(rhs)
    
    lines = []
    
    if lhs_steps:
        lines.append("Simplify LHS:")
        for i, (expr, reason) in enumerate(lhs_steps):
            lines.append(str(i + 1) + ". " + expr + "  (" + reason + ")")
        lines.append("= " + show(lhs_final))
    else:
        lines.append("LHS final: " + show(lhs_final))
    
    lines.append("")
    
    if rhs_steps:
        lines.append("Simplify RHS:")
        for i, (expr, reason) in enumerate(rhs_steps):
            lines.append(str(i + 1) + ". " + expr + "  (" + reason + ")")
        lines.append("= " + show(rhs_final))
    else:
        lines.append("RHS final: " + show(rhs_final))
    
    lines.append("")
    
    if same(lhs_final, rhs_final):
        lines.append("Both simplify to: " + show(lhs_final))
    else:
        lines.append("LHS final: " + show(lhs_final))
        lines.append("RHS final: " + show(rhs_final))
        if direct(mk("o", [lhs_final, rhs_final])):
            lines.append("(Both simplify to complement)")
    
    return lines, None


FAIL = "Could not prove identity. Check input."


MENU_LINE_WIDTH = 20
MENU_PAGE_LINES = 4


def wrap_menu_text(text, width=MENU_LINE_WIDTH):
    text = text.strip()
    if text == "":
        return [""]
    out = []
    remaining = text
    while remaining != "":
        if len(remaining) <= width:
            out.append(remaining)
            break
        split = remaining.rfind(" ", 0, width + 1)
        if split <= 0:
            out.append(remaining[:width])
            remaining = remaining[width:]
        else:
            out.append(remaining[:split])
            remaining = remaining[split + 1:]
        while remaining[:1] == " ":
            remaining = remaining[1:]
    return out


def build_menu_pages(options):
    pages = []
    page = []
    used = 0
    i = 0
    while i < len(options):
        key, label = options[i]
        lines = wrap_menu_text(key + " " + label)
        if used > 0 and used + len(lines) > MENU_PAGE_LINES:
            pages.append(page)
            page = []
            used = 0
        page.append(lines)
        used += len(lines)
        i += 1
    if len(page) == 0:
        pages.append([])
    else:
        pages.append(page)
    return pages


def paged_menu_input(prompt_label, options, default=None):
    pages = build_menu_pages(options)
    valid = {}
    i = 0
    while i < len(options):
        valid[options[i][0]] = True
        i += 1
    page_index = 0
    while True:
        page = pages[page_index]
        i = 0
        while i < len(page):
            lines = page[i]
            j = 0
            while j < len(lines):
                print(lines[j])
                j += 1
            i += 1
        prompt = prompt_label
        if len(pages) > 1:
            prompt += " " + str(page_index + 1) + "/" + str(len(pages)) + " n/p"
        prompt += ": "
        choice = input(prompt).strip()
        if choice == "":
            if default is not None:
                return default
        else:
            lowered = choice.lower()
            if lowered == "n" and len(pages) > 1:
                page_index += 1
                if page_index >= len(pages):
                    page_index = 0
                continue
            if lowered == "p" and len(pages) > 1:
                page_index -= 1
                if page_index < 0:
                    page_index = len(pages) - 1
                continue
            if choice in valid:
                return choice
        print("Mode must be 1-4.")


print("Syntax:")
print("  , for NOT (e.g. A, = NOT A)")
print("  . for AND (e.g. A.B = A AND B)")
print("  + for OR (e.g. A+B = A OR B)")
print("Variables: single letters (A, B) or multi-letter (AB = A AND B)")
print("")
mode = paged_menu_input("Mode", [
    ("1", "simplify"),
    ("2", "nand form"),
    ("3", "nor form"),
    ("4", "prove"),
], "1")

print("")
try:
    if mode == "1":
        s = input("Expression: ").strip()
        if s == "":
            s = "((B,.A),.B,),+A.B"
            print("Using: " + s)
        cur = parse(s)
        print("1. " + show(cur))
        n = 2
        while n <= 50:
            hit = step(cur)
            if not hit:
                break
            cur = hit[0]
            print(str(n) + ". " + show(cur) + "    (" + hit[1] + ")")
            n += 1
        print("Result: " + show(cur))
    elif mode == "2":
        s = input("Expression: ").strip()
        if s == "":
            s = "A.B"
            print("Using: " + s)
        cur = parse(s)
        print("1. " + show(cur))
        nanded = to_nand(cur)
        nanded = normalise(nanded)
        print("2. NAND form: " + show(nanded))
    elif mode == "3":
        s = input("Expression: ").strip()
        if s == "":
            s = "A+B"
            print("Using: " + s)
        cur = parse(s)
        print("1. " + show(cur))
        nored = to_nor(cur)
        nored = normalise(nored)
        print("2. NOR form: " + show(nored))
    elif mode == "4":
        lhs = input("LHS: ").strip()
        if lhs == "":
            lhs = "A.(B+C)"
        rhs = input("RHS: ").strip()
        if rhs == "":
            rhs = "A.B+A.C"
        print("1. LHS = " + lhs)
        print("2. RHS = " + rhs)
        lines, err = prove(lhs, rhs)
        if err:
            print("Error: " + err)
        else:
            print("")
            i = 3
            for line in lines:
                print(str(i) + ". " + line)
                i += 1
    else:
        print("Mode must be 1-4.")
except Exception as err:
    print("Input error: " + str(err))

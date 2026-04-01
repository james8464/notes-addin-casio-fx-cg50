Z = ("c", "0")
O = ("c", "1")


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
    kind = node[0]
    if kind == "c":
        return node[1]
    if kind == "v":
        return node[1]
    if kind == "n":
        return "n(" + sig(node[1]) + ")"
    parts = []
    for child in kids(node, kind):
        parts.append(sig(child))
    parts.sort()
    if kind == "a":
        return "a(" + ",".join(parts) + ")"
    return "o(" + ",".join(parts) + ")"


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
        return "(" + text + ")"
    return text


def short(node):
    return show(node).replace(" + ", "+")


def parse(text):
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
        elif ("A" <= ch <= "Z") or ("a" <= ch <= "z") or ch == "_":
            j = i + 1
            while j < len(text):
                cj = text[j]
                if ("A" <= cj <= "Z") or ("a" <= cj <= "z") or ("0" <= cj <= "9") or cj == "_":
                    j += 1
                else:
                    break
            toks.append(text[i:j])
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
                ok = direct(cand)
                if not ok:
                    for item in out:
                        if direct(item):
                            ok = True
                            break
                if ok:
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
                ok = direct(cand)
                if not ok:
                    for item in out:
                        if direct(item):
                            ok = True
                            break
                if ok:
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
        return to_nand(child)
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
        return to_nor(child)
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


print("  , for NOT")
print("  . for AND")
print("  + for OR")
print("Variables: single letters (A, B, ...) or multi-letter (AB, XYZ, ...)")
print("")
print("Mode:")
print("  1. Simplify")
print("  2. NAND form")
print("  3. NOR form")
mode = input("Choose: ").strip()
if mode == "":
    mode = "1"

print("")
s = input("Enter: ").strip()
if s == "":
    s = "((B,.A),.B,),+A.B"
    print("Using: " + s)

try:
    cur = parse(s)
except ValueError as err:
    print("Input error: " + str(err))
else:
    print("")
    print("Start: " + show(cur))
    n = 1
    while n <= 50:
        hit = step(cur)
        if not hit:
            break
        cur = hit[0]
        print(str(n) + ". " + show(cur) + "    (" + hit[1] + ")")
        n += 1
    print("Result: " + show(cur))

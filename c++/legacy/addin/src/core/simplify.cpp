#include "simplify.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace casio
{
namespace
{

static bool is_num(Node const &n) { return n.kind == NodeKind::Num; }

static bool is_zero(Node const &n) { return is_num(n) && n.num.num == 0; }
static bool is_one(Node const &n) { return is_num(n) && n.num.num == n.num.den; }
static bool is_int_num(Node const &n) { return is_num(n) && n.num.den == 1; }

static bool same_atom(Node const &a, Node const &b)
{
    if(a.kind != b.kind) return false;
    if(a.kind == NodeKind::Num) return a.num.num == b.num.num && a.num.den == b.num.den;
    if(a.kind == NodeKind::Sym) return a.text == b.text;
    if(a.kind == NodeKind::Const) return a.ckind == b.ckind;
    return false;
}

static Rational addq(Rational a, Rational b)
{
    Rational r;
    r.num = a.num * b.den + b.num * a.den;
    r.den = a.den * b.den;
    r.normalize();
    return r;
}
static Rational mulq(Rational a, Rational b)
{
    Rational r;
    r.num = a.num * b.num;
    r.den = a.den * b.den;
    r.normalize();
    return r;
}
static Rational divq(Rational a, Rational b)
{
    if(b.num == 0) throw std::runtime_error("division by zero");
    Rational r;
    r.num = a.num * b.den;
    r.den = a.den * b.num;
    r.normalize();
    return r;
}
static Rational negq(Rational a)
{
    a.num = -a.num;
    return a;
}

static NodeId make_add(Arena &a, std::vector<NodeId> parts);
static NodeId make_mul(Arena &a, std::vector<NodeId> parts);

static NodeId simplify_fn(Arena &a, FnKind fk, NodeId arg)
{
    NodeId sarg = simplify(a, arg);
    if(fk == FnKind::Exp) {
        // Match python: exp(x) -> pow(E, x)
        return simplify(a, a.pow(constant_e(a), sarg));
    }
    NodeId out = a.fn(fk, sarg);
    return out;
}

static NodeId simplify_pow(Arena &a, NodeId base, NodeId exp)
{
    NodeId b = simplify(a, base);
    NodeId e = simplify(a, exp);
    Node const &en = a.get(e);
    if(is_num(en)) {
        if(is_zero(en)) return num(a, 1);
        if(is_one(en)) return b;
    }
    Node const &bn = a.get(b);
    if(is_num(bn) && is_int_num(en)) {
        std::int64_t p = en.num.num;
        if(p >= 0) {
            // power for rationals when exponent int and small-ish; mimic python integer pow
            // (No overflow guard yet; we keep it simple first.)
            std::int64_t nnum = 1;
            std::int64_t nden = 1;
            for(std::int64_t i = 0; i < p; i++) {
                nnum *= bn.num.num;
                nden *= bn.num.den;
            }
            return num(a, nnum, nden);
        }
        else {
            p = -p;
            std::int64_t nnum = 1;
            std::int64_t nden = 1;
            for(std::int64_t i = 0; i < p; i++) {
                nnum *= bn.num.den;
                nden *= bn.num.num;
            }
            return num(a, nnum, nden);
        }
    }
    // Combine (a^b)^c -> a^(b*c) when b and c numeric (python checks is_num on both).
    if(bn.kind == NodeKind::Pow && is_num(a.get(bn.b)) && is_num(en)) {
        Rational bc = mulq(a.get(bn.b).num, en.num);
        return simplify(a, a.pow(bn.a, num(a, bc.num, bc.den)));
    }
    return a.pow(b, e);
}

static NodeId simplify_div(Arena &a, NodeId top, NodeId bot)
{
    NodeId t = simplify(a, top);
    NodeId b = simplify(a, bot);
    Node const &tn = a.get(t);
    Node const &bn = a.get(b);
    if(is_zero(tn)) return num(a, 0);
    if(is_one(bn)) return t;
    // Extract and divide leading integer from (k*...)/n where k,n are integers
    if(tn.kind == NodeKind::Mul && bn.kind == NodeKind::Num && bn.num.den == 1) {
        Node const &first = a.get(tn.kids[0]);
        if(first.kind == NodeKind::Num && first.num.den == 1) {
            Rational q = divq(first.num, bn.num);
            if(q.den == 1) {
                std::vector<NodeId> kept;
                for(size_t i = 1; i < tn.kids.size(); i++) kept.push_back(tn.kids[i]);
                if(q.num != 1) kept.insert(kept.begin(), num(a, q.num, 1));
                NodeId newt = kept.empty() ? num(a, 1) : (kept.size() == 1 ? kept[0] : a.mul(std::move(kept)));
                return simplify(a, newt);
            }
        }
    }
    if(is_num(tn) && is_num(bn)) {
        Rational q = divq(tn.num, bn.num);
        return num(a, q.num, q.den);
    }
    // Cancel simple common factors in (mul(...))/(mul(...)) or mul/atom.
    if(tn.kind == NodeKind::Mul) {
        // (a*b*...)/a -> b*...
        if(bn.kind == NodeKind::Sym || bn.kind == NodeKind::Num || bn.kind == NodeKind::Const) {
            std::vector<NodeId> kept;
            bool cancelled = false;
            for(auto k : tn.kids) {
                if(!cancelled && same_atom(a.get(k), bn)) {
                    cancelled = true;
                    continue;
                }
                kept.push_back(k);
            }
            if(cancelled) {
                NodeId newt = kept.empty() ? num(a, 1) : (kept.size() == 1 ? kept[0] : a.mul(std::move(kept)));
                return simplify(a, a.div(newt, b));
            }
        }
        if(bn.kind == NodeKind::Mul) {
            std::vector<NodeId> num_k = tn.kids;
            std::vector<NodeId> den_k = bn.kids;
            // Reduce leading numeric factors: (c1*...)/(c2*...) -> (c1/c2)*(...)/(...) .
            if(!num_k.empty() && !den_k.empty()) {
                Node const &n0 = a.get(num_k[0]);
                Node const &d0 = a.get(den_k[0]);
                if(n0.kind == NodeKind::Num && d0.kind == NodeKind::Num) {
                    Rational q = divq(n0.num, d0.num);
                    num_k.erase(num_k.begin());
                    den_k.erase(den_k.begin());
                    // Keep non-integer ratios as separate integer factors to prefer "(k*x)/m".
                    if(q.den == 1) {
                        if(!(q.num == 1)) num_k.insert(num_k.begin(), num(a, q.num, 1));
                    }
                    else {
                        num_k.insert(num_k.begin(), num(a, q.num, 1));
                        den_k.insert(den_k.begin(), num(a, q.den, 1));
                    }
                    if(num_k.empty()) num_k.push_back(num(a, 1));
                    if(den_k.empty()) den_k.push_back(num(a, 1));
                }
            }
            for(std::size_t i = 0; i < den_k.size(); i++) {
                for(std::size_t j = 0; j < num_k.size(); j++) {
                    if(same_atom(a.get(den_k[i]), a.get(num_k[j]))) {
                        den_k.erase(den_k.begin() + i);
                        num_k.erase(num_k.begin() + j);
                        i = (i == 0) ? 0 : i - 1;
                        break;
                    }
                    // Cancel sym against pow(sym, n) for integer n>0: x^n / x -> x^(n-1)
                    Node const &dk = a.get(den_k[i]);
                    Node const &nk = a.get(num_k[j]);
                    if(dk.kind == NodeKind::Sym && nk.kind == NodeKind::Pow) {
                        Node const &base = a.get(nk.a);
                        Node const &expn = a.get(nk.b);
                        if(base.kind == NodeKind::Sym && base.text == dk.text && is_int_num(expn) && expn.num.num > 0) {
                            // replace x^n with x^(n-1), remove x from denominator
                            Rational e = expn.num;
                            e.num -= e.den;
                            NodeId newpow = a.pow(nk.a, num(a, e.num, e.den));
                            num_k[j] = simplify(a, newpow);
                            den_k.erase(den_k.begin() + i);
                            i = (i == 0) ? 0 : i - 1;
                            break;
                        }
                    }
                }
            }
            if(num_k.size() != tn.kids.size() || den_k.size() != bn.kids.size()) {
                NodeId newt = num_k.empty() ? num(a, 1) : (num_k.size() == 1 ? num_k[0] : a.mul(std::move(num_k)));
                NodeId newb = den_k.empty() ? num(a, 1) : (den_k.size() == 1 ? den_k[0] : a.mul(std::move(den_k)));
                return simplify(a, a.div(newt, newb));
            }
        }
    }
    // Divide by a non-integer rational -> multiply by reciprocal (avoids ambiguous "…/1/2").
    // Keep division by integer constants as Div so formatting stays "x^3/3" instead of "1/3*x^3".
    if(is_num(bn) && bn.num.den != 1) {
        if(bn.num.num == 0) throw std::runtime_error("division by zero");
        Rational inv{bn.num.den, bn.num.num};
        inv.normalize();
        return simplify(a, a.mul({t, num(a, inv.num, inv.den)}));
    }
    // div by pow(x, n) -> mul(top, pow(x, -n)) when exponent numeric
    if(bn.kind == NodeKind::Pow && is_num(a.get(bn.b))) {
        Rational ne = negq(a.get(bn.b).num);
        NodeId negexp = num(a, ne.num, ne.den);
        return simplify(a, a.mul({t, a.pow(bn.a, negexp)}));
    }
    return a.div(t, b);
}

static NodeId make_add(Arena &a, std::vector<NodeId> parts)
{
    std::vector<NodeId> out;
    Rational c = Rational::from_int(0);

    for(NodeId id : parts) {
        NodeId s = simplify(a, id);
        Node const &n = a.get(s);
        if(n.kind == NodeKind::Add) {
            for(NodeId kid : n.kids) out.push_back(kid);
        }
        else if(is_num(n)) {
            c = addq(c, n.num);
        }
        else if(is_zero(n)) {
            // skip
        }
        else out.push_back(s);
    }
    if(c.num != 0) out.push_back(num(a, c.num, c.den));
    if(out.empty()) return num(a, 0);
    if(out.size() == 1) return out[0];
    return a.add(std::move(out));
}

static NodeId make_mul(Arena &a, std::vector<NodeId> parts)
{
    std::vector<NodeId> out;
    Rational c = Rational::from_int(1);

    for(NodeId id : parts) {
        NodeId s = simplify(a, id);
        Node const &n = a.get(s);
        if(n.kind == NodeKind::Mul) {
            for(NodeId kid : n.kids) out.push_back(kid);
        }
        else if(is_num(n)) {
            c = mulq(c, n.num);
        }
        else out.push_back(s);
    }

    // Cancel simple symbolic inverses: x*(1/x) and x*x^-1.
    // This is intentionally narrow but fixes common derivative forms (eg x*(1/x)).
    {
        std::unordered_map<std::string, int> sym_count;
        std::unordered_map<std::string, int> inv_count;
        sym_count.reserve(out.size());
        inv_count.reserve(out.size());

        auto is_one_node = [&](NodeId id) -> bool {
            Node const &n = a.get(id);
            return n.kind == NodeKind::Num && n.num.num == n.num.den;
        };
        auto is_minus_one_node = [&](NodeId id) -> bool {
            Node const &n = a.get(id);
            return n.kind == NodeKind::Num && n.num.den == 1 && n.num.num == -1;
        };

        for(NodeId id : out) {
            Node const &n = a.get(id);
            if(n.kind == NodeKind::Sym) sym_count[n.text]++;
            else if(n.kind == NodeKind::Div) {
                if(is_one_node(n.a)) {
                    Node const &b = a.get(n.b);
                    if(b.kind == NodeKind::Sym) inv_count[b.text]++;
                }
            }
            else if(n.kind == NodeKind::Pow) {
                Node const &b = a.get(n.a);
                Node const &e = a.get(n.b);
                if(b.kind == NodeKind::Sym && (is_minus_one_node(n.b) || (e.kind == NodeKind::Num && e.num.den == 1 && e.num.num == -1))) {
                    inv_count[b.text]++;
                }
            }
        }

        for(auto const &[name, sc] : sym_count) {
            auto it = inv_count.find(name);
            if(it == inv_count.end()) continue;
            int cancel = std::min(sc, it->second);
            if(cancel > 0) {
                sym_count[name] -= cancel;
                inv_count[name] -= cancel;
            }
        }

        if(!sym_count.empty() || !inv_count.empty()) {
            std::vector<NodeId> filtered;
            filtered.reserve(out.size());
            for(NodeId id : out) {
                Node const &n = a.get(id);
                if(n.kind == NodeKind::Sym) {
                    int &k = sym_count[n.text];
                    if(k > 0) {
                        filtered.push_back(id);
                        k--;
                    }
                    continue;
                }
                if(n.kind == NodeKind::Div) {
                    if(is_one_node(n.a)) {
                        Node const &b = a.get(n.b);
                        if(b.kind == NodeKind::Sym) {
                            int &k = inv_count[b.text];
                            if(k > 0) {
                                filtered.push_back(id);
                                k--;
                            }
                            continue;
                        }
                    }
                }
                if(n.kind == NodeKind::Pow) {
                    Node const &b = a.get(n.a);
                    Node const &e = a.get(n.b);
                    if(b.kind == NodeKind::Sym && e.kind == NodeKind::Num && e.num.den == 1 && e.num.num == -1) {
                        int &k = inv_count[b.text];
                        if(k > 0) {
                            filtered.push_back(id);
                            k--;
                        }
                        continue;
                    }
                }
                filtered.push_back(id);
            }
            out.swap(filtered);
        }
    }

    if(c.num == 0) return num(a, 0);
    if(!(c.num == c.den)) out.insert(out.begin(), num(a, c.num, c.den));
    if(out.empty()) return num(a, 1);
    if(out.size() == 1) return out[0];
    return a.mul(std::move(out));
}

} // namespace

NodeId num(Arena &a, std::int64_t n, std::int64_t d)
{
    if(d == 0) throw std::runtime_error("zero denominator");
    Rational r{n, d};
    r.normalize();
    return a.num(r);
}

NodeId sym(Arena &a, std::string_view name) { return a.sym(name); }

NodeId constant_pi(Arena &a) { return a.constant(ConstKind::Pi); }
NodeId constant_e(Arena &a) { return a.constant(ConstKind::E); }

NodeId add(Arena &a, std::vector<NodeId> parts) { return simplify(a, make_add(a, std::move(parts))); }
NodeId mul(Arena &a, std::vector<NodeId> parts) { return simplify(a, make_mul(a, std::move(parts))); }
NodeId div(Arena &a, NodeId top, NodeId bot) { return simplify(a, a.div(top, bot)); }
NodeId power(Arena &a, NodeId base, NodeId exp) { return simplify(a, a.pow(base, exp)); }

static FnKind fn_kind_from_name(std::string_view name)
{
    // Match FUNC_ALIASES and FUNC_NAMES in casio_core.py.
    // NOTE: exp handled in simplify_fn.
    if(name == "sin") return FnKind::Sin;
    if(name == "cos") return FnKind::Cos;
    if(name == "tan") return FnKind::Tan;
    if(name == "sec") return FnKind::Sec;
    if(name == "cosec") return FnKind::Cosec;
    if(name == "cot") return FnKind::Cot;
    if(name == "asin") return FnKind::Asin;
    if(name == "acos") return FnKind::Acos;
    if(name == "atan") return FnKind::Atan;
    if(name == "sinh") return FnKind::Sinh;
    if(name == "cosh") return FnKind::Cosh;
    if(name == "tanh") return FnKind::Tanh;
    if(name == "exp") return FnKind::Exp;
    if(name == "log") return FnKind::Log;
    if(name == "log10") return FnKind::Log10;
    if(name == "sqrt") return FnKind::Sqrt;
    if(name == "abs") return FnKind::Abs;
    throw std::runtime_error("Unknown function: " + std::string(name));
}

NodeId fn(Arena &a, std::string_view name, NodeId arg)
{
    // Alias rules: ln->log, csc->cosec
    if(name == "ln") name = "log";
    if(name == "csc") name = "cosec";
    FnKind fk = fn_kind_from_name(name);
    if(fk == FnKind::Exp) {
        return simplify(a, a.pow(constant_e(a), simplify(a, arg)));
    }
    return simplify(a, a.fn(fk, arg));
}

NodeId neg(Arena &a, NodeId node)
{
    Node const &n = a.get(node);
    if(is_num(n)) {
        Rational r = n.num;
        r.num = -r.num;
        return num(a, r.num, r.den);
    }
    return mul(a, {num(a, -1), node});
}

NodeId simplify(Arena &a, NodeId node)
{
    static thread_local int depth = 0;
    struct Guard
    {
        int &d;
        explicit Guard(int &dd) : d(dd)
        {
            d++;
            if(d > 512) throw std::runtime_error("Simplify recursion limit exceeded.");
        }
        ~Guard() { d--; }
    } guard(depth);

    if(a.has_simplify_cache(node)) return a.get_simplify_cache(node);
    Node const &n = a.get(node);
    NodeId out = node;
    switch(n.kind) {
    case NodeKind::Num:
    case NodeKind::Sym:
    case NodeKind::Const:
        out = node;
        break;
    case NodeKind::Fn:
        out = simplify_fn(a, n.fkind, n.a);
        break;
    case NodeKind::Pow:
        out = simplify_pow(a, n.a, n.b);
        break;
    case NodeKind::Div:
        out = simplify_div(a, n.a, n.b);
        break;
    case NodeKind::Add:
        out = make_add(a, n.kids);
        break;
    case NodeKind::Mul:
        out = make_mul(a, n.kids);
        break;
    }
    a.set_simplify_cache(node, out);
    return out;
}

} // namespace casio


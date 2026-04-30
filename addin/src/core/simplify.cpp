#include "simplify.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace casio
{
namespace
{

static bool is_num(Node const &n) { return n.kind == NodeKind::Num; }
static bool is_sym(Node const &n) { return n.kind == NodeKind::Sym; }
static bool is_const(Node const &n) { return n.kind == NodeKind::Const; }

static bool is_zero(Node const &n) { return is_num(n) && n.num.num == 0; }
static bool is_one(Node const &n) { return is_num(n) && n.num.num == n.num.den; }
static bool is_minus_one(Node const &n) { return is_num(n) && n.num.num == -n.num.den; }
static bool is_int_num(Node const &n) { return is_num(n) && n.num.den == 1; }

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

static bool same_const_e(Node const &n)
{
    return n.kind == NodeKind::Const && n.ckind == ConstKind::E;
}

static void flat_kind(Arena &a, NodeId node, NodeKind kind, std::vector<NodeId> &out)
{
    Node const &n = a.get(node);
    if(n.kind != kind) {
        out.push_back(node);
        return;
    }
    for(NodeId kid : n.kids) {
        Node const &k = a.get(kid);
        if(k.kind == kind) flat_kind(a, kid, kind, out);
        else out.push_back(kid);
    }
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
    if(is_num(tn) && is_num(bn)) {
        Rational q = divq(tn.num, bn.num);
        return num(a, q.num, q.den);
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
    Node const &n = a.get(node);
    switch(n.kind) {
    case NodeKind::Num:
    case NodeKind::Sym:
    case NodeKind::Const:
        return node;
    case NodeKind::Fn:
        return simplify_fn(a, n.fkind, n.a);
    case NodeKind::Pow:
        return simplify_pow(a, n.a, n.b);
    case NodeKind::Div:
        return simplify_div(a, n.a, n.b);
    case NodeKind::Add:
        return make_add(a, n.kids);
    case NodeKind::Mul:
        return make_mul(a, n.kids);
    }
    return node;
}

} // namespace casio


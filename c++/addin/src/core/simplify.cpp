#include "simplify.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
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

static std::int64_t small_factorial(std::int64_t n)
{
    std::int64_t out = 1;
    for(std::int64_t i = 2; i <= n; ++i) out *= i;
    return out;
}

static bool same_atom(Node const &a, Node const &b)
{
    if(a.kind != b.kind) return false;
    if(a.kind == NodeKind::Num) return a.num.num == b.num.num && a.num.den == b.num.den;
    if(a.kind == NodeKind::Sym) return a.text == b.text;
    if(a.kind == NodeKind::Const) return a.ckind == b.ckind;
    return false;
}

static bool same_tree(Arena &a, NodeId lhs, NodeId rhs)
{
    Node const &L = a.get(lhs);
    Node const &R = a.get(rhs);
    if(L.kind != R.kind) return false;
    switch(L.kind) {
    case NodeKind::Num:
    case NodeKind::Sym:
    case NodeKind::Const:
        return same_atom(L, R);
    case NodeKind::Fn:
        return L.fkind == R.fkind && same_tree(a, L.a, R.a);
    case NodeKind::Pow:
    case NodeKind::Div:
        return same_tree(a, L.a, R.a) && same_tree(a, L.b, R.b);
    case NodeKind::Add:
    case NodeKind::Mul:
        if(L.kids.size() != R.kids.size()) return false;
        for(std::size_t i = 0; i < L.kids.size(); ++i)
            if(!same_tree(a, L.kids[i], R.kids[i])) return false;
        return true;
    }
    return false;
}

static bool contains_symbol(Arena &a, NodeId id)
{
    Node const &n = a.get(id);
    if(n.kind == NodeKind::Sym) return true;
    if(n.kind == NodeKind::Fn) return contains_symbol(a, n.a);
    if(n.kind == NodeKind::Pow || n.kind == NodeKind::Div) return contains_symbol(a, n.a) || contains_symbol(a, n.b);
    if(n.kind == NodeKind::Add || n.kind == NodeKind::Mul) {
        for(NodeId k : n.kids)
            if(contains_symbol(a, k)) return true;
    }
    return false;
}

static bool rational_eq(Rational const &a, std::int64_t n, std::int64_t d = 1)
{
    Rational b{n, d};
    b.normalize();
    return a.num == b.num && a.den == b.den;
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

static std::optional<NodeId> as_half_angle(Arena &a, NodeId arg)
{
    Node const &x = a.get(arg);
    if(x.kind != NodeKind::Mul || x.kids.empty()) return std::nullopt;
    Node const &first = a.get(x.kids[0]);
    if(first.kind != NodeKind::Num || !rational_eq(first.num, 2)) return std::nullopt;
    std::vector<NodeId> rest;
    for(std::size_t i = 1; i < x.kids.size(); ++i) rest.push_back(x.kids[i]);
    if(rest.empty()) return std::nullopt;
    if(rest.size() == 1) return rest[0];
    return a.mul(std::move(rest));
}

static bool is_fn_power(Arena &a, NodeId id, FnKind fk, std::int64_t pow, NodeId *arg_out = nullptr)
{
    Node const &n = a.get(id);
    if(n.kind != NodeKind::Pow) return false;
    Node const &base = a.get(n.a);
    Node const &expn = a.get(n.b);
    if(base.kind != NodeKind::Fn || base.fkind != fk) return false;
    if(expn.kind != NodeKind::Num || expn.num.den != 1 || expn.num.num != pow) return false;
    if(arg_out) *arg_out = base.a;
    return true;
}

static bool is_fn_node(Arena &a, NodeId id, FnKind fk, NodeId *arg_out = nullptr)
{
    Node const &n = a.get(id);
    if(n.kind != NodeKind::Fn || n.fkind != fk) return false;
    if(arg_out) *arg_out = n.a;
    return true;
}

struct ScaledTerm
{
    Rational coeff{1, 1};
    NodeId body = 0;
    bool constant = false;
};

static ScaledTerm split_scaled(Arena &a, NodeId id)
{
    Node const &n = a.get(id);
    if(n.kind == NodeKind::Num) return ScaledTerm{n.num, 0, true};
    if(n.kind == NodeKind::Div) {
        Node const &den = a.get(n.b);
        if(den.kind == NodeKind::Num) {
            Node const &top = a.get(n.a);
            if(top.kind == NodeKind::Num) return ScaledTerm{divq(top.num, den.num), 0, true};
            ScaledTerm top_scaled = split_scaled(a, n.a);
            if(top_scaled.constant) return ScaledTerm{divq(top_scaled.coeff, den.num), 0, true};
            if(!(top_scaled.coeff.num == top_scaled.coeff.den))
                return ScaledTerm{divq(top_scaled.coeff, den.num), top_scaled.body, false};
            return ScaledTerm{divq(Rational{1, 1}, den.num), n.a, false};
        }
    }
    if(n.kind == NodeKind::Mul && !n.kids.empty()) {
        Rational coeff{1, 1};
        std::vector<NodeId> rest;
        bool saw_num = false;
        for(NodeId kid : n.kids) {
            Node const &k = a.get(kid);
            if(k.kind == NodeKind::Num) {
                coeff = mulq(coeff, k.num);
                saw_num = true;
            }
            else rest.push_back(kid);
        }
        if(saw_num) {
            if(rest.empty()) return ScaledTerm{coeff, 0, true};
            NodeId body = rest.size() == 1 ? rest[0] : a.mul(std::move(rest));
            return ScaledTerm{coeff, body, false};
        }
    }
    return ScaledTerm{Rational{1, 1}, id, false};
}

static std::optional<NodeId> try_trig_sum_identity(Arena &a, std::vector<NodeId> const &parts)
{
    if(parts.size() != 2) return std::nullopt;
    ScaledTerm t0 = split_scaled(a, parts[0]);
    ScaledTerm t1 = split_scaled(a, parts[1]);

    auto fn_pow = [&](ScaledTerm const &t, FnKind fk, NodeId &arg) {
        return !t.constant && rational_eq(t.coeff, 1) && is_fn_power(a, t.body, fk, 2, &arg);
    };
    auto neg_fn_pow = [&](ScaledTerm const &t, FnKind fk, NodeId &arg) {
        return !t.constant && rational_eq(t.coeff, -1) && is_fn_power(a, t.body, fk, 2, &arg);
    };
    auto same_arg = [&](NodeId lhs, NodeId rhs) { return same_tree(a, lhs, rhs); };

    NodeId a0 = 0, a1 = 0;
    if(fn_pow(t0, FnKind::Sin, a0) && fn_pow(t1, FnKind::Cos, a1) && same_arg(a0, a1)) return num(a, 1);
    if(fn_pow(t0, FnKind::Cos, a0) && fn_pow(t1, FnKind::Sin, a1) && same_arg(a0, a1)) return num(a, 1);
    if(fn_pow(t0, FnKind::Sec, a0) && neg_fn_pow(t1, FnKind::Tan, a1) && same_arg(a0, a1)) return num(a, 1);
    if(fn_pow(t1, FnKind::Sec, a0) && neg_fn_pow(t0, FnKind::Tan, a1) && same_arg(a0, a1)) return num(a, 1);
    if(fn_pow(t0, FnKind::Cosec, a0) && neg_fn_pow(t1, FnKind::Cot, a1) && same_arg(a0, a1)) return num(a, 1);
    if(fn_pow(t1, FnKind::Cosec, a0) && neg_fn_pow(t0, FnKind::Cot, a1) && same_arg(a0, a1)) return num(a, 1);

    auto one_plus_pow = [&](ScaledTerm const &one, ScaledTerm const &term, FnKind fk, FnKind out_fk) -> std::optional<NodeId> {
        NodeId arg = 0;
        if(!one.constant || !rational_eq(one.coeff, 1) || !fn_pow(term, fk, arg)) return std::nullopt;
        return a.pow(a.fn(out_fk, arg), num(a, 2));
    };
    if(auto r = one_plus_pow(t0, t1, FnKind::Tan, FnKind::Sec)) return r;
    if(auto r = one_plus_pow(t1, t0, FnKind::Tan, FnKind::Sec)) return r;
    if(auto r = one_plus_pow(t0, t1, FnKind::Cot, FnKind::Cosec)) return r;
    if(auto r = one_plus_pow(t1, t0, FnKind::Cot, FnKind::Cosec)) return r;

    auto one_plus_minus_cos = [&](ScaledTerm const &one, ScaledTerm const &term) -> std::optional<NodeId> {
        if(!one.constant || !rational_eq(one.coeff, 1) || term.constant) return std::nullopt;
        Node const &body = a.get(term.body);
        if(body.kind != NodeKind::Fn || body.fkind != FnKind::Cos) return std::nullopt;
        auto half = as_half_angle(a, body.a);
        if(!half) return std::nullopt;
        if(rational_eq(term.coeff, -1)) {
            return a.mul({num(a, 2), a.pow(a.fn(FnKind::Sin, *half), num(a, 2))});
        }
        if(rational_eq(term.coeff, 1)) {
            return a.mul({num(a, 2), a.pow(a.fn(FnKind::Cos, *half), num(a, 2))});
        }
        return std::nullopt;
    };
    if(auto r = one_plus_minus_cos(t0, t1)) return r;
    if(auto r = one_plus_minus_cos(t1, t0)) return r;

    auto sin_cos_sum_square = [&](ScaledTerm const &t, NodeId &arg) {
        if(t.constant || !rational_eq(t.coeff, 1)) return false;
        Node const &p = a.get(t.body);
        if(p.kind != NodeKind::Pow) return false;
        Node const &e = a.get(p.b);
        if(e.kind != NodeKind::Num || !rational_eq(e.num, 2)) return false;
        Node const &base = a.get(p.a);
        if(base.kind != NodeKind::Add || base.kids.size() != 2) return false;
        NodeId sarg = 0, carg = 0;
        if(is_fn_node(a, base.kids[0], FnKind::Sin, &sarg) && is_fn_node(a, base.kids[1], FnKind::Cos, &carg) &&
           same_arg(sarg, carg)) {
            arg = sarg;
            return true;
        }
        if(is_fn_node(a, base.kids[1], FnKind::Sin, &sarg) && is_fn_node(a, base.kids[0], FnKind::Cos, &carg) &&
           same_arg(sarg, carg)) {
            arg = sarg;
            return true;
        }
        return false;
    };
    auto neg_two_sin_cos = [&](ScaledTerm const &t, NodeId &arg) {
        if(t.constant || !rational_eq(t.coeff, -2)) return false;
        Node const &m = a.get(t.body);
        if(m.kind != NodeKind::Mul || m.kids.size() != 2) return false;
        NodeId sarg = 0, carg = 0;
        if(is_fn_node(a, m.kids[0], FnKind::Sin, &sarg) && is_fn_node(a, m.kids[1], FnKind::Cos, &carg) && same_arg(sarg, carg)) {
            arg = sarg;
            return true;
        }
        if(is_fn_node(a, m.kids[1], FnKind::Sin, &sarg) && is_fn_node(a, m.kids[0], FnKind::Cos, &carg) && same_arg(sarg, carg)) {
            arg = sarg;
            return true;
        }
        return false;
    };
    if(sin_cos_sum_square(t0, a0) && neg_two_sin_cos(t1, a1) && same_arg(a0, a1)) return num(a, 1);
    if(sin_cos_sum_square(t1, a0) && neg_two_sin_cos(t0, a1) && same_arg(a0, a1)) return num(a, 1);
    return std::nullopt;
}

static NodeId simplify_fn(Arena &a, FnKind fk, NodeId arg)
{
    NodeId sarg = simplify(a, arg);
    if(fk == FnKind::Exp) {
        // Match python: exp(x) -> pow(E, x)
        return simplify(a, a.pow(constant_e(a), sarg));
    }
    if(fk == FnKind::Factorial) {
        Node const &x = a.get(sarg);
        if(is_int_num(x) && x.num.num >= 0 && x.num.num <= 12) {
            return num(a, small_factorial(x.num.num));
        }
    }
    if(fk == FnKind::Sign) {
        Node const &x = a.get(sarg);
        if(is_num(x)) {
            if(x.num.num > 0) return num(a, 1);
            if(x.num.num < 0) return num(a, -1);
            return num(a, 0);
        }
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
    if(bn.kind == NodeKind::Pow && is_num(en)) {
        Node const &inner_base = a.get(bn.a);
        if(inner_base.kind == NodeKind::Const && inner_base.ckind == ConstKind::E)
            return simplify(a, a.pow(bn.a, a.mul({bn.b, e})));
    }
    if(bn.kind == NodeKind::Mul && is_int_num(en) && en.num.num > 1 && en.num.num <= 12) {
        bool has_num = false;
        bool monomial = true;
        for(NodeId k : bn.kids) {
            Node const &kn = a.get(k);
            if(kn.kind == NodeKind::Num) {
                has_num = true;
                continue;
            }
            if(kn.kind == NodeKind::Sym || kn.kind == NodeKind::Const) continue;
            if(kn.kind == NodeKind::Pow && is_num(a.get(kn.b))) continue;
            monomial = false;
            break;
        }
        if(has_num && monomial) {
            std::vector<NodeId> factors;
            factors.reserve(bn.kids.size());
            for(NodeId k : bn.kids) factors.push_back(a.pow(k, e));
            return simplify(a, a.mul(std::move(factors)));
        }
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
    if(is_int_num(bn) && bn.num.num == -1) return simplify(a, a.mul({num(a, -1), t}));
    if(is_int_num(bn) && bn.num.num < 0) {
        return simplify(a, a.div(a.mul({num(a, -1), t}), num(a, -bn.num.num, 1)));
    }
    if(bn.kind == NodeKind::Mul && !bn.kids.empty()) {
        Node const &d0 = a.get(bn.kids[0]);
        if(d0.kind == NodeKind::Num && d0.num.den == 1 && d0.num.num < 0) {
            std::vector<NodeId> den;
            den.reserve(bn.kids.size());
            den.push_back(num(a, -d0.num.num, 1));
            for(std::size_t i = 1; i < bn.kids.size(); ++i) den.push_back(bn.kids[i]);
            return simplify(a, a.div(a.mul({num(a, -1), t}), a.mul(std::move(den))));
        }
    }
    // Reduce a numeric numerator against a leading numeric factor in the denominator:
    // 40/(28*cos(a)) -> 10/(7*cos(a)).
    if(tn.kind == NodeKind::Num && bn.kind == NodeKind::Mul && !bn.kids.empty()) {
        Node const &d0 = a.get(bn.kids[0]);
        if(d0.kind == NodeKind::Num && d0.num.den == 1) {
            Rational q = divq(tn.num, d0.num);
            if(q.num != tn.num.num || q.den != d0.num.num) {
                std::vector<NodeId> den;
                den.reserve(bn.kids.size());
                if(q.den != 1) den.push_back(num(a, q.den, 1));
                for(std::size_t i = 1; i < bn.kids.size(); ++i) den.push_back(bn.kids[i]);
                NodeId newb = den.empty() ? num(a, 1) : (den.size() == 1 ? den[0] : a.mul(std::move(den)));
                return simplify(a, a.div(num(a, q.num, 1), newb));
            }
        }
    }
    // Extract and divide an integer factor from (k*...)/n where k,n are integers
    if(tn.kind == NodeKind::Mul && bn.kind == NodeKind::Num && bn.num.den == 1) {
        for(size_t factor_i = 0; factor_i < tn.kids.size(); ++factor_i) {
            Node const &factor = a.get(tn.kids[factor_i]);
            if(factor.kind != NodeKind::Num || factor.num.den != 1) continue;
            Rational q = divq(factor.num, bn.num);
            if(q.den == 1) {
                std::vector<NodeId> kept;
                for(size_t i = 0; i < tn.kids.size(); i++)
                    if(i != factor_i) kept.push_back(tn.kids[i]);
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
    if(tn.kind == NodeKind::Add && bn.kind == NodeKind::Num && bn.num.den == 1) {
        std::vector<NodeId> terms;
        bool ok = true;
        for(NodeId kid : tn.kids) {
            ScaledTerm t = split_scaled(a, kid);
            Rational q = divq(t.coeff, bn.num);
            if(q.den != 1) {
                ok = false;
                break;
            }
            if(t.constant) terms.push_back(num(a, q.num, 1));
            else if(q.num == 1) terms.push_back(t.body);
            else terms.push_back(a.mul({num(a, q.num, 1), t.body}));
        }
        if(ok) return simplify(a, a.add(std::move(terms)));
    }
    // Cancel simple common factors in (mul(...))/(mul(...)) or mul/atom.
    if(tn.kind == NodeKind::Mul) {
        // Generic cancellation for structured factors: sin(x)^2/sin(x), f/f, etc.
        if(!(bn.kind == NodeKind::Mul)) {
            std::vector<NodeId> kept;
            bool cancelled = false;
            for(auto k : tn.kids) {
                if(!cancelled && same_tree(a, k, b)) {
                    cancelled = true;
                    continue;
                }
                if(!cancelled) {
                    Node const &nk = a.get(k);
                    if(nk.kind == NodeKind::Pow && same_tree(a, nk.a, b)) {
                        Node const &expn = a.get(nk.b);
                        if(is_int_num(expn) && expn.num.num > 0) {
                            Rational e = expn.num;
                            e.num -= e.den;
                            NodeId replacement = e.num == 0 ? num(a, 1) : a.pow(nk.a, num(a, e.num, e.den));
                            kept.push_back(simplify(a, replacement));
                            cancelled = true;
                            continue;
                        }
                    }
                }
                kept.push_back(k);
            }
            if(cancelled) {
                NodeId newt = kept.empty() ? num(a, 1) : (kept.size() == 1 ? kept[0] : a.mul(std::move(kept)));
                return simplify(a, newt);
            }
        }
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
    if(tn.kind == NodeKind::Pow && same_tree(a, tn.a, b)) {
        Node const &expn = a.get(tn.b);
        if(is_int_num(expn) && expn.num.num > 0) {
            Rational e = expn.num;
            e.num -= e.den;
            if(e.num == 0) return num(a, 1);
            return simplify(a, a.pow(tn.a, num(a, e.num, e.den)));
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
    {
        bool changed = true;
        while(changed) {
            changed = false;
            for(std::size_t i = 0; i < out.size() && !changed; ++i) {
                ScaledTerm ti = split_scaled(a, out[i]);
                if(ti.constant || !rational_eq(ti.coeff, 1)) continue;
                for(std::size_t j = 0; j < out.size(); ++j) {
                    if(i == j) continue;
                    ScaledTerm tj = split_scaled(a, out[j]);
                    if(tj.constant || !rational_eq(tj.coeff, -1)) continue;
                    Node const &body = a.get(tj.body);
                    if(body.kind != NodeKind::Add) continue;
                    for(std::size_t k = 0; k < body.kids.size(); ++k) {
                        ScaledTerm tk = split_scaled(a, body.kids[k]);
                        if(tk.constant || !rational_eq(tk.coeff, 1) || !same_tree(a, ti.body, tk.body)) continue;
                        std::vector<NodeId> repl;
                        for(std::size_t m = 0; m < out.size(); ++m)
                            if(m != i && m != j) repl.push_back(out[m]);
                        for(std::size_t m = 0; m < body.kids.size(); ++m)
                            if(m != k) repl.push_back(make_mul(a, {num(a, -1), body.kids[m]}));
                        out.swap(repl);
                        changed = true;
                        break;
                    }
                    if(changed) break;
                }
            }
        }
    }
    {
        std::vector<NodeId> nonconst;
        nonconst.reserve(out.size());
        for(NodeId id : out) {
            Node const &n = a.get(id);
            if(is_num(n)) c = addq(c, n.num);
            else nonconst.push_back(id);
        }
        out.swap(nonconst);
    }
    if(c.num != 0) out.push_back(num(a, c.num, c.den));
    if(out.empty()) return num(a, 0);
    if(auto rewritten = try_trig_sum_identity(a, out)) return simplify(a, *rewritten);
    {
        bool changed = true;
        while(changed) {
            changed = false;
            for(std::size_t i = 0; i < out.size() && !changed; ++i) {
                ScaledTerm ti = split_scaled(a, out[i]);
                if(ti.constant) continue;
                for(std::size_t j = i + 1; j < out.size(); ++j) {
                    ScaledTerm tj = split_scaled(a, out[j]);
                    if(tj.constant || !same_tree(a, ti.body, tj.body)) continue;
                    Rational sum = addq(ti.coeff, tj.coeff);
                    out.erase(out.begin() + j);
                    if(sum.num == 0) out.erase(out.begin() + i);
                    else if(sum.num == sum.den) out[i] = ti.body;
                    else out[i] = a.mul({num(a, sum.num, sum.den), ti.body});
                    changed = true;
                    break;
                }
            }
        }
    }
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
        else if(n.kind == NodeKind::Div && is_num(a.get(n.a))) {
            c = mulq(c, a.get(n.a).num);
            Node const &den = a.get(n.b);
            if(den.kind == NodeKind::Div) {
                out.push_back(den.b);
                out.push_back(a.div(num(a, 1), den.a));
            }
            else {
                out.push_back(a.div(num(a, 1), n.b));
            }
        }
        else out.push_back(s);
    }

    for(std::size_t i = 0; i < out.size(); ++i) {
        Node const &n = a.get(out[i]);
        if(n.kind != NodeKind::Div) continue;
        for(std::size_t j = 0; j < out.size(); ++j) {
            if(i == j || !same_tree(a, n.b, out[j])) continue;
            NodeId top = n.a;
            if(i > j) {
                out.erase(out.begin() + i);
                out.erase(out.begin() + j);
            }
            else {
                out.erase(out.begin() + j);
                out.erase(out.begin() + i);
            }
            out.push_back(top);
            i = 0;
            break;
        }
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

    // Combine/cancel identical symbolic powers, e.g. sec(x)^2*sec(x)^-2 -> 1.
    {
        bool changed = true;
        while(changed) {
            changed = false;
            for(std::size_t i = 0; i < out.size() && !changed; ++i) {
                Node const &ni = a.get(out[i]);
                for(std::size_t j = i + 1; j < out.size(); ++j) {
                    Node const &nj = a.get(out[j]);
                    NodeId base_i = out[i];
                    NodeId base_j = out[j];
                    Rational exp_i{1, 1};
                    Rational exp_j{1, 1};
                    auto one_over = [&](Node const &n, NodeId &base, Rational &exp) {
                        if(n.kind != NodeKind::Div || !is_one(a.get(n.a))) return false;
                        base = n.b;
                        exp = Rational{-1, 1};
                        return true;
                    };
                    if(ni.kind == NodeKind::Pow) {
                        Node const &ei = a.get(ni.b);
                        if(ei.kind != NodeKind::Num) continue;
                        base_i = ni.a;
                        exp_i = ei.num;
                    }
                    else one_over(ni, base_i, exp_i);
                    if(nj.kind == NodeKind::Pow) {
                        Node const &ej = a.get(nj.b);
                        if(ej.kind != NodeKind::Num) continue;
                        base_j = nj.a;
                        exp_j = ej.num;
                    }
                    else one_over(nj, base_j, exp_j);
                    if(!same_tree(a, base_i, base_j)) continue;
                    Rational sum = addq(exp_i, exp_j);
                    out.erase(out.begin() + j);
                    if(sum.num == 0) {
                        out.erase(out.begin() + i);
                    }
                    else if(sum.num == sum.den) {
                        out[i] = base_i;
                    }
                    else {
                        out[i] = a.pow(base_i, num(a, sum.num, sum.den));
                    }
                    changed = true;
                    break;
                }
            }
        }
    }

    {
        int add_i = -1;
        bool const_only = true;
        for(std::size_t i = 0; i < out.size(); ++i) {
            Node const &n = a.get(out[i]);
            if(contains_symbol(a, out[i])) {
                const_only = false;
                break;
            }
            if(n.kind == NodeKind::Add) {
                if(add_i >= 0) {
                    const_only = false;
                    break;
                }
                add_i = static_cast<int>(i);
            }
        }
        if(const_only && add_i >= 0) {
            Node const &addn = a.get(out[add_i]);
            std::vector<NodeId> terms;
            for(NodeId term : addn.kids) {
                std::vector<NodeId> factors;
                if(!(c.num == c.den)) factors.push_back(num(a, c.num, c.den));
                for(std::size_t i = 0; i < out.size(); ++i)
                    if(static_cast<int>(i) != add_i) factors.push_back(out[i]);
                factors.push_back(term);
                terms.push_back(factors.size() == 1 ? factors[0] : a.mul(std::move(factors)));
            }
            return simplify(a, a.add(std::move(terms)));
        }
    }

    if(c.num == -1 && c.den == 1 && out.size() == 1) {
        Node const &only = a.get(out[0]);
        if(only.kind == NodeKind::Add) {
            std::vector<NodeId> terms;
            terms.reserve(only.kids.size());
            for(NodeId term : only.kids) terms.push_back(a.mul({num(a, -1), term}));
            return simplify(a, a.add(std::move(terms)));
        }
    }

    if(c.num == 0) return num(a, 0);
    if(out.size() == 1) {
        Node const &only = a.get(out[0]);
        if(only.kind == NodeKind::Div && is_one(a.get(only.a)) && !(c.num == c.den)) {
            NodeId top = num(a, c.num, 1);
            NodeId bot = c.den == 1 ? only.b : a.mul({num(a, c.den, 1), only.b});
            return simplify(a, a.div(top, bot));
        }
    }
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
    if(name == "exp") return FnKind::Exp;
    if(name == "log") return FnKind::Log;
    if(name == "log10") return FnKind::Log10;
    if(name == "sqrt") return FnKind::Sqrt;
    if(name == "abs") return FnKind::Abs;
    if(name == "sign") return FnKind::Sign;
    if(name == "factorial") return FnKind::Factorial;
    throw std::runtime_error("Unknown function: " + std::string(name));
}

NodeId fn(Arena &a, std::string_view name, NodeId arg)
{
    // Alias rules: ln->log, csc->cosec
    if(name == "ln") name = "log";
    if(name == "csc") name = "cosec";
    if(name == "arcsec" || name == "asec") return fn(a, "acos", div(a, num(a, 1), arg));
    if(name == "arccosec" || name == "arccsc" || name == "acsc") return fn(a, "asin", div(a, num(a, 1), arg));
    if(name == "arccot" || name == "acot") return fn(a, "atan", div(a, num(a, 1), arg));
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

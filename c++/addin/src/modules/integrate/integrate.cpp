#include "integrate.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/simplify.hpp"

#include <optional>

namespace casio::integrate
{

static bool is_sym(Arena &a, NodeId n, std::string const &name)
{
    auto const &x = a.get(n);
    return x.kind == NodeKind::Sym && x.text == name;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static bool is_const(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    return x.kind == NodeKind::Num || x.kind == NodeKind::Const;
}

struct Affine
{
    NodeId k = 0; // numeric node
    NodeId b = 0; // numeric node
};

static std::optional<NodeId> const_times_var(Arena &a, NodeId n, std::string const &var)
{
    // Match k*x or x*k where k is numeric constant.
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Mul || x.kids.size() != 2) return std::nullopt;
    auto n0 = as_num(a, x.kids[0]);
    auto n1 = as_num(a, x.kids[1]);
    if(n0 && is_sym(a, x.kids[1], var)) return x.kids[0];
    if(n1 && is_sym(a, x.kids[0], var)) return x.kids[1];
    return std::nullopt;
}

static std::optional<Affine> match_affine(Arena &a, NodeId n, std::string const &var)
{
    // Match k*x + b (or b + k*x). b must be numeric constant for now.
    if(is_sym(a, n, var)) {
        return Affine{casio::num(a, 1), casio::num(a, 0)};
    }
    if(auto k = const_times_var(a, n, var)) {
        return Affine{*k, casio::num(a, 0)};
    }
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return std::nullopt;

    NodeId a0 = x.kids[0];
    NodeId a1 = x.kids[1];

    auto b0 = as_num(a, a0);
    auto b1 = as_num(a, a1);
    if(b0) {
        if(auto k = const_times_var(a, a1, var)) return Affine{*k, a0};
        if(is_sym(a, a1, var)) return Affine{casio::num(a, 1), a0};
    }
    if(b1) {
        if(auto k = const_times_var(a, a0, var)) return Affine{*k, a1};
        if(is_sym(a, a0, var)) return Affine{casio::num(a, 1), a1};
    }
    return std::nullopt;
}

static std::optional<NodeId> integrate_simple(Arena &a, NodeId expr, std::string const &var);

static std::optional<NodeId> integrate_mul(Arena &a, NodeId expr, std::string const &var)
{
    auto const &x = a.get(expr);
    if(x.kind != NodeKind::Mul) return std::nullopt;

    // Pull out a constant factor.
    std::vector<NodeId> nonconst;
    std::vector<NodeId> consts;
    for(auto kid : x.kids) {
        if(is_const(a, kid)) consts.push_back(kid);
        else nonconst.push_back(kid);
    }
    if(consts.empty()) return std::nullopt;

    NodeId c = (consts.size() == 1) ? consts[0] : casio::mul(a, consts);
    NodeId rest = (nonconst.size() == 1) ? nonconst[0] : casio::mul(a, nonconst);

    auto prim = integrate_simple(a, rest, var);
    if(!prim) return std::nullopt;
    return casio::simplify(a, casio::mul(a, {c, *prim}));
}

static std::optional<NodeId> integrate_simple(Arena &a, NodeId expr, std::string const &var)
{
    auto const &x = a.get(expr);

    // Rational split: (a+b+...)/x -> a/x + b/x + ...
    if(x.kind == NodeKind::Div) {
        Node const &bot = a.get(x.b);
        Node const &top = a.get(x.a);
        if(bot.kind == NodeKind::Sym && bot.text == var && top.kind == NodeKind::Add) {
            std::vector<NodeId> parts;
            parts.reserve(top.kids.size());
            for(auto kid : top.kids) {
                // local cancel for common cases: (c*x^n)/x -> c*x^(n-1)
                NodeId num = kid;
                Node const &kn = a.get(kid);
                bool cancelled = false;
                if(kn.kind == NodeKind::Mul) {
                    std::vector<NodeId> newf;
                    newf.reserve(kn.kids.size());
                    for(auto f : kn.kids) {
                        Node const &fn = a.get(f);
                        if(!cancelled && fn.kind == NodeKind::Sym && fn.text == var) {
                            cancelled = true;
                            continue;
                        }
                        if(!cancelled && fn.kind == NodeKind::Pow && is_sym(a, fn.a, var)) {
                            if(auto er = as_num(a, fn.b); er && er->den == 1 && er->num >= 1) {
                                Rational nm1 = *er;
                                nm1.num -= nm1.den;
                                nm1.normalize();
                                cancelled = true;
                                if(nm1.num == 0) continue; // x^0 -> 1
                                newf.push_back(casio::power(a, fn.a, a.num(nm1)));
                                continue;
                            }
                        }
                        newf.push_back(f);
                    }
                    if(cancelled) num = (newf.size() == 1) ? newf[0] : casio::mul(a, newf);
                }
                else if(kn.kind == NodeKind::Pow && is_sym(a, kn.a, var)) {
                    if(auto er = as_num(a, kn.b); er && er->den == 1 && er->num >= 1) {
                        Rational nm1 = *er;
                        nm1.num -= nm1.den;
                        nm1.normalize();
                        cancelled = true;
                        if(nm1.num == 0) num = casio::num(a, 1);
                        else num = casio::power(a, kn.a, a.num(nm1));
                    }
                }
                NodeId term = cancelled ? casio::simplify(a, num) : casio::simplify(a, casio::div(a, num, x.b));
                auto prim = integrate_simple(a, term, var);
                if(!prim) return std::nullopt;
                parts.push_back(*prim);
            }
            return casio::simplify(a, casio::add(a, parts));
        }
    }
    // Also handle (a+b+...)*x^-1 form.
    if(x.kind == NodeKind::Mul) {
        NodeId inv = 0xFFFFFFFFu;
        std::vector<NodeId> rest;
        rest.reserve(x.kids.size());
        for(auto kid : x.kids) {
            Node const &k = a.get(kid);
            bool is_inv = false;
            if(k.kind == NodeKind::Pow && is_sym(a, k.a, var)) {
                if(auto er = as_num(a, k.b); er && er->den == 1 && er->num == -1) is_inv = true;
            }
            if(is_inv && inv == 0xFFFFFFFFu) inv = kid;
            else rest.push_back(kid);
        }
        if(inv != 0xFFFFFFFFu && rest.size() == 1) {
            Node const &r0 = a.get(rest[0]);
            if(r0.kind == NodeKind::Add) {
                std::vector<NodeId> parts;
                parts.reserve(r0.kids.size());
                for(auto term : r0.kids) {
                    NodeId t = casio::simplify(a, casio::mul(a, {term, inv}));
                    auto prim = integrate_simple(a, t, var);
                    if(!prim) return std::nullopt;
                    parts.push_back(*prim);
                }
                return casio::simplify(a, casio::add(a, parts));
            }
        }
    }

    // Tiny trig power rewrite: sin(x)^2 / cos(x)^2 (only direct x) to unlock table rules.
    if(x.kind == NodeKind::Pow) {
        auto nrat = as_num(a, x.b);
        if(nrat && nrat->num == 2 && nrat->den == 1) {
            Node const &base = a.get(x.a);
            if(base.kind == NodeKind::Fn && is_sym(a, base.a, var)) {
                NodeId one = casio::num(a, 1);
                NodeId two = casio::num(a, 2);
                NodeId half = casio::div(a, one, two);
                NodeId arg2 = casio::mul(a, {two, base.a});
                if(base.fkind == FnKind::Sin) {
                    // sin^2 x = (1 - cos(2x))/2
                    NodeId rhs = casio::mul(a, {half, casio::add(a, {one, casio::neg(a, a.fn(FnKind::Cos, arg2))})});
                    return integrate_simple(a, casio::simplify(a, rhs), var);
                }
                if(base.fkind == FnKind::Cos) {
                    // cos^2 x = (1 + cos(2x))/2
                    NodeId rhs = casio::mul(a, {half, casio::add(a, {one, a.fn(FnKind::Cos, arg2)})});
                    return integrate_simple(a, casio::simplify(a, rhs), var);
                }
            }
        }
    }

    // ∫ c dx = c*x
    if(is_const(a, expr) && !(x.kind == NodeKind::Const && x.ckind == ConstKind::Pi)) {
        NodeId v = casio::sym(a, var);
        return casio::simplify(a, casio::mul(a, {expr, v}));
    }

    // ∫ x dx = x^2/2
    if(is_sym(a, expr, var)) {
        NodeId v = casio::sym(a, var);
        return casio::simplify(a, casio::div(a, casio::power(a, v, casio::num(a, 2)), casio::num(a, 2)));
    }

    // Linearity: ∫(a+b+...) = ∫a + ∫b + ...
    if(x.kind == NodeKind::Add) {
        std::vector<NodeId> parts;
        parts.reserve(x.kids.size());
        for(auto kid : x.kids) {
            auto prim = integrate_simple(a, kid, var);
            if(!prim) return std::nullopt;
            parts.push_back(*prim);
        }
        return casio::simplify(a, casio::add(a, parts));
    }

    // Constant multiple
    if(x.kind == NodeKind::Mul) {
        if(auto out = integrate_mul(a, expr, var)) return out;
    }

    // Reciprocal affine: ∫ 1/(k*x+b) dx = ln|k*x+b| / k
    if(x.kind == NodeKind::Div) {
        if(auto top = as_num(a, x.a)) {
            if(auto aff = match_affine(a, x.b, var)) {
                // ∫ c/(k*x+b) dx = c*ln|k*x+b| / k
                NodeId ln = casio::fn(a, "log", casio::fn(a, "abs", x.b));
                NodeId frac = casio::div(a, ln, aff->k);
                if(!(top->num == 1 && top->den == 1)) {
                    frac = casio::mul(a, {a.num(*top), frac});
                }
                return casio::simplify(a, frac);
            }
        }
    }

    // Power rule: ∫ x^n dx
    if(x.kind == NodeKind::Pow && is_sym(a, x.a, var)) {
        auto n = as_num(a, x.b);
        if(!n) return std::nullopt;
        // n == -1 → ln|x|
        if(n->num == -1 && n->den == 1) {
            NodeId v = casio::sym(a, var);
            return casio::fn(a, "log", casio::fn(a, "abs", v));
        }
        // x^(n+1)/(n+1)
        Rational np1 = *n;
        np1.num += np1.den;
        if(np1.num == 0) return std::nullopt;
        NodeId v = casio::sym(a, var);
        NodeId pow = casio::power(a, v, a.num(np1));
        return casio::simplify(a, casio::div(a, pow, a.num(np1)));
    }

    // Affine power rule: ∫ (k*x+b)^n dx
    if(x.kind == NodeKind::Pow) {
        if(auto aff = match_affine(a, x.a, var)) {
            auto nrat = as_num(a, x.b);
            if(!nrat) return std::nullopt;
            // n == -1 → ln|k*x+b| / k
            if(nrat->num == -1 && nrat->den == 1) {
                NodeId ln = casio::fn(a, "log", casio::fn(a, "abs", x.a));
                return casio::simplify(a, casio::div(a, ln, aff->k));
            }
            Rational np1 = *nrat;
            np1.num += np1.den;
            if(np1.num == 0) return std::nullopt;
            NodeId pow = casio::power(a, x.a, a.num(np1));
            NodeId denom = casio::mul(a, {aff->k, a.num(np1)});
            return casio::simplify(a, casio::div(a, pow, denom));
        }
    }

    // Exponential: ∫ e^(kx) dx = e^(kx)/k
    if(x.kind == NodeKind::Pow) {
        auto const &base = a.get(x.a);
        if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) {
            if(auto aff = match_affine(a, x.b, var)) {
                return casio::simplify(a, casio::div(a, expr, aff->k));
            }
        }
    }

    // Basic trig/exp/log
    if(x.kind == NodeKind::Fn) {
        NodeId arg = x.a;
        if(x.fkind == FnKind::Sin) {
            if(auto aff = match_affine(a, arg, var)) {
                // ∫ sin(kx+b) dx = -cos(kx+b)/k
                NodeId coeff = casio::div(a, casio::num(a, -1), aff->k);
                return casio::simplify(a, casio::mul(a, {coeff, a.fn(FnKind::Cos, arg)}));
            }
        }
        if(x.fkind == FnKind::Cos) {
            if(auto aff = match_affine(a, arg, var)) {
                // ∫ cos(kx+b) dx = sin(kx+b)/k
                return casio::simplify(a, casio::div(a, a.fn(FnKind::Sin, arg), aff->k));
            }
        }
        if(x.fkind == FnKind::Exp) {
            if(auto aff = match_affine(a, arg, var)) {
                // ∫ exp(kx+b) dx = exp(kx+b)/k
                return casio::simplify(a, casio::div(a, expr, aff->k));
            }
        }
        if(x.fkind == FnKind::Tan) {
            // ∫ tan(kx+b) dx = -ln|cos(kx+b)|/k
            if(auto aff = match_affine(a, arg, var)) {
                NodeId coeff = casio::div(a, casio::num(a, -1), aff->k);
                NodeId ln = casio::fn(a, "log", casio::fn(a, "abs", a.fn(FnKind::Cos, arg)));
                return casio::simplify(a, casio::mul(a, {coeff, ln}));
            }
            // ∫ tan(x) dx = -ln|cos(x)|
            if(is_sym(a, arg, var)) {
                NodeId c = a.fn(FnKind::Cos, arg);
                return casio::neg(a, casio::fn(a, "log", casio::fn(a, "abs", c)));
            }
        }
        if(x.fkind == FnKind::Cot) {
            // ∫ cot(kx+b) dx = ln|sin(kx+b)|/k
            if(auto aff = match_affine(a, arg, var)) {
                NodeId coeff = casio::div(a, casio::num(a, 1), aff->k);
                NodeId ln = casio::fn(a, "log", casio::fn(a, "abs", a.fn(FnKind::Sin, arg)));
                return casio::simplify(a, casio::mul(a, {coeff, ln}));
            }
            // ∫ cot(x) dx = ln|sin(x)|
            if(is_sym(a, arg, var)) {
                NodeId s = a.fn(FnKind::Sin, arg);
                return casio::fn(a, "log", casio::fn(a, "abs", s));
            }
        }
        if(x.fkind == FnKind::Sec) {
            // ∫ sec(x) dx = ln|sec(x) + tan(x)|
            if(is_sym(a, arg, var)) {
                NodeId sec_plustan = casio::add(a, {a.fn(FnKind::Sec, arg), a.fn(FnKind::Tan, arg)});
                return casio::fn(a, "log", casio::fn(a, "abs", sec_plustan));
            }
        }
        if(x.fkind == FnKind::Cosec) {
            // ∫ cosec(x) dx = ln|csc(x) - cot(x)|
            if(is_sym(a, arg, var)) {
                NodeId csc_minus_cot = casio::add(a, {a.fn(FnKind::Cosec, arg), casio::neg(a, a.fn(FnKind::Cot, arg))});
                return casio::fn(a, "log", casio::fn(a, "abs", csc_minus_cot));
            }
        }
        if(x.fkind == FnKind::Log && is_sym(a, arg, var)) {
            // ∫ ln(x) dx = x ln(x) - x
            NodeId v = casio::sym(a, var);
            NodeId xlnx = casio::mul(a, {v, expr});
            return casio::simplify(a, casio::add(a, {xlnx, casio::neg(a, v)}));
        }
    }
    
    // ∫ 1/(x^2 + a^2) dx = (1/a) * atan(x/a)
    if(x.kind == NodeKind::Pow && as_num(a, x.b) && as_num(a, x.b)->num == -1) {
        Node const &base = a.get(x.a);
        if(base.kind == NodeKind::Add && base.kids.size() == 2) {
            // Check for form: a^2 + x^2 or x^2 + a^2
            for(int swap = 0; swap < 2; swap++) {
                NodeId t0 = base.kids[swap];
                NodeId t1 = base.kids[1-swap];
                Node const &t0_node = a.get(t0);
                Node const &t1_node = a.get(t1);
                // t0 should be x^2, t1 should be a^2
                if(t0_node.kind == NodeKind::Pow && is_sym(a, t0_node.a, var) && 
                   as_num(a, t0_node.b) && as_num(a, t0_node.b)->num == 2) {
                    auto a2 = as_num(a, t1);
                    if(a2 && a2->num > 0) {
                        NodeId a_val = casio::num(a, a2->num);
                        NodeId a_sqrt = a.fn(FnKind::Sqrt, a_val);
                        NodeId coeff = casio::div(a, casio::num(a, 1), a_sqrt);
                        NodeId atan = a.fn(FnKind::Atan, casio::div(a, casio::sym(a, var), a_sqrt));
                        return casio::simplify(a, casio::mul(a, {coeff, atan}));
                    }
                }
            }
        }
    }
    
    // Special trig products/powers
    if(x.kind == NodeKind::Pow && a.get(x.a).kind == NodeKind::Fn) {
        auto const &f = a.get(x.a);
        if(auto nrat = as_num(a, x.b); nrat && nrat->num == 2 && nrat->den == 1) {
            NodeId arg = f.a;
            if(f.fkind == FnKind::Sec && is_sym(a, arg, var)) {
                return a.fn(FnKind::Tan, arg);
            }
            if(f.fkind == FnKind::Cosec && is_sym(a, arg, var)) {
                return casio::neg(a, a.fn(FnKind::Cot, arg));
            }
            if(f.fkind == FnKind::Tan && is_sym(a, arg, var)) {
                // ∫ tan^2 x dx = tan x - x
                NodeId v = casio::sym(a, var);
                return casio::simplify(a, casio::add(a, {a.fn(FnKind::Tan, arg), casio::neg(a, v)}));
            }
            if(f.fkind == FnKind::Cot && is_sym(a, arg, var)) {
                // ∫ cot^2 x dx = -cot x - x
                NodeId v = casio::sym(a, var);
                return casio::simplify(a, casio::add(a, {casio::neg(a, a.fn(FnKind::Cot, arg)), casio::neg(a, v)}));
            }
        }
        // sec^2(x), cosec^2(x)
        if(x.kind == NodeKind::Pow && a.get(x.a).kind == NodeKind::Fn) {
            auto const &f = a.get(x.a);
            auto nrat = as_num(a, x.b);
            if(nrat && nrat->num == 2 && nrat->den == 1) {
                NodeId arg = f.a;
                if(f.fkind == FnKind::Sec && is_sym(a, arg, var)) {
                    // ∫ sec^2(x) dx = tan(x)
                    return a.fn(FnKind::Tan, arg);
                }
                if(f.fkind == FnKind::Cosec && is_sym(a, arg, var)) {
                    // ∫ cosec^2(x) dx = -cot(x)
                    return casio::neg(a, a.fn(FnKind::Cot, arg));
                }
            }
        }
    }
    if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
        // ∫ sec(x)tan(x) dx = sec(x)
        // ∫ cosec(x)cot(x) dx = -cosec(x)
        auto const &a0 = a.get(x.kids[0]);
        auto const &a1 = a.get(x.kids[1]);
        auto is_fn = [&](Node const &n, FnKind k) { return n.kind == NodeKind::Fn && n.fkind == k; };
        if(a0.kind == NodeKind::Fn && a1.kind == NodeKind::Fn && is_sym(a, a0.a, var) && is_sym(a, a1.a, var)) {
            if((is_fn(a0, FnKind::Sec) && is_fn(a1, FnKind::Tan)) ||
               (is_fn(a1, FnKind::Sec) && is_fn(a0, FnKind::Tan))) {
                return a.fn(FnKind::Sec, casio::sym(a, var));
            }
            if((is_fn(a0, FnKind::Cosec) && is_fn(a1, FnKind::Cot)) ||
               (is_fn(a1, FnKind::Cosec) && is_fn(a0, FnKind::Cot))) {
                return casio::neg(a, a.fn(FnKind::Cosec, casio::sym(a, var)));
            }
        }
        
        // x * sin(x) - integration by parts
        Node const &n0 = a.get(x.kids[0]);
        Node const &n1 = a.get(x.kids[1]);
        if((n0.kind == NodeKind::Sym && n0.text == var && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Sin) ||
           (n1.kind == NodeKind::Sym && n1.text == var && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Sin)) {
            if(is_sym(a, n0.text == var ? n1.a : n0.a, var)) {
                NodeId sym_x = casio::sym(a, var);
                NodeId term1 = casio::mul(a, {sym_x, casio::neg(a, a.fn(FnKind::Cos, sym_x))});
                NodeId term2 = a.fn(FnKind::Sin, sym_x);
                return casio::simplify(a, casio::add(a, {term1, term2}));
            }
        }
        
        // x * cos(x)
        if((n0.kind == NodeKind::Sym && n0.text == var && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Cos) ||
           (n1.kind == NodeKind::Sym && n1.text == var && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Cos)) {
            if(is_sym(a, n0.text == var ? n1.a : n0.a, var)) {
                NodeId sym_x = casio::sym(a, var);
                NodeId term1 = casio::mul(a, {sym_x, a.fn(FnKind::Sin, sym_x)});
                NodeId term2 = a.fn(FnKind::Cos, sym_x);
                return casio::simplify(a, casio::add(a, {term1, term2}));
            }
        }
        
        // x^2 * sin(x)
        if((n0.kind == NodeKind::Pow && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Sin) ||
           (n1.kind == NodeKind::Pow && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Sin)) {
            Node const &pow = n0.kind == NodeKind::Pow ? n0 : n1;
            Node const &fn = n0.kind == NodeKind::Fn ? n0 : n1;
            if(is_sym(a, pow.a, var) && is_sym(a, fn.a, var)) {
                auto n = as_num(a, pow.b);
                if(n && n->num == 2 && n->den == 1) {
                    NodeId sym_x = casio::sym(a, var);
                    NodeId x2 = casio::power(a, sym_x, casio::num(a, 2));
                    NodeId term1 = casio::mul(a, {casio::num(a, -1), x2, a.fn(FnKind::Cos, sym_x)});
                    NodeId two = casio::num(a, 2);
                    NodeId term2 = casio::mul(a, {two, sym_x, a.fn(FnKind::Sin, sym_x)});
                    NodeId term3 = casio::mul(a, {two, a.fn(FnKind::Cos, sym_x)});
                    return casio::simplify(a, casio::add(a, {term1, term2, term3}));
                }
            }
        }
        
        // x^2 * cos(x)
        if((n0.kind == NodeKind::Pow && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Cos) ||
           (n1.kind == NodeKind::Pow && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Cos)) {
            Node const &pow = n0.kind == NodeKind::Pow ? n0 : n1;
            Node const &fn = n0.kind == NodeKind::Fn ? n0 : n1;
            if(is_sym(a, pow.a, var) && is_sym(a, fn.a, var)) {
                auto n = as_num(a, pow.b);
                if(n && n->num == 2 && n->den == 1) {
                    NodeId sym_x = casio::sym(a, var);
                    NodeId x2 = casio::power(a, sym_x, casio::num(a, 2));
                    NodeId term1 = casio::mul(a, {x2, a.fn(FnKind::Sin, sym_x)});
                    NodeId two = casio::num(a, 2);
                    NodeId term2 = casio::mul(a, {two, sym_x, a.fn(FnKind::Cos, sym_x)});
                    NodeId neg2 = casio::neg(a, two);
                    NodeId term3 = casio::mul(a, {neg2, a.fn(FnKind::Sin, sym_x)});
                    return casio::simplify(a, casio::add(a, {term1, term2, term3}));
                }
            }
        }
        
        // x * exp(x) - integration by parts
        if((n0.kind == NodeKind::Sym && n0.text == var && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Exp) ||
           (n1.kind == NodeKind::Sym && n1.text == var && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Exp)) {
            NodeId the_exp = (n0.kind == NodeKind::Fn) ? n0 : n1;
            if(is_sym(a, the_exp.a, var)) {
                NodeId sym_x = casio::sym(a, var);
                NodeId term1 = casio::mul(a, {sym_x, a.fn(FnKind::Exp, sym_x)});
                NodeId term2 = a.fn(FnKind::Exp, sym_x);
                return casio::simplify(a, casio::add(a, {term1, casio::neg(a, term2)}));
            }
        }
        
        // x * exp(a*x) pattern (a > 1)
        if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
            Node const &n0 = a.get(x.kids[0]);
            Node const &n1 = a.get(x.kids[1]);
            // x^n * sin(x)
            if((n0.kind == NodeKind::Pow && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Sin) ||
               (n1.kind == NodeKind::Pow && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Sin)) {
                Node const &pow = n0.kind == NodeKind::Pow ? n0 : n1;
                Node const &fn = n0.kind == NodeKind::Fn ? n0 : n1;
                if(is_sym(a, pow.a, var) && is_sym(a, fn.a, var)) {
                    auto n = as_num(a, pow.b);
                    if(n && n->den == 1 && n->num >= 1) {
                        // ∫ x^n sin(x) dx = -x^n cos(x) + n ∫ x^(n-1) cos(x) dx
                        NodeId sym_x = casio::sym(a, var);
                        NodeId neg_cos = casio::neg(a, a.fn(FnKind::Cos, sym_x));
                        NodeId xn_fact = casio::power(a, sym_x, a.num(*n));
                        NodeId term1 = casio::mul(a, {xn_fact, neg_cos});
                        // Recursively integrate x^(n-1) * cos(x)
                        NodeId xn1 = casio::power(a, sym_x, casio::num(a, n->num - 1));
                        NodeId xn1_cos = casio::mul(a, {xn1, a.fn(FnKind::Cos, sym_x)});
                        auto prim = integrate_simple(a, xn1_cos, var);
                        if(prim) {
                            NodeId n_node = a.num(*n);
                            NodeId term2 = casio::mul(a, {n_node, *prim});
                            return casio::simplify(a, casio::add(a, {term1, term2}));
                        }
                    }
                }
            }
            // x^n * cos(x)
            if((n0.kind == NodeKind::Pow && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Cos) ||
               (n1.kind == NodeKind::Pow && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Cos)) {
                Node const &pow = n0.kind == NodeKind::Pow ? n0 : n1;
                Node const &fn = n0.kind == NodeKind::Fn ? n0 : n1;
                if(is_sym(a, pow.a, var) && is_sym(a, fn.a, var)) {
                    auto n = as_num(a, pow.b);
                    if(n && n->den == 1 && n->num >= 1) {
                        // ∫ x^n cos(x) dx = x^n sin(x) - n ∫ x^(n-1) sin(x) dx
                        NodeId sym_x = casio::sym(a, var);
                        NodeId xn_fact = casio::power(a, sym_x, a.num(*n));
                        NodeId term1 = casio::mul(a, {xn_fact, a.fn(FnKind::Sin, sym_x)});
                        NodeId xn1 = casio::power(a, sym_x, casio::num(a, n->num - 1));
                        NodeId xn1_sin = casio::mul(a, {xn1, a.fn(FnKind::Sin, sym_x)});
                        auto prim = integrate_simple(a, xn1_sin, var);
                        if(prim) {
                            NodeId n_node = a.num(*n);
                            NodeId term2 = casio::mul(a, {n_node, *prim});
                            return casio::simplify(a, casio::add(a, {term1, casio::neg(a, term2)}));
                        }
                    }
                }
            }
        }
        
        // x * exp(a*x) pattern
        if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
            Node const &n0 = a.get(x.kids[0]);
            Node const &n1 = a.get(x.kids[1]);
            if((n0.kind == NodeKind::Sym && n0.text == var && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Exp) ||
               (n1.kind == NodeKind::Sym && n1.text == var && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Exp)) {
                NodeId arg = (n0.kind == NodeKind::Sym) ? n1.a : n0.a;
                if(is_sym(a, arg, var)) {
                    NodeId sym_x = casio::sym(a, var);
                    NodeId term1 = casio::mul(a, {sym_x, a.fn(FnKind::Exp, sym_x)});
                    NodeId term2 = a.fn(FnKind::Exp, sym_x);
                    return casio::simplify(a, casio::add(a, {term1, casio::neg(a, term2)}));
                }
            }
        }
        // x * exp(a*x) pattern (a > 1)
        if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
            Node const &n0 = a.get(x.kids[0]);
            Node const &n1 = a.get(x.kids[1]);
            if((n0.kind == NodeKind::Sym && n0.text == var && n1.kind == NodeKind::Pow) ||
               (n1.kind == NodeKind::Sym && n1.text == var && n0.kind == NodeKind::Pow)) {
                Node const &pow = n0.kind == NodeKind::Pow ? n0 : n1;
                Node const &base = a.get(pow.a);
                if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) {
                    auto n = as_num(a, pow.b);
                    if(n && n->den == 1 && n->num > 1) {
                        // e^(ax) = exp(a*x), integrate by parts once
                        NodeId sym_x = casio::sym(a, var);
                        NodeId a_val = a.num(*n);
                        NodeId exp_ax = a.fn(FnKind::Exp, casio::mul(a, {a_val, sym_x}));
                        NodeId x_exp = casio::mul(a, {sym_x, exp_ax});
                        NodeId div_a = casio::div(a, exp_ax, a_val);
                        return casio::simplify(a, casio::add(a, {x_exp, casio::neg(a, div_a)}));
                    }
                }
            }
        }

    return std::nullopt;
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter f."};

    NodeId parsed = parse_expr(arena, req.expr);
    auto pre = casio::build_exam_prelude(arena, req.expr, parsed);
    NodeId node = casio::simplify(arena, parsed);

    auto prim = integrate_simple(arena, node, req.var);
    if(!prim) {
        return casio::exam_fallback(
            "integration (limited)",
            pre,
            "Full symbolic integral route not available for this form.",
            "Integral not recognised."
        );
    }

    NodeId simp = casio::simplify(arena, *prim);
    std::string ans = format_expr_human(arena, simp);
    return casio::exam_block(
        "direct table / linearity (limited)",
        {
            "Normalize: " + pre.norm,
            "Parse: " + pre.parsed,
            "Simplify: " + pre.simplified,
            "Integrate each term where possible.",
            "Simplify the result.",
            "I = " + ans + " + C",
        },
        ans + " + C"
    );
}

} // namespace casio::integrate


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
            // ∫ tan(x) dx = -ln|cos(x)|  (affine not yet)
            if(is_sym(a, arg, var)) {
                NodeId c = a.fn(FnKind::Cos, arg);
                return casio::neg(a, casio::fn(a, "log", casio::fn(a, "abs", c)));
            }
        }
        if(x.fkind == FnKind::Log && is_sym(a, arg, var)) {
            // ∫ ln(x) dx = x ln(x) - x
            NodeId v = casio::sym(a, var);
            NodeId xlnx = casio::mul(a, {v, expr});
            return casio::simplify(a, casio::add(a, {xlnx, casio::neg(a, v)}));
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
        },
        ans + " + C"
    );
}

} // namespace casio::integrate


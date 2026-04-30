#include "integrate.hpp"

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

    // Exponential: ∫ e^(kx) dx = e^(kx)/k
    if(x.kind == NodeKind::Pow) {
        auto const &base = a.get(x.a);
        if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) {
            if(is_sym(a, x.b, var)) return expr;
            if(auto k = const_times_var(a, x.b, var)) {
                return casio::simplify(a, casio::div(a, expr, *k));
            }
        }
    }

    // Basic trig/exp/log
    if(x.kind == NodeKind::Fn) {
        NodeId arg = x.a;
        if(x.fkind == FnKind::Sin) {
            if(is_sym(a, arg, var)) return casio::simplify(a, casio::neg(a, a.fn(FnKind::Cos, arg)));
            if(auto k = const_times_var(a, arg, var)) {
                // ∫ sin(kx) dx = -cos(kx)/k
                return casio::simplify(a, casio::div(a, casio::neg(a, a.fn(FnKind::Cos, arg)), *k));
            }
        }
        if(x.fkind == FnKind::Cos) {
            if(is_sym(a, arg, var)) return casio::simplify(a, a.fn(FnKind::Sin, arg));
            if(auto k = const_times_var(a, arg, var)) {
                // ∫ cos(kx) dx = sin(kx)/k
                return casio::simplify(a, casio::div(a, a.fn(FnKind::Sin, arg), *k));
            }
        }
        if(x.fkind == FnKind::Exp) {
            if(is_sym(a, arg, var)) return expr;
            if(auto k = const_times_var(a, arg, var)) {
                // ∫ exp(kx) dx = exp(kx)/k
                return casio::simplify(a, casio::div(a, expr, *k));
            }
        }
        if(x.fkind == FnKind::Log && is_sym(a, arg, var)) {
            // ∫ ln(x) dx = x ln(x) - x
            NodeId v = casio::sym(a, var);
            NodeId xlnx = casio::mul(a, {v, expr});
            return casio::simplify(a, casio::add(a, {xlnx, casio::neg(a, v)}));
        }
    }

    return std::nullopt;
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter f."};

    NodeId node = parse_expr(arena, req.expr);
    node = casio::simplify(arena, node);

    auto prim = integrate_simple(arena, node, req.var);
    if(!prim) {
        return {
            "Failed: integral not recognised (limited port).",
            "Expr: " + format_expr(arena, node),
        };
    }

    NodeId simp = casio::simplify(arena, *prim);
    std::string ans = format_expr(arena, simp);
    return {
        "Answer: " + ans + " + C",
    };
}

} // namespace casio::integrate


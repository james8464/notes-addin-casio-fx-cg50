#include "derive.hpp"

#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace casio::derive
{
namespace
{

static bool is_sym(Arena &a, NodeId n, std::string const &name)
{
    auto const &x = a.get(n);
    return x.kind == NodeKind::Sym && x.text == name;
}

static bool depends_on(Arena &a, NodeId n, std::string const &var)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Sym) return x.text == var;
    if(x.kind == NodeKind::Fn) return depends_on(a, x.a, var);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return depends_on(a, x.a, var) || depends_on(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(depends_on(a, k, var)) return true;
    }
    return false;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static NodeId diff(Arena &a, NodeId n, std::string const &var, std::string const &dep = "")
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return casio::num(a, 0);
    if(x.kind == NodeKind::Const) return casio::num(a, 0);
    if(x.kind == NodeKind::Sym) {
        if(x.text == var) return casio::num(a, 1);
        if(!dep.empty() && x.text == dep) return casio::sym(a, "d" + dep + "/d" + var);
        return casio::num(a, 0);
    }

    if(x.kind == NodeKind::Add) {
        std::vector<NodeId> parts;
        parts.reserve(x.kids.size());
        for(auto k : x.kids) parts.push_back(diff(a, k, var, dep));
        return casio::simplify(a, casio::add(a, parts));
    }
    if(x.kind == NodeKind::Mul) {
        std::vector<NodeId> sum_terms;
        for(std::size_t i = 0; i < x.kids.size(); i++) {
            std::vector<NodeId> prod;
            prod.reserve(x.kids.size());
            for(std::size_t j = 0; j < x.kids.size(); j++) {
                prod.push_back((j == i) ? diff(a, x.kids[j], var, dep) : x.kids[j]);
            }
            sum_terms.push_back(casio::mul(a, prod));
        }
        return casio::simplify(a, casio::add(a, sum_terms));
    }
    if(x.kind == NodeKind::Div) {
        NodeId u = x.a;
        NodeId v = x.b;
        NodeId up = diff(a, u, var, dep);
        NodeId vp = diff(a, v, var, dep);
        NodeId top = casio::add(a, {casio::mul(a, {up, v}), casio::neg(a, casio::mul(a, {u, vp}))});
        NodeId bot = casio::power(a, v, casio::num(a, 2));
        return casio::simplify(a, casio::div(a, top, bot));
    }
    if(x.kind == NodeKind::Pow) {
        NodeId u = x.a;
        NodeId v = x.b;
        auto vr = as_num(a, v);
        bool base_depends = depends_on(a, u, var) || (!dep.empty() && depends_on(a, u, dep));
        if(vr && base_depends && !depends_on(a, v, var) && (dep.empty() || !depends_on(a, v, dep))) {
            Rational nrat = *vr;
            NodeId n_node = a.num(nrat);
            Rational n_minus1 = nrat;
            n_minus1.num -= n_minus1.den;
            n_minus1.normalize();
            NodeId um1 = casio::power(a, u, a.num(n_minus1));
            NodeId up = diff(a, u, var, dep);
            return casio::simplify(a, casio::mul(a, {n_node, um1, up}));
        }
        NodeId up = diff(a, u, var, dep);
        NodeId vp = diff(a, v, var, dep);
        NodeId ln_u = casio::fn(a, "log", u);
        NodeId term1 = casio::mul(a, {vp, ln_u});
        NodeId term2 = casio::mul(a, {v, casio::div(a, up, u)});
        NodeId sum = casio::add(a, {term1, term2});
        return casio::simplify(a, casio::mul(a, {n, sum}));
    }
    if(x.kind == NodeKind::Fn) {
        NodeId u = x.a;
        NodeId up = diff(a, u, var, dep);
        switch(x.fkind) {
        case FnKind::Sin: return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Cos, u), up}));
        case FnKind::Cos: return casio::simplify(a, casio::mul(a, {casio::neg(a, a.fn(FnKind::Sin, u)), up}));
        case FnKind::Tan:
            return casio::simplify(a, casio::mul(a, {casio::power(a, a.fn(FnKind::Sec, u), casio::num(a, 2)), up}));
        case FnKind::Sec:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Sec, u), a.fn(FnKind::Tan, u), up}));
        case FnKind::Cosec:
            return casio::simplify(a, casio::mul(a, {casio::neg(a, casio::mul(a, {a.fn(FnKind::Cosec, u), a.fn(FnKind::Cot, u)})), up}));
        case FnKind::Cot:
            return casio::simplify(a, casio::mul(a, {casio::neg(a, casio::power(a, a.fn(FnKind::Cosec, u), casio::num(a, 2))), up}));
        case FnKind::Exp:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Exp, u), up}));
        case FnKind::Log:
            return casio::simplify(a, casio::mul(a, {casio::div(a, casio::num(a, 1), u), up}));
        case FnKind::Sqrt:
            return casio::simplify(a, casio::div(a, up, casio::mul(a, {casio::num(a, 2), a.fn(FnKind::Sqrt, u)})));
        case FnKind::Abs:
            return casio::simplify(a, casio::mul(a, {casio::div(a, u, a.fn(FnKind::Abs, u)), up}));
        default:
            return casio::num(a, 0);
        }
    }

    return casio::num(a, 0);
}

static std::vector<std::string> split_csv(std::string const &s)
{
    std::vector<std::string> parts;
    std::string cur;
    int depth = 0;
    for(char c : s) {
        if(c == '(' || c == '[' || c == '{') depth++;
        if(c == ')' || c == ']' || c == '}') depth--;
        if(c == ',' && depth == 0) {
            parts.push_back(cur);
            cur.clear();
        }
        else cur.push_back(c);
    }
    if(!cur.empty()) parts.push_back(cur);
    for(auto &p : parts) {
        while(!p.empty() && (p.front() == ' ' || p.front() == '\t')) p.erase(p.begin());
        while(!p.empty() && (p.back() == ' ' || p.back() == '\t')) p.pop_back();
    }
    return parts;
}

static std::pair<NodeId, NodeId> coeff_of(Arena &a, NodeId whole, std::string const &dname)
{
    whole = casio::simplify(a, whole);
    Node const &w = a.get(whole);
    if(w.kind == NodeKind::Add) {
        NodeId coef = casio::num(a, 0);
        std::vector<NodeId> rest_terms;
        for(auto t : w.kids) {
            auto [c, r] = coeff_of(a, t, dname);
            coef = casio::simplify(a, casio::add(a, {coef, c}));
            if(!(a.get(r).kind == NodeKind::Num && a.get(r).num.num == 0)) rest_terms.push_back(r);
        }
        NodeId rest = rest_terms.empty() ? casio::num(a, 0) : casio::add(a, rest_terms);
        return {coef, rest};
    }
    if(is_sym(a, whole, dname)) {
        return {casio::num(a, 1), casio::num(a, 0)};
    }
    if(w.kind == NodeKind::Mul) {
        std::vector<NodeId> others;
        bool found = false;
        for(auto f : w.kids) {
            if(is_sym(a, f, dname)) found = true;
            else others.push_back(f);
        }
        if(found) {
            NodeId c = others.empty() ? casio::num(a, 1) : casio::mul(a, others);
            return {c, casio::num(a, 0)};
        }
    }
    return {casio::num(a, 0), whole};
}

} // namespace

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter an expression."};

    try {
        if(req.mode == 1 || req.mode == 4) {
            auto parts = split_csv(req.expr);
            std::string expr = parts[0];
            std::string var = (parts.size() >= 2 && !parts[1].empty()) ? parts[1] : "x";

            NodeId n = casio::simplify(arena, casio::parse_expr(arena, expr));
            NodeId d1 = casio::simplify(arena, diff(arena, n, var));
            NodeId out = d1;
            std::string label = "dy/d" + var;
            if(req.mode == 4) {
                NodeId d2 = casio::simplify(arena, diff(arena, d1, var));
                out = d2;
                label = "d2y/d" + var + "2";
            }
            return {
                "Answer: " + label + " = " + format_expr_human(arena, out),
            };
        }
        if(req.mode == 2) {
            auto eq = casio::parse_equation(arena, req.expr);
            if(!eq) return {"Error: Use left=right."};
            NodeId left = casio::simplify(arena, eq->lhs);
            NodeId right = casio::simplify(arena, eq->rhs);
            std::string var = "x";
            std::string dep = "y";
            NodeId work = casio::simplify(arena, casio::add(arena, {left, casio::neg(arena, right)}));
            NodeId d = casio::simplify(arena, diff(arena, work, var, dep));
            std::string dname = "d" + dep + "/d" + var;
            auto [coef, rest] = coeff_of(arena, d, dname);
            NodeId ans = casio::simplify(arena, casio::div(arena, casio::neg(arena, rest), coef));
            return {
                "Implicit differentiation (limited port).",
                "d/dx(lhs)=d/dx(rhs)",
                "Make dy/dx the subject.",
                "Answer: " + dname + " = " + format_expr_human(arena, ans),
            };
        }
        if(req.mode == 3) {
            auto parts = split_csv(req.expr);
            if(parts.size() < 2) return {"Error: Provide x(t), y(t)."};
            std::string xt = parts[0];
            std::string yt = parts[1];
            std::string tvar = (parts.size() >= 3 && !parts[2].empty()) ? parts[2] : "t";
            NodeId xnode = casio::simplify(arena, casio::parse_expr(arena, xt));
            NodeId ynode = casio::simplify(arena, casio::parse_expr(arena, yt));
            NodeId dxdt = casio::simplify(arena, diff(arena, xnode, tvar));
            NodeId dydt = casio::simplify(arena, diff(arena, ynode, tvar));
            NodeId dydx = casio::simplify(arena, casio::div(arena, dydt, dxdt));
            return {
                "Parametric differentiation (limited port).",
                "dx/dt = " + format_expr_human(arena, dxdt),
                "dy/dt = " + format_expr_human(arena, dydt),
                "Answer: dy/dx = " + format_expr_human(arena, dydx),
            };
        }
        return {"Error: unknown mode."};
    }
    catch(std::exception const &e) {
        return {std::string("Error: ") + e.what()};
    }
}

} // namespace casio::derive


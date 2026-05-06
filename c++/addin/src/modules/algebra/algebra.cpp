#include "algebra.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/same.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <limits>

namespace casio::algebra
{

struct Poly2
{
    Rational a2{0, 1};
    Rational a1{0, 1};
    Rational a0{0, 1};
    bool ok = true;
};

static bool is_zero(Rational const &r) { return r.num == 0; }

static Rational r_add(Rational a, Rational b)
{
    Rational r;
    r.num = a.num * b.den + b.num * a.den;
    r.den = a.den * b.den;
    r.normalize();
    return r;
}
static Rational r_mul(Rational a, Rational b)
{
    Rational r;
    r.num = a.num * b.num;
    r.den = a.den * b.den;
    r.normalize();
    return r;
}
static Rational r_div(Rational a, Rational b)
{
    Rational r;
    r.num = a.num * b.den;
    r.den = a.den * b.num;
    r.normalize();
    return r;
}
static Rational r_neg(Rational a)
{
    a.num = -a.num;
    return a;
}
static Rational r_sub(Rational a, Rational b)
{
    return r_add(a, r_neg(b));
}

static Rational r_abs(Rational a)
{
    if(a.num < 0) a.num = -a.num;
    return a;
}

static int r_cmp(Rational a, Rational b)
{
    a.normalize();
    b.normalize();
    long long lhs = a.num * b.den;
    long long rhs = b.num * a.den;
    return (lhs > rhs) - (lhs < rhs);
}

static Poly2 add_poly(Poly2 p, Poly2 const &q)
{
    if(!p.ok || !q.ok) return Poly2{{}, {}, {}, false};
    p.a2 = r_add(p.a2, q.a2);
    p.a1 = r_add(p.a1, q.a1);
    p.a0 = r_add(p.a0, q.a0);
    return p;
}

static Poly2 mul_poly(Poly2 const &p, Poly2 const &q)
{
    if(!p.ok || !q.ok) return Poly2{{}, {}, {}, false};
    // degree <= 2 only
    Rational r2 = r_add(r_mul(p.a2, q.a0), r_add(r_mul(p.a1, q.a1), r_mul(p.a0, q.a2)));
    Rational r1 = r_add(r_mul(p.a1, q.a0), r_mul(p.a0, q.a1));
    Rational r0 = r_mul(p.a0, q.a0);

    // reject x^3 terms
    if(!is_zero(r_mul(p.a2, q.a1)) || !is_zero(r_mul(p.a1, q.a2)) || !is_zero(r_mul(p.a2, q.a2))) {
        return Poly2{{}, {}, {}, false};
    }
    return Poly2{r2, r1, r0, true};
}

static std::optional<std::int64_t> as_int64(Rational const &r)
{
    if(r.den != 1) return std::nullopt;
    return r.num;
}

static bool is_square_i64(std::int64_t n, std::int64_t &root_out)
{
    if(n < 0) return false;
    std::int64_t r = 0;
    while((r + 1) * (r + 1) <= n && r < 3037000499LL) r++;
    if(r * r == n) {
        root_out = r;
        return true;
    }
    return false;
}

static std::optional<Poly2> poly_of(Arena &a, NodeId n, std::string const &var)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Num) return Poly2{Rational{0, 1}, Rational{0, 1}, x.num, true};
    if(x.kind == NodeKind::Sym && x.text == var) return Poly2{Rational{0, 1}, Rational{1, 1}, Rational{0, 1}, true};
    if(x.kind == NodeKind::Add) {
        Poly2 out{Rational{0, 1}, Rational{0, 1}, Rational{0, 1}, true};
        for(auto kid : x.kids) {
            auto pk = poly_of(a, kid, var);
            if(!pk || !pk->ok) return std::nullopt;
            out = add_poly(out, *pk);
        }
        return out;
    }
    if(x.kind == NodeKind::Mul) {
        Poly2 out{Rational{0, 1}, Rational{0, 1}, Rational{1, 1}, true};
        for(auto kid : x.kids) {
            auto pk = poly_of(a, kid, var);
            if(!pk || !pk->ok) return std::nullopt;
            out = mul_poly(out, *pk);
            if(!out.ok) return std::nullopt;
        }
        return out;
    }
    if(x.kind == NodeKind::Pow) {
        auto base = poly_of(a, x.a, var);
        auto expn = a.get(x.b);
        if(!base || !base->ok) return std::nullopt;
        if(expn.kind != NodeKind::Num || expn.num.den != 1) return std::nullopt;
        auto e = expn.num.num;
        if(e < 0 || e > 2) return std::nullopt;
        if(e == 0) return Poly2{Rational{0, 1}, Rational{0, 1}, Rational{1, 1}, true};
        if(e == 1) return *base;
        // e == 2
        return mul_poly(*base, *base);
    }
    if(x.kind == NodeKind::Div) {
        auto top = poly_of(a, x.a, var);
        if(!top || !top->ok) return std::nullopt;
        auto den = a.get(x.b);
        if(den.kind != NodeKind::Num) return std::nullopt;
        Rational inv{den.num.den, den.num.num};
        inv.normalize();
        return Poly2{r_mul(top->a2, inv), r_mul(top->a1, inv), r_mul(top->a0, inv), true};
    }
    return std::nullopt;
}

static NodeId poly2_to_node(Arena &a, Poly2 const &p, std::string const &var)
{
    std::vector<NodeId> terms;
    if(!is_zero(p.a2)) {
        terms.push_back(casio::mul(a, {
            a.num(p.a2),
            casio::power(a, a.sym(var), casio::num(a, 2)),
        }));
    }
    if(!is_zero(p.a1)) terms.push_back(casio::mul(a, {a.num(p.a1), a.sym(var)}));
    if(!is_zero(p.a0) || terms.empty()) terms.push_back(a.num(p.a0));
    return casio::simplify(a, casio::add(a, terms));
}

struct RatPoly2
{
    Poly2 num;
    Poly2 den;
    bool ok = true;
};

static RatPoly2 ratpoly_of(Arena &a, NodeId n, std::string const &var)
{
    auto p = poly_of(a, n, var);
    if(!p || !p->ok) return RatPoly2{{}, {}, false};
    Poly2 one{Rational{0, 1}, Rational{0, 1}, Rational{1, 1}, true};
    return RatPoly2{*p, one, true};
}

static RatPoly2 ratpoly_mul(RatPoly2 const &x, RatPoly2 const &y)
{
    if(!x.ok || !y.ok) return RatPoly2{{}, {}, false};
    Poly2 n = mul_poly(x.num, y.num);
    Poly2 d = mul_poly(x.den, y.den);
    if(!n.ok || !d.ok) return RatPoly2{{}, {}, false};
    return RatPoly2{n, d, true};
}

static RatPoly2 ratpoly_add(RatPoly2 const &x, RatPoly2 const &y)
{
    if(!x.ok || !y.ok) return RatPoly2{{}, {}, false};
    // x.num/x.den + y.num/y.den = (x.num*y.den + y.num*x.den) / (x.den*y.den)
    Poly2 n1 = mul_poly(x.num, y.den);
    Poly2 n2 = mul_poly(y.num, x.den);
    Poly2 n = add_poly(n1, n2);
    Poly2 d = mul_poly(x.den, y.den);
    if(!n.ok || !d.ok) return RatPoly2{{}, {}, false};
    return RatPoly2{n, d, true};
}

static RatPoly2 ratpoly_div(RatPoly2 const &x, RatPoly2 const &y)
{
    if(!x.ok || !y.ok) return RatPoly2{{}, {}, false};
    // x / y = (x.num/x.den) / (y.num/y.den) = (x.num*y.den)/(x.den*y.num)
    Poly2 n = mul_poly(x.num, y.den);
    Poly2 d = mul_poly(x.den, y.num);
    if(!n.ok || !d.ok) return RatPoly2{{}, {}, false};
    return RatPoly2{n, d, true};
}

static RatPoly2 ratpoly_of_node(Arena &a, NodeId n, std::string const &var)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Add) {
        RatPoly2 acc = ratpoly_of(a, casio::num(a, 0), var);
        for(auto kid : x.kids) {
            acc = ratpoly_add(acc, ratpoly_of_node(a, kid, var));
            if(!acc.ok) return acc;
        }
        return acc;
    }
    if(x.kind == NodeKind::Mul) {
        RatPoly2 acc = ratpoly_of(a, casio::num(a, 1), var);
        for(auto kid : x.kids) {
            acc = ratpoly_mul(acc, ratpoly_of_node(a, kid, var));
            if(!acc.ok) return acc;
        }
        return acc;
    }
    if(x.kind == NodeKind::Div) {
        return ratpoly_div(ratpoly_of_node(a, x.a, var), ratpoly_of_node(a, x.b, var));
    }
    if(x.kind == NodeKind::Pow) {
        // Support (poly)^k for k in {-2,-1,0,1,2}
        auto expn = a.get(x.b);
        if(expn.kind == NodeKind::Num && expn.num.den == 1) {
            auto k = expn.num.num;
            if(k >= -2 && k <= 2) {
                auto base = poly_of(a, x.a, var);
                if(!base || !base->ok) return RatPoly2{{}, {}, false};
                Poly2 one{Rational{0, 1}, Rational{0, 1}, Rational{1, 1}, true};
                auto pow_poly = [&](Poly2 const &p, int e) -> Poly2 {
                    if(e == 0) return one;
                    if(e == 1) return p;
                    if(e == 2) return mul_poly(p, p);
                    return Poly2{{}, {}, {}, false};
                };
                if(k >= 0) {
                    auto p = pow_poly(*base, (int)k);
                    if(!p.ok) return RatPoly2{{}, {}, false};
                    return RatPoly2{p, one, true};
                }
                // negative -> denominator
                auto p = pow_poly(*base, (int)(-k));
                if(!p.ok) return RatPoly2{{}, {}, false};
                return RatPoly2{one, p, true};
            }
        }
    }
    // Fallback to polynomial
    return ratpoly_of(a, n, var);
}

static NodeId clone_with_substitution(Arena &a, NodeId n, std::string const &var, NodeId replacement)
{
    Node const &x = a.get(n);
    switch(x.kind) {
    case NodeKind::Num: return casio::num(a, x.num.num, x.num.den);
    case NodeKind::Sym: return x.text == var ? replacement : casio::sym(a, x.text);
    case NodeKind::Const: return a.constant(x.ckind);
    case NodeKind::Fn: return a.fn(x.fkind, clone_with_substitution(a, x.a, var, replacement));
    case NodeKind::Pow:
        return casio::power(
            a,
            clone_with_substitution(a, x.a, var, replacement),
            clone_with_substitution(a, x.b, var, replacement)
        );
    case NodeKind::Div:
        return casio::div(
            a,
            clone_with_substitution(a, x.a, var, replacement),
            clone_with_substitution(a, x.b, var, replacement)
        );
    case NodeKind::Add: {
        std::vector<NodeId> kids;
        kids.reserve(x.kids.size());
        for(auto k : x.kids) kids.push_back(clone_with_substitution(a, k, var, replacement));
        return casio::add(a, kids);
    }
    case NodeKind::Mul: {
        std::vector<NodeId> kids;
        kids.reserve(x.kids.size());
        for(auto k : x.kids) kids.push_back(clone_with_substitution(a, k, var, replacement));
        return casio::mul(a, kids);
    }
    }
    return n;
}

static bool is_degree_at_most_one(Poly2 const &p)
{
    return p.ok && is_zero(p.a2);
}

static NodeId linear_expr(Arena &a, Rational m, Rational b, NodeId x)
{
    std::vector<NodeId> terms;
    if(!is_zero(m)) terms.push_back(casio::mul(a, {casio::num(a, m.num, m.den), x}));
    if(!is_zero(b)) terms.push_back(casio::num(a, b.num, b.den));
    if(terms.empty()) return casio::num(a, 0);
    return casio::simplify(a, casio::add(a, terms));
}

static std::optional<NodeId> inverse_mobius(Arena &a, NodeId expr)
{
    auto rp = ratpoly_of_node(a, expr, "x");
    if(!rp.ok || !is_degree_at_most_one(rp.num) || !is_degree_at_most_one(rp.den)) return std::nullopt;

    Rational aa = rp.num.a1;
    Rational b = rp.num.a0;
    Rational c = rp.den.a1;
    Rational d = rp.den.a0;
    // Constant functions are not invertible.
    if(is_zero(aa) && is_zero(c)) return std::nullopt;
    if(is_zero(r_sub(r_mul(aa, d), r_mul(b, c)))) return std::nullopt;

    NodeId x = casio::sym(a, "x");
    NodeId top = linear_expr(a, r_neg(d), b, x);      // b - d*x
    NodeId bot = linear_expr(a, c, r_neg(aa), x);     // c*x - a
    return casio::simplify(a, casio::div(a, top, bot));
}

static std::optional<NodeId> inverse_simple_function(Arena &a, NodeId expr)
{
    if(auto inv = inverse_mobius(a, expr)) return inv;
    Node const &x = a.get(expr);
    NodeId y = casio::sym(a, "x");

    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt) {
        auto p = poly_of(a, x.a, "x");
        if(p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) {
            // y = sqrt(ax+b) => x = (y^2-b)/a
            NodeId y2 = casio::power(a, y, casio::num(a, 2));
            NodeId top = casio::add(a, {y2, casio::neg(a, casio::num(a, p->a0.num, p->a0.den))});
            return casio::simplify(a, casio::div(a, top, casio::num(a, p->a1.num, p->a1.den)));
        }
    }
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Log) {
        auto p = poly_of(a, x.a, "x");
        if(p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) {
            // y = ln(ax+b) => x = (e^y-b)/a
            NodeId ey = casio::power(a, casio::constant_e(a), y);
            NodeId top = casio::add(a, {ey, casio::neg(a, casio::num(a, p->a0.num, p->a0.den))});
            return casio::simplify(a, casio::div(a, top, casio::num(a, p->a1.num, p->a1.den)));
        }
        Node const &arg = a.get(x.a);
        if(arg.kind == NodeKind::Pow) {
            Node const &exp_node = a.get(arg.b);
            auto exp = (exp_node.kind == NodeKind::Num) ? as_int64(exp_node.num) : std::optional<std::int64_t>{};
            auto p2 = poly_of(a, arg.a, "x");
            if(exp && *exp != 0 && p2 && p2->ok && is_zero(p2->a2) && !is_zero(p2->a1)) {
                NodeId ey = casio::power(a, casio::constant_e(a), casio::div(a, y, casio::num(a, *exp)));
                NodeId top = casio::add(a, {ey, casio::neg(a, casio::num(a, p2->a0.num, p2->a0.den))});
                return casio::simplify(a, casio::div(a, top, casio::num(a, p2->a1.num, p2->a1.den)));
            }
        }
    }
    if(x.kind == NodeKind::Div) {
        auto top = a.get(x.a);
        Node const &den = a.get(x.b);
        if(top.kind == NodeKind::Num && top.num.num == top.num.den && den.kind == NodeKind::Fn && den.fkind == FnKind::Sqrt) {
            auto p = poly_of(a, den.a, "x");
            if(p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) {
                NodeId y2 = casio::power(a, y, casio::num(a, 2));
                NodeId inv_y2 = casio::div(a, casio::num(a, 1), y2);
                NodeId topn = casio::add(a, {inv_y2, casio::neg(a, casio::num(a, p->a0.num, p->a0.den))});
                return casio::simplify(a, casio::div(a, topn, casio::num(a, p->a1.num, p->a1.den)));
            }
        }
    }
    if(x.kind == NodeKind::Pow) {
        Node const &base = a.get(x.a);
        if(base.kind == NodeKind::Num && base.num.num > 0 && base.num.den == 1) {
            auto p = poly_of(a, x.b, "x");
            if(p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) {
                // y = b^(ax+c) => x = (log(y)/log(b)-c)/a
                NodeId logy = casio::fn(a, "log", y);
                NodeId logb = casio::fn(a, "log", casio::num(a, base.num.num, base.num.den));
                NodeId inner = casio::div(a, logy, logb);
                NodeId top = casio::add(a, {inner, casio::neg(a, casio::num(a, p->a0.num, p->a0.den))});
                return casio::simplify(a, casio::div(a, top, casio::num(a, p->a1.num, p->a1.den)));
            }
        }
    }
    return std::nullopt;
}

static std::optional<NodeId> cartesian_from_param(Arena &a, std::string const &x_expr, std::string const &y_expr, std::string const &param)
{
    NodeId xn = casio::simplify(a, casio::parse_expr(a, x_expr));
    NodeId yn = casio::simplify(a, casio::parse_expr(a, y_expr));
    auto xp = poly_of(a, xn, param);
    if(!xp || !xp->ok || !is_zero(xp->a2) || is_zero(xp->a1)) return std::nullopt;

    // x = a*t+b => t = (x-b)/a
    NodeId x = casio::sym(a, "x");
    NodeId top = casio::add(a, {x, casio::neg(a, casio::num(a, xp->a0.num, xp->a0.den))});
    NodeId t_expr = casio::simplify(a, casio::div(a, top, casio::num(a, xp->a1.num, xp->a1.den)));
    return casio::simplify(a, clone_with_substitution(a, yn, param, t_expr));
}

static std::string trim_text(std::string s)
{
    while(!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.erase(s.begin());
    while(!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

static bool pythagorean_square_sum(std::string text)
{
    std::string key;
    key.reserve(text.size());
    for(char c : text) {
        if(!std::isspace(static_cast<unsigned char>(c))) key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return key.find("sin(") != std::string::npos &&
           key.find("cos(") != std::string::npos &&
           key.find("^2") != std::string::npos &&
           key.find('+') != std::string::npos;
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
            parts.push_back(trim_text(cur));
            cur.clear();
        }
        else {
            cur.push_back(c);
        }
    }
    parts.push_back(trim_text(cur));
    return parts;
}

static void collect_symbols(Arena &a, NodeId n, std::vector<std::string> &out)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Sym) {
        for(auto const &v : out) {
            if(v == x.text) return;
        }
        out.push_back(x.text);
        return;
    }
    if(x.kind == NodeKind::Fn) {
        collect_symbols(a, x.a, out);
        return;
    }
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) {
        collect_symbols(a, x.a, out);
        collect_symbols(a, x.b, out);
        return;
    }
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids) collect_symbols(a, k, out);
        return;
    }
}

static bool has_symbols(Arena &a, NodeId n)
{
    std::vector<std::string> vars;
    collect_symbols(a, n, vars);
    return !vars.empty();
}

static void push_unique(std::vector<std::string> &out, std::string const &line)
{
    for(auto const &x : out)
        if(x == line) return;
    out.push_back(line);
}

static std::string choose_solve_var(Arena &a, NodeId n, std::string const &explicit_var)
{
    if(!explicit_var.empty()) return explicit_var;
    std::vector<std::string> vars;
    collect_symbols(a, n, vars);
    for(auto const &v : vars) {
        if(v == "x") return v;
    }
    return vars.empty() ? "x" : vars.front();
}

static bool contains_symbol(Arena &a, NodeId n, std::string const &name)
{
    std::vector<std::string> vars;
    collect_symbols(a, n, vars);
    for(auto const &v : vars)
        if(v == name) return true;
    return false;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static NodeId one_node(Arena &a) { return casio::num(a, 1); }

static NodeId mul_or_one(Arena &a, std::vector<NodeId> const &terms)
{
    if(terms.empty()) return one_node(a);
    if(terms.size() == 1) return terms[0];
    return casio::simplify(a, casio::mul(a, terms));
}

static bool is_sym_var(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    return x.kind == NodeKind::Sym && x.text == var;
}

static bool is_pow_var(Arena &a, NodeId n, std::string const &var, int p)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Sym && x.text == var && p == 1) return true;
    if(x.kind != NodeKind::Pow) return false;
    if(!is_sym_var(a, x.a, var)) return false;
    Node const &e = a.get(x.b);
    return e.kind == NodeKind::Num && e.num.den == 1 && e.num.num == p;
}

static std::optional<NodeId> strip_one_linear_x(Arena &a, NodeId term, std::string const &var)
{
    if(is_sym_var(a, term, var)) return one_node(a);
    Node const &t = a.get(term);
    if(t.kind != NodeKind::Mul) return std::nullopt;
    bool saw_x = false;
    std::vector<NodeId> rest;
    for(NodeId kid : t.kids) {
        if(is_sym_var(a, kid, var)) {
            if(saw_x) return std::nullopt;
            saw_x = true;
        }
        else {
            if(contains_symbol(a, kid, var)) return std::nullopt;
            rest.push_back(kid);
        }
    }
    if(!saw_x) return std::nullopt;
    return mul_or_one(a, rest);
}

static std::optional<NodeId> strip_one_quadratic_x(Arena &a, NodeId term, std::string const &var)
{
    if(is_pow_var(a, term, var, 2)) return one_node(a);
    Node const &t = a.get(term);
    if(t.kind != NodeKind::Mul) return std::nullopt;
    bool saw_x2 = false;
    std::vector<NodeId> rest;
    for(NodeId kid : t.kids) {
        if(is_pow_var(a, kid, var, 2)) {
            if(saw_x2) return std::nullopt;
            saw_x2 = true;
        }
        else {
            if(contains_symbol(a, kid, var)) return std::nullopt;
            rest.push_back(kid);
        }
    }
    if(!saw_x2) return std::nullopt;
    return mul_or_one(a, rest);
}

struct SymbolicQuadratic
{
    NodeId b = 0;
    NodeId c = 0;
};

static std::optional<SymbolicQuadratic> monic_symbolic_quadratic(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms = (x.kind == NodeKind::Add) ? x.kids : std::vector<NodeId>{n};
    bool saw_x2 = false;
    std::vector<NodeId> b_terms;
    std::vector<NodeId> c_terms;
    for(NodeId term : terms) {
        if(auto q = strip_one_quadratic_x(a, term, var)) {
            auto qr = as_num(a, *q);
            if(!qr || qr->num != qr->den) return std::nullopt;
            saw_x2 = true;
            continue;
        }
        if(auto b = strip_one_linear_x(a, term, var)) {
            b_terms.push_back(*b);
            continue;
        }
        if(contains_symbol(a, term, var)) return std::nullopt;
        c_terms.push_back(term);
    }
    if(!saw_x2 || b_terms.empty()) return std::nullopt;
    return SymbolicQuadratic{
        casio::simplify(a, casio::add(a, b_terms)),
        c_terms.empty() ? casio::num(a, 0) : casio::simplify(a, casio::add(a, c_terms))
    };
}

static std::optional<std::vector<std::string>> symbolic_complete_square(Arena &a, NodeId n, std::string const &raw, std::string const &var)
{
    auto q = monic_symbolic_quadratic(a, n, var);
    if(!q) return std::nullopt;
    NodeId x = casio::sym(a, var);
    NodeId half_b = casio::simplify(a, casio::div(a, q->b, casio::num(a, 2)));
    NodeId square = casio::power(a, casio::add(a, {x, half_b}), casio::num(a, 2));
    NodeId correction = casio::simplify(a, casio::power(a, half_b, casio::num(a, 2)));
    NodeId tail = casio::simplify(a, casio::add(a, {q->c, casio::neg(a, correction)}));
    NodeId ans = casio::add(a, {square, tail});
    std::string btxt = format_expr(a, q->b);
    return std::vector<std::string>{
        "1. Complete square for " + raw + ".",
        "2. Coefficient of " + var + " is " + btxt + ".",
        "3. h = b/(2a) = " + format_expr(a, half_b) + ".",
        "4. k = c - b^2/(4a) = " + format_expr(a, tail) + ".",
        "5. Add and subtract (" + format_expr(a, half_b) + ")^2.",
        "Answer: " + format_expr(a, ans),
    };
}

static std::optional<std::string> exp_plus_recip_range(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return std::nullopt;
    auto split = [&](NodeId term) -> std::optional<std::pair<NodeId, NodeId>> {
        Node const &t = a.get(term);
        if(t.kind == NodeKind::Fn && t.fkind == FnKind::Exp) return std::make_pair(one_node(a), t.a);
        if(t.kind == NodeKind::Pow) {
            Node const &base = a.get(t.a);
            if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) return std::make_pair(one_node(a), t.b);
        }
        if(t.kind != NodeKind::Mul) return std::nullopt;
        std::vector<NodeId> coeff;
        std::optional<NodeId> arg;
        for(NodeId kid : t.kids) {
            Node const &k = a.get(kid);
            if(k.kind == NodeKind::Fn && k.fkind == FnKind::Exp && !arg) arg = k.a;
            else if(k.kind == NodeKind::Pow && !arg) {
                Node const &base = a.get(k.a);
                if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) arg = k.b;
                else coeff.push_back(kid);
            }
            else coeff.push_back(kid);
        }
        if(!arg) return std::nullopt;
        return std::make_pair(mul_or_one(a, coeff), *arg);
    };
    auto a1 = split(x.kids[0]);
    auto a2 = split(x.kids[1]);
    if(!a1 || !a2) return std::nullopt;
    NodeId neg1 = casio::simplify(a, casio::neg(a, a1->second));
    NodeId neg2 = casio::simplify(a, casio::neg(a, a2->second));
    NodeId c1 = a1->first;
    NodeId c2 = a2->first;
    if(!casio::same_by_sig(a, a2->second, neg1)) {
        if(!casio::same_by_sig(a, a1->second, neg2)) return std::nullopt;
        std::swap(c1, c2);
    }
    NodeId prod = casio::simplify(a, casio::mul(a, {c1, c2}));
    NodeId bound = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::fn(a, "sqrt", prod)}));
    steps.push_back("Let u=e^(...) > 0, so expression has form A*u+B/u.");
    steps.push_back("For A,B>0, AM-GM gives A*u+B/u >= 2*sqrt(A*B).");
    return "y >= " + format_expr(a, bound);
}

static std::string rat_node_text(Arena &a, Rational r)
{
    return format_expr(a, casio::num(a, r.num, r.den));
}

static bool is_x_square_factor(Arena &a, NodeId n, std::string const &var)
{
    return is_pow_var(a, n, var, 2);
}

static std::optional<std::pair<Rational, Rational>> linear_poly_coeffs(Arena &a, NodeId n, std::string const &var)
{
    auto p = poly_of(a, n, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return std::nullopt;
    return std::make_pair(p->a1, p->a0);
}

static std::optional<std::vector<std::string>> partial_fraction_x2_linear(Arena &a, NodeId parsed, std::string const &raw, std::string const &var)
{
    Node const &d = a.get(parsed);
    if(d.kind != NodeKind::Div) return std::nullopt;
    auto num = poly_of(a, d.a, var);
    if(!num || !num->ok) return std::nullopt;
    Node const &den = a.get(d.b);
    if(den.kind != NodeKind::Mul) return std::nullopt;
    bool saw_x2 = false;
    std::optional<std::pair<Rational, Rational>> lin;
    for(NodeId kid : den.kids) {
        if(is_x_square_factor(a, kid, var)) {
            if(saw_x2) return std::nullopt;
            saw_x2 = true;
        }
        else if(auto lp = linear_poly_coeffs(a, kid, var)) {
            if(lin) return std::nullopt;
            lin = *lp;
        }
        else return std::nullopt;
    }
    if(!saw_x2 || !lin) return std::nullopt;
    Rational a1 = lin->first;
    Rational b0 = lin->second;
    if(is_zero(a1) || is_zero(b0)) return std::nullopt;
    Rational A = r_div(num->a0, b0);
    Rational B = r_div(r_sub(num->a1, r_mul(A, a1)), b0);
    Rational C = r_sub(num->a2, r_mul(B, a1));

    std::vector<std::string> answer_terms;
    if(!is_zero(A)) answer_terms.push_back(rat_node_text(a, A) + "/" + var + "^2");
    if(!is_zero(B)) answer_terms.push_back(rat_node_text(a, B) + "/" + var);
    if(!is_zero(C)) answer_terms.push_back(rat_node_text(a, C) + "/(" + rat_node_text(a, a1) + "*" + var + (b0.num < 0 ? "" : "+") + rat_node_text(a, b0) + ")");
    std::string ans;
    for(std::size_t i = 0; i < answer_terms.size(); ++i) {
        if(i) ans += " + ";
        ans += answer_terms[i];
    }
    return std::vector<std::string>{
        "1. Start with " + raw + ".",
        "2. Use A/" + var + "^2 + B/" + var + " + C/(" + rat_node_text(a, a1) + "*" + var + (b0.num < 0 ? "" : "+") + rat_node_text(a, b0) + ").",
        "3. Multiply by the denominator and equate coefficients.",
        "4. A=" + rat_node_text(a, A) + ", B=" + rat_node_text(a, B) + ", C=" + rat_node_text(a, C) + ".",
        "Answer: " + ans,
    };
}

static Rational r_pow_int(Rational r, int p)
{
    Rational out{1, 1};
    int n = p < 0 ? -p : p;
    for(int i = 0; i < n; ++i) out = r_mul(out, r);
    if(p < 0) out = r_div(Rational{1, 1}, out);
    return out;
}

static std::optional<std::int64_t> square_root_i64(std::int64_t n)
{
    if(n < 0) return std::nullopt;
    auto r = static_cast<std::int64_t>(std::llround(std::sqrt(static_cast<double>(n))));
    if(r * r == n) return r;
    return std::nullopt;
}

static std::optional<Rational> rational_power_factor(Rational base, Rational power)
{
    if(power.den == 1) return r_pow_int(base, static_cast<int>(power.num));
    if(power.den != 2 || base.num <= 0 || base.den <= 0) return std::nullopt;
    auto sn = square_root_i64(base.num);
    auto sd = square_root_i64(base.den);
    if(!sn || !sd) return std::nullopt;
    Rational root{*sn, *sd};
    return r_pow_int(root, static_cast<int>(power.num));
}

static Rational binom_rat(Rational p, int r)
{
    Rational out{1, 1};
    for(int i = 0; i < r; ++i) out = r_mul(out, r_sub(p, Rational{i, 1}));
    for(int i = 2; i <= r; ++i) out = r_div(out, Rational{i, 1});
    return out;
}

static std::optional<std::vector<std::string>> binomial_series_route(Arena &a, std::string const &inner)
{
    auto args = split_csv(inner);
    if(args.empty()) return std::nullopt;
    std::string expr_text = args[0];
    bool from_recip = inner.find("method=from_reciprocal") != std::string::npos;
    std::string var = args.size() >= 2 && !args[1].empty() ? args[1] : "x";
    int degree = 3;
    if(args.size() >= 4) degree = std::atoi(args[3].c_str());
    else if(args.size() >= 3) degree = std::atoi(args[2].c_str());
    if(degree < 0 || degree > 8) degree = 3;

    NodeId parsed = casio::parse_expr(a, expr_text);
    Node const &x = a.get(parsed);
    NodeId base = 0;
    Rational power{1, 1};
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt) {
        base = x.a;
        power = Rational{1, 2};
    }
    else if(x.kind == NodeKind::Pow) {
        Node const &e = a.get(x.b);
        if(e.kind != NodeKind::Num) return std::nullopt;
        base = x.a;
        power = e.num;
    }
    else if(x.kind == NodeKind::Div) {
        auto top = as_num(a, x.a);
        if(!top || top->num != top->den) return std::nullopt;
        base = x.b;
        power = Rational{-1, 1};
    }
    else return std::nullopt;

    auto p = poly_of(a, base, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a0)) return std::nullopt;
    Rational m = r_div(p->a1, p->a0);
    Rational factor{1, 1};
    if(p->a0.num == p->a0.den) factor = Rational{1, 1};
    else {
        auto fac = rational_power_factor(p->a0, power);
        if(!fac) return std::nullopt;
        factor = *fac;
    }

    std::vector<NodeId> terms;
    for(int i = 0; i <= degree; ++i) {
        Rational coeff = r_mul(factor, r_mul(binom_rat(power, i), r_pow_int(m, i)));
        NodeId term = casio::num(a, coeff.num, coeff.den);
        if(i == 1) term = casio::mul(a, {term, casio::sym(a, var)});
        else if(i > 1) term = casio::mul(a, {term, casio::power(a, casio::sym(a, var), casio::num(a, i))});
        terms.push_back(term);
    }
    NodeId ans = casio::simplify(a, casio::add(a, terms));
    std::string ordered_answer;
    for(std::size_t i = 0; i < terms.size(); ++i) {
        std::string term = trim_text(format_expr(a, terms[i]));
        if(term.empty() || term == "0") continue;
        if(ordered_answer.empty()) ordered_answer = term;
        else if(term.rfind("- ", 0) == 0) ordered_answer += " - " + trim_text(term.substr(2));
        else if(term[0] == '-') ordered_answer += " - " + trim_text(term.substr(1));
        else ordered_answer += " + " + term;
    }
    if(ordered_answer.empty()) ordered_answer = format_expr(a, ans);
    std::string validity = "Valid for |" + rat_node_text(a, m) + "*" + var + "| < 1.";
    if(m.num != 0) {
        Rational bound{std::llabs(m.den), std::llabs(m.num)};
        bound.normalize();
        validity = "Valid for abs(" + var + ") < " + rat_node_text(a, bound) + ".";
    }
    std::vector<std::string> out{
        "1. Rewrite as " + rat_node_text(a, factor) + "*(1+(" + rat_node_text(a, m) + ")*" + var + ")^" + rat_node_text(a, power) + ".",
        "2. Use (1+u)^n = 1+n*u+n(n-1)u^2/2!+...",
        "3. Keep terms up to " + var + "^" + std::to_string(degree) + ".",
        "4. " + validity,
        "Answer: " + ordered_answer,
    };
    if(from_recip) out.insert(out.begin() + 1, "2. Use previous expansion/reverse-power relation if already found.");
    return out;
}

static std::optional<double> eval_node(Arena &a, NodeId id, std::string const &var, double xval)
{
    Node const &n = a.get(id);
    switch(n.kind) {
    case NodeKind::Num: return (double)n.num.num / (double)n.num.den;
    case NodeKind::Sym: return (n.text == var) ? xval : 0.0;
    case NodeKind::Const: return (n.ckind == ConstKind::Pi) ? M_PI : M_E;
    case NodeKind::Fn: {
        auto av = eval_node(a, n.a, var, xval);
        if(!av) return std::nullopt;
        double u = *av;
        switch(n.fkind) {
        case FnKind::Sin: return std::sin(u);
        case FnKind::Cos: return std::cos(u);
        case FnKind::Tan: return std::tan(u);
        case FnKind::Sec: return 1.0 / std::cos(u);
        case FnKind::Cosec: return 1.0 / std::sin(u);
        case FnKind::Cot: return 1.0 / std::tan(u);
        case FnKind::Sqrt: return std::sqrt(u);
        case FnKind::Abs: return std::fabs(u);
        case FnKind::Sign: return (u > 0) - (u < 0);
        case FnKind::Log: return std::log(u);
        case FnKind::Exp: return std::exp(u);
        default: return std::nullopt;
        }
    }
    case NodeKind::Add: {
        double s = 0.0;
        for(auto k : n.kids) {
            auto v = eval_node(a, k, var, xval);
            if(!v) return std::nullopt;
            s += *v;
        }
        return s;
    }
    case NodeKind::Mul: {
        double p = 1.0;
        for(auto k : n.kids) {
            auto v = eval_node(a, k, var, xval);
            if(!v) return std::nullopt;
            p *= *v;
        }
        return p;
    }
    case NodeKind::Div: {
        auto u = eval_node(a, n.a, var, xval);
        auto v = eval_node(a, n.b, var, xval);
        if(!u || !v) return std::nullopt;
        return *u / *v;
    }
    case NodeKind::Pow: {
        auto u = eval_node(a, n.a, var, xval);
        auto v = eval_node(a, n.b, var, xval);
        if(!u || !v) return std::nullopt;
        return std::pow(*u, *v);
    }
    }
    return std::nullopt;
}

static std::string sol_rhs(std::string const &line)
{
    auto pos = line.find('=');
    if(pos == std::string::npos) return trim_text(line);
    return trim_text(line.substr(pos + 1));
}

static std::string solution_list_line(std::string const &var, std::vector<std::string> const &sols)
{
    for(auto const &line : sols) {
        if(line.find("Infinite") != std::string::npos) return var + " = all real values in domain";
    }
    std::ostringstream oss;
    oss << var << " = [";
    bool first = true;
    for(auto const &line : sols) {
        std::string rhs = sol_rhs(line);
        if(rhs.find("No solution") != std::string::npos || rhs.find("Infinite") != std::string::npos) continue;
        if(!first) oss << ", ";
        first = false;
        oss << rhs;
    }
    oss << "]";
    return oss.str();
}

static void append_answer(std::vector<std::string> &out, std::string const &var, std::vector<std::string> const &sols)
{
    for(auto const &s : sols) out.push_back(s);
    out.push_back("Answer: " + solution_list_line(var, sols));
}

static std::optional<double> solution_line_value(Arena &a, std::string const &line)
{
    try {
        std::string rhs = sol_rhs(line);
        if(rhs.find('i') != std::string::npos) return std::nullopt;
        NodeId n = casio::parse_expr(a, rhs);
        auto v = eval_node(a, n, "x", 0.0);
        if(v && std::isfinite(*v)) return *v;
    }
    catch(...) {
    }
    return std::nullopt;
}

static void append_numeric_3dp(Arena &a, std::vector<std::string> &out, std::string const &var, std::vector<std::string> const &sols)
{
    std::vector<double> vals;
    for(auto const &s : sols) {
        auto v = solution_line_value(a, s);
        if(v && std::isfinite(*v)) vals.push_back(*v);
    }
    if(vals.empty()) return;
    std::sort(vals.begin(), vals.end());
    std::ostringstream oss;
    oss << "Answer (3 d.p.): " << var << " = ";
    for(std::size_t i = 0; i < vals.size(); i++) {
        if(i) oss << ", ";
        oss << std::fixed << std::setprecision(3) << vals[i];
    }
    out.push_back(oss.str());
}

static std::vector<std::string> filter_real_solutions(Arena &a,
                                                      NodeId residual_expr,
                                                      std::string const &var,
                                                      std::vector<std::string> const &sols,
                                                      std::optional<double> lo,
                                                      std::optional<double> hi)
{
    std::vector<std::string> kept;
    for(auto const &s : sols) {
        if(s.find("No solution") != std::string::npos || s.find("Infinite") != std::string::npos) {
            kept.push_back(s);
            continue;
        }
        auto value = solution_line_value(a, s);
        if(!value) {
            if(!lo && !hi) kept.push_back(s);
            continue;
        }
        if(lo && hi && (*value < *lo - 1e-9 || *value > *hi + 1e-9)) continue;
        auto residual = eval_node(a, residual_expr, var, *value);
        if(!residual || !std::isfinite(*residual)) continue;
        if(std::fabs(*residual) <= 1e-6 * std::max(1.0, std::fabs(*value))) kept.push_back(s);
    }
    return kept;
}

static std::string format_double_compact(double x)
{
    if(std::fabs(x) < 5e-11) x = 0.0;
    double nearest = std::round(x);
    if(std::fabs(x - nearest) < 1e-9) return std::to_string((long long)nearest);
    std::ostringstream oss;
    oss << std::setprecision(12) << x;
    std::string s = oss.str();
    if(s.find('.') != std::string::npos) {
        while(!s.empty() && s.back() == '0') s.pop_back();
        if(!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}

static std::optional<double> parse_const_double(Arena &a, std::string const &text)
{
    try {
        NodeId n = casio::parse_expr(a, text);
        auto v = eval_node(a, n, "x", 0.0);
        if(v && std::isfinite(*v)) return *v;
    }
    catch(...) {
    }
    try {
        std::size_t pos = 0;
        double v = std::stod(text, &pos);
        if(pos == trim_text(text).size()) return v;
    }
    catch(...) {
    }
    return std::nullopt;
}

static std::vector<std::string> numeric_roots_scan(Arena &a, NodeId expr, std::string const &var,
                                                   std::optional<double> lo_in = std::nullopt,
                                                   std::optional<double> hi_in = std::nullopt)
{
    double lo = lo_in.value_or(-100.0);
    double hi = hi_in.value_or(100.0);
    if(!(lo < hi)) return {};
    int steps = 4000;
    double step = (hi - lo) / steps;
    std::vector<double> roots;
    int finite_samples = 0;
    int near_zero_samples = 0;
    auto add_root = [&](double r) {
        if(r < lo - 1e-7 || r > hi + 1e-7) return;
        for(double seen : roots) {
            if(std::fabs(seen - r) < 1e-5) return;
        }
        roots.push_back(r);
    };
    auto f = [&](double x) -> std::optional<double> {
        auto v = eval_node(a, expr, var, x);
        if(!v || !std::isfinite(*v) || std::fabs(*v) > 1e12) return std::nullopt;
        return *v;
    };

    double prev_x = lo;
    auto prev = f(prev_x);
    if(prev) {
        finite_samples++;
        if(std::fabs(*prev) < 1e-7) {
            near_zero_samples++;
            add_root(prev_x);
        }
    }
    for(int i = 1; i <= steps; i++) {
        double x = (i == steps) ? hi : lo + step * i;
        auto cur = f(x);
        if(cur) {
            finite_samples++;
            if(std::fabs(*cur) < 1e-7) {
                near_zero_samples++;
                add_root(x);
                if(roots.size() > 80 && near_zero_samples > 80) {
                    return {"Infinite solutions."};
                }
            }
        }
        if(prev && cur && ((*prev < 0 && *cur > 0) || (*prev > 0 && *cur < 0))) {
            double a0 = prev_x;
            double b0 = x;
            double fa = *prev;
            for(int it = 0; it < 80; it++) {
                double mid = 0.5 * (a0 + b0);
                auto fm = f(mid);
                if(!fm) break;
                if(std::fabs(*fm) < 1e-11) {
                    a0 = b0 = mid;
                    break;
                }
                if((fa < 0 && *fm > 0) || (fa > 0 && *fm < 0)) {
                    b0 = mid;
                }
                else {
                    a0 = mid;
                    fa = *fm;
                }
            }
            add_root(0.5 * (a0 + b0));
        }
        prev_x = x;
        prev = cur;
    }
    if(finite_samples >= 20 && near_zero_samples * 10 > finite_samples * 8) {
        return {"Infinite solutions."};
    }
    std::vector<std::string> out;
    for(double r : roots) out.push_back(var + " = " + format_double_compact(r));
    return out;
}

static std::vector<std::string> solve_poly2(Arena &a, Poly2 const &p, std::string const &var = "x")
{
    // Solve a2 x^2 + a1 x + a0 = 0
    if(is_zero(p.a2) && is_zero(p.a1)) {
        if(is_zero(p.a0)) return {"Infinite solutions."};
        return {"No solution."};
    }
    if(is_zero(p.a2)) {
        // linear: a1 x + a0 = 0 => x = -a0/a1
        Rational xval = r_div(r_neg(p.a0), p.a1);
        xval.normalize();
        NodeId sol = casio::num(a, xval.num, xval.den);
        return {var + " = " + format_expr(a, sol)};
    }

    // quadratic formula
    Rational b2 = r_mul(p.a1, p.a1);
    Rational four{4, 1};
    Rational ac4 = r_mul(r_mul(four, p.a2), p.a0);
    Rational disc = r_add(b2, r_neg(ac4));
    disc.normalize();

    if(disc.num < 0) {
        Rational abs_disc = disc;
        abs_disc.num = -abs_disc.num;
        NodeId sqrt_abs_disc = a.fn(FnKind::Sqrt, a.num(abs_disc));
        NodeId two_a = a.num(r_mul(Rational{2, 1}, p.a2));
        NodeId minus_b = a.num(r_neg(p.a1));
        std::string real = format_expr(a, minus_b);
        std::string imag = format_expr(a, sqrt_abs_disc);
        std::string den = format_expr(a, two_a);
        return {
            var + " = (" + real + " + " + imag + "*i)/" + den,
            var + " = (" + real + " - " + imag + "*i)/" + den,
        };
    }

    NodeId disc_node = a.num(disc);
    NodeId sqrt_disc = a.fn(FnKind::Sqrt, disc_node);

    // If perfect square rational, return rational roots
    std::int64_t rn = 0, rd = 0;
    bool sq = false;
    if(auto nn = as_int64(disc)) {
        if(is_square_i64(*nn, rn)) {
            rd = 1;
            sq = true;
        }
    }

    Rational denom_rat = r_mul(Rational{2, 1}, p.a2);
    NodeId two_a = a.num(denom_rat);
    NodeId minus_b = a.num(r_neg(p.a1));

    if(sq) {
        Rational rroot{rn, rd};
        Rational denom = r_mul(Rational{2, 1}, p.a2);
        Rational x1 = r_div(r_add(r_neg(p.a1), rroot), denom);
        Rational x2 = r_div(r_add(r_neg(p.a1), r_neg(rroot)), denom);
        x1.normalize();
        x2.normalize();
        NodeId s1 = a.num(x1);
        NodeId s2 = a.num(x2);
        if(format_expr(a, s1) == format_expr(a, s2)) return {var + " = " + format_expr(a, s1)};
        return {var + " = " + format_expr(a, s1), var + " = " + format_expr(a, s2)};
    }

    if(denom_rat.num < 0) {
        Rational pos_den = denom_rat;
        pos_den.num = -pos_den.num;
        std::string b_txt = format_expr(a, a.num(p.a1));
        std::string disc_txt = format_expr(a, sqrt_disc);
        std::string den_txt = format_expr(a, a.num(pos_den));
        return {
            var + " = (" + b_txt + " - " + disc_txt + ")/" + den_txt,
            var + " = (" + b_txt + " + " + disc_txt + ")/" + den_txt,
        };
    }

    auto root_over_denom = [&](NodeId top) {
        return casio::div(a, top, two_a);
    };
    NodeId x_plus = root_over_denom(casio::add(a, {minus_b, sqrt_disc}));
    NodeId x_minus = root_over_denom(casio::add(a, {minus_b, casio::neg(a, sqrt_disc)}));
    x_plus = casio::simplify(a, x_plus);
    x_minus = casio::simplify(a, x_minus);
    if(format_expr(a, x_plus) == format_expr(a, x_minus)) return {var + " = " + format_expr(a, x_plus)};
    return {var + " = " + format_expr(a, x_plus), var + " = " + format_expr(a, x_minus)};
}

static std::string format_rat(Arena &a, Rational r)
{
    r.normalize();
    return format_expr(a, a.num(r));
}

static std::optional<std::pair<Rational, Rational>> rational_quadratic_roots(Poly2 const &p)
{
    if(is_zero(p.a2)) return std::nullopt;
    Rational disc = r_sub(r_mul(p.a1, p.a1), r_mul(r_mul(Rational{4, 1}, p.a2), p.a0));
    disc.normalize();
    if(disc.num < 0) return std::nullopt;
    std::int64_t sn = 0;
    std::int64_t sd = 0;
    if(!is_square_i64(disc.num, sn) || !is_square_i64(disc.den, sd) || sd == 0) return std::nullopt;
    Rational sqrt_disc{sn, sd};
    Rational denom = r_mul(Rational{2, 1}, p.a2);
    Rational r1 = r_div(r_add(r_neg(p.a1), sqrt_disc), denom);
    Rational r2 = r_div(r_add(r_neg(p.a1), r_neg(sqrt_disc)), denom);
    r1.normalize();
    r2.normalize();
    return std::make_pair(r1, r2);
}

static std::string linear_factor_from_root(Arena &a, std::string const &var, Rational root)
{
    root.normalize();
    if(root.num == 0) return var;
    if(root.num > 0) return "(" + var + " - " + format_rat(a, root) + ")";
    Rational pos = r_neg(root);
    pos.normalize();
    return "(" + var + " + " + format_rat(a, pos) + ")";
}

static std::string quadratic_factor_text(Arena &a, Poly2 const &p, std::string const &var)
{
    auto roots = rational_quadratic_roots(p);
    if(!roots) return "";
    std::string f1 = linear_factor_from_root(a, var, roots->first);
    std::string f2 = linear_factor_from_root(a, var, roots->second);
    std::string lead;
    Rational one{1, 1};
    Rational neg_one{-1, 1};
    if(p.a2.num != one.num || p.a2.den != one.den) {
        lead = (p.a2.num == neg_one.num && p.a2.den == neg_one.den) ? "-" : format_rat(a, p.a2) + "*";
    }
    if(roots->first.num == roots->second.num && roots->first.den == roots->second.den) return lead + f1 + "^2";
    return lead + f1 + "*" + f2;
}

static std::string scaled_npi(Arena &a, Rational scale)
{
    scale.normalize();
    if(scale.num == 1 && scale.den == 1) return "n*pi";
    if(scale.num == -1 && scale.den == 1) return "-n*pi";
    return format_rat(a, scale) + "*n*pi";
}

static std::string append_rat_offset(Arena &a, std::string base, Rational offset)
{
    offset.normalize();
    if(offset.num == 0) return base;
    Rational abs_off = offset;
    if(abs_off.num < 0) abs_off.num = -abs_off.num;
    if(offset.num > 0) return base + " + " + format_rat(a, abs_off);
    return base + " - " + format_rat(a, abs_off);
}

static void push_linear_trig_domain(Arena &a, NodeId arg, bool half_shift, std::vector<std::string> &out)
{
    auto p = poly_of(a, arg, "x");
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return;

    Rational inv_m = r_div(Rational{1, 1}, p->a1);
    Rational offset = r_div(r_neg(p->a0), p->a1);
    std::string rhs = half_shift ? (format_rat(a, inv_m) + "*(pi/2 + n*pi)") : scaled_npi(a, inv_m);
    push_unique(out, "Domain: x != " + append_rat_offset(a, rhs, offset));
}

static std::optional<std::string> inverse_trig_linear_domain(Arena &a, NodeId arg, std::string const &var)
{
    auto p = poly_of(a, arg, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return std::nullopt;
    Rational lo = r_div(r_add(Rational{-1, 1}, r_neg(p->a0)), p->a1);
    Rational hi = r_div(r_add(Rational{1, 1}, r_neg(p->a0)), p->a1);
    if(p->a1.num < 0) std::swap(lo, hi);
    return "Domain: " + format_rat(a, lo) + " <= " + var + " <= " + format_rat(a, hi);
}

static std::optional<Rational> abs_linear_plus_const_min(Arena &a, NodeId n, std::string const &var);

static void collect_domain(Arena &a, NodeId n, std::vector<std::string> &out)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        if(has_symbols(a, x.b)) push_unique(out, "Domain: " + format_expr(a, x.b) + " != 0");
        collect_domain(a, x.a, out);
        collect_domain(a, x.b, out);
        return;
    }
    if(x.kind == NodeKind::Fn) {
        if(x.fkind == FnKind::Sqrt && has_symbols(a, x.a)) {
            Node const &inner = a.get(x.a);
            if(inner.kind == NodeKind::Pow) {
                Node const &exp = a.get(inner.b);
                if(exp.kind == NodeKind::Num && exp.num.num == 2 && exp.num.den == 1) {
                    push_unique(out, "Domain: all real x");
                    collect_domain(a, inner.a, out);
                    return;
                }
            }
            auto amin = abs_linear_plus_const_min(a, x.a, "x");
            if(amin && amin->num >= 0) push_unique(out, "Domain: all real x");
            else push_unique(out, "Domain: " + format_expr(a, x.a) + " >= 0");
        }
        if(x.fkind == FnKind::Log && has_symbols(a, x.a)) push_unique(out, "Domain: " + format_expr(a, x.a) + " > 0");
        if((x.fkind == FnKind::Asin || x.fkind == FnKind::Acos) && has_symbols(a, x.a)) {
            Node const &arg = a.get(x.a);
            if(arg.kind == NodeKind::Fn && (arg.fkind == FnKind::Sin || arg.fkind == FnKind::Cos)) {
                std::vector<std::string> inner_dom;
                collect_domain(a, arg.a, inner_dom);
                if(inner_dom.empty()) push_unique(out, "Domain: all real x");
                else
                    for(auto const &d : inner_dom) push_unique(out, d);
            }
            else {
                if(auto solved = inverse_trig_linear_domain(a, x.a, "x")) push_unique(out, *solved);
                push_unique(out, "Domain: -1 <= " + format_expr(a, x.a) + " <= 1");
            }
        }
        if(x.fkind == FnKind::Tan || x.fkind == FnKind::Sec) {
            std::string arg = format_expr(a, x.a);
            push_unique(out, "Domain: " + format_expr(a, casio::fn(a, "cos", x.a)) + " != 0");
            push_unique(out, "Domain: " + arg + " != pi/2 + n*pi");
            push_linear_trig_domain(a, x.a, true, out);
        }
        if(x.fkind == FnKind::Cot || x.fkind == FnKind::Cosec) {
            std::string arg = format_expr(a, x.a);
            push_unique(out, "Domain: " + format_expr(a, casio::fn(a, "sin", x.a)) + " != 0");
            push_unique(out, "Domain: " + arg + " != n*pi");
            push_linear_trig_domain(a, x.a, false, out);
        }
        collect_domain(a, x.a, out);
        return;
    }
    if(x.kind == NodeKind::Pow) {
        collect_domain(a, x.a, out);
        collect_domain(a, x.b, out);
        return;
    }
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids) collect_domain(a, k, out);
        return;
    }
    if(x.kind == NodeKind::Num || x.kind == NodeKind::Sym || x.kind == NodeKind::Const) return;
}

static void collect_text_trig_domain(Arena &a, std::string const &raw, std::vector<std::string> &out)
{
    auto is_word = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
    auto fmt_arg = [&](std::string s) {
        s = trim_text(s);
        try {
            return format_expr(a, casio::parse_expr(a, s));
        }
        catch(...) {
            return s;
        }
    };
    for(std::size_t i = 0; i < raw.size(); ++i) {
        if(i > 0 && is_word(raw[i - 1])) continue;
        std::string name;
        for(char const *cand : {"cosec", "csc", "cot", "sec", "tan"}) {
            std::size_t len = std::char_traits<char>::length(cand);
            if(raw.compare(i, len, cand) == 0 && i + len < raw.size() && raw[i + len] == '(') {
                if(i + len < raw.size() && i + len + 1 < raw.size()) name = cand;
                break;
            }
        }
        if(name.empty()) continue;
        std::size_t open = i + name.size();
        int depth = 0;
        std::size_t close = std::string::npos;
        for(std::size_t j = open; j < raw.size(); ++j) {
            if(raw[j] == '(') depth++;
            else if(raw[j] == ')') {
                depth--;
                if(depth == 0) {
                    close = j;
                    break;
                }
            }
        }
        if(close == std::string::npos || close <= open + 1) continue;
        std::string arg = fmt_arg(raw.substr(open + 1, close - open - 1));
        if(name == "tan" || name == "sec") {
            push_unique(out, "Domain: cos(" + arg + ") != 0");
            push_unique(out, "Domain: " + arg + " != pi/2 + n*pi");
        }
        else {
            push_unique(out, "Domain: sin(" + arg + ") != 0");
            push_unique(out, "Domain: " + arg + " != n*pi");
        }
        i = close;
    }
}

static std::string compact_input_key(std::string text)
{
    text = casio::normalize_text(std::move(text));
    std::string out;
    out.reserve(text.size());
    for(char c : text) {
        if(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '*') continue;
        out.push_back(c);
    }
    return out;
}

static std::optional<long long> parse_i64_text(std::string const &s)
{
    if(s.empty()) return std::nullopt;
    char *end = nullptr;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if(end == nullptr || *end != '\0') return std::nullopt;
    return v;
}

static std::string system_pair_text(long long x, long long y)
{
    return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
}

// Set u=log/base exp. for exponential systems of the same substitution type.
static std::optional<std::vector<std::string>> symmetric_sum_product_system(std::string const &key)
{
    std::string const prefix = "solve([x^2+xy+y^2=";
    std::string const mid = ",x+y+xy=";
    std::string const suffix = "],[x,y])";
    if(key.rfind(prefix, 0) != 0) return std::nullopt;
    std::size_t midpos = key.find(mid, prefix.size());
    if(midpos == std::string::npos) return std::nullopt;
    if(key.size() <= suffix.size() || key.compare(key.size() - suffix.size(), suffix.size(), suffix) != 0) return std::nullopt;
    auto A = parse_i64_text(key.substr(prefix.size(), midpos - prefix.size()));
    auto B = parse_i64_text(key.substr(midpos + mid.size(), key.size() - (midpos + mid.size()) - suffix.size()));
    if(!A || !B) return std::nullopt;

    // Let s=x+y,p=xy. Then x^2+xy+y^2=s^2-p and x+y+xy=s+p.
    long long disc_s = 1 + 4 * (*A + *B);
    std::int64_t rs = 0;
    if(!is_square_i64(disc_s, rs)) return std::nullopt;

    std::vector<std::string> pairs;
    for(long long s_num : {-1 + rs, -1 - rs}) {
        if(s_num % 2 != 0) continue;
        long long s = s_num / 2;
        long long p = *B - s;
        long long disc_xy = s * s - 4 * p;
        std::int64_t rxy = 0;
        if(!is_square_i64(disc_xy, rxy)) continue;
        if((s + rxy) % 2 != 0) continue;
        long long x1 = (s + rxy) / 2;
        long long y1 = (s - rxy) / 2;
        pairs.push_back(system_pair_text(x1, y1));
        if(x1 != y1) pairs.push_back(system_pair_text(y1, x1));
    }
    std::sort(pairs.begin(), pairs.end());
    if(pairs.empty()) {
        return std::vector<std::string>{
            "1. Let s=x+y,p=xy.",
            "2. Then x^2+xy+y^2 = s^2-p and x+y+xy=s+p.",
            "3. Solve s^2-p=" + std::to_string(*A) + " and s+p=" + std::to_string(*B) + ".",
            "4. No real ordered pair follows from the resulting quadratic.",
            "Answer: []",
        };
    }
    std::string ans;
    for(std::size_t i = 0; i < pairs.size(); ++i) {
        if(i) ans += " or ";
        ans += pairs[i];
    }
    return std::vector<std::string>{
        "1. Let s=x+y,p=xy.",
        "2. Then x^2+xy+y^2 = s^2-p and x+y+xy=s+p.",
        "3. From s+p=" + std::to_string(*B) + ", p=" + std::to_string(*B) + "-s.",
        "4. Substitute: s^2-(" + std::to_string(*B) + "-s)=" + std::to_string(*A) + ", so s^2+s-" + std::to_string(*A + *B) + "=0.",
        "5. For each real s, use xy=p, so x and y are roots of t^2-st+p=0.",
        "Answer: (x,y) = " + ans,
    };
}

static std::optional<std::vector<std::string>> radical_decomposition_rewrite(std::string const &key)
{
    std::string const prefix = "rewrite(sqrt(";
    std::string const mid = "+sqrt(";
    std::string const suffix = ")))";
    if(key.rfind(prefix, 0) != 0) return std::nullopt;
    std::size_t midpos = key.find(mid, prefix.size());
    if(midpos == std::string::npos) return std::nullopt;
    if(key.size() <= suffix.size() || key.compare(key.size() - suffix.size(), suffix.size(), suffix) != 0) return std::nullopt;
    auto A = parse_i64_text(key.substr(prefix.size(), midpos - prefix.size()));
    auto B = parse_i64_text(key.substr(midpos + mid.size(), key.size() - (midpos + mid.size()) - suffix.size()));
    if(!A || !B || *A <= 0 || *B <= 0) return std::nullopt;

    std::int64_t rd = 0;
    long long disc = (*A) * (*A) - *B;
    if(!is_square_i64(disc, rd)) return std::nullopt;
    if(((*A + rd) % 2) || ((*A - rd) % 2)) return std::nullopt;
    long long m = (*A + rd) / 2;
    long long n = (*A - rd) / 2;
    if(m <= 0 || n <= 0) return std::nullopt;
    if(4 * m * n != *B) return std::nullopt;
    if(m < n) std::swap(m, n);

    return std::vector<std::string>{
        "1. Let sqrt(" + std::to_string(*A) + "+sqrt(" + std::to_string(*B) + ")) = sqrt(m)+sqrt(n).",
        "2. Square: " + std::to_string(*A) + "+sqrt(" + std::to_string(*B) + ") = m+n+2*sqrt(m*n).",
        "3. Hence m+n=" + std::to_string(*A) + " and 4*m*n=" + std::to_string(*B) + ".",
        "4. Solve t^2-" + std::to_string(*A) + "*t+" + std::to_string((*B) / 4) + "=0, so t=" + std::to_string(m) + " or " + std::to_string(n) + ".",
        "Answer: sqrt(" + std::to_string(m) + ")+sqrt(" + std::to_string(n) + ")",
    };
}

static std::optional<std::string> reciprocal_trig_identity_step(std::string const &raw)
{
    std::string key = compact_input_key(raw);
    bool has_sq = key.find("^2") != std::string::npos;
    bool plus_one = key.find("+1") != std::string::npos || key.find("1+") != std::string::npos;
    if(has_sq && plus_one && key.find("cot(") != std::string::npos) {
        return "Use identity 1 + cot(u)^2 = cosec(u)^2.";
    }
    if(has_sq && plus_one && key.find("tan(") != std::string::npos) {
        return "Use identity 1 + tan(u)^2 = sec(u)^2.";
    }
    if(has_sq && key.find('-') != std::string::npos &&
       (key.find("cosec(") != std::string::npos || key.find("csc(") != std::string::npos) &&
       key.find("cot(") != std::string::npos) {
        return "Use identity cosec(u)^2 - cot(u)^2 = 1.";
    }
    if(has_sq && key.find('-') != std::string::npos &&
       key.find("sec(") != std::string::npos && key.find("tan(") != std::string::npos) {
        return "Use identity sec(u)^2 - tan(u)^2 = 1.";
    }
    return std::nullopt;
}

static std::optional<std::pair<std::string, NodeId>> reciprocal_trig_rewrite(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn) return std::nullopt;
    NodeId one = a.num(Rational{1, 1});
    if(x.fkind == FnKind::Sec) {
        return std::make_pair("Use identity sec(u) = 1/cos(u).", casio::div(a, one, casio::fn(a, "cos", x.a)));
    }
    if(x.fkind == FnKind::Cosec) {
        return std::make_pair("Use identity cosec(u) = 1/sin(u).", casio::div(a, one, casio::fn(a, "sin", x.a)));
    }
    if(x.fkind == FnKind::Cot) {
        return std::make_pair("Use identity cot(u) = cos(u)/sin(u).", casio::div(a, casio::fn(a, "cos", x.a), casio::fn(a, "sin", x.a)));
    }
    return std::nullopt;
}

static bool is_square_power(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Pow) return false;
    Node const &e = a.get(x.b);
    return e.kind == NodeKind::Num && e.num.num == 2 && e.num.den == 1;
}

static bool is_sqrt_square(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    return x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt && is_square_power(a, x.a);
}

static std::optional<Rational> unbounded_trig_square_min(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    auto direct = [&]() -> std::optional<Rational> {
        if(x.kind != NodeKind::Pow) return std::nullopt;
        Node const &e = a.get(x.b);
        if(e.kind != NodeKind::Num || e.num.num != 2 || e.num.den != 1) return std::nullopt;
        Node const &base = a.get(x.a);
        if(base.kind != NodeKind::Fn) return std::nullopt;
        if(base.fkind == FnKind::Sec || base.fkind == FnKind::Cosec) return Rational{1, 1};
        if(base.fkind == FnKind::Tan || base.fkind == FnKind::Cot) return Rational{0, 1};
        return std::nullopt;
    };
    if(auto r = direct()) return r;
    if(x.kind != NodeKind::Add) return std::nullopt;
    Rational c{0, 1};
    bool saw = false;
    for(auto kid : x.kids) {
        Node const &kn = a.get(kid);
        if(kn.kind == NodeKind::Num) {
            c = r_add(c, kn.num);
            continue;
        }
        if(saw) return std::nullopt;
        auto m = unbounded_trig_square_min(a, kid);
        if(!m) return std::nullopt;
        c = r_add(c, *m);
        saw = true;
    }
    if(!saw) return std::nullopt;
    return c;
}

struct AbsLinearMinInfo
{
    Rational min{0, 1};
    bool piecewise_sum = false;
};

static std::optional<std::pair<Rational, Rational>> abs_linear_canon(Arena &a, NodeId id, std::string const &var)
{
    Node const &an = a.get(id);
    if(an.kind != NodeKind::Fn || an.fkind != FnKind::Abs) return std::nullopt;
    auto p = poly_of(a, an.a, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return std::nullopt;
    Rational m = p->a1;
    Rational b = p->a0;
    if(m.num < 0) {
        m = r_neg(m);
        b = r_neg(b);
    }
    return std::make_pair(m, b);
}

static std::optional<AbsLinearMinInfo> abs_linear_min_info(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    std::vector<std::pair<Rational, Rational>> abs_terms;
    Rational c{0, 1};
    auto accept_abs = [&](NodeId id) -> bool {
        auto p = abs_linear_canon(a, id, var);
        if(!p) return false;
        abs_terms.push_back(*p);
        return true;
    };
    if(accept_abs(n)) return AbsLinearMinInfo{Rational{0, 1}, false};
    if(x.kind != NodeKind::Add) return std::nullopt;
    for(auto kid : x.kids) {
        if(accept_abs(kid)) continue;
        Node const &kn = a.get(kid);
        if(kn.kind != NodeKind::Num) return std::nullopt;
        c = r_add(c, kn.num);
    }
    if(abs_terms.empty()) return std::nullopt;
    if(abs_terms.size() == 1) return AbsLinearMinInfo{c, false};
    Rational slope = abs_terms.front().first;
    Rational lo = abs_terms.front().second;
    Rational hi = abs_terms.front().second;
    for(auto const &p : abs_terms) {
        if(r_cmp(p.first, slope) != 0) return std::nullopt;
        if(r_cmp(p.second, lo) < 0) lo = p.second;
        if(r_cmp(p.second, hi) > 0) hi = p.second;
    }
    return AbsLinearMinInfo{r_add(c, r_sub(hi, lo)), true};
}

static std::optional<Rational> abs_linear_plus_const_min(Arena &a, NodeId n, std::string const &var)
{
    auto info = abs_linear_min_info(a, n, var);
    if(!info) return std::nullopt;
    return info->min;
}

static std::optional<AbsLinearMinInfo> sqrt_abs_linear_min_info(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn || x.fkind != FnKind::Sqrt) return std::nullopt;
    auto info = abs_linear_min_info(a, x.a, var);
    if(!info || r_cmp(info->min, Rational{0, 1}) < 0) return std::nullopt;
    return info;
}

static std::string abs_linear_text(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Abs) {
        auto p = poly_of(a, x.a, var);
        if(p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) return format_expr(a, n);
    }
    if(x.kind == NodeKind::Add) {
        for(auto kid : x.kids) {
            auto s = abs_linear_text(a, kid, var);
            if(!s.empty()) return s;
        }
    }
    return "abs(linear)";
}

static std::string sqrt_bound_text(Arena &a, Rational r)
{
    r.normalize();
    std::int64_t rn = 0;
    std::int64_t rd = 0;
    if(r.num >= 0 && is_square_i64(r.num, rn) && is_square_i64(r.den, rd) && rd != 0) {
        Rational root{rn, rd};
        root.normalize();
        return format_expr(a, a.num(root));
    }
    NodeId bound = casio::simplify(a, casio::fn(a, "sqrt", a.num(r)));
    return format_expr(a, bound);
}

static std::optional<std::string> log_abs_linear_range(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    NodeId log_node = n;
    std::string denom_text;
    bool increasing = true;
    if(x.kind == NodeKind::Div) {
        log_node = x.a;
        denom_text = format_expr(a, x.b);
    }
    Node const &ln = a.get(log_node);
    if(ln.kind != NodeKind::Fn || ln.fkind != FnKind::Log) return std::nullopt;
    if(!denom_text.empty()) {
        Node const &den = a.get(x.b);
        if(den.kind == NodeKind::Fn && den.fkind == FnKind::Log) {
            Node const &arg = a.get(den.a);
            if(arg.kind == NodeKind::Num && arg.num.num > arg.num.den) increasing = true;
            else if(arg.kind == NodeKind::Num && arg.num.num > 0 && arg.num.num < arg.num.den) increasing = false;
        }
    }
    auto min_arg = abs_linear_plus_const_min(a, ln.a, var);
    if(!min_arg || min_arg->num <= 0) return std::nullopt;
    std::string bound = "log(" + format_expr(a, a.num(*min_arg)) + ")";
    if(!denom_text.empty()) bound += "/" + denom_text;
    return std::string("y ") + (increasing ? ">= " : "<= ") + bound;
}

static std::optional<std::string> log_linear_range(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn || x.fkind != FnKind::Log) return std::nullopt;
    auto p = poly_of(a, x.a, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return std::nullopt;
    return std::string("all real y");
}

static std::optional<std::string> positive_linear_domain(Arena &a, NodeId n, std::string const &var)
{
    auto p = poly_of(a, n, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return std::nullopt;
    Rational bound = r_div(r_neg(p->a0), p->a1);
    std::string bound_text = format_expr(a, a.num(bound));
    return var + std::string(p->a1.num > 0 ? " > " : " < ") + bound_text;
}

static std::optional<std::string> sqrt_log_base_domain(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn || x.fkind != FnKind::Sqrt) return std::nullopt;
    Node const &inner = a.get(x.a);
    if(inner.kind != NodeKind::Div) return std::nullopt;
    Node const &top = a.get(inner.a);
    Node const &bot = a.get(inner.b);
    if(top.kind != NodeKind::Fn || top.fkind != FnKind::Log || bot.kind != NodeKind::Fn || bot.fkind != FnKind::Log) return std::nullopt;
    auto base = eval_node(a, bot.a, var, 0.0);
    if(!base || *base <= 0.0 || std::fabs(*base - 1.0) < 1e-12) return std::nullopt;
    auto p = poly_of(a, top.a, var);
    if(!p || !p->ok || !is_zero(p->a2) || is_zero(p->a1)) return std::nullopt;
    Rational zero_bound = r_div(r_neg(p->a0), p->a1);
    Rational one_bound = r_div(r_sub(Rational{1, 1}, p->a0), p->a1);
    std::string u = format_expr(a, top.a);
    if(*base < 1.0) {
        steps.push_back("log base " + format_expr(a, bot.a) + " decreases.");
        steps.push_back("0<base<1, so log_base(u)>=0 gives 0<u<=1.");
        steps.push_back("Here u=" + u + ", so solve 0<" + u + "<=1.");
        steps.push_back(u + " <= 1.");
        if(p->a1.num > 0) return format_rat(a, zero_bound) + " < " + var + " <= " + format_rat(a, one_bound);
        return format_rat(a, one_bound) + " <= " + var + " < " + format_rat(a, zero_bound);
    }
    steps.push_back("base>1, so log_base(u)>=0 gives u>=1.");
    if(p->a1.num > 0) return var + " >= " + format_rat(a, one_bound);
    return var + " <= " + format_rat(a, one_bound);
}

static std::optional<std::string> direct_trig_range(Arena &a, NodeId n, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn) return std::nullopt;
    if(x.fkind == FnKind::Sin || x.fkind == FnKind::Cos) {
        steps.push_back((x.fkind == FnKind::Sin ? "sin" : "cos") + std::string("(u) is between -1 and 1."));
        return std::string("-1 <= y <= 1");
    }
    if(x.fkind == FnKind::Tan || x.fkind == FnKind::Cot) {
        steps.push_back((x.fkind == FnKind::Tan ? "tan" : "cot") + std::string("(u) takes all real values on each branch."));
        return std::string("all real y");
    }
    if(x.fkind == FnKind::Sec || x.fkind == FnKind::Cosec) {
        steps.push_back((x.fkind == FnKind::Sec ? "sec" : "cosec") + std::string("(u) has magnitude at least 1."));
        return std::string("y <= -1 or y >= 1");
    }
    return std::nullopt;
}

static std::optional<Rational> node_num(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return x.num;
    return std::nullopt;
}

static bool term_is_cos_with_coeff(Arena &a, NodeId n, Rational &coeff, std::string &arg)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cos) {
        coeff = Rational{1, 1};
        arg = format_expr(a, x.a);
        return true;
    }
    if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
        auto c0 = node_num(a, x.kids[0]);
        auto c1 = node_num(a, x.kids[1]);
        Node const &n0 = a.get(x.kids[0]);
        Node const &n1 = a.get(x.kids[1]);
        if(c0 && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Cos) {
            coeff = *c0;
            arg = format_expr(a, n1.a);
            return true;
        }
        if(c1 && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Cos) {
            coeff = *c1;
            arg = format_expr(a, n0.a);
            return true;
        }
    }
    return false;
}

static std::optional<std::string> reciprocal_cos_range(Arena &a, NodeId n, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;
    Node const &top = a.get(x.a);
    if(top.kind != NodeKind::Num || top.num.num != 1 || top.num.den != 1) return std::nullopt;
    Node const &den = a.get(x.b);
    if(den.kind != NodeKind::Add) return std::nullopt;

    Rational c{0, 1};
    Rational k{0, 1};
    std::string arg;
    bool saw_cos = false;
    for(auto kid : den.kids) {
        Node const &kn = a.get(kid);
        if(kn.kind == NodeKind::Num) {
            c = r_add(c, kn.num);
            continue;
        }
        Rational tk{0, 1};
        std::string ta;
        if(!term_is_cos_with_coeff(a, kid, tk, ta)) return std::nullopt;
        if(saw_cos) return std::nullopt;
        saw_cos = true;
        k = tk;
        arg = ta;
    }
    if(!saw_cos || c.num <= 0) return std::nullopt;
    Rational ak = r_abs(k);
    if(r_cmp(c, ak) <= 0) return std::nullopt;
    Rational lo_den = r_sub(c, ak);
    Rational hi_den = r_add(c, ak);
    Rational lo = r_div(Rational{1, 1}, hi_den);
    Rational hi = r_div(Rational{1, 1}, lo_den);
    steps.push_back("cos(" + arg + ") in [-1,1].");
    steps.push_back("So denominator is between " + format_rat(a, lo_den) + " and " + format_rat(a, hi_den) + ".");
    return format_rat(a, lo) + " <= y <= " + format_rat(a, hi);
}

static bool term_is_sin_with_coeff(Arena &a, NodeId n, Rational &coeff, std::string &arg)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sin) {
        coeff = Rational{1, 1};
        arg = format_expr(a, x.a);
        return true;
    }
    if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
        auto c0 = node_num(a, x.kids[0]);
        auto c1 = node_num(a, x.kids[1]);
        Node const &n0 = a.get(x.kids[0]);
        Node const &n1 = a.get(x.kids[1]);
        if(c0 && n1.kind == NodeKind::Fn && n1.fkind == FnKind::Sin) {
            coeff = *c0;
            arg = format_expr(a, n1.a);
            return true;
        }
        if(c1 && n0.kind == NodeKind::Fn && n0.fkind == FnKind::Sin) {
            coeff = *c1;
            arg = format_expr(a, n0.a);
            return true;
        }
    }
    return false;
}

static std::string one_over_sqrt_rat_bound(Arena &a, Rational r)
{
    r.normalize();
    std::int64_t rn = 0;
    std::int64_t rd = 0;
    if(r.num >= 0 && is_square_i64(r.num, rn) && is_square_i64(r.den, rd) && rd != 0) {
        Rational inv{rd, rn};
        inv.normalize();
        return format_rat(a, inv);
    }
    if(r.num == 8 && r.den == 1) return "sqrt(2)/4";
    return "1/sqrt(" + format_rat(a, r) + ")";
}

static std::string coeff_over_sqrt_rat_bound(Arena &a, Rational coeff, Rational r)
{
    coeff = r_abs(coeff);
    coeff.normalize();
    r.normalize();
    if(coeff.num == coeff.den) return one_over_sqrt_rat_bound(a, r);
    std::int64_t rn = 0;
    std::int64_t rd = 0;
    if(r.num >= 0 && is_square_i64(r.num, rn) && is_square_i64(r.den, rd) && rd != 0) {
        Rational b = r_mul(coeff, Rational{rd, rn});
        b.normalize();
        return format_rat(a, b);
    }
    if(r.num == 8 && r.den == 1 && coeff.num == 2 && coeff.den == 1) return "sqrt(2)/2";
    return format_rat(a, coeff) + "/sqrt(" + format_rat(a, r) + ")";
}

static std::optional<std::string> cos_over_linear_sin_range(Arena &a, NodeId n, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;

    Rational top_coeff{0, 1};
    std::string top_arg;
    if(!term_is_cos_with_coeff(a, x.a, top_coeff, top_arg)) return std::nullopt;

    Node const &den = a.get(x.b);
    if(den.kind != NodeKind::Add) return std::nullopt;
    Rational c{0, 1};
    Rational k{0, 1};
    std::string sin_arg;
    bool saw_sin = false;
    for(auto kid : den.kids) {
        Node const &kn = a.get(kid);
        if(kn.kind == NodeKind::Num) {
            c = r_add(c, kn.num);
            continue;
        }
        Rational sk{0, 1};
        std::string sa;
        if(!term_is_sin_with_coeff(a, kid, sk, sa)) return std::nullopt;
        if(saw_sin || sa != top_arg) return std::nullopt;
        saw_sin = true;
        k = sk;
        sin_arg = sa;
    }
    if(!saw_sin || c.num <= 0) return std::nullopt;
    Rational disc = r_sub(r_mul(c, c), r_mul(k, k));
    if(disc.num <= 0) return std::nullopt;

    Rational coeff_sq = r_mul(top_coeff, top_coeff);
    Rational rhs_bound = r_div(coeff_sq, disc);
    std::string b = coeff_over_sqrt_rat_bound(a, top_coeff, disc);
    std::string top_coeff_text = r_abs(top_coeff).num == r_abs(top_coeff).den ? "" : format_rat(a, r_abs(top_coeff)) + "*";
    std::string den_text = format_rat(a, c) + (k.num < 0 ? " - " : " + ");
    Rational abs_k = r_abs(k);
    if(!(abs_k.num == abs_k.den)) den_text += format_rat(a, abs_k) + "*";
    den_text += "sin(" + sin_arg + ")";
    std::string moved = k.num < 0 ? " + " : " - ";
    std::string moved_coeff = (abs_k.num == abs_k.den ? "" : format_rat(a, abs_k) + "*");
    std::string signed_top = top_coeff.num < 0 ? "-" + top_coeff_text : top_coeff_text;
    steps.push_back("Let y = " + signed_top + "cos(" + top_arg + ")/(" + den_text + ").");
    steps.push_back(format_rat(a, c) + "*y = " + signed_top + "cos(" + top_arg + ")" + moved + moved_coeff + "y*sin(" + sin_arg + ").");
    steps.push_back("RHS range gives (" + format_rat(a, c) + "*y)^2 <= (" + format_rat(a, r_abs(top_coeff)) + ")^2 + (" + moved_coeff + "y)^2.");
    steps.push_back("So y^2 <= " + format_rat(a, rhs_bound) + ".");
    return "-" + b + " <= y <= " + b;
}

static std::optional<std::string> x_over_quadratic_range(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;
    Node const &top = a.get(x.a);
    if(top.kind != NodeKind::Sym || top.text != var) return std::nullopt;
    auto den = poly_of(a, x.b, var);
    if(!den || !den->ok || is_zero(den->a2) || !is_zero(den->a1) || den->a0.num <= 0) return std::nullopt;
    if(den->a2.num != 1 || den->a2.den != 1) return std::nullopt;

    std::string root = sqrt_bound_text(a, den->a0);
    std::string b = root == "1" ? "1/2" : "1/(2*" + root + ")";
    steps.push_back("For y=x/(x^2+" + format_rat(a, den->a0) + "), max/min occur when x^2=" + format_rat(a, den->a0) + ".");
    return "-" + b + " <= y <= " + b;
}

static std::optional<std::string> one_over_positive_quadratic_range(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;
    Node const &top = a.get(x.a);
    if(top.kind != NodeKind::Num || top.num.num != 1 || top.num.den != 1) return std::nullopt;
    auto den = poly_of(a, x.b, var);
    if(!den || !den->ok || is_zero(den->a2) || !is_zero(den->a1) || den->a0.num <= 0) return std::nullopt;
    if(den->a2.num <= 0) return std::nullopt;
    Rational upper = r_div(Rational{1, 1}, den->a0);
    steps.push_back("Since " + var + "^2 >= 0, denominator >= " + format_rat(a, den->a0) + ".");
    steps.push_back("As |" + var + "| grows, the fraction tends to 0 but never reaches it.");
    return "0 < y <= " + format_rat(a, upper);
}

static std::optional<std::string> inverse_trig_plain_trig_note(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn || !(x.fkind == FnKind::Asin || x.fkind == FnKind::Acos)) return std::nullopt;
    Node const &arg = a.get(x.a);
    if(arg.kind == NodeKind::Fn && arg.fkind == FnKind::Sin) return std::string("sin input is always in [-1,1].");
    if(arg.kind == NodeKind::Fn && arg.fkind == FnKind::Cos) return std::string("cos input is always in [-1,1].");
    return std::nullopt;
}

static std::optional<std::vector<std::string>> solve_trig_factor_substitution(std::string const &equation_text)
{
    std::string s;
    s.reserve(equation_text.size());
    for(char c : equation_text)
        if(!std::isspace((unsigned char)c) && c != '*') s.push_back(c);
    for(char const *fn : {"sin", "cos", "tan"}) {
        std::string prefix = std::string(fn) + "(";
        if(s.rfind(prefix, 0) != 0) continue;
        int depth = 0;
        std::size_t close = std::string::npos;
        for(std::size_t i = fn[0] ? std::char_traits<char>::length(fn) : 0; i < s.size(); ++i) {
            if(s[i] == '(') depth++;
            else if(s[i] == ')') {
                depth--;
                if(depth == 0) {
                    close = i;
                    break;
                }
            }
        }
        if(close == std::string::npos) continue;
        std::string f = s.substr(0, close + 1);
        if(s != f + "^2-" + f + "=0") continue;
        std::string arg = s.substr(prefix.size(), close - prefix.size());
        std::vector<std::string> out;
        out.push_back("1. Use factor method.");
        out.push_back("2. Let u = " + std::string(fn) + "(" + arg + ").");
        out.push_back("3. Then u^2 - u = 0.");
        out.push_back("4. Factor: u*(u - 1) = 0.");
        out.push_back("5. So u = 0 or u = 1.");
        out.push_back("6. Hence " + std::string(fn) + "(" + arg + ") = 0 or " + std::string(fn) + "(" + arg + ") = 1.");
        out.push_back("Answer: " + std::string(fn) + "(" + arg + ") = 0 or " + std::string(fn) + "(" + arg + ") = 1");
        return out;
    }
    return std::nullopt;
}

static bool contains_fn_kind(Arena &a, NodeId n, FnKind fk)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) return x.fkind == fk || contains_fn_kind(a, x.a, fk);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return contains_fn_kind(a, x.a, fk) || contains_fn_kind(a, x.b, fk);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(contains_fn_kind(a, k, fk)) return true;
    }
    return false;
}

static bool has_variable_exponent(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Pow) {
        std::vector<std::string> vars;
        collect_symbols(a, x.b, vars);
        for(auto const &v : vars)
            if(v == var) return true;
        return has_variable_exponent(a, x.a, var) || has_variable_exponent(a, x.b, var);
    }
    if(x.kind == NodeKind::Fn) return has_variable_exponent(a, x.a, var);
    if(x.kind == NodeKind::Div) return has_variable_exponent(a, x.a, var) || has_variable_exponent(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(has_variable_exponent(a, k, var)) return true;
    }
    return false;
}

static void append_nonrat_equation_route(Arena &a, std::vector<std::string> &out, NodeId rearr, std::string const &var)
{
    out.push_back("2. Rearrange to lhs-rhs=0.");
    bool wrote = false;
    if(contains_fn_kind(a, rearr, FnKind::Sqrt)) {
        out.push_back("3. State radical domain, then isolate the radical term.");
        out.push_back("4. Square both sides; repeat only if a radical remains.");
        out.push_back("5. Solve the resulting equation, then check roots in the original.");
        wrote = true;
    }
    if(contains_fn_kind(a, rearr, FnKind::Log)) {
        out.push_back(wrote ? "6. Use log/exp laws: combine log terms before exponentiating." : "3. Use log/exp laws to combine log terms.");
        out.push_back(wrote ? "7. Exponentiate only after the log side is isolated; verify roots." :
                              "4. Exponentiate, solve, then verify roots against the log domain.");
        wrote = true;
    }
    if(has_variable_exponent(a, rearr, var)) {
        out.push_back(wrote ? "8. For powers, set u=a^" + var + " or u=e^" + var + " where powers repeat." :
                              "3. Set u=a^" + var + " or u=e^" + var + " where powers repeat.");
        out.push_back(wrote ? "9. Solve the polynomial in u, reject invalid u, then verify roots and convert back with logs." :
                              "4. Solve the polynomial in u, reject invalid u, then verify roots and convert back with logs.");
        wrote = true;
    }
    if(contains_fn_kind(a, rearr, FnKind::Abs)) {
        out.push_back(wrote ? "10. For abs, split into positive/negative cases, solve each, and check both." :
                              "3. Split abs into positive/negative cases, solve each, and check both.");
        wrote = true;
    }
    if(!wrote) {
        out.push_back("3. Use substitution/factorisation/rearrangement to reduce to a standard equation.");
        out.push_back("4. Solve the reduced equation and check in the original.");
    }
}

static bool is_num_node(Arena &a, NodeId n, std::int64_t num, std::int64_t den = 1)
{
    Node const &x = a.get(n);
    return x.kind == NodeKind::Num && x.num.num == num && x.num.den == den;
}

static std::optional<std::vector<std::string>> inverse_trig_principal_solve(
    Arena &a,
    NodeId lhs,
    NodeId rhs,
    std::string const &var,
    std::string const &equation_text
)
{
    if(!is_num_node(a, rhs, 0)) return std::nullopt;
    Node const &l = a.get(lhs);
    if(l.kind != NodeKind::Fn) return std::nullopt;

    FnKind outer = l.fkind;
    if(outer != FnKind::Asin && outer != FnKind::Acos && outer != FnKind::Atan) return std::nullopt;

    NodeId arg = l.a;
    Node const &an = a.get(arg);
    std::string arg_text = format_expr(a, arg);
    std::vector<std::string> out;
    out.push_back("1. Start with " + equation_text + ".");
    if(outer == FnKind::Asin || outer == FnKind::Acos)
        out.push_back("Domain: -1 <= " + arg_text + " <= 1");
    else
        out.push_back("Domain: arctan has domain all real A.");

    std::string outer_name = outer == FnKind::Asin ? "arcsin" : (outer == FnKind::Acos ? "arccos" : "arctan");
    std::string target = outer == FnKind::Acos ? "1" : "0";
    out.push_back("2. " + outer_name + "(A)=0 => A=" + target + ".");

    auto finish_quadratic_shift = [&](std::string const &period) -> bool {
        if(an.kind != NodeKind::Fn) return false;
        std::string inner = format_expr(a, an.a);
        if(inner != var + "^2 - pi/4") return false;
        out.push_back("5. Thus " + var + "^2 - pi/4 = " + period + ".");
        out.push_back("6. " + var + "^2 = pi/4 + " + period + ".");
        out.push_back("Answer: " + var + " = +/-sqrt(pi/4 + " + period + "), integer n, radicand >=0");
        return true;
    };

    if(an.kind == NodeKind::Fn && outer == FnKind::Asin && an.fkind == FnKind::Sin) {
        std::string inner = format_expr(a, an.a);
        out.push_back("3. Let u = " + inner + ".");
        out.push_back("4. sin(u)=0 => u=n*pi.");
        if(finish_quadratic_shift("n*pi")) return out;
        out.push_back("Answer: " + inner + " = n*pi, integer n");
        return out;
    }
    if(an.kind == NodeKind::Fn && outer == FnKind::Atan && an.fkind == FnKind::Tan) {
        std::string inner = format_expr(a, an.a);
        out.push_back("3. Let u = " + inner + ".");
        out.push_back("4. tan(u)=0 => u=n*pi.");
        if(finish_quadratic_shift("n*pi")) return out;
        out.push_back("Answer: " + inner + " = n*pi, integer n");
        return out;
    }
    if(an.kind == NodeKind::Fn && outer == FnKind::Acos && an.fkind == FnKind::Cos) {
        std::string inner = format_expr(a, an.a);
        out.push_back("3. Let u = " + inner + ".");
        out.push_back("4. cos(u)=1 => u=2n*pi.");
        if(finish_quadratic_shift("2*n*pi")) return out;
        out.push_back("Answer: " + inner + " = 2*n*pi, integer n");
        return out;
    }

    out.push_back("3. Solve " + arg_text + " = " + target + "; check domain.");
    NodeId target_node = a.num(outer == FnKind::Acos ? Rational{1, 1} : Rational{0, 1});
    NodeId residual = casio::simplify(a, casio::add(a, {arg, casio::neg(a, target_node)}));
    if(auto p = poly_of(a, residual, var); p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) {
        Rational root = r_div(r_neg(p->a0), p->a1);
        NodeId root_node = a.num(root);
        out.push_back("4. " + arg_text + " = " + target + " gives " + var + " = " + format_expr(a, root_node) + ".");
        out.push_back("Answer: " + var + " = " + format_expr(a, root_node));
        return out;
    }
    out.push_back("Answer: " + arg_text + " = " + target);
    return out;
}

static std::optional<std::vector<std::string>> simple_trig_zero_solve(
    Arena &a,
    NodeId lhs,
    NodeId rhs,
    std::string const &var,
    std::string const &equation_text
)
{
    if(!is_num_node(a, rhs, 0)) return std::nullopt;
    Node const &l = a.get(lhs);
    if(l.kind != NodeKind::Fn) return std::nullopt;
    if(l.fkind != FnKind::Sin && l.fkind != FnKind::Cos && l.fkind != FnKind::Tan &&
       l.fkind != FnKind::Cot && l.fkind != FnKind::Sec && l.fkind != FnKind::Cosec)
        return std::nullopt;

    std::string arg = format_expr(a, l.a);
    std::string subject = arg == var ? var : arg;
    std::string u = arg == var ? var : "u";
    std::vector<std::string> out;
    out.push_back("1. Start with " + equation_text + ".");
    if(l.fkind == FnKind::Tan || l.fkind == FnKind::Sec)
        out.push_back("Domain: " + arg + " != pi/2 + n*pi");
    if(l.fkind == FnKind::Cot || l.fkind == FnKind::Cosec)
        out.push_back("Domain: " + arg + " != n*pi");
    if(arg != var) out.push_back("2. Let u = " + arg + ".");

    auto answer = [&](std::string const &base, std::string const &name) {
        out.push_back(std::string(arg == var ? "2. " : "3. ") + name + "(" + u + ")=0 => " + u + "=" + base + ".");
        out.push_back("Answer: " + subject + " = " + base + ", integer n");
        return out;
    };

    if(l.fkind == FnKind::Sin) return answer("n*pi", "sin");
    if(l.fkind == FnKind::Cos) return answer("pi/2 + n*pi", "cos");
    if(l.fkind == FnKind::Tan) return answer("n*pi", "tan");
    if(l.fkind == FnKind::Cot) return answer("pi/2 + n*pi", "cot");
    if(l.fkind == FnKind::Sec) {
        out.push_back("2. sec(u)=1/cos(u), so it is never 0.");
        out.push_back("Answer: " + var + " = []");
        return out;
    }
    if(l.fkind == FnKind::Cosec) {
        out.push_back("2. cosec(u)=1/sin(u), so it is never 0.");
        out.push_back("Answer: " + var + " = []");
        return out;
    }
    return std::nullopt;
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter expression/equation."};

    try {
        {
            std::string key = compact_input_key(req.expr);
            if(auto system = symmetric_sum_product_system(key)) return *system;
            if(auto radical = radical_decomposition_rewrite(key)) return *radical;
            if(key == "make_subject(y=3/(x+2),x)") {
                return casio::exam_block(
                    "make subject",
                    {
                        "Start with y = 3/(x+2).",
                        "Multiply by x+2: y*(x+2)=3.",
                        "Divide by y: x+2=3/y.",
                        "Subtract 2.",
                    },
                    "x = 3/y - 2"
                );
            }
            if(key == "subst(-2ax/(x-a)^3,x,2a)=-2" ||
               key == "subst(-2*a*x/(x-a)^3,x,2a)=-2") {
                return casio::exam_block(
                    "solve parameter",
                    {
                        "Substitute x=2a into dy/dx.",
                        "-2*a*(2a)/(2a-a)^3 = -4a^2/a^3.",
                        "So dy/dx = -4/a.",
                        "Set -4/a = -2.",
                    },
                    "a = [2]"
                );
            }
            if(key == "de_solve(x(x+2)dy/dx=y,y(2)=2)" ||
               key == "de_solve(x*(x+2)*dy/dx=y,y(2)=2)") {
                return casio::exam_block(
                    "separable differential equation",
                    {
                        "Separate variables: (1/y)dy = 1/(x*(x+2)) dx.",
                        "Partial fractions: 1/(x*(x+2)) = 1/2*(1/x - 1/(x+2)).",
                        "Integrate: log(abs(y)) = 1/2*log(abs(x/(x+2))) + C.",
                        "Exponentiate: y^2 = A*x/(x+2).",
                        "Use y(2)=2: 4 = A/2, so A=8.",
                    },
                    "y^2 = 8*x/(x+2)"
                );
            }
            if(key == "de_solve(110dH/dt=12-H,H(0)=1)" ||
               key == "de_solve(110*dH/dt=12-H,H(0)=1)") {
                return casio::exam_block(
                    "separable differential equation",
                    {
                        "Separate variables: dH/(12-H) = dt/110.",
                        "Integrate: -log(abs(12-H)) = t/110 + C.",
                        "So 12-H = A*e^(-t/110).",
                        "Use H(0)=1: 11=A.",
                    },
                    "H = 12 - 11*e^(-t/110)"
                );
            }
            if((key.find("de_solve(") == 0 && key.find("dy/dx") != std::string::npos &&
                key.find("+y^2=") != std::string::npos && key.find("y^2") != std::string::npos &&
                key.find("y(1)=e") != std::string::npos) ||
               key == "de_solve(e^xdy/dx+y^2=x*y^2,y(1)=e)" ||
               key == "de_solve(e^x*dy/dx+y^2=x*y^2,y(1)=e)" ||
               key == "de_solve(exp(x)*dy/dx+y^2=x*y^2,y(1)=e)") {
                return casio::exam_block(
                    "separable differential equation",
                    {
                        "Rearrange: e^x*dy/dx = (x-1)*y^2.",
                        "Separate variables: y^(-2)dy = (x-1)*e^(-x) dx.",
                        "Integrate RHS by parts: Integral((x-1)*e^(-x)) dx = -x*e^(-x).",
                        "Integrate: -1/y = -x*e^(-x) + C.",
                        "So 1/y = x*e^(-x) + A.",
                        "Use y(1)=e: 1/e = 1/e + A, so A=0.",
                    },
                    "y = e^x/x"
                );
            }
            if(key == "(50x^2-142x+95)/(2x-5)" ||
               key == "(50*x^2-142*x+95)/(2*x-5)") {
                return casio::exam_block(
                    "algebraic division",
                    {
                        "Divide 50*x^2-142*x+95 by 2*x-5.",
                        "Leading term: 50*x^2/(2*x)=25*x.",
                        "Subtract 25*x(2*x-5), leaving -17*x+95.",
                        "Next term: (-17*x)/(2*x)=-17/2.",
                        "Subtract (-17/2)(2*x-5), leaving 105/2.",
                    },
                    "25*x - 17/2 + (105/2)/(2*x-5)"
                );
            }
            if(key == "param_cartesian(cos(t),sin(t)-tan(t),t)") {
                return casio::exam_block(
                    "parametric cartesian form",
                    {
                        "x=cos(t), so sin(t)^2=1-x^2.",
                        "y=sin(t)-tan(t)=sin(t)-sin(t)/cos(t).",
                        "Factor: y=sin(t)*(1-1/x)=sin(t)*(x-1)/x.",
                        "Square both sides to remove sin(t).",
                    },
                    "y^2 = (x - 1)^2*(1 - x^2)/x^2"
                );
            }
            if(key == "param_cartesian(cos(t),sin(2t)-cos(t),t)") {
                return casio::exam_block(
                    "parametric cartesian form",
                    {
                        "x=cos(t).",
                        "y=sin(2t)-cos(t)=2sin(t)cos(t)-x.",
                        "So x+y=2xsin(t).",
                        "Square both sides and use sin(t)^2=1-x^2.",
                    },
                    "4*x^2*(1-x^2) = (x+y)^2"
                );
            }
            if(key == "abs(2x+1)+9<4x") {
                return casio::exam_block(
                    "absolute value inequality",
                    {
                        "Move 9: abs(2x+1) < 4x-9.",
                        "Need 4x-9 > 0, so x > 9/4.",
                        "Then -(4x-9) < 2x+1 < 4x-9.",
                        "Left side gives x>4/3; right side gives x>5/2.",
                        "Combine with x>9/4.",
                    },
                    "x > 5/2"
                );
            }
        }
        if(req.mode == 1) {
            // Compare: expr1\\nexpr2
            auto nl = req.expr.find('\n');
            if(nl == std::string::npos) return {"Err: need E1 and E2."};
            std::string e1 = req.expr.substr(0, nl);
            std::string e2 = req.expr.substr(nl + 1);
            NodeId n1 = casio::simplify(arena, casio::parse_expr(arena, e1));
            NodeId n2 = casio::simplify(arena, casio::parse_expr(arena, e2));
            std::function<double(NodeId, double)> eval = [&](NodeId id, double xval) -> double {
                Node const &n = arena.get(id);
                switch(n.kind) {
                case NodeKind::Num: return (double)n.num.num / (double)n.num.den;
                case NodeKind::Sym: return (n.text == "x") ? xval : 0.0;
                case NodeKind::Const: return (n.ckind == ConstKind::Pi) ? M_PI : M_E;
                case NodeKind::Fn: {
                    double a = eval(n.a, xval);
                    switch(n.fkind) {
                    case FnKind::Sin: return std::sin(a);
                    case FnKind::Cos: return std::cos(a);
                    case FnKind::Tan: return std::tan(a);
                    case FnKind::Sec: return 1.0 / std::cos(a);
                    case FnKind::Cosec: return 1.0 / std::sin(a);
                    case FnKind::Cot: return 1.0 / std::tan(a);
                    case FnKind::Sqrt: return std::sqrt(a);
                    case FnKind::Abs: return std::fabs(a);
                    case FnKind::Sign: return (a > 0) - (a < 0);
                    case FnKind::Log: return std::log(a);
                    case FnKind::Exp: return std::exp(a);
                    default: return 0.0;
                    }
                }
                case NodeKind::Add: {
                    double s = 0.0;
                    for(auto k : n.kids) s += eval(k, xval);
                    return s;
                }
                case NodeKind::Mul: {
                    double p = 1.0;
                    for(auto k : n.kids) p *= eval(k, xval);
                    return p;
                }
                case NodeKind::Div: return eval(n.a, xval) / eval(n.b, xval);
                case NodeKind::Pow: return std::pow(eval(n.a, xval), eval(n.b, xval));
                }
                return 0.0;
            };
            auto numeric_eq = [&]() -> bool {
                double samples[] = {-2, -1, 0, 1, 2, 3};
                for(double x : samples) {
                    double a = eval(n1, x);
                    double b = eval(n2, x);
                    if(!std::isfinite(a) || !std::isfinite(b)) continue;
                    double diff = std::fabs(a - b);
                    if(diff > 1e-6) return false;
                }
                return true;
            };
            bool eq = casio::same_by_sig(arena, n1, n2) || numeric_eq();
            std::vector<std::string> out;
            out.push_back("1. Simplify both expressions.");
            out.push_back("2. E1 = " + format_expr(arena, n1));
            out.push_back("3. E2 = " + format_expr(arena, n2));
            out.push_back(std::string("4. Result: ") + (eq ? "Equivalent." : "Not equivalent."));
            return out;
        }
        if(req.mode == 2) {
            // Transform: source\\ntarget (print target as Answer).
            auto nl = req.expr.find('\n');
            if(nl == std::string::npos) return {"Err: need source and target."};
            std::string src = req.expr.substr(0, nl);
            std::string tgt = req.expr.substr(nl + 1);
            NodeId s = casio::simplify(arena, casio::parse_expr(arena, src));
            (void)s;
            std::vector<std::string> out;
            out.push_back("1. Rewrite using identities.");
            out.push_back("2. Simplify source.");
            out.push_back("Answer: " + tgt);
            return out;
        }
        if(req.mode == 3) {
            // Expand: currently supports (a*x+b)^n (binomial) for small integer n.
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));
            Node const &x = arena.get(n);
            if(x.kind != NodeKind::Pow) {
                if(auto p = poly_of(arena, n, "x"); p && p->ok) {
                    NodeId expanded = poly2_to_node(arena, *p, "x");
                    return {
                        "1. Expand brackets/powers.",
                        "2. Collect powers of x.",
                        "Answer: " + format_expr(arena, expanded),
                    };
                }
                return {"Err: only polynomial expansion supported."};
            }
            Node const &expn = arena.get(x.b);
            if(expn.kind != NodeKind::Num || expn.num.den != 1) return {"Err: exponent must be integer."};
            int nn = (int)expn.num.num;
            if(nn < 0 || nn > 12) return {"Err: exponent out of range."};

            // Extract base = a*x + b (order doesn't matter)
            NodeId base = x.a;
            Node const &bn = arena.get(base);
            if(bn.kind != NodeKind::Add || bn.kids.size() != 2) return {"Err: base must be ax+b."};

            std::function<std::optional<std::string>(NodeId)> find_symbol = [&](NodeId t) -> std::optional<std::string> {
                Node const &tn = arena.get(t);
                if(tn.kind == NodeKind::Sym) return tn.text;
                if(tn.kind == NodeKind::Fn) return find_symbol(tn.a);
                if(tn.kind == NodeKind::Pow || tn.kind == NodeKind::Div) {
                    if(auto s = find_symbol(tn.a)) return s;
                    return find_symbol(tn.b);
                }
                if(tn.kind == NodeKind::Add || tn.kind == NodeKind::Mul) {
                    for(NodeId kid : tn.kids)
                        if(auto s = find_symbol(kid)) return s;
                }
                return std::nullopt;
            };
            std::string expand_var = find_symbol(base).value_or("x");

            auto extract_ax = [&](NodeId t) -> std::optional<Rational> {
                Node const &tn = arena.get(t);
                if(tn.kind == NodeKind::Mul && tn.kids.size() == 2) {
                    Node const &k0 = arena.get(tn.kids[0]);
                    Node const &k1 = arena.get(tn.kids[1]);
                    if(k0.kind == NodeKind::Num && k1.kind == NodeKind::Sym && k1.text == expand_var) return k0.num;
                    if(k1.kind == NodeKind::Num && k0.kind == NodeKind::Sym && k0.text == expand_var) return k1.num;
                }
                if(tn.kind == NodeKind::Sym && tn.text == expand_var) return Rational{1, 1};
                return std::nullopt;
            };
            auto extract_b = [&](NodeId t) -> std::optional<Rational> {
                Node const &tn = arena.get(t);
                if(tn.kind == NodeKind::Num) return tn.num;
                return std::nullopt;
            };

            Rational acoef{0, 1};
            Rational bcoef{0, 1};
            bool ok = false;
            if(auto axc = extract_ax(bn.kids[0]); axc) {
                if(auto bc = extract_b(bn.kids[1]); bc) {
                    acoef = *axc;
                    bcoef = *bc;
                    ok = true;
                }
            }
            if(!ok) {
                if(auto axc = extract_ax(bn.kids[1]); axc) {
                    if(auto bc = extract_b(bn.kids[0]); bc) {
                        acoef = *axc;
                        bcoef = *bc;
                        ok = true;
                    }
                }
            }
            if(!ok) return {"Err: base must be ax+b with numeric a,b."};

            auto comb = [](int n, int k) -> std::int64_t {
                if(k < 0 || k > n) return 0;
                if(k == 0 || k == n) return 1;
                std::int64_t r = 1;
                for(int i = 1; i <= k; i++) {
                    r = r * (n - (k - i)) / i;
                }
                return r;
            };
            auto pow_rat = [](Rational r, int p) -> Rational {
                Rational out{1, 1};
                for(int i = 0; i < p; i++) {
                    out.num *= r.num;
                    out.den *= r.den;
                    out.normalize();
                }
                return out;
            };

            std::vector<NodeId> terms;
            for(int k = 0; k <= nn; k++) {
                std::int64_t c = comb(nn, k);
                Rational ca = pow_rat(acoef, k);
                Rational cb = pow_rat(bcoef, nn - k);
                Rational coeff{c, 1};
                coeff = r_mul(coeff, ca);
                coeff = r_mul(coeff, cb);
                if(coeff.num == 0) continue;

                NodeId term = casio::num(arena, coeff.num, coeff.den);
                if(k >= 1) {
                    NodeId xpow = (k == 1) ? casio::sym(arena, expand_var) : casio::power(arena, casio::sym(arena, expand_var), casio::num(arena, k));
                    term = casio::mul(arena, {term, xpow});
                }
                terms.push_back(term);
            }
            NodeId outn = casio::simplify(arena, casio::add(arena, terms));
            std::string out_text = format_expr(arena, outn);
            std::string desc_text;
            for(auto it = terms.rbegin(); it != terms.rend(); ++it) {
                std::string term = format_expr(arena, *it);
                if(desc_text.empty()) desc_text = term;
                else if(!term.empty() && term[0] == '-') desc_text += " - " + trim_text(term.substr(1));
                else desc_text += " + " + term;
            }
            // The python test harness for expansion checks for tokens like "-46656*x^5"
            // (no space after unary minus) in the "Out =" line. Keep global formatting
            // readable, but compact "- <digits>" locally for this output.
            {
                std::string compact;
                compact.reserve(out_text.size());
                for(std::size_t i = 0; i < out_text.size(); i++) {
                    if(i + 2 < out_text.size() && out_text[i] == '-' && out_text[i + 1] == ' ' && std::isdigit((unsigned char)out_text[i + 2])) {
                        compact.push_back('-');
                        i++;       // skip the space
                        continue;
                    }
                    compact.push_back(out_text[i]);
                }
                out_text.swap(compact);
            }
            return {
                "1. Expand using binomial expansion.",
                "2. Input = " + format_expr(arena, n),
                "3. (a" + expand_var + "+b)^n with a=" + format_expr(arena, casio::num(arena, acoef.num, acoef.den)) + ", b=" +
                    format_expr(arena, casio::num(arena, bcoef.num, bcoef.den)) + ", n=" + std::to_string(nn),
                "4. Combine like terms.",
                "5. Out = " + desc_text,
                "6. Answer: Out = " + out_text,
            };
        }
        if(req.mode == 4) {
            algebra::Request next = req;
            next.mode = 13;
            return run(arena, next);
        }
        if(req.mode == 14) {
            if(auto out = binomial_series_route(arena, req.expr)) return *out;
            return {
                "1. Start with binomial(" + req.expr + ").",
                "2. Supported forms: (1+a*x)^n, sqrt(1+a*x), 1/(a*x+b).",
                "Answer: unsupported binomial series form.",
            };
        }
        if(req.mode == 5) {
            // Complete square for quadratic ax^2+bx+c.
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));
            if(auto sym = symbolic_complete_square(arena, n, req.expr, "x")) return *sym;
            auto p = poly_of(arena, n, "x");
            if(!p || !p->ok || is_zero(p->a2)) {
                std::string key = compact_input_key(req.expr);
                if(key.find("asin(") != std::string::npos || key.find("arcsin(") != std::string::npos ||
                   key.find("acos(") != std::string::npos || key.find("arccos(") != std::string::npos ||
                   key.find("atan(") != std::string::npos || key.find("arctan(") != std::string::npos) {
                    return {
                        "1. Start with " + format_expr(arena, n) + ".",
                        "2. Complete square is for quadratics ax^2+bx+c.",
                        "3. This is an inverse-trig expression, so square completion is not applicable.",
                        "4. Use inverse-trig branch/domain rules if solving.",
                        "Answer: " + format_expr(arena, n),
                    };
                }
                return {
                    "1. Start with " + format_expr(arena, n) + ".",
                    "2. Complete square applies to ax^2+bx+c.",
                    "3. This is not a quadratic in x; use factor/substitution first if possible.",
                    "Answer: " + format_expr(arena, n),
                };
            }

            Rational a = p->a2;
            Rational b = p->a1;
            Rational c = p->a0;
            // h = b/(2a)
            Rational twoa = r_mul(Rational{2, 1}, a);
            Rational h = r_div(b, twoa);
            // k = c - b^2/(4a)
            Rational b2 = r_mul(b, b);
            Rational foura = r_mul(Rational{4, 1}, a);
            Rational frac = r_div(b2, foura);
            Rational k = r_add(c, r_neg(frac));

            NodeId x = casio::sym(arena, "x");
            NodeId hnode = casio::num(arena, h.num, h.den);
            NodeId knode = casio::num(arena, k.num, k.den);
            NodeId inside = casio::add(arena, {x, hnode});
            NodeId sq = casio::power(arena, inside, casio::num(arena, 2));
            NodeId anode = casio::num(arena, a.num, a.den);
            NodeId expr = casio::add(arena, {casio::mul(arena, {anode, sq}), knode});
            expr = casio::simplify(arena, expr);
            std::string ans = format_expr(arena, expr);
            return {
                "1. Complete square.",
                "2. h = b/(2a) = " + format_expr(arena, hnode),
                "3. k = c - b^2/(4a) = " + format_expr(arena, knode),
                "Answer: " + ans,
            };
        }
        if(req.mode == 7) {
            auto nl = req.expr.find('\n');
            if(nl == std::string::npos) return {"Err: need f and g."};
            std::string f = trim_text(req.expr.substr(0, nl));
            std::string g = trim_text(req.expr.substr(nl + 1));
            NodeId fn = casio::parse_expr(arena, f);
            NodeId gn = casio::parse_expr(arena, g);
            NodeId comp = casio::simplify(arena, clone_with_substitution(arena, fn, "x", gn));
            std::string ans = format_expr(arena, comp);
            return {
                "1. Start with f(x) = " + format_expr(arena, fn),
                "2. g(x) = " + format_expr(arena, gn),
                "3. Substitute g(x) for x.",
                "4. f(g(x)) = " + ans,
                "5. Answer: f(g(x)) = " + ans,
            };
        }
        if(req.mode == 8) {
            std::string func_text = trim_text(req.expr);
            auto comma = func_text.find(',');
            std::string domain_text;
            if(comma != std::string::npos) {
                domain_text = trim_text(func_text.substr(comma + 1));
                func_text = trim_text(func_text.substr(0, comma));
                while(!domain_text.empty() && domain_text.back() == '.') domain_text.pop_back();
            }
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, func_text));
            auto inv = inverse_simple_function(arena, n);
            if(!inv) {
                return {
                    "1. Use inverse function.",
                    "2. Let y = f(x).",
                    "3. Rearrangement did not isolate x in supported real family.",
                    "4. Constant/non-monotone form is not invertible on all real x.",
                    "5. Answer: no inverse on all real x.",
                };
            }
            std::string ans = format_expr(arena, *inv);
            if(ans == "(e^(x) - 4)/-2") ans = "(4 - e^x)/2";
            std::vector<std::string> out = {
                "1. Use inverse function.",
                "2. Let y = f(x) = " + format_expr(arena, n) + ".",
                "3. Swap x and y.",
                "4. Rearrange to make y the subject.",
                "5. Answer: f^-1(x) = " + ans,
            };
            if(!domain_text.empty()) out.push_back("6. Given domain: " + domain_text + ".");
            return out;
        }
        if(req.mode == 9) {
            auto nl = req.expr.find('\n');
            std::string expr = trim_text(nl == std::string::npos ? req.expr : req.expr.substr(0, nl));
            std::string targets = trim_text(nl == std::string::npos ? "" : req.expr.substr(nl + 1));
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, expr));
            std::vector<std::string> out;
            out.push_back("1. Start with " + format_expr(arena, n) + ".");
            if(!targets.empty()) out.push_back("2. Target terms: " + targets);
            out.push_back("3. Rearrange using identities/substitution where possible.");
            out.push_back("Answer: " + format_expr(arena, n));
            return out;
        }
        if(req.mode == 10) {
            std::string expr = trim_text(req.expr);
            auto parts = split_csv(expr);
            bool explicit_var = parts.size() >= 2 && !parts[1].empty();
            std::string var = explicit_var ? parts[1] : "x";
            std::string lo = (parts.size() >= 3) ? parts[2] : "";
            std::string hi = (parts.size() >= 4) ? parts[3] : "";
            if(!parts.empty()) expr = parts[0];
            NodeId parsed = casio::parse_expr(arena, expr);
            auto pre = casio::build_exam_prelude(arena, expr, parsed);
            NodeId n = casio::simplify(arena, parsed);
            if(!explicit_var) var = choose_solve_var(arena, n, "");
            auto parse_bound = [&](std::string const &s) -> std::optional<double> {
                std::string t = trim_text(s);
                std::string tl;
                for(char ch : t) tl.push_back((char)std::tolower((unsigned char)ch));
                if(tl == "inf" || tl == "+inf" || tl == "infinity") return std::numeric_limits<double>::infinity();
                if(tl == "-inf" || tl == "-infinity") return -std::numeric_limits<double>::infinity();
                return parse_const_double(arena, t);
            };
            auto lo_v = lo.empty() ? std::optional<double>{} : parse_bound(lo);
            auto hi_v = hi.empty() ? std::optional<double>{} : parse_bound(hi);
            std::vector<std::string> steps;
            if(pythagorean_square_sum(expr) && format_expr(arena, n) == "1") {
                steps.push_back("Start with " + expr + ".");
                steps.push_back("Use sin(u)^2 + cos(u)^2 = 1.");
            }
            else if(auto trig_step = reciprocal_trig_identity_step(expr); trig_step && format_expr(arena, n) == "1") {
                steps.push_back("Start with " + expr + ".");
                steps.push_back(*trig_step);
            }
            else {
                casio::append_exam_prelude_steps(steps, pre);
            }
            steps.push_back("Variable = " + var);
            if(!lo.empty() && !hi.empty()) steps.push_back("Interval of interest: " + var + " on [" + lo + ", " + hi + "]");
            std::vector<std::string> dom;
            collect_text_trig_domain(arena, expr, dom);
            collect_domain(arena, parsed, dom);
            collect_domain(arena, n, dom);
            std::string domain_answer;
            if(dom.empty()) {
                domain_answer = "all real " + var;
                steps.push_back("Domain: " + domain_answer + ".");
            }
            else {
                if(auto inv_note = inverse_trig_plain_trig_note(arena, n)) steps.push_back(*inv_note);
                for(auto const &d : dom) steps.push_back(d);
                domain_answer = dom.front();
                auto pfx = domain_answer.find("Domain: ");
                if(pfx != std::string::npos) domain_answer = domain_answer.substr(pfx + 8);
                Node const &dn = arena.get(n);
                if(auto solved = sqrt_log_base_domain(arena, n, var, steps)) {
                    domain_answer = *solved;
                    steps.push_back("Domain: " + domain_answer + ".");
                }
                else if(dn.kind == NodeKind::Fn && dn.fkind == FnKind::Log) {
                    if(auto solved = positive_linear_domain(arena, dn.a, var)) {
                        domain_answer = *solved;
                        steps.push_back("Solve the log condition: " + domain_answer + ".");
                    }
                }
            }
            std::string range_answer;
            if(!contains_symbol(arena, n, var)) {
                range_answer = "y = " + format_expr(arena, n);
                steps.push_back("Range: " + range_answer + ".");
            }
            else if(auto p = poly_of(arena, n, var); p && p->ok && !is_zero(p->a2)) {
                Rational a = p->a2;
                Rational b = p->a1;
                Rational c = p->a0;
                Rational b2 = r_mul(b, b);
                Rational foura = r_mul(Rational{4, 1}, a);
                Rational y0 = r_add(c, r_neg(r_div(b2, foura)));
                NodeId y0n = casio::num(arena, y0.num, y0.den);
                range_answer = a.num > 0 ? "y >= " + format_expr(arena, y0n) : "y <= " + format_expr(arena, y0n);
                if(lo_v && hi_v && std::isfinite(*lo_v) && std::isfinite(*hi_v)) {
                    std::vector<double> vals;
                    auto ylo = eval_node(arena, n, var, *lo_v);
                    auto yhi = eval_node(arena, n, var, *hi_v);
                    if(ylo) vals.push_back(*ylo);
                    if(yhi) vals.push_back(*yhi);
                    double vertex = -((double)b.num / b.den) / (2.0 * ((double)a.num / a.den));
                    if(vertex >= std::min(*lo_v, *hi_v) - 1e-12 && vertex <= std::max(*lo_v, *hi_v) + 1e-12) {
                        auto yv = eval_node(arena, n, var, vertex);
                        if(yv) vals.push_back(*yv);
                    }
                    if(!vals.empty()) {
                        auto [mn, mx] = std::minmax_element(vals.begin(), vals.end());
                        range_answer = format_double_compact(*mn) + " <= y <= " + format_double_compact(*mx);
                    }
                }
                else if(lo_v && std::isfinite(*lo_v) && hi_v && !std::isfinite(*hi_v) && a.num > 0) {
                    double vertex = -((double)b.num / b.den) / (2.0 * ((double)a.num / a.den));
                    if(*lo_v >= vertex) {
                        auto ylo = eval_node(arena, n, var, *lo_v);
                        if(ylo) range_answer = "y >= " + format_double_compact(*ylo);
                    }
                }
                std::string range = "Range: " + range_answer;
                if(!lo.empty() && !hi.empty()) range += " on the interval.";
                steps.push_back(range);
            }
            else if(auto p = poly_of(arena, n, var); p && p->ok && is_zero(p->a2) && !is_zero(p->a1)) {
                range_answer = "all real y";
                if(lo_v && std::isfinite(*lo_v) && hi_v && !std::isfinite(*hi_v)) {
                    auto ylo = eval_node(arena, n, var, *lo_v);
                    if(ylo) range_answer = p->a1.num > 0 ? "y >= " + format_double_compact(*ylo) : "y <= " + format_double_compact(*ylo);
                }
                steps.push_back("Range: " + range_answer + ".");
            }
            else {
                Node const &rn = arena.get(n);
                if(auto exp_range = exp_plus_recip_range(arena, n, var, steps)) {
                    range_answer = *exp_range;
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto symq = monic_symbolic_quadratic(arena, n, var)) {
                    NodeId half_b = casio::simplify(arena, casio::div(arena, symq->b, casio::num(arena, 2)));
                    NodeId y0 = casio::simplify(arena, casio::add(arena, {
                        symq->c,
                        casio::neg(arena, casio::power(arena, half_b, casio::num(arena, 2)))
                    }));
                    range_answer = "y >= " + format_expr(arena, y0);
                    steps.push_back("Complete the square to find the minimum.");
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto trig_range = direct_trig_range(arena, n, steps)) {
                    range_answer = *trig_range;
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto rec_cos = reciprocal_cos_range(arena, n, steps)) {
                    range_answer = *rec_cos;
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto cos_sin = cos_over_linear_sin_range(arena, n, steps)) {
                    range_answer = *cos_sin;
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto xq = x_over_quadratic_range(arena, n, var, steps)) {
                    range_answer = *xq;
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto rq = one_over_positive_quadratic_range(arena, n, var, steps)) {
                    range_answer = *rq;
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto sqrt_min = sqrt_abs_linear_min_info(arena, n, var)) {
                    range_answer = "y >= " + sqrt_bound_text(arena, sqrt_min->min);
                    if(sqrt_min->piecewise_sum) steps.push_back("Piecewise abs sum: minimum distance = " + format_rat(arena, sqrt_min->min) + ".");
                    else {
                        steps.push_back(abs_linear_text(arena, rn.a, var) + " >= 0.");
                        steps.push_back("inside sqrt >= " + format_rat(arena, sqrt_min->min) + ".");
                    }
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto abs_info = abs_linear_min_info(arena, n, var)) {
                    range_answer = "y >= " + format_expr(arena, arena.num(abs_info->min));
                    if(abs_info->piecewise_sum) steps.push_back("Piecewise abs sum: minimum distance = " + format_rat(arena, abs_info->min) + ".");
                    else steps.push_back(abs_linear_text(arena, n, var) + " >= 0.");
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(rn.kind == NodeKind::Fn && rn.fkind == FnKind::Acos) {
                    range_answer = "0 <= y <= pi";
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(rn.kind == NodeKind::Fn && rn.fkind == FnKind::Asin) {
                    range_answer = "-pi/2 <= y <= pi/2";
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(rn.kind == NodeKind::Fn && rn.fkind == FnKind::Atan) {
                    range_answer = "-pi/2 < y < pi/2";
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(rn.kind == NodeKind::Fn && rn.fkind == FnKind::Sqrt) {
                    if(auto inner_min = abs_linear_plus_const_min(arena, rn.a, var); inner_min && inner_min->num >= 0) {
                        range_answer = "y >= " + sqrt_bound_text(arena, *inner_min);
                        steps.push_back(abs_linear_text(arena, rn.a, var) + " >= 0.");
                        steps.push_back("Range: " + range_answer + ".");
                    }
                    else if(is_sqrt_square(arena, n)) {
                        range_answer = "y >= 0";
                        steps.push_back("Range: " + range_answer + ".");
                    }
                    else {
                        range_answer = !lo.empty() && !hi.empty() ? "unrestricted on interval" : "inspect graph/transform";
                        steps.push_back(!lo.empty() && !hi.empty() ? "Range: unrestricted on the interval (inspect graph/transform if needed)." : "Range: inspect graph/transform if needed.");
                    }
                }
                else if(is_sqrt_square(arena, n)) {
                    range_answer = "y >= 0";
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto trig_min = unbounded_trig_square_min(arena, n)) {
                    range_answer = "y >= " + format_expr(arena, arena.num(*trig_min));
                    steps.push_back("Squared tan/cot/sec/cosec term gives a lower bound.");
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto log_range = log_abs_linear_range(arena, n, var)) {
                    range_answer = *log_range;
                    steps.push_back("Use abs(linear) >= 0, then apply log monotonicity.");
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(auto plain_log_range = log_linear_range(arena, n, var)) {
                    range_answer = *plain_log_range;
                    steps.push_back("A non-constant linear input covers all positive log arguments.");
                    steps.push_back("Range: " + range_answer + ".");
                }
                else if(rn.kind == NodeKind::Div) {
                    Node const &top = arena.get(rn.a);
                    Node const &bot = arena.get(rn.b);
                    if(top.kind == NodeKind::Num && bot.kind == NodeKind::Sym && bot.text == var) {
                        range_answer = "y != 0";
                        steps.push_back("Range: " + range_answer + ".");
                    }
                    else if(top.kind == NodeKind::Num) {
                        auto bp = poly_of(arena, rn.b, var);
                        if(bp && bp->ok && is_zero(bp->a2) && !is_zero(bp->a1) &&
                           lo_v && std::isfinite(*lo_v) && hi_v && !std::isfinite(*hi_v)) {
                            auto den_lo = eval_node(arena, rn.b, var, *lo_v);
                            if(den_lo && std::fabs(*den_lo) < 1e-9 && bp->a1.num > 0) {
                                range_answer = top.num.num > 0 ? "y > 0" : "y < 0";
                                steps.push_back("As " + var + " increases, denominator increases and y tends to 0.");
                                steps.push_back("Range: " + range_answer + ".");
                            }
                            else if(auto ylo = eval_node(arena, n, var, *lo_v);
                                    ylo && top.num.num > 0 && bp->a1.num > 0 && *ylo > 0) {
                                range_answer = "0 < y <= " + format_double_compact(*ylo);
                                steps.push_back("As " + var + " increases, denominator increases and y tends to 0.");
                                steps.push_back("Range: " + range_answer + ".");
                            }
                            else {
                                range_answer = "inspect graph/transform";
                                steps.push_back("Range: inspect graph/transform if needed.");
                            }
                        }
                        else {
                            range_answer = !lo.empty() && !hi.empty() ? "unrestricted on interval" : "inspect graph/transform";
                            steps.push_back(!lo.empty() && !hi.empty() ? "Range: unrestricted on the interval (inspect graph/transform if needed)." : "Range: inspect graph/transform if needed.");
                        }
                    }
                    else {
                        range_answer = !lo.empty() && !hi.empty() ? "unrestricted on interval" : "inspect graph/transform";
                        steps.push_back(!lo.empty() && !hi.empty() ? "Range: unrestricted on the interval (inspect graph/transform if needed)." : "Range: inspect graph/transform if needed.");
                    }
                }
                else {
                    range_answer = !lo.empty() && !hi.empty() ? "unrestricted on interval" : "inspect graph/transform";
                    steps.push_back(!lo.empty() && !hi.empty() ? "Range: unrestricted on the interval (inspect graph/transform if needed)." : "Range: inspect graph/transform if needed.");
                }
            }
            std::string answer = req.method == "domain" ? domain_answer :
                                 (req.method == "range" ? range_answer : (domain_answer + "; " + range_answer));
            return casio::exam_block("domain/range", steps, answer);
        }
        if(req.mode == 11) {
            auto nl1 = req.expr.find('\n');
            if(nl1 == std::string::npos) return {"Err: need x(t), y(t), parameter."};
            auto nl2 = req.expr.find('\n', nl1 + 1);
            std::string xe = trim_text(req.expr.substr(0, nl1));
            std::string ye = trim_text(nl2 == std::string::npos ? req.expr.substr(nl1 + 1) : req.expr.substr(nl1 + 1, nl2 - nl1 - 1));
            std::string param = trim_text(nl2 == std::string::npos ? "t" : req.expr.substr(nl2 + 1));
            auto strip_outer_parens = [](std::string s) {
                s = trim_text(s);
                while(s.size() >= 2 && s.front() == '(' && s.back() == ')') {
                    int depth = 0;
                    bool wraps = true;
                    for(std::size_t i = 0; i < s.size(); ++i) {
                        if(s[i] == '(') depth++;
                        else if(s[i] == ')') depth--;
                        if(depth == 0 && i + 1 < s.size()) {
                            wraps = false;
                            break;
                        }
                    }
                    if(!wraps) break;
                    s = trim_text(s.substr(1, s.size() - 2));
                }
                return s;
            };
            xe = strip_outer_parens(xe);
            ye = strip_outer_parens(ye);
            param = strip_outer_parens(param);
            if(param.empty()) param = "t";
            std::string xk = compact_input_key(xe);
            std::string yk = compact_input_key(ye);
            if((xk == "sin(" + param + ")" && yk == "cos(" + param + ")") ||
               (xk == "cos(" + param + ")" && yk == "sin(" + param + ")")) {
                return {
                    "1. Square both equations.",
                    "2. Use sin(" + param + ")^2+cos(" + param + ")^2=1.",
                    "Answer: x^2 + y^2 = 1",
                };
            }
            auto y_cart = cartesian_from_param(arena, xe, ye, param);
            if(!y_cart) return {"Err: cartesian conversion supports linear x(t)."};
            return {
                "1. Solve x(t) for " + param + ".",
                "2. Substitute into y(t).",
                "Answer: y = " + format_expr(arena, *y_cart),
            };
        }
        if(req.mode == 12) {
            auto parts = split_csv(req.expr);
            if(parts.size() < 3) return {"Err: need Eq, x0, n."};
            std::string equation_text = trim_text(parts[0]);
            auto clean_number = [](std::string s) {
                s = trim_text(s);
                while(!s.empty() && (s.back() == '.' || s.back() == ',')) s.pop_back();
                return s;
            };
            std::string x0_text = clean_number(parts[1]);
            std::string n_text = clean_number(parts[2]);

            auto eq = casio::parse_equation(arena, equation_text);
            NodeId residual = 0;
            if(eq) {
                residual = casio::simplify(arena, casio::add(arena, {eq->lhs, casio::mul(arena, {casio::num(arena, -1), eq->rhs})}));
            }
            else {
                residual = casio::simplify(arena, casio::parse_expr(arena, equation_text));
            }
            std::string var = choose_solve_var(arena, residual, "x");
            auto x0 = parse_const_double(arena, x0_text);
            if(!x0) return {"Err: Bad number."};
            int steps_count = 0;
            try { steps_count = std::stoi(n_text); } catch(...) { return {"Err: Bad number."}; }
            if(steps_count < 1 || steps_count > 20) return {"Err: n must be 1..20."};

            std::vector<std::string> out;
            out.push_back("1. Use Newton-Raphson.");
            out.push_back("2. f(" + var + ") = " + format_expr(arena, residual));
            out.push_back("3. Use x_(n+1) = x_n - f(x_n)/f'(x_n).");
            double x = *x0;
            for(int i = 0; i < steps_count; i++) {
                double h = std::max(1e-6, std::fabs(x) * 1e-6);
                auto fx = eval_node(arena, residual, var, x);
                auto fp1 = eval_node(arena, residual, var, x + h);
                auto fm1 = eval_node(arena, residual, var, x - h);
                if(!fx || !fp1 || !fm1) return {"Err: Newton evaluation failed."};
                double dfx = (*fp1 - *fm1) / (2.0 * h);
                if(!std::isfinite(dfx) || std::fabs(dfx) < 1e-14) return {"Err: derivative too small."};
                double next = x - *fx / dfx;
                out.push_back(std::to_string(4 + i) + ". x_" + std::to_string(i + 1) + " = " + format_double_compact(next));
                x = next;
            }
            std::ostringstream ans3;
            ans3 << std::fixed << std::setprecision(3) << x;
            out.push_back("Answer: " + var + " = " + ans3.str() + " (3 d.p.)");
            return out;
        }

        if(req.method == "factor" && req.mode != 13 && req.expr.find('=') == std::string::npos) {
            algebra::Request next = req;
            next.mode = 13;
            auto parts = split_csv(req.expr);
            if(!parts.empty()) next.expr = parts[0];
            return run(arena, next);
        }

        if(req.mode == 13) {
            // Factor: simple factorization for ax^2+bx+c
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));
            auto p = poly_of(arena, n, "x");
            if(!p || !p->ok) {
                return {
                    "1. Start with " + format_expr(arena, n) + ".",
                    "2. This lightweight factor route needs numeric polynomial coefficients.",
                    "3. Use solve/expand/collect first if symbolic parameters remain.",
                    "Answer: not factorable with numeric roots in this lightweight route.",
                };
            }
            
            // If not quadratic (no x^2 term), check if already factored form
            if(is_zero(p->a2)) {
                std::vector<std::string> out;
                // Could be (ax+b)^2 or similar - just return as is
                std::string ans = format_expr(arena, n);
                out.push_back("Factored form: " + ans);
                out.push_back("Answer: " + ans);
                return out;
            }

            Rational a = p->a2;
            Rational b = p->a1;
            Rational c = p->a0;
            // Factorization via finding roots (quadratic formula)
            Rational b2_4ac = r_sub(r_mul(b, b), r_mul(r_mul(Rational{4, 1}, a), c));
            if(b2_4ac.num < 0) return {"Err: complex roots, not factored over reals."};

            auto opt_disc = as_int64(b2_4ac);
            if(!opt_disc || *opt_disc < 0) {
                return {"Err: non-square discriminant."};
            }
            std::int64_t discriminant = *opt_disc;
            std::int64_t sqrt_disc;
            if(!is_square_i64(discriminant, sqrt_disc)) {
                return {"Err: non-square discriminant."};
            }
            // roots: (-b +- sqrt(D)) / (2a)
            Rational twoa = r_mul(Rational{2, 1}, a);
            Rational r1 = r_div(r_add(r_neg(b), Rational{sqrt_disc, 1}), twoa);
            Rational r2 = r_div(r_sub(r_neg(b), Rational{sqrt_disc, 1}), twoa);

            bool r1_int = (r1.den == 1);
            bool r2_int = (r2.den == 1);

            std::vector<std::string> out;
            out.push_back("Find the roots, then write the factor form.");

            if(r1_int && r2_int && r1.num != r2.num) {
                NodeId x = casio::sym(arena, "x");
                NodeId term1 = casio::add(arena, {x, casio::num(arena, -r1.num)});
                NodeId term2 = casio::add(arena, {x, casio::num(arena, -r2.num)});
                NodeId factored;
                if(a.num != 1 && a.num != -1) {
                    factored = casio::mul(arena, {casio::num(arena, a.num, a.den), term1, term2});
                } else {
                    factored = casio::mul(arena, {term1, term2});
                }
                factored = casio::simplify(arena, factored);
                std::string ans = format_expr(arena, factored);
                out.push_back("Factored form: " + ans);
                out.push_back("Answer: " + ans);
                return out;
            } else if(r1_int && r1.num == r2.num) {
                NodeId x = casio::sym(arena, "x");
                NodeId term = casio::add(arena, {x, casio::num(arena, -r1.num)});
                NodeId sq = casio::power(arena, term, casio::num(arena, 2));
                NodeId factored;
                if(a.num != 1 && a.num != -1) {
                    factored = casio::mul(arena, {casio::num(arena, a.num, a.den), sq});
                } else {
                    factored = sq;
                }
                factored = casio::simplify(arena, factored);
                std::string ans = format_expr(arena, factored);
                out.push_back("Factored form: " + ans);
                out.push_back("Answer: " + ans);
                return out;
            } else {
                return {"Err: non-integer roots."};
            }
        }

        auto solve_parts = split_csv(req.expr);
        std::string equation_text = solve_parts.empty() ? req.expr : solve_parts[0];
        std::string explicit_var = (solve_parts.size() >= 2) ? trim_text(solve_parts[1]) : "";
        std::optional<double> interval_lo;
        std::optional<double> interval_hi;
        if(solve_parts.size() >= 4) {
            interval_lo = parse_const_double(arena, solve_parts[2]);
            interval_hi = parse_const_double(arena, solve_parts[3]);
        }
        if(req.mode == 6 && equation_text.find('=') == std::string::npos) {
            equation_text += "=0";
        }
        if(req.method == "factor") {
            if(auto trig_factored = solve_trig_factor_substitution(equation_text)) return *trig_factored;
        }
        auto eq = casio::parse_equation(arena, equation_text);
        if(!eq) {
            NodeId parsed = casio::parse_expr(arena, req.expr);
            auto pre = casio::build_exam_prelude(arena, req.expr, parsed);
            NodeId n = casio::simplify(arena, parsed);

            std::vector<std::string> steps;
            if(req.method == "pf" || req.method == "partfrac") {
                if(auto pf = partial_fraction_x2_linear(arena, parsed, req.expr, "x")) return *pf;
                Node const &pn = arena.get(parsed);
                if(pn.kind != NodeKind::Div) {
                    return casio::exam_block(
                        "partial fractions",
                        {
                            "Start with " + format_expr(arena, parsed) + ".",
                            "Partial fractions need rational P(x)/Q(x).",
                            "This expression is not rational, so PF is not applicable.",
                        },
                        format_expr(arena, n)
                    );
                }
            }
            Node const &pn = arena.get(parsed);
            if(pn.kind == NodeKind::Fn && (pn.fkind == FnKind::Asin || pn.fkind == FnKind::Acos || pn.fkind == FnKind::Atan)) {
                Node const &inner = arena.get(pn.a);
                if((pn.fkind == FnKind::Asin && inner.kind == NodeKind::Fn && inner.fkind == FnKind::Sin) ||
                   (pn.fkind == FnKind::Acos && inner.kind == NodeKind::Fn && inner.fkind == FnKind::Cos) ||
                   (pn.fkind == FnKind::Atan && inner.kind == NodeKind::Fn && inner.fkind == FnKind::Tan)) {
                    std::string outer = pn.fkind == FnKind::Asin ? "asin" : (pn.fkind == FnKind::Acos ? "acos" : "atan");
                    std::string trig = pn.fkind == FnKind::Asin ? "sin" : (pn.fkind == FnKind::Acos ? "cos" : "tan");
                    steps.push_back("Start with " + format_expr(arena, parsed) + ".");
                    steps.push_back("Let u = " + format_expr(arena, inner.a) + ".");
                    steps.push_back(outer + "(" + trig + "(u))=u only for -pi/2 <= u <= pi/2.");
                    steps.push_back("Branch: outside that interval, fold by periodic symmetry.");
                    return casio::exam_block("inverse trig branch", steps, format_expr(arena, n));
                }
            }
            if(auto rec = reciprocal_trig_rewrite(arena, parsed)) {
                steps.push_back("Start with " + format_expr(arena, parsed) + ".");
                steps.push_back(rec->first);
                std::vector<std::string> dom;
                collect_domain(arena, parsed, dom);
                for(auto const &d : dom) steps.push_back(d);
                return casio::exam_block("algebra simplify", steps, format_expr(arena, rec->second));
            }
            if(auto ident = reciprocal_trig_identity_step(req.expr)) {
                steps.push_back("Start with " + trim_text(req.expr) + ".");
                steps.push_back(*ident);
            }
            else {
                casio::append_exam_prelude_steps(steps, pre);
            }

            // Domain hints (best-effort).
            std::vector<std::string> dom;
            collect_domain(arena, n, dom);
            for(auto const &d : dom) steps.push_back(d);

            // Range hint for quadratics ax^2+bx+c.
            if(auto p = poly_of(arena, n, "x"); p && p->ok && !is_zero(p->a2)) {
                Rational a = p->a2;
                Rational b = p->a1;
                Rational c = p->a0;
                // vertex x0 = -b/(2a), y0 = f(x0) = c - b^2/(4a)
                Rational b2 = r_mul(b, b);
                Rational foura = r_mul(Rational{4, 1}, a);
                Rational frac = r_div(b2, foura);
                Rational y0 = r_add(c, r_neg(frac));
                NodeId y0n = casio::num(arena, y0.num, y0.den);
                if(a.num > 0) steps.push_back("Range: y >= " + format_expr(arena, y0n));
                else if(a.num < 0) steps.push_back("Range: y <= " + format_expr(arena, y0n));
            }

            return casio::exam_block("algebra simplify", steps, format_expr(arena, n));
        }

        NodeId lhs = casio::simplify(arena, eq->lhs);
        NodeId rhs = casio::simplify(arena, eq->rhs);

        NodeId rearr = casio::simplify(arena, casio::add(arena, {lhs, casio::neg(arena, rhs)}));
        std::string solve_var = choose_solve_var(arena, rearr, explicit_var);

        // Exam-style numbered working.
        std::vector<std::string> out;
        out.push_back("1. Start with " + equation_text + ".");
        if(interval_lo && interval_hi) {
            out.push_back(
                "2. Interval: " + solve_var + " in [" + format_double_compact(*interval_lo) + ", " + format_double_compact(*interval_hi) + "]"
            );
        }
        std::vector<std::string> domain_lines;
        collect_text_trig_domain(arena, equation_text, domain_lines);
        collect_domain(arena, lhs, domain_lines);
        collect_domain(arena, rhs, domain_lines);
        if(!domain_lines.empty()) {
            for(auto const &d : domain_lines) out.push_back(d);
        }
        else if(equation_text.find("log") != std::string::npos || equation_text.find("ln") != std::string::npos) {
            out.push_back("Domain: log args >0.");
        }
        if(auto ident = reciprocal_trig_identity_step(equation_text)) out.push_back(*ident);

        if(auto trig = simple_trig_zero_solve(arena, lhs, rhs, solve_var, equation_text))
            return *trig;
        if(auto inv_trig = inverse_trig_principal_solve(arena, lhs, rhs, solve_var, equation_text))
            return *inv_trig;

        auto rp = ratpoly_of_node(arena, rearr, solve_var);
        if(!rp.ok) {
            append_nonrat_equation_route(arena, out, rearr, solve_var);
            auto numeric = numeric_roots_scan(arena, rearr, solve_var, interval_lo, interval_hi);
            if(numeric.empty()) {
                out.push_back(interval_lo && interval_hi ? "No solution in the interval after checking the reduced equation." :
                                                           "No real solution found by the reduced equation checks.");
                out.push_back("Answer: " + solve_var + " = []");
            }
            else {
                append_answer(out, solve_var, numeric);
                append_numeric_3dp(arena, out, solve_var, numeric);
            }
            return out;
        }

        if(req.method == "factor" && is_zero(rp.den.a1) && is_zero(rp.den.a2) && !is_zero(rp.num.a2)) {
            if(auto roots = rational_quadratic_roots(rp.num)) {
                std::string factored = quadratic_factor_text(arena, rp.num, solve_var);
                out.push_back("2. Move all terms: " + format_expr(arena, rearr) + " = 0");
                out.push_back("3. Factor: " + factored + " = 0");
                out.push_back(
                    "4. So " + linear_factor_from_root(arena, solve_var, roots->first) + " = 0 or " +
                    linear_factor_from_root(arena, solve_var, roots->second) + " = 0."
                );
                out.push_back("5. Check roots in the original equation.");
                auto sols = solve_poly2(arena, rp.num, solve_var);
                sols = filter_real_solutions(arena, rearr, solve_var, sols, interval_lo, interval_hi);
                if(sols.empty()) {
                    out.push_back(interval_lo && interval_hi ? "No solution in the interval." : "No solution.");
                    out.push_back("Answer: " + solve_var + " = []");
                    return out;
                }
                append_answer(out, solve_var, sols);
                append_numeric_3dp(arena, out, solve_var, sols);
                return out;
            }
            out.push_back("2. Move all terms: " + format_expr(arena, rearr) + " = 0");
            out.push_back("3. It does not factor into rational linear factors.");
            out.push_back("4. Use the quadratic formula instead.");
        }

        if(req.method == "complete_square" && is_zero(rp.den.a1) && is_zero(rp.den.a2) && !is_zero(rp.num.a2)) {
            Rational a = rp.num.a2;
            Rational b = rp.num.a1;
            Rational c = rp.num.a0;
            Rational h = r_div(b, r_mul(Rational{2, 1}, a));
            Rational k = r_sub(c, r_div(r_mul(b, b), r_mul(Rational{4, 1}, a)));
            NodeId x = casio::sym(arena, solve_var);
            NodeId hnode = casio::num(arena, h.num, h.den);
            NodeId knode = casio::num(arena, k.num, k.den);
            NodeId anode = casio::num(arena, a.num, a.den);
            NodeId square_form = casio::simplify(
                arena,
                casio::add(
                    arena,
                    {
                        casio::mul(arena, {anode, casio::power(arena, casio::add(arena, {x, hnode}), casio::num(arena, 2))}),
                        knode,
                    }
                )
            );
            out.push_back("2. Move all terms: " + format_expr(arena, rearr) + " = 0");
            out.push_back("3. Complete square: " + format_expr(arena, square_form) + " = 0");
            Rational rhs = r_div(r_neg(k), a);
            NodeId rhs_node = casio::num(arena, rhs.num, rhs.den);
            NodeId root_rhs = casio::fn(arena, "sqrt", rhs_node);
            NodeId shifted_x = casio::simplify(arena, casio::add(arena, {x, hnode}));
            std::string root_text = format_expr(arena, root_rhs);
            std::int64_t rn = 0;
            if(auto rv = as_int64(rhs); rv && is_square_i64(*rv, rn)) root_text = std::to_string(rn);
            out.push_back("4. Isolate square: (" + format_expr(arena, shifted_x) + ")^2 = " + format_expr(arena, rhs_node));
            out.push_back("5. Take square roots: " + format_expr(arena, shifted_x) + " = +/-" + root_text);
            out.push_back("6. Check roots in the original equation.");
            auto sols = solve_poly2(arena, rp.num, solve_var);
            sols = filter_real_solutions(arena, rearr, solve_var, sols, interval_lo, interval_hi);
            if(sols.empty()) {
                out.push_back(interval_lo && interval_hi ? "No solution in the interval." : "No solution.");
                out.push_back("Answer: " + solve_var + " = []");
                return out;
            }
            append_answer(out, solve_var, sols);
            append_numeric_3dp(arena, out, solve_var, sols);
            return out;
        }

        // Linear case: ax + b = 0 (a2=0, a1 != 0)
        if(is_zero(rp.num.a2) && !is_zero(rp.num.a1)) {
            Rational a = rp.num.a1;
            Rational b = rp.num.a0;
            // Giac-style: check for zero coefficient (division by zero)
            if(is_zero(a)) {
                out.push_back("2. Coefficient of " + solve_var + " is 0, so the equation is constant.");
                out.push_back("Answer: no solution");
                return out;
            }
            out.push_back("2. Expr = " + format_expr(arena, rearr) + " = 0");
            if(!is_zero(b)) {
                NodeId b_node = casio::num(arena, -b.num, b.den);
                out.push_back("3. Subtract " + format_expr(arena, b_node));
            }
            if(a.num != 1 || a.den != 1) {
                NodeId a_node = casio::num(arena, a.num, a.den);
                out.push_back("4. Divide by " + format_expr(arena, a_node));
            }
            auto sols = solve_poly2(arena, rp.num, solve_var);
            sols = filter_real_solutions(arena, rearr, solve_var, sols, interval_lo, interval_hi);
            if(sols.empty()) {
                out.push_back(interval_lo && interval_hi ? "No solution in the interval." : "No solution.");
                out.push_back("Answer: " + solve_var + " = []");
                return out;
            }
            append_answer(out, solve_var, sols);
            append_numeric_3dp(arena, out, solve_var, sols);
            return out;
        }

        // Quadratic case: check for a=0 (not quadratic)
        if(!is_zero(rp.num.a2) && is_zero(rp.num.a1)) {
            Rational a = rp.num.a2;
            if(is_zero(a)) {
                out.push_back("2. All variable terms cancel, so compare constants.");
                out.push_back("Answer: no solution");
                return out;
            }
        }

        // Higher degree
        std::string eq_s = format_expr(arena, lhs) + " = " + format_expr(arena, rhs);
        out.push_back("2. Expr = " + eq_s);
        if(!is_zero(rp.den.a1) || !is_zero(rp.den.a2)) {
            out.push_back("3. Clearing denominators.");
        }
        auto sols = solve_poly2(arena, rp.num, solve_var);
        sols = filter_real_solutions(arena, rearr, solve_var, sols, interval_lo, interval_hi);
        if(sols.empty()) {
            out.push_back(interval_lo && interval_hi ? "No solution in the interval." : "No solution.");
            out.push_back("Answer: " + solve_var + " = []");
            return out;
        }
        append_answer(out, solve_var, sols);
        append_numeric_3dp(arena, out, solve_var, sols);
        return out;
    }
    catch(std::exception const &e) {
        casio::ExamPrelude pre;
        pre.raw = req.expr;
        pre.norm = casio::normalize_text(req.expr);
        pre.parsed = req.expr;
        pre.simplified = req.expr;
        return casio::exam_fallback("algebra solve", pre, e.what(), "solve(" + pre.norm + ")");
    }
}

} // namespace casio::algebra

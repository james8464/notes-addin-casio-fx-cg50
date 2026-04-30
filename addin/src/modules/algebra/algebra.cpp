#include "algebra.hpp"

#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>

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

static std::vector<std::string> solve_poly2(Arena &a, Poly2 const &p)
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
        return {"x = " + format_expr(a, sol)};
    }

    // quadratic formula
    Rational b2 = r_mul(p.a1, p.a1);
    Rational four{4, 1};
    Rational ac4 = r_mul(r_mul(four, p.a2), p.a0);
    Rational disc = r_add(b2, r_neg(ac4));
    disc.normalize();

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

    NodeId two_a = a.num(r_mul(Rational{2, 1}, p.a2));
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
        if(format_expr(a, s1) == format_expr(a, s2)) return {"x = " + format_expr(a, s1)};
        return {"x = " + format_expr(a, s1), "x = " + format_expr(a, s2)};
    }

    NodeId x_plus = casio::div(a, casio::add(a, {minus_b, sqrt_disc}), two_a);
    NodeId x_minus = casio::div(a, casio::add(a, {minus_b, casio::neg(a, sqrt_disc)}), two_a);
    x_plus = casio::simplify(a, x_plus);
    x_minus = casio::simplify(a, x_minus);
    if(format_expr(a, x_plus) == format_expr(a, x_minus)) return {"x = " + format_expr(a, x_plus)};
    return {"x = " + format_expr(a, x_plus), "x = " + format_expr(a, x_minus)};
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter expression/equation."};

    try {
        auto eq = casio::parse_equation(arena, req.expr);
        if(!eq) {
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));
            return {"Simplify: " + format_expr(arena, n)};
        }

        NodeId lhs = casio::simplify(arena, eq->lhs);
        NodeId rhs = casio::simplify(arena, eq->rhs);

        NodeId zero = casio::num(arena, 0);
        NodeId rearr = casio::simplify(arena, casio::add(arena, {lhs, casio::neg(arena, rhs)}));

        std::vector<std::string> out;
        out.push_back("Solve:");
        out.push_back(format_expr(arena, lhs) + " = " + format_expr(arena, rhs));
        out.push_back("Rearrange:");
        out.push_back(format_expr(arena, rearr) + " = " + format_expr(arena, zero));

        auto p = poly_of(arena, rearr, "x");
        if(!p || !p->ok) {
            out.push_back("Failed: only linear/quadratic in x supported (for now).");
            return out;
        }

        auto sols = solve_poly2(arena, *p);
        out.push_back("Answer:");
        for(auto const &s : sols) out.push_back(s);
        return out;
    }
    catch(std::exception const &e) {
        return {std::string("Error: ") + e.what()};
    }
}

} // namespace casio::algebra


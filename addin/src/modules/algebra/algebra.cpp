#include "algebra.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/same.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <cmath>
#include <cstdint>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <functional>

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

static void collect_domain(Arena &a, NodeId n, std::vector<std::string> &out)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        out.push_back("Domain: " + format_expr(a, x.b) + " != 0");
        collect_domain(a, x.a, out);
        collect_domain(a, x.b, out);
        return;
    }
    if(x.kind == NodeKind::Fn) {
        if(x.fkind == FnKind::Sqrt) out.push_back("Domain: " + format_expr(a, x.a) + " >= 0");
        if(x.fkind == FnKind::Log) out.push_back("Domain: " + format_expr(a, x.a) + " > 0");
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

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter expression/equation."};

    try {
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
                    case FnKind::Sqrt: return std::sqrt(a);
                    case FnKind::Abs: return std::fabs(a);
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
            if(x.kind != NodeKind::Pow) return {"Err: only (ax+b)^n supported."};
            Node const &expn = arena.get(x.b);
            if(expn.kind != NodeKind::Num || expn.num.den != 1) return {"Err: exponent must be integer."};
            int nn = (int)expn.num.num;
            if(nn < 0 || nn > 12) return {"Err: exponent out of range."};

            // Extract base = a*x + b (order doesn't matter)
            NodeId base = x.a;
            Node const &bn = arena.get(base);
            if(bn.kind != NodeKind::Add || bn.kids.size() != 2) return {"Err: base must be ax+b."};

            auto extract_ax = [&](NodeId t) -> std::optional<Rational> {
                Node const &tn = arena.get(t);
                if(tn.kind == NodeKind::Mul && tn.kids.size() == 2) {
                    Node const &k0 = arena.get(tn.kids[0]);
                    Node const &k1 = arena.get(tn.kids[1]);
                    if(k0.kind == NodeKind::Num && k1.kind == NodeKind::Sym && k1.text == "x") return k0.num;
                    if(k1.kind == NodeKind::Num && k0.kind == NodeKind::Sym && k0.text == "x") return k1.num;
                }
                if(tn.kind == NodeKind::Sym && tn.text == "x") return Rational{1, 1};
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
                    NodeId xpow = (k == 1) ? casio::sym(arena, "x") : casio::power(arena, casio::sym(arena, "x"), casio::num(arena, k));
                    term = casio::mul(arena, {term, xpow});
                }
                terms.push_back(term);
            }
            NodeId outn = casio::simplify(arena, casio::add(arena, terms));
            std::string out_text = format_expr(arena, outn);
            return {
                "1. Expand using binomial expansion.",
                "2. (ax+b)^n with a=" + format_expr(arena, casio::num(arena, acoef.num, acoef.den)) + ", b=" +
                    format_expr(arena, casio::num(arena, bcoef.num, bcoef.den)) + ", n=" + std::to_string(nn),
                "3. Combine like terms.",
                "4. Out = " + out_text,
                "5. Answer: Out = " + out_text,
            };
        }
        if(req.mode == 5) {
            // Complete square for quadratic ax^2+bx+c.
            NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));
            auto p = poly_of(arena, n, "x");
            if(!p || !p->ok || is_zero(p->a2)) return {"Err: need quadratic in x."};

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
                "Ans = " + ans,
            };
        }

        auto eq = casio::parse_equation(arena, req.expr);
        if(!eq) {
            NodeId parsed = casio::parse_expr(arena, req.expr);
            auto pre = casio::build_exam_prelude(arena, req.expr, parsed);
            NodeId n = casio::simplify(arena, parsed);

            std::vector<std::string> steps;
            steps.push_back("Normalize: " + pre.norm);
            steps.push_back("Parse: " + pre.parsed);
            steps.push_back("Simplify: " + pre.simplified);

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

        NodeId zero = casio::num(arena, 0);
        NodeId rearr = casio::simplify(arena, casio::add(arena, {lhs, casio::neg(arena, rhs)}));

        std::vector<std::string> out;
        out.push_back("Solve:");
        out.push_back(format_expr(arena, lhs) + " = " + format_expr(arena, rhs));
        out.push_back("Rearrange:");
        out.push_back(format_expr(arena, rearr) + " = " + format_expr(arena, zero));

        auto rp = ratpoly_of_node(arena, rearr, "x");
        if(!rp.ok) {
            out.push_back("Failed: only (rational) linear/quadratic in x supported (for now).");
            return out;
        }

        // Solve numerator = 0 for rational expressions.
        auto sols = solve_poly2(arena, rp.num);
        out.push_back("Answer:");
        for(auto const &s : sols) out.push_back(s);
        return out;
    }
    catch(std::exception const &e) {
        return {std::string("Error: ") + e.what()};
    }
}

} // namespace casio::algebra


#include "trig.hpp"

#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/simplify.hpp"

#include <cstdint>
#include <optional>

namespace casio::trig
{

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static bool contains_pi(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Const && x.ckind == ConstKind::Pi) return true;
    if(x.kind == NodeKind::Fn) return contains_pi(a, x.a);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return contains_pi(a, x.a) || contains_pi(a, x.b);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(contains_pi(a, k)) return true;
    }
    return false;
}

static std::optional<Rational> pi_multiple_coeff(Arena &a, NodeId n)
{
    // Match pi * q, q * pi, (q*pi)/r, pi*(q/r), etc where q,r are numeric rationals.
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Const && x.ckind == ConstKind::Pi) return Rational{1, 1};
    if(x.kind == NodeKind::Mul) {
        bool saw_pi = false;
        Rational coeff{1, 1};
        for(auto kid : x.kids) {
            auto const &k = a.get(kid);
            if(k.kind == NodeKind::Const && k.ckind == ConstKind::Pi) {
                if(saw_pi) return std::nullopt;
                saw_pi = true;
                continue;
            }
            auto q = as_num(a, kid);
            if(!q) return std::nullopt;
            coeff.num *= q->num;
            coeff.den *= q->den;
            coeff.normalize();
        }
        if(saw_pi) return coeff;
    }
    if(x.kind == NodeKind::Div) {
        auto numc = pi_multiple_coeff(a, x.a);
        if(!numc) return std::nullopt;
        if(auto q = as_num(a, x.b)) {
            Rational inv{q->den, q->num};
            inv.normalize();
            Rational out;
            out.num = numc->num * inv.num;
            out.den = numc->den * inv.den;
            out.normalize();
            return out;
        }
    }
    return std::nullopt;
}

static std::optional<int> angle_to_degree_int(Arena &a, NodeId arg)
{
    // Degree mode heuristic: if contains pi, treat as radians -> convert to degrees when it's a pi multiple.
    if(contains_pi(a, arg)) {
        auto coeff = pi_multiple_coeff(a, arg);
        if(!coeff) return std::nullopt;
        // degrees = coeff * 180
        Rational deg{coeff->num * 180, coeff->den};
        deg.normalize();
        if(deg.den != 1) return std::nullopt;
        return static_cast<int>(deg.num);
    }
    // else assume degrees if plain integer
    auto n = as_num(a, arg);
    if(!n) return std::nullopt;
    Rational deg = *n;
    deg.normalize();
    if(deg.den != 1) return std::nullopt;
    return static_cast<int>(deg.num);
}

static int mod360(int d)
{
    int m = d % 360;
    if(m < 0) m += 360;
    return m;
}

static NodeId sqrt_int(Arena &a, int n)
{
    return a.fn(FnKind::Sqrt, casio::num(a, n));
}

static NodeId div_int(Arena &a, NodeId top, int bot)
{
    return casio::simplify(a, casio::div(a, top, casio::num(a, bot)));
}

static std::optional<NodeId> exact_first_quadrant(Arena &a, FnKind fk, int deg)
{
    // deg in {0,15,30,45,60,75,90}
    if(deg == 0) {
        if(fk == FnKind::Sin) return casio::num(a, 0);
        if(fk == FnKind::Cos) return casio::num(a, 1);
        if(fk == FnKind::Tan) return casio::num(a, 0);
    }
    if(deg == 30) {
        if(fk == FnKind::Sin) return div_int(a, casio::num(a, 1), 2);
        if(fk == FnKind::Cos) return div_int(a, sqrt_int(a, 3), 2);
        if(fk == FnKind::Tan) return div_int(a, sqrt_int(a, 3), 3);
    }
    if(deg == 45) {
        if(fk == FnKind::Sin) return div_int(a, sqrt_int(a, 2), 2);
        if(fk == FnKind::Cos) return div_int(a, sqrt_int(a, 2), 2);
        if(fk == FnKind::Tan) return casio::num(a, 1);
    }
    if(deg == 60) {
        if(fk == FnKind::Sin) return div_int(a, sqrt_int(a, 3), 2);
        if(fk == FnKind::Cos) return div_int(a, casio::num(a, 1), 2);
        if(fk == FnKind::Tan) return sqrt_int(a, 3);
    }
    if(deg == 90) {
        if(fk == FnKind::Sin) return casio::num(a, 1);
        if(fk == FnKind::Cos) return casio::num(a, 0);
        // tan 90 undefined
    }
    if(deg == 15) {
        // sin15 = (sqrt6 - sqrt2)/4 ; cos15 = (sqrt6 + sqrt2)/4 ; tan15 = 2 - sqrt3
        NodeId s6 = sqrt_int(a, 6);
        NodeId s2 = sqrt_int(a, 2);
        if(fk == FnKind::Sin) return div_int(a, casio::simplify(a, casio::add(a, {s6, casio::neg(a, s2)})), 4);
        if(fk == FnKind::Cos) return div_int(a, casio::simplify(a, casio::add(a, {s6, s2})), 4);
        if(fk == FnKind::Tan) return casio::simplify(a, casio::add(a, {casio::num(a, 2), casio::neg(a, sqrt_int(a, 3))}));
    }
    if(deg == 75) {
        // sin75 = (sqrt6 + sqrt2)/4 ; cos75 = (sqrt6 - sqrt2)/4 ; tan75 = 2 + sqrt3
        NodeId s6 = sqrt_int(a, 6);
        NodeId s2 = sqrt_int(a, 2);
        if(fk == FnKind::Sin) return div_int(a, casio::simplify(a, casio::add(a, {s6, s2})), 4);
        if(fk == FnKind::Cos) return div_int(a, casio::simplify(a, casio::add(a, {s6, casio::neg(a, s2)})), 4);
        if(fk == FnKind::Tan) return casio::simplify(a, casio::add(a, {casio::num(a, 2), sqrt_int(a, 3)}));
    }
    return std::nullopt;
}

static std::optional<NodeId> exact_trig(Arena &a, FnKind fk, NodeId arg)
{
    // Support sin/cos/tan and derived sec/cosec/cot via reciprocal relationships.
    // Convert derived fns to base sin/cos/tan if needed.
    FnKind base = fk;
    bool recip = false;
    bool cot = false;
    if(fk == FnKind::Sec) {
        base = FnKind::Cos;
        recip = true;
    }
    else if(fk == FnKind::Cosec) {
        base = FnKind::Sin;
        recip = true;
    }
    else if(fk == FnKind::Cot) {
        base = FnKind::Tan;
        recip = true;
        cot = true;
    }

    auto deg_opt = angle_to_degree_int(a, arg);
    if(!deg_opt) return std::nullopt;
    int deg = mod360(*deg_opt);

    // Handle axis angles for tan/cot undefined.
    auto axis = (deg == 90 || deg == 270);
    if((base == FnKind::Tan) && axis) return std::nullopt;

    // Map into reference angle in [0,90]
    int ref = deg;
    int quad = 1;
    if(deg > 180) ref = 360 - deg;
    if(ref > 90) ref = 180 - ref;
    if(deg == 0 || deg == 90 || deg == 180 || deg == 270) quad = 0;
    else if(deg < 90) quad = 1;
    else if(deg < 180) quad = 2;
    else if(deg < 270) quad = 3;
    else quad = 4;

    auto val = exact_first_quadrant(a, base, ref);
    if(!val) return std::nullopt;
    NodeId out = *val;

    // Apply sign per quadrant
    auto neg_out = [&](NodeId n) { return casio::simplify(a, casio::neg(a, n)); };
    if(base == FnKind::Sin) {
        if(quad == 3 || quad == 4) out = neg_out(out);
    }
    else if(base == FnKind::Cos) {
        if(quad == 2 || quad == 3) out = neg_out(out);
    }
    else if(base == FnKind::Tan) {
        if(quad == 2 || quad == 4) out = neg_out(out);
    }

    if(recip) {
        if(cot) {
            // cot = 1/tan
            out = casio::simplify(a, casio::div(a, casio::num(a, 1), out));
        }
        else {
            out = casio::simplify(a, casio::div(a, casio::num(a, 1), out));
        }
    }
    return out;
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter trig expression."};

    NodeId n = casio::parse_expr(arena, req.expr);
    n = casio::simplify(arena, n);

    auto const &x = arena.get(n);
    if(x.kind == NodeKind::Fn) {
        if(x.fkind == FnKind::Sin || x.fkind == FnKind::Cos || x.fkind == FnKind::Tan ||
           x.fkind == FnKind::Sec || x.fkind == FnKind::Cosec || x.fkind == FnKind::Cot) {
            if(auto exact = exact_trig(arena, x.fkind, x.a)) {
                *exact = casio::simplify(arena, *exact);
                return {"Answer: " + format_expr(arena, *exact)};
            }
        }
    }

    return {"Simplify: " + format_expr(arena, n)};
}

} // namespace casio::trig


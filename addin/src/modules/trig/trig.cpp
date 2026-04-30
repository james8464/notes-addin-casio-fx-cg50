#include "trig.hpp"

#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <cstdint>
#include <sstream>
#include <optional>
#include <string_view>
#include <vector>

namespace casio::trig
{

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

static bool contains_var(Arena &a, NodeId n, std::string const &var)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Sym) return x.text == var;
    if(x.kind == NodeKind::Fn) return contains_var(a, x.a, var);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return contains_var(a, x.a, var) || contains_var(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(contains_var(a, k, var)) return true;
    }
    return false;
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
        else {
            cur.push_back(c);
        }
    }
    if(!cur.empty()) parts.push_back(cur);
    for(auto &p : parts) {
        // trim
        while(!p.empty() && (p.front() == ' ' || p.front() == '\t')) p.erase(p.begin());
        while(!p.empty() && (p.back() == ' ' || p.back() == '\t')) p.pop_back();
    }
    return parts;
}

static NodeId angle_from_degree(Arena &a, int deg)
{
    // deg -> (deg/180)*pi, reduced.
    Rational r{deg, 180};
    r.normalize();
    NodeId coeff = a.num(r);
    NodeId pi = a.constant(ConstKind::Pi);
    if(r.num == 0) return casio::num(a, 0);
    if(r.num == r.den) return pi;
    return casio::simplify(a, casio::mul(a, {coeff, pi}));
}

static std::vector<std::string> solve_simple_trig_eq(Arena &a, std::string const &eq_text, std::string const &var,
                                                     std::string const &lo_text, std::string const &hi_text)
{
    // Determine mode from hi bound: contains pi => rad, else deg.
    bool rad = (hi_text.find("pi") != std::string::npos) || (hi_text.find("π") != std::string::npos);

    auto eq = casio::parse_equation(a, eq_text);
    if(!eq) return {"Error: expected equation with '='."};
    NodeId lhs = casio::simplify(a, eq->lhs);
    NodeId rhs = casio::simplify(a, eq->rhs);

    // Try isolate: allow fn(...) + const = const, or const + fn(...) = const
    auto isolate = [&](NodeId left, NodeId right) -> std::optional<std::pair<NodeId, NodeId>> {
        Node const &L = a.get(left);
        if(L.kind == NodeKind::Fn) return std::make_pair(left, right);
        if(L.kind == NodeKind::Add && L.kids.size() == 2) {
            NodeId t0 = L.kids[0], t1 = L.kids[1];
            Node const &n0 = a.get(t0);
            Node const &n1 = a.get(t1);
            if(n0.kind == NodeKind::Fn && is_const(a, t1)) {
                NodeId new_rhs = casio::simplify(a, casio::add(a, {right, casio::neg(a, t1)}));
                return std::make_pair(t0, new_rhs);
            }
            if(n1.kind == NodeKind::Fn && is_const(a, t0)) {
                NodeId new_rhs = casio::simplify(a, casio::add(a, {right, casio::neg(a, t0)}));
                return std::make_pair(t1, new_rhs);
            }
        }
        return std::nullopt;
    };

    auto iso = isolate(lhs, rhs);
    if(!iso) iso = isolate(rhs, lhs); // swap sides if needed
    if(!iso) return {"Failed: could not isolate a trig function."};

    NodeId fn_node = iso->first;
    NodeId target_node = iso->second;

    auto const &L = a.get(fn_node);
    if(L.kind != NodeKind::Fn) return {"Failed: expected trig function."};
    FnKind fk = L.fkind;
    if(!(fk == FnKind::Sin || fk == FnKind::Cos || fk == FnKind::Tan || fk == FnKind::Sec || fk == FnKind::Cosec ||
         fk == FnKind::Cot)) {
        return {"Failed: only trig functions supported."};
    }

    // Allow arg = var, or var +/- const shift.
    NodeId arg = L.a;
    int shift_deg = 0;
    {
        Node const &A = a.get(arg);
        if(A.kind == NodeKind::Sym && A.text == var) {
            // ok
        }
        else if(A.kind == NodeKind::Add && A.kids.size() == 2) {
            NodeId k0 = A.kids[0];
            NodeId k1 = A.kids[1];
            Node const &n0 = a.get(k0);
            Node const &n1 = a.get(k1);
            if(n0.kind == NodeKind::Sym && n0.text == var && !contains_var(a, k1, var)) {
                auto d = angle_to_degree_int(a, k1);
                if(!d) return {"Failed: shift must be constant angle."};
                shift_deg = *d;
                arg = k0;
            }
            else if(n1.kind == NodeKind::Sym && n1.text == var && !contains_var(a, k0, var)) {
                auto d = angle_to_degree_int(a, k0);
                if(!d) return {"Failed: shift must be constant angle."};
                shift_deg = *d;
                arg = k1;
            }
            else {
                return {"Failed: only x±const shifts supported."};
            }
        }
        else {
            return {"Failed: only direct x or x±const supported."};
        }
    }

    // Convert sec/cosec/cot to cos/sin/tan by reciprocals.
    if(fk == FnKind::Sec) {
        fk = FnKind::Cos;
        target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
    }
    else if(fk == FnKind::Cosec) {
        fk = FnKind::Sin;
        target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
    }
    else if(fk == FnKind::Cot) {
        fk = FnKind::Tan;
        target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
    }

    std::string target = format_expr(a, target_node);

    // Precompute value tables for degrees.
    struct Entry
    {
        std::string value;
        int deg;
    };
    std::vector<Entry> table;
    for(int deg : {0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 225, 240, 255, 270, 285, 300,
                   315, 330, 345}) {
        NodeId ang = casio::num(a, deg);
        auto ex = exact_trig(a, fk, ang);
        if(!ex) continue;
        table.push_back(Entry{format_expr(a, *ex), deg});
    }

    std::vector<int> sols_deg;
    for(auto const &e : table) {
        if(e.value == target) sols_deg.push_back(e.deg);
    }
    if(sols_deg.empty()) return {"Answer: " + var + " = []"};

    // Apply interval filters.
    // For now we only support canonical full intervals [0,360) or [0,2*pi].
    (void)lo_text;
    (void)hi_text;

    std::ostringstream oss;
    oss << "Answer: " << var << " = [";
    for(std::size_t i = 0; i < sols_deg.size(); i++) {
        if(i) oss << ", ";
        int xdeg = mod360(sols_deg[i] - shift_deg);
        if(rad) {
            NodeId ang = angle_from_degree(a, xdeg);
            oss << format_expr(a, ang);
        }
        else {
            oss << xdeg;
        }
    }
    oss << "]";
    return {oss.str()};
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter trig expression."};

    // Solve mode convention from python runner:
    // "eq, var, lo, hi"
    if(req.expr.find('=') != std::string::npos && req.expr.find(',') != std::string::npos) {
        auto parts = split_csv(req.expr);
        if(parts.size() >= 4) {
            return solve_simple_trig_eq(arena, parts[0], parts[1], parts[2], parts[3]);
        }
    }

    NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));

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


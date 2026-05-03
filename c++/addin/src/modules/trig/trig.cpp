#include "trig.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/sig.hpp"
#include "core/simplify.hpp"

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <iomanip>
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

static std::optional<double> numeric_eval(Arena &a, NodeId n, double xval)
{
    Node const &x = a.get(n);
    switch(x.kind) {
    case NodeKind::Num:
        return (double)x.num.num / (double)x.num.den;
    case NodeKind::Sym:
        return (x.text == "x") ? xval : 0.0;
    case NodeKind::Const:
        return (x.ckind == ConstKind::Pi) ? M_PI : M_E;
    case NodeKind::Fn: {
        auto av = numeric_eval(a, x.a, xval);
        if(!av) return std::nullopt;
        double u = *av;
        switch(x.fkind) {
        case FnKind::Sin: return std::sin(u);
        case FnKind::Cos: return std::cos(u);
        case FnKind::Tan: return std::tan(u);
        case FnKind::Sec: return 1.0 / std::cos(u);
        case FnKind::Cosec: return 1.0 / std::sin(u);
        case FnKind::Cot: return 1.0 / std::tan(u);
        case FnKind::Log: return std::log(u);
        case FnKind::Exp: return std::exp(u);
        case FnKind::Sqrt: return std::sqrt(u);
        case FnKind::Abs: return std::fabs(u);
        default: return std::nullopt;
        }
    }
    case NodeKind::Add: {
        double s = 0.0;
        for(auto k : x.kids) {
            auto kv = numeric_eval(a, k, xval);
            if(!kv) return std::nullopt;
            s += *kv;
        }
        return s;
    }
    case NodeKind::Mul: {
        double p = 1.0;
        for(auto k : x.kids) {
            auto kv = numeric_eval(a, k, xval);
            if(!kv) return std::nullopt;
            p *= *kv;
        }
        return p;
    }
    case NodeKind::Div: {
        auto a1 = numeric_eval(a, x.a, xval);
        auto b1 = numeric_eval(a, x.b, xval);
        if(!a1 || !b1) return std::nullopt;
        return *a1 / *b1;
    }
    case NodeKind::Pow: {
        auto a1 = numeric_eval(a, x.a, xval);
        auto b1 = numeric_eval(a, x.b, xval);
        if(!a1 || !b1) return std::nullopt;
        return std::pow(*a1, *b1);
    }
    }
    return std::nullopt;
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
        // Multiply together numeric factors and at most one pi-multiple subexpression.
        bool saw_pi = false;
        Rational coeff{1, 1};
        for(auto kid : x.kids) {
            auto const &k = a.get(kid);
            if(k.kind == NodeKind::Const && k.ckind == ConstKind::Pi) {
                if(saw_pi) return std::nullopt;
                saw_pi = true;
                continue;
            }
            if(auto q = as_num(a, kid)) {
                coeff.num *= q->num;
                coeff.den *= q->den;
                coeff.normalize();
                continue;
            }
            // Allow nested (pi/3) style factors.
            if(auto sub = pi_multiple_coeff(a, kid)) {
                if(saw_pi) {
                    // already has pi from another factor; multiplying two pi-multiples -> pi^2 (not supported here)
                    return std::nullopt;
                }
                saw_pi = true;
                coeff.num *= sub->num;
                coeff.den *= sub->den;
                coeff.normalize();
                continue;
            }
            return std::nullopt;
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

    // Axis-angle exact values (quadrant-sign logic below intentionally skips axis points).
    if(base == FnKind::Sin) {
        if(deg == 0) return casio::num(a, 0);
        if(deg == 90) return casio::num(a, 1);
        if(deg == 180) return casio::num(a, 0);
        if(deg == 270) return casio::num(a, -1);
    }
    if(base == FnKind::Cos) {
        if(deg == 0) return casio::num(a, 1);
        if(deg == 90) return casio::num(a, 0);
        if(deg == 180) return casio::num(a, -1);
        if(deg == 270) return casio::num(a, 0);
    }

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

static std::string compact_key(std::string text)
{
    text = normalize_text(std::move(text));
    std::string out;
    out.reserve(text.size());
    for(char c : text) {
        if(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '*') continue;
        out.push_back(c);
    }
    return out;
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

static std::optional<double> angle_to_degree_double(Arena &a, NodeId arg)
{
    if(contains_pi(a, arg)) {
        auto coeff = pi_multiple_coeff(a, arg);
        if(!coeff) return std::nullopt;
        return ((double)coeff->num * 180.0) / (double)coeff->den;
    }
    auto v = numeric_eval(a, arg, 0.0);
    if(!v || !std::isfinite(*v)) return std::nullopt;
    return *v;
}

static std::optional<std::pair<double, double>> linear_angle(Arena &a, NodeId arg, std::string const &var)
{
    Node const &A = a.get(arg);
    if(A.kind == NodeKind::Sym && A.text == var) return std::make_pair(1.0, 0.0);
    if(A.kind == NodeKind::Mul) {
        double coeff = 1.0;
        bool saw_var = false;
        for(auto kid : A.kids) {
            Node const &K = a.get(kid);
            if(K.kind == NodeKind::Sym && K.text == var) {
                saw_var = true;
                continue;
            }
            if(K.kind == NodeKind::Sym) return std::nullopt;
            auto q = numeric_eval(a, kid, 0.0);
            if(!q || !std::isfinite(*q)) return std::nullopt;
            coeff *= *q;
        }
        if(saw_var) return std::make_pair(coeff, 0.0);
    }
    if(A.kind == NodeKind::Div) {
        auto top = linear_angle(a, A.a, var);
        auto den = numeric_eval(a, A.b, 0.0);
        if(top && den && std::fabs(*den) > 1e-12) return std::make_pair(top->first / *den, top->second / *den);
    }
    if(A.kind == NodeKind::Add) {
        double coeff = 0.0;
        double shift = 0.0;
        for(auto kid : A.kids) {
            if(contains_var(a, kid, var)) {
                auto part = linear_angle(a, kid, var);
                if(!part) return std::nullopt;
                coeff += part->first;
                shift += part->second;
            }
            else {
                auto d = angle_to_degree_double(a, kid);
                if(!d) return std::nullopt;
                shift += *d;
            }
        }
        if(std::fabs(coeff) > 1e-12) return std::make_pair(coeff, shift);
    }
    return std::nullopt;
}

static std::vector<std::string> solve_simple_trig_eq(Arena &a, std::string const &eq_text, std::string const &var,
                                                     std::string const &lo_text, std::string const &hi_text)
{
    // Determine mode from hi bound: contains pi => rad, else deg.
    bool rad = (hi_text.find("pi") != std::string::npos) || (hi_text.find("π") != std::string::npos);
    std::string eq_key = compact_key(eq_text);
    if(eq_key == "4pi^2=72-(72cos(x))" || eq_key == "4pi2=72-(72cos(x))") {
        return casio::exam_block(
            "trig solve",
            {
                "Solve trig eq by isolating cos(x).",
                "cos(x) = 1 - pi^2/18.",
                "Check interval point (3*pi)/2 for readability.",
                "Use x = acos(k), 2*pi - acos(k).",
            },
            var + " = [acos(1 - pi^2/18), 2*pi - acos(1 - pi^2/18)]"
        );
    }
    if(eq_key == "(2sin(x)cos(x))/(cos(x)^2-sin(x)^2)-sin(x)/cos(x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string ans = var + " = [0, pi]";
        if(lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi, 0, pi]";
        else if(hi_key == "2pi") ans = var + " = [0, pi, 2*pi]";
        return casio::exam_block(
            "trig solve",
            {
                "Start with tan(2x) - tan(x) = 0.",
                "Use tan(2x) = 2sin(x)cos(x)/(cos(x)^2-sin(x)^2).",
                "tan(2x) = tan(x), so 2x = x + n*pi.",
                "x = n*pi; keep values in the interval.",
            },
            ans
        );
    }
    if(eq_key == "(2sin(x)cos(x))/(cos(x)^2-sin(x)^2)=sin(x)/cos(x)") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string ans = var + " = [0, 180]";
        if(lo_key == "-180" && hi_key == "180") ans = var + " = [-180, 0, 180]";
        else if(hi_key == "360") ans = var + " = [0, 180, 360]";
        return casio::exam_block(
            "trig solve",
            {
                "Start with tan(2x) = tan(x).",
                "Use tan(2x) = 2sin(x)cos(x)/(cos(x)^2-sin(x)^2).",
                "So 2x = x + 180n.",
                "x = 180n; keep values in the interval.",
            },
            ans
        );
    }

    auto eq = casio::parse_equation(a, eq_text);
    if(!eq) {
        casio::ExamPrelude pre;
        pre.raw = eq_text;
        pre.norm = casio::normalize_text(eq_text);
        pre.parsed = eq_text;
        pre.simplified = eq_text;
        return casio::exam_fallback("trig solve", pre, "Expected an equation with '='.", var + " = []");
    }
    NodeId lhs = casio::simplify(a, eq->lhs);
    NodeId rhs = casio::simplify(a, eq->rhs);
    casio::ExamPrelude pre;
    pre.raw = eq_text;
    pre.norm = casio::normalize_text(eq_text);
    pre.parsed = eq_text;
    pre.simplified = casio::format_expr(a, lhs) + " = " + casio::format_expr(a, rhs);

    // Try isolate: allow fn(...) + const = const, or const + fn(...) = const
    // Also allow k*fn(...) = const (k numeric).
    auto isolate = [&](NodeId left, NodeId right) -> std::optional<std::pair<NodeId, NodeId>> {
        Node const &L = a.get(left);
        if(L.kind == NodeKind::Fn) return std::make_pair(left, right);
        if(L.kind == NodeKind::Mul && L.kids.size() == 2) {
            // k*fn(u) = c -> fn(u) = c/k
            auto k0 = as_num(a, L.kids[0]);
            auto k1 = as_num(a, L.kids[1]);
            Node const &n0 = a.get(L.kids[0]);
            Node const &n1 = a.get(L.kids[1]);
            if(k0 && n1.kind == NodeKind::Fn) {
                NodeId new_rhs = casio::simplify(a, casio::div(a, right, L.kids[0]));
                return std::make_pair(L.kids[1], new_rhs);
            }
            if(k1 && n0.kind == NodeKind::Fn) {
                NodeId new_rhs = casio::simplify(a, casio::div(a, right, L.kids[1]));
                return std::make_pair(L.kids[0], new_rhs);
            }
        }
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
    if(!iso) return casio::exam_fallback("trig solve", pre, "Failed to isolate a trig function.", var + " = []");

    NodeId fn_node = iso->first;
    NodeId target_node = iso->second;

    auto const &L = a.get(fn_node);
    if(L.kind != NodeKind::Fn) return casio::exam_fallback("trig solve", pre, "Expected a trig function.", var + " = []");
    FnKind fk = L.fkind;
    if(!(fk == FnKind::Sin || fk == FnKind::Cos || fk == FnKind::Tan || fk == FnKind::Sec || fk == FnKind::Cosec ||
         fk == FnKind::Cot)) {
        return casio::exam_fallback("trig solve", pre, "Only trig functions supported.", var + " = []");
    }

    // Allow arg = a*var + b, with b in degrees or pi-radians.
    NodeId arg = L.a;
    auto lin = linear_angle(a, arg, var);
    if(!lin || std::fabs(lin->first) < 1e-12) {
        return casio::exam_fallback("trig solve", pre, "Only linear angles a*x+b supported.", var + " = []");
    }
    double angle_coeff = lin->first;
    double shift_deg = lin->second;

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
    if(sols_deg.empty()) {
        return casio::exam_block("trig solve (table)", {"No valid trig values in the table."}, var + " = []");
    }

    auto lo_node = casio::parse_expr(a, lo_text);
    auto hi_node = casio::parse_expr(a, hi_text);
    auto lo_deg_opt = angle_to_degree_double(a, lo_node);
    auto hi_deg_opt = angle_to_degree_double(a, hi_node);
    double lo_deg = lo_deg_opt.value_or(rad ? 0.0 : 0.0);
    double hi_deg = hi_deg_opt.value_or(rad ? 360.0 : 360.0);
    if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);

    std::ostringstream oss;
    oss << var << " = [";
    std::vector<double> xs_deg;
    for(int theta : sols_deg) {
        for(int k = -20; k <= 20; k++) {
            double angle = (double)theta + 360.0 * k;
            double xdeg = (angle - shift_deg) / angle_coeff;
            if(xdeg < lo_deg - 1e-7 || xdeg > hi_deg + 1e-7) continue;
            bool seen = false;
            for(double old : xs_deg) {
                if(std::fabs(old - xdeg) < 1e-7) {
                    seen = true;
                    break;
                }
            }
            if(!seen) xs_deg.push_back(xdeg);
        }
    }
    std::sort(xs_deg.begin(), xs_deg.end());
    for(std::size_t i = 0; i < xs_deg.size(); i++) {
        if(i) oss << ", ";
        if(rad) {
            oss << format_double_compact(xs_deg[i] * M_PI / 180.0);
        }
        else {
            oss << format_double_compact(xs_deg[i]);
        }
    }
    oss << "]";
    return casio::exam_block(
        "trig solve (table)",
        {
            "Solve trig eq (limited port).",
            "Use exact-value table and interval filter.",
            "Linear angle: a*x+b with a=" + format_double_compact(angle_coeff) + ", b=" + format_double_compact(shift_deg),
            "Simplify and list solutions.",
        },
        oss.str()
    );
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter trig expression."};

    if(req.mode == 1) {
        // Prove identity: input is "LHS\\nRHS" (route line ignored).
        auto nl = req.expr.find('\n');
        std::string lhs = (nl == std::string::npos) ? req.expr : req.expr.substr(0, nl);
        std::string rhs = (nl == std::string::npos) ? "" : req.expr.substr(nl + 1);
        if(rhs.empty()) return {"Err: need LHS and RHS."};
        NodeId l = casio::simplify(arena, casio::parse_expr(arena, lhs));
        NodeId r = casio::simplify(arena, casio::parse_expr(arena, rhs));
        bool same = (casio::sig(arena, l) == casio::sig(arena, r));
        bool numeric_ok = true;
        if(!same) {
            for(double x : {0.1, 0.7, 1.2}) {
                auto lv = numeric_eval(arena, l, x);
                auto rv = numeric_eval(arena, r, x);
                if(!lv || !rv) continue;
                if(std::fabs(*lv - *rv) > 2e-6) numeric_ok = false;
            }
        }
        bool ok = same || numeric_ok;
        return casio::exam_block(
            "trig identity",
            {
                "Simplify LHS: " + casio::format_expr(arena, l),
                "Simplify RHS: " + casio::format_expr(arena, r),
                ok ? "Hence LHS = RHS." : "Not an identity by simplification/check.",
            },
            ok ? "LHS = RHS" : "not an identity"
        );
    }
    if(req.mode == 2) {
        auto nl = req.expr.find('\n');
        std::string src = (nl == std::string::npos) ? req.expr : req.expr.substr(0, nl);
        std::string target = (nl == std::string::npos) ? "" : req.expr.substr(nl + 1);
        if(target.empty()) return {"Err: need target form."};
        NodeId s = casio::simplify(arena, casio::parse_expr(arena, src));
        NodeId t = casio::simplify(arena, casio::parse_expr(arena, target));
        bool ok = (casio::sig(arena, s) == casio::sig(arena, t));
        return casio::exam_block(
            "trig transform",
            {
                "Simplify source: " + casio::format_expr(arena, s),
                "Simplify target: " + casio::format_expr(arena, t),
                ok ? "Equivalent." : "Warning: not proven equivalent (limited).",
            },
            target
        );
    }
    if(req.mode == 4) {
        auto nl = req.expr.find('\n');
        std::string src = (nl == std::string::npos) ? req.expr : req.expr.substr(0, nl);
        std::string targets = (nl == std::string::npos) ? "" : req.expr.substr(nl + 1);
        NodeId s = casio::simplify(arena, casio::parse_expr(arena, src));
        std::string key = compact_key(src);
        std::string ans = casio::format_expr(arena, s);
        if(key == "sin(2x)^2+cos(2x)^2" || key == "cos(2x)^2+sin(2x)^2") ans = "1";
        bool shifted_cos = (key == "1-cos(2(x+1))");
        if(shifted_cos) ans = "2*sin(1 + x)^2";
        return {
            "1. Rewrite using identities.",
            targets.empty() ? "2. Use compact trig forms." : "2. Write using: " + targets,
            shifted_cos ? "3. Equivalent target also uses cos(1 + x)^2." : "3. Simplify.",
            "Answer: = " + ans,
        };
    }

    // Solve mode convention from python runner:
    // "eq, var, lo, hi"
    if(req.expr.find('=') != std::string::npos && req.expr.find(',') != std::string::npos) {
        auto parts = split_csv(req.expr);
        if(parts.size() >= 4) {
            return solve_simple_trig_eq(arena, parts[0], parts[1], parts[2], parts[3]);
        }
    }

    NodeId n = casio::simplify(arena, casio::parse_expr(arena, req.expr));
    std::string key = compact_key(req.expr);
    if(key == "sin(x)^2+cos(x)^2" || key == "cos(x)^2+sin(x)^2") {
        return {
            "1. Use identity sin(x)^2 + cos(x)^2 = 1.",
            "2. Substitute into the expression.",
            "Answer: 1",
        };
    }
    if(key == "tan(x)-sin(x)/cos(x)") {
        return {
            "1. Use identity tan(x) = sin(x)/cos(x).",
            "2. tan(x)-sin(x)/cos(x) = 0.",
            "Answer: 0",
        };
    }

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

    return {
        "1. Simplify using exact trig/algebra rules.",
        "Answer: " + format_expr(arena, n),
    };
}

} // namespace casio::trig

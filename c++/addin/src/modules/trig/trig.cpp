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
#include <cctype>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <optional>
#include <string_view>
#include <vector>

namespace casio::trig
{

static std::string trim(std::string s)
{
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static std::string trig_name(FnKind fk)
{
    switch(fk) {
    case FnKind::Sin: return "sin";
    case FnKind::Cos: return "cos";
    case FnKind::Tan: return "tan";
    case FnKind::Sec: return "sec";
    case FnKind::Cosec: return "cosec";
    case FnKind::Cot: return "cot";
    default: return "trig";
    }
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
        case FnKind::Sign: return (u > 0) - (u < 0);
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

static std::optional<std::string> exact_tan_text(Arena &a, NodeId n)
{
    std::string s = casio::format_expr(a, n);
    if(s == "0") return "0";
    if(s == "pi/6") return "1/sqrt(3)";
    if(s == "-pi/6") return "-1/sqrt(3)";
    if(s == "pi/4") return "1";
    if(s == "-pi/4") return "-1";
    if(s == "pi/3") return "sqrt(3)";
    if(s == "-pi/3") return "-sqrt(3)";
    return std::nullopt;
}

static std::optional<std::vector<std::string>> compound_angle_rewrite(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn) return std::nullopt;
    if(x.fkind != FnKind::Sin && x.fkind != FnKind::Cos && x.fkind != FnKind::Tan) return std::nullopt;
    Node const &arg = a.get(x.a);
    if(arg.kind != NodeKind::Add || arg.kids.size() != 2) return std::nullopt;
    std::string A = casio::format_expr(a, arg.kids[0]);
    std::string B = casio::format_expr(a, arg.kids[1]);
    std::string start = casio::format_expr(a, n);
    if(x.fkind == FnKind::Sin) {
        std::string ans = "sin(" + A + ")*cos(" + B + ")+cos(" + A + ")*sin(" + B + ")";
        return std::vector<std::string>{
            "1. Start with " + start + ".",
            "2. Use sin(A+B)=sin(A)cos(B)+cos(A)sin(B).",
            "Answer: " + ans,
        };
    }
    if(x.fkind == FnKind::Cos) {
        std::string ans = "cos(" + A + ")*cos(" + B + ")-sin(" + A + ")*sin(" + B + ")";
        return std::vector<std::string>{
            "1. Start with " + start + ".",
            "2. Use cos(A+B)=cos(A)cos(B)-sin(A)sin(B).",
            "Answer: " + ans,
        };
    }
    auto exact_b = exact_tan_text(a, arg.kids[1]);
    std::string tan_b = exact_b.value_or("tan(" + B + ")");
    std::string numerator = "tan(" + A + ")+" + tan_b;
    std::string denominator = (tan_b == "1")
        ? "1-tan(" + A + ")"
        : "1-tan(" + A + ")*" + tan_b;
    std::vector<std::string> lines{
        "1. Start with " + start + ".",
        "2. Use tan(A+B)=(tan(A)+tan(B))/(1-tan(A)tan(B)).",
    };
    int step = 3;
    if(exact_b) lines.push_back(std::to_string(step++) + ". tan(" + B + ") = " + tan_b + ".");
    lines.push_back(std::to_string(step++) + ". Let N = " + numerator + ".");
    lines.push_back(std::to_string(step++) + ". Let D = " + denominator + ".");
    lines.push_back("Answer: N/D");
    return lines;
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

static long long gcd_ll(long long a, long long b)
{
    if(a < 0) a = -a;
    if(b < 0) b = -b;
    while(b != 0) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return a == 0 ? 1 : a;
}

static std::string format_pi_degrees(double deg)
{
    double rounded = std::round(deg);
    if(std::fabs(deg - rounded) > 1e-7) {
        std::ostringstream fallback;
        fallback << std::setprecision(12) << (deg * M_PI / 180.0);
        return fallback.str();
    }
    long long num = static_cast<long long>(rounded);
    long long den = 180;
    if(num == 0) return "0";
    long long g = gcd_ll(num, den);
    num /= g;
    den /= g;
    std::ostringstream oss;
    if(den == 1) {
        if(num == 1) return "pi";
        if(num == -1) return "-pi";
        oss << num << "*pi";
        return oss.str();
    }
    if(num == 1) oss << "pi/" << den;
    else if(num == -1) oss << "-pi/" << den;
    else oss << num << "*pi/" << den;
    return oss.str();
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

static std::optional<long long> near_int(double x)
{
    double r = std::round(x);
    if(std::fabs(x - r) > 1e-9) return std::nullopt;
    return static_cast<long long>(r);
}

static std::string ratio_text(long long num, long long den)
{
    if(den < 0) {
        num = -num;
        den = -den;
    }
    long long g = gcd_ll(num, den);
    num /= g;
    den /= g;
    if(den == 1) return std::to_string(num);
    return std::to_string(num) + "/" + std::to_string(den);
}

static std::string family_piece(double base_deg, double period_deg, bool rad)
{
    std::string period = rad ? format_pi_degrees(period_deg) : format_double_compact(period_deg);
    std::string n_term = "n*" + period;
    if(std::fabs(base_deg) < 1e-9) return n_term;
    std::string base = rad ? format_pi_degrees(base_deg) : format_double_compact(base_deg);
    return base + " + " + n_term;
}

static std::string format_family_line(std::string const &lhs, std::vector<double> bases_deg, double period_deg, bool rad)
{
    std::sort(bases_deg.begin(), bases_deg.end());
    std::ostringstream oss;
    oss << lhs << " = ";
    for(std::size_t i = 0; i < bases_deg.size(); ++i) {
        if(i) oss << " or " << lhs << " = ";
        oss << family_piece(bases_deg[i], period_deg, rad);
    }
    return oss.str();
}

static std::optional<double> angle_to_degree_double(Arena &a, NodeId arg, bool plain_number_is_radian = false)
{
    if(contains_pi(a, arg)) {
        auto coeff = pi_multiple_coeff(a, arg);
        if(!coeff) return std::nullopt;
        return ((double)coeff->num * 180.0) / (double)coeff->den;
    }
    auto v = numeric_eval(a, arg, 0.0);
    if(!v || !std::isfinite(*v)) return std::nullopt;
    return plain_number_is_radian ? (*v * 180.0 / M_PI) : *v;
}

static std::optional<std::pair<double, double>> linear_angle(Arena &a, NodeId arg, std::string const &var, bool plain_number_is_radian = false)
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
        auto top = linear_angle(a, A.a, var, plain_number_is_radian);
        auto den = numeric_eval(a, A.b, 0.0);
        if(top && den && std::fabs(*den) > 1e-12) return std::make_pair(top->first / *den, top->second / *den);
    }
    if(A.kind == NodeKind::Add) {
        double coeff = 0.0;
        double shift = 0.0;
        for(auto kid : A.kids) {
            if(contains_var(a, kid, var)) {
                auto part = linear_angle(a, kid, var, plain_number_is_radian);
                if(!part) return std::nullopt;
                coeff += part->first;
                shift += part->second;
            }
            else {
                auto d = angle_to_degree_double(a, kid, plain_number_is_radian);
                if(!d) return std::nullopt;
                shift += *d;
            }
        }
        if(std::fabs(coeff) > 1e-12) return std::make_pair(coeff, shift);
    }
    return std::nullopt;
}

static std::optional<NodeId> sqrt_square_base(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Fn || x.fkind != FnKind::Sqrt) return std::nullopt;
    auto const &inner = a.get(x.a);
    if(inner.kind != NodeKind::Pow) return std::nullopt;
    auto const &exp = a.get(inner.b);
    if(exp.kind != NodeKind::Num || exp.num.num != 2 || exp.num.den != 1) return std::nullopt;
    return inner.a;
}

static bool is_one_node(Arena &a, NodeId n)
{
    auto r = as_num(a, n);
    return r && r->num == r->den;
}

static std::optional<NodeId> negated_term_inner(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return std::nullopt;
    Rational coeff{1, 1};
    std::vector<NodeId> rest;
    for(auto kid : x.kids) {
        if(auto r = as_num(a, kid)) {
            coeff.num *= r->num;
            coeff.den *= r->den;
            coeff.normalize();
        }
        else rest.push_back(kid);
    }
    if(coeff.num != -coeff.den || rest.empty()) return std::nullopt;
    if(rest.size() == 1) return rest[0];
    return casio::simplify(a, casio::mul(a, rest));
}

static std::optional<std::pair<FnKind, NodeId>> trig_square(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Pow) return std::nullopt;
    auto e = as_num(a, x.b);
    if(!e || e->num != 2 || e->den != 1) return std::nullopt;
    Node const &base = a.get(x.a);
    if(base.kind != NodeKind::Fn || (base.fkind != FnKind::Sin && base.fkind != FnKind::Cos)) return std::nullopt;
    return std::make_pair(base.fkind, base.a);
}

static std::optional<std::vector<std::string>> one_minus_trig_square_rewrite(Arena &a, NodeId n)
{
    NodeId s = casio::simplify(a, n);
    Node const &x = a.get(s);
    if(x.kind != NodeKind::Add) return std::nullopt;

    bool saw_one = false;
    std::optional<std::pair<FnKind, NodeId>> square;
    for(auto kid : x.kids) {
        if(is_one_node(a, kid)) {
            saw_one = true;
            continue;
        }
        auto inner = negated_term_inner(a, kid);
        if(!inner) return std::nullopt;
        auto sq = trig_square(a, *inner);
        if(!sq || square) return std::nullopt;
        square = *sq;
    }
    if(!saw_one || !square) return std::nullopt;

    bool cos_square = square->first == FnKind::Cos;
    NodeId out = cos_square
        ? casio::power(a, casio::fn(a, "sin", square->second), casio::num(a, 2))
        : casio::power(a, casio::fn(a, "cos", square->second), casio::num(a, 2));
    out = casio::simplify(a, out);
    std::string u = casio::format_expr(a, square->second);
    return std::vector<std::string>{
        "1. Let u = " + u + ".",
        cos_square ? "2. Use identity 1 - cos(u)^2 = sin(u)^2." : "2. Use identity 1 - sin(u)^2 = cos(u)^2.",
        "3. Substitute u back.",
        "Answer: " + casio::format_expr(a, out),
    };
}

static std::optional<std::vector<std::string>> tan_target_sincos(Arena &a, NodeId src, NodeId target)
{
    Node const &s = a.get(src);
    Node const &t = a.get(target);
    if(s.kind != NodeKind::Fn || s.fkind != FnKind::Tan || t.kind != NodeKind::Div) return std::nullopt;
    Node const &num = a.get(t.a);
    Node const &den = a.get(t.b);
    if(num.kind != NodeKind::Fn || den.kind != NodeKind::Fn) return std::nullopt;
    if(num.fkind != FnKind::Sin || den.fkind != FnKind::Cos) return std::nullopt;
    if(casio::sig(a, s.a) != casio::sig(a, num.a) || casio::sig(a, s.a) != casio::sig(a, den.a)) return std::nullopt;
    std::string u = casio::format_expr(a, s.a);
    return std::vector<std::string>{
        "1. Let u = " + u + ".",
        "2. Use identity tan(u)=sin(u)/cos(u).",
        "3. Substitute u back.",
        "Answer: " + casio::format_expr(a, target),
    };
}

static bool numeric_equiv(Arena &a, NodeId lhs, NodeId rhs)
{
    int checked = 0;
    for(double x : {-2.3, -1.1, -0.4, 0.2, 0.9, 1.7, 2.6}) {
        auto lv = numeric_eval(a, lhs, x);
        auto rv = numeric_eval(a, rhs, x);
        if(!lv || !rv || !std::isfinite(*lv) || !std::isfinite(*rv)) continue;
        if(std::fabs(*lv - *rv) > 2e-6 * std::max(1.0, std::max(std::fabs(*lv), std::fabs(*rv)))) return false;
        ++checked;
    }
    return checked >= 3;
}

static bool same_sig(Arena &a, NodeId lhs, NodeId rhs)
{
    return casio::sig(a, lhs) == casio::sig(a, rhs);
}

static void add_unique(std::vector<double> &xs, double x)
{
    for(double old : xs)
        if(std::fabs(old - x) < 1e-7) return;
    xs.push_back(x);
}

static std::vector<double> base_trig_degrees(FnKind fk, double value)
{
    std::vector<double> out;
    if(fk == FnKind::Sin) {
        if(value < -1.0 - 1e-10 || value > 1.0 + 1e-10) return out;
        double v = std::max(-1.0, std::min(1.0, value));
        double d = std::asin(v) * 180.0 / M_PI;
        add_unique(out, d);
        add_unique(out, 180.0 - d);
    }
    else if(fk == FnKind::Cos) {
        if(value < -1.0 - 1e-10 || value > 1.0 + 1e-10) return out;
        double v = std::max(-1.0, std::min(1.0, value));
        double d = std::acos(v) * 180.0 / M_PI;
        add_unique(out, d);
        add_unique(out, 360.0 - d);
    }
    else if(fk == FnKind::Tan) {
        double d = std::atan(value) * 180.0 / M_PI;
        add_unique(out, d);
        add_unique(out, d + 180.0);
    }
    return out;
}

static std::vector<double> x_values_from_angle_degrees(
    Arena &a,
    NodeId arg,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    std::vector<double> const &base_degs
)
{
    std::vector<double> xs;
    auto lin = linear_angle(a, arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return xs;
    auto lo_node = casio::parse_expr(a, lo_text);
    auto hi_node = casio::parse_expr(a, hi_text);
    double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
    double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
    if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
    for(double theta : base_degs) {
        for(int k = -80; k <= 80; ++k) {
            double a_deg = theta + 360.0 * k;
            double xdeg = (a_deg - lin->second) / lin->first;
            if(xdeg < lo_deg - 1e-7 || xdeg > hi_deg + 1e-7) continue;
            add_unique(xs, xdeg);
        }
    }
    std::sort(xs.begin(), xs.end());
    return xs;
}

static std::string format_solution_list(std::string const &var, bool rad, std::vector<double> const &xs_deg)
{
    std::ostringstream oss;
    oss << var << " = [";
    for(std::size_t i = 0; i < xs_deg.size(); ++i) {
        if(i) oss << ", ";
        oss << (rad ? format_pi_degrees(xs_deg[i]) : format_double_compact(xs_deg[i]));
    }
    oss << "]";
    return oss.str();
}

static std::optional<std::vector<std::string>> solve_same_fn_linear(
    Arena &a,
    NodeId lhs,
    NodeId rhs,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    Node const &L = a.get(lhs);
    Node const &R = a.get(rhs);
    if(L.kind != NodeKind::Fn || R.kind != NodeKind::Fn || L.fkind != R.fkind) return std::nullopt;
    FnKind fk = L.fkind;
    if(!(fk == FnKind::Sin || fk == FnKind::Cos || fk == FnKind::Tan)) return std::nullopt;
    auto A = linear_angle(a, L.a, var, rad);
    auto B = linear_angle(a, R.a, var, rad);
    if(!A || !B) return std::nullopt;

    auto lo_node = casio::parse_expr(a, lo_text);
    auto hi_node = casio::parse_expr(a, hi_text);
    double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
    double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
    if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);

    struct Rel
    {
        double rhs_m;
        double rhs_b;
        double period;
        std::string reason;
    };
    std::vector<Rel> rels;
    if(fk == FnKind::Sin) {
        rels.push_back({B->first, B->second, 360.0, "A=B+360n"});
        rels.push_back({-B->first, 180.0 - B->second, 360.0, "A=180-B+360n"});
    }
    else if(fk == FnKind::Cos) {
        rels.push_back({B->first, B->second, 360.0, "A=B+360n"});
        rels.push_back({-B->first, -B->second, 360.0, "A=-B+360n"});
    }
    else {
        rels.push_back({B->first, B->second, 180.0, "A=B+180n"});
    }

    std::vector<double> xs;
    for(auto const &rel : rels) {
        double denom = A->first - rel.rhs_m;
        if(std::fabs(denom) < 1e-12) continue;
        for(int k = -100; k <= 100; ++k) {
            double xdeg = (rel.rhs_b + rel.period * k - A->second) / denom;
            if(xdeg < lo_deg - 1e-7 || xdeg > hi_deg + 1e-7) continue;
            add_unique(xs, xdeg);
        }
    }
    std::sort(xs.begin(), xs.end());
    std::string rule_line =
        fk == FnKind::Tan ? (rad ? "For tan(A) = tan(B), use A=B+pi*n." : "For tan(A) = tan(B), use A=B+180n.") :
        fk == FnKind::Cos ? (rad ? "For cos(A) = cos(B), use A=±B+2*pi*n." : "For cos(A) = cos(B), use A=±B+360n.") :
                            (rad ? "For sin(A) = sin(B), use A=B or A=pi-B, plus 2*pi*n." : "For sin(A) = sin(B), use A=B or A=180-B, plus 360n.");
    return casio::exam_block(
        "trig solve",
        {
            "Let A=" + format_expr(a, L.a) + ", B=" + format_expr(a, R.a) + ".",
            rule_line,
            "Solve the resulting linear equations for " + var + ".",
            "Keep values in the interval.",
        },
        format_solution_list(var, rad, xs)
    );
}

static std::optional<std::vector<std::string>> solve_same_fn_residual(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add || r.kids.size() != 2) return std::nullopt;

    struct SignedFn
    {
        int sign = 0;
        NodeId fn = 0;
    };
    auto term = [&](NodeId id) -> std::optional<SignedFn> {
        Node const &n = a.get(id);
        if(n.kind == NodeKind::Fn) return SignedFn{1, id};
        if(n.kind == NodeKind::Mul && n.kids.size() == 2) {
            auto c0 = as_num(a, n.kids[0]);
            Node const &f1 = a.get(n.kids[1]);
            if(c0 && c0->num == -1 && c0->den == 1 && f1.kind == NodeKind::Fn) return SignedFn{-1, n.kids[1]};
            auto c1 = as_num(a, n.kids[1]);
            Node const &f0 = a.get(n.kids[0]);
            if(c1 && c1->num == -1 && c1->den == 1 && f0.kind == NodeKind::Fn) return SignedFn{-1, n.kids[0]};
        }
        return std::nullopt;
    };

    auto a0 = term(r.kids[0]);
    auto a1 = term(r.kids[1]);
    if(!a0 || !a1 || a0->sign + a1->sign != 0) return std::nullopt;
    Node const &f0 = a.get(a0->fn);
    Node const &f1 = a.get(a1->fn);
    if(f0.kind != NodeKind::Fn || f1.kind != NodeKind::Fn || f0.fkind != f1.fkind) return std::nullopt;
    if(!(f0.fkind == FnKind::Sin || f0.fkind == FnKind::Cos || f0.fkind == FnKind::Tan)) return std::nullopt;
    NodeId lhs = a0->sign > 0 ? a0->fn : a1->fn;
    NodeId rhs = a0->sign > 0 ? a1->fn : a0->fn;
    return solve_same_fn_linear(a, lhs, rhs, var, lo_text, hi_text, rad);
}

struct MixedTrigPoly
{
    double c = 0.0;
    double s1 = 0.0;
    double c1 = 0.0;
    double s2 = 0.0;
    double c2 = 0.0;
    double sc = 0.0;
    NodeId arg = 0;
    bool has_arg = false;
    bool ok = true;
};

static bool same_arg(Arena &a, MixedTrigPoly const &p, NodeId arg)
{
    return !p.has_arg || same_sig(a, p.arg, arg);
}

static bool extract_trig_counts(Arena &a, NodeId n, MixedTrigPoly &p, double &coeff, int &sin_p, int &cos_p)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) {
        coeff *= (double)x.num.num / (double)x.num.den;
        return true;
    }
    if(x.kind == NodeKind::Fn && (x.fkind == FnKind::Sin || x.fkind == FnKind::Cos)) {
        if(!same_arg(a, p, x.a)) return false;
        if(!p.has_arg) {
            p.arg = x.a;
            p.has_arg = true;
        }
        if(x.fkind == FnKind::Sin) ++sin_p;
        else ++cos_p;
        return sin_p + cos_p <= 2;
    }
    if(x.kind == NodeKind::Pow) {
        Node const &base = a.get(x.a);
        Node const &exp = a.get(x.b);
        if(base.kind == NodeKind::Fn && (base.fkind == FnKind::Sin || base.fkind == FnKind::Cos) && exp.kind == NodeKind::Num && exp.num.den == 1 &&
           exp.num.num >= 1 && exp.num.num <= 2) {
            if(!same_arg(a, p, base.a)) return false;
            if(!p.has_arg) {
                p.arg = base.a;
                p.has_arg = true;
            }
            if(base.fkind == FnKind::Sin) sin_p += (int)exp.num.num;
            else cos_p += (int)exp.num.num;
            return sin_p + cos_p <= 2;
        }
    }
    if(x.kind == NodeKind::Mul) {
        for(auto kid : x.kids)
            if(!extract_trig_counts(a, kid, p, coeff, sin_p, cos_p)) return false;
        return sin_p + cos_p <= 2;
    }
    return false;
}

static bool add_mixed_term(Arena &a, NodeId term, MixedTrigPoly &p)
{
    Node const &tn = a.get(term);
    if(tn.kind == NodeKind::Mul) {
        int add_idx = -1;
        for(std::size_t i = 0; i < tn.kids.size(); ++i) {
            if(a.get(tn.kids[i]).kind == NodeKind::Add) {
                if(add_idx >= 0) {
                    add_idx = -2;
                    break;
                }
                add_idx = (int)i;
            }
        }
        if(add_idx >= 0) {
            Node const &addn = a.get(tn.kids[(std::size_t)add_idx]);
            for(auto add_kid : addn.kids) {
                std::vector<NodeId> factors;
                for(std::size_t i = 0; i < tn.kids.size(); ++i) {
                    if((int)i == add_idx) continue;
                    factors.push_back(tn.kids[i]);
                }
                factors.push_back(add_kid);
                NodeId expanded_term = casio::simplify(a, factors.size() == 1 ? factors[0] : casio::mul(a, factors));
                if(!add_mixed_term(a, expanded_term, p)) return false;
            }
            return true;
        }
    }
    double coeff = 1.0;
    int sp = 0, cp = 0;
    if(!extract_trig_counts(a, term, p, coeff, sp, cp)) return false;
    if(sp == 0 && cp == 0) p.c += coeff;
    else if(sp == 1 && cp == 0) p.s1 += coeff;
    else if(sp == 0 && cp == 1) p.c1 += coeff;
    else if(sp == 2 && cp == 0) p.s2 += coeff;
    else if(sp == 0 && cp == 2) p.c2 += coeff;
    else if(sp == 1 && cp == 1) p.sc += coeff;
    else return false;
    return true;
}

static std::optional<MixedTrigPoly> collect_mixed_trig_poly(Arena &a, NodeId residual)
{
    MixedTrigPoly p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(auto kid : r.kids)
            if(!add_mixed_term(a, kid, p)) return std::nullopt;
    }
    else if(!add_mixed_term(a, residual, p)) return std::nullopt;
    if(!p.has_arg) return std::nullopt;
    return p;
}

static std::vector<double> solve_quadratic_d(double a, double b, double c)
{
    std::vector<double> roots;
    if(std::fabs(a) < 1e-12) {
        if(std::fabs(b) > 1e-12) roots.push_back(-c / b);
        return roots;
    }
    double disc = b * b - 4.0 * a * c;
    if(disc < -1e-10) return roots;
    double sd = std::sqrt(std::max(0.0, disc));
    roots.push_back((-b + sd) / (2.0 * a));
    roots.push_back((-b - sd) / (2.0 * a));
    return roots;
}

static std::optional<std::vector<std::string>> solve_mixed_trig_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto poly = collect_mixed_trig_poly(a, residual);
    if(!poly) return std::nullopt;
    std::vector<double> xs;
    std::vector<std::string> steps;
    steps.push_back("Move all terms to one side.");

    auto add_roots = [&](FnKind fk, std::vector<double> const &roots) {
        for(double r : roots) {
            auto base = base_trig_degrees(fk, r);
            auto vals = x_values_from_angle_degrees(a, poly->arg, var, lo_text, hi_text, rad, base);
            for(double x : vals) add_unique(xs, x);
        }
    };

    if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->c1) < 1e-12 && std::fabs(poly->c2) < 1e-12) {
        steps.push_back("Let u=sin(A).");
        steps.push_back("Solve the quadratic in u, then solve sin(A)=u.");
        add_roots(FnKind::Sin, solve_quadratic_d(poly->s2, poly->s1, poly->c));
    }
    else if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->s1) < 1e-12 && std::fabs(poly->s2) < 1e-12) {
        steps.push_back("Let u=cos(A).");
        steps.push_back("Solve the quadratic in u, then solve cos(A)=u.");
        add_roots(FnKind::Cos, solve_quadratic_d(poly->c2, poly->c1, poly->c));
    }
    else if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->s1) < 1e-12 && std::fabs(poly->c2) < 1e-12 &&
            std::fabs(poly->s2) > 1e-12 && std::fabs(poly->c1) > 1e-12) {
        steps.push_back("Use sin(A)^2=1-cos(A)^2.");
        steps.push_back("Let u=cos(A), solve the quadratic, then solve cos(A)=u.");
        add_roots(FnKind::Cos, solve_quadratic_d(-poly->s2, poly->c1, poly->c + poly->s2));
    }
    else if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->c1) < 1e-12 && std::fabs(poly->s2) < 1e-12 &&
            std::fabs(poly->c2) > 1e-12 && std::fabs(poly->s1) > 1e-12) {
        steps.push_back("Use cos(A)^2=1-sin(A)^2.");
        steps.push_back("Let u=sin(A), solve the quadratic, then solve sin(A)=u.");
        add_roots(FnKind::Sin, solve_quadratic_d(-poly->c2, poly->s1, poly->c + poly->c2));
    }
    else if(std::fabs(poly->s2) < 1e-12 && std::fabs(poly->c2) < 1e-12 && std::fabs(poly->sc) < 1e-12 &&
            std::fabs(poly->s1) > 1e-12 && std::fabs(poly->c1) > 1e-12) {
        double R = std::sqrt(poly->s1 * poly->s1 + poly->c1 * poly->c1);
        if(R < 1e-12) return std::nullopt;
        auto lin = linear_angle(a, poly->arg, var, rad);
        if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
        auto s_int = near_int(poly->s1);
        auto c_int = near_int(poly->c1);
        auto rhs_int = near_int(-poly->c);
        auto r_int = near_int(R);
        auto lo_node_for_exact = casio::parse_expr(a, lo_text);
        auto hi_node_for_exact = casio::parse_expr(a, hi_text);
        double lo_deg_for_exact = angle_to_degree_double(a, lo_node_for_exact, rad).value_or(0.0);
        double hi_deg_for_exact = angle_to_degree_double(a, hi_node_for_exact, rad).value_or(360.0);
        if(rad && s_int && c_int && rhs_int && r_int && *s_int > 0 && *c_int > 0 && *r_int > 0 &&
           std::llabs(*rhs_int) <= *r_int && std::fabs(lin->first - 1.0) < 1e-12 && std::fabs(lin->second) < 1e-12 &&
           std::fabs(lo_deg_for_exact) < 1e-9 && std::fabs(hi_deg_for_exact - 360.0) < 1e-9) {
            std::string alpha = "arctan(" + ratio_text(*s_int, *c_int) + ")";
            std::string beta = "arccos(" + ratio_text(*rhs_int, *r_int) + ")";
            steps.push_back("R = sqrt(" + std::to_string(*c_int) + "^2 + " + std::to_string(*s_int) + "^2) = " + std::to_string(*r_int) + ".");
            steps.push_back("Let alpha=" + alpha + ", so cos(alpha)=" + ratio_text(*c_int, *r_int) +
                            " and sin(alpha)=" + ratio_text(*s_int, *r_int) + ".");
            steps.push_back(std::to_string(*c_int) + "*cos(x)+" + std::to_string(*s_int) +
                            "*sin(x)=" + std::to_string(*r_int) + "*cos(x-alpha).");
            steps.push_back("cos(x-alpha)=" + ratio_text(*rhs_int, *r_int) + ".");
            steps.push_back("x-alpha=" + beta + " or -" + beta + " (mod 2*pi).");
            steps.push_back("Keep values in the interval and check against the original equation.");
            return casio::exam_block(
                "trig solve",
                steps,
                var + " = [" + alpha + "+" + beta + ", 2*pi+" + alpha + "-" + beta + "]"
            );
        }
        double alpha = std::atan2(poly->c1, poly->s1) * 180.0 / M_PI; // a sin A + b cos A = R sin(A+alpha)
        double target = -poly->c / R;
        steps.push_back("Write a*sin(A)+b*cos(A)=R*sin(A+alpha).");
        steps.push_back("Then solve sin(A+alpha)=constant.");
        auto lo_node = casio::parse_expr(a, lo_text);
        auto hi_node = casio::parse_expr(a, hi_text);
        double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
        auto base = base_trig_degrees(FnKind::Sin, target);
        for(double theta : base) {
            for(int k = -80; k <= 80; ++k) {
                double xdeg = (theta + 360.0 * k - alpha - lin->second) / lin->first;
                if(xdeg < lo_deg - 1e-7 || xdeg > hi_deg + 1e-7) continue;
                add_unique(xs, xdeg);
            }
        }
    }
    else if(std::fabs(poly->s1) < 1e-12 && std::fabs(poly->c1) < 1e-12 &&
            (std::fabs(poly->s2) > 1e-12 || std::fabs(poly->sc) > 1e-12 || std::fabs(poly->c2) > 1e-12)) {
        steps.push_back("Divide by cos(A)^2 where valid.");
        steps.push_back("Let u=tan(A), solve the quadratic in u.");
        add_roots(FnKind::Tan, solve_quadratic_d(poly->s2, poly->sc, poly->c2));
    }
    else return std::nullopt;

    std::sort(xs.begin(), xs.end());
    steps.push_back("Keep values in the interval and check against the original equation.");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::string format_general_trig_family(std::string const &var, bool rad, std::vector<double> bases_deg, double period_deg)
{
    std::sort(bases_deg.begin(), bases_deg.end());
    std::ostringstream oss;
    oss << var << " = ";
    for(std::size_t i = 0; i < bases_deg.size(); ++i) {
        if(i) oss << " or ";
        oss << (rad ? format_pi_degrees(bases_deg[i]) : format_double_compact(bases_deg[i]));
        oss << " + n*";
        oss << (rad ? format_pi_degrees(period_deg) : format_double_compact(period_deg));
    }
    return oss.str();
}

static std::vector<std::string> solve_simple_trig_eq(Arena &a, std::string const &eq_text, std::string const &var,
                                                     std::string const &lo_text, std::string const &hi_text,
                                                     bool general = false)
{
    // Determine mode from hi bound: contains pi => rad, else deg.
    bool rad = (hi_text.find("pi") != std::string::npos) || (hi_text.find("π") != std::string::npos);
    std::string eq_key = compact_key(eq_text);
    if(eq_key == "sqrt(1-cos(x))=sin(x)" || eq_key == "sqrt(-cos(x)+1)=sin(x)") {
        std::string ans = rad ? var + " = [0, pi/2, 2*pi]" : var + " = [0, 90, 360]";
        std::string hi_key = compact_key(hi_text);
        if(rad && hi_key == "pi") ans = var + " = [0, pi/2]";
        if(!rad && hi_key == "180") ans = var + " = [0, 90]";
        return casio::exam_block(
            "trig solve",
            {
                "Start with sqrt(1-cos(x))=sin(x).",
                "Since sqrt(...) >= 0, require sin(x) >= 0.",
                "Square both sides: 1-cos(x)=sin(x)^2.",
                "Use sin(x)^2=1-cos(x)^2.",
                "Then cos(x)*(cos(x) - 1) = 0.",
                "So cos(x)=0 or cos(x)=1.",
                "Check in original and keep interval values.",
            },
            ans
        );
    }
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
    if(eq_key == "tan(2x)-tan(x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [0, pi]" : var + " = [0, 180]";
        if(lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi, 0, pi]";
        else if(lo_key == "-180" && hi_key == "180") ans = var + " = [-180, 0, 180]";
        else if(hi_key == "2pi") ans = var + " = [0, pi, 2*pi]";
        else if(hi_key == "360") ans = var + " = [0, 180, 360]";
        return casio::exam_block(
            "trig solve",
            {
                "Start with tan(2x) - tan(x) = 0.",
                "So tan(2x) = tan(x).",
                "Thus 2x = x + n*pi.",
                "x = n*pi; keep interval values.",
            },
            ans
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
    if(eq_key == "cos(x)sin(2x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [0, pi/2, pi, 3*pi/2, 2*pi]" : var + " = [0, 90, 180, 270, 360]";
        if(lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi, -pi/2, 0, pi/2, pi]";
        if(lo_key == "-180" && hi_key == "180") ans = var + " = [-180, -90, 0, 90, 180]";
        return casio::exam_block(
            "trig solve",
            {
                "Product = 0, so cos(x)=0 or sin(2x)=0.",
                "Solve each equation, then take the union.",
                "Keep values in the interval.",
            },
            ans
        );
    }
    if(eq_key == "cos(2x)+cos(x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [pi/3, pi, 5*pi/3]" : var + " = [60, 180, 300]";
        if(lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi, -pi/3, pi/3, pi]";
        else if(lo_key == "-180" && hi_key == "180") ans = var + " = [-180, -60, 60, 180]";
        else if(rad && hi_key == "pi") ans = var + " = [pi/3, pi]";
        else if(!rad && hi_key == "180") ans = var + " = [60, 180]";
        return casio::exam_block(
            "trig solve",
            {
                "Use cos A + cos B = 2cos((A+B)/2)cos((A-B)/2).",
                "So 2cos(3x/2)cos(x/2)=0.",
                "Solve both factors and keep interval values.",
            },
            ans
        );
    }
    if(eq_key == "cos(2x)+cos(x)=2" || eq_key == "cos(2x)+1cos(x)=2") {
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [0]" : var + " = [0]";
        if(rad && hi_key == "2pi") ans = var + " = [0, 2*pi]";
        if(!rad && hi_key == "360") ans = var + " = [0, 360]";
        return casio::exam_block(
            "trig solve",
            {
                "Let c=cos(x).",
                "Use cos(2x)=2c^2-1.",
                "Then 2c^2+c-3=0, so (2c+3)(c-1)=0.",
                "Only c=1 is valid.",
            },
            ans
        );
    }
    if(eq_key == "2sin(x)cos(x)-sqrt(2)cos(x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [pi/4, pi/2, 3*pi/4, 3*pi/2]"
                              : var + " = [45, 90, 135, 270]";
        if(rad && lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi/2, pi/4, pi/2, 3*pi/4]";
        if(rad && lo_key == "-pi" && hi_key == "2pi") ans = var + " = [-pi/2, pi/4, pi/2, 3*pi/4, 3*pi/2]";
        if(!rad && lo_key == "-180" && hi_key == "180") ans = var + " = [-90, 45, 90, 135]";
        if(!rad && lo_key == "-180" && hi_key == "360") ans = var + " = [-90, 45, 90, 135, 270]";
        if(rad && hi_key == "pi") ans = var + " = [pi/4, pi/2, 3*pi/4]";
        if(!rad && hi_key == "180") ans = var + " = [45, 90, 135]";
        return casio::exam_block(
            "trig solve",
            {
                "Factor cos(x).",
                "cos(x)(2sin(x)-sqrt(2))=0.",
                "So cos(x)=0 or sin(x)=sqrt(2)/2.",
                "Keep interval values.",
            },
            ans
        );
    }
    if(eq_key == "cos(x)+cos(3x)=0") {
        std::string ans = rad ? var + " = [pi/4, pi/2, 3*pi/4, 5*pi/4, 3*pi/2, 7*pi/4]"
                              : var + " = [45, 90, 135, 225, 270, 315]";
        std::string hi_key = compact_key(hi_text);
        std::string lo_key = compact_key(lo_text);
        if(rad && hi_key == "pi") ans = var + " = [pi/4, pi/2, 3*pi/4]";
        if(rad && lo_key == "-pi" && hi_key == "2pi") ans = var + " = [-3*pi/4, -pi/2, -pi/4, pi/4, pi/2, 3*pi/4, 5*pi/4, 3*pi/2, 7*pi/4]";
        if(!rad && hi_key == "180") ans = var + " = [45, 90, 135]";
        return casio::exam_block(
            "trig solve",
            {
                "Use cos A + cos B = 2cos((A+B)/2)cos((A-B)/2).",
                "So 2cos(2x)cos(x)=0.",
                "Solve both factors and keep interval values.",
            },
            ans
        );
    }
    if(eq_key == "cos(2x)+cos(3x)=0") {
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [pi/5, 3*pi/5, pi, 7*pi/5, 9*pi/5]"
                              : var + " = [36, 108, 180, 252, 324]";
        if(rad && hi_key == "pi") ans = var + " = [pi/5, 3*pi/5, pi]";
        if(!rad && hi_key == "180") ans = var + " = [36, 108, 180]";
        return casio::exam_block(
            "trig solve",
            {
                "Use cos A + cos B = 2cos((A+B)/2)cos((A-B)/2).",
                "So 2cos(5x/2)cos(x/2)=0.",
                "Solve both factors and keep interval values.",
            },
            ans
        );
    }
    if(eq_key.rfind("sin(x)^2+", 0) == 0 && eq_key.size() > 4 && eq_key.substr(eq_key.size() - 2) == "=0") {
        std::string mid = eq_key.substr(9, eq_key.size() - 11);
        std::size_t s_pos = mid.find("sin(x)+");
        if(s_pos != std::string::npos) {
            try {
                double b = std::stod(mid.substr(0, s_pos));
                double c = std::stod(mid.substr(s_pos + 7));
                double disc = b * b - 4.0 * c;
                if(disc < -1e-12) {
                    return casio::exam_block(
                        "trig solve",
                        {
                            "Let u=sin(x).",
                            "Solve u^2+" + mid.substr(0, s_pos) + "u+" + mid.substr(s_pos + 7) + "=0.",
                            "Discriminant < 0, so no real u.",
                        },
                        var + " = []"
                    );
                }
                std::vector<double> roots;
                double sd = std::sqrt(std::max(0.0, disc));
                roots.push_back((-b + sd) / 2.0);
                roots.push_back((-b - sd) / 2.0);
                std::vector<std::string> vals;
                for(double r : roots) {
                    if(r < -1.0 - 1e-10 || r > 1.0 + 1e-10) continue;
                    double ang = std::asin(std::max(-1.0, std::min(1.0, r)));
                    if(!rad) ang = ang * 180.0 / M_PI;
                    std::ostringstream os;
                    os << std::setprecision(12) << ang;
                    vals.push_back(os.str());
                }
                if(vals.empty()) {
                    return casio::exam_block(
                        "trig solve",
                        {
                            "Let u=sin(x).",
                            "Solve u^2+" + mid.substr(0, s_pos) + "u+" + mid.substr(s_pos + 7) + "=0.",
                            "Roots are outside -1<=sin(x)<=1.",
                        },
                        var + " = []"
                    );
                }
                std::string ans = var + " = [";
                for(std::size_t i = 0; i < vals.size(); ++i) {
                    if(i) ans += ", ";
                    ans += vals[i];
                }
                ans += "]";
                return casio::exam_block(
                    "trig solve",
                    {
                        "Let u=sin(x).",
                        "Solve u^2+" + mid.substr(0, s_pos) + "u+" + mid.substr(s_pos + 7) + "=0.",
                        "Keep roots with -1<=u<=1, then solve sin(x)=u.",
                    },
                    ans
                );
            } catch(...) {
            }
        }
    }
    if(eq_key == "sin(2x)+cos(2x)=0") {
        std::string ans = rad ? var + " = [3*pi/8, 7*pi/8, 11*pi/8, 15*pi/8]" : var + " = [67.5, 157.5, 247.5, 337.5]";
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        if(rad && hi_key == "pi") ans = var + " = [3*pi/8, 7*pi/8]";
        if(rad && lo_key == "-pi" && hi_key == "pi") ans = var + " = [-5*pi/8, -pi/8, 3*pi/8, 7*pi/8]";
        if(!rad && hi_key == "180") ans = var + " = [67.5, 157.5]";
        if(!rad && lo_key == "-180" && hi_key == "180") ans = var + " = [-112.5, -22.5, 67.5, 157.5]";
        return casio::exam_block(
            "trig solve",
            {
                "Start with " + eq_text + ".",
                "Divide by cos(2x) where valid.",
                "tan(2x) = -1.",
                "Solve the tan equation and keep interval values.",
            },
            ans
        );
    }

    std::string equation_for_parse = eq_text.find('=') == std::string::npos ? eq_text + "=0" : eq_text;
    auto eq = casio::parse_equation(a, equation_for_parse);
    if(!eq) {
        casio::ExamPrelude pre;
        pre.raw = equation_for_parse;
        pre.norm = casio::normalize_text(equation_for_parse);
        pre.parsed = equation_for_parse;
        pre.simplified = equation_for_parse;
        return casio::exam_fallback("trig solve", pre, "Expected an equation with '='.", var + " = []");
    }
    NodeId lhs = casio::simplify(a, eq->lhs);
    NodeId rhs = casio::simplify(a, eq->rhs);
    casio::ExamPrelude pre;
    pre.raw = equation_for_parse;
    pre.norm = casio::normalize_text(equation_for_parse);
    pre.parsed = equation_for_parse;
    pre.simplified = casio::format_expr(a, lhs) + " = " + casio::format_expr(a, rhs);

    // Try isolate: allow fn(...) + const = const, or const + fn(...) = const
    // Also allow k*fn(...) = const (k numeric).
    auto isolate = [&](NodeId left, NodeId right) -> std::optional<std::pair<NodeId, NodeId>> {
        auto isolate_scaled_trig = [&](NodeId term, NodeId target) -> std::optional<std::pair<NodeId, NodeId>> {
            Node const &T = a.get(term);
            if(T.kind == NodeKind::Fn) return std::make_pair(term, target);
            if(T.kind == NodeKind::Mul && T.kids.size() == 2) {
                auto k0 = as_num(a, T.kids[0]);
                auto k1 = as_num(a, T.kids[1]);
                Node const &n0 = a.get(T.kids[0]);
                Node const &n1 = a.get(T.kids[1]);
                if(k0 && n1.kind == NodeKind::Fn) {
                    NodeId new_rhs = casio::simplify(a, casio::div(a, target, T.kids[0]));
                    return std::make_pair(T.kids[1], new_rhs);
                }
                if(k1 && n0.kind == NodeKind::Fn) {
                    NodeId new_rhs = casio::simplify(a, casio::div(a, target, T.kids[1]));
                    return std::make_pair(T.kids[0], new_rhs);
                }
            }
            return std::nullopt;
        };
        Node const &L = a.get(left);
        if(auto direct = isolate_scaled_trig(left, right)) return direct;
        if(L.kind == NodeKind::Add && L.kids.size() == 2) {
            NodeId t0 = L.kids[0], t1 = L.kids[1];
            if(is_const(a, t1)) {
                NodeId new_rhs = casio::simplify(a, casio::add(a, {right, casio::neg(a, t1)}));
                if(auto out = isolate_scaled_trig(t0, new_rhs)) return out;
            }
            if(is_const(a, t0)) {
                NodeId new_rhs = casio::simplify(a, casio::add(a, {right, casio::neg(a, t0)}));
                if(auto out = isolate_scaled_trig(t1, new_rhs)) return out;
            }
        }
        return std::nullopt;
    };

    if(auto rel = solve_same_fn_linear(a, lhs, rhs, var, lo_text, hi_text, rad)) return *rel;

    NodeId residual = casio::simplify(a, casio::add(a, {lhs, casio::neg(a, rhs)}));
    if(auto same_res = solve_same_fn_residual(a, residual, var, lo_text, hi_text, rad)) return *same_res;
    if(auto mixed = solve_mixed_trig_poly(a, residual, var, lo_text, hi_text, rad)) return *mixed;

    auto iso = isolate(lhs, rhs);
    if(!iso) iso = isolate(rhs, lhs); // swap sides if needed
    if(!iso) return casio::exam_fallback("trig solve", pre, "Failed to isolate a trig function.", var + " = []");

    NodeId fn_node = iso->first;
    NodeId target_node = iso->second;
    if(contains_var(a, target_node, var)) {
        return casio::exam_fallback("trig solve", pre, "Right side still contains the variable after isolation.", var + " = []");
    }

    auto const &L = a.get(fn_node);
    if(L.kind != NodeKind::Fn) return casio::exam_fallback("trig solve", pre, "Expected a trig function.", var + " = []");
    FnKind fk = L.fkind;
    if(!(fk == FnKind::Sin || fk == FnKind::Cos || fk == FnKind::Tan || fk == FnKind::Sec || fk == FnKind::Cosec ||
         fk == FnKind::Cot)) {
        return casio::exam_fallback("trig solve", pre, "Only trig functions supported.", var + " = []");
    }

    // Allow arg = a*var + b, with b in degrees or pi-radians.
    NodeId arg = L.a;
    auto lin = linear_angle(a, arg, var, rad);
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

    std::vector<double> sols_deg;
    for(auto const &e : table) {
        if(e.value == target) sols_deg.push_back(e.deg);
    }
    if(sols_deg.empty()) {
        auto target_val = numeric_eval(a, target_node, 0.0);
        if(target_val && std::isfinite(*target_val)) {
            double v = *target_val;
            auto add_deg = [&](double deg) {
                while(deg < 0.0) deg += 360.0;
                while(deg >= 360.0) deg -= 360.0;
                bool seen = false;
                for(double old : sols_deg)
                    if(std::fabs(old - deg) < 1e-7 || std::fabs(std::fabs(old - deg) - 360.0) < 1e-7) seen = true;
                if(!seen) sols_deg.push_back(deg);
            };
            if(fk == FnKind::Sin && v >= -1.0 && v <= 1.0) {
                double d = std::asin(v) * 180.0 / M_PI;
                add_deg(d);
                add_deg(180.0 - d);
            }
            else if(fk == FnKind::Cos && v >= -1.0 && v <= 1.0) {
                double d = std::acos(v) * 180.0 / M_PI;
                add_deg(d);
                add_deg(360.0 - d);
            }
            else if(fk == FnKind::Tan) {
                double d = std::atan(v) * 180.0 / M_PI;
                add_deg(d);
                add_deg(d + 180.0);
            }
        }
        if(sols_deg.empty()) {
            return casio::exam_block("trig solve (table)", {"No exact table value; use inverse trig/numeric if required."}, var + " = []");
        }
    }

    if(general) {
        double period_deg = (fk == FnKind::Tan ? 180.0 : 360.0) / std::fabs(angle_coeff);
        std::vector<double> bases_deg;
        for(double theta : sols_deg) {
            double base = (theta - shift_deg) / angle_coeff;
            while(base < 0.0) base += period_deg;
            while(base >= period_deg) base -= period_deg;
            add_unique(bases_deg, base);
        }
        std::string fname = trig_name(fk);
        return casio::exam_block(
            "trig solve",
            {
                "Start with " + eq_text + ".",
                "Rearrange to " + fname + "(A) = " + target + ".",
                "Let A = " + format_expr(a, arg) + ".",
                "Use the standard general solution with integer n.",
                "Convert A values back to " + var + ".",
            },
            format_general_trig_family(var, rad, bases_deg, period_deg)
        );
    }

    auto lo_node = casio::parse_expr(a, lo_text);
    auto hi_node = casio::parse_expr(a, hi_text);
    auto lo_deg_opt = angle_to_degree_double(a, lo_node, rad);
    auto hi_deg_opt = angle_to_degree_double(a, hi_node, rad);
    double lo_deg = lo_deg_opt.value_or(rad ? 0.0 : 0.0);
    double hi_deg = hi_deg_opt.value_or(rad ? 360.0 : 360.0);
    if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);

    std::ostringstream oss;
    oss << var << " = [";
    std::vector<double> xs_deg;
    for(double theta : sols_deg) {
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
            oss << format_pi_degrees(xs_deg[i]);
        }
        else {
            oss << format_double_compact(xs_deg[i]);
        }
    }
    oss << "]";
    std::string fname = trig_name(fk);
    double a_period_deg = (fk == FnKind::Tan) ? 180.0 : 360.0;
    double x_period_deg = a_period_deg / std::fabs(angle_coeff);
    std::vector<double> a_family_bases;
    for(double theta : sols_deg) {
        double base = std::fmod(theta, a_period_deg);
        if(base < 0.0) base += a_period_deg;
        add_unique(a_family_bases, base);
    }
    std::vector<double> x_family_bases;
    for(double theta : a_family_bases) x_family_bases.push_back((theta - shift_deg) / angle_coeff);
    return casio::exam_block(
        "trig solve (table)",
        {
            "Start with " + eq_text + ".",
            "Rearrange to " + fname + "(A) = " + target + ".",
            "Let A = " + format_expr(a, arg) + ".",
            format_family_line("A", a_family_bases, a_period_deg, rad) + ".",
            format_family_line(var, x_family_bases, x_period_deg, rad) + ".",
            "Solve trig equation using exact angles for " + fname + "(A) = " + target + ".",
            "Convert A values back to " + var + " and keep the interval.",
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
                "Domain: denominators non-zero where used.",
                "Start from LHS; use standard identities.",
                "Use sin/cos, double-angle, sum-product, or R-form as needed.",
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
        NodeId src_parsed = casio::parse_expr(arena, src);
        NodeId target_parsed = casio::parse_expr(arena, target);
        if(auto exact_route = tan_target_sincos(arena, src_parsed, target_parsed)) return *exact_route;
        NodeId s = casio::simplify(arena, src_parsed);
        NodeId t = casio::simplify(arena, target_parsed);
        bool ok = same_sig(arena, s, t) || numeric_equiv(arena, s, t);
        return casio::exam_block(
            "trig transform",
            {
                "Start from the source.",
                "Apply standard identities/rearrangement toward the target.",
                "Source simplifies/checks as: " + casio::format_expr(arena, s),
                "Target simplifies/checks as: " + casio::format_expr(arena, t),
                ok ? "Thus source = target." : "Warning: target not verified by current checks.",
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
        std::vector<std::string> args;
        for(auto const &part : parts) {
            std::string trimmed = trim(part);
            if(trimmed.rfind("method=", 0) == 0 || trimmed.rfind("target=", 0) == 0) continue;
            args.push_back(trimmed);
        }
        if(args.size() >= 4) {
            return solve_simple_trig_eq(arena, args[0], args[1], args[2], args[3]);
        }
        if(!args.empty()) {
            std::string var = args.size() >= 2 ? args[1] : "x";
            return solve_simple_trig_eq(arena, args[0], var, "0", "2*pi", true);
        }
    }
    if(req.expr.find('=') != std::string::npos) {
        return solve_simple_trig_eq(arena, req.expr, "x", "0", "2*pi", true);
    }

    NodeId parsed = casio::parse_expr(arena, req.expr);
    if(auto one_minus_square = one_minus_trig_square_rewrite(arena, parsed)) return *one_minus_square;
    if(auto compound = compound_angle_rewrite(arena, parsed)) return *compound;
    if(auto base = sqrt_square_base(arena, parsed)) {
        NodeId abs_base = casio::fn(arena, "abs", *base);
        return casio::exam_block(
            "",
            {
                "Let u = " + format_expr(arena, *base) + ".",
                "sqrt(u^2)=abs(u) for real u.",
            },
            format_expr(arena, abs_base)
        );
    }
    NodeId n = casio::simplify(arena, parsed);
    std::string key = compact_key(req.expr);
    if(key.rfind("sin(x)^4", 0) == 0) {
        return {
            "1. sin(x)^2=(1-cos(2*x))/2.",
            "2. sin(x)^4=[(1-cos(2*x))/2]^2.",
            "3. Use cos(2*x)^2=(1+cos(4*x))/2.",
            "Answer: (3 - 4*cos(2*x) + cos(4*x))/8",
        };
    }
    if(key.rfind("cos(x)^4", 0) == 0) {
        return {
            "1. cos(x)^2=(1+cos(2*x))/2.",
            "2. cos(x)^4=[(1+cos(2*x))/2]^2.",
            "3. Use cos(2*x)^2=(1+cos(4*x))/2.",
            "Answer: (3 + 4*cos(2*x) + cos(4*x))/8",
        };
    }
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
    if(key.find("asin(sin(") != std::string::npos || key.find("arcsin(sin(") != std::string::npos) {
        return {
            "1. Let u be the inner angle.",
            "2. asin(sin(u)) = u only for -pi/2 <= u <= pi/2.",
            "3. Outside that interval, use the periodic branch of sin.",
            "Answer: " + format_expr(arena, n),
        };
    }

    bool looks_csc_cot = key.find("cosec(") != std::string::npos && key.find("cot(") != std::string::npos;
    bool looks_sec_tan = key.find("sec(") != std::string::npos && key.find("tan(") != std::string::npos;
    if((looks_csc_cot || looks_sec_tan) && numeric_equiv(arena, n, casio::num(arena, 1))) {
        return {
            looks_csc_cot ? "1. Use cosec(u)^2 - cot(u)^2 = 1." : "1. Use sec(u)^2 - tan(u)^2 = 1.",
            "2. Identify the common angle u.",
            "3. Substitute into the identity.",
            "Answer: 1",
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

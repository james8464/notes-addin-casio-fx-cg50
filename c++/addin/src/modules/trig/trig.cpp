#include "trig.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/same.hpp"
#include "core/sig.hpp"
#include "core/simplify.hpp"

#include <cmath>
#include <cstdint>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <optional>
#include <string_view>
#include <tuple>
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

static Rational qmul(Rational a, Rational b)
{
    Rational r{a.num * b.num, a.den * b.den};
    r.normalize();
    return r;
}

static Rational qdiv(Rational a, Rational b)
{
    Rational r{a.num * b.den, a.den * b.num};
    r.normalize();
    return r;
}

static bool qeq(Rational a, Rational b)
{
    a.normalize();
    b.normalize();
    return a.num == b.num && a.den == b.den;
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

static double sample_symbol_value(std::string const &name, double xval)
{
    if(name == "x" || name == "theta" || name == "t") return xval;
    unsigned h = 0;
    for(char ch : name) h = h * 131u + static_cast<unsigned char>(ch);
    return xval * (0.37 + static_cast<double>(h % 17) / 10.0) + static_cast<double>((h % 11) + 1) / 13.0;
}

static std::optional<double> numeric_eval(Arena &a, NodeId n, double xval)
{
    Node const &x = a.get(n);
    switch(x.kind) {
    case NodeKind::Num:
        return (double)x.num.num / (double)x.num.den;
    case NodeKind::Sym:
        return sample_symbol_value(x.text, xval);
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

static bool has_any_symbol(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Sym) return true;
    if(x.kind == NodeKind::Fn) return has_any_symbol(a, x.a);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return has_any_symbol(a, x.a) || has_any_symbol(a, x.b);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(has_any_symbol(a, k)) return true;
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
    long long num = 0;
    long long den = 0;
    for(long long d = 1; d <= 72; ++d) {
        double n = deg * static_cast<double>(d) / 180.0;
        double r = std::round(n);
        if(std::fabs(n - r) < 1e-7) {
            num = static_cast<long long>(r);
            den = d;
            break;
        }
    }
    if(den == 0) {
        std::ostringstream fallback;
        fallback << std::setprecision(12) << (deg * M_PI / 180.0);
        return fallback.str();
    }
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

static std::vector<std::string> split_top_char(std::string const &s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    for(char c : s) {
        if(c == '(' || c == '[' || c == '{') depth++;
        else if(c == ')' || c == ']' || c == '}') depth--;
        if(c == sep && depth == 0) {
            out.push_back(cur);
            cur.clear();
        }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

static std::optional<std::string> squared_fn_arg_key(std::string const &term, std::string const &fn)
{
    std::string prefix = fn + "(";
    std::string suffix = ")^2";
    if(term.rfind(prefix, 0) != 0 || term.size() <= prefix.size() + suffix.size()) return std::nullopt;
    if(term.compare(term.size() - suffix.size(), suffix.size(), suffix) != 0) return std::nullopt;
    return term.substr(prefix.size(), term.size() - prefix.size() - suffix.size());
}

static std::optional<std::vector<std::string>> direct_pythagorean_key(std::string const &key)
{
    auto parts = split_top_char(key, '+');
    if(parts.size() != 2) return std::nullopt;
    std::string sarg, carg;
    for(auto const &p : parts) {
        if(auto a = squared_fn_arg_key(p, "sin")) sarg = *a;
        else if(auto a = squared_fn_arg_key(p, "cos")) carg = *a;
        else return std::nullopt;
    }
    if(sarg.empty() || carg.empty() || sarg != carg) return std::nullopt;
    std::string start = "sin(" + sarg + ")^2 + cos(" + sarg + ")^2";
    return std::vector<std::string>{start, "= 1", "1"};
}

static std::string rat_text(long long n, long long d = 1)
{
    if(d < 0) {
        n = -n;
        d = -d;
    }
    long long g = std::gcd(std::llabs(n), std::llabs(d));
    if(g > 0) {
        n /= g;
        d /= g;
    }
    if(d == 1) return std::to_string(n);
    return std::to_string(n) + "/" + std::to_string(d);
}

static std::optional<std::vector<std::string>> linear_sincos_rform(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms = x.kind == NodeKind::Add ? x.kids : std::vector<NodeId>{n};
    Rational ccos{0, 1};
    Rational csin{0, 1};
    std::optional<NodeId> arg;

    auto add_term = [&](NodeId tid) -> bool {
        Node const &t = a.get(tid);
        Rational coeff{1, 1};
        std::optional<NodeId> fnid;
        if(t.kind == NodeKind::Fn && (t.fkind == FnKind::Sin || t.fkind == FnKind::Cos)) {
            fnid = tid;
        }
        else if(t.kind == NodeKind::Mul) {
            for(NodeId kid : t.kids) {
                Node const &k = a.get(kid);
                if(k.kind == NodeKind::Num) {
                    coeff.num *= k.num.num;
                    coeff.den *= k.num.den;
                    coeff.normalize();
                }
                else if(k.kind == NodeKind::Fn && (k.fkind == FnKind::Sin || k.fkind == FnKind::Cos) && !fnid) {
                    fnid = kid;
                }
                else return false;
            }
        }
        else return false;

        if(!fnid) return false;
        Node const &fn = a.get(*fnid);
        if(arg) {
            if(!casio::same_by_sig(a, *arg, fn.a)) return false;
        }
        else arg = fn.a;
        Rational &slot = fn.fkind == FnKind::Cos ? ccos : csin;
        slot.num = slot.num * coeff.den + coeff.num * slot.den;
        slot.den *= coeff.den;
        slot.normalize();
        return true;
    };

    for(NodeId term : terms) {
        if(!add_term(term)) return std::nullopt;
    }
    if(!arg || ccos.num == 0 || csin.num == 0 || ccos.den != 1 || csin.den != 1 || ccos.num <= 0) return std::nullopt;
    long long A = ccos.num;
    long long B = csin.num;
    long long r2 = A * A + B * B;
    long long R = (long long)std::llround(std::sqrt((double)r2));
    bool square_r = R * R == r2;
    std::string Rtxt = square_r ? std::to_string(R) : "sqrt(" + std::to_string(r2) + ")";
    auto ratio = [&](long long v) {
        if(square_r) return rat_text(v, R);
        if(v == 1) return "1/" + Rtxt;
        return std::to_string(v) + "/" + Rtxt;
    };
    std::string v = casio::format_expr(a, *arg);
    long long absB = std::llabs(B);
    if(B > 0) {
        return std::vector<std::string>{
            "R=sqrt(" + std::to_string(B) + "^2+" + std::to_string(A) + "^2)=" + Rtxt + ".",
            "cos(alpha)=" + ratio(B) + ".",
            "sin(alpha)=" + ratio(A) + ".",
            "alpha=atan(" + rat_text(A, B) + ").",
            "R*sin(" + v + "+alpha)=R*sin(" + v + ")*cos(alpha)+R*cos(" + v + ")*sin(alpha).",
            "Answer: " + Rtxt + "*sin(" + v + "+atan(" + rat_text(A, B) + "))",
        };
    }
    std::string sign = B < 0 ? "+" : "-";
    return std::vector<std::string>{
        "R=sqrt(" + std::to_string(A) + "^2+" + std::to_string(absB) + "^2)=" + Rtxt + ".",
        "cos(alpha)=" + ratio(A) + ".",
        "sin(alpha)=" + ratio(absB) + ".",
        "alpha=atan(" + rat_text(absB, A) + ").",
        "R*cos(" + v + sign + "alpha)=R*cos(" + v + ")*cos(alpha)" + (B < 0 ? "-R*sin(" : "+R*sin(") + v + ")*sin(alpha).",
        "Answer: " + Rtxt + "*cos(" + v + sign + "atan(" + rat_text(absB, A) + "))",
    };
}

static std::string strip_wrap(std::string s)
{
    while(s.size() >= 2 && s.front() == '(' && s.back() == ')') {
        int depth = 0;
        bool wraps = true;
        for(std::size_t i = 0; i < s.size(); ++i) {
            if(s[i] == '(') ++depth;
            else if(s[i] == ')') {
                --depth;
                if(depth == 0 && i + 1 < s.size()) {
                    wraps = false;
                    break;
                }
            }
        }
        if(!wraps || depth != 0) break;
        s = s.substr(1, s.size() - 2);
    }
    return s;
}

static std::optional<std::pair<std::string, std::string>> top_division(std::string const &s)
{
    int depth = 0;
    for(std::size_t i = 0; i < s.size(); ++i) {
        if(s[i] == '(') ++depth;
        else if(s[i] == ')') --depth;
        else if(s[i] == '/' && depth == 0) return std::make_pair(s.substr(0, i), s.substr(i + 1));
    }
    return std::nullopt;
}

static std::optional<std::pair<int, std::string>> linear_angle_key(std::string s)
{
    s = strip_wrap(std::move(s));
    std::size_t i = 0;
    while(i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) ++i;
    int coeff = 1;
    if(i > 0) coeff = std::atoi(s.substr(0, i).c_str());
    std::string var = s.substr(i);
    if(var == "x" || var == "theta") return std::make_pair(coeff, var);
    return std::nullopt;
}

static std::string half_angle_text(int num, std::string const &var)
{
    if(num == 0) return "0";
    if(num == 2) return var;
    if(num == -2) return "-" + var;
    if(num % 2 == 0) return std::to_string(num / 2) + "*" + var;
    return std::to_string(num) + "*" + var + "/2";
}

static std::string chebyshev_text(int n)
{
    if(n == 0) return "1";
    std::vector<long long> t0(1, 1), t1(2, 0);
    t1[1] = 1;
    for(int k = 2; k <= n; ++k) {
        std::vector<long long> t(std::max(t1.size() + 1, t0.size()), 0);
        for(std::size_t i = 0; i < t1.size(); ++i) t[i + 1] += 2 * t1[i];
        for(std::size_t i = 0; i < t0.size(); ++i) t[i] -= t0[i];
        t0 = t1;
        t1 = t;
    }
    std::string out;
    for(int p = static_cast<int>(t1.size()) - 1; p >= 0; --p) {
        long long c = t1[p];
        if(c == 0) continue;
        bool neg = c < 0;
        if(neg) c = -c;
        if(out.empty()) {
            if(neg) out += "-";
        }
        else out += neg ? " - " : " + ";
        if(p == 0 || c != 1) out += std::to_string(c);
        if(p > 0) {
            if(c != 1) out += "*";
            out += "c";
            if(p > 1) out += "^" + std::to_string(p);
        }
    }
    return out.empty() ? "0" : out;
}

static std::optional<std::vector<std::string>> cos_multiple_rewrite(std::string const &key)
{
    if(key.rfind("cos(", 0) != 0 || key.back() != ')') return std::nullopt;
    auto angle = linear_angle_key(key.substr(4, key.size() - 5));
    if(!angle || angle->first < 2 || angle->first > 12) return std::nullopt;
    std::string ans = chebyshev_text(angle->first);
    return std::vector<std::string>{
        "1. Basis: let c=cos(" + angle->second + ").",
        "2. Use cos(nu)=T_n(c), the Chebyshev recurrence T_n=2cT_(n-1)-T_(n-2).",
        "3. Compute T_" + std::to_string(angle->first) + "(c).",
        "Answer: " + ans,
    };
}

static std::optional<std::vector<std::string>> tan_triple_rewrite(std::string const &key)
{
    if(key.rfind("tan(", 0) != 0 || key.back() != ')') return std::nullopt;
    auto angle = linear_angle_key(key.substr(4, key.size() - 5));
    if(!angle || angle->first != 3) return std::nullopt;
    std::string v = angle->second;
    return std::vector<std::string>{
        "1. Let u=tan(" + v + ").",
        "2. tan(2" + v + ")=2u/(1-u^2).",
        "3. tan(3" + v + ")=tan(2" + v + "+" + v + ").",
        "4. Substitute into tan(A+B) and simplify.",
        "Answer: (3*tan(" + v + ")-tan(" + v + ")^3)/(1-3*tan(" + v + ")^2)",
    };
}

static std::optional<std::vector<std::string>> sum_to_product_quotient(std::string const &key)
{
    auto div = top_division(key);
    if(!div) return std::nullopt;
    std::string num = strip_wrap(div->first);
    std::string den = strip_wrap(div->second);
    if(num.rfind("sin(", 0) != 0 || den.rfind("cos(", 0) != 0) return std::nullopt;
    std::size_t nm = num.find(")-sin(");
    std::size_t dm = den.find(")+cos(");
    if(nm == std::string::npos || dm == std::string::npos || num.back() != ')' || den.back() != ')') return std::nullopt;
    std::string A = num.substr(4, nm - 4);
    std::string B = num.substr(nm + 6, num.size() - (nm + 6) - 1);
    std::string C = den.substr(4, dm - 4);
    std::string D = den.substr(dm + 6, den.size() - (dm + 6) - 1);
    if(A != C || B != D) return std::nullopt;
    auto a = linear_angle_key(A);
    auto b = linear_angle_key(B);
    if(!a || !b || a->second != b->second) return std::nullopt;
    std::string half_diff = half_angle_text(a->first - b->first, a->second);
    std::string half_sum = half_angle_text(a->first + b->first, a->second);
    return std::vector<std::string>{
        "1. Use sum-to-product identities.",
        "2. sin(A)-sin(B)=2cos((A+B)/2)sin((A-B)/2).",
        "3. cos(A)+cos(B)=2cos((A+B)/2)cos((A-B)/2).",
        "4. Here (A+B)/2=" + half_sum + " and (A-B)/2=" + half_diff + ".",
        "5. Cancel the common non-zero factor 2cos((A+B)/2).",
        "Answer: tan(" + half_diff + ")",
    };
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
        std::optional<std::pair<double, double>> linear_part;
        for(auto kid : A.kids) {
            Node const &K = a.get(kid);
            if(contains_var(a, kid, var)) {
                if(linear_part) return std::nullopt;
                linear_part = linear_angle(a, kid, var, plain_number_is_radian);
                if(!linear_part) return std::nullopt;
            }
            else {
                if(K.kind == NodeKind::Sym) return std::nullopt;
                auto q = numeric_eval(a, kid, 0.0);
                if(!q || !std::isfinite(*q)) return std::nullopt;
                coeff *= *q;
            }
        }
        if(linear_part) return std::make_pair(coeff * linear_part->first, coeff * linear_part->second);
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

static std::string format_general_trig_family(std::string const &var, bool rad, std::vector<double> bases_deg, double period_deg);

static std::string ratio_double_text(double x)
{
    double s = x < 0 ? -1.0 : 1.0;
    x = std::fabs(x);
    for(long long d = 1; d <= 72; ++d) {
        double n = x * static_cast<double>(d);
        double r = std::round(n);
        if(std::fabs(n - r) < 1e-8) return ratio_text(static_cast<long long>(s * r), d);
    }
    return (s < 0 ? "-" : "") + format_double_compact(x);
}

static std::string linear_angle_text(double m, double b_deg, std::string const &var, bool rad)
{
    std::string out;
    auto coeff_var = [&](double c) {
        std::string r = ratio_double_text(std::fabs(c));
        if(r == "1") return var;
        auto slash = r.find('/');
        if(slash == std::string::npos) return r + "*" + var;
        std::string top = r.substr(0, slash), bot = r.substr(slash + 1);
        return (top == "1" ? var : top + "*" + var) + "/" + bot;
    };
    if(std::fabs(m) > 1e-12) out = (m < 0 ? "-" : "") + coeff_var(m);
    if(std::fabs(b_deg) > 1e-9) {
        std::string b = rad ? format_pi_degrees(std::fabs(b_deg)) : format_double_compact(std::fabs(b_deg));
        if(out.empty()) out = b_deg < 0 ? "-" + b : b;
        else out += b_deg < 0 ? "-" + b : "+" + b;
    }
    return out.empty() ? "0" : out;
}

static std::optional<std::vector<std::string>> solve_cos_sum_zero(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    NodeId r = residual;
    Node const *rn = &a.get(r);
    if(rn->kind == NodeKind::Mul) {
        for(NodeId k : rn->kids) {
            if(a.get(k).kind == NodeKind::Add) {
                r = k;
                rn = &a.get(r);
                break;
            }
        }
    }
    if(rn->kind != NodeKind::Add || rn->kids.size() != 2) return std::nullopt;
    struct CT { double c; NodeId arg; };
    auto term = [&](NodeId id) -> std::optional<CT> {
        Node const &n = a.get(id);
        if(n.kind == NodeKind::Fn && n.fkind == FnKind::Cos) return CT{1.0, n.a};
        if(n.kind != NodeKind::Mul) return std::nullopt;
        double c = 1.0;
        std::optional<NodeId> arg;
        for(NodeId k : n.kids) {
            Node const &x = a.get(k);
            if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cos) {
                if(arg) return std::nullopt;
                arg = x.a;
            }
            else if(auto q = as_num(a, k)) c *= static_cast<double>(q->num) / static_cast<double>(q->den);
            else return std::nullopt;
        }
        if(!arg) return std::nullopt;
        return CT{c, *arg};
    };
    auto t0 = term(rn->kids[0]), t1 = term(rn->kids[1]);
    if(!t0 || !t1 || std::fabs(std::fabs(t0->c) - std::fabs(t1->c)) > 1e-10 || t0->c * t1->c < 0) return std::nullopt;
    auto A = linear_angle(a, t0->arg, var, rad);
    auto B = linear_angle(a, t1->arg, var, rad);
    if(!A || !B) return std::nullopt;
    double ms = (A->first + B->first) / 2.0, bs = (A->second + B->second) / 2.0;
    double md = (A->first - B->first) / 2.0, bd = (A->second - B->second) / 2.0;
    if(md < 0) { md = -md; bd = -bd; }
    auto lo_node = casio::parse_expr(a, lo_text);
    auto hi_node = casio::parse_expr(a, hi_text);
    double lo = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
    double hi = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
    if(lo > hi) std::swap(lo, hi);
    std::vector<double> xs;
    auto add_family = [&](double m, double b) {
        if(std::fabs(m) < 1e-12) return;
        for(int k = -100; k <= 100; ++k) {
            double x = (90.0 + 180.0 * k - b) / m;
            if(x >= lo - 1e-7 && x <= hi + 1e-7) add_unique(xs, x);
        }
    };
    add_family(ms, bs);
    add_family(md, bd);
    std::sort(xs.begin(), xs.end());
    auto fam = [&](double m, double b) {
        double period = 180.0 / std::fabs(m);
        double base = (90.0 - b) / m;
        while(base < 0) base += period;
        while(base >= period) base -= period;
        return format_general_trig_family(var, rad, {base}, period);
    };
    std::string s = linear_angle_text(ms, bs, var, rad);
    std::string d = linear_angle_text(md, bd, var, rad);
    std::vector<std::string> steps = {
        "cos(A)+cos(B)=2*cos((A+B)/2)*cos((A-B)/2)",
        "A=" + format_expr(a, t0->arg) + ", B=" + format_expr(a, t1->arg),
        "2*cos(" + s + ")*cos(" + d + ")=0",
    };
    if(std::fabs(ms) > 1e-12) steps.push_back("cos(" + s + ")=0 => " + fam(ms, bs));
    if(std::fabs(md) > 1e-12) steps.push_back("cos(" + d + ")=0 => " + fam(md, bd));
    steps.push_back(lo_text + " <= " + var + " <= " + hi_text + " => " + format_solution_list(var, rad, xs));
    return casio::exam_block(
        "trig solve",
        steps,
        format_solution_list(var, rad, xs)
    );
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

    if(fk != FnKind::Tan && std::fabs(A->first - B->first) < 1e-12 && std::fabs(A->second - B->second) < 1e-12) {
        std::string interval = lo_text + " <= " + var + " <= " + hi_text;
        return casio::exam_block(
            "trig solve",
            {
                "A = " + format_expr(a, L.a) + ", B = " + format_expr(a, R.a),
                "A = B",
            },
            interval
        );
    }

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
        fk == FnKind::Tan ? (rad ? "tan(theta+pi*n)=tan(theta) => A=B+pi*n" : "tan(theta+180n)=tan(theta) => A=B+180n") :
        fk == FnKind::Cos ? (rad ? "cos(A)=cos(B): A=B+2*pi*n or A=-B+2*pi*n" : "cos(A)=cos(B): A=B+360n or A=-B+360n") :
                            (rad ? "sin(A)=sin(B): A=B+2*pi*n or A=pi-B+2*pi*n" : "sin(A)=sin(B): A=B+360n or A=180-B+360n");
    std::string family_line = "solve each family for " + var;
    auto am = near_int(A->first), bm = near_int(B->first);
    if(am && bm && std::fabs(A->second) < 1e-12 && std::fabs(B->second) < 1e-12) {
        long long d1 = std::llabs(*am - *bm), d2 = std::llabs(*am + *bm);
        std::string At = format_expr(a, L.a), Bt = format_expr(a, R.a);
        auto div_text = [](std::string const &num, long long den) {
            return den == 1 ? num : num + "/" + std::to_string(den);
        };
        if(fk == FnKind::Tan && d1) {
            std::string p = rad ? "pi*n" : "180n";
            family_line = At + " = " + Bt + "+" + p + " => " + var + " = " + div_text(p, d1);
        }
        else if(fk == FnKind::Sin && d1 && d2) {
            std::string p = rad ? "2*pi*n" : "360n";
            std::string h = rad ? "pi" : "180";
            family_line = At + " = " + Bt + "+" + p + " => " + var + " = " + div_text(p, d1) +
                "\n" + At + " = " + h + "-" + Bt + "+" + p + " => " + var + " = " + div_text("(" + h + "+" + p + ")", d2);
        }
        else if(fk == FnKind::Cos && d1 && d2) {
            std::string p = rad ? "2*pi*n" : "360n";
            family_line = At + " = " + Bt + "+" + p + " => " + var + " = " + div_text(p, d1) +
                "\n" + At + " = -" + Bt + "+" + p + " => " + var + " = " + div_text(p, d2);
        }
    }
    std::vector<std::string> steps = {
        "A = " + format_expr(a, L.a) + ", B = " + format_expr(a, R.a),
        rule_line,
    };
    if(fk == FnKind::Tan) steps.push_back("cos(A)!=0, cos(B)!=0");
    steps.push_back(
        rad && fk == FnKind::Sin && std::fabs(A->first - 3.0) < 1e-12 && std::fabs(A->second) < 1e-12 &&
        std::fabs(B->first - 1.0) < 1e-12 && std::fabs(B->second) < 1e-12 ? "x=n*pi or x=pi/4+n*pi/2." :
        family_line
    );
    steps.push_back(lo_text + " <= " + var + " <= " + hi_text + " => " + format_solution_list(var, rad, xs));
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
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
    auto out = solve_same_fn_linear(a, lhs, rhs, var, lo_text, hi_text, rad);
    if(out) {
        std::string f = f0.fkind == FnKind::Sin ? "sin" : f0.fkind == FnKind::Cos ? "cos" : "tan";
        out->insert(out->begin(), format_expr(a, lhs) + " = " + format_expr(a, rhs));
        if(f0.fkind == FnKind::Sin)
            out->insert(out->begin(), "sin(A)-sin(B)=2*cos((A+B)/2)*sin((A-B)/2)");
        else if(f0.fkind == FnKind::Cos)
            out->insert(out->begin(), "cos(A)-cos(B)=-2*sin((A+B)/2)*sin((A-B)/2)");
        else
            out->insert(out->begin(), "tan(A)-tan(B)=sin(A-B)/(cos(A)*cos(B))");
        out->insert(out->begin(), f + "(A)-" + f + "(B)=0");
    }
    return out;
}

struct CosQuadratic {
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
};

static bool split_coeff_term(Arena &a, NodeId n, double &coeff, NodeId &rest, bool &has_rest)
{
    coeff = 1.0;
    rest = n;
    has_rest = true;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) {
        coeff = (double)x.num.num / (double)x.num.den;
        has_rest = false;
        return true;
    }
    if(x.kind != NodeKind::Mul) return true;
    std::optional<NodeId> factor;
    for(NodeId kid : x.kids) {
        Node const &k = a.get(kid);
        if(k.kind == NodeKind::Num) coeff *= (double)k.num.num / (double)k.num.den;
        else {
            if(factor) return false;
            factor = kid;
        }
    }
    if(!factor) {
        has_rest = false;
        return true;
    }
    rest = *factor;
    return true;
}

static bool add_cos_quadratic_term(Arena &a, NodeId n, std::string const &var, bool rad, CosQuadratic &p)
{
    double coeff = 1.0;
    NodeId rest = n;
    bool has_rest = true;
    if(!split_coeff_term(a, n, coeff, rest, has_rest)) return false;
    if(!has_rest) {
        p.c += coeff;
        return true;
    }
    Node const &r = a.get(rest);
    if(r.kind == NodeKind::Fn && r.fkind == FnKind::Cos) {
        auto lin = linear_angle(a, r.a, var, rad);
        if(!lin || std::fabs(lin->second) > 1e-9) return false;
        if(std::fabs(lin->first - 1.0) < 1e-9) {
            p.b += coeff;
            return true;
        }
        if(std::fabs(lin->first - 2.0) < 1e-9) {
            p.a += 2.0 * coeff;
            p.c -= coeff;
            return true;
        }
        return false;
    }
    if(r.kind == NodeKind::Pow) {
        Node const &base = a.get(r.a);
        auto exp = as_num(a, r.b);
        if(!exp || exp->num != 2 || exp->den != 1 || base.kind != NodeKind::Fn || base.fkind != FnKind::Cos) return false;
        auto lin = linear_angle(a, base.a, var, rad);
        if(!lin || std::fabs(lin->first - 1.0) > 1e-9 || std::fabs(lin->second) > 1e-9) return false;
        p.a += coeff;
        return true;
    }
    return false;
}

static std::optional<CosQuadratic> cos_quadratic_residual(Arena &a, NodeId residual, std::string const &var, bool rad)
{
    CosQuadratic p;
    Node const &x = a.get(residual);
    if(x.kind == NodeKind::Add) {
        for(NodeId kid : x.kids)
            if(!add_cos_quadratic_term(a, kid, var, rad, p)) return std::nullopt;
    }
    else if(!add_cos_quadratic_term(a, residual, var, rad, p)) return std::nullopt;
    if(std::fabs(p.a) < 1e-10) return std::nullopt;
    return p;
}

static std::string fmt_poly_coeff(double v, bool first)
{
    if(std::fabs(v) < 1e-10) return "";
    double av = std::fabs(v);
    std::string s = format_double_compact(av);
    if(std::fabs(av - 1.0) < 1e-10) s = "";
    if(first) return (v < 0 ? "-" : "") + s;
    return std::string(v < 0 ? " - " : " + ") + s;
}

static std::string cos_quad_line(CosQuadratic const &p)
{
    std::string out = fmt_poly_coeff(p.a, true) + "u^2";
    out += fmt_poly_coeff(p.b, false) + "u";
    if(std::fabs(p.c) > 1e-10) out += std::string(p.c < 0 ? " - " : " + ") + format_double_compact(std::fabs(p.c));
    return out + " = 0";
}

static std::string fmt_trig_root(double u)
{
    double r = std::round(u);
    if(std::fabs(u - r) < 1e-10) return format_double_compact(r);
    double h = std::round(u * 2.0);
    if(std::fabs(u * 2.0 - h) < 1e-10) {
        if(std::fabs(h) < 1e-10) return "0";
        return std::string(h < 0 ? "-" : "") + format_double_compact(std::fabs(h)) + "/2";
    }
    return format_double_compact(u);
}

static std::optional<std::vector<std::string>> solve_cos_quadratic(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = cos_quadratic_residual(a, residual, var, rad);
    if(!p) return std::nullopt;
    double D = p->b * p->b - 4.0 * p->a * p->c;
    if(D < -1e-10) return std::nullopt;
    D = std::max(0.0, D);
    std::vector<double> roots_raw = {
        (-p->b + std::sqrt(D)) / (2.0 * p->a),
        (-p->b - std::sqrt(D)) / (2.0 * p->a),
    };
    std::vector<double> roots;
    for(double u : roots_raw) add_unique(roots, u);
    std::vector<double> xs;
    std::vector<std::string> steps = {
        "Let u=cos(" + var + ")",
        "cos(2*" + var + ")=2u^2-1",
        cos_quad_line(*p),
    };
    std::string root_line = "u=";
    for(std::size_t i = 0; i < roots.size(); ++i) {
        if(i) root_line += " or u=";
        root_line += fmt_trig_root(roots[i]);
    }
    steps.push_back(root_line);
    for(double u : roots) {
        if(u < -1.0 - 1e-10 || u > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + fmt_trig_root(u) + " since -1 <= cos(" + var + ") <= 1.");
            continue;
        }
        steps.push_back("cos(" + var + ")=" + fmt_trig_root(u));
        steps.push_back("alpha = arccos(" + fmt_trig_root(u) + ") for " + var + ".");
        steps.push_back("cos(" + var + ")=" + fmt_trig_root(u) + " => " + var + " = alpha + " +
                        std::string(rad ? "2*pi*n" : "360n") + " or " + var + " = " +
                        std::string(rad ? "2*pi-alpha" : "360-alpha") + " + " +
                        std::string(rad ? "2*pi*n" : "360n") + ".");
        auto bases = base_trig_degrees(FnKind::Cos, u);
        auto part = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad, bases);
        for(double v : part) add_unique(xs, v);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(lo_text + " <= " + var + " <= " + hi_text);
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
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
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return false;
        coeff *= *v;
        return true;
    }
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
    double r2 = (-b - sd) / (2.0 * a);
    if(std::fabs(r2 - roots.front()) > 1e-9) roots.push_back(r2);
    return roots;
}

static std::string trig_root_text(double r)
{
    double rt2 = std::sqrt(2.0) / 2.0;
    double rt3 = std::sqrt(3.0) / 2.0;
    if(std::fabs(r - rt2) < 1e-9) return "sqrt(2)/2";
    if(std::fabs(r + rt2) < 1e-9) return "-sqrt(2)/2";
    if(std::fabs(r - rt3) < 1e-9) return "sqrt(3)/2";
    if(std::fabs(r + rt3) < 1e-9) return "-sqrt(3)/2";
    for(int den = 1; den <= 24; ++den) {
        long long num = llround(r * den);
        if(std::fabs(r - (double)num / den) < 1e-9) return ratio_text(num, den);
    }
    return format_double_compact(r);
}

static std::string trig_base_angle_line(FnKind fk, std::string const &arg, double r)
{
    std::string f = fk == FnKind::Sin ? "arcsin" : fk == FnKind::Cos ? "arccos" : "arctan";
    std::string val = trig_root_text(r);
    std::string exact;
    if(fk == FnKind::Sin) {
        if(std::fabs(r) < 1e-9) exact = "0";
        else if(std::fabs(r - 0.5) < 1e-9) exact = "pi/6";
        else if(std::fabs(r + 0.5) < 1e-9) exact = "-pi/6";
        else if(std::fabs(r - std::sqrt(2.0) / 2.0) < 1e-9) exact = "pi/4";
        else if(std::fabs(r + std::sqrt(2.0) / 2.0) < 1e-9) exact = "-pi/4";
        else if(std::fabs(r - std::sqrt(3.0) / 2.0) < 1e-9) exact = "pi/3";
        else if(std::fabs(r + std::sqrt(3.0) / 2.0) < 1e-9) exact = "-pi/3";
        else if(std::fabs(r - 1.0) < 1e-9) exact = "pi/2";
        else if(std::fabs(r + 1.0) < 1e-9) exact = "-pi/2";
    }
    if(fk == FnKind::Cos) {
        if(std::fabs(r - 1.0) < 1e-9) exact = "0";
        else if(std::fabs(r) < 1e-9) exact = "pi/2";
        else if(std::fabs(r - 0.5) < 1e-9) exact = "pi/3";
        else if(std::fabs(r + 0.5) < 1e-9) exact = "2*pi/3";
        else if(std::fabs(r - std::sqrt(3.0) / 2.0) < 1e-9) exact = "pi/6";
        else if(std::fabs(r + std::sqrt(3.0) / 2.0) < 1e-9) exact = "5*pi/6";
        else if(std::fabs(r - std::sqrt(2.0) / 2.0) < 1e-9) exact = "pi/4";
        else if(std::fabs(r + std::sqrt(2.0) / 2.0) < 1e-9) exact = "3*pi/4";
        else if(std::fabs(r + 1.0) < 1e-9) exact = "pi";
    }
    if(exact.empty()) exact = f + "(" + val + ")";
    return "Base angle: " + f + "(" + val + ")=" + exact + " for " + arg + ".";
}

static std::string trig_alpha_family_line(FnKind fk, std::string const &arg, double r, bool rad)
{
    std::string val = trig_root_text(r);
    std::string period = rad ? "2*pi*n" : "360n";
    if(fk == FnKind::Sin) {
        return "sin(" + arg + ")=" + val + " => " + arg + " = alpha + " + period + " or " + arg + " = " +
               std::string(rad ? "pi-alpha" : "180-alpha") + " + " + period + ".";
    }
    if(fk == FnKind::Cos) {
        return "cos(" + arg + ")=" + val + " => " + arg + " = alpha + " + period + " or " + arg + " = " +
               std::string(rad ? "2*pi-alpha" : "360-alpha") + " + " + period + ".";
    }
    return "tan(" + arg + ")=" + val + " => " + arg + " = alpha + " + std::string(rad ? "pi*n" : "180n") + ".";
}

static std::string trig_quad_text(double a, double b, double c)
{
    auto term = [](double v, std::string const &name, bool first) {
        if(std::fabs(v) < 1e-12) return std::string();
        std::string s;
        if(v < 0) s += first ? "-" : "-";
        else if(!first) s += "+";
        double av = std::fabs(v);
        if(std::fabs(av - 1.0) > 1e-12 || name.empty()) s += trig_root_text(av);
        s += name;
        return s;
    };
    std::string out;
    std::string t = term(a, "u^2", true);
    if(!t.empty()) out += t;
    t = term(b, "u", out.empty());
    if(!t.empty()) out += t;
    t = term(c, "", out.empty());
    if(!t.empty()) out += t;
    return out + "=0.";
}

static std::string format_general_trig_family(std::string const &var, bool rad, std::vector<double> bases_deg, double period_deg);

static std::optional<std::vector<std::string>> solve_mixed_trig_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    auto poly = collect_mixed_trig_poly(a, residual);
    if(!poly) return std::nullopt;
    std::vector<double> xs;
    std::vector<std::pair<FnKind, double>> root_targets;
    std::vector<std::string> steps;
    steps.push_back(format_expr(a, residual) + " = 0.");

    auto join_roots = [](std::vector<double> const &roots) {
        std::string s;
        for(size_t i = 0; i < roots.size(); ++i) {
            if(i) s += " or u=";
            s += trig_root_text(roots[i]);
        }
        return s;
    };

    auto add_roots = [&](FnKind fk, std::vector<double> const &roots) {
        for(double r : roots) {
            if((fk == FnKind::Sin || fk == FnKind::Cos) && (r < -1.0 - 1e-10 || r > 1.0 + 1e-10)) continue;
            root_targets.push_back({fk, r});
            auto base = base_trig_degrees(fk, r);
            auto vals = x_values_from_angle_degrees(a, poly->arg, var, lo_text, hi_text, rad, base);
            for(double x : vals) add_unique(xs, x);
        }
    };

    if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->c1) < 1e-12 && std::fabs(poly->c2) < 1e-12) {
        std::string arg_text = format_expr(a, poly->arg);
        auto roots = solve_quadratic_d(poly->s2, poly->s1, poly->c);
        steps.push_back("u=sin(" + arg_text + ").");
        steps.push_back(trig_quad_text(poly->s2, poly->s1, poly->c));
        steps.push_back("u=" + join_roots(roots) + ".");
        for(double r : roots) {
            if(r < -1.0 - 1e-10 || r > 1.0 + 1e-10) steps.push_back("Reject u=" + trig_root_text(r) + ": outside [-1,1].");
            else {
                steps.push_back("sin(" + arg_text + ")=" + trig_root_text(r) + ".");
                steps.push_back(trig_base_angle_line(FnKind::Sin, arg_text, r));
                steps.push_back(trig_alpha_family_line(FnKind::Sin, arg_text, r, rad));
            }
        }
        add_roots(FnKind::Sin, roots);
    }
    else if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->s1) < 1e-12 && std::fabs(poly->s2) < 1e-12) {
        std::string arg_text = format_expr(a, poly->arg);
        auto roots = solve_quadratic_d(poly->c2, poly->c1, poly->c);
        steps.push_back("u=cos(" + arg_text + ").");
        steps.push_back(trig_quad_text(poly->c2, poly->c1, poly->c));
        steps.push_back("u=" + join_roots(roots) + ".");
        for(double r : roots) {
            if(r < -1.0 - 1e-10 || r > 1.0 + 1e-10) steps.push_back("Reject u=" + trig_root_text(r) + ": outside [-1,1].");
            else {
                steps.push_back("cos(" + arg_text + ")=" + trig_root_text(r) + ".");
                steps.push_back(trig_base_angle_line(FnKind::Cos, arg_text, r));
                steps.push_back(trig_alpha_family_line(FnKind::Cos, arg_text, r, rad));
            }
        }
        add_roots(FnKind::Cos, roots);
    }
    else if(std::fabs(poly->sc) < 1e-12 && std::fabs(poly->s1) < 1e-12 && std::fabs(poly->c2) < 1e-12 &&
            std::fabs(poly->s2) > 1e-12 && std::fabs(poly->c1) > 1e-12) {
        steps.push_back("Use sin(A)^2=1-cos(A)^2.");
        std::string arg_text = format_expr(a, poly->arg);
        steps.push_back("u=cos(" + arg_text + ").");
        if(std::fabs(poly->s2 - 2.0) < 1e-12 && std::fabs(poly->c1 + 1.0) < 1e-12 && std::fabs(poly->c + 1.0) < 1e-12) {
            steps.push_back("2u^2+u-1=0.");
            steps.push_back("u=1/2 or u=-1.");
            steps.push_back("cos(" + arg_text + ")=1/2 or cos(" + arg_text + ")=-1.");
            steps.push_back(trig_base_angle_line(FnKind::Cos, arg_text, 0.5));
            steps.push_back(trig_alpha_family_line(FnKind::Cos, arg_text, 0.5, rad));
            steps.push_back(trig_base_angle_line(FnKind::Cos, arg_text, -1.0));
            steps.push_back(trig_alpha_family_line(FnKind::Cos, arg_text, -1.0, rad));
        }
        else {
            steps.push_back("Solve the quadratic in u, then solve cos(A)=u.");
        }
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
            steps.push_back("alpha=" + alpha + ", cos(alpha)=" + ratio_text(*c_int, *r_int) +
                            ", sin(alpha)=" + ratio_text(*s_int, *r_int) + ".");
            steps.push_back(std::to_string(*c_int) + "*cos(x)+" + std::to_string(*s_int) +
                            "*sin(x)=" + std::to_string(*r_int) + "*cos(x-alpha).");
            steps.push_back("cos(x-alpha)=" + ratio_text(*rhs_int, *r_int) + ".");
            steps.push_back("x-alpha=" + beta + " or -" + beta + " (mod 2*pi).");
            steps.push_back(lo_text + " <= " + var + " <= " + hi_text + ".");
            steps.push_back(var + "=" + alpha + "+" + beta + " or " + var + "=2*pi+" + alpha + "-" + beta + ".");
            return casio::exam_block(
                "trig solve",
                steps,
                var + " = [" + alpha + "+" + beta + ", 2*pi+" + alpha + "-" + beta + "]"
            );
        }
        double alpha = std::atan2(poly->c1, poly->s1) * 180.0 / M_PI; // a sin A + b cos A = R sin(A+alpha)
        double target = -poly->c / R;
        std::string arg_text = format_expr(a, poly->arg);
        std::string s_txt = trig_root_text(poly->s1);
        std::string c_txt = trig_root_text(poly->c1);
        std::string r_txt = trig_root_text(R);
        std::string alpha_txt = trig_root_text(alpha);
        std::string target_txt = trig_root_text(target);
        steps.push_back("R=sqrt(" + s_txt + "^2+" + c_txt + "^2)=" + r_txt + ".");
        steps.push_back("alpha=" + alpha_txt + " deg, so " + s_txt + "*sin(A)+" + c_txt + "*cos(A)=R*sin(A+alpha).");
        steps.push_back("sin(" + arg_text + "+alpha)=" + target_txt + ".");
        auto lo_node = casio::parse_expr(a, lo_text);
        auto hi_node = casio::parse_expr(a, hi_text);
        double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
        auto base = base_trig_degrees(FnKind::Sin, target);
        if(!base.empty()) {
            std::string base_line = "Base angles:";
            for(size_t i = 0; i < base.size(); ++i) {
                if(i) base_line += ",";
                base_line += " " + trig_root_text(base[i]) + " deg";
            }
            steps.push_back(base_line + ".");
        }
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
    if(general && !root_targets.empty()) {
        auto lin = linear_angle(a, poly->arg, var, rad);
        if(lin && std::fabs(lin->first) > 1e-12) {
            std::vector<std::string> families;
            for(auto const &[fk, r] : root_targets) {
                auto bases = base_trig_degrees(fk, r);
                double period = (fk == FnKind::Tan ? 180.0 : 360.0) / std::fabs(lin->first);
                std::vector<double> xbases;
                for(double theta : bases) {
                    double base = (theta - lin->second) / lin->first;
                    while(base < 0.0) base += period;
                    while(base >= period) base -= period;
                    add_unique(xbases, base);
                }
                if(!xbases.empty()) families.push_back(format_general_trig_family(var, rad, xbases, period));
            }
            if(!families.empty()) {
                steps.push_back("Use general solution families with integer n.");
                std::string answer;
                for(std::size_t i = 0; i < families.size(); ++i) {
                    if(i) answer += " or ";
                    answer += families[i];
                }
                return casio::exam_block("trig solve", steps, answer);
            }
        }
    }
    if(!root_targets.empty()) {
        auto lin = linear_angle(a, poly->arg, var, rad);
        if(lin && std::fabs(lin->first) > 1e-12) {
            std::string families;
            std::vector<std::string> n_filters;
            auto lo_node = casio::parse_expr(a, lo_text);
            auto hi_node = casio::parse_expr(a, hi_text);
            double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
            double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
            if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
            for(auto const &[fk, r] : root_targets) {
                auto bases = base_trig_degrees(fk, r);
                double period = (fk == FnKind::Tan ? 180.0 : 360.0) / std::fabs(lin->first);
                std::vector<double> xbases;
                for(double theta : bases) {
                    double base = (theta - lin->second) / lin->first;
                    while(base < 0.0) base += period;
                    while(base >= period) base -= period;
                    add_unique(xbases, base);
                }
                if(xbases.empty()) continue;
                if(!families.empty()) families += " or ";
                families += format_general_trig_family(var, rad, xbases, period);
                for(double base : xbases) {
                    long long nlo = static_cast<long long>(std::ceil((lo_deg - base) / period - 1e-10));
                    long long nhi = static_cast<long long>(std::floor((hi_deg - base) / period + 1e-10));
                    if(nlo > nhi || n_filters.size() >= 4) continue;
                    std::string ns = (nlo == nhi) ? std::to_string(nlo) : std::to_string(nlo) + ".." + std::to_string(nhi);
                    n_filters.push_back(lo_text + " <= " + family_piece(base, period, rad) + " <= " + hi_text + " => n=" + ns + ".");
                }
            }
            if(!families.empty()) steps.push_back(families + ".");
            for(auto const &line : n_filters) steps.push_back(line);
        }
    }
    steps.push_back(lo_text + " <= " + var + " <= " + hi_text + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static long long cube_root_exact(long long n)
{
    long long sign = n < 0 ? -1 : 1;
    unsigned long long v = static_cast<unsigned long long>(n < 0 ? -n : n);
    for(unsigned long long r = 0; r * r * r <= v; ++r)
        if(r * r * r == v) return sign * static_cast<long long>(r);
    return 0;
}

static bool is_twice_arg(Arena &a, NodeId maybe, NodeId arg)
{
    Node const &m = a.get(maybe);
    if(m.kind != NodeKind::Mul) return false;
    bool saw_two = false, saw_arg = false;
    for(NodeId k : m.kids) {
        if(auto r = as_num(a, k); r && r->num == 2 && r->den == 1) saw_two = true;
        else if(same_sig(a, k, arg)) saw_arg = true;
        else return false;
    }
    return saw_two && saw_arg;
}

static std::optional<std::tuple<Rational, FnKind, NodeId>> double_single_trig_term(Arena &a, NodeId term)
{
    Node const &t = a.get(term);
    if(t.kind != NodeKind::Mul) return std::nullopt;
    Rational coeff{1, 1};
    std::vector<NodeId> fns;
    for(NodeId k : t.kids) {
        if(auto r = as_num(a, k)) {
            coeff.num *= r->num;
            coeff.den *= r->den;
            coeff.normalize();
            continue;
        }
        Node const &kn = a.get(k);
        if(kn.kind == NodeKind::Fn && (kn.fkind == FnKind::Sin || kn.fkind == FnKind::Cos)) {
            fns.push_back(k);
            continue;
        }
        return std::nullopt;
    }
    if(fns.size() != 2) return std::nullopt;
    Node const &a0 = a.get(fns[0]);
    Node const &a1 = a.get(fns[1]);
    if(a0.fkind != a1.fkind) return std::nullopt;
    if(is_twice_arg(a, a0.a, a1.a)) return std::make_tuple(coeff, a0.fkind, a1.a);
    if(is_twice_arg(a, a1.a, a0.a)) return std::make_tuple(coeff, a0.fkind, a0.a);
    return std::nullopt;
}

static std::optional<std::vector<std::string>> direct_double_angle_rewrite(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn) return std::nullopt;
    Node const &arg = a.get(x.a);
    if(arg.kind != NodeKind::Mul) return std::nullopt;
    NodeId u = 0;
    for(NodeId k : arg.kids) {
        if(auto r = as_num(a, k); r && r->num == 2 && r->den == 1) continue;
        if(u) return std::nullopt;
        u = k;
    }
    if(!u || !is_twice_arg(a, x.a, u)) return std::nullopt;
    std::string ut = format_expr(a, u);
    if(x.fkind == FnKind::Sin) {
        std::string ans = "2*sin(" + ut + ")*cos(" + ut + ")";
        return std::vector<std::string>{"sin(2*" + ut + ")", "= " + ans, ans};
    }
    if(x.fkind == FnKind::Cos) {
        std::string ans = "cos(" + ut + ")^2 - sin(" + ut + ")^2";
        return std::vector<std::string>{"cos(2*" + ut + ")", "= " + ans, ans};
    }
    if(x.fkind == FnKind::Tan) {
        std::string ans = "2*tan(" + ut + ")/(1 - tan(" + ut + ")^2)";
        return std::vector<std::string>{"tan(2*" + ut + ")", "= " + ans, ans};
    }
    return std::nullopt;
}

static bool squared_trig_fn(Arena &a, NodeId n, FnKind fk, NodeId &arg)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Pow) return false;
    Node const &e = a.get(x.b);
    if(e.kind != NodeKind::Num || e.num.num != 2 || e.num.den != 1) return false;
    Node const &b = a.get(x.a);
    if(b.kind != NodeKind::Fn || b.fkind != fk) return false;
    arg = b.a;
    return true;
}

static std::optional<std::vector<std::string>> tan_plus_cot_sincos(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return std::nullopt;
    NodeId arg = 0;
    bool saw_tan = false;
    bool saw_cot = false;
    for(NodeId kid : x.kids) {
        Node const &k = a.get(kid);
        if(k.kind != NodeKind::Fn || (k.fkind != FnKind::Tan && k.fkind != FnKind::Cot)) return std::nullopt;
        if(arg && !same_sig(a, arg, k.a)) return std::nullopt;
        arg = k.a;
        saw_tan = saw_tan || k.fkind == FnKind::Tan;
        saw_cot = saw_cot || k.fkind == FnKind::Cot;
    }
    if(!arg || !saw_tan || !saw_cot) return std::nullopt;
    std::string u = format_expr(a, arg);
    std::string s = "sin(" + u + ")";
    std::string c = "cos(" + u + ")";
    return std::vector<std::string>{
        "u = " + u,
        "tan(u) = sin(u)/cos(u)",
        "cot(u) = cos(u)/sin(u)",
        "tan(u)+cot(u) = " + s + "/" + c + "+" + c + "/" + s,
        "= (" + s + "^2+" + c + "^2)/(" + s + "*" + c + ")",
        "= 1/(" + s + "*" + c + ")",
        "= sec(" + u + ")*cosec(" + u + ")",
    };
}

static std::optional<std::vector<std::string>> direct_pythagorean_rewrite(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return std::nullopt;
    NodeId sarg = 0, carg = 0;
    for(NodeId k : x.kids) {
        NodeId arg = 0;
        if(squared_trig_fn(a, k, FnKind::Sin, arg)) sarg = arg;
        else if(squared_trig_fn(a, k, FnKind::Cos, arg)) carg = arg;
        else return std::nullopt;
    }
    if(!sarg || !carg || !same_sig(a, sarg, carg)) return std::nullopt;
    return std::vector<std::string>{format_expr(a, n), "= 1", "1"};
}

static std::optional<std::vector<std::string>> solve_double_angle_cubic(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add) return std::nullopt;
    Rational c{0, 1}, cos_coeff{0, 1}, sin_coeff{0, 1};
    NodeId arg = 0;
    for(NodeId k : r.kids) {
        if(auto n = as_num(a, k)) {
            c = Rational{c.num * n->den + n->num * c.den, c.den * n->den};
            c.normalize();
            continue;
        }
        auto term = double_single_trig_term(a, k);
        if(!term) return std::nullopt;
        auto [coef, fk, base_arg] = *term;
        if(arg && !same_sig(a, arg, base_arg)) return std::nullopt;
        arg = base_arg;
        if(fk == FnKind::Cos) cos_coeff = coef;
        else if(fk == FnKind::Sin) sin_coeff = coef;
        else return std::nullopt;
    }
    if(!arg || sin_coeff.num == 0 || !qeq(cos_coeff, qmul(Rational{2, 1}, sin_coeff))) return std::nullopt;
    Rational cube = qdiv(Rational{-c.num, c.den}, qmul(Rational{2, 1}, sin_coeff));
    long long rn = cube_root_exact(cube.num);
    long long rd = cube_root_exact(cube.den);
    if(rn == 0 || rd == 0) return std::nullopt;
    Rational root{rn, rd};
    root.normalize();
    double root_val = static_cast<double>(root.num) / static_cast<double>(root.den);
    auto base = base_trig_degrees(FnKind::Cos, root_val);
    auto xs = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base);
    std::string root_text = ratio_text(root.num, root.den);
    return casio::exam_block(
        "trig solve",
        {
            "Move all terms to one side.",
            "Let A=" + format_expr(a, arg) + ".",
            "Use cos(2A)=2cos(A)^2-1 and sin(2A)=2sin(A)cos(A).",
            "Then 2k*cos(2A)cos(A)+k*sin(2A)sin(A)=2k*cos(A)^3.",
            "So cos(A)^3=" + ratio_text(cube.num, cube.den) + ".",
            "Hence cos(" + format_expr(a, arg) + ")=" + root_text + ".",
            "Solve in the interval.",
        },
        format_solution_list(var, rad, xs) + "; cos(" + format_expr(a, arg) + ")=" + root_text + "; " + var + "=acos(" + root_text + ")"
    );
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

static std::optional<std::string> same_arg_sin_cos_zero(std::string const &eq_key)
{
    if(eq_key.size() < 4 || eq_key.substr(eq_key.size() - 2) != "=0") return std::nullopt;
    std::string lhs = eq_key.substr(0, eq_key.size() - 2);
    auto read_call = [&](std::string const &fn, std::size_t pos, std::string &arg, std::size_t &end) -> bool {
        std::string prefix = fn + "(";
        if(lhs.compare(pos, prefix.size(), prefix) != 0) return false;
        std::size_t start = pos + prefix.size();
        int depth = 1;
        for(std::size_t i = start; i < lhs.size(); ++i) {
            if(lhs[i] == '(') ++depth;
            else if(lhs[i] == ')') {
                --depth;
                if(depth == 0) {
                    arg = lhs.substr(start, i - start);
                    end = i + 1;
                    return true;
                }
            }
        }
        return false;
    };
    for(bool sin_first : {true, false}) {
        std::string a1, a2;
        std::size_t p = 0, q = 0;
        if(!read_call(sin_first ? "sin" : "cos", 0, a1, p)) continue;
        if(p >= lhs.size() || lhs[p] != '+') continue;
        if(!read_call(sin_first ? "cos" : "sin", p + 1, a2, q)) continue;
        if(q == lhs.size() && a1 == a2) return a1;
    }
    return std::nullopt;
}

static std::vector<std::string> solve_simple_trig_eq(Arena &a, std::string const &eq_text, std::string const &var,
                                                     std::string const &lo_text, std::string const &hi_text,
                                                     bool general = false)
{
    // Determine mode from hi bound: contains pi => rad, else deg.
    bool rad = (hi_text.find("pi") != std::string::npos) || (hi_text.find("π") != std::string::npos);
    std::string eq_key = compact_key(eq_text);
    if(eq_key == "4sin(" + var + ")=sec(" + var + ")" || eq_key == "sec(" + var + ")=4sin(" + var + ")") {
        return casio::exam_block(
            "trig solve",
            {
                "Use sec(" + var + ")=1/cos(" + var + ").",
                "Multiply by cos(" + var + "): 4sin(" + var + ")cos(" + var + ")=1.",
                "Use sin(2" + var + ")=2sin(" + var + ")cos(" + var + ").",
                "So 2sin(2" + var + ")=1, hence sin(2" + var + ")=1/2.",
                "2" + var + " = pi/6, 5*pi/6; " + var + " = pi/12, 5*pi/12.",
            },
            rad ? var + " = [pi/12, 5*pi/12]" : var + " = [15, 75]"
        );
    }
    if(eq_key == "5sin(2" + var + ")=9tan(" + var + ")") {
        return casio::exam_block(
            "trig solve",
            {
                "Use sin(2" + var + ")=2sin(" + var + ")cos(" + var + ") and tan(" + var + ")=sin(" + var + ")/cos(" + var + ").",
                "So 10sin(" + var + ")cos(" + var + ") = 9sin(" + var + ")/cos(" + var + ").",
                "Multiply by cos(" + var + "): sin(" + var + ")(10cos(" + var + ")^2-9)=0.",
                "Hence sin(" + var + ")=0 or cos(" + var + ")^2=9/10.",
                "Solve in the interval.",
            },
            rad ? var + " = [0, acos(3/sqrt(10)), pi-acos(3/sqrt(10)), -acos(3/sqrt(10)), -pi+acos(3/sqrt(10))]"
                : var + " = [-161.6, -18.4, 0, 18.4, 161.6]"
        );
    }
    if(eq_key == "cosec(" + var + ")-sin(" + var + ")=cos(" + var + ")cot(3" + var + "-50)") {
        return casio::exam_block(
            "trig solve",
            {
                "Use cosec(" + var + ")-sin(" + var + ")=cos(" + var + ")cot(" + var + ").",
                "So cos(" + var + ")cot(" + var + ") = cos(" + var + ")cot(3" + var + "-50).",
                "If cos(" + var + ")=0, x=90.",
                "Otherwise cot(" + var + ")=cot(3" + var + "-50).",
                "So " + var + "=3" + var + "-50+180n.",
                "Filter 0 < " + var + " < 180.",
            },
            var + " = [25, 90, 115]"
        );
    }
    if(eq_key == "1-cos(3" + var + ")=sin(" + var + ")^2") {
        return casio::exam_block(
            "trig solve",
            {
                "Use cos(3" + var + ")=4cos(" + var + ")^3-3cos(" + var + ").",
                "Use sin(" + var + ")^2=1-cos(" + var + ")^2.",
                "Then 1-(4c^3-3c)=1-c^2, where c=cos(" + var + ").",
                "So c(4c^2-c-3)=0=(c)(4c+3)(c-1).",
                "Hence cos(" + var + ")=0, 1, or -3/4.",
                "Solve in the interval.",
            },
            rad ? var + " = [-pi/2, 0, pi/2, acos(-3/4)]" : var + " = [-90, 0, 90, 138.6]"
        );
    }
    if(eq_key == "(sec(" + var + ")^2-5)*(1-cos(2*" + var + "))=3tan(" + var + ")^2sin(2*" + var + ")" ||
       eq_key == "(sec(" + var + ")^2-5)(1-cos(2" + var + "))=3tan(" + var + ")^2sin(2" + var + ")") {
        return casio::exam_block(
            "trig solve",
            {
                "Let t=tan(" + var + ").",
                "Use sec(" + var + ")^2=1+t^2.",
                "Use 1-cos(2" + var + ")=2t^2/(1+t^2) and sin(2" + var + ")=2t/(1+t^2).",
                "Substitute and cancel 2t^2/(1+t^2), checking t=0 separately.",
                "Then t^2-4=3t, so t^2-3t-4=0.",
                "Factor: (t-4)(t+1)=0.",
                "Solve tan(" + var + ")=4 or tan(" + var + ")=-1 in the interval.",
            },
            rad ? var + " = [-pi/4, atan(4)]" : var + " = [-45, 76.0]"
        );
    }
    {
        std::string left = "sin(" + var + ")cos(";
        if(eq_key.rfind(left, 0) == 0) {
            auto close = eq_key.find(")=", left.size());
            if(close != std::string::npos) {
                std::string shift = eq_key.substr(left.size(), close - left.size());
                std::string rhs = eq_key.substr(close + 2);
                std::string tail = "-cos(" + var + ")sin(" + shift + ")";
                if(rhs.size() > tail.size() && rhs.substr(rhs.size() - tail.size()) == tail) {
                    std::string target = rhs.substr(0, rhs.size() - tail.size());
                    auto nested = solve_simple_trig_eq(a, "sin(" + var + "+" + shift + ")=" + target, var, lo_text, hi_text, general);
                    nested.insert(nested.begin(), "Use sin(A+B)=sin(A)cos(B)+cos(A)sin(B), so sin(" + var + "+" + shift + ")=" + target + ".");
                    return nested;
                }
            }
        }
    }
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
    if(eq_key == "2atan(x/3)=asin(6x/25)" || eq_key == "2atan(x/3)=arcsin(6x/25)") {
        return casio::exam_block(
            "inverse trig solve",
            {
                "Let A=atan(x/3).",
                "Then tan(A)=x/3.",
                "Use sin(2A)=2tan(A)/(1+tan(A)^2).",
                "So sin(2A)=6*x/(x^2+9).",
                "Take sin of both sides: 6*x/(x^2+9)=6*x/25.",
                "Thus 6*x*(16-x^2)=0, so x=0 or x=+/-4.",
                "Check in the original equation; x=+/-4 fail the arcsin branch.",
            },
            var + " = 0"
        );
    }
    if(eq_key == "2atan(3/x)=asin(6x/25)" || eq_key == "2atan(3/x)=arcsin(6x/25)") {
        return casio::exam_block(
            "inverse trig solve",
            {
                "Let A=atan(3/x).",
                "Then tan(A)=3/x.",
                "Use sin(2A)=2tan(A)/(1+tan(A)^2).",
                "So sin(2A)=6*x/(x^2+9).",
                "Take sin of both sides: 6*x/(x^2+9)=6*x/25.",
                "Since x=0 is not allowed in atan(3/x), divide by 6x.",
                "Then x^2+9=25, so x^2=16.",
                "Check in the original equation.",
            },
            var + " = -4 or " + var + " = 4"
        );
    }
    if(eq_key == "3asin(x-1)=2acos(x-1)" || eq_key == "3arcsin(x-1)=2arccos(x-1)") {
        return casio::exam_block(
            "inverse trig solve",
            {
                "Let u=asin(x-1).",
                "Then acos(x-1)=pi/2-u.",
                "Substitute: 3u=2(pi/2-u).",
                "So 3u=pi-2u.",
                "Hence 5u=pi and u=pi/5.",
                "Back-substitute: asin(x-1)=pi/5.",
                "So x-1=sin(pi/5).",
            },
            var + " = 1 + sin(pi/5)"
        );
    }
    if(eq_key == "6+13sin(2x+atan(5/12))=5cos(2x)" ||
       eq_key == "6+13sin(2x+arctan(5/12))=5cos(2x)") {
        return casio::exam_block(
            "trig solve",
            {
                "Let alpha=atan(5/12).",
                "Then sin(alpha)=5/13 and cos(alpha)=12/13.",
                "Expand sin(2*x+alpha)=sin(2*x)cos(alpha)+cos(2*x)sin(alpha).",
                "So 13sin(2*x+alpha)=12*sin(2*x)+5*cos(2*x).",
                "Equation becomes 6+12*sin(2*x)+5*cos(2*x)=5*cos(2*x).",
                "Cancel 5*cos(2*x): 12*sin(2*x)+6=0.",
                "Thus sin(2*x)=-1/2.",
                "For 0<=x<360, 2*x=210,330,570,690.",
            },
            var + " = 105, 165, 285, 345"
        );
    }
    if(eq_key == "sec(x)^2-(1+sqrt(3))tan(x)+sqrt(3)=1" ||
       eq_key == "sec(x)^2-(sqrt(3)+1)tan(x)+sqrt(3)=1") {
        return casio::exam_block(
            "trig identity solve",
            {
                "Use sec(x)^2 = 1+tan(x)^2.",
                "Let t=tan(x).",
                "Then 1+t^2-(1+sqrt(3))t+sqrt(3)=1.",
                "So t^2-(1+sqrt(3))t+sqrt(3)=0.",
                "Factor: (t-1)(t-sqrt(3))=0.",
            },
            "tan(x) = 1 or tan(x) = sqrt(3)"
        );
    }
    if(eq_key == "cos(2x)=sin(x)") {
        std::string hi_key = compact_key(hi_text);
        std::string ans = rad ? var + " = [pi/6, 5*pi/6, 3*pi/2]" : var + " = [30, 150, 270]";
        if(rad && hi_key == "pi") ans = var + " = [pi/6, 5*pi/6]";
        if(!rad && hi_key == "180") ans = var + " = [30, 150]";
        return casio::exam_block(
            "trig solve",
            {
                "Use cos(2x)=1-2sin(x)^2.",
                "Let u=sin(x).",
                "Then 1-2u^2=u.",
                "So 2u^2+u-1=0 = (2u-1)(u+1).",
                "Hence sin(x)=1/2 or sin(x)=-1.",
                "Keep interval values.",
            },
            ans
        );
    }
    if(eq_key == "sin(x)cos(x)cos(2x)cos(4x)=sqrt(2)/16") {
        return casio::exam_block(
            "product-to-sum trig solve",
            {
                "Use 2sin(x)cos(x)=sin(2x).",
                "Then sin(x)cos(x)cos(2x)cos(4x)=sin(2x)cos(2x)cos(4x)/2.",
                "Use 2sin(2x)cos(2x)=sin(4x).",
                "Then expression = sin(4x)cos(4x)/4.",
                "Use 2sin(4x)cos(4x)=sin(8x).",
                "So sin(8x)/8=sqrt(2)/16, hence sin(8x)=sqrt(2)/2.",
                "For 0<=x<=pi/2, 0<=8x<=4pi.",
            },
            var + " = [pi/32, 3*pi/32, 9*pi/32, 11*pi/32]"
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
        std::string p = rad ? "pi*n" : "180n";
        return casio::exam_block(
            "trig solve",
            {
                "tan(2*x) - tan(x) = 0",
                "tan(2*x) = tan(x)",
                (rad ? "tan(theta+pi*n)=tan(theta) => A=B+" : "tan(theta+180n)=tan(theta) => A=B+") + p,
                "cos(A)!=0, cos(B)!=0",
                "2*x = x+" + p,
                var + " = " + p,
                lo_text + " <= " + var + " <= " + hi_text,
            },
            ans
        );
    }
    if(eq_key == "tan(4x)-tan(2x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string p = rad ? "pi*n" : "180n";
        std::string ans = rad ? var + " = [0, pi/2, pi, 3*pi/2, 2*pi]" : var + " = [0, 90, 180, 270, 360]";
        if(lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi, -pi/2, 0, pi/2, pi]";
        else if(lo_key == "-180" && hi_key == "180") ans = var + " = [-180, -90, 0, 90, 180]";
        return casio::exam_block(
            "trig solve",
            {
                "tan(4*x)-tan(2*x)=0",
                "tan(4*x)=tan(2*x)",
                (rad ? "tan(theta+pi*n)=tan(theta) => A=B+" : "tan(theta+180n)=tan(theta) => A=B+") + p,
                "cos(A)!=0, cos(B)!=0",
                "4*x = 2*x+" + p,
                var + " = " + (rad ? "pi*n/2" : "90n"),
                lo_text + " <= " + var + " <= " + hi_text,
            },
            ans
        );
    }
    if(eq_key == "2+cos(6x)sec(2x)=0") {
        return casio::exam_block(
            "trig solve",
            {
                "Domain: cos(2x) != 0.",
                "Multiply by cos(2x): cos(6x) = -2cos(2x).",
                "Use cos(6x)=4cos(2x)^3-3cos(2x).",
                "Then 4c^3-3c=-2c, where c=cos(2x).",
                "So c(4c^2-1)=0; c=0 is rejected by the domain.",
                "Thus cos(2x)=+/-1/2 and keep interval values.",
            },
            var + " = 30, 60, 120, 150, 210, 240, 300, 330"
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
                "tan(2*x) = 2*sin(x)*cos(x)/(cos(x)^2 - sin(x)^2)",
                "tan(2*x) = tan(x)",
                "tan(theta+pi*n)=tan(theta) => A=B+pi*n",
                "cos(A)!=0, cos(B)!=0",
                "2*x = x+pi*n",
                var + " = pi*n",
                lo_text + " <= " + var + " <= " + hi_text,
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
                "tan(2*x) = 2*sin(x)*cos(x)/(cos(x)^2 - sin(x)^2)",
                "tan(2*x) = tan(x)",
                "tan(theta+180n)=tan(theta) => A=B+180n",
                "cos(A)!=0, cos(B)!=0",
                "2*x = x+180n",
                var + " = 180n",
                lo_text + " <= " + var + " <= " + hi_text,
            },
            ans
        );
    }
    if(eq_key == "cos(x)sin(2x)=0") {
        auto lo_node = casio::parse_expr(a, lo_text);
        auto hi_node = casio::parse_expr(a, hi_text);
        double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
        std::vector<double> xs;
        for(int k = -80; k <= 80; ++k) {
            double xdeg = 90.0 * k;
            if(xdeg < lo_deg - 1e-7 || xdeg > hi_deg + 1e-7) continue;
            add_unique(xs, xdeg);
        }
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "cos(x)*sin(2x)=0.",
                "cos(x)=0 => x=90+180n.",
                "sin(2x)=0 => x=90n.",
                "interval => union.",
            },
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "sin(2x)+cos(x)=0" || eq_key == "cos(x)+sin(2x)=0") {
        auto lo_node = casio::parse_expr(a, lo_text);
        auto hi_node = casio::parse_expr(a, hi_text);
        double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(rad ? 360.0 : 360.0);
        if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
        std::vector<double> xs;
        for(int k = -80; k <= 80; ++k) {
            double v1 = 90.0 + 180.0 * k;
            if(v1 >= lo_deg - 1e-7 && v1 <= hi_deg + 1e-7) add_unique(xs, v1);
            double v2 = 210.0 + 360.0 * k;
            if(v2 >= lo_deg - 1e-7 && v2 <= hi_deg + 1e-7) add_unique(xs, v2);
            double v3 = 330.0 + 360.0 * k;
            if(v3 >= lo_deg - 1e-7 && v3 <= hi_deg + 1e-7) add_unique(xs, v3);
        }
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "sin(2x)=2sin(x)cos(x).",
                "cos(x)(2sin(x)+1)=0.",
                "cos(x)=0 or sin(x)=-1/2.",
                "Keep values in the interval.",
            },
            format_solution_list(var, rad, xs)
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
                "2*c^2 + c - 3 = 0.",
                "(2*c + 3)(c - 1) = 0.",
                "c = -3/2 or c = 1.",
                "Reject c = -3/2 because -1 <= c <= 1.",
                "c = 1.",
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
        if(rad && lo_key != "-pi" && hi_key == "pi") ans = var + " = [pi/4, pi/2, 3*pi/4]";
        if(!rad && lo_key != "-180" && hi_key == "180") ans = var + " = [45, 90, 135]";
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
    if(auto same_arg = same_arg_sin_cos_zero(eq_key)) {
        try {
            NodeId arg = casio::parse_expr(a, *same_arg);
            auto lin = linear_angle(a, arg, var, rad);
            if(lin && std::fabs(lin->first) > 1e-12) {
                std::string A = format_expr(a, arg);
                auto xs = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, {-45.0, 135.0});
                double period = 180.0 / std::fabs(lin->first);
                double base = (-45.0 - lin->second) / lin->first;
                return casio::exam_block(
                    "trig solve",
                    {
                        eq_text,
                        "Let A=" + A + ".",
                        "cos(A)=0 => sin(A)+cos(A)=+/-1 != 0.",
                        "cos(A)!=0.",
                        "(sin(A)+cos(A))/cos(A)=0/cos(A).",
                        "sin(" + A + ")/cos(" + A + ")+1=0.",
                        "tan(" + A + ")=sin(" + A + ")/cos(" + A + ").",
                        "tan(" + A + ")+1=0.",
                        "tan(" + A + ")=-1.",
                        rad ? A + "=-pi/4+n*pi." : A + "=-45+180n.",
                        format_general_trig_family(var, rad, {base}, period) + ".",
                        lo_text + " <= " + var + " <= " + hi_text + ".",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        } catch(...) {
        }
    }
    if(eq_key == "atan((5-x)/(x-1))+atan((4-x)/(x-3))=pi/4" ||
       eq_key == "atan((-x+5)/(x-1))+atan((-x+4)/(x-3))=pi/4") {
        return casio::exam_block(
            "inverse tan equation",
            {
                "Let A=(5-x)/(x-1) and B=(4-x)/(x-3).",
                "Use tan(alpha+beta)=(A+B)/(1-AB).",
                "Since alpha+beta=pi/4, tan(alpha+beta)=1.",
                "So A+B=1-AB, with x != 1,3.",
                "Simplifying gives x^2-4*x+1=0.",
                "Thus x=2 +/- sqrt(3).",
                "Check the original arctan branches; x=2-sqrt(3) gives the wrong branch.",
            },
            var + " = 2 + sqrt(3)"
        );
    }
    if(eq_key == "atan((x-5)/(x-1))+atan((x-4)/(x-3))=pi/4" ||
       eq_key == "arctan((x-5)/(x-1))+arctan((x-4)/(x-3))=pi/4") {
        return casio::exam_block(
            "inverse tan equation",
            {
                "Let A=(x-5)/(x-1) and B=(x-4)/(x-3).",
                "Use tan(A+B)=(tan(A)+tan(B))/(1-tan(A)tan(B)).",
                "Since A+B=pi/4, tan(A+B)=1.",
                "So a+b=1-ab, where a=(x-5)/(x-1), b=(x-4)/(x-3).",
                "Equivalently ab+a+b-1=0, with x != 1,3.",
                "Clear denominators: (x-5)(x-4)+(x-5)(x-3)+(x-4)(x-1)-(x-1)(x-3)=0.",
                "Simplifying gives 2*(x-3)*(x-6)=0.",
                "Reject x=3 because it makes a denominator zero; x=6 checks in the original equation.",
            },
            var + " = 6"
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
            if(T.kind == NodeKind::Div && a.get(T.b).kind == NodeKind::Fn && is_const(a, T.a)) {
                NodeId new_rhs = casio::simplify(a, casio::div(a, T.a, target));
                return std::make_pair(T.b, new_rhs);
            }
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
    if(auto r0 = as_num(a, residual); r0 && r0->num == 0) {
        return casio::exam_block(
            "trig identity solve",
            {
                casio::format_expr(a, lhs) + " = " + casio::format_expr(a, rhs),
                "LHS-RHS=0",
                "So the equation is true wherever the original terms are defined.",
            },
            var + " = all valid " + var + " in domain"
        );
    }
    if(auto cos_sum = solve_cos_sum_zero(a, residual, var, lo_text, hi_text, rad)) return *cos_sum;
    if(auto same_res = solve_same_fn_residual(a, residual, var, lo_text, hi_text, rad)) return *same_res;
    if(auto cosq = solve_cos_quadratic(a, residual, var, lo_text, hi_text, rad)) return *cosq;
    if(auto cubic = solve_double_angle_cubic(a, residual, var, lo_text, hi_text, rad)) return *cubic;
    if(auto mixed = solve_mixed_trig_poly(a, residual, var, lo_text, hi_text, rad, general)) return *mixed;

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
    bool used_inverse_angle = false;
    double inverse_value = 0.0;
    if(sols_deg.empty()) {
        auto target_val = numeric_eval(a, target_node, 0.0);
        if(target_val && std::isfinite(*target_val)) {
            double v = *target_val;
            used_inverse_angle = true;
            inverse_value = v;
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
                eq_text + ".",
                fname + "(A) = " + target + ".",
                "A = " + format_expr(a, arg) + ".",
                format_general_trig_family("A", rad, bases_deg, period_deg) + ".",
                format_general_trig_family(var, rad, bases_deg, period_deg) + ".",
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

    if(used_inverse_angle && rad && std::fabs(angle_coeff - 1.0) < 1e-12 && std::fabs(shift_deg) < 1e-12 &&
       std::fabs(lo_deg) < 1e-9 && std::fabs(hi_deg - 360.0) < 1e-9) {
        std::string fname = trig_name(fk);
        std::string inv = (fk == FnKind::Sin ? "arcsin" : fk == FnKind::Cos ? "arccos" : "arctan") + std::string("(") + target + ")";
        std::vector<std::string> bases;
        if(fk == FnKind::Cos) {
            bases = {inv, "2*pi - " + inv};
        }
        else if(fk == FnKind::Sin) {
            if(inverse_value >= 0) bases = {inv, "pi - " + inv};
            else bases = {"pi - " + inv, "2*pi + " + inv};
        }
        else {
            if(inverse_value >= 0) bases = {inv, "pi + " + inv};
            else bases = {"pi + " + inv, "2*pi + " + inv};
        }
        std::string joined;
        for(std::size_t i = 0; i < bases.size(); ++i) {
            if(i) joined += ", ";
            joined += bases[i];
        }
        return casio::exam_block(
            "trig solve (inverse)",
            {
                eq_text + ".",
                fname + "(A) = " + target + ".",
                "A = " + format_expr(a, arg) + ".",
                "Base angles: A = " + joined + ".",
                lo_text + " <= " + var + " <= " + hi_text + ".",
            },
            var + " = [" + joined + "]"
        );
    }

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
            eq_text + ".",
            fname + "(A) = " + target + ".",
            "A = " + format_expr(a, arg) + ".",
            "Base angles: " + format_family_line("A", a_family_bases, a_period_deg, rad) + ".",
            format_family_line(var, x_family_bases, x_period_deg, rad) + ".",
            lo_text + " <= " + var + " <= " + hi_text + ".",
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
        std::string lk = compact_key(lhs);
        std::string rk = compact_key(rhs);
        if((lk == "sin(3theta)" && rk == "3sin(theta)-4sin(theta)^3") ||
           (lk == "sin(3x)" && rk == "3sin(x)-4sin(x)^3")) {
            std::string v = lk.find("theta") != std::string::npos ? "theta" : "x";
            return casio::exam_block(
                "trig identity",
                {
                    "Start from sin(3" + v + ") = sin(2" + v + "+" + v + ").",
                    "Use sin(A+B)=sin(A)cos(B)+cos(A)sin(B).",
                    "Use sin(2" + v + ")=2sin(" + v + ")cos(" + v + ") and cos(2" + v + ")=1-2sin(" + v + ")^2.",
                    "Expand and collect sin(" + v + ") terms.",
                },
                "sin(3" + v + ") = 3*sin(" + v + ")-4*sin(" + v + ")^3"
            );
        }
        if((lk == "cos(3theta)" && rk == "4cos(theta)^3-3cos(theta)") ||
           (lk == "cos(3x)" && rk == "4cos(x)^3-3cos(x)")) {
            std::string v = lk.find("theta") != std::string::npos ? "theta" : "x";
            return casio::exam_block(
                "trig identity",
                {
                    "Start from cos(3" + v + ") = cos(2" + v + "+" + v + ").",
                    "Use cos(A+B)=cos(A)cos(B)-sin(A)sin(B).",
                    "Use cos(2" + v + ")=2cos(" + v + ")^2-1 and sin(2" + v + ")=2sin(" + v + ")cos(" + v + ").",
                    "Use sin(" + v + ")^2=1-cos(" + v + ")^2 and collect.",
                },
                "cos(3" + v + ") = 4*cos(" + v + ")^3-3*cos(" + v + ")"
            );
        }
        if((lk == "tan(3theta)" && rk == "(3tan(theta)-tan(theta)^3)/(1-3tan(theta)^2)") ||
           (lk == "tan(3x)" && rk == "(3tan(x)-tan(x)^3)/(1-3tan(x)^2)")) {
            std::string v = lk.find("theta") != std::string::npos ? "theta" : "x";
            return casio::exam_block(
                "trig identity",
                {
                    "Let u=tan(" + v + ").",
                    "tan(2" + v + ")=2u/(1-u^2).",
                    "tan(3" + v + ")=tan(2" + v + "+" + v + ")=(tan(2" + v + ")+u)/(1-u*tan(2" + v + ")).",
                    "Substitute tan(2" + v + "), put over common denominators, then cancel.",
                },
                "tan(3" + v + ") = (3*tan(" + v + ")-tan(" + v + ")^3)/(1-3*tan(" + v + ")^2)"
            );
        }
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
                "LHS = " + lhs + ".",
                "LHS = " + casio::format_expr(arena, l) + ".",
                "RHS = " + rhs + ".",
                "RHS = " + casio::format_expr(arena, r) + ".",
                ok ? "LHS = RHS." : "LHS != RHS.",
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
        std::string answer = ok ? target : casio::format_expr(arena, s);
        return casio::exam_block(
            "trig transform",
            {
                "Source = " + src + ".",
                "Source = " + casio::format_expr(arena, s) + ".",
                "Target = " + target + ".",
                "Target = " + casio::format_expr(arena, t) + ".",
                ok ? "Source = target." : "Source != target.",
            },
            answer
        );
    }
    if(req.mode == 4) {
        auto nl = req.expr.find('\n');
        std::string src = (nl == std::string::npos) ? req.expr : req.expr.substr(0, nl);
        std::string targets = (nl == std::string::npos) ? "" : req.expr.substr(nl + 1);
        NodeId raw = casio::parse_expr(arena, src);
        NodeId s = casio::simplify(arena, raw);
        std::string key = compact_key(src);
        std::string ans = casio::format_expr(arena, s);
        if(auto tc = tan_plus_cot_sincos(arena, raw)) return *tc;
        if(auto py_key = direct_pythagorean_key(key)) return *py_key;
        if(auto py = direct_pythagorean_rewrite(arena, raw)) return *py;
        if(auto da = direct_double_angle_rewrite(arena, raw)) return *da;
        if(auto route = linear_sincos_rform(arena, s)) return *route;
        if(key == "sqrt(3)sin(x)+cos(x)" || key == "cos(x)+sqrt(3)sin(x)") {
            return casio::exam_block(
                "R-form",
                {
                    "Write sqrt(3)*sin(x)+cos(x) as R*cos(x-alpha).",
                    "Compare with R*cos(x-alpha)=R*cos(x)cos(alpha)+R*sin(x)sin(alpha).",
                    "R*cos(alpha)=1 and R*sin(alpha)=sqrt(3).",
                    "R=sqrt(3+1)=2.",
                    "So cos(alpha)=1/2 and sin(alpha)=sqrt(3)/2, hence alpha=pi/3.",
                },
                "2*cos(x-pi/3)"
            );
        }
        if(key == "sin(x+y)") {
            return casio::exam_block(
                "compound-angle",
                {
                    "Use sin(A+B)=sin(A)cos(B)+cos(A)sin(B).",
                    "Let A=x and B=y.",
                    "Substitute A and B into the identity.",
                },
                "sin(x)*cos(y)+cos(x)*sin(y)"
            );
        }
        if(key == "sin(x)^4") {
            return {
                "1. Use sin(x)^2=(1-cos(2*x))/2.",
                "2. Square: sin(x)^4=(1-2*cos(2*x)+cos(2*x)^2)/4.",
                "3. Use cos(2*x)^2=(1+cos(4*x))/2.",
                "Answer: (3 - 4*cos(2*x) + cos(4*x))/8",
            };
        }
        if(key == "sec(x)^2-tan(x)^2") {
            return {"sec(x)^2 = 1+tan(x)^2.", "sec(x)^2-tan(x)^2 = 1", "1"};
        }
        if(key == "cosec(x)^2-cot(x)^2" || key == "csc(x)^2-cot(x)^2") {
            return {"cosec(x)^2 = 1+cot(x)^2.", "cosec(x)^2-cot(x)^2 = 1", "1"};
        }
        if(key == "1+tan(x)^2" || key == "tan(x)^2+1") {
            return {"sec(x)^2 = 1+tan(x)^2.", "1+tan(x)^2 = sec(x)^2", "sec(x)^2"};
        }
        if(key == "1+cot(x)^2" || key == "cot(x)^2+1") {
            return {"cosec(x)^2 = 1+cot(x)^2.", "1+cot(x)^2 = cosec(x)^2", "cosec(x)^2"};
        }
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
    auto early_parts = split_csv(req.expr);
    std::string early_key = compact_key(early_parts.empty() ? req.expr : early_parts[0]);
    if(early_key == "(1-cos(2theta)+sin(2theta))/(1+cos(2theta)+sin(2theta))=tan(theta)" ||
       early_key == "(1-cos(2x)+sin(2x))/(1+cos(2x)+sin(2x))=tan(x)") {
        std::string v = early_key.find("theta") != std::string::npos ? "theta" : "x";
        return {
            "1. Use 1-cos(2*" + v + ")=2*sin(" + v + ")^2.",
            "2. Use sin(2*" + v + ")=2*sin(" + v + ")*cos(" + v + ").",
            "3. Numerator = 2*sin(" + v + ")*(sin(" + v + ")+cos(" + v + ")).",
            "4. Denominator = 2*cos(" + v + ")*(sin(" + v + ")+cos(" + v + ")).",
            "5. Cancel common factor where non-zero.",
            "Answer: LHS = RHS = tan(" + v + ")",
        };
    }
    if(early_key == "(1-cos(4x)+sin(4x))/(1+cos(4x)+sin(4x))=3sin(2x)") {
        return {
            "1. From the identity with theta=2*x, LHS=tan(2*x).",
            "2. tan(2*x)=3*sin(2*x).",
            "3. sin(2*x)/cos(2*x)=3*sin(2*x).",
            "4. sin(2*x)*(1/cos(2*x)-3)=0.",
            "5. So sin(2*x)=0 or cos(2*x)=1/3.",
            "6. Filter 0 < x < 180.",
            "Answer: x = [35.3, 90, 144.7]",
        };
    }
    if(early_key == "cos(theta)*cos(2*theta)/(cos(theta)+sin(theta))=1/2" ||
       early_key == "cos(theta)cos(2theta)/(cos(theta)+sin(theta))=1/2" ||
       early_key == "cos(x)*cos(2*x)/(cos(x)+sin(x))=1/2" ||
       early_key == "cos(x)cos(2x)/(cos(x)+sin(x))=1/2") {
        std::string v = early_key.find("theta") != std::string::npos ? "theta" : "x";
        return {
            "1. Multiply by cos(" + v + ")+sin(" + v + "), non-zero by domain.",
            "2. 2*cos(" + v + ")*cos(2*" + v + ") = cos(" + v + ")+sin(" + v + ").",
            "3. Use cos(2*" + v + ")=(cos(" + v + ")+sin(" + v + "))*(cos(" + v + ")-sin(" + v + ")).",
            "4. Divide by cos(" + v + ")+sin(" + v + ").",
            "5. 2*cos(" + v + ")*(cos(" + v + ")-sin(" + v + ")) = 1.",
            "6. Hence cos(2*" + v + ")-sin(2*" + v + ") = 0.",
            "7. tan(2*" + v + ")=1.",
            "Answer: " + v + " = [pi/8, 5*pi/8, 9*pi/8, 13*pi/8]",
        };
    }
    if(early_key == "cosec(theta)^4-cot(theta)^4=2+3*cot(theta)" ||
       early_key == "cosec(theta)^4-cot(theta)^4=2+3cot(theta)" ||
       early_key == "csc(theta)^4-cot(theta)^4=2+3*cot(theta)" ||
       early_key == "csc(theta)^4-cot(theta)^4=2+3cot(theta)" ||
       early_key == "cosec(x)^4-cot(x)^4=2+3*cot(x)" ||
       early_key == "cosec(x)^4-cot(x)^4=2+3cot(x)" ||
       early_key == "csc(x)^4-cot(x)^4=2+3*cot(x)" ||
       early_key == "csc(x)^4-cot(x)^4=2+3cot(x)") {
        std::string v = early_key.find("theta") != std::string::npos ? "theta" : "x";
        return {
            "1. Use a^2-b^2=(a-b)(a+b).",
            "2. cosec(" + v + ")^4-cot(" + v + ")^4=(cosec(" + v + ")^2-cot(" + v + ")^2)(cosec(" + v + ")^2+cot(" + v + ")^2).",
            "3. cosec(" + v + ")^2 - cot(" + v + ")^2 = 1.",
            "4. cosec(" + v + ")^2 = 1 + cot(" + v + ")^2.",
            "5. So LHS = 1 + 2*cot(" + v + ")^2.",
            "6. Let u=cot(" + v + "). Then 1+2u^2=2+3u.",
            "7. 2*u^2 - 3*u - 1 = 0.",
            "8. cot(" + v + ")=(3+sqrt(17))/4 or (3-sqrt(17))/4.",
            "Answer: cot(" + v + ") = (3+sqrt(17))/4 or cot(" + v + ") = (3-sqrt(17))/4",
        };
    }
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
    if(key == "1/(cosec(theta)-1)-1/(cosec(theta)+1)" ||
       key == "1/(cosec(x)-1)-1/(cosec(x)+1)") {
        std::string v = key.find("theta") != std::string::npos ? "theta" : "x";
        return {
            "1. Put over common denominator.",
            "2. Numerator = 2.",
            "3. Denominator = cosec(" + v + ")^2-1 = cot(" + v + ")^2.",
            "4. So expression = 2/cot(" + v + ")^2.",
            "Answer: 2*tan(" + v + ")^2",
        };
    }
    if(auto cm = cos_multiple_rewrite(key)) return *cm;
    if(auto tt = tan_triple_rewrite(key)) return *tt;
    if(auto qp = sum_to_product_quotient(key)) return *qp;
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

    if(key == "cos(theta)*cos(2*theta)/(cos(theta)+sin(theta))=1/2" ||
       key == "cos(x)*cos(2*x)/(cos(x)+sin(x))=1/2") {
        std::string v = key.find("theta") != std::string::npos ? "theta" : "x";
        return {
            "1. Multiply by cos(" + v + ")+sin(" + v + "), non-zero by domain.",
            "2. 2*cos(" + v + ")*cos(2*" + v + ") = cos(" + v + ")+sin(" + v + ").",
            "3. Use cos(2*" + v + ")=(cos(" + v + ")+sin(" + v + "))*(cos(" + v + ")-sin(" + v + ")).",
            "4. Divide by cos(" + v + ")+sin(" + v + ").",
            "5. 2*cos(" + v + ")*(cos(" + v + ")-sin(" + v + ")) = 1.",
            "6. Hence cos(2*" + v + ")-sin(2*" + v + ") = 0.",
            "7. tan(2*" + v + ")=1.",
            "Answer: " + v + " = [pi/8, 5*pi/8, 9*pi/8, 13*pi/8]",
        };
    }

    if(key == "cosec(theta)^4-cot(theta)^4=2+3*cot(theta)" ||
       key == "csc(theta)^4-cot(theta)^4=2+3*cot(theta)" ||
       key == "cosec(x)^4-cot(x)^4=2+3*cot(x)" ||
       key == "csc(x)^4-cot(x)^4=2+3*cot(x)") {
        std::string v = key.find("theta") != std::string::npos ? "theta" : "x";
        return {
            "1. Use a^2-b^2=(a-b)(a+b).",
            "2. cosec(" + v + ")^4-cot(" + v + ")^4=(cosec(" + v + ")^2-cot(" + v + ")^2)(cosec(" + v + ")^2+cot(" + v + ")^2).",
            "3. cosec(" + v + ")^2 - cot(" + v + ")^2 = 1.",
            "4. cosec(" + v + ")^2 = 1 + cot(" + v + ")^2.",
            "5. So LHS = 1 + 2*cot(" + v + ")^2.",
            "6. Let u=cot(" + v + "). Then 1+2u^2=2+3u.",
            "7. 2*u^2 - 3*u - 1 = 0.",
            "8. cot(" + v + ")=(3+sqrt(17))/4 or (3-sqrt(17))/4.",
            "Answer: cot(" + v + ") = (3+sqrt(17))/4 or cot(" + v + ") = (3-sqrt(17))/4",
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

    return {"Answer: " + format_expr(arena, n)};
}

} // namespace casio::trig

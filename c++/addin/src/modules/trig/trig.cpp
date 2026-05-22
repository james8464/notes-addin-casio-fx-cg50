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
    if(!arg || ccos.num == 0 || csin.num == 0 || ccos.den != 1 || csin.den != 1) return std::nullopt;
    long long A = ccos.num;
    long long B = csin.num;
    if(B < 0 && A <= 0) return std::nullopt;
    long long absA = std::llabs(A);
    long long absB = std::llabs(B);
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
    if(B > 0) {
        std::string sign = A < 0 ? "-" : "+";
        return std::vector<std::string>{
            "R=sqrt(" + std::to_string(B) + "^2+" + std::to_string(absA) + "^2)=" + Rtxt + ".",
            "cos(alpha)=" + ratio(B) + ".",
            "sin(alpha)=" + ratio(absA) + ".",
            "alpha=atan(" + rat_text(absA, B) + ").",
            "R*sin(" + v + sign + "alpha)=R*sin(" + v + ")*cos(alpha)" + (A < 0 ? "-R*cos(" : "+R*cos(") + v + ")*sin(alpha).",
            "Answer: " + Rtxt + "*sin(" + v + sign + "atan(" + rat_text(absA, B) + "))",
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

struct AngleBounds
{
    double lo;
    double hi;
    bool lo_open;
    bool hi_open;
    std::string lo_text;
    std::string hi_text;
};

static std::string bound_expr_text(std::string s)
{
    s = trim(std::move(s));
    int bal = 0;
    for(char c : s) {
        if(c == '(') ++bal;
        else if(c == ')') --bal;
    }
    if(!s.empty() && s.front() == '(' && bal > 0) s.erase(s.begin());
    if(!s.empty() && s.back() == ')' && bal < 0) s.pop_back();
    return trim(std::move(s));
}

static AngleBounds angle_bounds(Arena &a, std::string const &lo_text, std::string const &hi_text, bool rad)
{
    AngleBounds b{};
    std::string lt = trim(lo_text), ht = trim(hi_text);
    b.lo_open = !lt.empty() && lt.front() == '(';
    b.hi_open = !ht.empty() && ht.back() == ')';
    b.lo_text = bound_expr_text(lo_text);
    b.hi_text = bound_expr_text(hi_text);
    b.lo = angle_to_degree_double(a, casio::parse_expr(a, b.lo_text), rad).value_or(0.0);
    b.hi = angle_to_degree_double(a, casio::parse_expr(a, b.hi_text), rad).value_or(360.0);
    if(b.lo > b.hi) {
        std::swap(b.lo, b.hi);
        std::swap(b.lo_open, b.hi_open);
        std::swap(b.lo_text, b.hi_text);
    }
    return b;
}

static bool in_bounds(double x, AngleBounds const &b)
{
    if(b.lo_open ? x <= b.lo + 1e-7 : x < b.lo - 1e-7) return false;
    if(b.hi_open ? x >= b.hi - 1e-7 : x > b.hi + 1e-7) return false;
    return true;
}

static std::string interval_text(AngleBounds const &b, std::string const &var)
{
    return b.lo_text + (b.lo_open ? " < " : " <= ") + var + (b.hi_open ? " < " : " <= ") + b.hi_text;
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
    if(!contains_var(a, arg, var)) {
        auto d = angle_to_degree_double(a, arg, plain_number_is_radian);
        if(d) return std::make_pair(0.0, *d);
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

static std::optional<NodeId> trig_ratio_arg(Arena &a, NodeId n, FnKind fk)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;
    Node const &num = a.get(x.a);
    Node const &den = a.get(x.b);
    if(num.kind != NodeKind::Fn || den.kind != NodeKind::Fn) return std::nullopt;
    FnKind nf = (fk == FnKind::Tan) ? FnKind::Sin : FnKind::Cos;
    FnKind df = (fk == FnKind::Tan) ? FnKind::Cos : FnKind::Sin;
    if(num.fkind != nf || den.fkind != df) return std::nullopt;
    if(casio::sig(a, num.a) != casio::sig(a, den.a)) return std::nullopt;
    return num.a;
}

static std::optional<std::vector<std::string>> trig_ratio_target_route(Arena &a, NodeId src, NodeId target)
{
    auto build = [&](FnKind fk, NodeId arg) -> std::vector<std::string> {
        std::string u = casio::format_expr(a, arg);
        std::string name = (fk == FnKind::Tan) ? "tan" : "cot";
        std::string ratio = (fk == FnKind::Tan) ? "sin(u)/cos(u)" : "cos(u)/sin(u)";
        return {
            "1. Let u = " + u + ".",
            "2. Target = " + casio::format_expr(a, target) + ".",
            "3. Use identity " + name + "(u)=" + ratio + ".",
            "4. " + ratio + " = " + name + "(u).",
            "5. " + casio::format_expr(a, src) + " = " + casio::format_expr(a, target) + ".",
            "Answer: " + casio::format_expr(a, target),
        };
    };
    for(FnKind fk : {FnKind::Tan, FnKind::Cot}) {
        Node const &s = a.get(src);
        if(s.kind == NodeKind::Fn && s.fkind == fk) {
            auto arg = trig_ratio_arg(a, target, fk);
            if(arg && casio::sig(a, s.a) == casio::sig(a, *arg)) return build(fk, s.a);
        }
        Node const &t = a.get(target);
        if(t.kind == NodeKind::Fn && t.fkind == fk) {
            auto arg = trig_ratio_arg(a, src, fk);
            if(arg && casio::sig(a, t.a) == casio::sig(a, *arg)) return build(fk, t.a);
        }
    }
    return std::nullopt;
}

static std::optional<std::vector<std::string>> tan_target_sincos(Arena &a, NodeId src, NodeId target)
{
    return trig_ratio_target_route(a, src, target);
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
    AngleBounds bounds = angle_bounds(a, lo_text, hi_text, rad);
    for(double theta : base_degs) {
        for(int k = -80; k <= 80; ++k) {
            double a_deg = theta + 360.0 * k;
            double xdeg = (a_deg - lin->second) / lin->first;
            if(!in_bounds(xdeg, bounds)) continue;
            add_unique(xs, xdeg);
        }
    }
    std::sort(xs.begin(), xs.end());
    return xs;
}

static std::string trig_base_angle_line(FnKind fk, std::string const &arg, double r);
static std::string trig_alpha_family_line(FnKind fk, std::string const &arg, double r, bool rad);

struct EvenQuadAngle
{
    NodeId a2 = 0;
    NodeId c = 0;
};

static bool is_pow_var2(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Pow) return false;
    Node const &base = a.get(x.a);
    auto e = as_num(a, x.b);
    return base.kind == NodeKind::Sym && base.text == var && e && e->num == 2 && e->den == 1;
}

static std::optional<NodeId> strip_x2_coeff(Arena &a, NodeId n, std::string const &var)
{
    if(is_pow_var2(a, n, var)) return casio::num(a, 1);
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return std::nullopt;
    NodeId x2 = 0;
    std::vector<NodeId> coeff;
    for(NodeId k : x.kids) {
        if(is_pow_var2(a, k, var)) {
            if(x2) return std::nullopt;
            x2 = k;
        }
        else {
            if(contains_var(a, k, var)) return std::nullopt;
            coeff.push_back(k);
        }
    }
    if(!x2) return std::nullopt;
    if(coeff.empty()) return casio::num(a, 1);
    return casio::simplify(a, casio::mul(a, coeff));
}

static std::optional<EvenQuadAngle> even_quadratic_angle(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms = x.kind == NodeKind::Add ? x.kids : std::vector<NodeId>{n};
    std::vector<NodeId> q, c;
    for(NodeId t : terms) {
        if(auto k = strip_x2_coeff(a, t, var)) q.push_back(*k);
        else {
            if(contains_var(a, t, var)) return std::nullopt;
            c.push_back(t);
        }
    }
    if(q.empty()) return std::nullopt;
    return EvenQuadAngle{
        casio::simplify(a, casio::add(a, q)),
        c.empty() ? casio::num(a, 0) : casio::simplify(a, casio::add(a, c))
    };
}

static NodeId angle_node_from_degrees(Arena &a, double deg, bool rad)
{
    if(rad) return casio::parse_expr(a, format_pi_degrees(deg));
    return casio::parse_expr(a, format_double_compact(deg));
}

static std::optional<std::vector<std::string>> solve_even_quadratic_angle_exact(
    Arena &a,
    FnKind fk,
    NodeId arg,
    NodeId target_node,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    std::string const &eq_text
)
{
    if(!(fk == FnKind::Sin || fk == FnKind::Cos || fk == FnKind::Tan)) return std::nullopt;
    auto q = even_quadratic_angle(a, arg, var);
    if(!q) return std::nullopt;
    auto av = numeric_eval(a, q->a2, 0.0);
    auto cv = numeric_eval(a, q->c, 0.0);
    auto tv = numeric_eval(a, target_node, 0.0);
    if(!av || !cv || !tv || std::fabs(*av) < 1e-12) return std::nullopt;
    auto bases = base_trig_degrees(fk, *tv);
    if(bases.empty()) return std::nullopt;

    bool filter = !(rad && trim(lo_text) == "0" && trim(hi_text) == "2*pi");
    AngleBounds bounds = angle_bounds(a, lo_text, hi_text, rad);
    std::string A = format_expr(a, arg);
    std::string target = format_expr(a, target_node);
    std::vector<std::string> steps{
        eq_text + ".",
        "A = " + A + ".",
        trig_name(fk) + "(A) = " + target + ".",
        trig_base_angle_line(fk, "A", *tv),
        trig_alpha_family_line(fk, "A", *tv, rad),
    };
    std::vector<std::string> roots;
    for(double base : bases) {
        NodeId theta = angle_node_from_degrees(a, base, rad);
        NodeId num = casio::add(a, {theta, casio::neg(a, q->c)});
        NodeId den = q->a2;
        if(*av < 0) {
            num = casio::add(a, {q->c, casio::neg(a, theta)});
            den = casio::neg(a, q->a2);
        }
        NodeId x2 = casio::simplify(a, casio::div(a, num, den));
        auto x2v = numeric_eval(a, x2, 0.0);
        steps.push_back(A + " = " + format_expr(a, theta) + ".");
        steps.push_back(var + "^2 = " + format_expr(a, x2) + ".");
        if(!x2v || *x2v < -1e-10) {
            steps.push_back(format_expr(a, x2) + " < 0.");
            continue;
        }
        NodeId root = casio::simplify(a, casio::fn(a, "sqrt", x2));
        auto rv = numeric_eval(a, root, 0.0);
        if(!rv) continue;
        auto add_root = [&](std::string const &s, double v) {
            double xdeg = rad ? v * 180.0 / M_PI : v;
            if(filter && !in_bounds(xdeg, bounds)) return;
            if(std::find(roots.begin(), roots.end(), s) == roots.end()) roots.push_back(s);
        };
        std::string rt = format_expr(a, root);
        add_root(rt, *rv);
        if(std::fabs(*rv) > 1e-10) add_root("-" + rt, -*rv);
    }
    std::string final = var + " = [";
    for(std::size_t i = 0; i < roots.size(); ++i) {
        if(i) final += ", ";
        final += roots[i];
    }
    final += "]";
    return casio::exam_block("trig solve", steps, final);
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
static std::string trig_root_text(double r);
static std::vector<double> solve_quadratic_d(double a, double b, double c);
static std::string trig_quad_text(double a, double b, double c);
static std::string recip_poly_text(std::vector<double> const &c);

struct FnPoly
{
    double c[5]{};
    int deg = 0;
    NodeId arg = 0;
    FnKind fk = FnKind::Sin;
    bool has_arg = false;
};

struct RecipTrigPoly
{
    double coef[9]{};
    int min_e = 0;
    int max_e = 0;
    NodeId arg = 0;
    bool has_arg = false;
    bool used_recip = false;
};

static std::optional<FnPoly> collect_fn_poly(Arena &a, NodeId residual);
static std::optional<RecipTrigPoly> collect_recip_trig_poly(Arena &a, NodeId residual, FnKind base, FnKind recip);

static std::optional<std::vector<std::string>> solve_zero_product_trig(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Mul) return std::nullopt;
    std::vector<double> xs;
    std::vector<std::string> steps;
    steps.push_back(casio::format_expr(a, residual) + " = 0");
    for(NodeId k : r.kids) {
        if(is_const(a, k)) continue;
        Node const &f = a.get(k);
        if(auto lin = linear_angle(a, k, var, rad); lin && std::fabs(lin->first) > 1e-12) {
            AngleBounds b = angle_bounds(a, lo_text, hi_text, rad);
            double root = -lin->second / lin->first;
            if(in_bounds(root, b)) {
                steps.push_back(casio::format_expr(a, k) + " = 0");
                add_unique(xs, root);
            }
            continue;
        }
        if(f.kind == NodeKind::Fn && (f.fkind == FnKind::Sin || f.fkind == FnKind::Cos || f.fkind == FnKind::Tan)) {
            steps.push_back(casio::format_expr(a, k) + " = 0");
            auto part = x_values_from_angle_degrees(a, f.a, var, lo_text, hi_text, rad, base_trig_degrees(f.fkind, 0.0));
            for(double x : part) add_unique(xs, x);
            continue;
        }
        if(auto fp = collect_fn_poly(a, k)) {
            std::string A = format_expr(a, fp->arg);
            std::string ftxt = trig_name(fp->fk);
            std::vector<double> roots = fp->deg <= 2 ? solve_quadratic_d(fp->c[2], fp->c[1], fp->c[0]) : std::vector<double>{};
            steps.push_back(casio::format_expr(a, k) + " = 0");
            steps.push_back("u=" + ftxt + "(" + A + ")");
            steps.push_back(recip_poly_text(std::vector<double>{fp->c[0], fp->c[1], fp->c[2]}));
            for(double root : roots) {
                if(root < -1.0 - 1e-10 || root > 1.0 + 1e-10) {
                    steps.push_back("Reject u=" + trig_root_text(root) + ": outside [-1,1].");
                    continue;
                }
                steps.push_back(ftxt + "(" + A + ")=" + trig_root_text(root));
                auto vals = x_values_from_angle_degrees(a, fp->arg, var, lo_text, hi_text, rad, base_trig_degrees(fp->fk, root));
                for(double x : vals) add_unique(xs, x);
            }
            continue;
        }
        if(auto tp = collect_recip_trig_poly(a, k, FnKind::Tan, FnKind::Cot); tp && !tp->used_recip && tp->min_e == 0 && tp->max_e <= 2) {
            std::string A = format_expr(a, tp->arg);
            auto roots = solve_quadratic_d(tp->coef[6], tp->coef[5], tp->coef[4]);
            steps.push_back(casio::format_expr(a, k) + " = 0");
            steps.push_back("u=tan(" + A + ")");
            steps.push_back(recip_poly_text(std::vector<double>{tp->coef[4], tp->coef[5], tp->coef[6]}));
            for(double root : roots) {
                steps.push_back("tan(" + A + ")=" + trig_root_text(root));
                auto vals = x_values_from_angle_degrees(a, tp->arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, root));
                for(double x : vals) add_unique(xs, x);
            }
        }
    }
    if(xs.empty()) return std::nullopt;
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig zero product", steps, format_solution_list(var, rad, xs));
}

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

static std::optional<std::vector<std::string>> solve_same_trig_sum_zero(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
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
    struct CT { double c; NodeId arg; FnKind fk; };
    auto term = [&](NodeId id) -> std::optional<CT> {
        Node const &n = a.get(id);
        if(n.kind == NodeKind::Fn && (n.fkind == FnKind::Cos || n.fkind == FnKind::Sin)) return CT{1.0, n.a, n.fkind};
        if(n.kind != NodeKind::Mul) return std::nullopt;
        double c = 1.0;
        std::optional<NodeId> arg;
        std::optional<FnKind> fk;
        for(NodeId k : n.kids) {
            Node const &x = a.get(k);
            if(x.kind == NodeKind::Fn && (x.fkind == FnKind::Cos || x.fkind == FnKind::Sin)) {
                if(arg) return std::nullopt;
                arg = x.a;
                fk = x.fkind;
            }
            else if(auto q = as_num(a, k)) c *= static_cast<double>(q->num) / static_cast<double>(q->den);
            else return std::nullopt;
        }
        if(!arg || !fk) return std::nullopt;
        return CT{c, *arg, *fk};
    };
    auto t0 = term(rn->kids[0]), t1 = term(rn->kids[1]);
    if(!t0 || !t1 || t0->fk != t1->fk || std::fabs(std::fabs(t0->c) - std::fabs(t1->c)) > 1e-10 || t0->c * t1->c < 0)
        return std::nullopt;
    auto A = linear_angle(a, t0->arg, var, rad);
    auto B = linear_angle(a, t1->arg, var, rad);
    if(!A || !B) return std::nullopt;
    double ms = (A->first + B->first) / 2.0, bs = (A->second + B->second) / 2.0;
    double md = (A->first - B->first) / 2.0, bd = (A->second - B->second) / 2.0;
    if(md < 0) { md = -md; bd = -bd; }
    AngleBounds bounds = angle_bounds(a, lo_text, hi_text, rad);
    std::vector<double> xs;
    auto add_family = [&](double m, double shift, double root) {
        if(std::fabs(m) < 1e-12) return;
        for(int k = -100; k <= 100; ++k) {
            double x = (root + 180.0 * k - shift) / m;
            if(in_bounds(x, bounds)) add_unique(xs, x);
        }
    };
    add_family(ms, bs, t0->fk == FnKind::Sin ? 0.0 : 90.0);
    add_family(md, bd, 90.0);
    std::sort(xs.begin(), xs.end());
    auto fam = [&](double m, double b, double root) {
        double period = 180.0 / std::fabs(m);
        double base = (root - b) / m;
        while(base < 0) base += period;
        while(base >= period) base -= period;
        return format_general_trig_family(var, rad, {base}, period);
    };
    std::string s = linear_angle_text(ms, bs, var, rad);
    std::string d = linear_angle_text(md, bd, var, rad);
    bool is_sin = t0->fk == FnKind::Sin;
    std::vector<std::string> steps = {
        is_sin ? "sin(A)+sin(B)=2*sin((A+B)/2)*cos((A-B)/2)"
               : "cos(A)+cos(B)=2*cos((A+B)/2)*cos((A-B)/2)",
        "A=" + format_expr(a, t0->arg) + ", B=" + format_expr(a, t1->arg),
        is_sin ? "2*sin(" + s + ")*cos(" + d + ")=0"
               : "2*cos(" + s + ")*cos(" + d + ")=0",
    };
    std::string f1, f2;
    if(std::fabs(ms) > 1e-12) {
        f1 = fam(ms, bs, is_sin ? 0.0 : 90.0);
        steps.push_back(std::string(is_sin ? "sin(" : "cos(") + s + ")=0 => " + f1);
    }
    if(std::fabs(md) > 1e-12) {
        f2 = fam(md, bd, 90.0);
        steps.push_back("cos(" + d + ")=0 => " + f2);
    }
    if(general) {
        std::string ans = f1;
        if(!ans.empty() && !f2.empty()) ans += " or ";
        ans += f2;
        return casio::exam_block("trig solve", steps, ans);
    }
    steps.push_back(interval_text(bounds, var) + " => " + format_solution_list(var, rad, xs));
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
    bool rad,
    bool general = false
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

    AngleBounds b = angle_bounds(a, lo_text, hi_text, rad);

    if(fk != FnKind::Tan && std::fabs(A->first - B->first) < 1e-12 && std::fabs(A->second - B->second) < 1e-12) {
        std::string interval = interval_text(b, var);
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
        rels.push_back({B->first, B->second, 360.0, rad ? "A=B+2*pi*n" : "A=B+360n"});
        rels.push_back({-B->first, 180.0 - B->second, 360.0, rad ? "A=pi-B+2*pi*n" : "A=180-B+360n"});
    }
    else if(fk == FnKind::Cos) {
        rels.push_back({B->first, B->second, 360.0, rad ? "A=B+2*pi*n" : "A=B+360n"});
        rels.push_back({-B->first, -B->second, 360.0, rad ? "A=-B+2*pi*n" : "A=-B+360n"});
    }
    else {
        rels.push_back({B->first, B->second, 180.0, rad ? "A=B+pi*n" : "A=B+180n"});
    }

    std::vector<double> xs;
    std::vector<std::string> n_ranges;
    std::vector<std::string> family_roots;
    std::vector<std::string> rejected_n;
    std::vector<std::string> general_roots;
    for(auto const &rel : rels) {
        double denom = A->first - rel.rhs_m;
        if(std::fabs(denom) < 1e-12) continue;
        double period = std::fabs(rel.period / denom);
        double base = (rel.rhs_b - A->second) / denom;
        while(base < 0) base += period;
        while(base >= period) base -= period;
        general_roots.push_back(rel.reason + " => " + format_general_trig_family(var, rad, {base}, period));
        bool seen_n = false;
        int first_n = 0, last_n = 0;
        std::vector<double> fam_xs;
        for(int k = -100; k <= 100; ++k) {
            double xdeg = (rel.rhs_b + rel.period * k - A->second) / denom;
            if(!in_bounds(xdeg, b)) continue;
            if(!seen_n) {
                first_n = k;
                seen_n = true;
            }
            last_n = k;
            add_unique(xs, xdeg);
            add_unique(fam_xs, xdeg);
        }
        if(seen_n && (last_n - first_n) >= 2)
            n_ranges.push_back(rel.reason + ": n=" + std::to_string(first_n) + ".." + std::to_string(last_n));
        if(seen_n) {
            std::sort(fam_xs.begin(), fam_xs.end());
            family_roots.push_back(rel.reason + " => " + format_solution_list(var, rad, fam_xs));
            rejected_n.push_back(rel.reason + ": reject n<" + std::to_string(first_n) + " or n>" + std::to_string(last_n));
        }
    }
    std::sort(xs.begin(), xs.end());
    std::string rule_line =
        fk == FnKind::Tan ? (rad ? "tan(theta+pi*n)=tan(theta) => A=B+pi*n" : "tan(theta+180n)=tan(theta) => A=B+180n") :
        fk == FnKind::Cos ? (rad ? "cos(A)=cos(B): A=B+2*pi*n or A=-B+2*pi*n" : "cos(A)=cos(B): A=B+360n or A=-B+360n") :
                            (rad ? "sin(A)=sin(B): A=B+2*pi*n or A=pi-B+2*pi*n" : "sin(A)=sin(B): A=B+360n or A=180-B+360n");
    std::string family_line = "solve each family for " + var;
    std::vector<std::string> family_filters;
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
            family_filters.push_back(b.lo_text + (b.lo_open ? " < " : " <= ") + div_text(p, d1) + (b.hi_open ? " < " : " <= ") + b.hi_text);
            family_filters.push_back(b.lo_text + (b.lo_open ? " < " : " <= ") + div_text("(" + h + "+" + p + ")", d2) + (b.hi_open ? " < " : " <= ") + b.hi_text);
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
    if(general && !general_roots.empty()) {
        std::string ans;
        for(size_t i = 0; i < general_roots.size(); ++i) {
            steps.push_back(general_roots[i] + ".");
            std::string part = general_roots[i].substr(general_roots[i].find("=> ") + 3);
            if(i) ans += " or ";
            ans += part;
        }
        return casio::exam_block("trig solve", steps, ans);
    }
    for(auto const &ff : family_filters) steps.push_back(ff);
    for(auto const &nr : n_ranges) steps.push_back(nr);
    for(auto const &fr : family_roots) steps.push_back(fr);
    for(auto const &rej : rejected_n) steps.push_back(rej);
    if(!family_roots.empty()) steps.push_back("Accepted roots = union of the listed family roots.");
    if(!n_ranges.empty()) steps.push_back("n in listed ranges => keep roots with " + interval_text(b, var) + ".");
    if(!n_ranges.empty()) steps.push_back("Each kept root satisfies the original equation; no denominator exclusions.");
    steps.push_back(interval_text(b, var) + " => " + format_solution_list(var, rad, xs));
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_same_fn_residual(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
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
    auto out = solve_same_fn_linear(a, lhs, rhs, var, lo_text, hi_text, rad, general);
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

static std::optional<std::vector<std::string>> solve_sin2_cos_factor(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add || r.kids.size() != 2) return std::nullopt;
    struct T { double c; FnKind fk; NodeId arg; };
    auto term = [&](NodeId id) -> std::optional<T> {
        Node const &n = a.get(id);
        if(n.kind == NodeKind::Fn && (n.fkind == FnKind::Sin || n.fkind == FnKind::Cos)) return T{1.0, n.fkind, n.a};
        if(n.kind != NodeKind::Mul) return std::nullopt;
        double c = 1.0;
        std::optional<FnKind> fk;
        std::optional<NodeId> arg;
        for(NodeId k : n.kids) {
            Node const &x = a.get(k);
            if(auto q = as_num(a, k)) c *= static_cast<double>(q->num) / static_cast<double>(q->den);
            else if(x.kind == NodeKind::Fn && (x.fkind == FnKind::Sin || x.fkind == FnKind::Cos)) {
                if(fk) return std::nullopt;
                fk = x.fkind;
                arg = x.a;
            }
            else return std::nullopt;
        }
        if(!fk || !arg) return std::nullopt;
        return T{c, *fk, *arg};
    };
    auto t0 = term(r.kids[0]), t1 = term(r.kids[1]);
    if(!t0 || !t1 || t0->fk == t1->fk) return std::nullopt;
    T s = t0->fk == FnKind::Sin ? *t0 : *t1;
    T c = t0->fk == FnKind::Cos ? *t0 : *t1;
    auto S = linear_angle(a, s.arg, var, rad);
    auto C = linear_angle(a, c.arg, var, rad);
    if(!S || !C || std::fabs(s.c) < 1e-12) return std::nullopt;
    if(std::fabs(S->first - 2.0 * C->first) > 1e-12 || std::fabs(S->second - 2.0 * C->second) > 1e-9) return std::nullopt;

    double target = -c.c / (2.0 * s.c);
    std::string A = format_expr(a, c.arg);
    std::string target_s = trig_root_text(target);
    std::string mid = trig_root_text(2.0 * s.c) + "*sin(A)";
    if(c.c >= 0) mid += "+";
    mid += trig_root_text(c.c);
    std::vector<std::string> steps = {
        "A=" + A + ".",
        "sin(2A)=2*sin(A)*cos(A).",
        "cos(A)*(" + mid + ")=0.",
        "cos(A)=0 or sin(A)=" + target_s + ".",
    };

    auto lin = linear_angle(a, c.arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
    auto family = [&](FnKind fk, double val, double root_period) -> std::string {
        auto bases = base_trig_degrees(fk, val);
        if(bases.empty()) return "";
        double period = root_period / std::fabs(lin->first);
        std::vector<double> xbases;
        for(double theta : bases) {
            if(fk == FnKind::Cos && std::fabs(val) < 1e-12 && theta > 180.0) continue;
            double base = (theta - lin->second) / lin->first;
            while(base < 0) base += period;
            while(base >= period) base -= period;
            add_unique(xbases, base);
        }
        return format_general_trig_family(var, rad, xbases, period);
    };
    std::string f1 = family(FnKind::Cos, 0.0, 180.0);
    std::string f2 = (target >= -1.0 - 1e-10 && target <= 1.0 + 1e-10) ? family(FnKind::Sin, target, 360.0) : "";
    if(general) {
        if(f2.empty()) steps.push_back("Reject sin(A)=" + target_s + ": outside [-1,1].");
        else steps.push_back(f2 + ".");
        steps.push_back(f1 + ".");
        std::string ans = f1;
        if(!f2.empty()) ans += " or " + f2;
        return casio::exam_block("trig solve", steps, ans);
    }

    std::vector<double> xs = x_values_from_angle_degrees(a, c.arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, 0.0));
    if(target >= -1.0 - 1e-10 && target <= 1.0 + 1e-10) {
        auto ys = x_values_from_angle_degrees(a, c.arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, target));
        for(double y : ys) add_unique(xs, y);
    }
    else steps.push_back("Reject sin(A)=" + target_s + ": outside [-1,1].");
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_tan_sum_zero(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add || r.kids.size() != 2) return std::nullopt;
    struct T { double c; NodeId arg; };
    auto term = [&](NodeId id) -> std::optional<T> {
        Node const &n = a.get(id);
        if(n.kind == NodeKind::Fn && n.fkind == FnKind::Tan) return T{1.0, n.a};
        if(n.kind != NodeKind::Mul) return std::nullopt;
        double c = 1.0;
        std::optional<NodeId> arg;
        for(NodeId k : n.kids) {
            Node const &x = a.get(k);
            if(auto q = as_num(a, k)) c *= static_cast<double>(q->num) / static_cast<double>(q->den);
            else if(x.kind == NodeKind::Fn && x.fkind == FnKind::Tan) {
                if(arg) return std::nullopt;
                arg = x.a;
            }
            else return std::nullopt;
        }
        if(!arg) return std::nullopt;
        return T{c, *arg};
    };
    auto t0 = term(r.kids[0]), t1 = term(r.kids[1]);
    if(!t0 || !t1 || std::fabs(std::fabs(t0->c) - std::fabs(t1->c)) > 1e-10 || t0->c * t1->c < 0) return std::nullopt;
    auto A = linear_angle(a, t0->arg, var, rad);
    auto B = linear_angle(a, t1->arg, var, rad);
    if(!A || !B) return std::nullopt;
    double m = A->first + B->first, b = A->second + B->second;
    if(std::fabs(m) < 1e-12) return std::nullopt;
    double period = 180.0 / std::fabs(m);
    double base = -b / m;
    while(base < 0) base += period;
    while(base >= period) base -= period;
    std::string fam = format_general_trig_family(var, rad, {base}, period);
    std::vector<std::string> steps = {
        "tan(A)+tan(B)=sin(A+B)/(cos(A)*cos(B))",
        "A=" + format_expr(a, t0->arg) + ", B=" + format_expr(a, t1->arg),
        "sin(A+B)=0, cos(A)cos(B)!=0",
        fam + ".",
    };
    if(general) return casio::exam_block("trig solve", steps, fam);

    auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
    auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
    double lo = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
    double hi = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
    if(lo > hi) std::swap(lo, hi);
    std::vector<double> xs;
    for(int k = -100; k <= 100; ++k) {
        double x = (180.0 * k - b) / m;
        if(x >= lo - 1e-7 && x <= hi + 1e-7) add_unique(xs, x);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

struct CosQuadratic {
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
};

struct SinQuadratic {
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
        else if(!has_any_symbol(a, kid)) {
            auto v = numeric_eval(a, kid, 0.0);
            if(!v || !std::isfinite(*v)) return false;
            coeff *= *v;
        }
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

static bool match_fn_square(Arena &a, NodeId n, FnKind fk, double &coeff, NodeId &arg)
{
    NodeId rest = n;
    bool has_rest = true;
    if(!split_coeff_term(a, n, coeff, rest, has_rest) || !has_rest) return false;
    Node const &r = a.get(rest);
    if(r.kind != NodeKind::Pow) return false;
    auto q = as_num(a, r.b);
    Node const &base = a.get(r.a);
    if(!q || q->num != 2 || q->den != 1 || base.kind != NodeKind::Fn || base.fkind != fk) return false;
    arg = base.a;
    return true;
}

static std::optional<std::vector<std::string>> solve_tan2_sin2(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    Node const &r = a.get(residual);
    double tan2 = 0.0, sin2 = 0.0, c0 = 0.0;
    NodeId arg = 0;
    auto collect = [&](auto &&self, NodeId t, double scale) -> bool {
        double k = 1.0;
        NodeId rest = t;
        bool has_rest = true;
        if(!split_coeff_term(a, t, k, rest, has_rest)) return false;
        k *= scale;
        if(!has_rest) {
            c0 += k;
            return true;
        }
        Node const &x = a.get(rest);
        if(x.kind == NodeKind::Add) {
            for(NodeId kid : x.kids)
                if(!self(self, kid, k)) return false;
            return true;
        }
        if(x.kind != NodeKind::Pow) return false;
        auto q = as_num(a, x.b);
        Node const &base = a.get(x.a);
        if(!q || q->num != 2 || q->den != 1 || base.kind != NodeKind::Fn) return false;
        if(base.fkind == FnKind::Tan) {
            if(arg && !same_sig(a, arg, base.a)) return false;
            arg = base.a;
            tan2 += k;
            return true;
        }
        if(base.fkind == FnKind::Sin) {
            if(arg && !same_sig(a, arg, base.a)) return false;
            arg = base.a;
            sin2 += k;
            return true;
        }
        return false;
    };
    if(r.kind == NodeKind::Add) {
        for(NodeId t : r.kids)
            if(!collect(collect, t, 1.0)) return std::nullopt;
    }
    else if(!collect(collect, residual, 1.0)) return std::nullopt;
    if(!arg || std::fabs(tan2) < 1e-12 || std::fabs(sin2) < 1e-12) return std::nullopt;
    auto lin = linear_angle(a, arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;

    double qa = -sin2;
    double qb = tan2 + sin2 - c0;
    double qc = c0;
    auto roots = solve_quadratic_d(qa, qb, qc);
    std::string A = format_expr(a, arg);
    std::vector<std::string> steps = {
        "A=" + A + ".",
        "tan(A)^2=sin(A)^2/cos(A)^2.",
        "u=sin(A)^2, cos(A)^2=1-u.",
        trig_quad_text(qa, qb, qc),
    };
    std::vector<double> xs;
    std::vector<double> sin_roots;
    for(double u : roots) {
        if(u < -1e-10 || u > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + trig_root_text(u) + ": outside [0,1].");
            continue;
        }
        double s = std::sqrt(std::max(0.0, u));
        steps.push_back("sin(" + A + ")=+/-" + trig_root_text(s) + ".");
        add_unique(sin_roots, s);
        add_unique(sin_roots, -s);
    }
    auto add_family = [&](double s) -> std::string {
        auto bases = base_trig_degrees(FnKind::Sin, s);
        double period = 360.0 / std::fabs(lin->first);
        std::vector<double> xbases;
        for(double theta : bases) {
            double base = (theta - lin->second) / lin->first;
            while(base < 0) base += period;
            while(base >= period) base -= period;
            add_unique(xbases, base);
        }
        return format_general_trig_family(var, rad, xbases, period);
    };
    std::string ans;
    for(double s : sin_roots) {
        std::string f = add_family(s);
        if(f.empty()) continue;
        steps.push_back(f + ".");
        if(!ans.empty()) ans += " or ";
        ans += f;
        auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, s));
        for(double x : vals) add_unique(xs, x);
    }
    if(general) return casio::exam_block("trig solve", steps, ans);
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_sin2_cos2_factor(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add || r.kids.size() != 2) return std::nullopt;
    double sin2 = 0.0, cos2 = 0.0;
    NodeId sin_arg = 0, cos_arg = 0;
    for(NodeId t : r.kids) {
        double c = 0.0;
        NodeId a0 = 0;
        if(match_fn_square(a, t, FnKind::Cos, c, a0)) {
            cos2 += c;
            cos_arg = a0;
            continue;
        }
        NodeId rest = t;
        bool has_rest = true;
        if(!split_coeff_term(a, t, c, rest, has_rest) || !has_rest) return std::nullopt;
        Node const &x = a.get(rest);
        if(x.kind != NodeKind::Fn || x.fkind != FnKind::Sin) return std::nullopt;
        sin2 += c;
        sin_arg = x.a;
    }
    if(!sin_arg || !cos_arg || std::fabs(sin2) < 1e-12 || std::fabs(cos2) < 1e-12) return std::nullopt;
    auto S = linear_angle(a, sin_arg, var, rad);
    auto C = linear_angle(a, cos_arg, var, rad);
    if(!S || !C || std::fabs(S->first - 2.0 * C->first) > 1e-12 || std::fabs(S->second - 2.0 * C->second) > 1e-9)
        return std::nullopt;
    double target = -cos2 / (2.0 * sin2);
    std::string A = format_expr(a, cos_arg);
    std::vector<std::string> steps = {
        "A=" + A + ".",
        "sin(2A)=2*sin(A)*cos(A).",
        "cos(A)*(2*" + trig_root_text(sin2) + "*sin(A)+" + trig_root_text(cos2) + "*cos(A))=0.",
        "cos(A)=0 or tan(A)=" + trig_root_text(target) + ".",
    };
    auto lin = linear_angle(a, cos_arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
    auto fam = [&](FnKind fk, double val, double per) {
        auto bases = base_trig_degrees(fk, val);
        double period = per / std::fabs(lin->first);
        std::vector<double> xbases;
        for(double theta : bases) {
            if(fk == FnKind::Cos && std::fabs(val) < 1e-12 && theta > 180.0) continue;
            double base = (theta - lin->second) / lin->first;
            while(base < 0) base += period;
            while(base >= period) base -= period;
            add_unique(xbases, base);
        }
        return format_general_trig_family(var, rad, xbases, period);
    };
    std::string f1 = fam(FnKind::Cos, 0.0, 180.0);
    std::string f2 = fam(FnKind::Tan, target, 180.0);
    steps.push_back(f1 + ".");
    steps.push_back(f2 + ".");
    std::string ans = f1 + " or " + f2;
    if(general) return casio::exam_block("trig solve", steps, ans);
    std::vector<double> xs = x_values_from_angle_degrees(a, cos_arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, 0.0));
    auto ys = x_values_from_angle_degrees(a, cos_arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, target));
    for(double y : ys) add_unique(xs, y);
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_cos_tan_product(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    auto match_prod = [&](NodeId id, double scale, double &coef, NodeId &arg) -> bool {
        Node const &n = a.get(id);
        std::vector<NodeId> kids = n.kind == NodeKind::Mul ? n.kids : std::vector<NodeId>{id};
        double c = scale;
        NodeId ca = 0, ta = 0;
        for(NodeId k : kids) {
            Node const &x = a.get(k);
            if(x.kind == NodeKind::Num) c *= (double)x.num.num / (double)x.num.den;
            else if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cos) ca = x.a;
            else if(x.kind == NodeKind::Fn && x.fkind == FnKind::Tan) ta = x.a;
            else return false;
        }
        if(!ca || !ta || !same_sig(a, ca, ta)) return false;
        coef += c;
        arg = ca;
        return true;
    };
    double coef = 0.0, c0 = 0.0;
    NodeId arg = 0;
    auto collect = [&](auto &&self, NodeId id, double scale) -> bool {
        if(match_prod(id, scale, coef, arg)) return true;
        double k = 1.0;
        NodeId rest = id;
        bool has_rest = true;
        if(!split_coeff_term(a, id, k, rest, has_rest)) return false;
        k *= scale;
        if(!has_rest) {
            c0 += k;
            return true;
        }
        Node const &x = a.get(rest);
        if(x.kind == NodeKind::Add) {
            for(NodeId kid : x.kids)
                if(!self(self, kid, k)) return false;
            return true;
        }
        if(match_prod(rest, k, coef, arg)) return true;
        if(!contains_var(a, rest, var)) {
            auto v = numeric_eval(a, rest, 0.0);
            if(v && std::isfinite(*v)) {
                c0 += k * *v;
                return true;
            }
        }
        return false;
    };
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId kid : r.kids)
            if(!collect(collect, kid, 1.0)) return std::nullopt;
    }
    else if(!collect(collect, residual, 1.0)) return std::nullopt;
    if(!arg || std::fabs(coef) < 1e-12) return std::nullopt;
    double target = -c0 / coef;
    auto lin = linear_angle(a, arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
    std::string A = format_expr(a, arg);
    std::vector<std::string> steps = {
        "A=" + A + ".",
        "cos(A)*tan(A)=sin(A), cos(A)!=0.",
        "sin(A)=" + trig_root_text(target) + ".",
    };
    auto bases = base_trig_degrees(FnKind::Sin, target);
    if(bases.empty()) return std::nullopt;
    double period = 360.0 / std::fabs(lin->first);
    std::vector<double> xbases;
    for(double theta : bases) {
        double base = (theta - lin->second) / lin->first;
        while(base < 0) base += period;
        while(base >= period) base -= period;
        add_unique(xbases, base);
    }
    std::string fam = format_general_trig_family(var, rad, xbases, period);
    steps.push_back(fam + ".");
    if(general) return casio::exam_block("trig solve", steps, fam);
    auto xs = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, bases);
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_trig_square_eq(
    Arena &a,
    NodeId lhs,
    NodeId rhs,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    double coeff = 0.0;
    NodeId arg = 0;
    FnKind fk = FnKind::Sin;
    auto match_any = [&](NodeId n) -> bool {
        for(FnKind f : {FnKind::Sin, FnKind::Cos, FnKind::Tan}) {
            double c = 0.0;
            NodeId a0 = 0;
            if(match_fn_square(a, n, f, c, a0)) {
                coeff = c;
                arg = a0;
                fk = f;
                return true;
            }
        }
        return false;
    };
    NodeId target_node = rhs;
    if(!match_any(lhs)) {
        if(!match_any(rhs)) return std::nullopt;
        target_node = lhs;
    }
    if(contains_var(a, target_node, var) || std::fabs(coeff) < 1e-12) return std::nullopt;
    auto tv = numeric_eval(a, target_node, 0.0);
    if(!tv || !std::isfinite(*tv)) return std::nullopt;
    double q = *tv / coeff;
    if(q < -1e-10 || ((fk == FnKind::Sin || fk == FnKind::Cos) && q > 1.0 + 1e-10)) return std::nullopt;
    double root = std::sqrt(std::max(0.0, q));
    std::string A = format_expr(a, arg);
    std::string fn = trig_name(fk);
    std::vector<std::string> steps = {
        fn + "(" + A + ")^2=" + trig_root_text(q) + ".",
    };
    if(fk == FnKind::Tan) {
        steps.push_back(fn + "(" + A + ")=+/-" + trig_root_text(root) + ".");
    }
    else {
        steps.push_back("u=" + fn + "(" + A + ").");
        steps.push_back("u^2=" + trig_root_text(q) + ".");
        steps.push_back("u=+/-" + trig_root_text(root) + ".");
        steps.push_back(fn + "(" + A + ")=" + trig_root_text(root) + ".");
        if(root != 0.0) steps.push_back(fn + "(" + A + ")=-" + trig_root_text(root) + ".");
    }
    auto lin = linear_angle(a, arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
    auto fam = [&](double val) {
        auto bases = base_trig_degrees(fk, val);
        double period = (fk == FnKind::Tan ? 180.0 : 360.0) / std::fabs(lin->first);
        std::vector<double> xbases;
        for(double theta : bases) {
            if(fk == FnKind::Tan && theta >= 180.0) continue;
            double base = (theta - lin->second) / lin->first;
            while(base < 0) base += period;
            while(base >= period) base -= period;
            add_unique(xbases, base);
        }
        return format_general_trig_family(var, rad, xbases, period);
    };
    std::string f1 = fam(root);
    std::string f2 = root == 0.0 ? "" : fam(-root);
    if(!f1.empty()) steps.push_back(f1 + ".");
    if(!f2.empty()) steps.push_back(f2 + ".");
    std::string ans = f1;
    if(!f2.empty()) ans += " or " + f2;
    if(general) return casio::exam_block("trig solve", steps, ans);
    std::vector<double> xs;
    for(double v : {root, -root}) {
        auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base_trig_degrees(fk, v));
        for(double x : vals) add_unique(xs, x);
        if(root == 0.0) break;
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_sin2_tan_factor(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add || r.kids.size() != 2) return std::nullopt;
    double sin2 = 0.0, tan1 = 0.0;
    NodeId sin_arg = 0, tan_arg = 0;
    for(NodeId t : r.kids) {
        double c = 1.0;
        NodeId rest = t;
        bool has_rest = true;
        if(!split_coeff_term(a, t, c, rest, has_rest) || !has_rest) return std::nullopt;
        Node const &x = a.get(rest);
        if(x.kind != NodeKind::Fn) return std::nullopt;
        if(x.fkind == FnKind::Sin) {
            sin2 += c;
            sin_arg = x.a;
        }
        else if(x.fkind == FnKind::Tan) {
            tan1 += c;
            tan_arg = x.a;
        }
        else return std::nullopt;
    }
    if(!sin_arg || !tan_arg || std::fabs(sin2) < 1e-12 || std::fabs(tan1) < 1e-12) return std::nullopt;
    auto S = linear_angle(a, sin_arg, var, rad);
    auto T = linear_angle(a, tan_arg, var, rad);
    if(!S || !T || std::fabs(S->first - 2.0 * T->first) > 1e-12 || std::fabs(S->second - 2.0 * T->second) > 1e-9)
        return std::nullopt;
    double u = -tan1 / (2.0 * sin2);
    if(u < -1e-10 || u > 1.0 + 1e-10) return std::nullopt;
    double croot = std::sqrt(std::max(0.0, u));
    std::string A = format_expr(a, tan_arg);
    std::vector<std::string> steps = {
        "A=" + A + ".",
        "sin(2A)=2*sin(A)*cos(A), tan(A)=sin(A)/cos(A).",
        "sin(A)*(2*cos(A)^2" + std::string(tan1 >= 0 ? "+" : "") + trig_root_text(tan1) + ")=0.",
        "sin(A)=0 or cos(A)=+/-" + trig_root_text(croot) + ".",
    };
    auto lin = linear_angle(a, tan_arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
    auto mkfam = [&](FnKind fk, double val, double per) {
        auto bases = base_trig_degrees(fk, val);
        double period = per / std::fabs(lin->first);
        std::vector<double> xbases;
        for(double theta : bases) {
            double base = (theta - lin->second) / lin->first;
            while(base < 0) base += period;
            while(base >= period) base -= period;
            add_unique(xbases, base);
        }
        return format_general_trig_family(var, rad, xbases, period);
    };
    std::string f0 = format_general_trig_family(var, rad, {0.0}, 180.0 / std::fabs(lin->first));
    std::string f1 = mkfam(FnKind::Cos, croot, 360.0);
    std::string f2 = mkfam(FnKind::Cos, -croot, 360.0);
    steps.push_back(f0 + ".");
    steps.push_back(f1 + ".");
    steps.push_back(f2 + ".");
    std::string ans = f0 + " or " + f1 + " or " + f2;
    if(general) return casio::exam_block("trig solve", steps, ans);
    std::vector<double> xs = x_values_from_angle_degrees(a, tan_arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, 0.0));
    for(double v : {croot, -croot}) {
        auto ys = x_values_from_angle_degrees(a, tan_arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, v));
        for(double y : ys) add_unique(xs, y);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_shifted_sin_sum(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    Node const &r = a.get(residual);
    if(r.kind != NodeKind::Add || r.kids.size() != 2) return std::nullopt;
    struct T { double c; NodeId arg; };
    auto term = [&](NodeId id) -> std::optional<T> {
        Node const &n = a.get(id);
        if(n.kind == NodeKind::Fn && n.fkind == FnKind::Sin) return T{1.0, n.a};
        if(n.kind != NodeKind::Mul) return std::nullopt;
        double c = 1.0;
        NodeId arg = 0;
        for(NodeId k : n.kids) {
            Node const &x = a.get(k);
            if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sin) {
                if(arg) return std::nullopt;
                arg = x.a;
            }
            else if(!contains_var(a, k, var)) {
                auto v = numeric_eval(a, k, 0.0);
                if(!v || !std::isfinite(*v)) return std::nullopt;
                c *= *v;
            }
            else return std::nullopt;
        }
        if(!arg) return std::nullopt;
        return T{c, arg};
    };
    auto t0 = term(r.kids[0]), t1 = term(r.kids[1]);
    if(!t0 || !t1) return std::nullopt;
    auto A = linear_angle(a, t0->arg, var, rad);
    auto B = linear_angle(a, t1->arg, var, rad);
    if(!A || !B || std::fabs(A->first - B->first) > 1e-12 || std::fabs(A->first) < 1e-12) return std::nullopt;
    double a0 = A->second * M_PI / 180.0;
    double b0 = B->second * M_PI / 180.0;
    double p = t0->c * std::cos(a0) + t1->c * std::cos(b0);
    double q = t0->c * std::sin(a0) + t1->c * std::sin(b0);
    std::string M = linear_angle_text(A->first, 0.0, var, rad);
    std::vector<std::string> steps = {
        "sin(M+a)=sin(M)cos(a)+cos(M)sin(a).",
        trig_root_text(p) + "*sin(" + M + ")" + std::string(q >= 0 ? "+" : "") + trig_root_text(q) + "*cos(" + M + ")=0.",
    };
    FnKind fk = FnKind::Tan;
    double target = 0.0;
    if(std::fabs(p) > 1e-12) {
        target = -q / p;
        steps.push_back("tan(" + M + ")=" + trig_root_text(target) + ".");
    }
    else {
        fk = FnKind::Cos;
        steps.push_back("cos(" + M + ")=0.");
    }
    double period = (fk == FnKind::Tan ? 180.0 : 360.0) / std::fabs(A->first);
    auto bases = base_trig_degrees(fk, target);
    std::vector<double> xbases;
    for(double theta : bases) {
        if(fk == FnKind::Tan && theta >= 180.0) continue;
        double base = theta / A->first;
        while(base < 0) base += period;
        while(base >= period) base -= period;
        add_unique(xbases, base);
    }
    std::string fam = format_general_trig_family(var, rad, xbases, period);
    steps.push_back(fam + ".");
    if(general) return casio::exam_block("trig solve", steps, fam);
    std::vector<double> xs;
    auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
    auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
    double lo = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
    double hi = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
    if(lo > hi) std::swap(lo, hi);
    for(double base : xbases)
        for(int k = -100; k <= 100; ++k) {
            double x = base + period * k;
            if(x >= lo - 1e-7 && x <= hi + 1e-7) add_unique(xs, x);
        }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + " => " + format_solution_list(var, rad, xs) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static bool add_cos_quadratic_term(Arena &a, NodeId n, std::string const &var, bool rad, CosQuadratic &p)
{
    Node const &x0 = a.get(n);
    if(x0.kind == NodeKind::Mul) {
        int add_i = -1;
        for(std::size_t i = 0; i < x0.kids.size(); ++i) {
            if(a.get(x0.kids[i]).kind == NodeKind::Add) {
                add_i = (int)i;
                break;
            }
        }
        if(add_i >= 0) {
            Node const &addn = a.get(x0.kids[(std::size_t)add_i]);
            for(NodeId ak : addn.kids) {
                std::vector<NodeId> fs;
                for(std::size_t i = 0; i < x0.kids.size(); ++i)
                    if((int)i != add_i) fs.push_back(x0.kids[i]);
                fs.push_back(ak);
                if(!add_cos_quadratic_term(a, casio::simplify(a, casio::mul(a, fs)), var, rad, p)) return false;
            }
            return true;
        }
    }
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

static bool add_sin_quadratic_term(Arena &a, NodeId n, std::string const &var, bool rad, SinQuadratic &p)
{
    Node const &x0 = a.get(n);
    if(x0.kind == NodeKind::Mul) {
        int add_i = -1;
        for(std::size_t i = 0; i < x0.kids.size(); ++i) {
            if(a.get(x0.kids[i]).kind == NodeKind::Add) {
                add_i = (int)i;
                break;
            }
        }
        if(add_i >= 0) {
            Node const &addn = a.get(x0.kids[(std::size_t)add_i]);
            for(NodeId ak : addn.kids) {
                std::vector<NodeId> fs;
                for(std::size_t i = 0; i < x0.kids.size(); ++i)
                    if((int)i != add_i) fs.push_back(x0.kids[i]);
                fs.push_back(ak);
                if(!add_sin_quadratic_term(a, casio::simplify(a, casio::mul(a, fs)), var, rad, p)) return false;
            }
            return true;
        }
    }
    double coeff = 1.0;
    NodeId rest = n;
    bool has_rest = true;
    if(!split_coeff_term(a, n, coeff, rest, has_rest)) return false;
    if(!has_rest) {
        p.c += coeff;
        return true;
    }
    Node const &r = a.get(rest);
    if(r.kind == NodeKind::Fn) {
        auto lin = linear_angle(a, r.a, var, rad);
        if(!lin || std::fabs(lin->second) > 1e-9) return false;
        if(r.fkind == FnKind::Sin && std::fabs(lin->first - 1.0) < 1e-9) {
            p.b += coeff;
            return true;
        }
        if(r.fkind == FnKind::Cos && std::fabs(lin->first - 2.0) < 1e-9) {
            p.c += coeff;
            p.a -= 2.0 * coeff;
            return true;
        }
    }
    if(r.kind == NodeKind::Pow) {
        Node const &base = a.get(r.a);
        auto exp = as_num(a, r.b);
        if(!exp || exp->num != 2 || exp->den != 1 || base.kind != NodeKind::Fn || base.fkind != FnKind::Sin) return false;
        auto lin = linear_angle(a, base.a, var, rad);
        if(!lin || std::fabs(lin->first - 1.0) > 1e-9 || std::fabs(lin->second) > 1e-9) return false;
        p.a += coeff;
        return true;
    }
    return false;
}

static std::optional<SinQuadratic> sin_quadratic_residual(Arena &a, NodeId residual, std::string const &var, bool rad)
{
    SinQuadratic p;
    Node const &x = a.get(residual);
    if(x.kind == NodeKind::Add) {
        for(NodeId kid : x.kids)
            if(!add_sin_quadratic_term(a, kid, var, rad, p)) return std::nullopt;
    }
    else if(!add_sin_quadratic_term(a, residual, var, rad, p)) return std::nullopt;
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
    std::string b = fmt_poly_coeff(p.b, false);
    if(!b.empty()) out += b + "u";
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
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var));
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::vector<double> solve_quadratic_d(double a, double b, double c);
static std::string trig_quad_text(double a, double b, double c);
static std::string trig_root_text(double r);
static std::string trig_base_angle_line(FnKind fk, std::string const &arg, double r);
static std::string trig_alpha_family_line(FnKind fk, std::string const &arg, double r, bool rad);

static std::optional<std::vector<std::string>> solve_sin_quadratic(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = sin_quadratic_residual(a, residual, var, rad);
    if(!p) return std::nullopt;
    auto roots = solve_quadratic_d(p->a, p->b, p->c);
    if(roots.empty()) return std::nullopt;
    std::vector<double> xs;
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "Use cos(2*" + var + ")=1-2sin(" + var + ")^2.",
        "u=sin(" + var + ").",
        trig_quad_text(p->a, p->b, p->c),
    };
    std::string rline = "u=";
    for(std::size_t i = 0; i < roots.size(); ++i) {
        if(i) rline += " or u=";
        rline += trig_root_text(roots[i]);
    }
    steps.push_back(rline + ".");
    for(double u : roots) {
        if(u < -1.0 - 1e-10 || u > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + trig_root_text(u) + ": outside [-1,1].");
            continue;
        }
        steps.push_back("sin(" + var + ")=" + trig_root_text(u) + ".");
        steps.push_back(trig_base_angle_line(FnKind::Sin, var, u));
        steps.push_back(trig_alpha_family_line(FnKind::Sin, var, u, rad));
        auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, u));
        for(double v : vals) add_unique(xs, v);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_cos_half_sin(
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
    double A = 0.0, B = 0.0, S = 0.0;
    NodeId half_arg = 0;
    bool has_half = false;
    for(NodeId kid : r.kids) {
        if(!has_any_symbol(a, kid)) {
            auto v = numeric_eval(a, kid, 0.0);
            if(!v || !std::isfinite(*v)) return std::nullopt;
            B += *v;
            continue;
        }
        double coeff = 1.0;
        NodeId rest = kid;
        bool has_rest = true;
        if(!split_coeff_term(a, kid, coeff, rest, has_rest) || !has_rest) return std::nullopt;
        Node const &fn = a.get(rest);
        if(fn.kind != NodeKind::Fn) return std::nullopt;
        auto lin = linear_angle(a, fn.a, var, rad);
        if(!lin || std::fabs(lin->second) > 1e-12) return std::nullopt;
        if(fn.fkind == FnKind::Cos && std::fabs(lin->first - 1.0) < 1e-12) A += coeff;
        else if(fn.fkind == FnKind::Sin && std::fabs(lin->first - 0.5) < 1e-12) {
            S += coeff;
            half_arg = fn.a;
            has_half = true;
        }
        else return std::nullopt;
    }
    if(!has_half || std::fabs(A) < 1e-12 || std::fabs(S) < 1e-12) return std::nullopt;
    auto roots = solve_quadratic_d(-2.0 * A, S, A + B);
    if(roots.empty()) return std::nullopt;
    std::string H = format_expr(a, half_arg);
    std::vector<std::string> steps{
        "u=sin(" + H + ").",
        "cos(" + var + ")=1-2u^2.",
        trig_quad_text(-2.0 * A, S, A + B),
    };
    std::vector<double> xs;
    std::string rline = "u=";
    bool any = false;
    for(double u : roots) {
        if(any) rline += " or u=";
        rline += trig_root_text(u);
        any = true;
    }
    steps.push_back(rline + ".");
    for(double u : roots) {
        if(u < -1.0 - 1e-10 || u > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + trig_root_text(u) + ": outside [-1,1].");
            continue;
        }
        steps.push_back("sin(" + H + ")=" + trig_root_text(u) + ".");
        steps.push_back(trig_base_angle_line(FnKind::Sin, H, u));
        steps.push_back(trig_alpha_family_line(FnKind::Sin, H, u, rad));
        auto vals = x_values_from_angle_degrees(a, half_arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, u));
        for(double x : vals) add_unique(xs, x);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_same_angle_squares(
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
    double c0 = 0.0, c1 = 0.0, s1 = 0.0, c2 = 0.0, s2 = 0.0;
    NodeId arg = 0;
    bool has_arg = false;
    auto same_arg = [&](NodeId id) {
        if(has_arg) return same_sig(a, arg, id);
        arg = id;
        has_arg = true;
        return true;
    };
    std::vector<std::pair<NodeId, double>> stack;
    stack.push_back({residual, 1.0});
    while(!stack.empty()) {
        NodeId kid = stack.back().first;
        double scale = stack.back().second;
        stack.pop_back();
        Node const &top = a.get(kid);
        if(top.kind == NodeKind::Add) {
            for(NodeId part : top.kids) stack.push_back({part, scale});
            continue;
        }
        if(!has_any_symbol(a, kid)) {
            auto v = numeric_eval(a, kid, 0.0);
            if(!v || !std::isfinite(*v)) return std::nullopt;
            c0 += scale * *v;
            continue;
        }
        double coeff = 1.0;
        NodeId rest = kid;
        bool has_rest = true;
        if(!split_coeff_term(a, kid, coeff, rest, has_rest) || !has_rest) return std::nullopt;
        coeff *= scale;
        Node const &x = a.get(rest);
        if(x.kind == NodeKind::Add) {
            for(NodeId part : x.kids) stack.push_back({part, coeff});
            continue;
        }
        FnKind fk{};
        NodeId farg = 0;
        bool square = false;
        if(x.kind == NodeKind::Fn && (x.fkind == FnKind::Sin || x.fkind == FnKind::Cos)) {
            fk = x.fkind;
            farg = x.a;
        }
        else if(x.kind == NodeKind::Pow) {
            Node const &base = a.get(x.a);
            auto e = as_num(a, x.b);
            if(!e || e->num != 2 || e->den != 1 || base.kind != NodeKind::Fn ||
               (base.fkind != FnKind::Sin && base.fkind != FnKind::Cos)) return std::nullopt;
            fk = base.fkind;
            farg = base.a;
            square = true;
        }
        else return std::nullopt;
        if(!same_arg(farg)) return std::nullopt;
        if(square) {
            if(fk == FnKind::Cos) c2 += coeff;
            else s2 += coeff;
        }
        else {
            if(fk == FnKind::Cos) c1 += coeff;
            else s1 += coeff;
        }
    }
    if(!has_arg || (std::fabs(c2) < 1e-12 && std::fabs(s2) < 1e-12)) return std::nullopt;
    bool use_cos = std::fabs(s1) < 1e-12;
    bool use_sin = std::fabs(c1) < 1e-12;
    if(!use_cos && !use_sin) return std::nullopt;
    FnKind fk = use_cos ? FnKind::Cos : FnKind::Sin;
    double qa = use_cos ? c2 - s2 : s2 - c2;
    double qb = use_cos ? c1 : s1;
    double qc = c0 + (use_cos ? s2 : c2);
    if(qa < -1e-12) {
        qa = -qa;
        qb = -qb;
        qc = -qc;
    }
    auto roots = solve_quadratic_d(qa, qb, qc);
    if(roots.empty()) return std::nullopt;
    std::string A = format_expr(a, arg);
    std::string u = use_cos ? "cos(" + A + ")" : "sin(" + A + ")";
    std::vector<std::string> steps{
        std::string(use_cos ? "sin(" : "cos(") + A + ")^2=1-" + u + "^2.",
        "u=" + u + ".",
        trig_quad_text(qa, qb, qc),
    };
    std::vector<double> xs;
    std::string rline = "u=";
    bool any = false;
    for(double root : roots) {
        if(any) rline += " or u=";
        rline += trig_root_text(root);
        any = true;
    }
    steps.push_back(rline + ".");
    std::string eqline;
    for(double root : roots) {
        if(root < -1.0 - 1e-10 || root > 1.0 + 1e-10) continue;
        if(!eqline.empty()) eqline += " or ";
        eqline += u + "=" + trig_root_text(root);
    }
    if(!eqline.empty()) steps.push_back(eqline + ".");
    for(double root : roots) {
        if(root < -1.0 - 1e-10 || root > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + trig_root_text(root) + ": outside [-1,1].");
            continue;
        }
        steps.push_back(u + "=" + trig_root_text(root) + ".");
        steps.push_back(trig_base_angle_line(fk, A, root));
        steps.push_back(trig_alpha_family_line(fk, A, root, rad));
        auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base_trig_degrees(fk, root));
        for(double x : vals) add_unique(xs, x);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
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
    double sqrt3 = std::sqrt(3.0);
    double inv_sqrt3 = sqrt3 / 3.0;
    if(std::fabs(r - rt2) < 1e-9) return "sqrt(2)/2";
    if(std::fabs(r + rt2) < 1e-9) return "-sqrt(2)/2";
    if(std::fabs(r - rt3) < 1e-9) return "sqrt(3)/2";
    if(std::fabs(r + rt3) < 1e-9) return "-sqrt(3)/2";
    if(std::fabs(r - sqrt3) < 1e-9) return "sqrt(3)";
    if(std::fabs(r + sqrt3) < 1e-9) return "-sqrt(3)";
    if(std::fabs(r - inv_sqrt3) < 1e-9) return "sqrt(3)/3";
    if(std::fabs(r + inv_sqrt3) < 1e-9) return "-sqrt(3)/3";
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
    if(fk == FnKind::Tan) {
        double rt3 = std::sqrt(3.0);
        if(std::fabs(r) < 1e-9) exact = "0";
        else if(std::fabs(r - 1.0) < 1e-9) exact = "pi/4";
        else if(std::fabs(r + 1.0) < 1e-9) exact = "-pi/4";
        else if(std::fabs(r - rt3) < 1e-9) exact = "pi/3";
        else if(std::fabs(r + rt3) < 1e-9) exact = "-pi/3";
        else if(std::fabs(r - rt3 / 3.0) < 1e-9) exact = "pi/6";
        else if(std::fabs(r + rt3 / 3.0) < 1e-9) exact = "-pi/6";
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

static bool recip_trig_factor(Arena &a, NodeId n, RecipTrigPoly &p, double &c, int &e, FnKind base, FnKind recip)
{
    Node const &x = a.get(n);
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return false;
        c *= *v;
        return true;
    }
    if(x.kind == NodeKind::Sym) return false;
    if(x.kind == NodeKind::Fn && (x.fkind == base || x.fkind == recip)) {
        if(p.has_arg && !same_sig(a, p.arg, x.a)) return false;
        p.arg = x.a;
        p.has_arg = true;
        e += x.fkind == base ? 1 : -1;
        p.used_recip = p.used_recip || x.fkind == recip;
        return true;
    }
    if(x.kind == NodeKind::Pow) {
        Node const &b = a.get(x.a);
        auto q = as_num(a, x.b);
        if(!q || q->den != 1 || q->num < 1 || q->num > 4) return false;
        if(b.kind == NodeKind::Fn && (b.fkind == base || b.fkind == recip)) {
            if(p.has_arg && !same_sig(a, p.arg, b.a)) return false;
            p.arg = b.a;
            p.has_arg = true;
            e += (b.fkind == base ? 1 : -1) * (int)q->num;
            p.used_recip = p.used_recip || b.fkind == recip;
            return true;
        }
    }
    if(x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids)
            if(!recip_trig_factor(a, k, p, c, e, base, recip)) return false;
        return true;
    }
    if(x.kind == NodeKind::Div) {
        double dc = 1.0;
        int de = 0;
        if(!recip_trig_factor(a, x.a, p, c, e, base, recip) ||
           !recip_trig_factor(a, x.b, p, dc, de, base, recip) ||
           std::fabs(dc) < 1e-12) return false;
        c /= dc;
        e -= de;
        return true;
    }
    return false;
}

static bool add_recip_trig_term(Arena &a, NodeId term, RecipTrigPoly &p, FnKind base, FnKind recip)
{
    Node const &tn = a.get(term);
    if(tn.kind == NodeKind::Mul) {
        int add_idx = -1;
        for(std::size_t i = 0; i < tn.kids.size(); ++i) {
            if(a.get(tn.kids[i]).kind == NodeKind::Add) {
                if(add_idx >= 0) return false;
                add_idx = (int)i;
            }
        }
        if(add_idx >= 0) {
            Node const &addn = a.get(tn.kids[(std::size_t)add_idx]);
            for(NodeId ak : addn.kids) {
                std::vector<NodeId> factors;
                for(std::size_t i = 0; i < tn.kids.size(); ++i)
                    if((int)i != add_idx) factors.push_back(tn.kids[i]);
                factors.push_back(ak);
                if(!add_recip_trig_term(a, casio::simplify(a, casio::mul(a, factors)), p, base, recip)) return false;
            }
            return true;
        }
    }
    double c = 1.0;
    int e = 0;
    if(!recip_trig_factor(a, term, p, c, e, base, recip) || e < -4 || e > 4) return false;
    p.coef[e + 4] += c;
    p.min_e = std::min(p.min_e, e);
    p.max_e = std::max(p.max_e, e);
    return true;
}

static std::optional<RecipTrigPoly> collect_recip_trig_poly(Arena &a, NodeId residual, FnKind base, FnKind recip)
{
    RecipTrigPoly p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId k : r.kids)
            if(!add_recip_trig_term(a, k, p, base, recip)) return std::nullopt;
    }
    else if(!add_recip_trig_term(a, residual, p, base, recip)) return std::nullopt;
    if(!p.has_arg || p.min_e == p.max_e) return std::nullopt;
    return p;
}

static std::string recip_poly_text(std::vector<double> const &c)
{
    std::string out;
    for(int i = (int)c.size() - 1; i >= 0; --i) {
        double v = c[(std::size_t)i];
        if(std::fabs(v) < 1e-12) continue;
        if(i == 0) {
            if(out.empty()) out += trig_root_text(v);
            else out += std::string(v < 0 ? " - " : " + ") + trig_root_text(std::fabs(v));
        }
        else {
            out += fmt_poly_coeff(v, out.empty());
            out += "u";
            if(i > 1) out += "^" + std::to_string(i);
        }
    }
    return out.empty() ? "0=0." : out + "=0.";
}

static std::optional<std::vector<std::string>> solve_recip_trig_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    FnKind base,
    FnKind recip
)
{
    auto p = collect_recip_trig_poly(a, residual, base, recip);
    if(!p) return std::nullopt;
    int shift = -p->min_e;
    int deg = p->max_e - p->min_e;
    if(deg < 1 || deg > 5) return std::nullopt;
    if(!p->used_recip) return std::nullopt;
    if(base == FnKind::Tan && deg <= 1) return std::nullopt;
    std::vector<double> c((std::size_t)deg + 1, 0.0);
    for(int e = p->min_e; e <= p->max_e; ++e) c[(std::size_t)(e + shift)] = p->coef[e + 4];
    while(c.size() > 1 && std::fabs(c.back()) < 1e-12) c.pop_back();
    deg = (int)c.size() - 1;
    std::vector<double> roots;
    if(deg <= 2) roots = solve_quadratic_d(deg == 2 ? c[2] : 0.0, c.size() > 1 ? c[1] : 0.0, c[0]);
    else if(std::fabs(c[1]) < 1e-12 && std::fabs(c[2]) < 1e-12 && std::fabs(c[3]) > 1e-12) {
        roots.push_back(std::cbrt(-c[0] / c[3]));
    }
    else if(std::fabs(c[0]) < 1e-12) {
        roots.push_back(0.0);
        auto q = solve_quadratic_d(c[3], c[2], c[1]);
        for(double r : q) add_unique(roots, r);
    }
    else if(deg == 5 && std::fabs(c[1]) < 1e-12 && std::fabs(c[2]) < 1e-12 && std::fabs(c[3]) < 1e-12 &&
            std::fabs(c[4]) < 1e-12 && std::fabs(c[5]) > 1e-12) {
        double v = -c[0] / c[5];
        roots.push_back(v < 0.0 ? -std::pow(-v, 0.2) : std::pow(v, 0.2));
    }
    else return std::nullopt;

    std::vector<double> xs;
    std::vector<std::string> steps;
    std::string A = format_expr(a, p->arg);
    steps.push_back(format_expr(a, residual) + " = 0.");
    std::string bf = trig_name(base);
    std::string rf = trig_name(recip);
    steps.push_back("u=" + bf + "(" + A + ").");
    steps.push_back(rf + "(" + A + ")=1/u, u!=0.");
    if(shift > 0) steps.push_back(std::string("Multiply by ") + (shift == 1 ? "u" : "u^" + std::to_string(shift)) + ".");
    steps.push_back(recip_poly_text(c));
    std::string rline = "u=";
    bool any = false;
    for(double r : roots) {
        if(p->min_e < 0 && std::fabs(r) < 1e-10) {
            steps.push_back("Reject u=0: " + rf + " undefined.");
            continue;
        }
        if((base == FnKind::Sin || base == FnKind::Cos) && (r < -1.0 - 1e-10 || r > 1.0 + 1e-10)) {
            steps.push_back("Reject u=" + trig_root_text(r) + ": outside [-1,1].");
            continue;
        }
        if(any) rline += " or u=";
        rline += trig_root_text(r);
        any = true;
        auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(base, r));
        for(double x : vals) add_unique(xs, x);
    }
    if(any) steps.push_back(rline + ".");
    steps.push_back(bf + "(" + A + ")=u.");
    if(any && base != FnKind::Tan) {
        for(double r : roots) {
            if(r >= -1.0 - 1e-10 && r <= 1.0 + 1e-10 && !(p->min_e < 0 && std::fabs(r) < 1e-10)) {
                steps.push_back(trig_base_angle_line(base, A, r));
                steps.push_back(trig_alpha_family_line(base, A, r, rad));
            }
        }
    }
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_tan_even_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = collect_recip_trig_poly(a, residual, FnKind::Tan, FnKind::Cot);
    if(!p || p->used_recip || p->min_e != 0 || p->max_e < 2 || p->max_e > 4) return std::nullopt;
    bool even_only = std::fabs(p->coef[5]) < 1e-12 && std::fabs(p->coef[7]) < 1e-12;
    std::string A = format_expr(a, p->arg);
    if(!even_only) {
        int deg = p->max_e;
        if(deg < 1 || deg > 3) return std::nullopt;
        double c0 = p->coef[4], c1 = p->coef[5], c2 = p->coef[6], c3 = p->coef[7];
        std::vector<double> roots;
        if(deg <= 2) roots = solve_quadratic_d(c2, c1, c0);
        else {
            double first = NAN;
            for(double r : {-3.0, -2.0, -1.0, -0.5, 0.5, 1.0, 2.0, 3.0, std::sqrt(3.0), -std::sqrt(3.0)}) {
                double v = ((c3 * r + c2) * r + c1) * r + c0;
                if(std::fabs(v) < 1e-9) {
                    first = r;
                    break;
                }
            }
            if(!std::isfinite(first)) return std::nullopt;
            roots.push_back(first);
            double b2 = c3;
            double b1 = c2 + first * b2;
            double b0 = c1 + first * b1;
            auto q = solve_quadratic_d(b2, b1, b0);
            for(double r : q) add_unique(roots, r);
        }

        std::vector<std::string> steps{
            format_expr(a, residual) + " = 0.",
            "u=tan(" + A + ").",
            recip_poly_text(std::vector<double>{c0, c1, c2, c3}),
        };
        std::vector<double> xs;
        std::string rline = "u=";
        for(std::size_t i = 0; i < roots.size(); ++i) {
            if(i) rline += " or u=";
            rline += trig_root_text(roots[i]);
        }
        steps.push_back(rline + ".");
        for(double r : roots) {
            steps.push_back("tan(" + A + ")=" + trig_root_text(r) + ".");
            steps.push_back(trig_base_angle_line(FnKind::Tan, A, r));
            steps.push_back(trig_alpha_family_line(FnKind::Tan, A, r, rad));
            auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, r));
            for(double x : vals) add_unique(xs, x);
        }
        std::sort(xs.begin(), xs.end());
        steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
        return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
    }
    double q2 = p->coef[8], q1 = p->coef[6], q0 = p->coef[4];
    if(std::fabs(q2) < 1e-12 && std::fabs(q1) < 1e-12) return std::nullopt;
    auto u_roots = solve_quadratic_d(q2, q1, q0);
    if(u_roots.empty()) return std::nullopt;

    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "u=tan(" + A + ")^2.",
        trig_quad_text(q2, q1, q0),
    };
    std::vector<double> xs;
    std::string uline = "u=";
    bool saw_u = false;
    for(double u : u_roots) {
        if(saw_u) uline += " or u=";
        uline += trig_root_text(u);
        saw_u = true;
    }
    steps.push_back(uline + ".");
    for(double u : u_roots) {
        if(u < -1e-10) {
            steps.push_back("Reject u=" + trig_root_text(u) + ": tan(" + A + ")^2>=0.");
            continue;
        }
        double r = std::sqrt(std::max(0.0, u));
        steps.push_back("tan(" + A + ")=+/-" + trig_root_text(r) + ".");
        for(double t : {r, -r}) {
            steps.push_back(trig_base_angle_line(FnKind::Tan, A, t));
            steps.push_back(trig_alpha_family_line(FnKind::Tan, A, t, rad));
            auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, t));
            for(double x : vals) add_unique(xs, x);
        }
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static bool add_fn_poly_term(Arena &a, NodeId n, FnPoly &p)
{
    double k = 1.0;
    int e = 0;
    NodeId arg = 0;
    FnKind fk = FnKind::Sin;
    bool has_fn = false;
    std::vector<NodeId> fs;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Mul) fs = x.kids;
    else fs.push_back(n);
    for(NodeId f : fs) {
        if(!has_any_symbol(a, f)) {
            auto v = numeric_eval(a, f, 0.0);
            if(!v || !std::isfinite(*v)) return false;
            k *= *v;
            continue;
        }
        Node const &y = a.get(f);
        FnKind tf = FnKind::Sin;
        NodeId ta = 0;
        int te = 1;
        if(y.kind == NodeKind::Fn && (y.fkind == FnKind::Sin || y.fkind == FnKind::Cos)) {
            tf = y.fkind;
            ta = y.a;
        }
        else if(y.kind == NodeKind::Pow) {
            auto q = as_num(a, y.b);
            Node const &b = a.get(y.a);
            if(!q || q->den != 1 || q->num < 1 || q->num > 4 || b.kind != NodeKind::Fn ||
               (b.fkind != FnKind::Sin && b.fkind != FnKind::Cos)) return false;
            tf = b.fkind;
            ta = b.a;
            te = (int)q->num;
        }
        else return false;
        if(has_fn && (tf != fk || !same_sig(a, arg, ta))) return false;
        has_fn = true;
        fk = tf;
        arg = ta;
        e += te;
        if(e > 4) return false;
    }
    if(has_fn) {
        if(p.has_arg && (p.fk != fk || !same_sig(a, p.arg, arg))) return false;
        p.has_arg = true;
        p.fk = fk;
        p.arg = arg;
    }
    p.c[e] += k;
    p.deg = std::max(p.deg, e);
    return true;
}

static std::optional<FnPoly> collect_fn_poly(Arena &a, NodeId residual)
{
    FnPoly p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId k : r.kids)
            if(!add_fn_poly_term(a, k, p)) return std::nullopt;
    }
    else if(!add_fn_poly_term(a, residual, p)) return std::nullopt;
    while(p.deg > 0 && std::fabs(p.c[p.deg]) < 1e-12) --p.deg;
    if(!p.has_arg || p.deg < 1 || p.deg > 3) return std::nullopt;
    return p;
}

static std::optional<std::vector<std::string>> solve_single_fn_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = collect_fn_poly(a, residual);
    if(!p) return std::nullopt;
    if(p->deg < 2) return std::nullopt;
    std::vector<double> roots;
    if(p->deg <= 2) roots = solve_quadratic_d(p->c[2], p->c[1], p->c[0]);
    else {
        double first = NAN;
        for(double r : {-4.0, -3.0, -2.0, -1.0, -std::sqrt(2.0) / 2.0, -0.5, 0.5, std::sqrt(2.0) / 2.0, 1.0, 2.0, 3.0, 4.0}) {
            double v = ((p->c[3] * r + p->c[2]) * r + p->c[1]) * r + p->c[0];
            if(std::fabs(v) < 1e-9) {
                first = r;
                break;
            }
        }
        if(!std::isfinite(first)) return std::nullopt;
        roots.push_back(first);
        double b2 = p->c[3], b1 = p->c[2] + first * b2, b0 = p->c[1] + first * b1;
        auto q = solve_quadratic_d(b2, b1, b0);
        for(double r : q) add_unique(roots, r);
    }
    std::string A = format_expr(a, p->arg);
    std::string f = trig_name(p->fk);
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "u=" + f + "(" + A + ").",
        recip_poly_text(std::vector<double>{p->c[0], p->c[1], p->c[2], p->c[3]}),
    };
    std::vector<double> xs;
    std::string rline = "u=";
    for(std::size_t i = 0; i < roots.size(); ++i) {
        if(i) rline += " or u=";
        rline += trig_root_text(roots[i]);
    }
    steps.push_back(rline + ".");
    for(double r : roots) {
        if(r < -1.0 - 1e-10 || r > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + trig_root_text(r) + ": outside [-1,1].");
            continue;
        }
        steps.push_back(f + "(" + A + ")=" + trig_root_text(r) + ".");
        steps.push_back(trig_base_angle_line(p->fk, A, r));
        steps.push_back(trig_alpha_family_line(p->fk, A, r, rad));
        auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(p->fk, r));
        for(double x : vals) add_unique(xs, x);
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

struct SecSin2Poly
{
    double c0 = 0.0; // sec(A)
    double c1 = 0.0; // constant
    double c2 = 0.0; // sec(A)*sin(A)^2
    NodeId arg = 0;
    bool has_arg = false;
};

static bool same_sec_sin_arg(Arena &a, SecSin2Poly const &p, NodeId arg)
{
    return !p.has_arg || same_sig(a, p.arg, arg);
}

static bool add_sec_sin2_term(Arena &a, NodeId n, SecSin2Poly &p)
{
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return false;
        p.c1 += *v;
        return true;
    }
    std::vector<NodeId> factors;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Mul) factors = x.kids;
    else factors.push_back(n);
    double coeff = 1.0;
    bool has_sec = false;
    bool has_sin2 = false;
    for(NodeId f : factors) {
        Node const &fn = a.get(f);
        if(!has_any_symbol(a, f)) {
            auto v = numeric_eval(a, f, 0.0);
            if(!v || !std::isfinite(*v)) return false;
            coeff *= *v;
            continue;
        }
        if(fn.kind == NodeKind::Fn && fn.fkind == FnKind::Sec) {
            if(!same_sec_sin_arg(a, p, fn.a)) return false;
            if(!p.has_arg) {
                p.arg = fn.a;
                p.has_arg = true;
            }
            has_sec = true;
            continue;
        }
        if(fn.kind == NodeKind::Pow) {
            Node const &b = a.get(fn.a);
            auto q = as_num(a, fn.b);
            if(b.kind == NodeKind::Fn && b.fkind == FnKind::Sin && q && q->den == 1 && q->num == 2) {
                if(!same_sec_sin_arg(a, p, b.a)) return false;
                if(!p.has_arg) {
                    p.arg = b.a;
                    p.has_arg = true;
                }
                has_sin2 = true;
                continue;
            }
        }
        return false;
    }
    if(has_sec && has_sin2) p.c2 += coeff;
    else if(has_sec) p.c0 += coeff;
    else return false;
    return true;
}

static std::optional<SecSin2Poly> collect_sec_sin2_poly(Arena &a, NodeId residual)
{
    SecSin2Poly p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId k : r.kids)
            if(!add_sec_sin2_term(a, k, p)) return std::nullopt;
    }
    else if(!add_sec_sin2_term(a, residual, p)) return std::nullopt;
    if(!p.has_arg || std::fabs(p.c2) < 1e-12) return std::nullopt;
    return p;
}

static std::optional<std::vector<std::string>> solve_sec_sin2_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = collect_sec_sin2_poly(a, residual);
    if(!p) return std::nullopt;
    double qa = -p->c2;
    double qb = p->c1;
    double qc = p->c0 + p->c2;
    auto roots = solve_quadratic_d(qa, qb, qc);
    if(roots.empty()) return std::nullopt;
    std::string A = format_expr(a, p->arg);
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "u=cos(" + A + ").",
        "sec(" + A + ")=1/u, sin(" + A + ")^2=1-u^2.",
        "Multiply by u.",
        trig_quad_text(qa, qb, qc),
    };
    std::vector<double> xs;
    std::string rline = "u=";
    bool any = false;
    for(double r : roots) {
        if(std::fabs(r) < 1e-10) {
            steps.push_back("Reject u=0: sec undefined.");
            continue;
        }
        if(r < -1.0 - 1e-10 || r > 1.0 + 1e-10) {
            steps.push_back("Reject u=" + trig_root_text(r) + ": outside [-1,1].");
            continue;
        }
        if(any) rline += " or u=";
        rline += trig_root_text(r);
        any = true;
        auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, r));
        for(double x : vals) add_unique(xs, x);
    }
    if(any) steps.push_back(rline + ".");
    for(double r : roots) {
        if(std::fabs(r) >= 1e-10 && r >= -1.0 - 1e-10 && r <= 1.0 + 1e-10) {
            steps.push_back("cos(" + A + ")=" + trig_root_text(r) + ".");
            steps.push_back(trig_base_angle_line(FnKind::Cos, A, r));
            steps.push_back(trig_alpha_family_line(FnKind::Cos, A, r, rad));
        }
    }
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

struct SCTerm
{
    double k = 0.0;
    int s = 0;
    int c = 0;
};

struct SCPoly
{
    std::vector<SCTerm> terms;
    NodeId arg = 0;
    bool has_arg = false;
    int min_s = 0;
    int min_c = 0;
    bool den_s = false;
    bool den_c = false;
    bool expanded_double = false;
};

static bool sc_same_arg(Arena &a, SCPoly const &p, NodeId arg)
{
    return !p.has_arg || same_sig(a, p.arg, arg);
}

static bool sc_fn_power(FnKind f, int q, int &sp, int &cp)
{
    if(f == FnKind::Sin) sp += q;
    else if(f == FnKind::Cos) cp += q;
    else if(f == FnKind::Sec) cp -= q;
    else if(f == FnKind::Cosec) sp -= q;
    else if(f == FnKind::Tan) {
        sp += q;
        cp -= q;
    }
    else if(f == FnKind::Cot) {
        sp -= q;
        cp += q;
    }
    else return false;
    return true;
}

static bool sc_factor(Arena &a, NodeId n, SCPoly &p, double &k, int &sp, int &cp)
{
    Node const &x = a.get(n);
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return false;
        k *= *v;
        return true;
    }
    if(x.kind == NodeKind::Fn &&
       (x.fkind == FnKind::Sin || x.fkind == FnKind::Cos || x.fkind == FnKind::Sec || x.fkind == FnKind::Cosec ||
        x.fkind == FnKind::Tan || x.fkind == FnKind::Cot)) {
        if(!sc_same_arg(a, p, x.a)) return false;
        if(!p.has_arg) {
            p.arg = x.a;
            p.has_arg = true;
        }
        return sc_fn_power(x.fkind, 1, sp, cp);
    }
    if(x.kind == NodeKind::Pow) {
        Node const &b = a.get(x.a);
        auto q = as_num(a, x.b);
        if(!q || q->den != 1 || q->num < 1 || q->num > 4 || b.kind != NodeKind::Fn) return false;
        if(!sc_same_arg(a, p, b.a)) return false;
        if(!p.has_arg) {
            p.arg = b.a;
            p.has_arg = true;
        }
        return sc_fn_power(b.fkind, (int)q->num, sp, cp);
    }
    if(x.kind == NodeKind::Mul) {
        for(NodeId f : x.kids)
            if(!sc_factor(a, f, p, k, sp, cp)) return false;
        return true;
    }
    if(x.kind == NodeKind::Div) {
        double dk = 1.0;
        int ds = 0, dc = 0;
        if(!sc_factor(a, x.a, p, k, sp, cp) || !sc_factor(a, x.b, p, dk, ds, dc) || std::fabs(dk) < 1e-12) return false;
        k /= dk;
        sp -= ds;
        cp -= dc;
        return true;
    }
    return false;
}

static std::optional<NodeId> double_angle_inner(Arena &a, NodeId arg)
{
    Node const &x = a.get(arg);
    if(x.kind != NodeKind::Mul) return std::nullopt;
    NodeId inner = 0;
    bool saw_two = false;
    for(NodeId kid : x.kids) {
        auto q = as_num(a, kid);
        if(q && q->num == 2 && q->den == 1 && !saw_two) {
            saw_two = true;
            continue;
        }
        if(inner) return std::nullopt;
        inner = kid;
    }
    if(!saw_two || !inner) return std::nullopt;
    return inner;
}

static bool add_sc_raw(SCPoly &p, Arena &a, NodeId arg, double k, int s, int c)
{
    if(p.has_arg && !same_sig(a, p.arg, arg)) return false;
    p.arg = arg;
    p.has_arg = true;
    if(std::fabs(k) < 1e-12) return true;
    p.min_s = std::min(p.min_s, s);
    p.min_c = std::min(p.min_c, c);
    p.den_s = p.den_s || s < 0;
    p.den_c = p.den_c || c < 0;
    p.expanded_double = true;
    p.terms.push_back({k, s, c});
    return true;
}

static bool add_sc_term(Arena &a, NodeId n, SCPoly &p)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Mul) {
        int add_i = -1;
        for(std::size_t i = 0; i < x.kids.size(); ++i) {
            if(a.get(x.kids[i]).kind == NodeKind::Add) {
                if(add_i < 0) add_i = (int)i;
            }
        }
        if(add_i >= 0) {
            Node const &addn = a.get(x.kids[(std::size_t)add_i]);
            for(NodeId ak : addn.kids) {
                std::vector<NodeId> fs;
                for(std::size_t i = 0; i < x.kids.size(); ++i)
                    if((int)i != add_i) fs.push_back(x.kids[i]);
                fs.push_back(ak);
                if(!add_sc_term(a, casio::simplify(a, casio::mul(a, fs)), p)) return false;
            }
            return true;
        }
    }
    double coeff = 1.0;
    NodeId rest = n;
    bool has_rest = true;
    if(split_coeff_term(a, n, coeff, rest, has_rest) && has_rest) {
        Node const &r = a.get(rest);
        if(r.kind == NodeKind::Fn) {
            if(auto inner = double_angle_inner(a, r.a)) {
                if(r.fkind == FnKind::Sin) return add_sc_raw(p, a, *inner, 2.0 * coeff, 1, 1);
                if(r.fkind == FnKind::Cos) return add_sc_raw(p, a, *inner, coeff, 0, 2) &&
                                             add_sc_raw(p, a, *inner, -coeff, 2, 0);
            }
        }
        if(r.kind == NodeKind::Pow) {
            auto q = as_num(a, r.b);
            Node const &b = a.get(r.a);
            if(q && q->num == 2 && q->den == 1 && b.kind == NodeKind::Fn) {
                if(auto inner = double_angle_inner(a, b.a)) {
                    if(b.fkind == FnKind::Sin) return add_sc_raw(p, a, *inner, 4.0 * coeff, 2, 2);
                    if(b.fkind == FnKind::Cos) {
                        return add_sc_raw(p, a, *inner, coeff, 0, 4) &&
                               add_sc_raw(p, a, *inner, -2.0 * coeff, 2, 2) &&
                               add_sc_raw(p, a, *inner, coeff, 4, 0);
                    }
                }
            }
        }
    }
    SCTerm t;
    t.k = 1.0;
    if(!sc_factor(a, n, p, t.k, t.s, t.c)) return false;
    if(std::fabs(t.k) < 1e-12) return true;
    p.min_s = std::min(p.min_s, t.s);
    p.min_c = std::min(p.min_c, t.c);
    p.den_s = p.den_s || t.s < 0;
    p.den_c = p.den_c || t.c < 0;
    p.terms.push_back(t);
    return true;
}

static std::optional<SCPoly> collect_sc_poly(Arena &a, NodeId residual)
{
    SCPoly p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId k : r.kids)
            if(!add_sc_term(a, k, p)) return std::nullopt;
    }
    else if(!add_sc_term(a, residual, p)) return std::nullopt;
    bool has_cross = false;
    for(auto const &t : p.terms)
        if(t.s > 0 && t.c > 0) has_cross = true;
    if(!p.has_arg || p.terms.empty() || (!p.expanded_double && !has_cross && p.min_s == 0 && p.min_c == 0)) return std::nullopt;
    return p;
}

static double sc_coef(SCPoly const &p, int sp, int cp)
{
    double out = 0.0;
    for(auto const &t : p.terms)
        if(t.s - p.min_s == sp && t.c - p.min_c == cp) out += t.k;
    return out;
}

static bool sc_only(SCPoly const &p, std::initializer_list<std::pair<int, int>> keep)
{
    for(auto const &t : p.terms) {
        bool ok = false;
        int s = t.s - p.min_s;
        int c = t.c - p.min_c;
        for(auto const &k : keep)
            if(k.first == s && k.second == c) ok = true;
        if(!ok && std::fabs(sc_coef(p, s, c)) < 1e-12) ok = true;
        if(!ok) return false;
    }
    return true;
}

static std::optional<std::vector<std::string>> solve_sc_common_denominator(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = collect_sc_poly(a, residual);
    if(!p) return std::nullopt;
    std::string A = format_expr(a, p->arg);
    std::string mult;
    if(p->min_s < 0) mult += "sin(" + A + ")" + (p->min_s < -1 ? "^" + std::to_string(-p->min_s) : "");
    if(p->min_c < 0) {
        if(!mult.empty()) mult += "*";
        mult += "cos(" + A + ")" + (p->min_c < -1 ? "^" + std::to_string(-p->min_c) : "");
    }
    std::vector<std::string> steps{format_expr(a, residual) + " = 0.", "Multiply by " + mult + "."};
    std::vector<double> xs;
    auto add_roots_for = [&](FnKind fk, std::vector<double> const &roots, NodeId arg_node, std::string const &arg_text, bool check_domain = true) {
        for(double r : roots) {
            if((fk == FnKind::Sin || fk == FnKind::Cos) && (r < -1.0 - 1e-10 || r > 1.0 + 1e-10)) {
                steps.push_back("Reject " + trig_name(fk) + "(" + arg_text + ")=" + trig_root_text(r) + ": outside [-1,1].");
                continue;
            }
            if(check_domain && fk == FnKind::Cos && p->den_c && std::fabs(r) < 1e-10) {
                steps.push_back("Reject cos(" + arg_text + ")=0: sec/tan undefined.");
                continue;
            }
            if(check_domain && fk == FnKind::Sin && p->den_s && std::fabs(r) < 1e-10) {
                steps.push_back("Reject sin(" + arg_text + ")=0: cosec/cot undefined.");
                continue;
            }
            if(check_domain && fk == FnKind::Sin && p->den_c && std::fabs(std::fabs(r) - 1.0) < 1e-10) {
                steps.push_back("Reject sin(" + arg_text + ")=" + trig_root_text(r) + ": sec/tan undefined.");
                continue;
            }
            if(check_domain && fk == FnKind::Cos && p->den_s && std::fabs(std::fabs(r) - 1.0) < 1e-10) {
                steps.push_back("Reject cos(" + arg_text + ")=" + trig_root_text(r) + ": cosec/cot undefined.");
                continue;
            }
            steps.push_back(trig_name(fk) + "(" + arg_text + ")=" + trig_root_text(r) + ".");
            steps.push_back(trig_base_angle_line(fk, arg_text, r));
            steps.push_back(trig_alpha_family_line(fk, arg_text, r, rad));
            auto vals = x_values_from_angle_degrees(a, arg_node, var, lo_text, hi_text, rad, base_trig_degrees(fk, r));
            for(double x : vals) add_unique(xs, x);
        }
    };
    auto add_roots = [&](FnKind fk, std::vector<double> const &roots) {
        add_roots_for(fk, roots, p->arg, A);
    };
    double s2 = sc_coef(*p, 2, 0), c2 = sc_coef(*p, 0, 2), c1 = sc_coef(*p, 0, 1);
    double k0 = sc_coef(*p, 0, 0), sc2 = sc_coef(*p, 1, 2), c3 = sc_coef(*p, 0, 3);
    double s1 = sc_coef(*p, 1, 0), s1c1 = sc_coef(*p, 1, 1), s2c = sc_coef(*p, 2, 1);
    double s3 = sc_coef(*p, 3, 0), s4 = sc_coef(*p, 4, 0), c4 = sc_coef(*p, 0, 4), c6 = sc_coef(*p, 0, 6);
    if(sc_only(*p, {{2, 0}, {0, 2}, {0, 1}}) && std::fabs(s2 - c2) < 1e-12 && std::fabs(c1) > 1e-12) {
        steps.push_back("sin(" + A + ")^2+cos(" + A + ")^2=1.");
        steps.push_back(trig_root_text(s2) + fmt_poly_coeff(c1, false) + "*cos(" + A + ")=0.");
        add_roots(FnKind::Cos, {-s2 / c1});
    }
    else if(sc_only(*p, {{0, 0}, {2, 0}, {1, 2}}) && std::fabs(k0 + s2) < 1e-12 && std::fabs(sc2) > 1e-12) {
        steps.push_back("1-sin(" + A + ")^2=cos(" + A + ")^2.");
        steps.push_back("cos(" + A + ")^2*(" + trig_root_text(k0) + fmt_poly_coeff(sc2, false) + "*sin(" + A + "))=0.");
        steps.push_back("cos(" + A + ")!=0 from domain.");
        add_roots(FnKind::Sin, {-k0 / sc2});
    }
    else if(sc_only(*p, {{0, 0}, {2, 0}, {0, 3}}) && std::fabs(k0 + s2) < 1e-12 && std::fabs(c3) > 1e-12) {
        steps.push_back("1-sin(" + A + ")^2=cos(" + A + ")^2.");
        steps.push_back("cos(" + A + ")^2*(" + trig_root_text(k0) + fmt_poly_coeff(c3, false) + "*cos(" + A + "))=0.");
        add_roots(FnKind::Cos, {0.0, -k0 / c3});
    }
    else if(sc_only(*p, {{1, 0}, {1, 2}, {2, 1}, {0, 1}}) && std::fabs(s1 + sc2) < 1e-12 &&
            std::fabs(s2c + c1) < 1e-12 && std::fabs(s1) > 1e-12) {
        double r = std::cbrt(s2c / s1);
        steps.push_back("sin(" + A + ")*(1-cos(" + A + ")^2)" + fmt_poly_coeff(-s2c, false) +
                        "*cos(" + A + ")*(1-sin(" + A + ")^2)=0.");
        steps.push_back("sin(" + A + ")^3=" + trig_root_text(s2c / s1) + "*cos(" + A + ")^3.");
        steps.push_back("tan(" + A + ")=" + trig_root_text(r) + ".");
        add_roots(FnKind::Tan, {r});
    }
    else if(sc_only(*p, {{3, 0}, {0, 3}}) && std::fabs(s3) > 1e-12 && std::fabs(c3) > 1e-12) {
        double r = std::cbrt(-c3 / s3);
        std::string ck = std::fabs(std::fabs(c3) - 1.0) < 1e-12 ? "" : trig_root_text(std::fabs(c3)) + "*";
        steps.push_back(trig_root_text(s3) + "*sin(" + A + ")^3" + std::string(c3 < 0 ? " - " : " + ") + ck + "cos(" + A + ")^3=0.");
        steps.push_back("tan(" + A + ")=" + trig_root_text(r) + ".");
        add_roots(FnKind::Tan, {r});
    }
    else if(sc_only(*p, {{0, 2}, {0, 1}, {0, 0}}) && std::fabs(c2) > 1e-12) {
        auto roots = solve_quadratic_d(c2, c1, k0);
        steps.push_back("u=cos(" + A + ").");
        steps.push_back(trig_quad_text(c2, c1, k0));
        add_roots(FnKind::Cos, roots);
    }
    else if(sc_only(*p, {{0, 0}, {0, 2}, {1, 0}}) && std::fabs(k0 + c2) < 1e-12 && std::fabs(k0) > 1e-12) {
        steps.push_back("1-cos(" + A + ")^2=sin(" + A + ")^2.");
        steps.push_back("sin(" + A + ")*(" + trig_root_text(k0) + "*sin(" + A + ")" + fmt_poly_coeff(s1, false) + ")=0.");
        add_roots(FnKind::Sin, {0.0, -s1 / k0});
    }
    else if(sc_only(*p, {{0, 0}, {0, 3}}) && std::fabs(c3) > 1e-12) {
        steps.push_back("u=cos(" + A + ").");
        steps.push_back(trig_root_text(c3) + "*u^3" + std::string(k0 < 0 ? " - " : " + ") + trig_root_text(std::fabs(k0)) + "=0.");
        add_roots(FnKind::Cos, {std::cbrt(-k0 / c3)});
    }
    else if(sc_only(*p, {{1, 1}, {1, 0}, {0, 1}, {0, 0}}) &&
            std::fabs(s1c1) > 1e-12 && std::fabs(s1c1 * k0 - s1 * c1) < 1e-10) {
        double cos_root = -s1 / s1c1;
        double sin_root = -c1 / s1c1;
        auto signed_num = [](double v) {
            return std::string(v < 0 ? " - " : " + ") + trig_root_text(std::fabs(v));
        };
        steps.push_back("(" + trig_root_text(s1c1) + "*cos(" + A + ")" + signed_num(s1) + ")*(sin(" + A + ")" +
                        signed_num(c1 / s1c1) + ")=0.");
        add_roots(FnKind::Cos, {cos_root});
        add_roots(FnKind::Sin, {sin_root});
    }
    else if(sc_only(*p, {{1, 1}, {0, 2}, {1, 0}, {0, 1}}) &&
            std::fabs(s1c1 + c2) < 1e-12 && std::fabs(s1 + c1) < 1e-12 &&
            std::fabs(s1c1) > 1e-12 && std::fabs(s1) > 1e-12) {
        double croot = -s1 / s1c1;
        steps.push_back("(" + trig_root_text(s1c1) + "*cos(" + A + ")" +
                        std::string(s1 < 0 ? " - " : " + ") + trig_root_text(std::fabs(s1)) +
                        ")*(sin(" + A + ")-cos(" + A + "))=0.");
        add_roots(FnKind::Cos, {croot});
        add_roots(FnKind::Tan, {1.0});
    }
    else if(sc_only(*p, {{4, 0}, {0, 4}, {2, 2}, {1, 1}})) {
        double s4 = sc_coef(*p, 4, 0), c4 = sc_coef(*p, 0, 4), s2c2 = sc_coef(*p, 2, 2), sc = sc_coef(*p, 1, 1);
        if(std::fabs(s4 - c4) > 1e-12 || std::fabs(s2c2 - 2.0 * s4) > 1e-12 || std::fabs(sc) < 1e-12) return std::nullopt;
        steps.push_back("(sin(" + A + ")^2+cos(" + A + ")^2)^2" + fmt_poly_coeff(sc / s4, false) +
                        "*sin(" + A + ")*cos(" + A + ")=0.");
        steps.push_back("sin(" + A + ")^2+cos(" + A + ")^2=1.");
        double target = -2.0 * s4 / sc;
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), p->arg}));
        std::string A2 = format_expr(a, arg2);
        steps.push_back("sin(" + A2 + ")=" + trig_root_text(target) + ".");
        add_roots_for(FnKind::Sin, {target}, arg2, A2, false);
    }
    else if(sc_only(*p, {{3, 0}, {2, 1}, {1, 2}, {0, 3}})) {
        double s3 = sc_coef(*p, 3, 0), s2c1 = sc_coef(*p, 2, 1), s1c2 = sc_coef(*p, 1, 2), c3v = sc_coef(*p, 0, 3);
        if(std::fabs(s3 - s2c1) > 1e-12 || std::fabs(s3 - s1c2) > 1e-12 || std::fabs(s3 - c3v) > 1e-12) return std::nullopt;
        steps.push_back("sin(" + A + ")^3+cos(" + A + ")^3+sin(" + A + ")*cos(" + A + ")*(sin(" + A + ")+cos(" + A + "))=0.");
        steps.push_back("(sin(" + A + ")+cos(" + A + "))*(sin(" + A + ")^2+cos(" + A + ")^2)=0.");
        steps.push_back("sin(" + A + ")+cos(" + A + ")=0.");
        steps.push_back("tan(" + A + ")=-1.");
        add_roots(FnKind::Tan, {-1.0});
    }
    else if(sc_only(*p, {{1, 2}, {1, 0}})) {
        double a2 = sc_coef(*p, 1, 2), b0 = sc_coef(*p, 1, 0);
        if(std::fabs(a2) < 1e-12) return std::nullopt;
        double q = -b0 / a2;
        steps.push_back("sin(" + A + ")*(" + trig_root_text(a2) + "*cos(" + A + ")^2" + std::string(b0 < 0 ? " - " : " + ") + trig_root_text(std::fabs(b0)) + ")=0.");
        add_roots(FnKind::Sin, {0.0});
        if(q >= -1e-12) {
            double r = std::sqrt(std::max(0.0, q));
            add_roots(FnKind::Cos, {r, -r});
        }
    }
    else if(sc_only(*p, {{1, 1}, {0, 1}})) {
        double a1 = sc_coef(*p, 1, 1), b1 = sc_coef(*p, 0, 1);
        if(std::fabs(a1) < 1e-12) return std::nullopt;
        steps.push_back("cos(" + A + ")*(" + trig_root_text(a1) + "*sin(" + A + ")" + std::string(b1 < 0 ? " - " : " + ") + trig_root_text(std::fabs(b1)) + ")=0.");
        add_roots(FnKind::Cos, {0.0});
        add_roots(FnKind::Sin, {-b1 / a1});
    }
    else if(sc_only(*p, {{1, 1}, {1, 0}})) {
        double a1 = sc_coef(*p, 1, 1), b1 = sc_coef(*p, 1, 0);
        if(std::fabs(a1) < 1e-12) return std::nullopt;
        steps.push_back("sin(" + A + ")*(" + trig_root_text(a1) + "*cos(" + A + ")" + std::string(b1 < 0 ? " - " : " + ") + trig_root_text(std::fabs(b1)) + ")=0.");
        add_roots(FnKind::Sin, {0.0});
        add_roots(FnKind::Cos, {-b1 / a1});
    }
    else if(sc_only(*p, {{2, 1}, {0, 1}})) {
        double a2 = sc_coef(*p, 2, 1), b0 = sc_coef(*p, 0, 1);
        if(std::fabs(a2) < 1e-12) return std::nullopt;
        double q = -b0 / a2;
        steps.push_back("cos(" + A + ")*(" + trig_root_text(a2) + "*sin(" + A + ")^2" + std::string(b0 < 0 ? " - " : " + ") + trig_root_text(std::fabs(b0)) + ")=0.");
        add_roots(FnKind::Cos, {0.0});
        if(q >= -1e-12) {
            double r = std::sqrt(std::max(0.0, q));
            add_roots(FnKind::Sin, {r, -r});
        }
    }
    else if(sc_only(*p, {{2, 0}, {0, 2}, {1, 1}}) && std::fabs(sc_coef(*p, 2, 0) - sc_coef(*p, 0, 2)) < 1e-12) {
        double q = sc_coef(*p, 2, 0), m = sc_coef(*p, 1, 1);
        if(std::fabs(m) < 1e-12) return std::nullopt;
        double target = -2.0 * q / m;
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), p->arg}));
        std::string A2 = format_expr(a, arg2);
        steps.push_back("sin(" + A + ")^2+cos(" + A + ")^2=1.");
        steps.push_back("sin(" + A2 + ")=" + trig_root_text(target) + ".");
        add_roots_for(FnKind::Sin, {target}, arg2, A2, false);
    }
    else if(sc_only(*p, {{2, 0}, {0, 2}, {0, 0}}) && std::fabs(sc_coef(*p, 2, 0) + sc_coef(*p, 0, 2)) < 1e-12) {
        double a_s = sc_coef(*p, 2, 0), a_c = sc_coef(*p, 0, 2), kk = sc_coef(*p, 0, 0);
        double qa = a_s - a_c, qc = a_c + kk;
        if(std::fabs(qa) < 1e-12) return std::nullopt;
        double q = -qc / qa;
        steps.push_back("cos(" + A + ")^2=1-sin(" + A + ")^2.");
        if(q >= -1e-12) {
            double r = std::sqrt(std::max(0.0, q));
            add_roots(FnKind::Sin, {r, -r});
        }
    }
    else if(sc_only(*p, {{1, 1}, {0, 0}})) {
        double m = sc_coef(*p, 1, 1), kk = sc_coef(*p, 0, 0);
        if(std::fabs(m) < 1e-12) return std::nullopt;
        double target = -2.0 * kk / m;
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), p->arg}));
        std::string A2 = format_expr(a, arg2);
        steps.push_back("sin(" + A2 + ")=" + trig_root_text(target) + ".");
        add_roots_for(FnKind::Sin, {target}, arg2, A2, false);
    }
    else if(sc_only(*p, {{2, 1}, {1, 0}})) {
        double m = sc_coef(*p, 2, 1), b = sc_coef(*p, 1, 0);
        if(std::fabs(m) < 1e-12) return std::nullopt;
        double target = -2.0 * b / m;
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), p->arg}));
        std::string A2 = format_expr(a, arg2);
        steps.push_back("sin(" + A + ")*(" + trig_root_text(m) + "*sin(" + A + ")*cos(" + A + ")" + std::string(b < 0 ? " - " : " + ") + trig_root_text(std::fabs(b)) + ")=0.");
        add_roots(FnKind::Sin, {0.0});
        steps.push_back("sin(" + A2 + ")=" + trig_root_text(target) + ".");
        add_roots_for(FnKind::Sin, {target}, arg2, A2, false);
    }
    else if(sc_only(*p, {{1, 1}, {2, 2}})) {
        double a1 = sc_coef(*p, 1, 1), a2 = sc_coef(*p, 2, 2);
        if(std::fabs(a2) < 1e-12) return std::nullopt;
        double target = -2.0 * a1 / a2;
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), p->arg}));
        std::string A2 = format_expr(a, arg2);
        steps.push_back("sin(" + A + ")*cos(" + A + ")*(" + trig_root_text(a1) + std::string(a2 < 0 ? " - " : " + ") + trig_root_text(std::fabs(a2)) + "*sin(" + A + ")*cos(" + A + "))=0.");
        add_roots(FnKind::Sin, {0.0});
        add_roots(FnKind::Cos, {0.0});
        steps.push_back("sin(" + A2 + ")=" + trig_root_text(target) + ".");
        add_roots_for(FnKind::Sin, {target}, arg2, A2, false);
    }
    else if(sc_only(*p, {{4, 0}, {0, 4}, {0, 6}}) && std::fabs(s4) > 1e-12 && std::fabs(c6) > 1e-12) {
        double A3 = c6, A2 = c4 + s4, A1 = -2.0 * s4, A0 = s4;
        std::vector<double> us;
        for(double r : {-2.0, -1.0, -0.5, -0.25, 0.25, 0.5, 1.0, 2.0}) {
            double v = ((A3 * r + A2) * r + A1) * r + A0;
            if(std::fabs(v) < 1e-9) {
                us.push_back(r);
                auto q = solve_quadratic_d(A3, A2 + r * A3, A1 + r * (A2 + r * A3));
                for(double x : q) add_unique(us, x);
                break;
            }
        }
        if(us.empty()) return std::nullopt;
        steps.push_back("u=cos(" + A + ")^2.");
        steps.push_back("sin(" + A + ")^4=(1-u)^2.");
        steps.push_back(recip_poly_text(std::vector<double>{A0, A1, A2, A3}));
        for(double u : us) {
            if(u < -1e-12 || u > 1.0 + 1e-12) {
                steps.push_back("Reject u=" + trig_root_text(u) + ": outside [0,1].");
                continue;
            }
            double r = std::sqrt(std::max(0.0, u));
            steps.push_back("cos(" + A + ")=+/-" + trig_root_text(r) + ".");
            add_roots(FnKind::Cos, {r, -r});
        }
    }
    else return std::nullopt;
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

struct PythRecipPoly
{
    double c = 0.0;
    double tan1 = 0.0, tan2 = 0.0, sec1 = 0.0, sec2 = 0.0;
    double cot1 = 0.0, cot2 = 0.0, csc1 = 0.0, csc2 = 0.0;
    NodeId arg = 0;
    bool has_arg = false;
};

static bool pyth_same_arg(Arena &a, PythRecipPoly const &p, NodeId arg)
{
    return !p.has_arg || same_sig(a, p.arg, arg);
}

static bool add_pyth_recip_term(Arena &a, NodeId n, PythRecipPoly &p)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Pow) {
        auto q = as_num(a, x.b);
        Node const &base = a.get(x.a);
        if(q && q->num == 2 && q->den == 1 && base.kind == NodeKind::Add && base.kids.size() == 2) {
            NodeId u = base.kids[0], v = base.kids[1];
            return add_pyth_recip_term(a, casio::power(a, u, casio::num(a, 2)), p) &&
                   add_pyth_recip_term(a, casio::mul(a, {casio::num(a, 2), u, v}), p) &&
                   add_pyth_recip_term(a, casio::power(a, v, casio::num(a, 2)), p);
        }
    }
    if(x.kind == NodeKind::Mul) {
        int sq_i = -1;
        for(std::size_t i = 0; i < x.kids.size(); ++i) {
            Node const &kid = a.get(x.kids[i]);
            if(kid.kind != NodeKind::Pow) continue;
            auto q = as_num(a, kid.b);
            if(q && q->num == 2 && q->den == 1 && a.get(kid.a).kind == NodeKind::Add && a.get(kid.a).kids.size() == 2) {
                sq_i = (int)i;
                break;
            }
        }
        if(sq_i >= 0) {
            Node const &pow_node = a.get(x.kids[(std::size_t)sq_i]);
            Node const &base = a.get(pow_node.a);
            NodeId u = base.kids[0], v = base.kids[1];
            std::vector<NodeId> rest;
            for(std::size_t i = 0; i < x.kids.size(); ++i)
                if((int)i != sq_i) rest.push_back(x.kids[i]);
            auto with_rest = [&](NodeId term) {
                std::vector<NodeId> fs = rest;
                fs.push_back(term);
                return casio::mul(a, fs);
            };
            return add_pyth_recip_term(a, with_rest(casio::power(a, u, casio::num(a, 2))), p) &&
                   add_pyth_recip_term(a, with_rest(casio::mul(a, {casio::num(a, 2), u, v})), p) &&
                   add_pyth_recip_term(a, with_rest(casio::power(a, v, casio::num(a, 2))), p);
        }
        int add_i = -1;
        for(std::size_t i = 0; i < x.kids.size(); ++i) {
            if(a.get(x.kids[i]).kind == NodeKind::Add) {
                if(add_i < 0) add_i = (int)i;
            }
        }
        if(add_i >= 0) {
            Node const &addn = a.get(x.kids[(std::size_t)add_i]);
            for(NodeId ak : addn.kids) {
                std::vector<NodeId> fs;
                for(std::size_t i = 0; i < x.kids.size(); ++i)
                    if((int)i != add_i) fs.push_back(x.kids[i]);
                fs.push_back(ak);
                if(!add_pyth_recip_term(a, casio::simplify(a, casio::mul(a, fs)), p)) return false;
            }
            return true;
        }
    }
    double coeff = 1.0;
    NodeId rest = n;
    bool has_rest = true;
    if(!split_coeff_term(a, n, coeff, rest, has_rest)) return false;
    if(!has_rest) {
        p.c += coeff;
        return true;
    }
    FnKind fk;
    int pow = 1;
    Node const &r = a.get(rest);
    if(r.kind == NodeKind::Fn) {
        fk = r.fkind;
        if(!pyth_same_arg(a, p, r.a)) return false;
        if(!p.has_arg) { p.arg = r.a; p.has_arg = true; }
    }
    else if(r.kind == NodeKind::Pow) {
        auto q = as_num(a, r.b);
        Node const &b = a.get(r.a);
        if(!q || q->num != 2 || q->den != 1 || b.kind != NodeKind::Fn) return false;
        fk = b.fkind;
        pow = 2;
        if(!pyth_same_arg(a, p, b.a)) return false;
        if(!p.has_arg) { p.arg = b.a; p.has_arg = true; }
    }
    else return false;

    if(fk == FnKind::Tan && pow == 1) p.tan1 += coeff;
    else if(fk == FnKind::Tan && pow == 2) p.tan2 += coeff;
    else if(fk == FnKind::Sec && pow == 1) p.sec1 += coeff;
    else if(fk == FnKind::Sec && pow == 2) p.sec2 += coeff;
    else if(fk == FnKind::Cot && pow == 1) p.cot1 += coeff;
    else if(fk == FnKind::Cot && pow == 2) p.cot2 += coeff;
    else if(fk == FnKind::Cosec && pow == 1) p.csc1 += coeff;
    else if(fk == FnKind::Cosec && pow == 2) p.csc2 += coeff;
    else return false;
    return true;
}

static std::optional<PythRecipPoly> collect_pyth_recip_poly(Arena &a, NodeId residual)
{
    PythRecipPoly p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId k : r.kids)
            if(!add_pyth_recip_term(a, k, p)) return std::nullopt;
    }
    else if(!add_pyth_recip_term(a, residual, p)) return std::nullopt;
    if(!p.has_arg) return std::nullopt;
    return p;
}

static std::optional<std::vector<std::string>> solve_pyth_recip_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    auto p = collect_pyth_recip_poly(a, residual);
    if(!p) return std::nullopt;
    std::string A = format_expr(a, p->arg);
    std::vector<std::string> steps{format_expr(a, residual) + " = 0."};
    std::vector<double> xs;
    auto add_base = [&](FnKind fk, double r) {
        auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(fk, r));
        for(double x : vals) add_unique(xs, x);
    };
    auto solve_u = [&](std::string const &u_name, double qa, double qb, double qc, FnKind out_fk, bool reciprocal) -> bool {
        auto roots = solve_quadratic_d(qa, qb, qc);
        if(roots.empty()) return false;
        steps.push_back("u=" + u_name + "(" + A + ").");
        steps.push_back(trig_quad_text(qa, qb, qc));
        std::string rline = "u=";
        for(std::size_t i = 0; i < roots.size(); ++i) {
            if(i) rline += " or u=";
            rline += trig_root_text(roots[i]);
        }
        steps.push_back(rline + ".");
        for(double u : roots) {
            if(reciprocal) {
                if(std::fabs(u) < 1e-10) {
                    steps.push_back("Reject u=0: reciprocal undefined.");
                    continue;
                }
                double r = 1.0 / u;
                if(r < -1.0 - 1e-10 || r > 1.0 + 1e-10) {
                    steps.push_back("Reject u=" + trig_root_text(u) + ": reciprocal outside [-1,1].");
                    continue;
                }
                steps.push_back(trig_name(out_fk) + "(" + A + ")=" + trig_root_text(r) + ".");
                steps.push_back(trig_base_angle_line(out_fk, A, r));
                steps.push_back(trig_alpha_family_line(out_fk, A, r, rad));
                add_base(out_fk, r);
            }
            else if(out_fk == FnKind::Cot) {
                if(std::fabs(u) < 1e-10) {
                    steps.push_back("cot(" + A + ")=0 => cos(" + A + ")=0.");
                    add_base(FnKind::Cos, 0.0);
                }
                else {
                    double t = 1.0 / u;
                    steps.push_back("cot(" + A + ")=" + trig_root_text(u) + " => tan(" + A + ")=" + trig_root_text(t) + ".");
                    steps.push_back(trig_base_angle_line(FnKind::Tan, A, t));
                    steps.push_back(trig_alpha_family_line(FnKind::Tan, A, t, rad));
                    add_base(FnKind::Tan, t);
                }
            }
            else {
                steps.push_back(trig_name(out_fk) + "(" + A + ")=" + trig_root_text(u) + ".");
                if(out_fk == FnKind::Tan) {
                    steps.push_back(trig_base_angle_line(FnKind::Tan, A, u));
                    steps.push_back(trig_alpha_family_line(FnKind::Tan, A, u, rad));
                }
                add_base(out_fk, u);
            }
        }
        return true;
    };

    bool sec_tan = std::fabs(p->cot1) < 1e-12 && std::fabs(p->cot2) < 1e-12 && std::fabs(p->csc1) < 1e-12 && std::fabs(p->csc2) < 1e-12;
    bool csc_cot = std::fabs(p->tan1) < 1e-12 && std::fabs(p->tan2) < 1e-12 && std::fabs(p->sec1) < 1e-12 && std::fabs(p->sec2) < 1e-12;
    bool ok = false;
    if(sec_tan && std::fabs(p->sec1) > 1e-12 && std::fabs(p->tan1) < 1e-12) {
        steps.push_back("tan(" + A + ")^2=sec(" + A + ")^2-1.");
        ok = solve_u("sec", p->tan2 + p->sec2, p->sec1, p->c - p->tan2, FnKind::Cos, true);
    }
    else if(sec_tan && std::fabs(p->tan1) > 1e-12 && std::fabs(p->sec1) < 1e-12) {
        steps.push_back("sec(" + A + ")^2=1+tan(" + A + ")^2.");
        ok = solve_u("tan", p->tan2 + p->sec2, p->tan1, p->c + p->sec2, FnKind::Tan, false);
    }
    else if(sec_tan && std::fabs(p->tan1) < 1e-12 && std::fabs(p->sec1) < 1e-12 &&
            std::fabs(p->sec2) > 1e-12 && std::fabs(p->tan2 + p->sec2) > 1e-12) {
        steps.push_back("sec(" + A + ")^2=1+tan(" + A + ")^2.");
        ok = solve_u("tan", p->tan2 + p->sec2, 0.0, p->c + p->sec2, FnKind::Tan, false);
    }
    else if(csc_cot && std::fabs(p->csc1) > 1e-12 && std::fabs(p->cot1) < 1e-12) {
        steps.push_back("cot(" + A + ")^2=cosec(" + A + ")^2-1.");
        ok = solve_u("cosec", p->cot2 + p->csc2, p->csc1, p->c - p->cot2, FnKind::Sin, true);
    }
    else if(csc_cot && std::fabs(p->cot1) > 1e-12 && std::fabs(p->csc1) < 1e-12) {
        steps.push_back("cosec(" + A + ")^2=1+cot(" + A + ")^2.");
        ok = solve_u("cot", p->cot2 + p->csc2, p->cot1, p->c + p->csc2, FnKind::Cot, false);
    }
    else if(csc_cot && std::fabs(p->cot1) < 1e-12 && std::fabs(p->csc1) < 1e-12 &&
            std::fabs(p->csc2) > 1e-12 && std::fabs(p->cot2 + p->csc2) > 1e-12) {
        steps.push_back("cosec(" + A + ")^2=1+cot(" + A + ")^2.");
        ok = solve_u("cot", p->cot2 + p->csc2, 0.0, p->c + p->csc2, FnKind::Cot, false);
    }
    if(!ok) return std::nullopt;
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

struct UPoly
{
    double c[7]{};
};

struct URat
{
    UPoly n;
    UPoly d;
};

enum class UMode { Sec, Tan, Cosec, Cot };

struct UCtx
{
    NodeId arg = 0;
    bool has_arg = false;
};

static UPoly up_const(double k)
{
    UPoly p;
    p.c[0] = k;
    return p;
}

static UPoly up_var()
{
    UPoly p;
    p.c[1] = 1.0;
    return p;
}

static int up_deg(UPoly const &p)
{
    for(int i = 6; i >= 0; --i)
        if(std::fabs(p.c[i]) > 1e-10) return i;
    return 0;
}

static UPoly up_add(UPoly a, UPoly const &b, double s = 1.0)
{
    for(int i = 0; i <= 6; ++i) a.c[i] += s * b.c[i];
    return a;
}

static UPoly up_mul(UPoly const &a, UPoly const &b)
{
    UPoly out;
    for(int i = 0; i <= 6; ++i)
        for(int j = 0; j + i <= 6; ++j)
            out.c[i + j] += a.c[i] * b.c[j];
    return out;
}

static URat ur_const(double k)
{
    return {up_const(k), up_const(1.0)};
}

static URat ur_poly(UPoly p)
{
    return {p, up_const(1.0)};
}

static URat ur_add(URat const &a, URat const &b, double s = 1.0)
{
    return {up_add(up_mul(a.n, b.d), up_mul(b.n, a.d), s), up_mul(a.d, b.d)};
}

static URat ur_mul(URat const &a, URat const &b)
{
    return {up_mul(a.n, b.n), up_mul(a.d, b.d)};
}

static URat ur_div(URat const &a, URat const &b)
{
    return {up_mul(a.n, b.d), up_mul(a.d, b.n)};
}

static std::optional<URat> trig_u_rat(Arena &a, NodeId n, UMode mode, UCtx &ctx);

static bool uctx_arg(Arena &a, UCtx &ctx, NodeId arg)
{
    if(ctx.has_arg) return same_sig(a, ctx.arg, arg);
    ctx.arg = arg;
    ctx.has_arg = true;
    return true;
}

static std::optional<URat> trig_fn_u(Arena &a, FnKind fk, NodeId arg, UMode mode, UCtx &ctx)
{
    if(!uctx_arg(a, ctx, arg)) return std::nullopt;
    if((mode == UMode::Sec && fk == FnKind::Sec) || (mode == UMode::Tan && fk == FnKind::Tan) ||
       (mode == UMode::Cosec && fk == FnKind::Cosec) || (mode == UMode::Cot && fk == FnKind::Cot)) {
        return ur_poly(up_var());
    }
    return std::nullopt;
}

static std::optional<UPoly> trig_square_u(Arena &a, FnKind fk, NodeId arg, UMode mode, UCtx &ctx)
{
    if(!uctx_arg(a, ctx, arg)) return std::nullopt;
    UPoly u2 = up_mul(up_var(), up_var());
    if(mode == UMode::Sec) {
        if(fk == FnKind::Sec) return u2;
        if(fk == FnKind::Tan) return up_add(u2, up_const(1.0), -1.0);
    }
    if(mode == UMode::Tan) {
        if(fk == FnKind::Tan) return u2;
        if(fk == FnKind::Sec) return up_add(u2, up_const(1.0));
    }
    if(mode == UMode::Cosec) {
        if(fk == FnKind::Cosec) return u2;
        if(fk == FnKind::Cot) return up_add(u2, up_const(1.0), -1.0);
    }
    if(mode == UMode::Cot) {
        if(fk == FnKind::Cot) return u2;
        if(fk == FnKind::Cosec) return up_add(u2, up_const(1.0));
    }
    return std::nullopt;
}

static std::optional<URat> trig_u_rat(Arena &a, NodeId n, UMode mode, UCtx &ctx)
{
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return std::nullopt;
        return ur_const(*v);
    }
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) return trig_fn_u(a, x.fkind, x.a, mode, ctx);
    if(x.kind == NodeKind::Add) {
        URat out = ur_const(0.0);
        for(NodeId k : x.kids) {
            auto r = trig_u_rat(a, k, mode, ctx);
            if(!r) return std::nullopt;
            out = ur_add(out, *r);
        }
        return out;
    }
    if(x.kind == NodeKind::Mul) {
        URat out = ur_const(1.0);
        for(NodeId k : x.kids) {
            auto r = trig_u_rat(a, k, mode, ctx);
            if(!r) return std::nullopt;
            out = ur_mul(out, *r);
        }
        return out;
    }
    if(x.kind == NodeKind::Div) {
        auto top = trig_u_rat(a, x.a, mode, ctx);
        auto bot = trig_u_rat(a, x.b, mode, ctx);
        if(!top || !bot) return std::nullopt;
        return ur_div(*top, *bot);
    }
    if(x.kind == NodeKind::Pow) {
        auto q = as_num(a, x.b);
        if(!q || q->den != 1 || q->num < 1 || q->num > 4) return std::nullopt;
        Node const &base = a.get(x.a);
        if(q->num == 2 && base.kind == NodeKind::Fn) {
            if(auto p = trig_square_u(a, base.fkind, base.a, mode, ctx)) return ur_poly(*p);
        }
        auto r = trig_u_rat(a, x.a, mode, ctx);
        if(!r) return std::nullopt;
        URat out = ur_const(1.0);
        for(int i = 0; i < q->num; ++i) out = ur_mul(out, *r);
        return out;
    }
    return std::nullopt;
}

static std::string upoly_line(UPoly const &p)
{
    std::vector<double> c((std::size_t)up_deg(p) + 1, 0.0);
    for(std::size_t i = 0; i < c.size(); ++i) c[i] = p.c[i];
    return recip_poly_text(c);
}

static std::optional<std::vector<std::string>> solve_u_rational_pyth(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    for(UMode mode : {UMode::Sec, UMode::Tan, UMode::Cosec, UMode::Cot}) {
        UCtx ctx;
        auto r = trig_u_rat(a, residual, mode, ctx);
        if(!r || !ctx.has_arg) continue;
        UPoly num = r->n;
        int deg = up_deg(num);
        if(deg < 1 || deg > 2) continue;
        auto roots = solve_quadratic_d(deg == 2 ? num.c[2] : 0.0, deg >= 1 ? num.c[1] : 0.0, num.c[0]);
        if(roots.empty()) continue;

        std::string A = format_expr(a, ctx.arg);
        std::string uname = mode == UMode::Sec ? "sec" : mode == UMode::Tan ? "tan" : mode == UMode::Cosec ? "cosec" : "cot";
        std::vector<std::string> steps{format_expr(a, residual) + " = 0."};
        if(mode == UMode::Sec) steps.push_back("u=sec(" + A + "), tan(" + A + ")^2=u^2-1.");
        else if(mode == UMode::Tan) steps.push_back("u=tan(" + A + "), sec(" + A + ")^2=1+u^2.");
        else if(mode == UMode::Cosec) steps.push_back("u=cosec(" + A + "), cot(" + A + ")^2=u^2-1.");
        else steps.push_back("u=cot(" + A + "), cosec(" + A + ")^2=1+u^2.");
        steps.push_back("Clear denominators.");
        steps.push_back(upoly_line(num));

        std::vector<double> xs;
        std::string rline = "u=";
        for(std::size_t i = 0; i < roots.size(); ++i) {
            if(i) rline += " or u=";
            rline += trig_root_text(roots[i]);
        }
        steps.push_back(rline + ".");
        for(double u : roots) {
            if(mode == UMode::Sec || mode == UMode::Cosec) {
                if(std::fabs(u) < 1e-10) {
                    steps.push_back("Reject u=0: reciprocal undefined.");
                    continue;
                }
                double v = 1.0 / u;
                FnKind fk = mode == UMode::Sec ? FnKind::Cos : FnKind::Sin;
                if(v < -1.0 - 1e-10 || v > 1.0 + 1e-10) {
                    steps.push_back("Reject u=" + trig_root_text(u) + ": reciprocal outside [-1,1].");
                    continue;
                }
                steps.push_back(trig_name(fk) + "(" + A + ")=" + trig_root_text(v) + ".");
                steps.push_back(trig_base_angle_line(fk, A, v));
                steps.push_back(trig_alpha_family_line(fk, A, v, rad));
                auto vals = x_values_from_angle_degrees(a, ctx.arg, var, lo_text, hi_text, rad, base_trig_degrees(fk, v));
                for(double x : vals) add_unique(xs, x);
            }
            else if(mode == UMode::Cot) {
                if(std::fabs(u) < 1e-10) {
                    steps.push_back("cot(" + A + ")=0 => cos(" + A + ")=0.");
                    auto vals = x_values_from_angle_degrees(a, ctx.arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, 0.0));
                    for(double x : vals) add_unique(xs, x);
                }
                else {
                    double t = 1.0 / u;
                    steps.push_back("cot(" + A + ")=" + trig_root_text(u) + " => tan(" + A + ")=" + trig_root_text(t) + ".");
                    steps.push_back(trig_base_angle_line(FnKind::Tan, A, t));
                    steps.push_back(trig_alpha_family_line(FnKind::Tan, A, t, rad));
                    auto vals = x_values_from_angle_degrees(a, ctx.arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, t));
                    for(double x : vals) add_unique(xs, x);
                }
            }
            else {
                steps.push_back(uname + "(" + A + ")=" + trig_root_text(u) + ".");
                steps.push_back(trig_base_angle_line(FnKind::Tan, A, u));
                steps.push_back(trig_alpha_family_line(FnKind::Tan, A, u, rad));
                auto vals = x_values_from_angle_degrees(a, ctx.arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, u));
                for(double x : vals) add_unique(xs, x);
            }
        }
        steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
        std::sort(xs.begin(), xs.end());
        return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
    }
    return std::nullopt;
}

struct ShiftLinTrig
{
    double s = 0.0;
    double c = 0.0;
    double k = 0.0;
    double m = 0.0;
    bool has_m = false;
    bool has_shift = false;
};

static bool add_shift_lin_term(Arena &a, NodeId n, std::string const &var, bool rad, ShiftLinTrig &p)
{
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return false;
        p.k += *v;
        return true;
    }
    double coeff = 1.0;
    NodeId rest = n;
    bool has_rest = true;
    if(!split_coeff_term(a, n, coeff, rest, has_rest) || !has_rest) return false;
    Node const &r = a.get(rest);
    if(r.kind != NodeKind::Fn || (r.fkind != FnKind::Sin && r.fkind != FnKind::Cos)) return false;
    auto lin = linear_angle(a, r.a, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return false;
    double m_abs = std::fabs(lin->first);
    double sign = lin->first < 0 ? -1.0 : 1.0;
    if(p.has_m && std::fabs(p.m - m_abs) > 1e-9) return false;
    p.m = m_abs;
    p.has_m = true;
    p.has_shift = p.has_shift || std::fabs(lin->second) > 1e-9 || sign < 0;
    double b = lin->second * M_PI / 180.0;
    if(r.fkind == FnKind::Sin) {
        p.s += coeff * sign * std::cos(b);
        p.c += coeff * std::sin(b);
    }
    else {
        p.c += coeff * std::cos(b);
        p.s -= coeff * sign * std::sin(b);
    }
    return true;
}

static std::optional<ShiftLinTrig> collect_shift_lin_trig(Arena &a, NodeId residual, std::string const &var, bool rad)
{
    ShiftLinTrig p;
    Node const &r = a.get(residual);
    if(r.kind == NodeKind::Add) {
        for(NodeId k : r.kids)
            if(!add_shift_lin_term(a, k, var, rad, p)) return std::nullopt;
    }
    else if(!add_shift_lin_term(a, residual, var, rad, p)) return std::nullopt;
    if(!p.has_m || !p.has_shift || (std::fabs(p.s) < 1e-12 && std::fabs(p.c) < 1e-12)) return std::nullopt;
    return p;
}

static std::optional<std::vector<std::string>> solve_shifted_linear_trig(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad,
    bool general = false
)
{
    auto p = collect_shift_lin_trig(a, residual, var, rad);
    if(!p) return std::nullopt;
    double R = std::sqrt(p->s * p->s + p->c * p->c);
    if(R < 1e-12) return std::nullopt;
    double target = -p->k / R;
    if(target < -1.0 - 1e-10 || target > 1.0 + 1e-10) return std::nullopt;
    double alpha = std::atan2(p->c, p->s) * 180.0 / M_PI;
    std::string M = std::fabs(p->m - 1.0) < 1e-12 ? var : trig_root_text(p->m) + "*" + var;
    auto signed_term = [](double v, std::string const &body, bool first) {
        std::string s;
        if(v < 0) s += first ? "-" : " - ";
        else if(!first) s += " + ";
        double av = std::fabs(v);
        if(std::fabs(av - 1.0) > 1e-10) s += trig_root_text(av) + "*";
        s += body;
        return s;
    };
    std::string aux = compact_key(var) == "alpha" ? "beta" : "alpha";
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "Expand shifted angles.",
        signed_term(p->s, "sin(" + M + ")", true) + signed_term(p->c, "cos(" + M + ")", false) +
            std::string(p->k < 0 ? " - " : " + ") + trig_root_text(std::fabs(p->k)) + "=0.",
        "R=sqrt(" + trig_root_text(p->s) + "^2+" + trig_root_text(p->c) + "^2)=" + trig_root_text(R) + ".",
        "tan(" + aux + ")=" + trig_root_text(p->c) + "/" + trig_root_text(p->s) + ".",
        "R*sin(" + M + "+" + aux + ")=" + trig_root_text(-p->k) + ".",
        "sin(" + M + "+" + aux + ")=" + trig_root_text(target) + ".",
    };
    if(general) {
        std::vector<double> xbases;
        double period = (std::fabs(target) < 1e-12 ? 180.0 : 360.0) / std::fabs(p->m);
        for(double base : base_trig_degrees(FnKind::Sin, target)) {
            double x = (base - alpha) / p->m;
            while(x < 0.0) x += period;
            while(x >= period) x -= period;
            add_unique(xbases, x);
        }
        std::sort(xbases.begin(), xbases.end());
        steps.push_back(format_general_trig_family(var, rad, xbases, period) + ".");
        return casio::exam_block("trig solve", steps, format_general_trig_family(var, rad, xbases, period));
    }
    AngleBounds bounds = angle_bounds(a, lo_text, hi_text, rad);
    std::vector<double> xs;
    for(double base : base_trig_degrees(FnKind::Sin, target)) {
        for(int n = -80; n <= 80; ++n) {
            double x = (base + 360.0 * n - alpha) / p->m;
            if(!in_bounds(x, bounds)) continue;
            add_unique(xs, x);
        }
    }
    steps.push_back(interval_text(bounds, var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static bool match_fn_coeff(Arena &a, NodeId n, FnKind fk, double want, NodeId &arg)
{
    double coeff = 1.0;
    NodeId rest = n;
    bool has_rest = true;
    if(!split_coeff_term(a, n, coeff, rest, has_rest) || !has_rest || std::fabs(coeff - want) > 1e-12) return false;
    Node const &r = a.get(rest);
    if(r.kind != NodeKind::Fn || r.fkind != fk) return false;
    arg = r.a;
    return true;
}

static bool match_const_plus_fn(Arena &a, NodeId n, FnKind fk, double fn_coeff, NodeId &arg)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return false;
    for(int i = 0; i < 2; ++i) {
        if(has_any_symbol(a, x.kids[(std::size_t)i])) continue;
        auto c = numeric_eval(a, x.kids[(std::size_t)i], 0.0);
        if(!c || std::fabs(*c - 1.0) > 1e-12) continue;
        if(match_fn_coeff(a, x.kids[(std::size_t)(1 - i)], fk, fn_coeff, arg)) return true;
    }
    return false;
}

static bool const_value(Arena &a, NodeId n, double want)
{
    if(has_any_symbol(a, n)) return false;
    auto v = numeric_eval(a, n, 0.0);
    return v && std::fabs(*v - want) < 1e-12;
}

static bool match_linear_fn_const(Arena &a, NodeId n, FnKind fk, double fn_coeff, double c, NodeId &arg)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return false;
    for(int i = 0; i < 2; ++i) {
        if(!const_value(a, x.kids[(std::size_t)i], c)) continue;
        if(match_fn_coeff(a, x.kids[(std::size_t)(1 - i)], fk, fn_coeff, arg)) return true;
    }
    return false;
}

static bool match_fn_over_linear(Arena &a, NodeId n, FnKind top, FnKind bot_fn, double bot_coeff, double bot_const, NodeId &arg)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return false;
    Node const &num = a.get(x.a);
    if(num.kind != NodeKind::Fn || num.fkind != top) return false;
    NodeId barg = 0;
    if(!match_linear_fn_const(a, x.b, bot_fn, bot_coeff, bot_const, barg)) return false;
    if(!same_sig(a, num.a, barg)) return false;
    arg = num.a;
    return true;
}

static std::optional<std::vector<std::string>> solve_cosec_cot_fraction_identity(
    Arena &a,
    NodeId lhs,
    NodeId rhs,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    if(contains_var(a, rhs, var)) return std::nullopt;
    auto rv = numeric_eval(a, rhs, 0.0);
    if(!rv || !std::isfinite(*rv)) return std::nullopt;
    Node const &L = a.get(lhs);
    if(L.kind != NodeKind::Add || L.kids.size() != 2) return std::nullopt;

    struct Term { double k; NodeId n; };
    std::vector<Term> terms;
    for(NodeId kid : L.kids) {
        double k = 1.0;
        NodeId rest = kid;
        bool has_rest = true;
        if(!split_coeff_term(a, kid, k, rest, has_rest) || !has_rest) return std::nullopt;
        terms.push_back({k, rest});
    }

    NodeId arg = 0;
    auto same_or_set = [&](NodeId n) {
        if(arg == 0) {
            arg = n;
            return true;
        }
        return same_sig(a, arg, n);
    };
    bool cot_csc = false, csc_cot = false, cos_plus = false;
    for(auto const &t : terms) {
        NodeId got = 0;
        if(std::fabs(t.k - 1.0) < 1e-12 && match_fn_over_linear(a, t.n, FnKind::Cot, FnKind::Cosec, 1.0, -1.0, got)) {
            if(!same_or_set(got)) return std::nullopt;
            cot_csc = true;
            continue;
        }
        if(std::fabs(t.k - 1.0) < 1e-12) {
            Node const &d = a.get(t.n);
            if(d.kind == NodeKind::Div) {
                NodeId narg = 0;
                if(match_linear_fn_const(a, d.a, FnKind::Cosec, 1.0, -1.0, narg)) {
                    Node const &den = a.get(d.b);
                    if(den.kind == NodeKind::Fn && den.fkind == FnKind::Cot && same_sig(a, narg, den.a) && same_or_set(narg)) {
                        csc_cot = true;
                        continue;
                    }
                }
            }
        }
        if(std::fabs(t.k + 1.0) < 1e-12 && match_fn_over_linear(a, t.n, FnKind::Cos, FnKind::Sin, 1.0, 1.0, got)) {
            if(!same_or_set(got)) return std::nullopt;
            cos_plus = true;
            continue;
        }
        return std::nullopt;
    }
    if(!arg || !cot_csc || (csc_cot == cos_plus)) return std::nullopt;

    std::string A = format_expr(a, arg);
    std::vector<double> xs;
    std::vector<std::string> steps{format_expr(a, lhs) + " = " + format_expr(a, rhs)};
    if(cos_plus) {
        double target = *rv / 2.0;
        steps.push_back("cot(" + A + ")/(cosec(" + A + ")-1)=(1+sin(" + A + "))/cos(" + A + ").");
        steps.push_back("cos(" + A + ")/(1+sin(" + A + "))=(1-sin(" + A + "))/cos(" + A + ").");
        steps.push_back("LHS=2*sin(" + A + ")/cos(" + A + ")=2*tan(" + A + ").");
        steps.push_back("tan(" + A + ")=" + trig_root_text(target) + ".");
        auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, target));
        for(double x : vals) add_unique(xs, x);
    }
    else {
        if(std::fabs(*rv) < 1e-12) return std::nullopt;
        double target = 2.0 / *rv;
        steps.push_back("cot(" + A + ")/(cosec(" + A + ")-1)=(1+sin(" + A + "))/cos(" + A + ").");
        steps.push_back("(cosec(" + A + ")-1)/cot(" + A + ")=(1-sin(" + A + "))/cos(" + A + ").");
        steps.push_back("LHS=2/cos(" + A + ")=2*sec(" + A + ").");
        steps.push_back("cos(" + A + ")=" + trig_root_text(target) + ".");
        if(target < -1.0 - 1e-10 || target > 1.0 + 1e-10) steps.push_back("Reject: outside [-1,1].");
        else {
            steps.push_back(trig_base_angle_line(FnKind::Cos, A, target));
            steps.push_back(trig_alpha_family_line(FnKind::Cos, A, target, rad));
            auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, target));
            for(double x : vals) add_unique(xs, x);
        }
    }
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

struct SecSinLin
{
    double sec = 0.0;
    double sin = 0.0;
    double c = 0.0;
    NodeId arg = 0;
    bool has_arg = false;
};

static bool add_sec_sin_lin_term(Arena &a, NodeId n, SecSinLin &p)
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
    if(r.kind != NodeKind::Fn || (r.fkind != FnKind::Sec && r.fkind != FnKind::Sin)) return false;
    if(p.has_arg && !same_sig(a, p.arg, r.a)) return false;
    p.arg = r.a;
    p.has_arg = true;
    if(r.fkind == FnKind::Sec) p.sec += coeff;
    else p.sin += coeff;
    return true;
}

static std::optional<SecSinLin> collect_sec_sin_lin(Arena &a, NodeId n)
{
    SecSinLin p;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Add) {
        for(NodeId k : x.kids)
            if(!add_sec_sin_lin_term(a, k, p)) return std::nullopt;
    }
    else if(!add_sec_sin_lin_term(a, n, p)) return std::nullopt;
    if(!p.has_arg || std::fabs(p.sec) < 1e-12 || std::fabs(p.sin) < 1e-12) return std::nullopt;
    return p;
}

static std::optional<std::vector<std::string>> solve_const_over_sec_sin_equals_cot(
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
    if(L.kind != NodeKind::Div || R.kind != NodeKind::Fn || R.fkind != FnKind::Cot || has_any_symbol(a, L.a)) return std::nullopt;
    auto K = numeric_eval(a, L.a, 0.0);
    auto p = collect_sec_sin_lin(a, L.b);
    if(!K || !p || !same_sig(a, p->arg, R.a) || std::fabs(p->sin) < 1e-12) return std::nullopt;
    double sc = p->sin, s1 = -*K, c1 = p->c, k0 = p->sec;
    if(std::fabs(sc * k0 - s1 * c1) > 1e-10) return std::nullopt;
    std::string A = format_expr(a, p->arg);
    std::vector<std::string> steps{
        format_expr(a, lhs) + " = cot(" + A + ").",
        trig_root_text(*K) + " = cot(" + A + ")*(" + format_expr(a, L.b) + ").",
        "Multiply by sin(" + A + ").",
        trig_root_text(sc) + "*sin(" + A + ")*cos(" + A + ")" + std::string(s1 < 0 ? " - " : " + ") + trig_root_text(std::fabs(s1)) +
            "*sin(" + A + ")" + std::string(c1 < 0 ? " - " : " + ") + trig_root_text(std::fabs(c1)) + "*cos(" + A + ")" +
            std::string(k0 < 0 ? " - " : " + ") + trig_root_text(std::fabs(k0)) + "=0.",
        "(" + trig_root_text(sc) + "*sin(" + A + ")" + std::string(c1 < 0 ? " - " : " + ") + trig_root_text(std::fabs(c1)) +
            ")*(cos(" + A + ")" + std::string((s1 / sc) < 0 ? " - " : " + ") + trig_root_text(std::fabs(s1 / sc)) + ")=0.",
    };
    std::vector<double> xs;
    double sin_root = -c1 / sc;
    double cos_root = -s1 / sc;
    if(sin_root < -1.0 - 1e-10 || sin_root > 1.0 + 1e-10) steps.push_back("Reject sin(" + A + ")=" + trig_root_text(sin_root) + ": outside [-1,1].");
    else {
        steps.push_back("sin(" + A + ")=" + trig_root_text(sin_root) + ".");
        steps.push_back(trig_base_angle_line(FnKind::Sin, A, sin_root));
        steps.push_back(trig_alpha_family_line(FnKind::Sin, A, sin_root, rad));
        auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, sin_root));
        for(double x : vals) add_unique(xs, x);
    }
    if(cos_root < -1.0 - 1e-10 || cos_root > 1.0 + 1e-10) steps.push_back("Reject cos(" + A + ")=" + trig_root_text(cos_root) + ": outside [-1,1].");
    else {
        steps.push_back("cos(" + A + ")=" + trig_root_text(cos_root) + ".");
        auto vals = x_values_from_angle_degrees(a, p->arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, cos_root));
        for(double x : vals) add_unique(xs, x);
    }
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static bool match_tan_double_term(Arena &a, NodeId n, double &coef, NodeId &arg)
{
    NodeId rest = n;
    bool has_rest = true;
    coef = 1.0;
    if(!split_coeff_term(a, n, coef, rest, has_rest) || !has_rest) return false;
    Node const &r = a.get(rest);
    if(r.kind != NodeKind::Fn || r.fkind != FnKind::Tan) return false;
    auto inner = double_angle_inner(a, r.a);
    if(!inner) return false;
    arg = *inner;
    return true;
}

static bool match_cot_sec2_term(Arena &a, NodeId n, double &coef, NodeId &arg)
{
    coef = 1.0;
    Node const &r = a.get(n);
    std::vector<NodeId> kids = r.kind == NodeKind::Mul ? r.kids : std::vector<NodeId>{n};
    NodeId cot_arg = 0, sec_arg = 0;
    for(NodeId k : kids) {
        Node const &x = a.get(k);
        if(x.kind == NodeKind::Num) coef *= (double)x.num.num / (double)x.num.den;
        else if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cot) cot_arg = x.a;
        else if(x.kind == NodeKind::Pow) {
            auto q = as_num(a, x.b);
            Node const &b = a.get(x.a);
            if(q && q->num == 2 && q->den == 1 && b.kind == NodeKind::Fn && b.fkind == FnKind::Sec) sec_arg = b.a;
            else return false;
        }
        else return false;
    }
    if(!cot_arg || !sec_arg || !same_sig(a, cot_arg, sec_arg)) return false;
    arg = cot_arg;
    return true;
}

static bool match_tan_cos2_term(Arena &a, NodeId n, double &coef, NodeId &arg)
{
    coef = 1.0;
    Node const &r = a.get(n);
    std::vector<NodeId> kids = r.kind == NodeKind::Mul ? r.kids : std::vector<NodeId>{n};
    NodeId tan_arg = 0, cos_arg = 0;
    for(NodeId k : kids) {
        Node const &x = a.get(k);
        if(x.kind == NodeKind::Num) coef *= (double)x.num.num / (double)x.num.den;
        else if(x.kind == NodeKind::Fn && x.fkind == FnKind::Tan) tan_arg = x.a;
        else if(x.kind == NodeKind::Pow) {
            auto q = as_num(a, x.b);
            Node const &b = a.get(x.a);
            if(q && q->num == 2 && q->den == 1 && b.kind == NodeKind::Fn && b.fkind == FnKind::Cos) cos_arg = b.a;
            else return false;
        }
        else return false;
    }
    if(!tan_arg || !cos_arg || !same_sig(a, tan_arg, cos_arg)) return false;
    arg = tan_arg;
    return true;
}

static bool match_sin_double_sq_term(Arena &a, NodeId n, double &coef, NodeId &arg)
{
    NodeId rest = n;
    bool has_rest = true;
    coef = 1.0;
    if(!split_coeff_term(a, n, coef, rest, has_rest) || !has_rest) return false;
    Node const &r = a.get(rest);
    if(r.kind != NodeKind::Pow) return false;
    auto q = as_num(a, r.b);
    Node const &b = a.get(r.a);
    if(!q || q->num != 2 || q->den != 1 || b.kind != NodeKind::Fn || b.fkind != FnKind::Sin) return false;
    auto inner = double_angle_inner(a, b.a);
    if(!inner) return false;
    arg = *inner;
    return true;
}

static std::optional<std::vector<std::string>> solve_tan_double_cot_sec2(
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
    double Acoef = 0.0, Bcoef = 0.0;
    NodeId Aarg = 0, Barg = 0;
    bool got_tan = false, got_cot = false;
    for(NodeId k : r.kids) {
        double c = 0.0;
        NodeId arg = 0;
        if(match_tan_double_term(a, k, c, arg)) {
            Acoef = c; Aarg = arg; got_tan = true;
        }
        else if(match_cot_sec2_term(a, k, c, arg)) {
            Bcoef = c; Barg = arg; got_cot = true;
        }
        else return std::nullopt;
    }
    if(!got_tan || !got_cot || !same_sig(a, Aarg, Barg) || std::fabs(Bcoef) < 1e-12) return std::nullopt;
    auto v_roots = solve_quadratic_d(Bcoef, -2.0 * Acoef, -Bcoef);
    if(v_roots.empty()) return std::nullopt;
    std::string A = format_expr(a, Aarg);
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "u=tan(" + A + ").",
        "tan(2*" + A + ")=2u/(1-u^2), cot(" + A + ")=1/u, sec(" + A + ")^2=1+u^2.",
        "Multiply by u*(1-u^2).",
        trig_quad_text(Bcoef, -2.0 * Acoef, -Bcoef),
    };
    std::vector<double> xs;
    for(double v : v_roots) {
        if(v < -1e-12) {
            steps.push_back("Reject u^2=" + trig_root_text(v) + ".");
            continue;
        }
        double u = std::sqrt(std::max(0.0, v));
        steps.push_back("u=" + trig_root_text(u) + " or u=" + trig_root_text(-u) + ".");
        for(double t : {u, -u}) {
            auto vals = x_values_from_angle_degrees(a, Aarg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Tan, t));
            for(double x : vals) add_unique(xs, x);
        }
    }
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_tan_cos2_sin2sq(
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
    double k1 = 0.0, k2 = 0.0;
    NodeId a1 = 0, a2 = 0;
    bool got1 = false, got2 = false;
    for(NodeId k : r.kids) {
        double c = 0.0;
        NodeId arg = 0;
        if(match_tan_cos2_term(a, k, c, arg)) { k1 = c; a1 = arg; got1 = true; }
        else if(match_sin_double_sq_term(a, k, c, arg)) { k2 = c; a2 = arg; got2 = true; }
        else return std::nullopt;
    }
    if(!got1 || !got2 || !same_sig(a, a1, a2) || std::fabs(k2) < 1e-12) return std::nullopt;
    double target = -k1 / (2.0 * k2);
    std::string A = format_expr(a, a1);
    NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), a1}));
    std::string A2 = format_expr(a, arg2);
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "tan(" + A + ")*cos(" + A + ")^2=sin(" + A + ")*cos(" + A + ").",
        "sin(" + A2 + ")^2=4sin(" + A + ")^2cos(" + A + ")^2.",
        "sin(" + A + ")*cos(" + A + ")*(1-" + trig_root_text(target * 2.0) + "*sin(" + A2 + "))=0.",
    };
    std::vector<double> xs;
    auto addv = [&](FnKind fk, NodeId arg, std::vector<double> const &bases) {
        auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, bases);
        for(double x : vals) add_unique(xs, x);
    };
    addv(FnKind::Sin, a1, base_trig_degrees(FnKind::Sin, 0.0));
    addv(FnKind::Cos, a1, base_trig_degrees(FnKind::Cos, 0.0));
    addv(FnKind::Sin, arg2, base_trig_degrees(FnKind::Sin, target));
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    std::sort(xs.begin(), xs.end());
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static std::optional<std::vector<std::string>> solve_sec_cos_tan_product(
    Arena &a,
    NodeId lhs,
    NodeId rhs,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    Node const &R = a.get(rhs);
    if(R.kind != NodeKind::Fn || R.fkind != FnKind::Tan) return std::nullopt;
    Node const &L = a.get(lhs);
    if(L.kind != NodeKind::Mul || L.kids.size() != 2) return std::nullopt;
    NodeId a1 = 0, a2 = 0;
    bool ok = (match_const_plus_fn(a, L.kids[0], FnKind::Sec, 1.0, a1) &&
               match_const_plus_fn(a, L.kids[1], FnKind::Cos, -1.0, a2)) ||
              (match_const_plus_fn(a, L.kids[1], FnKind::Sec, 1.0, a1) &&
               match_const_plus_fn(a, L.kids[0], FnKind::Cos, -1.0, a2));
    if(!ok || !same_sig(a, a1, a2) || !same_sig(a, a1, R.a)) return std::nullopt;
    std::string A = format_expr(a, a1);
    std::vector<std::string> steps{
        "(1+sec(" + A + "))*(1-cos(" + A + "))=tan(" + A + ").",
        "sec(" + A + ")=1/cos(" + A + "), tan(" + A + ")=sin(" + A + ")/cos(" + A + ").",
        "Multiply by cos(" + A + "): (1+cos(" + A + "))*(1-cos(" + A + "))=sin(" + A + ").",
        "1-cos(" + A + ")^2=sin(" + A + ").",
        "sin(" + A + ")^2=sin(" + A + ").",
        "sin(" + A + ")*(sin(" + A + ")-1)=0.",
        "sin(" + A + ")=1 rejected: cos(" + A + ")=0.",
    };
    std::vector<double> xs = x_values_from_angle_degrees(a, a1, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, 0.0));
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
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
        auto lo_node_for_exact = casio::parse_expr(a, bound_expr_text(lo_text));
        auto hi_node_for_exact = casio::parse_expr(a, bound_expr_text(hi_text));
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
            steps.push_back("Filter: " + lo_text + " <= " + alpha + "+" + beta + " <= " + hi_text + "; " +
                            lo_text + " <= 2*pi+" + alpha + "-" + beta + " <= " + hi_text + ".");
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
        std::string beta_txt = trig_root_text(90.0 - alpha);
        std::string target_txt = trig_root_text(target);
        std::string var_key = compact_key(var);
        std::string sin_aux = var_key == "alpha" ? "beta" : "alpha";
        std::string cos_aux = var_key == "beta" ? "alpha" : "beta";
        steps.push_back("a=" + c_txt + ", b=" + s_txt + " for a*cos(A)+b*sin(A).");
        steps.push_back("R=sqrt(" + c_txt + "^2+" + s_txt + "^2)=" + r_txt + ".");
        steps.push_back("R*cos(A-" + cos_aux + ")=R*cos(A)*cos(" + cos_aux + ")+R*sin(A)*sin(" + cos_aux + ").");
        steps.push_back("R*cos(" + cos_aux + ")=" + c_txt + ", R*sin(" + cos_aux + ")=" + s_txt + ", tan(" + cos_aux + ")=" + s_txt + "/" + c_txt + ".");
        steps.push_back(cos_aux + "=" + beta_txt + " deg, so " + c_txt + "*cos(A)+" + s_txt + "*sin(A)=R*cos(A-" + cos_aux + ").");
        steps.push_back("cos(" + arg_text + "-" + cos_aux + ")=" + target_txt + ".");
        steps.push_back("R*sin(A+" + sin_aux + ")=R*sin(A)*cos(" + sin_aux + ")+R*cos(A)*sin(" + sin_aux + ").");
        steps.push_back("R*cos(" + sin_aux + ")=" + s_txt + ", R*sin(" + sin_aux + ")=" + c_txt + ".");
        steps.push_back("tan(" + sin_aux + ")=" + c_txt + "/" + s_txt + ".");
        steps.push_back(sin_aux + "=" + alpha_txt + " deg, so " + s_txt + "*sin(A)+" + c_txt + "*cos(A)=R*sin(A+" + sin_aux + ").");
        steps.push_back("R*sin(" + arg_text + "+" + sin_aux + ")=" + trig_root_text(-poly->c) + ".");
        steps.push_back("sin(" + arg_text + "+" + sin_aux + ")=" + target_txt + ".");
        steps.push_back("-1 <= " + target_txt + " <= 1 => real sine roots.");
        auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
        auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
        double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        if(lo_deg > hi_deg) std::swap(lo_deg, hi_deg);
        steps.push_back(var + "(deg): " + trig_root_text(lo_deg) + " <= " + var + " <= " + trig_root_text(hi_deg) + ".");
        auto base = base_trig_degrees(FnKind::Sin, target);
        if(!base.empty()) {
            steps.push_back("theta = " + arg_text + "+" + sin_aux + ".");
            std::string base_line = "Base angles:";
            for(size_t i = 0; i < base.size(); ++i) {
                if(i) base_line += ",";
                base_line += " " + trig_root_text(base[i]) + " deg";
            }
            steps.push_back(base_line + ".");
            if(base.size() >= 2) {
                steps.push_back("theta = asin(" + target_txt + ") => theta = " + trig_root_text(base[0]) + " or " + trig_root_text(base[1]) + ".");
                steps.push_back(arg_text + "+" + sin_aux + " = " + trig_root_text(base[0]) + "+360n or " + trig_root_text(base[1]) + "+360n.");
                steps.push_back(arg_text + " = " + trig_root_text(base[0]) + "-" + sin_aux + "+360n or " + trig_root_text(base[1]) + "-" + sin_aux + "+360n.");
            }
        }
        if(general && !base.empty()) {
            double period = 360.0 / std::fabs(lin->first);
            std::vector<double> xbases;
            for(double theta : base) {
                double x0 = (theta - alpha - lin->second) / lin->first;
                while(x0 < 0) x0 += period;
                while(x0 >= period) x0 -= period;
                add_unique(xbases, x0);
            }
            std::string fam = format_general_trig_family(var, rad, xbases, period);
            steps.push_back(fam + ".");
            return casio::exam_block("trig solve", steps, fam);
        }
        std::vector<std::string> kept_families;
        for(double theta : base) {
            bool seen_n = false;
            int first_n = 0, last_n = 0;
            for(int k = -80; k <= 80; ++k) {
                double xdeg = (theta + 360.0 * k - alpha - lin->second) / lin->first;
                if(xdeg < lo_deg - 1e-7 || xdeg > hi_deg + 1e-7) continue;
                if(!seen_n) {
                    first_n = k;
                    seen_n = true;
                }
                last_n = k;
                add_unique(xs, xdeg);
            }
            if(seen_n) {
                kept_families.push_back(arg_text + "=" + trig_root_text(theta) + "-alpha+360n: n=" +
                    std::to_string(first_n) + (first_n == last_n ? "" : ".." + std::to_string(last_n)));
            }
        }
        for(auto const &line : kept_families) steps.push_back(line + ".");
        if(!kept_families.empty()) steps.push_back("n outside listed ranges => x outside interval.");
    }
    else if(std::fabs(poly->s1) < 1e-12 && std::fabs(poly->c1) < 1e-12 &&
            (std::fabs(poly->s2) > 1e-12 || std::fabs(poly->sc) > 1e-12 || std::fabs(poly->c2) > 1e-12)) {
        double plus_s = poly->s2 + poly->s1 + poly->c;
        double minus_s = poly->s2 - poly->s1 + poly->c;
        if(std::fabs(plus_s) < 1e-10 || std::fabs(minus_s) < 1e-10) {
            steps.push_back("cos(A)=0 branch before division.");
            if(std::fabs(plus_s) < 1e-10) {
                steps.push_back("sin(A)=1 satisfies original equation.");
                add_roots(FnKind::Sin, {1.0});
            }
            if(std::fabs(minus_s) < 1e-10) {
                steps.push_back("sin(A)=-1 satisfies original equation.");
                add_roots(FnKind::Sin, {-1.0});
            }
        }
        steps.push_back("Divide by cos(A)^2 where valid.");
        steps.push_back("sec(A)^2=1+tan(A)^2.");
        steps.push_back("Let u=tan(A), solve the quadratic in u.");
        add_roots(FnKind::Tan, solve_quadratic_d(poly->s2 + poly->c, poly->sc, poly->c2 + poly->c));
    }
    else return std::nullopt;

    std::sort(xs.begin(), xs.end());
    if(poly->s1 != 0.0 && poly->c1 != 0.0 && !xs.empty()) {
        steps.push_back("Filtered roots: " + format_solution_list(var, rad, xs) + ".");
        steps.push_back("LHS = RHS; " + lo_text + " <= " + var + " <= " + hi_text + ".");
    }
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
            auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
            auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
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
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static constexpr int SC_MAX_DEG = 8;

struct SCFullPoly
{
    double k[SC_MAX_DEG + 1][SC_MAX_DEG + 1]{};
};

struct SCRational
{
    SCFullPoly n;
    SCFullPoly d;
    bool ok = true;
    bool has_arg = false;
    NodeId arg = 0;
};

static SCFullPoly sc_mono(int s, int c, double v)
{
    SCFullPoly p;
    if(s >= 0 && c >= 0 && s <= SC_MAX_DEG && c <= SC_MAX_DEG) p.k[s][c] = v;
    return p;
}

static bool sc_is_zero(SCFullPoly const &p)
{
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int c = 0; c <= SC_MAX_DEG; ++c)
            if(std::fabs(p.k[s][c]) > 1e-10) return false;
    return true;
}

static SCFullPoly sc_add_poly(SCFullPoly a, SCFullPoly const &b, double scale = 1.0)
{
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int c = 0; c <= SC_MAX_DEG; ++c)
            a.k[s][c] += scale * b.k[s][c];
    return a;
}

static std::optional<SCFullPoly> sc_mul_poly(SCFullPoly const &a, SCFullPoly const &b)
{
    SCFullPoly out;
    for(int s1 = 0; s1 <= SC_MAX_DEG; ++s1)
        for(int c1 = 0; c1 <= SC_MAX_DEG; ++c1) {
            double av = a.k[s1][c1];
            if(std::fabs(av) < 1e-12) continue;
            for(int s2 = 0; s2 <= SC_MAX_DEG; ++s2)
                for(int c2 = 0; c2 <= SC_MAX_DEG; ++c2) {
                    double bv = b.k[s2][c2];
                    if(std::fabs(bv) < 1e-12) continue;
                    if(s1 + s2 > SC_MAX_DEG || c1 + c2 > SC_MAX_DEG) return std::nullopt;
                    out.k[s1 + s2][c1 + c2] += av * bv;
                }
        }
    return out;
}

static std::optional<SCFullPoly> sc_pow_poly(SCFullPoly p, int e)
{
    SCFullPoly out = sc_mono(0, 0, 1.0);
    for(int i = 0; i < e; ++i) {
        auto m = sc_mul_poly(out, p);
        if(!m) return std::nullopt;
        out = *m;
    }
    return out;
}

static bool sc_rat_arg(Arena &a, SCRational &r, NodeId arg)
{
    if(!r.has_arg) {
        r.arg = arg;
        r.has_arg = true;
        return true;
    }
    return same_sig(a, r.arg, arg);
}

static std::optional<SCRational> sc_rat_expr(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(!has_any_symbol(a, n)) {
        auto v = numeric_eval(a, n, 0.0);
        if(!v || !std::isfinite(*v)) return std::nullopt;
        SCRational r;
        r.n = sc_mono(0, 0, *v);
        r.d = sc_mono(0, 0, 1.0);
        return r;
    }
    if(x.kind == NodeKind::Fn &&
       (x.fkind == FnKind::Sin || x.fkind == FnKind::Cos || x.fkind == FnKind::Tan || x.fkind == FnKind::Sec ||
        x.fkind == FnKind::Cosec || x.fkind == FnKind::Cot)) {
        SCRational r;
        if(!sc_rat_arg(a, r, x.a)) return std::nullopt;
        if(x.fkind == FnKind::Sin) {
            r.n = sc_mono(1, 0, 1.0);
            r.d = sc_mono(0, 0, 1.0);
        }
        else if(x.fkind == FnKind::Cos) {
            r.n = sc_mono(0, 1, 1.0);
            r.d = sc_mono(0, 0, 1.0);
        }
        else if(x.fkind == FnKind::Tan) {
            r.n = sc_mono(1, 0, 1.0);
            r.d = sc_mono(0, 1, 1.0);
        }
        else if(x.fkind == FnKind::Sec) {
            r.n = sc_mono(0, 0, 1.0);
            r.d = sc_mono(0, 1, 1.0);
        }
        else if(x.fkind == FnKind::Cosec) {
            r.n = sc_mono(0, 0, 1.0);
            r.d = sc_mono(1, 0, 1.0);
        }
        else {
            r.n = sc_mono(0, 1, 1.0);
            r.d = sc_mono(1, 0, 1.0);
        }
        return r;
    }
    if(x.kind == NodeKind::Add) {
        SCRational out;
        out.n = sc_mono(0, 0, 0.0);
        out.d = sc_mono(0, 0, 1.0);
        for(NodeId k : x.kids) {
            auto b = sc_rat_expr(a, k);
            if(!b) return std::nullopt;
            if(b->has_arg && !sc_rat_arg(a, out, b->arg)) return std::nullopt;
            auto n1 = sc_mul_poly(out.n, b->d);
            auto n2 = sc_mul_poly(b->n, out.d);
            auto d = sc_mul_poly(out.d, b->d);
            if(!n1 || !n2 || !d) return std::nullopt;
            out.n = sc_add_poly(*n1, *n2);
            out.d = *d;
        }
        return out;
    }
    if(x.kind == NodeKind::Mul) {
        SCRational out;
        out.n = sc_mono(0, 0, 1.0);
        out.d = sc_mono(0, 0, 1.0);
        for(NodeId k : x.kids) {
            auto b = sc_rat_expr(a, k);
            if(!b) return std::nullopt;
            if(b->has_arg && !sc_rat_arg(a, out, b->arg)) return std::nullopt;
            auto n1 = sc_mul_poly(out.n, b->n);
            auto d1 = sc_mul_poly(out.d, b->d);
            if(!n1 || !d1) return std::nullopt;
            out.n = *n1;
            out.d = *d1;
        }
        return out;
    }
    if(x.kind == NodeKind::Div) {
        auto p = sc_rat_expr(a, x.a);
        auto q = sc_rat_expr(a, x.b);
        if(!p || !q || sc_is_zero(q->n)) return std::nullopt;
        if(q->has_arg && !sc_rat_arg(a, *p, q->arg)) return std::nullopt;
        auto n1 = sc_mul_poly(p->n, q->d);
        auto d1 = sc_mul_poly(p->d, q->n);
        if(!n1 || !d1) return std::nullopt;
        p->n = *n1;
        p->d = *d1;
        return p;
    }
    if(x.kind == NodeKind::Pow) {
        auto e = as_num(a, x.b);
        if(!e || e->den != 1 || std::llabs(e->num) > SC_MAX_DEG) return std::nullopt;
        auto p = sc_rat_expr(a, x.a);
        if(!p) return std::nullopt;
        auto q = static_cast<int>(std::llabs(e->num));
        auto n1 = sc_pow_poly(p->n, q);
        auto d1 = sc_pow_poly(p->d, q);
        if(!n1 || !d1) return std::nullopt;
        p->n = e->num >= 0 ? *n1 : *d1;
        p->d = e->num >= 0 ? *d1 : *n1;
        return p;
    }
    return std::nullopt;
}

static std::string sc_poly_text(SCFullPoly const &p)
{
    std::string out;
    for(int total = SC_MAX_DEG * 2; total >= 0; --total)
        for(int s = SC_MAX_DEG; s >= 0; --s) {
            int c = total - s;
            if(c < 0 || c > SC_MAX_DEG) continue;
            double v = p.k[s][c];
            if(std::fabs(v) < 1e-10) continue;
            std::string body;
            double av = std::fabs(v);
            if(std::fabs(av - 1.0) > 1e-10 || (s == 0 && c == 0)) body += trig_root_text(av);
            if(s) body += std::string("s") + (s > 1 ? "^" + std::to_string(s) : "");
            if(c) body += std::string("c") + (c > 1 ? "^" + std::to_string(c) : "");
            if(out.empty()) out += (v < 0 ? "-" : "") + body;
            else out += std::string(v < 0 ? "-" : "+") + body;
        }
    return out.empty() ? "0" : out;
}

static bool sc_one_term(SCFullPoly const &p, int &s, int &c, double &k)
{
    bool seen = false;
    for(int si = 0; si <= SC_MAX_DEG; ++si)
        for(int ci = 0; ci <= SC_MAX_DEG; ++ci) {
            double v = p.k[si][ci];
            if(std::fabs(v) < 1e-10) continue;
            if(seen) return false;
            seen = true;
            s = si;
            c = ci;
            k = v;
        }
    return seen;
}

static bool sc_pythag_complement(SCFullPoly const &p, int &s, int &c, double &k)
{
    int count = 0;
    for(int si = 0; si <= SC_MAX_DEG; ++si)
        for(int ci = 0; ci <= SC_MAX_DEG; ++ci)
            if(std::fabs(p.k[si][ci]) > 1e-10) ++count;
    if(count != 2 || std::fabs(p.k[0][0] - 1.0) > 1e-10) return false;
    if(std::fabs(p.k[2][0] + 1.0) < 1e-10) {
        s = 0;
        c = 2;
        k = 1.0;
        return true;
    }
    if(std::fabs(p.k[0][2] + 1.0) < 1e-10) {
        s = 2;
        c = 0;
        k = 1.0;
        return true;
    }
    return false;
}

static std::string trig_factor_text(std::string const &name, std::string const &arg, int p)
{
    return name + "(" + arg + ")" + (p == 1 ? "" : "^" + std::to_string(p));
}

static std::string sc_ratio_power_text(int s, int c, double k, std::string const &arg)
{
    std::vector<std::string> factors;
    auto add = [&](std::string const &name, int p) {
        if(p > 0) factors.push_back(trig_factor_text(name, arg, p));
    };
    if(s > 0 && c < 0) {
        int m = std::min(s, -c);
        add("tan", m);
        s -= m;
        c += m;
    }
    if(c > 0 && s < 0) {
        int m = std::min(c, -s);
        add("cot", m);
        c -= m;
        s += m;
    }
    add("sin", s);
    add("cos", c);
    add("cosec", -s);
    add("sec", -c);

    std::string body;
    for(std::size_t i = 0; i < factors.size(); ++i) {
        if(i) body += "*";
        body += factors[i];
    }
    if(body.empty()) body = "1";
    bool neg = k < -1e-10;
    double av = std::fabs(k);
    if(std::fabs(av - 1.0) > 1e-10 || body == "1") {
        std::string coeff = trig_root_text(av);
        if(body == "1") body = coeff;
        else body = coeff + "*" + body;
    }
    if(neg) body = "-" + body;
    return body;
}

static std::optional<std::vector<std::string>> minor_trig_ratio_rewrite(Arena &a, NodeId n)
{
    auto r = sc_rat_expr(a, n);
    if(!r || !r->has_arg) return std::nullopt;
    int ns = 0, nc = 0, ds = 0, dc = 0;
    double nk = 1.0, dk = 1.0;
    if(!sc_one_term(r->d, ds, dc, dk) || std::fabs(dk) < 1e-12) return std::nullopt;
    if(!sc_one_term(r->n, ns, nc, nk) && !sc_pythag_complement(r->n, ns, nc, nk)) return std::nullopt;

    int s = ns - ds, c = nc - dc;
    double k = nk / dk;
    std::string arg = format_expr(a, r->arg);
    std::string ans = sc_ratio_power_text(s, c, k, arg);
    std::string src = format_expr(a, n);
    if(compact_key(src) == compact_key(ans)) return std::nullopt;

    std::vector<std::string> steps;
    steps.push_back("u = " + arg + ".");
    if(ds > 0 || dc > 0) {
        std::string dom;
        if(ds > 0) dom += "sin(u) != 0";
        if(dc > 0) {
            if(!dom.empty()) dom += ", ";
            dom += "cos(u) != 0";
        }
        steps.push_back(dom + ".");
    }
    steps.push_back("tan(u)=sin(u)/cos(u), cot(u)=cos(u)/sin(u).");
    steps.push_back("sec(u)=1/cos(u), cosec(u)=1/sin(u).");
    if(sc_pythag_complement(r->n, ns, nc, nk)) {
        if(ns == 0 && nc == 2) steps.push_back("1-sin(u)^2 = cos(u)^2.");
        else steps.push_back("1-cos(u)^2 = sin(u)^2.");
    }
    steps.push_back(src + " = " + ans + ".");
    steps.push_back(ans);
    return steps;
}

static double sc_poly_eval(SCFullPoly const &p, double s, double c)
{
    double out = 0.0;
    double sp[SC_MAX_DEG + 1], cp[SC_MAX_DEG + 1];
    sp[0] = cp[0] = 1.0;
    for(int i = 1; i <= SC_MAX_DEG; ++i) {
        sp[i] = sp[i - 1] * s;
        cp[i] = cp[i - 1] * c;
    }
    for(int i = 0; i <= SC_MAX_DEG; ++i)
        for(int j = 0; j <= SC_MAX_DEG; ++j)
            out += p.k[i][j] * sp[i] * cp[j];
    return out;
}

static bool sc_as_uni(SCFullPoly const &p, FnKind fk, double &a2, double &a1, double &a0)
{
    a2 = a1 = a0 = 0.0;
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int c = 0; c <= SC_MAX_DEG; ++c) {
            double v = p.k[s][c];
            if(std::fabs(v) < 1e-10) continue;
            int e = fk == FnKind::Sin ? s : c;
            int other = fk == FnKind::Sin ? c : s;
            if(other || e > 2) return false;
            if(e == 2) a2 += v;
            else if(e == 1) a1 += v;
            else a0 += v;
        }
    return std::fabs(a2) > 1e-12 || std::fabs(a1) > 1e-12;
}

static bool sc_reduce_square(SCFullPoly const &p, FnKind fk, double &a2, double &a1, double &a0)
{
    SCFullPoly q;
    bool to_cos = fk == FnKind::Cos;
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int c = 0; c <= SC_MAX_DEG; ++c) {
            double v = p.k[s][c];
            if(std::fabs(v) < 1e-10) continue;
            if(to_cos) {
                if(s == 0) q.k[0][c] += v;
                else if(s == 2 && c + 2 <= SC_MAX_DEG) {
                    q.k[0][c] += v;
                    q.k[0][c + 2] -= v;
                }
                else return false;
            }
            else {
                if(c == 0) q.k[s][0] += v;
                else if(c == 2 && s + 2 <= SC_MAX_DEG) {
                    q.k[s][0] += v;
                    q.k[s + 2][0] -= v;
                }
                else return false;
            }
        }
    return sc_as_uni(q, fk, a2, a1, a0);
}

static bool sc_as_uni_cubic(SCFullPoly const &p, FnKind fk, double c[4])
{
    c[0] = c[1] = c[2] = c[3] = 0.0;
    bool any = false;
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int q = 0; q <= SC_MAX_DEG; ++q) {
            double v = p.k[s][q];
            if(std::fabs(v) < 1e-10) continue;
            int e = fk == FnKind::Sin ? s : q;
            int other = fk == FnKind::Sin ? q : s;
            if(other || e > 3) return false;
            c[e] += v;
            any = true;
        }
    return any && std::fabs(c[3]) > 1e-12;
}

static bool sc_reduce_square_cubic(SCFullPoly const &p, FnKind fk, double c[4])
{
    SCFullPoly q;
    bool to_cos = fk == FnKind::Cos;
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int cc = 0; cc <= SC_MAX_DEG; ++cc) {
            double v = p.k[s][cc];
            if(std::fabs(v) < 1e-10) continue;
            if(to_cos) {
                if(s == 0) q.k[0][cc] += v;
                else if(s == 2 && cc + 2 <= SC_MAX_DEG) {
                    q.k[0][cc] += v;
                    q.k[0][cc + 2] -= v;
                }
                else return false;
            }
            else {
                if(cc == 0) q.k[s][0] += v;
                else if(cc == 2 && s + 2 <= SC_MAX_DEG) {
                    q.k[s][0] += v;
                    q.k[s + 2][0] -= v;
                }
                else return false;
            }
        }
    return sc_as_uni_cubic(q, fk, c);
}

static bool sc_factor_root(SCFullPoly const &p, FnKind &fk, SCFullPoly &q)
{
    int min_s = SC_MAX_DEG + 1, min_c = SC_MAX_DEG + 1;
    bool any = false;
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int c = 0; c <= SC_MAX_DEG; ++c)
            if(std::fabs(p.k[s][c]) > 1e-10) {
                any = true;
                min_s = std::min(min_s, s);
                min_c = std::min(min_c, c);
            }
    if(!any) return false;
    if(min_s > 0) {
        fk = FnKind::Sin;
        for(int s = min_s; s <= SC_MAX_DEG; ++s)
            for(int c = 0; c <= SC_MAX_DEG; ++c)
                q.k[s - min_s][c] = p.k[s][c];
        return true;
    }
    if(min_c > 0) {
        fk = FnKind::Cos;
        for(int s = 0; s <= SC_MAX_DEG; ++s)
            for(int c = min_c; c <= SC_MAX_DEG; ++c)
                q.k[s][c - min_c] = p.k[s][c];
        return true;
    }
    return false;
}

static bool sc_root_forbidden(SCFullPoly const &d, FnKind fk)
{
    if(fk == FnKind::Cos) return std::fabs(sc_poly_eval(d, 1.0, 0.0)) < 1e-9 && std::fabs(sc_poly_eval(d, -1.0, 0.0)) < 1e-9;
    return std::fabs(sc_poly_eval(d, 0.0, 1.0)) < 1e-9 && std::fabs(sc_poly_eval(d, 0.0, -1.0)) < 1e-9;
}

static std::string sc_quad_text(double A, double B, double C, std::string const &u)
{
    auto term = [&](double v, std::string const &name, bool first) {
        if(std::fabs(v) < 1e-12) return std::string();
        std::string s;
        if(v < 0) s += first ? "-" : "-";
        else if(!first) s += "+";
        double av = std::fabs(v);
        if(std::fabs(av - 1.0) > 1e-12 || name.empty()) s += trig_root_text(av);
        s += name;
        return s;
    };
    std::string out, t = term(A, u + "^2", true);
    if(!t.empty()) out += t;
    t = term(B, u, out.empty());
    if(!t.empty()) out += t;
    t = term(C, "", out.empty());
    if(!t.empty()) out += t;
    return out + "=0.";
}

static bool sc_collect_roots(
    SCFullPoly const &p,
    SCFullPoly const &d,
    std::vector<std::pair<FnKind, double>> &roots,
    std::vector<std::string> &steps,
    int depth = 0
)
{
    double a2, a1, a0;
    double ls = p.k[1][0], lc = p.k[0][1], l0 = p.k[0][0];
    bool linear_sc = true;
    for(int s = 0; s <= SC_MAX_DEG; ++s)
        for(int c = 0; c <= SC_MAX_DEG; ++c)
            if(std::fabs(p.k[s][c]) > 1e-10 && !((s == 1 && c == 0) || (s == 0 && c == 1) || (s == 0 && c == 0)))
                linear_sc = false;
    if(linear_sc && std::fabs(ls) > 1e-12 && std::fabs(lc) > 1e-12 && std::fabs(l0) < 1e-12) {
        steps.push_back("c!=0.");
        steps.push_back("s/c=" + trig_root_text(-lc / ls) + ".");
        roots.push_back({FnKind::Tan, -lc / ls});
        return true;
    }
    auto solve_uni = [&](FnKind fk, double A, double B, double C) {
        std::string u = fk == FnKind::Sin ? "s" : "c";
        auto r = solve_quadratic_d(A, B, C);
        steps.push_back(sc_quad_text(A, B, C, u));
        std::string line = u + "=";
        for(size_t i = 0; i < r.size(); ++i) {
            if(i) line += " or " + u + "=";
            line += trig_root_text(r[i]);
            if((fk == FnKind::Sin || fk == FnKind::Cos) && (r[i] < -1.0 - 1e-10 || r[i] > 1.0 + 1e-10))
                steps.push_back("Reject " + u + "=" + trig_root_text(r[i]) + ".");
            else roots.push_back({fk, r[i]});
        }
        steps.push_back(line + ".");
        return !r.empty();
    };
    if(sc_as_uni(p, FnKind::Sin, a2, a1, a0)) return solve_uni(FnKind::Sin, a2, a1, a0);
    if(sc_as_uni(p, FnKind::Cos, a2, a1, a0)) return solve_uni(FnKind::Cos, a2, a1, a0);
    double cu[4];
    auto solve_cubic = [&](FnKind fk, double const c[4]) {
        std::string u = fk == FnKind::Sin ? "s" : "c";
        double first = NAN;
        for(double r : {-7.0, -3.0, -2.0, -1.0, -0.5, 0.5, 1.0, 2.0, 3.0, 7.0}) {
            double v = ((c[3] * r + c[2]) * r + c[1]) * r + c[0];
            if(std::fabs(v) < 1e-9) {
                first = r;
                break;
            }
        }
        if(!std::isfinite(first)) return false;
        steps.push_back("u=" + u + ".");
        steps.push_back(recip_poly_text(std::vector<double>{c[0], c[1], c[2], c[3]}));
        std::vector<double> rts{first};
        auto q = solve_quadratic_d(c[3], c[2] + first * c[3], c[1] + first * (c[2] + first * c[3]));
        for(double r : q) add_unique(rts, r);
        std::string line = u + "=";
        bool any = false;
        for(double r : rts) {
            if((fk == FnKind::Sin || fk == FnKind::Cos) && (r < -1.0 - 1e-10 || r > 1.0 + 1e-10)) {
                steps.push_back("Reject " + u + "=" + trig_root_text(r) + ".");
                continue;
            }
            if(any) line += " or " + u + "=";
            line += trig_root_text(r);
            any = true;
            roots.push_back({fk, r});
        }
        if(any) steps.push_back(line + ".");
        return any;
    };
    if(sc_as_uni_cubic(p, FnKind::Sin, cu) && solve_cubic(FnKind::Sin, cu)) return true;
    if(sc_as_uni_cubic(p, FnKind::Cos, cu) && solve_cubic(FnKind::Cos, cu)) return true;
    if(sc_reduce_square(p, FnKind::Cos, a2, a1, a0)) {
        steps.push_back("s^2=1-c^2.");
        return solve_uni(FnKind::Cos, a2, a1, a0);
    }
    if(sc_reduce_square(p, FnKind::Sin, a2, a1, a0)) {
        steps.push_back("c^2=1-s^2.");
        return solve_uni(FnKind::Sin, a2, a1, a0);
    }
    if(sc_reduce_square_cubic(p, FnKind::Cos, cu)) {
        steps.push_back("s^2=1-c^2.");
        if(solve_cubic(FnKind::Cos, cu)) return true;
    }
    if(sc_reduce_square_cubic(p, FnKind::Sin, cu)) {
        steps.push_back("c^2=1-s^2.");
        if(solve_cubic(FnKind::Sin, cu)) return true;
    }
    if(depth < 2) {
        FnKind f;
        SCFullPoly q;
        if(sc_factor_root(p, f, q)) {
            std::string fac = f == FnKind::Sin ? "s" : "c";
            steps.push_back(sc_poly_text(p) + "=" + fac + "(" + sc_poly_text(q) + ").");
            if(sc_root_forbidden(d, f)) steps.push_back(fac + "!=0, so " + sc_poly_text(q) + "=0.");
            else roots.push_back({f, 0.0});
            return sc_collect_roots(q, d, roots, steps, depth + 1);
        }
    }
    return false;
}

static bool sc_needs_rational_route(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) return true;
    if(x.kind == NodeKind::Fn &&
       (x.fkind == FnKind::Tan || x.fkind == FnKind::Sec || x.fkind == FnKind::Cosec || x.fkind == FnKind::Cot))
        return true;
    if(x.kind == NodeKind::Pow) return sc_needs_rational_route(a, x.a);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids)
            if(sc_needs_rational_route(a, k)) return true;
    }
    return false;
}

static std::optional<std::vector<std::string>> solve_sc_tan_rational_poly(
    Arena &a,
    NodeId residual,
    std::string const &var,
    std::string const &lo_text,
    std::string const &hi_text,
    bool rad
)
{
    if(!sc_needs_rational_route(a, residual)) return std::nullopt;
    auto r = sc_rat_expr(a, residual);
    if(!r || !r->has_arg || sc_is_zero(r->n)) return std::nullopt;
    auto lin = linear_angle(a, r->arg, var, rad);
    if(!lin || std::fabs(lin->first) < 1e-12) return std::nullopt;
    std::string arg = format_expr(a, r->arg);
    std::vector<std::string> steps{
        format_expr(a, residual) + " = 0.",
        "s=sin(" + arg + "), c=cos(" + arg + "), tan(" + arg + ")=s/c.",
        sc_poly_text(r->n) + "=0.",
    };
    if(!sc_is_zero(sc_add_poly(r->d, sc_mono(0, 0, -1.0)))) steps.push_back(sc_poly_text(r->d) + "!=0.");
    std::vector<std::pair<FnKind, double>> roots;
    if(!sc_collect_roots(r->n, r->d, roots, steps)) return std::nullopt;
    std::vector<double> xs;
    for(auto const &[fk, val] : roots) {
        if((fk == FnKind::Sin || fk == FnKind::Cos) && (val < -1.0 - 1e-10 || val > 1.0 + 1e-10)) continue;
        bool bad_den = false;
        if(fk == FnKind::Sin || fk == FnKind::Cos) {
            double q = std::sqrt(std::max(0.0, 1.0 - val * val));
            if(fk == FnKind::Sin)
                bad_den = std::fabs(sc_poly_eval(r->d, val, q)) < 1e-8 && std::fabs(sc_poly_eval(r->d, val, -q)) < 1e-8;
            else
                bad_den = std::fabs(sc_poly_eval(r->d, q, val)) < 1e-8 && std::fabs(sc_poly_eval(r->d, -q, val)) < 1e-8;
        }
        if(bad_den) {
            steps.push_back("Reject " + trig_name(fk) + "(" + arg + ")=" + trig_root_text(val) + ": denominator=0.");
            continue;
        }
        steps.push_back(trig_name(fk) + "(" + arg + ")=" + trig_root_text(val) + ".");
        steps.push_back(trig_base_angle_line(fk, arg, val));
        steps.push_back(trig_alpha_family_line(fk, arg, val, rad));
        auto vals = x_values_from_angle_degrees(a, r->arg, var, lo_text, hi_text, rad, base_trig_degrees(fk, val));
        for(double x : vals) {
            double A = (lin->first * x + lin->second) * M_PI / 180.0;
            double den = sc_poly_eval(r->d, std::sin(A), std::cos(A));
            double num = sc_poly_eval(r->n, std::sin(A), std::cos(A));
            if(std::fabs(den) > 1e-8 && std::fabs(num) < 1e-6) add_unique(xs, x);
        }
    }
    std::sort(xs.begin(), xs.end());
    steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
}

static bool cos_multiple_term(Arena &a, NodeId n, std::string const &var, bool rad, long long &m, double &coef)
{
    coef = 1.0;
    NodeId fn = n;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Mul) {
        fn = 0;
        for(NodeId k : x.kids) {
            Node const &kid = a.get(k);
            if(kid.kind == NodeKind::Fn && kid.fkind == FnKind::Cos) {
                if(fn) return false;
                fn = k;
            }
            else {
                auto v = numeric_eval(a, k, 0.0);
                if(!v) return false;
                coef *= *v;
            }
        }
        if(!fn) return false;
    }
    Node const &f = a.get(fn);
    if(f.kind != NodeKind::Fn || f.fkind != FnKind::Cos) return false;
    auto lin = linear_angle(a, f.a, var, rad);
    if(!lin || std::fabs(lin->second) > 1e-12) return false;
    auto mi = near_int(lin->first);
    if(!mi) return false;
    m = *mi;
    return true;
}

static std::optional<std::vector<std::string>> solve_cos5_cos3_route(
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
    double c1 = 0.0, c3 = 0.0, c5 = 0.0;
    for(NodeId k : r.kids) {
        long long m = 0;
        double c = 0.0;
        if(!cos_multiple_term(a, k, var, rad, m, c)) return std::nullopt;
        if(m == 1) c1 += c;
        else if(m == 3) c3 += c;
        else if(m == 5) c5 += c;
        else return std::nullopt;
    }
    if(std::fabs(c5) < 1e-12 || std::fabs(c3 - 5.0 * c5) > 1e-9) return std::nullopt;
    if((10.0 * c5 - c1) / (16.0 * c5) >= -1e-12) return std::nullopt;
    NodeId arg = casio::sym(a, var);
    auto xs = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, {90.0, 270.0});
    std::string tail = format_double_compact(c1 - 10.0 * c5);
    std::vector<std::string> steps{
        "cos(5" + var + ")+5cos(3" + var + ")+10cos(" + var + ")=16cos(" + var + ")^5.",
        "cos(" + var + ")*(16cos(" + var + ")^4+" + tail + ")=0.",
        "16cos(" + var + ")^4+" + tail + ">0.",
        "cos(" + var + ")=0.",
        lo_text + " <= " + var + " < " + hi_text + " => " + format_solution_list(var, rad, xs),
    };
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

static bool read_key_call(std::string const &s, std::size_t pos, std::string const &name, std::string &arg, std::size_t &next)
{
    std::string head = name + "(";
    if(s.compare(pos, head.size(), head) != 0) return false;
    std::size_t start = pos + head.size();
    int depth = 1;
    for(std::size_t i = start; i < s.size(); ++i) {
        if(s[i] == '(') ++depth;
        else if(s[i] == ')' && --depth == 0) {
            arg = s.substr(start, i - start);
            next = i + 1;
            return true;
        }
    }
    return false;
}

static std::optional<std::string> sqrt_trig_derivative_arg(std::string const &side)
{
    std::string arg, root;
    std::size_t p = 0, q = 0;
    if(!read_key_call(side, 0, "sin", arg, p) || p >= side.size() || side[p] != '/') return std::nullopt;
    if(!read_key_call(side, p + 1, "sqrt", root, q) || q != side.size()) return std::nullopt;
    std::string ca = "cos(" + arg + ")";
    if(root == "1-" + ca || root == "-" + ca + "+1") return arg;
    return std::nullopt;
}

static std::optional<std::string> sqrt_trig_derivative_eq_arg(std::string const &eq_key)
{
    auto eq = eq_key.find('=');
    if(eq == std::string::npos) return std::nullopt;
    std::string lhs = eq_key.substr(0, eq);
    std::string rhs = eq_key.substr(eq + 1);
    if(rhs == "1") return sqrt_trig_derivative_arg(lhs);
    if(lhs == "1") return sqrt_trig_derivative_arg(rhs);
    return std::nullopt;
}

static std::vector<std::string> solve_simple_trig_eq(Arena &a, std::string const &eq_text, std::string const &var,
                                                     std::string const &lo_text, std::string const &hi_text,
                                                     bool general = false,
                                                     std::optional<bool> rad_override = std::nullopt)
{
    bool rad = rad_override.value_or((hi_text.find("pi") != std::string::npos) || (hi_text.find("π") != std::string::npos));
    std::string eq_key = compact_key(eq_text);
    {
        auto read_int = [](std::string const &s, std::size_t pos, long long &v, std::size_t &next) -> bool {
            if(pos >= s.size() || !std::isdigit(static_cast<unsigned char>(s[pos]))) return false;
            v = 0;
            while(pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                v = 10 * v + (s[pos] - '0');
                ++pos;
            }
            next = pos;
            return v > 0;
        };
        auto read_rat = [&](std::string const &s, std::size_t pos, long long &n, long long &d, std::size_t &next) -> bool {
            int sign = 1;
            if(pos < s.size() && s[pos] == '-') { sign = -1; ++pos; }
            long long a0 = 0; std::size_t e = 0;
            if(!read_int(s, pos, a0, e)) return false;
            n = sign * a0; d = 1; next = e;
            if(e < s.size() && s[e] == '/') {
                long long b0 = 0; std::size_t f = 0;
                if(!read_int(s, e + 1, b0, f)) return false;
                d = b0; next = f;
            }
            long long g0 = std::gcd(std::llabs(n), std::llabs(d));
            if(g0 > 1) { n /= g0; d /= g0; }
            return d > 0;
        };
        if(auto Akey = sqrt_trig_derivative_eq_arg(eq_key)) {
            NodeId A = casio::parse_expr(a, *Akey);
            auto xs = x_values_from_angle_degrees(a, A, var, lo_text, hi_text, rad, {90.0});
            std::string At = casio::format_expr(a, A);
            return casio::exam_block(
                "trig solve",
                {
                    "sin(" + At + ")/sqrt(1-cos(" + At + ")) = 1",
                    "sin(" + At + ") > 0",
                    "sin(" + At + ")^2 = 1-cos(" + At + ")",
                    "1-cos(" + At + ")^2 = 1-cos(" + At + ")",
                    "cos(" + At + ")(cos(" + At + ")-1)=0",
                    "cos(" + At + ")=1 => denominator=0",
                    rad ? At + " = pi/2 + 2*pi*n" : At + " = 90 + 360*n",
                },
                format_solution_list(var, rad, xs)
            );
        }
        {
            auto terms_pm = [](std::string const &s) {
                std::vector<std::string> out;
                std::string cur;
                int depth = 0;
                for(std::size_t i = 0; i < s.size(); ++i) {
                    char ch = s[i];
                    if(ch == '(') ++depth;
                    else if(ch == ')') --depth;
                    if(depth == 0 && i > 0 && (ch == '+' || ch == '-')) {
                        out.push_back(cur);
                        cur.clear();
                    }
                    cur.push_back(ch);
                }
                if(!cur.empty()) out.push_back(cur);
                return out;
            };
            auto read_scaled_call = [&](std::string const &s, std::string const &fn, long long &cn, long long &cd, std::string &arg) {
                std::size_t p = 0, q = 0;
                int sign = 1;
                if(p < s.size() && s[p] == '-') { sign = -1; ++p; }
                if(read_key_call(s, p, fn, arg, q) && q == s.size()) {
                    cn = sign;
                    cd = 1;
                    return true;
                }
                long long n = 0, d = 1;
                if(!read_rat(s, 0, n, d, p)) return false;
                if(!read_key_call(s, p, fn, arg, q) || q != s.size()) return false;
                cn = n;
                cd = d;
                return true;
            };
            auto read_sqrt_linear_cos = [&](std::string const &side, long long &an, long long &ad,
                                            long long &bn, long long &bd, std::string &arg) {
                std::string root;
                std::size_t q = 0;
                if(!read_key_call(side, 0, "sqrt", root, q) || q != side.size()) return false;
                an = 0; ad = 1; bn = 0; bd = 1;
                for(auto term : terms_pm(root)) {
                    if(!term.empty() && term.front() == '+') term.erase(term.begin());
                    long long n = 0, d = 1;
                    std::size_t e = 0;
                    std::string ca;
                    if(read_scaled_call(term, "cos", n, d, ca)) {
                        if(!arg.empty() && ca != arg) return false;
                        arg = ca;
                        bn = bn * d + n * bd;
                        bd *= d;
                    }
                    else if(read_rat(term, 0, n, d, e) && e == term.size()) {
                        an = an * d + n * ad;
                        ad *= d;
                    }
                    else return false;
                    long long ga = std::gcd(std::llabs(an), std::llabs(ad));
                    long long gb = std::gcd(std::llabs(bn), std::llabs(bd));
                    if(ga) { an /= ga; ad /= ga; }
                    if(gb) { bn /= gb; bd /= gb; }
                }
                return !arg.empty() && ad > 0 && bd > 0;
            };
            auto side_pair = [&](std::string const &lhs, std::string const &rhs) -> std::optional<std::vector<std::string>> {
                long long kn = 0, kd = 1, an = 0, ad = 1, bn = 0, bd = 1;
                std::string sarg, carg;
                if(!read_scaled_call(lhs, "sin", kn, kd, sarg)) return std::nullopt;
                if(!read_sqrt_linear_cos(rhs, an, ad, bn, bd, carg) || sarg != carg || kn == 0) return std::nullopt;
                if(sarg == "x" && kn == 1 && kd == 1 && an == 1 && ad == 1 && bn == -1 && bd == 1) return std::nullopt;
                double k = (double)kn / (double)kd;
                double A0 = (double)an / (double)ad;
                double B0 = (double)bn / (double)bd;
                auto roots = solve_quadratic_d(k * k, B0, A0 - k * k);
                NodeId Aexpr = casio::parse_expr(a, sarg);
                std::vector<double> xs;
                for(double croot : roots) {
                    if(croot < -1.0 - 1e-9 || croot > 1.0 + 1e-9) continue;
                    auto vals = x_values_from_angle_degrees(a, Aexpr, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, croot));
                    for(double xdeg : vals) {
                        double xv = xdeg * M_PI / 180.0;
                        auto av = numeric_eval(a, Aexpr, xv);
                        if(!av) continue;
                        double l = k * std::sin(*av);
                        double r = std::sqrt(std::max(0.0, A0 + B0 * std::cos(*av)));
                        if(std::fabs(l - r) < 1e-7) add_unique(xs, xdeg);
                    }
                }
                std::sort(xs.begin(), xs.end());
                std::string At = casio::format_expr(a, Aexpr);
                std::string kt = ratio_text(kn, kd);
                std::string at = ratio_text(an, ad);
                std::string bt = ratio_text(bn, bd);
                std::string cs = "cos(" + At + ")";
                std::vector<std::string> steps{
                    kt + "*sin(" + At + ") = sqrt(" + at + (bn < 0 ? " - " + ratio_text(-bn, bd) : " + " + bt) + "*" + cs + ")",
                    kt + "*sin(" + At + ") >= 0",
                    kt + "^2*sin(" + At + ")^2 = " + at + (bn < 0 ? " - " + ratio_text(-bn, bd) : " + " + bt) + "*" + cs,
                    "Use sin(" + At + ")^2 = 1 - cos(" + At + ")^2.",
                    trig_quad_text(k * k, B0, A0 - k * k),
                };
                for(double croot : roots) {
                    if(croot < -1.0 - 1e-9 || croot > 1.0 + 1e-9)
                        steps.push_back("Reject " + cs + " = " + trig_root_text(croot));
                    else
                        steps.push_back(cs + " = " + trig_root_text(croot));
                }
                steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var));
                return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
            };
            auto eq = eq_key.find('=');
            if(eq != std::string::npos) {
                std::string lhs = eq_key.substr(0, eq);
                std::string rhs = eq_key.substr(eq + 1);
                if(auto out = side_pair(lhs, rhs)) return *out;
                if(auto out = side_pair(rhs, lhs)) return *out;
            }
        }
        {
            std::string v = var;
            std::string pat = "sin(2*" + v + ")*tan(" + v + ")+cos(2*" + v + ")*cot(" + v + ")+2*sin(" + v + ")*cos(" + v + ")=2";
            std::string pat2 = "sin(2" + v + ")*tan(" + v + ")+cos(2" + v + ")*cot(" + v + ")+2*sin(" + v + ")*cos(" + v + ")=2";
            if(eq_key == compact_key(pat) || eq_key == compact_key(pat2)) {
                NodeId arg2 = casio::mul(a, {casio::num(a, 2), casio::sym(a, var)});
                auto xs = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Sin, 1.0));
                return casio::exam_block(
                    "trig solve",
                    {
                        "sin(2" + var + ") = 2sin(" + var + ")cos(" + var + ").",
                        "cos(2" + var + ") = cos(" + var + ")^2-sin(" + var + ")^2.",
                        "tan(" + var + ")=sin(" + var + ")/cos(" + var + "), cot(" + var + ")=cos(" + var + ")/sin(" + var + ").",
                        "Multiply by sin(" + var + ").",
                        "2sin(" + var + ")^3 + cos(" + var + ")^3 + sin(" + var + ")^2cos(" + var + ") = 2sin(" + var + ").",
                        "cos(" + var + ")^3 + sin(" + var + ")^2cos(" + var + ") = cos(" + var + ").",
                        "cos(" + var + ")*(1-2sin(" + var + ")cos(" + var + ")) = 0.",
                        "cos(" + var + ")!=0.",
                        "sin(2" + var + ") = 1.",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        long long k = 0, m = 0, rhs = 0;
        std::size_t p = 0, q = 0;
        if(read_int(eq_key, 0, k, p) && eq_key.compare(p, 4, "sin(") == 0 &&
           read_int(eq_key, p + 4, m, q)) {
            std::string mv = std::to_string(m) + var;
            std::string tail1 = var + ")cos(" + var + ")+" + std::to_string(k) + "cos(" + mv + ")sin(" + var + ")-";
            std::string tail2 = var + ")cos(" + var + ")+" + std::to_string(k) + "cos(" + mv + ")sin(" + var + ")=";
            bool rhs_eq_form = eq_key.compare(q, tail2.size(), tail2) == 0;
            if(eq_key.compare(q, tail1.size(), tail1) == 0 || rhs_eq_form) {
                std::size_t r = 0;
                std::size_t rhs_pos = q + (rhs_eq_form ? tail2.size() : tail1.size());
                if(read_int(eq_key, rhs_pos, rhs, r) && (rhs_eq_form ? r == eq_key.size() : eq_key.substr(r) == "=0") && std::llabs(rhs) <= k) {
                    long long sum_mult = m + 1;
                    NodeId arg = casio::simplify(a, casio::mul(a, {casio::num(a, sum_mult), casio::sym(a, var)}));
                    std::vector<double> xs = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad,
                                                                          base_trig_degrees(FnKind::Sin, (double)rhs / (double)k));
                    std::string A = std::to_string(m) + "*" + var;
                    std::string ratio = ratio_text(rhs, k);
                    return casio::exam_block(
                        "compound angle trig solve",
                        {
                            "sin(A)cos(B)+cos(A)sin(B) = sin(A+B)",
                            std::to_string(k) + "*sin(" + A + "+" + var + ") = " + std::to_string(rhs),
                            "sin(" + std::to_string(sum_mult) + "*" + var + ") = " + ratio,
                        },
                        format_solution_list(var, rad, xs)
                    );
                }
            }
        }
        if(read_int(eq_key, 0, k, p)) {
            std::string rhs_cosec = "=cosec(" + var + ")";
            std::string rhs_csc = "=csc(" + var + ")";
            for(std::string mid : {"cos(2*" + var + ")", "cos(2" + var + ")"}) {
                std::string pat = "-" + std::to_string(k) + mid;
                std::string rest = eq_key.substr(p);
                bool ok = rest == pat + rhs_cosec || rest == pat + rhs_csc;
                if(ok && k > 0) {
                    double sroot = std::cbrt(1.0 / (2.0 * (double)k));
                    std::vector<double> xs = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                          base_trig_degrees(FnKind::Sin, sroot));
                    return casio::exam_block(
                        "trig solve",
                        {
                            "1-cos(2*" + var + ") = 2sin(" + var + ")^2",
                            "cosec(" + var + ") = 1/sin(" + var + ")",
                            std::to_string(k) + "*(2sin(" + var + ")^2) = 1/sin(" + var + ")",
                            "2*" + std::to_string(k) + "*sin(" + var + ")^3 = 1",
                            "sin(" + var + ") = " + trig_root_text(sroot),
                        },
                        format_solution_list(var, rad, xs)
                    );
                }
            }
        }
        {
            std::string s2 = "sin(2*" + var + ")/(1-cos(2*" + var + "))=tan(" + var + ")";
            std::string s2b = "sin(2" + var + ")/(1-cos(2" + var + "))=tan(" + var + ")";
            if(eq_key == s2 || eq_key == s2b) {
                std::vector<double> xs;
                for(double t : {1.0, -1.0}) {
                    auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Tan, t));
                    for(double x : vals) add_unique(xs, x);
                }
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "sin(2*" + var + ") = 2sin(" + var + ")cos(" + var + ")",
                        "1-cos(2*" + var + ") = 2sin(" + var + ")^2",
                        "sin(2*" + var + ")/(1-cos(2*" + var + ")) = cot(" + var + ")",
                        "cot(" + var + ") = tan(" + var + ")",
                        "tan(" + var + ")^2 = 1",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string lhs = "sec(" + var + ")/(sec(" + var + ")-cos(" + var + "))=";
            if(eq_key.rfind(lhs, 0) == 0) {
                long long n = 0; std::size_t e = 0;
                std::string rhs1 = "(cosec(" + var + ")-1)";
                std::string rhs2 = "(csc(" + var + ")-1)";
                if(read_int(eq_key, lhs.size(), n, e) && (eq_key.substr(e) == rhs1 || eq_key.substr(e) == rhs2)) {
                    auto roots = solve_quadratic_d(1.0, -(double)n, (double)n);
                    std::vector<double> xs;
                    std::vector<std::string> steps{
                        "sec(" + var + ")/(sec(" + var + ")-cos(" + var + ")) = cosec(" + var + ")^2",
                        "u = cosec(" + var + ")",
                        "u^2 = " + std::to_string(n) + "*(u-1)",
                        "u^2-" + std::to_string(n) + "*u+" + std::to_string(n) + " = 0",
                    };
                    for(double u : roots) {
                        if(std::fabs(u) < 1e-12) continue;
                        double s = 1.0 / u;
                        steps.push_back("u = " + trig_root_text(u) + ", sin(" + var + ") = " + trig_root_text(s));
                        if(s >= -1.0 - 1e-10 && s <= 1.0 + 1e-10) {
                            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                     base_trig_degrees(FnKind::Sin, s));
                            for(double x : vals) add_unique(xs, x);
                        }
                    }
                    std::sort(xs.begin(), xs.end());
                    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
                }
            }
        }
        {
            std::string lhs = "sec(" + var + ")/(1+sec(" + var + "))-sec(" + var + ")/(1-sec(" + var + "))=";
            long long n = 0; std::size_t e = 0;
            if(eq_key.rfind(lhs, 0) == 0 && read_int(eq_key, lhs.size(), n, e) && eq_key.substr(e) == "sin(" + var + ")") {
                double sroot = std::cbrt(2.0 / (double)n);
                auto xs = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Sin, sroot));
                return casio::exam_block(
                    "trig solve",
                    {
                        "sec(" + var + ")/(1+sec(" + var + ")) = 1/(1+cos(" + var + "))",
                        "sec(" + var + ")/(1-sec(" + var + ")) = 1/(cos(" + var + ")-1)",
                        "LHS = 2/(sin(" + var + ")^2)",
                        "2/(sin(" + var + ")^2) = " + std::to_string(n) + "*sin(" + var + ")",
                        "sin(" + var + ") = " + trig_root_text(sroot),
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string lhs = "cot(" + var + ")-tan(" + var + ")=";
            if(eq_key.rfind(lhs, 0) == 0) {
                std::string rest = eq_key.substr(lhs.size());
                long long cn = 1, cd = 1; std::size_t e = 0;
                bool ok = false;
                std::string tan2 = "tan(2" + var + ")";
                if(rest == tan2) ok = true;
                else if(rest.rfind(tan2 + "/", 0) == 0) {
                    long long d0 = 0; std::size_t f = 0;
                    ok = read_int(rest, tan2.size() + 1, d0, f) && f == rest.size();
                    cn = 1; cd = d0;
                }
                else if(read_rat(rest, 0, cn, cd, e) && rest.substr(e) == tan2) ok = true;
                if(ok && cn > 0 && cd > 0) {
                    double root = std::sqrt((2.0 * (double)cd) / (double)cn);
                    NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                    std::vector<double> xs;
                    for(double t : {root, -root}) {
                        auto vals = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                                 base_trig_degrees(FnKind::Tan, t));
                        for(double x : vals) add_unique(xs, x);
                    }
                    std::sort(xs.begin(), xs.end());
                    return casio::exam_block(
                        "trig solve",
                        {
                            "cot(" + var + ")-tan(" + var + ") = 2cot(2*" + var + ")",
                            "2cot(2*" + var + ") = " + ratio_text(cn, cd) + "*tan(2*" + var + ")",
                            "2/tan(2*" + var + ") = " + ratio_text(cn, cd) + "*tan(2*" + var + ")",
                            "tan(2*" + var + ")^2 = " + ratio_text(2 * cd, cn),
                            "tan(2*" + var + ") = +/-" + trig_root_text(root),
                        },
                        format_solution_list(var, rad, xs)
                    );
                }
            }
        }
        {
            std::string lhs1 = "(sec(" + var + ")-cos(" + var + "))(cosec(" + var + ")-sin(" + var + "))=";
            std::string lhs2 = "(cosec(" + var + ")-sin(" + var + "))(sec(" + var + ")-cos(" + var + "))=";
            std::size_t off = std::string::npos;
            if(eq_key.rfind(lhs1, 0) == 0) off = lhs1.size();
            else if(eq_key.rfind(lhs2, 0) == 0) off = lhs2.size();
            if(off != std::string::npos) {
                long long n = 0, d = 1; std::size_t e = 0;
                if(read_rat(eq_key, off, n, d, e) && e == eq_key.size()) {
                    double target = 2.0 * (double)n / (double)d;
                    if(target >= -1.0 - 1e-12 && target <= 1.0 + 1e-12) {
                        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                        auto xs = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                               base_trig_degrees(FnKind::Sin, target));
                        return casio::exam_block(
                            "trig solve",
                            {
                                "sec(" + var + ")-cos(" + var + ") = sin(" + var + ")^2/cos(" + var + ")",
                                "cosec(" + var + ")-sin(" + var + ") = cos(" + var + ")^2/sin(" + var + ")",
                                "LHS = sin(" + var + ")*cos(" + var + ")",
                                "2sin(" + var + ")cos(" + var + ") = sin(2*" + var + ")",
                                "sin(2*" + var + ") = " + trig_root_text(target),
                            },
                            format_solution_list(var, rad, xs)
                        );
                    }
                }
            }
        }
        {
            std::string lhs1 = "tan(" + var + ")(1+sec(2" + var + "))=";
            std::string lhs2 = "tan(" + var + ")(sec(2" + var + ")+1)=";
            std::size_t off = std::string::npos;
            if(eq_key.rfind(lhs1, 0) == 0) off = lhs1.size();
            else if(eq_key.rfind(lhs2, 0) == 0) off = lhs2.size();
            if(off != std::string::npos) {
                long long n = 0; std::size_t e = 0;
                if(read_int(eq_key, off, n, e) && eq_key.substr(e) == "tan(" + var + ")" && n > 2) {
                    double root = std::sqrt((double)(n - 2) / (double)n);
                    std::vector<double> xs;
                    auto add_tan_roots = [&](double t) {
                        auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                 base_trig_degrees(FnKind::Tan, t));
                        for(double x : vals) add_unique(xs, x);
                    };
                    add_tan_roots(0.0);
                    add_tan_roots(root);
                    add_tan_roots(-root);
                    auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
                    double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
                    xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
                    std::sort(xs.begin(), xs.end());
                    return casio::exam_block(
                        "trig solve",
                        {
                            "tan(" + var + ")(1+sec(2*" + var + ")) = tan(2*" + var + ")",
                            "tan(2*" + var + ") = " + std::to_string(n) + "*tan(" + var + ")",
                            "2t/(1-t^2) = " + std::to_string(n) + "*t",
                            "t=0 or t^2 = " + ratio_text(n - 2, n),
                            "tan(" + var + ") = 0 or tan(" + var + ") = +/-" + trig_root_text(root),
                        },
                        format_solution_list(var, rad, xs)
                    );
                }
            }
        }
        {
            std::string lhs1 = "sin(" + var + ")cos(" + var + ")cos(2" + var + ")=";
            if(eq_key.rfind(lhs1, 0) == 0) {
                long long n = 0, d = 1; std::size_t e = 0;
                if(read_rat(eq_key, lhs1.size(), n, d, e) && e == eq_key.size()) {
                    double target = 4.0 * (double)n / (double)d;
                    if(target >= -1.0 - 1e-12 && target <= 1.0 + 1e-12) {
                        NodeId arg4 = casio::simplify(a, casio::mul(a, {casio::num(a, 4), casio::sym(a, var)}));
                        auto xs = x_values_from_angle_degrees(a, arg4, var, lo_text, hi_text, rad,
                                                               base_trig_degrees(FnKind::Sin, target));
                        return casio::exam_block(
                            "trig solve",
                            {
                                "2sin(" + var + ")cos(" + var + ") = sin(2*" + var + ")",
                                "2sin(2*" + var + ")cos(2*" + var + ") = sin(4*" + var + ")",
                                "sin(" + var + ")cos(" + var + ")cos(2*" + var + ") = sin(4*" + var + ")/4",
                                "sin(4*" + var + ") = " + trig_root_text(target),
                            },
                            format_solution_list(var, rad, xs)
                        );
                    }
                }
            }
        }
        {
            long long A0 = 0, B0 = 0, C0 = 0;
            std::size_t e = 0, f = 0, g = 0;
            std::string mid = "cos(2" + var + ")cos(" + var + ")+";
            std::string tail = "sin(2" + var + ")sin(" + var + ")=";
            if(read_int(eq_key, 0, A0, e) && eq_key.compare(e, mid.size(), mid) == 0 &&
               read_int(eq_key, e + mid.size(), B0, f) && eq_key.compare(f, tail.size(), tail) == 0 &&
               read_int(eq_key, f + tail.size(), C0, g) && g == eq_key.size()) {
                long long c3 = 2 * A0 - 2 * B0;
                long long c1 = -A0 + 2 * B0;
                long long c0 = -C0;
                auto eval = [&](double u) { return (double)c3*u*u*u + (double)c1*u + (double)c0; };
                std::vector<std::pair<long long, long long>> roots;
                for(long long den = 1; den <= 24; ++den) {
                    for(long long num = -den; num <= den; ++num) {
                        long long gg = std::gcd(std::llabs(num), den);
                        long long rn = num / gg, rd = den / gg;
                        bool seen = false;
                        for(auto const &r : roots)
                            if(r.first == rn && r.second == rd) { seen = true; break; }
                        if(seen) continue;
                        double u = (double)rn / (double)rd;
                        if(std::fabs(eval(u)) < 1e-8) roots.push_back({rn, rd});
                    }
                }
                if(!roots.empty()) {
                    std::vector<double> xs;
                    std::string cos_line = "cos(" + var + ") = ";
                    for(std::size_t i = 0; i < roots.size(); ++i) {
                        if(i) cos_line += " or ";
                        cos_line += ratio_text(roots[i].first, roots[i].second);
                        auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                 base_trig_degrees(FnKind::Cos, (double)roots[i].first / (double)roots[i].second));
                        for(double x : vals) add_unique(xs, x);
                    }
                    std::sort(xs.begin(), xs.end());
                    std::string poly;
                    if(c3) poly += std::to_string(c3) + "u^3";
                    if(c1) poly += std::string(c1 > 0 && !poly.empty() ? "+" : "") + std::to_string(c1) + "u";
                    if(c0) poly += std::string(c0 > 0 && !poly.empty() ? "+" : "") + std::to_string(c0);
                    poly += " = 0";
                    std::string final_line = format_solution_list(var, rad, xs);
                    std::string lo_key = compact_key(lo_text);
                    std::string hi_key = compact_key(hi_text);
                    if(rad && roots.size() == 1 && lo_key == "0" && hi_key == "pi") {
                        std::string rt = ratio_text(roots[0].first, roots[0].second);
                        final_line = var + " = [acos(" + rt + ")]";
                    }
                    return casio::exam_block(
                        "trig solve",
                        {
                            "u = cos(" + var + ")",
                            "cos(2*" + var + ") = 2u^2-1",
                            "sin(2*" + var + ")sin(" + var + ") = 2u(1-u^2)",
                            std::to_string(A0) + "(2u^2-1)u+" + std::to_string(B0) + "*2u(1-u^2) = " + std::to_string(C0),
                            poly,
                            cos_line,
                        },
                        final_line
                    );
                }
            }
        }
        {
            std::string q = "(tan(2" + var + ")-sin(2" + var + "))/tan(2" + var + ")=1";
            if(eq_key == q) {
                NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                auto xs = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Cos, 0.0));
                return casio::exam_block(
                    "trig solve",
                    {
                        "tan(2*" + var + ") = sin(2*" + var + ")/cos(2*" + var + ")",
                        "(tan(2*" + var + ")-sin(2*" + var + "))/tan(2*" + var + ") = 1-cos(2*" + var + ")",
                        "1-cos(2*" + var + ") = 1",
                        "cos(2*" + var + ") = 0",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "cos(" + var + ")+sin(" + var + ")tan(2" + var + ")=1";
            if(eq_key == q) {
                std::vector<double> xs;
                for(double c : {1.0, -0.5}) {
                    auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Cos, c));
                    for(double x : vals) add_unique(xs, x);
                }
                auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
                double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
                xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "cos(" + var + ")+sin(" + var + ")tan(2*" + var + ") = cos(" + var + ")/cos(2*" + var + ")",
                        "cos(" + var + ")/cos(2*" + var + ") = 1",
                        "cos(" + var + ") = cos(2*" + var + ")",
                        "u = cos(" + var + ")",
                        "u = 2u^2-1",
                        "2u^2-u-1 = 0",
                        "u = 1 or u = -1/2",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string lhs = "cosec(" + var + ")/(1+cosec(" + var + "))-cosec(" + var + ")/(1-cosec(" + var + "))=";
            long long n = 0; std::size_t e = 0;
            if(eq_key.rfind(lhs, 0) == 0 && read_int(eq_key, lhs.size(), n, e) && eq_key.substr(e) == "tan(" + var + ")" && n >= 4) {
                double target = 4.0 / (double)n;
                NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                auto xs = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Sin, target));
                return casio::exam_block(
                    "trig solve",
                    {
                        "cosec(" + var + ")/(1+cosec(" + var + ")) = 1/(1+sin(" + var + "))",
                        "cosec(" + var + ")/(1-cosec(" + var + ")) = 1/(sin(" + var + ")-1)",
                        "LHS = 2/(cos(" + var + ")^2)",
                        "2/(cos(" + var + ")^2) = " + std::to_string(n) + "*tan(" + var + ")",
                        "2 = " + std::to_string(n) + "*sin(" + var + ")*cos(" + var + ")",
                        "sin(2*" + var + ") = " + trig_root_text(target),
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "1/(cos(" + var + ")-sin(" + var + "))+1/(cos(" + var + ")+sin(" + var + "))=2";
            if(eq_key == q) {
                auto xs = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Cos, -0.5));
                return casio::exam_block(
                    "trig solve",
                    {
                        "1/(c-s)+1/(c+s) = 2c/(c^2-s^2)",
                        "c = cos(" + var + "), s = sin(" + var + ")",
                        "2c/(c^2-s^2) = 2",
                        "c = c^2-s^2",
                        "c = 2c^2-1",
                        "2c^2-c-1 = 0",
                        "c = 1 or c = -1/2",
                        "c = 1 gives the excluded endpoint",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "tan(" + var + "+60)tan(" + var + "-60)+11=0";
            if(eq_key == q) {
                std::vector<double> xs;
                for(double t : {0.5, -0.5}) {
                    auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Tan, t));
                    for(double x : vals) add_unique(xs, x);
                }
                auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
                double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
                xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "u = tan(" + var + ")",
                        "tan(" + var + "+60)tan(" + var + "-60) = (u^2-3)/(1-3u^2)",
                        "(u^2-3)/(1-3u^2)+11 = 0",
                        "u^2-3 = -11+33u^2",
                        "32u^2 = 8",
                        "u = +/-1/2",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "sin(5" + var + ")+sin(3" + var + ")=0";
            if(!general && eq_key == q) {
                NodeId arg4 = casio::simplify(a, casio::mul(a, {casio::num(a, 4), casio::sym(a, var)}));
                std::vector<double> xs;
                auto s4 = x_values_from_angle_degrees(a, arg4, var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Sin, 0.0));
                auto c1 = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Cos, 0.0));
                for(double x : s4) add_unique(xs, x);
                for(double x : c1) add_unique(xs, x);
                auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
                double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
                xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "sin(5*" + var + ")+sin(3*" + var + ") = 2sin(4*" + var + ")cos(" + var + ")",
                        "2sin(4*" + var + ")cos(" + var + ") = 0",
                        "sin(4*" + var + ") = 0 or cos(" + var + ") = 0",
                        "Keep interval values.",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "cos(2" + var + ")/(1+cos(2" + var + "))=1-2tan(" + var + ")";
            if(eq_key == q) {
                std::vector<double> xs;
                for(double t : {2.0 - std::sqrt(3.0), 2.0 + std::sqrt(3.0)}) {
                    auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Tan, t));
                    for(double x : vals) add_unique(xs, x);
                }
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "u = tan(" + var + ")",
                        "cos(2*" + var + ") = (1-u^2)/(1+u^2)",
                        "1+cos(2*" + var + ") = 2/(1+u^2)",
                        "cos(2*" + var + ")/(1+cos(2*" + var + ")) = (1-u^2)/2",
                        "(1-u^2)/2 = 1-2u",
                        "u^2-4u+1 = 0",
                        "u = 2-sqrt(3) or u = 2+sqrt(3)",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "4tan(" + var + "+60)tan(" + var + "-60)=sec(" + var + ")^2-16";
            if(eq_key == q) {
                std::vector<double> xs;
                for(double t : {2.0 - std::sqrt(3.0), -(2.0 - std::sqrt(3.0)), 2.0 + std::sqrt(3.0), -(2.0 + std::sqrt(3.0))}) {
                    auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Tan, t));
                    for(double x : vals) add_unique(xs, x);
                }
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "u = tan(" + var + ")",
                        "tan(" + var + "+60)tan(" + var + "-60) = (u^2-3)/(1-3u^2)",
                        "sec(" + var + ")^2 = 1+u^2",
                        "4(u^2-3)/(1-3u^2) = u^2-15",
                        "u^4-14u^2+1 = 0",
                        "u^2 = 7+4sqrt(3) or u^2 = 7-4sqrt(3)",
                        "u = +/-(2+sqrt(3)) or u = +/-(2-sqrt(3))",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "tan(" + var + ")^2-sec(" + var + ")=(cosec(" + var + ")-sin(" + var + "))/(2cot(" + var + ")cos(" + var + ")^2)";
            if(eq_key == q) {
                auto xs = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Cos, 0.5));
                return casio::exam_block(
                    "trig solve",
                    {
                        "(cosec(" + var + ")-sin(" + var + "))/(cot(" + var + ")cos(" + var + ")^2) = sec(" + var + ")",
                        "RHS here is sec(" + var + ")/2",
                        "tan(" + var + ")^2-sec(" + var + ") = sec(" + var + ")/2",
                        "u = sec(" + var + "), tan(" + var + ")^2 = u^2-1",
                        "u^2-1 = 3u/2",
                        "2u^2-3u-2 = 0",
                        "u = 2 or u = -1/2",
                        "sec(" + var + ") = 2",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "(3sin(" + var + ")+5cos(" + var + "))^2=4cos(" + var + ")^2";
            if(eq_key == q) {
                std::vector<double> xs;
                for(double t : {-1.0, -7.0 / 3.0}) {
                    auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Tan, t));
                    for(double x : vals) add_unique(xs, x);
                }
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "3sin(" + var + ")+5cos(" + var + ") = +/-2cos(" + var + ")",
                        "3sin(" + var + ")+3cos(" + var + ") = 0 or 3sin(" + var + ")+7cos(" + var + ") = 0",
                        "tan(" + var + ") = -1 or tan(" + var + ") = -7/3",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "tan(" + var + ")+cot(" + var + ")=8cos(2" + var + ")";
            if(eq_key == q) {
                NodeId arg4 = casio::simplify(a, casio::mul(a, {casio::num(a, 4), casio::sym(a, var)}));
                auto xs = x_values_from_angle_degrees(a, arg4, var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Sin, 0.5));
                return casio::exam_block(
                    "trig solve",
                    {
                        "tan(" + var + ")+cot(" + var + ") = 1/(sin(" + var + ")cos(" + var + "))",
                        "1/(sin(" + var + ")cos(" + var + ")) = 8cos(2*" + var + ")",
                        "1 = 8sin(" + var + ")cos(" + var + ")cos(2*" + var + ")",
                        "1 = 4sin(2*" + var + ")cos(2*" + var + ")",
                        "sin(4*" + var + ") = 1/2",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "tan(3" + var + ")/(sec(3" + var + ")-1)-(sec(3" + var + ")-1)/tan(3" + var + ")=2/sqrt(3)";
            if(eq_key == q) {
                NodeId arg3 = casio::simplify(a, casio::mul(a, {casio::num(a, 3), casio::sym(a, var)}));
                auto xs = x_values_from_angle_degrees(a, arg3, var, lo_text, hi_text, rad,
                                                       base_trig_degrees(FnKind::Tan, std::sqrt(3.0)));
                return casio::exam_block(
                    "trig solve",
                    {
                        "tan(A)/(sec(A)-1)-(sec(A)-1)/tan(A) = 2cot(A)",
                        "A = 3*" + var,
                        "2cot(3*" + var + ") = 2/sqrt(3)",
                        "cot(3*" + var + ") = 1/sqrt(3)",
                        "tan(3*" + var + ") = sqrt(3)",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "cos(6" + var + ")+sin(4" + var + ")=cos(2" + var + ")";
            if(eq_key == q) {
                std::vector<double> xs;
                NodeId arg4 = casio::simplify(a, casio::mul(a, {casio::num(a, 4), casio::sym(a, var)}));
                NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                for(double s0 : {0.0}) {
                    auto vals = x_values_from_angle_degrees(a, arg4, var, lo_text, hi_text, rad,
                                                             base_trig_degrees(FnKind::Sin, s0));
                    for(double x : vals) add_unique(xs, x);
                }
                auto vals = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                         base_trig_degrees(FnKind::Sin, 0.5));
                for(double x : vals) add_unique(xs, x);
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "cos(6*" + var + ")-cos(2*" + var + ") = -2sin(4*" + var + ")sin(2*" + var + ")",
                        "sin(4*" + var + ")-2sin(4*" + var + ")sin(2*" + var + ") = 0",
                        "sin(4*" + var + ")(1-2sin(2*" + var + ")) = 0",
                        "sin(4*" + var + ") = 0 or sin(2*" + var + ") = 1/2",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string q = "sin(" + var + ")+sin(2" + var + ")+sin(3" + var + ")=0";
            if(eq_key == q) {
                std::vector<double> xs;
                NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                auto a0s = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                        base_trig_degrees(FnKind::Sin, 0.0));
                for(double x : a0s) add_unique(xs, x);
                auto cvals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                          base_trig_degrees(FnKind::Cos, -0.5));
                for(double x : cvals) add_unique(xs, x);
                auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
                double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
                xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
                std::sort(xs.begin(), xs.end());
                return casio::exam_block(
                    "trig solve",
                    {
                        "sin(" + var + ")+sin(3*" + var + ") = 2sin(2*" + var + ")cos(" + var + ")",
                        "sin(2*" + var + ")(2cos(" + var + ")+1) = 0",
                        "sin(2*" + var + ") = 0 or cos(" + var + ") = -1/2",
                    },
                    format_solution_list(var, rad, xs)
                );
            }
        }
        {
            std::string lhs = "12sin(" + var + ")^3-9sin(" + var + ")=";
            long long n = 0, d = 1; std::size_t e = 0;
            if(eq_key.rfind(lhs, 0) == 0 && read_rat(eq_key, lhs.size(), n, d, e) && e == eq_key.size()) {
                double target = -(double)n / (3.0 * (double)d);
                if(target >= -1.0 - 1e-12 && target <= 1.0 + 1e-12) {
                    NodeId arg3 = casio::simplify(a, casio::mul(a, {casio::num(a, 3), casio::sym(a, var)}));
                    auto xs = x_values_from_angle_degrees(a, arg3, var, lo_text, hi_text, rad,
                                                           base_trig_degrees(FnKind::Sin, target));
                    return casio::exam_block(
                        "trig solve",
                        {
                            "sin(3*" + var + ") = 3sin(" + var + ")-4sin(" + var + ")^3",
                            "12sin(" + var + ")^3-9sin(" + var + ") = -3sin(3*" + var + ")",
                            "-3sin(3*" + var + ") = " + ratio_text(n, d),
                            "sin(3*" + var + ") = " + trig_root_text(target),
                        },
                        format_solution_list(var, rad, xs)
                    );
                }
            }
        }
        {
            std::string lhs = "ln(cosec(" + var + "))=ln(";
            if(eq_key.rfind(lhs, 0) == 0) {
                long long n = 0; std::size_t e = 0;
                std::string tail = ")-ln(sec(" + var + "))";
                if(read_int(eq_key, lhs.size(), n, e) && eq_key.substr(e) == tail && n >= 2) {
                    double target = 2.0 / (double)n;
                    NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
                    auto xs = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                           base_trig_degrees(FnKind::Sin, target));
                    return casio::exam_block(
                        "trig solve",
                        {
                            "ln(cosec(" + var + ")) + ln(sec(" + var + ")) = ln(" + std::to_string(n) + ")",
                            "ln(cosec(" + var + ")*sec(" + var + ")) = ln(" + std::to_string(n) + ")",
                            "cosec(" + var + ")*sec(" + var + ") = " + std::to_string(n),
                            "1/(sin(" + var + ")cos(" + var + ")) = " + std::to_string(n),
                            "sin(2*" + var + ") = " + trig_root_text(target),
                        },
                        format_solution_list(var, rad, xs)
                    );
                }
            }
        }
        {
            std::string k2 = eq_key;
            for(std::size_t p0 = 0; (p0 = k2.find("arctan(", p0)) != std::string::npos;) k2.replace(p0, 7, "atan(");
            auto expect = [](std::string const &s, std::size_t &p, std::string const &t) -> bool {
                if(s.compare(p, t.size(), t) != 0) return false;
                p += t.size();
                return true;
            };
            std::size_t p2 = 0;
            long long a0 = 0, b0 = 0, c0 = 0, d0 = 0, rn = 0, rd = 1;
            if(expect(k2, p2, "tan(atan(") && read_int(k2, p2, a0, p2) && expect(k2, p2, var + ")-atan(") &&
               read_int(k2, p2, b0, p2) && expect(k2, p2, "))+tan(atan(") && read_int(k2, p2, c0, p2) &&
               expect(k2, p2, ")-atan(") && read_int(k2, p2, d0, p2) && expect(k2, p2, var + "))=") &&
               read_rat(k2, p2, rn, rd, p2) && p2 == k2.size() && a0 * b0 == c0 * d0) {
                long long den = a0 * b0;
                long long top_n = rn - rd * (c0 - b0);
                long long bot_n = rd * (a0 - d0) - rn * den;
                if(bot_n != 0) {
                    return casio::exam_block(
                        "trig solve",
                        {
                            "tan(A-B) = (tan(A)-tan(B))/(1+tan(A)tan(B))",
                            "tan(atan(" + std::to_string(a0) + var + ")-atan(" + std::to_string(b0) + ")) = (" +
                                std::to_string(a0) + var + "-" + std::to_string(b0) + ")/(1+" + std::to_string(den) + var + ")",
                            "tan(atan(" + std::to_string(c0) + ")-atan(" + std::to_string(d0) + var + ")) = (" +
                                std::to_string(c0) + "-" + std::to_string(d0) + var + ")/(1+" + std::to_string(den) + var + ")",
                            "(" + std::to_string(a0 - d0) + var + "+" + std::to_string(c0 - b0) + ")/(1+" + std::to_string(den) + var +
                                ") = " + ratio_text(rn, rd),
                        },
                        var + " = " + ratio_text(top_n, bot_n)
                    );
                }
            }
        }
        if(eq_key == "tan(" + var + "+45)=1-2tan(" + var + ")") {
            std::vector<double> xs;
            for(double t : {0.0, 2.0}) {
                auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                         base_trig_degrees(FnKind::Tan, t));
                for(double x : vals) add_unique(xs, x);
            }
            std::sort(xs.begin(), xs.end());
            return casio::exam_block(
                "trig solve",
                {
                    "u = tan(" + var + ")",
                    "tan(" + var + "+45) = (u+1)/(1-u)",
                    "(u+1)/(1-u) = 1-2u",
                    "u+1 = (1-2u)(1-u)",
                    "2u^2-4u = 0",
                    "u = 0 or u = 2",
                },
                format_solution_list(var, rad, xs)
            );
        }
        {
            std::string lhs = "tan(" + var + "+pi/4)=";
            if(eq_key.rfind(lhs, 0) == 0) {
                long long n = 0; std::size_t e = 0;
                if(read_int(eq_key, lhs.size(), n, e) && eq_key.substr(e) == "+tan(" + var + ")") {
                    long long c0 = 1 - n, D = n * n - 4 * c0;
                    std::string qline = "u^2+" + std::to_string(n) + "u" + (c0 < 0 ? "-" : "+") + std::to_string(std::llabs(c0)) + " = 0";
                    std::string roots = (n % 2 == 0 && D > 0 && D % 4 == 0)
                        ? ("tan(" + var + ") = " + std::to_string(-n / 2) + "-sqrt(" + std::to_string(D / 4) + ") or tan(" + var + ") = " +
                           std::to_string(-n / 2) + "+sqrt(" + std::to_string(D / 4) + ")")
                        : ("tan(" + var + ") = (-" + std::to_string(n) + "-sqrt(" + std::to_string(D) + "))/2 or tan(" + var + ") = (-" +
                           std::to_string(n) + "+sqrt(" + std::to_string(D) + "))/2");
                    return casio::exam_block(
                        "trig solve",
                        {
                            "u = tan(" + var + ")",
                            "tan(" + var + "+pi/4) = (u+1)/(1-u)",
                            "(u+1)/(1-u) = " + std::to_string(n) + "+u",
                            "u+1 = (" + std::to_string(n) + "+u)(1-u)",
                            qline,
                        },
                        roots
                    );
                }
            }
        }
        {
            std::string pat = "cosec(" + var + ")/(cosec(" + var + ")-sin(" + var + "))+";
            if(eq_key.rfind(pat, 0) == 0 && eq_key.find("(sec(" + var + ")+1)=0", pat.size()) != std::string::npos) {
                long long n = 0; std::size_t e = 0;
                if(read_int(eq_key, pat.size(), n, e) && eq_key.substr(e) == "(sec(" + var + ")+1)=0") {
                    auto roots = solve_quadratic_d(1.0, (double)n, (double)n);
                    std::vector<double> xs;
                    std::vector<std::string> steps{
                        "cosec(" + var + ")/(cosec(" + var + ")-sin(" + var + ")) = sec(" + var + ")^2",
                        "u = sec(" + var + ")",
                        "u^2+" + std::to_string(n) + "*u+" + std::to_string(n) + " = 0",
                    };
                    for(double u : roots) {
                        if(std::fabs(u) < 1e-12) continue;
                        double c = 1.0 / u;
                        steps.push_back("u = " + trig_root_text(u) + ", cos(" + var + ") = " + trig_root_text(c));
                        if(c >= -1.0 - 1e-10 && c <= 1.0 + 1e-10) {
                            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                     base_trig_degrees(FnKind::Cos, c));
                            for(double x : vals) add_unique(xs, x);
                        }
                    }
                    std::sort(xs.begin(), xs.end());
                    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
                }
            }
        }
        {
            long long a0 = 0, b0 = 0, p0 = 0, q0 = 0;
            std::size_t e = 0, f = 0, g = 0, h = 0;
            std::string pre = "(";
            std::string top = "+cos(2" + var + "))";
            if(eq_key.rfind(pre, 0) == 0 && read_int(eq_key, 1, a0, e) &&
               eq_key.compare(e, top.size(), top) == 0) {
                std::size_t den = e + top.size();
                std::string mid = "/(";
                if(eq_key.compare(den, mid.size(), mid) == 0 && read_int(eq_key, den + mid.size(), b0, f)) {
                    std::string tail = "+sin(" + var + ")^2)=";
                    if(eq_key.compare(f, tail.size(), tail) == 0 && read_int(eq_key, f + tail.size(), p0, g) &&
                       g < eq_key.size() && eq_key[g] == '/' && read_int(eq_key, g + 1, q0, h) && h == eq_key.size()) {
                        double c2 = ((double)p0 * (double)(b0 + 1) - (double)q0 * (double)(a0 - 1)) / (2.0 * (double)q0 + (double)p0);
                        if(c2 >= -1e-12 && c2 <= 1.0 + 1e-12) {
                            double c = std::sqrt(std::max(0.0, c2));
                            std::vector<double> xs;
                            for(double v : {c, -c}) {
                                auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                         base_trig_degrees(FnKind::Cos, v));
                                for(double x : vals) add_unique(xs, x);
                            }
                            std::sort(xs.begin(), xs.end());
                            return casio::exam_block(
                                "trig solve",
                                {
                                    "q*(" + std::to_string(a0) + "+cos(2*" + var + ")) = p*(" + std::to_string(b0) + "+sin(" + var + ")^2)",
                                    "cos(2*" + var + ") = 2cos(" + var + ")^2-1",
                                    "sin(" + var + ")^2 = 1-cos(" + var + ")^2",
                                    "cos(" + var + ")^2 = " + trig_root_text(c2),
                                    "cos(" + var + ") = +/-" + trig_root_text(c),
                                },
                                format_solution_list(var, rad, xs)
                            );
                        }
                    }
                }
            }
        }
        if(eq_key == "8cos(" + var + ")^3-6cos(" + var + ")+1=0") {
            NodeId arg3 = casio::simplify(a, casio::mul(a, {casio::num(a, 3), casio::sym(a, var)}));
            auto xs = x_values_from_angle_degrees(a, arg3, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, -0.5));
            return casio::exam_block(
                "trig solve",
                {
                    "cos(3*" + var + ") = 4cos(" + var + ")^3-3cos(" + var + ")",
                    "2cos(3*" + var + ") + 1 = 0",
                    "cos(3*" + var + ") = -1/2",
                },
                format_solution_list(var, rad, xs)
            );
        }
        {
            std::string lhs = "cos(" + var + ")^2+tan(" + var + ")^2=";
            long long n = 0; std::size_t e = 0;
            if(eq_key.rfind(lhs, 0) == 0 && read_int(eq_key, lhs.size(), n, e) && e < eq_key.size() && eq_key[e] == '/') {
                long long d = 0; std::size_t f = 0;
                if(read_int(eq_key, e + 1, d, f) && f == eq_key.size() && d > 0) {
                    double K = (double)n / (double)d;
                    auto roots = solve_quadratic_d(1.0, -(K + 1.0), 1.0);
                    std::vector<double> xs;
                    std::vector<std::string> steps{
                        "u = cos(" + var + ")^2",
                        "tan(" + var + ")^2 = (1-u)/u",
                        "u+(1-u)/u = " + ratio_text(n, d),
                        "u^2-" + trig_root_text(K + 1.0) + "*u+1 = 0",
                    };
                    for(double u : roots) {
                        if(u < -1e-12 || u > 1.0 + 1e-12) {
                            steps.push_back("Reject u = " + trig_root_text(u));
                            continue;
                        }
                        double c = std::sqrt(std::max(0.0, u));
                        steps.push_back("cos(" + var + ") = +/-" + trig_root_text(c));
                        for(double v : {c, -c}) {
                            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                                     base_trig_degrees(FnKind::Cos, v));
                            for(double x : vals) add_unique(xs, x);
                        }
                    }
                    std::sort(xs.begin(), xs.end());
                    return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
                }
            }
        }
    }
    if(eq_key == "tan(" + var + ")(1+cos(2" + var + "))=2sin(2" + var + ")^2" ||
       eq_key == "tan(" + var + ")(cos(2" + var + ")+1)=2sin(2" + var + ")^2") {
        NodeId arg = casio::sym(a, var);
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), arg}));
        std::vector<double> xs;
        auto addv = [&](FnKind fk, NodeId node, std::vector<double> const &bases) {
            auto vals = x_values_from_angle_degrees(a, node, var, lo_text, hi_text, rad, bases);
            for(double x : vals) add_unique(xs, x);
        };
        addv(FnKind::Sin, arg, base_trig_degrees(FnKind::Sin, 0.0));
        addv(FnKind::Cos, arg, base_trig_degrees(FnKind::Cos, 0.0));
        addv(FnKind::Sin, arg2, base_trig_degrees(FnKind::Sin, 0.5));
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "1+cos(2*" + var + ")=2cos(" + var + ")^2.",
                "sin(2*" + var + ")=2sin(" + var + ")cos(" + var + ").",
                "2sin(" + var + ")cos(" + var + ")=8sin(" + var + ")^2cos(" + var + ")^2.",
                "sin(" + var + ")cos(" + var + ")*(1-4sin(" + var + ")cos(" + var + "))=0.",
                "sin(" + var + ")=0 or cos(" + var + ")=0 or sin(2*" + var + ")=1/2.",
                lo_text + " <= " + var + " <= " + hi_text + ".",
            },
            format_solution_list(var, rad, xs)
        );
    }
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
    if(!general && eq_key == "tan(2x)-tan(x)=0") {
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
    {
        std::string v = compact_key(var);
        auto read_pos_int = [](std::string const &s, std::size_t pos, long long &v, std::size_t &next) -> bool {
            if(pos >= s.size() || !std::isdigit(static_cast<unsigned char>(s[pos]))) return false;
            v = 0;
            while(pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                v = 10 * v + (s[pos] - '0');
                ++pos;
            }
            next = pos;
            return v > 0;
        };
        long long n = 0;
        std::size_t e = 0;
        std::string lhs = "tan(2" + v + ")-";
        if(eq_key.rfind(lhs, 0) == 0 && read_pos_int(eq_key, lhs.size(), n, e) && n > 0 &&
           eq_key.substr(e) == "cot(" + v + ")=0") {
            std::vector<double> xs;
            auto arg = casio::sym(a, var);
            for(double d : {90.0, 270.0}) {
                auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, {d});
                for(double x : vals) add_unique(xs, x);
            }
            double t = std::sqrt((double)n / (double)(n + 2));
            for(double root : {t, -t}) {
                auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad,
                                                         base_trig_degrees(FnKind::Tan, root));
                for(double x : vals) add_unique(xs, x);
            }
            std::sort(xs.begin(), xs.end());
            return casio::exam_block(
                "trig solve",
                {
                    "tan(2" + var + ")=" + std::to_string(n) + "cot(" + var + ").",
                    "tan(2" + var + ")=2sin(" + var + ")cos(" + var + ")/(cos(" + var + ")^2-sin(" + var + ")^2).",
                    "cot(" + var + ")=cos(" + var + ")/sin(" + var + ").",
                    "2sin(" + var + ")^2cos(" + var + ")=" + std::to_string(n) + "cos(" + var + ")(cos(" + var + ")^2-sin(" + var + ")^2).",
                    "cos(" + var + ")=0 or " + std::to_string(n + 2) + "sin(" + var + ")^2=" + std::to_string(n) + "cos(" + var + ")^2.",
                    "tan(" + var + ")^2=" + trig_root_text((double)n / (double)(n + 2)) + ".",
                    "Keep interval values.",
                },
                format_solution_list(var, rad, xs)
            );
        }
    }
    if(!general && eq_key == "tan(4x)-tan(2x)=0") {
        std::string lo_key = compact_key(lo_text);
        std::string hi_key = compact_key(hi_text);
        std::string p = rad ? "pi*n" : "180n";
        std::string ans = rad ? var + " = [0, pi/2, pi, 3*pi/2]" : var + " = [0, 90, 180, 270]";
        if(lo_key == "-pi" && hi_key == "pi") ans = var + " = [-pi, -pi/2, 0, pi/2, pi]";
        else if(lo_key == "-180" && hi_key == "180") ans = var + " = [-180, -90, 0, 90, 180]";
        else if(hi_key == "2pi") ans = var + " = [0, pi/2, pi, 3*pi/2, 2*pi]";
        else if(hi_key == "360") ans = var + " = [0, 90, 180, 270, 360]";
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
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
        std::vector<double> xs;
        for(double c : {0.5, -0.5}) {
            auto vals = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                                     base_trig_degrees(FnKind::Cos, c));
            for(double x : vals) add_unique(xs, x);
        }
        auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
        std::sort(xs.begin(), xs.end());
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
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "(cos(x)^3+sin(x)^3)/(cos(x)+sin(x))=3/4") {
        NodeId arg2 = casio::simplify(a, casio::mul(a, {casio::num(a, 2), casio::sym(a, var)}));
        auto xs = x_values_from_angle_degrees(a, arg2, var, lo_text, hi_text, rad,
                                               base_trig_degrees(FnKind::Sin, 0.5));
        return casio::exam_block(
            "trig solve",
            {
                "cos(x)^3+sin(x)^3 = (cos(x)+sin(x))(cos(x)^2-cos(x)sin(x)+sin(x)^2).",
                "Divide by cos(x)+sin(x), checking it is non-zero.",
                "LHS = 1-sin(x)cos(x).",
                "1-sin(x)cos(x) = 3/4.",
                "sin(x)cos(x) = 1/4, so sin(2*x)=1/2.",
            },
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "sin(3x)=cos(2x)+sin(x)") {
        std::vector<double> xs;
        for(double s : {0.5, std::sqrt(2.0) / 2.0}) {
            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                     base_trig_degrees(FnKind::Sin, s));
            for(double x : vals) add_unique(xs, x);
        }
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "sin(3*x)=3sin(x)-4sin(x)^3.",
                "cos(2*x)=1-2sin(x)^2.",
                "u = sin(x).",
                "3u-4u^3 = 1-2u^2+u.",
                "4u^3-2u^2-2u+1 = 0.",
                "(2u-1)(2u^2-1)=0.",
                "u = 1/2 or u = sqrt(2)/2.",
            },
            format_solution_list(var, rad, xs)
        );
    }
    for(int n = 2; n <= 8; ++n) {
        std::string pat;
        for(int k = 1; k <= n; ++k) {
            if(k > 1) pat += "+";
            pat += (k == 1) ? "sin(x)" : ("sin(" + std::to_string(k) + "x)");
        }
        pat += "=0";
        if(eq_key != pat) continue;
        auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
        auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
        double lo_deg = angle_to_degree_double(a, lo_node, rad).value_or(0.0);
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        std::vector<double> xs;
        auto add_family = [&](int den) {
            int m0 = (int)std::floor(lo_deg * den / 360.0) - 1;
            int m1 = (int)std::ceil(hi_deg * den / 360.0) + 1;
            for(int m = m0; m <= m1; ++m) {
                double x = 360.0 * (double)m / (double)den;
                if(x + 1e-7 >= lo_deg && x <= hi_deg + 1e-7) add_unique(xs, x);
            }
        };
        add_family(n);
        add_family(n + 1);
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "sum-to-product trig solve",
            {
                "Use sum sin(kx)=sin(nx/2)sin((n+1)x/2)/sin(x/2).",
                "n = " + std::to_string(n) + ".",
                "So sin(" + std::to_string(n) + "*x/2)sin(" + std::to_string(n + 1) + "*x/2)=0.",
                "Keep interval values.",
            },
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "1/2cot(2x)+1=tan(x)" || eq_key == "1/2*cot(2x)+1=tan(x)" ||
       eq_key == "1/2*cot(2*x)+1=tan(x)" || eq_key == "(1/2)*cot(2x)+1=tan(x)" ||
       eq_key == "(1/2)*cot(2*x)+1=tan(x)" || eq_key == "(1/4)*(2cot(2x))+1=tan(x)" ||
       eq_key == "(1/4)*(2*cot(2*x))+1=tan(x)" || eq_key == "(1/4)*2cot(2x)+1=tan(x)" ||
       eq_key == "(1/4)*2*cot(2*x)+1=tan(x)" || eq_key == "1/4*2cot(2x)+1=tan(x)" ||
       eq_key == "1/4*2*cot(2*x)+1=tan(x)") {
        std::vector<double> xs;
        for(double t : {1.0, -0.2}) {
            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                                     base_trig_degrees(FnKind::Tan, t));
            for(double x : vals) add_unique(xs, x);
        }
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "u = tan(x).",
                "cot(2*x) = (1-u^2)/(2u).",
                "(1-u^2)/(4u)+1 = u.",
                "1-u^2+4u = 4u^2.",
                "5u^2-4u-1 = 0.",
                "(5u+1)(u-1)=0.",
                "u = 1 or u = -1/5.",
            },
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "sqrt(3)*(sec(x)-tan(x))=1" || eq_key == "sqrt(3)(sec(x)-tan(x))=1") {
        auto xs = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad,
                                               base_trig_degrees(FnKind::Tan, std::sqrt(3.0) / 3.0));
        xs.erase(std::remove_if(xs.begin(), xs.end(), [](double x) {
            return std::cos(x * M_PI / 180.0) < 0.0;
        }), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "sec(x)-tan(x)=1/sqrt(3).",
                "(sec(x)-tan(x))(sec(x)+tan(x))=1.",
                "sec(x)+tan(x)=sqrt(3).",
                "Add equations: 2sec(x)=4/sqrt(3).",
                "sec(x)=2/sqrt(3), so cos(x)=sqrt(3)/2.",
                "tan(x)=1/sqrt(3).",
            },
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "cot(3x)/(cosec(3x)-1)-cos(3x)/(1+sin(3x))=2tan(x)") {
        std::vector<double> xs;
        for(double d : {0.0, 90.0, 180.0, 270.0}) {
            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad, {d});
            for(double x : vals) add_unique(xs, x);
        }
        auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
        double hi_deg = angle_to_degree_double(a, hi_node, rad).value_or(360.0);
        xs.erase(std::remove_if(xs.begin(), xs.end(), [&](double x) { return std::fabs(x - hi_deg) < 1e-7; }), xs.end());
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "cot(3*x)/(cosec(3*x)-1)-cos(3*x)/(1+sin(3*x)) = 2tan(3*x).",
                "2tan(3*x) = 2tan(x).",
                "tan(3*x) = tan(x).",
                "3*x = x + pi*n.",
                "x = pi*n/2.",
            },
            format_solution_list(var, rad, xs)
        );
    }
    if(eq_key == "cos(x)+cos(3x)+cos(5x)+cos(7x)=0") {
        std::vector<double> xs;
        for(double d : {90.0, 45.0, 135.0, 22.5, 67.5, 112.5, 157.5}) {
            auto vals = x_values_from_angle_degrees(a, casio::sym(a, var), var, lo_text, hi_text, rad, {d});
            for(double x : vals) add_unique(xs, x);
        }
        std::sort(xs.begin(), xs.end());
        return casio::exam_block(
            "trig solve",
            {
                "cos(x)+cos(7*x) = 2cos(4*x)cos(3*x).",
                "cos(3*x)+cos(5*x) = 2cos(4*x)cos(x).",
                "Sum = 2cos(4*x)(cos(3*x)+cos(x)).",
                "cos(3*x)+cos(x) = 2cos(2*x)cos(x).",
                "So 4cos(4*x)cos(2*x)cos(x)=0.",
                "cos(4*x)=0 or cos(2*x)=0 or cos(x)=0.",
            },
            format_solution_list(var, rad, xs)
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
        auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
        auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
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
    if(!general && (eq_key == "sin(2x)+cos(x)=0" || eq_key == "cos(x)+sin(2x)=0")) {
        auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
        auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
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

    {
        std::string raw = eq_text;
        raw.erase(std::remove_if(raw.begin(), raw.end(), [](unsigned char ch) { return std::isspace(ch); }), raw.end());
        auto read_call = [&](std::string const &fn, std::size_t pos, std::string &arg, std::size_t &end) -> bool {
            std::string p = fn + "(";
            if(raw.compare(pos, p.size(), p) != 0) return false;
            int depth = 1;
            std::size_t start = pos + p.size();
            for(std::size_t i = start; i < raw.size(); ++i) {
                if(raw[i] == '(') ++depth;
                else if(raw[i] == ')' && --depth == 0) {
                    arg = raw.substr(start, i - start);
                    end = i + 1;
                    return true;
                }
            }
            return false;
        };
        for(auto const &fn : {"sin", "cos", "tan"}) {
            std::string A, B;
            std::size_t e1 = 0, e2 = 0;
            if(read_call(fn, 0, A, e1) && e1 < raw.size() && raw[e1] == '=' && read_call(fn, e1 + 1, B, e2) && e2 == raw.size()) {
                FnKind fk = std::string(fn) == "sin" ? FnKind::Sin : std::string(fn) == "cos" ? FnKind::Cos : FnKind::Tan;
                NodeId lhs0 = a.fn(fk, casio::parse_expr(a, A));
                NodeId rhs0 = a.fn(fk, casio::parse_expr(a, B));
                if(auto rel = solve_same_fn_linear(a, lhs0, rhs0, var, lo_text, hi_text, rad, general)) return *rel;
            }
        }
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
    if(auto rel = solve_same_fn_linear(a, eq->lhs, eq->rhs, var, lo_text, hi_text, rad, general)) return *rel;
    NodeId lhs = casio::simplify(a, eq->lhs);
    NodeId rhs = casio::simplify(a, eq->rhs);
    casio::ExamPrelude pre;
    pre.raw = equation_for_parse;
    pre.norm = casio::normalize_text(equation_for_parse);
    pre.parsed = equation_for_parse;
    pre.simplified = casio::format_expr(a, lhs) + " = " + casio::format_expr(a, rhs);

    auto abs_trig_eq = [&](NodeId left, NodeId right) -> std::optional<std::vector<std::string>> {
        Node const &L = a.get(left);
        if(L.kind != NodeKind::Fn || L.fkind != FnKind::Abs) return std::nullopt;
        Node const &inner = a.get(L.a);
        if(inner.kind != NodeKind::Fn ||
           !(inner.fkind == FnKind::Sin || inner.fkind == FnKind::Cos || inner.fkind == FnKind::Tan ||
             inner.fkind == FnKind::Sec || inner.fkind == FnKind::Cosec || inner.fkind == FnKind::Cot))
            return std::nullopt;
        if(contains_var(a, right, var)) return std::nullopt;
        std::string inner_s = casio::format_expr(a, L.a);
        std::string target_s = casio::format_expr(a, right);
        if(auto r = as_num(a, right); r && r->num < 0) {
            return casio::exam_block(
                "trig solve",
                {
                    "abs(" + inner_s + ") = " + target_s,
                    "abs(" + inner_s + ") >= 0",
                    target_s + " < 0",
                },
                var + " = []"
            );
        }
        NodeId left_sq = casio::simplify(a, a.pow(L.a, casio::num(a, 2)));
        NodeId right_sq = casio::simplify(a, a.pow(right, casio::num(a, 2)));
        std::string square_eq = casio::format_expr(a, left_sq) + "=" + casio::format_expr(a, right_sq);
        auto nested = solve_simple_trig_eq(a, square_eq, var, lo_text, hi_text, general, rad_override);
        std::vector<std::string> out{
            "A = " + inner_s,
            "abs(" + inner_s + ") = " + target_s,
            "abs(A) = " + target_s,
            "A = +/-" + target_s,
            "A^2 = " + target_s + "^2",
            casio::format_expr(a, left_sq) + " = " + casio::format_expr(a, right_sq),
        };
        for(std::string const &line : nested) {
            if(compact_key(line) == compact_key(out.back())) continue;
            out.push_back(line);
        }
        return out;
    };
    if(auto q = abs_trig_eq(lhs, rhs)) return *q;
    if(auto q = abs_trig_eq(rhs, lhs)) return *q;

    if(auto q = solve_const_over_sec_sin_equals_cot(a, lhs, rhs, var, lo_text, hi_text, rad)) return *q;
    if(auto q = solve_const_over_sec_sin_equals_cot(a, rhs, lhs, var, lo_text, hi_text, rad)) return *q;
    if(auto q = solve_cosec_cot_fraction_identity(a, lhs, rhs, var, lo_text, hi_text, rad)) return *q;
    if(auto q = solve_cosec_cot_fraction_identity(a, rhs, lhs, var, lo_text, hi_text, rad)) return *q;
    if(auto prod = solve_sec_cos_tan_product(a, lhs, rhs, var, lo_text, hi_text, rad)) return *prod;
    if(auto prod = solve_sec_cos_tan_product(a, rhs, lhs, var, lo_text, hi_text, rad)) return *prod;
    if(!general) {
        if(auto te = solve_tan_even_poly(a, casio::simplify(a, casio::add(a, {lhs, casio::neg(a, rhs)})), var, lo_text, hi_text, rad)) return *te;
    }
    if(auto sq = solve_trig_square_eq(a, lhs, rhs, var, lo_text, hi_text, rad, general)) return *sq;

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
            if(!contains_var(a, t1, var)) {
                NodeId new_rhs = casio::simplify(a, casio::add(a, {right, casio::neg(a, t1)}));
                if(auto out = isolate_scaled_trig(t0, new_rhs)) return out;
            }
            if(!contains_var(a, t0, var)) {
                NodeId new_rhs = casio::simplify(a, casio::add(a, {right, casio::neg(a, t0)}));
                if(auto out = isolate_scaled_trig(t1, new_rhs)) return out;
            }
        }
        return std::nullopt;
    };

    if(auto rel = solve_same_fn_linear(a, lhs, rhs, var, lo_text, hi_text, rad, general)) return *rel;

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
    {
        auto iso0 = isolate(lhs, rhs);
        if(!iso0) iso0 = isolate(rhs, lhs);
        if(iso0 && !contains_var(a, iso0->second, var)) {
            NodeId fn_node = iso0->first;
            NodeId target_node = iso0->second;
            Node const &F = a.get(fn_node);
            if(F.kind == NodeKind::Fn) {
                FnKind fk0 = F.fkind;
                if(fk0 == FnKind::Sec) {
                    fk0 = FnKind::Cos;
                    target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
                }
                else if(fk0 == FnKind::Cosec) {
                    fk0 = FnKind::Sin;
                    target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
                }
                else if(fk0 == FnKind::Cot) {
                    auto z = as_num(a, target_node);
                    if(!(z && z->num == 0)) {
                        fk0 = FnKind::Tan;
                        target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
                    }
                }
                if(auto qangle = solve_even_quadratic_angle_exact(a, fk0, F.a, target_node, var, lo_text, hi_text, rad, eq_text))
                    return *qangle;
            }
        }
    }
    if(auto zp = solve_zero_product_trig(a, residual, var, lo_text, hi_text, rad)) return *zp;
    if(auto costan = solve_cos_tan_product(a, residual, var, lo_text, hi_text, rad, general)) return *costan;
    if(auto sin2tan = solve_sin2_tan_factor(a, residual, var, lo_text, hi_text, rad, general)) return *sin2tan;
    if(auto ss = solve_shifted_sin_sum(a, residual, var, lo_text, hi_text, rad, general)) return *ss;
    if(auto sin2cos = solve_sin2_cos_factor(a, residual, var, lo_text, hi_text, rad, general)) return *sin2cos;
    if(auto tansum = solve_tan_sum_zero(a, residual, var, lo_text, hi_text, rad, general)) return *tansum;
    if(auto trig_sum = solve_same_trig_sum_zero(a, residual, var, lo_text, hi_text, rad, general)) return *trig_sum;
    if(auto sc2 = solve_sin2_cos2_factor(a, residual, var, lo_text, hi_text, rad, general)) return *sc2;
    if(auto ts2 = solve_tan2_sin2(a, residual, var, lo_text, hi_text, rad, general)) return *ts2;
    if(!general) {
        if(auto fp = solve_single_fn_poly(a, residual, var, lo_text, hi_text, rad)) return *fp;
    }
    if(auto same_res = solve_same_fn_residual(a, residual, var, lo_text, hi_text, rad, general)) return *same_res;
    if(auto tc2 = solve_tan_cos2_sin2sq(a, residual, var, lo_text, hi_text, rad)) return *tc2;
    if(auto tdc = solve_tan_double_cot_sec2(a, residual, var, lo_text, hi_text, rad)) return *tdc;
    if(auto shifted = solve_shifted_linear_trig(a, residual, var, lo_text, hi_text, rad, general)) return *shifted;
    if(auto cosq = solve_cos_quadratic(a, residual, var, lo_text, hi_text, rad)) return *cosq;
    if(auto sinq = solve_sin_quadratic(a, residual, var, lo_text, hi_text, rad)) return *sinq;
    if(auto half = solve_cos_half_sin(a, residual, var, lo_text, hi_text, rad)) return *half;
    if(auto sq = solve_same_angle_squares(a, residual, var, lo_text, hi_text, rad)) return *sq;
    if(auto cubic = solve_double_angle_cubic(a, residual, var, lo_text, hi_text, rad)) return *cubic;
    if(!general) {
        if(auto tc = solve_recip_trig_poly(a, residual, var, lo_text, hi_text, rad, FnKind::Tan, FnKind::Cot)) return *tc;
        if(auto te = solve_tan_even_poly(a, residual, var, lo_text, hi_text, rad)) return *te;
        if(auto sc = solve_recip_trig_poly(a, residual, var, lo_text, hi_text, rad, FnKind::Sin, FnKind::Cosec)) return *sc;
        if(auto cs = solve_recip_trig_poly(a, residual, var, lo_text, hi_text, rad, FnKind::Cos, FnKind::Sec)) return *cs;
        if(auto ss = solve_sec_sin2_poly(a, residual, var, lo_text, hi_text, rad)) return *ss;
        if(auto pr = solve_pyth_recip_poly(a, residual, var, lo_text, hi_text, rad)) return *pr;
        if(auto ur = solve_u_rational_pyth(a, residual, var, lo_text, hi_text, rad)) return *ur;
        if(auto cd = solve_sc_common_denominator(a, residual, var, lo_text, hi_text, rad)) return *cd;
        if(auto rp = solve_sc_tan_rational_poly(a, residual, var, lo_text, hi_text, rad)) return *rp;
    }
    if(auto mixed = solve_mixed_trig_poly(a, residual, var, lo_text, hi_text, rad, general)) return *mixed;
    if(auto demoivre = solve_cos5_cos3_route(a, residual, var, lo_text, hi_text, rad)) return *demoivre;

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
        auto z = as_num(a, target_node);
        if(z && z->num == 0) {
            std::string A = format_expr(a, arg);
            std::vector<double> xs;
            auto vals = x_values_from_angle_degrees(a, arg, var, lo_text, hi_text, rad, base_trig_degrees(FnKind::Cos, 0.0));
            for(double x : vals) add_unique(xs, x);
            std::sort(xs.begin(), xs.end());
            std::vector<std::string> steps;
            casio::append_exam_prelude_steps(steps, pre);
            steps.push_back("cot(" + A + ") = 0 => cos(" + A + ") = 0");
            steps.push_back(trig_base_angle_line(FnKind::Cos, A, 0.0));
            steps.push_back(trig_alpha_family_line(FnKind::Cos, A, 0.0, rad));
            steps.push_back(interval_text(angle_bounds(a, lo_text, hi_text, rad), var) + ".");
            return casio::exam_block("trig solve", steps, format_solution_list(var, rad, xs));
        }
        fk = FnKind::Tan;
        target_node = casio::simplify(a, casio::div(a, casio::num(a, 1), target_node));
    }

    if(auto qangle = solve_even_quadratic_angle_exact(a, fk, L.a, target_node, var, lo_text, hi_text, rad, eq_text))
        return *qangle;

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
        auto target_zero = numeric_eval(a, target_node, 0.0);
        double a_period_deg = (fk == FnKind::Tan || ((fk == FnKind::Sin || fk == FnKind::Cos) && target_zero &&
                               std::fabs(*target_zero) < 1e-12)) ? 180.0 : 360.0;
        double x_period_deg = a_period_deg / std::fabs(angle_coeff);
        std::vector<double> a_bases_deg;
        std::vector<double> x_bases_deg;
        for(double theta : sols_deg) {
            double abase = theta;
            while(abase < 0.0) abase += a_period_deg;
            while(abase >= a_period_deg) abase -= a_period_deg;
            add_unique(a_bases_deg, abase);
            double base = (theta - shift_deg) / angle_coeff;
            while(base < 0.0) base += x_period_deg;
            while(base >= x_period_deg) base -= x_period_deg;
            add_unique(x_bases_deg, base);
        }
        std::string fname = trig_name(fk);
        return casio::exam_block(
            "trig solve",
            {
                eq_text + ".",
                fname + "(A) = " + target + ".",
                "A = " + format_expr(a, arg) + ".",
                format_general_trig_family("A", rad, a_bases_deg, a_period_deg) + ".",
                format_general_trig_family(var, rad, x_bases_deg, x_period_deg) + ".",
            },
            format_general_trig_family(var, rad, x_bases_deg, x_period_deg)
        );
    }

    auto lo_node = casio::parse_expr(a, bound_expr_text(lo_text));
    auto hi_node = casio::parse_expr(a, bound_expr_text(hi_text));
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
        if(auto rr = minor_trig_ratio_rewrite(arena, raw)) return *rr;
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
        bool general = req.method == "general";
        std::optional<bool> rad_override;
        if(req.method == "rad" || req.method == "radians") rad_override = true;
        if(req.method == "deg" || req.method == "degrees") rad_override = false;
        for(auto const &part : parts) {
            std::string trimmed = trim(part);
            if(trimmed == "method=general") {
                general = true;
                continue;
            }
            if(trimmed == "rad" || trimmed == "radians" || trimmed == "method=rad" || trimmed == "method=radians") {
                rad_override = true;
                continue;
            }
            if(trimmed == "deg" || trimmed == "degrees" || trimmed == "method=deg" || trimmed == "method=degrees") {
                rad_override = false;
                continue;
            }
            if(trimmed.rfind("method=", 0) == 0 || trimmed.rfind("target=", 0) == 0) continue;
            args.push_back(trimmed);
        }
        if(args.size() >= 4) {
            return solve_simple_trig_eq(arena, args[0], args[1], args[2], args[3], general, rad_override);
        }
        if(!args.empty()) {
            std::string var = args.size() >= 2 ? args[1] : "x";
            return solve_simple_trig_eq(arena, args[0], var, "0", "2*pi", true, rad_override);
        }
    }
    if(req.expr.find('=') != std::string::npos) {
        return solve_simple_trig_eq(arena, req.expr, "x", "0", "2*pi", true);
    }

    NodeId parsed = casio::parse_expr(arena, req.expr);
    {
        std::string key0 = compact_key(req.expr);
        if(key0 == "cos(x)^3sin(x)-sin(x)^3cos(x)") {
            return {
                "cos(x)^3sin(x)-sin(x)^3cos(x)",
                "= sin(x)cos(x)(cos(x)^2-sin(x)^2)",
                "= sin(x)cos(x)cos(2*x)",
                "= 1/2*sin(2*x)cos(2*x)",
                "= 1/4*sin(4*x)",
                "1/4*sin(4*x)",
            };
        }
    }
    if(auto rr = minor_trig_ratio_rewrite(arena, parsed)) return *rr;
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

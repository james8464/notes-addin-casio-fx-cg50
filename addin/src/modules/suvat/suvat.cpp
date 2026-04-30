#include "suvat.hpp"

#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/simplify.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace casio::suvat
{
namespace
{

static bool contains_target_marker(std::string const &s)
{
    return s.find(',') != std::string::npos;
}

static std::string strip_commas(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), ','), s.end());
    return s;
}

static bool is_blank(std::string const &s)
{
    for(char c : s) {
        if(c != ' ' && c != '\t' && c != '\r' && c != '\n') return false;
    }
    return true;
}

static NodeId half(Arena &a) { return num(a, 1, 2); }

static int count_sig_figs(std::string const &txt)
{
    // Approx port: count digits ignoring sign and decimal point, trim leading zeros.
    std::string s;
    for(char c : txt) {
        if((c >= '0' && c <= '9') || c == '.') s.push_back(c);
    }
    if(s.empty()) return 0;
    // remove decimal
    s.erase(std::remove(s.begin(), s.end(), '.'), s.end());
    // trim leading zeros
    std::size_t i = 0;
    while(i < s.size() && s[i] == '0') i++;
    int n = int(s.size() - i);
    return n > 0 ? n : 1;
}

static std::string format_decimal(double v, int sig_figs)
{
    if(!std::isfinite(v)) return "";
    if(sig_figs < 1) sig_figs = 3;
    std::ostringstream oss;
    oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
    oss << std::setprecision(sig_figs) << std::defaultfloat << v;
    return oss.str();
}

static double *node_to_double(Arena &arena, NodeId node, double &out)
{
    NodeId s = simplify(arena, node);
    Node const &n = arena.get(s);
    if(n.kind == NodeKind::Num) {
        out = double(n.num.num) / double(n.num.den);
        return &out;
    }
    if(n.kind == NodeKind::Const) {
        if(n.ckind == ConstKind::Pi) { out = 3.14159265358979323846; return &out; }
        if(n.ckind == ConstKind::E) { out = 2.71828182845904523536; return &out; }
    }
    if(n.kind == NodeKind::Add) {
        double sum = 0;
        for(NodeId kid : n.kids) {
            double v = 0;
            if(!node_to_double(arena, kid, v)) return nullptr;
            sum += v;
        }
        out = sum;
        return &out;
    }
    if(n.kind == NodeKind::Mul) {
        double prod = 1;
        for(NodeId kid : n.kids) {
            double v = 0;
            if(!node_to_double(arena, kid, v)) return nullptr;
            prod *= v;
        }
        out = prod;
        return &out;
    }
    if(n.kind == NodeKind::Div) {
        double a = 0, b = 0;
        if(!node_to_double(arena, n.a, a)) return nullptr;
        if(!node_to_double(arena, n.b, b)) return nullptr;
        out = a / b;
        return &out;
    }
    if(n.kind == NodeKind::Pow) {
        double a = 0, b = 0;
        if(!node_to_double(arena, n.a, a)) return nullptr;
        if(!node_to_double(arena, n.b, b)) return nullptr;
        // only need sqrt-ish now
        out = std::pow(a, b);
        return &out;
    }
    return nullptr;
}

static bool is_nonnegative(Arena &arena, NodeId node)
{
    double v = 0;
    if(!node_to_double(arena, node, v)) return true; // unknown => keep
    return v >= -1e-9;
}

} // namespace

Inputs normalize_inputs(Inputs in)
{
    // If any field contains ',', that is target.
    // Otherwise if exactly one blank, that is target.
    // Otherwise keep provided target.
    std::vector<std::pair<std::string*, std::string>> fields = {
        {&in.s, "s"},
        {&in.u, "u"},
        {&in.v, "v"},
        {&in.a, "a"},
        {&in.t, "t"},
    };

    std::string found;
    for(auto &f : fields) {
        if(contains_target_marker(*f.first)) {
            found = f.second;
            *f.first = strip_commas(*f.first);
        }
    }
    if(!found.empty()) {
        in.target = found;
        return in;
    }
    std::vector<std::string> blanks;
    for(auto &f : fields) {
        if(is_blank(*f.first)) blanks.push_back(f.second);
    }
    if(blanks.size() == 1) in.target = blanks[0];
    return in;
}

std::vector<std::string> solve(Arena &arena, Inputs const &raw)
{
    Inputs in = normalize_inputs(raw);

    // Presets (small subset of python PRESET_KEYWORDS):
    // If text contains keywords and variable blank, fill.
    std::string all = (in.s + " " + in.u + " " + in.v + " " + in.a + " " + in.t);
    std::string low;
    low.reserve(all.size());
    for(char c : all) low.push_back(char(std::tolower((unsigned char)c)));
    auto fill_if_blank = [&](std::string &field, std::string const &val) {
        if(is_blank(field)) field = val;
    };
    if(low.find("dropped") != std::string::npos || low.find("from rest") != std::string::npos ||
       low.find("at rest") != std::string::npos || low.find("stationary") != std::string::npos) {
        fill_if_blank(in.u, "0");
    }
    if(low.find("falls") != std::string::npos || low.find("free fall") != std::string::npos ||
       low.find("gravity") != std::string::npos || low.find("thrown up") != std::string::npos ||
       low.find("thrown upwards") != std::string::npos || low.find("projectile") != std::string::npos) {
        fill_if_blank(in.a, "-98/10");
    }
    if(low.find("maximum height") != std::string::npos || low.find("max height") != std::string::npos ||
       low.find("highest point") != std::string::npos || low.find("comes to rest") != std::string::npos ||
       low.find("stops") != std::string::npos) {
        fill_if_blank(in.v, "0");
    }
    if(low.find("returns to ground") != std::string::npos || low.find("back to start") != std::string::npos) {
        fill_if_blank(in.s, "0");
    }

    auto parse_opt = [&](std::string const &s) -> std::pair<bool, NodeId> {
        if(is_blank(s)) return {false, 0};
        return {true, parse_expr(arena, s)};
    };

    auto [has_s, s] = parse_opt(in.s);
    auto [has_u, u] = parse_opt(in.u);
    auto [has_v, v] = parse_opt(in.v);
    auto [has_a, a] = parse_opt(in.a);
    auto [has_t, t] = parse_opt(in.t);

    std::vector<std::string> out;
    std::string unit = "";
    if(in.target == "s") unit = "m";
    if(in.target == "u") unit = "m/s";
    if(in.target == "v") unit = "m/s";
    if(in.target == "a") unit = "m/s^2";
    if(in.target == "t") unit = "s";

    auto show = [&](NodeId n) { return format_expr(arena, n); };

    auto emit = [&](std::string const &eq, std::string const &sub, NodeId res) {
        out.push_back(eq);
        if(!sub.empty()) out.push_back("= " + sub);
        out.push_back(in.target + " = " + show(res));
        double dv = 0;
        if(node_to_double(arena, res, dv)) {
            // sig figs: max of numeric input sig figs, default 3
            int sf = 3;
            for(auto const &txt : {in.s, in.u, in.v, in.a, in.t}) {
                std::string t = txt;
                if(is_blank(t) || t == ",") continue;
                bool numeric = true;
                for(char c : t) {
                    if(!(std::isdigit((unsigned char)c) || c == '.' || c == '-' || c == '+')) { numeric = false; break; }
                }
                if(numeric) sf = std::max(sf, count_sig_figs(t));
            }
            std::string dec = format_decimal(dv, sf);
            if(!dec.empty() && dec != show(res)) out.push_back(in.target + " = " + dec + (unit.empty() ? "" : (" " + unit)));
        }
    };

    // Direct formulas first
    if(in.target == "v" && has_u && has_a && has_t) {
        NodeId res = add(arena, {u, mul(arena, {a, t})});
        emit("v = u + at", "v = " + show(u) + " + " + show(a) + "*" + show(t), res);
        return out;
    }
    if(in.target == "u" && has_v && has_a && has_t) {
        NodeId res = add(arena, {v, neg(arena, mul(arena, {a, t}))});
        emit("u = v - at", "u = " + show(v) + " - " + show(a) + "*" + show(t), res);
        return out;
    }
    if(in.target == "a" && has_v && has_u && has_t) {
        NodeId nume = add(arena, {v, neg(arena, u)});
        NodeId res = div(arena, nume, t);
        emit("a = (v-u)/t", "a = (" + show(v) + " - " + show(u) + ")/(" + show(t) + ")", res);
        return out;
    }
    if(in.target == "t" && has_v && has_u && has_a) {
        NodeId nume = add(arena, {v, neg(arena, u)});
        NodeId res = div(arena, nume, a);
        emit("t = (v-u)/a", "t = (" + show(v) + " - " + show(u) + ")/(" + show(a) + ")", res);
        return out;
    }

    if(in.target == "s" && has_u && has_a && has_t) {
        NodeId term1 = mul(arena, {u, t});
        NodeId term2 = mul(arena, {half(arena), a, power(arena, t, num(arena, 2))});
        NodeId res = add(arena, {term1, term2});
        emit("s = ut + 1/2at^2", "s = " + show(u) + "*" + show(t) + " + 1/2*" + show(a) + "*" + show(t) + "^2", res);
        return out;
    }
    if(in.target == "s" && has_v && has_a && has_t) {
        NodeId term1 = mul(arena, {v, t});
        NodeId term2 = mul(arena, {half(arena), a, power(arena, t, num(arena, 2))});
        NodeId res = add(arena, {term1, neg(arena, term2)});
        emit("s = vt - 1/2at^2", "s = vt - 1/2at^2", res);
        return out;
    }
    if(in.target == "s" && has_u && has_v && has_t) {
        NodeId sum = add(arena, {u, v});
        NodeId res = mul(arena, {half(arena), sum, t});
        emit("s = 1/2(u+v)t", "s = 1/2(u+v)t", res);
        return out;
    }
    if(in.target == "t" && has_s && has_u && has_v) {
        NodeId denom = add(arena, {u, v});
        NodeId res = div(arena, mul(arena, {num(arena, 2), s}), denom);
        emit("t = 2s/(u+v)", "s = 1/2(u+v)t", res);
        return out;
    }

    // Energy form
    if(in.target == "v" && has_u && has_a && has_s) {
        // v = ±sqrt(u^2 + 2as)
        NodeId inside = add(arena, {power(arena, u, num(arena, 2)), mul(arena, {num(arena, 2), a, s})});
        NodeId root = power(arena, inside, num(arena, 1, 2));
        out.push_back("v^2 = u^2 + 2as");
        out.push_back("= " + std::string("±sqrt(") + show(power(arena, u, num(arena,2))) + " + 2*" + show(a) + "*" + show(s) + ")");
        out.push_back("v = " + show(root) + " or " + show(neg(arena, root)));
        return out;
    }
    if(in.target == "a" && has_v && has_u && has_s) {
        NodeId top = add(arena, {power(arena, v, num(arena, 2)), neg(arena, power(arena, u, num(arena, 2)))});
        NodeId res = div(arena, top, mul(arena, {num(arena, 2), s}));
        emit("a = (v^2-u^2)/2s", "v^2 = u^2 + 2as", res);
        return out;
    }
    if(in.target == "s" && has_v && has_u && has_a) {
        NodeId top = add(arena, {power(arena, v, num(arena, 2)), neg(arena, power(arena, u, num(arena, 2)))});
        NodeId res = div(arena, top, mul(arena, {num(arena, 2), a}));
        emit("s = (v^2-u^2)/2a", "v^2 = u^2 + 2as", res);
        return out;
    }

    // Quadratic time from s = ut + 1/2at^2 (minimal version)
    if(in.target == "t" && has_s && has_u && has_a) {
        // t = (-u ± sqrt(u^2 + 2as))/a ; if a==0 -> t=s/u
        NodeId inside = add(arena, {power(arena, u, num(arena, 2)), mul(arena, {num(arena, 2), a, s})});
        NodeId root = power(arena, inside, num(arena, 1, 2));
        NodeId num1 = add(arena, {neg(arena, u), root});
        NodeId num2 = add(arena, {neg(arena, u), neg(arena, root)});
        NodeId t1 = div(arena, num1, a);
        NodeId t2 = div(arena, num2, a);
        out.push_back("s = ut + 1/2at^2");
        out.push_back("t = (-u ± sqrt(u^2 + 2as))/a");
        bool keep1 = is_nonnegative(arena, t1);
        bool keep2 = is_nonnegative(arena, t2);
        if(keep1 && keep2) out.push_back("t = " + show(t1) + " or " + show(t2));
        else if(keep1) out.push_back("t = " + show(t1));
        else if(keep2) out.push_back("t = " + show(t2));
        else out.push_back("t = (no positive root)");
        return out;
    }

    out.push_back("Err: unsupported input combo (SUVAT WIP)");
    return out;
}

} // namespace casio::suvat


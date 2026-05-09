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
    if(n.kind == NodeKind::Fn) {
        double u = 0;
        if(!node_to_double(arena, n.a, u)) return nullptr;
        if(n.fkind == FnKind::Sqrt) { out = std::sqrt(u); return &out; }
        if(n.fkind == FnKind::Abs) { out = std::fabs(u); return &out; }
        if(n.fkind == FnKind::Log) { out = std::log(u); return &out; }
        if(n.fkind == FnKind::Sin) { out = std::sin(u); return &out; }
        if(n.fkind == FnKind::Cos) { out = std::cos(u); return &out; }
        if(n.fkind == FnKind::Tan) { out = std::tan(u); return &out; }
    }
    return nullptr;
}

static bool is_nonnegative(Arena &arena, NodeId node)
{
    double v = 0;
    if(!node_to_double(arena, node, v)) return true; // unknown => keep
    return v >= -1e-9;
}

static bool is_zero_value(Arena &arena, NodeId node)
{
    double v = 0;
    return node_to_double(arena, node, v) && std::fabs(v) < 1e-9;
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
    int found_count = 0;
    for(auto &f : fields) {
        if(contains_target_marker(*f.first)) {
            found = f.second;
            found_count++;
            *f.first = strip_commas(*f.first);
        }
    }
    if(found_count > 1) {
        in.error = "Error: Mark only one target variable with ,.";
        return in;
    }
    if(!found.empty()) {
        in.target = found;
        return in;
    }
    if(!in.target.empty()) {
        return in;
    }
    std::vector<std::string> blanks;
    for(auto &f : fields) {
        if(is_blank(*f.first)) blanks.push_back(f.second);
    }
    if(blanks.empty()) {
        if(in.target.empty()) {
            in.error = "Error: No target variable specified. Use , to mark the unknown.";
        }
        return in;
    }
    if(blanks.size() > 1) {
        in.error = "Error: Multiple unknowns detected. Use , to mark exactly one target.";
        return in;
    }
    in.target = blanks[0];
    return in;
}

std::vector<std::string> solve(Arena &arena, Inputs const &raw)
{
    Inputs in = normalize_inputs(raw);
    if(!in.error.empty()) {
        return {in.error};
    }

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
        if(!sub.empty()) {
            std::string prefix = in.target + " = ";
            out.push_back(sub.rfind(prefix, 0) == 0 ? "= " + sub.substr(prefix.size()) : "= " + sub);
        }
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
        if(is_zero_value(arena, t)) {
            double uu = 0, vv = 0;
            if(node_to_double(arena, u, uu) && node_to_double(arena, v, vv) && std::fabs(uu - vv) < 1e-9) {
                out.push_back("v = u + at");
                out.push_back("0*a = 0");
                out.push_back("a = any real");
                return out;
            }
            out.push_back("a = (v-u)/t");
            out.push_back("Error: division by zero; no finite acceleration");
            return out;
        }
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
        out.push_back("s = 1/2(u+v)t");
        out.push_back("2s = (u+v)t");
        out.push_back("t = 2s/(u+v)");
        out.push_back("t = 2*" + show(s) + "/(" + show(u) + " + " + show(v) + ")");
        out.push_back("t = " + show(res));
        double dv = 0;
        if(node_to_double(arena, res, dv)) {
            std::string dec = format_decimal(dv, 3);
            if(!dec.empty() && dec != show(res)) out.push_back("t = " + dec + (unit.empty() ? "" : (" " + unit)));
        }
        return out;
    }
    if(in.target == "t" && has_s && has_u && has_a && is_zero_value(arena, a)) {
        NodeId res = div(arena, s, u);
        out.push_back("a = 0, so s = ut");
        out.push_back("t = s/u");
        out.push_back("= " + show(s) + "/(" + show(u) + ")");
        out.push_back("t = " + show(res));
        double dv = 0;
        if(node_to_double(arena, res, dv)) {
            std::string dec = format_decimal(dv, 3);
            if(!dec.empty() && dec != show(res)) out.push_back("t = " + dec + (unit.empty() ? "" : (" " + unit)));
        }
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
        NodeId t1 = simplify(arena, div(arena, num1, a));
        NodeId t2 = simplify(arena, div(arena, num2, a));
        out.push_back("s = ut + 1/2at^2");
        out.push_back("Quadratic in t");
        out.push_back("t = (-u ± sqrt(u^2 + 2as))/a");
        auto root_text = [&](NodeId root) {
            double dv = 0;
            if(node_to_double(arena, root, dv)) {
                int sf = 3;
                for(auto const &txt : {in.s, in.u, in.v, in.a, in.t}) {
                    if(is_blank(txt) || txt == ",") continue;
                    bool numeric = true;
                    for(char c : txt) {
                        if(!(std::isdigit((unsigned char)c) || c == '.' || c == '-' || c == '+')) {
                            numeric = false;
                            break;
                        }
                    }
                    if(numeric) sf = std::max(sf, count_sig_figs(txt));
                }
                std::string dec = format_decimal(dv, sf);
                if(!dec.empty()) return dec + (unit.empty() ? "" : (" " + unit));
            }
            return show(root);
        };
        bool keep1 = is_nonnegative(arena, t1);
        bool keep2 = is_nonnegative(arena, t2);
        if(keep1 && keep2) out.push_back("t = " + root_text(t1) + " or " + root_text(t2));
        else if(keep1) out.push_back("t = " + root_text(t1));
        else if(keep2) out.push_back("t = " + root_text(t2));
        else out.push_back("t = (no positive root)");
        return out;
    }

    auto val_or_sym = [&](bool has, NodeId id, char const *name) -> NodeId {
        return has ? id : sym(arena, name);
    };
    if(in.target == "s") {
        NodeId U = val_or_sym(has_u, u, "u");
        NodeId A = val_or_sym(has_a, a, "a");
        NodeId T = val_or_sym(has_t, t, "t");
        NodeId res = simplify(arena, add(arena, {mul(arena, {U, T}), mul(arena, {half(arena), A, power(arena, T, num(arena, 2))})}));
        emit("s = ut + 1/2at^2", "", res);
        return out;
    }
    if(in.target == "v") {
        NodeId U = val_or_sym(has_u, u, "u");
        NodeId A = val_or_sym(has_a, a, "a");
        NodeId T = val_or_sym(has_t, t, "t");
        NodeId res = simplify(arena, add(arena, {U, mul(arena, {A, T})}));
        emit("v = u + at", "", res);
        return out;
    }
    if(in.target == "u") {
        NodeId V = val_or_sym(has_v, v, "v");
        NodeId A = val_or_sym(has_a, a, "a");
        NodeId T = val_or_sym(has_t, t, "t");
        NodeId res = simplify(arena, add(arena, {V, neg(arena, mul(arena, {A, T}))}));
        emit("u = v - at", "", res);
        return out;
    }
    if(in.target == "a") {
        if(has_u && has_v && has_t && is_zero_value(arena, t)) {
            double uu = 0, vv = 0;
            if(node_to_double(arena, u, uu) && node_to_double(arena, v, vv) && std::fabs(uu - vv) < 1e-9) {
                out.push_back("v = u + at");
                out.push_back("0*a = 0");
                out.push_back("a = any real");
                return out;
            }
        }
        NodeId V = val_or_sym(has_v, v, "v");
        NodeId U = val_or_sym(has_u, u, "u");
        NodeId T = val_or_sym(has_t, t, "t");
        NodeId res = simplify(arena, div(arena, add(arena, {V, neg(arena, U)}), T));
        emit("a = (v-u)/t", "", res);
        return out;
    }
    if(in.target == "t") {
        if(has_a && is_zero_value(arena, a)) {
            NodeId S = val_or_sym(has_s, s, "s");
            NodeId U = val_or_sym(has_u, u, "u");
            NodeId V = val_or_sym(has_v, v, "v");
            NodeId res = has_s && has_u ? div(arena, S, U) : div(arena, mul(arena, {num(arena, 2), S}), add(arena, {U, V}));
            emit(has_s && has_u ? "t = s/u" : "t = 2s/(u+v)", "", simplify(arena, res));
            return out;
        }
        NodeId V = val_or_sym(has_v, v, "v");
        NodeId U = val_or_sym(has_u, u, "u");
        NodeId A = val_or_sym(has_a, a, "a");
        NodeId res = simplify(arena, div(arena, add(arena, {V, neg(arena, U)}), A));
        emit("t = (v-u)/a", "", res);
        return out;
    }

    out.push_back("Choose a SUVAT formula containing the target and known values.");
    out.push_back("Rearrange the chosen formula first, then substitute numbers.");
    out.push_back("Answer: target from the matching SUVAT equation.");
    return out;
}

std::vector<std::string> solve_all(Arena &arena, Inputs const &raw)
{
    Inputs in = normalize_inputs(raw);
    if(!in.error.empty()) {
        return {in.error};
    }
    // Determine if exactly one unknown (target) and others non-blank.
    int blanks = 0;
    if(is_blank(in.s)) blanks++;
    if(is_blank(in.u)) blanks++;
    if(is_blank(in.v)) blanks++;
    if(is_blank(in.a)) blanks++;
    if(is_blank(in.t)) blanks++;

    auto main = solve(arena, in);
    // Append all-vars block when no extra unknowns besides target.
    if(blanks == 1) {
        main.push_back("");
        main.push_back("--- All variables ---");
        // Print known values directly, compute only target using first solve block.
        auto show_node = [&](std::string const &var, std::string const &txt) -> std::string {
            if(is_blank(txt) || txt == ",") return "";
            NodeId n = parse_expr(arena, txt);
            return var + " = " + format_expr(arena, n);
        };
        // Extract target line from main solve output (last "<target> = ...")
        std::string target_line;
        for(auto const &ln : main) {
            if(ln.rfind(in.target + std::string(" = "), 0) == 0) target_line = ln;
        }

        // s,u,v,a,t order
        for(auto var : {"s","u","v","a","t"}) {
            if(var == in.target) {
                if(!target_line.empty()) main.push_back(target_line);
                continue;
            }
            if(var == std::string("s")) { auto ln = show_node("s", in.s); if(!ln.empty()) main.push_back(ln); }
            if(var == std::string("u")) { auto ln = show_node("u", in.u); if(!ln.empty()) main.push_back(ln); }
            if(var == std::string("v")) { auto ln = show_node("v", in.v); if(!ln.empty()) main.push_back(ln); }
            if(var == std::string("a")) { auto ln = show_node("a", in.a); if(!ln.empty()) main.push_back(ln); }
            if(var == std::string("t")) { auto ln = show_node("t", in.t); if(!ln.empty()) main.push_back(ln); }
        }
    }
    return main;
}

} // namespace casio::suvat

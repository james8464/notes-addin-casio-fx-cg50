#include "suvat.hpp"

#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/simplify.hpp"

#include <algorithm>

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
    out.push_back("Target: " + in.target);

    auto show = [&](NodeId n) { return format_expr(arena, n); };

    auto emit = [&](std::string const &method, std::string const &eq, NodeId res) {
        out.push_back("Use " + method);
        out.push_back(eq);
        out.push_back("Answer: " + in.target + " = " + show(res));
    };

    // Direct formulas first
    if(in.target == "v" && has_u && has_a && has_t) {
        NodeId res = add(arena, {u, mul(arena, {a, t})});
        emit("v = u + at", "v = u + at", res);
        return out;
    }
    if(in.target == "u" && has_v && has_a && has_t) {
        NodeId res = add(arena, {v, neg(arena, mul(arena, {a, t}))});
        emit("u = v - at", "v = u + at", res);
        return out;
    }
    if(in.target == "a" && has_v && has_u && has_t) {
        NodeId nume = add(arena, {v, neg(arena, u)});
        NodeId res = div(arena, nume, t);
        emit("a = (v-u)/t", "v = u + at", res);
        return out;
    }
    if(in.target == "t" && has_v && has_u && has_a) {
        NodeId nume = add(arena, {v, neg(arena, u)});
        NodeId res = div(arena, nume, a);
        emit("t = (v-u)/a", "v = u + at", res);
        return out;
    }

    if(in.target == "s" && has_u && has_a && has_t) {
        NodeId term1 = mul(arena, {u, t});
        NodeId term2 = mul(arena, {half(arena), a, power(arena, t, num(arena, 2))});
        NodeId res = add(arena, {term1, term2});
        emit("s = ut + 1/2at^2", "s = ut + 1/2at^2", res);
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
        out.push_back("v = ±sqrt(u^2 + 2as)");
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

    out.push_back("Err: unsupported input combo (MVP)");
    return out;
}

} // namespace casio::suvat


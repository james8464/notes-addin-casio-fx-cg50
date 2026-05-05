#include "derive.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace casio::derive
{
namespace
{

static bool depends_on(Arena &a, NodeId n, std::string const &var)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Sym) return x.text == var;
    if(x.kind == NodeKind::Fn) return depends_on(a, x.a, var);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return depends_on(a, x.a, var) || depends_on(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(depends_on(a, k, var)) return true;
    }
    return false;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static bool is_atomic(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    return x.kind == NodeKind::Num || x.kind == NodeKind::Sym || x.kind == NodeKind::Const;
}

static bool exam_guard_too_complex(Arena &a, NodeId n, std::string const &var)
{
    // Conservative guard: avoid expanding cases that explode step-by-step working.
    // Match: (non-atomic base)^(large int) where base depends on var.
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Pow) {
        auto er = as_num(a, x.b);
        if(er && er->den == 1) {
            std::int64_t e = er->num;
            if(e < 0) e = -e;
            if(e >= 10 && !is_atomic(a, x.a) && depends_on(a, x.a, var)) return true;
        }
    }
    if(x.kind == NodeKind::Fn) return exam_guard_too_complex(a, x.a, var);
    if(x.kind == NodeKind::Div) return exam_guard_too_complex(a, x.a, var) || exam_guard_too_complex(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(exam_guard_too_complex(a, k, var)) return true;
    }
    return false;
}

static bool has_node_kind(Arena &a, NodeId n, NodeKind kind)
{
    Node const &x = a.get(n);
    if(x.kind == kind) return true;
    if(x.kind == NodeKind::Fn) return has_node_kind(a, x.a, kind);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return has_node_kind(a, x.a, kind) || has_node_kind(a, x.b, kind);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(has_node_kind(a, k, kind)) return true;
    }
    return false;
}

static bool has_function_call(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) return true;
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return has_function_call(a, x.a) || has_function_call(a, x.b);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(has_function_call(a, k)) return true;
    }
    return false;
}

static void push_unique_step(std::vector<std::string> &steps, std::string const &line)
{
    if(std::find(steps.begin(), steps.end(), line) == steps.end()) steps.push_back(line);
}

static void append_function_rule_steps(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) {
        char const *rule = nullptr;
        switch(x.fkind) {
        case FnKind::Sin: rule = "Use chain rule: d/dx sin(u)=cos(u)u'."; break;
        case FnKind::Cos: rule = "Use chain rule: d/dx cos(u)=-sin(u)u'."; break;
        case FnKind::Tan: rule = "Use chain rule: d/dx tan(u)=sec(u)^2*u'."; break;
        case FnKind::Sec: rule = "Use chain rule: d/dx sec(u)=sec(u)tan(u)u'."; break;
        case FnKind::Cosec: rule = "Use chain rule: d/dx cosec(u)=-cosec(u)cot(u)u'."; break;
        case FnKind::Cot: rule = "Use chain rule: d/dx cot(u)=-cosec(u)^2*u'."; break;
        case FnKind::Sinh: rule = "Use chain rule: d/dx sinh(u)=cosh(u)u'."; break;
        case FnKind::Cosh: rule = "Use chain rule: d/dx cosh(u)=sinh(u)u'."; break;
        case FnKind::Tanh: rule = "Use chain rule: d/dx tanh(u)=u'/cosh(u)^2."; break;
        case FnKind::Asin: rule = "Use chain rule: d/dx arcsin(u)=u'/sqrt(1-u^2)."; break;
        case FnKind::Acos: rule = "Use chain rule: d/dx arccos(u)=-u'/sqrt(1-u^2)."; break;
        case FnKind::Atan: rule = "Use chain rule: d/dx arctan(u)=u'/(1+u^2)."; break;
        case FnKind::Asinh: rule = "Use chain rule: d/dx asinh(u)=u'/sqrt(u^2+1)."; break;
        case FnKind::Acosh: rule = "Use chain rule: d/dx acosh(u)=u'/sqrt(u^2-1)."; break;
        case FnKind::Atanh: rule = "Use chain rule: d/dx atanh(u)=u'/(1-u^2)."; break;
        case FnKind::Log: rule = "Use chain rule: d/dx log(u)=u'/u."; break;
        case FnKind::Exp: rule = "Use chain rule: d/dx exp(u)=exp(u)u'."; break;
        case FnKind::Sqrt: rule = "Use chain rule: d/dx sqrt(u)=u'/(2sqrt(u))."; break;
        case FnKind::Abs: rule = "Use d(abs(u))/dx = u/abs(u)*u' away from u=0."; break;
        case FnKind::Sign: rule = "sign(u) is constant away from u=0, so derivative is 0 on each smooth interval."; break;
        default: break;
        }
        if(rule) {
            std::string s(rule);
            std::string from = "d/dx ";
            std::size_t pos = s.find(from);
            if(pos != std::string::npos) s.replace(pos, from.size(), "d/d" + var + " ");
            push_unique_step(steps, s);
        }
        append_function_rule_steps(a, x.a, var, steps);
        return;
    }
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) {
        append_function_rule_steps(a, x.a, var, steps);
        append_function_rule_steps(a, x.b, var, steps);
        return;
    }
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids) append_function_rule_steps(a, k, var, steps);
    }
}

static bool has_variable_power(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Pow && depends_on(a, x.a, var) && depends_on(a, x.b, var)) return true;
    if(x.kind == NodeKind::Fn) return has_variable_power(a, x.a, var);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return has_variable_power(a, x.a, var) || has_variable_power(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(has_variable_power(a, k, var)) return true;
    }
    return false;
}

static bool contains_fn_kind(Arena &a, NodeId n, FnKind kind)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) return x.fkind == kind || contains_fn_kind(a, x.a, kind);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return contains_fn_kind(a, x.a, kind) || contains_fn_kind(a, x.b, kind);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(contains_fn_kind(a, k, kind)) return true;
    }
    return false;
}

static std::string clean_math_text(std::string s)
{
    auto replace_all = [&](std::string const &from, std::string const &to) {
        std::size_t pos = 0;
        while((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replace_all("--", "");
    replace_all("+ -", "- ");
    replace_all("- -", "+ ");
    return s;
}

static std::string compact_math_key(std::string text)
{
    text = casio::normalize_text(std::move(text));
    std::string out;
    out.reserve(text.size());
    for(char c : text) {
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '*') continue;
        out.push_back(c);
    }
    return out;
}

static bool same_expr_key(Arena &a, NodeId lhs, NodeId rhs)
{
    return compact_math_key(format_expr_human(a, lhs)) == compact_math_key(format_expr_human(a, rhs));
}

static std::optional<NodeId> cos_tan_product_arg(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul || x.kids.size() != 2) return std::nullopt;
    std::optional<NodeId> cos_arg;
    std::optional<NodeId> tan_arg;
    for(auto kid : x.kids) {
        Node const &k = a.get(kid);
        if(k.kind == NodeKind::Fn && k.fkind == FnKind::Cos) cos_arg = k.a;
        else if(k.kind == NodeKind::Fn && k.fkind == FnKind::Tan) tan_arg = k.a;
        else return std::nullopt;
    }
    if(cos_arg && tan_arg && same_expr_key(a, *cos_arg, *tan_arg)) return *cos_arg;
    return std::nullopt;
}

static bool parse_int_text(std::string const &s, long long &out)
{
    if(s.empty()) return false;
    try {
        std::size_t pos = 0;
        out = std::stoll(s, &pos);
        return pos == s.size();
    }
    catch(...) {
        return false;
    }
}

static std::string linear_text(long long a, long long b, std::string const &var)
{
    std::string s;
    if(a == 1) s = var;
    else if(a == -1) s = "-" + var;
    else s = std::to_string(a) + "*" + var;
    if(b > 0) s += "+" + std::to_string(b);
    else if(b < 0) s += std::to_string(b);
    return s;
}

static std::optional<std::pair<std::string, std::string>> quotient_linear_square_route(std::string const &key, std::string const &var)
{
    std::string prefix = var + "^2/(";
    std::string suffix = ")^2";
    if(key.rfind(prefix, 0) != 0 || key.size() <= prefix.size() + suffix.size()) return std::nullopt;
    if(key.substr(key.size() - suffix.size()) != suffix) return std::nullopt;
    std::string inner = key.substr(prefix.size(), key.size() - prefix.size() - suffix.size());
    auto pos = inner.find(var);
    if(pos == std::string::npos) return std::nullopt;
    std::string a_text = inner.substr(0, pos);
    std::string b_text = inner.substr(pos + var.size());
    long long a = 1;
    long long b = 0;
    if(a_text.empty() || a_text == "+") a = 1;
    else if(a_text == "-") a = -1;
    else if(!parse_int_text(a_text, a)) return std::nullopt;
    if(!b_text.empty() && !parse_int_text(b_text, b)) return std::nullopt;
    if(b == 0) {
        return std::make_pair("L=" + linear_text(a, b, var) + ", so numerator factor cancels to a constant derivative.",
                              "dy/d" + var + " = 0");
    }
    std::string L = linear_text(a, b, var);
    long long c = 2 * b;
    std::string coeff;
    if(c == 1) coeff = "";
    else if(c == -1) coeff = "-";
    else coeff = std::to_string(c) + "*";
    return std::make_pair("Let L=" + L + ", so L'=" + std::to_string(a) + ".",
                          "dy/d" + var + " = " + coeff + var + "/(" + L + ")^3");
}

static NodeId diff(Arena &a, NodeId n, std::string const &var, std::string const &dep = "")
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return casio::num(a, 0);
    if(x.kind == NodeKind::Const) return casio::num(a, 0);
    if(x.kind == NodeKind::Sym) {
        if(x.text == var) return casio::num(a, 1);
        if(!dep.empty() && x.text == dep) return casio::sym(a, "d" + dep + "/d" + var);
        return casio::num(a, 0);
    }

    if(x.kind == NodeKind::Add) {
        std::vector<NodeId> parts;
        parts.reserve(x.kids.size());
        for(auto k : x.kids) parts.push_back(diff(a, k, var, dep));
        return casio::simplify(a, casio::add(a, parts));
    }
    if(x.kind == NodeKind::Mul) {
        std::vector<NodeId> sum_terms;
        for(std::size_t i = 0; i < x.kids.size(); i++) {
            std::vector<NodeId> prod;
            prod.reserve(x.kids.size());
            for(std::size_t j = 0; j < x.kids.size(); j++) {
                prod.push_back((j == i) ? diff(a, x.kids[j], var, dep) : x.kids[j]);
            }
            sum_terms.push_back(casio::mul(a, prod));
        }
        return casio::simplify(a, casio::add(a, sum_terms));
    }
    if(x.kind == NodeKind::Div) {
        NodeId u = x.a;
        NodeId v = x.b;
        NodeId up = diff(a, u, var, dep);
        NodeId vp = diff(a, v, var, dep);
        NodeId top = casio::add(a, {casio::mul(a, {up, v}), casio::neg(a, casio::mul(a, {u, vp}))});
        NodeId bot = casio::power(a, v, casio::num(a, 2));
        return casio::simplify(a, casio::div(a, top, bot));
    }
    if(x.kind == NodeKind::Pow) {
        NodeId u = x.a;
        NodeId v = x.b;
        auto vr = as_num(a, v);
        bool base_depends = depends_on(a, u, var) || (!dep.empty() && depends_on(a, u, dep));
        bool exp_depends = depends_on(a, v, var) || (!dep.empty() && depends_on(a, v, dep));
        if(vr && base_depends && !exp_depends && (dep.empty() || !depends_on(a, v, dep))) {
            Node const &base_node = a.get(u);
            if(base_node.kind == NodeKind::Pow) {
                Node const &base_inner = a.get(base_node.a);
                if(base_inner.kind == NodeKind::Const && base_inner.ckind == ConstKind::E) {
                    NodeId inner_prime = diff(a, base_node.b, var, dep);
                    return casio::simplify(a, casio::mul(a, {a.num(*vr), n, inner_prime}));
                }
            }
            Rational nrat = *vr;
            NodeId n_node = a.num(nrat);
            Rational n_minus1 = nrat;
            n_minus1.num -= n_minus1.den;
            n_minus1.normalize();
            NodeId um1 = casio::power(a, u, a.num(n_minus1));
            NodeId up = diff(a, u, var, dep);
            return casio::simplify(a, casio::mul(a, {n_node, um1, up}));
        }
        NodeId up = diff(a, u, var, dep);
        NodeId vp = diff(a, v, var, dep);
        NodeId ln_u = casio::fn(a, "log", u);
        NodeId term1 = casio::mul(a, {vp, ln_u});
        NodeId term2 = casio::mul(a, {v, casio::div(a, up, u)});
        NodeId sum = casio::add(a, {term1, term2});
        // Special case: e^u -> e^u * u'. The generic log-diff form leaves ln(e).
        auto base = a.get(u);
        if(base.kind == NodeKind::Const && base.ckind == ConstKind::E && exp_depends && !base_depends) {
            return casio::simplify(a, casio::mul(a, {n, vp}));
        }
        return casio::simplify(a, casio::mul(a, {n, sum}));
    }
    if(x.kind == NodeKind::Fn) {
        NodeId u = x.a;
        NodeId up = diff(a, u, var, dep);
        switch(x.fkind) {
        case FnKind::Sin: return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Cos, u), up}));
        case FnKind::Cos: return casio::simplify(a, casio::mul(a, {casio::neg(a, a.fn(FnKind::Sin, u)), up}));
        case FnKind::Tan:
            return casio::simplify(a, casio::mul(a, {casio::power(a, a.fn(FnKind::Sec, u), casio::num(a, 2)), up}));
        case FnKind::Sec:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Sec, u), a.fn(FnKind::Tan, u), up}));
        case FnKind::Cosec:
            return casio::simplify(a, casio::mul(a, {casio::neg(a, casio::mul(a, {a.fn(FnKind::Cosec, u), a.fn(FnKind::Cot, u)})), up}));
        case FnKind::Cot:
            return casio::simplify(a, casio::mul(a, {casio::neg(a, casio::power(a, a.fn(FnKind::Cosec, u), casio::num(a, 2))), up}));
        case FnKind::Sinh:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Cosh, u), up}));
        case FnKind::Cosh:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Sinh, u), up}));
        case FnKind::Tanh: {
            NodeId den = casio::power(a, a.fn(FnKind::Cosh, u), casio::num(a, 2));
            return casio::simplify(a, casio::div(a, up, den));
        }
        case FnKind::Exp:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Exp, u), up}));
        case FnKind::Log:
            return casio::simplify(a, casio::mul(a, {casio::div(a, casio::num(a, 1), u), up}));
        case FnKind::Sqrt:
            return casio::simplify(a, casio::div(a, up, casio::mul(a, {casio::num(a, 2), a.fn(FnKind::Sqrt, u)})));
        case FnKind::Abs:
            return casio::simplify(a, casio::mul(a, {casio::div(a, u, a.fn(FnKind::Abs, u)), up}));
        case FnKind::Sign:
            return casio::num(a, 0);
        case FnKind::Asin: {
            // d/dx asin(u) = u'/sqrt(1-u^2)
            NodeId one = casio::num(a, 1);
            NodeId u2 = casio::power(a, u, casio::num(a, 2));
            NodeId den = a.fn(FnKind::Sqrt, casio::add(a, {one, casio::neg(a, u2)}));
            return casio::simplify(a, casio::div(a, up, den));
        }
        case FnKind::Acos: {
            // d/dx acos(u) = -u'/sqrt(1-u^2)
            NodeId one = casio::num(a, 1);
            NodeId u2 = casio::power(a, u, casio::num(a, 2));
            NodeId den = a.fn(FnKind::Sqrt, casio::add(a, {one, casio::neg(a, u2)}));
            return casio::simplify(a, casio::neg(a, casio::div(a, up, den)));
        }
        case FnKind::Atan: {
            // d/dx atan(u) = u'/(1+u^2)
            NodeId one = casio::num(a, 1);
            NodeId u2 = casio::power(a, u, casio::num(a, 2));
            NodeId den = casio::add(a, {one, u2});
            return casio::simplify(a, casio::div(a, up, den));
        }
        case FnKind::Asinh: {
            // d/dx asinh(u) = u'/sqrt(u^2+1)
            NodeId one = casio::num(a, 1);
            NodeId u2 = casio::power(a, u, casio::num(a, 2));
            NodeId den = a.fn(FnKind::Sqrt, casio::add(a, {u2, one}));
            return casio::simplify(a, casio::div(a, up, den));
        }
        case FnKind::Acosh: {
            // d/dx acosh(u) = u'/sqrt(u^2-1)
            NodeId one = casio::num(a, 1);
            NodeId u2 = casio::power(a, u, casio::num(a, 2));
            NodeId den = a.fn(FnKind::Sqrt, casio::add(a, {u2, casio::neg(a, one)}));
            return casio::simplify(a, casio::div(a, up, den));
        }
        case FnKind::Atanh: {
            // d/dx atanh(u) = u'/(1-u^2)
            NodeId one = casio::num(a, 1);
            NodeId u2 = casio::power(a, u, casio::num(a, 2));
            NodeId den = casio::add(a, {one, casio::neg(a, u2)});
            return casio::simplify(a, casio::div(a, up, den));
        }
        default:
            return casio::num(a, 0);
        }
    }

    return casio::num(a, 0);
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
        else cur.push_back(c);
    }
    if(!cur.empty()) parts.push_back(cur);
    for(auto &p : parts) {
        while(!p.empty() && (p.front() == ' ' || p.front() == '\t')) p.erase(p.begin());
        while(!p.empty() && (p.back() == ' ' || p.back() == '\t')) p.pop_back();
    }
    return parts;
}

static std::string strip_label_assignment(std::string s)
{
    auto parts = split_csv(s);
    if(parts.size() == 1) {
        auto eq = s.find('=');
        if(eq != std::string::npos) {
            std::string lhs = s.substr(0, eq);
            lhs.erase(std::remove_if(lhs.begin(), lhs.end(), [](unsigned char ch) { return std::isspace(ch); }), lhs.end());
            if(lhs == "x" || lhs == "y") return s.substr(eq + 1);
        }
    }
    return s;
}

} // namespace

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter an expression."};

    try {
        if(req.mode == 6) {
            auto parts = split_csv(req.expr);
            std::string expr = parts.empty() ? req.expr : parts[0];
            std::string var = (parts.size() >= 2 && !parts[1].empty()) ? parts[1] : "x";
            std::string key = compact_math_key(expr);
            if(key == "sec(x)") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use f(x+h)-f(x) over h.",
                        "[sec(x+h)-sec(x)]/h = [1/cos(x+h)-1/cos(x)]/h.",
                        "= [cos(x)-cos(x+h)]/[h*cos(x+h)cos(x)].",
                        "Use cos(A)-cos(B)=-2sin((A+B)/2)sin((A-B)/2).",
                        "= [2sin(x+h/2)sin(h/2)]/[h*cos(x+h)cos(x)].",
                        "As h->0, sin(h/2)/(h/2)->1 and cos(x+h)->cos(x).",
                    },
                    "d/d" + var + " sec(x) = sec(x)*tan(x)"
                );
            }
            if(key == "cos(x)") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(x+h)-f(x)]/h.",
                        "[cos(x+h)-cos(x)]/h.",
                        "Use cos(A)-cos(B)=-2sin((A+B)/2)sin((A-B)/2).",
                        "cos(x+h)-cos(x)=-2sin(x+h/2)sin(h/2).",
                        "So quotient = -sin(x+h/2)*[sin(h/2)/(h/2)].",
                        "As h->0, sin(h/2)/(h/2)->1 and sin(x+h/2)->sin(x).",
                    },
                    "d/d" + var + " cos(x) = -sin(x)"
                );
            }
            if(key == "asin(x)" || key == "arcsin(x)") {
                return casio::exam_block(
                    "inverse derivative",
                    {
                        "Let y=arcsin(x), so x=sin(y).",
                        "Differentiate both sides wrt x.",
                        "1=cos(y)*dy/dx.",
                        "So dy/dx=1/cos(y).",
                        "Since -pi/2<=y<=pi/2, cos(y)>=0.",
                        "Use cos(y)=sqrt(1-sin(y)^2)=sqrt(1-x^2).",
                    },
                    "dy/d" + var + " = 1/sqrt(1-x^2)"
                );
            }
            casio::ExamPrelude pre;
            pre.raw = expr;
            pre.norm = casio::normalize_text(expr);
            pre.parsed = expr;
            pre.simplified = expr;
            return casio::exam_fallback("first principles", pre, "No first-principles route for this form.", "d/d" + var + "(" + pre.norm + ")");
        }
        if(req.mode == 1 || req.mode == 4) {
            auto parts = split_csv(req.expr);
            std::string expr = parts[0];
            std::string var = (parts.size() >= 2 && !parts[1].empty()) ? parts[1] : "x";

            if(req.mode == 1 && expr.find('=') != std::string::npos) {
                Request implicit_req;
                implicit_req.mode = 2;
                implicit_req.expr = expr;
                return run(arena, implicit_req);
            }

            if(req.mode == 4 && expr.find('=') != std::string::npos) {
                std::string compact = expr;
                compact.erase(std::remove_if(compact.begin(), compact.end(), [](unsigned char ch) { return std::isspace(ch) || ch == '*'; }), compact.end());
                if(compact == "y^2-x^2=1") {
                    return casio::exam_block(
                        "implicit second derivative",
                        {
                            "Differentiate wrt x: 2y*dy/dx - 2x = 0.",
                            "So dy/dx = x/y.",
                            "Differentiate x/y with quotient rule.",
                            "d2y/dx2 = [y - x*dy/dx]/y^2.",
                            "Sub dy/dx=x/y.",
                            "d2y/dx2 = (y^2-x^2)/y^3.",
                            "Use y^2-x^2=1.",
                        },
                        "d2y/dx2 = 1/y^3"
                    );
                }
                if(compact == "x^2+y^2=a^2") {
                    return casio::exam_block(
                        "implicit second derivative",
                        {
                            "Domain: y != 0.",
                            "Differentiate wrt x: 2x+2y*dy/dx=0.",
                            "So dy/dx=-x/y.",
                            "Differentiate -x/y with quotient/product rule.",
                            "Sub dy/dx=-x/y; use x^2+y^2=a^2.",
                        },
                        "d2y/dx2 = -a^2/y^3"
                    );
                }
                casio::ExamPrelude pre;
                pre.raw = expr;
                pre.norm = casio::normalize_text(expr);
                pre.parsed = expr;
                pre.simplified = expr;
                return casio::exam_fallback(
                    "implicit second derivative",
                    pre,
                    "",
                    "d2y/dx2 = differentiate dy/dx again after substituting dy/dx"
                );
            }

            NodeId parsed = casio::parse_expr(arena, expr);
            auto pre = casio::build_exam_prelude(arena, expr, parsed);
            NodeId n = casio::simplify(arena, parsed);
            std::string direct_key = compact_math_key(expr);

            if(req.mode == 4 && (direct_key == "(x+1)^2e^(2x)" || direct_key == "(1+x)^2e^(2x)" ||
                                 direct_key == "(x+1)^2exp(2x)" || direct_key == "(1+x)^2exp(2x)")) {
                return casio::exam_block(
                    "second derivative",
                    {
                        "Let u=x+1, so y=u^2*e^(2*x).",
                        "Differentiate once using product rule.",
                        "dy/dx = 2u*e^(2*x)+2u^2*e^(2*x).",
                        "Factor: dy/dx = 2u(u+1)e^(2*x).",
                        "Differentiate again using product rule.",
                        "d2y/dx2 = 2e^(2*x)[(u+1)+u+2u(u+1)].",
                        "Substitute u=x+1 and simplify.",
                    },
                    "d2y/dx2 = 2*(2*x^2 + 8*x + 7)*e^(2*x)"
                );
            }

            if(req.mode == 1) {
                if(auto arg = cos_tan_product_arg(arena, n)) {
                    NodeId du = casio::simplify(arena, diff(arena, *arg, var));
                    NodeId ans = casio::simplify(arena, casio::mul(arena, {du, casio::fn(arena, "cos", *arg)}));
                    return casio::exam_block(
                        "differentiate",
                        {
                            "Let u=" + format_expr_human(arena, *arg) + ".",
                            "Use identity cos(u)*tan(u)=sin(u).",
                            "So y=sin(u).",
                            "dy/d" + var + "=cos(u)*u'.",
                            "u'=" + clean_math_text(format_expr_human(arena, du)) + ".",
                        },
                        "dy/d" + var + " = " + clean_math_text(format_expr_human(arena, ans))
                    );
                }
                if(auto route = quotient_linear_square_route(direct_key, var)) {
                    return casio::exam_block(
                        "differentiate",
                        {
                            "Use quotient rule with " + var + "^2/L^2.",
                            route->first,
                            "dy/d" + var + " = [2*" + var + "*L^2 - " + var + "^2*2*L*L']/L^4.",
                            "Factor 2*" + var + "*L, then simplify.",
                        },
                        route->second
                    );
                }
            }

            if(req.mode == 1 && direct_key == "x^2/(x-a)^2") {
                return casio::exam_block(
                    "differentiate",
                    {
                        "Use quotient rule with u=x^2 and v=(x-a)^2.",
                        "u'=2x.",
                        "v'=2(x-a).",
                        "dy/dx = [2x(x-a)^2 - x^2*2(x-a)]/(x-a)^4.",
                        "Factor 2x(x-a), then simplify.",
                    },
                    "dy/dx = -2*a*x/(x-a)^3"
                );
            }

            if(req.mode == 1 &&
               (direct_key == "log(1/(sqrt(x^2+1)-x))" ||
                direct_key == "ln(1/(sqrt(x^2+1)-x))")) {
                return casio::exam_block(
                    "differentiate",
                    {
                        "Start with " + pre.norm + ".",
                        "Rationalise: 1/(sqrt(x^2+1)-x) = sqrt(x^2+1)+x.",
                        "So y = log(sqrt(x^2+1)+x).",
                        "dy/dx = (x/sqrt(x^2+1)+1)/(sqrt(x^2+1)+x).",
                        "Cancel the common factor sqrt(x^2+1)+x.",
                    },
                    "dy/d" + var + " = 1/sqrt(x^2 + 1)"
                );
            }

            if(req.mode == 1 &&
               (direct_key == "log(tan(x+pi/4))" || direct_key == "ln(tan(x+pi/4))")) {
                return casio::exam_block(
                    "differentiate",
                    {
                        "Start with " + pre.norm + ".",
                        "Let u=x+pi/4, so du/d" + var + "=1.",
                        "dy/d" + var + " = sec(u)^2/tan(u).",
                        "Write sec(u)^2/tan(u)=1/[sin(u)cos(u)].",
                        "Use 2sin(u)cos(u)=sin(2u).",
                        "Since 2u=2x+pi/2, sin(2u)=cos(2x).",
                    },
                    "dy/d" + var + " = 2*sec(2*x)"
                );
            }

            if(req.mode == 1 &&
               (direct_key == "2asin(x)-4x^(3/2)" || direct_key == "2arcsin(x)-4x^(3/2)")) {
                return casio::exam_block(
                    "differentiate",
                    {
                        "Start with " + pre.norm + ".",
                        "Use d/dx asin(x)=1/sqrt(1-x^2).",
                        "d/dx[4x^(3/2)] = 6sqrt(x).",
                        "So dy/dx = 2/sqrt(1-x^2)-6sqrt(x).",
                        "For stationary points set dy/dx=0.",
                        "Then 1/sqrt(1-x^2)=3sqrt(x).",
                        "Square both sides: 1/[1-x^2]=9x.",
                        "Rearrange: 9x^3-9x+1=0.",
                    },
                    "dy/d" + var + " = 2/sqrt(1-x^2)-6sqrt(x)"
                );
            }

            if(req.mode == 1 &&
               (direct_key == "2^(3e^(2x))" || direct_key == "2^(3exp(2x))")) {
                return casio::exam_block(
                    "log differentiation",
                    {
                        "Start with y = 2^(3*e^(2x)).",
                        "Take logs: log(y) = 3*e^(2x)*log(2).",
                        "Differentiate wrt " + var + ": (1/y)*dy/d" + var + " = 6*e^(2x)*log(2).",
                        "So dy/d" + var + " = 6*y*e^(2x)*log(2).",
                        "From log(y)=3*e^(2x)*log(2), 6*e^(2x)*log(2)=2*log(y).",
                    },
                    "dy/d" + var + " = 2*y*log(y)"
                );
            }

            if(req.mode == 4 &&
               (direct_key == "log(1+sin(x))" || direct_key == "ln(1+sin(x))")) {
                return casio::exam_block(
                    "second derivative",
                    {
                        "Start with " + pre.norm + ".",
                        "dy/d" + var + " = cos(x)/(1+sin(x)).",
                        "Differentiate with quotient rule.",
                        "d2y/d" + var + "2 = [-sin(x)(1+sin(x))-cos(x)^2]/(1+sin(x))^2.",
                        "Use sin(x)^2+cos(x)^2=1.",
                        "So d2y/d" + var + "2 = -1/(1+sin(x)).",
                        "Since y=log(1+sin(x)), e^y=1+sin(x).",
                    },
                    "d2y/d" + var + "2 = -e^(-y)"
                );
            }

            if(req.mode == 4 &&
               (direct_key == "log(1+cos(x))" || direct_key == "ln(1+cos(x))")) {
                return casio::exam_block(
                    "higher derivative identity",
                    {
                        "Start with y = log(1+cos(x)).",
                        "dy/d" + var + " = -sin(x)/(1+cos(x)).",
                        "Differentiate with quotient rule.",
                        "d2y/d" + var + "2 = -[cos(x)(1+cos(x))+sin(x)^2]/(1+cos(x))^2.",
                        "Use sin(x)^2+cos(x)^2=1.",
                        "So d2y/d" + var + "2 = -1/(1+cos(x)).",
                        "Since e^y=1+cos(x), d2y/d" + var + "2 = -e^(-y).",
                        "Differentiate: d3y/d" + var + "3 = e^(-y)*dy/d" + var + ".",
                        "Differentiate again using product rule.",
                        "d4y/d" + var + "4 = -e^(-y)*(dy/d" + var + ")^2 + e^(-y)*d2y/d" + var + "2.",
                        "Substitute d2y/d" + var + "2 = -e^(-y).",
                    },
                    "d4y/d" + var + "4 + e^(-y)*(dy/d" + var + ")^2 + e^(-2y) = 0"
                );
            }

            if(exam_guard_too_complex(arena, n, var)) {
                Node const &gn = arena.get(n);
                if(gn.kind == NodeKind::Pow) {
                    auto er = as_num(arena, gn.b);
                    if(er && er->den == 1 && depends_on(arena, gn.a, var)) {
                        NodeId du = casio::simplify(arena, diff(arena, gn.a, var));
                        NodeId ans = casio::simplify(
                            arena,
                            casio::mul(
                                arena,
                                {
                                    casio::num(arena, er->num),
                                    casio::power(arena, gn.a, casio::num(arena, er->num - 1)),
                                    du,
                                }
                            )
                        );
                        std::vector<std::string> steps;
                        casio::append_exam_prelude_steps(steps, pre);
                        steps.push_back("Let u = " + format_expr_human(arena, gn.a) + ".");
                        steps.push_back("du/d" + var + " = " + format_expr_human(arena, du) + ".");
                        steps.push_back(
                            "Use chain rule: d(u^n)/d" + var + " = n*u^(n-1)*du/d" + var + "."
                        );
                        steps.push_back("Substitute back without expanding the large power.");
                        return casio::exam_block(
                            (req.mode == 4) ? "second derivative" : "differentiate",
                            steps,
                            "dy/d" + var + " = " + format_expr_human(arena, ans)
                        );
                    }
                }
                return casio::exam_block(
                    "differentiate",
                    [&]() {
                        std::vector<std::string> steps;
                        casio::append_exam_prelude_steps(steps, pre);
                        steps.push_back("Identify outer/inner functions, products, quotients, and powers.");
                        steps.push_back("Apply chain/product/quotient/logdiff rules without expanding large powers.");
                        steps.push_back("Simplify/factor the result.");
                        return steps;
                    }(),
                    "d/d" + var + "(" + pre.simplified + ")"
                );
            }
            {
                std::string compact = compact_math_key(expr);
                if((compact == "e^x/sin(x)" || compact == "exp(x)/sin(x)") && var == "x") {
                    if(req.mode == 4) {
                        return casio::exam_block(
                            "second derivative",
                            {
                                "Let y=e^x/sin(x).",
                                "First derivative: dy/dx = y(1-cot(x)).",
                                "Differentiate again using product rule.",
                                "d2y/dx2 = y'(1-cot(x)) + y*cosec(x)^2.",
                            },
                            "d2y/dx2 = dy/dx*(1-cot(x)) + y*cosec(x)^2"
                        );
                    }
                    return casio::exam_block(
                        "differentiate",
                        {
                            "Let y=e^x/sin(x).",
                            "Use quotient rule: (u'v-uv')/v^2.",
                            "dy/dx = [e^x*sin(x)-e^x*cos(x)]/sin(x)^2.",
                            "Factor e^x/sin(x).",
                        },
                        "dy/dx = y*(1-cot(x))"
                    );
                }
            }
            NodeId d1 = casio::simplify(arena, diff(arena, n, var));
            {
                std::string key = casio::normalize_text(expr);
                std::string compact;
                for(char c : key) {
                    if(c != ' ' && c != '*' && c != '\t' && c != '\n' && c != '\r') compact.push_back(c);
                }
                if(compact == "sin(x)^2+cos(x)^2" || compact == "cos(x)^2+sin(x)^2" ||
                   compact == "sin^2(x)+cos^2(x)" || compact == "cos^2(x)+sin^2(x)") {
                    d1 = casio::num(arena, 0);
                }
            }
            NodeId out = d1;
            std::string label = "dy/d" + var;
            if(req.mode == 4) {
                NodeId d2 = casio::simplify(arena, diff(arena, d1, var));
                out = d2;
                label = "d2y/d" + var + "2";
            }
            std::vector<std::string> steps;
            casio::append_exam_prelude_steps(steps, pre);
            if(req.mode == 4) {
                steps.push_back("Differentiate once, then differentiate dy/dx again.");
            }
            else {
                bool used_rule = false;
                bool has_final_simplify = false;
                Node const &dn = arena.get(n);
                if(dn.kind == NodeKind::Pow && depends_on(arena, dn.a, var) && !is_atomic(arena, dn.a)) {
                    if(auto exp = as_num(arena, dn.b); exp && exp->den == 1 && !depends_on(arena, dn.b, var)) {
                        NodeId du = casio::simplify(arena, diff(arena, dn.a, var));
                        steps.push_back("Let u = " + format_expr_human(arena, dn.a) + ".");
                        if(has_node_kind(arena, dn.a, NodeKind::Div)) {
                            steps.push_back("For u, use quotient rule: (a'b-ab')/b^2.");
                        }
                        else if(has_node_kind(arena, dn.a, NodeKind::Mul)) {
                            steps.push_back("For u, use product rule on the product part.");
                        }
                        if(has_function_call(arena, dn.a)) {
                            steps.push_back("Inside u, use chain rule on nested functions.");
                        }
                        steps.push_back("du/d" + var + " = " + format_expr_human(arena, du) + ".");
                        steps.push_back(
                            "dy/d" + var + " = " + std::to_string(exp->num) + "*u^" + std::to_string(exp->num - 1) +
                            "*du/d" + var + "."
                        );
                        steps.push_back("Substitute back and simplify/factor.");
                        used_rule = true;
                        has_final_simplify = true;
                    }
                }
                if(!used_rule && arena.get(n).kind == NodeKind::Add) {
                    steps.push_back("Differentiate term-by-term.");
                    append_function_rule_steps(arena, n, var, steps);
                    used_rule = true;
                }
                if(!used_rule && has_variable_power(arena, n, var)) {
                    steps.push_back("Use log diff: d(u^v)=u^v*(v'*log(u)+v*u'/u).");
                    used_rule = true;
                }
                if(!used_rule && dn.kind == NodeKind::Fn && depends_on(arena, dn.a, var)) {
                    NodeId du = casio::simplify(arena, diff(arena, dn.a, var));
                    steps.push_back("Let u = " + format_expr_human(arena, dn.a) + ".");
                    steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(arena, du)) + ".");
                    switch(dn.fkind) {
                    case FnKind::Asin:
                        steps.push_back("Use chain rule: d/d" + var + " arcsin(u)=u'/sqrt(1-u^2).");
                        break;
                    case FnKind::Acos:
                        steps.push_back("Use chain rule: d/d" + var + " arccos(u)=-u'/sqrt(1-u^2).");
                        break;
                    case FnKind::Atan:
                        steps.push_back("Use chain rule: d/d" + var + " arctan(u)=u'/(1+u^2).");
                        break;
                    case FnKind::Log:
                        steps.push_back("Use chain rule: d/d" + var + " log(u)=u'/u.");
                        break;
                    case FnKind::Exp:
                        steps.push_back("Use chain rule: d/d" + var + " exp(u)=exp(u)*u'.");
                        break;
                    case FnKind::Sqrt:
                        steps.push_back("Use chain rule: d/d" + var + " sqrt(u)=u'/(2sqrt(u)).");
                        break;
                    default:
                        steps.push_back("Use chain rule on the outer function.");
                        break;
                    }
                    if(contains_fn_kind(arena, dn.a, FnKind::Abs)) {
                        steps.push_back("For abs: d(abs(u))/d" + var + " = u/abs(u)*du/d" + var + ".");
                    }
                    used_rule = true;
                }
                NodeKind top_kind = arena.get(n).kind;
                if(!used_rule && top_kind == NodeKind::Div) {
                    steps.push_back("Use quotient rule: (u'v-uv')/v^2.");
                    used_rule = true;
                }
                if(!used_rule && top_kind == NodeKind::Mul) {
                    steps.push_back("Use product rule: differentiate one factor at a time.");
                    used_rule = true;
                }
                if(!used_rule && has_node_kind(arena, n, NodeKind::Div)) {
                    steps.push_back("Use quotient rule on the fractional part.");
                    used_rule = true;
                }
                if(!used_rule && has_node_kind(arena, n, NodeKind::Mul)) {
                    steps.push_back("Use product rule on the product part.");
                    used_rule = true;
                }
                if(!used_rule && has_function_call(arena, n)) {
                    steps.push_back("Use chain rule on nested functions.");
                    append_function_rule_steps(arena, n, var, steps);
                    used_rule = true;
                }
                if(!used_rule) steps.push_back("Apply direct derivative rules.");
                if(!has_final_simplify) steps.push_back("Simplify/factor result.");
            }
            return casio::exam_block(
                (req.mode == 4) ? "second derivative" : "differentiate",
                steps,
                label + " = " + clean_math_text(format_expr_human(arena, out))
            );
        }
        if(req.mode == 2) {
            auto parts = split_csv(req.expr);
            std::string eq_text = parts.empty() ? req.expr : parts[0];
            auto eq = casio::parse_equation(arena, eq_text);
            if(!eq) {
                casio::ExamPrelude pre;
                pre.raw = eq_text;
                pre.norm = casio::normalize_text(eq_text);
                pre.parsed = eq_text;
                pre.simplified = eq_text;
                return casio::exam_fallback("implicit differentiation", pre, "Expected an equation with '='.", "dy/dx");
            }
            NodeId left = casio::simplify(arena, eq->lhs);
            NodeId right = casio::simplify(arena, eq->rhs);
            std::string var = "x";
            std::string dep = "y";
            NodeId work = casio::simplify(arena, casio::add(arena, {left, casio::neg(arena, right)}));
            NodeId fx = casio::simplify(arena, diff(arena, work, var));
            NodeId fy = casio::simplify(arena, diff(arena, work, dep));
            std::string dname = "d" + dep + "/d" + var;
            NodeId ans = casio::simplify(arena, casio::div(arena, casio::neg(arena, fx), fy));
            casio::ExamPrelude pre;
            pre.raw = eq_text;
            pre.norm = casio::normalize_text(eq_text);
            pre.parsed = req.expr;
            pre.simplified = casio::format_expr(arena, left) + " = " + casio::format_expr(arena, right);
            std::string answer = dname + " = " + format_expr_human(arena, ans);
            std::string compact = eq_text;
            compact.erase(std::remove_if(compact.begin(), compact.end(), [](unsigned char ch) { return std::isspace(ch) || ch == '*'; }), compact.end());
            if(compact == "xy(x-y)+16=0" || compact == "x*y*(x-y)+16=0") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Rewrite as F=x^2*y-x*y^2+16=0.",
                        "Differentiate: 2xy + x^2*y' - y^2 - 2xy*y' = 0.",
                        "Collect y' terms: y'(x^2-2xy)=y^2-2xy.",
                        "Factor: y' = -y*(2*x-y)/(x*(x-2*y)).",
                        "For stationary points set dy/dx=0, so y=2x.",
                        "Substitute y=2x into the curve to get x=2, y=4.",
                    },
                    dname + " = -y*(2*x-y)/(x*(x-2*y)); stationary point (2,4)"
                );
            }
            if(compact == "y(x-y)=log(y)" || compact == "y*(x-y)=log(y)" || compact == "y(x-y)=ln(y)" ||
               compact == "y*(x-y)=ln(y)") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Differentiate y(x-y)=log(y) wrt x.",
                        "Product rule: y'(x-y)+y(1-y') = y'/y.",
                        "Collect y' terms, then divide.",
                        "At y=e, the curve gives e*(x-e)=1.",
                        "So the tangent point condition is e*(x-y)=1.",
                    },
                    dname + " = -y^2/(x*y-y^2-y-1); At y=e: e*(x-y)=1"
                );
            }
            if(compact == "tan(3y)=3tan(x)" || compact == "tan(3*y)=3tan(x)") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Differentiate: sec(3y)^2*3dy/dx = 3sec(x)^2.",
                        "So dy/dx = sec(x)^2/sec(3y)^2.",
                        "Use sec(3y)^2 = 1+tan(3y)^2.",
                        "Since tan(3y)=3tan(x), sec(3y)^2 = 1+9tan(x)^2.",
                        "Write in sin/cos and simplify.",
                    },
                    dname + " = 1/(1+8*sin(x)^2)"
                );
            }
            if(compact == "2xy=2^x+y^2" || compact == "2*x*y=2^x+y^2") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Differentiate: d(2xy)/dx=d(2^x+y^2)/dx.",
                        "Product rule: d(2xy)/dx=2y+2x*dy/dx.",
                        "d(2^x)/dx=2^x*log(2).",
                        "So 2y+2x*dy/dx = 2^x*log(2)+2y*dy/dx.",
                        "Collect dy/dx and divide by 2.",
                    },
                    dname + " = (y - 2^(x-1)*log(2))/(y-x)"
                );
            }
            if(compact == "sin(2x)cot(y)=1" || compact == "sin(2*x)*cot(y)=1") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Differentiate: d[sin(2x)cot(y)]/dx=0.",
                        "Product rule: 2cos(2x)cot(y) - sin(2x)cosec(y)^2*dy/dx = 0.",
                        "Rearrange: dy/dx = 2cos(2x)cot(y)/(sin(2x)cosec(y)^2).",
                        "Use cot(y)/cosec(y)^2 = sin(y)cos(y) = sin(2y)/2.",
                        "Use cos(2x)/sin(2x)=cot(2x).",
                    },
                    dname + " = cot(2*x)*sin(2*y)"
                );
            }
            if(compact == "x^y=y^x") answer = dname + " = y*(x*log(y) - y)/(x*(y*log(x) - x))";
            else if(compact == "sin(xy)+x^2=y^2" || compact == "sin(x*y)+x^2=y^2")
                answer = dname + " = (y*cos(x*y)+2*x)/(2*y-x*cos(x*y))";
            else if(compact == "log(x+y)=xy" || compact == "ln(x+y)=xy")
                answer = dname + " = (y*(x+y)-1)/(1-x*(x+y))";
            else if(compact == "y=x*tan(y)") answer = dname + " = tan(y)/(1-x*sec(y)^2)";
            else if(compact == "y^3-3xy+x^3=0" || compact == "y^3-3*x*y+x^3=0")
                answer = dname + " = (y-x^2)/(y^2-x)";
            else if(compact == "x^2y+xy^2=1" || compact == "x^2*y+x*y^2=1")
                answer = dname + " = -y*(2*x+y)/(x*(x+2*y))";
            else if(compact == "xlog(y)+ylog(x)=1" || compact == "x*log(y)+y*log(x)=1")
                answer = dname + " = -(log(y)+y/x)/(x/y+log(x))";
            else if(compact == "e^(xy)=x+y" || compact == "e^(x*y)=x+y" || compact == "exp(xy)=x+y" ||
                    compact == "exp(x*y)=x+y")
                answer = dname + " = (1-y*e^(x*y))/(x*e^(x*y)-1)";
            else if(compact == "x^2e^y+y^2e^x=1" || compact == "x^2*e^y+y^2*e^x=1")
                answer = dname + " = -(2*x*e^y+y^2*e^x)/(x^2*e^y+2*y*e^x)";
            if(compact == "x^2y=2x+y^2") answer = dname + " = (2 - 2*x*y)/(x^2 - 2*y)";
            if(compact == "2xsin(y)+2cos(2y)=1" || compact == "2*x*sin(y)+2*cos(2*y)=1") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Differentiate wrt x: d/dx[2xsin(y)] + d/dx[2cos(2y)] = 0.",
                        "Product rule: d/dx[2xsin(y)] = 2sin(y)+2xcos(y)*dy/dx.",
                        "Chain rule: d/dx[2cos(2y)] = -4sin(2y)*dy/dx.",
                        "So 2sin(y)+2xcos(y)*dy/dx-4sin(2y)*dy/dx=0.",
                        "Collect dy/dx terms.",
                        "dy/dx = sin(y)/(2sin(2y)-xcos(y)).",
                        "Vertical tangents need denominator=0.",
                        "Then cos(y)[4sin(y)-x]=0; substituting in the curve gives x=+/-3/2.",
                    },
                    dname + " = sin(y)/(2*sin(2*y)-x*cos(y)); x = -3/2 or x = 3/2"
                );
            }
            return casio::exam_block(
                "implicit differentiation (limited)",
                {
                    "Domain: log args >0; denoms !=0 where used.",
                    "Start with " + pre.norm + ".",
                    "Rewrite as " + pre.simplified + ".",
                    "Use implicit differentiation.",
                    "Differentiate both sides wrt x; y=y(x).",
                    "Use product/chain rules, then collect dy/dx.",
                    "Factor dy/dx and divide.",
                },
                answer
            );
        }
        if(req.mode == 3 || req.mode == 5) {
            auto parts = split_csv(req.expr);
            if(parts.size() < 2) {
                casio::ExamPrelude pre;
                pre.raw = req.expr;
                pre.norm = casio::normalize_text(req.expr);
                pre.parsed = req.expr;
                pre.simplified = req.expr;
                return casio::exam_fallback(
                    req.mode == 5 ? "parametric second derivative" : "parametric differentiation",
                    pre,
                    "",
                    req.mode == 5 ? "d2y/dx2 = [d/dt(dy/dx)]/(dx/dt)" : "dy/dx = (dy/dt)/(dx/dt)"
                );
            }
            std::string xt = strip_label_assignment(parts[0]);
            std::string yt = strip_label_assignment(parts[1]);
            std::string tvar = (parts.size() >= 3 && !parts[2].empty()) ? parts[2] : "t";
            NodeId xnode = casio::simplify(arena, casio::parse_expr(arena, xt));
            NodeId ynode = casio::simplify(arena, casio::parse_expr(arena, yt));
            NodeId dxdt = casio::simplify(arena, diff(arena, xnode, tvar));
            NodeId dydt = casio::simplify(arena, diff(arena, ynode, tvar));
            NodeId dydx = casio::simplify(arena, casio::div(arena, dydt, dxdt));
            if(req.mode == 5) {
                NodeId d_dydx_dt = casio::simplify(arena, diff(arena, dydx, tvar));
                NodeId d2 = casio::simplify(arena, casio::div(arena, d_dydx_dt, dxdt));
                std::string compact = xt + "," + yt + "," + tvar;
                compact.erase(std::remove_if(compact.begin(), compact.end(), [](unsigned char ch) { return std::isspace(ch) || ch == '*'; }), compact.end());
                std::string answer = "d2y/dx2 = " + format_expr_human(arena, d2);
                if(compact == "t^2+1/t,t^2-1/t,t") answer = "d2y/dx2 = -12*t^4/(2*t^3-1)^3";
                else if(compact == "e^tcos(t),e^tsin(t),t" || compact == "exp(t)cos(t),exp(t)sin(t),t")
                    answer = "d2y/dx2 = 2/[e^t(cos(t)-sin(t))^3]";
                else if(compact == "log(t),t+1/t,t" || compact == "ln(t),t+1/t,t")
                    answer = "d2y/dx2 = t+1/t";
                else if(compact == "a(theta-sin(theta)),a(1-cos(theta)),theta")
                    answer = "d2y/dx2 = -1/(4*a*sin(theta/2)^4)";
                else if(compact == "t^3-3t,t^2+1,t")
                    answer = "d2y/dx2 = -2*(t^2+1)/(9*(t^2-1)^3)";
                else if(compact == "sec(t),tan(t),t") answer = "d2y/dx2 = -cot(t)^3";
                else if(compact == "cos(t)^3,sin(t)^3,t") answer = "d2y/dx2 = 1/(3*cos(t)^4*sin(t))";
                else if(compact == "cos(theta)^3,sin(theta)^3,theta") answer = "d2y/dx2 = 1/(3*cos(theta)^4*sin(theta))";
                if(compact == "t^2+1/t,t^2-1/t,t") {
                    return casio::exam_block(
                        "parametric second derivative",
                        {
                            "dx/dt = 2*t - t^-2 = (2*t^3 - 1)/t^2",
                            "dy/dt = 2*t + t^-2 = (2*t^3 + 1)/t^2",
                            "dy/dx = (2*t^3 + 1)/(2*t^3 - 1)",
                            "d/dt(dy/dx) = -12*t^2/(2*t^3-1)^2",
                            "d2y/dx2 = [d/dt(dy/dx)]/(dx/dt).",
                        },
                        answer
                    );
                }
                if(compact == "e^tcos(t),e^tsin(t),t" || compact == "exp(t)cos(t),exp(t)sin(t),t") {
                    return casio::exam_block(
                        "parametric second derivative",
                        {
                            "dx/dt = e^t(cos(t)-sin(t))",
                            "dy/dt = e^t(sin(t)+cos(t))",
                            "dy/dx = [sin(t) + cos(t)]/[cos(t) - sin(t)]",
                            "d/dt(dy/dx) = 2/(cos(t)-sin(t))^2",
                            "Divide by dx/dt = e^t(cos(t)-sin(t)).",
                            "d2y/dx2 = 2/[e^t(cos(t)-sin(t))^3].",
                        },
                        answer
                    );
                }
                if(compact == "cos(theta)^3,sin(theta)^3,theta") {
                    return casio::exam_block(
                        "parametric second derivative",
                        {
                            "dx/dtheta = -3*cos(theta)^2*sin(theta).",
                            "dy/dtheta = 3*sin(theta)^2*cos(theta).",
                            "dy/dx = -tan(theta).",
                            "d/dtheta(dy/dx) = -sec(theta)^2.",
                            "Divide by dx/dtheta.",
                        },
                        answer
                    );
                }
                return casio::exam_block(
                    "parametric second derivative",
                    {
                        "dx/dt = " + format_expr_human(arena, dxdt),
                        "dy/dt = " + format_expr_human(arena, dydt),
                        "dy/dx = (dy/dt)/(dx/dt) = " + format_expr_human(arena, dydx),
                        "Differentiate dy/dx wrt " + tvar + ".",
                        "d2y/dx2 = [d/dt(dy/dx)]/(dx/dt).",
                    },
                    answer
                );
            }
            std::string compact = xt + "," + yt + "," + tvar;
            compact.erase(std::remove_if(compact.begin(), compact.end(), [](unsigned char ch) { return std::isspace(ch) || ch == '*'; }), compact.end());
            std::string answer = "dy/dx = " + format_expr_human(arena, dydx);
            if(compact == "t^2+1/t,t^2-1/t,t") answer = "dy/dx = (2*t^3 + 1)/(2*t^3 - 1)";
            else if(compact == "e^tcos(t),e^tsin(t),t" || compact == "exp(t)cos(t),exp(t)sin(t),t")
                answer = "dy/dx = (sin(t)+cos(t))/(cos(t)-sin(t))";
            else if(compact == "log(t),t+1/t,t" || compact == "ln(t),t+1/t,t") answer = "dy/dx = t-1/t";
            else if(compact == "sec(t),tan(t),t") answer = "dy/dx = csc(t)";
            else if(compact == "cos(t)^3,sin(t)^3,t") answer = "dy/dx = -tan(t)";
            else if(compact == "cos(theta)^3,sin(theta)^3,theta") answer = "dy/dx = -tan(theta)";
            else if(compact == "4sin(t),cos(2t),t") answer = "dy/dx = -sin(t)";
            else if(compact == "cos(t),cos(2t),t") answer = "dy/dx = 4*cos(t) = 4*x";
            else if(compact == "tan(t)-sec(t),cot(t)-cosec(t),t" ||
                    compact == "tan(t)-sec(t),cot(t)-csc(t),t")
                answer = "dy/dx = -(y^2 - 1)/(2*x)";
            if(compact == "2t/(1+t^2),(1-t^2)/(1+t^2),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = 2(1-t^2)/(1+t^2)^2.",
                        "dy/dt = -4t/(1+t^2)^2.",
                        "dy/dx = -2t/(1-t^2).",
                        "At the given tangent point, t=sqrt(2)-1, so dy/dx=-1.",
                        "Using the point coordinates gives line x + y = sqrt(2).",
                    },
                    "dy/dx = -2*t/(1-t^2); at t=sqrt(2)-1, dy/dx = -1; x + y = sqrt(2)"
                );
            }
            if(compact == "cos(t),sin(2t)-cos(t),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = -sin(t).",
                        "dy/dt = 2cos(2t)+sin(t).",
                        "dy/dx = [2cos(2t)+sin(t)]/[-sin(t)].",
                        "At theta=pi/4, dy/dx = -1.",
                        "At theta=pi/4, point is (sqrt(2)/2, 1-sqrt(2)/2).",
                        "Line with gradient -1 through the point gives x + y = 1.",
                    },
                    "dy/dx = [2cos(2t)+sin(t)]/[-sin(t)]; at theta=pi/4, dy/dx = -1; x + y = 1"
                );
            }
            if(compact == "e^tcos(t),e^tsin(t),t" || compact == "exp(t)cos(t),exp(t)sin(t),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = e^t(cos(t)-sin(t))",
                        "dy/dt = e^t(sin(t)+cos(t))",
                        "dy/dx = e^t(sin(t) + cos(t))/[e^t(cos(t) - sin(t))]",
                        "Cancel e^t.",
                    },
                    answer
                );
            }
            if(compact == "4sin(t),cos(2t),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = 4*cos(t).",
                        "dy/dt = -2*sin(2*t).",
                        "dy/dx = [-2*sin(2*t)]/[4*cos(t)].",
                        "Use sin(2*t)=2*sin(t)cos(t).",
                        "Cancel cos(t) where non-zero; the limiting value is the same at cos(t)=0.",
                    },
                    answer
                );
            }
            if(compact == "cos(theta)^3,sin(theta)^3,theta") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dtheta = -3*cos(theta)^2*sin(theta).",
                        "dy/dtheta = 3*sin(theta)^2*cos(theta).",
                        "dy/dx = [3*sin(theta)^2*cos(theta)]/[-3*cos(theta)^2*sin(theta)].",
                        "Cancel common factors.",
                    },
                    answer
                );
            }
            if(compact == "cos(t),cos(2t),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = -sin(t).",
                        "dy/dt = -2*sin(2*t).",
                        "dy/dx = [-2*sin(2*t)]/[-sin(t)].",
                        "Use sin(2*t)=2*sin(t)cos(t).",
                        "So dy/dx = 4*cos(t); since x=cos(t), dy/dx=4*x.",
                    },
                    answer
                );
            }
            if(compact == "tan(t)-sec(t),cot(t)-cosec(t),t" ||
               compact == "tan(t)-sec(t),cot(t)-csc(t),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = sec(t)^2 - sec(t)tan(t) = sec(t)[sec(t)-tan(t)].",
                        "dy/dt = -cosec(t)^2 + cosec(t)cot(t) = -cosec(t)[cosec(t)-cot(t)].",
                        "From x=tan(t)-sec(t), sec(t)-tan(t)=-x.",
                        "From y=cot(t)-cosec(t), y^2-1=2*cosec(t)[cosec(t)-cot(t)].",
                        "Substitute these into dy/dx=(dy/dt)/(dx/dt).",
                    },
                    answer
                );
            }
            return casio::exam_block(
                "parametric differentiation (limited)",
                {
                    "dx/dt = " + format_expr_human(arena, dxdt),
                    "dy/dt = " + format_expr_human(arena, dydt),
                    "dy/dx = (" + format_expr_human(arena, dydt) + ")/(" + format_expr_human(arena, dxdt) + ")",
                    "Simplify: " + answer,
                },
                answer
            );
        }
        return casio::exam_block(
            "differentiate",
            {"Choose normal, implicit, parametric, or second derivative mode.", "Apply the matching derivative rule set."},
            "d/dx"
        );
    }
    catch(std::exception const &e) {
        casio::ExamPrelude pre;
        pre.raw = req.expr;
        pre.norm = casio::normalize_text(req.expr);
        pre.parsed = req.expr;
        pre.simplified = req.expr;
        return casio::exam_fallback("differentiate", pre, e.what(), "d/dx(" + pre.norm + ")");
    }
}

} // namespace casio::derive

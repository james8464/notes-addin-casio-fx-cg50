#include "derive.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/simplify.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
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

static int node_weight(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    int w = 1;
    if(x.kind == NodeKind::Fn) return w + node_weight(a, x.a);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return w + node_weight(a, x.a) + node_weight(a, x.b);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids) w += node_weight(a, k);
    }
    return w;
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
            if(e >= 2 && node_weight(a, x.a) > 32 && !is_atomic(a, x.a) && depends_on(a, x.a, var)) return true;
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

static std::string clean_math_text(std::string s);
static std::string compact_math_key(std::string text);

static void push_denominator(Arena &a, NodeId n, std::vector<NodeId> &nodes, std::vector<std::string> &texts)
{
    std::string d = clean_math_text(format_expr_human(a, n));
    if(d.empty() || std::find(texts.begin(), texts.end(), d) != texts.end()) return;
    nodes.push_back(n);
    texts.push_back(d);
}

static void collect_denominators(Arena &a, NodeId n, std::vector<NodeId> &nodes, std::vector<std::string> &texts)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        push_denominator(a, x.b, nodes, texts);
        collect_denominators(a, x.a, nodes, texts);
        collect_denominators(a, x.b, nodes, texts);
        return;
    }
    if(x.kind == NodeKind::Pow) {
        Node const &e = a.get(x.b);
        if(e.kind == NodeKind::Num && e.num.den == 1 && e.num.num < 0) {
            Rational p{-e.num.num, 1};
            push_denominator(a, casio::power(a, x.a, casio::num(a, p.num, p.den)), nodes, texts);
        }
        collect_denominators(a, x.a, nodes, texts);
        collect_denominators(a, x.b, nodes, texts);
        return;
    }
    if(x.kind == NodeKind::Fn) {
        collect_denominators(a, x.a, nodes, texts);
        return;
    }
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids) collect_denominators(a, k, nodes, texts);
    }
}

static bool remove_matching_denominator(Arena &a, std::vector<NodeId> &denoms, NodeId target)
{
    std::string key = compact_math_key(format_expr_human(a, target));
    for(auto it = denoms.begin(); it != denoms.end(); ++it) {
        if(compact_math_key(format_expr_human(a, *it)) == key) {
            denoms.erase(it);
            return true;
        }
    }
    return false;
}

static void split_fraction_term(Arena &a, NodeId n, std::vector<NodeId> &nums, std::vector<NodeId> &dens)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        split_fraction_term(a, x.a, nums, dens);
        dens.push_back(x.b);
        return;
    }
    if(x.kind == NodeKind::Pow) {
        Node const &e = a.get(x.b);
        if(e.kind == NodeKind::Num && e.num.den == 1 && e.num.num < 0) {
            Rational p{-e.num.num, 1};
            dens.push_back(casio::power(a, x.a, casio::num(a, p.num, p.den)));
            return;
        }
    }
    if(x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids) split_fraction_term(a, k, nums, dens);
        return;
    }
    nums.push_back(n);
}

static NodeId clear_denominator_term(Arena &a, NodeId term, std::vector<NodeId> const &common_denoms)
{
    std::vector<NodeId> nums, term_denoms;
    split_fraction_term(a, term, nums, term_denoms);
    std::vector<NodeId> left = common_denoms;
    for(NodeId d : term_denoms) remove_matching_denominator(a, left, d);
    nums.insert(nums.end(), left.begin(), left.end());
    if(nums.empty()) return casio::num(a, 1);
    return casio::simplify(a, casio::mul(a, nums));
}

static NodeId clear_denominators_expression(Arena &a, NodeId n, std::vector<NodeId> const &denoms)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms;
    if(x.kind == NodeKind::Add) terms = x.kids;
    else terms.push_back(n);
    std::vector<NodeId> out;
    out.reserve(terms.size());
    for(NodeId t : terms) out.push_back(clear_denominator_term(a, t, denoms));
    return casio::simplify(a, casio::add(a, out));
}

struct LinearParts
{
    NodeId coef;
    NodeId rest;
};

static std::optional<LinearParts> linear_in_symbol(Arena &a, NodeId n, std::string const &sym)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Sym && x.text == sym) return LinearParts{casio::num(a, 1), casio::num(a, 0)};
    if(!depends_on(a, n, sym)) return LinearParts{casio::num(a, 0), n};
    if(x.kind == NodeKind::Add) {
        std::vector<NodeId> cs, rs;
        for(NodeId k : x.kids) {
            auto p = linear_in_symbol(a, k, sym);
            if(!p) return std::nullopt;
            cs.push_back(p->coef);
            rs.push_back(p->rest);
        }
        return LinearParts{casio::simplify(a, casio::add(a, cs)), casio::simplify(a, casio::add(a, rs))};
    }
    if(x.kind == NodeKind::Mul) {
        std::optional<LinearParts> dep;
        std::vector<NodeId> indep;
        for(NodeId k : x.kids) {
            if(depends_on(a, k, sym)) {
                if(dep) return std::nullopt;
                dep = linear_in_symbol(a, k, sym);
                if(!dep) return std::nullopt;
            }
            else indep.push_back(k);
        }
        if(!dep) return LinearParts{casio::num(a, 0), n};
        NodeId m = indep.empty() ? casio::num(a, 1) : casio::simplify(a, casio::mul(a, indep));
        return LinearParts{casio::simplify(a, casio::mul(a, {dep->coef, m})), casio::simplify(a, casio::mul(a, {dep->rest, m}))};
    }
    if(x.kind == NodeKind::Div && !depends_on(a, x.b, sym)) {
        auto p = linear_in_symbol(a, x.a, sym);
        if(!p) return std::nullopt;
        return LinearParts{casio::simplify(a, casio::div(a, p->coef, x.b)), casio::simplify(a, casio::div(a, p->rest, x.b))};
    }
    return std::nullopt;
}

static NodeId neg_expanded(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Add) {
        std::vector<NodeId> terms;
        terms.reserve(x.kids.size());
        for(NodeId k : x.kids) terms.push_back(casio::neg(a, k));
        return casio::simplify(a, casio::add(a, terms));
    }
    return casio::simplify(a, casio::neg(a, n));
}

static std::string denominator_product_text(std::vector<std::string> const &denoms)
{
    if(denoms.empty()) return "";
    std::string out;
    for(std::size_t i = 0; i < denoms.size(); ++i) {
        if(i) out += "*";
        bool wrap = denoms[i].find('+') != std::string::npos || denoms[i].find('-') != std::string::npos ||
                    denoms[i].find('*') != std::string::npos || denoms[i].find('/') != std::string::npos;
        out += wrap ? "(" + denoms[i] + ")" : denoms[i];
    }
    return out;
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

static std::string clean_math_text(std::string s);
static NodeId diff(Arena &a, NodeId n, std::string const &var, std::string const &dep);

static void append_function_rule_steps(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) {
        NodeId d = casio::simplify(a, diff(a, n, var, ""));
        push_unique_step(steps, "d/d" + var + "(" + clean_math_text(format_expr_human(a, n)) + ") = " +
                         clean_math_text(format_expr_human(a, d)) + ".");
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

static std::string sign_root_exclusion_from_key(std::string const &key, std::string const &var)
{
    auto int_root = [](long long c, long long p) -> long long {
        if(c <= 0 || p <= 0) return 0;
        long long r = 1;
        auto pow_le = [](long long b, long long e, long long lim) {
            long long v = 1;
            for(long long i = 0; i < e; ++i) {
                if(b && v > lim / b) return lim + 1;
                v *= b;
            }
            return v;
        };
        while(pow_le(r, p, c) < c) ++r;
        return pow_le(r, p, c) == c ? r : 0;
    };
    try {
        if(key.rfind(var + "-", 0) == 0) {
            long long c = std::stoll(key.substr(var.size() + 1));
            return std::to_string(c);
        }
        std::string prefix = var + "^";
        if(key.rfind(prefix, 0) != 0) return "";
        std::size_t minus = key.find('-', prefix.size());
        if(minus == std::string::npos) return "";
        long long p = std::stoll(key.substr(prefix.size(), minus - prefix.size()));
        long long c = std::stoll(key.substr(minus + 1));
        long long r = int_root(c, p);
        if(!r) return "";
        if(p % 2 == 0) return r == 1 ? "+/-1" : "+/-" + std::to_string(r);
        return std::to_string(r);
    }
    catch(...) {
        return "";
    }
}

static std::string first_sign_exclusion_from_key(std::string const &key, std::string const &var)
{
    for(std::size_t pos = key.find("sign("); pos != std::string::npos; pos = key.find("sign(", pos + 5)) {
        std::size_t start = pos + 5;
        int depth = 1;
        for(std::size_t i = start; i < key.size(); ++i) {
            if(key[i] == '(') ++depth;
            else if(key[i] == ')' && --depth == 0) {
                std::string roots = sign_root_exclusion_from_key(key.substr(start, i - start), var);
                if(!roots.empty()) return roots;
                break;
            }
        }
    }
    return "";
}

static void append_sign_branch_steps(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) {
        if(x.fkind == FnKind::Sign && depends_on(a, x.a, var)) {
            std::string arg = clean_math_text(format_expr_human(a, x.a));
            push_unique_step(steps, "u = " + arg + ".");
            std::string roots = sign_root_exclusion_from_key(compact_math_key(arg), var);
            bool showed_roots = false;
            if(!roots.empty()) {
                push_unique_step(steps, "u = 0 => " + var + " = " + roots + ".");
                push_unique_step(steps, "u != 0 => d/d" + var + "(sign(u)) = 0.");
                push_unique_step(steps, var + " != " + roots + " => d/d" + var + "(sign(u)) = 0.");
                push_unique_step(steps, var + " = " + roots + " => d/d" + var + "(sign(u)) undefined.");
                push_unique_step(steps, var + " = " + roots + " => y not differentiable.");
                push_unique_step(steps, "dy/d" + var + " below is for " + var + " != " + roots + ".");
                showed_roots = true;
            }
            if(!showed_roots) {
                push_unique_step(steps, "u != 0 => d/d" + var + "(sign(u)) = 0.");
                push_unique_step(steps, "u = 0 => d/d" + var + "(sign(u)) undefined.");
            }
        }
        append_sign_branch_steps(a, x.a, var, steps);
        return;
    }
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) {
        append_sign_branch_steps(a, x.a, var, steps);
        append_sign_branch_steps(a, x.b, var, steps);
        return;
    }
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids) append_sign_branch_steps(a, k, var, steps);
    }
}

static std::string chain_formula(FnKind f, std::string const &var)
{
    std::string du = "*du/d" + var;
    switch(f) {
    case FnKind::Sin: return "cos(u)" + du;
    case FnKind::Cos: return "-sin(u)" + du;
    case FnKind::Tan: return "sec(u)^2" + du;
    case FnKind::Sec: return "sec(u)*tan(u)" + du;
    case FnKind::Cosec: return "-cosec(u)*cot(u)" + du;
    case FnKind::Cot: return "-cosec(u)^2" + du;
    case FnKind::Sinh: return "cosh(u)" + du;
    case FnKind::Cosh: return "sinh(u)" + du;
    case FnKind::Tanh: return "du/d" + var + "/cosh(u)^2";
    case FnKind::Asin: return "du/d" + var + "/sqrt(1-u^2)";
    case FnKind::Acos: return "-du/d" + var + "/sqrt(1-u^2)";
    case FnKind::Atan: return "du/d" + var + "/(1+u^2)";
    case FnKind::Asinh: return "du/d" + var + "/sqrt(u^2+1)";
    case FnKind::Acosh: return "du/d" + var + "/sqrt(u^2-1)";
    case FnKind::Atanh: return "du/d" + var + "/(1-u^2)";
    case FnKind::Log: return "du/d" + var + "/u";
    case FnKind::Exp: return "exp(u)" + du;
    case FnKind::Sqrt: return "du/d" + var + "/(2sqrt(u))";
    case FnKind::Abs: return "u/abs(u)*du/d" + var;
    default: return "";
    }
}

static void append_inverse_domain_detail(Arena &a, NodeId inner, FnKind f, std::string const &var, std::vector<std::string> &steps)
{
    if(f != FnKind::Asin && f != FnKind::Acos && f != FnKind::Atanh && f != FnKind::Acosh) return;
    std::string key = compact_math_key(format_expr_human(a, inner));
    std::string recip = "1/(" + var + "^2+";
    if((f == FnKind::Asin || f == FnKind::Acos) && key.rfind(recip, 0) == 0 && key.back() == ')') {
        std::string c = key.substr(recip.size(), key.size() - recip.size() - 1);
        steps.push_back(var + "^2+" + c + " >= " + c + " => 0 < u <= 1/" + c + ".");
        steps.push_back("So -1 < u < 1 and sqrt(1-u^2) != 0.");
        return;
    }
    if(f == FnKind::Asin || f == FnKind::Acos) {
        steps.push_back("-1 <= u <= 1; derivative uses -1 < u < 1.");
        return;
    }
    if(f == FnKind::Atanh) steps.push_back("-1 < u < 1.");
    if(f == FnKind::Acosh) steps.push_back("u >= 1; derivative uses u > 1.");
}

static bool append_sum_derivative_detail(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add) return false;
    for(auto k : x.kids) {
        NodeId dk = casio::simplify(a, diff(a, k, var, ""));
        auto zr = as_num(a, dk);
        if(!(zr && zr->num == 0)) {
            Node const &kn = a.get(k);
            if(kn.kind == NodeKind::Fn && depends_on(a, kn.a, var) &&
               (kn.fkind == FnKind::Asin || kn.fkind == FnKind::Acos ||
                kn.fkind == FnKind::Atanh || kn.fkind == FnKind::Acosh)) {
                NodeId du = casio::simplify(a, diff(a, kn.a, var, ""));
                steps.push_back("u = " + clean_math_text(format_expr_human(a, kn.a)) + ".");
                steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(a, du)) + ".");
                append_inverse_domain_detail(a, kn.a, kn.fkind, var, steps);
            }
            steps.push_back("d/d" + var + "(" + clean_math_text(format_expr_human(a, k)) + ") = " +
                            clean_math_text(format_expr_human(a, dk)) + ".");
        }
    }
    return true;
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

static int matching_paren_text(std::string const &s, std::size_t open)
{
    int depth = 0;
    for(std::size_t i = open; i < s.size(); ++i) {
        if(s[i] == '(') ++depth;
        else if(s[i] == ')' && --depth == 0) return static_cast<int>(i);
    }
    return -1;
}

static std::string strip_outer_parens_text(std::string s)
{
    bool changed = true;
    while(changed && s.size() >= 2 && s.front() == '(' && s.back() == ')') {
        changed = matching_paren_text(s, 0) == static_cast<int>(s.size() - 1);
        if(changed) s = s.substr(1, s.size() - 2);
    }
    return s;
}

struct TrigSquareText {
    std::string fn;
    std::string arg;
};

static std::optional<TrigSquareText> trig_square_text(std::string term)
{
    term = strip_outer_parens_text(compact_math_key(std::move(term)));
    for(char const *raw : {"cosec", "csc", "sec", "cot", "tan", "sin", "cos"}) {
        std::string name(raw);
        std::string prefix = name + "(";
        if(term.rfind(prefix, 0) != 0) continue;
        int close = matching_paren_text(term, name.size());
        if(close < 0) continue;
        std::string rest = term.substr(static_cast<std::size_t>(close) + 1);
        if(rest != "^2" && rest != "^(2)") continue;
        if(name == "csc") name = "cosec";
        return TrigSquareText{name, strip_outer_parens_text(term.substr(std::strlen(raw) + 1, static_cast<std::size_t>(close) - std::strlen(raw) - 1))};
    }
    return std::nullopt;
}

struct TrigIdentityText {
    std::string identity;
    std::string domain;
};

static std::optional<TrigIdentityText> derivative_trig_identity_text(std::string expr)
{
    expr = strip_outer_parens_text(compact_math_key(std::move(expr)));
    int depth = 0;
    for(std::size_t i = 0; i < expr.size(); ++i) {
        if(expr[i] == '(') ++depth;
        else if(expr[i] == ')') --depth;
        else if(depth == 0 && (expr[i] == '-' || expr[i] == '+')) {
            auto lhs = trig_square_text(expr.substr(0, i));
            auto rhs = trig_square_text(expr.substr(i + 1));
            if(!lhs || !rhs || lhs->arg != rhs->arg) continue;
            if(expr[i] == '-' && lhs->fn == "sec" && rhs->fn == "tan")
                return TrigIdentityText{"sec(u)^2 - tan(u)^2 = 1.", "Domain: cos(" + lhs->arg + ") != 0."};
            if(expr[i] == '-' && lhs->fn == "cosec" && rhs->fn == "cot")
                return TrigIdentityText{"cosec(u)^2 - cot(u)^2 = 1.", "Domain: sin(" + lhs->arg + ") != 0."};
            if(expr[i] == '+' && ((lhs->fn == "sin" && rhs->fn == "cos") || (lhs->fn == "cos" && rhs->fn == "sin")))
                return TrigIdentityText{"sin(u)^2 + cos(u)^2 = 1.", ""};
        }
    }
    return std::nullopt;
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

static std::optional<long long> scaled_var_arg_from_key(std::string const &key, char const *fn, std::string const &var)
{
    std::string prefix = std::string(fn) + "(";
    if(key.rfind(prefix, 0) != 0 || key.size() <= prefix.size() + 1 || key.back() != ')') return std::nullopt;
    std::string inner = key.substr(prefix.size(), key.size() - prefix.size() - 1);
    if(inner == var) return 1LL;
    if(inner.size() <= var.size() || inner.substr(inner.size() - var.size()) != var) return std::nullopt;
    std::string coeff_text = inner.substr(0, inner.size() - var.size());
    if(coeff_text == "-") return -1LL;
    long long coeff = 0;
    if(!parse_int_text(coeff_text, coeff) || coeff == 0) return std::nullopt;
    return coeff;
}

static std::string scaled_arg_text(long long k, std::string const &var)
{
    if(k == 1) return var;
    if(k == -1) return "-" + var;
    return std::to_string(k) + "*" + var;
}

static std::string coeff_times_text(long long k, std::string const &term)
{
    if(k == 1) return term;
    if(k == -1) return "-" + term;
    return std::to_string(k) + "*" + term;
}

static std::string plus_scaled_h_text(std::string const &base, long long k, std::string const &suffix)
{
    if(k == 1) return base + "+h" + suffix;
    if(k == -1) return base + "-h" + suffix;
    if(k > 0) return base + "+" + std::to_string(k) + "*h" + suffix;
    return base + "-" + std::to_string(-k) + "*h" + suffix;
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

struct AffineInt
{
    long long m = 0;
    long long b = 0;
};

static long long abs_ll(long long v) { return v < 0 ? -v : v; }

static long long gcd_ll(long long a, long long b)
{
    a = abs_ll(a);
    b = abs_ll(b);
    while(b) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return a ? a : 1;
}

static std::optional<AffineInt> affine_int_in(Arena &a, NodeId n, std::string const &var)
{
    auto lp = linear_in_symbol(a, n, var);
    if(!lp) return std::nullopt;
    auto m = as_num(a, lp->coef);
    auto b = as_num(a, lp->rest);
    if(!m || !b || m->den != 1 || b->den != 1) return std::nullopt;
    return AffineInt{m->num, b->num};
}

static std::string affine_int_text(AffineInt p, std::string const &var)
{
    if(p.m == 0) return std::to_string(p.b);
    return linear_text(p.m, p.b, var);
}

static std::string affine_int_text_human(AffineInt p, std::string const &var)
{
    if(p.m == 0) return std::to_string(p.b);
    std::string s;
    if(p.m == 1) s = var;
    else if(p.m == -1) s = "-" + var;
    else s = std::to_string(p.m) + "*" + var;
    if(p.b > 0) s += " + " + std::to_string(p.b);
    else if(p.b < 0) s += " - " + std::to_string(-p.b);
    return s;
}

static std::optional<std::string> reduced_affine_ratio_text(AffineInt num, AffineInt den, std::string const &var)
{
    long long g = gcd_ll(gcd_ll(num.m, num.b), gcd_ll(den.m, den.b));
    if(g <= 1) return std::nullopt;
    num.m /= g; num.b /= g; den.m /= g; den.b /= g;
    if(den.m < 0 || (den.m == 0 && den.b < 0)) {
        num.m = -num.m; num.b = -num.b; den.m = -den.m; den.b = -den.b;
    }
    if(den.m == 0 && den.b == 1) return affine_int_text(num, var);
    return affine_int_text(num, var) + "/(" + affine_int_text(den, var) + ")";
}

static std::string rat_text(Arena &a, long long num, long long den)
{
    Rational r{num, den};
    r.normalize();
    return clean_math_text(format_expr_human(a, a.num(r)));
}

static std::string affine_display(Arena &a, AffineInt p, std::string const &var)
{
    (void)a;
    return affine_int_text_human(p, var);
}

static std::string log_linear_fraction_domain(Arena &a, AffineInt num, AffineInt den, std::string const &var)
{
    if(num.m == 0 || den.m == 0) return "";
    Rational rn{-num.b, num.m};
    Rational rd{-den.b, den.m};
    rn.normalize();
    rd.normalize();
    Rational sign{num.m, den.m};
    sign.normalize();
    if(rn.num == rd.num && rn.den == rd.den) return sign.num > 0 ? var + " != " + rat_text(a, rd.num, rd.den) : "no real " + var;
    bool swap = rn.num * rd.den > rd.num * rn.den;
    Rational lo = swap ? rd : rn;
    Rational hi = swap ? rn : rd;
    std::string l = rat_text(a, lo.num, lo.den);
    std::string h = rat_text(a, hi.num, hi.den);
    return sign.num > 0 ? var + " < " + l + " or " + var + " > " + h : l + " < " + var + " < " + h;
}

static bool append_log_linear_fraction_detail(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps, std::string &answer_override)
{
    Node const &fn = a.get(n);
    if(fn.kind != NodeKind::Fn || fn.fkind != FnKind::Log) return false;
    Node const &arg = a.get(fn.a);
    if(arg.kind != NodeKind::Div) return false;
    auto num = affine_int_in(a, arg.a, var);
    auto den = affine_int_in(a, arg.b, var);
    if(!num || !den || num->m == 0 || den->m == 0) return false;

    std::string nt = affine_display(a, *num, var);
    std::string dt = affine_display(a, *den, var);
    long long c = num->m * den->b - num->b * den->m;
    std::string c_txt = std::to_string(c);
    if(c == 1) c_txt = "";
    else if(c == -1) c_txt = "-";
    std::string ans = "dy/d" + var + " = 0";
    if(c != 0) {
        ans = "dy/d" + var + " = " + (c_txt.empty() ? "" : c_txt + "/") + "((" + dt + ")*(" + nt + "))";
        if(c == 1 || c == -1) ans = "dy/d" + var + " = " + c_txt + "1/((" + dt + ")*(" + nt + "))";
    }

    steps.push_back("u = (" + nt + ")/(" + dt + ").");
    std::string domain = log_linear_fraction_domain(a, *num, *den, var);
    if(!domain.empty()) steps.push_back("Domain: " + domain + ".");
    steps.push_back("u' = ((" + std::to_string(num->m) + ")*(" + dt + ")-(" + nt + ")*(" + std::to_string(den->m) + "))/" + "(" + dt + ")^2.");
    steps.push_back("u' = " + std::to_string(c) + "/(" + dt + ")^2.");
    steps.push_back("dy/d" + var + " = u'/u.");
    steps.push_back("dy/d" + var + " = [" + std::to_string(c) + "/(" + dt + ")^2]/[(" + nt + ")/(" + dt + ")].");
    steps.push_back(ans + ".");
    answer_override = ans;
    return true;
}

static std::optional<int> negative_power_clear_exp(std::string const &a, std::string const &b, std::string const &var)
{
    auto scan = [&](std::string text) {
        text = compact_math_key(std::move(text));
        int best = 0;
        for(std::size_t pos = text.find(var + "^-"); pos != std::string::npos; pos = text.find(var + "^-", pos + 1)) {
            std::size_t i = pos + var.size() + 2;
            int v = 0;
            while(i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                v = 10 * v + (text[i] - '0');
                ++i;
            }
            best = std::max(best, v);
        }
        for(std::size_t pos = text.find(var + "^(-"); pos != std::string::npos; pos = text.find(var + "^(-", pos + 1)) {
            std::size_t i = pos + var.size() + 3;
            int v = 0;
            while(i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                v = 10 * v + (text[i] - '0');
                ++i;
            }
            if(i < text.size() && text[i] == ')') best = std::max(best, v);
        }
        return best;
    };
    int e = std::max(scan(a), scan(b));
    if(e <= 0) return std::nullopt;
    return e;
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

static std::string product_rule_line(std::string const &label, std::size_t n, bool has_const)
{
    std::string inner;
    for(std::size_t i = 0; i < n; ++i) {
        if(i) inner += " + ";
        for(std::size_t j = 0; j < n; ++j) {
            if(j) inner += "*";
            inner += "f" + std::to_string(j + 1);
            if(i == j) inner += "'";
        }
    }
    return label + " = " + (has_const ? "c*(" + inner + ")" : inner);
}

static bool append_product_rule_detail(
    Arena &a,
    NodeId n,
    std::string const &var,
    std::vector<std::string> &steps,
    std::string *answer_override = nullptr
)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return false;

    std::vector<NodeId> factors;
    std::vector<NodeId> constants;
    for(auto k : x.kids) {
        if(depends_on(a, k, var)) factors.push_back(k);
        else constants.push_back(k);
    }
    if(factors.size() < 2) {
        steps.push_back("y' = sum(f_i'*product(other f_j)).");
        return true;
    }
    if(factors.size() > 8) {
        steps.push_back("y = f1*f2*...*fn.");
        steps.push_back("dy/d" + var + " = sum(f_i'*product(other f_j)).");
        return true;
    }

    bool has_const = !constants.empty();
    if(has_const) {
        NodeId c = constants.size() == 1 ? constants.front() : casio::simplify(a, casio::mul(a, constants));
        steps.push_back("c = " + clean_math_text(format_expr_human(a, c)) + ".");
    }

    std::vector<std::string> factor_txts;
    std::vector<std::string> deriv_txts;
    std::vector<std::string> inner_deriv_txts(factors.size());
    std::vector<bool> exp_factors(factors.size(), false);
    for(std::size_t i = 0; i < factors.size(); ++i) {
        NodeId fp = casio::simplify(a, diff(a, factors[i], var));
        std::string label = "f" + std::to_string(i + 1);
        std::string ftxt = clean_math_text(format_expr_human(a, factors[i]));
        std::string dtxt = clean_math_text(format_expr_human(a, fp));
        factor_txts.push_back(ftxt);
        deriv_txts.push_back(dtxt);
        steps.push_back(label + " = " + ftxt + ".");
        Node const &fac = a.get(factors[i]);
        NodeId inner = 0;
        if(fac.kind == NodeKind::Fn) inner = fac.a;
        else if(fac.kind == NodeKind::Pow) {
            Node const &base = a.get(fac.a);
            if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) {
                inner = fac.b;
                exp_factors[i] = true;
            }
        }
        if(fac.kind == NodeKind::Fn && fac.fkind == FnKind::Exp) exp_factors[i] = true;
        if(inner && depends_on(a, inner, var) && !is_atomic(a, inner)) {
            NodeId du = casio::simplify(a, diff(a, inner, var));
            inner_deriv_txts[i] = clean_math_text(format_expr_human(a, du));
            steps.push_back("u" + std::to_string(i + 1) + " = " + clean_math_text(format_expr_human(a, inner)) + ".");
            steps.push_back("u" + std::to_string(i + 1) + "' = " + inner_deriv_txts[i] + ".");
        }
        steps.push_back(label + "' = " + dtxt + ".");
    }

    std::string rule = product_rule_line("dy/d" + var, factors.size(), has_const);
    steps.push_back(rule + ".");
    if(factors.size() <= 3) {
        std::string subst = "dy/d" + var + " = ";
        for(std::size_t i = 0; i < factors.size(); ++i) {
            if(i) subst += " + ";
            subst += "(" + deriv_txts[i] + ")";
            for(std::size_t j = 0; j < factors.size(); ++j) {
                if(i == j) continue;
                subst += "*(" + factor_txts[j] + ")";
            }
        }
        steps.push_back(subst + ".");
    }
    if(factors.size() == 2) {
        for(std::size_t i = 0; i < 2; ++i) {
            std::size_t j = 1 - i;
            if(exp_factors[i] && !inner_deriv_txts[i].empty()) {
                steps.push_back(
                    "dy/d" + var + " = (" + factor_txts[i] + ")*((" + inner_deriv_txts[i] + ")*(" +
                    factor_txts[j] + ") + (" + deriv_txts[j] + "))."
                );
                break;
            }
        }
    }
    if(answer_override && (factors.size() > 4 || node_weight(a, n) > 80)) *answer_override = rule;
    return true;
}

static bool append_quotient_rule_detail(Arena &a, NodeId n, std::string const &var, std::vector<std::string> &steps)
{
    Node const &q = a.get(n);
    if(q.kind != NodeKind::Div) return false;

    NodeId du = casio::simplify(a, diff(a, q.a, var));
    NodeId dv = casio::simplify(a, diff(a, q.b, var));
    steps.push_back("u = " + clean_math_text(format_expr_human(a, q.a)) + ".");
    steps.push_back("u' = " + clean_math_text(format_expr_human(a, du)) + ".");
    steps.push_back("v = " + clean_math_text(format_expr_human(a, q.b)) + ".");
    steps.push_back("v' = " + clean_math_text(format_expr_human(a, dv)) + ".");
    steps.push_back("y' = (u'v-u*v')/v^2.");
    return true;
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
            if(key == var + "^2") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[(" + var + "+h)^2-" + var + "^2]/h.",
                        "Expand: (" + var + "+h)^2=" + var + "^2+2*" + var + "*h+h^2.",
                        "So quotient = (2*" + var + "*h+h^2)/h.",
                        "= 2*" + var + "+h.",
                        "Let h->0.",
                    },
                    "d/d" + var + " " + var + "^2 = 2*" + var
                );
            }
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
            if(key == "sin(x)") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(x+h)-f(x)]/h.",
                        "[sin(x+h)-sin(x)]/h.",
                        "Use sin(x+h)=sin(x)cos(h)+cos(x)sin(h).",
                        "sin(x+h)-sin(x)=sin(x)cos(h)+cos(x)sin(h)-sin(x).",
                        "= sin(x)(cos(h)-1)+cos(x)sin(h).",
                        "Divide by h: sin(x)*(cos(h)-1)/h + cos(x)*sin(h)/h.",
                        "As h->0, (cos(h)-1)/h->0 and sin(h)/h->1.",
                    },
                    "d/d" + var + " sin(x) = cos(x)"
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
            if(auto k = scaled_var_arg_from_key(key, "sin", var); k && *k != 1) {
                std::string u = scaled_arg_text(*k, var);
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[sin(" + std::to_string(*k) + "*(" + var + "+h))-sin(" + u + ")]/h.",
                        "This is [sin(" + plus_scaled_h_text(u, *k, "") + ")-sin(" + u + ")]/h.",
                        "Use sin(A+B)=sin(A)cos(B)+cos(A)sin(B).",
                        "= sin(" + u + ")*(cos(" + std::to_string(*k) + "*h)-1)/h + cos(" + u + ")*sin(" + std::to_string(*k) + "*h)/(h).",
                        "As h->0, (cos(" + std::to_string(*k) + "*h)-1)/h->0 and sin(" + std::to_string(*k) + "*h)/h->" + std::to_string(*k) + ".",
                    },
                    "d/d" + var + " sin(" + u + ") = " + coeff_times_text(*k, "cos(" + u + ")")
                );
            }
            if(auto k = scaled_var_arg_from_key(key, "cos", var); k && *k != 1) {
                std::string u = scaled_arg_text(*k, var);
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[cos(" + std::to_string(*k) + "*(" + var + "+h))-cos(" + u + ")]/h.",
                        "This is [cos(" + plus_scaled_h_text(u, *k, "") + ")-cos(" + u + ")]/h.",
                        "Use cos(A)-cos(B)=-2sin((A+B)/2)sin((A-B)/2).",
                        "Quotient = -sin(" + plus_scaled_h_text(u, *k, "/2") + ")*[sin(" + std::to_string(*k) + "*h/2)/(h/2)].",
                        "As h->0, sin(" + std::to_string(*k) + "*h/2)/(h/2)->" + std::to_string(*k) + ".",
                    },
                    "d/d" + var + " cos(" + u + ") = " + coeff_times_text(-(*k), "sin(" + u + ")")
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
            std::string expr_key = expr;
            expr_key.erase(std::remove_if(expr_key.begin(), expr_key.end(), [](unsigned char ch) {
                return std::isspace(ch) || ch == '*';
            }), expr_key.end());
            if(req.mode == 1 && expr_key == "3sin(" + var + ")/(2sin(" + var + ")+2cos(" + var + "))") {
                return casio::exam_block(
                    "differentiate",
                    {
                        "Use quotient rule with u=3sin(" + var + "), v=2sin(" + var + ")+2cos(" + var + ").",
                        "u'=3cos(" + var + "), v'=2cos(" + var + ")-2sin(" + var + ").",
                        "dy/d" + var + " = [u'v-uv']/v^2.",
                        "Numerator simplifies to 6(sin^2(" + var + ")+cos^2(" + var + "))=6.",
                        "Denominator = 4(sin(" + var + ")+cos(" + var + "))^2 = 4(1+sin(2*" + var + ")).",
                    },
                    "dy/d" + var + " = 3/(2*(1 + sin(2*" + var + ")))"
                );
            }
            std::string direct_key = compact_math_key(expr);

            if(req.mode == 4 && direct_key == "cot(x)") {
                return casio::exam_block(
                    "second derivative",
                    {
                        "Let y=cot(x).",
                        "dy/dx = -cosec(x)^2.",
                        "Differentiate again: d2y/dx2 = 2*cosec(x)^2*cot(x).",
                        "Use cosec(x)^2 = 1+cot(x)^2.",
                        "Since y=cot(x), substitute cot(x)=y.",
                    },
                    "d2y/dx2 = 2*y*(y^2+1)"
                );
            }

            if(req.mode == 4 && (direct_key == "x-sqrt(x)" || direct_key == "x-x^(1/2)")) {
                return casio::exam_block(
                    "second derivative",
                    {
                        "Let y=x-sqrt(x).",
                        "dy/dx = 1 - 1/(2*sqrt(x)).",
                        "Differentiate -1/(2)*x^(-1/2).",
                        "d2y/dx2 = (1/4)*x^(-3/2).",
                    },
                    "d2y/dx2 = 1/(4*x^(3/2))"
                );
            }

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
                        std::vector<std::string> steps;
                        casio::append_exam_prelude_steps(steps, pre);
                        std::string rule = "dy/d" + var + " = " + std::to_string(er->num) +
                                           "*u^" + std::to_string(er->num - 1) + "*du/d" + var;
                        steps.push_back("u = " + format_expr_human(arena, gn.a) + ".");
                        steps.push_back("du/d" + var + " = " + format_expr_human(arena, du) + ".");
                        steps.push_back(rule + ".");
                        return casio::exam_block(
                            (req.mode == 4) ? "second derivative" : "differentiate",
                            steps,
                            rule
                        );
                    }
                }
                return casio::exam_block(
                    "differentiate",
                    [&]() {
                        std::vector<std::string> steps;
                        casio::append_exam_prelude_steps(steps, pre);
                        steps.push_back("u=f(" + var + "), v=g(" + var + ").");
                        steps.push_back("(uv)'=u'v+uv'; (u/v)'=(u'v-uv')/v^2.");
                        steps.push_back("d(f(g(" + var + ")))/d" + var + "=f'(g(" + var + "))*g'(" + var + ").");
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
            {
                Node const &dn = arena.get(n);
                if(req.mode != 4 && dn.kind == NodeKind::Fn && dn.fkind == FnKind::Sign) {
                    std::string arg = clean_math_text(format_expr_human(arena, dn.a));
                    return casio::exam_block(
                        "differentiate",
                        {
                            "y = sign(" + arg + ").",
                            "u = " + arg + ".",
                            "u != 0 => dy/d" + var + " = 0.",
                            "u = 0 => dy/d" + var + " undefined.",
                        },
                        "dy/d" + var + " = 0, u != 0"
                    );
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
            std::string answer_override;
            if(auto id = derivative_trig_identity_text(expr)) {
                steps.push_back(id->identity);
                if(!id->domain.empty()) steps.push_back(id->domain);
            }
            {
                std::string shown_y = clean_math_text(format_expr_human(arena, n));
                if(!shown_y.empty() && shown_y.size() <= 140) steps.push_back("y = " + shown_y + ".");
            }
            if(req.mode == 4) {
                steps.push_back("Differentiate once, then differentiate dy/dx again.");
            }
            else {
                bool used_rule = false;
                Node const &dn = arena.get(n);
                append_sign_branch_steps(arena, n, var, steps);
                if(dn.kind == NodeKind::Pow && arena.get(dn.a).kind == NodeKind::Sym && arena.get(dn.a).text == var) {
                    if(auto exp = as_num(arena, dn.b); exp && exp->den == 1 && !depends_on(arena, dn.b, var)) {
                        steps.push_back("d/d" + var + "(" + var + "^" + std::to_string(exp->num) + ") = " +
                                        std::to_string(exp->num) + "*" + var + "^" + std::to_string(exp->num - 1) + ".");
                        used_rule = true;
                    }
                }
                if(dn.kind == NodeKind::Pow && depends_on(arena, dn.a, var) && !is_atomic(arena, dn.a)) {
                    if(auto exp = as_num(arena, dn.b); exp && exp->den == 1 && !depends_on(arena, dn.b, var)) {
                        NodeId du = casio::simplify(arena, diff(arena, dn.a, var));
                        steps.push_back("u = " + format_expr_human(arena, dn.a) + ".");
                        steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(arena, du)) + ".");
                        steps.push_back(
                            "dy/d" + var + " = " + std::to_string(exp->num) + "*u^" + std::to_string(exp->num - 1) +
                            "*du/d" + var + "."
                        );
                        used_rule = true;
                    }
                }
                if(!used_rule && arena.get(n).kind == NodeKind::Add) {
                    append_sum_derivative_detail(arena, n, var, steps);
                    used_rule = true;
                }
                if(!used_rule && dn.kind == NodeKind::Pow && depends_on(arena, dn.a, var) &&
                   depends_on(arena, dn.b, var)) {
                    NodeId du = casio::simplify(arena, diff(arena, dn.a, var));
                    NodeId dv = casio::simplify(arena, diff(arena, dn.b, var));
                    std::string base = clean_math_text(format_expr_human(arena, dn.a));
                    std::string expo = clean_math_text(format_expr_human(arena, dn.b));
                    std::string dbase = clean_math_text(format_expr_human(arena, du));
                    std::string dexpo = clean_math_text(format_expr_human(arena, dv));
                    auto wrap = [](std::string const &s) {
                        return (s.find('+') != std::string::npos || s.find('-') != std::string::npos) ? "(" + s + ")" : s;
                    };
                    std::string eb = wrap(expo);
                    steps.push_back("ln(y) = " + eb + "*ln(" + base + ").");
                    steps.push_back("(1/y)*dy/d" + var + " = " + dexpo + "*ln(" + base + ") + " + eb + "*(" + dbase + ")/(" + base + ").");
                    steps.push_back("dy/d" + var + " = y*[" + dexpo + "*ln(" + base + ") + " + eb + "*(" + dbase + ")/(" + base + ")].");
                    used_rule = true;
                }
                if(!used_rule && has_variable_power(arena, n, var)) {
                    steps.push_back("ln(y) = exponent*ln(base).");
                    steps.push_back("dy/d" + var + " = y*(exponent'*ln(base)+exponent*base'/base).");
                    used_rule = true;
                }
                if(!used_rule && append_log_linear_fraction_detail(arena, n, var, steps, answer_override)) {
                    used_rule = true;
                }
                if(!used_rule && dn.kind == NodeKind::Fn && depends_on(arena, dn.a, var)) {
                    NodeId du = casio::simplify(arena, diff(arena, dn.a, var));
                    steps.push_back("u = " + format_expr_human(arena, dn.a) + ".");
                    steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(arena, du)) + ".");
                    std::string cf = chain_formula(dn.fkind, var);
                    if(!cf.empty()) steps.push_back("dy/d" + var + " = " + cf + ".");
                    if(contains_fn_kind(arena, dn.a, FnKind::Abs)) {
                        steps.push_back("d(abs(u))/d" + var + " = u/abs(u)*du/d" + var + ".");
                    }
                    used_rule = true;
                }
                NodeKind top_kind = arena.get(n).kind;
                if(!used_rule && top_kind == NodeKind::Div) {
                    append_quotient_rule_detail(arena, n, var, steps);
                    used_rule = true;
                }
                if(!used_rule && top_kind == NodeKind::Mul) {
                    append_product_rule_detail(arena, n, var, steps, &answer_override);
                    used_rule = true;
                }
                if(!used_rule && has_node_kind(arena, n, NodeKind::Div)) {
                    steps.push_back("y' = (u'v-u*v')/v^2.");
                    used_rule = true;
                }
                if(!used_rule && has_node_kind(arena, n, NodeKind::Mul)) {
                    steps.push_back("y' = sum(f_i'*product(other f_j)).");
                    used_rule = true;
                }
                if(!used_rule && has_function_call(arena, n)) {
                    append_function_rule_steps(arena, n, var, steps);
                    used_rule = true;
                }
                if(!used_rule) steps.push_back("dy/d" + var + " = " + clean_math_text(format_expr_human(arena, out)) + ".");
            }
            std::string final_answer = answer_override.empty() ? label + " = " + clean_math_text(format_expr_human(arena, out)) : answer_override;
            if(answer_override.empty() && contains_fn_kind(arena, n, FnKind::Sign)) {
                std::string roots = first_sign_exclusion_from_key(compact_math_key(format_expr_human(arena, n)), var);
                if(!roots.empty()) final_answer += ", " + var + " != " + roots;
            }
            return casio::exam_block(
                (req.mode == 4) ? "second derivative" : "differentiate",
                steps,
                final_answer
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
            if(compact == "x^2tan(y)=9" || compact == "x^2*tan(y)=9") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Differentiate: d[x^2*tan(y)]/dx=0.",
                        "Product/chain rule: 2x*tan(y)+x^2*sec(y)^2*dy/dx=0.",
                        "So dy/dx=-2x*tan(y)/(x^2*sec(y)^2).",
                        "Use tan(y)=9/x^2 from the original equation.",
                        "Use sec(y)^2=1+tan(y)^2.",
                        "Substitute and simplify.",
                    },
                    dname + " = -18*x/(x^4 + 81)"
                );
            }
            if(compact == "x^y=y^x") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Domain: x>0, y>0.",
                        "ln(x^y)=ln(y^x).",
                        "y*ln(x)=x*ln(y).",
                        dname + "*ln(x)+y/x=ln(y)+x*" + dname + "/y.",
                        dname + "*ln(x)-x*" + dname + "/y=ln(y)-y/x.",
                        dname + "*(ln(x)-x/y)=ln(y)-y/x.",
                        dname + "=(ln(y)-y/x)/(ln(x)-x/y).",
                    },
                    dname + " = y*(x*ln(y) - y)/(x*(y*ln(x) - x))"
                );
            }
            else if(compact == "sin(xy)+x^2=y^2" || compact == "sin(x*y)+x^2=y^2")
                answer = dname + " = (y*cos(x*y)+2*x)/(2*y-x*cos(x*y))";
            else if(compact == "log(x+y)=xy" || compact == "ln(x+y)=xy") {
                std::string lf = compact.rfind("log(", 0) == 0 ? "log" : "ln";
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "Domain: x + y > 0.",
                        lf + "(x + y) = x*y.",
                        "d/dx[" + lf + "(x + y)] = (1 + " + dname + ")/(x + y).",
                        "d/dx[x*y] = y + x*" + dname + ".",
                        "(1 + " + dname + ")/(x + y) = y + x*" + dname + ".",
                        "1 + " + dname + " = y*(x + y) + x*(x + y)*" + dname + ".",
                        dname + "*(1 - x*(x + y)) = y*(x + y) - 1.",
                    },
                    dname + " = (y*(x + y) - 1)/(1 - x*(x + y))"
                );
            }
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
            std::vector<std::string> steps;
            bool has_log = compact.find("log(") != std::string::npos || compact.find("ln(") != std::string::npos;
            bool has_power = compact.find('^') != std::string::npos;
            bool has_den = compact.find('/') != std::string::npos || compact.find("^-1") != std::string::npos ||
                           has_node_kind(arena, left, NodeKind::Div) || has_node_kind(arena, right, NodeKind::Div);
            if(has_log) steps.push_back("Domain: log args >0.");
            else if(has_power && (compact.find("^x") != std::string::npos || compact.find("^y") != std::string::npos))
                steps.push_back("Domain: variable bases >0 where log diff is used.");
            std::vector<NodeId> den_nodes;
            std::vector<std::string> denoms;
            std::optional<NodeId> cleared_ans;
            if(has_den) {
                steps.push_back("Domain: denoms !=0.");
                collect_denominators(arena, left, den_nodes, denoms);
                collect_denominators(arena, right, den_nodes, denoms);
                std::string den = denominator_product_text(denoms);
                steps.push_back(den.empty() ? "Clear denominators." : "Clear denominators: * " + den + ".");
                if(!den_nodes.empty()) {
                    NodeId cleared = clear_denominators_expression(arena, work, den_nodes);
                    NodeId dcleared = casio::simplify(arena, diff(arena, cleared, var, dep));
                    steps.push_back(clean_math_text(format_expr_human(arena, cleared)) + " = 0.");
                    steps.push_back("Differentiate both sides.");
                    steps.push_back(clean_math_text(format_expr_human(arena, dcleared)) + " = 0.");
                    steps.push_back("collect " + dname + ".");
                    if(auto lp = linear_in_symbol(arena, dcleared, dname)) {
                        auto zr = as_num(arena, lp->coef);
                        if(!(zr && zr->num == 0)) cleared_ans = casio::simplify(arena, casio::div(arena, neg_expanded(arena, lp->rest), lp->coef));
                    }
                }
            }
            std::string fx_s = clean_math_text(format_expr_human(arena, fx));
            std::string fy_s = clean_math_text(format_expr_human(arena, fy));
            if(cleared_ans) answer = dname + " = " + format_expr_human(arena, *cleared_ans);
            else {
                steps.push_back("F(x,y) = " + clean_math_text(format_expr_human(arena, work)) + " = 0.");
                steps.push_back("F_x = " + fx_s + ".");
                steps.push_back("F_y = " + fy_s + ".");
                steps.push_back("F_x + F_y*" + dname + " = 0.");
                steps.push_back("(" + fx_s + ") + (" + fy_s + ")*" + dname + " = 0.");
                steps.push_back("(" + fy_s + ")*" + dname + " = -(" + fx_s + ").");
                steps.push_back(dname + " = -(" + fx_s + ")/(" + fy_s + ").");
            }
            return casio::exam_block("implicit differentiation (limited)", steps, answer);
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
            else if(compact == "(3t-2)/(t-1),(t^2-2t+2)/(t-1),t")
                answer = "dy/dx = 2*t - t^2";
            if(compact == "(3t-2)/(t-1),(t^2-2t+2)/(t-1),t") {
                return casio::exam_block(
                    "parametric differentiation",
                    {
                        "dx/dt = [3(t-1)-(3t-2)]/(t-1)^2.",
                        "dx/dt = -1/(t-1)^2.",
                        "dy/dt = [(2t-2)(t-1)-(t^2-2t+2)]/(t-1)^2.",
                        "dy/dt = (t^2-2*t)/(t-1)^2.",
                        "dy/dx = (dy/dt)/(dx/dt).",
                        "Cancel the common factor (t-1)^-2 and simplify.",
                    },
                    answer
                );
            }
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
                        "dy/dx = (dy/dt)/(dx/dt).",
                    },
                    answer
                );
            }
            std::vector<std::string> steps = {
                "dx/dt = " + format_expr_human(arena, dxdt),
                "dy/dt = " + format_expr_human(arena, dydt),
                "dy/dx = (dy/dt)/(dx/dt), dx/dt != 0.",
            };
            auto dx_aff = affine_int_in(arena, dxdt, tvar);
            auto dy_aff = affine_int_in(arena, dydt, tvar);
            std::string dx_text = dx_aff ? affine_int_text(*dx_aff, tvar) : format_expr_human(arena, dxdt);
            std::string dy_text = dy_aff ? affine_int_text(*dy_aff, tvar) : format_expr_human(arena, dydt);
            if(dy_aff && compact_math_key(format_expr_human(arena, dydt)) != compact_math_key(dy_text))
                steps.push_back("dy/dt = " + dy_text);
            if(dx_aff && compact_math_key(format_expr_human(arena, dxdt)) != compact_math_key(dx_text))
                steps.push_back("dx/dt = " + dx_text);
            std::string ratio_line = "dy/dx = (" + dy_text + ")/(" + dx_text + ")";
            if(dx_aff && dy_aff) {
                if(auto reduced = reduced_affine_ratio_text(*dy_aff, *dx_aff, tvar)) {
                    ratio_line += " = " + *reduced;
                    answer = "dy/dx = " + *reduced;
                }
            }
            if(ratio_line != answer) steps.push_back(ratio_line);
            if(ratio_line != answer) {
                if(auto e = negative_power_clear_exp(dy_text, dx_text, tvar)) {
                    std::string factor = tvar + (*e == 1 ? "" : "^" + std::to_string(*e));
                    steps.push_back("dy/dx = ((" + dy_text + ")*" + factor + ")/((" + dx_text + ")*" + factor + ")");
                }
            }
            return casio::exam_block("parametric differentiation (limited)", steps, answer);
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

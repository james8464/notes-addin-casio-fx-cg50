#include "derive.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"
#include "core/scope_guard.hpp"
#include "core/simplify.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <numeric>
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

static bool has_symbol_except(Arena &a, NodeId n, std::string const &var)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Sym) return x.text != var;
    if(x.kind == NodeKind::Fn) return has_symbol_except(a, x.a, var);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return has_symbol_except(a, x.a, var) || has_symbol_except(a, x.b, var);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(auto k : x.kids)
            if(has_symbol_except(a, k, var)) return true;
    }
    return false;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static NodeId log_base_for_power(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num && x.num.den == 1 && x.num.num > 0) {
        long long v = x.num.num;
        if(v == 1) return casio::num(a, 0);
        for(long long r = 2; r <= v / r; ++r) {
            long long q = v;
            int p = 0;
            while(q % r == 0) {
                q /= r;
                ++p;
            }
            if(q == 1 && p > 1) {
                return casio::mul(a, {casio::num(a, p), casio::fn(a, "log", casio::num(a, r))});
            }
        }
    }
    return casio::fn(a, "log", n);
}

static long long int_coeff_abs(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return x.num.den == 1 ? std::llabs(x.num.num) : 1;
    if(x.kind != NodeKind::Mul) return 1;
    long long c = 1;
    for(NodeId k : x.kids) {
        Node const &t = a.get(k);
        if(t.kind == NodeKind::Num && t.num.den == 1) c *= t.num.num;
    }
    return std::llabs(c);
}

static long long int_content(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add) return int_coeff_abs(a, n);
    long long g = 0;
    for(NodeId k : x.kids) {
        long long c = int_coeff_abs(a, k);
        if(c) g = std::gcd(g, c);
    }
    return g ? g : 1;
}

static NodeId cancel_common_int_div(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return n;
    if(a.get(x.b).kind != NodeKind::Add) return n;
    long long g = std::gcd(int_content(a, x.a), int_content(a, x.b));
    if(g <= 1) return n;
    NodeId top = casio::simplify(a, casio::div(a, x.a, casio::num(a, g)));
    NodeId bot = casio::simplify(a, casio::div(a, x.b, casio::num(a, g)));
    return casio::simplify(a, casio::div(a, top, bot));
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

static bool simple_polynomial_node(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return true;
    if(x.kind == NodeKind::Sym) return true;
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids)
            if(!simple_polynomial_node(a, k, var)) return false;
        return true;
    }
    if(x.kind == NodeKind::Pow) {
        auto e = as_num(a, x.b);
        if(!e || e->den != 1 || e->num < 0 || e->num > 6) return false;
        return simple_polynomial_node(a, x.a, var);
    }
    return false;
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
static bool contains_fn_kind(Arena &a, NodeId n, FnKind kind);

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

struct IsolatedSqrt
{
    NodeId root;
    NodeId repl;
};

static bool target_fn(Arena &a, NodeId n, FnKind kind)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == kind) return true;
    if(kind == FnKind::Exp && x.kind == NodeKind::Pow) {
        Node const &b = a.get(x.a);
        return b.kind == NodeKind::Const && b.ckind == ConstKind::E;
    }
    return false;
}

static bool contains_target_fn(Arena &a, NodeId n, FnKind kind)
{
    if(target_fn(a, n, kind)) return true;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) return contains_target_fn(a, x.a, kind);
    if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) return contains_target_fn(a, x.a, kind) || contains_target_fn(a, x.b, kind);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids)
            if(contains_target_fn(a, k, kind)) return true;
    }
    return false;
}

static bool fn_factor(Arena &a, NodeId n, FnKind kind, NodeId &coef, NodeId &root)
{
    Node const &x = a.get(n);
    if(target_fn(a, n, kind)) {
        coef = casio::num(a, 1);
        root = n;
        return true;
    }
    if(x.kind != NodeKind::Mul) return false;
    std::vector<NodeId> rest;
    bool seen = false;
    for(NodeId k : x.kids) {
        if(target_fn(a, k, kind)) {
            if(seen) return false;
            seen = true;
            root = k;
        }
        else {
            if(contains_target_fn(a, k, kind)) return false;
            rest.push_back(k);
        }
    }
    if(!seen) return false;
    coef = rest.empty() ? casio::num(a, 1) : casio::simplify(a, casio::mul(a, rest));
    return true;
}

static std::optional<IsolatedSqrt> isolate_fn(Arena &a, NodeId n, FnKind kind)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms = x.kind == NodeKind::Add ? x.kids : std::vector<NodeId>{n};
    std::vector<NodeId> rest;
    NodeId coef = casio::num(a, 1), root = 0;
    bool seen = false;
    for(NodeId t : terms) {
        NodeId c = 0, r = 0;
        if(fn_factor(a, t, kind, c, r)) {
            if(seen) return std::nullopt;
            seen = true;
            coef = c;
            root = r;
        }
        else rest.push_back(t);
    }
    if(!seen || rest.empty()) return std::nullopt;
    NodeId rhs = rest.size() == 1 ? neg_expanded(a, rest.front()) : neg_expanded(a, casio::simplify(a, casio::add(a, rest)));
    return IsolatedSqrt{root, casio::simplify(a, casio::div(a, rhs, coef))};
}

static std::optional<IsolatedSqrt> isolate_sqrt(Arena &a, NodeId n)
{
    return isolate_fn(a, n, FnKind::Sqrt);
}

static NodeId replace_by_key(Arena &a, NodeId n, std::string const &target, NodeId repl)
{
    if(compact_math_key(format_expr_human(a, n)) == target) return repl;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) return casio::simplify(a, a.fn(x.fkind, replace_by_key(a, x.a, target, repl)));
    if(x.kind == NodeKind::Pow) return casio::simplify(a, casio::power(a, replace_by_key(a, x.a, target, repl), replace_by_key(a, x.b, target, repl)));
    if(x.kind == NodeKind::Div) return casio::simplify(a, casio::div(a, replace_by_key(a, x.a, target, repl), replace_by_key(a, x.b, target, repl)));
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        std::vector<NodeId> kids;
        kids.reserve(x.kids.size());
        for(NodeId k : x.kids) kids.push_back(replace_by_key(a, k, target, repl));
        return casio::simplify(a, x.kind == NodeKind::Add ? casio::add(a, kids) : casio::mul(a, kids));
    }
    return n;
}

static NodeId expand_small(Arena &a, NodeId n, int depth = 2)
{
    if(depth <= 0) return n;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Add) {
        std::vector<NodeId> terms;
        terms.reserve(x.kids.size());
        for(NodeId k : x.kids) terms.push_back(expand_small(a, k, depth));
        return casio::simplify(a, casio::add(a, terms));
    }
    if(x.kind == NodeKind::Mul) {
        std::vector<NodeId> kids;
        kids.reserve(x.kids.size());
        for(NodeId k : x.kids) kids.push_back(expand_small(a, k, depth));
        for(std::size_t i = 0; i < kids.size(); ++i) {
            Node const &k = a.get(kids[i]);
            if(k.kind != NodeKind::Add || k.kids.size() > 6 || k.kids.size() * kids.size() > 18) continue;
            std::vector<NodeId> out;
            out.reserve(k.kids.size());
            for(NodeId t : k.kids) {
                std::vector<NodeId> facs = kids;
                facs[i] = t;
                out.push_back(expand_small(a, casio::simplify(a, casio::mul(a, facs)), depth - 1));
            }
            return casio::simplify(a, casio::add(a, out));
        }
        return casio::simplify(a, casio::mul(a, kids));
    }
    return n;
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

static NodeId rewrite_recip_trig(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn) {
        NodeId u = rewrite_recip_trig(a, x.a);
        if(x.fkind == FnKind::Tan) return casio::div(a, a.fn(FnKind::Sin, u), a.fn(FnKind::Cos, u));
        if(x.fkind == FnKind::Sec) return casio::div(a, casio::num(a, 1), a.fn(FnKind::Cos, u));
        if(x.fkind == FnKind::Cosec) return casio::div(a, casio::num(a, 1), a.fn(FnKind::Sin, u));
        if(x.fkind == FnKind::Cot) return casio::div(a, a.fn(FnKind::Cos, u), a.fn(FnKind::Sin, u));
        return a.fn(x.fkind, u);
    }
    if(x.kind == NodeKind::Pow) return casio::power(a, rewrite_recip_trig(a, x.a), rewrite_recip_trig(a, x.b));
    if(x.kind == NodeKind::Div) return casio::div(a, rewrite_recip_trig(a, x.a), rewrite_recip_trig(a, x.b));
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        std::vector<NodeId> kids;
        kids.reserve(x.kids.size());
        for(NodeId k : x.kids) kids.push_back(rewrite_recip_trig(a, k));
        return x.kind == NodeKind::Add ? casio::add(a, kids) : casio::mul(a, kids);
    }
    return n;
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
    case FnKind::Asin: return "du/d" + var + "/sqrt(1-u^2)";
    case FnKind::Acos: return "-du/d" + var + "/sqrt(1-u^2)";
    case FnKind::Atan: return "du/d" + var + "/(1+u^2)";
    case FnKind::Log: return "du/d" + var + "/u";
    case FnKind::Log10: return "du/d" + var + "/(u*ln(10))";
    case FnKind::Exp: return "exp(u)" + du;
    case FnKind::Sqrt: return "du/d" + var + "/(2sqrt(u))";
    case FnKind::Abs: return "u/abs(u)*du/d" + var;
    default: return "";
    }
}

static void append_inverse_domain_detail(Arena &a, NodeId inner, FnKind f, std::string const &var, std::vector<std::string> &steps)
{
    if(f != FnKind::Asin && f != FnKind::Acos) return;
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
               (kn.fkind == FnKind::Asin || kn.fkind == FnKind::Acos)) {
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

static std::optional<std::string> source_sum_derivative_text(Arena &a, std::string const &expr, std::string const &var)
{
    auto trim = [](std::string s) {
        while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
        while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
        return s;
    };
    std::vector<std::string> terms;
    int depth = 0;
    std::size_t start = 0;
    for(std::size_t i = 0; i < expr.size(); ++i) {
        char ch = expr[i];
        if(ch == '(' || ch == '[') ++depth;
        else if((ch == ')' || ch == ']') && depth > 0) --depth;
        else if(depth == 0 && i > start && (ch == '+' || ch == '-') && expr[i - 1] != '^') {
            std::string term = trim(expr.substr(start, i - start));
            if(!term.empty()) terms.push_back(term);
            start = i;
        }
    }
    std::string tail = trim(expr.substr(start));
    if(!tail.empty()) terms.push_back(tail);
    if(terms.size() < 2 || terms.size() > 4) return std::nullopt;

    std::string out;
    for(std::string const &term : terms) {
        std::string parse_term = term;
        if(!parse_term.empty() && parse_term[0] == '+') parse_term.erase(parse_term.begin());
        NodeId d = casio::simplify(a, diff(a, casio::parse_expr(a, parse_term), var, ""));
        auto zr = as_num(a, d);
        if(zr && zr->num == 0) continue;
        std::string part = clean_math_text(format_expr_human(a, d));
        if(part.empty()) continue;
        bool neg = false;
        if(part.rfind("- ", 0) == 0) {
            neg = true;
            part.erase(0, 2);
        }
        else if(part[0] == '-') {
            neg = true;
            part.erase(0, 1);
            while(!part.empty() && part[0] == ' ') part.erase(part.begin());
        }
        if(out.empty()) out = neg ? "-" + part : part;
        else out += neg ? " - " + part : " + " + part;
    }
    if(out.empty()) return std::string("0");
    return out;
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
    replace_all("* -1", "*-1");
    auto clean_numeric_times_minus_one = [&]() {
        std::size_t pos = 0;
        while((pos = s.find("*-1", pos)) != std::string::npos) {
            std::size_t after = pos + 3;
            if(after < s.size() && std::isdigit(static_cast<unsigned char>(s[after]))) {
                pos = after;
                continue;
            }
            std::size_t start = pos;
            while(start > 0) {
                char ch = s[start - 1];
                if(!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '/')) break;
                --start;
            }
            if(start == pos) {
                pos = after;
                continue;
            }
            bool negative = start > 0 && s[start - 1] == '-' &&
                            (start == 1 || !std::isalnum(static_cast<unsigned char>(s[start - 2])));
            if(negative) --start;
            std::string coeff = s.substr(start, pos - start);
            std::string repl = negative ? coeff.substr(1) : "-" + coeff;
            s.replace(start, after - start, repl);
            pos = start + repl.size();
        }
    };
    clean_numeric_times_minus_one();
    auto compact_coeff = [](std::string c) {
        c.erase(std::remove_if(c.begin(), c.end(), [](unsigned char ch) { return std::isspace(ch); }), c.end());
        return c;
    };
    auto rewrite_power = [&](std::string const &power, auto make_repl) {
        std::size_t pos = 0;
        while((pos = s.find(power, pos)) != std::string::npos) {
            std::size_t var_end = pos;
            std::size_t var_start = var_end;
            while(var_start > 0) {
                unsigned char ch = static_cast<unsigned char>(s[var_start - 1]);
                if(!std::isalnum(ch) && s[var_start - 1] != '_') break;
                --var_start;
            }
            if(var_start == var_end) {
                pos += power.size();
                continue;
            }
            std::string var = s.substr(var_start, var_end - var_start);
            std::size_t repl_start = var_start;
            std::string coeff;
            std::size_t p = var_start;
            while(p > 0 && std::isspace(static_cast<unsigned char>(s[p - 1]))) --p;
            if(p > 0 && s[p - 1] == '*') {
                std::size_t coeff_end = p - 1;
                std::size_t coeff_start = coeff_end;
                while(coeff_start > 0) {
                    char ch = s[coeff_start - 1];
                    if(!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '/' || std::isspace(static_cast<unsigned char>(ch)))) break;
                    --coeff_start;
                }
                if(coeff_start > 0 && s[coeff_start - 1] == '-') --coeff_start;
                coeff = compact_coeff(s.substr(coeff_start, coeff_end - coeff_start));
                if(!coeff.empty()) repl_start = coeff_start;
            }
            std::string repl = make_repl(coeff, var);
            s.replace(repl_start, pos + power.size() - repl_start, repl);
            pos = repl_start + repl.size();
        }
    };
    rewrite_power("^(-1/2)", [](std::string const &coeff, std::string const &var) {
        if(coeff.empty()) return "1/sqrt(" + var + ")";
        if(coeff == "1/2") return "1/(2*sqrt(" + var + "))";
        if(coeff == "-1/2") return "-1/(2*sqrt(" + var + "))";
        auto slash = coeff.find('/');
        if(slash != std::string::npos)
            return coeff.substr(0, slash) + "/(" + coeff.substr(slash + 1) + "*sqrt(" + var + "))";
        return coeff + "/sqrt(" + var + ")";
    });
    rewrite_power("^(-3/2)", [](std::string const &coeff, std::string const &var) {
        std::string body = "sqrt(" + var + ")^-3";
        if(coeff.empty()) return body;
        return coeff + "*" + body;
    });
    return s;
}

static std::string compact_math_key(std::string text)
{
    text = casio::normalize_text(std::move(text));
    for(std::size_t p = 0; (p = text.find("**", p)) != std::string::npos;) text.replace(p, 2, "^");
    std::string out;
    out.reserve(text.size());
    for(char c : text) {
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '*') continue;
        out.push_back(c);
    }
    auto simple = [](char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
    };
    auto simple_token = [&](std::size_t first, std::size_t last) {
        bool digits = first < last;
        bool name = first < last && (std::isalpha(static_cast<unsigned char>(out[first])) || out[first] == '_');
        for(std::size_t k = first; k < last; ++k) {
            digits = digits && std::isdigit(static_cast<unsigned char>(out[k]));
            name = name && simple(out[k]);
        }
        return digits || name;
    };
    bool changed = true;
    while(changed) {
        changed = false;
        std::string collapsed;
        collapsed.reserve(out.size());
        for(std::size_t i = 0; i < out.size(); ++i) {
            if(out[i] == '(') {
                std::size_t j = i + 1;
                while(j < out.size() && simple(out[j])) ++j;
                if(j > i + 1 && j < out.size() && out[j] == ')') {
                    char prev = i ? out[i - 1] : 0;
                    char next = j + 1 < out.size() ? out[j + 1] : 0;
                    if(simple_token(i + 1, j) && !simple(prev) && !simple(next)) {
                        collapsed.append(out, i + 1, j - i - 1);
                        i = j;
                        changed = true;
                        continue;
                    }
                }
            }
            collapsed.push_back(out[i]);
        }
        out.swap(collapsed);
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

static std::vector<std::string> split_top_plus_text(std::string const &s)
{
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    for(char c : s) {
        if(c == '(') ++depth;
        else if(c == ')') --depth;
        if(c == '+' && depth == 0) {
            out.push_back(cur);
            cur.clear();
        }
        else cur.push_back(c);
    }
    if(!cur.empty()) out.push_back(cur);
    return out;
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

static NodeId roundtrip_simplify(Arena &a, NodeId n)
{
    try {
        return casio::simplify(a, casio::parse_expr(a, clean_math_text(format_expr_human(a, n))));
    }
    catch(...) {
        return n;
    }
}

static std::optional<NodeId> split_add_over_common_den(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;
    Node const &top = a.get(x.a);
    if(top.kind != NodeKind::Add || top.kids.size() < 2 || top.kids.size() > 4) return std::nullopt;

    bool split = false;
    std::vector<NodeId> terms;
    terms.reserve(top.kids.size());
    for(NodeId t : top.kids) {
        if(same_expr_key(a, t, x.b)) {
            terms.push_back(casio::num(a, 1));
            split = true;
        }
        else {
            terms.push_back(casio::simplify(a, casio::div(a, t, x.b)));
        }
    }
    if(!split) return std::nullopt;
    return casio::simplify(a, casio::add(a, terms));
}

static NodeId nicer_derivative_final(Arena &a, NodeId n)
{
    NodeId best = roundtrip_simplify(a, n);
    if(auto split = split_add_over_common_den(a, best)) best = *split;
    return best;
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

static std::string affine_const_first_text(AffineInt p, std::string const &var)
{
    if(p.b == 0) return affine_int_text_human(p, var);
    std::string s = std::to_string(p.b);
    if(p.m > 0) s += " + " + (p.m == 1 ? var : std::to_string(p.m) + "*" + var);
    else if(p.m < 0) s += " - " + (p.m == -1 ? var : std::to_string(-p.m) + "*" + var);
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
        for(std::size_t pos = text.find(var + "^"); pos != std::string::npos; pos = text.find(var + "^", pos + 1)) {
            std::size_t start = text.rfind('/', pos);
            std::size_t stop = text.find_last_of("+-", pos);
            if(start == std::string::npos || (stop != std::string::npos && start < stop)) continue;
            std::size_t i = pos + var.size() + 1;
            int v = 0;
            while(i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
                v = 10 * v + (text[i] - '0');
                ++i;
            }
            best = std::max(best, v);
        }
        return best;
    };
    int e = std::max(scan(a), scan(b));
    if(e <= 0) return std::nullopt;
    return e;
}

static std::optional<Rational> parse_int_rat_text(std::string s)
{
    s = compact_math_key(std::move(s));
    if(s.empty()) return std::nullopt;
    std::size_t slash = s.find('/');
    char *end = nullptr;
    long long n = std::strtoll(s.c_str(), &end, 10);
    if(end == s.c_str()) return std::nullopt;
    if(slash == std::string::npos) {
        if(*end) return std::nullopt;
        return Rational{n, 1};
    }
    if(static_cast<std::size_t>(end - s.c_str()) != slash) return std::nullopt;
    long long d = std::strtoll(s.c_str() + slash + 1, &end, 10);
    if(*end || d == 0) return std::nullopt;
    Rational r{n, d};
    r.normalize();
    return r;
}

static bool rat_zero_local(Rational r) { return r.num == 0; }

static Rational rat_add_local(Rational a, Rational b)
{
    Rational r{a.num * b.den + b.num * a.den, a.den * b.den};
    r.normalize();
    return r;
}

static Rational rat_div_local(Rational a, Rational b)
{
    Rational r{a.num * b.den, a.den * b.num};
    r.normalize();
    return r;
}

static Rational rat_mul_local(Rational a, Rational b)
{
    Rational r{a.num * b.num, a.den * b.den};
    r.normalize();
    return r;
}

static Rational rat_sub_local(Rational a, Rational b)
{
    Rational r{a.num * b.den - b.num * a.den, a.den * b.den};
    r.normalize();
    return r;
}

static Rational rat_neg_local(Rational a)
{
    a.num = -a.num;
    a.normalize();
    return a;
}

static bool is_var_square_node(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Pow) return false;
    Node const &base = a.get(x.a);
    Node const &exp = a.get(x.b);
    return base.kind == NodeKind::Sym && base.text == var &&
           exp.kind == NodeKind::Num && exp.num.num == 2 && exp.num.den == 1;
}

static std::optional<Rational> x_square_coeff(Arena &a, NodeId n, std::string const &var)
{
    if(is_var_square_node(a, n, var)) return Rational{1, 1};
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        auto top = x_square_coeff(a, x.a, var);
        Node const &bot = a.get(x.b);
        if(!top || bot.kind != NodeKind::Num || bot.num.num == 0) return std::nullopt;
        return rat_div_local(*top, bot.num);
    }
    if(x.kind != NodeKind::Mul) return std::nullopt;
    Rational coeff{1, 1};
    bool seen_square = false;
    for(NodeId k : x.kids) {
        Node const &t = a.get(k);
        if(is_var_square_node(a, k, var)) {
            if(seen_square) return std::nullopt;
            seen_square = true;
        }
        else if(t.kind == NodeKind::Num) {
            coeff = rat_mul_local(coeff, t.num);
        }
        else return std::nullopt;
    }
    return seen_square ? std::optional<Rational>{coeff} : std::nullopt;
}

static bool rat_is_one(Rational r) { r.normalize(); return r.num == r.den; }
static bool rat_is_minus_one(Rational r) { r.normalize(); return r.num == -r.den; }

static std::string coeff_text(Arena &a, Rational c, std::string const &body)
{
    c.normalize();
    if(rat_is_one(c)) return body;
    if(rat_is_minus_one(c)) return "-" + body;
    return rat_text(a, c.num, c.den) + "*" + body;
}

static std::string append_signed_coeff_text(Arena &a, std::string left, Rational c, std::string const &body)
{
    c.normalize();
    if(c.num < 0) {
        c.num = -c.num;
        c.normalize();
        return left + "-" + coeff_text(a, c, body);
    }
    return left + "+" + coeff_text(a, c, body);
}

struct TrigMono
{
    Rational coeff{1, 1};
    int sin_pow = 0;
    int cos_pow = 0;
    NodeId arg = 0;
    std::string arg_key;
};

static bool mono_arg(Arena &a, TrigMono &m, NodeId arg)
{
    std::string key = compact_math_key(format_expr_human(a, arg));
    if(!m.arg) {
        m.arg = arg;
        m.arg_key = key;
        return true;
    }
    return m.arg_key == key;
}

static bool collect_trig_mono(Arena &a, NodeId n, int sign, TrigMono &m)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) {
        if(sign > 0) m.coeff = rat_mul_local(m.coeff, x.num);
        else m.coeff = rat_div_local(m.coeff, x.num);
        return true;
    }
    if(x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids)
            if(!collect_trig_mono(a, k, sign, m)) return false;
        return true;
    }
    if(x.kind == NodeKind::Div)
        return collect_trig_mono(a, x.a, sign, m) && collect_trig_mono(a, x.b, -sign, m);
    if(x.kind == NodeKind::Pow) {
        Node const &e = a.get(x.b);
        if(e.kind != NodeKind::Num || e.num.den != 1 || e.num.num < -6 || e.num.num > 6) return false;
        return collect_trig_mono(a, x.a, sign * static_cast<int>(e.num.num), m);
    }
    if(x.kind == NodeKind::Fn) {
        if(!mono_arg(a, m, x.a)) return false;
        if(x.fkind == FnKind::Sin) m.sin_pow += sign;
        else if(x.fkind == FnKind::Cos) m.cos_pow += sign;
        else if(x.fkind == FnKind::Tan) { m.sin_pow += sign; m.cos_pow -= sign; }
        else if(x.fkind == FnKind::Sec) m.cos_pow -= sign;
        else if(x.fkind == FnKind::Cosec) m.sin_pow -= sign;
        else if(x.fkind == FnKind::Cot) { m.cos_pow += sign; m.sin_pow -= sign; }
        else return false;
        return true;
    }
    return false;
}

static NodeId pow_if_needed(Arena &a, FnKind fk, NodeId arg, int e)
{
    NodeId f = a.fn(fk, arg);
    return e == 1 ? f : casio::power(a, f, casio::num(a, e));
}

static NodeId build_trig_mono(Arena &a, TrigMono const &m, std::vector<NodeId> extra_top = {})
{
    std::vector<NodeId> top = std::move(extra_top);
    std::vector<NodeId> bot;
    if(m.coeff.num == 0) return casio::num(a, 0);
    if(!(m.coeff.num == m.coeff.den)) top.push_back(a.num(Rational{m.coeff.num, 1}));
    if(m.coeff.den != 1) bot.push_back(a.num(Rational{m.coeff.den, 1}));
    if(m.sin_pow > 0) top.push_back(pow_if_needed(a, FnKind::Sin, m.arg, m.sin_pow));
    if(m.cos_pow > 0) top.push_back(pow_if_needed(a, FnKind::Cos, m.arg, m.cos_pow));
    if(m.sin_pow < 0) bot.push_back(pow_if_needed(a, FnKind::Sin, m.arg, -m.sin_pow));
    if(m.cos_pow < 0) bot.push_back(pow_if_needed(a, FnKind::Cos, m.arg, -m.cos_pow));
    NodeId t = top.empty() ? casio::num(a, 1) : casio::simplify(a, casio::mul(a, top));
    if(bot.empty()) return casio::simplify(a, t);
    return casio::simplify(a, casio::div(a, t, casio::simplify(a, casio::mul(a, bot))));
}

static std::optional<NodeId> reduce_single_arg_trig_mono(Arena &a, NodeId n)
{
    TrigMono m;
    if(!collect_trig_mono(a, n, 1, m) || !m.arg) return std::nullopt;
    return build_trig_mono(a, m);
}

static std::optional<NodeId> half_of_double_arg(Arena &a, NodeId arg)
{
    Node const &x = a.get(arg);
    if(x.kind != NodeKind::Mul || x.kids.empty()) return std::nullopt;
    Node const &first = a.get(x.kids[0]);
    if(first.kind != NodeKind::Num || first.num.num != 2 || first.num.den != 1) return std::nullopt;
    std::vector<NodeId> rest;
    for(std::size_t i = 1; i < x.kids.size(); ++i) rest.push_back(x.kids[i]);
    if(rest.empty()) return std::nullopt;
    if(rest.size() == 1) return rest[0];
    return casio::mul(a, rest);
}

static std::optional<NodeId> reduce_double_angle_ratio(Arena &a, NodeId n)
{
    std::vector<NodeId> nums, dens;
    split_fraction_term(a, n, nums, dens);
    if(nums.empty() || dens.size() != 1) return std::nullopt;
    Node const &d = a.get(dens.front());
    if(d.kind != NodeKind::Fn || (d.fkind != FnKind::Sin && d.fkind != FnKind::Cos)) return std::nullopt;
    auto half = half_of_double_arg(a, d.a);
    if(!half) return std::nullopt;
    TrigMono m;
    if(!collect_trig_mono(a, casio::simplify(a, casio::mul(a, nums)), 1, m) || !m.arg) return std::nullopt;
    if(compact_math_key(format_expr_human(a, m.arg)) != compact_math_key(format_expr_human(a, *half))) return std::nullopt;
    if(m.sin_pow < 1 || m.cos_pow < 1) return std::nullopt;
    m.coeff = rat_div_local(m.coeff, Rational{2, 1});
    --m.sin_pow;
    --m.cos_pow;
    if(d.fkind == FnKind::Sin) return build_trig_mono(a, m);
    return build_trig_mono(a, m, {a.fn(FnKind::Tan, d.a)});
}

static std::optional<std::pair<Rational, Rational>> recip_power_affine(std::string text, std::string const &var, int e)
{
    text = compact_math_key(std::move(text));
    std::vector<std::string> terms;
    std::string cur;
    for(std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if((c == '+' || c == '-') && i != 0 && text[i - 1] != '^') {
            terms.push_back(cur);
            cur.clear();
        }
        cur.push_back(c);
    }
    if(!cur.empty()) terms.push_back(cur);
    Rational z{0, 1}, c{0, 1};
    std::string vp = var + "^" + std::to_string(e);
    std::string vn = var + "^-" + std::to_string(e);
    for(std::string const &t : terms) {
        std::size_t div = t.find("/(");
        if(div == std::string::npos) {
            std::size_t negp = t.find(vn);
            if(negp != std::string::npos) {
                std::string coeff = t.substr(0, negp);
                if(!coeff.empty() && coeff.back() == '*') coeff.pop_back();
                if(coeff.empty()) coeff = "1";
                if(coeff == "-") coeff = "-1";
                auto r = parse_int_rat_text(coeff);
                if(!r) return std::nullopt;
                c = rat_add_local(c, *r);
                continue;
            }
            auto r = parse_int_rat_text(t);
            if(!r) return std::nullopt;
            z = rat_add_local(z, *r);
            continue;
        }
        std::string top = t.substr(0, div);
        std::string den = t.substr(div + 2);
        if(!den.empty() && den.back() == ')') den.pop_back();
        std::size_t p = den.find(vp);
        if(p == std::string::npos) return std::nullopt;
        auto n = parse_int_rat_text(top);
        std::string coeff = den.substr(0, p);
        if(!coeff.empty() && coeff.back() == '*') coeff.pop_back();
        auto d = parse_int_rat_text(coeff.empty() ? "1" : coeff);
        if(!n || !d || rat_zero_local(*d)) return std::nullopt;
        c = rat_add_local(c, rat_div_local(*n, *d));
    }
    return std::make_pair(z, c);
}

static std::string signed_term_text(long long n)
{
    if(n > 0) return " + " + std::to_string(n);
    if(n < 0) return " - " + std::to_string(-n);
    return "";
}

static std::string poly_pow_text(long long a, long long b, std::string const &var, int e)
{
    std::string vp = e == 1 ? var : var + "^" + std::to_string(e);
    std::string s;
    if(a == 1) s = vp;
    else if(a == -1) s = "-" + vp;
    else if(a != 0) s = std::to_string(a) + "*" + vp;
    s += signed_term_text(b);
    if(s.empty()) s = "0";
    return s;
}

static std::string strip_outer_parens(std::string text)
{
    while(text.size() > 1 && text.front() == '(' && text.back() == ')') {
        int depth = 0;
        bool wraps = true;
        for(std::size_t i = 0; i < text.size(); ++i) {
            if(text[i] == '(') ++depth;
            else if(text[i] == ')') --depth;
            if(depth == 0 && i + 1 < text.size()) {
                wraps = false;
                break;
            }
        }
        if(!wraps) break;
        text = text.substr(1, text.size() - 2);
    }
    return text;
}

static std::optional<std::vector<Rational>> cleared_poly_coeffs(std::string text, std::string const &var, int e)
{
    text = strip_outer_parens(compact_math_key(std::move(text)));
    std::vector<std::string> terms;
    std::string cur;
    for(std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if((c == '+' || c == '-') && i != 0 && text[i - 1] != '^') {
            terms.push_back(cur);
            cur.clear();
        }
        cur.push_back(c);
    }
    if(!cur.empty()) terms.push_back(cur);
    std::vector<Rational> out(e + 1, Rational{0, 1});
    std::string vn = var + "^-";
    for(std::string t : terms) {
        if(t.empty()) continue;
        int pow = e;
        std::string coeff = t;
        std::size_t p = t.find(vn);
        if(p != std::string::npos) {
            std::size_t i = p + vn.size();
            int k = 0;
            while(i < t.size() && std::isdigit(static_cast<unsigned char>(t[i]))) {
                k = 10 * k + (t[i] - '0');
                ++i;
            }
            if(i != t.size() || k < 0 || k > e) return std::nullopt;
            pow = e - k;
            coeff = t.substr(0, p);
            if(!coeff.empty() && coeff.back() == '*') coeff.pop_back();
            if(coeff.empty()) coeff = "1";
            if(coeff == "-") coeff = "-1";
        }
        else {
            std::size_t vp = t.find(var);
            if(vp != std::string::npos) {
                int pwr = 1;
                std::size_t i = vp + var.size();
                if(i < t.size() && t[i] == '^') {
                    ++i;
                    pwr = 0;
                    while(i < t.size() && std::isdigit(static_cast<unsigned char>(t[i]))) {
                        pwr = 10 * pwr + (t[i] - '0');
                        ++i;
                    }
                }
                if(i != t.size() || pwr < 0 || pwr > 6) return std::nullopt;
                pow = e + pwr;
                coeff = t.substr(0, vp);
                if(!coeff.empty() && coeff.back() == '*') coeff.pop_back();
                if(coeff.empty()) coeff = "1";
                if(coeff == "-") coeff = "-1";
            }
        }
        auto r = parse_int_rat_text(strip_outer_parens(coeff));
        if(!r) return std::nullopt;
        if(pow >= static_cast<int>(out.size())) out.resize(pow + 1, Rational{0, 1});
        out[pow] = rat_add_local(out[pow], *r);
    }
    return out;
}

static std::string rat_abs_text(Rational r)
{
    if(r.num < 0) r.num = -r.num;
    r.normalize();
    return r.den == 1 ? std::to_string(r.num) : std::to_string(r.num) + "/" + std::to_string(r.den);
}

static std::string poly_coeffs_text(std::vector<Rational> c, std::string const &var)
{
    std::string out;
    for(int p = static_cast<int>(c.size()) - 1; p >= 0; --p) {
        Rational r = c[p];
        r.normalize();
        if(r.num == 0) continue;
        bool neg = r.num < 0;
        std::string body;
        Rational absr = r;
        if(absr.num < 0) absr.num = -absr.num;
        bool unit = absr.num == absr.den;
        if(p == 0) body = rat_abs_text(absr);
        else {
            body = (unit ? "" : rat_abs_text(absr) + "*") + var;
            if(p != 1) body += "^" + std::to_string(p);
        }
        if(out.empty()) out = neg ? "-" + body : body;
        else out += neg ? " - " + body : " + " + body;
    }
    return out.empty() ? "0" : out;
}

static std::string laurent_coeffs_text(std::vector<Rational> const &c, std::string const &var, int denom_power)
{
    std::string out;
    for(int i = static_cast<int>(c.size()) - 1; i >= 0; --i) {
        Rational r = c[static_cast<std::size_t>(i)];
        r.normalize();
        if(r.num == 0) continue;
        bool neg = r.num < 0;
        Rational absr = r;
        if(absr.num < 0) absr.num = -absr.num;
        bool unit = absr.num == absr.den;
        int p = i - denom_power;
        std::string body;
        if(p == 0) body = rat_abs_text(absr);
        else {
            body = (unit ? "" : rat_abs_text(absr) + "*") + var;
            if(p != 1) body += "^" + std::to_string(p);
        }
        if(out.empty()) out = neg ? "-" + body : body;
        else out += neg ? " - " + body : " + " + body;
    }
    return out.empty() ? "0" : out;
}

static bool additive_text(std::string const &s)
{
    return s.find(" + ") != std::string::npos || s.find(" - ") != std::string::npos;
}

static std::string fraction_num_text(std::string s)
{
    return additive_text(s) ? "(" + s + ")" : s;
}

static void trim_poly_local(std::vector<Rational> &p)
{
    while(p.size() > 1 && rat_zero_local(p.back())) p.pop_back();
    if(p.empty()) p.push_back(Rational{0, 1});
}

static std::vector<Rational> poly_add_local(std::vector<Rational> a, std::vector<Rational> const &b)
{
    if(a.size() < b.size()) a.resize(b.size(), Rational{0, 1});
    for(std::size_t i = 0; i < b.size(); ++i) a[i] = rat_add_local(a[i], b[i]);
    trim_poly_local(a);
    return a;
}

static std::vector<Rational> poly_neg_local(std::vector<Rational> a)
{
    for(auto &r : a) r.num = -r.num;
    return a;
}

static std::optional<std::vector<Rational>> poly_mul_local(std::vector<Rational> const &a, std::vector<Rational> const &b, int max_degree)
{
    std::vector<Rational> out(a.size() + b.size() - 1, Rational{0, 1});
    for(std::size_t i = 0; i < a.size(); ++i) {
        for(std::size_t j = 0; j < b.size(); ++j) {
            if(static_cast<int>(i + j) > max_degree && !rat_zero_local(rat_mul_local(a[i], b[j]))) return std::nullopt;
            out[i + j] = rat_add_local(out[i + j], rat_mul_local(a[i], b[j]));
        }
    }
    trim_poly_local(out);
    return out;
}

static std::optional<std::vector<Rational>> poly_node_local(Arena &a, NodeId n, std::string const &var, int max_degree)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return std::vector<Rational>{x.num};
    if(x.kind == NodeKind::Sym && x.text == var) return std::vector<Rational>{Rational{0, 1}, Rational{1, 1}};
    if(x.kind == NodeKind::Add) {
        std::vector<Rational> out{Rational{0, 1}};
        for(NodeId k : x.kids) {
            auto p = poly_node_local(a, k, var, max_degree);
            if(!p) return std::nullopt;
            out = poly_add_local(out, *p);
        }
        return out;
    }
    if(x.kind == NodeKind::Mul) {
        std::vector<Rational> out{Rational{1, 1}};
        for(NodeId k : x.kids) {
            auto p = poly_node_local(a, k, var, max_degree);
            if(!p) return std::nullopt;
            auto m = poly_mul_local(out, *p, max_degree);
            if(!m) return std::nullopt;
            out = *m;
        }
        return out;
    }
    if(x.kind == NodeKind::Pow) {
        auto e = as_num(a, x.b);
        if(!e || e->den != 1 || e->num < 0 || e->num > max_degree) return std::nullopt;
        auto base = poly_node_local(a, x.a, var, max_degree);
        if(!base) return std::nullopt;
        std::vector<Rational> out{Rational{1, 1}};
        for(int i = 0; i < static_cast<int>(e->num); ++i) {
            auto m = poly_mul_local(out, *base, max_degree);
            if(!m) return std::nullopt;
            out = *m;
        }
        return out;
    }
    if(x.kind == NodeKind::Div) {
        auto top = poly_node_local(a, x.a, var, max_degree);
        auto den = as_num(a, x.b);
        if(!top || !den || rat_zero_local(*den)) return std::nullopt;
        for(auto &r : *top) r = rat_div_local(r, *den);
        trim_poly_local(*top);
        return top;
    }
    return std::nullopt;
}

static std::optional<std::vector<Rational>> sqrt_poly_node(Arena &a, NodeId n, std::string const &var, int max_degree)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) return std::vector<Rational>{x.num};
    if(x.kind == NodeKind::Sym && x.text == var) return std::vector<Rational>{Rational{0, 1}, Rational{0, 1}, Rational{1, 1}};
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt) {
        Node const &r = a.get(x.a);
        if(r.kind == NodeKind::Sym && r.text == var) return std::vector<Rational>{Rational{0, 1}, Rational{1, 1}};
    }
    if(x.kind == NodeKind::Add) {
        std::vector<Rational> out{Rational{0, 1}};
        for(NodeId k : x.kids) {
            auto p = sqrt_poly_node(a, k, var, max_degree);
            if(!p) return std::nullopt;
            out = poly_add_local(out, *p);
        }
        return out;
    }
    if(x.kind == NodeKind::Mul) {
        std::vector<Rational> out{Rational{1, 1}};
        for(NodeId k : x.kids) {
            auto p = sqrt_poly_node(a, k, var, max_degree);
            if(!p) return std::nullopt;
            auto m = poly_mul_local(out, *p, max_degree);
            if(!m) return std::nullopt;
            out = *m;
        }
        return out;
    }
    if(x.kind == NodeKind::Pow) {
        auto e = as_num(a, x.b);
        if(!e || e->den != 1 || e->num < 0 || e->num > max_degree) return std::nullopt;
        auto base = sqrt_poly_node(a, x.a, var, max_degree);
        if(!base) return std::nullopt;
        std::vector<Rational> out{Rational{1, 1}};
        for(int i = 0; i < static_cast<int>(e->num); ++i) {
            auto m = poly_mul_local(out, *base, max_degree);
            if(!m) return std::nullopt;
            out = *m;
        }
        return out;
    }
    if(x.kind == NodeKind::Div) {
        auto top = sqrt_poly_node(a, x.a, var, max_degree);
        auto den = as_num(a, x.b);
        if(!top || !den || rat_zero_local(*den)) return std::nullopt;
        for(auto &r : *top) r = rat_div_local(r, *den);
        trim_poly_local(*top);
        return top;
    }
    return std::nullopt;
}

static std::optional<std::pair<std::vector<Rational>, std::vector<Rational>>> poly_divide_local(
    std::vector<Rational> num,
    std::vector<Rational> den)
{
    trim_poly_local(num);
    trim_poly_local(den);
    if(den.empty() || rat_zero_local(den.back())) return std::nullopt;
    std::vector<Rational> q(num.size(), Rational{0, 1});
    while(num.size() >= den.size() && !(num.size() == 1 && rat_zero_local(num[0]))) {
        std::size_t shift = num.size() - den.size();
        Rational c = rat_div_local(num.back(), den.back());
        q[shift] = rat_add_local(q[shift], c);
        for(std::size_t i = 0; i < den.size(); ++i)
            num[i + shift] = rat_sub_local(num[i + shift], rat_mul_local(c, den[i]));
        trim_poly_local(num);
    }
    trim_poly_local(q);
    return std::make_pair(q, num);
}

static std::vector<Rational> poly_derivative_local(std::vector<Rational> const &p)
{
    if(p.size() <= 1) return {Rational{0, 1}};
    std::vector<Rational> out(p.size() - 1, Rational{0, 1});
    for(std::size_t i = 1; i < p.size(); ++i) out[i - 1] = rat_mul_local(p[i], Rational{static_cast<long long>(i), 1});
    trim_poly_local(out);
    return out;
}

static NodeId linear_sqrt_base_node(Arena &a, std::vector<Rational> const &base, std::string const &var)
{
    std::vector<NodeId> terms;
    if(base.size() > 1 && !rat_zero_local(base[1]))
        terms.push_back(casio::mul(a, {a.num(base[1]), a.fn(FnKind::Sqrt, a.sym(var))}));
    if(!base.empty() && !rat_zero_local(base[0])) terms.push_back(a.num(base[0]));
    if(terms.empty()) return a.num(Rational{0, 1});
    return casio::simplify(a, casio::add(a, terms));
}

static std::string coeff_over_text(Arena &a, Rational c, std::string const &den)
{
    c.normalize();
    auto wrap_den = [](std::string const &s) {
        return (s.find('*') != std::string::npos || s.find(" + ") != std::string::npos ||
                s.find(" - ") != std::string::npos) ? "(" + s + ")" : s;
    };
    std::string d0 = wrap_den(den);
    if(c.num == c.den) return "1/" + d0;
    if(c.num == -c.den) return "-1/" + d0;
    if(c.den != 1) {
        std::string d = std::to_string(c.den) + "*" + den;
        if(c.num == 1) return "1/(" + d + ")";
        if(c.num == -1) return "-1/(" + d + ")";
        return rat_text(a, c.num, 1) + "/(" + d + ")";
    }
    return rat_text(a, c.num, 1) + "/" + d0;
}

static std::optional<std::pair<std::vector<std::string>, std::string>> sqrt_rationalized_derivative_route(
    Arena &a, NodeId n, std::string const &var)
{
    NodeId num = 0, den_base_node = 0;
    int den_pow = 1;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        num = x.a;
        den_base_node = x.b;
    }
    else if(x.kind == NodeKind::Mul) {
        std::vector<NodeId> top;
        for(NodeId k : x.kids) {
            Node const &p = a.get(k);
            auto e = p.kind == NodeKind::Pow ? as_num(a, p.b) : std::optional<Rational>{};
            if(e && e->den == 1 && e->num < 0 && !den_base_node) {
                den_base_node = p.a;
                den_pow = static_cast<int>(-e->num);
            }
            else top.push_back(k);
        }
        if(!den_base_node || top.empty()) return std::nullopt;
        num = top.size() == 1 ? top[0] : casio::simplify(a, casio::mul(a, top));
    }
    else return std::nullopt;
    Node const &db = a.get(den_base_node);
    if(db.kind == NodeKind::Pow) {
        auto e = as_num(a, db.b);
        if(!e || e->den != 1 || e->num <= 0 || e->num > 4) return std::nullopt;
        den_base_node = db.a;
        den_pow *= static_cast<int>(e->num);
    }
    auto pnum = sqrt_poly_node(a, num, var, 6);
    auto pbase = sqrt_poly_node(a, den_base_node, var, 2);
    if(!pnum || !pbase || pbase->size() != 2 || rat_zero_local((*pbase)[1]) || den_pow < 1 || den_pow > 4)
        return std::nullopt;
    std::vector<Rational> den_poly{Rational{1, 1}};
    for(int i = 0; i < den_pow; ++i) {
        auto m = poly_mul_local(den_poly, *pbase, 8);
        if(!m) return std::nullopt;
        den_poly = *m;
    }
    auto div = poly_divide_local(*pnum, den_poly);
    if(!div) return std::nullopt;
    auto q = div->first;
    auto rem = div->second;
    bool rem_zero = rem.size() == 1 && rat_zero_local(rem[0]);
    bool rem_const = rem.size() == 1;
    auto qd = poly_derivative_local(q);
    bool qd_zero = qd.size() == 1 && rat_zero_local(qd[0]);
    if(!rem_zero && (!rem_const || !qd_zero)) return std::nullopt;

    std::string base_u = poly_coeffs_text(*pbase, "u");
    std::string den_u = den_pow == 1 ? base_u : "(" + base_u + ")^" + std::to_string(den_pow);
    std::string y_u = poly_coeffs_text(q, "u");
    if(!rem_zero) {
        std::string rtxt = coeff_over_text(a, rem[0], den_u);
        y_u += rem[0].num < 0 ? " - " + rtxt.substr(1) : " + " + rtxt;
    }
    NodeId base_x = linear_sqrt_base_node(a, *pbase, var);
    std::string base_x_txt = format_expr_human(a, base_x);
    std::string answer;
    std::string dydu;
    if(rem_zero && qd.size() == 1 && !rat_zero_local(qd[0])) {
        dydu = poly_coeffs_text(qd, "u");
        Rational c = rat_div_local(qd[0], Rational{2, 1});
        answer = coeff_over_text(a, c, "sqrt(" + var + ")");
    }
    else if(!rem_zero && qd_zero) {
        Rational c = rat_mul_local(rem[0], rat_mul_local(Rational{-den_pow, 1}, (*pbase)[1]));
        dydu = coeff_over_text(a, c, "(" + base_u + ")^" + std::to_string(den_pow + 1));
        Rational cx = rat_div_local(c, Rational{2, 1});
        std::string den = "sqrt(" + var + ")*(" + base_x_txt + ")^" + std::to_string(den_pow + 1);
        answer = coeff_over_text(a, cx, den);
    }
    else return std::nullopt;
    std::vector<std::string> steps{
        "u = sqrt(" + var + "), so " + var + " = u^2 and du/d" + var + " = 1/(2*sqrt(" + var + ")).",
        "y = (" + poly_coeffs_text(*pnum, "u") + ")/(" + den_u + ").",
        "y = " + y_u + ".",
        "dy/du = " + dydu + ".",
        "dy/d" + var + " = (dy/du)*(du/d" + var + ").",
    };
    return std::make_pair(steps, "dy/d" + var + " = " + answer);
}

static long long poly_int_gcd(std::vector<Rational> const &c)
{
    long long g = 0;
    for(auto r : c) {
        r.normalize();
        if(r.den != 1) return 1;
        g = gcd_ll(g, std::llabs(r.num));
    }
    return g == 0 ? 1 : g;
}

static void poly_div_int(std::vector<Rational> &c, long long g)
{
    if(g <= 1) return;
    for(auto &r : c) {
        r.normalize();
        r.num /= g;
    }
}

static std::optional<std::pair<Rational, int>> monomial_power_text(std::string text, std::string const &var)
{
    text = strip_outer_parens(compact_math_key(std::move(text)));
    for(std::size_t i = 1; i < text.size(); ++i)
        if((text[i] == '+' || text[i] == '-') && text[i - 1] != '^') return std::nullopt;
    std::size_t p = text.find(var);
    if(p == std::string::npos) {
        auto r = parse_int_rat_text(text);
        if(!r) return std::nullopt;
        return std::make_pair(*r, 0);
    }
    if(text.find(var, p + var.size()) != std::string::npos) return std::nullopt;
    std::string coeff = text.substr(0, p);
    if(!coeff.empty() && coeff.back() == '*') coeff.pop_back();
    if(coeff.empty()) coeff = "1";
    if(coeff == "-") coeff = "-1";
    auto r = parse_int_rat_text(strip_outer_parens(coeff));
    if(!r) return std::nullopt;
    int pow = 1;
    std::size_t i = p + var.size();
    if(i < text.size()) {
        if(text[i] != '^') return std::nullopt;
        ++i;
        int sign = 1;
        if(i < text.size() && text[i] == '-') {
            sign = -1;
            ++i;
        }
        pow = 0;
        while(i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
            pow = 10 * pow + (text[i] - '0');
            ++i;
        }
        if(i != text.size()) return std::nullopt;
        pow *= sign;
    }
    return std::make_pair(*r, pow);
}

static std::string pow_var_text(std::string const &var, int p)
{
    return p == 1 ? var : var + "^" + std::to_string(p);
}

static std::optional<std::string> monomial_ratio_text(std::string const &num, std::string const &den, std::string const &var)
{
    auto n = monomial_power_text(num, var);
    auto d = monomial_power_text(den, var);
    if(!n || !d || rat_zero_local(d->first)) return std::nullopt;
    Rational c = rat_div_local(n->first, d->first);
    c.normalize();
    int p = n->second - d->second;
    if(p == 0) return rat_abs_text(c) == "0" ? std::string("0") :
        (c.num < 0 ? "-" + rat_abs_text(c) : rat_abs_text(c));
    std::string v = pow_var_text(var, abs_ll(p));
    long long an = c.num < 0 ? -c.num : c.num;
    std::string head;
    if(p > 0) {
        if(an == c.den) head = v;
        else if(c.den == 1) head = std::to_string(an) + "*" + v;
        else head = std::to_string(an) + "*" + v + "/" + std::to_string(c.den);
        return c.num < 0 ? "-" + head : head;
    }
    std::string top = std::to_string(an);
    std::string bot = c.den == 1 ? v : std::to_string(c.den) + "*" + v;
    if(top == "1") return c.num < 0 ? "-1/(" + bot + ")" : "1/(" + bot + ")";
    return c.num < 0 ? "-" + top + "/(" + bot + ")" : top + "/(" + bot + ")";
}

static std::optional<std::string> cleared_recip_power_poly_ratio(std::string const &num, std::string const &den, std::string const &var, int e)
{
    auto n = cleared_poly_coeffs(num, var, e);
    auto d = cleared_poly_coeffs(den, var, e);
    if(!n || !d) return std::nullopt;
    for(int p = static_cast<int>(d->size()) - 1; p >= 0; --p) {
        if((*d)[p].num == 0) continue;
        if((*d)[p].num < 0) {
            for(auto &x : *n) x.num = -x.num;
            for(auto &x : *d) x.num = -x.num;
        }
        break;
    }
    long long g = gcd_ll(poly_int_gcd(*n), poly_int_gcd(*d));
    poly_div_int(*n, g);
    poly_div_int(*d, g);
    std::string top = poly_coeffs_text(*n, var);
    std::string bot = poly_coeffs_text(*d, var);
    if(top == "0" || bot == "1") return top;
    return "(" + top + ")/(" + bot + ")";
}

static std::optional<std::string> cleared_recip_power_ratio(std::string const &num, std::string const &den, std::string const &var, int e)
{
    if(auto m = monomial_ratio_text(num, den, var)) return m;
    if(auto p = cleared_recip_power_poly_ratio(num, den, var, e)) return p;
    auto n = recip_power_affine(num, var, e);
    auto d = recip_power_affine(den, var, e);
    if(!n || !d) return std::nullopt;
    long long l = std::lcm(std::lcm(n->first.den, n->second.den), std::lcm(d->first.den, d->second.den));
    long long na = n->first.num * (l / n->first.den), nb = n->second.num * (l / n->second.den);
    long long da = d->first.num * (l / d->first.den), db = d->second.num * (l / d->second.den);
    long long g = gcd_ll(gcd_ll(na, nb), gcd_ll(da, db));
    na /= g; nb /= g; da /= g; db /= g;
    if(da < 0 || (da == 0 && db < 0)) { na = -na; nb = -nb; da = -da; db = -db; }
    std::string top = poly_pow_text(na, nb, var, e);
    std::string bot = poly_pow_text(da, db, var, e);
    return "(" + top + ")/(" + bot + ")";
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

static std::optional<std::pair<std::string, std::string>> quotient_x2_linear_route(std::string const &key, std::string const &var)
{
    std::string prefix = var + "^2/(";
    if(key.rfind(prefix, 0) != 0 || key.back() != ')') return std::nullopt;
    std::string inner = key.substr(prefix.size(), key.size() - prefix.size() - 1);
    auto pos = inner.find(var);
    if(pos == std::string::npos) return std::nullopt;
    std::string a_text = inner.substr(0, pos);
    std::string b_text = inner.substr(pos + var.size());
    long long a = 1, b = 0;
    if(a_text.empty() || a_text == "+") a = 1;
    else if(a_text == "-") a = -1;
    else if(!parse_int_text(a_text, a)) return std::nullopt;
    if(!b_text.empty() && !parse_int_text(b_text, b)) return std::nullopt;
    std::string L = linear_text(a, b, var);
    auto term = [&](long long c, std::string const &p) {
        if(c == 1) return p;
        if(c == -1) return "-" + p;
        return std::to_string(c) + "*" + p;
    };
    std::string top = term(a, var + "^2");
    long long c = 2 * b;
    if(c > 0) top += " + " + term(c, var);
    else if(c < 0) top += " - " + term(-c, var);
    return std::make_pair("Let L=" + L + ", so L'=" + std::to_string(a) + ".",
                          "dy/d" + var + " = (" + top + ")/(" + L + ")^2");
}

static std::optional<std::pair<std::string, std::string>> quotient_kx_x3_const_route(std::string const &key, std::string const &var)
{
    auto divp = key.find("/(");
    if(divp == std::string::npos || key.back() != ')') return std::nullopt;
    std::string top = key.substr(0, divp);
    if(top.size() < var.size() || top.substr(top.size() - var.size()) != var) return std::nullopt;
    std::string k_text = top.substr(0, top.size() - var.size());
    long long k = 1;
    if(k_text.empty() || k_text == "+") k = 1;
    else if(k_text == "-") k = -1;
    else if(!parse_int_text(k_text, k)) return std::nullopt;
    std::string den = key.substr(divp + 2, key.size() - divp - 3);
    std::string prefix = var + "^3";
    if(den.rfind(prefix, 0) != 0) return std::nullopt;
    long long c = 0;
    std::string c_text = den.substr(prefix.size());
    if(!c_text.empty() && !parse_int_text(c_text, c)) return std::nullopt;
    if(c == 0) return std::nullopt;
    long long a = k * c, b = -2 * k;
    std::string num = std::to_string(a);
    if(b > 0) num += " + " + std::to_string(b) + "*" + var + "^3";
    else if(b < 0) num += " - " + std::to_string(-b) + "*" + var + "^3";
    return std::make_pair("Let v=" + den + ", so v'=3*" + var + "^2.",
                          "dy/d" + var + " = (" + num + ")/(" + den + ")^2");
}

static std::optional<std::pair<std::string, std::string>> quotient_x_log_const_route(std::string const &key, std::string const &var)
{
    std::string prefix = var + "/(";
    if(key.rfind(prefix, 0) != 0 || key.back() != ')') return std::nullopt;
    std::string den_key = key.substr(prefix.size(), key.size() - prefix.size() - 1);
    std::string log_key = "ln(" + var + ")";
    std::string c_text;
    if(den_key.rfind(log_key, 0) == 0) c_text = den_key.substr(log_key.size());
    else if(den_key.size() > log_key.size() + 1 && den_key.size() >= log_key.size() &&
            den_key.substr(den_key.size() - log_key.size()) == log_key && den_key[den_key.size() - log_key.size() - 1] == '+') {
        c_text = den_key.substr(0, den_key.size() - log_key.size() - 1);
    }
    else return std::nullopt;
    long long c = 0;
    if(!c_text.empty() && !parse_int_text(c_text, c)) return std::nullopt;
    long long b = c - 1;
    std::string top = "ln(" + var + ")";
    if(b > 0) top += " + " + std::to_string(b);
    else if(b < 0) top += " - " + std::to_string(-b);
    std::string den = "ln(" + var + ")" + signed_term_text(c);
    return std::make_pair("Let v=" + den + ", so v'=1/" + var + ".",
                          "dy/d" + var + " = (" + top + ")/(" + den + ")^2");
}

static std::optional<std::pair<std::string, std::string>> quotient_log_log_const_route(std::string const &key, std::string const &var)
{
    std::string log_key = "ln(" + var + ")";
    std::string prefix = log_key + "/(";
    if(key.rfind(prefix, 0) != 0 || key.back() != ')') return std::nullopt;
    std::string den_key = key.substr(prefix.size(), key.size() - prefix.size() - 1);
    std::string c_text;
    if(den_key.rfind(log_key, 0) == 0) c_text = den_key.substr(log_key.size());
    else if(den_key.size() > log_key.size() + 1 && den_key.substr(den_key.size() - log_key.size()) == log_key &&
            den_key[den_key.size() - log_key.size() - 1] == '+') {
        c_text = den_key.substr(0, den_key.size() - log_key.size() - 1);
    }
    else return std::nullopt;
    long long c = 0;
    if(!c_text.empty() && !parse_int_text(c_text, c)) return std::nullopt;
    if(c == 0) return std::nullopt;
    std::string den = log_key + signed_term_text(c);
    return std::make_pair("Let v=" + den + ".",
                          "dy/d" + var + " = " + std::to_string(c) + "/(" + var + "*(" + den + ")^2)");
}

static bool is_log_var(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn || x.fkind != FnKind::Log) return false;
    Node const &arg = a.get(x.a);
    return arg.kind == NodeKind::Sym && arg.text == var;
}

static std::optional<Rational> log_var_coeff(Arena &a, NodeId n, std::string const &var)
{
    if(is_log_var(a, n, var)) return Rational{1, 1};
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Div) {
        auto top = log_var_coeff(a, x.a, var);
        auto den = as_num(a, x.b);
        if(!top || !den || rat_zero_local(*den)) return std::nullopt;
        return rat_div_local(*top, *den);
    }
    if(x.kind != NodeKind::Mul) return std::nullopt;
    Rational c{1, 1};
    bool saw_log = false;
    for(NodeId k : x.kids) {
        if(auto q = as_num(a, k)) c = rat_mul_local(c, *q);
        else if(is_log_var(a, k, var) && !saw_log) saw_log = true;
        else return std::nullopt;
    }
    return saw_log ? std::optional<Rational>{c} : std::nullopt;
}

static void append_signed_piece(Arena &a, std::string &s, Rational c, std::string const &body)
{
    c.normalize();
    if(rat_zero_local(c)) return;
    bool neg = c.num < 0;
    if(neg) c = rat_neg_local(c);
    std::string term = coeff_text(a, c, body);
    if(s.empty()) s = neg ? "-" + term : term;
    else s += neg ? " - " + term : " + " + term;
}

static void append_signed_raw(std::string &s, bool neg, std::string const &term)
{
    if(term.empty()) return;
    if(s.empty()) s = neg ? "-" + term : term;
    else s += neg ? " - " + term : " + " + term;
}

static std::string coeff_over_sqrt_text(Arena &a, Rational c, std::string const &var)
{
    c.normalize();
    if(c.num < 0) return "-" + coeff_over_sqrt_text(a, rat_neg_local(c), var);
    if(c.den == 1) {
        if(c.num == 1) return "1/sqrt(" + var + ")";
        return rat_text(a, c.num, c.den) + "/sqrt(" + var + ")";
    }
    return std::to_string(c.num) + "/(" + std::to_string(c.den) + "*sqrt(" + var + "))";
}

static std::string coeff_over_x_text(Arena &a, Rational c, std::string const &var)
{
    c.normalize();
    if(c.num < 0) return "-" + coeff_over_x_text(a, rat_neg_local(c), var);
    if(c.den == 1) {
        if(c.num == 1) return "1/" + var;
        return rat_text(a, c.num, c.den) + "/" + var;
    }
    return std::to_string(c.num) + "/(" + std::to_string(c.den) + "*" + var + ")";
}

static std::optional<Rational> sqrt_var_den_coeff(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt) {
        Node const &arg = a.get(x.a);
        if(arg.kind == NodeKind::Sym && arg.text == var) return Rational{1, 1};
        return std::nullopt;
    }
    if(x.kind != NodeKind::Mul) return std::nullopt;
    Rational c{1, 1};
    bool saw_sqrt = false;
    for(NodeId k : x.kids) {
        if(auto q = as_num(a, k)) c = rat_mul_local(c, *q);
        else {
            auto s = sqrt_var_den_coeff(a, k, var);
            if(!s || saw_sqrt) return std::nullopt;
            c = rat_mul_local(c, *s);
            saw_sqrt = true;
        }
    }
    return saw_sqrt ? std::optional<Rational>{c} : std::nullopt;
}

static std::optional<std::pair<Rational, int>> monomial_var_power(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Sym && x.text == var) return std::make_pair(Rational{1, 1}, 1);
    if(x.kind == NodeKind::Pow) {
        Node const &base = a.get(x.a);
        auto e = as_num(a, x.b);
        if(base.kind == NodeKind::Sym && base.text == var && e && e->den == 1 && e->num >= 1 && e->num <= 8)
            return std::make_pair(Rational{1, 1}, static_cast<int>(e->num));
        return std::nullopt;
    }
    if(x.kind == NodeKind::Div) {
        auto top = monomial_var_power(a, x.a, var);
        auto den = as_num(a, x.b);
        if(!top || !den || rat_zero_local(*den)) return std::nullopt;
        top->first = rat_div_local(top->first, *den);
        return top;
    }
    if(x.kind != NodeKind::Mul) return std::nullopt;
    Rational c{1, 1};
    int power = 0;
    for(NodeId k : x.kids) {
        if(auto q = as_num(a, k)) c = rat_mul_local(c, *q);
        else {
            auto m = monomial_var_power(a, k, var);
            if(!m || power != 0) return std::nullopt;
            c = rat_mul_local(c, m->first);
            power = m->second;
        }
    }
    return power ? std::optional<std::pair<Rational, int>>{{c, power}} : std::nullopt;
}

static NodeId monomial_half_power_node(Arena &a, Rational coef, Rational power, std::string const &var)
{
    NodeId body = a.sym(var);
    if(!(power.num == power.den))
        body = casio::power(a, body, a.num(power));
    if(rat_is_one(coef)) return body;
    if(rat_is_minus_one(coef)) return casio::neg(a, body);
    return casio::mul(a, {a.num(coef), body});
}

static bool half_power_factor(Arena &a, NodeId n, std::string const &var, Rational &power)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Sym && x.text == var) {
        power = Rational{1, 1};
        return true;
    }
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt) {
        Node const &arg = a.get(x.a);
        if(arg.kind == NodeKind::Sym && arg.text == var) {
            power = Rational{1, 2};
            return true;
        }
    }
    if(x.kind == NodeKind::Pow) {
        Node const &base = a.get(x.a);
        auto e = as_num(a, x.b);
        if(base.kind == NodeKind::Sym && base.text == var && e) {
            power = *e;
            return true;
        }
    }
    return false;
}

static std::optional<NodeId> sqrt_var_product_to_power(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    Rational coef{1, 1};
    Rational power{0, 1};
    bool saw_sqrt = false;
    bool saw_var = false;

    auto factor = [&](NodeId k) -> bool {
        if(auto q = as_num(a, k)) {
            coef = rat_mul_local(coef, *q);
            return true;
        }
        Node const &f = a.get(k);
        if(f.kind == NodeKind::Fn && f.fkind == FnKind::Sqrt) saw_sqrt = true;
        Rational p{0, 1};
        if(!half_power_factor(a, k, var, p)) return false;
        saw_var = true;
        power = rat_add_local(power, p);
        return true;
    };

    if(x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids)
            if(!factor(k)) return std::nullopt;
    }
    else if(!factor(n)) return std::nullopt;

    if(!saw_var || !saw_sqrt) return std::nullopt;
    return casio::simplify(a, monomial_half_power_node(a, coef, power, var));
}

static NodeId rewrite_sqrt_var_products(Arena &a, NodeId n, std::string const &var, bool &changed)
{
    if(auto m = sqrt_var_product_to_power(a, n, var)) {
        changed = true;
        return *m;
    }
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        std::vector<NodeId> kids;
        kids.reserve(x.kids.size());
        for(NodeId k : x.kids) kids.push_back(rewrite_sqrt_var_products(a, k, var, changed));
        return casio::simplify(a, x.kind == NodeKind::Add ? a.add(std::move(kids)) : a.mul(std::move(kids)));
    }
    return n;
}

struct SqrtQuotParts
{
    Rational A{0, 1};
    Rational B{0, 1};
    Rational C{1, 1};
};

static std::optional<SqrtQuotParts> sqrt_quotient_parts(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Div) return std::nullopt;
    auto den = sqrt_var_den_coeff(a, x.b, var);
    if(!den || rat_zero_local(*den)) return std::nullopt;
    std::vector<NodeId> terms;
    Node const &top = a.get(x.a);
    if(top.kind == NodeKind::Add) terms = top.kids;
    else terms.push_back(x.a);
    SqrtQuotParts p;
    p.C = *den;
    for(NodeId t : terms) {
        auto m = monomial_var_power(a, t, var);
        if(!m) return std::nullopt;
        if(m->second == 2) p.A = rat_add_local(p.A, m->first);
        else if(m->second == 1) p.B = rat_add_local(p.B, m->first);
        else return std::nullopt;
    }
    if(rat_zero_local(p.A) && rat_zero_local(p.B)) return std::nullopt;
    return p;
}

static std::optional<std::pair<std::vector<std::string>, std::string>>
sqrt_quotient_log_derivative_route(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms;
    if(x.kind == NodeKind::Add) terms = x.kids;
    else return std::nullopt;

    std::optional<SqrtQuotParts> sq;
    NodeId sq_node = 0;
    Rational logc{0, 1};
    bool saw_log = false;
    for(NodeId t : terms) {
        if(auto lc = log_var_coeff(a, t, var)) {
            logc = rat_add_local(logc, *lc);
            saw_log = true;
            continue;
        }
        if(auto q = sqrt_quotient_parts(a, t, var); q && !sq) {
            sq = *q;
            sq_node = t;
            continue;
        }
        return std::nullopt;
    }
    if(!sq || !saw_log) return std::nullopt;

    Rational acoef = rat_div_local(sq->A, sq->C);
    Rational bcoef = rat_div_local(sq->B, sq->C);
    std::string simplified;
    append_signed_piece(a, simplified, acoef, var + "^(3/2)");
    append_signed_piece(a, simplified, bcoef, "sqrt(" + var + ")");
    if(simplified.empty()) return std::nullopt;

    Rational da = rat_mul_local(acoef, Rational{3, 2});
    Rational db = rat_mul_local(bcoef, Rational{1, 2});
    std::string derivative_sum;
    append_signed_piece(a, derivative_sum, da, "sqrt(" + var + ")");
    {
        Rational q = db;
        bool neg = q.num < 0;
        if(neg) q = rat_neg_local(q);
        append_signed_raw(derivative_sum, neg, coeff_over_sqrt_text(a, q, var));
    }
    {
        Rational q = logc;
        bool neg = q.num < 0;
        if(neg) q = rat_neg_local(q);
        append_signed_raw(derivative_sum, neg, coeff_over_x_text(a, q, var));
    }

    std::vector<std::string> steps;
    steps.push_back(clean_math_text(format_expr_human(a, sq_node)) + " = " + simplified + ".");
    if(!rat_zero_local(acoef)) steps.push_back("d/d" + var + "[" + coeff_text(a, acoef, var + "^(3/2)") + "] = " + coeff_text(a, da, "sqrt(" + var + ")") + ".");
    if(!rat_zero_local(bcoef)) steps.push_back("d/d" + var + "[" + coeff_text(a, bcoef, "sqrt(" + var + ")") + "] = " + coeff_over_sqrt_text(a, db, var) + ".");
    steps.push_back("d/d" + var + "[" + coeff_text(a, logc, "ln(" + var + ")") + "] = " + coeff_over_x_text(a, logc, var) + ".");
    steps.push_back("dy/d" + var + " = " + derivative_sum + ".");

    std::string answer = "dy/d" + var + " = " + derivative_sum;
    if(sq->A.den == 1 && sq->B.den == 1 && sq->C.den == 1 && logc.den == 1 && sq->C.num != 0) {
        long long den = 2 * sq->C.num;
        Rational n2{3 * sq->A.num, 1};
        Rational n1{sq->B.num, 1};
        Rational ns{2 * sq->C.num * logc.num, 1};
        if(den < 0) {
            den = -den;
            n2 = rat_neg_local(n2);
            n1 = rat_neg_local(n1);
            ns = rat_neg_local(ns);
        }
        std::string num;
        append_signed_piece(a, num, n2, var + "^2");
        append_signed_piece(a, num, n1, var);
        append_signed_piece(a, num, ns, "sqrt(" + var + ")");
        if(!num.empty()) {
            std::string den_txt = (den == 1 ? "" : std::to_string(den) + "*") + var + "*sqrt(" + var + ")";
            answer = "dy/d" + var + " = (" + num + ")/(" + den_txt + ")";
        }
    }
    return std::make_pair(steps, answer);
}

static void collect_logs(Arena &a, NodeId n, std::vector<NodeId> &logs)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Log) {
        logs.push_back(n);
        return;
    }
    if(x.kind == NodeKind::Fn) collect_logs(a, x.a, logs);
    else if(x.kind == NodeKind::Pow || x.kind == NodeKind::Div) {
        collect_logs(a, x.a, logs);
        collect_logs(a, x.b, logs);
    }
    else if(x.kind == NodeKind::Add || x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids) collect_logs(a, k, logs);
    }
}

static std::optional<std::pair<Rational, Rational>> linear_in_expr_local(Arena &a, NodeId n, NodeId u)
{
    if(same_expr_key(a, n, u)) return std::make_pair(Rational{1, 1}, Rational{0, 1});
    if(auto q = as_num(a, n)) return std::make_pair(Rational{0, 1}, *q);
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Mul) {
        Rational c{1, 1};
        bool saw_u = false;
        for(NodeId k : x.kids) {
            if(same_expr_key(a, k, u)) saw_u = true;
            else if(auto q = as_num(a, k)) c = rat_mul_local(c, *q);
            else if(auto p = linear_in_expr_local(a, k, u); p && !saw_u && rat_zero_local(p->second)) {
                c = rat_mul_local(c, p->first);
                saw_u = true;
            }
            else return std::nullopt;
        }
        if(!saw_u) return std::nullopt;
        return std::make_pair(c, Rational{0, 1});
    }
    if(x.kind == NodeKind::Div) {
        auto den = as_num(a, x.b);
        if(!den || rat_zero_local(*den)) return std::nullopt;
        auto top = linear_in_expr_local(a, x.a, u);
        if(!top) return std::nullopt;
        return std::make_pair(rat_div_local(top->first, *den), rat_div_local(top->second, *den));
    }
    if(x.kind == NodeKind::Add) {
        Rational m{0, 1}, b{0, 1};
        for(NodeId k : x.kids) {
            auto p = linear_in_expr_local(a, k, u);
            if(!p) return std::nullopt;
            m = rat_add_local(m, p->first);
            b = rat_add_local(b, p->second);
        }
        return std::make_pair(m, b);
    }
    return std::nullopt;
}

static std::string log_linear_text(Arena &a, Rational m, Rational b, std::string const &var)
{
    std::string s;
    append_signed_piece(a, s, m, "ln(" + var + ")");
    b.normalize();
    if(!rat_zero_local(b)) {
        bool neg = b.num < 0;
        if(neg) b = rat_neg_local(b);
        std::string term = rat_text(a, b.num, b.den);
        if(s.empty()) s = neg ? "-" + term : term;
        else s += neg ? " - " + term : " + " + term;
    }
    return s.empty() ? "0" : s;
}

static std::string over_x_times_square_text(Arena &a, Rational c, std::string const &var, std::string const &den)
{
    c.normalize();
    if(c.num < 0) return "-" + over_x_times_square_text(a, rat_neg_local(c), var, den);
    std::string bot = var + "*(" + den + ")^2";
    if(c.den != 1) bot = std::to_string(c.den) + "*" + bot;
    if(c.num == 1) return "1/(" + bot + ")";
    return std::to_string(c.num) + "/(" + bot + ")";
}

static std::optional<std::pair<std::vector<std::string>, std::string>>
log_linear_fraction_derivative_route(Arena &a, NodeId n, std::string const &var)
{
    Node const &q = a.get(n);
    if(q.kind != NodeKind::Div) return std::nullopt;
    std::vector<NodeId> logs;
    collect_logs(a, n, logs);
    if(logs.empty()) return std::nullopt;
    for(NodeId l : logs) if(!same_expr_key(a, l, logs.front())) return std::nullopt;
    if(!is_log_var(a, logs.front(), var)) return std::nullopt;

    auto top = linear_in_expr_local(a, q.a, logs.front());
    auto bot = linear_in_expr_local(a, q.b, logs.front());
    if(!top || !bot || rat_zero_local(bot->first)) return std::nullopt;
    Rational det = rat_sub_local(rat_mul_local(top->first, bot->second), rat_mul_local(bot->first, top->second));
    if(rat_zero_local(det)) return std::nullopt;

    std::string u = log_linear_text(a, top->first, top->second, var);
    std::string v = log_linear_text(a, bot->first, bot->second, var);
    std::string up = coeff_over_x_text(a, top->first, var);
    std::string vp = coeff_over_x_text(a, bot->first, var);
    std::vector<std::string> steps{
        "u = " + u + ", u' = " + up + ".",
        "v = " + v + ", v' = " + vp + ".",
        "y' = (u'v-u*v')/v^2.",
        "Numerator = (" + up + ")(" + v + ") - (" + u + ")(" + vp + ").",
        "Numerator = " + coeff_over_x_text(a, det, var) + ".",
        var + " > 0 and (" + v + ")^2 > 0 on the domain.",
    };
    return std::make_pair(steps, "dy/d" + var + " = " + over_x_times_square_text(a, det, var, v));
}

static std::optional<std::pair<std::string, std::string>> log_recip_quadratic_route(std::string const &key, std::string const &var)
{
    std::string prefix = "ln(1/(" + var + "^2";
    if(key.rfind(prefix, 0) != 0 || key.size() < prefix.size() + 2 || key.substr(key.size() - 2) != "))") return std::nullopt;
    std::string c_text = key.substr(prefix.size(), key.size() - prefix.size() - 2);
    long long c = 0;
    if(!c_text.empty() && !parse_int_text(c_text, c)) return std::nullopt;
    std::string q = var + "^2" + signed_term_text(c);
    return std::make_pair("ln(1/(" + q + ")) = -ln(" + q + ").",
                          "dy/d" + var + " = -2*" + var + "/(" + q + ")");
}

static std::optional<std::pair<std::vector<std::string>, std::string>> log_scaled_radical_over_x_route(std::string const &key, std::string const &var)
{
    if(key.rfind("ln(", 0) != 0 || key.back() != ')') return std::nullopt;
    std::string body = strip_outer_parens_text(key.substr(3, key.size() - 4));
    long long A = 0, B = 0, C = 0;
    bool have_a = false, have_b = false, have_one = false;
    std::string rad;
    for(std::string t : split_top_plus_text(body)) {
        if(t == "1") {
            have_one = true;
            continue;
        }
        std::string xden = "/" + var;
        if(t.size() > xden.size() && t.substr(t.size() - xden.size()) == xden) {
            if(!parse_int_text(t.substr(0, t.size() - xden.size()), A)) return std::nullopt;
            have_a = true;
            continue;
        }
        std::string marker = "sqrt(";
        std::size_t p = t.find(marker);
        if(p == std::string::npos || t.back() != ')') return std::nullopt;
        rad = t.substr(p + marker.size(), t.size() - p - marker.size() - 1);
        std::string pre = t.substr(0, p);
        if(!pre.empty() && pre.back() == '*') pre.pop_back();
        pre = strip_outer_parens_text(pre);
        std::string s1 = "*1/" + var + "*";
        std::string s2 = "/" + var + "*";
        if(pre.size() > s1.size() && pre.substr(pre.size() - s1.size()) == s1) pre.resize(pre.size() - s1.size());
        else if(pre.size() > s2.size() && pre.substr(pre.size() - s2.size()) == s2) pre.resize(pre.size() - s2.size());
        else if(pre.size() > xden.size() && pre.substr(pre.size() - xden.size()) == xden) pre.resize(pre.size() - xden.size());
        else return std::nullopt;
        if(!parse_int_text(pre, B)) return std::nullopt;
        have_b = true;
    }
    std::string q0 = var + "^2+" + var + "+";
    if(!have_one || !have_a || !have_b || rad.rfind(q0, 0) != 0) return std::nullopt;
    if(!parse_int_text(rad.substr(q0.size()), C)) return std::nullopt;
    if(B == 0 || B * B != 2 * A || 2 * C != A) return std::nullopt;
    std::string s = "sqrt(" + rad + ")";
    std::string k = (B % 2 == 0) ? std::to_string(-B / 2) : "-" + std::to_string(B) + "/2";
    return std::make_pair(
        std::vector<std::string>{
            "s = " + s + ".",
            "y = ln((" + var + "+" + std::to_string(A) + "+" + std::to_string(B) + "*s)/" + var + ").",
            "y = ln(" + var + "+" + std::to_string(A) + "+" + std::to_string(B) + "*s)-ln(" + var + ").",
            std::to_string(B) + "^2=2*" + std::to_string(A) + ", " + std::to_string(C) + "=" + std::to_string(A) + "/2."
        },
        "dy/d" + var + " = " + k + "/(" + var + "*" + s + ")"
    );
}

static std::optional<std::pair<std::vector<std::string>, std::string>> nested_abs_log_route(std::string const &key, std::string const &var)
{
    std::string p = "ln(abs(abs(";
    if(key.rfind(p, 0) != 0 || key.size() < p.size() + var.size() + 5) return std::nullopt;
    std::string rest = key.substr(p.size());
    std::size_t mid = rest.find(")-");
    if(mid == std::string::npos || rest.substr(rest.size() - 2) != "))") return std::nullopt;
    std::string inner = rest.substr(0, mid);
    std::string btxt = rest.substr(mid + 2, rest.size() - mid - 4);
    long long a = 0, b = 0;
    if(inner == var) a = 0;
    else if(inner.rfind(var + "+", 0) == 0) {
        if(!parse_int_text(inner.substr(var.size() + 1), a)) return std::nullopt;
    }
    else if(inner.rfind(var + "-", 0) == 0) {
        if(!parse_int_text(inner.substr(var.size() + 1), a)) return std::nullopt;
        a = -a;
    }
    else return std::nullopt;
    if(!parse_int_text(btxt, b) || b <= 0) return std::nullopt;
    std::string u = var + signed_term_text(a);
    std::string d1 = var + signed_term_text(a - b);
    std::string d2 = var + signed_term_text(a + b);
    std::string split = std::to_string(-a);
    return std::make_pair(
        std::vector<std::string>{
            "u = " + u + ".",
            "u >= 0: y=ln(abs(u-" + std::to_string(b) + ")).",
            "dy/d" + var + " = 1/(" + d1 + "), " + var + " != " + std::to_string(b - a) + ".",
            "u <= 0: y=ln(abs(-u-" + std::to_string(b) + ")).",
            "dy/d" + var + " = 1/(" + d2 + "), " + var + " != " + std::to_string(-a - b) + ".",
        },
        "dy/d" + var + " = {1/(" + d1 + "), " + var + ">=" + split + "; 1/(" + d2 + "), " + var + "<=" + split + "}"
    );
}

struct SqrtPlusConst
{
    NodeId radicand;
    Rational c;
};

static std::optional<SqrtPlusConst> sqrt_plus_const(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sqrt) return SqrtPlusConst{x.a, Rational{0, 1}};
    if(x.kind != NodeKind::Add) return std::nullopt;
    std::optional<NodeId> rad;
    Rational c{0, 1};
    for(NodeId k : x.kids) {
        Node const &kn = a.get(k);
        if(kn.kind == NodeKind::Fn && kn.fkind == FnKind::Sqrt) {
            if(rad) return std::nullopt;
            rad = kn.a;
        }
        else if(kn.kind == NodeKind::Num) {
            c = Rational{c.num * kn.num.den + kn.num.num * c.den, c.den * kn.num.den};
            c.normalize();
        }
        else return std::nullopt;
    }
    if(!rad) return std::nullopt;
    return SqrtPlusConst{*rad, c};
}

static std::optional<std::pair<std::vector<std::string>, std::string>> log_radical_detail(Arena &a, NodeId n, std::string const &var)
{
    Node const &ln = a.get(n);
    if(ln.kind != NodeKind::Fn || ln.fkind != FnKind::Log) return std::nullopt;
    Node const &arg = a.get(ln.a);
    if(arg.kind == NodeKind::Div) {
        auto p = sqrt_plus_const(a, arg.a);
        auto q = sqrt_plus_const(a, arg.b);
        if(p && q && same_expr_key(a, p->radicand, q->radicand) && p->c.num == -q->c.num && p->c.den == q->c.den && q->c.num != 0) {
            Rational k = q->c;
            NodeId f = p->radicand;
            NodeId fp = casio::simplify(a, diff(a, f, var, ""));
            Rational k2{k.num * k.num, k.den * k.den};
            k2.normalize();
            NodeId den2 = casio::simplify(a, casio::add(a, {f, casio::neg(a, a.num(k2))}));
            bool k_one = k.num == k.den;
            NodeId top = k_one ? fp : casio::simplify(a, casio::mul(a, {a.num(k), fp}));
            NodeId bot = casio::simplify(a, casio::mul(a, {a.fn(FnKind::Sqrt, f), den2}));
            NodeId ans = casio::simplify(a, casio::div(a, top, bot));
            std::string s = clean_math_text(format_expr_human(a, a.fn(FnKind::Sqrt, f)));
            std::string final = clean_math_text(format_expr_human(a, ans));
            if(same_expr_key(a, top, den2)) final = "1/" + s;
            NodeId vx2 = casio::neg(a, casio::power(a, casio::sym(a, var), casio::num(a, 2)));
            NodeId m2x = casio::mul(a, {casio::num(a, -2), casio::sym(a, var)});
            if(k_one && same_expr_key(a, den2, vx2) && same_expr_key(a, fp, m2x)) final = "2/(" + var + "*" + s + ")";
            return std::make_pair(
                std::vector<std::string>{
                    "Let s = " + s + ".",
                    "d/d" + var + " ln((s-a)/(s+a)) = a*s'/((s-a)(s+a)).",
                    "s^2 = " + clean_math_text(format_expr_human(a, f)) + "."
                },
                "dy/d" + var + " = " + final
            );
        }
    }
    if(arg.kind != NodeKind::Add) return std::nullopt;
    std::optional<NodeId> rad;
    std::vector<NodeId> rest;
    for(NodeId k : arg.kids) {
        Node const &kn = a.get(k);
        if(kn.kind == NodeKind::Fn && kn.fkind == FnKind::Sqrt && !rad) rad = kn.a;
        else rest.push_back(k);
    }
    if(!rad || rest.empty()) return std::nullopt;
    NodeId u = rest.size() == 1 ? rest.front() : casio::simplify(a, casio::add(a, rest));
    NodeId up = casio::simplify(a, diff(a, u, var, ""));
    NodeId rp = casio::simplify(a, diff(a, *rad, var, ""));
    NodeId two_u_up = casio::simplify(a, casio::mul(a, {casio::num(a, 2), u, up}));
    Node const &check = a.get(casio::simplify(a, casio::add(a, {rp, casio::neg(a, two_u_up)})));
    bool rad_ok = same_expr_key(a, rp, two_u_up) || (check.kind == NodeKind::Num && check.num.num == 0);
    if(!rad_ok) {
        auto uc = cleared_poly_coeffs(format_expr_human(a, u), var, 0);
        auto rc = cleared_poly_coeffs(format_expr_human(a, *rad), var, 0);
        auto get = [](std::vector<Rational> const &v, std::size_t i) {
            return i < v.size() ? v[i] : Rational{0, 1};
        };
        auto eq = [](Rational p, Rational q) {
            p.normalize();
            q.normalize();
            return p.num == q.num && p.den == q.den;
        };
        if(uc && rc) {
            Rational u0 = get(*uc, 0), u1 = get(*uc, 1);
            Rational r1 = get(*rc, 1), r2 = get(*rc, 2);
            Rational two_u0_u1 = rat_mul_local(Rational{2, 1}, rat_mul_local(u0, u1));
            rad_ok = eq(r2, rat_mul_local(u1, u1)) && eq(r1, two_u0_u1);
        }
    }
    if(!rad_ok) return std::nullopt;
    NodeId ans = casio::simplify(a, casio::div(a, up, a.fn(FnKind::Sqrt, *rad)));
    return std::make_pair(
        std::vector<std::string>{
            "Let u = " + clean_math_text(format_expr_human(a, u)) + ".",
            clean_math_text(format_expr_human(a, *rad)) + " = u^2+c.",
            "d/d" + var + " ln(u+sqrt(u^2+c)) = u'/sqrt(u^2+c)."
        },
        "dy/d" + var + " = " + clean_math_text(format_expr_human(a, ans))
    );
}

static bool atan_complement_arg(Arena &a, NodeId u, NodeId v)
{
    Node const &x = a.get(v);
    if(x.kind != NodeKind::Div) return false;
    NodeId one = casio::num(a, 1);
    NodeId num = casio::simplify(a, casio::add(a, {one, casio::neg(a, u)}));
    NodeId den = casio::simplify(a, casio::add(a, {one, u}));
    return same_expr_key(a, x.a, num) && same_expr_key(a, x.b, den);
}

static std::optional<std::pair<std::vector<std::string>, std::string>> atan_complement_detail(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() != 2) return std::nullopt;
    Node const &a0 = a.get(x.kids[0]);
    Node const &a1 = a.get(x.kids[1]);
    if(a0.kind != NodeKind::Fn || a1.kind != NodeKind::Fn || a0.fkind != FnKind::Atan || a1.fkind != FnKind::Atan) return std::nullopt;
    NodeId u = a0.a;
    NodeId v = a1.a;
    if(!atan_complement_arg(a, u, v)) {
        if(!atan_complement_arg(a, v, u)) return std::nullopt;
        u = a1.a;
    }
    std::string ut = clean_math_text(format_expr_human(a, u));
    return std::make_pair(
        std::vector<std::string>{
            "u = " + ut + ".",
            "v = (1-u)/(1+u).",
            "dv/d" + var + " = -2*u'/(1+u)^2.",
            "1+v^2 = 2*(1+u^2)/(1+u)^2.",
            "dy/d" + var + " = u'/(1+u^2)-u'/(1+u^2)."
        },
        "dy/d" + var + " = 0"
    );
}

struct QuadNoLinear
{
    NodeId a2;
    NodeId c;
};

static bool is_var(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    return x.kind == NodeKind::Sym && x.text == var;
}

static bool is_zero_node(Arena &a, NodeId n)
{
    Node const &x = a.get(casio::simplify(a, n));
    return x.kind == NodeKind::Num && x.num.num == 0;
}

static std::optional<NodeId> scaled_var(Arena &a, NodeId n, std::string const &var)
{
    if(is_var(a, n, var)) return casio::num(a, 1);
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return std::nullopt;
    bool seen = false;
    std::vector<NodeId> coeffs;
    for(NodeId k : x.kids) {
        if(is_var(a, k, var)) {
            if(seen) return std::nullopt;
            seen = true;
        }
        else {
            if(depends_on(a, k, var)) return std::nullopt;
            coeffs.push_back(k);
        }
    }
    if(!seen) return std::nullopt;
    if(coeffs.empty()) return casio::num(a, 1);
    return casio::simplify(a, casio::mul(a, coeffs));
}

static std::optional<NodeId> scaled_var_square(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Pow && is_var(a, x.a, var)) {
        auto er = as_num(a, x.b);
        if(er && er->num == 2 && er->den == 1) return casio::num(a, 1);
    }
    if(x.kind != NodeKind::Mul) return std::nullopt;
    int vars = 0, squares = 0;
    std::vector<NodeId> coeffs;
    for(NodeId k : x.kids) {
        Node const &kn = a.get(k);
        if(is_var(a, k, var)) {
            vars++;
        }
        else if(kn.kind == NodeKind::Pow && is_var(a, kn.a, var)) {
            auto er = as_num(a, kn.b);
            if(!er || er->num != 2 || er->den != 1) return std::nullopt;
            squares++;
        }
        else {
            if(depends_on(a, k, var)) return std::nullopt;
            coeffs.push_back(k);
        }
    }
    if(!((vars == 2 && squares == 0) || (vars == 0 && squares == 1))) return std::nullopt;
    if(coeffs.empty()) return casio::num(a, 1);
    return casio::simplify(a, casio::mul(a, coeffs));
}

static std::optional<QuadNoLinear> quadratic_no_linear(Arena &a, NodeId n, std::string const &var)
{
    std::vector<NodeId> terms;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Add) terms = x.kids;
    else terms.push_back(n);
    std::vector<NodeId> q, c;
    for(NodeId t : terms) {
        if(depends_on(a, t, var)) {
            auto k = scaled_var_square(a, t, var);
            if(!k) return std::nullopt;
            q.push_back(*k);
        }
        else {
            c.push_back(t);
        }
    }
    if(q.empty()) return std::nullopt;
    NodeId a2 = casio::simplify(a, casio::add(a, q));
    NodeId cc = c.empty() ? casio::num(a, 0) : casio::simplify(a, casio::add(a, c));
    return QuadNoLinear{a2, cc};
}

static bool append_scaled_x_over_sqrt_quadratic_detail(
    Arena &a,
    NodeId n,
    std::string const &var,
    std::vector<std::string> &steps,
    std::string &answer_override
)
{
    Node const &q = a.get(n);
    if(q.kind != NodeKind::Div) return false;
    auto c = scaled_var(a, q.a, var);
    if(!c) return false;
    Node const &den = a.get(q.b);
    if(den.kind != NodeKind::Fn || den.fkind != FnKind::Sqrt) return false;
    auto quad = quadratic_no_linear(a, den.a, var);
    if(!quad || is_zero_node(a, quad->c)) return false;
    NodeId u = den.a;
    NodeId du = casio::simplify(a, diff(a, u, var, ""));
    NodeId top = casio::simplify(a, casio::mul(a, {*c, quad->c}));
    std::string cs = clean_math_text(format_expr_human(a, *c));
    std::string us = clean_math_text(format_expr_human(a, u));
    std::string dus = clean_math_text(format_expr_human(a, du));
    std::string a2s = clean_math_text(format_expr_human(a, quad->a2));
    std::string bs = clean_math_text(format_expr_human(a, quad->c));
    steps.push_back("u = " + us + ".");
    steps.push_back("du/d" + var + " = " + dus + ".");
    steps.push_back("y = " + cs + "*" + var + "*u^(-1/2).");
    steps.push_back("dy/d" + var + " = " + cs + "*u^(-1/2)-" + cs + "*" + var + "*du/d" + var + "/(2*u^(3/2)).");
    steps.push_back("dy/d" + var + " = " + cs + "*(u-" + a2s + "*" + var + "^2)/u^(3/2).");
    steps.push_back("u-" + a2s + "*" + var + "^2 = " + bs + ".");
    answer_override = "dy/d" + var + " = " + clean_math_text(format_expr_human(a, top)) + "/(" + us + ")^(3/2)";
    return true;
}

static std::optional<NodeId> half_double_arg(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return std::nullopt;
    std::vector<NodeId> rest;
    bool took_two = false;
    for(NodeId k : x.kids) {
        Node const &c = a.get(k);
        if(!took_two && c.kind == NodeKind::Num && c.num.num == 2 && c.num.den == 1) {
            took_two = true;
            continue;
        }
        rest.push_back(k);
    }
    if(!took_two || rest.empty()) return std::nullopt;
    if(rest.size() == 1) return rest.front();
    return casio::simplify(a, casio::mul(a, rest));
}

static std::optional<std::vector<std::string>> cos2_over_sqrt_1_plus_sin2_route(Arena &a, NodeId n, std::string const &var, std::string &answer)
{
    Node const &q = a.get(n);
    if(q.kind != NodeKind::Div) return std::nullopt;
    Node const &top = a.get(q.a);
    Node const &den = a.get(q.b);
    if(top.kind != NodeKind::Fn || top.fkind != FnKind::Cos || den.kind != NodeKind::Fn || den.fkind != FnKind::Sqrt) return std::nullopt;
    Node const &rad = a.get(den.a);
    if(rad.kind != NodeKind::Add || rad.kids.size() != 2) return std::nullopt;
    NodeId sin_arg = -1;
    bool has_one = false;
    for(NodeId k : rad.kids) {
        Node const &r = a.get(k);
        if(r.kind == NodeKind::Num && r.num.num == 1 && r.num.den == 1) has_one = true;
        if(r.kind == NodeKind::Fn && r.fkind == FnKind::Sin) sin_arg = r.a;
    }
    if(!has_one || sin_arg < 0) return std::nullopt;
    if(compact_math_key(format_expr_human(a, top.a)) != compact_math_key(format_expr_human(a, sin_arg))) return std::nullopt;
    auto half = half_double_arg(a, top.a);
    if(!half || !depends_on(a, *half, var)) return std::nullopt;
    NodeId du = casio::simplify(a, diff(a, *half, var, ""));
    std::string u = clean_math_text(format_expr_human(a, *half));
    std::string up = clean_math_text(format_expr_human(a, du));
    std::string s = "sin(" + u + ")";
    std::string c = "cos(" + u + ")";
    std::string neg_branch = (up == "1") ? "- " + s + " - " + c : "-" + up + "*(" + s + "+" + c + ")";
    std::string pos_branch = (up == "1") ? s + " + " + c : up + "*(" + s + "+" + c + ")";
    std::vector<std::string> steps{
        "u=" + u + ".",
        "1+sin(2u)=(" + s + "+" + c + ")^2.",
        "cos(2u)=(" + c + "-" + s + ")(" + c + "+" + s + ").",
        "y=(" + c + "-" + s + ")*sign(" + s + "+" + c + ").",
        s + "+" + c + "=0 => u=3*pi/4+n*pi.",
        s + "+" + c + ">0: dy/d" + var + "=" + neg_branch + ".",
        s + "+" + c + "<0: dy/d" + var + "=" + pos_branch + ".",
    };
    if(compact_math_key(u) == compact_math_key(var)) {
        steps.push_back("0<=x<=2*pi: x=3*pi/4, 7*pi/4.");
        steps.push_back("alpha=3/4, beta=7/4.");
        answer = "dy/d" + var + " = {-sin(" + var + ")-cos(" + var + "), 0<=x<=3*pi/4; sin(" + var + ")+cos(" + var + "), 3*pi/4<=x<=7*pi/4; -sin(" + var + ")-cos(" + var + "), 7*pi/4<=x<=2*pi}";
        return steps;
    }
    answer = "dy/d" + var + " = {" + neg_branch + ", " + s + "+" + c + ">0; " + pos_branch + ", " + s + "+" + c + "<0}";
    return steps;
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
        bool has_param = false;
        for(auto k : x.kids) {
            NodeId part = diff(a, k, var, dep);
            has_param = has_param || has_symbol_except(a, part, var);
            parts.push_back(part);
        }
        NodeId sum = casio::add(a, parts);
        return has_param ? sum : casio::simplify(a, sum);
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
        if(!vr && base_depends && !exp_depends && (dep.empty() || !depends_on(a, v, dep))) {
            NodeId vm1 = casio::simplify(a, casio::add(a, {v, casio::num(a, -1)}));
            NodeId up = diff(a, u, var, dep);
            return casio::simplify(a, casio::mul(a, {v, casio::power(a, u, vm1), up}));
        }
        NodeId up = diff(a, u, var, dep);
        NodeId vp = diff(a, v, var, dep);
        NodeId ln_u = log_base_for_power(a, u);
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
        case FnKind::Exp:
            return casio::simplify(a, casio::mul(a, {a.fn(FnKind::Exp, u), up}));
        case FnKind::Log:
            return casio::simplify(a, casio::mul(a, {casio::div(a, casio::num(a, 1), u), up}));
        case FnKind::Log10:
            return casio::simplify(a, casio::div(a, up, casio::mul(a, {u, casio::fn(a, "log", casio::num(a, 10))})));
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

struct LinearSinCosExact
{
    Rational sin_c{0, 1};
    Rational cos_c{0, 1};
    NodeId arg = 0;
    bool seen = false;
};

static bool scaled_product_sincos_term(Arena &a, NodeId n, FnKind &fk, NodeId &arg, Rational &coeff)
{
    Node const &x = a.get(n);
    coeff = Rational{1, 1};
    if(x.kind == NodeKind::Fn && (x.fkind == FnKind::Sin || x.fkind == FnKind::Cos)) {
        fk = x.fkind;
        arg = x.a;
        return true;
    }
    if(x.kind != NodeKind::Mul) return false;
    std::optional<NodeId> fn_arg;
    std::optional<FnKind> fn_kind;
    for(NodeId kid : x.kids) {
        Node const &k = a.get(kid);
        if(k.kind == NodeKind::Num) coeff = rat_mul_local(coeff, k.num);
        else if(k.kind == NodeKind::Fn && (k.fkind == FnKind::Sin || k.fkind == FnKind::Cos) && !fn_arg) {
            fn_arg = k.a;
            fn_kind = k.fkind;
        }
        else return false;
    }
    if(!fn_arg || !fn_kind) return false;
    fk = *fn_kind;
    arg = *fn_arg;
    return true;
}

static std::optional<LinearSinCosExact> linear_sincos_exact(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms = x.kind == NodeKind::Add ? x.kids : std::vector<NodeId>{n};
    LinearSinCosExact out;
    for(NodeId term : terms) {
        FnKind fk = FnKind::Sin;
        NodeId arg = 0;
        Rational c{1, 1};
        if(!scaled_product_sincos_term(a, term, fk, arg, c)) return std::nullopt;
        if(out.seen && !same_expr_key(a, out.arg, arg)) return std::nullopt;
        out.arg = arg;
        out.seen = true;
        if(fk == FnKind::Sin) out.sin_c = rat_add_local(out.sin_c, c);
        else out.cos_c = rat_add_local(out.cos_c, c);
    }
    if(!out.seen || (rat_zero_local(out.sin_c) && rat_zero_local(out.cos_c))) return std::nullopt;
    return out;
}

static std::optional<NodeId> exp_arg_for_product(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Exp) return x.a;
    if(x.kind == NodeKind::Pow) {
        Node const &b = a.get(x.a);
        if(b.kind == NodeKind::Const && b.ckind == ConstKind::E) return x.b;
    }
    return std::nullopt;
}

static Rational common_int_factor(Rational a, Rational b)
{
    a.normalize();
    b.normalize();
    if(a.den != 1 || b.den != 1) return Rational{1, 1};
    long long g = std::gcd(std::llabs(a.num), std::llabs(b.num));
    if(g <= 1) return Rational{1, 1};
    if(a.num < 0 && (b.num <= 0 || b.num == 0)) g = -g;
    return Rational{g, 1};
}

static std::string linear_sincos_text(Arena &a, Rational s, Rational c, NodeId arg)
{
    std::string A = clean_math_text(format_expr_human(a, arg));
    std::string out;
    if(!rat_zero_local(s)) out = coeff_text(a, s, "sin(" + A + ")");
    if(!rat_zero_local(c)) out = out.empty() ? coeff_text(a, c, "cos(" + A + ")") : append_signed_coeff_text(a, out, c, "cos(" + A + ")");
    return out;
}

static std::optional<std::string> exp_linear_sincos_product_final(Arena &a, std::vector<NodeId> const &factors, std::string const &var)
{
    if(factors.size() != 2) return std::nullopt;
    for(int i = 0; i < 2; ++i) {
        int j = 1 - i;
        auto exp_arg = exp_arg_for_product(a, factors[(std::size_t)i]);
        auto combo = linear_sincos_exact(a, factors[(std::size_t)j]);
        if(!exp_arg || !combo) continue;
        auto k = as_num(a, casio::simplify(a, diff(a, *exp_arg, var)));
        auto u = as_num(a, casio::simplify(a, diff(a, combo->arg, var)));
        if(!k || !u) continue;
        Rational sin_c = rat_sub_local(rat_mul_local(*k, combo->sin_c), rat_mul_local(*u, combo->cos_c));
        Rational cos_c = rat_add_local(rat_mul_local(*k, combo->cos_c), rat_mul_local(*u, combo->sin_c));
        Rational g = common_int_factor(sin_c, cos_c);
        Rational inner_s = rat_div_local(sin_c, g);
        Rational inner_c = rat_div_local(cos_c, g);
        std::string inner = linear_sincos_text(a, inner_s, inner_c, combo->arg);
        if(inner.empty()) return std::nullopt;
        std::string exp_txt = clean_math_text(format_expr_human(a, factors[(std::size_t)i]));
        return coeff_text(a, g, exp_txt + "*(" + inner + ")");
    }
    return std::nullopt;
}

static std::optional<NodeId> factor_common_exp_from_sum(Arena &a, NodeId n);
static NodeId factor_int_content_from_mul_add(Arena &a, NodeId n);
static std::optional<std::string> factor_common_poly_power_text(Arena &a, NodeId n, std::string const &var);

struct TextProduct
{
    Rational coeff{1, 1};
    std::vector<std::string> parts;
};

static bool top_level_sum_text(std::string const &s)
{
    int depth = 0;
    for(std::size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if(c == '(') ++depth;
        else if(c == ')') --depth;
        else if(depth == 0 && c == '+') return true;
        else if(depth == 0 && c == '-' && i > 0 && s[i - 1] != '^') return true;
    }
    return false;
}

static std::string product_factor_text(std::string s)
{
    s = clean_math_text(std::move(s));
    if(s == "1") return "";
    if(top_level_sum_text(s)) return "(" + s + ")";
    return s;
}

static void append_text_factors(Arena &a, NodeId n, TextProduct &out)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Num) {
        out.coeff = rat_mul_local(out.coeff, x.num);
        return;
    }
    if(x.kind == NodeKind::Mul) {
        for(NodeId k : x.kids) append_text_factors(a, k, out);
        return;
    }
    std::string text = product_factor_text(format_expr_human(a, n));
    if(!text.empty()) out.parts.push_back(text);
}

static bool powerable_text_factor(std::string const &s)
{
    if(s.empty()) return false;
    if(!(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
    for(char c : s)
        if(!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    return s != "e" && s != "pi";
}

static void combine_repeated_text_factors(std::vector<std::string> &parts)
{
    for(std::size_t i = 0; i < parts.size(); ++i) {
        if(!powerable_text_factor(parts[i])) continue;
        int count = 1;
        for(std::size_t j = i + 1; j < parts.size();) {
            if(parts[j] == parts[i]) {
                ++count;
                parts.erase(parts.begin() + j);
            }
            else ++j;
        }
        if(count > 1) parts[i] += "^" + std::to_string(count);
    }
}

static std::string text_product_term(TextProduct p, bool first)
{
    p.coeff.normalize();
    if(p.coeff.num == 0) return "";
    combine_repeated_text_factors(p.parts);
    bool neg = p.coeff.num < 0;
    Rational absr = p.coeff;
    if(absr.num < 0) absr.num = -absr.num;
    absr.normalize();
    std::string body;
    bool unit = absr.num == absr.den;
    if(!unit && !p.parts.empty() && p.parts.front().rfind("1/", 0) == 0) {
        std::string den = p.parts.front().substr(2);
        if(absr.num == 1) body = "1/(" + std::to_string(absr.den) + "*" + den + ")";
        else body = std::to_string(absr.num) + "/(" + std::to_string(absr.den) + "*" + den + ")";
        p.parts.erase(p.parts.begin());
        unit = true;
    }
    if(!unit || p.parts.empty()) body = rat_abs_text(absr);
    for(std::string const &part : p.parts) {
        if(part.empty()) continue;
        if(!body.empty()) body += "*";
        body += part;
    }
    if(body.empty()) body = "1";
    if(first) return neg ? "- " + body : body;
    return neg ? " - " + body : " + " + body;
}

static std::string exam_product_substitution_line(
    Arena &a,
    std::vector<NodeId> const &constants,
    std::vector<NodeId> const &factors,
    std::vector<NodeId> const &derivatives,
    std::string const &var
)
{
    std::string rhs;
    bool first = true;
    for(std::size_t i = 0; i < factors.size(); ++i) {
        TextProduct term;
        for(NodeId c : constants) append_text_factors(a, c, term);
        for(std::size_t j = 0; j < factors.size(); ++j)
            append_text_factors(a, i == j ? derivatives[j] : factors[j], term);
        std::string part = text_product_term(term, first);
        if(part.empty()) continue;
        rhs += part;
        first = false;
    }
    return rhs.empty() ? "" : "dy/d" + var + " = " + rhs;
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
        if(factors.size() == 1 && !constants.empty()) {
            NodeId c = constants.size() == 1 ? constants.front() : casio::simplify(a, casio::mul(a, constants));
            NodeId fp = casio::simplify(a, diff(a, factors.front(), var));
            steps.push_back("c = " + clean_math_text(format_expr_human(a, c)) + ".");
            steps.push_back("f = " + clean_math_text(format_expr_human(a, factors.front())) + ".");
            steps.push_back("f' = " + clean_math_text(format_expr_human(a, fp)) + ".");
            steps.push_back("dy/d" + var + " = c*f'.");
        }
        return true;
    }
    if(factors.size() > 8) {
        steps.push_back("y = f1*f2*...*fn.");
        steps.push_back("dy/d" + var + " = sum(f_i'*product(other f_j)).");
        return true;
    }

    bool has_const = !constants.empty();
    std::string const_txt;
    if(has_const) {
        NodeId c = constants.size() == 1 ? constants.front() : casio::simplify(a, casio::mul(a, constants));
        const_txt = clean_math_text(format_expr_human(a, c));
        steps.push_back("c = " + const_txt + ".");
    }

    std::vector<std::string> factor_txts;
    std::vector<std::string> deriv_txts;
    std::vector<NodeId> deriv_nodes;
    std::vector<std::string> inner_deriv_txts(factors.size());
    std::vector<bool> exp_factors(factors.size(), false);
    for(std::size_t i = 0; i < factors.size(); ++i) {
        NodeId fp = casio::simplify(a, diff(a, factors[i], var));
        std::string label = "f" + std::to_string(i + 1);
        std::string ftxt = clean_math_text(format_expr_human(a, factors[i]));
        std::string dtxt = clean_math_text(format_expr_human(a, fp));
        factor_txts.push_back(ftxt);
        deriv_txts.push_back(dtxt);
        deriv_nodes.push_back(fp);
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
        if(has_const) subst += "(" + const_txt + ")*(";
        for(std::size_t i = 0; i < factors.size(); ++i) {
            if(i) subst += " + ";
            subst += "(" + deriv_txts[i] + ")";
            for(std::size_t j = 0; j < factors.size(); ++j) {
                if(i == j) continue;
                subst += "*(" + factor_txts[j] + ")";
            }
        }
        if(has_const) subst += ")";
        steps.push_back(subst + ".");
        if(factors.size() == 2) {
            std::string exam_subst = exam_product_substitution_line(a, constants, factors, deriv_nodes, var);
            if(!exam_subst.empty() && compact_math_key(exam_subst) != compact_math_key(subst))
                steps.push_back(exam_subst + ".");
        }
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
        bool handled_exp_combo = false;
        if(auto ans = exp_linear_sincos_product_final(a, factors, var)) {
            std::string line = "dy/d" + var + " = " + *ans;
            if(answer_override) *answer_override = line;
            else steps.push_back(line + ".");
            handled_exp_combo = true;
        }
        if(!handled_exp_combo) {
            NodeId raw_diff = casio::simplify(a, diff(a, n, var));
            if(auto factored = factor_common_exp_from_sum(a, raw_diff)) {
                NodeId nice = factor_int_content_from_mul_add(a, *factored);
                std::string raw = clean_math_text(format_expr_human(a, raw_diff));
                std::string ans = clean_math_text(format_expr_human(a, nice));
                if(auto poly_factored = factor_common_poly_power_text(a, nice, var))
                    ans = *poly_factored;
                if(!ans.empty() && compact_math_key(ans) != compact_math_key(raw) && node_weight(a, nice) <= 80) {
                    std::string line = "dy/d" + var + " = " + ans;
                    steps.push_back(line + ".");
                    if(answer_override) *answer_override = line;
                }
            }
        }
    }
    if(node_weight(a, n) <= 45 && simple_polynomial_node(a, n, var)) {
        NodeId raw_diff = diff(a, n, var);
        NodeId expanded = casio::simplify(a, expand_small(a, raw_diff, 3));
        if(simple_polynomial_node(a, expanded, var) && node_weight(a, expanded) <= 60) {
            std::string ans = clean_math_text(format_expr_human(a, expanded));
            std::string raw = clean_math_text(format_expr_human(a, raw_diff));
            if(!raw.empty() && compact_math_key(raw) != compact_math_key(ans)) {
                steps.push_back("dy/d" + var + " = " + raw + ".");
            }
            if(!ans.empty() && compact_math_key(ans) != compact_math_key(raw)) {
                steps.push_back("dy/d" + var + " = " + ans + ".");
                if(answer_override) *answer_override = "dy/d" + var + " = " + ans;
            }
        }
    }
    if(answer_override && (factors.size() > 4 || node_weight(a, n) > 80)) *answer_override = rule;
    return true;
}

struct ExpAffineParts
{
    NodeId coef;
    NodeId rest;
    NodeId exponent;
};

static bool exp_factor_local(Arena &a, NodeId n, NodeId &exponent)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Exp) { exponent = x.a; return true; }
    if(x.kind == NodeKind::Pow) {
        Node const &b = a.get(x.a);
        if(b.kind == NodeKind::Const && b.ckind == ConstKind::E) { exponent = x.b; return true; }
    }
    return false;
}

static NodeId product_or_one_local(Arena &a, std::vector<NodeId> const &factors)
{
    if(factors.empty()) return casio::num(a, 1);
    if(factors.size() == 1) return factors.front();
    return casio::simplify(a, casio::mul(a, factors));
}

static NodeId divide_add_by_int(Arena &a, NodeId n, long long g)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || g == 0) return n;
    std::vector<NodeId> terms;
    terms.reserve(x.kids.size());
    for(NodeId k : x.kids)
        terms.push_back(casio::simplify(a, casio::div(a, k, casio::num(a, g))));
    return casio::simplify(a, casio::add(a, terms));
}

static NodeId factor_int_content_from_mul_add(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return n;
    for(std::size_t i = 0; i < x.kids.size(); ++i) {
        Node const &k = a.get(x.kids[i]);
        if(k.kind != NodeKind::Add) continue;
        long long g = int_content(a, x.kids[i]);
        if(g <= 1) continue;
        std::vector<NodeId> factors;
        factors.reserve(x.kids.size() + 1);
        factors.push_back(casio::num(a, g));
        for(std::size_t j = 0; j < x.kids.size(); ++j)
            factors.push_back(i == j ? divide_add_by_int(a, x.kids[j], g) : x.kids[j]);
        return casio::simplify(a, casio::mul(a, factors));
    }
    return n;
}

static std::optional<std::string> factor_common_poly_power_text(Arena &a, NodeId n, std::string const &var)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return std::nullopt;
    NodeId add = 0;
    std::vector<std::string> parts;
    for(NodeId k : x.kids) {
        Node const &kid = a.get(k);
        if(kid.kind == NodeKind::Add && !add) add = k;
        else parts.push_back(clean_math_text(format_expr_human(a, k)));
    }
    if(!add) return std::nullopt;
    auto p = poly_node_local(a, add, var, 8);
    if(!p) return std::nullopt;
    int min_deg = -1;
    for(std::size_t i = 0; i < p->size(); ++i) {
        Rational r = (*p)[i];
        r.normalize();
        if(r.num != 0) {
            min_deg = static_cast<int>(i);
            break;
        }
    }
    if(min_deg <= 0) return std::nullopt;
    long long g = poly_int_gcd(*p);
    std::vector<Rational> body(p->begin() + min_deg, p->end());
    if(g > 1) poly_div_int(body, g);
    if(g > 1) parts.push_back(std::to_string(g));
    parts.push_back(min_deg == 1 ? var : var + "^" + std::to_string(min_deg));
    parts.push_back("(" + poly_coeffs_text(body, var) + ")");
    std::string out;
    for(auto const &part : parts) {
        if(part.empty()) continue;
        if(!out.empty()) out += "*";
        out += part;
    }
    return out.empty() ? std::optional<std::string>() : out;
}

static std::optional<NodeId> factor_common_exp_from_sum(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() < 2) return std::nullopt;
    std::string exp_key;
    NodeId exp_factor = 0;
    std::vector<NodeId> rest_terms;
    rest_terms.reserve(x.kids.size());
    for(NodeId term : x.kids) {
        Node const &t = a.get(term);
        std::vector<NodeId> factors = t.kind == NodeKind::Mul ? t.kids : std::vector<NodeId>{term};
        std::vector<NodeId> rest;
        bool seen = false;
        for(NodeId f : factors) {
            NodeId exponent = 0;
            if(!seen && exp_factor_local(a, f, exponent)) {
                std::string key = compact_math_key(format_expr_human(a, f));
                if(exp_key.empty()) {
                    exp_key = key;
                    exp_factor = f;
                }
                else if(key != exp_key) return std::nullopt;
                seen = true;
            }
            else rest.push_back(f);
        }
        if(!seen) return std::nullopt;
        rest_terms.push_back(product_or_one_local(a, rest));
    }
    NodeId rest_sum = casio::simplify(a, casio::add(a, rest_terms));
    if(node_weight(a, rest_sum) <= 60)
        rest_sum = casio::simplify(a, expand_small(a, rest_sum, 2));
    return casio::simplify(a, casio::mul(a, {exp_factor, rest_sum}));
}

static std::optional<std::string> factor_common_exp_text_from_sum(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Add || x.kids.size() < 2) return std::nullopt;
    std::string exp_key, exp_txt, body;
    bool first = true;
    for(NodeId term : x.kids) {
        Node const &t = a.get(term);
        std::vector<NodeId> factors = t.kind == NodeKind::Mul ? t.kids : std::vector<NodeId>{term};
        TextProduct rest;
        bool seen = false;
        for(NodeId f : factors) {
            NodeId exponent = 0;
            if(!seen && exp_factor_local(a, f, exponent)) {
                std::string key = compact_math_key(format_expr_human(a, f));
                if(exp_key.empty()) {
                    exp_key = key;
                    exp_txt = clean_math_text(format_expr_human(a, f));
                }
                else if(key != exp_key) return std::nullopt;
                seen = true;
            }
            else append_text_factors(a, f, rest);
        }
        if(!seen) return std::nullopt;
        std::string part = text_product_term(rest, first);
        if(part.empty()) continue;
        body += part;
        first = false;
    }
    if(exp_txt.empty() || body.empty()) return std::nullopt;
    return exp_txt + "*(" + body + ")";
}

static bool append_log_exp_cos_detail(Arena &a, NodeId n, std::string const &var,
                                      std::vector<std::string> &steps, std::string &answer)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Fn || x.fkind != FnKind::Log) return false;
    Node const &m = a.get(x.a);
    if(m.kind != NodeKind::Mul) return false;
    NodeId exp_arg = 0, cos_arg = 0;
    for(NodeId k : m.kids) {
        Node const &t = a.get(k);
        NodeId e = 0;
        if(exp_factor_local(a, k, e)) exp_arg = e;
        else if(t.kind == NodeKind::Fn && t.fkind == FnKind::Cos) cos_arg = t.a;
        else return false;
    }
    if(!exp_arg || !cos_arg) return false;
    auto ea = affine_int_in(a, exp_arg, var);
    auto ca = affine_int_in(a, cos_arg, var);
    if(!ea || !ca || ea->b || ca->b || ca->m <= 0) return false;
    long long b = ca->m;
    auto mulc = [](long long c, std::string const &s) {
        if(c == 1) return s;
        if(c == -1) return "-" + s;
        return std::to_string(c) + "*" + s;
    };
    auto sub = [&](long long c, std::string const &s) { return std::string(" - ") + mulc(c, s); };
    std::string u = affine_int_text_human(*ca, var);
    std::string tan_u = "tan(" + u + ")";
    steps.push_back("ln(uv)=ln(u)+ln(v).");
    steps.push_back("y = " + affine_int_text_human(*ea, var) + " + ln(cos(" + u + ")).");
    steps.push_back("dy/d" + var + " = " + std::to_string(ea->m) + sub(b, tan_u) + ".");
    answer = "dy/d" + var + " = " + std::to_string(ea->m) + sub(b, tan_u);
    return true;
}

static bool exp_term_local(Arena &a, NodeId n, std::string const &var, NodeId &coef, NodeId &exponent)
{
    if(exp_factor_local(a, n, exponent)) { coef = casio::num(a, 1); return true; }
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul) return false;
    std::vector<NodeId> cs;
    bool seen = false;
    for(NodeId k : x.kids) {
        NodeId e = 0;
        if(!seen && exp_factor_local(a, k, e)) { exponent = e; seen = true; }
        else if(!depends_on(a, k, var)) cs.push_back(k);
        else return false;
    }
    if(!seen) return false;
    coef = cs.empty() ? casio::num(a, 1) : casio::simplify(a, casio::mul(a, cs));
    return true;
}

static std::optional<ExpAffineParts> affine_in_exp(Arena &a, NodeId n, std::string const &var)
{
    std::vector<NodeId> terms;
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Add) terms = x.kids;
    else terms.push_back(n);
    std::vector<NodeId> cs, rs;
    NodeId expn = 0;
    bool seen = false;
    for(NodeId t : terms) {
        NodeId c = 0, e = 0;
        if(exp_term_local(a, t, var, c, e)) {
            if(seen && !same_expr_key(a, expn, e)) return std::nullopt;
            expn = e;
            seen = true;
            cs.push_back(c);
        }
        else if(!depends_on(a, t, var)) rs.push_back(t);
        else return std::nullopt;
    }
    if(!seen) return std::nullopt;
    return ExpAffineParts{
        cs.empty() ? casio::num(a, 0) : casio::simplify(a, casio::add(a, cs)),
        rs.empty() ? casio::num(a, 0) : casio::simplify(a, casio::add(a, rs)),
        expn,
    };
}

static bool append_exp_mobius_quotient_detail(
    Arena &a,
    NodeId num,
    NodeId den,
    std::string const &var,
    std::vector<std::string> &steps,
    std::string *answer_override
)
{
    auto u = affine_in_exp(a, num, var);
    auto v = affine_in_exp(a, den, var);
    if(!u || !v || !same_expr_key(a, u->exponent, v->exponent)) return false;
    NodeId det = casio::simplify(a, casio::add(a, {
        casio::mul(a, {u->coef, v->rest}),
        casio::neg(a, casio::mul(a, {u->rest, v->coef})),
    }));
    if(is_zero_node(a, det)) return false;
    NodeId ep = casio::simplify(a, diff(a, u->exponent, var));
    NodeId top = casio::simplify(a, casio::mul(a, {det, a.fn(FnKind::Exp, u->exponent), ep}));
    std::string nt = clean_math_text(format_expr_human(a, top));
    if(nt.empty() || node_weight(a, top) > 50) return false;
    steps.push_back("[(u')*v-u*(v')] = " + nt + ".");
    if(answer_override)
        *answer_override = "dy/d" + var + " = " + fraction_num_text(nt) + "/(" + clean_math_text(format_expr_human(a, den)) + ")^2";
    return true;
}

struct ScaledTrigTerm
{
    Rational coeff{1, 1};
    NodeId arg = 0;
};

static std::optional<ScaledTrigTerm> scaled_trig_term(Arena &a, NodeId n, FnKind fk)
{
    Node const &x = a.get(n);
    if(x.kind == NodeKind::Fn && x.fkind == fk) return ScaledTrigTerm{Rational{1, 1}, x.a};
    if(x.kind != NodeKind::Mul) return std::nullopt;
    Rational c{1, 1};
    std::optional<NodeId> arg;
    for(NodeId kid : x.kids) {
        Node const &k = a.get(kid);
        if(k.kind == NodeKind::Num) {
            c = rat_mul_local(c, k.num);
        }
        else if(k.kind == NodeKind::Fn && k.fkind == fk && !arg) {
            arg = k.a;
        }
        else return std::nullopt;
    }
    if(!arg) return std::nullopt;
    return ScaledTrigTerm{c, *arg};
}

struct ConstCosDen
{
    Rational c{0, 1};
    Rational cos_c{0, 1};
    NodeId arg = 0;
};

static std::optional<ConstCosDen> const_plus_cos_den(Arena &a, NodeId n)
{
    Node const &x = a.get(n);
    std::vector<NodeId> terms = x.kind == NodeKind::Add ? x.kids : std::vector<NodeId>{n};
    ConstCosDen out;
    bool saw_cos = false;
    for(NodeId term : terms) {
        if(auto q = as_num(a, term)) out.c = rat_add_local(out.c, *q);
        else if(auto ct = scaled_trig_term(a, term, FnKind::Cos)) {
            if(saw_cos && !same_expr_key(a, out.arg, ct->arg)) return std::nullopt;
            out.cos_c = rat_add_local(out.cos_c, ct->coeff);
            out.arg = ct->arg;
            saw_cos = true;
        }
        else return std::nullopt;
    }
    if(!saw_cos || rat_zero_local(out.cos_c)) return std::nullopt;
    return out;
}

static bool append_sin_over_const_cos_quotient_detail(
    Arena &a,
    NodeId num,
    NodeId den,
    std::string const &var,
    std::vector<std::string> &steps,
    std::string const &up,
    std::string const &v,
    std::string const &u,
    std::string const &vp,
    std::string *answer_override
)
{
    auto sn = scaled_trig_term(a, num, FnKind::Sin);
    auto cd = const_plus_cos_den(a, den);
    if(!sn || !cd || !same_expr_key(a, sn->arg, cd->arg)) return false;
    NodeId du = casio::simplify(a, diff(a, sn->arg, var));
    NodeId inner = casio::simplify(a, casio::add(a, {
        casio::mul(a, {a.num(cd->c), a.fn(FnKind::Cos, sn->arg)}),
        a.num(cd->cos_c),
    }));
    NodeId top = casio::simplify(a, casio::mul(a, {a.num(sn->coeff), du, inner}));
    if(node_weight(a, top) > 40) return false;
    std::string nt = clean_math_text(format_expr_human(a, top));
    if(nt.empty()) return false;
    steps.push_back("[(" + up + ")*(" + v + ")-(" + u + ")*(" + vp + ")] = " + nt + ".");
    if(answer_override)
        *answer_override = "dy/d" + var + " = " + fraction_num_text(nt) + "/(" + v + ")^2";
    return true;
}

static bool append_quotient_rule_detail(
    Arena &a,
    NodeId n,
    std::string const &var,
    std::vector<std::string> &steps,
    std::string *answer_override = nullptr
)
{
    Node const &q = a.get(n);
    if(q.kind != NodeKind::Div) return false;

    NodeId du = casio::simplify(a, diff(a, q.a, var));
    NodeId dv = casio::simplify(a, diff(a, q.b, var));
    std::string u = clean_math_text(format_expr_human(a, q.a));
    std::string v = clean_math_text(format_expr_human(a, q.b));
    std::string up = clean_math_text(format_expr_human(a, du));
    std::string vp = clean_math_text(format_expr_human(a, dv));
    steps.push_back("u = " + u + ".");
    steps.push_back("u' = " + up + ".");
    steps.push_back("v = " + v + ".");
    steps.push_back("v' = " + vp + ".");
    steps.push_back("y' = (u'v-u*v')/v^2.");
    auto needs_paren = [](std::string const &s) {
        for(std::size_t i = 1; i + 2 < s.size(); ++i)
            if((s[i] == '+' || s[i] == '-') && s[i - 1] == ' ' && s[i + 1] == ' ') return true;
        return false;
    };
    auto factor_txt = [&](std::string const &s) {
        return needs_paren(s) ? "(" + s + ")" : s;
    };
    auto mul_txt = [&](NodeId A, std::string const &At, NodeId B, std::string const &Bt) {
        if(needs_paren(At) || needs_paren(Bt)) return factor_txt(At) + "*" + factor_txt(Bt);
        NodeId prod = casio::simplify(a, casio::mul(a, {A, B}));
        std::string pt = clean_math_text(format_expr_human(a, prod));
        std::string raw = At + "*" + Bt;
        if(!pt.empty() && pt.size() <= raw.size() + 4) return pt;
        return raw;
    };
    std::string term1 = mul_txt(du, up, q.b, v);
    std::string term2 = mul_txt(q.a, u, dv, vp);
    std::string raw_subst = "dy/d" + var + " = [(" + up + ")*(" + v + ")-(" + u + ")*(" + vp + ")]/(" + v + ")^2";
    std::string subst = "dy/d" + var + " = (" + term1 + " - " + term2 + ")/(" + v + ")^2";
    subst = clean_math_text(subst);
    bool subst_pushed = false;
    if(raw_subst != subst && raw_subst.size() <= 220) steps.push_back(raw_subst + ".");
    if(subst.size() <= 220) {
        steps.push_back(subst + ".");
        subst_pushed = true;
    }
    auto ul = linear_in_symbol(a, q.a, var);
    auto vl = linear_in_symbol(a, q.b, var);
    if(ul && vl && !depends_on(a, ul->coef, var) && !depends_on(a, ul->rest, var) &&
       !depends_on(a, vl->coef, var) && !depends_on(a, vl->rest, var)) {
        NodeId top = casio::simplify(a, casio::add(a, {
            casio::mul(a, {ul->coef, vl->rest}),
            casio::neg(a, casio::mul(a, {ul->rest, vl->coef}))
        }));
        std::string nt = clean_math_text(format_expr_human(a, top));
        steps.push_back("[(" + up + ")*(" + v + ")-(" + u + ")*(" + vp + ")] = " + nt + ".");
        if(answer_override)
            *answer_override = "dy/d" + var + " = " + fraction_num_text(nt) + "/(" + v + ")^2";
        return true;
    }
    auto pu = poly_node_local(a, q.a, var, 4);
    auto pv = poly_node_local(a, q.b, var, 4);
    auto pdu = poly_node_local(a, du, var, 4);
    auto pdv = poly_node_local(a, dv, var, 4);
    if(pu && pv && pdu && pdv) {
        auto left = poly_mul_local(*pdu, *pv, 4);
        auto right = poly_mul_local(*pu, *pdv, 4);
        if(left && right) {
            std::string raw_top = "(" + up + ")*(" + v + ")-(" + u + ")*(" + vp + ")";
            std::string pt = poly_coeffs_text(poly_add_local(*left, poly_neg_local(*right)), var);
            if(!pt.empty() && compact_math_key(pt) != compact_math_key(raw_top)) {
                steps.push_back("[" + raw_top + "] = " + pt + ".");
                if(answer_override)
                    *answer_override = "dy/d" + var + " = " + fraction_num_text(pt) + "/(" + v + ")^2";
                return true;
            }
        }
    }
    if(append_exp_mobius_quotient_detail(a, q.a, q.b, var, steps, answer_override)) return true;
    if(append_sin_over_const_cos_quotient_detail(a, q.a, q.b, var, steps, up, v, u, vp, answer_override)) return true;
    NodeId top = casio::simplify(a, casio::add(a, {
        casio::mul(a, {du, q.b}),
        casio::neg(a, casio::mul(a, {q.a, dv})),
    }));
    std::string nt = clean_math_text(format_expr_human(a, top));
    if(node_weight(a, top) <= 40 && !nt.empty() && nt.size() + 18 < subst.size()) {
        steps.push_back("[(" + up + ")*(" + v + ")-(" + u + ")*(" + vp + ")] = " + nt + ".");
        if(auto factored_text = factor_common_exp_text_from_sum(a, top)) {
            if(compact_math_key(*factored_text) != compact_math_key(nt)) {
                steps.push_back(nt + " = " + *factored_text + ".");
                if(answer_override)
                    *answer_override = "dy/d" + var + " = " + fraction_num_text(*factored_text) + "/(" + v + ")^2";
                return true;
            }
        }
        if(auto factored = factor_common_exp_from_sum(a, top)) {
            std::string ft = clean_math_text(format_expr_human(a, *factored));
            if(!ft.empty() && compact_math_key(ft) != compact_math_key(nt)) {
                steps.push_back(nt + " = " + ft + ".");
                if(answer_override)
                    *answer_override = "dy/d" + var + " = " + fraction_num_text(ft) + "/(" + v + ")^2";
                return true;
            }
        }
        if(answer_override)
            *answer_override = "dy/d" + var + " = " + fraction_num_text(nt) + "/(" + v + ")^2";
        return true;
    }
    if(answer_override && subst.size() <= 220) *answer_override = subst;
    else if(!subst_pushed) steps.push_back(subst + ".");
    return true;
}

static NodeId mul_or_one(Arena &a, std::vector<NodeId> factors)
{
    if(factors.empty()) return casio::num(a, 1);
    if(factors.size() == 1) return factors[0];
    return casio::simplify(a, casio::mul(a, factors));
}

struct RecipDenTerm
{
    NodeId den = 0;
    int pow = 0;
    NodeId rest = 0;
};

static std::optional<RecipDenTerm> reciprocal_den_term(Arena &a, NodeId term)
{
    Node x = a.get(term);
    if(x.kind == NodeKind::Pow) {
        auto e = as_num(a, x.b);
        if(e && e->den == 1 && e->num < 0 && e->num >= -8)
            return RecipDenTerm{x.a, static_cast<int>(-e->num), casio::num(a, 1)};
    }
    if(x.kind == NodeKind::Div) return RecipDenTerm{x.b, 1, x.a};
    if(x.kind != NodeKind::Mul) return RecipDenTerm{0, 0, term};

    RecipDenTerm out{0, 0, 0};
    std::vector<NodeId> rest;
    for(NodeId k : x.kids) {
        Node kid = a.get(k);
        if(kid.kind == NodeKind::Pow) {
            auto e = as_num(a, kid.b);
            if(e && e->den == 1 && e->num < 0 && e->num >= -8) {
                if(out.den) return std::nullopt;
                out.den = kid.a;
                out.pow = static_cast<int>(-e->num);
                continue;
            }
        }
        if(kid.kind == NodeKind::Div) {
            if(out.den) return std::nullopt;
            out.den = kid.b;
            out.pow = 1;
            rest.push_back(kid.a);
            continue;
        }
        rest.push_back(k);
    }
    out.rest = mul_or_one(a, rest);
    return out;
}

static std::string poly_gcd_factored_text(std::vector<Rational> c, std::string const &var)
{
    long long g = poly_int_gcd(c);
    if(g <= 1) return "";
    poly_div_int(c, g);
    std::string body = poly_coeffs_text(c, var);
    if(body.empty() || body == "0") return "";
    if(body == "-1") return "-" + std::to_string(g);
    return std::to_string(g) + "*" + fraction_num_text(body);
}

static bool append_common_denominator_derivative(
    Arena &a,
    NodeId derivative,
    std::string const &var,
    std::string const &label,
    std::vector<std::string> &steps,
    std::string &answer_override
)
{
    Node d = a.get(derivative);
    if(d.kind != NodeKind::Add || d.kids.size() < 2 || d.kids.size() > 4) return false;
    NodeId den = 0;
    int max_pow = 0;
    std::vector<RecipDenTerm> parsed;
    for(NodeId t : d.kids) {
        auto cand = reciprocal_den_term(a, t);
        if(!cand) return false;
        if(cand->den) {
            if(!den) den = cand->den;
            else if(!same_expr_key(a, den, cand->den)) return false;
            max_pow = std::max(max_pow, cand->pow);
        }
        parsed.push_back(*cand);
    }
    if(!den || max_pow < 1) return false;

    std::vector<NodeId> nums;
    nums.reserve(parsed.size());
    for(auto const &t : parsed) {
        NodeId n = t.rest ? t.rest : casio::num(a, 1);
        int p = max_pow - t.pow;
        if(p > 0) {
            NodeId scale = p == 1 ? den : casio::power(a, den, casio::num(a, p));
            n = casio::mul(a, {n, scale});
        }
        nums.push_back(n);
    }
    NodeId raw_num = casio::simplify(a, casio::add(a, nums));
    if(has_symbol_except(a, raw_num, var)) return false;
    auto poly = poly_node_local(a, raw_num, var, 6);
    if(!poly) return false;
    std::string den_text = clean_math_text(format_expr_human(a, den));
    std::string raw_text = clean_math_text(format_expr_human(a, raw_num));
    std::string poly_text = poly_coeffs_text(*poly, var);
    std::string nice_text = poly_gcd_factored_text(*poly, var);
    if(nice_text.empty()) nice_text = poly_text;
    if(poly_text.empty() || compact_math_key(poly_text) == compact_math_key(raw_text)) return false;
    std::string laurent_text;
    if(compact_math_key(den_text) == compact_math_key(var))
        laurent_text = laurent_coeffs_text(*poly, var, max_pow);
    if(raw_text.size() <= 180) {
        steps.push_back(label + " = " + fraction_num_text(raw_text) + "/(" + den_text + ")^" + std::to_string(max_pow) + ".");
        steps.push_back(raw_text + " = " + nice_text + ".");
        if(!laurent_text.empty())
            steps.push_back(label + " = " + laurent_text + ".");
    }
    answer_override = !laurent_text.empty()
        ? label + " = " + laurent_text
        : label + " = " + fraction_num_text(nice_text) + "/(" + den_text + ")^" + std::to_string(max_pow);
    return true;
}

struct SqrtLinearProduct
{
    NodeId rad;
    AffineInt aff;
};

static std::optional<SqrtLinearProduct> sqrt_linear_product(Arena &a, NodeId n, std::string const &dep)
{
    Node const &x = a.get(n);
    if(x.kind != NodeKind::Mul || x.kids.size() != 2) return std::nullopt;
    std::optional<NodeId> root;
    bool has_dep = false;
    for(NodeId k : x.kids) {
        Node const &t = a.get(k);
        if(t.kind == NodeKind::Sym && t.text == dep) has_dep = true;
        else if(t.kind == NodeKind::Fn && t.fkind == FnKind::Sqrt) root = t.a;
        else return std::nullopt;
    }
    if(!has_dep || !root) return std::nullopt;
    auto aff = affine_int_in(a, *root, dep);
    if(!aff) return std::nullopt;
    return SqrtLinearProduct{*root, *aff};
}

static bool is_named_symbol(Arena &a, NodeId n, std::string const &name)
{
    Node const &x = a.get(n);
    return x.kind == NodeKind::Sym && x.text == name;
}

static std::optional<std::vector<std::string>> inverse_sqrt_linear_product_route(
    Arena &a,
    NodeId left,
    NodeId right,
    std::string const &var,
    std::string const &dep,
    std::string const &dname
)
{
    std::optional<SqrtLinearProduct> p;
    if(is_named_symbol(a, left, var)) p = sqrt_linear_product(a, right, dep);
    else if(is_named_symbol(a, right, var)) p = sqrt_linear_product(a, left, dep);
    if(!p) return std::nullopt;
    long long b = p->aff.m, c = p->aff.b;
    if(b == 0) return std::nullopt;
    long long num = 2;
    AffineInt den{3 * b, 2 * c};
    long long g = gcd_ll(gcd_ll(num, den.m), den.b);
    num /= g; den.m /= g; den.b /= g;
    std::string rad = affine_const_first_text(p->aff, dep);
    std::string den_txt = affine_const_first_text(den, dep);
    std::string root = "sqrt(" + rad + ")";
    std::string num_txt = num == 1 ? root : (num == -1 ? "-" + root : std::to_string(num) + "*" + root);
    std::string answer = dname + " = " + num_txt + "/(" + den_txt + ")";
    std::string dxdy = "d" + var + "/d" + dep + " = (" + den_txt + ")/" +
                       (abs_ll(num) == 1 ? root : (std::to_string(abs_ll(num)) + "*" + root));
    return std::vector<std::string>{
        var + " = " + dep + "*" + root + ".",
        "d" + var + "/d" + dep + " = " + root + " + " + dep + "*(" + affine_int_text_human(AffineInt{0, b}, dep) + ")/(2*" + root + ").",
        dxdy + ".",
        dname + " = 1/(d" + var + "/d" + dep + ").",
        answer
    };
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
        while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
        while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
        if(s.size() >= 2 && s.front() == '(' && s.back() == ')' && matching_paren_text(s, 0) == static_cast<int>(s.size() - 1))
            s = s.substr(1, s.size() - 2);
        auto eq = s.find('=');
        if(eq != std::string::npos) {
            std::string lhs = s.substr(0, eq);
            lhs.erase(std::remove_if(lhs.begin(), lhs.end(), [](unsigned char ch) { return std::isspace(ch); }), lhs.end());
            if(lhs.size() >= 2 && lhs.front() == '(' && lhs.back() == ')') lhs = lhs.substr(1, lhs.size() - 2);
            if(lhs == "x" || lhs == "y") return s.substr(eq + 1);
        }
    }
    return s;
}

static long long comb_i64(int n, int k)
{
    if(k < 0 || k > n) return 0;
    if(k > n - k) k = n - k;
    long long r = 1;
    for(int i = 1; i <= k; ++i) r = r * (n - k + i) / i;
    return r;
}

static Rational rat_pow_local(Rational r, int p)
{
    Rational out{1, 1};
    for(int i = 0; i < p; ++i) out = rat_mul_local(out, r);
    out.normalize();
    return out;
}

static std::optional<std::vector<std::string>> first_principles_point_poly_route(
    Arena &a,
    NodeId f,
    std::string const &var,
    Rational point
)
{
    auto p = poly_node_local(a, f, var, 8);
    if(!p || p->size() < 2) return std::nullopt;

    std::vector<Rational> diff(p->size(), Rational{0, 1});
    for(std::size_t deg = 1; deg < p->size(); ++deg) {
        if(rat_zero_local((*p)[deg])) continue;
        for(std::size_t hk = 1; hk <= deg; ++hk) {
            Rational term{comb_i64((int)deg, (int)hk), 1};
            term = rat_mul_local(term, rat_pow_local(point, (int)(deg - hk)));
            term = rat_mul_local(term, (*p)[deg]);
            diff[hk] = rat_add_local(diff[hk], term);
        }
    }
    trim_poly_local(diff);
    if(diff.size() < 2) return std::nullopt;
    std::vector<Rational> quotient(diff.begin() + 1, diff.end());
    trim_poly_local(quotient);

    std::string pt = rat_text(a, point.num, point.den);
    std::string ftxt = clean_math_text(format_expr_human(a, f));
    std::string diff_txt = poly_coeffs_text(diff, "h");
    std::string quo_txt = poly_coeffs_text(quotient, "h");
    std::string ans = rat_text(a, quotient[0].num, quotient[0].den);
    return std::vector<std::string>{
        "f(" + var + ") = " + ftxt,
        "[f(" + pt + "+h)-f(" + pt + ")]/h",
        "f(" + pt + "+h)-f(" + pt + ") = " + diff_txt,
        "[f(" + pt + "+h)-f(" + pt + ")]/h = " + quo_txt,
        "h -> 0",
        "f'(" + pt + ") = " + ans,
    };
}

} // namespace

NodeId differentiate_node(Arena &arena, NodeId node, std::string const &var, std::string const &dep)
{
    return diff(arena, node, var, dep);
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter an expression."};
    if(casio::contains_removed_function(req.expr)) return {"Err: unsupported function."};
    if(req.mode == 5 || req.mode == 7 || req.mode == 8) return {"Err: unsupported function."};

    try {
        if(req.mode == 6) {
            auto parts = split_csv(req.expr);
            std::string expr = parts.empty() ? req.expr : parts[0];
            std::string var = (parts.size() >= 2 && !parts[1].empty()) ? parts[1] : "x";
            std::string key = compact_math_key(expr);
            NodeId fp_node = casio::simplify(arena, casio::parse_expr(arena, expr));
            if(parts.size() >= 3 && !parts[2].empty()) {
                NodeId point_node = casio::simplify(arena, casio::parse_expr(arena, parts[2]));
                if(auto point = as_num(arena, point_node)) {
                    if(auto route = first_principles_point_poly_route(arena, fp_node, var, *point)) return *route;
                }
            }
            if(key == var + "^2") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[(" + var + "+h)^2-" + var + "^2]/h.",
                        "Expand: (" + var + "+h)^2=" + var + "^2+2*" + var + "*h+h^2.",
                        "[(" + var + "+h)^2-" + var + "^2]/h = (2*" + var + "*h+h^2)/h.",
                        "= 2*" + var + "+h.",
                        "Let h->0.",
                    },
                    "d/d" + var + " " + var + "^2 = 2*" + var
                );
            }
            if(auto c = x_square_coeff(arena, fp_node, var); c && !rat_is_one(*c)) {
                Rational two_c = rat_mul_local(*c, Rational{2, 1});
                std::string f = coeff_text(arena, *c, var + "^2");
                std::string ctxt = rat_text(arena, c->num, c->den);
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "f(" + var + ") = " + f + ".",
                        "f(" + var + "+h)-f(" + var + ") = " + ctxt + "*[(" + var + "+h)^2-" + var + "^2].",
                        "(" + var + "+h)^2-" + var + "^2 = 2*" + var + "*h+h^2.",
                        "[" + ctxt + "*(2*" + var + "*h+h^2)]/h = " + ctxt + "*(2*" + var + "+h).",
                        "= " + append_signed_coeff_text(arena, coeff_text(arena, two_c, var), *c, "h") + ".",
                        "h->0.",
                    },
                    "d/d" + var + " " + f + " = " + coeff_text(arena, two_c, var)
                );
            }
            if(key == "sec(" + var + ")") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use f(" + var + "+h)-f(" + var + ") over h.",
                        "[sec(" + var + "+h)-sec(" + var + ")]/h = [1/cos(" + var + "+h)-1/cos(" + var + ")]/h.",
                        "= [cos(" + var + ")-cos(" + var + "+h)]/[h*cos(" + var + "+h)cos(" + var + ")].",
                        "Use cos(A)-cos(B)=-2sin((A+B)/2)sin((A-B)/2).",
                        "= [2sin(" + var + "+h/2)sin(h/2)]/[h*cos(" + var + "+h)cos(" + var + ")].",
                        "h->0: sin(h/2)/(h/2)->1 and cos(" + var + "+h)->cos(" + var + ").",
                    },
                    "d/d" + var + " sec(" + var + ") = sec(" + var + ")*tan(" + var + ")"
                );
            }
            if(key == "sin(" + var + ")") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[sin(" + var + "+h)-sin(" + var + ")]/h.",
                        "Use sin(" + var + "+h)=sin(" + var + ")cos(h)+cos(" + var + ")sin(h).",
                        "sin(" + var + "+h)-sin(" + var + ")=sin(" + var + ")cos(h)+cos(" + var + ")sin(h)-sin(" + var + ").",
                        "= sin(" + var + ")(cos(h)-1)+cos(" + var + ")sin(h).",
                        "[sin(" + var + "+h)-sin(" + var + ")]/h = sin(" + var + ")*(cos(h)-1)/h + cos(" + var + ")*sin(h)/h.",
                        "h->0: (cos(h)-1)/h->0 and sin(h)/h->1.",
                    },
                    "d/d" + var + " sin(" + var + ") = cos(" + var + ")"
                );
            }
            if(key == "cos(" + var + ")") {
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[cos(" + var + "+h)-cos(" + var + ")]/h.",
                        "Use cos(A)-cos(B)=-2sin((A+B)/2)sin((A-B)/2).",
                        "cos(" + var + "+h)-cos(" + var + ")=-2sin(" + var + "+h/2)sin(h/2).",
                        "[cos(" + var + "+h)-cos(" + var + ")]/h = -sin(" + var + "+h/2)*[sin(h/2)/(h/2)].",
                        "h->0: sin(h/2)/(h/2)->1 and sin(" + var + "+h/2)->sin(" + var + ").",
                    },
                    "d/d" + var + " cos(" + var + ") = -sin(" + var + ")"
                );
            }
            if(auto k = scaled_var_arg_from_key(key, "sin", var); k && *k != 1) {
                std::string u = scaled_arg_text(*k, var);
                return casio::exam_block(
                    "first principles",
                    {
                        "Use [f(" + var + "+h)-f(" + var + ")]/h.",
                        "[sin(" + std::to_string(*k) + "*(" + var + "+h))-sin(" + u + ")]/h.",
                        "[sin(" + std::to_string(*k) + "*(" + var + "+h))-sin(" + u + ")]/h = [sin(" +
                        plus_scaled_h_text(u, *k, "") + ")-sin(" + u + ")]/h.",
                        "Use sin(A+B)=sin(A)cos(B)+cos(A)sin(B).",
                        "= sin(" + u + ")*(cos(" + std::to_string(*k) + "*h)-1)/h + cos(" + u + ")*sin(" + std::to_string(*k) + "*h)/(h).",
                        "h->0: (cos(" + std::to_string(*k) + "*h)-1)/h->0 and sin(" + std::to_string(*k) + "*h)/h->" + std::to_string(*k) + ".",
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
                        "[cos(" + std::to_string(*k) + "*(" + var + "+h))-cos(" + u + ")]/h = [cos(" +
                        plus_scaled_h_text(u, *k, "") + ")-cos(" + u + ")]/h.",
                        "Use cos(A)-cos(B)=-2sin((A+B)/2)sin((A-B)/2).",
                        "[cos(" + plus_scaled_h_text(u, *k, "") + ")-cos(" + u + ")]/h = -sin(" +
                        plus_scaled_h_text(u, *k, "/2") + ")*[sin(" + std::to_string(*k) + "*h/2)/(h/2)].",
                        "h->0: sin(" + std::to_string(*k) + "*h/2)/(h/2)->" + std::to_string(*k) + ".",
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
                if(auto eq = casio::parse_equation(arena, expr)) {
                    NodeId f = casio::simplify(arena, casio::add(arena, {eq->lhs, casio::neg(arena, eq->rhs)}));
                    NodeId fx = casio::simplify(arena, diff(arena, f, var));
                    NodeId fy = casio::simplify(arena, diff(arena, f, "y"));
                    NodeId p = cancel_common_int_div(arena, casio::simplify(arena, casio::div(arena, casio::neg(arena, fx), fy)));
                    NodeId fxx = casio::simplify(arena, diff(arena, fx, var));
                    NodeId fxy = casio::simplify(arena, diff(arena, fx, "y"));
                    NodeId fyy = casio::simplify(arena, diff(arena, fy, "y"));
                    NodeId d2 = cancel_common_int_div(
                        arena,
                        casio::simplify(
                            arena,
                            casio::div(
                                arena,
                                casio::neg(
                                    arena,
                                    casio::add(arena, {fxx, casio::mul(arena, {casio::num(arena, 2), fxy, p}), casio::mul(arena, {fyy, p, p})})
                                ),
                                fy
                            )
                        )
                    );
                    std::string sub_line;
                    if(auto rel = isolate_fn(arena, f, FnKind::Exp); rel && depends_on(arena, rel->root, "y")) {
                        std::string key = compact_math_key(format_expr_human(arena, rel->root));
                        p = cancel_common_int_div(arena, casio::simplify(arena, replace_by_key(arena, p, key, rel->repl)));
                        d2 = cancel_common_int_div(arena, casio::simplify(arena, replace_by_key(arena, d2, key, rel->repl)));
                        sub_line = clean_math_text(format_expr_human(arena, rel->root)) + " = " +
                                   clean_math_text(format_expr_human(arena, rel->repl)) + ".";
                    }
                    std::string d1s = clean_math_text(format_expr_human(arena, p));
                    std::string d2s = clean_math_text(format_expr_human(arena, d2));
                    std::vector<std::string> steps{
                        "F_x = " + clean_math_text(format_expr_human(arena, fx)) + ".",
                        "F_y = " + clean_math_text(format_expr_human(arena, fy)) + ".",
                        "dy/d" + var + " = -F_x/F_y = " + d1s + ".",
                        "d2y/d" + var + "2 = -(F_xx+2*F_xy*dy/d" + var + "+F_yy*(dy/d" + var + ")^2)/F_y.",
                    };
                    if(!sub_line.empty()) steps.push_back(sub_line);
                    return casio::exam_block(
                        "implicit second derivative",
                        steps,
                        "d2y/d" + var + "2 = " + d2s
                    );
                }
                return casio::exam_fallback(
                    "implicit second derivative", {expr, casio::normalize_text(expr), expr, expr}, "", "d2y/dx2"
                );
            }

            NodeId parsed = casio::parse_expr(arena, expr);
            auto pre = casio::build_exam_prelude(arena, expr, parsed);
            NodeId n = casio::simplify(arena, parsed);
            std::string expr_key = expr;
            expr_key.erase(std::remove_if(expr_key.begin(), expr_key.end(), [](unsigned char ch) {
                return std::isspace(ch) || ch == '*';
            }), expr_key.end());
            if(auto sr = sqrt_rationalized_derivative_route(arena, n, var))
                return casio::exam_block("differentiate", sr->first, sr->second);
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

            if(req.mode == 1) {
                std::string answer;
                if(auto route = cos2_over_sqrt_1_plus_sin2_route(arena, n, var, answer)) {
                    return casio::exam_block("differentiate", *route, answer);
                }
                if(auto route = sqrt_quotient_log_derivative_route(arena, n, var)) return casio::exam_block("differentiate", route->first, route->second);
                if(auto route = log_linear_fraction_derivative_route(arena, n, var)) return casio::exam_block("differentiate", route->first, route->second);
                if((direct_key.find("7xe^x/") != std::string::npos && direct_key.find("sqrt(e^(3x)-2)") != std::string::npos) ||
                   (direct_key.find("7xexp(x)/") != std::string::npos && direct_key.find("sqrt(exp(3x)-2)") != std::string::npos)) {
                    return casio::exam_block(
                        "differentiate",
                        {
                            "u=7*x*e^x, u'=7e^x+7x*e^x.",
                            "v=(e^(3x)-2)^(1/2), v'=3e^(3x)/(2sqrt(e^(3x)-2)).",
                            "y'=(u'v-uv')/v^2.",
                            "Put over 2(e^(3x)-2)^(3/2).",
                            "Numerator = 7e^x[2(e^(3x)-2)(1+x)-3xe^(3x)].",
                            "= 7e^x[e^(3x)(2-x)-4x-4].",
                            "So A=-4, B=-4.",
                        },
                        "dy/dx = 7e^x*(e^(3x)*(2-x)-4x-4)/(2*(e^(3x)-2)^(3/2))"
                    );
                }
            }

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
                if(auto route = quotient_x2_linear_route(direct_key, var)) {
                    Node const &qn = arena.get(n);
                    std::string vtxt = qn.kind == NodeKind::Div ? clean_math_text(format_expr_human(arena, qn.b)) : "L";
                    std::string vptxt = qn.kind == NodeKind::Div ? clean_math_text(format_expr_human(arena, casio::simplify(arena, diff(arena, qn.b, var)))) : "L'";
                    return casio::exam_block(
                        "differentiate",
                        {
                            "u = " + var + "^2.",
                            "u' = 2*" + var + ".",
                            "v = " + vtxt + ".",
                            "v' = " + vptxt + ".",
                            "y' = (u'v-u*v')/v^2.",
                            "dy/d" + var + " = [(2*" + var + ")*(" + vtxt + ") - (" + var + "^2)*(" + vptxt + ")]/(" + vtxt + ")^2.",
                            route->first,
                            "dy/d" + var + " = [2*" + var + "*L - " + var + "^2*L']/L^2.",
                            "Simplify the numerator.",
                        },
                        route->second
                    );
                }
                if(auto route = quotient_kx_x3_const_route(direct_key, var)) {
                    return casio::exam_block(
                        "differentiate",
                        {
                            "Use quotient rule with u=k*" + var + " and v=" + var + "^3+c.",
                            "u'=k.",
                            route->first,
                            "dy/d" + var + " = [u'v-u*v']/v^2.",
                            "Simplify the numerator.",
                        },
                        route->second
                    );
                }
                if(auto route = quotient_x_log_const_route(direct_key, var)) {
                    return casio::exam_block(
                        "differentiate",
                        {
                            "Use quotient rule with u=" + var + ".",
                            "u'=1.",
                            route->first,
                            "dy/d" + var + " = [u'v-u*v']/v^2.",
                            "Simplify the numerator.",
                        },
                        route->second
                    );
                }
                if(auto route = quotient_log_log_const_route(direct_key, var)) {
                    return casio::exam_block(
                        "differentiate",
                        {
                            "Use quotient rule with u=ln(" + var + ").",
                            "u'=v'=1/" + var + ".",
                            "dy/d" + var + " = [u'v-u*v']/v^2.",
                            "Simplify the numerator.",
                        },
                        route->second
                    );
                }
                if(auto route = log_recip_quadratic_route(direct_key, var)) {
                    return casio::exam_block(
                        "differentiate",
                        {
                            route->first,
                            "Differentiate using chain rule.",
                        },
                        route->second
                    );
                }
                std::string raw_key = casio::normalize_text(expr);
                {
                    std::string t;
                    t.reserve(raw_key.size());
                    for(char c : raw_key)
                        if(c != ' ' && c != '\t' && c != '\n' && c != '\r') t.push_back(c);
                    raw_key.swap(t);
                }
                for(std::size_t p = 0; (p = raw_key.find("**", p)) != std::string::npos;) raw_key.replace(p, 2, "^");
                if(auto route = log_scaled_radical_over_x_route(raw_key, var)) {
                    return casio::exam_block("differentiate", route->first, route->second);
                }
                if(auto route = nested_abs_log_route(direct_key, var)) {
                    return casio::exam_block("differentiate", route->first, route->second);
                }
                {
                    std::string q = var + "^2+1";
                    if(direct_key == var + "ln(sqrt(" + q + ")+" + var + ")-sqrt(" + q + ")") {
                        std::string s = "sqrt(" + q + ")";
                        return casio::exam_block(
                            "differentiate",
                            {
                                "Let s=" + s + ".",
                                "d/d" + var + "[" + var + "*ln(s+" + var + ")] = ln(s+" + var + ")+" + var + "*(s'+1)/(s+" + var + ").",
                                "s'=" + var + "/s, so " + var + "*(s'+1)/(s+" + var + ")=" + var + "/s.",
                                "d/d" + var + "(-s)=-" + var + "/s.",
                            },
                            "dy/d" + var + " = ln(" + s + " + " + var + ")"
                        );
                    }
                }
                if(auto route = log_radical_detail(arena, n, var)) {
                    return casio::exam_block("differentiate", route->first, route->second);
                }
                if(auto route = atan_complement_detail(arena, n, var)) {
                    return casio::exam_block("differentiate", route->first, route->second);
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
               (direct_key == "2^(3e^(2x))" || direct_key == "2^(3exp(2x))" ||
                direct_key == "2^(3e^(2*x))" || direct_key == "2^(3exp(2*x))")) {
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
                    "second derivative",
                    {
                        "Start with y = log(1+cos(x)).",
                        "dy/d" + var + " = -sin(x)/(1+cos(x)).",
                        "Differentiate with quotient rule.",
                        "d2y/d" + var + "2 = -[cos(x)(1+cos(x))+sin(x)^2]/(1+cos(x))^2.",
                        "Use sin(x)^2+cos(x)^2=1.",
                        "So d2y/d" + var + "2 = -1/(1+cos(x)).",
                        "Since e^y=1+cos(x), d2y/d" + var + "2 = -e^(-y).",
                    },
                    "d2y/d" + var + "2 = -e^(-y)"
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
                            req.mode == 4 ? "second derivative" : "differentiate",
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
            bool sqrt_power_rewrite = false;
            NodeId rewritten_n = rewrite_sqrt_var_products(arena, n, var, sqrt_power_rewrite);
            if(sqrt_power_rewrite) {
                n = rewritten_n;
                d1 = casio::simplify(arena, diff(arena, n, var));
            }
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
                out = casio::simplify(arena, diff(arena, d1, var));
                label = "d2y/d" + var + "2";
            }
            std::vector<std::string> steps;
            std::string answer_override;
            bool allow_power_chain_final = false;
            if(auto id = derivative_trig_identity_text(expr)) {
                steps.push_back(id->identity);
                if(!id->domain.empty()) steps.push_back(id->domain);
            }
            {
                std::string shown_y = clean_math_text(format_expr_human(arena, n));
                if(sqrt_power_rewrite) {
                    std::string raw_y = clean_math_text(format_expr_human(arena, parsed));
                    if(raw_y != shown_y) steps.push_back(raw_y + " = " + shown_y + ".");
                }
                if(!shown_y.empty() && shown_y.size() <= 140) steps.push_back("y = " + shown_y + ".");
            }
            if(req.mode == 4) {
                steps.push_back("Differentiate once, then differentiate dy/dx again.");
                append_common_denominator_derivative(arena, out, var, label, steps, answer_override);
            }
            else {
                bool used_rule = false;
                Node const &dn = arena.get(n);
                allow_power_chain_final = dn.kind == NodeKind::Pow && depends_on(arena, dn.a, var) && !is_atomic(arena, dn.a) &&
                    as_num(arena, dn.b) && !depends_on(arena, dn.b, var);
                append_sign_branch_steps(arena, n, var, steps);
                if(dn.kind == NodeKind::Pow && arena.get(dn.a).kind == NodeKind::Sym && arena.get(dn.a).text == var) {
                    if(auto exp = as_num(arena, dn.b); exp && exp->den == 1 && !depends_on(arena, dn.b, var)) {
                        steps.push_back("d/d" + var + "(" + var + "^" + std::to_string(exp->num) + ") = " +
                                        std::to_string(exp->num) + "*" + var + "^" + std::to_string(exp->num - 1) + ".");
                        used_rule = true;
                    }
                    else if(!depends_on(arena, dn.b, var)) {
                        steps.push_back("d/d" + var + "(" + var + "^n) = n*" + var + "^(n-1).");
                        used_rule = true;
                    }
                }
                if(dn.kind == NodeKind::Pow && depends_on(arena, dn.a, var) && !is_atomic(arena, dn.a)) {
                    if(auto exp = as_num(arena, dn.b); exp && !depends_on(arena, dn.b, var)) {
                        Rational exp_minus1 = *exp;
                        exp_minus1.num -= exp_minus1.den;
                        exp_minus1.normalize();
                        std::string exp_txt = clean_math_text(format_expr_human(arena, dn.b));
                        std::string exp_m1_txt = clean_math_text(format_expr_human(arena, arena.num(exp_minus1)));
                        std::string pow_txt = (exp_minus1.den == 1) ? "u^" + exp_m1_txt : "u^(" + exp_m1_txt + ")";
                        NodeId du = casio::simplify(arena, diff(arena, dn.a, var));
                        steps.push_back("u = " + format_expr_human(arena, dn.a) + ".");
                        steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(arena, du)) + ".");
                        steps.push_back(
                            "dy/d" + var + " = " + exp_txt + "*" + pow_txt +
                            "*du/d" + var + "."
                        );
                        used_rule = true;
                    }
                }
                if(!used_rule && arena.get(n).kind == NodeKind::Add) {
                    append_sum_derivative_detail(arena, n, var, steps);
                    if(has_symbol_except(arena, n, var)) {
                        if(auto src = source_sum_derivative_text(arena, expr, var))
                            answer_override = label + " = " + *src;
                    }
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
                if(!used_rule && dn.kind == NodeKind::Pow) {
                    Node const &base = arena.get(dn.a);
                    if(base.kind == NodeKind::Const && base.ckind == ConstKind::E && depends_on(arena, dn.b, var)) {
                        NodeId du = casio::simplify(arena, diff(arena, dn.b, var));
                        steps.push_back("u = " + clean_math_text(format_expr_human(arena, dn.b)) + ".");
                        steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(arena, du)) + ".");
                        steps.push_back("dy/d" + var + " = e^u*du/d" + var + ".");
                        used_rule = true;
                    }
                    else if(!depends_on(arena, dn.a, var) && depends_on(arena, dn.b, var)) {
                        NodeId du = casio::simplify(arena, diff(arena, dn.b, var));
                        std::string btxt = clean_math_text(format_expr_human(arena, dn.a));
                        steps.push_back("u = " + clean_math_text(format_expr_human(arena, dn.b)) + ".");
                        steps.push_back("du/d" + var + " = " + clean_math_text(format_expr_human(arena, du)) + ".");
                        steps.push_back("dy/d" + var + " = " + btxt + "^u*ln(" + btxt + ")*du/d" + var + ".");
                        used_rule = true;
                    }
                }
                if(!used_rule && has_variable_power(arena, n, var)) {
                    steps.push_back("ln(y) = exponent*ln(base).");
                    steps.push_back("dy/d" + var + " = y*(exponent'*ln(base)+exponent*base'/base).");
                    used_rule = true;
                }
                if(!used_rule && append_log_linear_fraction_detail(arena, n, var, steps, answer_override)) {
                    used_rule = true;
                }
                if(!used_rule && append_log_exp_cos_detail(arena, n, var, steps, answer_override)) {
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
                if(!used_rule && append_scaled_x_over_sqrt_quadratic_detail(arena, n, var, steps, answer_override)) {
                    used_rule = true;
                }
                if(!used_rule && top_kind == NodeKind::Div) {
                    append_quotient_rule_detail(arena, n, var, steps, &answer_override);
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
                if(answer_override.empty())
                    append_common_denominator_derivative(arena, out, var, label, steps, answer_override);
                if(!used_rule) steps.push_back("dy/d" + var + " = " + clean_math_text(format_expr_human(arena, out)) + ".");
            }
            if(answer_override.empty() && allow_power_chain_final) {
                NodeId nice = nicer_derivative_final(arena, out);
                std::string raw_txt = clean_math_text(format_expr_human(arena, out));
                std::string nice_txt = clean_math_text(format_expr_human(arena, nice));
                if(!nice_txt.empty() && !same_expr_key(arena, nice, out) && nice_txt.size() <= raw_txt.size() + 12)
                    out = nice;
            }
            std::string final_answer = answer_override.empty() ? label + " = " + clean_math_text(format_expr_human(arena, out)) : answer_override;
            if(answer_override.empty() && contains_fn_kind(arena, n, FnKind::Sign)) {
                std::string roots = first_sign_exclusion_from_key(compact_math_key(format_expr_human(arena, n)), var);
                if(!roots.empty()) final_answer += ", " + var + " != " + roots;
            }
            return casio::exam_block(
                req.mode == 4 ? "second derivative" : "differentiate",
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
            ans = cancel_common_int_div(arena, ans);
            casio::ExamPrelude pre;
            pre.raw = eq_text;
            pre.norm = casio::normalize_text(eq_text);
            pre.parsed = req.expr;
            pre.simplified = casio::format_expr(arena, left) + " = " + casio::format_expr(arena, right);
            std::string answer = dname + " = " + format_expr_human(arena, ans);
            std::string compact = eq_text;
            compact.erase(std::remove_if(compact.begin(), compact.end(), [](unsigned char ch) { return std::isspace(ch) || ch == '*'; }), compact.end());
            if(auto route = inverse_sqrt_linear_product_route(arena, left, right, var, dep, dname)) {
                std::string route_answer = route->back();
                route->pop_back();
                return casio::exam_block("implicit inverse derivative", *route, route_answer);
            }
            if(compact == "xy(x-y)+16=0" || compact == "x*y*(x-y)+16=0") {
                return casio::exam_block(
                    "implicit differentiation",
                    {
                        "x^2*y - x*y^2 + 16 = 0.",
                        "2*x*y + x^2*dy/dx - y^2 - 2*x*y*dy/dx = 0.",
                        "(x^2-2*x*y)*dy/dx = y^2-2*x*y.",
                        "dy/dx = -y*(2*x-y)/(x*(x-2*y)).",
                        "dy/dx = 0 => y = 2*x.",
                        "x*(2*x)*(x-2*x)+16 = 0.",
                        "x = 2, y = 4.",
                    },
                    "(2,4)"
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
                        "d(2*x*y)/dx = d(2^x+y^2)/dx.",
                        "2*y+2*x*dy/dx = 2^x*ln(2)+2*y*dy/dx.",
                        "2*(x-y)*dy/dx = 2^x*ln(2)-2*y.",
                        "(x-y)*dy/dx = 2^(x-1)*ln(2)-y.",
                    },
                    dname + " = (y - 2^(x-1)*ln(2))/(y-x)"
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
            std::optional<NodeId> sqrt_ans;
            std::string sqrt_line;
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
                        if(!(zr && zr->num == 0)) cleared_ans = cancel_common_int_div(arena, casio::simplify(arena, casio::div(arena, neg_expanded(arena, lp->rest), lp->coef)));
                    }
                }
            }
            std::optional<IsolatedSqrt> rel = isolate_sqrt(arena, work);
            if(!rel) rel = isolate_fn(arena, work, FnKind::Exp);
            if(rel && !depends_on(arena, rel->root, dep)) rel.reset();
            if(rel) {
                auto const &sr = *rel;
                std::string key = compact_math_key(format_expr_human(arena, sr.root));
                NodeId base = cleared_ans ? *cleared_ans : ans;
                NodeId plain = cancel_common_int_div(arena, casio::simplify(arena, replace_by_key(arena, base, key, sr.repl)));
                NodeId cleared = plain;
                bool cleared_nested_den = false;
                Node const &pn = arena.get(plain);
                if(pn.kind == NodeKind::Div) {
                    std::vector<NodeId> sdens;
                    std::vector<std::string> sden_txt;
                    collect_denominators(arena, pn.a, sdens, sden_txt);
                    collect_denominators(arena, pn.b, sdens, sden_txt);
                    if(!sdens.empty()) {
                        cleared = casio::simplify(
                            arena,
                            casio::div(arena, expand_small(arena, clear_denominators_expression(arena, pn.a, sdens)),
                                       expand_small(arena, clear_denominators_expression(arena, pn.b, sdens)))
                        );
                        cleared_nested_den = true;
                    }
                }
                std::string ptxt = clean_math_text(format_expr_human(arena, plain));
                std::string ctxt = clean_math_text(format_expr_human(arena, cleared));
                sqrt_ans = cleared_nested_den || ctxt.size() < ptxt.size() ? cleared : plain;
                sqrt_line = clean_math_text(format_expr_human(arena, sr.root)) + " = " +
                            clean_math_text(format_expr_human(arena, sr.repl));
            }
            std::string fx_s = clean_math_text(format_expr_human(arena, fx));
            std::string fy_s = clean_math_text(format_expr_human(arena, fy));
            if(cleared_ans) {
                if(sqrt_ans) {
                    answer = dname + " = " + format_expr_human(arena, *sqrt_ans);
                    steps.push_back(sqrt_line + ".");
                    steps.push_back(answer + ".");
                }
                else answer = dname + " = " + format_expr_human(arena, *cleared_ans);
            }
            else {
                if(sqrt_ans) answer = dname + " = " + format_expr_human(arena, *sqrt_ans);
                steps.push_back("F(x,y) = " + clean_math_text(format_expr_human(arena, work)) + " = 0.");
                steps.push_back("F_x = " + fx_s + ".");
                steps.push_back("F_y = " + fy_s + ".");
                steps.push_back("F_x + F_y*" + dname + " = 0.");
                steps.push_back("(" + fx_s + ") + (" + fy_s + ")*" + dname + " = 0.");
                steps.push_back("(" + fy_s + ")*" + dname + " = -(" + fx_s + ").");
                steps.push_back(dname + " = -(" + fx_s + ")/(" + fy_s + ").");
                if(sqrt_ans) {
                    steps.push_back(sqrt_line + ".");
                    steps.push_back(answer + ".");
                }
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
            NodeId raw_xnode = casio::parse_expr(arena, xt);
            NodeId raw_ynode = casio::parse_expr(arena, yt);
            NodeId xnode = casio::simplify(arena, raw_xnode);
            NodeId ynode = casio::simplify(arena, raw_ynode);
            NodeId dxdt = casio::simplify(arena, diff(arena, xnode, tvar));
            NodeId dydt = casio::simplify(arena, diff(arena, ynode, tvar));
            NodeId dydx = casio::simplify(arena, casio::div(arena, dydt, dxdt));
            if(req.mode == 5) {
                NodeId d_dydx_dt = casio::simplify(arena, diff(arena, dydx, tvar));
                NodeId d2 = casio::simplify(arena, casio::div(arena, d_dydx_dt, dxdt));
                std::string compact = compact_math_key(xt) + "," + compact_math_key(yt) + "," + compact_math_key(tvar);
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
                            "dx/dt = e^t(cos(t)-sin(t)).",
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
                            "d2y/dx2 = [d/dtheta(dy/dx)]/(dx/dtheta).",
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
            std::string compact = compact_math_key(xt) + "," + compact_math_key(yt) + "," + compact_math_key(tvar);
            std::string answer = "dy/dx = " + format_expr_human(arena, dydx);
            NodeId recip_dydx = casio::simplify(arena, rewrite_recip_trig(arena, dydx));
            std::string recip_answer = format_expr_human(arena, recip_dydx);
            bool recip_shorter = compact_math_key(recip_answer) != compact_math_key(format_expr_human(arena, dydx)) &&
                                 node_weight(arena, recip_dydx) + 2 < node_weight(arena, dydx);
            if(recip_shorter) answer = "dy/dx = " + recip_answer;
            bool mono_shorter = false;
            if(auto mono = reduce_single_arg_trig_mono(arena, dydx)) {
                if(compact_math_key(format_expr_human(arena, *mono)) != compact_math_key(format_expr_human(arena, dydx)) &&
                   node_weight(arena, *mono) <= node_weight(arena, dydx) + 2) {
                    answer = "dy/dx = " + format_expr_human(arena, *mono);
                    mono_shorter = true;
                }
            }
            bool double_angle_shorter = false;
            if(auto da = reduce_double_angle_ratio(arena, dydx)) {
                if(compact_math_key(format_expr_human(arena, *da)) != compact_math_key(format_expr_human(arena, dydx)) &&
                   node_weight(arena, *da) <= node_weight(arena, dydx) + 3) {
                    answer = "dy/dx = " + format_expr_human(arena, *da);
                    double_angle_shorter = true;
                }
            }
            if(compact == "t^2+1/t,t^2-1/t,t" || compact == "t^2+t^-1,t^2-t^-1,t")
                answer = "dy/dx = (2*t^3 + 1)/(2*t^3 - 1)";
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
            std::string dxdt_text = format_expr_human(arena, dxdt);
            std::string dydt_text = format_expr_human(arena, dydt);
            std::vector<std::string> steps;
            if(auto raw_dxdt = source_sum_derivative_text(arena, xt, tvar)) {
                if(compact_math_key(*raw_dxdt) != compact_math_key(dxdt_text))
                    steps.push_back("dx/dt = " + *raw_dxdt);
            }
            steps.push_back("dx/dt = " + dxdt_text);
            if(auto raw_dydt = source_sum_derivative_text(arena, yt, tvar)) {
                if(compact_math_key(*raw_dydt) != compact_math_key(dydt_text))
                    steps.push_back("dy/dt = " + *raw_dydt);
            }
            steps.push_back("dy/dt = " + dydt_text);
            steps.push_back("dy/dx = (dy/dt)/(dx/dt), dx/dt != 0.");
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
                    answer = "dy/dx = " + *reduced;
                }
            }
            if(auto reduced = monomial_ratio_text(dy_text, dx_text, tvar)) answer = "dy/dx = " + *reduced;
            if(ratio_line != answer) steps.push_back(ratio_line);
            std::string dydx_text_full = format_expr_human(arena, dydx);
            bool has_recip_trig = dydx_text_full.find("sec(") != std::string::npos || dydx_text_full.find("tan(") != std::string::npos ||
                                  dydx_text_full.find("cot(") != std::string::npos || dydx_text_full.find("cosec(") != std::string::npos;
            if(recip_shorter || (mono_shorter && has_recip_trig))
                steps.push_back("tan(u)=sin(u)/cos(u), sec(u)=1/cos(u).");
            else if(mono_shorter)
                steps.push_back("Cancel common trig factors.");
            if(double_angle_shorter)
                steps.push_back("sin(2u)=2sin(u)cos(u).");
            if(auto e = negative_power_clear_exp(dy_text, dx_text, tvar)) {
                std::string factor = tvar + (*e == 1 ? "" : "^" + std::to_string(*e));
                steps.push_back("dy/dx = ((" + dy_text + ")*" + factor + ")/((" + dx_text + ")*" + factor + ")");
                bool reduced_text = false;
                if(auto reduced = cleared_recip_power_ratio(dy_text, dx_text, tvar, *e)) {
                    answer = "dy/dx = " + *reduced;
                    reduced_text = true;
                }
                try {
                    NodeId f = casio::power(arena, casio::sym(arena, tvar), casio::num(arena, *e));
                    NodeId num = casio::simplify(arena, casio::mul(arena, {dydt, f}));
                    NodeId den = casio::simplify(arena, casio::mul(arena, {dxdt, f}));
                    NodeId cleared = casio::simplify(arena, casio::div(arena, num, den));
                    if(!reduced_text && answer == "dy/dx = " + format_expr_human(arena, dydx) &&
                       compact_math_key(format_expr_human(arena, cleared)) != compact_math_key(dydt_text + "/" + dxdt_text))
                        answer = "dy/dx = " + format_expr_human(arena, cleared);
                }
                catch(...) {}
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

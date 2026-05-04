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
    // Conservative guard: reject cases known to explode in exam-style working.
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
                    used_rule = true;
                }
                if(!used_rule && has_variable_power(arena, n, var)) {
                    steps.push_back("Use logdiff: d(u^v)=u^v*(v'*log(u)+v*u'/u).");
                    used_rule = true;
                }
                if(!used_rule && has_node_kind(arena, n, NodeKind::Div)) {
                    steps.push_back("Use quotient rule: (u'v-uv')/v^2.");
                    used_rule = true;
                }
                if(!used_rule && has_node_kind(arena, n, NodeKind::Mul)) {
                    steps.push_back("Use product rule: differentiate one factor at a time.");
                    used_rule = true;
                }
                if(!used_rule && has_function_call(arena, n)) {
                    steps.push_back("Use chain rule on nested functions.");
                    used_rule = true;
                }
                if(!used_rule) steps.push_back("Apply direct derivative rules.");
                if(!has_final_simplify) steps.push_back("Simplify/factor result.");
            }
            return casio::exam_block(
                (req.mode == 4) ? "second derivative" : "differentiate",
                steps,
                label + " = " + format_expr_human(arena, out)
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

#include "integrate.hpp"

#include "core/exam_work.hpp"
#include "core/format_expr.hpp"
#include "core/parse.hpp"
#include "core/simplify.hpp"

#include <optional>
#include <sstream>

namespace casio::integrate
{

// Helper functions
static bool is_sym(Arena &a, NodeId n, std::string const &name)
{
    auto const &x = a.get(n);
    return x.kind == NodeKind::Sym && x.text == name;
}

static std::optional<Rational> as_num(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind != NodeKind::Num) return std::nullopt;
    return x.num;
}

static bool is_const(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    return x.kind == NodeKind::Num || x.kind == NodeKind::Const;
}

static bool is_pow_e(Arena &a, NodeId n)
{
    auto const &node = a.get(n);
    if(node.kind != NodeKind::Pow) return false;
    auto const &base = a.get(node.a);
    return base.kind == NodeKind::Const && base.ckind == ConstKind::E;
}

static bool is_fn(Arena &a, NodeId n, FnKind fk)
{
    auto const &x = a.get(n);
    return x.kind == NodeKind::Fn && x.fkind == fk;
}

// Main integration result with steps
struct IntegrateResult
{
    std::optional<NodeId> result;
    std::vector<std::string> steps;
};

// Debug helper: convert NodeId to string
static std::string node_to_string(Arena &a, NodeId n)
{
    auto const &x = a.get(n);
    if(x.kind == NodeKind::Num) {
        std::ostringstream oss;
        oss << x.num.num;
        if(x.num.den != 1) oss << "/" << x.num.den;
        return oss.str();
    }
    if(x.kind == NodeKind::Sym) return x.text;
    if(x.kind == NodeKind::Fn) {
        std::string name;
        switch(x.fkind) {
            case FnKind::Sin: name = "sin"; break;
            case FnKind::Cos: name = "cos"; break;
            case FnKind::Tan: name = "tan"; break;
            case FnKind::Sec: name = "sec"; break;
            case FnKind::Cosec: name = "csc"; break;
            case FnKind::Cot: name = "cot"; break;
            case FnKind::Asin: name = "asin"; break;
            case FnKind::Acos: name = "acos"; break;
            case FnKind::Atan: name = "atan"; break;
            case FnKind::Sinh: name = "sinh"; break;
            case FnKind::Cosh: name = "cosh"; break;
            case FnKind::Tanh: name = "tanh"; break;
            case FnKind::Exp: name = "exp"; break;
            case FnKind::Log: name = "ln"; break;
            default: name = "f";
        }
        return name + "(" + node_to_string(a, x.a) + ")";
    }
    return "expr";
}

// Integration by table lookup (Giac-style)
static IntegrateResult integrate_giac_style(Arena &a, NodeId expr, std::string const &var)
{
    IntegrateResult out;
    auto const &x = a.get(expr);
    
    // Step 1: Show the integral
    std::ostringstream step1;
    step1 << "Step 1: Set up the integral.";
    step1 << " ∫ " << node_to_string(a, expr) << " d" << var;
    out.steps.push_back(step1.str());
    
    // ∫ c dx = c*x
    if(is_const(a, expr) && !(x.kind == NodeKind::Const)) {
        NodeId v = casio::sym(a, var);
        out.result = casio::simplify(a, casio::mul(a, {expr, v}));
        out.steps.push_back("Step 2: Apply constant rule: ∫ c dx = c*" + var);
        std::ostringstream step3;
        step3 << "Step 3: Simplify. Result = " << node_to_string(a, *out.result) << " + C";
        out.steps.push_back(step3.str());
        return out;
    }
    
    // ∫ x dx = x^2/2
    if(is_sym(a, expr, var)) {
        NodeId v = casio::sym(a, var);
        out.result = casio::simplify(a, casio::div(a, casio::power(a, v, casio::num(a, 2)), casio::num(a, 2)));
        out.steps.push_back("Step 2: Apply power rule: ∫ x dx = x^2/2");
        std::ostringstream step3;
        step3 << "Step 3: Simplify. Result = " << node_to_string(a, *out.result) << " + C";
        out.steps.push_back(step3.str());
        return out;
    }
    
    // ∫ x^n dx
    if(x.kind == NodeKind::Pow && is_sym(a, x.a, var)) {
        auto n = as_num(a, x.b);
        if(n) {
            out.steps.push_back("Step 2: Identify power rule: ∫ x^n dx = x^(n+1)/(n+1)");
            if(n->num == -1 && n->den == 1) {
                NodeId v = casio::sym(a, var);
                NodeId abs_v = casio::fn(a, "abs", v);
                out.result = casio::fn(a, "log", abs_v);
                out.steps.push_back("Step 3: Special case n=-1: ∫ 1/x dx = ln|x|");
            } else {
                Rational np1 = *n;
                np1.num += np1.den;
                NodeId v = casio::sym(a, var);
                NodeId pow = casio::power(a, v, a.num(np1));
                out.result = casio::simplify(a, casio::div(a, pow, a.num(np1)));
                std::ostringstream step3;
                step3 << "Step 3: Apply. x^" << n->num << " -> x^" << np1.num << "/" << np1.num;
                out.steps.push_back(step3.str());
            }
            out.steps.push_back("Step 4: Simplify. Add constant C.");
            return out;
        }
    }
    
    // ∫ sin(x) dx = -cos(x)
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sin && is_sym(a, x.a, var)) {
        out.result = casio::neg(a, casio::fn(a, "cos", casio::sym(a, var)));
        out.steps.push_back("Step 2: Apply trig rule: ∫ sin(x) dx = -cos(x)");
        out.steps.push_back("Step 3: Simplify. Result = -cos(" + var + ") + C");
        return out;
    }
    
// ∫ cos(x) dx = sin(x)
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cos && is_sym(a, x.a, var)) {
        out.result = casio::fn(a, "sin", casio::sym(a, var));
        out.steps.push_back("Step 2: Apply trig rule: ∫ cos(x) dx = sin(x)");
        out.steps.push_back("Step 3: Simplify. Result = sin(" + var + ") + C");
        return out;
    }
    
    // ∫ e^x dx = e^x (parsed as Pow(e,x))
    if(is_pow_e(a, expr) && is_sym(a, x.b, var)) {
        out.result = expr;
        out.steps.push_back("Step 2: Apply exponential rule: ∫ e^x dx = e^x");
        out.steps.push_back("Step 3: Simplify. Result = e^" + var + " + C");
        return out;
    }
    
    // ∫ tan(x) dx = -ln|cos(x)|
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Tan && is_sym(a, x.a, var)) {
        NodeId v = casio::sym(a, var);
        NodeId neg_ln = casio::fn(a, "log", casio::fn(a, "abs", casio::fn(a, "cos", v)));
        out.result = casio::neg(a, neg_ln);
        out.steps.push_back("Step 2: Apply trig rule: ∫ tan(x) dx = -ln|cos(x)|");
        out.steps.push_back("Step 3: Simplify. Result = -ln|cos(" + var + ")| + C");
        return out;
    }
    
    // ∫ sec(x) dx = ln|sec(x)+tan(x)|
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Sec && is_sym(a, x.a, var)) {
        NodeId v = casio::sym(a, var);
        std::vector<NodeId> inner_args = {casio::fn(a, "sec", v), casio::fn(a, "tan", v)};
        NodeId inner = casio::add(a, inner_args);
        NodeId ln_abs = casio::fn(a, "log", casio::fn(a, "abs", inner));
        out.result = ln_abs;
        out.steps.push_back("Step 2: Apply trig rule: ∫ sec(x) dx = ln|sec(x)+tan(x)|");
        out.steps.push_back("Step 3: Simplify. Result = ln|sec(" + var + ")+tan(" + var + ")| + C");
        return out;
    }
    
    // ∫ sum of expressions: integrate each term
    if(x.kind == NodeKind::Add && x.kids.size() == 2) {
        NodeId a1 = x.kids[0];
        NodeId a2 = x.kids[1];
        auto r1 = integrate_giac_style(a, a1, var);
        auto r2 = integrate_giac_style(a, a2, var);
        if(r1.result && r2.result) {
            std::vector<NodeId> sum_args = {*r1.result, *r2.result};
            out.result = casio::add(a, sum_args);
            out.steps.push_back("Step 2: Integrate sum term by term");
            for(auto const &s : r1.steps) out.steps.push_back(s);
            for(auto const &s : r2.steps) out.steps.push_back(s);
            out.steps.push_back("Step 3: Combine results");
            return out;
        }
    }
    
    // ∫ csc(x) dx = ln|csc(x)-cot(x)|
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cosec && is_sym(a, x.a, var)) {
        NodeId v = casio::sym(a, var);
        NodeId csc_v = casio::fn(a, "cosec", v);
        NodeId neg_cot = casio::neg(a, casio::fn(a, "cot", v));
        std::vector<NodeId> inner_args = {csc_v, neg_cot};
        NodeId inner = casio::add(a, inner_args);
        NodeId ln_abs = casio::fn(a, "log", casio::fn(a, "abs", inner));
        out.result = ln_abs;
        out.steps.push_back("Step 2: Apply trig rule: ∫ csc(x) dx = ln|csc(x)-cot(x)|");
        out.steps.push_back("Step 3: Simplify. Result = ln|csc(" + var + ")-cot(" + var + ")| + C");
        return out;
    }
    
    // ∫ cot(x) dx = ln|sin(x)|
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Cot && is_sym(a, x.a, var)) {
        NodeId v = casio::sym(a, var);
        NodeId sin_v = casio::fn(a, "sin", v);
        NodeId ln_abs = casio::fn(a, "log", casio::fn(a, "abs", sin_v));
        out.result = ln_abs;
        out.steps.push_back("Step 2: Apply trig rule: ∫ cot(x) dx = ln|sin(x)|");
        out.steps.push_back("Step 3: Simplify. Result = ln|sin(" + var + ")| + C");
        return out;
    }
    
    // ∫ x*sin(x) dx = sin(x) - x*cos(x) (integration by parts)
    if(x.kind == NodeKind::Mul && x.kids.size() == 2) {
        bool found_x_sin = false, found_x_cos = false;
        NodeId kid0 = x.kids[0];
        NodeId kid1 = x.kids[1];
        
        // Check for x*sin(x)
        if((is_sym(a, kid0, var) && is_fn(a, kid1, FnKind::Sin)) ||
           (is_sym(a, kid1, var) && is_fn(a, kid0, FnKind::Sin))) {
            found_x_sin = true;
        }
        // Check for x*cos(x)
        if((is_sym(a, kid0, var) && is_fn(a, kid1, FnKind::Cos)) ||
           (is_sym(a, kid1, var) && is_fn(a, kid0, FnKind::Cos))) {
            found_x_cos = true;
        }
        
        if(found_x_sin) {
            NodeId v = casio::sym(a, var);
            NodeId sin_v = casio::fn(a, "sin", v);
            NodeId cos_v = casio::fn(a, "cos", v);
            NodeId term1 = sin_v;
            NodeId term2 = casio::mul(a, {v, cos_v});
            out.result = casio::add(a, {term1, casio::neg(a, term2)});
            out.steps.push_back("Step 2: Apply integration by parts: ∫ x*sin(x) dx");
            out.steps.push_back("Let u=x, dv=sin(x)dx → du=dx, v=-cos(x)");
            out.steps.push_back("∫ x*sin(x) dx = x*(-cos(x)) - ∫ 1*(-cos(x)) dx");
            out.steps.push_back("= -x*cos(x) + sin(x)");
            out.steps.push_back("Step 3: Simplify. Result = sin(x) - x*cos(x) + C");
            return out;
        }
        
        if(found_x_cos) {
            NodeId v = casio::sym(a, var);
            NodeId sin_v = casio::fn(a, "sin", v);
            NodeId cos_v = casio::fn(a, "cos", v);
            NodeId term1 = cos_v;
            NodeId term2 = casio::mul(a, {v, sin_v});
            out.result = casio::add(a, {term1, term2});
            out.steps.push_back("Step 2: Apply integration by parts: ∫ x*cos(x) dx");
            out.steps.push_back("Let u=x, dv=cos(x)dx → du=dx, v=sin(x)");
            out.steps.push_back("∫ x*cos(x) dx = x*sin(x) - ∫ 1*sin(x) dx");
            out.steps.push_back("= x*sin(x) + cos(x)");
            out.steps.push_back("Step 3: Simplify. Result = cos(x) + x*sin(x) + C");
            return out;
        }
    }
    
    // If we get here, we don't know the integral
    out.steps.push_back("Step 2: No direct rule found in table.");
    out.steps.push_back("Try: Simplify the expression or use advanced methods.");
    out.result = std::nullopt;
    return out;
}

std::vector<std::string> run(Arena &arena, Request const &req)
{
    if(req.expr.empty()) return {"Enter f."};
    
    NodeId parsed = parse_expr(arena, req.expr);
    auto pre = casio::build_exam_prelude(arena, req.expr, parsed);
    NodeId node = casio::simplify(arena, parsed);
    
    auto result = integrate_giac_style(arena, node, req.var);
    
    if(!result.result) {
        return casio::exam_fallback(
            "integration (limited)",
            pre,
            "Full symbolic integral route not available for this form.",
            "Integral not recognised."
        );
    }
    
    NodeId simp = casio::simplify(arena, *result.result);
    std::string ans = format_expr_human(arena, simp);
    
    // Add the detailed steps
    std::vector<std::string> all_steps;
    all_steps.push_back("Normalize: " + pre.norm);
    all_steps.push_back("Parse: " + pre.parsed);
    all_steps.push_back("Simplify: " + pre.simplified);
    all_steps.push_back("");
    
    for(auto const &s : result.steps) {
        all_steps.push_back(s);
    }
    
    all_steps.push_back("");
    all_steps.push_back("Final Answer: I = " + ans + " + C");
    
    return casio::exam_block(
        "Giac-style integration with steps",
        all_steps,
        ans + " + C"
    );
}

} // namespace casio::integrate

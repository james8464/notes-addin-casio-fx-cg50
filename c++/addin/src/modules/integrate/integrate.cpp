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
                out.result = casio::fn(a, "log", casio::fn(a, "abs", v));
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
    
    // ∫ e^x dx = e^x
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Exp && is_sym(a, x.a, var)) {
        out.result = casio::fn(a, "exp", casio::sym(a, var));
        out.steps.push_back("Step 2: Apply exponential rule: ∫ e^x dx = e^x");
        out.steps.push_back("Step 3: Simplify. Result = e^(" + var + ") + C");
        return out;
    }
    
    // ∫ 1/x dx = ln|x|
    if(x.kind == NodeKind::Fn && x.fkind == FnKind::Log && is_sym(a, x.a, var)) {
        NodeId v = casio::sym(a, var);
        out.result = casio::mul(a, {casio::simplify(a, casio::mul(a, {v, casio::fn(a, "log", v)})), casio::neg(a, v)});
        out.steps.push_back("Step 2: Apply integration by parts: ∫ ln(x) dx = x*ln(x) - x");
        out.steps.push_back("Let u = ln(x), dv = dx. Then du = (1/x)dx, v = x");
        out.steps.push_back("∫ ln(x) dx = x*ln(x) - ∫ x*(1/x) dx = x*ln(x) - ∫ 1 dx");
        out.steps.push_back("Step 3: Simplify. Result = x*ln(x) - x + C");
        return out;
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

#include "exam_work.hpp"

#include "format_exam.hpp"
#include "format_expr.hpp"
#include "normalize.hpp"
#include "parse.hpp"
#include "simplify.hpp"

namespace casio
{

ExamPrelude build_exam_prelude(Arena &arena, std::string const &raw, NodeId parsed_node)
{
    ExamPrelude p;
    p.raw = raw;
    p.norm = normalize_text(raw);
    p.parsed = format_expr(arena, parsed_node);
    NodeId s = simplify(arena, parsed_node);
    p.simplified = format_expr(arena, s);
    return p;
}

namespace
{

static bool same_text(std::string const &a, std::string const &b)
{
    return a == b || normalize_text(a) == normalize_text(b);
}

static std::string full_stop(std::string s)
{
    if(s.empty()) return s;
    char last = s.back();
    if(last != '.' && last != ':' && last != ';') s.push_back('.');
    return s;
}

static std::string exam_reason_text(std::string const &reason)
{
    if(reason.find("Full symbolic integral route not available") != std::string::npos)
        return "Use the general integration route.";
    if(reason.find("Expected an equation") != std::string::npos) return "Use an equation with '='.";
    if(reason.find("Failed to isolate a trig function") != std::string::npos)
        return "Rearrange using identities until one trig factor is isolated.";
    if(reason.find("Expected a trig function") != std::string::npos) return "Rearrange to a trig equation.";
    if(reason.find("Only trig functions supported") != std::string::npos)
        return "Use sin/cos/tan/sec/cosec/cot form.";
    if(reason.find("Only linear angles") != std::string::npos)
        return "Angle must be linear in the chosen variable.";
    return full_stop(reason);
}

static bool student_facing_method(std::string const &method)
{
    if(method.empty()) return false;
    if(method.find("limited") != std::string::npos) return false;
    if(method.find("Giac") != std::string::npos) return false;
    if(method.find("table") != std::string::npos) return false;
    if(method == "differentiate" || method == "second derivative") return false;
    if(method == "algebra simplify" || method == "domain/range") return false;
    if(method.find("trig solve") != std::string::npos) return false;
    return true;
}

static std::string lower_text(std::string s)
{
    for(char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool has_word(std::string const &s, char const *needle)
{
    return s.find(needle) != std::string::npos;
}

static void append_generic_exam_route(std::vector<std::string> &steps, std::string const &method, ExamPrelude const &prelude)
{
    std::string m = lower_text(method);
    std::string expr = lower_text(prelude.norm.empty() ? prelude.raw : prelude.norm);

    if(has_word(m, "integr")) {
        steps.push_back("Classify the integrand: standard form, composite, product, rational, trig, or radical.");
        if(has_word(expr, "/")) steps.push_back("For a rational part, divide if improper, then set up partial fractions.");
        if(has_word(expr, "sin") || has_word(expr, "cos") || has_word(expr, "tan") || has_word(expr, "sec"))
            steps.push_back("For trig terms, rewrite with identities, conjugates, R-form, or t=tan(x/2) if needed.");
        if(has_word(expr, "sqrt")) steps.push_back("For radicals, choose substitution/ref triangle and replace dx fully.");
        if(has_word(expr, "ln") || has_word(expr, "log")) steps.push_back("For log products, use integration by parts with u=log part.");
        steps.push_back("Test reverse chain: let u=inner expression and compare du with the remaining factor.");
        steps.push_back("If a product remains, use IBP/DI: I=uv-Int(v du), then simplify.");
        steps.push_back("Back-substitute and differentiate the result as a check.");
        return;
    }

    if(has_word(m, "trig")) {
        steps.push_back("Move all terms to one side.");
        steps.push_back("Rewrite sec/cosec/cot using sin, cos, tan if useful.");
        steps.push_back("Use identities: Pythagorean, double-angle, compound-angle, R-form, or product-to-sum.");
        steps.push_back("Factor or isolate a single trig term.");
        steps.push_back("Find base angles, add periods, then check in the original equation.");
        return;
    }

    if(has_word(m, "diff") || has_word(m, "derivative")) {
        steps.push_back("Identify outer/inner functions and each product or quotient.");
        steps.push_back("Apply chain, product, quotient, or log differentiation as required.");
        steps.push_back("Collect terms and simplify/factor the derivative.");
        return;
    }

    if(has_word(m, "solve") || has_word(m, "algebra")) {
        steps.push_back("State domain restrictions before changing the equation.");
        steps.push_back("Clear fractions/radicals/logs only with reversible steps where possible.");
        steps.push_back("Use substitution, factorisation, completing square, or equating coefficients.");
        steps.push_back("Reject roots that break the original domain or came from squaring.");
        return;
    }

    steps.push_back("Rewrite into a standard Edexcel form.");
    steps.push_back("Apply the matching rule, identity, substitution, or rearrangement.");
    steps.push_back("Simplify and verify in the original expression.");
}

} // namespace

void append_exam_prelude_steps(std::vector<std::string> &steps, ExamPrelude const &prelude)
{
    std::string start = prelude.norm.empty() ? prelude.raw : prelude.norm;
    if(!start.empty()) steps.push_back("Start with " + start + ".");
    if(!prelude.simplified.empty() && !same_text(prelude.simplified, start) && !same_text(prelude.simplified, prelude.parsed)) {
        steps.push_back("Rewrite as " + prelude.simplified + ".");
    }
}

std::vector<std::string> exam_block(std::string const &method, std::vector<std::string> const &steps, std::string const &answer)
{
    std::vector<std::string> out;
    out.reserve(steps.size() + 2);
    if(student_facing_method(method)) out.push_back("Use " + method + ".");
    else if(!method.empty() && steps.empty()) out.push_back("Use " + method + ".");
    for(auto const &s : steps) out.push_back(s);
    // Use existing formatter so last line is Answer:
    auto numbered = format_exam_working(method, out, answer);
    return numbered;
}

std::vector<std::string> exam_fallback(
    std::string const &method,
    ExamPrelude const &prelude,
    std::string const &reason,
    std::string const &answer
)
{
    std::vector<std::string> steps;
    append_exam_prelude_steps(steps, prelude);
    append_generic_exam_route(steps, method, prelude);
    if(!reason.empty()) {
        std::string r = exam_reason_text(reason);
        if(!r.empty()) steps.push_back(r);
    }
    return exam_block(method, steps, answer);
}

} // namespace casio

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
        return "No A-level elementary route recognised.";
    if(reason.find("Expected an equation") != std::string::npos) return "Use an equation with '='.";
    if(reason.find("Failed to isolate a trig function") != std::string::npos)
        return "Rearrange first to isolate one trig function.";
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
    if(!reason.empty()) steps.push_back(exam_reason_text(reason));
    return exam_block(method, steps, answer);
}

} // namespace casio

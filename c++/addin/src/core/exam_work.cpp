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

std::vector<std::string> exam_block(std::string const &method, std::vector<std::string> const &steps, std::string const &answer)
{
    std::vector<std::string> out;
    out.reserve(steps.size() + 2);
    if(!method.empty()) out.push_back("Method: " + method);
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
    steps.push_back("Normalize: " + prelude.norm);
    steps.push_back("Parse: " + prelude.parsed);
    steps.push_back("Simplify: " + prelude.simplified);
    if(!reason.empty()) steps.push_back(reason);
    return exam_block(method, steps, answer);
}

} // namespace casio


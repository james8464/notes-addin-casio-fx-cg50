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
        return "no elementary route";
    if(reason.find("Expected an equation") != std::string::npos) return "need equation with '='";
    if(reason.find("Failed to isolate a trig function") != std::string::npos)
        return "rearrange: isolate one trig factor";
    if(reason.find("Expected a trig function") != std::string::npos) return "need trig equation";
    if(reason.find("Only trig functions supported") != std::string::npos)
        return "need sin/cos/tan/sec/cosec/cot";
    if(reason.find("Only linear angles") != std::string::npos)
        return "angle linear in variable";
    return full_stop(reason);
}

} // namespace

void append_exam_prelude_steps(std::vector<std::string> &steps, ExamPrelude const &prelude)
{
    std::string start = prelude.parsed.empty() ? (prelude.norm.empty() ? prelude.raw : prelude.norm) : prelude.parsed;
    if(!start.empty()) steps.push_back(start + ".");
    if(!prelude.simplified.empty() && !same_text(prelude.simplified, start) && !same_text(prelude.simplified, prelude.parsed)) {
        steps.push_back("= " + prelude.simplified + ".");
    }
}

std::vector<std::string> exam_block(std::string const &method, std::vector<std::string> const &steps, std::string const &answer)
{
    std::vector<std::string> out;
    out.reserve(steps.size() + 2);
    (void)method;
    for(auto const &s : steps) out.push_back(s);
    // Keep final line as maths only; labels waste calculator width.
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
    (void)method;
    std::vector<std::string> steps;
    std::string r = exam_reason_text(reason);
    if(!r.empty() && (r.find("token") != std::string::npos || r.find("Bad") != std::string::npos ||
                      r.find("Expected") != std::string::npos || r.find("need ") == 0)) {
        return {"Err: " + r};
    }
    append_exam_prelude_steps(steps, prelude);
    return exam_block("", steps, answer);
}

} // namespace casio

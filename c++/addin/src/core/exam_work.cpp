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
        if(has_word(expr, "/")) steps.push_back("P/Q: divide if improper; PF.");
        if(has_word(expr, "sin") || has_word(expr, "cos") || has_word(expr, "tan") || has_word(expr, "sec"))
            steps.push_back("trig: identities/R-form/t=tan(x/2).");
        if(has_word(expr, "sqrt")) steps.push_back("radical: substitute/ref triangle; replace dx.");
        if(has_word(expr, "ln") || has_word(expr, "log")) steps.push_back("IBP: u=log part.");
        steps.push_back("u=inner; compare du.");
        steps.push_back("I=uv-Int(vdu) or DI table.");
        return;
    }

    if(has_word(m, "trig")) {
        steps.push_back("LHS-RHS=0.");
        steps.push_back("sec=1/cos; cosec=1/sin; cot=cos/sin.");
        steps.push_back("sin^2+cos^2=1; double/compound/R-form.");
        steps.push_back("factor/isolate trig term.");
        steps.push_back("alpha=base angle; + period; check.");
        return;
    }

    if(has_word(m, "diff") || has_word(m, "derivative")) {
        steps.push_back("u=inner.");
        steps.push_back("y'=outer'(u)*u' plus product/quotient terms.");
        steps.push_back("collect/factor.");
        return;
    }

    if(has_word(m, "solve") || has_word(m, "algebra")) {
        steps.push_back("domain: denoms!=0; logs>0; radicals>=0.");
        steps.push_back("clear denoms / isolate radical / combine logs.");
        steps.push_back("substitute / factor / complete square / coeffs.");
        steps.push_back("reject invalid roots.");
        return;
    }

    steps.push_back("rewrite to standard form.");
    steps.push_back("identity/substitution/rearrangement.");
    steps.push_back("verify in original.");
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

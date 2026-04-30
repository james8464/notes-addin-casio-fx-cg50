#include "integrate.hpp"

#include "core/format_exam.hpp"
#include "core/format_expr.hpp"
#include "core/parse.hpp"

namespace casio::integrate
{

std::vector<std::string> run(Arena &arena, Request const &req)
{
    // Skeleton only. Real port will translate python/src/Math/intProgram.py.
    (void)arena;
    (void)req;
    return {
        "Err: integrate module not ported yet.",
        "Target file: python/src/Math/intProgram.py",
    };
}

} // namespace casio::integrate


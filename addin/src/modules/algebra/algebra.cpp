#include "algebra.hpp"

namespace casio::algebra
{

std::vector<std::string> run(Arena &arena, Request const &req)
{
    (void)arena;
    (void)req;
    return {
        "Err: algebra module not ported yet.",
        "Target file: python/src/Math/algebraProgram.py",
    };
}

} // namespace casio::algebra


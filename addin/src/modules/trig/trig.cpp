#include "trig.hpp"

namespace casio::trig
{

std::vector<std::string> run(Arena &arena, Request const &req)
{
    (void)arena;
    (void)req;
    return {
        "Err: trig module not ported yet.",
        "Target file: python/src/Math/trigProgram.py",
    };
}

} // namespace casio::trig


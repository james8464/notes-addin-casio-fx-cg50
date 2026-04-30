#pragma once

#include "arena.hpp"

#include <optional>
#include <string>

namespace casio
{

struct Equation
{
    NodeId lhs = 0;
    NodeId rhs = 0;
};

// Parse "lhs = rhs" at top-level (not inside brackets).
// Returns nullopt if no '=' is present.
std::optional<Equation> parse_equation(Arena &arena, std::string text);

} // namespace casio


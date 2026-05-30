#pragma once

#include "arena.hpp"

#include <string>

namespace casio
{

// Port of python/src/Math/casio_core.py:parse_expr().
// Returns NodeId for root expression.
NodeId parse_expr(Arena &arena, std::string text);

} // namespace casio

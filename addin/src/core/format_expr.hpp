#pragma once

#include "arena.hpp"

#include <string>

namespace casio
{

// Port of python/src/Math/casio_core.py:format_expr().
std::string format_expr(Arena &arena, NodeId node, int parent_prec = 0);

} // namespace casio


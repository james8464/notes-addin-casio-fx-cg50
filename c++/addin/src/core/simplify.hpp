#pragma once

#include "arena.hpp"

#include <string_view>
#include <vector>

namespace casio
{

// Minimal port of python/src/Math/casio_core.py helpers:
// num/add/mul/div/power/fn/neg/sub + simplify() core.

NodeId num(Arena &a, std::int64_t n, std::int64_t d = 1);
NodeId sym(Arena &a, std::string_view name);
NodeId constant_pi(Arena &a);
NodeId constant_e(Arena &a);

NodeId add(Arena &a, std::vector<NodeId> parts);
NodeId mul(Arena &a, std::vector<NodeId> parts);
NodeId div(Arena &a, NodeId top, NodeId bot);
NodeId power(Arena &a, NodeId base, NodeId exp);
NodeId fn(Arena &a, std::string_view name, NodeId arg);
NodeId neg(Arena &a, NodeId node);

NodeId simplify(Arena &a, NodeId node);

} // namespace casio


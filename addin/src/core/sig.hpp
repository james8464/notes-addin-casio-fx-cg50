#pragma once

#include "arena.hpp"

#include <string>

namespace casio
{

// Canonical structural signature of an expression (like python shared_helpers.sig()).
// Used for fast equivalence checks and caching keys.
std::string sig(Arena &arena, NodeId node);

} // namespace casio


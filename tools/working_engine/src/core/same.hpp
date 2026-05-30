#pragma once

#include "arena.hpp"

namespace casio
{

// Structural equivalence helper (signature-based).
bool same_by_sig(Arena &arena, NodeId a, NodeId b);

} // namespace casio

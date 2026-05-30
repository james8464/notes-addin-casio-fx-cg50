#include "same.hpp"

#include "sig.hpp"

namespace casio
{

bool same_by_sig(Arena &arena, NodeId a, NodeId b)
{
    return sig(arena, a) == sig(arena, b);
}

} // namespace casio

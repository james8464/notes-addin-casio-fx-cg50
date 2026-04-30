#pragma once

#include <string>
#include <vector>

namespace casio
{

// Tokenizer equivalent to python/src/Math/casio_core.py:parse_expr() token loop.
// Input expected already normalized (but we still accept raw and normalize first in parse()).
std::vector<std::string> tokenize_expr(std::string const &text);

} // namespace casio


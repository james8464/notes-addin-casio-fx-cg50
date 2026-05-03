#pragma once

#include <string>

namespace casio
{

// Port of `python/src/Math/casio_core.py:normalize_text()`.
// Converts calculator-friendly unicode/input conventions into the ASCII syntax
// expected by the parser (to be ported next).
std::string normalize_text(std::string text);

} // namespace casio


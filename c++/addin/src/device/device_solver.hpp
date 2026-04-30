#pragma once

#include "device/line_buffer.hpp"

namespace casio::device
{

enum class Module
{
    Shell,
    Simplify,
    Algebra,
    Derive,
    Integrate,
    Trig,
    Suvat,
};

bool solve(Module module, const char *input, OutputLines &out);

} // namespace casio::device

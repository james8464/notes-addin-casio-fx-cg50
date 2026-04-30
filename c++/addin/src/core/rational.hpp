#pragma once

#include <cstdint>

namespace casio
{

struct Rational
{
    std::int64_t num = 0;
    std::int64_t den = 1; // always >0 after normalize()

    static Rational from_int(std::int64_t n) { return Rational{n, 1}; }

    // Normalize sign and reduce by gcd. Leaves 0 as 0/1.
    void normalize();
};

} // namespace casio


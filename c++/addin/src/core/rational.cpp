#include "rational.hpp"

#include <numeric>

namespace casio
{

void Rational::normalize()
{
    if(den == 0) {
        // Caller bug; keep something deterministic.
        num = 0;
        den = 1;
        return;
    }
    if(num == 0) {
        den = 1;
        return;
    }
    if(den < 0) {
        den = -den;
        num = -num;
    }
    std::int64_t a = num >= 0 ? num : -num;
    std::int64_t g = std::gcd(a, den);
    if(g > 1) {
        num /= g;
        den /= g;
    }
}

} // namespace casio


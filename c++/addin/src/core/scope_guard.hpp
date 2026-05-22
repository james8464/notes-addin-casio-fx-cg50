#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace casio
{

inline bool contains_removed_function(std::string const &text)
{
    static constexpr char const *names[] = {
        "comb(", "normald(", "mean(", "median(", "stdev(", "stddev(",
        "correlation(", "covariance(", "linear_regression(",
        "plot(", "plotcontour(", "plotfield(", "plotlist(", "plotode(",
        "plotparam(", "plotpolar(", "plotseq(", "disque(",
        "rationalise(", "rationalize(", "tabular(", "weierstrass(",
        "symmetry(", "mean_value(", "volume_x(", "volume_y(",
        "area_between(", "param_area(", "param_area_y(",
        "param_volume_x(", "param_volume_y(", "ztest(", "spark(",
        "sinh(", "cosh(", "tanh(", "csch(", "sech(", "coth(", "cosech(",
        "asinh(", "acosh(", "atanh(", "acsch(", "asech(", "acoth(", "acosech(",
        "arcsinh(", "arccosh(", "arctanh(", "arcsch(", "arcsech(", "arccoth(", "arcosech(",
        "arsinh(", "arcosh(", "artanh(", "arsch(", "arsech(", "arcoth("
    };
    std::string folded;
    folded.reserve(text.size());
    for(char c : text) {
        if(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '*') continue;
        folded.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return std::any_of(std::begin(names), std::end(names), [&](char const *name) {
        return folded.find(name) != std::string::npos;
    });
}

} // namespace casio

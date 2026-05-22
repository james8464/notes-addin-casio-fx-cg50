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
        "plot(", "plotarea(", "plotfunc(", "plotcontour(", "plotfield(", "plotlist(", "plotode(",
        "plotparam(", "plotpolar(", "plotseq(", "areaplot(", "disque(",
        "rationalise(", "rationalize(", "tabular(", "tabvar(", "weierstrass(",
        "symmetry(", "mean_value(", "volume_x(", "volume_y(",
        "area_between(", "param_area(", "param_area_y(",
        "param_volume_x(", "param_volume_y(", "ztest(", "spark(",
        "bool_simplify(", "prove_bool(", "nand(", "nor(",
        "csolve(", "cfactor(", "cpartfrac(", "complex(", "arg(", "re(", "im(", "conj(",
        "matrix(", "det(", "rref(", "tran(", "transpose(", "idn(", "inv(",
        "cross(", "dot(", "linsolve(", "lu(", "qr(", "matpow(", "trace(",
        "egv(", "egvl(", "eigenvals(", "eigenvalues(", "eigenvects(", "eigenvectors(",
        "jordan(", "svd(", "gramschmidt(", "rank(",
        "evalc(", "real(", "imag(", "conjugate(",
        "polar2rectangular(", "rectangular2polar(", "polar_complex(",
        "laplace(", "ilaplace(", "invlaplace(", "fourier_an(", "fourier_bn(", "fourier_cn(",
        "maclaurin(", "taylor(",
        "exponentiald(", "uniformd(", "ranm(", "ranv(",
        "normald_cdf(", "normald_icdf(", "randnormald(", "randnorm(",
        "quartile1(", "quartile3(", "quartiles(", "variance(", "stddevp(",
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
        std::size_t pos = folded.find(name);
        while(pos != std::string::npos) {
            if(pos == 0) return true;
            unsigned char before = static_cast<unsigned char>(folded[pos - 1]);
            if(!std::isalnum(before) && folded[pos - 1] != '_') return true;
            pos = folded.find(name, pos + 1);
        }
        return false;
    });
}

} // namespace casio

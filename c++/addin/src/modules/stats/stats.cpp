#include "modules/stats/stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>

namespace casio::stats
{
namespace
{

static constexpr long double SQRT2 = 1.414213562373095048801688724209698079L;

static std::string lower(std::string s)
{
    for(char &c : s) {
        if(c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    }
    return s;
}

static std::string trim(std::string s)
{
    std::size_t a = 0;
    while(a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r')) a++;
    std::size_t b = s.size();
    while(b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r')) b--;
    return s.substr(a, b - a);
}

static bool starts_with(std::string const &s, char const *prefix)
{
    std::string p(prefix);
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

static std::string inside_call(std::string s, char const *name)
{
    s = trim(s);
    std::string lo = lower(s);
    std::string p(name);
    if(!starts_with(lo, p.c_str())) return s;
    if(s.size() <= p.size() || s[p.size()] != '(' || s.back() != ')') return s;
    return s.substr(p.size() + 1, s.size() - p.size() - 2);
}

static std::string fmt(long double v)
{
    if(std::isnan((double)v)) return "undefined";
    if(std::isinf((double)v)) return v < 0 ? "-inf" : "inf";
    if(std::fabsl(v) < 5e-13L) v = 0.0L;
    std::ostringstream oss;
    long double av = std::fabsl(v);
    if((av != 0.0L && av < 1e-6L) || av >= 1e9L) {
        oss << std::scientific << std::setprecision(6) << (double)v;
    }
    else {
        oss << std::fixed << std::setprecision(8) << (double)v;
    }
    std::string s = oss.str();
    if(auto e = s.find('e'); e != std::string::npos) {
        std::string a = s.substr(0, e);
        while(!a.empty() && a.back() == '0') a.pop_back();
        if(!a.empty() && a.back() == '.') a.pop_back();
        std::string b = s.substr(e + 1);
        if(!b.empty() && b[0] == '+') b.erase(b.begin());
        while(b.size() > 2 && (b[0] == '-' ? b[1] == '0' : b[0] == '0')) b.erase(b.begin() + (b[0] == '-' ? 1 : 0));
        return a + "e" + b;
    }
    while(!s.empty() && s.back() == '0') s.pop_back();
    if(!s.empty() && s.back() == '.') s.pop_back();
    return s.empty() ? "0" : s;
}

static std::vector<std::string> split_on(std::string const &s, char sep)
{
    std::vector<std::string> out;
    int depth = 0;
    std::string cur;
    for(char c : s) {
        if(c == '(' || c == '[' || c == '{') depth++;
        else if((c == ')' || c == ']' || c == '}') && depth > 0) depth--;
        if(c == sep && depth == 0) {
            out.push_back(trim(cur));
            cur.clear();
        }
        else cur.push_back(c);
    }
    out.push_back(trim(cur));
    return out;
}

static std::vector<long double> parse_numbers(std::string const &text)
{
    std::vector<long double> out;
    std::string compact;
    compact.reserve(text.size());
    for(std::size_t i = 0; i < text.size(); i++) {
        compact.push_back(text[i]);
        if((text[i] == '-' || text[i] == '+') && i + 1 < text.size()) {
            std::size_t j = i + 1;
            while(j < text.size() && (text[j] == ' ' || text[j] == '\t')) j++;
            if(j < text.size() && ((text[j] >= '0' && text[j] <= '9') || text[j] == '.')) i = j - 1;
        }
    }
    char const *s = compact.c_str();
    char *end = nullptr;
    for(std::size_t i = 0; i < compact.size();) {
        if((s[i] == '-' || s[i] == '+') && i + 1 < compact.size()) {
            std::size_t j = i + 1;
            while(j < compact.size() && (s[j] == ' ' || s[j] == '\t')) j++;
            if(j < compact.size() && s[j] == '(') {
                j++;
                while(j < compact.size() && (s[j] == ' ' || s[j] == '\t')) j++;
                if(j < compact.size() && ((s[j] >= '0' && s[j] <= '9') || s[j] == '.')) {
                    long double v = std::strtold(s + j, &end);
                    if(end != s + j) {
                        out.push_back(s[i] == '-' ? -v : v);
                        i = static_cast<std::size_t>(end - s);
                        continue;
                    }
                }
            }
        }
        if((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == '-' || s[i] == '+') {
            long double v = std::strtold(s + i, &end);
            if(end != s + i) {
                out.push_back(v);
                i = static_cast<std::size_t>(end - s);
                continue;
            }
        }
        i++;
    }
    return out;
}

static std::optional<long double> parse_scalar(std::string text)
{
    text = trim(text);
    while(text.size() >= 2 && text.front() == '(' && text.back() == ')') {
        text = trim(text.substr(1, text.size() - 2));
    }
    std::string lo = lower(text);
    if(lo == "inf" || lo == "+inf") return std::numeric_limits<long double>::infinity();
    if(lo == "-inf") return -std::numeric_limits<long double>::infinity();
    auto parts = split_on(text, '/');
    if(parts.size() == 2) {
        auto a = parse_scalar(parts[0]);
        auto b = parse_scalar(parts[1]);
        if(a && b && *b != 0.0L) return *a / *b;
        return std::nullopt;
    }
    char *end = nullptr;
    long double v = std::strtold(text.c_str(), &end);
    while(end && (*end == ' ' || *end == '\t')) end++;
    if(end && *end == '\0') return v;
    return std::nullopt;
}

static std::vector<long double> parse_call_numbers(std::string const &text)
{
    auto args = split_on(text, ',');
    std::vector<long double> out;
    out.reserve(args.size());
    for(auto const &arg : args) {
        std::string lo = lower(arg);
        if(lo == "cdf" || lo == "pmf" || lo == "tail" || lo == "ge" || lo == "gt") continue;
        auto v = parse_scalar(arg);
        if(!v) return parse_numbers(text);
        out.push_back(*v);
    }
    return out;
}

static long double normal_cdf(long double z)
{
    return 0.5L * (1.0L + std::erf((double)(z / SQRT2)));
}

static long double binom_pmf(int n, long double p, int k)
{
    if(k < 0 || k > n) return 0.0L;
    if(p < 0 || p > 1) return std::numeric_limits<long double>::quiet_NaN();
    if(p == 0) return k == 0 ? 1.0L : 0.0L;
    if(p == 1) return k == n ? 1.0L : 0.0L;
    long double logp = std::lgammal(n + 1.0L) - std::lgammal(k + 1.0L) - std::lgammal(n - k + 1.0L);
    logp += k * std::log(p) + (n - k) * std::log1pl(-p);
    if(logp < -745.0L) return 0.0L;
    return std::expl(logp);
}

static long double binom_cdf(int n, long double p, int k)
{
    if(k < 0) return 0.0L;
    if(k >= n) return 1.0L;
    if(n > 5000) {
        long double mu = n * p;
        long double sd = std::sqrt((double)(n * p * (1.0L - p)));
        if(sd == 0) return k >= mu ? 1.0L : 0.0L;
        return normal_cdf((k + 0.5L - mu) / sd);
    }
    long double s = 0.0L;
    for(int i = 0; i <= k; i++) s += binom_pmf(n, p, i);
    return std::min(1.0L, std::max(0.0L, s));
}

static std::vector<std::string> binomial(std::string const &expr)
{
    std::string lo = lower(expr);
    auto nums = parse_call_numbers(expr);
    if(nums.size() < 3) return {"Err: binomial needs n,p,r."};
    int n = (int)std::llround((double)nums[0]);
    long double p = nums[1];
    int r = (int)std::llround((double)nums[2]);
    if(n < 0 || p < 0.0L || p > 1.0L) return {"Err: require n>=0 and 0<=p<=1."};
    bool ge = lo.find(">=") != std::string::npos || lo.find("tail") != std::string::npos || lo.find("ge") != std::string::npos;
    bool gt = !ge && lo.find('>') != std::string::npos;
    bool cdf = ge || gt || lo.find("cdf") != std::string::npos || lo.find("<=") != std::string::npos;
    long double ans = 0.0L;
    std::string event;
    if(!cdf) {
        ans = binom_pmf(n, p, r);
        event = "X = " + std::to_string(r);
    }
    else if(ge || gt) {
        int k0 = gt ? r + 1 : r;
        ans = 1.0L - binom_cdf(n, p, k0 - 1);
        event = gt ? "X > " + std::to_string(r) : "X >= " + std::to_string(r);
    }
    else {
        ans = binom_cdf(n, p, r);
        event = "X <= " + std::to_string(r);
    }
    std::vector<std::string> out;
    long double q = 1.0L - p;
    auto pmf_formula = [&](std::string const &k) {
        return std::to_string(n) + "C" + k + "*" + fmt(p) + "^" + k + "*" + fmt(q) + "^(" + std::to_string(n) + "-" + k + ")";
    };
    out.push_back("X ~ B(" + std::to_string(n) + ", " + fmt(p) + ")");
    if(!cdf && (r < 0 || r > n)) {
        out.push_back(std::to_string(r) + " is outside 0 <= r <= " + std::to_string(n));
        out.push_back("P(" + event + ") = 0");
        return out;
    }
    if(cdf && !ge && !gt && (r < 0 || r >= n)) {
        out.push_back(r < 0 ? "r < 0" : ("r >= " + std::to_string(n)));
        out.push_back("P(" + event + ") = " + fmt(ans));
        return out;
    }
    if((ge || gt) && ((gt && r >= n) || (!gt && r > n) || r < 0)) {
        out.push_back((r < 0) ? "r < 0" : "event outside support");
        out.push_back("P(" + event + ") = " + fmt(ans));
        return out;
    }
    if(!cdf) {
        out.push_back("P(X = r) = nCr*p^r*(1-p)^(n-r)");
        out.push_back("P(X = " + std::to_string(r) + ") = " + pmf_formula(std::to_string(r)));
    }
    else if(ge || gt) {
        int k0 = gt ? r + 1 : r;
        int below = k0 - 1;
        out.push_back("P(" + event + ") = 1 - P(X <= " + std::to_string(below) + ")");
        out.push_back("P(X <= " + std::to_string(below) + ") = sum_{x=0}^" + std::to_string(below) + " " + pmf_formula("x"));
    }
    else {
        out.push_back("P(X <= " + std::to_string(r) + ") = sum_{x=0}^" + std::to_string(r) + " " + pmf_formula("x"));
    }
    if(n > 5000 && cdf) {
        long double mu = n * p;
        long double sd = std::sqrt((double)(n * p * q));
        int k0 = (ge || gt) ? (gt ? r + 1 : r) : r;
        long double cc = (ge || gt) ? (k0 - 0.5L) : (r + 0.5L);
        long double z = sd == 0.0L ? 0.0L : (cc - mu) / sd;
        out.push_back("mu = np = " + fmt(mu) + ", sigma = sqrt(np(1-p)) = " + fmt(sd));
        if(ge || gt) out.push_back("P(" + event + ") ~= 1 - Phi((" + fmt(cc) + " - " + fmt(mu) + ")/" + fmt(sd) + ") = 1 - Phi(" + fmt(z) + ")");
        else out.push_back("P(" + event + ") ~= Phi((" + fmt(cc) + " - " + fmt(mu) + ")/" + fmt(sd) + ") = Phi(" + fmt(z) + ")");
    }
    out.push_back("P(" + event + ") = " + fmt(ans));
    return out;
}

static std::vector<std::string> normal(std::string const &expr)
{
    auto nums = parse_call_numbers(expr);
    if(nums.size() < 4) return {"Err: normalcdf needs mu,sigma,lo,hi."};
    long double mu = nums[0], sigma = nums[1], lo = nums[2], hi = nums[3];
    if(sigma <= 0.0L) return {"Err: sigma must be positive."};
    long double z1 = (lo - mu) / sigma;
    long double z2 = (hi - mu) / sigma;
    long double ans = normal_cdf(z2) - normal_cdf(z1);
    return {
        "X ~ N(" + fmt(mu) + ", " + fmt(sigma) + "^2)",
        "standardise: Z = (X-" + fmt(mu) + ")/" + fmt(sigma) + " ~ N(0,1)",
        "z1 = (" + fmt(lo) + "-" + fmt(mu) + ")/" + fmt(sigma) + " = " + fmt(z1),
        "z2 = (" + fmt(hi) + "-" + fmt(mu) + ")/" + fmt(sigma) + " = " + fmt(z2),
        "P(" + fmt(lo) + " < X < " + fmt(hi) + ") = Phi(" + fmt(z2) + ") - Phi(" + fmt(z1) + ") = " + fmt(ans),
    };
}

static Request autodetect(Request req)
{
    std::string s = trim(req.expr);
    std::string lo = lower(s);
    if(starts_with(lo, "binomcdf(") || starts_with(lo, "binomialcdf(") ||
            starts_with(lo, "binomial_cdf(")) {
        req.mode = 3;
        if(starts_with(lo, "binomcdf(")) req.expr = inside_call(s, "binomcdf") + ",cdf";
        else if(starts_with(lo, "binomialcdf(")) req.expr = inside_call(s, "binomialcdf") + ",cdf";
        else req.expr = inside_call(s, "binomial_cdf") + ",cdf";
    }
    else if(starts_with(lo, "binom(") || starts_with(lo, "binomial(")) {
        req.mode = 3;
        req.expr = inside_call(s, starts_with(lo, "binom(") ? "binom" : "binomial");
    }
    else if(starts_with(lo, "normalcdf(")) {
        req.mode = 4;
        req.expr = inside_call(s, "normalcdf");
    }
    else {
        req.mode = 99;
    }
    return req;
}

} // namespace

std::vector<std::string> run(Arena &arena, Request const &request)
{
    (void)arena;
    Request req = request.mode == 0 ? autodetect(request) : request;
    try {
        switch(req.mode) {
        case 3: return binomial(req.expr);
        case 4: return normal(req.expr);
        case 99: return {"Err: unsupported stats function."};
        default: return {"Err: unknown stats mode."};
        }
    }
    catch(std::exception const &e) {
        return {std::string("Err: ") + e.what()};
    }
}

} // namespace casio::stats

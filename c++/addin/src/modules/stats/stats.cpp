#include "modules/stats/stats.hpp"

#include "core/node.hpp"
#include "core/parse.hpp"

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

static constexpr long double PI = 3.141592653589793238462643383279502884L;
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

static std::string regression_line_text(long double a0, long double b)
{
    auto near = [](long double v, long double t) { return std::fabsl(v - t) < 5e-13L; };
    std::string rhs;
    if(near(b, 0.0L)) rhs = fmt(a0);
    else {
        if(near(b, 1.0L)) rhs = "x";
        else if(near(b, -1.0L)) rhs = "-x";
        else rhs = fmt(b) + "*x";
        if(!near(a0, 0.0L)) {
            if(a0 > 0) rhs += " + " + fmt(a0);
            else rhs += " - " + fmt(-a0);
        }
    }
    return "y = " + rhs;
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

static long double median_sorted(std::vector<long double> const &v, std::size_t lo, std::size_t hi)
{
    if(hi <= lo) return std::numeric_limits<long double>::quiet_NaN();
    std::size_t n = hi - lo;
    if(n % 2) return v[lo + n / 2];
    return (v[lo + n / 2 - 1] + v[lo + n / 2]) / 2.0L;
}

static std::string list_short(std::vector<long double> const &v, std::size_t max_items = 8)
{
    std::ostringstream oss;
    oss << "[";
    for(std::size_t i = 0; i < v.size() && i < max_items; i++) {
        if(i) oss << ", ";
        oss << fmt(v[i]);
    }
    if(v.size() > max_items) oss << ", ...";
    oss << "]";
    return oss.str();
}

static long double sum_of(std::vector<long double> const &v)
{
    long double s = 0;
    for(long double x : v) s += x;
    return s;
}

static std::string sparkline(std::vector<long double> const &v)
{
    if(v.empty()) return "";
    static char const *levels = " .:-=+*#";
    auto [mn_it, mx_it] = std::minmax_element(v.begin(), v.end());
    long double mn = *mn_it;
    long double mx = *mx_it;
    if(mx == mn) return std::string(v.size(), '-');
    std::string out;
    for(long double x : v) {
        int k = (int)std::llround(((x - mn) / (mx - mn)) * 7.0L);
        if(k < 0) k = 0;
        if(k > 7) k = 7;
        out.push_back(levels[k]);
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

static std::optional<long double> eval_node(Arena &a, NodeId id, long double xval)
{
    Node const &n = a.get(id);
    switch(n.kind) {
    case NodeKind::Num:
        return (long double)n.num.num / (long double)n.num.den;
    case NodeKind::Sym:
        return n.text == "x" ? std::optional<long double>(xval) : std::optional<long double>(0.0L);
    case NodeKind::Const:
        return n.ckind == ConstKind::Pi ? PI : std::expl(1.0L);
    case NodeKind::Fn: {
        auto av = eval_node(a, n.a, xval);
        if(!av) return std::nullopt;
        long double u = *av;
        switch(n.fkind) {
        case FnKind::Sin: return std::sin((double)u);
        case FnKind::Cos: return std::cos((double)u);
        case FnKind::Tan: return std::tan((double)u);
        case FnKind::Sec: return 1.0L / std::cos((double)u);
        case FnKind::Cosec: return 1.0L / std::sin((double)u);
        case FnKind::Cot: return 1.0L / std::tan((double)u);
        case FnKind::Asin: return std::asin((double)u);
        case FnKind::Acos: return std::acos((double)u);
        case FnKind::Atan: return std::atan((double)u);
        case FnKind::Exp: return std::exp((double)u);
        case FnKind::Log: return std::log((double)u);
        case FnKind::Log10: return std::log10((double)u);
        case FnKind::Sqrt: return std::sqrt((double)u);
        case FnKind::Abs: return std::fabs((double)u);
        case FnKind::Sign: return (u > 0) - (u < 0);
        default: return std::nullopt;
        }
    }
    case NodeKind::Add: {
        long double s = 0.0L;
        for(NodeId k : n.kids) {
            auto v = eval_node(a, k, xval);
            if(!v) return std::nullopt;
            s += *v;
        }
        return s;
    }
    case NodeKind::Mul: {
        long double p = 1.0L;
        for(NodeId k : n.kids) {
            auto v = eval_node(a, k, xval);
            if(!v) return std::nullopt;
            p *= *v;
        }
        return p;
    }
    case NodeKind::Div: {
        auto u = eval_node(a, n.a, xval);
        auto v = eval_node(a, n.b, xval);
        if(!u || !v || *v == 0.0L) return std::nullopt;
        return *u / *v;
    }
    case NodeKind::Pow: {
        auto u = eval_node(a, n.a, xval);
        auto v = eval_node(a, n.b, xval);
        if(!u || !v) return std::nullopt;
        return std::pow((double)*u, (double)*v);
    }
    }
    return std::nullopt;
}

static std::vector<std::string> one_var(std::string const &expr)
{
    std::vector<std::string> out;
    auto v = parse_numbers(expr);
    if(v.empty()) return {"Err: stats needs a numeric list."};
    std::sort(v.begin(), v.end());
    long double n = (long double)v.size();
    long double sx = sum_of(v);
    long double mean = sx / n;
    long double sxx = 0.0L;
    for(long double x : v) sxx += (x - mean) * (x - mean);
    long double pop_var = sxx / n;
    long double sample_var = v.size() > 1 ? sxx / (n - 1.0L) : 0.0L;
    std::size_t mid = v.size() / 2;
    long double med = median_sorted(v, 0, v.size());
    long double q1 = median_sorted(v, 0, mid);
    long double q3 = median_sorted(v, v.size() % 2 ? mid + 1 : mid, v.size());

    out.push_back("1. Sort data: " + list_short(v));
    out.push_back("2. n = " + std::to_string(v.size()) + ", sum x = " + fmt(sx));
    out.push_back("3. mean = sum x/n = " + fmt(mean));
    out.push_back("4. min = " + fmt(v.front()) + ", Q1 = " + fmt(q1) + ", median = " + fmt(med) + ", Q3 = " + fmt(q3) + ", max = " + fmt(v.back()));
    out.push_back("5. Sxx = sum((x-mean)^2) = " + fmt(sxx));
    out.push_back("6. var(pop) = " + fmt(pop_var) + ", var(sample) = " + fmt(sample_var));
    out.push_back("Answer: mean=" + fmt(mean) + ", sd=" + fmt(std::sqrt((double)pop_var)) + ", s=" + fmt(std::sqrt((double)sample_var)));
    return out;
}

static std::vector<std::string> two_var(std::string const &expr, std::string const &expr2)
{
    std::string xtext = expr;
    std::string ytext = expr2;
    if(ytext.empty()) {
        auto parts = split_on(expr, ';');
        if(parts.size() < 2) parts = split_on(expr, '|');
        if(parts.size() < 2) parts = split_on(expr, '\n');
        if(parts.size() < 2) parts = split_on(expr, ',');
        if(parts.size() >= 2) {
            xtext = parts[0];
            ytext = parts[1];
        }
    }
    auto x = parse_numbers(xtext);
    auto y = parse_numbers(ytext);
    if(x.empty() || y.empty()) return {"1. Enter paired x and y data lists.", "Answer: no covariance: missing data."};
    if(x.size() != y.size()) {
        return {
            "1. Pair each x value with the matching y value.",
            "2. List lengths must match before covariance/regression is defined.",
            "Answer: no covariance: list lengths differ.",
        };
    }
    long double n = (long double)x.size();
    long double xb = sum_of(x) / n;
    long double yb = sum_of(y) / n;
    long double sxx = 0.0L, syy = 0.0L, sxy = 0.0L;
    for(std::size_t i = 0; i < x.size(); i++) {
        long double dx = x[i] - xb;
        long double dy = y[i] - yb;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
    }
    if(sxx == 0.0L || syy == 0.0L) return {"Err: regression undefined because one variable is constant."};
    long double b = sxy / sxx;
    long double a0 = yb - b * xb;
    long double r = sxy / std::sqrt((double)(sxx * syy));
    return {
        "1. xbar = " + fmt(xb) + ", ybar = " + fmt(yb),
        "2. Sxx = " + fmt(sxx) + ", Syy = " + fmt(syy) + ", Sxy = " + fmt(sxy),
        "3. b = Sxy/Sxx = " + fmt(b),
        "4. a = ybar - b*xbar = " + fmt(a0),
        "5. r = Sxy/sqrt(Sxx*Syy) = " + fmt(r),
        "Answer: " + regression_line_text(a0, b) + ", r = " + fmt(r) + ", r^2 = " + fmt(r * r),
    };
}

static std::vector<std::string> binomial(std::string const &expr)
{
    std::string lo = lower(expr);
    auto nums = parse_numbers(expr);
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
    out.push_back("X ~ B(" + std::to_string(n) + ", " + fmt(p) + ")");
    if(!cdf) out.push_back("P(X = r) = nCr*p^r*(1-p)^(n-r)");
    else if(ge || gt) out.push_back("P(" + event + ") = 1 - P(X < " + std::to_string(ge ? r : r + 1) + ")");
    else out.push_back("P(X <= r) = sum_{x=0}^r nCx*p^x*(1-p)^(n-x)");
    if(n > 5000 && cdf) out.push_back("normal approx with continuity correction");
    out.push_back("P(" + event + ") = " + fmt(ans));
    return out;
}

static std::vector<std::string> normal(std::string const &expr)
{
    auto nums = parse_numbers(expr);
    if(nums.size() < 4) return {"Err: normalcdf needs mu,sigma,lo,hi."};
    long double mu = nums[0], sigma = nums[1], lo = nums[2], hi = nums[3];
    if(sigma <= 0.0L) return {"Err: sigma must be positive."};
    long double z1 = (lo - mu) / sigma;
    long double z2 = (hi - mu) / sigma;
    long double ans = normal_cdf(z2) - normal_cdf(z1);
    return {
        "X ~ N(" + fmt(mu) + ", " + fmt(sigma) + "^2)",
        "z1 = " + fmt(z1) + ", z2 = " + fmt(z2),
        "P(" + fmt(lo) + " < X < " + fmt(hi) + ") = Phi(z2) - Phi(z1)",
        "P(" + fmt(lo) + " < X < " + fmt(hi) + ") = " + fmt(ans),
    };
}

static std::vector<std::string> ztest(std::string const &expr)
{
    auto nums = parse_numbers(expr);
    if(nums.size() < 4) return {"Err: ztest needs xbar,mu,sigma,n[,alpha]."};
    std::string lo = lower(expr);
    long double xbar = nums[0], mu = nums[1], sigma = nums[2], n = nums[3];
    long double alpha = nums.size() >= 5 ? nums[4] : 0.05L;
    if(sigma <= 0.0L || n <= 0.0L) return {"Err: require sigma>0 and n>0."};
    long double z = (xbar - mu) / (sigma / std::sqrt((double)n));
    long double pval = 0.0L;
    std::string tail = "two tail";
    if(lo.find("gt") != std::string::npos || lo.find('>') != std::string::npos) {
        pval = 1.0L - normal_cdf(z);
        tail = "right tail";
    }
    else if(lo.find("lt") != std::string::npos || lo.find('<') != std::string::npos) {
        pval = normal_cdf(z);
        tail = "left tail";
    }
    else {
        pval = 2.0L * std::min(normal_cdf(z), 1.0L - normal_cdf(z));
    }
    return {
        "H0: mu = " + fmt(mu),
        "z = (xbar-mu)/(sigma/sqrt(n)) = " + fmt(z),
        tail + " p = " + fmt(pval),
        std::string(pval < alpha ? "reject H0" : "do not reject H0") + " at alpha = " + fmt(alpha),
    };
}

static std::vector<std::string> plot(Arena &arena, std::string const &expr)
{
    auto parts = split_on(expr, ',');
    if(parts.size() < 4) return {"Err: plot needs expr,xmin,xmax,n."};
    std::string f = parts[0];
    auto xmin_vals = parse_numbers(parts[1]);
    auto xmax_vals = parse_numbers(parts[2]);
    auto n_vals = parse_numbers(parts[3]);
    if(xmin_vals.empty() || xmax_vals.empty() || n_vals.empty()) return {"Err: plot bounds must be numeric."};
    long double xmin = xmin_vals[0];
    long double xmax = xmax_vals[0];
    int n = (int)std::llround((double)n_vals[0]);
    if(n < 2) n = 2;
    if(n > 80) n = 80;
    NodeId root = parse_expr(arena, f);
    std::vector<long double> xs, ys;
    std::vector<std::string> roots;
    std::vector<long double> root_vals;
    long double root_tol = std::max((xmax - xmin) / std::max(1, n - 1) * 0.25L, 1e-7L);
    auto add_root = [&](long double xr) {
        for(long double old : root_vals)
            if(std::fabsl(old - xr) <= root_tol) return;
        if(roots.size() < 6) {
            root_vals.push_back(xr);
            roots.push_back(fmt(xr));
        }
    };
    std::optional<long double> prev_y;
    long double prev_x = xmin;
    for(int i = 0; i < n; i++) {
        long double x = xmin + (xmax - xmin) * i / (n - 1);
        auto y = eval_node(arena, root, x);
        if(!y || !std::isfinite((double)*y)) {
            prev_y.reset();
            continue;
        }
        if(prev_y && ((*prev_y <= 0 && *y >= 0) || (*prev_y >= 0 && *y <= 0))) {
            long double xr = (*prev_y == *y) ? x : prev_x + (0.0L - *prev_y) * (x - prev_x) / (*y - *prev_y);
            add_root(xr);
        }
        xs.push_back(x);
        ys.push_back(*y);
        prev_x = x;
        prev_y = *y;
    }
    if(ys.empty()) return {"Err: plot produced no finite points."};
    auto [mn_it, mx_it] = std::minmax_element(ys.begin(), ys.end());
    std::string root_text = roots.empty() ? "none detected" : list_short(parse_numbers(roots[0]), 0);
    if(!roots.empty()) {
        root_text = roots[0];
        for(std::size_t i = 1; i < roots.size(); i++) root_text += ", " + roots[i];
    }
    return {
        "x in [" + fmt(xmin) + ", " + fmt(xmax) + "], n = " + std::to_string(n),
        "y-range = [" + fmt(*mn_it) + ", " + fmt(*mx_it) + "]",
        "spark = " + sparkline(ys),
        "x-intercepts near " + root_text,
    };
}

static Request autodetect(Request req)
{
    std::string s = trim(req.expr);
    std::string lo = lower(s);
    if(starts_with(lo, "stats(")) {
        req.mode = 1;
        req.expr = inside_call(s, "stats");
    }
    else if(starts_with(lo, "corr(") || starts_with(lo, "correlation(") ||
            starts_with(lo, "covariance(") || starts_with(lo, "cov(") || starts_with(lo, "reg(")) {
        req.mode = 2;
        if(starts_with(lo, "correlation(")) req.expr = inside_call(s, "correlation");
        else if(starts_with(lo, "covariance(")) req.expr = inside_call(s, "covariance");
        else if(starts_with(lo, "cov(")) req.expr = inside_call(s, "cov");
        else req.expr = inside_call(s, starts_with(lo, "corr(") ? "corr" : "reg");
    }
    else if(starts_with(lo, "binomcdf(") || starts_with(lo, "binomialcdf(") ||
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
    else if(starts_with(lo, "ztest(")) {
        req.mode = 5;
        req.expr = inside_call(s, "ztest");
    }
    else if(starts_with(lo, "spark(")) {
        req.mode = 6;
        req.expr = inside_call(s, "spark");
    }
    else if(starts_with(lo, "plot(")) {
        req.mode = 7;
        req.expr = inside_call(s, "plot");
    }
    else {
        req.mode = 1;
    }
    return req;
}

} // namespace

std::vector<std::string> run(Arena &arena, Request const &request)
{
    Request req = request.mode == 0 ? autodetect(request) : request;
    try {
        switch(req.mode) {
        case 1: return one_var(req.expr);
        case 2: return two_var(req.expr, req.expr2);
        case 3: return binomial(req.expr);
        case 4: return normal(req.expr);
        case 5: return ztest(req.expr);
        case 6: {
            auto v = parse_numbers(req.expr);
            if(v.empty()) return {"Err: spark needs a numeric list."};
            return {"y-range sampled from data", "spark = " + sparkline(v), sparkline(v)};
        }
        case 7: return plot(arena, req.expr);
        default: return {"Err: unknown stats mode."};
        }
    }
    catch(std::exception const &e) {
        return {std::string("Err: ") + e.what()};
    }
}

} // namespace casio::stats

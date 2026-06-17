#include "../../../khicas/upstream/giac90_1addin/cascas_working.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string trim(std::string s)
{
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static std::string lower_ascii(std::string s)
{
    for(char &c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool starts_with(std::string const &s, char const *prefix)
{
    std::string p(prefix);
    return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
}

static std::vector<std::string> split_top_csv(std::string const &s)
{
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    for(char c : s) {
        if(c == '(' || c == '[' || c == '{') ++depth;
        else if(c == ')' || c == ']' || c == '}') --depth;
        if(c == ',' && depth == 0) {
            out.push_back(trim(cur));
            cur.clear();
        }
        else cur.push_back(c);
    }
    if(!cur.empty() || !out.empty()) out.push_back(trim(cur));
    return out;
}

static bool has_word(std::string const &text, std::string const &needle)
{
    for(std::size_t pos = text.find(needle); pos != std::string::npos; pos = text.find(needle, pos + 1)) {
        bool left = pos == 0 || !(std::isalnum(static_cast<unsigned char>(text[pos - 1])) || text[pos - 1] == '_');
        std::size_t end = pos + needle.size();
        bool right = end >= text.size() || !(std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_');
        if(left && right) return true;
    }
    return false;
}

static bool has_call(std::string const &text, std::string const &name)
{
    for(std::size_t pos = text.find(name); pos != std::string::npos; pos = text.find(name, pos + 1)) {
        bool left = pos == 0 || !(std::isalnum(static_cast<unsigned char>(text[pos - 1])) || text[pos - 1] == '_');
        std::size_t i = pos + name.size();
        while(i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if(left && i < text.size() && text[i] == '(') return true;
    }
    return false;
}

static std::string unwrap_call(std::string const &text, std::string const &name)
{
    std::string t = trim(text);
    if(t.size() <= name.size() + 1) return "";
    if(t.compare(0, name.size(), name) != 0) return "";
    std::size_t i = name.size();
    while(i < t.size() && std::isspace(static_cast<unsigned char>(t[i]))) ++i;
    if(i >= t.size() || t[i] != '(' || t.back() != ')') return "";
    return t.substr(i + 1, t.size() - i - 2);
}

static bool removed_feature(std::string const &expr)
{
    std::string s = lower_ascii(expr);
    std::string inner = unwrap_call(s, "binomial");
    if(!inner.empty() && split_top_csv(inner).size() != 4) return true;

    static char const *const calls[] = {
        "normalcdf","binomcdf","binompdf","mean","median","stddev","variance","det",
        "plot","plotlist","plotparam","plotpolar","plotseq","paramplot","seqplot",
        "funcplot","plotdensity","plotmatrix","densityplot","graphe","polarplot",
        "courbe_polaire","plotimplicit","implicitplot","conic","quadric",
        "reduced_conic","reduced_quadric","courbe_parametrique","graphe_suite","rand","randint","randperm","ranv","ranm",
        "irem","smod","permuorder","concat","extend","select","filter","remove",
        "csolve","cfactor","cpartfrac","abcuv","iabcuv","egcd","iegcd","interp",
        "jordan","gramschmidt","hermite","laguerre","legendre","tchebyshev1",
        "tchebyshev2","residue","resultant","sinh","cosh","tanh","asinh","acosh",
        "atanh","python","numpy","pylab","matplotlib","set_pixel","draw_pixel",
        "matrix","sign","comb","perm","binomial_cdf","binomial_icdf","normald",
        "normal_cdf","normal_icdf","normald_cdf","normald_icdf","uniformd",
        "uniform_cdf","uniform_icdf","uniformd_cdf","uniformd_icdf","poisson",
        "poisson_cdf","poisson_icdf","randbinomial","randpoisson","randnormald",
        "normalvariate","quartile1","quartile3","quartiles","moustache","histogram",
        "camembert","covariance","correlation","stddevp","laplace","ilaplace","fourier",
        "fourier_an","fourier_bn","fourier_cn","odesolve","desolve","plotode",
        "plotfield","fft","ifft","read","write","charpoly","pcar","gaussjord",
        "pivot","assume","asc","char","euler","barplot","bar_plot","scatterplot",
        "listplot","polygonplot","polygonscatterplot","halftan","halftan_hyp2exp",
        "exp2trig","trig2exp","evalc","mult_c_conjugate","q2a","a2q","turtle",
        "denom","numer","comdenom","getdenom","curl","ichinrem","iquorem","conj",
        "re","im","plotarea","plotfunc","plotcontour","point","point2d","point3d",
        "polar_point","line","half_line","line_inter","single_inter","segment",
        "circle","bitmap","bezier","animation","hyperplan","hypersphere",
        "hypersurface","frame_3d","plot3d","graphe3d","plotinequation",
        "inequationplot","envelope","locus","linetan","tabvar","draw_arc","draw_circle","draw_line",
        "draw_polygon","draw_rectangle","python_compat","complex","jordanblock",
        "trace","transpose","cholesky","about","cone","cube","cylinder",
        "dodecahedron","icosahedron","octahedron","plane","sphere","tetrahedron",
        "volume","program","bloc","ifte","for","while","local","return",
        "try_catch","throw","break","continue","inputform","choosebox","output",
        "input","dialog","pause","sleep","execute","exponentiald","exponential",
        "exponential_cdf","exponential_icdf","betad","cauchyd","cauchy",
        "cauchy_cdf","cauchy_icdf","gammad","geometric","weibulld","weibull",
        "weibull_cdf","weibull_icdf","student","student_cdf","student_icdf",
        "chisquare","chisquare_cdf","chisquare_icdf","fisher","fisher_cdf",
        "fisher_icdf","snedecor","snedecor_cdf","snedecor_icdf","uniform",
        "cdf","plotcdf","kolmogorovd","kolmogorovt","wilcoxons","wilcoxonp",
        "wilcoxont","chisquaret","normalt","studentt","multinomial",
        "randmultinomial","randchisquare","randstudent","randfisher","randexp",
        "randgammad","randbetad","randweibulld","randgeometric","randvector",
        "randmatrix","random","random_variable","randvar","hasard","srand",
        "randpoly","egv","egvl","eigenvals","eigenvalues","eigenvects",
        "eigenvectors","svd","ker","kernel","rank","rat_jordan",
        "rat_jordan_block","pcar_hessenberg","det_minor","basis","image",
        "rref","ref","pulley","force"
    };
    for(char const *name : calls) {
        if(has_call(s, name)) return true;
    }

    static char const *const words[] = {
        "mod","gl_x","gl_y","crypto","keep_pivot","minor_det","rational_det"
    };
    for(char const *name : words) {
        if(has_word(s, name)) return true;
    }
    return false;
}

static bool numeric_literal(std::string const &expr)
{
    std::string s = trim(expr);
    if(s.empty()) return false;
    bool digit = false;
    bool dot = false;
    std::size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if(i >= s.size()) return false;
    for(; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if(std::isdigit(c)) {
            digit = true;
            continue;
        }
        if(s[i] == '.' && !dot) {
            dot = true;
            continue;
        }
        return false;
    }
    return digit;
}

static char const *pure_method_fallback(bool removed)
{
    return removed
        ? "Pure method fallback:\n"
          "unsupported built-in removed from this Pure build.\n"
          "1. Rewrite the question using algebra, trig, logs or calculus.\n"
          "2. Try solve, diff, integrate, range, domain, xform, texpand or factor.\n"
          "3. For stats, matrices, plotting, scripts or mechanics, use the calculator app for that topic.\n"
        : "General Pure method:\n"
          "1. Identify the target: simplify, solve, differentiate, integrate, prove, range or domain.\n"
          "2. Rewrite with standard A-level identities and restrictions first.\n"
          "3. Then use solve, diff, integrate, range, domain, xform, texpand or factor on the clean sub-step.\n";
}

static std::string stdin_text()
{
    std::ostringstream out;
    out << std::cin.rdbuf();
    return out.str();
}

static std::string shell_quote(std::string const &s)
{
    std::string out = "'";
    for(char c : s) {
        if(c == '\'') out += "'\\''";
        else out.push_back(c);
    }
    out += "'";
    return out;
}

static bool host_exact_eval(char const *expr, cascas::working_string &out)
{
    if(!std::getenv("CASCAS_HOST_PRODUCTION")) return false;
    std::string cmd = "python3 tools/host/khicas_host_eval.py " + shell_quote(expr ? expr : "");
    FILE *pipe = popen(cmd.c_str(), "r");
    if(!pipe) return false;
    char buf[512];
    std::string text;
    while(fgets(buf, sizeof(buf), pipe)) text += buf;
    int status = pclose(pipe);
    if(status != 0) return false;
    while(!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    if(text.empty()) return false;
    out = text;
    return true;
}

int main(int argc, char **argv)
{
    cascas::set_khicas_eval_callback(host_exact_eval);

    if(argc < 2) {
        std::cerr << "usage: casio_host [--alg|--derive|--int|--trig|--stats] EXPR\n";
        return 2;
    }

    std::string flag = argv[1];
    if(flag == "--stdin-program") {
        (void)stdin_text();
        std::cout << pure_method_fallback(true);
        return 0;
    }
    if(flag == "--stats" || flag == "--bool" || flag == "--nand" || flag == "--nor") {
        std::cout << pure_method_fallback(true);
        return 0;
    }

    bool flagged = starts_with(flag, "--");
    std::string expr = flagged ? (argc >= 3 ? argv[2] : "") : flag;

    if(removed_feature(expr)) {
        std::cout << pure_method_fallback(true);
        return 0;
    }
    if(numeric_literal(expr)) {
        std::cout << trim(expr) << "\n";
        return 0;
    }

    cascas::working_string out;
    if(cascas::eval_with_working(expr.c_str(), out)) {
        std::cout << out;
        if(out.empty() || out[out.size() - 1] != '\n') std::cout << "\n";
        return 0;
    }
    if(host_exact_eval(expr.c_str(), out)) {
        std::cout << out;
        if(out.empty() || out[out.size() - 1] != '\n') std::cout << "\n";
        return 0;
    }

    std::cout << pure_method_fallback(false);
    return 0;
}

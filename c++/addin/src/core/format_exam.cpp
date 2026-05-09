#include "format_exam.hpp"

#include "simplify.hpp"

#include <cctype>
#include <sstream>

namespace casio
{
namespace
{

static std::string num_text(Rational const &q)
{
    if(q.den == 1) return std::to_string(q.num);
    return "(" + std::to_string(q.num) + "/" + std::to_string(q.den) + ")";
}

static std::string fn_text(FnKind k)
{
    switch(k) {
    case FnKind::Sin: return "sin";
    case FnKind::Cos: return "cos";
    case FnKind::Tan: return "tan";
    case FnKind::Sec: return "sec";
    case FnKind::Cosec: return "cosec";
    case FnKind::Cot: return "cot";
    case FnKind::Asin: return "asin";
    case FnKind::Acos: return "acos";
    case FnKind::Atan: return "atan";
    case FnKind::Asinh: return "asinh";
    case FnKind::Acosh: return "acosh";
    case FnKind::Atanh: return "atanh";
    case FnKind::Sinh: return "sinh";
    case FnKind::Cosh: return "cosh";
    case FnKind::Tanh: return "tanh";
    case FnKind::Exp: return "exp";
    case FnKind::Log: return "log";
    case FnKind::Log10: return "log10";
    case FnKind::Sqrt: return "sqrt";
    case FnKind::Abs: return "abs";
    case FnKind::Sign: return "sign";
    case FnKind::Factorial: return "factorial";
    }
    return "fn";
}

static constexpr int FORMAT_EXAM_MAX_DEPTH = 48;
static constexpr int FORMAT_EXAM_MAX_LINES = 44;
static constexpr int FORMAT_EXAM_MAX_CHARS = 3600;
static constexpr int FORMAT_EXAM_MAX_LINE_CHARS = 420;

static std::string truncate_exam_line(std::string s)
{
    if(static_cast<int>(s.size()) <= FORMAT_EXAM_MAX_LINE_CHARS) return s;
    if(FORMAT_EXAM_MAX_LINE_CHARS <= 3) return "...";
    s.resize(FORMAT_EXAM_MAX_LINE_CHARS - 3);
    s += "...";
    return s;
}

static std::string trim_copy(std::string s)
{
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static bool starts_with(std::string const &s, char const *prefix)
{
    std::string p(prefix);
    return s.rfind(p, 0) == 0;
}

static void replace_all(std::string &s, char const *from, char const *to)
{
    std::string f(from), t(to);
    if(f.empty()) return;
    std::size_t pos = 0;
    while((pos = s.find(f, pos)) != std::string::npos) {
        s.replace(pos, f.size(), t);
        pos += t.size();
    }
}

static void space_plain_equals(std::string &s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for(std::size_t i = 0; i < s.size(); ++i) {
        if(s[i] != '=') {
            out.push_back(s[i]);
            continue;
        }
        char prev = '\0';
        for(std::size_t j = i; j > 0; --j) {
            if(!std::isspace(static_cast<unsigned char>(s[j - 1]))) {
                prev = s[j - 1];
                break;
            }
        }
        char next = '\0';
        for(std::size_t j = i + 1; j < s.size(); ++j) {
            if(!std::isspace(static_cast<unsigned char>(s[j]))) {
                next = s[j];
                break;
            }
        }
        if(prev == '<' || prev == '>' || prev == '!' || prev == '=' ||
           prev == '~' || next == '=' || next == '>') {
            out.push_back('=');
            continue;
        }
        while(!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) out.pop_back();
        out += " = ";
        while(i + 1 < s.size() && std::isspace(static_cast<unsigned char>(s[i + 1]))) ++i;
    }
    s.swap(out);
}

static void prefer_ln_for_natural_logs(std::string &s)
{
    std::size_t pos = 0;
    while((pos = s.find("log(", pos)) != std::string::npos) {
        int depth = 0;
        bool comma = false;
        std::size_t close = std::string::npos;
        for(std::size_t i = pos + 3; i < s.size(); ++i) {
            if(s[i] == '(') ++depth;
            else if(s[i] == ')') {
                --depth;
                if(depth == 0) {
                    close = i;
                    break;
                }
            }
            else if(s[i] == ',' && depth == 1) comma = true;
        }
        if(close != std::string::npos && !comma) s.replace(pos, 3, "ln");
        pos = close == std::string::npos ? pos + 4 : close + 1;
    }
}

static std::string strip_step_prefix(std::string s)
{
    if(!starts_with(s, "Step ")) return s;
    std::size_t colon = s.find(':');
    if(colon == std::string::npos || colon > 8) return s;
    return trim_copy(s.substr(colon + 1));
}

static std::string strip_leading_word(std::string s, char const *word)
{
    if(starts_with(s, word)) return trim_copy(s.substr(std::string(word).size()));
    return s;
}

static bool keep_short_exam_label(std::string const &s)
{
    return starts_with(s, "Domain:") || starts_with(s, "Valid:") || starts_with(s, "Err:") ||
           starts_with(s, "Range:") || starts_with(s, "Equivalent") || starts_with(s, "equivalent") ||
           starts_with(s, "Not equivalent") || starts_with(s, "not equivalent") ||
           starts_with(s, "all real") || starts_with(s, "check:") || starts_with(s, "reject ") ||
           starts_with(s, "PF:") || starts_with(s, "D:") || starts_with(s, "I:") ||
           starts_with(s, "Signs:") || starts_with(s, "No elementary");
}

static bool has_math_signal(std::string const &s)
{
    bool digit = false, alpha = false;
    for(char c : s) {
        if(std::isdigit(static_cast<unsigned char>(c))) digit = true;
        if(std::isalpha(static_cast<unsigned char>(c))) alpha = true;
    }
    if(digit && !alpha) return true;
    if(s.find('=') != std::string::npos || s.find('<') != std::string::npos ||
       s.find('>') != std::string::npos || s.find('^') != std::string::npos ||
       s.find('/') != std::string::npos || s.find('*') != std::string::npos ||
       s.find('+') != std::string::npos || s.find('-') != std::string::npos ||
       s.find('~') != std::string::npos || s.find("Int") != std::string::npos ||
       s.find("dx") != std::string::npos || s.find("du") != std::string::npos ||
       s.find("dy") != std::string::npos || s.find("ln(") != std::string::npos ||
       s.find("sin(") != std::string::npos || s.find("cos(") != std::string::npos ||
       s.find("tan(") != std::string::npos || s.find("sqrt(") != std::string::npos)
        return true;
    return false;
}

static std::pair<bool, Rational> split_coeff(Arena &arena, NodeId node, NodeId &rest_out)
{
    Node const &n = arena.get(node);
    if(n.kind == NodeKind::Mul && !n.kids.empty()) {
        Node const &k0 = arena.get(n.kids.front());
        if(k0.kind == NodeKind::Num) {
            Rational coeff = k0.num;
            if(n.kids.size() == 1) {
                rest_out = sym(arena, "1");
            }
            else {
                // Build rest as mul(kids[1:]) (alloc ok; used only for printing).
                std::vector<NodeId> rest;
                rest.reserve(n.kids.size() - 1);
                for(std::size_t i = 1; i < n.kids.size(); i++) rest.push_back(n.kids[i]);
                rest_out = mul(arena, std::move(rest));
            }
            return {true, coeff};
        }
    }
    rest_out = node;
    return {false, Rational::from_int(1)};
}

} // namespace

std::string math_style_line(std::string line)
{
    line = trim_copy(line);
    if(line.empty()) return line;

    std::size_t i = 0;
    while(i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) ++i;
    if(i > 0 && i + 1 < line.size() && line[i] == '.' && line[i + 1] == ' ') {
        line = trim_copy(line.substr(i + 2));
    }

    line = strip_step_prefix(line);

    if(starts_with(line, "Answer:")) {
        line = trim_copy(line.substr(7));
    }
    else if(starts_with(line, "Result:")) {
        line = trim_copy(line.substr(7));
    }

    replace_all(line, " → ", " => ");
    replace_all(line, "≤", "<=");
    replace_all(line, "≥", ">=");
    replace_all(line, "π", "pi");
    replace_all(line, "∫", "Int");
    replace_all(line, "Integral [", "Int(");
    replace_all(line, "Integral(", "Int(");
    replace_all(line, "Integral ", "Int ");
    replace_all(line, "Integral acos", "Int(acos");
    replace_all(line, "Integral asin", "Int(asin");
    replace_all(line, "Integral atan", "Int(atan");
    replace_all(line, "] dx", ") dx");
    replace_all(line, " integral ", " Int ");
    replace_all(line, ", so ", "; ");
    replace_all(line, " so ", "; ");
    replace_all(line, ", then ", "; ");
    replace_all(line, " then ", "; ");
    replace_all(line, " and ", "; ");

    if(starts_with(line, "Use integration by parts")) line = "I = u*v - Int(v du).";
    else if(starts_with(line, "Apply integration by parts")) line = "I = u*v - Int(v du).";
    else if(starts_with(line, "Use parts:")) line = trim_copy(line.substr(10));
    else if(starts_with(line, "Use inverse function")) line.clear();
    else if(starts_with(line, "Use partial fractions first")) line.clear();
    else if(starts_with(line, "Parts:")) line = trim_copy(line.substr(6));
    else if(starts_with(line, "Use identity ")) line = trim_copy(line.substr(13));
    else if(starts_with(line, "Use identities:")) line = trim_copy(line.substr(15));
    else if(starts_with(line, "Use ")) line = trim_copy(line.substr(4));
    else if(starts_with(line, "Start with ")) line = trim_copy(line.substr(11));
    else if(starts_with(line, "Write ")) line = trim_copy(line.substr(6));
    else if(starts_with(line, "Let ")) line = trim_copy(line.substr(4));
    else if(starts_with(line, "Then ")) line = trim_copy(line.substr(5));
    else if(starts_with(line, "Hence ")) line = trim_copy(line.substr(6));
    else if(starts_with(line, "Therefore ")) line = trim_copy(line.substr(10));
    else if(starts_with(line, "So ")) line = trim_copy(line.substr(3));
    else if(starts_with(line, "Back-substitute;")) line = "x: " + trim_copy(line.substr(16));
    else if(starts_with(line, "Back-substitute ")) line = "x: " + trim_copy(line.substr(16));
    else if(starts_with(line, "Differentiate both sides")) line = "d/dx(LHS)=d/dx(RHS).";
    else if(line == "implicit differentiation." || line == "implicit differentiation") line = "d/dx(LHS)=d/dx(RHS).";
    else if(starts_with(line, "Rewrite as ")) line = trim_copy(line.substr(11));
    else if(starts_with(line, "Rewrite ")) line = trim_copy(line.substr(8));
    else if(starts_with(line, "Use implicit differentiation")) line = "d/dx(LHS)=d/dx(RHS).";
    else if(starts_with(line, "Use product/chain rules") || starts_with(line, "product/chain rules")) line = "dy/dx terms -> one side.";
    else if(starts_with(line, "Factor dy/dx")) line = "dy/dx = RHS/coeff(dy/dx).";
    else if(starts_with(line, "Clear denominators")) line = "clear denoms.";
    else if(starts_with(line, "Collect ")) line = strip_leading_word(line, "Collect ");
    else if(starts_with(line, "Multiply through")) line = strip_leading_word(line, "Multiply through");
    else if(starts_with(line, "Equate coefficients")) line = "coeffs: LHS=RHS.";
    else if(starts_with(line, "Move all terms:")) line = trim_copy(line.substr(15));
    else if(starts_with(line, "Factor:")) line = trim_copy(line.substr(7));
    else if(starts_with(line, "Factored form:")) line = trim_copy(line.substr(14));
    else if(starts_with(line, "quadratic formula:")) line = trim_copy(line.substr(18));
    else if(starts_with(line, "Complete the square")) line = strip_leading_word(line, "Complete the square");
    else if(starts_with(line, "Substitute ")) line = strip_leading_word(line, "Substitute ");
    else if(starts_with(line, "Set ")) line = trim_copy(line.substr(4));
    else if(starts_with(line, "Using ")) line = trim_copy(line.substr(6));
    else if(starts_with(line, "Split: ")) line = trim_copy(line.substr(7));
    else if(starts_with(line, "Split into ")) line = trim_copy(line.substr(11));
    else if(starts_with(line, "For the first part, let ")) line = trim_copy(line.substr(24));
    else if(starts_with(line, "For first part, let ")) line = trim_copy(line.substr(20));
    else if(starts_with(line, "For the second part, ")) line = trim_copy(line.substr(21));
    else if(starts_with(line, "Second part: ")) line = trim_copy(line.substr(13));
    else if(starts_with(line, "Put over ")) line.clear();
    else if(starts_with(line, "Rearrange ")) line = trim_copy(line.substr(10));
    else if(starts_with(line, "Multiply top and bottom by ")) line = trim_copy(line.substr(27));
    else if(starts_with(line, "Compare remaining coefficients: ")) line = "coeffs: " + trim_copy(line.substr(32));
    else if(starts_with(line, "Solve coefficients")) line.clear();
    else if(starts_with(line, "Complete squares: ")) line = trim_copy(line.substr(18));
    else if(starts_with(line, "Integrate and simplify")) line.clear();
    else if(starts_with(line, "Classify the integrand")) line.clear();
    else if(starts_with(line, "Try substitution")) line = "u=f(x), du=f'(x) dx.";
    else if(starts_with(line, "Test reverse chain")) line = "u=f(x), du=f'(x) dx.";
    else if(starts_with(line, "If a product remains")) line = "I = u*v - Int(v du) or DI table.";
    else if(starts_with(line, "Identify reverse-chain power rule")) line.clear();
    else if(starts_with(line, "Divide by inner derivative")) line.clear();
    else if(starts_with(line, "Factor out constant")) line.clear();
    else if(starts_with(line, "Multiply the primitive by the constant")) line.clear();
    else if(starts_with(line, "Split the Int over the sum")) line.clear();
    else if(starts_with(line, "Integrate the remaining power")) line.clear();
    else if(starts_with(line, "Use atan standard result")) line.clear();
    else if(starts_with(line, "Integrate by the chain rule")) line.clear();
    else if(starts_with(line, "Rearrange using identities/substitution")) line.clear();
    else if(starts_with(line, "Rewrite using identities")) line.clear();
    else if(starts_with(line, "using exact trig/algebra rules")) line.clear();
    else if(starts_with(line, "Write using:")) line.clear();
    else if(starts_with(line, "Expand using binomial expansion")) line.clear();
    else if(starts_with(line, "Expand brackets/powers")) line.clear();
    else if(starts_with(line, "powers of x")) line.clear();
    else if(starts_with(line, "No brackets/powers need expanding")) line.clear();
    else if(starts_with(line, "Expression is already expanded")) line.clear();
    else if(starts_with(line, "Combine like terms")) line.clear();
    else if(starts_with(line, "inverse function")) line.clear();
    else if(starts_with(line, "Swap x and y")) line = "x = f(y)";
    else if(starts_with(line, "Target terms: ")) line = "u = " + trim_copy(line.substr(14));
    else if(starts_with(line, "Rearrange to make")) line.clear();
    else if(starts_with(line, "Reverse-chain trig substitution")) line.clear();
    else if(starts_with(line, "Reverse-chain substitution")) line.clear();
    else if(starts_with(line, "reverse chain sine rule")) line.clear();
    else if(starts_with(line, "reverse chain cosine rule")) line.clear();
    else if(starts_with(line, "reverse chain log rule")) line.clear();
    else if(starts_with(line, "Reverse-chain atan substitution")) line.clear();
    else if(starts_with(line, "Since d/dx")) line.clear();
    else if(starts_with(line, "Since Int(")) line.clear();
    else if(starts_with(line, "DI table integration by parts")) line.clear();
    else if(starts_with(line, "D column:")) line = "D: " + trim_copy(line.substr(9));
    else if(starts_with(line, "I column:")) line = "I: " + trim_copy(line.substr(9));
    else if(starts_with(line, "Alternate signs:")) line = "Signs: " + trim_copy(line.substr(16));
    else if(starts_with(line, "Combine:")) line = trim_copy(line.substr(8));
    else if(starts_with(line, "Odd sine power:")) line.clear();
    else if(starts_with(line, "Odd cosine power:")) line.clear();
    else if(starts_with(line, "Solve both factors")) line.clear();
    else if(starts_with(line, "Keep interval values")) line.clear();
    else if(starts_with(line, "Filter ")) line.clear();
    else if(starts_with(line, "the resulting polynomial")) line.clear();
    else if(starts_with(line, "the linear-factor fractions")) line.clear();
    else if(starts_with(line, "Split numerator:")) line = trim_copy(line.substr(16));
    else if(starts_with(line, "Split numerator into ")) line = "N = " + trim_copy(line.substr(21));
    else if(starts_with(line, "Denominator becomes ")) line = "den = " + trim_copy(line.substr(20));
    else if(starts_with(line, "The numerator becomes exactly ")) line = "num/x^2 = " + trim_copy(line.substr(30));
    else if(starts_with(line, "Divide numerator; denominator by ")) line = "N,D / " + trim_copy(line.substr(33));
    else if(starts_with(line, "Complete square: ")) line = trim_copy(line.substr(17));
    else if(starts_with(line, "A*D'/D by")) line.clear();
    else if(starts_with(line, "Polynomial division:")) line = "N/D = Q + R/D";
    else if(starts_with(line, "Divide polynomial numerator by linear denominator")) line = "N/D = Q + R/D";
    else if(starts_with(line, "quotient by power rule")) line.clear();
    else if(starts_with(line, "linear factor by log")) line.clear();
    else if(starts_with(line, "Apply the power rule")) line.clear();
    else if(starts_with(line, "power rule")) line.clear();
    else if(starts_with(line, "No elementary antiderivative")) line = "No elementary primitive";
    else if(starts_with(line, "2A*sqrt(D) plus log form")) line.clear();
    else if(starts_with(line, "log terms and repeated-root term")) line.clear();
    else if(starts_with(line, "Use sine-integral")) line.clear();
    else if(starts_with(line, "Use logarithmic integral")) line.clear();
    else if(starts_with(line, "Use Fresnel integral")) line.clear();
    else if(starts_with(line, "Use elliptic integral")) line.clear();
    else if(starts_with(line, "Use solve/expand/collect first")) line.clear();
    else if(starts_with(line, "solve/expand/collect first")) line.clear();
    else if(starts_with(line, "Partial fractions need rational")) line = "Err: PF needs rational P(x)/Q(x)";
    else if(starts_with(line, "List lengths must match")) line = "Err: list lengths differ";
    else if(starts_with(line, "Unsupported") || starts_with(line, "unsupported")) line.clear();
    else if(starts_with(line, "This lightweight factor route needs")) line = "Err: numeric coeffs needed";
    else if(starts_with(line, "not factorable with numeric roots")) line = "no rational factor";
    else if(starts_with(line, "exact endpoint values")) line.clear();
    else if(starts_with(line, "the constant and")) line.clear();
    else if(starts_with(line, "Apply constant rule")) line.clear();
    else if(starts_with(line, "Add the primitives")) line.clear();
    else if(starts_with(line, "term-by-term")) line.clear();
    else if(starts_with(line, "Fractional linear function is monotone")) line.clear();
    else if(starts_with(line, "Evaluate endpoints: ")) line = trim_copy(line.substr(20));
    else if(starts_with(line, "Evaluate endpoints since")) line.clear();
    else if(line == "to expr + C" || line == "to expr + C.") line.clear();
    else if(starts_with(line, "Apply power rule:")) line = trim_copy(line.substr(17));
    else if(starts_with(line, "Use standard result ")) line = trim_copy(line.substr(20));
    else if(starts_with(line, "standard result ")) line = trim_copy(line.substr(16));

    if(starts_with(line, "Integral becomes ")) line = "I=" + trim_copy(line.substr(17));
    if(starts_with(line, "The Int becomes ")) line = "I=" + trim_copy(line.substr(16));
    if(starts_with(line, "Int becomes ")) line = "I=" + trim_copy(line.substr(12));
    if(starts_with(line, "The integral becomes ")) line = "I=" + trim_copy(line.substr(21));
    if(starts_with(line, "Integrate: ")) line = trim_copy(line.substr(11));
    std::size_t as_pos = line.find(" as ");
    if(as_pos != std::string::npos && line.find('=') == std::string::npos) line.replace(as_pos, 4, " = ");
    if(starts_with(line, "Input = ")) line = trim_copy(line.substr(8));
    if(starts_with(line, "Out = ")) line = trim_copy(line.substr(6));
    if(starts_with(line, "Variable = ")) line.clear();
    if(starts_with(line, ": ")) line = trim_copy(line.substr(2));
    if(starts_with(line, "Integrate ")) line = strip_leading_word(line, "Integrate ");
    if(starts_with(line, "Simplify. ")) line = strip_leading_word(line, "Simplify. ");
    if(starts_with(line, "Simplify ")) line = strip_leading_word(line, "Simplify ");
    if(line == "Simplify" || line == "Simplify.") line.clear();
    if(starts_with(line, "Integral 1/x dx =")) line = "Int(1/x) dx = " + trim_copy(line.substr(17));
    if(starts_with(line, "Add +C")) line.clear();
    if(line == "reverse chain exponential rule") line.clear();
    if(starts_with(line, "solve/expand/collect first")) line.clear();
    if(starts_with(line, "plot summary ready")) line.clear();
    if(starts_with(line, "Check ")) line = "check: " + trim_copy(line.substr(6));
    if(starts_with(line, "Base angle: ")) line = "alpha = " + trim_copy(line.substr(12));
    if(starts_with(line, "First term: ")) line = trim_copy(line.substr(12));
    if(starts_with(line, "to ") && line.find("+ C") != std::string::npos) line = trim_copy(line.substr(3));
    if(line.find("asin(sin(u)) = u only for ") != std::string::npos) {
        std::size_t p = line.find(" only for ");
        line = trim_copy(line.substr(p + 10)) + " => asin(sin(u)) = u";
    }
    std::size_t missing_du = line.find(", but du/dx is missing");
    if(missing_du != std::string::npos) line = trim_copy(line.substr(0, missing_du)) + "; du/dx not constant";
    replace_all(line, " + 1*", " + ");
    replace_all(line, " - 1*", " - ");
    replace_all(line, " over sqrt(D)", "");
    if(starts_with(line, "1*")) line = trim_copy(line.substr(2));
    space_plain_equals(line);

    if(line == "Add constant C." || line == "Add constant C") line = "+ C.";
    std::size_t du_matches = line.find("; du matches");
    if(du_matches != std::string::npos) line = trim_copy(line.substr(0, du_matches));
    line = trim_copy(line);
    if(!line.empty() && !keep_short_exam_label(line) && !has_math_signal(line)) line.clear();
    replace_all(line, ".)", ")");
    replace_all(line, " -> ", "=>");
    if(!line.empty() && line.back() == '.') line.pop_back();
    if(line.empty()) return "";
    prefer_ln_for_natural_logs(line);
    return line;
}

std::vector<std::string> numbered_steps(std::vector<std::string> const &lines)
{
    std::vector<std::string> out;
    out.reserve(lines.size());
    int chars = 0;
    for(auto const &ln : lines) {
        if(static_cast<int>(out.size()) >= FORMAT_EXAM_MAX_LINES) {
            out.push_back("...");
            break;
        }
        std::string s = math_style_line(ln);
        if(s.empty()) continue;
        s = truncate_exam_line(s);
        if(chars + static_cast<int>(s.size()) > FORMAT_EXAM_MAX_CHARS) {
            out.push_back("...");
            break;
        }
        chars += static_cast<int>(s.size());
        out.push_back(s);
    }
    return out;
}

std::vector<std::string> format_exam_working(
    std::string const & /*method*/,
    std::vector<std::string> const &steps,
    std::string const &answer
)
{
    auto lines = numbered_steps(steps);
    if(!answer.empty()) {
        bool has_error = false;
        for(auto const &line : lines) {
            if(starts_with(line, "Err:")) {
                has_error = true;
                break;
            }
        }
        if(has_error) return lines;
        std::string text = answer;
        std::string spaced;
        spaced.reserve(text.size() + 8);
        for(std::size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if((c == '+' || c == '-') && i > 0 && i + 1 < text.size()) {
                char prev = text[i - 1];
                char next = text[i + 1];
                bool unary = prev == '(' || prev == '[' || prev == '{' || prev == '=' || prev == '/' || prev == '*' || prev == '^' ||
                             prev == '+' || prev == '-';
                bool exp_sign = (prev == 'e' || prev == 'E') && i >= 2 && std::isdigit(static_cast<unsigned char>(text[i - 2]));
                if(!unary && !exp_sign && prev != ' ' && next != ' ') {
                    spaced.push_back(' ');
                    spaced.push_back(c);
                    spaced.push_back(' ');
                    continue;
                }
            }
            spaced.push_back(c);
        }
        text.swap(spaced);
        prefer_ln_for_natural_logs(text);
        while(!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.erase(text.begin());
        text = truncate_exam_line(text);
        std::string low;
        low.reserve(text.size());
        for(char c : text) low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        if(low.rfind("answer:", 0) == 0) text = text.substr(7);
        else if(low.rfind("out =", 0) == 0) text = text.substr(5);
        else if(low.rfind("input =", 0) == 0) text = text.substr(7);
        while(!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.erase(text.begin());
        lines.push_back(text);
    }
    return lines;
}

static std::string format_equation_human_readable_impl(Arena &arena, NodeId node, int parent, int depth)
{
    if(depth > FORMAT_EXAM_MAX_DEPTH) return "...";
    Node const &n = arena.get(node);

    if(n.kind == NodeKind::Num) return num_text(n.num);
    if(n.kind == NodeKind::Sym) return n.text;
    if(n.kind == NodeKind::Const) return (n.ckind == ConstKind::Pi) ? "pi" : "e";

    if(n.kind == NodeKind::Fn) {
        if(n.fkind == FnKind::Factorial) {
            return format_equation_human_readable_impl(arena, n.a, 4, depth + 1) + "!";
        }
        if(n.fkind == FnKind::Log) {
            return "ln(" + format_equation_human_readable_impl(arena, n.a, 0, depth + 1) + ")";
        }
        if(n.fkind == FnKind::Exp) {
            return "e^(" + format_equation_human_readable_impl(arena, n.a, 0, depth + 1) + ")";
        }
        return fn_text(n.fkind) + "(" + format_equation_human_readable_impl(arena, n.a, 0, depth + 1) + ")";
    }

    if(n.kind == NodeKind::Pow) {
        std::string base = format_equation_human_readable_impl(arena, n.a, 3, depth + 1);
        std::string exponent = format_equation_human_readable_impl(arena, n.b, 3, depth + 1);
        Node const &bn = arena.get(n.a);
        Node const &en = arena.get(n.b);
        if(bn.kind == NodeKind::Add || bn.kind == NodeKind::Mul || bn.kind == NodeKind::Div ||
           (bn.kind == NodeKind::Num && bn.num.den != 1)) {
            base = "(" + base + ")";
        }
        if(!(en.kind == NodeKind::Num && en.num.den == 1)) {
            exponent = "(" + exponent + ")";
        }
        return base + "^" + exponent;
    }

    if(n.kind == NodeKind::Mul) {
        std::ostringstream oss;
        bool first = true;
        for(NodeId kid : n.kids) {
            std::string part = format_equation_human_readable_impl(arena, kid, 2, depth + 1);
            Node const &kn = arena.get(kid);
            if(kn.kind == NodeKind::Add) part = "(" + part + ")";
            if(!first) oss << "*";
            first = false;
            oss << part;
        }
        return oss.str();
    }

    if(n.kind == NodeKind::Div) {
        std::string top = format_equation_human_readable_impl(arena, n.a, 2, depth + 1);
        std::string bot = format_equation_human_readable_impl(arena, n.b, 2, depth + 1);
        Node const &tn = arena.get(n.a);
        Node const &bn = arena.get(n.b);
        if(tn.kind == NodeKind::Add || tn.kind == NodeKind::Mul) top = "(" + top + ")";
        if(bn.kind == NodeKind::Add || bn.kind == NodeKind::Mul) bot = "(" + bot + ")";
        return top + "/" + bot;
    }

    if(n.kind == NodeKind::Add) {
        std::ostringstream oss;
        bool first = true;
        for(NodeId kid : n.kids) {
            NodeId rest = kid;
            auto [has_coeff, coeff] = split_coeff(arena, kid, rest);
            std::string term;
            if(!has_coeff) {
                term = format_equation_human_readable_impl(arena, kid, 1, depth + 1);
            }
            else {
                term = format_equation_human_readable_impl(arena, rest, 1, depth + 1);
                if(!(coeff.num == coeff.den && coeff.den == 1)) {
                    term = num_text(coeff) + "*" + term;
                }
            }
            if(!first) oss << " + ";
            first = false;
            oss << term;
        }
        return oss.str();
    }

    return "<expr>";
}

std::string format_equation_human_readable(Arena &arena, NodeId node, int parent)
{
    NodeId s = simplify(arena, node);
    return format_equation_human_readable_impl(arena, s, parent, 0);
}

} // namespace casio

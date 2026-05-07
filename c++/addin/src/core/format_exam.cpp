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
        return line;
    }
    if(starts_with(line, "Result:")) {
        line = trim_copy(line.substr(7));
        return line;
    }

    replace_all(line, " → ", " => ");
    replace_all(line, "≤", "<=");
    replace_all(line, "≥", ">=");
    replace_all(line, "π", "pi");
    replace_all(line, "∫", "Int");
    replace_all(line, "Integral [", "Int(");
    replace_all(line, "Integral(", "Int(");
    replace_all(line, "Integral acos", "Int(acos");
    replace_all(line, "Integral asin", "Int(asin");
    replace_all(line, "Integral atan", "Int(atan");
    replace_all(line, "] dx", ") dx");
    replace_all(line, " integral ", " Int ");
    replace_all(line, ", so ", "; ");
    replace_all(line, " so ", "; ");
    replace_all(line, ", then ", "; ");
    replace_all(line, " then ", "; ");

    if(starts_with(line, "Use integration by parts")) line = "I=uv-Int(vdu).";
    else if(starts_with(line, "Apply integration by parts")) line = "I=uv-Int(vdu).";
    else if(starts_with(line, "Use parts:")) line = trim_copy(line.substr(10));
    else if(starts_with(line, "Parts:")) line = trim_copy(line.substr(6));
    else if(starts_with(line, "Use identity ")) line = trim_copy(line.substr(13));
    else if(starts_with(line, "Use identities:")) line = trim_copy(line.substr(15));
    else if(starts_with(line, "Use ")) line = trim_copy(line.substr(4));
    else if(starts_with(line, "Start with ")) line = trim_copy(line.substr(11));
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
    else if(starts_with(line, "Use implicit differentiation")) line = "d/dx(LHS)=d/dx(RHS).";
    else if(starts_with(line, "Use product/chain rules") || starts_with(line, "product/chain rules")) line = "dy/dx terms -> one side.";
    else if(starts_with(line, "Factor dy/dx")) line = "dy/dx = RHS/coeff(dy/dx).";
    else if(starts_with(line, "Clear denominators")) line = "clear denoms.";
    else if(starts_with(line, "Collect ")) line = strip_leading_word(line, "Collect ");
    else if(starts_with(line, "Multiply through")) line = strip_leading_word(line, "Multiply through");
    else if(starts_with(line, "Equate coefficients")) line = "coeffs: LHS=RHS.";
    else if(starts_with(line, "Complete the square")) line = strip_leading_word(line, "Complete the square");
    else if(starts_with(line, "Substitute ")) line = strip_leading_word(line, "Substitute ");
    else if(starts_with(line, "Integrate and simplify")) line.clear();
    else if(starts_with(line, "Classify the integrand")) line.clear();
    else if(starts_with(line, "Try substitution")) line = "u=inner; compare du.";
    else if(starts_with(line, "Test reverse chain")) line = "u=inner; compare du.";
    else if(starts_with(line, "If a product remains")) line = "I=uv-Int(vdu) or DI table.";

    if(starts_with(line, "Integral becomes ")) line = "I=" + trim_copy(line.substr(17));
    if(starts_with(line, "Variable = ")) line = "var=" + trim_copy(line.substr(11));
    if(starts_with(line, ": ")) line = trim_copy(line.substr(2));
    if(starts_with(line, "Integrate ")) line = strip_leading_word(line, "Integrate ");
    if(starts_with(line, "Simplify. ")) line = strip_leading_word(line, "Simplify. ");
    if(starts_with(line, "Simplify ")) line = strip_leading_word(line, "Simplify ");
    if(starts_with(line, "Check ")) line = "check: " + trim_copy(line.substr(6));

    if(line == "Add constant C." || line == "Add constant C") line = "+ C.";
    line = trim_copy(line);
    replace_all(line, " = ", "=");
    replace_all(line, "= ", "=");
    replace_all(line, " =", "=");
    replace_all(line, " -> ", "=>");
    if(!line.empty() && line.back() == '.') line.pop_back();
    if(line.empty()) return "";
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
        text = truncate_exam_line(text);
        std::string low;
        low.reserve(text.size());
        for(char c : text) low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        if(low.rfind("answer:", 0) == 0) text = text.substr(7);
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
            return "log(" + format_equation_human_readable_impl(arena, n.a, 0, depth + 1) + ")";
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

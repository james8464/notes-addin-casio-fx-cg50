#include "format_exam.hpp"

#include "simplify.hpp"

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
    }
    return "fn";
}

static std::pair<bool, Rational> split_coeff(Arena &arena, NodeId node, NodeId &rest_out)
{
    NodeId s = simplify(arena, node);
    Node const &n = arena.get(s);
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
    rest_out = s;
    return {false, Rational::from_int(1)};
}

} // namespace

std::vector<std::string> numbered_steps(std::vector<std::string> const &lines)
{
    std::vector<std::string> out;
    out.reserve(lines.size());
    for(auto const &ln : lines) {
        std::string s = ln;
        // mimic Python .strip()
        while(!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
            s.erase(s.begin());
        while(!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
            s.pop_back();
        if(s.empty()) continue;
        out.push_back(std::to_string(out.size() + 1) + ". " + s);
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
        std::string low;
        low.reserve(text.size());
        for(char c : text) low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        if(low.rfind("answer:", 0) == 0) lines.push_back(text);
        else lines.push_back("Answer: " + text);
    }
    return lines;
}

std::string format_equation_human_readable(Arena &arena, NodeId node, int parent)
{
    NodeId s = simplify(arena, node);
    Node const &n = arena.get(s);

    if(n.kind == NodeKind::Num) return num_text(n.num);
    if(n.kind == NodeKind::Sym) return n.text;
    if(n.kind == NodeKind::Const) return (n.ckind == ConstKind::Pi) ? "pi" : "e";

    if(n.kind == NodeKind::Fn) {
        if(n.fkind == FnKind::Log) {
            return "log(" + format_equation_human_readable(arena, n.a, 0) + ")";
        }
        if(n.fkind == FnKind::Exp) {
            return "e^(" + format_equation_human_readable(arena, n.a, 0) + ")";
        }
        return fn_text(n.fkind) + "(" + format_equation_human_readable(arena, n.a, 0) + ")";
    }

    if(n.kind == NodeKind::Pow) {
        std::string base = format_equation_human_readable(arena, n.a, 3);
        std::string exponent = format_equation_human_readable(arena, n.b, 3);
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
            std::string part = format_equation_human_readable(arena, kid, 2);
            Node const &kn = arena.get(kid);
            if(kn.kind == NodeKind::Add) part = "(" + part + ")";
            if(!first) oss << "*";
            first = false;
            oss << part;
        }
        return oss.str();
    }

    if(n.kind == NodeKind::Div) {
        std::string top = format_equation_human_readable(arena, n.a, 2);
        std::string bot = format_equation_human_readable(arena, n.b, 2);
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
                term = format_equation_human_readable(arena, kid, 1);
            }
            else {
                term = format_equation_human_readable(arena, rest, 1);
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

} // namespace casio

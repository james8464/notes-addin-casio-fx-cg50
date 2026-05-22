#include "format_expr.hpp"

#include "simplify.hpp"

#include <sstream>

namespace casio
{
namespace
{

static int prec(Node const &n)
{
    if(n.kind == NodeKind::Add) return 1;
    if(n.kind == NodeKind::Mul || n.kind == NodeKind::Div) return 2;
    if(n.kind == NodeKind::Pow) return 3;
    return 4;
}

static std::string num_text(Rational const &q)
{
    if(q.den == 1) return std::to_string(q.num);
    return std::to_string(q.num) + "/" + std::to_string(q.den);
}

static std::string fn_name(FnKind k, bool human)
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

} // namespace

static std::string format_expr_impl(Arena &arena, NodeId node, int parent_prec, bool human)
{
    node = simplify(arena, node);
    Node const &n = arena.get(node);

    if(n.kind == NodeKind::Num) return num_text(n.num);
    if(n.kind == NodeKind::Sym) return n.text;
    if(n.kind == NodeKind::Const) return (n.ckind == ConstKind::Pi) ? "pi" : "e";
    if(n.kind == NodeKind::Fn) {
        if(n.fkind == FnKind::Factorial) {
            return format_expr_impl(arena, n.a, 4, human) + "!";
        }
        return fn_name(n.fkind, human) + "(" + format_expr_impl(arena, n.a, 0, human) + ")";
    }
    if(n.kind == NodeKind::Pow) {
        Node const &base = arena.get(n.a);
        std::string text;
        // Special-case sqrt: x^(1/2) -> sqrt(x) (matches SUVAT python output)
        Node const &expn = arena.get(n.b);
        if(expn.kind == NodeKind::Num && expn.num.num == 1 && expn.num.den == 2) {
            text = "sqrt(" + format_expr_impl(arena, n.a, 0, human) + ")";
        }
        else if(base.kind == NodeKind::Const && base.ckind == ConstKind::E) {
            text = "e^(" + format_expr_impl(arena, n.b, 0, human) + ")";
        }
        else {
            std::string exp_text = format_expr_impl(arena, n.b, 3, human);
            if(expn.kind == NodeKind::Num && expn.num.den != 1) exp_text = "(" + exp_text + ")";
            text = format_expr_impl(arena, n.a, 3, human) + "^" + exp_text;
        }
        if(prec(n) < parent_prec) return "(" + text + ")";
        return text;
    }
    if(n.kind == NodeKind::Div) {
        std::string text = format_expr_impl(arena, n.a, 2, human) + "/" + format_expr_impl(arena, n.b, 3, human);
        if(prec(n) < parent_prec) return "(" + text + ")";
        return text;
    }
    if(n.kind == NodeKind::Mul) {
        std::ostringstream oss;
        // Special-case leading -1: -1*x -> -x
        if(!n.kids.empty()) {
            Node const &k0 = arena.get(n.kids.front());
            if(k0.kind == NodeKind::Num && k0.num.num == -1 && k0.num.den == 1 && n.kids.size() > 1) {
                std::ostringstream rest;
                // render rest with multiplication
                for(std::size_t i = 1; i < n.kids.size(); i++) {
                    if(i > 1) rest << "*";
                    rest << format_expr_impl(arena, n.kids[i], 2, human);
                }
                std::string r = rest.str();
                std::string text = !r.empty() && r[0] == '-' ? r.substr(1) : "-" + r;
                if(prec(n) < parent_prec) return "(" + text + ")";
                return text;
            }
        }

        bool first = true;
        for(NodeId kid : n.kids) {
            if(!first) oss << "*";
            first = false;
            oss << format_expr_impl(arena, kid, 2, human);
        }
        std::string text = oss.str();
        if(prec(n) < parent_prec) return "(" + text + ")";
        return text;
    }
    if(n.kind == NodeKind::Add) {
        std::vector<std::string> parts;
        parts.reserve(n.kids.size());

        for(NodeId kid : n.kids) {
            Node const &it = arena.get(kid);

            // Negative number: "- <abs>"
            if(it.kind == NodeKind::Num && it.num.num < 0) {
                Rational q = it.num;
                q.num = -q.num;
                parts.push_back(std::string("- ") + num_text(q));
                continue;
            }

            // Negative leading coefficient in multiplication: "- <rest>"
            if(it.kind == NodeKind::Mul && !it.kids.empty()) {
                Node const &k0 = arena.get(it.kids.front());
                if(k0.kind == NodeKind::Num && k0.num.num < 0) {
                    std::ostringstream term;
                    term << "- ";
                    bool mf = true;
                    for(std::size_t i = 0; i < it.kids.size(); i++) {
                        if(i == 0) {
                            Rational q = k0.num;
                            q.num = -q.num;
                            bool coeff_is_one = (q.den == 1 && q.num == 1);
                            if(!(coeff_is_one && it.kids.size() > 1)) {
                                if(!mf) term << "*";
                                mf = false;
                                term << num_text(q);
                            }
                        }
                        else {
                            if(!mf) term << "*";
                            mf = false;
                            term << format_expr(arena, it.kids[i], 2);
                        }
                    }
                    parts.push_back(term.str());
                    continue;
                }
            }

            // Ordinary term: first as-is, later prefixed with "+ ".
            if(parts.empty()) parts.push_back(format_expr_impl(arena, kid, 1, human));
            else parts.push_back(std::string("+ ") + format_expr_impl(arena, kid, 1, human));
        }

        std::ostringstream oss;
        for(std::size_t i = 0; i < parts.size(); i++) {
            if(i) oss << " ";
            oss << parts[i];
        }

        std::string text = oss.str();
        if(prec(n) < parent_prec) return "(" + text + ")";
        return text;
    }
    return "<node>";
}

std::string format_expr(Arena &arena, NodeId node, int parent_prec)
{
    return format_expr_impl(arena, node, parent_prec, false);
}

std::string format_expr_human(Arena &arena, NodeId node, int parent_prec)
{
    return format_expr_impl(arena, node, parent_prec, true);
}

} // namespace casio

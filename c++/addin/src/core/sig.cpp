#include "sig.hpp"

#include "format_expr.hpp"
#include "simplify.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace casio
{

static void sig_rec(Arena &arena, NodeId node, std::ostringstream &out, int depth)
{
    if(depth > 512) throw std::runtime_error("sig() recursion limit exceeded.");
    node = simplify(arena, node);
    Node const &n = arena.get(node);
    switch(n.kind) {
    case NodeKind::Num:
        out << "num(" << n.num.num << "/" << n.num.den << ")";
        return;
    case NodeKind::Sym:
        out << "sym(" << n.text << ")";
        return;
    case NodeKind::Const:
        out << (n.ckind == ConstKind::Pi ? "pi" : "e");
        return;
    case NodeKind::Fn: {
        out << "fn(" << static_cast<int>(n.fkind) << ",";
        sig_rec(arena, n.a, out, depth + 1);
        out << ")";
        return;
    }
    case NodeKind::Pow:
        out << "pow(";
        sig_rec(arena, n.a, out, depth + 1);
        out << ",";
        sig_rec(arena, n.b, out, depth + 1);
        out << ")";
        return;
    case NodeKind::Div:
        out << "div(";
        sig_rec(arena, n.a, out, depth + 1);
        out << ",";
        sig_rec(arena, n.b, out, depth + 1);
        out << ")";
        return;
    case NodeKind::Add:
    case NodeKind::Mul: {
        std::vector<std::string> kids;
        kids.reserve(n.kids.size());
        for(auto k : n.kids) {
            std::ostringstream tmp;
            sig_rec(arena, k, tmp, depth + 1);
            kids.push_back(tmp.str());
        }
        std::sort(kids.begin(), kids.end());
        out << (n.kind == NodeKind::Add ? "add(" : "mul(");
        for(std::size_t i = 0; i < kids.size(); i++) {
            if(i) out << ",";
            out << kids[i];
        }
        out << ")";
        return;
    }
    }
}

std::string sig(Arena &arena, NodeId node)
{
    std::ostringstream out;
    sig_rec(arena, node, out, 0);
    return out.str();
}

} // namespace casio


#include "arena.hpp"

#include <stdexcept>

namespace casio
{

static constexpr NodeId kUnset = 0xFFFFFFFFu;

Arena::Arena()
{
    nodes.reserve(4096);
    simplify_cache.reserve(4096);
}

void Arena::push(Node &&n)
{
    if(max_nodes != 0 && nodes.size() >= max_nodes) {
        throw std::runtime_error("Arena node limit exceeded.");
    }
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
}

NodeId Arena::num(Rational r)
{
    r.normalize();
    Node n;
    n.kind = NodeKind::Num;
    n.num = r;
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::sym(std::string_view name)
{
    Node n;
    n.kind = NodeKind::Sym;
    n.text = std::string(name);
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::constant(ConstKind k)
{
    Node n;
    n.kind = NodeKind::Const;
    n.ckind = k;
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::fn(FnKind k, NodeId arg)
{
    Node n;
    n.kind = NodeKind::Fn;
    n.fkind = k;
    n.a = arg;
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::add(std::vector<NodeId> terms)
{
    Node n;
    n.kind = NodeKind::Add;
    n.kids = std::move(terms);
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::mul(std::vector<NodeId> factors)
{
    Node n;
    n.kind = NodeKind::Mul;
    n.kids = std::move(factors);
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::pow(NodeId base, NodeId exp)
{
    Node n;
    n.kind = NodeKind::Pow;
    n.a = base;
    n.b = exp;
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

NodeId Arena::div(NodeId nnum, NodeId dden)
{
    Node n;
    n.kind = NodeKind::Div;
    n.a = nnum;
    n.b = dden;
    push(std::move(n));
    return static_cast<NodeId>(nodes.size() - 1u);
}

bool Arena::has_simplify_cache(NodeId id) const
{
    if(id >= simplify_cache.size()) return false;
    return simplify_cache[id] != kUnset;
}

NodeId Arena::get_simplify_cache(NodeId id) const { return simplify_cache.at(id); }

void Arena::set_simplify_cache(NodeId id, NodeId simplified)
{
    if(id >= simplify_cache.size()) simplify_cache.resize(id + 1, kUnset);
    simplify_cache[id] = simplified;
}

} // namespace casio


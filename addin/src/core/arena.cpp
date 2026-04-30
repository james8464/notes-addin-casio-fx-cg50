#include "arena.hpp"

namespace casio
{

static constexpr NodeId kUnset = 0xFFFFFFFFu;

Arena::Arena()
{
    nodes.reserve(4096);
    simplify_cache.reserve(4096);
}

NodeId Arena::num(Rational r)
{
    r.normalize();
    Node n;
    n.kind = NodeKind::Num;
    n.num = r;
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::sym(std::string_view name)
{
    Node n;
    n.kind = NodeKind::Sym;
    n.text = std::string(name);
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::constant(ConstKind k)
{
    Node n;
    n.kind = NodeKind::Const;
    n.ckind = k;
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::fn(FnKind k, NodeId arg)
{
    Node n;
    n.kind = NodeKind::Fn;
    n.fkind = k;
    n.a = arg;
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::add(std::vector<NodeId> terms)
{
    Node n;
    n.kind = NodeKind::Add;
    n.kids = std::move(terms);
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::mul(std::vector<NodeId> factors)
{
    Node n;
    n.kind = NodeKind::Mul;
    n.kids = std::move(factors);
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::pow(NodeId base, NodeId exp)
{
    Node n;
    n.kind = NodeKind::Pow;
    n.a = base;
    n.b = exp;
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
}

NodeId Arena::div(NodeId nnum, NodeId dden)
{
    Node n;
    n.kind = NodeKind::Div;
    n.a = nnum;
    n.b = dden;
    nodes.push_back(std::move(n));
    simplify_cache.push_back(kUnset);
    return static_cast<NodeId>(nodes.size() - 1);
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


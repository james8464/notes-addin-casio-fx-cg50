#pragma once

#include "node.hpp"

#include <string_view>
#include <vector>

namespace casio
{

class Arena
{
public:
    Arena();

    NodeId num(Rational r);
    NodeId sym(std::string_view name);
    NodeId constant(ConstKind k);
    NodeId fn(FnKind k, NodeId arg);
    NodeId add(std::vector<NodeId> terms);
    NodeId mul(std::vector<NodeId> factors);
    NodeId pow(NodeId base, NodeId exp);
    NodeId div(NodeId n, NodeId d);

    Node const &get(NodeId id) const { return nodes.at(id); }
    Node &get(NodeId id) { return nodes.at(id); }

    std::size_t size() const { return nodes.size(); }

private:
    std::vector<Node> nodes;
};

} // namespace casio


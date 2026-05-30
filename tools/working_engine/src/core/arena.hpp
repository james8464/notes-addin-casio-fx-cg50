#pragma once

#include "node.hpp"

#if defined(TARGET_PRIZM)
// PrizmSDK: use compat header
#include "device/casio_compat.hpp"
#else
#include <string_view>
#include <vector>
#endif

namespace casio
{

class Arena
{
public:
    Arena();

    // Hard budget to prevent hangs/crashes on pathological inputs.
    // 0 means unlimited.
    void set_max_nodes(std::size_t n) { max_nodes = n; }
    std::size_t get_max_nodes() const { return max_nodes; }

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

    // Simplify memoization (per-node, per-arena).
    // Returned NodeId is the simplified version of the corresponding node id.
    bool has_simplify_cache(NodeId id) const;
    NodeId get_simplify_cache(NodeId id) const;
    void set_simplify_cache(NodeId id, NodeId simplified);

private:
    void push(Node &&n);
    std::vector<Node> nodes;
    std::vector<NodeId> simplify_cache;
    std::size_t max_nodes = 0;
};

} // namespace casio

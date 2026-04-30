#pragma once

#include <memory>
#include <string>
#include <vector>

namespace casio::boolean
{

struct Node;
using NodeRef = std::shared_ptr<Node>;

enum class Kind : char
{
    Const, // "c"
    Var,   // "v"
    Not,   // "n"
    And,   // "a" (.)
    Or,    // "o" (+)
};

struct Node
{
    Kind kind = Kind::Const;
    // Const: value "0" or "1"
    // Var: name
    // Not: kids[0]
    // And/Or: kids list
    std::string text{};
    std::vector<NodeRef> kids{};
};

NodeRef Z(); // 0
NodeRef O(); // 1

NodeRef parse(std::string const &text);  // booleanProgram.parse()
std::string show(NodeRef const &node);   // booleanProgram.show()
std::string short_text(NodeRef const &node); // booleanProgram.short()

bool same(NodeRef const &a, NodeRef const &b); // booleanProgram.same()
bool comp(NodeRef const &a, NodeRef const &b); // complement check

// One simplification step. Returns (newNode, reason) or (nullptr, "") if no step.
std::pair<NodeRef, std::string> step(NodeRef const &node);

NodeRef normalise(NodeRef const &node);
NodeRef to_nand(NodeRef const &node);
NodeRef to_nor(NodeRef const &node);

std::pair<NodeRef, std::vector<std::pair<std::string, std::string>>> prove_both(
    NodeRef const &node,
    int max_steps = 30
);

std::pair<std::vector<std::string>, std::string> prove(
    std::string const &lhs_text,
    std::string const &rhs_text
);

} // namespace casio::boolean


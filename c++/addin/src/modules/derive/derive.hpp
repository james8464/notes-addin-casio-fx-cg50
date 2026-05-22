#pragma once
#include "core/arena.hpp"
#include <string>
#include <vector>
namespace casio
{
namespace derive
{
struct Request
{
    int mode = 1;          // 1=normal, 2=implicit, 3=parametric, 4=second
    std::string expr;      // raw input expression (may contain commas for extra args)
};
NodeId differentiate_node(Arena &arena, NodeId node, std::string const &var, std::string const &dep = "");
std::vector<std::string> run(Arena &arena, Request const &req);
} // namespace derive
} // namespace casio

#pragma once

#include "arena.hpp"

#include <string>
#include <vector>

namespace casio
{

// Port of python/src/shared_formatting.py for the C++ AST.

std::vector<std::string> numbered_steps(std::vector<std::string> const &lines);
std::string math_style_line(std::string line);
std::vector<std::string> format_exam_working(
    std::string const &method,
    std::vector<std::string> const &steps,
    std::string const &answer
);

std::string format_equation_human_readable(Arena &arena, NodeId node, int parent = 0);

} // namespace casio

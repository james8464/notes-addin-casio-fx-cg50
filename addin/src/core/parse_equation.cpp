#include "parse_equation.hpp"

#include "parse.hpp"

#include <cstddef>
#include <stdexcept>

namespace casio
{

static std::size_t find_top_level_equals(std::string const &s)
{
    int depth = 0;
    for(std::size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if(c == '(' || c == '[' || c == '{') depth++;
        else if(c == ')' || c == ']' || c == '}') depth--;
        else if(c == '=' && depth == 0) return i;
    }
    return std::string::npos;
}

std::optional<Equation> parse_equation(Arena &arena, std::string text)
{
    auto pos = find_top_level_equals(text);
    if(pos == std::string::npos) return std::nullopt;

    // Reject multiple '=' at top-level.
    auto pos2 = find_top_level_equals(text.substr(pos + 1));
    if(pos2 != std::string::npos) throw std::runtime_error("Only one '=' supported.");

    std::string left = text.substr(0, pos);
    std::string right = text.substr(pos + 1);
    if(left.empty() || right.empty()) throw std::runtime_error("Invalid equation.");

    Equation eq;
    eq.lhs = parse_expr(arena, left);
    eq.rhs = parse_expr(arena, right);
    return eq;
}

} // namespace casio


#include "parse_equation.hpp"

#include "parse.hpp"

#include <cstddef>
#include <cctype>
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

static bool wraps_outer_parens(std::string const &s)
{
    if(s.size() < 2) return false;
    if(s.front() != '(' || s.back() != ')') return false;
    int depth = 0;
    for(std::size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if(c == '(') depth++;
        else if(c == ')') depth--;
        if(depth == 0 && i != s.size() - 1) return false;
    }
    return depth == 0;
}

std::optional<Equation> parse_equation(Arena &arena, std::string text)
{
    // Trim leading/trailing whitespace (stdin-program payloads often include blank lines).
    while(!text.empty() && std::isspace((unsigned char)text.front())) text.erase(text.begin());
    while(!text.empty() && std::isspace((unsigned char)text.back())) text.pop_back();

    // Handle equations wrapped in a single outer (...) pair, eg "(x^2-1=0)".
    // Python harness generates these frequently.
    while(wraps_outer_parens(text) && find_top_level_equals(text.substr(1, text.size() - 2)) != std::string::npos) {
        text = text.substr(1, text.size() - 2);
    }

    auto pos = find_top_level_equals(text);
    if(pos == std::string::npos) {
        // Fallback: some fuzzed inputs contain awkward parentheses that prevent a
        // clean top-level scan. If there is exactly one '=', try splitting on it.
        auto first = text.find('=');
        if(first == std::string::npos) return std::nullopt;
        if(text.find('=', first + 1) != std::string::npos) throw std::runtime_error("Only one '=' supported.");
        std::string left = text.substr(0, first);
        std::string right = text.substr(first + 1);
        if(left.empty() || right.empty()) throw std::runtime_error("Invalid equation.");
        Equation eq;
        eq.lhs = parse_expr(arena, left);
        eq.rhs = parse_expr(arena, right);
        return eq;
    }

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

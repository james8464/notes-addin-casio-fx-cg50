#pragma once

#include "core/arena.hpp"

#include <string>
#include <vector>

namespace casio::suvat
{

struct Inputs
{
    // Order: s,u,v,a,t (match Python file)
    std::string s;
    std::string u;
    std::string v;
    std::string a;
    std::string t;
    // Target variable: one of "s","u","v","a","t"
    std::string target;
};

// Parse user strings into nodes (None if blank/target).
// Accept comma marker inside field to mark target, like Python UX.
Inputs normalize_inputs(Inputs in);

// Solve target variable. Returns output lines (already exam-ish).
std::vector<std::string> solve(Arena &arena, Inputs const &in);

} // namespace casio::suvat


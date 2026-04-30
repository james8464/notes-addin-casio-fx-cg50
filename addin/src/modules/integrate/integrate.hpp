#pragma once

#include "core/arena.hpp"

#include <string>
#include <vector>

namespace casio::integrate
{

struct Request
{
    int mode = 1;          // mirror python menu later
    std::string expr;      // integrand
    std::string var = "x"; // default
};

std::vector<std::string> run(Arena &arena, Request const &req);

} // namespace casio::integrate


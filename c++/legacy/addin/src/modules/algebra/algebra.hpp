#pragma once

#include "core/arena.hpp"

#include <string>
#include <vector>

namespace casio::algebra
{

struct Request
{
    int mode = 1;
    std::string expr;
};

std::vector<std::string> run(Arena &arena, Request const &req);

} // namespace casio::algebra


#pragma once

#include "core/arena.hpp"

#include <string>
#include <vector>

namespace casio::trig
{

struct Request
{
    int mode = 1;
    std::string expr;
    std::string method;
};

std::vector<std::string> run(Arena &arena, Request const &req);

} // namespace casio::trig

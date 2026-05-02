#pragma once

#include "core/arena.hpp"

#include <string>
#include <vector>

namespace casio::stats
{

struct Request {
    int mode = 0;
    std::string expr;
    std::string expr2;
};

std::vector<std::string> run(Arena &arena, Request const &req);

} // namespace casio::stats

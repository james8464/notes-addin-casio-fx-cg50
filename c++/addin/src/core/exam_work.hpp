#pragma once

#include "arena.hpp"

#include <string>
#include <vector>

namespace casio
{

struct ExamPrelude
{
    std::string raw;
    std::string norm;
    std::string parsed;
    std::string simplified;
};

// Build common prelude data for exam-style lines.
ExamPrelude build_exam_prelude(Arena &arena, std::string const &raw, NodeId parsed_node);

// Append student-facing prelude lines, never parser/debug diagnostics.
void append_exam_prelude_steps(std::vector<std::string> &steps, ExamPrelude const &prelude);

// Produce a consistent working block with an Answer line at end.
std::vector<std::string> exam_block(
    std::string const &method,
    std::vector<std::string> const &steps,
    std::string const &answer
);

// Fallback: always give some working + best-effort final.
std::vector<std::string> exam_fallback(
    std::string const &method,
    ExamPrelude const &prelude,
    std::string const &reason,
    std::string const &answer
);

} // namespace casio

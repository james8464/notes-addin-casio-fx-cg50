#pragma once

#include "rational.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace casio
{

using NodeId = std::uint32_t;

enum class NodeKind : std::uint8_t
{
    Num,
    Sym,
    Const,
    Fn,
    Add,
    Mul,
    Pow,
    Div,
};

enum class ConstKind : std::uint8_t
{
    E,
    Pi,
};

// Mirror your Python FUNC_NAMES/FUNC_ALIASES at the core layer.
enum class FnKind : std::uint8_t
{
    Sin,
    Cos,
    Tan,
    Sec,
    Cosec,
    Cot,
    Asin,
    Acos,
    Atan,
    Sinh,
    Cosh,
    Tanh,
    Exp,
    Log,   // internal name: log, display: ln
    Log10,
    Sqrt,
    Abs,
};

struct Node
{
    NodeKind kind = NodeKind::Num;

    // Note: This is intentionally simple and not the final memory layout.
    // We'll likely move to a more compact tagged layout once parity is reached.
    Rational num{};
    std::string text{};
    ConstKind ckind = ConstKind::E;
    FnKind fkind = FnKind::Sin;
    NodeId a = 0;
    NodeId b = 0;
    std::vector<NodeId> kids{};
};

} // namespace casio


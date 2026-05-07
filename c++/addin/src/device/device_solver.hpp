#pragma once

#include "device/line_buffer.hpp"

namespace casio::device
{

enum class Module
{
    Shell,
    Simplify,
    Algebra,
    Derive,
    Integrate,
    Trig,
    Suvat,
    Stats,
    Boolean,
    DeriveNormal,
    DeriveImplicit,
    DeriveParam,
    IntDE,
    IntParamArea,
    AlgCompare,
    AlgTransform,
    AlgExpand,
    AlgPoly,
    AlgCompSq,
    AlgComplain,
    AlgInverse,
    AlgRewrite,
    AlgDomRng,
    AlgCartesian,
    AlgNewton,
    TrigProve,
    TrigTransform,
    TrigSolve,
    TrigRewrite,
    BoolNAND,
    BoolNOR,
    BoolProve,
};

bool solve(Module module, const char *input, OutputLines &out);

} // namespace casio::device

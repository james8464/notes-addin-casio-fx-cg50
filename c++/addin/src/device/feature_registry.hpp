#pragma once

// PrizmSDK compatibility: use <stdint.h> (no C++ stdlib)
#if defined(TARGET_PRIZM)
#include <stdint.h>
#else
#include <cstdint>
#endif

namespace casio::feature
{

using FeatureId = uint32_t;

enum : FeatureId
{
    F_SIMPLIFY = 100,

    F_DERIVE_N = 200,
    F_DERIVE_IMP = 201,
    F_DERIVE_PAR = 202,

    F_INT = 300,
    F_INT_DE = 301,

    F_ALG_CMP = 400,
    F_ALG_XFORM = 401,
    F_ALG_EXP = 402,
    F_ALG_POLY = 403,
    F_ALG_COMP_SQ = 404,
    F_ALG_SOLVE = 405,
    F_ALG_COMP = 406,
    F_ALG_INV = 407,
    F_ALG_RW = 408,
    F_ALG_DOM_RNG = 409,
    F_ALG_CART = 410,

    F_TRIG_PROVE = 500,
    F_TRIG_XFORM = 501,
    F_TRIG_SOLVE = 502,
    F_TRIG_RW = 503,

    F_SUVAT = 600,

    F_SHELL = 900,
};

struct PromptSpec
{
    const char* name;
    const char* prompt;
    const char* help;
    const char* defopt;
};

struct FeatureSpec
{
    FeatureId id;
    const char* group;
    const char* short_label;
    const char* long_label;
    const char** aliases;
    int alias_count;
    const PromptSpec* prompts;
    int prompt_count;
};

const char* ALIAS_SIMPLIFY[] = {"smp", "simplify"};
const char* ALIAS_DERIVE_N[] = {"drv", "derive", "d/dx", "norm"};
const char* ALIAS_DERIVE_IMP[] = {"imp", "impDiff", "implicit"};
const char* ALIAS_DERIVE_PAR[] = {"par", "paramD", "param"};
const char* ALIAS_INT[] = {"int", "integrate"};
const char* ALIAS_INT_DE[] = {"de", "diffEq"};
const char* ALIAS_ALG_CMP[] = {"cmp", "compare"};
const char* ALIAS_ALG_XFORM[] = {"xform", "transform"};
const char* ALIAS_ALG_EXP[] = {"exp", "expand"};
const char* ALIAS_ALG_POLY[] = {"poly", "polynomial"};
const char* ALIAS_ALG_COMP_SQ[] = {"csq", "compSq", "completeSquare"};
const char* ALIAS_ALG_SOLVE[] = {"solve"};
const char* ALIAS_ALG_COMP[] = {"comp", "complain"};
const char* ALIAS_ALG_INV[] = {"inv", "inverse"};
const char* ALIAS_ALG_RW[] = {"rw", "rewrite"};
const char* ALIAS_ALG_DOM_RNG[] = {"dr", "dom", "rng", "domain", "range"};
const char* ALIAS_ALG_CART[] = {"cart", "cartesian"};
const char* ALIAS_TRIG_PROVE[] = {"prove"};
const char* ALIAS_TRIG_XFORM[] = {"xform", "transform"};
const char* ALIAS_TRIG_SOLVE[] = {"solve"};
const char* ALIAS_TRIG_RW[] = {"rw", "rewrite"};
const char* ALIAS_SUVAT[] = {"suvat"};
const char* ALIAS_SHELL[] = {"shell", "sh"};

const PromptSpec PROMPTS_NONE[] = {
    {},
};

const PromptSpec PROMPT_Y = {
    "y", "y: ", "Enter expression", ""
};
const PromptSpec PROMPT_E1 = {
    "E1", "E1: ", "Enter first expression", ""
};
const PromptSpec PROMPT_E2 = {
    "E2", "E2: ", "Enter second expression", ""
};
const PromptSpec PROMPT_F = {
    "f", "f: ", "Enter expression", ""
};
const PromptSpec PROMPT_U = {
    "u", "u: ", "Enter expression", ""
};
const PromptSpec PROMPT_X = {
    "X", "X: ", "Enter variable", "x"
};
const PromptSpec PROMPT_EQ = {
    "Eq", "Eq: ", "Enter equation", ""
};
const PromptSpec PROMPT_S = {
    "S", "S: ", "Enter expression", ""
};

const FeatureSpec FEATURES[] = {
    {F_SIMPLIFY, "Simplify", "SMP", "Simplify", ALIAS_SIMPLIFY, 2, PROMPTS_NONE, 0},

    {F_DERIVE_N, "Derive", "DRV", "Normal Deriv", ALIAS_DERIVE_N, 4, &PROMPT_Y, 1},
    {F_DERIVE_IMP, "Derive", "IMP", "Imp Diff", ALIAS_DERIVE_IMP, 3, &PROMPT_EQ, 1},
    {F_DERIVE_PAR, "Derive", "PRD", "Param D", ALIAS_DERIVE_PAR, 3, PROMPTS_NONE, 0},

    {F_INT, "Integrate", "INT", "Integrate", ALIAS_INT, 2, &PROMPT_F, 1},
    {F_INT_DE, "Integrate", "DE", "Diff Eq", ALIAS_INT_DE, 2, PROMPTS_NONE, 0},

    {F_ALG_CMP, "Algebra", "CMP", "Compare", ALIAS_ALG_CMP, 2, PROMPTS_NONE, 0},
    {F_ALG_XFORM, "Algebra", "XFM", "Transform", ALIAS_ALG_XFORM, 2, PROMPTS_NONE, 0},
    {F_ALG_EXP, "Algebra", "EXP", "Expand", ALIAS_ALG_EXP, 1, PROMPTS_NONE, 0},
    {F_ALG_POLY, "Algebra", "PLY", "Polynomial", ALIAS_ALG_POLY, 1, PROMPTS_NONE, 0},
    {F_ALG_COMP_SQ, "Algebra", "CSQ", "Comp Sq", ALIAS_ALG_COMP_SQ, 3, PROMPTS_NONE, 0},
    {F_ALG_SOLVE, "Algebra", "SLV", "Solve", ALIAS_ALG_SOLVE, 1, &PROMPT_EQ, 1},
    {F_ALG_COMP, "Algebra", "CMP", "Complain", ALIAS_ALG_COMP, 1, PROMPTS_NONE, 0},
    {F_ALG_INV, "Algebra", "INV", "Inverse", ALIAS_ALG_INV, 2, PROMPTS_NONE, 0},
    {F_ALG_RW, "Algebra", "RW", "Rewrite", ALIAS_ALG_RW, 2, PROMPTS_NONE, 0},
    {F_ALG_DOM_RNG, "Algebra", "DR", "Dom/Rng", ALIAS_ALG_DOM_RNG, 4, PROMPTS_NONE, 0},
    {F_ALG_CART, "Algebra", "CRT", "Cartesian", ALIAS_ALG_CART, 1, PROMPTS_NONE, 0},

    {F_TRIG_PROVE, "Trig", "PRV", "Prove", ALIAS_TRIG_PROVE, 1, PROMPTS_NONE, 0},
    {F_TRIG_XFORM, "Trig", "XFM", "Transform", ALIAS_TRIG_XFORM, 2, PROMPTS_NONE, 0},
    {F_TRIG_SOLVE, "Trig", "SLV", "Solve", ALIAS_TRIG_SOLVE, 1, PROMPTS_NONE, 0},
    {F_TRIG_RW, "Trig", "RW", "Rewrite", ALIAS_TRIG_RW, 2, PROMPTS_NONE, 0},

    {F_SUVAT, "SUVAT", "SUV", "SUVAT", ALIAS_SUVAT, 1, PROMPTS_NONE, 0},

    {F_SHELL, "Shell", "SHL", "Shell", ALIAS_SHELL, 2, PROMPTS_NONE, 0},
};

constexpr int FEATURE_COUNT = sizeof(FEATURES) / sizeof(FEATURES[0]);

const FeatureSpec* find_by_alias(const char* name);
const FeatureSpec* find_by_id(FeatureId id);

}

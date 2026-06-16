#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


CATEGORIES = {
    "CAT_CATEGORY_ALL": 0,
    "CAT_CATEGORY_MECH": 1,
    "CAT_CATEGORY_STATS": 2,
    "CAT_CATEGORY_DATA": 3,
    "CAT_CATEGORY_NUMBER": 4,
    "CAT_CATEGORY_FLOAT": 5,
    "CAT_CATEGORY_STORAGE": 6,
    "CAT_CATEGORY_ALGO": 7,
    "CAT_CATEGORY_BOOL": 8,
}

P3_COMMANDS = [
    ("suvat(u=2,a=3,t=4)", "CAT_CATEGORY_MECH"),
    ("suvat(", "CAT_CATEGORY_MECH"),
    ("projectile(20,30)", "CAT_CATEGORY_MECH"),
    ("projectile(", "CAT_CATEGORY_MECH"),
    ("projectileh(", "CAT_CATEGORY_MECH"),
    ("projectiley(", "CAT_CATEGORY_MECH"),
    ("projectileat(", "CAT_CATEGORY_MECH"),
    ("projectileangle(", "CAT_CATEGORY_MECH"),
    ("force(12,3)", "CAT_CATEGORY_MECH"),
    ("force(", "CAT_CATEGORY_MECH"),
    ("weight(", "CAT_CATEGORY_MECH"),
    ("friction(", "CAT_CATEGORY_MECH"),
    ("incline(5,30,0.2)", "CAT_CATEGORY_MECH"),
    ("incline(", "CAT_CATEGORY_MECH"),
    ("inclineacc(", "CAT_CATEGORY_MECH"),
    ("pulley(3,5)", "CAT_CATEGORY_MECH"),
    ("pulley(", "CAT_CATEGORY_MECH"),
    ("connected(", "CAT_CATEGORY_MECH"),
    ("beam(10,30,4,20)", "CAT_CATEGORY_MECH"),
    ("beam(", "CAT_CATEGORY_MECH"),
    ("moment(", "CAT_CATEGORY_MECH"),
    ("ladder(", "CAT_CATEGORY_MECH"),
    ("varacct(6,-4,0,3,5)", "CAT_CATEGORY_MECH"),
    ("varacct(", "CAT_CATEGORY_MECH"),
    ("varaccx(", "CAT_CATEGORY_MECH"),
    ("vectorkin(", "CAT_CATEGORY_MECH"),
    ("hypbinom(20,0.4,4,0.05,-1)", "CAT_CATEGORY_STATS"),
    ("hypbinom(", "CAT_CATEGORY_STATS"),
    ("critbinom(", "CAT_CATEGORY_STATS"),
    ("binomcrit(", "CAT_CATEGORY_STATS"),
    ("normalprob(40,60,50,10)", "CAT_CATEGORY_STATS"),
    ("normalprob(", "CAT_CATEGORY_STATS"),
    ("normalprobvar(", "CAT_CATEGORY_STATS"),
    ("normaltail(", "CAT_CATEGORY_STATS"),
    ("invnormal(", "CAT_CATEGORY_STATS"),
    ("normalcrit(", "CAT_CATEGORY_STATS"),
    ("normalparams(", "CAT_CATEGORY_STATS"),
    ("samplemean(", "CAT_CATEGORY_STATS"),
    ("samplemeantail(", "CAT_CATEGORY_STATS"),
    ("binom(10,0.4,3)", "CAT_CATEGORY_STATS"),
    ("binom(", "CAT_CATEGORY_STATS"),
    ("binomtail(", "CAT_CATEGORY_STATS"),
    ("binomstats(", "CAT_CATEGORY_STATS"),
    ("binomnorm(", "CAT_CATEGORY_STATS"),
    ("largebinomnormal(", "CAT_CATEGORY_STATS"),
    ("poisson(", "CAT_CATEGORY_STATS"),
    ("poissontail(", "CAT_CATEGORY_STATS"),
    ("poissonstats(", "CAT_CATEGORY_STATS"),
    ("critpoisson(", "CAT_CATEGORY_STATS"),
    ("cond(0.2,0.5)", "CAT_CATEGORY_DATA"),
    ("cond(", "CAT_CATEGORY_DATA"),
    ("probor(", "CAT_CATEGORY_DATA"),
    ("bayes(", "CAT_CATEGORY_DATA"),
    ("independent(", "CAT_CATEGORY_DATA"),
    ("regresscalc(5,20,30,10,18,8)", "CAT_CATEGORY_DATA"),
    ("regresscalc(", "CAT_CATEGORY_DATA"),
    ("groupmean(5,12,15,30,25,18)", "CAT_CATEGORY_DATA"),
    ("groupmean(", "CAT_CATEGORY_DATA"),
    ("histdensity(", "CAT_CATEGORY_DATA"),
    ("groupmedian(", "CAT_CATEGORY_DATA"),
    ("meanvar(", "CAT_CATEGORY_DATA"),
]

CS_COMMANDS = [
    ("convert(45,10,16)", "CAT_CATEGORY_NUMBER"),
    ("convert(", "CAT_CATEGORY_NUMBER"),
    ("base(", "CAT_CATEGORY_NUMBER"),
    ("bin(", "CAT_CATEGORY_NUMBER"),
    ("hex(", "CAT_CATEGORY_NUMBER"),
    ("den(", "CAT_CATEGORY_NUMBER"),
    ("bitsneeded(127)", "CAT_CATEGORY_NUMBER"),
    ("bitsneeded(", "CAT_CATEGORY_NUMBER"),
    ("unsignedrange(", "CAT_CATEGORY_NUMBER"),
    ("twosrange(", "CAT_CATEGORY_NUMBER"),
    ("signmagrange(", "CAT_CATEGORY_NUMBER"),
    ("onesrange(", "CAT_CATEGORY_NUMBER"),
    ("twos(-5,8)", "CAT_CATEGORY_NUMBER"),
    ("twos(", "CAT_CATEGORY_NUMBER"),
    ("twosdec(", "CAT_CATEGORY_NUMBER"),
    ("twosadd(", "CAT_CATEGORY_NUMBER"),
    ("twossub(5,9,8)", "CAT_CATEGORY_NUMBER"),
    ("twossub(", "CAT_CATEGORY_NUMBER"),
    ("signmag(", "CAT_CATEGORY_NUMBER"),
    ("signmagdec(", "CAT_CATEGORY_NUMBER"),
    ("ones(", "CAT_CATEGORY_NUMBER"),
    ("onesdec(", "CAT_CATEGORY_NUMBER"),
    ("binadd(1011,0110,4)", "CAT_CATEGORY_NUMBER"),
    ("binadd(", "CAT_CATEGORY_NUMBER"),
    ("binsub(", "CAT_CATEGORY_NUMBER"),
    ("shift(", "CAT_CATEGORY_NUMBER"),
    ("arithshift(", "CAT_CATEGORY_NUMBER"),
    ("xorbits(", "CAT_CATEGORY_NUMBER"),
    ("andbits(", "CAT_CATEGORY_NUMBER"),
    ("orbits(", "CAT_CATEGORY_NUMBER"),
    ("notbits(", "CAT_CATEGORY_NUMBER"),
    ("parity(", "CAT_CATEGORY_NUMBER"),
    ("repeatenc(", "CAT_CATEGORY_NUMBER"),
    ("repeatdec(", "CAT_CATEGORY_NUMBER"),
    ("hamming(", "CAT_CATEGORY_NUMBER"),
    ("hammingenc(", "CAT_CATEGORY_NUMBER"),
    ("checksum(", "CAT_CATEGORY_NUMBER"),
    ("checkdigit(", "CAT_CATEGORY_NUMBER"),
    ("grayenc(", "CAT_CATEGORY_NUMBER"),
    ("graydec(", "CAT_CATEGORY_NUMBER"),
    ("rpn(", "CAT_CATEGORY_NUMBER"),
    ("fixed(", "CAT_CATEGORY_FLOAT"),
    ("fixedtc(", "CAT_CATEGORY_FLOAT"),
    ("fixedenc(5.625,3,3)", "CAT_CATEGORY_FLOAT"),
    ("fixedenc(", "CAT_CATEGORY_FLOAT"),
    ("fixedfrac(", "CAT_CATEGORY_FLOAT"),
    ("fixedtcenc(", "CAT_CATEGORY_FLOAT"),
    ("floatdec(0101100,11101)", "CAT_CATEGORY_FLOAT"),
    ("floatdec(", "CAT_CATEGORY_FLOAT"),
    ("floatenc(12.75,8,4)", "CAT_CATEGORY_FLOAT"),
    ("floatenc(", "CAT_CATEGORY_FLOAT"),
    ("floatnorm(00011010,0110)", "CAT_CATEGORY_FLOAT"),
    ("floatnorm(", "CAT_CATEGORY_FLOAT"),
    ("normal(", "CAT_CATEGORY_FLOAT"),
    ("floatrange(8,4)", "CAT_CATEGORY_FLOAT"),
    ("floatrange(", "CAT_CATEGORY_FLOAT"),
    ("floatnearest(", "CAT_CATEGORY_FLOAT"),
    ("floatprecision(", "CAT_CATEGORY_FLOAT"),
    ("floatbitsadd(", "CAT_CATEGORY_FLOAT"),
    ("floatcanrepresent(", "CAT_CATEGORY_FLOAT"),
    ("floatadd(", "CAT_CATEGORY_FLOAT"),
    ("floatsub(", "CAT_CATEGORY_FLOAT"),
    ("floatmul(", "CAT_CATEGORY_FLOAT"),
    ("floatdiv(", "CAT_CATEGORY_FLOAT"),
    ("image(800,600,24)", "CAT_CATEGORY_STORAGE"),
    ("image(", "CAT_CATEGORY_STORAGE"),
    ("imagesize(", "CAT_CATEGORY_STORAGE"),
    ("imagecolors(", "CAT_CATEGORY_STORAGE"),
    ("colourdepth(", "CAT_CATEGORY_STORAGE"),
    ("colourcount(", "CAT_CATEGORY_STORAGE"),
    ("sound(44100,60,16,2)", "CAT_CATEGORY_STORAGE"),
    ("sound(", "CAT_CATEGORY_STORAGE"),
    ("soundsize(", "CAT_CATEGORY_STORAGE"),
    ("bitrate(48000000,12)", "CAT_CATEGORY_STORAGE"),
    ("bitrate(", "CAT_CATEGORY_STORAGE"),
    ("transfer(", "CAT_CATEGORY_STORAGE"),
    ("compress(1000,250)", "CAT_CATEGORY_STORAGE"),
    ("compress(", "CAT_CATEGORY_STORAGE"),
    ("rle(", "CAT_CATEGORY_STORAGE"),
    ("chars(120,8)", "CAT_CATEGORY_STORAGE"),
    ("chars(", "CAT_CATEGORY_STORAGE"),
    ("records(1200,32)", "CAT_CATEGORY_STORAGE"),
    ("records(", "CAT_CATEGORY_STORAGE"),
    ("ascii(", "CAT_CATEGORY_STORAGE"),
    ("unicode(", "CAT_CATEGORY_STORAGE"),
    ("symbolbits(", "CAT_CATEGORY_STORAGE"),
    ("sqlselect(", "CAT_CATEGORY_STORAGE"),
    ("sqlcount(", "CAT_CATEGORY_STORAGE"),
    ("hashmod(", "CAT_CATEGORY_STORAGE"),
    ("hashlinear(", "CAT_CATEGORY_STORAGE"),
    ("hashquadratic(", "CAT_CATEGORY_STORAGE"),
    ("addressspace(", "CAT_CATEGORY_STORAGE"),
    ("addressbits(", "CAT_CATEGORY_STORAGE"),
    ("memorycapacity(", "CAT_CATEGORY_STORAGE"),
    ("binarysearch(7,1,3,5,7,9)", "CAT_CATEGORY_ALGO"),
    ("binarysearch(", "CAT_CATEGORY_ALGO"),
    ("linearsearch(", "CAT_CATEGORY_ALGO"),
    ("bubblesort(", "CAT_CATEGORY_ALGO"),
    ("insertionsort(", "CAT_CATEGORY_ALGO"),
    ("selectionsort(", "CAT_CATEGORY_ALGO"),
    ("mergesort(", "CAT_CATEGORY_ALGO"),
    ("stack(", "CAT_CATEGORY_ALGO"),
    ("queue(", "CAT_CATEGORY_ALGO"),
    ("preorder(", "CAT_CATEGORY_ALGO"),
    ("inorder(", "CAT_CATEGORY_ALGO"),
    ("postorder(", "CAT_CATEGORY_ALGO"),
    ("dijkstra(", "CAT_CATEGORY_ALGO"),
    ("fsm(", "CAT_CATEGORY_ALGO"),
    ("fsmout(", "CAT_CATEGORY_ALGO"),
    ("bool_simplify(", "CAT_CATEGORY_BOOL"),
    ("bool(", "CAT_CATEGORY_BOOL"),
    ("truth(", "CAT_CATEGORY_BOOL"),
    ("truthbits(", "CAT_CATEGORY_BOOL"),
    ("minterms(", "CAT_CATEGORY_BOOL"),
    ("maxterms(", "CAT_CATEGORY_BOOL"),
    ("kmap(", "CAT_CATEGORY_BOOL"),
    ("kmapdc(", "CAT_CATEGORY_BOOL"),
    ("posform(", "CAT_CATEGORY_BOOL"),
    ("nandform(", "CAT_CATEGORY_BOOL"),
    ("norform(", "CAT_CATEGORY_BOOL"),
    ("boolprove(", "CAT_CATEGORY_BOOL"),
    ("demorgan(", "CAT_CATEGORY_BOOL"),
    ("nand(", "CAT_CATEGORY_BOOL"),
]

P3_FOLDERS = [
    ("Mechanics", "CAT_CATEGORY_MECH"),
    ("Statistics", "CAT_CATEGORY_STATS"),
    ("Data/prob", "CAT_CATEGORY_DATA"),
]

CS_FOLDERS = [
    ("Number", "CAT_CATEGORY_NUMBER"),
    ("Float/fixed", "CAT_CATEGORY_FLOAT"),
    ("Storage", "CAT_CATEGORY_STORAGE"),
    ("Algorithms", "CAT_CATEGORY_ALGO"),
    ("Boolean", "CAT_CATEGORY_BOOL"),
]

CS_BOOL_COMMANDS = [
    "bool_simplify(expression)",
    "nandform(expression)",
    "norform(expression)",
    "boolprove(lhs,rhs)",
]


def emit(app: str) -> str:
    if app == "p3":
        commands, folders, title, about = P3_COMMANDS, P3_FOLDERS, "CASP3 Catalog", "CASP3"
    elif app == "cs":
        commands = [c for c in CS_COMMANDS if c[1] == "CAT_CATEGORY_BOOL"]
        folders, title, about = [("Boolean", "CAT_CATEGORY_BOOL")], "CSCalc Catalog", "CSCalc"
    else:
        raise SystemExit("usage: khicas_suite_catalog.py p3|cs")

    cat_defs = "\n".join(f"#define {name} {value}" for name, value in CATEGORIES.items())
    def esc(s: str) -> str:
        return s.replace("\\", "\\\\").replace('"', '\\"')

    def base_name(s: str) -> str:
        return s.split("(", 1)[0]

    def insert_text(s: str) -> str:
        b = base_name(s)
        return b + "(" if "(" in s else b

    def desc(s: str) -> str:
        b = base_name(s)
        if app == "p3":
            if b in {"suvat", "varacct", "varaccx", "vectorkin", "vectorsuvat", "vectormotion"}:
                return "Mechanics working: formula, substitution, answer."
            if b.startswith("projectile"):
                return "Projectile working with components and suvat."
            if b in {"force", "weight", "friction", "incline", "inclineacc", "pulley", "connected"}:
                return "Forces working with Newton's second law."
            if b in {"beam", "moment", "ladder"}:
                return "Moments/equilibrium working."
            if b in {"hypbinom", "critbinom", "binomcrit"}:
                return "Binomial hypothesis test working and conclusion."
            if b.startswith("normal") or b.startswith("sample") or b == "invnormal":
                return "Normal distribution setup and standardisation."
            if b.startswith("binom") or b.startswith("poisson"):
                return "Discrete distribution setup and probability."
            return "Stats/data working with formula and substitution."
        if b in {"convert", "base", "bin", "hex", "den", "bitsneeded"}:
            return "Number-base working."
        if "twos" in b or b in {"signmag", "signmagdec", "ones", "onesdec", "binadd", "binsub", "shift", "arithshift", "xorbits", "andbits", "orbits", "notbits", "parity", "repeatenc", "repeatdec", "hamming", "hammingenc", "checksum", "checkdigit", "grayenc", "graydec", "rpn"}:
            return "Binary integer working with range/overflow where needed."
        if b.startswith("fixed") or b.startswith("float") or b == "normal":
            return "Fixed/floating point working."
        if b in {"image", "imagesize", "imagecolors", "colourdepth", "colourcount", "sound", "soundsize", "bitrate", "transfer", "compress", "rle", "chars", "records", "ascii", "unicode", "symbolbits", "sqlselect", "sqlcount", "hashmod", "hashlinear", "hashquadratic", "addressspace", "addressbits", "memorycapacity"}:
            return "Storage/data-size working."
        if b in {"bool", "bool_simplify", "truth", "truthbits", "minterms", "maxterms", "kmap", "kmapdc", "posform", "nandform", "norform", "boolprove", "demorgan", "nand"}:
            return "Boolean algebra working with rule names."
        return "Algorithm/data-structure working."

    def example(s: str) -> str:
        if "(" in s and ")" in s:
            return "#" + s
        examples = {
            "suvat(": "suvat(u=2,a=3,t=4)",
            "projectile(": "projectile(20,30)",
            "force(": "force(12,3)",
            "incline(": "incline(5,30,0.2)",
            "pulley(": "pulley(3,5)",
            "beam(": "beam(10,30,4,20)",
            "varacct(": "varacct(6,-4,0,3,5)",
            "hypbinom(": "hypbinom(20,0.4,4,0.05,-1)",
            "normalprob(": "normalprob(40,60,50,10)",
            "binom(": "binom(10,0.4,3)",
            "cond(": "cond(0.2,0.5)",
            "regresscalc(": "regresscalc(5,20,30,10,18,8)",
            "groupmean(": "groupmean(5,12,15,30,25,18)",
            "convert(": "convert(45,10,16)",
            "bitsneeded(": "bitsneeded(127)",
            "twos(": "twos(-5,8)",
            "twossub(": "twossub(5,9,8)",
            "binadd(": "binadd(1011,0110,4)",
            "fixedenc(": "fixedenc(5.625,3,3)",
            "floatdec(": "floatdec(0101100,11101)",
            "floatenc(": "floatenc(12.75,8,4)",
            "floatnorm(": "floatnorm(00011010,0110)",
            "floatrange(": "floatrange(8,4)",
            "image(": "image(800,600,24)",
            "sound(": "sound(44100,60,16,2)",
            "bitrate(": "bitrate(48000000,12)",
            "compress(": "compress(1000,250)",
            "chars(": "chars(120,8)",
            "records(": "records(1200,32)",
            "binarysearch(": "binarysearch(7,1,3,5,7,9)",
            "bool_simplify(": "bool_simplify(A and not B)",
            "truthbits(": "truthbits(A,B,0110)",
            "minterms(": "minterms(A,B,C,1,3,7)",
            "maxterms(": "maxterms(A,B,0,2)",
            "kmap(": "kmap(A,B,C,1,3,7)",
            "kmapdc(": "kmapdc(A,B,C,1,3,dc,0)",
            "posform(": "posform(A+B')",
            "nandform(": "nandform(A+B)",
            "norform(": "norform(A*B)",
            "boolprove(": "boolprove((A*B)',A'+B')",
        }
        return "#" + examples.get(s, insert_text(s) + ")")

    def detail(s: str) -> tuple[str, str, str, str, str]:
        b = base_name(s)
        ex = example(s)
        if ex.startswith("#"):
            ex = ex[1:]
        if app == "p3":
            if b == "suvat":
                return ("suvat(u=...,v=...,a=...,t=...,s=...)",
                        "Give any 3 SUVAT values as named arguments.",
                        "Finds every missing constant-acceleration value.",
                        "Selects a valid SUVAT formula, substitutes, solves, then states answer lines.",
                        "suvat(u=2,a=3,t=4)")
            if b.startswith("projectile"):
                return ("projectile(speed,angle[,g]) or projectileh/s/at variants",
                        "Angle in degrees. Optional g defaults to 9.8.",
                        "Projectile components, time, range, height, or value at time.",
                        "Resolves velocity into horizontal/vertical components then applies SUVAT.",
                        "projectile(20,30), projectileat(20,30,2)")
            if b in {"force", "weight", "friction", "incline", "inclineacc", "pulley", "connected"}:
                return (f"{b}(...)",
                        "Use masses, angles, coefficients, forces or acceleration as shown by the prompt.",
                        "Mechanics force working.",
                        "Writes Newton's second law or friction/weight rule, substitutes, then solves.",
                        ex)
            if b in {"beam", "moment", "ladder"}:
                return (f"{b}(...)",
                        "Forces and distances from a pivot.",
                        "Moment/equilibrium working.",
                        "Uses clockwise moment = anticlockwise moment and resolves reactions where needed.",
                        ex)
            if b.startswith("normal") or b in {"invnormal", "samplemean", "samplemeantail"}:
                return (f"{b}(...)",
                        "Bounds/probability, mean, standard deviation, and sample size where needed.",
                        "Normal distribution setup.",
                        "Standardises with z=(x-mu)/sigma; sample mean uses sigma/sqrt(n).",
                        ex)
            if b.startswith("binom") or b in {"hypbinom", "critbinom", "binomcrit"}:
                return (f"{b}(...)",
                        "n, p, value/tail/alpha depending on the command.",
                        "Binomial probability or hypothesis-test working.",
                        "States X~B(n,p), applies nCr p^r(1-p)^(n-r), sums tails or compares alpha.",
                        ex)
            return (f"{b}(...)",
                    "Use the parameters shown in the example.",
                    desc(s),
                    "Writes formula/setup, substitutes values, then gives final answer.",
                    ex)
        if b in {"convert", "base"}:
            return ("convert(value,from_base,to_base)",
                    "Bases can be 2, 8, 10 or 16.",
                    "Converts number bases.",
                    "Converts to denary by place value, then converts to target base.",
                    ex)
        if b in {"bin", "hex", "den"}:
            return (f"{b}(value[,base])",
                    "Value and optional source base.",
                    "Shortcut base conversion.",
                    "Uses place-value expansion then groups/divides for the target base.",
                    ex)
        if "twos" in b or b in {"signmag", "signmagdec", "ones", "onesdec"}:
            return (f"{b}(value_or_bits[,bits])",
                    "Signed value plus bit width, or encoded bit pattern.",
                    "Signed binary encode/decode.",
                    "Uses sign bit/range rule; two's-complement negatives use 2^n+value.",
                    ex)
        if b in {"binadd", "binsub", "shift", "arithshift", "xorbits", "andbits", "orbits", "notbits"}:
            return (f"{b}(bits[,bits_or_amount,width])",
                    "Bit strings, direction/amount or optional width.",
                    "Binary arithmetic/logic.",
                    "Shows aligned operation, carry/borrow or sign-preserving shift.",
                    ex)
        if b in {"parity", "repeatenc", "repeatdec", "hamming", "hammingenc", "checksum", "checkdigit", "grayenc", "graydec"}:
            return (f"{b}(...)",
                    "Bit strings and scheme-specific options.",
                    "Error-detection/coding working.",
                    "Calculates parity/check bits or applies the stated code rule step by step.",
                    ex)
        if b.startswith("fixed"):
            return (f"{b}(...)",
                    "Fixed-point bits or value plus whole/fraction bit counts.",
                    "Fixed-point encode/decode.",
                    "Uses binary place values either side of the point.",
                    ex)
        if b.startswith("float") or b == "normal":
            return (f"{b}(...)",
                    "Mantissa/exponent bits, widths, or a denary value.",
                    "Floating-point encode/decode/check.",
                    "Decodes exponent as two's complement, shifts point, normalises, then rounds/checks.",
                    ex)
        if b in {"image", "imagesize", "imagecolors", "colourdepth", "colourcount", "sound", "soundsize", "bitrate", "transfer", "compress", "rle", "chars", "records", "ascii", "unicode", "symbolbits"}:
            return (f"{b}(...)",
                    "Counts, dimensions, bit depths, rates or file sizes.",
                    "Storage/data-size working.",
                    "Multiplies the relevant counts, converts units, then states final size/time/ratio.",
                    ex)
        if b in {"sqlselect", "sqlcount", "hashmod", "hashlinear", "hashquadratic", "addressspace", "addressbits", "memorycapacity"}:
            return (f"{b}(...)",
                    "Table/query fields, hash keys or memory/address sizes.",
                    "Database/memory calculation working.",
                    "Applies the exact selection/hash/address formula and shows each step.",
                    ex)
        if b in {"binarysearch", "linearsearch", "bubblesort", "insertionsort", "selectionsort", "mergesort", "stack", "queue", "preorder", "inorder", "postorder", "dijkstra", "fsm", "fsmout", "rpn"}:
            return (f"{b}(...)",
                    "Target/list, operations, graph edges or state transitions.",
                    "Algorithm trace.",
                    "Shows comparisons/passes/updates in exam trace order.",
                    ex)
        if b in {"bool", "bool_simplify", "truth", "truthbits", "minterms", "maxterms", "kmap", "kmapdc", "posform", "nandform", "norform", "boolprove", "demorgan", "nand"}:
            return (f"{b}(expression[,variables])",
                    "Boolean expressions use and/or/not, +, *, ', brackets. Variable list is optional.",
                    "Boolean simplification, proof, table, K-map or gate form.",
                    "Applies named laws first, including bracket expansion, then exact truth-table/K-map grouping if shorter.",
                    ex)
        return (f"{b}(...)",
                "Use the parameters shown in the example.",
                desc(s),
                "Shows setup, substitution/trace and final answer.",
                ex)

    command_rows = "\n".join(
        f'  {{(char*)"{esc(name)}", (char*)"{esc(insert_text(name))}", (char*)"{esc(desc(name))}", (char*)"{esc(example(name))}", 0, {cat}}},'
        for name, cat in commands
    )
    help_rows = "\n".join(
        f'  {{(char*)"{esc(detail(name)[0])}", (char*)"{esc(detail(name)[1])}", (char*)"{esc(detail(name)[2])}", (char*)"{esc(detail(name)[3])}", (char*)"{esc(detail(name)[4])}"}},'
        for name, _cat in commands
    )
    folder_rows = "\n".join(f'  {{(char*)"{name}", {cat}}},' for name, cat in folders)
    return f'''#include <stdio.h>
#include <fxcg/keyboard.h>
#include "giacPCH.h"
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
extern "C" {{
#include <fxcg/rtc.h>
}}
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>

#include "history.h"
#include "dConsole.h"
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "kdisplay.h"
#include "input_lexer.h"
#include "input_parser.h"
#include "graphicsProvider.hpp"
#define STATUS_AREA_PX 24

{cat_defs}

using namespace giac;
extern giac::context * contextptr;
void reset_alpha(){{
  SetSetupSetting( (unsigned int)0x14, 0);
  DisplayStatusArea();
}}

int lang=0;
const catalogFunc completeCat[] = {{
{command_rows}
}};

typedef struct {{
  char* syntax;
  char* args;
  char* does;
  char* work;
  char* examples;
}} catalogHelp;

const catalogHelp completeHelp[] = {{
{help_rows}
}};

typedef struct {{
  char* title;
  int category;
}} catalogFolder;

const catalogFolder catalogFolders[] = {{
{folder_rows}
}};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="{title}";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="Use catalog for commands.";
const char apropos_string[]="{about}";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);
int CAT_FOLDER_COUNT=sizeof(catalogFolders)/sizeof(catalogFolder);

ustl::string insert_string(int index){{
  return completeCat[index].insert;
}}

int showCatalog(char* insertText,int preselect,int menupos) {{
  return doCatalogMenu(insertText, (char*)"{title}", CAT_CATEGORY_ALL);
}}

static int catalog_count_for_category(int category){{
  if (category==CAT_CATEGORY_ALL)
    return CAT_FOLDER_COUNT;
  int n=0;
  for (int i=0;i<CAT_COMPLETE_COUNT;++i)
    if (completeCat[i].category==category)
      ++n;
  return n;
}}

static void draw_catalog_fkeys(bool folders){{
  drawFkeyLabels(folders?0:0x03FC,folders?0:0x03fc,0,0,0,folders?0:0x03FD);
}}

int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {{
  int allcmds=CAT_COMPLETE_COUNT;
  int nitems = catalog_count_for_category(category);
  if (nitems<=0)
    return 0;
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,menusel=-1;
  if (category==CAT_CATEGORY_ALL){{
    while(cur<CAT_FOLDER_COUNT) {{
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = TEXT_COLOR_BLUE;
      menuitems[curmi].isfolder = cur+1;
      menuitems[curmi].text = catalogFolders[cur].title;
      curmi++;
      cur++;
    }}
  }}
  else {{
    while(cur<allcmds) {{
      if (completeCat[cur].category==category){{
        menuitems[curmi].type = MENUITEM_NORMAL;
        menuitems[curmi].color = TEXT_COLOR_BLACK;
        menuitems[curmi].isfolder = cur;
        menuitems[curmi].text = completeCat[cur].name;
        curmi++;
      }}
      cur++;
    }}
  }}

  Menu menu;
  if (menusel>=0)
    menu.selection=menusel+1;
  menu.items=menuitems;
  menu.numitems=curmi;
  if (curmi>=100)
    SetSetupSetting( (unsigned int)0x14, 0x88);
  DisplayStatusArea();
  menu.scrollout=1;
  menu.title = title;
  menu.type = category==CAT_CATEGORY_ALL ? MENUTYPE_NORMAL : MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {{
    draw_catalog_fkeys(category==CAT_CATEGORY_ALL);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){{
      reset_alpha();
      return 0;
    }}
    int index=menuitems[menu.selection-1].isfolder;
    if(category!=CAT_CATEGORY_ALL && sres == KEY_CTRL_F6) {{
      textArea text;
      text.editable=false;
      text.clipline=-1;
      text.title = (char*)"Command help";
      text.allowF1=true;
      text.python=python_compat(contextptr);
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement>(6);
      elem[0].s = completeCat[index].name;
      elem[0].color = COLOR_BLUE;
      elem[1].newLine = 1;
      elem[1].lineSpacing = 3;
      elem[1].s = ustl::string("Syntax: ") + completeHelp[index].syntax;
      elem[2].newLine = 1;
      elem[2].lineSpacing = 3;
      elem[2].s = ustl::string("Inputs: ") + completeHelp[index].args;
      elem[3].newLine = 1;
      elem[3].lineSpacing = 3;
      elem[3].s = ustl::string("Does: ") + completeHelp[index].does;
      elem[4].newLine = 1;
      elem[4].lineSpacing = 3;
      elem[4].s = ustl::string("Method: ") + completeHelp[index].work;
      elem[5].newLine = 1;
      elem[5].lineSpacing = 3;
      elem[5].s = ustl::string("Examples: ") + completeHelp[index].examples;
      sres=doTextArea(&text);
    }}
    if(category!=CAT_CATEGORY_ALL && sres == KEY_CTRL_F2) {{
      reset_alpha();
      char * example=completeCat[index].example;
      strcpy(insertText, example && example[0]=='#' ? example+1 : insert_string(index).c_str());
      return 1;
    }}
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {{
      if (category==CAT_CATEGORY_ALL){{
        int folder=index-1;
        if (folder>=0 && folder<CAT_FOLDER_COUNT){{
          if (doCatalogMenu(insertText,catalogFolders[folder].title,catalogFolders[folder].category))
            return 1;
          DisplayStatusArea();
        }}
        continue;
      }}
      reset_alpha();
      strcpy(insertText,index<allcmds?insert_string(index).c_str():menuitems[menu.selection-1].text);
      return 1;
    }}
  }}
  return 0;
}}
'''


def patch_cs_catalog(path: Path) -> None:
    text = path.read_text()
    category = "CAT_CATEGORY_COMPUTER_SCIENCE"
    if category not in text:
        marker = "#define CAT_CATEGORY_LOGO 20 // should be the last one\n"
        if marker not in text:
            raise SystemExit("catalog category marker missing")
        text = text.replace(marker, marker + f"#define {category} 21\n", 1)

    if '"bool_simplify(expression)"' not in text:
        insert = "".join(f'  {{"{name}", {category}}},\n' for name in CS_BOOL_COMMANDS)
        needle = "\n\n};\n\ntypedef struct {\n  char* title;\n  int category;\n} catalogFolder;\n"
        if needle not in text:
            raise SystemExit("completeCat terminator missing")
        text = text.replace(needle, "\n" + insert + needle, 1)

    if '"Computer Science"' not in text:
        needle = '  {(char*)"Vectors", CAT_CATEGORY_VECTOR},\n'
        if needle not in text:
            raise SystemExit("catalog folder insertion point missing")
        text = text.replace(needle, needle + f'  {{(char*)"Computer Science", {category}}},\n', 1)

    path.write_text(text)


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: khicas_suite_catalog.py p3|cs output")
    out = Path(sys.argv[2])
    if sys.argv[1] == "cs":
        patch_cs_catalog(out)
    else:
        out.write_text(emit(sys.argv[1]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

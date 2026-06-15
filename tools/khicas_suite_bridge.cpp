#include "khicas_suite_bridge.hpp"
#include "giacPCH.h"
#include "console.h"
#include "input_lexer.h"
#include "input_parser.h"
#include "prog.h"

#if defined(SUITE_APP_P3)
#include "p3_engine.hpp"
#elif defined(SUITE_APP_CS)
#include "cscalc_engine.hpp"
#endif

#include <string.h>

static const char *const p3_commands[] = {
    "suvat", "projectile", "projectileh", "projectiley", "projectileat",
    "projectileangle", "force", "weight", "friction", "incline",
    "inclineacc", "pulley", "connected", "beam", "moment", "ladder",
    "varacct", "varaccx", "hypbinom", "critbinom", "binomcrit",
    "normalprob", "normalcrit", "samplemean", "samplemeantail", "binom",
    "binomtail", "binomnorm", "largebinomnormal", "poisson", "poissontail",
    "cond", "probor", "regresscalc", "groupmean", "histdensity", 0};

static const char *const cs_commands[] = {
    "convert", "base", "bin", "hex", "den", "bitsneeded", "unsignedrange",
    "twosrange", "signmagrange", "onesrange", "twos", "twosdec", "twosadd",
    "twossub", "signmag", "signmagdec", "ones", "onesdec", "binadd",
    "binsub", "shift", "arithshift", "fixed", "fixedtc", "fixedenc",
    "fixedfrac", "fixedtcenc", "floatdec", "floatenc", "floatnorm",
    "floatrange", "floatnearest", "image", "imagesize", "sound",
    "soundsize", "bitrate", "compress", "chars", "records", "ascii",
    "binarysearch", "linearsearch", "bubblesort", "insertionsort",
    "selectionsort", "mergesort", "stack", "queue", "preorder", "inorder",
    "postorder", "dijkstra", "bool", "truth", "demorgan", "nand", 0};

static const char p3_fmenu[] =
    "F1 mech\n"
    "suvat(\nprojectile(\nforce(\nincline(\npulley(\nbeam(\nvaracct(\n"
    "F2 stat\n"
    "hypbinom(\ncritbinom(\nnormalprob(\nnormalcrit(\nbinom(\nbinomtail(\npoisson(\n"
    "F3 data\n"
    "cond(\nprobor(\nregresscalc(\ngroupmean(\nhistdensity(\n"
    "F4 cmds\n"
    "suvat(\nprojectile(\nhypbinom(\nnormalprob(\nregresscalc(\n"
    "F5 A<>a\n"
    "F6 more\n"
    "catalog\nhelp(\nrestart\n";

static const char cs_fmenu[] =
    "F1 num\n"
    "convert(\nbitsneeded(\ntwos(\ntwosdec(\nbinadd(\ntwossub(\nshift(\n"
    "F2 float\n"
    "fixedenc(\nfixed(\nfloatdec(\nfloatenc(\nfloatnorm(\nfloatrange(\n"
    "F3 store\n"
    "image(\nsound(\nbitrate(\ncompress(\nchars(\nrecords(\n"
    "F4 alg\n"
    "binarysearch(\nlinearsearch(\nbubblesort(\ninsertionsort(\nmergesort(\n"
    "F5 bool\n"
    "bool(\ntruth(\ndemorgan(\nnand(\n"
    "F6 more\n"
    "catalog\nhelp(\nrestart\n";

const char *suite_console_fmenu_config() {
#if defined(SUITE_APP_P3)
  return p3_fmenu;
#elif defined(SUITE_APP_CS)
  return cs_fmenu;
#else
  return 0;
#endif
}

int suite_custom_fkey_label(int fkey) {
  (void)fkey;
  return 0;
}

void suite_register_lexer_symbols() {
#if defined(SUITE_APP_P3)
  const char *const *names = p3_commands;
#elif defined(SUITE_APP_CS)
  const char *const *names = cs_commands;
#else
  const char *const *names = 0;
#endif
  if (!names) return;
  for (int i = 0; names[i]; ++i)
    giac::lexer_functions_register(giac::at_nop, names[i], T_UNARY_OP);
}

static void suite_output_lines(const char *const *lines, int count) {
  for (int i = 0; i < count; ++i) {
    if (i) Console_NewLine(LINE_TYPE_OUTPUT, 1);
    Console_Output((const unsigned char *)lines[i]);
  }
}

bool suite_eval_with_working(const char *expr) {
  if (!expr || !*expr) return false;
#if defined(SUITE_APP_P3)
  char lines[P3_MAX_LINES][P3_LINE_LEN];
  int count = p3_eval(expr, lines);
  const char *ptrs[P3_MAX_LINES];
  for (int i = 0; i < count; ++i) ptrs[i] = lines[i];
  suite_output_lines(ptrs, count);
  return true;
#elif defined(SUITE_APP_CS)
  char lines[CSCALC_MAX_LINES][CSCALC_LINE_LEN];
  int count = cscalc_eval(expr, lines);
  const char *ptrs[CSCALC_MAX_LINES];
  for (int i = 0; i < count; ++i) ptrs[i] = lines[i];
  suite_output_lines(ptrs, count);
  return true;
#else
  return false;
#endif
}

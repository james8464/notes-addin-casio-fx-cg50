#include "khicas_suite_bridge.hpp"
#include "giacPCH.h"
#include "console.h"

#if defined(SUITE_APP_P3)
#include "p3_engine.hpp"
#elif defined(SUITE_APP_CS)
#include "cscalc_engine.hpp"
#endif

#include <string.h>

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

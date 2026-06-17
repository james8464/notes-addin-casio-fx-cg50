#include "p3_engine.hpp"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  char input[256] = {0};
  if (argc > 1) {
    for (int i = 1; i < argc; ++i) {
      if (i > 1) strncat(input, " ", sizeof(input) - strlen(input) - 1);
      strncat(input, argv[i], sizeof(input) - strlen(input) - 1);
    }
  } else if (!fgets(input, sizeof(input), stdin)) return 1;
  char lines[P3_MAX_LINES][P3_LINE_LEN];
  int n = p3_eval(input, lines);
  for (int i = 0; i < n; ++i) puts(lines[i]);
  return 0;
}

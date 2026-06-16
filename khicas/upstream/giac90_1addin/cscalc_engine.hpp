#ifndef CSCALC_ENGINE_HPP
#define CSCALC_ENGINE_HPP

#define CSCALC_MAX_LINES 96
#define CSCALC_LINE_LEN 80

int cscalc_eval(const char *input, char out[CSCALC_MAX_LINES][CSCALC_LINE_LEN]);

#endif

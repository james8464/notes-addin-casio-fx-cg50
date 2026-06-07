#ifndef CASCAS_WORKING_H
#define CASCAS_WORKING_H

#include <string>

namespace cascas {
#ifdef CASCAS_HOST_STD_STRING
typedef std::string working_string;
#else
typedef ustl::string working_string;
#endif

typedef bool (*khicas_eval_callback)(const char *expr, working_string &out);
void set_khicas_eval_callback(khicas_eval_callback cb);
bool eval_with_working(const char *input, working_string &out);
bool fallback_working(const char *input, working_string &out);
bool reject_removed_feature(const char *input);
}

#endif

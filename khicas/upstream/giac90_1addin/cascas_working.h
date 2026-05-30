#ifndef CASCAS_WORKING_H
#define CASCAS_WORKING_H

#include <string>

namespace cascas {
#ifdef CASCAS_HOST_STD_STRING
typedef std::string working_string;
#else
typedef ustl::string working_string;
#endif

bool eval_with_working(const char *input, working_string &out);
}

#endif

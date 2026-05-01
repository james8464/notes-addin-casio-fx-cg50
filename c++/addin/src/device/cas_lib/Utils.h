#ifndef CASIO_CAS_UTILS_H
#define CASIO_CAS_UTILS_H

#include <string>
#include <map>

namespace casio::cas_lib {

int opPrecedence(char op);

bool endOfString(int i, const std::string* s);

bool isOperator(char c);

bool isOperator(const std::string *s);

std::string formatDouble(double d, int maxPrecision);

std::string removeSpaces(const std::string* s);

} // namespace casio::cas_lib

#endif //CASIO_CAS_UTILS_H

#ifndef CASIO_CAS_ARITHMETIC_H
#define CASIO_CAS_ARITHMETIC_H

#include <string>
#include <map>

#include "Term.h"
#include "Expression.h"

namespace casio::cas_lib {

bool canAdd(Term* one, Term* two);

bool canExponentiate(Term* one, Term* two);

bool operable(Term* one, Term* two, const std::string* op);

std::map<char, double> addVariables(Term* one, Term* two);

std::map<char, double> subtractVariables(Term* one, Term* two);

Term *operate(Term *one, Term *two, const std::string *op);

Term* addTerms(Term* one, Term* two);

Term* multiplyTerms(Term* one, Term* two);

Term* divideTerms(Term* one, Term* two);

Term* pow(Term* one, Term* two);

} // namespace casio::cas_lib

#endif //CASIO_CAS_ARITHMETIC_H

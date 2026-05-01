#ifndef CASIO_CAS_EXPRESSION_H
#define CASIO_CAS_EXPRESSION_H

#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <iostream>

#include "Term.h"

namespace casio::cas_lib {

class Expression {
private:
    bool simplified = false;

    std::map<std::string, std::stack<Term*>> sortedTerms;

    void evaluate(std::queue<std::string> *tokens);

public:
    Expression(std::string input);

    Expression() { } ;

    ~Expression();

    void insertTerm(Term* t);

    void insertTerms(std::vector<Term*>* terms);

    void removeTerms(const std::string variables);

    void simplify();

    void multiply(Expression* e);

    std::map<std::string, std::stack<Term*>> getTerms() { return sortedTerms; } ;

    std::string toString();

    void print() { std::cout << toString() << std::endl; } ;

};

} // namespace casio::cas_lib

#endif //CASIO_CAS_EXPRESSION_H

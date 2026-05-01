#ifndef CASIO_CAS_EQUATION_H
#define CASIO_CAS_EQUATION_H

#include <string>
#include <iostream>

#include "Expression.h"

namespace casio::cas_lib {

class Equation {
private:
    Expression* left;
    Expression* right;

public:
    Equation(std::string input);

    ~Equation();

    std::string toString();

    void solveFor(char variable);

    void print() { std::cout << toString() << std::endl; };

    Expression* getLeft() { return left; }
    Expression* getRight() { return right; }
};

} // namespace casio::cas_lib

#endif //CASIO_CAS_EQUATION_H

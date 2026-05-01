#include "Equation.h"
#include "Arithmetic.h"

#define string std::string

namespace casio::cas_lib {

Equation::Equation(string input) {
    int eqIndex = input.find('=');
    left = new Expression(input.substr(0, eqIndex));
    right = new Expression(input.substr(eqIndex + 1, input.length() - (eqIndex + 1)));
}

Equation::~Equation() {
    delete left;
    delete right;
}

void Equation::solveFor(char variable) {
    left->simplify();

    for (auto iter : left->getTerms()) {
        if (iter.first.find(variable) != string::npos) {
            Term* negOne = new Term(-1, nullptr);
            right->insertTerm(multiplyTerms(iter.second.top(), negOne));
            delete negOne;

            Term* t = iter.second.top();
            iter.second.pop();
            left->removeTerms(iter.first);
        }
    }

    right->simplify();

    for (auto iter : right->getTerms()) {
        if (iter.first.find(variable) != string::npos) {
            Term* negOne = new Term(-1, nullptr);
            left->insertTerm(multiplyTerms(iter.second.top(), negOne));
            delete negOne;

            Term* t = iter.second.top();
            iter.second.pop();
            right->removeTerms(iter.first);
        }
    }

    left->simplify();

    Term* one = new Term(1, nullptr);
    auto* multiplicand = new Expression;
    multiplicand->insertTerm(divideTerms(one, new Term(left->getTerms().begin()->second.top()->getCoefficient(), nullptr)));

    right->multiply(multiplicand);
    left->multiply(multiplicand);

    delete one;
    delete multiplicand;
}

string Equation::toString() {
    string result;

    result.append(left->toString());
    result.append("=");
    result.append(right->toString());

    return result;
}

} // namespace casio::cas_lib

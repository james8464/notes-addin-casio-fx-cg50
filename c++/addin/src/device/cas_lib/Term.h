#ifndef CASIO_CAS_TERM_H
#define CASIO_CAS_TERM_H

#include <map>
#include <string>

namespace casio::cas_lib {

class Term {
private:
    double coefficient;
    std::map<char, double> variables;
    const int MAX_DECIMAL_PRECISION = 6;

public:
    Term(double value, std::map<char, double> *variables);

    double getCoefficient() { return coefficient; }

    std::map<char, double> getVariables() { return variables; }

    void setCoefficient(const double val) { coefficient = val; }

    std::string toString();

    bool varsEqual(Term* t);

    bool equals(Term* t);

    bool noVars() { return variables.empty(); }

    std::map<char, double> copyVariables();

    static Term* parseTerm(const std::string* s) ;

    std::string varsToString();
};

} // namespace casio::cas_lib

#endif //CASIO_CAS_TERM_H

#include "Term.h"
#include "Utils.h"

#define string std::string
#define map std::map

namespace casio::cas_lib {

Term::Term(double value, map<char, double> *variables) {
    this->coefficient = value;
    if (variables != nullptr) this->variables = *variables;
}

string Term::toString() {
    string result;

    result += formatDouble(coefficient, MAX_DECIMAL_PRECISION);
    result += varsToString();

    return result;
}

string Term::varsToString() {
    string varString;
    if (variables.empty()) return varString;

    for (auto & variable : variables) {
        varString += variable.first;
        varString += "^(";
        varString += formatDouble(variable.second,  MAX_DECIMAL_PRECISION);
        varString += ")";
    }

    return varString;
}

bool Term::varsEqual(Term* t) {
    map<char, double> twoVars = t->getVariables();

    if (variables.size() != twoVars.size()) return false;

    for (auto & iter : variables) {
        auto var = twoVars.find(iter.first);

        if (var == twoVars.end()) return false;
        if (var->second != variables.find(iter.first)->second) return false;
    }

    for (auto & iter : twoVars) {
        auto var = variables.find(iter.first);

        if (var == variables.end()) return false;
        if (var->second != twoVars.find(iter.first)->second) return false;
    }

    return true;
}

bool Term::equals(Term* t) {
    return (coefficient == t->getCoefficient() && varsEqual(t));
}

map<char, double> Term::copyVariables() {
    map<char, double> copy;

    for (auto iter : variables) {
        copy.emplace(iter.first, iter.second);
    }

    return copy;
}

Term* Term::parseTerm(const string *s) {
    Term* term;

    map<char, double> variables;

    bool parsingVars = false;
    int partition = 0;
    for (int i = 0, j = 0; i < s->length(); i++) {
        char c = s->at(i);

        // If at the end of the term
        if (i == s->length() - 1) {
            if (isalpha(c)) {
                j = i + 1;
                variables.emplace(c, 1);
            }

            if (parsingVars) {
                // Add the last variable to the map
                string exponent = s->substr(j + 3, (i - (j + 3)));
                variables.emplace(s->at(j), std::stod(exponent));

                term = (partition == 0) ? new Term(1, &variables) : new Term(std::stod(s->substr(0, partition)), &variables);
            } else term = new Term(std::stod(*s), &variables);
        }

        // If at the beginning of the variables
        if (!parsingVars && isalpha(c)) {
            j = i;
            partition = j;
            parsingVars = true;
            continue;
        }

        // If a variable is encountered
        if (parsingVars) {
            // j is the index of the current variable, so add 3 to account for the caret and open parenthesis
            // i is the index of the next variable, so subtract 1 to get the index of the closing parenthesis
            if (isalpha(c)) {
                string exponent = s->substr(j + 3, (i - (j + 3) - 1));
                variables.emplace(s->at(j), std::stod(exponent));

                j = i;
            }

        }
    }

    return term;
}

} // namespace casio::cas_lib

#include "Utils.h"

#define string std::string

namespace casio::cas_lib {

int opPrecedence(char op) {
    if (op == '+') return 1;
    if (op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    if (op == '(' || op == ')') return 4;

    return -1;
}

bool isOperator(const char c) {
    return opPrecedence(c) != -1;
}

bool isOperator(const string *s) {
    return isOperator(s->at(0));
}

bool endOfString(int i, const string* s) {
    return (i == s->length() - 1);
}

string removeSpaces(const string* s) {
    string result;

    for (char c : *s) {
        if (c != ' ') result += c;
    }

    return result;
}

string formatDouble(double d, int maxPrecision) {
    string formatted;

    string toString = std::to_string(d);
    int decimal = toString.find('.');

    if (maxPrecision + decimal > toString.length()) formatted = toString;
    else formatted = toString.substr(0, decimal + maxPrecision + 1);

    // Shed unnecessary trailing zeroes
    for (int i = (int) formatted.length() - 1; i >= 0; i--) {
        if (formatted.at(i) != '0') break;
        else formatted = formatted.substr(0, formatted.length() - 1);
    }

    // If all zeroes following the decimal were superfluous, remove the decimal as well
    if (formatted.at(formatted.length() - 1) == '.') 
        formatted = formatted.substr(0, formatted.length() - 1);

    return formatted;
}

} // namespace casio::cas_lib

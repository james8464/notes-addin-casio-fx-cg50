#include <stack>
#include <iostream>

#include "Expression.h"
#include "Utils.h"
#include "Arithmetic.h"
#include <map>

#define vector std::vector
#define string std::string
#define stack std::stack
#define queue std::queue
#define map std::map

#define cout std::cout
#define endl std::endl

namespace casio::cas_lib {

string formatInputExpression(const string* s) {
    string input = removeSpaces(s);
    string processed;

    for (int j = 0, i = 0; i < input.length(); i++) {
        char c = input.at(i);

        // If at the end of the input string
        if (endOfString(i, &input)) {
            processed.append(input.substr(j, (i - j + 1)));
            break;
        }

        if (c == '-') {
            processed.append(input.substr(j, (i - j)));
            char pLast = (processed.empty()) ? ' ' : processed[processed.length() - 1];

            if (pLast == '-') {
                if (processed.length() > 1) {
                    if (processed[processed.length() - 2] == '+') 
                        processed.replace(processed.length() - 2, processed.length(), "+");
                    else 
                        processed[processed.length() - 1] = '+';
                }
            }
            else {
                if (pLast == ' ' || (isOperator(pLast) && pLast != ')')) 
                    processed.append("-");
                else 
                    processed.append("+-");
            }

            j = i + 1;
        }
        else if (c == '(') {
            char b = (i > 0) ? input.at(i - 1) : ' ';

            if (i == 0) continue;
            else if (!isOperator(b) && b != '-') {
                processed.append(input.substr(j, (i - j)));
                processed.append("*");
                j = i;
            } else if (b == '-') {
                processed.append(input.substr(j, i - j));
                processed.append("1*");
            }
        }
    }

    return processed;
}

vector<string> tokenizeExpression(const string *expression) {
    vector<string> tokens;

    bool parsingVariables = false;

    for (int i = 0, j = 0; i < expression->length(); i++) {
        const char c = expression->at(i);

        if (isalpha(c)) parsingVariables = true;

        if (endOfString(i, expression)) {
            if (isOperator(c)) {
                if (parsingVariables && c == ')') {
                    if (expression->at(i - 1) == ')') {
                        tokens.push_back(expression->substr(j, (i - j)));
                        tokens.emplace_back(")");
                    }
                    else tokens.push_back(expression->substr(j, (i - j) + 1));
                }
                else {
                    if (i > j) tokens.push_back(expression->substr(j, (i - j)));
                    tokens.push_back(expression->substr(i, 1));
                }
            }
            else tokens.push_back(expression->substr(j, (expression->length() - j)));
        } else if (isOperator(c)) {
            if (parsingVariables && isOperator(c) && expression->at(i - 1) == ')') 
                parsingVariables = false;
            if (!parsingVariables) {
                if (j != i) tokens.push_back(expression->substr(j, (i - j)));
                tokens.push_back(expression->substr(i, 1));
                j = i + 1;
            }
        }
    }

    return tokens;
}

queue<string> infixToPostfix(vector<string> *tokens) {
    queue<string> postfixed;
    stack<string> operators;

    for (string s : *tokens) {
        if (isOperator(&s)) {
            int top_precedence = (operators.empty()) ? -1 : opPrecedence(operators.top().at(0));
            int this_precedence = opPrecedence(s.at(0));

            if (s == ")") {
                while (operators.top() != "(") {
                    postfixed.push(operators.top());
                    operators.pop();
                }

                operators.pop();
            }
            else if (!operators.empty() && operators.top() == "(") 
                operators.push(s);
            else if (top_precedence < this_precedence) 
                operators.push(s);
            else {
                while (!operators.empty() && ((operators.top() != "(" && top_precedence >= this_precedence) || (operators.top() == ")" && top_precedence > this_precedence))) {
                    postfixed.push(operators.top());
                    operators.pop();

                    top_precedence = (operators.empty()) ? -1 : opPrecedence(operators.top().at(0));
                }

                operators.push(s);
            }
        }
        else postfixed.push(s);
    }

    while (!operators.empty()) {
        postfixed.push(operators.top());
        operators.pop();
    }

    return postfixed;
}

Expression::Expression(const string input) {
    string formatted = formatInputExpression(&input);
    vector<string> tokens = tokenizeExpression(&formatted);
    queue<string> postfixed = infixToPostfix(&tokens);
    evaluate(&postfixed);
}

Expression::~Expression() {
    simplify();

    for (auto & iter : sortedTerms) {
        stack<Term*> terms = iter.second;
        while (!terms.empty()) {
            Term* tmp = terms.top();
            terms.pop();

            delete tmp;
        }
    }
}

void Expression::evaluate(queue<string> *tokens) {
    stack<Term*> output;
    vector<Term*> terms;

    while (!tokens->empty()) {
        string front = tokens->front();
        if (isOperator(&front)) {
            Term* two = output.top();
            output.pop();

            Term* one = output.top();
            output.pop();

            if (operable(one, two, &front)) {
                Term* r = operate(one, two, &front);
                output.push(r);
                delete one;
                delete two;
            }
            else {
                output.push(one);
                output.push(two);
            }
        }
        else {
            string number;
            output.push(Term::parseTerm(&front));
        }

        tokens->pop();
    }

    while (!output.empty()) {
        terms.push_back(output.top());
        output.pop();
    }

    insertTerms(&terms);
}

void Expression::insertTerm(Term* t) {
    auto findTVars = sortedTerms.find(t->varsToString());

    if (findTVars != sortedTerms.end()) 
        findTVars->second.push(t);
    else {
        stack<Term*> v;
        v.push(t);
        sortedTerms.emplace(t->varsToString(), v);
    }

    simplified = false;
}

void Expression::insertTerms(vector<Term*>* terms) {
    for (auto t : *terms) {
        insertTerm(t);
    }
    simplify();
}

void Expression::removeTerms(const string variables) {
    sortedTerms.erase(variables);
}

void Expression::simplify() {
    if (simplified) return;

    for (const auto & vars : sortedTerms) {
        stack<Term*> termStack = vars.second;
        while (termStack.size() > 1) {
            Term* one = termStack.top();
            termStack.pop();
            Term* two = termStack.top();
            termStack.pop();

            const string op = "+";
            Term* result = operate(one, two, &op);
            termStack.push(result);

            delete one;
            delete two;
        }

        if (termStack.top()->getCoefficient() != 0) 
            sortedTerms.find(vars.first)->second = termStack;
        else 
            delete termStack.top();
    }

    simplified = true;
}

void Expression::multiply(Expression* multiplicand) {
    simplify();
    vector<Term*> products;

    for (const auto& thisIter : sortedTerms) {
        for (const auto& thatIter : multiplicand->getTerms()) {
            products.push_back(multiplyTerms(thisIter.second.top(), thatIter.second.top()));
        }
    }

    sortedTerms.clear();
    insertTerms(&products);
}

string Expression::toString() {
    string stringified;

    for (auto iter = --sortedTerms.end(); iter != sortedTerms.begin(); iter--) {
        stringified.append(iter->second.top()->toString());
        stringified.append("+");
    }

    stringified.append(sortedTerms.begin()->second.top()->toString());

    return stringified;
}

} // namespace casio::cas_lib

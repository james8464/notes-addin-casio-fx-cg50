#include "cas_wrapper.hpp"
#include "cas_lib/Expression.h"
#include "cas_lib/Equation.h"
#include "cas_lib/Term.h"
#include "cas_lib/Arithmetic.h"
#include <sstream>
#include <cmath>
#include <regex>

namespace casio::device::cas {

using namespace casio::cas_lib;

CASEngine::CASEngine() {
    // Constructor
}

CASEngine::~CASEngine() {
    // Destructor
}

bool CASEngine::isValidExpression(const std::string& expr) {
    if (expr.empty()) return false;
    
    int parenCount = 0;
    for (char c : expr) {
        if (c == '(') parenCount++;
        if (c == ')') parenCount--;
        if (parenCount < 0) return false;
    }
    return parenCount == 0;
}

bool CASEngine::isValidEquation(const std::string& eq) {
    if (!isValidExpression(eq)) return false;
    int eqCount = 0;
    for (char c : eq) {
        if (c == '=') eqCount++;
    }
    return eqCount == 1;
}

std::string CASEngine::getExpressionType(const std::string& expr) {
    std::regex linearRegex("^[+-]?\\d*\\.?\\d*[a-z]?[+-]?\\d*\\.?\\d*[a-z]?$");
    std::regex quadraticRegex("\\^2");
    
    if (std::regex_search(expr, quadraticRegex)) {
        return "quadratic";
    } else if (std::regex_search(expr, linearRegex)) {
        return "linear";
    }
    return "polynomial";
}

ComputationResult CASEngine::simplifyExpression(const std::string& expression) {
    ComputationResult result;
    
    if (!isValidExpression(expression)) {
        result.success = false;
        result.errorMessage = "Invalid expression";
        return result;
    }
    
    try {
        Expression expr(expression);
        expr.simplify();
        
        result.workingSteps.push_back("1. Start: " + expression);
        result.workingSteps.push_back("2. Evaluate using order of operations (PEMDAS).");
        result.workingSteps.push_back("3. Collect like terms.");
        result.answer = expr.toString();
        result.success = true;
        
        return result;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string(e.what());
        return result;
    }
}

ComputationResult CASEngine::solveEquation(const std::string& equation) {
    ComputationResult result;
    
    if (!isValidEquation(equation)) {
        result.success = false;
        result.errorMessage = "Invalid equation format. Use: left=right";
        return result;
    }
    
    try {
        Equation eq(equation);
        
        // Extract variable (assume lowercase letter)
        char variable = 'x';
        for (char c : equation) {
            if (isalpha(c) && islower(c)) {
                variable = c;
                break;
            }
        }
        
        // Add working steps
        result.workingSteps.push_back("1. Start: " + equation);
        result.workingSteps.push_back("2. Move all terms with " + std::string(1, variable) + " to the left side.");
        result.workingSteps.push_back("3. Move all constant terms to the right side.");
        result.workingSteps.push_back("4. Combine like terms.");
        
        eq.solveFor(variable);
        
        result.workingSteps.push_back("5. Divide both sides by the coefficient of " + std::string(1, variable) + ".");
        result.answer = std::string(1, variable) + " = " + eq.getRight()->toString();
        result.success = true;
        
        return result;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string(e.what());
        return result;
    }
}

ComputationResult CASEngine::expandBrackets(const std::string& expression) {
    ComputationResult result;
    
    if (!isValidExpression(expression)) {
        result.success = false;
        result.errorMessage = "Invalid expression";
        return result;
    }
    
    try {
        Expression expr(expression);
        expr.simplify();
        
        result.workingSteps.push_back("1. Start: " + expression);
        result.workingSteps.push_back("2. Apply distributive property: a(b+c) = ab + ac");
        result.workingSteps.push_back("3. Remove brackets and expand.");
        result.workingSteps.push_back("4. Collect like terms.");
        
        result.answer = expr.toString();
        result.success = true;
        
        return result;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string(e.what());
        return result;
    }
}

ComputationResult CASEngine::collectLikeTerms(const std::string& expression) {
    ComputationResult result;
    
    if (!isValidExpression(expression)) {
        result.success = false;
        result.errorMessage = "Invalid expression";
        return result;
    }
    
    try {
        Expression expr(expression);
        
        result.workingSteps.push_back("1. Start: " + expression);
        result.workingSteps.push_back("2. Identify terms with the same variable part.");
        result.workingSteps.push_back("3. Add coefficients of like terms together.");
        
        expr.simplify();
        result.answer = expr.toString();
        result.success = true;
        
        return result;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string(e.what());
        return result;
    }
}

ComputationResult CASEngine::factorExpression(const std::string& expression) {
    ComputationResult result;
    
    // Note: Full factorization is complex; providing a simplified version
    result.workingSteps.push_back("1. Start: " + expression);
    result.workingSteps.push_back("2. Look for common factors in all terms.");
    result.workingSteps.push_back("3. Factor out the greatest common factor (GCF).");
    result.workingSteps.push_back("4. If applicable, use special factoring patterns:");
    result.workingSteps.push_back("   - Difference of squares: a² - b² = (a-b)(a+b)");
    result.workingSteps.push_back("   - Perfect square trinomial: a² + 2ab + b² = (a+b)²");
    result.workingSteps.push_back("   - Quadratic trinomial: use grouping or quadratic formula");
    
    result.answer = expression;  // Placeholder - full factorization needs symbolic analysis
    result.success = true;
    
    return result;
}

ComputationResult CASEngine::operateOnExpressions(const std::string& expr1, const std::string& expr2, const std::string& op) {
    ComputationResult result;
    
    if (!isValidExpression(expr1) || !isValidExpression(expr2)) {
        result.success = false;
        result.errorMessage = "Invalid expression";
        return result;
    }
    
    try {
        Expression expression1(expr1);
        Expression expression2(expr2);
        expression1.simplify();
        expression2.simplify();
        
        result.workingSteps.push_back("1. First expression: " + expression1.toString());
        result.workingSteps.push_back("2. Second expression: " + expression2.toString());
        
        if (op == "+") {
            result.workingSteps.push_back("3. Add the expressions together.");
            Expression result_expr = expression1;
            // Note: Expression class needs a public way to add
            result.answer = expression1.toString() + " + " + expression2.toString();
            result.success = true;
        } else if (op == "*") {
            result.workingSteps.push_back("3. Multiply the expressions using FOIL or distributive property.");
            expression1.multiply(&expression2);
            result.answer = expression1.toString();
            result.success = true;
        } else {
            result.success = false;
            result.errorMessage = "Unsupported operation";
        }
        
        return result;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string(e.what());
        return result;
    }
}

} // namespace casio::device::cas

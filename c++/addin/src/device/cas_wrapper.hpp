#pragma once

#include <string>
#include <vector>

namespace casio::device::cas
{

// Result structure for computation with working steps
struct ComputationResult {
    bool success;
    std::string answer;
    std::vector<std::string> workingSteps;
    std::string errorMessage;
    
    ComputationResult() : success(false) {}
};

// Enum for operation types
enum class OperationType {
    Simplify,
    SolveLinear,
    SolveQuadratic,
    ExpandBrackets,
    CollectLikeTerms,
    FactorExpression,
    MultiplyExpressions,
    DivideExpressions,
    Add,
    Subtract
};

// Main CAS wrapper class
class CASEngine {
public:
    CASEngine();
    ~CASEngine();
    
    // Core algebra operations
    ComputationResult simplifyExpression(const std::string& expression);
    ComputationResult solveEquation(const std::string& equation);
    ComputationResult expandBrackets(const std::string& expression);
    ComputationResult collectLikeTerms(const std::string& expression);
    ComputationResult factorExpression(const std::string& expression);
    ComputationResult operateOnExpressions(const std::string& expr1, const std::string& expr2, const std::string& op);
    
    // Helper functions
    static bool isValidExpression(const std::string& expr);
    static bool isValidEquation(const std::string& eq);
    static std::string getExpressionType(const std::string& expr);
};

} // namespace casio::device::cas

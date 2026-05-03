#include "device/device_solver.hpp"

#include <cstring>
#include <iostream>

namespace
{

bool contains(casio::device::OutputLines const &lines, const char *needle)
{
    for(int i = 0; i < lines.count(); i++) {
        if(std::strstr(lines.line(i), needle) != nullptr) return true;
    }
    return false;
}

bool check(casio::device::Module module, const char *input, const char *expected)
{
    casio::device::OutputLines lines;
    bool ok = casio::device::solve(module, input, lines);
    if(!ok || !contains(lines, expected)) {
        std::cerr << "device smoke failed for input: " << input << "\n";
        std::cerr << "expected snippet: " << expected << "\n";
        for(int i = 0; i < lines.count(); i++) std::cerr << lines.line(i) << "\n";
        return false;
    }
    return true;
}

} // namespace

int main()
{
    bool ok = true;
    ok = check(casio::device::Module::Simplify, "2x+3-x+4", "Answer: x + 7") && ok;
    ok = check(casio::device::Module::Simplify, "x^2+2x+x^2", "Answer: 2x^2 + 2x") && ok;
    ok = check(casio::device::Module::Simplify, "(x+1)^2", "Answer: x^2 + 2x + 1") && ok;
    ok = check(casio::device::Module::Simplify, "2(x+1)-x", "Answer: x + 2") && ok;
    ok = check(casio::device::Module::Shell, "diff(x^2)", "Answer: dy/dx = 2x") && ok;
    ok = check(casio::device::Module::Shell, "diff((x+1)^2)", "Answer: dy/dx = 2x + 2") && ok;
    ok = check(casio::device::Module::Shell, "int(x^2)", "Answer: x^3/3 + C") && ok;
    ok = check(casio::device::Module::Shell, "int((x+1)^2)", "Answer: x^3/3 + x^2 + x + C") && ok;
    ok = check(casio::device::Module::Shell, "solve(2x+3=7)", "Answer: x = 2") && ok;
    ok = check(casio::device::Module::Shell, "solve(2(x+1)=x+5)", "Answer: x = 3") && ok;
    ok = check(casio::device::Module::Shell, "solve((x+1)^2=9)", "Answer: x = -4 or x = 2") && ok;
    ok = check(casio::device::Module::Shell, "solve(x(x-1)=6)", "Answer: x = -2 or x = 3") && ok;
    ok = check(casio::device::Module::Shell, "solve(x^3-6x^2+11x-6=0)", "Answer: x = 1 or x = 2 or x = 3") && ok;
    ok = check(casio::device::Module::Shell, "simplify(2x+3-x+4)", "Answer: x + 7") && ok;
    ok = check(casio::device::Module::Shell, "trig(sin(30))", "Answer: 1/2") && ok;
    ok = check(casio::device::Module::Shell, "trig(sin^2(x)+cos^2(x))", "Answer: 1") && ok;
    ok = check(casio::device::Module::Shell, "solve_trig(2sin(x)+1=0)", "x = 210 + 360n") && ok;
    ok = check(casio::device::Module::Shell, "solve_trig(sin(x)^2-1=0)", "x = 90 + 360n") && ok;
    ok = check(casio::device::Module::Shell, "det2(1,2,3,4)", "Answer: det = -2") && ok;
    ok = check(casio::device::Module::Shell, "inv2(1,2,3,4)", "Answer: [[-2,1],[3/2,-1/2]]") && ok;
    ok = check(casio::device::Module::Shell, "dot3(1,2,3,4,5,6)", "Answer: 32") && ok;
    ok = check(casio::device::Module::Shell, "cross3(1,0,0,0,1,0)", "Answer: (0,0,1)") && ok;
    ok = check(casio::device::Module::Shell, "binom(4,1/2,2)", "Answer: 3/8") && ok;
    ok = check(casio::device::Module::Shell, "binomcdf(4,1/2,2)", "Answer: 11/16") && ok;
    ok = check(casio::device::Module::Shell, "stats(1,2,3,4)", "mean = sum x/n = 5/2") && ok;
    ok = check(casio::device::Module::Shell, "gcd(84,126,210)", "Answer: gcd = 42") && ok;
    ok = check(casio::device::Module::Shell, "lcm(6,10,15)", "Answer: lcm = 30") && ok;
    ok = check(casio::device::Module::Shell, "factorial(6)", "Answer: 720") && ok;
    ok = check(casio::device::Module::Shell, "ncr(10,3)", "Answer: 120") && ok;
    ok = check(casio::device::Module::Shell, "isprime(97)", "Answer: prime") && ok;
    ok = check(casio::device::Module::Shell, "factors(360)", "Answer: 2^3*3^2*5") && ok;
    ok = check(casio::device::Module::Shell, "divisors(28)", "Answer: 1,2,4,7,14,28") && ok;
    ok = check(casio::device::Module::Shell, "suvat(s=10,u=0,v=?,a=2,t=5)", "Answer: v = 10") && ok;
    ok = check(casio::device::Module::Algebra, "2x+3=7", "Answer: x = 2") && ok;
    ok = check(casio::device::Module::Algebra, "x^2-5x+6=0", "Answer: x = 2 or x = 3") && ok;
    ok = check(casio::device::Module::Algebra, "x^2-2=0", "sqrt(8)") && ok;
    ok = check(casio::device::Module::Algebra, "x^2+1=0", "i*sqrt(4)") && ok;
    ok = check(casio::device::Module::Derive, "3x^2+2x+1", "Answer: dy/dx = 6x + 2") && ok;
    ok = check(casio::device::Module::Integrate, "3x^2+2x+1", "Answer: x^3 + x^2 + x + C") && ok;
    ok = check(casio::device::Module::Trig, "sin(30)", "Answer: 1/2") && ok;
    ok = check(casio::device::Module::Trig, "cos(5*pi/3)", "Answer: 1/2") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=0,v=?,a=2,t=5", "Answer: v = 10") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=0,v=4,a=?,t=5", "Answer: a = 4/5") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=?,v=4,t=5", "Answer: u = 0") && ok;
    return ok ? 0 : 1;
}

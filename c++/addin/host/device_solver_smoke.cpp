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
    ok = check(casio::device::Module::Shell, "diff(x^2)", "Answer: dy/dx = 2x") && ok;
    ok = check(casio::device::Module::Shell, "int(x^2)", "Answer: x^3/3 + C") && ok;
    ok = check(casio::device::Module::Algebra, "2x+3=7", "Answer: x = 2") && ok;
    ok = check(casio::device::Module::Derive, "3x^2+2x+1", "Answer: dy/dx = 6x + 2") && ok;
    ok = check(casio::device::Module::Integrate, "3x^2+2x+1", "Answer: x^3 + x^2 + x + C") && ok;
    ok = check(casio::device::Module::Trig, "sin(30)", "Answer: 1/2") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=0,v=?,a=2,t=5", "Answer: v = 10") && ok;
    return ok ? 0 : 1;
}

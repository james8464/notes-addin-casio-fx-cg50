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

bool module_from_name(const char *name, casio::device::Module &out)
{
    if(std::strcmp(name, "shell") == 0) out = casio::device::Module::Shell;
    else if(std::strcmp(name, "simplify") == 0) out = casio::device::Module::Simplify;
    else if(std::strcmp(name, "algebra") == 0) out = casio::device::Module::Algebra;
    else if(std::strcmp(name, "derive") == 0) out = casio::device::Module::Derive;
    else if(std::strcmp(name, "integrate") == 0) out = casio::device::Module::Integrate;
    else if(std::strcmp(name, "trig") == 0) out = casio::device::Module::Trig;
    else if(std::strcmp(name, "suvat") == 0) out = casio::device::Module::Suvat;
    else if(std::strcmp(name, "stats") == 0) out = casio::device::Module::Stats;
    else return false;
    return true;
}

int run_cli(int argc, char **argv)
{
    if(argc < 3) {
        std::cerr << "usage: device_solver_smoke <module> <input>\n";
        return 2;
    }
    casio::device::Module module = casio::device::Module::Shell;
    if(!module_from_name(argv[1], module)) {
        std::cerr << "unknown module: " << argv[1] << "\n";
        return 2;
    }
    casio::device::OutputLines lines;
    bool ok = casio::device::solve(module, argv[2], lines);
    for(int i = 0; i < lines.count(); i++) std::cout << lines.line(i) << "\n";
    return ok ? 0 : 1;
}

} // namespace

int main(int argc, char **argv)
{
    if(argc > 1) return run_cli(argc, argv);

    bool ok = true;
    ok = check(casio::device::Module::Simplify, "2x+3-x+4", "x + 7") && ok;
    ok = check(casio::device::Module::Simplify, "x^2+2x+x^2", "2*x^2 + 2*x") && ok;
    ok = check(casio::device::Module::Simplify, "(x+1)^2", "(x + 1)^2") && ok;
    ok = check(casio::device::Module::Simplify, "2(x+1)-x", "x + 2") && ok;
    ok = check(casio::device::Module::Shell, "diff(x^2)", "dy/dx = 2*x") && ok;
    ok = check(casio::device::Module::Shell, "diff((x+1)^2)", "dy/dx = 2*(x + 1)") && ok;
    ok = check(casio::device::Module::Shell, "diff(ln(x))", "dy/dx = 1/x") && ok;
    ok = check(casio::device::Module::Shell, "diff((2x+ln(x))^3)", "dy/dx = 3*(2*x + ln(x))^2*(1/x + 2)") && ok;
    ok = check(casio::device::Module::Shell, "diff((2x+ln((x)))^3", "dy/dx = 3*(2*x + ln(x))^2*(1/x + 2)") && ok;
    ok = check(casio::device::Module::Shell, "int(x^2)", "x^3/3 + C") && ok;
    ok = check(casio::device::Module::Shell, "int((x+1)^2)", "(x + 1)^3 + C") && ok;
    ok = check(casio::device::Module::Shell, "solve(2x+3=7)", "x = [2]") && ok;
    ok = check(casio::device::Module::Shell, "solve(2(x+1)=x+5)", "x = [3]") && ok;
    ok = check(casio::device::Module::Shell, "solve((x+1)^2=9)", "x = [2, -4]") && ok;
    ok = check(casio::device::Module::Shell, "solve(x(x-1)=6)", "x = [3, -2]") && ok;
    ok = check(casio::device::Module::Shell, "solve(x^3-6x^2+11x-6=0)", "x = [1, 2, 3]") && ok;
    ok = check(casio::device::Module::Shell, "complete_square(x^2-6x+5)", "(x - 3)^2 - 4") && ok;
    ok = check(casio::device::Module::Shell, "compare(2(x+1),2x+2)", "equivalent") && ok;
    ok = check(casio::device::Module::Shell, "transform(x^2+2x+1,(x+1)^2)", "0") && ok;
    ok = check(casio::device::Module::Shell, "compose(2x+1,x^2)", "2*x^2 + 1") && ok;
    ok = check(casio::device::Module::Shell, "domain(x^2-4x+1)", "y >= -3") && ok;
    ok = check(casio::device::Module::Algebra, "2x+3=7", "x = [2]") && ok;
    ok = check(casio::device::Module::Algebra, "x^2-5x+6=0", "x = [3, 2]") && ok;
    ok = check(casio::device::Module::Algebra, "x^2+1=0", "No real") && ok;
    ok = check(casio::device::Module::Derive, "3x^2+2x+1", "dy/dx = 6*x + 2") && ok;
    ok = check(casio::device::Module::Integrate, "3x^2+2x+1", "x^3 + x^2 + x + C") && ok;
    ok = check(casio::device::Module::Trig, "sin(30)", "1/2") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=0,v=?,a=2,t=5", "v = 10") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=0,v=4,a=?,t=5", "a = 4/5") && ok;
    ok = check(casio::device::Module::Suvat, "s=10,u=?,v=4,t=5", "u = 0") && ok;
    return ok ? 0 : 1;
}

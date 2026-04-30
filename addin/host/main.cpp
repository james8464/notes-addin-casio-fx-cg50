#include "core/arena.hpp"
#include "core/format_exam.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"

#include "modules/boolean/boolean.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cerr << "usage: casio_host \"EXPR\"\n";
        std::cerr << "   or: casio_host --bool \"BOOL_EXPR\"\n";
        return 2;
    }

    bool is_bool = (std::string(argv[1]) == "--bool");
    std::string expr = is_bool ? (argc >= 3 ? argv[2] : "") : argv[1];
    casio::Arena arena;

    try {
        if(is_bool) {
            auto n = casio::boolean::parse(expr);
            std::cout << "BOOL: " << casio::boolean::show(n) << "\n";
            auto hit = casio::boolean::step(n);
            if(hit.first) {
                std::cout << "STEP: " << casio::boolean::show(hit.first) << "\n";
                std::cout << "WHY: " << hit.second << "\n";
            }
            else {
                std::cout << "STEP: (none)\n";
            }
        }
        else {
            std::string norm = casio::normalize_text(expr);
            auto root = casio::parse_expr(arena, expr);
            std::string fmt = casio::format_expr(arena, root);
            std::string human = casio::format_equation_human_readable(arena, root);
            std::cout << "NORM: " << norm << "\n";
            std::cout << "FMT: " << fmt << "\n";
            std::cout << "HUMAN: " << human << "\n";
        }
        return 0;
    }
    catch(std::exception const &e) {
        std::cout << "ERR: " << e.what() << "\n";
        return 1;
    }
}


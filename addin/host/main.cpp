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

    std::string flag = argv[1];
    bool is_bool = (flag == "--bool");
    bool is_bool_nand = (flag == "--nand");
    bool is_bool_nor = (flag == "--nor");
    bool is_bool_prove = (flag == "--prove");
    bool any_bool = is_bool || is_bool_nand || is_bool_nor || is_bool_prove;

    std::string expr = any_bool ? (argc >= 3 ? argv[2] : "") : argv[1];
    casio::Arena arena;

    try {
        if(is_bool || is_bool_nand || is_bool_nor || is_bool_prove) {
            if(is_bool_prove) {
                if(argc < 4) {
                    std::cout << "ERR: need LHS RHS\n";
                    return 2;
                }
                auto [lines, err] = casio::boolean::prove(argv[2], argv[3]);
                if(!err.empty()) {
                    std::cout << "ERR: " << err << "\n";
                    return 1;
                }
                for(auto const &ln : lines) std::cout << ln << "\n";
                return 0;
            }
            auto n = casio::boolean::parse(expr);
            if(is_bool_nand) {
                auto out = casio::boolean::normalise(casio::boolean::to_nand(n));
                std::cout << casio::boolean::show(out) << "\n";
                return 0;
            }
            if(is_bool_nor) {
                auto out = casio::boolean::normalise(casio::boolean::to_nor(n));
                std::cout << casio::boolean::show(out) << "\n";
                return 0;
            }

            std::cout << "BOOL: " << casio::boolean::show(n) << "\n";
            std::cout << "SHORT: " << casio::boolean::short_text(n) << "\n";
            auto hit = casio::boolean::step(n);
            if(hit.first) {
                std::cout << "STEP: " << casio::boolean::show(hit.first) << "\n";
                std::cout << "WHY: " << hit.second << "\n";
            }
            else std::cout << "STEP: (none)\n";
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


#include "core/arena.hpp"
#include "core/format_exam.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"

#include "modules/boolean/boolean.hpp"
#include "modules/suvat/suvat.hpp"
#include "modules/integrate/integrate.hpp"
#include "modules/algebra/algebra.hpp"
#include "modules/trig/trig.hpp"
#include "modules/derive/derive.hpp"

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
    bool is_suvat = (flag == "--suvat");
    bool is_int = (flag == "--int");
    bool is_alg = (flag == "--alg");
    bool is_trig = (flag == "--trig");
    bool is_derive = (flag == "--derive");

    std::string expr = (any_bool || is_suvat || is_int || is_alg || is_trig || is_derive) ? (argc >= 3 ? argv[2] : "") : argv[1];
    casio::Arena arena;

    try {
        if(is_suvat) {
            // Usage: --suvat "s=...,u=...,v=...,a=...,t=...,target=v"
            casio::suvat::Inputs in;
            in.target = "v";
            auto parse_kv = [&](std::string const &kv) {
                auto eq = kv.find('=');
                if(eq == std::string::npos) return;
                std::string k = kv.substr(0, eq);
                std::string v = kv.substr(eq + 1);
                if(k == "s") in.s = v;
                else if(k == "u") in.u = v;
                else if(k == "v") in.v = v;
                else if(k == "a") in.a = v;
                else if(k == "t") in.t = v;
                else if(k == "target") in.target = v;
            };
            std::string text = expr;
            std::size_t i = 0;
            while(i < text.size()) {
                std::size_t j = text.find(',', i);
                if(j == std::string::npos) j = text.size();
                parse_kv(text.substr(i, j - i));
                i = j + 1;
            }
            auto lines = casio::suvat::solve_all(arena, in);
            for(auto const &ln : lines) std::cout << ln << "\n";
            return 0;
        }
        if(is_int) {
            casio::integrate::Request req;
            req.expr = expr;
            auto lines = casio::integrate::run(arena, req);
            for(auto const &ln : lines) std::cout << ln << "\n";
            return 0;
        }
        if(is_alg) {
            casio::algebra::Request req;
            req.expr = expr;
            auto lines = casio::algebra::run(arena, req);
            for(auto const &ln : lines) std::cout << ln << "\n";
            return 0;
        }
        if(is_trig) {
            casio::trig::Request req;
            req.expr = expr;
            auto lines = casio::trig::run(arena, req);
            for(auto const &ln : lines) std::cout << ln << "\n";
            return 0;
        }
        if(is_derive) {
            casio::derive::Request req;
            // Accept optional prefix: "mode:<n>,<expr...>"
            // Example: --derive "mode:4,x^2"
            req.mode = 1;
            req.expr = expr;
            if(expr.rfind("mode:", 0) == 0) {
                auto comma = expr.find(',');
                if(comma != std::string::npos) {
                    try {
                        req.mode = std::stoi(expr.substr(5, comma - 5));
                        req.expr = expr.substr(comma + 1);
                    } catch(...) {
                        // keep defaults
                    }
                }
            }
            auto lines = casio::derive::run(arena, req);
            for(auto const &ln : lines) std::cout << ln << "\n";
            return 0;
        }
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


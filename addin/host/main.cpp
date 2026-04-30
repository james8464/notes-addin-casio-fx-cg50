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
#include <sstream>
#include <string>
#include <cstdlib>

static std::string read_all_stdin()
{
    std::ostringstream oss;
    oss << std::cin.rdbuf();
    return oss.str();
}

static std::vector<std::string> split_lines(std::string const &s)
{
    std::vector<std::string> out;
    std::string cur;
    for(char c : s) {
        if(c == '\n') {
            out.push_back(cur);
            cur.clear();
        }
        else if(c != '\r') cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

static int run_stdin_program(casio::Arena &arena, std::string const &program, std::string const &stdin_text)
{
    auto lines = split_lines(stdin_text);
    auto get = [&](std::size_t i) -> std::string {
        if(i >= lines.size()) return "";
        return lines[i];
    };

    // Program dispatch by file name (matches python/tests/run_tests.py usage).
    if(program == "deriveProgram.py") {
        int mode = 1;
        try { mode = std::stoi(get(0)); } catch(...) {}
        casio::derive::Request req;
        req.mode = mode;
        if(mode == 1) req.expr = get(1);
        else if(mode == 2) req.expr = get(1);
        else if(mode == 3) req.expr = get(1) + "," + get(2) + ",t";
        else if(mode == 4) req.expr = get(1);
        auto out = casio::derive::run(arena, req);
        for(auto const &ln : out) std::cout << ln << "\n";
        return 0;
    }
    if(program == "intProgram.py") {
        // mode 1: integrate, input lines: 1, <f>, <method>
        std::string mode = get(0);
        if(mode != "1") {
            std::cout << "Err: int mode not supported yet.\n";
            return 0;
        }
        casio::integrate::Request req;
        req.mode = 1;
        req.expr = get(1);
        auto out = casio::integrate::run(arena, req);
        for(auto const &ln : out) std::cout << ln << "\n";
        return 0;
    }
    if(program == "algebraProgram.py") {
        std::string mode = get(0);
        casio::algebra::Request req;
        try { req.mode = std::stoi(mode); } catch(...) { req.mode = 1; }
        if(req.mode == 1 || req.mode == 2) {
            req.expr = get(1) + "\n" + get(2);
        }
        else {
            req.expr = get(1);
        }
        auto out = casio::algebra::run(arena, req);
        for(auto const &ln : out) std::cout << ln << "\n";
        return 0;
    }
    if(program == "trigProgram.py") {
        std::string mode = get(0);
        casio::trig::Request req;
        try { req.mode = std::stoi(mode); } catch(...) { req.mode = 3; }
        if(req.mode == 1 || req.mode == 2) {
            // mode1: prove, mode2: transform
            req.expr = get(1) + "\n" + get(2);
        }
        else {
            req.expr = get(1);
        }
        auto out = casio::trig::run(arena, req);
        for(auto const &ln : out) std::cout << ln << "\n";
        return 0;
    }
    if(program == "SUVATprogram.py") {
        // Inputs are 5 lines: s,u,v,a,t (blank for unknown). Target inferred in C++ by blank.
        casio::suvat::Inputs in;
        in.s = get(0);
        in.u = get(1);
        in.v = get(2);
        in.a = get(3);
        in.t = get(4);
        auto out = casio::suvat::solve_all(arena, in);
        for(auto const &ln : out) std::cout << ln << "\n";
        return 0;
    }

    std::cout << "Err: unknown --stdin-program.\n";
    return 0;
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cerr << "usage: casio_host \"EXPR\"\n";
        std::cerr << "   or: casio_host --bool \"BOOL_EXPR\"\n";
        return 2;
    }

    std::string flag = argv[1];
    bool is_stdin_program = (flag == "--stdin-program");
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

    std::string expr = (is_stdin_program || any_bool || is_suvat || is_int || is_alg || is_trig || is_derive) ? (argc >= 3 ? argv[2] : "") : argv[1];
    casio::Arena arena;
    // Resource budget: cap node growth (prevents pathological hangs/crashes).
    if(char const *env = std::getenv("CASIO_MAX_NODES")) {
        try {
            arena.set_max_nodes(static_cast<std::size_t>(std::stoul(env)));
        } catch(...) {
            arena.set_max_nodes(50000);
        }
    }
    else arena.set_max_nodes(50000);

    try {
        if(is_stdin_program) {
            std::string stdin_text = read_all_stdin();
            return run_stdin_program(arena, expr, stdin_text);
        }
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
            req.mode = 6;
            req.expr = expr;
            auto lines = casio::algebra::run(arena, req);
            for(auto const &ln : lines) std::cout << ln << "\n";
            return 0;
        }
        if(is_trig) {
            casio::trig::Request req;
            req.mode = 0;
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


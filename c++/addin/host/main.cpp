#include "core/arena.hpp"
#include "core/format_exam.hpp"
#include "core/format_expr.hpp"
#include "core/normalize.hpp"
#include "core/parse.hpp"
#include "core/parse_equation.hpp"

#include "modules/boolean/boolean.hpp"
#include "modules/suvat/suvat.hpp"
#include "modules/integrate/integrate.hpp"
#include "modules/algebra/algebra.hpp"
#include "modules/trig/trig.hpp"
#include "modules/derive/derive.hpp"
#include "modules/stats/stats.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <cctype>
#include <algorithm>
#include <set>

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

static std::string trim(std::string s)
{
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while(!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static std::vector<std::string> split_top_csv(std::string const &s)
{
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    for(char c : s) {
        if(c == '(' || c == '[' || c == '{') ++depth;
        else if(c == ')' || c == ']' || c == '}') --depth;
        if(c == ',' && depth == 0) {
            out.push_back(trim(cur));
            cur.clear();
        }
        else cur.push_back(c);
    }
    if(!cur.empty() || !out.empty()) out.push_back(trim(cur));
    return out;
}

static bool key_arg(std::string const &arg, std::string const &key, std::string &value)
{
    std::string t = trim(arg);
    if(t.rfind(key + "=", 0) != 0) return false;
    value = trim(t.substr(key.size() + 1));
    return !value.empty();
}

static std::string strip_method_args(std::string expr, std::string &method, std::string &u, bool strip_u_arg = false, std::string *target = nullptr)
{
    method.clear();
    u.clear();
    if(target) target->clear();
    auto parts = split_top_csv(expr);
    if(parts.empty()) return expr;
    std::vector<std::string> kept;
    for(auto const &p : parts) {
        std::string v;
        if(key_arg(p, "method", v) || key_arg(p, "met", v)) {
            method = v;
            continue;
        }
        if(strip_u_arg && key_arg(p, "u", v)) {
            u = v;
            continue;
        }
        if(target && key_arg(p, "target", v)) {
            *target = v;
            continue;
        }
        kept.push_back(p);
    }
    if(method.empty() && u.empty() && (!target || target->empty())) return expr;
    std::ostringstream oss;
    for(std::size_t i = 0; i < kept.size(); ++i) {
        if(i) oss << ",";
        oss << kept[i];
    }
    return oss.str();
}

static char const *valid_methods(std::string const &feature)
{
    if(feature == "int") return "|auto|direct|reverse_chain|sub|parts|di|trig|pf|div|weierstrass|symmetry|";
    if(feature == "derive") return "|auto|chain|product|quotient|logdiff|first_principles|implicit|param|second|param_second|";
    if(feature == "trig") return "|auto|general|bounded|cast|identity|rform|square_then_check|sin_cos|pythag|double_angle|compound_angle|target|";
    if(feature == "alg") return "|auto|linear|factor|quad_formula|complete_square|substitution|clear_denoms|log_exp|numeric|interval|expand|collect|partfrac|pf|rationalise|canonical|target|equate_coeffs|simultaneous|";
    if(feature == "stats") return "|auto|summary|regression|hypothesis_test|binomial|normal|poisson|confidence_interval|";
    if(feature == "suvat") return "|auto|suvat|energy|moments|projectile|forces|variable_accel|";
    return "|auto|";
}

static bool method_allowed(char const *valid, std::string const &method)
{
    if(method.empty() || method == "auto") return true;
    return std::string(valid).find("|" + method + "|") != std::string::npos;
}

static void print_lines(std::vector<std::string> const &lines)
{
    for(auto const &ln : lines) {
        std::string s = casio::math_style_line(ln);
        if(!s.empty()) std::cout << s << "\n";
    }
}

static bool print_method_header(std::string const &feature, std::string const &method, std::string const &u)
{
    if(method.empty() || method == "auto") return true;
    char const *valid = valid_methods(feature);
    if(!method_allowed(valid, method)) {
        std::cout << "Err: invalid method. Valid: " << valid << "\n";
        return false;
    }
    (void)u;
    return true;
}

static bool print_method_header_for_expr(std::string const &feature, std::string const &method, std::string const &u, std::string const &expr)
{
    (void)expr;
    return print_method_header(feature, method, u);
}

static int run_stdin_program(casio::Arena &arena, std::string const &program, std::string const &stdin_text)
{
    auto lines = split_lines(stdin_text);
    auto get = [&](std::size_t i) -> std::string {
        if(i >= lines.size()) return "";
        return lines[i];
    };

    // Program dispatch by legacy calculator program name.
    if(program == "deriveProgram.py") {
        int mode = 1;
        try { mode = std::stoi(get(0)); } catch(...) {}
        casio::derive::Request req;
        req.mode = mode;
        if(mode == 1) req.expr = get(1);
        else if(mode == 2) {
            req.expr = get(1);
            std::string marker = "Differentiate ";
            auto start = req.expr.find(marker);
            if(start != std::string::npos) {
                start += marker.size();
                auto end = req.expr.find(" with respect", start);
                if(end == std::string::npos) end = req.expr.find(':', start);
                if(end != std::string::npos && end > start) req.expr = req.expr.substr(start, end - start);
            }
        }
        else if(mode == 3) req.expr = get(1) + "," + get(2) + ",t";
        else if(mode == 4) req.expr = get(1);
        auto out = casio::derive::run(arena, req);
        print_lines(out);
        return 0;
    }
    if(program == "intProgram.py") {
        casio::integrate::Request req;
        try { req.mode = std::stoi(get(0)); } catch(...) { req.mode = 1; }
        if(req.mode == 1) {
            req.expr = get(1);
        }
        else if(req.mode == 2) {
            req.expr = get(1) + "\n" + get(2);
        }
        else {
            std::cout << "Err: int mode not supported yet.\n";
            return 0;
        }
        auto out = casio::integrate::run(arena, req);
        print_lines(out);
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
            // Preserve the full payload after the mode line (many modes are multiline).
            std::ostringstream oss;
            for(std::size_t i = 1; i < lines.size(); i++) {
                if(i != 1) oss << "\n";
                oss << lines[i];
            }
            req.expr = oss.str();
        }
        std::string method, method_u, target;
        req.expr = strip_method_args(req.expr, method, method_u, false, &target);
        if(!method.empty()) req.method = method;
        if(!target.empty() && req.expr.find('=') == std::string::npos) req.expr += "=" + target;
        auto out = casio::algebra::run(arena, req);
        print_lines(out);
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
        else if(req.mode == 4) {
            std::ostringstream oss;
            for(std::size_t i = 1; i < lines.size(); i++) {
                if(i != 1) oss << "\n";
                oss << lines[i];
            }
            req.expr = oss.str();
        }
        else {
            req.expr = get(1);
        }
        auto out = casio::trig::run(arena, req);
        print_lines(out);
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
        auto mark_target = [&](std::string &field, char const *name) {
            if(field.find(',') == std::string::npos) return;
            in.target = name;
            std::string clean;
            for(char c : field) {
                if(c != ',') clean.push_back(c);
            }
            field = clean;
        };
        mark_target(in.s, "s");
        mark_target(in.u, "u");
        mark_target(in.v, "v");
        mark_target(in.a, "a");
        mark_target(in.t, "t");
        auto out = casio::suvat::solve_all(arena, in);
        print_lines(out);
        return 0;
    }
    if(program == "ComputerScience/booleanProgram.py" || program == "booleanProgram.py") {
        int mode = 1;
        try { mode = std::stoi(get(0)); } catch(...) {}
        if(mode == 4) {
            std::string lhs = get(1).empty() ? "A.(B+C)" : get(1);
            std::string rhs = get(2).empty() ? "A.B+A.C" : get(2);
            std::cout << "1. LHS = " << lhs << "\n";
            std::cout << "2. RHS = " << rhs << "\n";
            auto [proof, err] = casio::boolean::prove(lhs, rhs);
            if(!err.empty()) {
                std::cout << "Error: " << err << "\n";
                return 0;
            }
            int i = 3;
            for(auto const &ln : proof) std::cout << i++ << ". " << ln << "\n";
            std::cout << i << ". proved\n";
            return 0;
        }

        std::string expr = get(1);
        if(expr.empty()) {
            if(mode == 2) expr = "A.B";
            else if(mode == 3) expr = "A+B";
            else expr = "((B,.A),.B,),+A.B";
        }
        auto cur = casio::boolean::parse(expr);
        std::cout << "1. " << casio::boolean::show(cur) << "\n";
        if(mode == 2) {
            auto out = casio::boolean::normalise(casio::boolean::to_nand(cur));
            std::cout << "2. NAND form: " << casio::boolean::show(out) << "\n";
            return 0;
        }
        if(mode == 3) {
            auto out = casio::boolean::normalise(casio::boolean::to_nor(cur));
            std::cout << "2. NOR form: " << casio::boolean::show(out) << "\n";
            return 0;
        }

        int n = 2;
        std::set<std::string> seen;
        seen.insert(casio::boolean::show(cur));
        while(n <= 50) {
            auto hit = casio::boolean::step(cur);
            if(!hit.first) break;
            std::string next = casio::boolean::show(hit.first);
            if(seen.count(next)) break;
            cur = hit.first;
            seen.insert(next);
            std::cout << n << ". " << next << "    (" << hit.second << ")\n";
            ++n;
        }
        std::cout << "Result: " << casio::boolean::show(cur) << "\n";
        return 0;
    }
    if(program == "statsProgram.py") {
        casio::stats::Request req;
        try { req.mode = std::stoi(get(0)); } catch(...) { req.mode = 0; }
        if(req.mode == 2) {
            req.expr = get(1);
            req.expr2 = get(2);
        }
        else {
            std::ostringstream oss;
            for(std::size_t i = 1; i < lines.size(); i++) {
                if(i != 1) oss << "\n";
                oss << lines[i];
            }
            req.expr = oss.str();
        }
        auto out = casio::stats::run(arena, req);
        print_lines(out);
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
    bool is_stats = (flag == "--stats");

    std::string expr = (is_stdin_program || any_bool || is_suvat || is_int || is_alg || is_trig || is_derive || is_stats) ? (argc >= 3 ? argv[2] : "") : argv[1];
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
            std::string method, method_u;
            expr = strip_method_args(expr, method, method_u, false);
            if(!print_method_header("suvat", method, method_u)) return 1;
            if(expr.rfind("suvat(", 0) == 0 && expr.size() > 7 && expr.back() == ')') {
                expr = expr.substr(6, expr.size() - 7);
            }
            casio::suvat::Inputs in;
            in.target = "v";
            std::vector<std::string> targets;
            auto parse_kv = [&](std::string const &kv) {
                auto eq = kv.find('=');
                if(eq == std::string::npos) return;
                std::string k = trim(kv.substr(0, eq));
                std::string v = trim(kv.substr(eq + 1));
                if(k == "s") in.s = v;
                else if(k == "u") in.u = v;
                else if(k == "v") in.v = v;
                else if(k == "a") in.a = v;
                else if(k == "t") in.t = v;
                else if(k == "target") in.target = v;
                else if(k == "find") {
                    if(v.size() >= 2 && v.front() == '[' && v.back() == ']') v = v.substr(1, v.size() - 2);
                    for(auto part : split_top_csv(v)) {
                        part = trim(part);
                        if(!part.empty()) targets.push_back(part);
                    }
                }
            };
            for(auto const &part : split_top_csv(expr)) parse_kv(part);
            if(!targets.empty()) {
                for(std::size_t i = 0; i < targets.size(); ++i) {
                    casio::suvat::Inputs one = in;
                    one.target = targets[i];
                    auto lines = casio::suvat::solve(arena, one);
                    if(i) std::cout << "\n";
                    print_lines(lines);
                }
                return 0;
            }
            auto lines = casio::suvat::solve_all(arena, in);
            print_lines(lines);
            return 0;
        }
        if(is_int) {
            std::string method, method_u;
            expr = strip_method_args(expr, method, method_u, true);
            if(!print_method_header_for_expr("int", method, method_u, expr)) return 1;
            casio::integrate::Request req;
            req.expr = expr;
            req.method = method;
            auto lines = casio::integrate::run(arena, req);
            print_lines(lines);
            return 0;
        }
        if(is_alg) {
            std::string method, method_u, target;
            expr = strip_method_args(expr, method, method_u, false, &target);
            if(!target.empty() && expr.find('=') == std::string::npos) expr += "=" + target;
            if(!print_method_header("alg", method, method_u)) return 1;
            casio::algebra::Request req;
            req.method = method;
            auto unwrap_call = [](std::string const &text, std::string const &name) -> std::string {
                if(text.rfind(name, 0) != 0 || text.size() <= name.size() + 1 || text.back() != ')') return "";
                return text.substr(name.size(), text.size() - name.size() - 1);
            };
            std::string inner;
            if(!(inner = unwrap_call(expr, "range(")).empty()) {
                req.mode = 10;
                req.method = "range";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "domain(")).empty()) {
                req.mode = 10;
                req.method = "domain";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "complete_square(")).empty() ||
                    !(inner = unwrap_call(expr, "comp_square(")).empty()) {
                req.mode = 5;
                req.method = "complete_square";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "partfrac(")).empty() ||
                    !(inner = unwrap_call(expr, "pf(")).empty()) {
                req.mode = 0;
                req.method = "partfrac";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "binomial(")).empty() ||
                    !(inner = unwrap_call(expr, "binomial_series(")).empty()) {
                req.mode = 14;
                req.method = "binomial";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "newton(")).empty()) {
                req.mode = 12;
                req.method = "newton";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "solve(")).empty()) {
                auto has_trig = [](std::string const &s) {
                    for(char const *k : {"sin(", "cos(", "tan(", "sec(", "cosec(", "csc(", "cot("}) {
                        if(s.find(k) != std::string::npos) return true;
                    }
                    return false;
                };
                if(has_trig(inner)) {
                    auto parts = split_top_csv(inner);
                    std::string trig_expr = inner;
                    if(parts.size() >= 2) {
                        auto eq = parts[1].find('=');
                        auto dots = parts[1].find("..");
                        if(eq != std::string::npos && dots != std::string::npos && eq < dots) {
                            trig_expr = parts[0] + "," + trim(parts[1].substr(0, eq)) + "," +
                                        trim(parts[1].substr(eq + 1, dots - eq - 1)) + "," +
                                        trim(parts[1].substr(dots + 2));
                        }
                    }
                    casio::trig::Request treq;
                    treq.mode = (method == "rform" || method == "sin_cos" || method == "pythag" ||
                                 method == "double_angle" || method == "compound_angle") ? 4 : 0;
                    treq.expr = trig_expr;
                    auto lines = casio::trig::run(arena, treq);
                    print_lines(lines);
                    return 0;
                }
                req.mode = 6;
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "inverse(")).empty()) {
                req.mode = 8;
                auto parts = split_top_csv(inner);
                if(parts.size() >= 2 && parts[1].find_first_of("<>=") == std::string::npos) req.expr = parts[0];
                else req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "compare(")).empty() ||
                    !(inner = unwrap_call(expr, "match(")).empty()) {
                auto parts = split_top_csv(inner);
                req.mode = 1;
                if(parts.size() >= 2) req.expr = parts[0] + "\n" + parts[1];
                else if(auto eq = casio::parse_equation(arena, inner)) {
                    req.expr = casio::format_expr(arena, eq->lhs) + "\n" + casio::format_expr(arena, eq->rhs);
                }
                else req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "transform(")).empty() ||
                    !(inner = unwrap_call(expr, "xform(")).empty()) {
                auto parts = split_top_csv(inner);
                req.mode = 2;
                if(parts.size() >= 2) req.expr = parts[0] + "\n" + parts[1];
                else if(auto eq = casio::parse_equation(arena, inner)) {
                    req.expr = casio::format_expr(arena, eq->lhs) + "\n" + casio::format_expr(arena, eq->rhs);
                }
                else req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "rewrite(")).empty()) {
                auto parts = split_top_csv(inner);
                req.mode = 9;
                req.expr = parts.size() >= 2 ? parts[0] + "\n" + parts[1] : inner;
            }
            else if(!(inner = unwrap_call(expr, "fitconst(")).empty()) {
                req.mode = 15;
                req.method = "fitconst";
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "factor(")).empty()) {
                std::string inner_method, inner_u, inner_target;
                inner = strip_method_args(inner, inner_method, inner_u, false, &inner_target);
                req.mode = 13;
                req.method = inner_method.empty() ? "factor" : inner_method;
                req.expr = inner;
            }
            else if(!(inner = unwrap_call(expr, "expand(")).empty()) {
                std::string inner_method, inner_u, inner_target;
                inner = strip_method_args(inner, inner_method, inner_u, false, &inner_target);
                req.mode = 3;
                req.method = inner_method.empty() ? "expand" : inner_method;
                req.expr = inner;
            }
            else {
                req.mode = (method == "expand" && expr.find('=') == std::string::npos) ? 3 :
                           (method == "complete_square" && expr.find('=') == std::string::npos) ? 5 :
                           ((expr.find('=') != std::string::npos) ? 6 : 0);
                req.expr = expr;
            }
            auto lines = casio::algebra::run(arena, req);
            print_lines(lines);
            return 0;
        }
        if(is_trig) {
            std::string method, method_u, target;
            expr = strip_method_args(expr, method, method_u, false, &target);
            if(!print_method_header("trig", method, method_u)) return 1;
            casio::trig::Request req;
            req.mode = 0;
            if(expr.find('\n') != std::string::npos) req.mode = 1;
            bool trig_equation = expr.find('=') != std::string::npos;
            if(!trig_equation && (method == "general" || method == "bounded" || method == "cast" ||
               method == "square_then_check")) {
                expr += "=0";
                trig_equation = true;
            }
            if(!trig_equation && (method == "rform" || method == "sin_cos" || method == "pythag" ||
               method == "double_angle" || method == "compound_angle")) {
                req.mode = 4;
            }
            if(!target.empty()) {
                req.mode = 2;
                expr += "\n" + target;
            }
            req.expr = expr;
            auto lines = casio::trig::run(arena, req);
            print_lines(lines);
            return 0;
        }
        if(is_derive) {
            std::string method, method_u;
            expr = strip_method_args(expr, method, method_u, false);
            auto unwrap_call = [](std::string const &text, std::string const &name) -> std::string {
                if(text.rfind(name, 0) != 0 || text.size() <= name.size() + 1 || text.back() != ')') return "";
                return text.substr(name.size(), text.size() - name.size() - 1);
            };
            std::string inner;
            if(!(inner = unwrap_call(expr, "diff(")).empty() ||
               !(inner = unwrap_call(expr, "derive(")).empty()) {
                auto parts = split_top_csv(inner);
                if(!parts.empty()) {
                    std::vector<std::string> kept;
                    std::string inner_method;
                    for(auto const &p : parts) {
                        std::string v;
                        if(key_arg(p, "method", v) || key_arg(p, "met", v)) {
                            inner_method = v;
                            continue;
                        }
                        kept.push_back(p);
                    }
                    if(method.empty() && !inner_method.empty()) method = inner_method;
                    expr = kept[0];
                    expr += ",";
                    expr += kept.size() >= 2 && !kept[1].empty() ? kept[1] : "x";
                }
            }
            if(!print_method_header("derive", method, method_u)) return 1;
            casio::derive::Request req;
            // Accept optional prefix: "mode:<n>,<expr...>"
            // Example: --derive "mode:4,x^2"
            req.mode = 1;
            req.expr = expr;
            if(method == "implicit") req.mode = 2;
            else if(method == "param") req.mode = 3;
            else if(method == "param_second") req.mode = 5;
            else if(method == "first_principles") req.mode = 6;
            else if(method == "second") req.mode = 4;
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
            print_lines(lines);
            return 0;
        }
        if(is_stats) {
            std::string method, method_u;
            expr = strip_method_args(expr, method, method_u, false);
            if(!print_method_header("stats", method, method_u)) return 1;
            casio::stats::Request req;
            req.mode = 0;
            req.expr = expr;
            if(method == "regression" || method == "corr" || method == "correlation" ||
               method == "cov" || method == "covariance") {
                req.mode = 2;
            }
            auto lines = casio::stats::run(arena, req);
            print_lines(lines);
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
                print_lines(lines);
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

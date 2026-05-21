#include "parse.hpp"

#include "normalize.hpp"
#include "simplify.hpp"
#include "tokenize.hpp"

#include <cctype>
#include <stdexcept>
#include <string_view>

namespace casio
{
namespace
{

static bool starts_atom(std::string const *tok)
{
    if(!tok) return false;
    if(*tok == "(") return true;
    if(*tok == "+" || *tok == "-" || *tok == "*" || *tok == "/" || *tok == "^" ||
       *tok == "**" || *tok == ")" || *tok == "," || *tok == "=" || *tok == "!")
        return false;
    return true;
}

static bool atom_ok(std::string const *tok)
{
    if(!tok) return false;
    if(*tok == "+" || *tok == "-" || *tok == "*" || *tok == "/" || *tok == "^" ||
       *tok == "**" || *tok == ")" || *tok == "," || *tok == "=" || *tok == "!")
        return false;
    return true;
}

static bool is_number_token(std::string const &tok)
{
    return (!tok.empty() && (std::isdigit(static_cast<unsigned char>(tok[0])) || tok[0] == '.'));
}

static std::string lower(std::string const &s)
{
    std::string out;
    out.reserve(s.size());
    for(char c : s) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

struct Parser
{
    Arena &a;
    std::vector<std::string> toks;
    std::size_t pos = 0;

    std::string const *peek() const
    {
        if(pos >= toks.size()) return nullptr;
        return &toks[pos];
    }

    std::string take(std::string_view expect = {})
    {
        auto p = peek();
        if(!p) throw std::runtime_error("Unexpected end of input");
        if(!expect.empty() && *p != expect) {
            throw std::runtime_error("Expected " + std::string(expect) + " but found " + *p);
        }
        pos++;
        return *p;
    }

    NodeId consume_func_arg()
    {
        if(peek() && *peek() == "(") {
            take("(");
            NodeId out = parse_add();
            take(")");
            return out;
        }
        if(starts_atom(peek())) {
            return parse_atom();
        }
        throw std::runtime_error("Bad function form.");
    }

    NodeId parse_atom()
    {
        auto p = peek();
        if(p && *p == "(") {
            take("(");
            NodeId out = parse_add();
            take(")");
            return out;
        }
        if(!atom_ok(p)) throw std::runtime_error("Bad token.");
        std::string tok = take();

        if(is_number_token(tok)) {
            // Match casio_core.num_text()
            auto dot = tok.find('.');
            if(dot == std::string::npos) {
                return num(a, std::stoll(tok), 1);
            }
            std::string whole = tok.substr(0, dot);
            std::string frac = tok.substr(dot + 1);
            if(whole.empty()) whole = "0";
            std::int64_t scale = 1;
            for(std::size_t i = 0; i < frac.size(); i++) scale *= 10;
            int sign = (whole.size() && whole[0] == '-') ? -1 : 1;
            std::int64_t whole_abs = std::llabs(std::stoll(whole));
            std::int64_t frac_val = frac.empty() ? 0 : std::stoll(frac);
            std::int64_t value = whole_abs * scale + frac_val;
            return num(a, sign * value, scale);
        }

        std::string name = lower(tok);
        // FUNC_ALIASES
        if(name == "ln") name = "log";
        if(name == "csc") name = "cosec";
        if(name == "arcsin") name = "asin";
        if(name == "arccos") name = "acos";
        if(name == "arctan") name = "atan";
        if(name == "arcsinh") name = "asinh";
        if(name == "arccosh") name = "acosh";
        if(name == "arctanh") name = "atanh";
        if(name == "arsinh") name = "asinh";
        if(name == "arcosh") name = "acosh";
        if(name == "artanh") name = "atanh";
        if(name == "cuberoot") name = "cbrt";
        bool inv_sec = name == "arcsec" || name == "asec";
        bool inv_csc = name == "arccosec" || name == "arccsc" || name == "acsc";
        bool inv_cot = name == "arccot" || name == "acot";
        
        if(name == "pi") return constant_pi(a);
        if(name == "e") return constant_e(a);
        
        auto is_func = [&](std::string const &n) {
            static const char *funcs[] = {
                "sin","cos","tan","sec","cosec","cot",
                "exp","log","log10","sqrt","abs","sign","factorial",
                "atan","asin","acos","sinh","cosh","tanh","asinh","acosh","atanh",
                "cbrt"
            };
            for(auto f : funcs) if(n == f) return true;
            return false;
        };

        if(inv_sec || inv_csc || inv_cot) {
            NodeId arg = consume_func_arg();
            NodeId recip = div(a, num(a, 1), arg);
            return fn(a, inv_sec ? "acos" : inv_csc ? "asin" : "atan", recip);
        }

        if(is_func(name)) {
            // sin^2(x) shorthand
            if(peek() && (*peek() == "^" || *peek() == "**")) {
                take(*peek());
                NodeId exp = parse_unary();
                NodeId arg = consume_func_arg();
                NodeId inner = name == "cbrt" ? power(a, arg, num(a, 1, 3)) : fn(a, name, arg);
                return power(a, inner, exp);
            }
            if(peek() && *peek() == "(") {
                take("(");
                NodeId arg = parse_add();
                if(peek() && *peek() == ",") {
                    take(",");
                    NodeId arg2 = parse_add();
                    take(")");
                    if(name == "log") {
                        // log(a,b) -> log(b)/log(a)
                        return div(a, fn(a, "log", arg2), fn(a, "log", arg));
                    }
                    // Otherwise, function of 2 args becomes fn(name, arg*arg2) per python
                    return fn(a, name, mul(a, {arg, arg2}));
                }
                take(")");
                if(name == "cbrt") return power(a, arg, num(a, 1, 3));
                return fn(a, name, arg);
            }
            if(starts_atom(peek())) {
                NodeId arg = parse_atom();
                if(name == "cbrt") return power(a, arg, num(a, 1, 3));
                return fn(a, name, arg);
            }
        }

        return sym(a, tok);
    }

    NodeId parse_power()
    {
        NodeId out = parse_atom();
        while(peek() && *peek() == "!") {
            take("!");
            out = fn(a, "factorial", out);
        }
        if(peek() && (*peek() == "^" || *peek() == "**")) {
            take(*peek());
            out = power(a, out, parse_unary());
        }
        return out;
    }

    NodeId parse_unary()
    {
        if(peek() && *peek() == "-") {
            take("-");
            return neg(a, parse_unary());
        }
        return parse_power();
    }

    NodeId parse_mul()
    {
        NodeId out = parse_unary();
        while(true) {
            auto p = peek();
            if(p && *p == "*") {
                take("*");
                out = mul(a, {out, parse_unary()});
            }
            else if(p && *p == "/") {
                take("/");
                out = div(a, out, parse_unary());
            }
            else if(starts_atom(p)) {
                // implicit multiplication
                out = mul(a, {out, parse_unary()});
            }
            else return out;
        }
    }

    NodeId parse_add()
    {
        NodeId out = parse_mul();
        while(peek() && (*peek() == "+" || *peek() == "-")) {
            if(*peek() == "+") {
                take("+");
                out = add(a, {out, parse_mul()});
            }
            else {
                take("-");
                out = add(a, {out, neg(a, parse_mul())});
            }
        }
        return out;
    }
};

} // namespace

NodeId parse_expr(Arena &arena, std::string text)
{
    text = normalize_text(std::move(text));
    Parser p{arena, tokenize_expr(text), 0};
    NodeId root = p.parse_add();
    if(p.pos != p.toks.size()) {
        throw std::runtime_error("Unexpected token: " + *p.peek());
    }
    return simplify(arena, root);
}

} // namespace casio

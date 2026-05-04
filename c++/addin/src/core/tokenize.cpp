#include "tokenize.hpp"

#include <cctype>
#include <stdexcept>

namespace casio
{

std::vector<std::string> tokenize_expr(std::string const &text)
{
    std::vector<std::string> tokens;
    std::size_t i = 0;
    while(i < text.size()) {
        char ch = text[i];
        if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            i++;
            continue;
        }
        if(i + 1 < text.size() && text[i] == '*' && text[i + 1] == '*') {
            tokens.emplace_back("**");
            i += 2;
            continue;
        }
        if(std::string_view("()+-*/^=,!").find(ch) != std::string_view::npos) {
            tokens.emplace_back(1, ch);
            i++;
            continue;
        }
        if(std::isdigit(static_cast<unsigned char>(ch)) ||
           (ch == '.' && i + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[i + 1])))) {
            std::size_t j = i + 1;
            int dots = (ch == '.') ? 1 : 0;
            while(j < text.size() &&
                  (std::isdigit(static_cast<unsigned char>(text[j])) || text[j] == '.')) {
                if(text[j] == '.') dots++;
                j++;
            }
            if(dots > 1) throw std::runtime_error("Bad number.");
            tokens.push_back(text.substr(i, j - i));
            i = j;
            continue;
        }
        if(std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
            std::size_t j = i + 1;
            while(j < text.size()) {
                unsigned char cj = static_cast<unsigned char>(text[j]);
                if(std::isalnum(cj) || text[j] == '_') j++;
                else break;
            }
            tokens.push_back(text.substr(i, j - i));
            i = j;
            continue;
        }
        throw std::runtime_error(std::string("Unexpected character: ") + ch);
    }
    return tokens;
}

} // namespace casio

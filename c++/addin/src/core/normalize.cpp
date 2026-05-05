#include "normalize.hpp"

#include <cctype>
#include <string_view>
#include <utility>
#include <vector>

namespace casio
{
namespace
{

static bool is_digit_char(char ch) { return ch >= '0' && ch <= '9'; }
static bool is_alpha_char(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}
static bool is_name_start(char ch) { return is_alpha_char(ch) || ch == '_'; }
static bool is_name_char(char ch) { return is_name_start(ch) || is_digit_char(ch); }

// NOTE: Python version uses unicode superscript digits. We handle the same
// symbols by scanning UTF-8 and matching known sequences.
//
// This is intentionally simple (string replace); correctness over speed.
static void replace_all(std::string &s, std::string_view from, std::string_view to)
{
    if(from.empty()) return;
    std::size_t pos = 0;
    while(true) {
        pos = s.find(from, pos);
        if(pos == std::string::npos) break;
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::string normalize_superscripts(std::string text)
{
    // Map common superscripts to ASCII equivalents.
    // Python logic groups consecutive superscripts and prefixes with '^'.
    //
    // Here we implement a close analogue by repeatedly replacing known unicode
    // superscripts with a sentinel form, then post-processing runs.
    static const std::pair<std::string_view, std::string_view> map[] = {
        {"⁰", "0"}, {"¹", "1"}, {"²", "2"}, {"³", "3"}, {"⁴", "4"},
        {"⁵", "5"}, {"⁶", "6"}, {"⁷", "7"}, {"⁸", "8"}, {"⁹", "9"},
        {"⁻", "-"}, {"⁺", "+"},
    };

    std::string out;
    out.reserve(text.size());

    // Scan bytes; whenever we see a superscript token, start a run.
    std::size_t i = 0;
    while(i < text.size()) {
        bool matched = false;
        std::string run;

        // Try to match one superscript token at this position.
        for(auto const &kv : map) {
            auto key = kv.first;
            if(key.size() && text.compare(i, key.size(), key) == 0) {
                matched = true;
                run += std::string(kv.second);
                i += key.size();
                break;
            }
        }

        if(!matched) {
            out.push_back(text[i++]);
            continue;
        }

        // Continue consuming superscripts.
        while(i < text.size()) {
            bool m2 = false;
            for(auto const &kv : map) {
                auto key = kv.first;
                if(key.size() && text.compare(i, key.size(), key) == 0) {
                    m2 = true;
                    run += std::string(kv.second);
                    i += key.size();
                    break;
                }
            }
            if(!m2) break;
        }

        out.push_back('^');
        out += run;
    }

    return out;
}

static std::string normalize_inverse_trig_notation(std::string text)
{
    // Direct port of casio_core._normalize_inverse_trig_notation().
    static const std::pair<std::string_view, std::string_view> pairs[] = {
        {"arcsin^-1", "asin"},
        {"arccos^-1", "acos"},
        {"arctan^-1", "atan"},
        {"sin**-1", "asin"},
        {"cos**-1", "acos"},
        {"tan**-1", "atan"},
        {"sin^-1", "asin"},
        {"cos^-1", "acos"},
        {"tan^-1", "atan"},
    };
    for(auto const &p : pairs) replace_all(text, p.first, p.second);
    return text;
}

static std::string normalize_sqrt_symbol(std::string text)
{
    // Port of casio_core._normalize_sqrt_symbol().
    if(text.find("√") == std::string::npos) return text;

    std::string out;
    out.reserve(text.size());
    std::size_t i = 0;
    while(i < text.size()) {
        if(text.compare(i, 3, "√") != 0) {
            out.push_back(text[i++]);
            continue;
        }
        i += 3;
        while(i < text.size() && (text[i] == ' ' || text[i] == '\t')) i++;
        if(i < text.size() && text[i] == '(') {
            out += "sqrt";
            continue;
        }
        std::size_t start = i;
        if(i < text.size() && (is_name_start(text[i]) || is_digit_char(text[i]) || text[i] == '.')) {
            i++;
            while(i < text.size() && (is_name_char(text[i]) || text[i] == '.')) i++;
            out += "sqrt(";
            out.append(text, start, i - start);
            out += ")";
        }
        else {
            out += "sqrt";
        }
    }
    return out;
}

static char previous_significant_char(std::string const &text, std::size_t index)
{
    if(index == 0) return 0;
    std::size_t i = index;
    while(i-- > 0) {
        char ch = text[i];
        if(ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') return ch;
    }
    return 0;
}

static bool should_open_abs_pipe(std::string const &text, std::size_t index)
{
    char prev = previous_significant_char(text, index);
    if(prev == 0) return true;
    std::string_view openers = "([{,+-*/^=<>";
    return openers.find(prev) != std::string_view::npos;
}

static std::string convert_abs_pipes(std::string text)
{
    // Port of casio_core._convert_abs_pipes().
    std::size_t count = 0;
    for(char ch : text) if(ch == '|') count++;
    if(count % 2 != 0) return text;

    std::string out;
    out.reserve(text.size());
    int depth = 0;
    for(std::size_t i = 0; i < text.size(); i++) {
        char ch = text[i];
        if(ch == '|') {
            if(depth == 0 || should_open_abs_pipe(text, i)) {
                out += "abs(";
                depth++;
            }
            else {
                out += ")";
                depth--;
            }
        }
        else out.push_back(ch);
    }
    if(depth != 0) return text;
    return out;
}

static constexpr std::string_view compact_words[] = {
    "arcsinh", "arccosh", "arctanh",
    "arcsin", "arccos", "arctan",
    "asinh", "acosh", "atanh",
    "cosec",
    "asin", "acos", "atan",
    "sinh", "cosh", "tanh",
    "sqrt", "sin", "cos", "tan", "sec", "cot",
    "abs", "exp", "log", "ln", "csc",
};

static std::pair<std::string, std::string> split_compact_function_word(std::string const &word)
{
    // Port of casio_core._split_compact_function_word().
    std::string low;
    low.reserve(word.size());
    for(char c : word) low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    for(auto token : compact_words) {
        if(low == token) return {"", ""};
    }
    if(low == "pi" || low == "e") return {"", ""};

    for(auto token : compact_words) {
        if(low.rfind(token, 0) == 0 && word.size() > token.size()) {
            std::string name(token);
            if(name == "ln") name = "log";
            else if(name == "csc") name = "cosec";
            std::string rest = word.substr(token.size());
            return {name, rest};
        }
    }
    return {"", ""};
}

static std::string normalize_compact_function_words(std::string text)
{
    // Port of casio_core._normalize_compact_function_words().
    std::string out;
    out.reserve(text.size());
    std::size_t i = 0;
    while(i < text.size()) {
        char ch = text[i];
        if(is_name_start(ch)) {
            std::size_t j = i + 1;
            while(j < text.size() && is_name_char(text[j])) j++;
            std::string word = text.substr(i, j - i);
            auto [name, rest] = split_compact_function_word(word);
            if(!name.empty()) {
                out += name;
                out.push_back('(');
                out += normalize_compact_function_words(rest);
                out.push_back(')');
            }
            else {
                out += word;
            }
            i = j;
            continue;
        }
        out.push_back(ch);
        i++;
    }
    return out;
}

static std::string normalize_function_pipe_calls(std::string text)
{
    // Port of casio_core._normalize_function_pipe_calls(): accepts sin|x| form.
    std::string out;
    out.reserve(text.size());

    std::size_t i = 0;
    while(i < text.size()) {
        bool matched = false;
        for(auto token : compact_words) {
            std::size_t end = i + token.size();
            if(end > text.size()) continue;

            // case-insensitive compare for token
            bool eq = true;
            for(std::size_t k = 0; k < token.size(); k++) {
                char a = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i + k])));
                char b = static_cast<char>(std::tolower(static_cast<unsigned char>(token[k])));
                if(a != b) { eq = false; break; }
            }
            if(!eq) continue;

            bool prev_ok = (i == 0) || !is_name_char(text[i - 1]);
            if(!prev_ok) continue;
            if(end >= text.size() || text[end] != '|') continue;

            // Find closing pipe.
            std::size_t close = end + 1;
            while(close < text.size() && text[close] != '|') close++;
            if(close >= text.size()) continue;

            // Expand: token|...| -> token(abs(...)) will be handled by convert_abs_pipes later.
            out.append(text, i, end - i);
            out.push_back('(');
            out.push_back('|');
            out.append(text, end + 1, close - (end + 1));
            out.push_back('|');
            out.push_back(')');
            i = close + 1;
            matched = true;
            break;
        }
        if(matched) continue;
        out.push_back(text[i++]);
    }

    return out;
}

} // namespace

std::string normalize_text(std::string text)
{
    // Port of python/src/Math/casio_core.py:normalize_text().
    // NOTE: This function assumes UTF-8 input and performs a few UTF-8 substring matches.
    if(text.empty()) return text;

    // Trim (ASCII whitespace only; matches Python's str.strip() sufficiently here).
    while(!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r' || text.front() == '\n')) {
        text.erase(text.begin());
    }
    while(!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r' || text.back() == '\n')) {
        text.pop_back();
    }

    // Symbol replacements (unicode operators etc.).
    replace_all(text, "−", "-");
    replace_all(text, "–", "-");
    replace_all(text, "—", "-");
    replace_all(text, "×", "*");
    replace_all(text, "∗", "*");
    replace_all(text, "⋅", "*");
    replace_all(text, "÷", "/");
    replace_all(text, "⁄", "/");
    replace_all(text, "π", "pi");
    replace_all(text, "Π", "pi");
    replace_all(text, "°", "");
    replace_all(text, "½", "(1/2)");
    replace_all(text, "¼", "(1/4)");
    replace_all(text, "¾", "(3/4)");

    text = normalize_superscripts(std::move(text));
    text = normalize_inverse_trig_notation(std::move(text));
    text = normalize_sqrt_symbol(std::move(text));
    if(text.find('|') != std::string::npos) {
        text = normalize_function_pipe_calls(std::move(text));
        text = convert_abs_pipes(std::move(text));
    }
    text = normalize_compact_function_words(std::move(text));
    return text;
}

} // namespace casio

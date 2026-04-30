#include "boolean.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <stdexcept>

namespace casio::boolean
{
namespace
{

NodeRef make_const(char v)
{
    auto n = std::make_shared<Node>();
    n->kind = Kind::Const;
    n->text = std::string(1, v);
    return n;
}

NodeRef make_var(std::string name)
{
    auto n = std::make_shared<Node>();
    n->kind = Kind::Var;
    n->text = std::move(name);
    return n;
}

NodeRef make1(Kind k, NodeRef child)
{
    auto n = std::make_shared<Node>();
    n->kind = k;
    n->kids = {std::move(child)};
    return n;
}

NodeRef mk(Kind kind, std::vector<NodeRef> items)
{
    // Port of booleanProgram.mk(): flatten same-kind children.
    std::vector<NodeRef> flat;
    for(auto const &it : items) {
        if(it && it->kind == kind) {
            for(auto const &c : it->kids) flat.push_back(c);
        }
        else flat.push_back(it);
    }
    if(flat.empty()) {
        return (kind == Kind::And) ? O() : Z();
    }
    if(flat.size() == 1) return flat[0];
    auto n = std::make_shared<Node>();
    n->kind = kind;
    n->kids = std::move(flat);
    return n;
}

static void kids(NodeRef const &node, Kind kind, std::vector<NodeRef> &out)
{
    if(!node || node->kind != kind) {
        out.push_back(node);
        return;
    }
    for(auto const &ch : node->kids) {
        if(ch && ch->kind == kind) kids(ch, kind, out);
        else out.push_back(ch);
    }
}

static std::string sig(NodeRef const &node);

static bool has(std::vector<NodeRef> const &items, NodeRef const &target)
{
    for(auto const &it : items) if(sig(it) == sig(target)) return true;
    return false;
}

static bool sub(std::vector<NodeRef> const &a, std::vector<NodeRef> const &b)
{
    for(auto const &it : a) if(!has(b, it)) return false;
    return true;
}

static std::vector<NodeRef> rem1(std::vector<NodeRef> items, NodeRef const &target)
{
    bool done = false;
    std::vector<NodeRef> out;
    for(auto const &it : items) {
        if(!done && sig(it) == sig(target)) done = true;
        else out.push_back(it);
    }
    return out;
}

static int prio(NodeRef const &node)
{
    if(!node) return 0;
    if(node->kind == Kind::Const || node->kind == Kind::Var) return 4;
    if(node->kind == Kind::Not) return 3;
    if(node->kind == Kind::And) return 2;
    return 1;
}

static bool simple_not(NodeRef const &node)
{
    if(!node) return false;
    return node->kind == Kind::Const || node->kind == Kind::Var || node->kind == Kind::Not;
}

static std::string sig(NodeRef const &node)
{
    if(!node) return "";
    if(node->kind == Kind::Const) return node->text;
    if(node->kind == Kind::Var) return node->text;
    if(node->kind == Kind::Not) return "n(" + sig(node->kids.at(0)) + ")";

    Kind kind = node->kind;
    std::vector<std::string> parts;
    std::vector<NodeRef> flat;
    kids(node, kind, flat);
    for(auto const &ch : flat) parts.push_back(sig(ch));
    std::sort(parts.begin(), parts.end());
    std::string head = (kind == Kind::And) ? "a(" : "o(";
    std::string out = head;
    for(std::size_t i = 0; i < parts.size(); i++) {
        if(i) out += ",";
        out += parts[i];
    }
    out += ")";
    return out;
}

static std::vector<std::string> expand_vars(std::string token)
{
    // Port of booleanProgram.expand_vars().
    for(char &c : token) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if(token.size() <= 1) return {token};
    std::vector<std::string> result;
    std::string current;
    current.push_back(token[0]);
    for(std::size_t i = 1; i < token.size(); i++) {
        char ch = token[i];
        if(ch >= 'A' && ch <= 'Z') {
            result.push_back(current);
            current = std::string(1, ch);
        }
        else current.push_back(ch);
    }
    result.push_back(current);
    return result;
}

} // namespace

NodeRef Z()
{
    static NodeRef z = make_const('0');
    return z;
}

NodeRef O()
{
    static NodeRef o = make_const('1');
    return o;
}

bool same(NodeRef const &a, NodeRef const &b) { return sig(a) == sig(b); }

bool comp(NodeRef const &a, NodeRef const &b)
{
    // Port: return (a[0]=='n' and same(a[1], b)) or vice versa.
    if(!a || !b) return false;
    if(a->kind == Kind::Not && same(a->kids.at(0), b)) return true;
    if(b->kind == Kind::Not && same(b->kids.at(0), a)) return true;
    return false;
}

std::string show(NodeRef const &node)
{
    // Port of booleanProgram.show() (with caching removed).
    if(!node) return "";

    if(node->kind == Kind::Const || node->kind == Kind::Var) return node->text;

    if(node->kind == Kind::Not) {
        auto child = node->kids.at(0);
        std::string t = show(child);
        if(simple_not(child)) t += ",";
        else t = "(" + t + "),";
        if(prio(node) < 0) return "(" + t + ")";
        return t;
    }

    std::string joiner = ".";
    if(node->kind == Kind::Or) joiner = " + ";

    std::vector<NodeRef> flat;
    kids(node, node->kind, flat);
    std::vector<std::string> parts;
    for(auto const &ch : flat) {
        std::string part = show(ch);
        if(prio(ch) < prio(node)) part = "(" + part + ")";
        parts.push_back(part);
    }
    std::string text;
    for(std::size_t i = 0; i < parts.size(); i++) {
        if(i) text += joiner;
        text += parts[i];
    }
    if(prio(node) < 0) return "(" + text + ")";
    return text;
}

NodeRef parse(std::string const &text)
{
    // Port of booleanProgram.parse().
    std::string up;
    up.reserve(text.size());
    for(char c : text) up.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));

    std::vector<std::string> toks;
    std::size_t i = 0;
    while(i < up.size()) {
        char ch = up[i];
        if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            i++;
        }
        else if(std::string_view("(),+.").find(ch) != std::string_view::npos) {
            toks.emplace_back(1, ch);
            i++;
        }
        else if(ch == '0' || ch == '1') {
            toks.emplace_back(1, ch);
            i++;
        }
        else if((ch >= 'A' && ch <= 'Z') || ch == '_') {
            std::size_t j = i + 1;
            while(j < up.size()) {
                char cj = up[j];
                if((cj >= 'A' && cj <= 'Z') || (cj >= '0' && cj <= '9') || cj == '_') j++;
                else break;
            }
            auto expanded = expand_vars(up.substr(i, j - i));
            for(std::size_t k = 0; k + 1 < expanded.size(); k++) {
                toks.push_back(expanded[k]);
                toks.emplace_back(".");
            }
            toks.push_back(expanded.back());
            i = j;
        }
        else throw std::runtime_error(std::string("Unexpected character: ") + ch);
    }

    std::size_t p = 0;
    auto cur = [&]() -> std::string {
        if(p >= toks.size()) return "";
        return toks[p];
    };
    auto eat = [&](std::string const &x) {
        if(cur() != x) throw std::runtime_error("Expected '" + x + "' but found '" + cur() + "'.");
        p++;
    };

    std::function<NodeRef()> expr;
    std::function<NodeRef()> term;
    std::function<NodeRef()> atom;

    atom = [&]() -> NodeRef {
        std::string t = cur();
        NodeRef node;
        if(t == "(") {
            eat("(");
            node = expr();
            eat(")");
        }
        else if(t == "0") { p++; node = Z(); }
        else if(t == "1") { p++; node = O(); }
        else if(!t.empty() && t[0] != '(' && t[0] != ')' && t[0] != ',' && t[0] != '+' && t[0] != '.') {
            p++;
            node = make_var(t);
        }
        else throw std::runtime_error("Unexpected token: " + t);

        while(cur() == ",") {
            eat(",");
            node = make1(Kind::Not, node);
        }
        return node;
    };

    term = [&]() -> NodeRef {
        NodeRef node = atom();
        std::vector<NodeRef> parts{node};
        while(cur() == ".") {
            eat(".");
            parts.push_back(atom());
        }
        return mk(Kind::And, std::move(parts));
    };

    expr = [&]() -> NodeRef {
        NodeRef node = term();
        std::vector<NodeRef> parts{node};
        while(cur() == "+") {
            eat("+");
            parts.push_back(term());
        }
        return mk(Kind::Or, std::move(parts));
    };

    NodeRef node = expr();
    if(!cur().empty()) throw std::runtime_error("Unexpected token: " + cur());
    return node;
}

std::pair<NodeRef, std::string> step(NodeRef const &node)
{
    // Partial port: enough for early parity. Full port later.
    if(!node) return {nullptr, ""};
    Kind kind = node->kind;
    if(kind == Kind::Const || kind == Kind::Var) return {nullptr, ""};

    if(kind == Kind::Not) {
        auto hit = step(node->kids.at(0));
        if(hit.first) return {make1(Kind::Not, hit.first), hit.second};

        auto child = node->kids.at(0);
        if(same(child, Z()) || same(child, O())) {
            return {same(child, Z()) ? O() : Z(), "Common sense"};
        }
        if(child->kind == Kind::Not) return {child->kids.at(0), "Double complement"};
        if(child->kind == Kind::And) {
            std::vector<NodeRef> out;
            std::vector<NodeRef> parts;
            kids(child, Kind::And, parts);
            for(auto const &it : parts) out.push_back(make1(Kind::Not, it));
            return {mk(Kind::Or, std::move(out)), "De Morgan's law"};
        }
        if(child->kind == Kind::Or) {
            std::vector<NodeRef> out;
            std::vector<NodeRef> parts;
            kids(child, Kind::Or, parts);
            for(auto const &it : parts) out.push_back(make1(Kind::Not, it));
            return {mk(Kind::And, std::move(out)), "De Morgan's law"};
        }
        return {nullptr, ""};
    }

    // Recurse into children first.
    std::vector<NodeRef> parts;
    kids(node, kind, parts);
    for(std::size_t i = 0; i < parts.size(); i++) {
        auto hit = step(parts[i]);
        if(hit.first) {
            auto new_parts = parts;
            new_parts[i] = hit.first;
            return {mk(kind, std::move(new_parts)), hit.second};
        }
    }

    // Common sense short-circuits
    if(kind == Kind::Or) {
        for(auto const &it : parts) if(same(it, O())) return {O(), "Common sense"};
        if(parts.size() > 1) {
            for(std::size_t i = 0; i < parts.size(); i++) {
                if(same(parts[i], Z())) {
                    auto newp = parts;
                    newp.erase(newp.begin() + i);
                    return {mk(Kind::Or, std::move(newp)), "Common sense"};
                }
            }
        }
    }
    else { // And
        for(auto const &it : parts) if(same(it, Z())) return {Z(), "Common sense"};
        if(parts.size() > 1) {
            for(std::size_t i = 0; i < parts.size(); i++) {
                if(same(parts[i], O())) {
                    auto newp = parts;
                    newp.erase(newp.begin() + i);
                    return {mk(Kind::And, std::move(newp)), "Common sense"};
                }
            }
        }
    }

    // Tautology and complements
    for(std::size_t i = 0; i < parts.size(); i++) {
        for(std::size_t j = i + 1; j < parts.size(); j++) {
            if(same(parts[i], parts[j])) {
                auto newp = parts;
                newp.erase(newp.begin() + j);
                return {mk(kind, std::move(newp)), "Tautology"};
            }
            if(comp(parts[i], parts[j])) {
                if(parts.size() == 2) return {(kind == Kind::Or) ? O() : Z(), "Tautology"};
                auto newp = parts;
                newp[i] = (kind == Kind::Or) ? O() : Z();
                newp.erase(newp.begin() + j);
                return {mk(kind, std::move(newp)), "Tautology"};
            }
        }
    }

    return {nullptr, ""};
}

} // namespace casio::boolean


#include "device/device_solver.hpp"

namespace casio::device
{
namespace
{

struct Linear {
    int x = 0;
    int c = 0;
    bool ok = true;
};

struct Poly {
    int coeff[6]{};
    bool ok = true;
};

struct FractionTerm {
    int coeff = 0;
    int den = 1;
    int power = 0;
};

static int abs_int(int v)
{
    return v < 0 ? -v : v;
}

static int gcd_int(int a, int b)
{
    a = abs_int(a);
    b = abs_int(b);
    while(b != 0) {
        int r = a % b;
        a = b;
        b = r;
    }
    return a == 0 ? 1 : a;
}

static bool read_int(const char *s, int &i, int end, int &out)
{
    if(i >= end || !is_digit(s[i])) return false;
    int value = 0;
    while(i < end && is_digit(s[i])) {
        value = value * 10 + (s[i] - '0');
        i++;
    }
    out = value;
    return true;
}

static int compact(const char *src, char *dst, int cap)
{
    int n = 0;
    if(cap <= 0) return 0;
    for(int i = 0; src != nullptr && src[i] != '\0'; i++) {
        if(is_space(src[i])) continue;
        if(n + 1 >= cap) break;
        dst[n++] = lower_ascii(src[i]);
    }
    dst[n] = '\0';
    return n;
}

static int find_char(const char *s, char needle)
{
    for(int i = 0; s != nullptr && s[i] != '\0'; i++) {
        if(s[i] == needle) return i;
    }
    return -1;
}

static void append_term(FixedString<96> &line, int coeff, bool x_term)
{
    if(x_term) {
        if(coeff == 1) line.append("x");
        else if(coeff == -1) line.append("-x");
        else {
            line.append_int(coeff);
            line.append("x");
        }
    }
    else {
        line.append_int(coeff);
    }
}

static void append_linear(FixedString<96> &line, Linear const &v)
{
    bool wrote = false;
    if(v.x != 0) {
        append_term(line, v.x, true);
        wrote = true;
    }
    if(v.c != 0 || !wrote) {
        if(wrote && v.c > 0) line.append(" + ");
        if(wrote && v.c < 0) {
            line.append(" - ");
            line.append_int(-v.c);
        }
        else line.append_int(v.c);
    }
}

static Linear parse_linear_range(const char *s, int begin, int end)
{
    Linear out;
    int i = begin;
    int sign = 1;
    bool consumed = false;

    while(i < end) {
        if(s[i] == '+') {
            sign = 1;
            i++;
            continue;
        }
        if(s[i] == '-') {
            sign = -1;
            i++;
            continue;
        }

        int coeff = 1;
        int number = 0;
        bool has_number = read_int(s, i, end, number);
        if(has_number) coeff = number;
        if(i < end && s[i] == '*') i++;

        if(i < end && s[i] == 'x') {
            out.x += sign * coeff;
            i++;
        }
        else if(has_number) {
            out.c += sign * coeff;
        }
        else {
            out.ok = false;
            return out;
        }

        consumed = true;
        sign = 1;
    }

    out.ok = consumed;
    return out;
}

static bool parse_poly_term(const char *s, int begin, int end, int sign, Poly &poly)
{
    int i = begin;
    int coeff = 1;
    int number = 0;
    bool has_number = read_int(s, i, end, number);
    if(has_number) coeff = number;
    if(i < end && s[i] == '*') i++;

    if(i < end && s[i] == 'x') {
        int power = 1;
        i++;
        if(i < end && s[i] == '^') {
            i++;
            if(!read_int(s, i, end, power)) return false;
        }
        if(power < 0 || power > 5 || i != end) return false;
        poly.coeff[power] += sign * coeff;
        return true;
    }

    if(has_number && i == end) {
        poly.coeff[0] += sign * coeff;
        return true;
    }

    return false;
}

static Poly parse_poly(const char *text)
{
    char s[128];
    int n = compact(text, s, (int)sizeof(s));
    Poly poly;
    if(n == 0) {
        poly.ok = false;
        return poly;
    }

    int start = 0;
    int sign = 1;
    if(s[0] == '+') start = 1;
    else if(s[0] == '-') {
        sign = -1;
        start = 1;
    }

    for(int i = start; i <= n; i++) {
        if(i == n || s[i] == '+' || s[i] == '-') {
            if(i == start || !parse_poly_term(s, start, i, sign, poly)) {
                poly.ok = false;
                return poly;
            }
            if(i < n) {
                sign = s[i] == '-' ? -1 : 1;
                start = i + 1;
            }
        }
    }
    return poly;
}

static void append_power(FixedString<96> &line, int power)
{
    if(power == 0) return;
    line.append("x");
    if(power != 1) {
        line.append("^");
        line.append_int(power);
    }
}

static void append_poly(FixedString<96> &line, Poly const &poly)
{
    bool wrote = false;
    for(int p = 5; p >= 0; p--) {
        int c = poly.coeff[p];
        if(c == 0) continue;
        if(wrote) line.append(c < 0 ? " - " : " + ");
        else if(c < 0) line.append("-");

        int mag = abs_int(c);
        if(p == 0 || mag != 1) line.append_int(mag);
        if(p > 0) append_power(line, p);
        wrote = true;
    }
    if(!wrote) line.append("0");
}

static void append_fraction_poly(FixedString<96> &line, FractionTerm const *terms, int count)
{
    bool wrote = false;
    for(int i = count - 1; i >= 0; i--) {
        int c = terms[i].coeff;
        int d = terms[i].den;
        int p = terms[i].power;
        if(c == 0) continue;

        int g = gcd_int(c, d);
        c /= g;
        d /= g;

        if(wrote) line.append(c < 0 ? " - " : " + ");
        else if(c < 0) line.append("-");

        int mag = abs_int(c);
        if(d == 1) {
            if(p == 0 || mag != 1) line.append_int(mag);
            if(p > 0) append_power(line, p);
        }
        else {
            if(mag != 1) line.append_int(mag);
            if(p > 0) append_power(line, p);
            else line.append("1");
            line.append("/");
            line.append_int(d);
        }
        wrote = true;
    }
    if(!wrote) line.append("0");
}

static void add_input_line(OutputLines &out, const char *prefix, const char *input)
{
    FixedString<96> &line = out.next();
    line.append(prefix);
    line.append(input);
}

static bool solve_simplify(const char *input, OutputLines &out)
{
    char s[128];
    int n = compact(input, s, (int)sizeof(s));
    Linear expr = parse_linear_range(s, 0, n);
    if(!expr.ok) {
        out.add("Unsupported: use terms like 2x+3-x+4.");
        return false;
    }

    add_input_line(out, "1. Start: ", input);
    out.add("2. Collect constant and x terms.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_linear(ans, expr);
    return true;
}

static bool solve_algebra(const char *input, OutputLines &out)
{
    char s[128];
    int n = compact(input, s, (int)sizeof(s));
    int eq = find_char(s, '=');
    if(eq <= 0 || eq >= n - 1) {
        out.add("Unsupported: enter a linear equation like 2x+3=7.");
        return false;
    }

    Linear left = parse_linear_range(s, 0, eq);
    Linear right = parse_linear_range(s, eq + 1, n);
    if(!left.ok || !right.ok) {
        out.add("Unsupported: only integer linear terms are supported.");
        return false;
    }

    int a = left.x - right.x;
    int b = right.c - left.c;

    add_input_line(out, "1. Start: ", input);

    FixedString<96> &collect = out.next();
    collect.append("2. Collect x terms: ");
    append_term(collect, a, true);
    collect.append(" = ");
    collect.append_int(b);

    if(a == 0) {
        out.add(b == 0 ? "Answer: infinitely many solutions." : "Answer: no solution.");
        return b == 0;
    }

    FixedString<96> &divide = out.next();
    divide.append("3. Divide both sides by ");
    divide.append_int(a);
    divide.append(".");

    int g = gcd_int(b, a);
    int num = b / g;
    int den = a / g;
    if(den < 0) {
        den = -den;
        num = -num;
    }

    FixedString<96> &ans = out.next();
    ans.append("Answer: x = ");
    if(den == 1) ans.append_int(num);
    else {
        ans.append_int(num);
        ans.append("/");
        ans.append_int(den);
    }
    return true;
}

static bool solve_derive(const char *input, OutputLines &out)
{
    Poly p = parse_poly(input);
    if(!p.ok) {
        out.add("Unsupported: use polynomial terms up to x^5.");
        return false;
    }

    Poly d;
    for(int power = 1; power <= 5; power++) {
        d.coeff[power - 1] += p.coeff[power] * power;
    }

    add_input_line(out, "1. Start: y = ", input);
    out.add("2. Use d/dx(ax^n) = anx^(n-1).");
    out.add("3. Differentiate each term.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: dy/dx = ");
    append_poly(ans, d);
    return true;
}

static bool solve_integrate(const char *input, OutputLines &out)
{
    Poly p = parse_poly(input);
    if(!p.ok) {
        out.add("Unsupported: use polynomial terms up to x^5.");
        return false;
    }

    FractionTerm terms[6];
    int count = 0;
    for(int power = 0; power <= 5; power++) {
        if(p.coeff[power] == 0) continue;
        terms[count].coeff = p.coeff[power];
        terms[count].den = power + 1;
        terms[count].power = power + 1;
        count++;
    }

    add_input_line(out, "1. Start: integrate ", input);
    out.add("2. Use int(ax^n) = ax^(n+1)/(n+1).");
    out.add("3. Add the constant of integration.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_fraction_poly(ans, terms, count);
    ans.append(" + C");
    return true;
}

static const char *exact_trig(const char *fn, int deg)
{
    while(deg < 0) deg += 360;
    deg %= 360;

    if(cstr_eq(fn, "sin")) {
        if(deg == 0 || deg == 180) return "0";
        if(deg == 30 || deg == 150) return "1/2";
        if(deg == 45 || deg == 135) return "sqrt(2)/2";
        if(deg == 60 || deg == 120) return "sqrt(3)/2";
        if(deg == 90) return "1";
        if(deg == 210 || deg == 330) return "-1/2";
        if(deg == 225 || deg == 315) return "-sqrt(2)/2";
        if(deg == 240 || deg == 300) return "-sqrt(3)/2";
        if(deg == 270) return "-1";
    }
    if(cstr_eq(fn, "cos")) {
        if(deg == 0) return "1";
        if(deg == 30 || deg == 330) return "sqrt(3)/2";
        if(deg == 45 || deg == 315) return "sqrt(2)/2";
        if(deg == 60 || deg == 300) return "1/2";
        if(deg == 90 || deg == 270) return "0";
        if(deg == 120 || deg == 240) return "-1/2";
        if(deg == 135 || deg == 225) return "-sqrt(2)/2";
        if(deg == 150 || deg == 210) return "-sqrt(3)/2";
        if(deg == 180) return "-1";
    }
    if(cstr_eq(fn, "tan")) {
        if(deg == 0 || deg == 180) return "0";
        if(deg == 30 || deg == 210) return "sqrt(3)/3";
        if(deg == 45 || deg == 225) return "1";
        if(deg == 60 || deg == 240) return "sqrt(3)";
        if(deg == 90 || deg == 270) return "undefined";
        if(deg == 120 || deg == 300) return "-sqrt(3)";
        if(deg == 135 || deg == 315) return "-1";
        if(deg == 150 || deg == 330) return "-sqrt(3)/3";
    }
    return nullptr;
}

static bool solve_trig(const char *input, OutputLines &out)
{
    char s[64];
    int n = compact(input, s, (int)sizeof(s));
    int lp = find_char(s, '(');
    int rp = find_char(s, ')');
    if(lp <= 0 || rp <= lp + 1 || rp != n - 1) {
        out.add("Unsupported: use sin(30), cos(60), or tan(45).");
        return false;
    }

    char fn[4];
    int fn_len = lp < 3 ? lp : 3;
    for(int i = 0; i < fn_len; i++) fn[i] = s[i];
    fn[fn_len] = '\0';

    int i = lp + 1;
    int deg = 0;
    bool neg = false;
    if(s[i] == '-') {
        neg = true;
        i++;
    }
    if(!read_int(s, i, rp, deg) || i != rp) {
        out.add("Unsupported: common degree angles only for now.");
        return false;
    }
    if(neg) deg = -deg;

    const char *value = exact_trig(fn, deg);
    if(value == nullptr) {
        out.add("Unsupported: exact table covers 0,30,45,60,90...");
        return false;
    }

    add_input_line(out, "1. Start: ", input);
    out.add("2. Reduce angle to the unit-circle table.");
    out.add("3. Read the exact special-angle value.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    ans.append(value);
    return true;
}

struct SuvatInputs {
    bool has_s = false;
    bool has_u = false;
    bool has_v = false;
    bool has_a = false;
    bool has_t = false;
    int s = 0;
    int u = 0;
    int v = 0;
    int a = 0;
    int t = 0;
    char target = '\0';
};

static bool parse_value(const char *s, int begin, int end, int &out)
{
    int i = begin;
    bool neg = false;
    if(i < end && s[i] == '-') {
        neg = true;
        i++;
    }
    int value = 0;
    if(!read_int(s, i, end, value) || i != end) return false;
    out = neg ? -value : value;
    return true;
}

static bool parse_suvat(const char *input, SuvatInputs &out)
{
    char s[128];
    int n = compact(input, s, (int)sizeof(s));
    int start = 0;
    while(start < n) {
        int end = start;
        while(end < n && s[end] != ',') end++;
        int eq = -1;
        for(int i = start; i < end; i++) {
            if(s[i] == '=') {
                eq = i;
                break;
            }
        }
        if(eq <= start || eq >= end - 1) return false;
        char key = s[start];
        int value = 0;
        if(key == 't' && starts_with(s + start, "target=")) {
            out.target = s[eq + 1];
        }
        else {
            bool known = !(s[eq + 1] == '?' || s[eq + 1] == '_');
            if(known && !parse_value(s, eq + 1, end, value)) return false;
            if(key == 's') { out.has_s = known; out.s = value; if(!known) out.target = 's'; }
            else if(key == 'u') { out.has_u = known; out.u = value; if(!known) out.target = 'u'; }
            else if(key == 'v') { out.has_v = known; out.v = value; if(!known) out.target = 'v'; }
            else if(key == 'a') { out.has_a = known; out.a = value; if(!known) out.target = 'a'; }
            else if(key == 't') { out.has_t = known; out.t = value; if(!known) out.target = 't'; }
            else return false;
        }
        start = end + 1;
    }
    return out.target != '\0';
}

static void append_fraction_value(FixedString<96> &line, int num, int den)
{
    int g = gcd_int(num, den);
    num /= g;
    den /= g;
    if(den < 0) {
        den = -den;
        num = -num;
    }
    if(den == 1) line.append_int(num);
    else {
        line.append_int(num);
        line.append("/");
        line.append_int(den);
    }
}

static bool solve_suvat(const char *input, OutputLines &out)
{
    SuvatInputs in;
    if(!parse_suvat(input, in)) {
        out.add("Use s=10,u=0,v=?,a=2,t=5 or target=v.");
        return false;
    }

    add_input_line(out, "1. Start: ", input);
    if(in.target == 'v' && in.has_u && in.has_a && in.has_t) {
        out.add("2. Use v = u + at.");
        out.add("3. Substitute known values.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: v = ");
        ans.append_int(in.u + in.a * in.t);
        return true;
    }
    if(in.target == 's' && in.has_u && in.has_a && in.has_t) {
        out.add("2. Use s = ut + (1/2)at^2.");
        out.add("3. Substitute known values.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: s = ");
        append_fraction_value(ans, 2 * in.u * in.t + in.a * in.t * in.t, 2);
        return true;
    }
    if(in.target == 't' && in.has_u && in.has_v && in.has_a && in.a != 0) {
        out.add("2. Use v = u + at.");
        out.add("3. Rearrange to t = (v-u)/a.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: t = ");
        append_fraction_value(ans, in.v - in.u, in.a);
        return true;
    }

    out.add("Unsupported: this SUVAT combination is not ported yet.");
    return false;
}

} // namespace

bool solve(Module module, const char *input, OutputLines &out)
{
    out.clear();
    if(input == nullptr || input[0] == '\0') {
        out.add("Enter an expression first.");
        return false;
    }

    switch(module) {
        case Module::Shell:
            if(starts_with(input, "diff(")) return solve_derive(input + 5, out);
            if(starts_with(input, "int(")) return solve_integrate(input + 4, out);
            if(find_char(input, '=') >= 0) return solve_algebra(input, out);
            return solve_simplify(input, out);
        case Module::Simplify: return solve_simplify(input, out);
        case Module::Algebra: return solve_algebra(input, out);
        case Module::Derive: return solve_derive(input, out);
        case Module::Integrate: return solve_integrate(input, out);
        case Module::Trig: return solve_trig(input, out);
        case Module::Suvat: return solve_suvat(input, out);
    }

    out.add("Unsupported module.");
    return false;
}

} // namespace casio::device

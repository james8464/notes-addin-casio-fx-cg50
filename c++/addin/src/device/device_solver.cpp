#include "device/device_solver.hpp"

namespace casio::device
{
namespace
{

struct Fraction {
    int num = 0;
    int den = 1;
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

static bool is_square_int(int value, int &root)
{
    if(value < 0) return false;
    root = 0;
    while(root * root < value) root++;
    return root * root == value;
}

static Fraction make_fraction(int num, int den)
{
    Fraction f;
    if(den < 0) {
        num = -num;
        den = -den;
    }
    int g = gcd_int(num, den);
    f.num = num / g;
    f.den = den / g;
    return f;
}

static bool less_fraction(Fraction const &a, Fraction const &b)
{
    return a.num * b.den < b.num * a.den;
}

static bool same_fraction(Fraction const &a, Fraction const &b)
{
    return a.num == b.num && a.den == b.den;
}

static bool is_zero(Fraction const &a)
{
    return a.num == 0;
}

static Fraction frac_abs(Fraction a)
{
    if(a.num < 0) a.num = -a.num;
    return a;
}

static Fraction frac_neg(Fraction a)
{
    a.num = -a.num;
    return a;
}

static Fraction frac_add(Fraction a, Fraction b)
{
    return make_fraction(a.num * b.den + b.num * a.den, a.den * b.den);
}

static Fraction frac_sub(Fraction a, Fraction b)
{
    return frac_add(a, frac_neg(b));
}

static Fraction frac_mul(Fraction a, Fraction b)
{
    return make_fraction(a.num * b.num, a.den * b.den);
}

static bool frac_div(Fraction a, Fraction b, Fraction &out)
{
    if(b.num == 0) return false;
    out = make_fraction(a.num * b.den, a.den * b.num);
    return true;
}

static int lcm_int(int a, int b)
{
    a = abs_int(a);
    b = abs_int(b);
    if(a == 0 || b == 0) return 0;
    return (a / gcd_int(a, b)) * b;
}

static const int RPOLY_MAX_DEG = 8;

struct RPoly {
    Fraction coeff[RPOLY_MAX_DEG + 1]{};
    bool ok = true;
};

static RPoly poly_zero()
{
    RPoly out;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) out.coeff[i] = make_fraction(0, 1);
    return out;
}

static RPoly poly_const(Fraction v)
{
    RPoly out = poly_zero();
    out.coeff[0] = v;
    return out;
}

static RPoly poly_var()
{
    RPoly out = poly_zero();
    out.coeff[1] = make_fraction(1, 1);
    return out;
}

static int rpoly_degree(RPoly const &p)
{
    for(int i = RPOLY_MAX_DEG; i >= 0; i--) {
        if(!is_zero(p.coeff[i])) return i;
    }
    return 0;
}

static bool rpoly_is_const(RPoly const &p)
{
    if(!p.ok) return false;
    for(int i = 1; i <= RPOLY_MAX_DEG; i++) {
        if(!is_zero(p.coeff[i])) return false;
    }
    return true;
}

static bool rpoly_is_zero(RPoly const &p)
{
    if(!p.ok) return false;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) {
        if(!is_zero(p.coeff[i])) return false;
    }
    return true;
}

static RPoly rpoly_add(RPoly const &a, RPoly const &b)
{
    RPoly out = poly_zero();
    out.ok = a.ok && b.ok;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) out.coeff[i] = frac_add(a.coeff[i], b.coeff[i]);
    return out;
}

static RPoly rpoly_neg(RPoly const &a)
{
    RPoly out = poly_zero();
    out.ok = a.ok;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) out.coeff[i] = frac_neg(a.coeff[i]);
    return out;
}

static RPoly rpoly_sub(RPoly const &a, RPoly const &b)
{
    return rpoly_add(a, rpoly_neg(b));
}

static RPoly rpoly_mul(RPoly const &a, RPoly const &b)
{
    RPoly out = poly_zero();
    out.ok = a.ok && b.ok;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) {
        if(is_zero(a.coeff[i])) continue;
        for(int j = 0; j <= RPOLY_MAX_DEG; j++) {
            if(is_zero(b.coeff[j])) continue;
            if(i + j > RPOLY_MAX_DEG) {
                out.ok = false;
                continue;
            }
            out.coeff[i + j] = frac_add(out.coeff[i + j], frac_mul(a.coeff[i], b.coeff[j]));
        }
    }
    return out;
}

static RPoly rpoly_div_const(RPoly const &a, Fraction d)
{
    RPoly out = poly_zero();
    out.ok = a.ok && d.num != 0;
    if(d.num == 0) return out;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) {
        if(!frac_div(a.coeff[i], d, out.coeff[i])) out.ok = false;
    }
    return out;
}

static RPoly rpoly_scale(RPoly const &a, Fraction k)
{
    RPoly out = poly_zero();
    out.ok = a.ok;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) out.coeff[i] = frac_mul(a.coeff[i], k);
    return out;
}

static RPoly rpoly_pow(RPoly base, int exp)
{
    if(exp < 0 || exp > RPOLY_MAX_DEG) {
        RPoly bad = poly_zero();
        bad.ok = false;
        return bad;
    }
    RPoly out = poly_const(make_fraction(1, 1));
    for(int i = 0; i < exp; i++) out = rpoly_mul(out, base);
    return out;
}

static bool atom_starts_poly(char c, char var)
{
    return c == '(' || c == var || c == '.' || is_digit(c);
}

struct RPolyParser {
    const char *s = nullptr;
    int n = 0;
    int pos = 0;
    char var = 'x';
    bool ok = true;

    char peek() const { return pos < n ? s[pos] : '\0'; }
    bool take(char c)
    {
        if(peek() != c) return false;
        pos++;
        return true;
    }

    bool read_number(Fraction &out)
    {
        int start = pos;
        int whole = 0;
        bool has_whole = false;
        while(pos < n && is_digit(s[pos])) {
            has_whole = true;
            whole = whole * 10 + (s[pos] - '0');
            pos++;
        }
        if(pos < n && s[pos] == '.') {
            pos++;
            int frac = 0;
            int scale = 1;
            bool has_frac = false;
            while(pos < n && is_digit(s[pos])) {
                has_frac = true;
                frac = frac * 10 + (s[pos] - '0');
                scale *= 10;
                pos++;
            }
            if(!has_whole && !has_frac) {
                pos = start;
                return false;
            }
            out = make_fraction(whole * scale + frac, scale);
            return true;
        }
        if(!has_whole) return false;
        out = make_fraction(whole, 1);
        return true;
    }

    bool read_small_int(int &out)
    {
        if(pos >= n || !is_digit(s[pos])) return false;
        out = 0;
        while(pos < n && is_digit(s[pos])) {
            out = out * 10 + (s[pos] - '0');
            pos++;
            if(out > RPOLY_MAX_DEG) return false;
        }
        return true;
    }

    RPoly parse_atom()
    {
        if(take('(')) {
            RPoly out = parse_add();
            if(!take(')')) ok = false;
            return out;
        }
        if(peek() == var) {
            pos++;
            return poly_var();
        }
        Fraction v;
        if(read_number(v)) return poly_const(v);
        ok = false;
        return poly_zero();
    }

    RPoly parse_power()
    {
        RPoly out = parse_atom();
        if(take('^')) {
            int exp = 0;
            if(!read_small_int(exp)) ok = false;
            out = rpoly_pow(out, exp);
        }
        return out;
    }

    RPoly parse_unary()
    {
        if(take('+')) return parse_unary();
        if(take('-')) return rpoly_neg(parse_unary());
        return parse_power();
    }

    RPoly parse_mul()
    {
        RPoly out = parse_unary();
        while(ok) {
            if(take('*')) {
                out = rpoly_mul(out, parse_unary());
            }
            else if(take('/')) {
                RPoly d = parse_unary();
                if(!rpoly_is_const(d)) {
                    ok = false;
                    return out;
                }
                out = rpoly_div_const(out, d.coeff[0]);
            }
            else if(atom_starts_poly(peek(), var)) {
                out = rpoly_mul(out, parse_unary());
            }
            else break;
        }
        return out;
    }

    RPoly parse_add()
    {
        RPoly out = parse_mul();
        while(ok) {
            if(take('+')) out = rpoly_add(out, parse_mul());
            else if(take('-')) out = rpoly_sub(out, parse_mul());
            else break;
        }
        return out;
    }
};

static int known_function_len(const char *s, int pos)
{
    const char *names[] = {
        "asin", "acos", "atan", "sqrt", "cbrt", "log", "ln",
        "sin", "cos", "tan", "abs", "exp", "sec", "cot",
    };
    for(unsigned int k = 0; k < sizeof(names) / sizeof(names[0]); k++) {
        int i = 0;
        while(names[k][i] != '\0' && s[pos + i] == names[k][i]) i++;
        if(names[k][i] == '\0') return i;
    }
    return 0;
}

static char detect_poly_var(const char *s)
{
    for(int i = 0; s != nullptr && s[i] != '\0'; i++) {
        char c = s[i];
        if(c >= 'a' && c <= 'z') {
            int fn_len = known_function_len(s, i);
            if(fn_len > 0) {
                i += fn_len - 1;
                continue;
            }
            if(c == 'p' && s[i + 1] == 'i') {
                i++;
                continue;
            }
            if(c == 'e') continue;
            return c;
        }
    }
    return 'x';
}

static RPoly parse_rpoly_compact(const char *s, int begin, int end, char var)
{
    RPolyParser p{s + begin, end - begin, 0, var, true};
    RPoly out = p.parse_add();
    out.ok = out.ok && p.ok && p.pos == p.n;
    return out;
}

static int find_top_level_equals(const char *s)
{
    int depth = 0;
    for(int i = 0; s != nullptr && s[i] != '\0'; i++) {
        if(s[i] == '(' || s[i] == '[' || s[i] == '{') depth++;
        else if(s[i] == ')' || s[i] == ']' || s[i] == '}') depth--;
        else if(s[i] == '=' && depth == 0) return i;
    }
    return -1;
}

static int find_matching_paren(const char *s, int open, int end)
{
    if(open < 0 || open >= end || s[open] != '(') return -1;
    int depth = 0;
    for(int i = open; i < end; i++) {
        if(s[i] == '(') depth++;
        else if(s[i] == ')') {
            depth--;
            if(depth == 0) return i;
            if(depth < 0) return -1;
        }
    }
    return -1;
}

static bool starts_at(const char *s, int pos, const char *pat)
{
    if(s == nullptr || pat == nullptr || pos < 0) return false;
    for(int i = 0; pat[i] != '\0'; i++) {
        if(s[pos + i] != pat[i]) return false;
    }
    return true;
}

static void copy_range(const char *s, int begin, int end, char *out, int cap)
{
    int j = 0;
    if(out == nullptr || cap <= 0) return;
    for(int i = begin; i < end && j + 1 < cap; i++) out[j++] = s[i];
    out[j] = '\0';
}

static bool is_wrapped_var(const char *s, int begin, int end, char var)
{
    while(end - begin >= 2 && s[begin] == '(') {
        int close = find_matching_paren(s, begin, end);
        if(close != end - 1) break;
        begin++;
        end--;
    }
    return end == begin + 1 && s[begin] == var;
}

static RPoly rpoly_derivative(RPoly const &p)
{
    RPoly d = poly_zero();
    d.ok = p.ok;
    for(int power = 1; power <= RPOLY_MAX_DEG; power++) {
        d.coeff[power - 1] = frac_mul(p.coeff[power], make_fraction(power, 1));
    }
    return d;
}

static void append_fraction(FixedString<96> &line, Fraction const &f);

static void append_rpoly(FixedString<96> &line, RPoly const &poly, char var)
{
    bool wrote = false;
    for(int p = RPOLY_MAX_DEG; p >= 0; p--) {
        Fraction c = poly.coeff[p];
        if(is_zero(c)) continue;
        if(wrote) line.append(c.num < 0 ? " - " : " + ");
        else if(c.num < 0) line.append("-");

        Fraction mag = frac_abs(c);
        bool one = (mag.num == mag.den);
        if(p > 0) {
            if(mag.den != 1) {
                if(mag.num != 1) line.append_int(mag.num);
            }
            else if(!one) line.append_int(mag.num);
            line.append_char(var);
            if(p != 1) {
                line.append("^");
                line.append_int(p);
            }
            if(mag.den != 1) {
                line.append("/");
                line.append_int(mag.den);
            }
        }
        else append_fraction(line, mag);
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

static void add_method_fallback(
    OutputLines &out,
    const char *input)
{
    add_input_line(out, "", input);
    out.add("Err: unsupported form.");
}

static void append_fraction_value(FixedString<96> &line, int num, int den)
{
    Fraction f = make_fraction(num, den);
    if(f.den == 1) line.append_int(f.num);
    else {
        line.append_int(f.num);
        line.append("/");
        line.append_int(f.den);
    }
}

static void append_fraction(FixedString<96> &line, Fraction const &f)
{
    if(f.den == 1) line.append_int(f.num);
    else {
        line.append_int(f.num);
        line.append("/");
        line.append_int(f.den);
    }
}

static void append_poly_plus_recip(FixedString<96> &line, RPoly const &poly_d, char var, bool negative_log)
{
    bool has_poly = !rpoly_is_zero(poly_d);
    if(has_poly) append_rpoly(line, poly_d, var);
    if(has_poly) line.append(negative_log ? " - 1/" : " + 1/");
    else {
        if(negative_log) line.append("-");
        line.append("1/");
    }
    line.append_char(var);
}

static bool remove_log_term(const char *body, int n, char var, char *rest, int cap, bool &negative_log)
{
    for(int i = 0; i < n; i++) {
        int open = -1;
        if(starts_at(body, i, "ln(")) open = i + 2;
        else if(starts_at(body, i, "log(")) open = i + 3;
        if(open < 0) continue;

        int close = find_matching_paren(body, open, n);
        if(close < 0 || !is_wrapped_var(body, open + 1, close, var)) continue;

        int begin = i;
        negative_log = false;
        if(i > 0 && (body[i - 1] == '+' || body[i - 1] == '-')) {
            begin = i - 1;
            negative_log = body[i - 1] == '-';
        }
        int end = close + 1;

        int j = 0;
        for(int k = 0; k < n && j + 1 < cap; k++) {
            if(k >= begin && k < end) continue;
            rest[j++] = body[k];
        }
        rest[j] = '\0';

        int src = 0;
        while(rest[src] == '+') src++;
        int dst = 0;
        while(rest[src] != '\0' && dst + 1 < cap) rest[dst++] = rest[src++];
        while(dst > 0 && (rest[dst - 1] == '+' || rest[dst - 1] == '-')) dst--;
        rest[dst] = '\0';
        return true;
    }
    return false;
}

static bool solve_log_chain_derivative(const char *input, const char *s, char var, OutputLines &out)
{
    int n = cstr_len(s);
    int body_begin = 0;
    int body_end = n;
    int exp = 1;

    if(n >= 4 && s[0] == '(') {
        int close = find_matching_paren(s, 0, n);
        if(close > 0 && close + 2 < n && s[close + 1] == '^') {
            int pos = close + 2;
            if(!read_int(s, pos, n, exp) || pos != n || exp < 0) return false;
            body_begin = 1;
            body_end = close;
        }
    }

    char body[96];
    copy_range(s, body_begin, body_end, body, (int)sizeof(body));
    int body_n = cstr_len(body);

    char rest[96];
    bool negative_log = false;
    if(!remove_log_term(body, body_n, var, rest, (int)sizeof(rest), negative_log)) return false;

    RPoly base = poly_zero();
    if(rest[0] != '\0') {
        base = parse_rpoly_compact(rest, 0, cstr_len(rest), var);
        if(!base.ok) return false;
    }
    RPoly inner_d = rpoly_derivative(base);

    add_input_line(out, "1. Start: y = ", input);
    if(exp == 0) {
        out.add("2. Constant expression.");
        out.add("Answer: dy/dx = 0");
        return true;
    }
    if(exp != 1) {
        FixedString<96> &u = out.next();
        u.append("2. Let u = ");
        u.append(body);
        out.add("3. Chain rule: dy/dx = n*u^(n-1)*u'.");
        FixedString<96> &du = out.next();
        du.append("4. u' = ");
        append_poly_plus_recip(du, inner_d, var, negative_log);

        FixedString<96> &ans = out.next();
        ans.append("Answer: dy/dx = ");
        ans.append_int(exp);
        ans.append("*(");
        ans.append(body);
        ans.append(")^");
        ans.append_int(exp - 1);
        ans.append("*(");
        append_poly_plus_recip(ans, inner_d, var, negative_log);
        ans.append(")");
        return true;
    }

    out.add("2. Use d/dx ln(x) = 1/x.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: dy/dx = ");
    append_poly_plus_recip(ans, inner_d, var, negative_log);
    return true;
}

static void append_surd_root(FixedString<96> &line, int base, char op, int disc, int den);

static bool rpoly_to_int_coeffs(RPoly const &p, int *out, int max_deg)
{
    int lcm = 1;
    for(int i = 0; i <= max_deg; i++) {
        lcm = lcm_int(lcm, p.coeff[i].den);
        if(lcm == 0 || lcm > 1000000) return false;
    }
    for(int i = 0; i <= max_deg; i++) {
        int v = p.coeff[i].num * (lcm / p.coeff[i].den);
        out[i] = v;
    }
    int g = 0;
    for(int i = 0; i <= max_deg; i++) g = gcd_int(g, out[i]);
    if(g > 1) {
        for(int i = 0; i <= max_deg; i++) out[i] /= g;
    }
    return true;
}

static Fraction eval_int_poly(const int *coeff, int deg, Fraction x)
{
    Fraction acc = make_fraction(0, 1);
    for(int p = deg; p >= 0; p--) {
        acc = frac_mul(acc, x);
        acc = frac_add(acc, make_fraction(coeff[p], 1));
    }
    return acc;
}

static void synthetic_divide_linear(int *coeff, int &deg, int root)
{
    int out[RPOLY_MAX_DEG + 1]{};
    out[deg - 1] = coeff[deg];
    for(int p = deg - 2; p >= 0; p--) out[p] = coeff[p + 1] + root * out[p + 1];
    for(int p = 0; p <= deg - 1; p++) coeff[p] = out[p];
    coeff[deg] = 0;
    deg--;
}

static bool append_quadratic_roots(OutputLines &out, const int *coeff, char var)
{
    int a2 = coeff[2];
    int b2 = coeff[1];
    int c2 = coeff[0];
    int disc = b2 * b2 - 4 * a2 * c2;
    int root = 0;

    FixedString<96> &disc_line = out.next();
    disc_line.append("4. Discriminant D = ");
    disc_line.append_int(disc);
    disc_line.append(".");

    if(disc < 0) {
        out.add("5. D < 0: no real roots.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: ");
        ans.append_char(var);
        ans.append(" = []");
        return true;
    }
    if(!is_square_int(disc, root)) {
        out.add("5. Keep exact surd roots.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: ");
        ans.append_char(var);
        ans.append(" = ");
        append_surd_root(ans, -b2, '-', disc, 2 * a2);
        ans.append(" or ");
        ans.append_char(var);
        ans.append(" = ");
        append_surd_root(ans, -b2, '+', disc, 2 * a2);
        return true;
    }

    out.add("5. Use quadratic formula.");
    Fraction r1 = make_fraction(-b2 - root, 2 * a2);
    Fraction r2 = make_fraction(-b2 + root, 2 * a2);
    if(less_fraction(r2, r1)) {
        Fraction tmp = r1;
        r1 = r2;
        r2 = tmp;
    }

    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    ans.append_char(var);
    ans.append(" = ");
    append_fraction(ans, r1);
    if(!same_fraction(r1, r2)) {
        ans.append(" or ");
        ans.append_char(var);
        ans.append(" = ");
        append_fraction(ans, r2);
    }
    return true;
}

static bool append_polynomial_roots(OutputLines &out, RPoly const &poly, char var)
{
    int deg = rpoly_degree(poly);
    if(deg == 0) {
        out.add(is_zero(poly.coeff[0]) ? "Answer: infinitely many solutions." : "Answer: no solution.");
        return true;
    }
    if(deg == 1) {
        Fraction rhs = frac_neg(poly.coeff[0]);
        Fraction sol;
        if(!frac_div(rhs, poly.coeff[1], sol)) {
            out.add("Answer: no solution.");
            return true;
        }
        out.add("4. Solve linear equation.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: ");
        ans.append_char(var);
        ans.append(" = ");
        append_fraction(ans, sol);
        return true;
    }

    int coeff[RPOLY_MAX_DEG + 1]{};
    if(!rpoly_to_int_coeffs(poly, coeff, deg)) {
        out.add("4. Clear fractions, then use exact coefficient comparison.");
        out.add("5. Use host CAS if coefficients exceed compact integer range.");
        return false;
    }

    if(deg == 2) return append_quadratic_roots(out, coeff, var);

    Fraction roots[RPOLY_MAX_DEG]{};
    int root_count = 0;
    int work_deg = deg;
    bool changed = true;
    while(work_deg > 2 && changed) {
        changed = false;
        for(int r = -20; r <= 20; r++) {
            if(r == 0 && coeff[0] != 0) {
                // still test zero below when valid
            }
            Fraction val = eval_int_poly(coeff, work_deg, make_fraction(r, 1));
            if(is_zero(val)) {
                roots[root_count++] = make_fraction(r, 1);
                synthetic_divide_linear(coeff, work_deg, r);
                changed = true;
                break;
            }
        }
    }

    if(root_count > 0) {
        out.add("4. Use factor theorem to find rational roots.");
        FixedString<96> &found = out.next();
        found.append("5. Roots found: ");
        for(int i = 0; i < root_count; i++) {
            if(i) found.append(", ");
            found.append_char(var);
            found.append(" = ");
            append_fraction(found, roots[i]);
        }
    }

    if(work_deg == 1) {
        Fraction sol;
        if(frac_div(make_fraction(-coeff[0], 1), make_fraction(coeff[1], 1), sol)) roots[root_count++] = sol;
    }
    else if(work_deg == 2) {
        int a2 = coeff[2];
        int b2 = coeff[1];
        int c2 = coeff[0];
        int disc = b2 * b2 - 4 * a2 * c2;
        int root = 0;
        if(disc >= 0 && is_square_int(disc, root)) {
            roots[root_count++] = make_fraction(-b2 - root, 2 * a2);
            Fraction r2 = make_fraction(-b2 + root, 2 * a2);
            bool dup = false;
            for(int i = 0; i < root_count; i++) {
                if(same_fraction(roots[i], r2)) dup = true;
            }
            if(!dup) roots[root_count++] = r2;
        }
        else {
            append_quadratic_roots(out, coeff, var);
            if(root_count > 0) {
                FixedString<96> &extra = out.next();
                extra.append("Also include listed rational roots.");
            }
            return true;
        }
    }
    else {
        out.add("4. Use factor theorem, substitution, or numeric/factor mode for higher degree.");
        return false;
    }

    for(int i = 0; i < root_count; i++) {
        for(int j = i + 1; j < root_count; j++) {
            if(less_fraction(roots[j], roots[i])) {
                Fraction tmp = roots[i];
                roots[i] = roots[j];
                roots[j] = tmp;
            }
        }
    }

    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    for(int i = 0; i < root_count; i++) {
        if(i) ans.append(" or ");
        ans.append_char(var);
        ans.append(" = ");
        append_fraction(ans, roots[i]);
    }
    return true;
}

static void append_surd_root(FixedString<96> &line, int base, char op, int disc, int den)
{
    if(den < 0) {
        den = -den;
        base = -base;
        op = (op == '+') ? '-' : '+';
    }

    line.append("(");
    line.append_int(base);
    line.append(op == '+' ? " + sqrt(" : " - sqrt(");
    line.append_int(disc);
    line.append("))/");
    line.append_int(den);
}

static bool solve_simplify(const char *input, OutputLines &out)
{
    char s[128];
    compact(input, s, (int)sizeof(s));
    char var = detect_poly_var(s);
    RPoly poly = parse_rpoly_compact(s, 0, cstr_len(s), var);
    if(!poly.ok) {
        add_method_fallback(out, input);
        return true;
    }

    add_input_line(out, "1. Start: ", input);
    if(!cstr_eq(s, input)) {
        FixedString<96> &norm = out.next();
        norm.append("2. Normalize: ");
        norm.append(s);
        out.add("3. Expand brackets and collect powers.");
    }
    else {
        out.add("2. Expand brackets and collect powers.");
    }
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_rpoly(ans, poly, var);
    return true;
}

static bool solve_algebra(const char *input, OutputLines &out)
{
    char s[128];
    int n = compact(input, s, (int)sizeof(s));
    int eq = find_top_level_equals(s);
    if(eq <= 0 || eq >= n - 1) {
        add_method_fallback(out, input);
        return true;
    }

    char var = detect_poly_var(s);
    RPoly left = parse_rpoly_compact(s, 0, eq, var);
    RPoly right = parse_rpoly_compact(s, eq + 1, n, var);
    if(!left.ok || !right.ok) {
        add_method_fallback(out, input);
        return true;
    }
    RPoly q = rpoly_sub(left, right);

    add_input_line(out, "1. Start: ", input);
    out.add("2. Expand brackets on both sides.");
    FixedString<96> &stdform = out.next();
    stdform.append("3. Rearrange to ");
    append_rpoly(stdform, q, var);
    stdform.append(" = 0.");
    return append_polynomial_roots(out, q, var);
}

static bool solve_derive(const char *input, OutputLines &out)
{
    char s[128];
    compact(input, s, (int)sizeof(s));
    char var = detect_poly_var(s);
    RPoly p = parse_rpoly_compact(s, 0, cstr_len(s), var);
    if(!p.ok) {
        if(solve_log_chain_derivative(input, s, var, out)) return true;
        add_method_fallback(out, input);
        return true;
    }

    RPoly d = rpoly_derivative(p);

    add_input_line(out, "1. Start: y = ", input);
    out.add("2. Expand/collect first if needed.");
    out.add("3. Use d/dx(ax^n) = anx^(n-1).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: dy/dx = ");
    append_rpoly(ans, d, var);
    return true;
}

static bool solve_integrate(const char *input, OutputLines &out)
{
    char s[128];
    compact(input, s, (int)sizeof(s));
    char var = detect_poly_var(s);
    RPoly p = parse_rpoly_compact(s, 0, cstr_len(s), var);
    if(!p.ok) {
        add_method_fallback(out, input);
        return true;
    }

    RPoly integ = poly_zero();
    for(int power = 0; power <= RPOLY_MAX_DEG - 1; power++) {
        if(is_zero(p.coeff[power])) continue;
        Fraction div;
        if(!frac_div(p.coeff[power], make_fraction(power + 1, 1), div)) return false;
        integ.coeff[power + 1] = div;
    }

    add_input_line(out, "1. Start: integrate ", input);
    out.add("2. Expand/collect first if needed.");
    out.add("3. Use int(ax^n)=ax^(n+1)/(n+1).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_rpoly(ans, integ, var);
    ans.append(" + C");
    return true;
}

enum class TrigFn { None, Sin, Cos, Tan, Mixed };

struct TrigPoly {
    Fraction coeff[3]{};
    TrigFn fn = TrigFn::None;
    int k = 1;
    bool ok = true;
};

static TrigPoly trig_zero()
{
    TrigPoly out;
    for(int i = 0; i < 3; i++) out.coeff[i] = make_fraction(0, 1);
    return out;
}

static TrigPoly trig_const(Fraction v)
{
    TrigPoly out = trig_zero();
    out.coeff[0] = v;
    return out;
}

static TrigPoly trig_var(TrigFn fn, int k)
{
    TrigPoly out = trig_zero();
    out.coeff[1] = make_fraction(1, 1);
    out.fn = fn;
    out.k = k;
    return out;
}

static bool trig_merge(TrigPoly &a, TrigPoly const &b)
{
    if(b.fn == TrigFn::None) return true;
    if(a.fn == TrigFn::None) {
        a.fn = b.fn;
        a.k = b.k;
        return true;
    }
    return a.fn == b.fn && a.k == b.k;
}

static TrigPoly trig_add(TrigPoly a, TrigPoly const &b)
{
    if(!a.ok || !b.ok || !trig_merge(a, b)) {
        a.ok = false;
        return a;
    }
    for(int i = 0; i < 3; i++) a.coeff[i] = frac_add(a.coeff[i], b.coeff[i]);
    return a;
}

static TrigPoly trig_neg(TrigPoly a)
{
    for(int i = 0; i < 3; i++) a.coeff[i] = frac_neg(a.coeff[i]);
    return a;
}

static TrigPoly trig_sub(TrigPoly a, TrigPoly const &b)
{
    return trig_add(a, trig_neg(b));
}

static int trig_degree(TrigPoly const &p)
{
    for(int i = 2; i >= 0; i--) if(!is_zero(p.coeff[i])) return i;
    return 0;
}

static bool trig_is_const(TrigPoly const &p)
{
    return p.ok && is_zero(p.coeff[1]) && is_zero(p.coeff[2]);
}

static TrigPoly trig_mul(TrigPoly a, TrigPoly b)
{
    TrigPoly out = trig_zero();
    out.ok = a.ok && b.ok;
    if(!out.ok) return out;
    if(!trig_merge(out, a) || !trig_merge(out, b)) {
        out.ok = false;
        return out;
    }
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            if(i + j > 2 && !is_zero(a.coeff[i]) && !is_zero(b.coeff[j])) {
                out.ok = false;
                return out;
            }
            if(i + j <= 2) out.coeff[i + j] = frac_add(out.coeff[i + j], frac_mul(a.coeff[i], b.coeff[j]));
        }
    }
    return out;
}

static TrigPoly trig_div_const(TrigPoly a, Fraction d)
{
    if(d.num == 0) {
        a.ok = false;
        return a;
    }
    for(int i = 0; i < 3; i++) {
        if(!frac_div(a.coeff[i], d, a.coeff[i])) a.ok = false;
    }
    return a;
}

static const char *trig_fn_name(TrigFn fn)
{
    if(fn == TrigFn::Sin) return "sin";
    if(fn == TrigFn::Cos) return "cos";
    if(fn == TrigFn::Tan) return "tan";
    return "?";
}

static bool parse_angle_var(const char *s, int begin, int end, int &k)
{
    k = 1;
    if(begin >= end) return false;
    int i = begin;
    bool neg = false;
    if(s[i] == '-') {
        neg = true;
        i++;
    }
    int coeff = 0;
    bool has_coeff = read_int(s, i, end, coeff);
    if(i < end && s[i] == '*') i++;
    if(i >= end || (s[i] != 'x' && s[i] != 't')) return false;
    i++;
    if(i != end) return false;
    k = has_coeff ? coeff : 1;
    if(neg) k = -k;
    return k != 0;
}

struct TrigParser {
    const char *s = nullptr;
    int n = 0;
    int pos = 0;
    bool ok = true;

    char peek() const { return pos < n ? s[pos] : '\0'; }
    bool take(char c)
    {
        if(peek() != c) return false;
        pos++;
        return true;
    }
    bool starts_atom() const
    {
        char c = peek();
        return c == '(' || c == '.' || is_digit(c) || c == 's' || c == 'c' || c == 't';
    }
    bool starts_name(const char *name) const
    {
        for(int i = 0; name[i] != '\0'; i++) {
            if(pos + i >= n || s[pos + i] != name[i]) return false;
        }
        return true;
    }
    bool read_number(Fraction &out)
    {
        RPolyParser tmp{s, n, pos, 'x', true};
        bool ok_num = tmp.read_number(out);
        if(ok_num) pos = tmp.pos;
        return ok_num;
    }
    bool read_small_int(int &out)
    {
        RPolyParser tmp{s, n, pos, 'x', true};
        bool ok_int = tmp.read_small_int(out);
        if(ok_int) pos = tmp.pos;
        return ok_int;
    }
    TrigPoly parse_trig_call(TrigFn fn, int name_len, int preset_power)
    {
        pos += name_len;
        int power = preset_power;
        if(power == 0 && take('^')) {
            if(!read_small_int(power)) ok = false;
        }
        if(power == 0) power = 1;
        if(power < 1 || power > 2) ok = false;
        if(!take('(')) ok = false;
        int arg_begin = pos;
        int depth = 1;
        while(pos < n && depth > 0) {
            if(s[pos] == '(') depth++;
            else if(s[pos] == ')') depth--;
            if(depth > 0) pos++;
        }
        int arg_end = pos;
        if(!take(')')) ok = false;
        int k = 1;
        if(!parse_angle_var(s, arg_begin, arg_end, k)) ok = false;
        TrigPoly out = trig_var(fn, k);
        if(power == 2) out = trig_mul(out, out);
        return out;
    }
    TrigPoly parse_atom()
    {
        if(take('(')) {
            TrigPoly out = parse_add();
            if(!take(')')) ok = false;
            return out;
        }
        Fraction v;
        if(read_number(v)) return trig_const(v);
        if(starts_name("sin")) return parse_trig_call(TrigFn::Sin, 3, 0);
        if(starts_name("cos")) return parse_trig_call(TrigFn::Cos, 3, 0);
        if(starts_name("tan")) return parse_trig_call(TrigFn::Tan, 3, 0);
        ok = false;
        return trig_zero();
    }
    TrigPoly parse_power()
    {
        TrigPoly out = parse_atom();
        if(take('^')) {
            int power = 0;
            if(!read_small_int(power)) ok = false;
            if(power == 0) out = trig_const(make_fraction(1, 1));
            else if(power == 1) {}
            else if(power == 2) out = trig_mul(out, out);
            else ok = false;
        }
        return out;
    }
    TrigPoly parse_unary()
    {
        if(take('+')) return parse_unary();
        if(take('-')) return trig_neg(parse_unary());
        return parse_power();
    }
    TrigPoly parse_mul()
    {
        TrigPoly out = parse_unary();
        while(ok) {
            if(take('*')) out = trig_mul(out, parse_unary());
            else if(take('/')) {
                TrigPoly d = parse_unary();
                if(!trig_is_const(d)) ok = false;
                else out = trig_div_const(out, d.coeff[0]);
            }
            else if(starts_atom()) out = trig_mul(out, parse_unary());
            else break;
        }
        return out;
    }
    TrigPoly parse_add()
    {
        TrigPoly out = parse_mul();
        while(ok) {
            if(take('+')) out = trig_add(out, parse_mul());
            else if(take('-')) out = trig_sub(out, parse_mul());
            else break;
        }
        return out;
    }
};

static TrigPoly parse_trig_compact(const char *s, int begin, int end)
{
    TrigParser p{s + begin, end - begin, 0, true};
    TrigPoly out = p.parse_add();
    out.ok = out.ok && p.ok && p.pos == p.n;
    return out;
}

static bool append_angle_solution(FixedString<96> &line, int k, int angle, int period)
{
    if(k == 1) {
        line.append_int(angle);
        line.append(" + ");
        line.append_int(period);
        line.append("n");
        return true;
    }
    append_fraction_value(line, angle, k);
    line.append(" + ");
    append_fraction_value(line, period, k);
    line.append("n");
    return true;
}

static bool append_trig_value_solutions(OutputLines &out, TrigFn fn, int k, Fraction value)
{
    int angles[2]{};
    int count = 0;
    int period = 360;

    if(fn == TrigFn::Sin) {
        if(value.num == 0) { angles[count++] = 0; period = 180; }
        else if(value.num == value.den) angles[count++] = 90;
        else if(value.num == -value.den) angles[count++] = 270;
        else if(value.num == 1 && value.den == 2) { angles[count++] = 30; angles[count++] = 150; }
        else if(value.num == -1 && value.den == 2) { angles[count++] = 210; angles[count++] = 330; }
    }
    else if(fn == TrigFn::Cos) {
        if(value.num == 0) { angles[count++] = 90; period = 180; }
        else if(value.num == value.den) angles[count++] = 0;
        else if(value.num == -value.den) angles[count++] = 180;
        else if(value.num == 1 && value.den == 2) { angles[count++] = 60; angles[count++] = 300; }
        else if(value.num == -1 && value.den == 2) { angles[count++] = 120; angles[count++] = 240; }
    }
    else if(fn == TrigFn::Tan) {
        period = 180;
        if(value.num == 0) angles[count++] = 0;
        else if(value.num == value.den) angles[count++] = 45;
        else if(value.num == -value.den) angles[count++] = 135;
    }

    if(count == 0) {
        out.add("5. Use inverse trig for non-table values, then add the period.");
        return false;
    }

    FixedString<96> &ans = out.next();
    ans.append("Answer: x = ");
    for(int i = 0; i < count; i++) {
        if(i) ans.append(" or ");
        append_angle_solution(ans, k, angles[i], period);
    }
    return true;
}

static bool simplify_trig_identity(const char *s, OutputLines &out, const char *input)
{
    if(cstr_eq(s, "sin(x)^2+cos(x)^2") || cstr_eq(s, "cos(x)^2+sin(x)^2") ||
       cstr_eq(s, "sin^2(x)+cos^2(x)") || cstr_eq(s, "cos^2(x)+sin^2(x)")) {
        add_input_line(out, "1. Start: ", input);
        out.add("2. Use identity sin^2(x)+cos^2(x)=1.");
        out.add("Answer: 1");
        return true;
    }
    if(cstr_eq(s, "sin(x)^2+cos(x)^2=1") || cstr_eq(s, "cos(x)^2+sin(x)^2=1") ||
       cstr_eq(s, "sin^2(x)+cos^2(x)=1") || cstr_eq(s, "cos^2(x)+sin^2(x)=1")) {
        add_input_line(out, "1. Start: ", input);
        out.add("2. Use identity sin^2(x)+cos^2(x)=1.");
        out.add("Answer: true.");
        return true;
    }
    return false;
}

static bool solve_trig_equation(const char *input, const char *s, int n, OutputLines &out)
{
    int eq = find_top_level_equals(s);
    if(eq <= 0 || eq >= n - 1) return false;
    TrigPoly left = parse_trig_compact(s, 0, eq);
    TrigPoly right = parse_trig_compact(s, eq + 1, n);
    if(!left.ok || !right.ok) return false;
    TrigPoly q = trig_sub(left, right);
    if(!q.ok || q.fn == TrigFn::None) return false;

    add_input_line(out, "1. Start: ", input);
    out.add("2. Rearrange into trig polynomial = 0.");
    FixedString<96> &stdform = out.next();
    stdform.append("3. Let y = ");
    stdform.append(trig_fn_name(q.fn));
    stdform.append("(x). Solve in y.");

    int deg = trig_degree(q);
    if(deg == 1) {
        Fraction y;
        if(!frac_div(frac_neg(q.coeff[0]), q.coeff[1], y)) return false;
        FixedString<96> &val = out.next();
        val.append("4. ");
        val.append(trig_fn_name(q.fn));
        val.append("(x) = ");
        append_fraction(val, y);
        return append_trig_value_solutions(out, q.fn, q.k, y);
    }
    if(deg == 2) {
        int coeff[3]{};
        if(!rpoly_to_int_coeffs(RPoly{{q.coeff[0], q.coeff[1], q.coeff[2]}, true}, coeff, 2)) {
            out.add("4. Clear fractions, solve the trig quadratic, then check roots.");
            return false;
        }
        int a = coeff[2];
        int b = coeff[1];
        int c = coeff[0];
        int disc = b * b - 4 * a * c;
        int root = 0;
        if(disc < 0 || !is_square_int(disc, root)) {
            out.add("4. Use quadratic formula for y, then inverse trig for each valid y.");
            return false;
        }
        Fraction y1 = make_fraction(-b - root, 2 * a);
        Fraction y2 = make_fraction(-b + root, 2 * a);
        FixedString<96> &vals = out.next();
        vals.append("4. y = ");
        append_fraction(vals, y1);
        if(!same_fraction(y1, y2)) {
            vals.append(" or y = ");
            append_fraction(vals, y2);
        }
        bool ok = append_trig_value_solutions(out, q.fn, q.k, y1);
        if(!same_fraction(y1, y2)) ok = append_trig_value_solutions(out, q.fn, q.k, y2) && ok;
        return ok;
    }

    out.add("4. Factor the trig polynomial or use identities to reduce degree.");
    return false;
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

static bool parse_angle_degrees(const char *s, int begin, int end, int &deg)
{
    int i = begin;
    bool neg = false;
    if(i < end && s[i] == '-') {
        neg = true;
        i++;
    }

    int whole = 0;
    int before = i;
    bool has_number = read_int(s, i, end, whole);

    if(i < end && s[i] == '*') i++;
    if(i + 1 < end && s[i] == 'p' && s[i + 1] == 'i') {
        i += 2;
        int num = has_number ? whole : 1;
        int den = 1;
        if(i < end && s[i] == '/') {
            i++;
            if(!read_int(s, i, end, den) || den == 0) return false;
        }
        if(i != end) return false;
        int scaled = 180 * num;
        if(scaled % den != 0) return false;
        deg = scaled / den;
        if(neg) deg = -deg;
        return true;
    }

    i = before;
    int value = 0;
    if(!read_int(s, i, end, value) || i != end) return false;
    deg = neg ? -value : value;
    return true;
}

static bool solve_trig(const char *input, OutputLines &out)
{
    char s[128];
    int n = compact(input, s, (int)sizeof(s));
    if(simplify_trig_identity(s, out, input)) return true;
    if(find_top_level_equals(s) >= 0) {
        if(solve_trig_equation(input, s, n, out)) return true;
        add_method_fallback(out, input);
        return true;
    }

    int lp = find_char(s, '(');
    int rp = find_char(s, ')');
    if(lp <= 0 || rp <= lp + 1 || rp != n - 1) {
        add_method_fallback(out, input);
        return true;
    }

    char fn[4];
    int fn_len = lp < 3 ? lp : 3;
    for(int i = 0; i < fn_len; i++) fn[i] = s[i];
    fn[fn_len] = '\0';

    int deg = 0;
    if(!parse_angle_degrees(s, lp + 1, rp, deg)) {
        add_method_fallback(out, input);
        return true;
    }

    const char *value = exact_trig(fn, deg);
    if(value == nullptr) {
        add_method_fallback(out, input);
        return true;
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
    if(in.target == 'u' && in.has_v && in.has_a && in.has_t) {
        out.add("2. Use v = u + at.");
        out.add("3. Rearrange to u = v - at.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: u = ");
        ans.append_int(in.v - in.a * in.t);
        return true;
    }
    if(in.target == 'a' && in.has_v && in.has_u && in.has_t && in.t != 0) {
        out.add("2. Use v = u + at.");
        out.add("3. Rearrange to a = (v-u)/t.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: a = ");
        append_fraction_value(ans, in.v - in.u, in.t);
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
    if(in.target == 's' && in.has_u && in.has_a && in.has_t) {
        out.add("2. Use s = ut + (1/2)at^2.");
        out.add("3. Substitute known values.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: s = ");
        append_fraction_value(ans, 2 * in.u * in.t + in.a * in.t * in.t, 2);
        return true;
    }
    if(in.target == 'u' && in.has_s && in.has_a && in.has_t && in.t != 0) {
        out.add("2. Use s = ut + (1/2)at^2.");
        out.add("3. Rearrange to u = (s - at^2/2)/t.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: u = ");
        append_fraction_value(ans, 2 * in.s - in.a * in.t * in.t, 2 * in.t);
        return true;
    }
    if(in.target == 'a' && in.has_s && in.has_u && in.has_t && in.t != 0) {
        out.add("2. Use s = ut + (1/2)at^2.");
        out.add("3. Rearrange to a = 2(s-ut)/t^2.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: a = ");
        append_fraction_value(ans, 2 * (in.s - in.u * in.t), in.t * in.t);
        return true;
    }
    if(in.target == 's' && in.has_u && in.has_v && in.has_t) {
        out.add("2. Use s = ((u+v)/2)t.");
        out.add("3. Substitute known values.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: s = ");
        append_fraction_value(ans, (in.u + in.v) * in.t, 2);
        return true;
    }
    if(in.target == 'u' && in.has_s && in.has_v && in.has_t && in.t != 0) {
        out.add("2. Use s = ((u+v)/2)t.");
        out.add("3. Rearrange to u = 2s/t - v.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: u = ");
        append_fraction_value(ans, 2 * in.s - in.v * in.t, in.t);
        return true;
    }
    if(in.target == 'v' && in.has_s && in.has_u && in.has_t && in.t != 0) {
        out.add("2. Use s = ((u+v)/2)t.");
        out.add("3. Rearrange to v = 2s/t - u.");
        FixedString<96> &ans = out.next();
        ans.append("Answer: v = ");
        append_fraction_value(ans, 2 * in.s - in.u * in.t, in.t);
        return true;
    }
    if(in.target == 't' && in.has_s && in.has_u && in.has_v && in.u + in.v != 0) {
        out.add("2. Use s = ((u+v)/2)t.");
        out.add("3. Rearrange to t = 2s/(u+v).");
        FixedString<96> &ans = out.next();
        ans.append("Answer: t = ");
        append_fraction_value(ans, 2 * in.s, in.u + in.v);
        return true;
    }

    out.add("Pick an equation containing the unknown and no missing variables.");
    out.add("Rearrange that SUVAT formula, then substitute values.");
    return false;
}

static bool read_fraction_token(const char *s, int begin, int end, Fraction &out)
{
    int i = begin;
    bool neg = false;
    if(i < end && s[i] == '-') {
        neg = true;
        i++;
    }
    int num = 0;
    if(!read_int(s, i, end, num)) return false;
    int den = 1;
    if(i < end && s[i] == '/') {
        i++;
        if(!read_int(s, i, end, den) || den == 0) return false;
    }
    if(i != end) return false;
    out = make_fraction(neg ? -num : num, den);
    return true;
}

static bool parse_fraction_args(const char *input, Fraction *args, int expected)
{
    char s[160];
    int n = compact(input, s, (int)sizeof(s));
    int count = 0;
    int start = 0;
    for(int i = 0; i <= n; i++) {
        if(i == n || s[i] == ',') {
            if(count >= expected) return false;
            if(!read_fraction_token(s, start, i, args[count])) return false;
            count++;
            start = i + 1;
        }
    }
    return count == expected;
}

static bool parse_fraction_list(const char *input, Fraction *args, int cap, int &count)
{
    char s[160];
    int n = compact(input, s, (int)sizeof(s));
    count = 0;
    int start = 0;
    for(int i = 0; i <= n; i++) {
        if(i == n || s[i] == ',') {
            if(count >= cap || i == start) return false;
            if(!read_fraction_token(s, start, i, args[count])) return false;
            count++;
            start = i + 1;
        }
    }
    return count > 0;
}

static Fraction frac_pow(Fraction base, int exp)
{
    Fraction out = make_fraction(1, 1);
    for(int i = 0; i < exp; i++) out = frac_mul(out, base);
    return out;
}

static int comb_int(int n, int r)
{
    if(r < 0 || r > n) return 0;
    if(r > n - r) r = n - r;
    int out = 1;
    for(int i = 1; i <= r; i++) out = out * (n - r + i) / i;
    return out;
}

static bool parse_int_args(const char *input, int *args, int expected)
{
    Fraction vals[16];
    if(expected > 16 || !parse_fraction_args(input, vals, expected)) return false;
    for(int i = 0; i < expected; i++) {
        if(vals[i].den != 1) return false;
        args[i] = vals[i].num;
    }
    return true;
}

static bool parse_int_list(const char *input, int *args, int cap, int &count)
{
    Fraction vals[32];
    if(cap > 32 || !parse_fraction_list(input, vals, cap, count)) return false;
    for(int i = 0; i < count; i++) {
        if(vals[i].den != 1) return false;
        args[i] = vals[i].num;
    }
    return true;
}

static bool is_prime_int(int n)
{
    if(n < 2) return false;
    if(n == 2) return true;
    if(n % 2 == 0) return false;
    for(int d = 3; d * d <= n; d += 2) {
        if(n % d == 0) return false;
    }
    return true;
}

static bool solve_gcd_call(const char *input, OutputLines &out)
{
    int a[32];
    int count = 0;
    if(!parse_int_list(input, a, 32, count)) {
        out.add("Use gcd(a,b,...).");
        return false;
    }
    int g = 0;
    for(int i = 0; i < count; i++) g = (i == 0) ? abs_int(a[i]) : gcd_int(g, a[i]);
    out.add("1. Apply Euclid's algorithm.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: gcd = ");
    ans.append_int(g == 0 ? 0 : g);
    return true;
}

static bool solve_lcm_call(const char *input, OutputLines &out)
{
    int a[32];
    int count = 0;
    if(!parse_int_list(input, a, 32, count)) {
        out.add("Use lcm(a,b,...).");
        return false;
    }
    int l = 1;
    for(int i = 0; i < count; i++) l = lcm_int(l, a[i]);
    out.add("1. Use lcm(a,b)=abs(ab)/gcd(a,b).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: lcm = ");
    ans.append_int(l);
    return true;
}

static bool solve_factorial_call(const char *input, OutputLines &out)
{
    int a[1];
    if(!parse_int_args(input, a, 1) || a[0] < 0 || a[0] > 12) {
        out.add("Use factorial(n), 0<=n<=12.");
        return false;
    }
    int v = 1;
    for(int i = 2; i <= a[0]; i++) v *= i;
    out.add("1. n! = 1*2*...*n.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    ans.append_int(v);
    return true;
}

static bool solve_isprime_call(const char *input, OutputLines &out)
{
    int a[1];
    if(!parse_int_args(input, a, 1)) {
        out.add("Use isprime(n).");
        return false;
    }
    out.add("1. Trial divide up to sqrt(n).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    ans.append(is_prime_int(a[0]) ? "prime" : "not prime");
    return true;
}

static bool solve_factors_call(const char *input, OutputLines &out)
{
    int a[1];
    if(!parse_int_args(input, a, 1) || a[0] == 0) {
        out.add("Use factors(n), n!=0.");
        return false;
    }
    int n = abs_int(a[0]);
    out.add("1. Prime factorisation by trial division.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    bool first = true;
    for(int d = 2; d * d <= n; d += (d == 2 ? 1 : 2)) {
        int power = 0;
        while(n % d == 0) {
            n /= d;
            power++;
        }
        if(power > 0) {
            if(!first) ans.append("*");
            ans.append_int(d);
            if(power > 1) {
                ans.append("^");
                ans.append_int(power);
            }
            first = false;
        }
    }
    if(n > 1) {
        if(!first) ans.append("*");
        ans.append_int(n);
    }
    return true;
}

static bool solve_divisors_call(const char *input, OutputLines &out)
{
    int a[1];
    if(!parse_int_args(input, a, 1) || a[0] == 0 || abs_int(a[0]) > 10000) {
        out.add("Use divisors(n), 0<abs(n)<=10000.");
        return false;
    }
    int n = abs_int(a[0]);
    out.add("1. Test divisors up to n.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    bool first = true;
    for(int d = 1; d <= n; d++) {
        if(n % d != 0) continue;
        if(!first) ans.append(",");
        ans.append_int(d);
        first = false;
    }
    return true;
}

static bool solve_binom(const char *input, OutputLines &out)
{
    Fraction a[3];
    if(!parse_fraction_args(input, a, 3) || a[0].den != 1 || a[2].den != 1) {
        out.add("Use binom(n,p,r), e.g. binom(10,1/2,3).");
        return false;
    }
    int n = a[0].num;
    int r = a[2].num;
    if(n < 0 || r < 0 || r > n) {
        out.add("Answer: 0.");
        return true;
    }
    Fraction q = frac_sub(make_fraction(1, 1), a[1]);
    Fraction ansv = frac_mul(make_fraction(comb_int(n, r), 1), frac_mul(frac_pow(a[1], r), frac_pow(q, n - r)));
    out.add("1. X ~ B(n,p).");
    out.add("2. P(X=r)=nCr p^r (1-p)^(n-r).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_fraction(ans, ansv);
    return true;
}

static bool solve_binom_cdf(const char *input, OutputLines &out)
{
    Fraction a[3];
    if(!parse_fraction_args(input, a, 3) || a[0].den != 1 || a[2].den != 1) {
        out.add("Use binomcdf(n,p,r).");
        return false;
    }
    int n = a[0].num;
    int r = a[2].num;
    if(n < 0 || r < 0) {
        out.add("Answer: 0.");
        return true;
    }
    if(r > n) r = n;
    if(n > 20) {
        out.add("Use host stats for n>20.");
        return false;
    }
    Fraction q = frac_sub(make_fraction(1, 1), a[1]);
    Fraction total = make_fraction(0, 1);
    for(int k = 0; k <= r; k++) {
        Fraction term = frac_mul(make_fraction(comb_int(n, k), 1), frac_mul(frac_pow(a[1], k), frac_pow(q, n - k)));
        total = frac_add(total, term);
    }
    out.add("1. X ~ B(n,p).");
    out.add("2. P(X<=r)=sum nCk p^k(1-p)^(n-k).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_fraction(ans, total);
    return true;
}

static bool parse_call_args_compact(const char *input, char args[][96], int cap, int &count)
{
    char s[192];
    int n = compact(input, s, (int)sizeof(s));
    count = 0;
    int start = 0;
    int depth = 0;
    for(int i = 0; i <= n; i++) {
        char c = i < n ? s[i] : ',';
        if(c == '(' || c == '[' || c == '{') depth++;
        else if((c == ')' || c == ']' || c == '}') && depth > 0) depth--;
        if((i == n || (c == ',' && depth == 0))) {
            if(count >= cap || i == start) return false;
            copy_range(s, start, i, args[count], 96);
            count++;
            start = i + 1;
        }
    }
    return count > 0;
}

static bool parse_poly_arg(const char *text, char var, RPoly &out)
{
    int n = cstr_len(text);
    out = parse_rpoly_compact(text, 0, n, var);
    return out.ok;
}

static bool parse_equation_poly_arg(const char *text, char var, RPoly &out)
{
    int n = cstr_len(text);
    int eq = find_top_level_equals(text);
    if(eq < 0) {
        out = parse_rpoly_compact(text, 0, n, var);
        return out.ok;
    }
    RPoly left = parse_rpoly_compact(text, 0, eq, var);
    RPoly right = parse_rpoly_compact(text, eq + 1, n, var);
    if(!left.ok || !right.ok) return false;
    out = rpoly_sub(left, right);
    return out.ok;
}

static RPoly rpoly_compose(RPoly const &outer, RPoly const &inner)
{
    RPoly out = poly_zero();
    RPoly pow = poly_const(make_fraction(1, 1));
    out.ok = outer.ok && inner.ok;
    for(int i = 0; i <= RPOLY_MAX_DEG; i++) {
        if(!is_zero(outer.coeff[i])) out = rpoly_add(out, rpoly_scale(pow, outer.coeff[i]));
        if(i != RPOLY_MAX_DEG) pow = rpoly_mul(pow, inner);
    }
    return out;
}

static Fraction rpoly_eval(RPoly const &poly, Fraction x)
{
    Fraction acc = make_fraction(0, 1);
    for(int i = RPOLY_MAX_DEG; i >= 0; i--) {
        acc = frac_mul(acc, x);
        acc = frac_add(acc, poly.coeff[i]);
    }
    return acc;
}

static void append_signed_fraction(FixedString<96> &line, Fraction f)
{
    if(f.num < 0) {
        line.append(" - ");
        f.num = -f.num;
    }
    else {
        line.append(" + ");
    }
    append_fraction(line, f);
}

static void append_linear_factor(FixedString<96> &line, char var, Fraction root)
{
    line.append("(");
    line.append_char(var);
    if(root.num == 0) {
        line.append(")");
        return;
    }
    if(root.num < 0) {
        root.num = -root.num;
        line.append(" + ");
    }
    else {
        line.append(" - ");
    }
    append_fraction(line, root);
    line.append(")");
}

static bool solve_factor_call(const char *input, OutputLines &out)
{
    char args[2][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 2, argc) || argc != 1) {
        out.add("Use factor(expr).");
        return false;
    }
    char var = detect_poly_var(args[0]);
    RPoly poly;
    if(!parse_poly_arg(args[0], var, poly)) {
        add_method_fallback(out, args[0]);
        return true;
    }

    add_input_line(out, "1. Input: ", args[0]);
    out.add("2. Expand and collect before factorising.");

    int deg = rpoly_degree(poly);
    if(deg <= 0) {
        FixedString<96> &ans = out.next();
        ans.append("Answer: ");
        append_rpoly(ans, poly, var);
        return true;
    }

    int coeff[RPOLY_MAX_DEG + 1]{};
    if(!rpoly_to_int_coeffs(poly, coeff, deg)) {
        out.add("3. Clear fractions, then factor by coefficient comparison/factor theorem.");
        return false;
    }

    Fraction roots[RPOLY_MAX_DEG]{};
    int root_count = 0;
    int work_deg = deg;
    while(work_deg > 0) {
        bool changed = false;
        for(int r = -20; r <= 20; r++) {
            if(is_zero(eval_int_poly(coeff, work_deg, make_fraction(r, 1)))) {
                roots[root_count++] = make_fraction(r, 1);
                synthetic_divide_linear(coeff, work_deg, r);
                changed = true;
                break;
            }
        }
        if(!changed) break;
    }

    if(root_count == 0 && deg == 2) {
        int a = coeff[2];
        int b = coeff[1];
        int c = coeff[0];
        int disc = b * b - 4 * a * c;
        int root = 0;
        if(disc >= 0 && is_square_int(disc, root)) {
            roots[root_count++] = make_fraction(-b - root, 2 * a);
            Fraction r2 = make_fraction(-b + root, 2 * a);
            if(!same_fraction(roots[0], r2)) roots[root_count++] = r2;
        }
    }

    if(root_count == 0) {
        out.add("Answer: irreducible over small rational roots.");
        return true;
    }

    out.add("3. Use factor theorem / quadratic roots.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    if(coeff[work_deg] != 1 || work_deg > 0) {
        ans.append_int(coeff[work_deg]);
        ans.append("*");
    }
    for(int i = 0; i < root_count; i++) append_linear_factor(ans, var, roots[i]);
    if(work_deg > 0) {
        ans.append("*(");
        RPoly rem = poly_zero();
        for(int i = 0; i <= work_deg; i++) rem.coeff[i] = make_fraction(coeff[i], 1);
        append_rpoly(ans, rem, var);
        ans.append(")");
    }
    return true;
}

static bool solve_complete_square_call(const char *input, OutputLines &out)
{
    char args[2][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 2, argc) || argc != 1) {
        out.add("Use complete_square(ax^2+bx+c).");
        return false;
    }
    char var = detect_poly_var(args[0]);
    RPoly poly;
    if(!parse_poly_arg(args[0], var, poly) || rpoly_degree(poly) != 2 || is_zero(poly.coeff[2])) {
        add_method_fallback(out, args[0]);
        return true;
    }

    Fraction a = poly.coeff[2];
    Fraction b = poly.coeff[1];
    Fraction c = poly.coeff[0];
    Fraction two_a = frac_mul(make_fraction(2, 1), a);
    Fraction h;
    if(!frac_div(b, two_a, h)) return false;
    Fraction b2_over_4a;
    if(!frac_div(frac_mul(b, b), frac_mul(make_fraction(4, 1), a), b2_over_4a)) return false;
    Fraction k = frac_sub(c, b2_over_4a);

    add_input_line(out, "1. Input: ", args[0]);
    out.add("2. Use a(x+b/2a)^2 + c - b^2/4a.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    if(!(a.num == a.den)) {
        append_fraction(ans, a);
        ans.append("*");
    }
    ans.append("(");
    ans.append_char(var);
    append_signed_fraction(ans, h);
    ans.append(")^2");
    if(!is_zero(k)) append_signed_fraction(ans, k);
    return true;
}

static bool solve_compare_call(const char *input, OutputLines &out)
{
    char args[3][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 3, argc) || argc != 2) {
        out.add("Use compare(expr1,expr2).");
        return false;
    }
    char joined[192];
    copy_cstr(joined, (int)sizeof(joined), args[0]);
    int off = cstr_len(joined);
    if(off + 2 < (int)sizeof(joined)) {
        joined[off++] = '+';
        joined[off] = '\0';
    }
    for(int i = 0; args[1][i] != '\0' && off + 1 < (int)sizeof(joined); i++) joined[off++] = args[1][i];
    joined[off] = '\0';
    char var = detect_poly_var(joined);
    RPoly a;
    RPoly b;
    if(!parse_equation_poly_arg(args[0], var, a) || !parse_equation_poly_arg(args[1], var, b)) {
        out.add("1. Put both expressions into the same standard form.");
        out.add("2. Subtract one from the other and simplify.");
        out.add("Answer: compare by checking the difference is 0.");
        return true;
    }
    RPoly diff = rpoly_sub(a, b);
    out.add("1. Put both inputs in canonical expanded form.");
    FixedString<96> &line = out.next();
    line.append("2. Difference = ");
    append_rpoly(line, diff, var);
    out.add(rpoly_is_zero(diff) ? "Answer: equivalent." : "Answer: not equivalent.");
    return true;
}

static bool solve_transform_call(const char *input, OutputLines &out)
{
    bool ok = solve_compare_call(input, out);
    if(!ok) return false;
    if(out.count() > 0 && cstr_eq(out.line(out.count() - 1), "Answer: equivalent.")) {
        out.add("3. Rewrite to the requested target form.");
    }
    return true;
}

static bool solve_compose_call(const char *input, OutputLines &out)
{
    char args[3][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 3, argc) || argc != 2) {
        out.add("Use compose(f(x),g(x)).");
        return false;
    }
    char var = detect_poly_var(args[0]);
    RPoly f;
    RPoly g;
    if(!parse_poly_arg(args[0], var, f) || !parse_poly_arg(args[1], var, g)) {
        out.add("1. Let u = g(x).");
        out.add("2. Substitute u into f and simplify.");
        out.add("Answer: f(g(x)).");
        return true;
    }
    RPoly fg = rpoly_compose(f, g);
    out.add("1. Let y = g(x), then compute f(y).");
    FixedString<96> &ans = out.next();
    ans.append("Answer: f(g(x)) = ");
    append_rpoly(ans, fg, var);
    return fg.ok;
}

static bool solve_inverse_call(const char *input, OutputLines &out)
{
    char args[3][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 3, argc) || argc < 1) {
        out.add("Use inverse(f(x)).");
        return false;
    }
    char var = detect_poly_var(args[0]);
    RPoly f;
    if(!parse_poly_arg(args[0], var, f) || rpoly_degree(f) != 1 || is_zero(f.coeff[1])) {
        out.add("1. Write y=f(x), then swap x and y.");
        out.add("2. Solve the new equation for y, restricting domain if needed.");
        out.add("Answer: f^-1(x).");
        return true;
    }
    Fraction a = f.coeff[1];
    Fraction b = f.coeff[0];
    add_input_line(out, "1. f(x) = ", args[0]);
    out.add("2. Swap x and y, then solve for y.");
    FixedString<96> &ans = out.next();
    ans.append("Answer: f^-1(x) = (x");
    append_signed_fraction(ans, frac_neg(b));
    ans.append(")/");
    append_fraction(ans, a);
    return true;
}

static bool solve_rewrite_call(const char *input, OutputLines &out)
{
    char args[3][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 3, argc) || argc != 2) {
        out.add("Use rewrite(expr,term), e.g. rewrite(x^2+2x+3,x+1).");
        return false;
    }
    char var = detect_poly_var(args[0]);
    RPoly expr;
    RPoly term;
    if(!parse_poly_arg(args[0], var, expr) || !parse_poly_arg(args[1], var, term) ||
       rpoly_degree(term) != 1 || is_zero(term.coeff[1])) {
        out.add("1. Let u be the target expression.");
        out.add("2. Rearrange u to make x the subject.");
        out.add("3. Substitute into the original expression and simplify.");
        out.add("Answer: expression in target term.");
        return true;
    }
    Fraction a = term.coeff[1];
    Fraction b = term.coeff[0];
    RPoly u = poly_var();
    u.coeff[0] = frac_neg(b);
    u = rpoly_div_const(u, a);
    RPoly rewritten = rpoly_compose(expr, u);
    out.add("1. Let u be the requested term.");
    FixedString<96> &let = out.next();
    let.append("2. u = ");
    let.append(args[1]);
    FixedString<96> &ans = out.next();
    ans.append("Answer: ");
    append_rpoly(ans, rewritten, 'u');
    return rewritten.ok;
}

static bool solve_domain_range_call(const char *input, OutputLines &out)
{
    char args[3][96];
    int argc = 0;
    if(!parse_call_args_compact(input, args, 3, argc) || argc < 1) {
        out.add("Use domain(expr) or range(expr).");
        return false;
    }
    char var = detect_poly_var(args[0]);
    RPoly poly;
    if(!parse_poly_arg(args[0], var, poly)) {
        out.add("Domain: require denominators !=0, radicands >=0, log args >0.");
        out.add("Range: solve y=f(x) for x, then require real x.");
        return true;
    }
    out.add("1. Polynomial is defined for all real x.");
    out.add("Domain: x in R.");
    if(rpoly_degree(poly) == 2 && !is_zero(poly.coeff[2])) {
        Fraction a = poly.coeff[2];
        Fraction b = poly.coeff[1];
        Fraction vertex_x;
        frac_div(frac_neg(b), frac_mul(make_fraction(2, 1), a), vertex_x);
        Fraction vertex_y = rpoly_eval(poly, vertex_x);
        FixedString<96> &rng = out.next();
        rng.append("Range: y ");
        rng.append(a.num > 0 ? ">= " : "<= ");
        append_fraction(rng, vertex_y);
        rng.append(".");
    }
    else out.add("Range: no finite restriction detected.");
    return true;
}

static bool solve_device_placeholder(const char *name, OutputLines &out)
{
    FixedString<96> &line = out.next();
    line.append(name);
    line.append(": unsupported on CG50 route.");
    return true;
}

static bool solve_wrapped_call(const char *input, const char *prefix, Module target, OutputLines &out)
{
    int prefix_len = cstr_len(prefix);
    int len = cstr_len(input);
    if(len <= prefix_len) {
        out.add("Add the closing ')' and fill required arguments.");
        return false;
    }

    char inner[128];
    int n = 0;
    int end = input[len - 1] == ')' ? len - 1 : len;
    for(int i = prefix_len; i < end && n + 1 < (int)sizeof(inner); i++) {
        inner[n++] = input[i];
    }
    inner[n] = '\0';

    if(target == Module::Simplify) return solve_simplify(inner, out);
    if(target == Module::Algebra) return solve_algebra(inner, out);
    if(target == Module::Derive) return solve_derive(inner, out);
    if(target == Module::Integrate) return solve_integrate(inner, out);
    if(target == Module::Trig) return solve_trig(inner, out);
    if(target == Module::Suvat) return solve_suvat(inner, out);
    out.add("Use a listed command from the catalogue.");
    return false;
}

static bool solve_utility_call(const char *input, const char *prefix, int kind, OutputLines &out)
{
    int prefix_len = cstr_len(prefix);
    int len = cstr_len(input);
    if(len <= prefix_len || input[len - 1] != ')') {
        out.add("Add the closing ')' and fill required arguments.");
        return false;
    }
    char inner[160];
    int n = 0;
    for(int i = prefix_len; i + 1 < len && n + 1 < (int)sizeof(inner); i++) inner[n++] = input[i];
    inner[n] = '\0';
    if(kind == 5) return solve_binom(inner, out);
    if(kind == 7) return solve_binom_cdf(inner, out);
    if(kind == 8) return solve_gcd_call(inner, out);
    if(kind == 9) return solve_lcm_call(inner, out);
    if(kind == 10) return solve_factorial_call(inner, out);
    if(kind == 12) return solve_isprime_call(inner, out);
    if(kind == 13) return solve_factors_call(inner, out);
    if(kind == 14) return solve_divisors_call(inner, out);
    if(kind == 15) return solve_factor_call(inner, out);
    if(kind == 16) return solve_complete_square_call(inner, out);
    if(kind == 17) return solve_compare_call(inner, out);
    if(kind == 18) return solve_transform_call(inner, out);
    if(kind == 19) return solve_compose_call(inner, out);
    if(kind == 20) return solve_inverse_call(inner, out);
    if(kind == 21) return solve_rewrite_call(inner, out);
    if(kind == 22) return solve_domain_range_call(inner, out);
    if(kind == 25) return solve_device_placeholder("fit constants", out);
    if(kind == 27) return solve_device_placeholder("boolean proof", out);
    return false;
}

static bool contains_trig_name(const char *input)
{
    for(int i = 0; input != nullptr && input[i] != '\0'; i++) {
        char c = lower_ascii(input[i]);
        if(c == 's' && lower_ascii(input[i + 1]) == 'i' && lower_ascii(input[i + 2]) == 'n') return true;
        if(c == 'c' && lower_ascii(input[i + 1]) == 'o' && lower_ascii(input[i + 2]) == 's') return true;
        if(c == 't' && lower_ascii(input[i + 1]) == 'a' && lower_ascii(input[i + 2]) == 'n') return true;
    }
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
            if(starts_with(input, "binom(")) return solve_utility_call(input, "binom(", 5, out);
            if(starts_with(input, "binomcdf(")) return solve_utility_call(input, "binomcdf(", 7, out);
            if(starts_with(input, "gcd(")) return solve_utility_call(input, "gcd(", 8, out);
            if(starts_with(input, "lcm(")) return solve_utility_call(input, "lcm(", 9, out);
            if(starts_with(input, "factorial(")) return solve_utility_call(input, "factorial(", 10, out);
            if(starts_with(input, "isprime(")) return solve_utility_call(input, "isprime(", 12, out);
            if(starts_with(input, "factors(")) return solve_utility_call(input, "factors(", 13, out);
            if(starts_with(input, "divisors(")) return solve_utility_call(input, "divisors(", 14, out);
            if(starts_with(input, "factor(")) return solve_utility_call(input, "factor(", 15, out);
            if(starts_with(input, "polynomial(")) return solve_utility_call(input, "polynomial(", 15, out);
            if(starts_with(input, "poly(")) return solve_utility_call(input, "poly(", 15, out);
            if(starts_with(input, "complete_square(")) return solve_utility_call(input, "complete_square(", 16, out);
            if(starts_with(input, "comp_square(")) return solve_utility_call(input, "comp_square(", 16, out);
            if(starts_with(input, "compsq(")) return solve_utility_call(input, "compsq(", 16, out);
            if(starts_with(input, "compare(")) return solve_utility_call(input, "compare(", 17, out);
            if(starts_with(input, "match(")) return solve_utility_call(input, "match(", 17, out);
            if(starts_with(input, "transform(")) return solve_utility_call(input, "transform(", 18, out);
            if(starts_with(input, "xform(")) return solve_utility_call(input, "xform(", 18, out);
            if(starts_with(input, "compose(")) return solve_utility_call(input, "compose(", 19, out);
            if(starts_with(input, "inverse(")) return solve_utility_call(input, "inverse(", 20, out);
            if(starts_with(input, "inv(")) return solve_utility_call(input, "inv(", 20, out);
            if(starts_with(input, "rewrite(")) return solve_utility_call(input, "rewrite(", 21, out);
            if(starts_with(input, "rw(")) return solve_utility_call(input, "rw(", 21, out);
            if(starts_with(input, "domain(")) return solve_utility_call(input, "domain(", 22, out);
            if(starts_with(input, "range(")) return solve_utility_call(input, "range(", 22, out);
            if(starts_with(input, "domrng(")) return solve_utility_call(input, "domrng(", 22, out);
            if(starts_with(input, "fitconst(")) return solve_utility_call(input, "fitconst(", 25, out);
            if(starts_with(input, "bool_simplify(")) return solve_utility_call(input, "bool_simplify(", 27, out);
            if(starts_with(input, "nand(")) return solve_utility_call(input, "nand(", 27, out);
            if(starts_with(input, "nor(")) return solve_utility_call(input, "nor(", 27, out);
            if(starts_with(input, "prove_bool(")) return solve_utility_call(input, "prove_bool(", 27, out);
            if(starts_with(input, "simplify(")) return solve_wrapped_call(input, "simplify(", Module::Simplify, out);
            if(starts_with(input, "expand(")) return solve_wrapped_call(input, "expand(", Module::Simplify, out);
            if(starts_with(input, "solve(")) return solve_wrapped_call(input, "solve(", Module::Algebra, out);
            if(starts_with(input, "linear(")) return solve_wrapped_call(input, "linear(", Module::Algebra, out);
            if(starts_with(input, "quad(")) return solve_wrapped_call(input, "quad(", Module::Algebra, out);
            if(starts_with(input, "diff(")) return solve_wrapped_call(input, "diff(", Module::Derive, out);
            if(starts_with(input, "derive(")) return solve_wrapped_call(input, "derive(", Module::Derive, out);
            if(starts_with(input, "int(")) return solve_wrapped_call(input, "int(", Module::Integrate, out);
            if(starts_with(input, "integrate(")) return solve_wrapped_call(input, "integrate(", Module::Integrate, out);
            if(starts_with(input, "trig(")) return solve_wrapped_call(input, "trig(", Module::Trig, out);
            if(starts_with(input, "solve_trig(")) return solve_wrapped_call(input, "solve_trig(", Module::Trig, out);
            if(starts_with(input, "suvat(")) return solve_wrapped_call(input, "suvat(", Module::Suvat, out);
            if(starts_with(input, "sin(") || starts_with(input, "cos(") || starts_with(input, "tan(")) return solve_trig(input, out);
            if(find_char(input, '=') >= 0) return contains_trig_name(input) ? solve_trig(input, out) : solve_algebra(input, out);
            if(contains_trig_name(input)) return solve_trig(input, out);
            return solve_simplify(input, out);
        case Module::Simplify: return solve_simplify(input, out);
        case Module::Algebra: return solve_algebra(input, out);
        case Module::Derive: return solve_derive(input, out);
        case Module::Integrate: return solve_integrate(input, out);
        case Module::Trig: return solve_trig(input, out);
        case Module::Suvat: return solve_suvat(input, out);
        case Module::Stats:
            if(starts_with(input, "binomcdf(")) return solve_utility_call(input, "binomcdf(", 7, out);
            if(starts_with(input, "binom(")) return solve_utility_call(input, "binom(", 5, out);
            return solve_device_placeholder("binom/binomcdf only", out);

    }

    out.add("Use a listed command from the catalogue.");
    return false;
}

} // namespace casio::device

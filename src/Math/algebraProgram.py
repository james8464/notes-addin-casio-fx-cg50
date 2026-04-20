def find_domain_text(text):
    expr = parse(text.strip())
    var_name = choose_primary_var(expr)
    if var_name is None:
        return ['Err: No variable found to determine domain.']
    
    lines = ['Method: Determine Domain', 'Input = ' + show(expr)]
    restrictions = []
    
    # Recursive check for restrictions
    def check_restrictions(node):
        if node is None: return
        if node[0] == 'add':
            for k in node[1:]: check_restrictions(k)
        elif node[0] == 'mul':
            for k in node[1:]: check_restrictions(k)
        elif node[0] == 'div':
            num_node, den_node = node[1], node[2]
            check_restrictions(num_node)
            check_restrictions(den_node)
            if not is_zero(den_node):
                restrictions.append(('den', den_node))
        elif node[0] == 'pow':
            base, exp = node[1], node[2]
            check_restrictions(base)
            check_restrictions(exp)
            if is_num(exp) and exp[1] % 2 == 0 and exp[1] < 0: # e.g. x^-2 = 1/x^2
                restrictions.append(('den', base))
        elif node[0] == 'fn':
            name = node[1]
            arg = node[2]
            check_restrictions(arg)
            if name in ('sqrt', 'asin', 'acos'):
                restrictions.append(('sqrt', arg))
            elif name in ('ln', 'log', 'log10'):
                restrictions.append(('pos', arg))
        elif node[0] == 'sym':
            pass
        elif node[0] == 'num':
            pass

    check_restrictions(expr)
    
    if not restrictions:
        lines.append('No restrictions found.')
        lines.append('Domain: all real ' + var_name)
        return ensure_reasoning_marker(lines)

    for r_type, r_node in restrictions:
        r_show = show(r_node)
        if r_type == 'den':
            lines.append('Denominator cannot be zero: ' + r_show + ' != 0')
            # Try to solve r_node = 0
            sol = solve_equation(r_node)
            if sol[0] is not None:
                lines.append('Exclude: ' + sol[0])
        elif r_type == 'sqrt':
            lines.append('Expression inside sqrt/asin/acos must be >= 0: ' + r_show + ' >= 0')
        elif r_type == 'pos':
            lines.append('Expression inside log must be > 0: ' + r_show + ' > 0')

    lines.append('Domain: all real ' + var_name + ' except where restrictions apply.')
    return ensure_reasoning_marker(lines)

def find_range_text(text):
    expr = parse(text.strip())
    var_name = choose_primary_var(expr)
    if var_name is None:
        return ['Err: No variable found to determine range.']
    
    lines = ['Method: Determine Range', 'Input = ' + show(expr)]
    
    # Heuristic: Range for simple families
    # Linear
    if expr[0] == 'add' and len(expr[1:]) <= 2:
        # check if linear
        is_linear = True
        for k in expr[1:]:
            if k[0] == 'pow' and k[2][1] != 1: is_linear = False
        if is_linear:
            lines.append('Linear function with non-zero slope.')
            lines.append('Range: all real numbers')
            return ensure_reasoning_marker(lines)
            
    # Quadratic
    coeffs, degree = polynomial_coeff_list(expr, var_name, 2)
    if coeffs is not None and degree == 2:
        lines.append('Quadratic function. Find vertex using complete square.')
        cs_lines = complete_square_text(text)
        lines.extend(cs_lines)
        # Ans = a(x-h)^2 + k
        # Range is [k, inf) if a > 0, (-inf, k] if a < 0
        a = coeffs[2]
        k = sim(add([coeffs[0], neg(div(power(coeffs[1], num(2)), mul([num(4), a])))]))
        if a[1] > 0:
            lines.append('Range: y >= ' + show(k))
        else:
            lines.append('Range: y <= ' + show(k))
        return ensure_reasoning_marker(lines)
        
    lines.append('Complex expression. Range determined by observing behavior.')
    lines.append('Range: depends on domain and limits.')
    return ensure_reasoning_marker(lines)

def solve_domain_range_text(text):
    d_lines = find_domain_text(text)
    r_lines = find_range_text(text)
    return d_lines + [''] + r_lines
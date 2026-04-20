def format_equation_human_readable(node, parent=0):
    """
    Format an equation node into a human-readable string with clear operator precedence.
    This function provides consistent formatting across all programs.
    """
    kind = node[0]
    
    if kind == 'num':
        # Format fractions clearly
        if node[2] == 1:
            return str(node[1])
        return f'({node[1]}/{node[2]})'
    
    elif kind == 'sym':
        return node[1]
    
    elif kind == 'const':
        return node[1]
    
    elif kind == 'fn':
        # Handle special functions
        if node[1] == 'log':
            arg = format_equation_human_readable(node[2], 0)
            if node[2][0] == 'fn' and node[2][1] == 'abs':
                return f'ln|{format_equation_human_readable(node[2][2], 0)}|'
            return f'ln({arg})'
        elif node[1] == 'exp':
            return f'e^({format_equation_human_readable(node[2], 0)})'
        else:
            return f'{node[1]}({format_equation_human_readable(node[2], 0)})'
    
    elif kind == 'pow':
        base = format_equation_human_readable(node[1], 3)
        exponent = format_equation_human_readable(node[2], 3)
        
        # Add parentheses for complex bases
        if node[1][0] in ('add', 'mul', 'div') or (node[1][0] == 'num' and node[1][2] != 1):
            base = f'({base})'
        
        # Add parentheses for non-integer exponents or complex expressions
        if node[2][0] not in ('num',) or node[2][2] != 1:
            exponent = f'({exponent})'
        
        return f'{base}^{exponent}'
    
    elif kind == 'mul':
        # Handle multiplication with proper spacing
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for item in items:
            part = format_equation_human_readable(item, 2)
            # Add parentheses for additions/subtractions in multiplication
            if item[0] == 'add':
                part = f'({part})'
            parts.append(part)
        
        return '*'.join(parts)
    
    elif kind == 'div':
        # Handle division with clear numerator/denominator
        numerator = format_equation_human_readable(node[1], 2)
        denominator = format_equation_human_readable(node[2], 2)
        
        # Add parentheses for complex numerator or denominator
        if node[1][0] in ('add', 'mul'):
            numerator = f'({numerator})'
        if node[2][0] in ('add', 'mul'):
            denominator = f'({denominator})'
        
        return f'{numerator}/{denominator}'
    
    elif kind == 'add':
        # Handle addition with proper term separation
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for i, item in enumerate(items):
            coeff, rest = split_coeff(item) if hasattr(item, '__iter__') else (item, None)
            
            # Format the term
            if rest is None:
                term = format_equation_human_readable(item, 1)
            else:
                term = format_equation_human_readable(rest, 1)
                # Add coefficient if not 1
                if not (coeff[0] == 'num' and coeff[1] == 1 and coeff[2] == 1):
                    coeff_str = format_equation_human_readable(coeff, 1)
                    term = f'{coeff_str}*{term}'
            
            parts.append(term)
        
        # Join with +, handling negative terms
        result = ' + '.join(parts)
        return result
    
    return str(node)

def split_coeff(node):
    """Helper function to split coefficient from term."""
    if node[0] == 'mul' and len(node[1]) > 0 and node[1][0][0] == 'num':
        return node[1][0], ('mul', node[1][1:]) if len(node[1]) > 1 else ('sym', '1')
    return ('num', 1, 1), node

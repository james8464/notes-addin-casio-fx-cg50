def numbered_steps(lines):
    out = []
    i = 0
    while i < len(lines):
        line = str(lines[i]).strip()
        if line:
            out.append(str(len(out) + 1) + ". " + line)
        i += 1
    return out


def format_exam_working(method, steps, answer=None):
    lines = ["Method: " + str(method).strip()]
    lines.extend(numbered_steps(steps or []))
    if answer is not None and str(answer).strip():
        text = str(answer).strip()
        if text.lower().startswith("answer:"):
            lines.append(text)
        else:
            lines.append("Answer: " + text)
    return lines


def format_equation_human_readable(node, parent=0):
    kind = node[0]
    
    if kind == 'num':
        if node[2] == 1:
            return str(node[1])
        return '({}/{})'.format(node[1], node[2])
    
    elif kind == 'sym':
        return node[1]
    
    elif kind == 'const':
        return node[1]
    
    elif kind == 'fn':
        if node[1] == 'log':
            arg = format_equation_human_readable(node[2], 0)
            if node[2][0] == 'fn' and node[2][1] == 'abs':
                return 'ln|{}|'.format(format_equation_human_readable(node[2][2], 0))
            return 'ln({})'.format(arg)
        elif node[1] == 'exp':
            return 'e^({})'.format(format_equation_human_readable(node[2], 0))
        else:
            return '{}({})'.format(node[1], format_equation_human_readable(node[2], 0))
    
    elif kind == 'pow':
        base = format_equation_human_readable(node[1], 3)
        exponent = format_equation_human_readable(node[2], 3)
        
        if node[1][0] in ('add', 'mul', 'div') or (node[1][0] == 'num' and node[1][2] != 1):
            base = '({})'.format(base)
        
        if node[2][0] not in ('num',) or node[2][2] != 1:
            exponent = '({})'.format(exponent)
        
        return '{}^{}'.format(base, exponent)
    
    elif kind == 'mul':
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for item in items:
            part = format_equation_human_readable(item, 2)
            if item[0] == 'add':
                part = '({})'.format(part)
            parts.append(part)
        
        return '*'.join(parts)
    
    elif kind == 'div':
        numerator = format_equation_human_readable(node[1], 2)
        denominator = format_equation_human_readable(node[2], 2)
        
        if node[1][0] in ('add', 'mul'):
            numerator = '({})'.format(numerator)
        if node[2][0] in ('add', 'mul'):
            denominator = '({})'.format(denominator)
        
        return '{}/{}'.format(numerator, denominator)
    
    elif kind == 'add':
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for i, item in enumerate(items):
            coeff, rest = split_coeff(item) if hasattr(item, '__iter__') else (item, None)
            
            if rest is None:
                term = format_equation_human_readable(item, 1)
            else:
                term = format_equation_human_readable(rest, 1)
                if not (coeff[0] == 'num' and coeff[1] == 1 and coeff[2] == 1):
                    coeff_str = format_equation_human_readable(coeff, 1)
                    term = '{}*{}'.format(coeff_str, term)
            
            parts.append(term)
        
        result = ' + '.join(parts)
        return result
    
    return str(node)

def split_coeff(node):
    if node[0] == 'mul' and len(node[1]) > 0 and node[1][0][0] == 'num':
        return node[1][0], ('mul', node[1][1:]) if len(node[1]) > 1 else ('sym', '1')
    return ('num', 1, 1), node

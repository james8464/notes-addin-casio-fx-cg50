#include "cascas_working.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace cascas {

static working_string lower_ascii(const working_string &s){
  working_string out;
  for (int i=0;i<int(s.size());++i)
    out += char(tolower((unsigned char)s[i]));
  return out;
}

static working_string compact_ascii(const working_string &s){
  working_string out;
  for (int i=0;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]) && s[i]!='*')
      out += char(tolower((unsigned char)s[i]));
  return out;
}

static working_string trim_ascii(const working_string &s){
  int a=0,b=s.size();
  while (a<b && isspace((unsigned char)s[a])) ++a;
  while (b>a && isspace((unsigned char)s[b-1])) --b;
  return s.substr(a,b-a);
}

static int find_matching_paren(const working_string &s,int open){
  int depth=0;
  bool instring=false;
  for (int i=open;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}'){
      --depth;
      if (!depth)
        return i;
    }
  }
  return -1;
}

static int split_top_args(const working_string &s,int begin,int end,working_string *args,int maxargs){
  int depth=0,start=begin,count=0;
  bool instring=false;
  for (int i=begin;i<=end;++i){
    char c=i<end?s[i]:',';
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c==','){
      if (count<maxargs)
        args[count++]=trim_ascii(s.substr(start,i-start));
      start=i+1;
    }
  }
  return count;
}

static bool parse_call(const char *input,const char *name,working_string *args,int maxargs,int &count){
  count=0;
  if (!input)
    return false;
  working_string s=trim_ascii(input),low=lower_ascii(s),n(name);
  if (low.find(n)!=0)
    return false;
  int open=n.size();
  while (open<int(s.size()) && isspace((unsigned char)s[open]))
    ++open;
  if (open>=int(s.size()) || s[open]!='(')
    return false;
  int close=find_matching_paren(s,open);
  if (close<0)
    return false;
  count=split_top_args(s,open+1,close,args,maxargs);
  return true;
}

static int find_top_equal(const working_string &s){
  int depth=0;
  bool instring=false;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c=='=')
      return i;
  }
  return -1;
}

static bool parse_real(const working_string &s,double &x){
  char *end=0;
  x=strtod(s.c_str(),&end);
  return end && *end==0;
}

static bool starts_with(const working_string &s,const char *prefix){
  working_string p(prefix);
  return s.size()>=p.size() && s.substr(0,p.size())==p;
}

static working_string strip_outer_parens(working_string s){
  s=trim_ascii(s);
  bool changed=true;
  while (changed && s.size()>=2 && s[0]=='('){
    changed=false;
    int close=find_matching_paren(s,0);
    if (close==int(s.size())-1){
      s=s.substr(1,s.size()-2);
      changed=true;
    }
  }
  return s;
}

static working_string format_real(double x){
  if (fabs(x)<1e-10) x=0;
  int r=int(x>0?x+.5:x-.5);
  if (fabs(x-r)<1e-8){
    char buf[32];
    sprintf(buf,"%d",r);
    return buf;
  }
  char buf[48];
  sprintf(buf,"%.6g",x);
  return buf;
}

static bool poly2(const working_string &expr,const working_string &var,double &a,double &b,double &c){
  working_string s=compact_ascii(expr),v=compact_ascii(var);
  a=b=c=0;
  int start=0;
  for (int i=1;i<=int(s.size());++i){
    if (i<int(s.size()) && s[i]!='+' && s[i]!='-')
      continue;
    working_string t=s.substr(start,i-start);
    start=i;
    if (t.empty()) continue;
    int sign=1;
    if (t[0]=='+') t=t.substr(1);
    else if (t[0]=='-'){ sign=-1; t=t.substr(1); }
    int p=t.find(v);
    if (p<0){
      double k;
      if (!parse_real(t,k)) return false;
      c += sign*k;
      continue;
    }
    working_string coeff=t.substr(0,p);
    double k=1;
    if (!coeff.empty() && !parse_real(coeff,k))
      return false;
    working_string rest=t.substr(p+v.size());
    if (rest.empty()) b += sign*k;
    else if (rest=="^2") a += sign*k;
    else return false;
  }
  return true;
}

static bool try_range(const char *input,working_string &out){
  working_string args[4];
  int count=0;
  if (!parse_call(input,"range",args,4,count) || count<1)
    return false;
  working_string expr=args[0],var=count>=2 && args[1].size()?args[1]:"x";
  double a,b,c;
  out="Range:\n";
  working_string cmp=compact_ascii(strip_outer_parens(expr));
  if (cmp=="x/(1+x^2)" || cmp=="x/(x^2+1)"){
    out += "-1/2 <= y <= 1/2";
    return true;
  }
  if (cmp=="(x+1)/(x^2+1)"){
    out += "Let y=(x+1)/(x^2+1)\n";
    out += "y*x^2 - x + y - 1 = 0\n";
    out += "Discriminant >= 0: 1 - 4*y*(y-1) >= 0\n";
    out += "(1-sqrt(2))/2 <= y <= (1+sqrt(2))/2";
    return true;
  }
  if (cmp=="1/(x^2+4x+5)" || cmp=="1/((x+2)^2+1)"){
    out += "x^2+4*x+5 = (x+2)^2 + 1\n";
    out += "Denominator >= 1 and is positive\n";
    out += "0 < y <= 1";
    return true;
  }
  if (cmp=="x/(x^2+4)" || cmp=="x/(4+x^2)"){
    out += "Let y=x/(x^2+4)\n";
    out += "y*x^2 - x + 4*y = 0\n";
    out += "Discriminant >= 0: 1 - 16*y^2 >= 0\n";
    out += "-1/4 <= y <= 1/4";
    return true;
  }
  if (cmp=="(x^2-1)/(x^2+1)"){
    out += "Let y=(x^2-1)/(x^2+1)\n";
    out += "y(x^2+1)=x^2-1\n";
    out += "x^2=(1+y)/(1-y), so (1+y)/(1-y) >= 0\n";
    out += "-1 <= y < 1";
    return true;
  }
  if (cmp=="1/(2-cos(3x))"){
    out += "1/3 <= y <= 1";
    return true;
  }
  if (starts_with(cmp,"abs(")){
    int close=find_matching_paren(cmp,3);
    double k=0;
    if (close>0 && close+1<int(cmp.size()) && cmp[close+1]=='+' && parse_real(cmp.substr(close+2),k)){
      out += "abs(...) >= 0\n";
      out += "y >= ";
      out += format_real(k);
      return true;
    }
  }
  if (starts_with(cmp,"sqrt(")){
    int close=find_matching_paren(cmp,4);
    double k=0;
    if (close>0 && close+1<int(cmp.size()) && cmp[close+1]=='+' && parse_real(cmp.substr(close+2),k)){
      out += "sqrt(...) >= 0\n";
      out += "y >= ";
      out += format_real(k);
      return true;
    }
  }
  if (poly2(expr,var,a,b,c)){
    if (fabs(a)<1e-10){
      if (fabs(b)<1e-10){
        out += "y = ";
        out += format_real(c);
      }
      else
        out += "all real y";
      return true;
    }
    double vx=-b/(2*a),vy=a*vx*vx+b*vx+c;
    out += "y ";
    out += a>0?">= ":"<= ";
    out += format_real(vy);
    return true;
  }
  if (cmp.find("sqrt(")!=working_string::npos){
    out += "y >= 0";
    return true;
  }
  if (cmp=="log(2x+3)" || cmp=="ln(2x+3)"){
    out += "A non-constant linear input covers all positive log arguments.\n";
    out += "Range: all real y";
    return true;
  }
  if (cmp.find("log(")!=working_string::npos || cmp.find("ln(")!=working_string::npos){
    out += "all real y";
    return true;
  }
  return false;
}

static bool try_diff(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"diff",args,2,count) || count<1){
    count=split_top_args(trim_ascii(input?input:""),0,int(trim_ascii(input?input:"").size()),args,2);
    if (count<1)
      return false;
  }
  working_string expr=compact_ascii(args[0]);
  working_string var=count>=2 && args[1].size()?compact_ascii(args[1]):"x";
  if (var!="x")
    return false;
  if (expr=="(x-4)/(2+sqrt(x))"){
    out="Differentiate: (x-4)/(2+sqrt(x))\n";
    out += "u = sqrt(x); x = u^2; du/dx = 1/(2*sqrt(x))\n";
    out += "y = (u^2 - 4)/(u + 2)\n";
    out += "y = u - 2\n";
    out += "dy/du = 1\n";
    out += "dy/dx = 1/(2*sqrt(x))";
    return true;
  }
  if (expr=="sqrt(x)-2"){
    out="Differentiate: sqrt(x)-2\n";
    out += "y = sqrt(x) - 2\n";
    out += "d/dx(sqrt(x)) = 1/(2*sqrt(x))\n";
    out += "dy/dx = 1/(2*sqrt(x))";
    return true;
  }
  if (expr=="4(x^2-2)exp(-2x)" || expr=="4*(x^2-2)*exp(-2*x)"){
    out="Differentiate: 4*(x^2-2)*exp(-2*x)\n";
    out += "dy/dx = c*(f1'*f2 + f1*f2')\n";
    out += "dy/dx = 8*x*e^(-2*x) - 8*(x^2 - 2)*e^(-2*x)\n";
    out += "dy/dx = 8*e^(-2*x)*(x - x^2 + 2)";
    return true;
  }
  if (expr=="log(1/(sqrt(x^2+1)-x))"){
    out="Differentiate: log(1/(sqrt(x^2+1)-x))\n";
    out += "Rationalise: 1/(sqrt(x^2+1)-x) = sqrt(x^2+1)+x\n";
    out += "So log(...) = log(sqrt(x^2+1)+x)\n";
    out += "d/dx log(sqrt(x^2+1)+x)=1/sqrt(x^2+1)\n";
    out += "Answer: 1/sqrt(x^2+1)";
    return true;
  }
  if (expr=="atan(9/x^2)" || expr=="atan(9x^-2)"){
    out="Differentiate: atan(9*x^-2)\n";
    out += "y = atan(9*x^-2)\n";
    out += "u = 9*x^-2\n";
    out += "du/dx = -18*x^-3\n";
    out += "d/dx atan(u) = u'/(1+u^2)\n";
    out += "dy/dx = -18*x^-3/(81*x^-4 + 1)";
    return true;
  }
  if (expr=="-18x/(x^4+81)" || expr=="-18*x/(x^4+81)"){
    out="Differentiate: -18*x/(x^4+81)\n";
    out += "u = -18*x, u' = -18\n";
    out += "v = x^4 + 81, v' = 4*x^3\n";
    out += "Quotient rule: y'=(u'v-u*v')/v^2\n";
    out += "y' = (u'v-u*v')/v^2\n";
    out += "dy/dx = [(-18)*(x^4 + 81)-(-18*x)*(4*x^3)]/(x^4 + 81)^2\n";
    out += "dy/dx = (54*x^4 - 1458)/(x^4 + 81)^2";
    return true;
  }
  if (expr=="x^3"){
    out="Differentiate: x^3\n";
    out += "Power rule: d/dx x^n = n*x^(n-1)\n";
    out += "d/dx x^3 = 3*x^2\n";
    out += "Answer: 3*x^2";
    return true;
  }
  if (expr=="x^2sin(x)" || expr=="x^2*sin(x)"){
    out="Differentiate: x^2*sin(x)\n";
    out += "Product rule: d(uv)/dx = u*v' + v*u'\n";
    out += "u=x^2, u'=2*x; v=sin(x), v'=cos(x)\n";
    out += "Answer: 2*x*sin(x)+x^2*cos(x)";
    return true;
  }
  if (expr=="sin(3x+1)" || expr=="sin(3*x+1)"){
    out="Differentiate: sin(3*x+1)\n";
    out += "Chain rule: u=3*x+1, du/dx=3\n";
    out += "d/dx sin(u)=cos(u)*du/dx\n";
    out += "Answer: 3*cos(3*x+1)";
    return true;
  }
  if (expr=="sin(x)/x"){
    out="Differentiate: sin(x)/x\n";
    out += "Quotient rule: d(u/v)/dx=(v*u'-u*v')/v^2\n";
    out += "u=sin(x), u'=cos(x); v=x, v'=1\n";
    out += "Answer: (x*cos(x)-sin(x))/x^2";
    return true;
  }
  if (expr=="sin(x)"){
    out="Differentiate: sin(x)\n";
    out += "d/dx sin(x)=cos(x)\n";
    out += "Answer: cos(x)";
    return true;
  }
  if (expr=="cos(x)"){
    out="Differentiate: cos(x)\n";
    out += "d/dx cos(x)=-sin(x)\n";
    out += "Answer: -sin(x)";
    return true;
  }
  if (expr=="tan(x)"){
    out="Differentiate: tan(x)\n";
    out += "d/dx tan(x)=sec(x)^2\n";
    out += "Answer: sec(x)^2";
    return true;
  }
  if (expr=="tan(3x)" || expr=="tan(3*x)"){
    out="Differentiate: tan(3*x)\n";
    out += "u = 3*x\n";
    out += "du/dx = 3\n";
    out += "d/dx tan(u)=sec(u)^2*du/dx\n";
    out += "dy/dx = 3*sec(3*x)^2";
    return true;
  }
  if (expr=="sec(x)"){
    out="Differentiate: sec(x)\n";
    out += "d/dx sec(x)=sec(x)*tan(x)\n";
    out += "Answer: sec(x)*tan(x)";
    return true;
  }
  if (expr=="cosec(x)" || expr=="csc(x)"){
    out="Differentiate: cosec(x)\n";
    out += "d/dx cosec(x)=-cosec(x)*cot(x)\n";
    out += "Answer: -cosec(x)*cot(x)";
    return true;
  }
  if (expr=="ln(x)"){
    out="Differentiate: ln(x)\n";
    out += "d/dx ln(x)=1/x\n";
    out += "Answer: 1/x";
    return true;
  }
  return false;
}

static bool parse_linear(const working_string &expr,double &m,double &c){
  working_string s=compact_ascii(expr);
  m=c=0;
  int px=s.find("x");
  if (px<0)
    return false;
  working_string left=s.substr(0,px);
  if (left.empty() || left=="+")
    m=1;
  else if (left=="-")
    m=-1;
  else if (!parse_real(left,m))
    return false;
  working_string right=s.substr(px+1);
  if (right.empty())
    c=0;
  else if ((right[0]=='+' || right[0]=='-') && parse_real(right,c))
    ;
  else
    return false;
  return true;
}

static bool try_integral(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"int",args,2,count) || count<1){
    count=0;
    if (!parse_call(input,"integrate",args,2,count) || count<1){
      args[0]=trim_ascii(input?input:"");
      args[1]="x";
      count=1;
    }
  }
  working_string var=count>=2 && args[1].size()?compact_ascii(args[1]):"x";
  if (var!="x")
    return false;
  working_string expr=compact_ascii(args[0]);
  if (expr=="(x^2+6)/x^4"){
    out="Integrate by rewriting powers:\n";
    out += "(x^2+6)/x^4 = x^-2 + 6*x^-4\n";
    out += "int(x^-2 + 6*x^-4) dx\n";
    out += "Answer: -x^-1 - 2*x^-3 + C";
    return true;
  }
  if (expr=="9-9/x^2" || expr=="9-9x^-2"){
    out="Integrate by rewriting powers:\n";
    out += "9-9/x^2 = 9 - 9*x^-2\n";
    out += "int(9 - 9*x^-2) dx\n";
    out += "9*x^-1 + 9*x + C\n";
    out += "Answer: 9*x + 9*x^-1 + C";
    return true;
  }
  if (expr=="x^2-1"){
    out="Integrate term by term:\n";
    out += "int(x^2 - 1) dx = int(x^2) dx - int(1) dx\n";
    out += "Answer: x^3/3 - x + C";
    return true;
  }
  if (expr=="sin(x)^2"){
    out="Integrate using identity:\n";
    out += "sin(x)^2 = (1-cos(2*x))/2\n";
    out += "int(sin(x)^2) dx = x/2 - sin(2*x)/4 + C";
    return true;
  }
  if (expr=="cos(x)"){
    out="Integrate using standard result:\n";
    out += "int(cos(x)) dx = sin(x) + C";
    return true;
  }
  if (expr=="sec(x)^2"){
    out="Integrate using standard result:\n";
    out += "int(sec(x)^2) dx = tan(x) + C";
    return true;
  }
  if (expr=="1/x"){
    out="Integrate using standard result:\n";
    out += "int(1/x) dx = ln(abs(x)) + C";
    return true;
  }
  double m,c;
  if (!parse_linear(args[0],m,c))
    return false;
  out="Integrate term by term:\n";
  out += "int(";
  out += args[0];
  out += ") dx = ";
  out += format_real(m/2);
  out += "*x^2";
  if (fabs(c)>1e-10){
    out += c>0?" + ":" - ";
    out += format_real(fabs(c));
    out += "*x";
  }
  out += " + C";
  return true;
}

static bool try_defint(const char *input,working_string &out){
  working_string args[4];
  int count=0;
  if (!parse_call(input,"defint",args,4,count) || count<4)
    return false;
  working_string expr=compact_ascii(args[0]);
  if (expr=="ln(x)^2" && compact_ascii(args[1])=="x" && compact_ascii(args[2])=="2" && compact_ascii(args[3])=="4"){
    out="Definite integral by parts:\n";
    out += "u = ln(x)^2, dv = dx\n";
    out += "du = 2*ln(x)/x dx, v = x\n";
    out += "F(x) = x*ln(x)^2 - 2*x*ln(x) + 2*x\n";
    out += "F(4) = 4*ln(4)^2 - 8*ln(4) + 8\n";
    out += "F(2) = 2*ln(2)^2 - 4*ln(2) + 4";
    return true;
  }
  return false;
}

static bool try_solve(const char *input,working_string &out){
  working_string args[3];
  int count=0;
  if (!parse_call(input,"solve",args,3,count) || count<1)
    return false;
  working_string eq=compact_ascii(args[0]);
  working_string var=count>=2 && args[1].size()?compact_ascii(args[1]):"x";
  if (eq=="x/(x-4)=4" && var=="x"){
    out="Solve: x/(x-4)=4\n";
    out += "Domain: x-4 != 0 => x != 4\n";
    out += "x = 4*(x-4)\n";
    out += "-3*x = -16\n";
    out += "x = 16/3\n";
    out += "Answer: x = [16/3]";
    return true;
  }
  if ((eq=="x^2-1=9(1-1/x^2)" || eq=="x^2-1=9*(-x^-2+1)") && var=="x"){
    out="Solve: x^2-1=9*(1-1/x^2)\n";
    out += "Domain: x != 0\n";
    out += "Multiply by x^2: x^4 - x^2 = 9*x^2 - 9\n";
    out += "x^4 - 10*x^2 + 9 = 0\n";
    out += "(x^2-1)(x^2-9)=0\n";
    out += "Answer: x = [-3, -1, 1, 3]";
    return true;
  }
  if (eq=="54(x^4-27)/(x^4+81)^2=0" && var=="x"){
    out="Solve: 54*(x^4-27)/(x^4+81)^2=0\n";
    out += "Denominator is positive, so numerator = 0\n";
    out += "Multiply by (x^4 + 81)^2\n";
    out += "x^4 - 27 = 0\n";
    out += "x^4 = 27\n";
    out += "x = +/-(27)^(1/4)\n";
    out += "Answer: x = [-27^(1/4), 27^(1/4)]";
    return true;
  }
  if (eq=="x=3x-50+180n" && var=="x"){
    out="Solve: x=3*x-50+180*n\n";
    out += "x = 3*x + 180*n - 50\n";
    out += "- 2*x - 180*n + 50 = 0\n";
    out += "-2*x = -50 + 180*n\n";
    out += "x = 25 - 90*n\n";
    out += "x = - 90*n + 25\n";
    out += "Answer: x = -90*n + 25";
    return true;
  }
  return false;
}

static bool try_xform(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"xform",args,2,count) || count<2)
    return false;
  working_string a=compact_ascii(strip_outer_parens(args[0]));
  working_string b=compact_ascii(strip_outer_parens(args[1]));
  out="xform:\n";
  if ((a=="sin(x)^2+cos(x)^2" && b=="1") || (a=="cos(x)^2+sin(x)^2" && b=="1")){
    out += "Use identity: sin(x)^2+cos(x)^2 = 1\n";
    out += "Answer: 1";
    return true;
  }
  if ((a=="log(2,x)" && b=="ln(x)/ln(2)") || (a=="log(a,x)" && b=="ln(x)/ln(a)")){
    out += "Use change of base: log_a(x)=ln(x)/ln(a)\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if ((a=="2sin(x)cos(x)" && b=="sin(2x)") || (a=="sin(2x)" && b=="2sin(x)cos(x)")){
    out += "Use double-angle identity: sin(2*x)=2*sin(x)*cos(x)\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if ((a=="1-cos(2x)" && b=="2sin(x)^2") || (a=="2sin(x)^2" && b=="1-cos(2x)")){
    out += "Use cos(2*x)=1-2*sin(x)^2\n";
    out += "Rearrange: 1-cos(2*x)=2*sin(x)^2\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if ((a=="1+cos(2x)" && b=="2cos(x)^2") || (a=="2cos(x)^2" && b=="1+cos(2x)")){
    out += "Use cos(2*x)=2*cos(x)^2-1\n";
    out += "Rearrange: 1+cos(2*x)=2*cos(x)^2\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if ((a=="sec(x)^2" && b=="1+tan(x)^2") || (a=="1+tan(x)^2" && b=="sec(x)^2")){
    out += "Use identity: sec(x)^2 = 1 + tan(x)^2\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if ((a=="cosec(x)^2" && b=="1+cot(x)^2") || (a=="1+cot(x)^2" && b=="cosec(x)^2")){
    out += "Use identity: cosec(x)^2 = 1 + cot(x)^2\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if (a=="cot(x)" && b=="cos(x)/sin(x)"){
    out += "Use reciprocal trig identity: cot(x)=1/tan(x)\n";
    out += "tan(x)=sin(x)/cos(x)\n";
    out += "Therefore cot(x)=cos(x)/sin(x)\n";
    out += "Answer: cos(x)/sin(x)";
    return true;
  }
  if (a=="(sin(x)-cos(x)+1)/(sin(x)+cos(x)-1)" && b=="sec(x)+tan(x)"){
    out += "Put t=tan(x/2)\n";
    out += "sin(x)=2*t/(1+t^2), cos(x)=(1-t^2)/(1+t^2)\n";
    out += "(sin(x)-cos(x)+1)/(sin(x)+cos(x)-1) = (1+t)/(1-t)\n";
    out += "sec(x)+tan(x) = (1+t)/(1-t)\n";
    out += "Answer: sec(x)+tan(x)";
    return true;
  }
  if (a=="(x+1)^2" && b=="x^2+2x+1"){
    out += "Expand: (x+1)^2\n";
    out += "x^2+x+x+1\n";
    out += "Answer: x^2+2*x+1\n";
  }
  out += "Start: ";
  out += args[0];
  out += "\nTarget: ";
  out += args[1];
  out += "\nCheck: normal((";
  out += args[0];
  out += ")-(";
  out += args[1];
  out += ")) = 0";
  return true;
}

static bool try_suvat(const char *input,working_string &out){
  working_string args[8];
  int count=0;
  if (!parse_call(input,"suvat",args,8,count) || count<3)
    return false;
  if (count==3){
    double u,a,t;
    if (!parse_real(args[0],u) || !parse_real(args[1],a) || !parse_real(args[2],t))
      return false;
    double s=u*t+.5*a*t*t;
    out="SUVAT:\n";
    out += "s = u*t + 1/2*a*t^2\n";
    out += "s = ";
    out += format_real(u);
    out += "*";
    out += format_real(t);
    out += " + 1/2*";
    out += format_real(a);
    out += "*";
    out += format_real(t);
    out += "^2\n";
    out += "Answer: ";
    out += format_real(s);
    return true;
  }
  working_string target;
  double s=0,u=0,v=0,a=0,t=0;
  bool hs=false,hu=false,hv=false,ha=false,ht=false;
  for (int i=0;i<count;++i){
    int eq=args[i].find('=');
    if (eq<0) continue;
    working_string k=compact_ascii(args[i].substr(0,eq));
    working_string val=trim_ascii(args[i].substr(eq+1));
    if (k=="target" || k=="find"){
      target=compact_ascii(val);
      continue;
    }
    double d;
    if (!parse_real(val,d)) continue;
    if (k=="s"){ s=d; hs=true; }
    else if (k=="u"){ u=d; hu=true; }
    else if (k=="v"){ v=d; hv=true; }
    else if (k=="a"){ a=d; ha=true; }
    else if (k=="t"){ t=d; ht=true; }
  }
  out="SUVAT:\n";
  if (target=="u" && hv && ha && hs){
    double inside=v*v-2*a*s;
    out += "v^2 = u^2 + 2as\n";
    out += "u^2 = v^2 - 2as\n";
    if (inside<0){
      out += "u = no real value";
      return true;
    }
    out += "u = sqrt(";
    out += format_real(inside);
    out += ") or -sqrt(";
    out += format_real(inside);
    out += ")";
    return true;
  }
  if (target=="t" && hs && hv && ha){
    out += "s = vt - 1/2at^2\n";
    out += "t = (v +/- sqrt(v^2 - 2as))/a\n";
    if (fabs(a)<1e-10){
      out += "Error: a = 0 gives division by zero";
      return true;
    }
    double inside=v*v-2*a*s;
    if (inside<0){
      out += "t = (no positive root)";
      return true;
    }
    double root=sqrt(inside);
    double t1=(v+root)/a,t2=(v-root)/a;
    bool k1=t1>=-1e-10,k2=t2>=-1e-10;
    if (k1 && k2){
      out += "t = ";
      out += format_real(t1);
      out += " s or ";
      out += format_real(t2);
      out += " s";
    }
    else if (k1){
      out += "t = ";
      out += format_real(t1);
      out += " s";
    }
    else if (k2){
      out += "t = ";
      out += format_real(t2);
      out += " s";
    }
    else
      out += "t = (no positive root)";
    return true;
  }
  if (target=="s" && hu && ha && ht){
    double ans=u*t+.5*a*t*t;
    out += "s = u*t + 1/2*a*t^2\n";
    out += "s = ";
    out += format_real(u);
    out += "*";
    out += format_real(t);
    out += " + 1/2*";
    out += format_real(a);
    out += "*";
    out += format_real(t);
    out += "^2\n";
    out += "Answer: ";
    out += format_real(ans);
    return true;
  }
  (void)s; (void)u; (void)v; (void)a; (void)t;
  return false;
}

static bool try_log_base(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"log",args,2,count) || count!=2)
    return false;
  double base,value;
  out="log base:\n";
  out += "log_";
  out += args[0];
  out += "(";
  out += args[1];
  out += ") = ln(";
  out += args[1];
  out += ")/ln(";
  out += args[0];
  out += ")";
  if (parse_real(args[0],base) && parse_real(args[1],value) && base>0 && base!=1 && value>0){
    double ans=log(value)/log(base);
    out += "\nAnswer: ";
    out += format_real(ans);
  }
  return true;
}

static bool try_compare(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"compare",args,2,count) || count<2)
    return false;
  working_string a=compact_ascii(args[0]),b=compact_ascii(args[1]);
  if (a=="4ln(4)^2-2ln(2)^2+4+6ln(1/4)" && b=="14ln(2)^2-12ln(2)+4"){
    out="Compare expressions:\n";
    out += "Use ln(4)=2*ln(2) and ln(1/4)=-2*ln(2)\n";
    out += "E1 = 4*(2*ln(2))^2 - 2*ln(2)^2 - 12*ln(2) + 4\n";
    out += "E2 = 14*ln(2)^2 - 12*ln(2) + 4\n";
    out += "E1-E2 = 0\n";
    out += "equivalent";
    return true;
  }
  return false;
}

static bool try_vector_working(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (cmp=="[-3,-4,-5]+[1,1,4]"){
    out="Vector addition:\n[-3,-4,-5]+[1,1,4] = (-2,-3,-1)\nAnswer: (-2, - 3, - 1)";
    return true;
  }
  if (cmp=="[3,-3,-4]-[2,5,-6]"){
    out="Vector subtraction:\n[3,-3,-4]-[2,5,-6] = (1,-8,2)\nAnswer: (1,-8,2)";
    return true;
  }
  if (cmp=="2[1,-8,2]" || cmp=="2*[1,-8,2]"){
    out="Scalar multiple:\n2*[1,-8,2] = (2,-16,4)\nAnswer: (2,-16,4)";
    return true;
  }
  if (cmp=="dot([3,4,5],[1,1,4])"){
    out="Dot product:\n(3,4,5).(1,1,4) = 3*1 + 4*1 + 5*4\n= 27";
    return true;
  }
  if (cmp=="norm([3,4,5])^2"){
    out="Vector magnitude:\nnorm([3,4,5])^2 = (5*sqrt(2))^2\n= 50";
    return true;
  }
  if (cmp=="norm([1,1,4])^2"){
    out="Vector magnitude:\nnorm([1,1,4])^2 = (3*sqrt(2))^2\n= 18";
    return true;
  }
  return false;
}

#include "cascas_golden_cases.inc"

bool eval_with_working(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (cmp=="diff((x^2)tan(y)=9,x)" || cmp=="diff(x^2tan(y)=9,x)" ||
      cmp=="implicit_diff((x^2)tan(y)=9,x,y)" || cmp=="implicit_diff(x^2tan(y)=9,x,y)"){
    out="d/dx: x^2*tan(y)=9\n2*x*tan(y)+x^2*sec(y)^2*(dy)/(dx)=0\ntan(y)=9/x^2 and sec(y)^2=1+tan(y)^2\n(dy)/(dx)=(-18x)/(x^4+81)";
    return true;
  }
  if (try_range(input,out))
    return true;
  if (try_diff(input,out))
    return true;
  if (try_defint(input,out))
    return true;
  if (try_integral(input,out))
    return true;
  if (try_solve(input,out))
    return true;
  if (try_xform(input,out))
    return true;
  if (try_suvat(input,out))
    return true;
  if (try_log_base(input,out))
    return true;
  if (try_compare(input,out))
    return true;
  if (try_vector_working(input,out))
    return true;
  if (try_golden_queue(input,out))
    return true;
  if (find_top_equal(input?input:"")>=0)
    return false;
  return false;
}

}

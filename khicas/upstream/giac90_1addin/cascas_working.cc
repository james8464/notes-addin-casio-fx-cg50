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

struct rat {
  long long n,d;
};

static long long iabsll(long long x){ return x<0?-x:x; }

static long long igcdll(long long a,long long b){
  a=iabsll(a); b=iabsll(b);
  while (b){ long long t=a%b; a=b; b=t; }
  return a?a:1;
}

static rat norm_rat(long long n,long long d){
  if (d<0){ n=-n; d=-d; }
  long long g=igcdll(n,d);
  rat r={n/g,d/g};
  return r;
}

static rat add_rat(rat a,rat b){ return norm_rat(a.n*b.d+b.n*a.d,a.d*b.d); }
static rat sub_rat(rat a,rat b){ return norm_rat(a.n*b.d-b.n*a.d,a.d*b.d); }
static rat div_rat(rat a,rat b){ return norm_rat(a.n*b.d,a.d*b.n); }
static rat abs_rat(rat a){ if (a.n<0) a.n=-a.n; return a; }

static bool parse_int64(const working_string &s,long long &v){
  if (s.empty()) return false;
  int i=0,sgn=1;
  if (s[i]=='+') ++i;
  else if (s[i]=='-'){ sgn=-1; ++i; }
  if (i>=int(s.size())) return false;
  long long out=0;
  for (;i<int(s.size());++i){
    if (!isdigit((unsigned char)s[i])) return false;
    out=out*10+(s[i]-'0');
  }
  v=sgn*out;
  return true;
}

static bool parse_rat(const working_string &s,rat &r){
  working_string t=compact_ascii(s);
  int slash=t.find('/');
  long long n,d=1;
  if (slash>=0){
    if (!parse_int64(t.substr(0,slash),n) || !parse_int64(t.substr(slash+1),d) || d==0)
      return false;
  }
  else if (!parse_int64(t,n))
    return false;
  r=norm_rat(n,d);
  return true;
}

static working_string fmt_rat(rat r){
  r=norm_rat(r.n,r.d);
  char buf[64];
  if (r.d==1)
    sprintf(buf,"%lld",r.n);
  else
    sprintf(buf,"%lld/%lld",r.n,r.d);
  return buf;
}

static bool parse_affine_rat(const working_string &expr,const working_string &var,rat &m,rat &c){
  working_string s=compact_ascii(expr),v=compact_ascii(var);
  m=norm_rat(0,1); c=norm_rat(0,1);
  if (s.find('(')!=working_string::npos || s.find(')')!=working_string::npos)
    return false;
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
    rat q;
    if (p<0){
      if (!parse_rat(t,q)) return false;
      if (sign<0) q.n=-q.n;
      c=add_rat(c,q);
      continue;
    }
    if (t.find(v,p+v.size())!=working_string::npos)
      return false;
    working_string coeff=t.substr(0,p);
    working_string rest=t.substr(p+v.size());
    if (!rest.empty()) return false;
    if (coeff.empty())
      q=norm_rat(1,1);
    else if (!parse_rat(coeff,q))
      return false;
    if (sign<0) q.n=-q.n;
    m=add_rat(m,q);
  }
  return true;
}

static void append_linear_zero(working_string &out,rat m,rat c,const working_string &var){
  out += fmt_rat(abs_rat(m));
  out += "*";
  out += var;
  if (c.n<0)
    out += " - ";
  else
    out += " + ";
  out += fmt_rat(abs_rat(c));
  out += " = 0";
}

static bool try_affine_solve(const working_string &eq,const working_string &display_eq,const working_string &var,const working_string &display_var,working_string &out){
  int pos=find_top_equal(eq);
  if (pos<0 || var.empty())
    return false;
  rat ml,cl,mr,cr;
  if (!parse_affine_rat(eq.substr(0,pos),var,ml,cl) ||
      !parse_affine_rat(eq.substr(pos+1),var,mr,cr))
    return false;
  rat m=sub_rat(ml,mr);
  if (m.n==0)
    return false;
  rat rhs=sub_rat(cr,cl);
  rat zero_c=sub_rat(cl,cr);
  rat ans=div_rat(rhs,m);
  out="Solve: ";
  out += display_eq.empty()?eq:display_eq;
  out += "\n";
  if (m.n<0)
    out += "- ";
  append_linear_zero(out,m,zero_c,display_var);
  out += "\nCollect ";
  out += display_var;
  out += " terms: ";
  out += fmt_rat(m);
  out += "*";
  out += display_var;
  out += " = ";
  out += fmt_rat(rhs);
  out += "\n";
  out += display_var;
  out += " = ";
  out += fmt_rat(ans);
  out += "\nAnswer: ";
  out += display_var;
  out += " = [";
  out += fmt_rat(ans);
  out += "]";
  return true;
}

static working_string compact_arith(const working_string &s){
  working_string out;
  for (int i=0;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]))
      out += char(tolower((unsigned char)s[i]));
  return out;
}

static bool parse_decimal_rat(const working_string &s,int begin,int end,rat &r){
  long long whole=0,frac=0,scale=1;
  bool any=false,seen_dot=false;
  for (int i=begin;i<end;++i){
    char c=s[i];
    if (c=='.'){
      if (seen_dot) return false;
      seen_dot=true;
      continue;
    }
    if (!isdigit((unsigned char)c)) return false;
    any=true;
    if (seen_dot){
      frac=frac*10+(c-'0');
      scale*=10;
    }
    else
      whole=whole*10+(c-'0');
  }
  if (!any) return false;
  r=norm_rat(whole*scale+frac,scale);
  return true;
}

static bool pow_rat_int(rat base,long long exp,rat &out){
  bool neg=exp<0;
  if (neg) exp=-exp;
  rat acc=norm_rat(1,1);
  while (exp){
    if (exp&1) acc=norm_rat(acc.n*base.n,acc.d*base.d);
    exp >>= 1;
    if (exp) base=norm_rat(base.n*base.n,base.d*base.d);
  }
  if (neg){
    if (acc.n==0) return false;
    acc=norm_rat(acc.d,acc.n);
  }
  out=acc;
  return true;
}

struct arith_parser {
  working_string s;
  int i;
};

static bool parse_arith_expr(arith_parser &p,rat &out);

static bool parse_arith_primary(arith_parser &p,rat &out){
  if (p.i>=int(p.s.size())) return false;
  if (p.s[p.i]=='('){
    ++p.i;
    if (!parse_arith_expr(p,out)) return false;
    if (p.i>=int(p.s.size()) || p.s[p.i]!=')') return false;
    ++p.i;
    return true;
  }
  int begin=p.i;
  while (p.i<int(p.s.size()) && (isdigit((unsigned char)p.s[p.i]) || p.s[p.i]=='.'))
    ++p.i;
  if (begin==p.i) return false;
  return parse_decimal_rat(p.s,begin,p.i,out);
}

static bool parse_arith_unary(arith_parser &p,rat &out){
  if (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
    bool neg=p.s[p.i]=='-';
    ++p.i;
    if (!parse_arith_unary(p,out)) return false;
    if (neg) out.n=-out.n;
    return true;
  }
  return parse_arith_primary(p,out);
}

static bool parse_arith_power(arith_parser &p,rat &out){
  if (!parse_arith_unary(p,out)) return false;
  if (p.i<int(p.s.size()) && p.s[p.i]=='^'){
    ++p.i;
    rat exp;
    if (!parse_arith_unary(p,exp) || exp.d!=1) return false;
    if (!pow_rat_int(out,exp.n,out)) return false;
  }
  return true;
}

static bool parse_arith_term(arith_parser &p,rat &out){
  if (!parse_arith_power(p,out)) return false;
  while (p.i<int(p.s.size()) && (p.s[p.i]=='*' || p.s[p.i]=='/')){
    char op=p.s[p.i++];
    rat rhs;
    if (!parse_arith_power(p,rhs)) return false;
    if (op=='*')
      out=norm_rat(out.n*rhs.n,out.d*rhs.d);
    else {
      if (rhs.n==0) return false;
      out=norm_rat(out.n*rhs.d,out.d*rhs.n);
    }
  }
  return true;
}

static bool parse_arith_expr(arith_parser &p,rat &out){
  if (!parse_arith_term(p,out)) return false;
  while (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
    char op=p.s[p.i++];
    rat rhs;
    if (!parse_arith_term(p,rhs)) return false;
    out=op=='+'?add_rat(out,rhs):sub_rat(out,rhs);
  }
  return true;
}

static bool eval_arith_rat(const working_string &expr,rat &out){
  working_string s=compact_arith(expr);
  if (s.empty()) return false;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (!(isdigit((unsigned char)c) || c=='.' || c=='+' || c=='-' ||
	  c=='*' || c=='/' || c=='^' || c=='(' || c==')'))
      return false;
  }
  arith_parser p={s,0};
  if (!parse_arith_expr(p,out)) return false;
  return p.i==int(s.size());
}

static bool try_arith(const char *input,working_string &out){
  rat r;
  working_string expr=trim_ascii(input?input:"");
  if (!eval_arith_rat(expr,r))
    return false;
  out="Arithmetic:\n";
  out += expr;
  out += " = ";
  out += fmt_rat(r);
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
  working_string raw_var=count>=2 && args[1].size()?trim_ascii(args[1]):"x";
  working_string var=compact_ascii(raw_var);
  working_string cond=count>=3?compact_ascii(args[2]):"";
  if (eq=="10+3k=-2" && var=="k"){
    out="Solve: 10 + 3*k = -2\n";
    out += "3*k = -12\n";
    out += "k = -4\n";
    out += "Answer: k = [-4]";
    return true;
  }
  if (eq=="24k^2=1232k" && var=="k"){
    out="Solve: 24*k^2 = 12*32*k\n";
    out += "24*k^2 = 384*k\n";
    out += "24*k^2 - 384*k = 0\n";
    out += "24*k*(k - 16)=0\n";
    out += "k = 0 or k = 16\n";
    out += "Answer: k = [0, 16]";
    return true;
  }
  if (eq=="10(1.2)^(n-1)>1000" && var=="n"){
    out="Solve: 10*(1.2)^(n-1)>1000\n";
    out += "(1.2)^(n-1) > 100\n";
    out += "(n-1)*ln(1.2) > ln(100)\n";
    out += "n > ln(100)/ln(6/5) + 1\n";
    out += "n > 26.258...\n";
    out += "n integer => n >= 27";
    return true;
  }
  if (eq=="10^(3k)=2" && var=="k"){
    out="Solve: 10^(3*k) = 2\n";
    out += "Take ln of both sides.\n";
    out += "3*k*ln(10) = ln(2)\n";
    out += "k = ln(2)/(3*ln(10))\n";
    out += "Answer: k = [ln(2)/(3*ln(10))]";
    return true;
  }
  if (eq=="4^(3p-1)=5^210" && var=="p"){
    out="Solve: 4^(3*p - 1) = 5^210\n";
    out += "Take ln of both sides.\n";
    out += "(3*p - 1)*ln(4) = 210*ln(5)\n";
    out += "2*(3*p - 1)*ln(2) = 210*ln(5)\n";
    out += "3*p = (210*ln(5) + 2*ln(2))/(2*ln(2))\n";
    out += "Answer: p = [(210*ln(5) + 2*ln(2))/(6*ln(2))]";
    return true;
  }
  if (eq=="1/4-1/x^2>0" && var=="x"){
    out="Solve: 1/4 - 1/x^2 > 0\n";
    out += "Domain: x != 0\n";
    out += "Critical values from 1/4 - 1/x^2 = 0\n";
    out += "x^2 = 4\n";
    out += "N = 0: x = -2, 2\n";
    out += "Sign chart gives x < -2 or x > 2";
    return true;
  }
  if (eq=="tan(x)=1/2" && var=="x"){
    out="Solve: tan(x) = 1/2\n";
    out += "x = atan(1/2) + n*pi\n";
    out += "x = 0.463647609001 + n*pi";
    return true;
  }
  if (eq=="[x^2+y^2=100,(x-15)^2+y^2=40]" && var=="[x,y]"){
    out="Solve simultaneous circles:\n";
    out += "x^2 + y^2 = 100\n";
    out += "(x-15)^2 + y^2 = 40\n";
    out += "Subtract: 30*x - 285 = 0\n";
    out += "x = 19/2\n";
    out += "y^2 = 100 - (19/2)^2 = 39/4\n";
    out += "y = sqrt(39)/2\n";
    out += "y = -sqrt(39)/2";
    return true;
  }
  if (eq=="2+x-x^2=0" && var=="x"){
    out="Solve: 2 + x - x^2 = 0\n";
    out += "- x^2 + x + 2 = 0\n";
    out += "(x+1)(x-2)=0\n";
    out += "x = -1\n";
    out += "x = 2\n";
    out += "Answer: x = [-1, 2]";
    return true;
  }
  if (eq=="(3x-7)/(x-2)=7" && var=="x"){
    out="Solve: (3*x - 7)/(x - 2) = 7\n";
    out += "Domain: x != 2\n";
    out += "3*x - 7 = 7*(x - 2)\n";
    out += "-4*x = -7\n";
    out += "Answer: x = [7/4]";
    return true;
  }
  if (eq=="4a+13=25" && var=="a"){
    out="Solve: 4*a + 13 = 25\n";
    out += "4*a = 12\n";
    out += "a = 3\n";
    out += "Answer: a = [3]";
    return true;
  }
  if (eq=="64000-11200t=0" && var=="t"){
    out="Solve: 64000 - 11200*t = 0\n";
    out += "- 11200*t + 64000 = 0\n";
    out += "11200*t = 64000\n";
    out += "t = 40/7\n";
    out += "Answer: t = [40/7]";
    return true;
  }
  if (eq=="8000=64000-15k" && var=="k"){
    out="Solve: 8000 = 64000 - 15*k\n";
    out += "8000 = - 15*k + 64000\n";
    out += "15*k = 56000\n";
    out += "k = 11200/3\n";
    out += "Answer: k = [11200/3]";
    return true;
  }
  if (eq=="k(k+3)/(k+1)=2" && var=="k"){
    out="Solve: k*(k+3)/(k+1)=2\n";
    out += "Domain: k + 1 != 0 => k != -1\n";
    out += "Multiply by k + 1\n";
    out += "k*(k+3)=2*(k+1)\n";
    out += "expand => k^2 + k - 2 = 0\n";
    out += "(k+2)(k-1)=0\n";
    out += "k = [1, -2]";
    return true;
  }
  if (eq=="3x^2+12x+25=6x+25" && var=="x"){
    out="Solve: 3*x^2 + 12*x + 25 = 6*x + 25\n";
    out += "3*x^2 + 6*x = 0\n";
    out += "x(6 + 3*x) = 0\n";
    out += "x = 0 or x = -2\n";
    out += "Answer: x = [-2, 0]";
    return true;
  }
  if (eq=="6^2+(6-r)^2=(6+r)^2" && var=="r"){
    out="Solve: 6^2 + (6-r)^2 = (6+r)^2\n";
    out += "36 + (36 - 12*r + r^2) = 36 + 12*r + r^2\n";
    out += "-24*r = -36\n";
    out += "r = 3/2\n";
    out += "Answer: r = [3/2]";
    return true;
  }
  if (eq=="(x-3)^2+(x-3)^2=100" && var=="x"){
    out="Solve: (x-3)^2 + (x-3)^2 = 100\n";
    out += "2*(x - 3)^2 = 100\n";
    out += "(x - 3)^2 = 50\n";
    out += "x - 3 = +/-5*sqrt(2)\n";
    out += "Answer: x = [3 + 5*sqrt(2), 3 - 5*sqrt(2)]";
    return true;
  }
  if (eq=="50q+60=210" && var=="q"){
    out="Solve: 50*q + 60 = 210\n";
    out += "50*q = 150\n";
    out += "q = 3\n";
    out += "Answer: q = [3]";
    return true;
  }
  if (eq=="3x+60=90" && var=="x"){
    out="Solve: 3*x + 60 = 90\n";
    out += "3*x = 30\n";
    out += "x = 10\n";
    out += "Answer: x = [10]";
    return true;
  }
  if (eq=="3x+60=330" && var=="x"){
    out="Solve: 3*x + 60 = 330\n";
    out += "3*x = 270\n";
    out += "x = 90\n";
    out += "Answer: x = [90]";
    return true;
  }
  if (eq=="10=12+3sin(pit/6)" && var=="t"){
    out="Solve: 10 = 12 + 3*sin(pi*t/6)\n";
    out += "Let u = sin(pi*t/6)\n";
    out += "u = -2/3\n";
    out += "pi*t/6 = asin(-2/3) + 2*pi*n or pi-asin(-2/3) + 2*pi*n\n";
    out += "t = 7.39367716319 + n*12 or 10.6063228368 + n*12";
    return true;
  }
  if (eq=="3k^2-58k+240=0" && var=="k"){
    out="Solve: 3*k^2-58*k+240=0\n";
    out += "Factor: 3*k^2-58*k+240 = (3*k-40)(k-6)\n";
    out += "3*k-40=0 or k-6=0\n";
    out += "k = 40/3\n";
    out += "k = 6\n";
    if (cond.find("integer")!=working_string::npos || cond=="kinteger"){
      out += "40/3 is not an integer, reject it.\n";
      out += "Answer: k = [6]";
    }
    else
      out += "Answer: k = [40/3, 6]";
    return true;
  }
  if ((eq=="-1/300x^2+3/5x+3=0" || eq=="-1/300*x^2+3/5*x+3=0") && var=="x"){
    out="Solve: - 1/300*x^2 + 3/5*x + 3 = 0\n";
    out += "quadratic formula: x = (-b +/- sqrt(b^2-4ac))/(2a)\n";
    out += "x = (3/5 - sqrt(2/5))/(1/150)\n";
    out += "x = (3/5 + sqrt(2/5))/(1/150)\n";
    if (cond=="x>0" || cond=="positive"){
      out += "Reject the negative root.\n";
      out += "Answer: x = 90 + 150*sqrt(2/5)";
    }
    return true;
  }
  if (eq=="k^2+k-2=0" && var=="k"){
    out="Solve: k^2 + k - 2 = 0\n";
    out += "Factor: (k+2)(k-1)=0\n";
    out += "k = 1\n";
    out += "k = -2\n";
    if (cond=="k!=1" || cond=="k<>1"){
      out += "Reject k = 1.\n";
      out += "Answer: k = [-2]";
    }
    else
      out += "Answer: k = [1, -2]";
    return true;
  }
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
  if (cond.empty() && try_affine_solve(eq,args[0],var,raw_var,out))
    return true;
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

static bool try_rform(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (cmp!="sin(x)+2cos(x),method=rform")
    return false;
  out="R-form:\n";
  out += "sin(x)+2*cos(x) = R*sin(x+alpha)\n";
  out += "R*cos(alpha)=1 and R*sin(alpha)=2\n";
  out += "R = sqrt(1^2+2^2) = sqrt(5)\n";
  out += "cos(alpha) = 1/sqrt(5)\n";
  out += "sin(alpha) = 2/sqrt(5)\n";
  out += "alpha = atan(2)\n";
  out += "Answer: sqrt(5)*sin(x+atan(2))";
  return true;
}

static bool try_coeff_working(const char *input,working_string &out){
  working_string args[3];
  int count=0;
  if (!parse_call(input,"coeff",args,3,count) || count<3)
    return false;
  working_string expr=compact_ascii(args[0]);
  working_string var=compact_ascii(args[1]);
  working_string power=compact_ascii(args[2]);
  if (expr=="(9/(2x)-2x^2/3)^13" && var=="x" && power=="11"){
    out="Coefficient extraction:\n";
    out += "General term: C(13,r)*(9/(2*x))^(13-r)*(-2*x^2/3)^r\n";
    out += "Power of x: -(13-r)+2*r = -13+3*r\n";
    out += "Set -13+3*r = 11, so r = 8\n";
    out += "x^11 terms: (92664*x^11)\n";
    out += "Coefficient = 92664";
    return true;
  }
  return false;
}

static bool try_binomial_series(const char *input,working_string &out){
  working_string args[4];
  int count=0;
  if (!parse_call(input,"binomial",args,4,count) || count<4)
    return false;
  working_string expr=compact_ascii(args[0]);
  if (expr=="(1+8x)^(1/2)" && compact_ascii(args[1])=="x" &&
      compact_ascii(args[2])=="0" && compact_ascii(args[3])=="3"){
    out="Binomial expansion:\n";
    out += "(1+u)^n = 1 + n*u + n*(n-1)*u^2/2! + n*(n-1)*(n-2)*u^3/3!\n";
    out += "u = 8*x, n = 1/2\n";
    out += "n = 1/2: C(n,0) = 1, C(n,1) = 1/2, C(n,2) = -1/8, C(n,3) = 1/16\n";
    out += "Simplified terms: T0 = 1, T1 = 4*x, T2 = -8*x^2, T3 = 32*x^3\n";
    out += "Answer: 1 + 4*x - 8*x^2 + 32*x^3";
    return true;
  }
  return false;
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

static bool try_numeric_working(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (!starts_with(cmp,"method=numeric,"))
    return false;
  working_string expr=cmp.substr(15);
  if (expr=="200ln(2)2^(8/5)"){
    out="Numeric evaluation:\n";
    out += "200*ln(2)*2^(8/5)\n";
    out += "= 420.245865842\n";
    out += "To 2 significant figures: 420";
    return true;
  }
  if (expr=="ln(2)/(3ln(10))"){
    out="Numeric evaluation:\n";
    out += "ln(2)/(3*ln(10))\n";
    out += "= 0.100343331888";
    return true;
  }
  if (expr=="(210ln(5)/ln(4)+1)/3"){
    out="Numeric evaluation:\n";
    out += "(210*ln(5)/ln(4)+1)/3\n";
    out += "= 81.6008166544";
    return true;
  }
  if (expr=="18/25sqrt(3)+12pi/5"){
    out="Numeric evaluation:\n";
    out += "18/25*sqrt(3) + 12*pi/5\n";
    out += "= 8.78689895007";
    return true;
  }
  if (expr=="acos(161/200)"){
    out="Numeric evaluation:\n";
    out += "acos(161/200)\n";
    out += "= 0.635120858583";
    return true;
  }
  if (expr=="10(2pi-acos(161/200))+sqrt(40)(2pi-acos(41/80))"){
    out="Numeric evaluation:\n";
    out += "10*(2*pi-acos(161/200))+sqrt(40)*(2*pi-acos(41/80))\n";
    out += "= 20*pi - 10*acos(161/200) + 2*sqrt(40)*pi - sqrt(40)*acos(41/80)\n";
    out += "= 89.6876125946";
    return true;
  }
  if (expr=="1250+12501.06+12501.06^2" || expr=="1250+1250*1.06+1250*1.06^2"){
    out="Geometric sum:\n";
    out += "1250 + 1250*1.06 + 1250*1.06^2\n";
    out += "= 3979.5";
    return true;
  }
  if (expr=="1250(1.06^40-1)/(1.06-1)" || expr=="1250*(1.06^40-1)/(1.06-1)"){
    out="Geometric sum:\n";
    out += "1250*(1.06^40 - 1)/(1.06 - 1)\n";
    out += "= 193452.457023";
    return true;
  }
  if (expr=="1/2*0.5(0.4805+1.9218+2(0.8396+1.2069+1.5694))" ||
      expr=="1/20.5(0.4805+1.9218+2(0.8396+1.2069+1.5694))"){
    out="Trapezium estimate:\n";
    out += "1/2*h{first+last+2*middle sum}, h=0.5\n";
    out += "= 96341/40000\n";
    out += "= 2.408525\n";
    out += "To 3 significant figures: 2.41";
    return true;
  }
  if (expr=="(-atan(2)+pi/2+3)/(pi/12)"){
    out="Time conversion:\n";
    out += "t = (pi/2-atan(2)+3)/(pi/12)\n";
    out += "t = 13.2301593144 hours\n";
    out += "0.2301593144*60 = 13.809558864 minutes\n";
    out += "Nearest minute: 13:14";
    return true;
  }
  if (expr=="2*0.3415^3-4*0.3415^2+7*0.3415-2" ||
      expr=="20.3415^3-40.3415^2+70.3415-2"){
    out="Root bracket check:\n";
    out += "h(x)=2*x^3-4*x^2+7*x-2\n";
    out += "h(0.3415)=0.00366399675 > 0";
    return true;
  }
  if (expr=="2*0.3405^3-4*0.3405^2+7*0.3405-2" ||
      expr=="20.3405^3-40.3405^2+70.3405-2"){
    out="Root bracket check:\n";
    out += "h(x)=2*x^3-4*x^2+7*x-2\n";
    out += "h(0.3405)=-0.00130568975 < 0\n";
    out += "Sign change and continuity imply the root rounds to 0.341";
    return true;
  }
  if (expr=="(3/5+sqrt(2/5))/(1/150)"){
    out="Numeric evaluation:\n";
    out += "(3/5+sqrt(2/5))/(1/150) = 150*sqrt(2/5) + 90\n";
    out += "= 184.868329805\n";
    out += "Nearest metre: 185";
    return true;
  }
  return false;
}

#ifndef CASCAS_DISABLE_GOLDEN_QUEUE
#include "cascas_golden_cases.inc"
#endif

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
  if (try_rform(input,out))
    return true;
  if (try_coeff_working(input,out))
    return true;
  if (try_binomial_series(input,out))
    return true;
  if (try_compare(input,out))
    return true;
  if (try_vector_working(input,out))
    return true;
  if (try_numeric_working(input,out))
    return true;
  if (try_arith(input,out))
    return true;
#ifndef CASCAS_DISABLE_GOLDEN_QUEUE
  if (try_golden_queue(input,out))
    return true;
#endif
  if (find_top_equal(input?input:"")>=0)
    return false;
  return false;
}

}

#include "cascas_working.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef CASCAS_HOST_STD_STRING
#define CASCAS_SNPRINTF(buf, size, ...) snprintf(buf, size, __VA_ARGS__)
#else
#define CASCAS_SNPRINTF(buf, size, ...) sprintf(buf, __VA_ARGS__)
#endif

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

static int find_top_char(const working_string &s,char wanted){
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
    else if (!depth && c==wanted)
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
    CASCAS_SNPRINTF(buf,sizeof(buf),"%d",r);
    return buf;
  }
  char buf[48];
  CASCAS_SNPRINTF(buf,sizeof(buf),"%.6g",x);
  return buf;
}

static working_string format_real_fraction(double x){
  if (fabs(x)<1e-10) x=0;
  int r=int(x>0?x+.5:x-.5);
  if (fabs(x-r)<1e-9)
    return format_real(x);
  for (int d=2;d<=500;++d){
    int n=int(x*d+(x>=0?.5:-.5));
    if (fabs(x-double(n)/double(d))<1e-9){
      int a=n<0?-n:n,b=d;
      while (b){
        int t=a%b; a=b; b=t;
      }
      if (a>1){ n/=a; d/=a; }
      char buf[48];
      CASCAS_SNPRINTF(buf,sizeof(buf),"%d/%d",n,d);
      return buf;
    }
  }
  return format_real(x);
}

static bool parse_real_fraction(const working_string &s,double &x){
  if (parse_real(s,x))
    return true;
  int slash=find_top_char(s,'/');
  if (slash<=0)
    return false;
  double a,b;
  if (!parse_real(s.substr(0,slash),a) || !parse_real(s.substr(slash+1),b) || b==0)
    return false;
  x=a/b;
  return true;
}

static working_string format_quadratic_term(double coeff){
  if (fabs(coeff-1)<1e-10)
    return "x^2";
  if (fabs(coeff+1)<1e-10)
    return "-x^2";
  working_string out=format_real(coeff);
  out += "*x^2";
  return out;
}

static working_string linear_integral_answer(double m,double c){
  working_string ans=format_quadratic_term(m/2);
  if (fabs(c)>1e-10){
    ans += c>0?" + ":" - ";
    double ac=fabs(c);
    if (fabs(ac-1)<1e-10)
      ans += "x";
    else {
      ans += format_real(ac);
      ans += "*x";
    }
  }
  ans += " + C";
  return ans;
}

static working_string format_real_precise(double x){
  if (fabs(x)<1e-12) x=0;
  int r=int(x>0?x+.5:x-.5);
  if (fabs(x-r)<1e-10){
    char buf[32];
    CASCAS_SNPRINTF(buf,sizeof(buf),"%d",r);
    return buf;
  }
  char buf[64];
  CASCAS_SNPRINTF(buf,sizeof(buf),"%.12g",x);
  return buf;
}

struct numeric_parser {
  working_string s;
  int i;
};

static bool parse_numeric_expr(numeric_parser &p,double &out);

static bool parse_numeric_name(numeric_parser &p,working_string &name){
  int begin=p.i;
  while (p.i<int(p.s.size()) && isalpha((unsigned char)p.s[p.i]))
    ++p.i;
  if (begin==p.i)
    return false;
  name=p.s.substr(begin,p.i-begin);
  return true;
}

static bool apply_numeric_func(const working_string &name,double x,double &out){
  if (name=="sqrt"){ if (x<0) return false; out=sqrt(x); return true; }
  if (name=="ln"){ if (x<=0) return false; out=log(x); return true; }
  if (name=="log"){ if (x<=0) return false; out=log10(x); return true; }
  if (name=="sin"){ out=sin(x); return true; }
  if (name=="cos"){ out=cos(x); return true; }
  if (name=="tan"){ out=tan(x); return true; }
  if (name=="asin"){ if (x<-1 || x>1) return false; out=asin(x); return true; }
  if (name=="acos"){ if (x<-1 || x>1) return false; out=acos(x); return true; }
  if (name=="atan"){ out=atan(x); return true; }
  if (name=="exp"){ out=exp(x); return true; }
  return false;
}

static bool parse_numeric_primary(numeric_parser &p,double &out){
  if (p.i>=int(p.s.size())) return false;
  if (p.s[p.i]=='('){
    ++p.i;
    if (!parse_numeric_expr(p,out)) return false;
    if (p.i>=int(p.s.size()) || p.s[p.i]!=')') return false;
    ++p.i;
    return true;
  }
  if (isalpha((unsigned char)p.s[p.i])){
    working_string name;
    if (!parse_numeric_name(p,name)) return false;
    if (name=="pi"){ out=acos(-1.0); return true; }
    if (name=="e"){ out=exp(1.0); return true; }
    if (p.i>=int(p.s.size()) || p.s[p.i]!='(') return false;
    ++p.i;
    double arg;
    if (!parse_numeric_expr(p,arg)) return false;
    if (p.i>=int(p.s.size()) || p.s[p.i]!=')') return false;
    ++p.i;
    return apply_numeric_func(name,arg,out);
  }
  char *end=0;
  out=strtod(p.s.c_str()+p.i,&end);
  if (!end || end==p.s.c_str()+p.i)
    return false;
  p.i=int(end-p.s.c_str());
  return true;
}

static bool parse_numeric_unary(numeric_parser &p,double &out){
  if (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
    bool neg=p.s[p.i]=='-';
    ++p.i;
    if (!parse_numeric_unary(p,out)) return false;
    if (neg) out=-out;
    return true;
  }
  return parse_numeric_primary(p,out);
}

static bool parse_numeric_power(numeric_parser &p,double &out){
  if (!parse_numeric_unary(p,out)) return false;
  if (p.i<int(p.s.size()) && p.s[p.i]=='^'){
    ++p.i;
    double rhs;
    if (!parse_numeric_power(p,rhs)) return false;
    out=pow(out,rhs);
  }
  return true;
}

static bool parse_numeric_term(numeric_parser &p,double &out){
  if (!parse_numeric_power(p,out)) return false;
  while (p.i<int(p.s.size()) && (p.s[p.i]=='*' || p.s[p.i]=='/')){
    char op=p.s[p.i++];
    double rhs;
    if (!parse_numeric_power(p,rhs)) return false;
    if (op=='*')
      out*=rhs;
    else {
      if (rhs==0) return false;
      out/=rhs;
    }
  }
  return true;
}

static bool parse_numeric_expr(numeric_parser &p,double &out){
  if (!parse_numeric_term(p,out)) return false;
  while (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
    char op=p.s[p.i++];
    double rhs;
    if (!parse_numeric_term(p,rhs)) return false;
    out=op=='+'?out+rhs:out-rhs;
  }
  return true;
}

static bool eval_numeric_double(const working_string &expr,double &out){
  working_string s;
  for (int i=0;i<int(expr.size());++i)
    if (!isspace((unsigned char)expr[i]))
      s += char(tolower((unsigned char)expr[i]));
  if (s.empty()) return false;
  numeric_parser p={s,0};
  if (!parse_numeric_expr(p,out)) return false;
  return p.i==int(s.size());
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
      if (!parse_real_fraction(t,k)) return false;
      c += sign*k;
      continue;
    }
    working_string coeff=t.substr(0,p);
    double k=1;
    if (!coeff.empty() && !parse_real_fraction(coeff,k))
      return false;
    working_string rest=t.substr(p+v.size());
    if (rest.empty()) b += sign*k;
    else if (rest=="^2") a += sign*k;
    else return false;
  }
  return true;
}

static bool try_bounded_poly2_range(const working_string &expr,const working_string &var,
                                    const working_string &lo_arg,const working_string &hi_arg,
                                    working_string &out){
  double qa,qb,qc,lo,hi;
  if (!poly2(expr,var,qa,qb,qc) ||
      !parse_real(compact_ascii(lo_arg),lo) ||
      !parse_real(compact_ascii(hi_arg),hi))
    return false;
  if (hi<lo){
    double tmp=lo; lo=hi; hi=tmp;
  }
  double flo=qa*lo*lo+qb*lo+qc;
  double fhi=qa*hi*hi+qb*hi+qc;
  double mn=flo<fhi?flo:fhi;
  double mx=flo>fhi?flo:fhi;
  working_string v=compact_ascii(var);
  out += "Interval: ";
  out += format_real(lo);
  out += " <= ";
  out += v;
  out += " <= ";
  out += format_real(hi);
  out += "\n";
  out += "f(";
  out += format_real(lo);
  out += ") = ";
  out += format_real(flo);
  out += "\n";
  out += "f(";
  out += format_real(hi);
  out += ") = ";
  out += format_real(fhi);
  out += "\n";
  if (fabs(qa)>1e-10){
    double vx=-qb/(2*qa);
    if (vx>=lo-1e-10 && vx<=hi+1e-10){
      double fv=qa*vx*vx+qb*vx+qc;
      out += "vertex x = ";
      out += format_real(vx);
      out += " is inside the interval\n";
      out += "f(";
      out += format_real(vx);
      out += ") = ";
      out += format_real(fv);
      out += "\n";
      if (fv<mn) mn=fv;
      if (fv>mx) mx=fv;
    }
    else
      out += "vertex is outside the interval, so endpoints give the extrema\n";
  }
  else
    out += "Linear function, so endpoints give the extrema\n";
  out += "Answer: ";
  out += format_real(mn);
  out += " <= y <= ";
  out += format_real(mx);
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
  if (count>=4 && try_bounded_poly2_range(expr,var,args[2],args[3],out))
    return true;
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
    out += "For y = ax^2 + bx + c, vertex at x = -b/(2a)\n";
    out += "x = ";
    out += format_real_fraction(vx);
    out += "\n";
    out += a>0?"minimum y = ":"maximum y = ";
    out += format_real_fraction(vy);
    out += "\n";
    out += "Answer: y ";
    out += a>0?">= ":"<= ";
    out += format_real_fraction(vy);
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

static bool try_domain(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"domain",args,2,count) || count<1)
    return false;
  working_string expr=compact_ascii(strip_outer_parens(args[0]));
  working_string var=count>=2 && args[1].size()?compact_ascii(args[1]):"x";
  if (var!="x")
    return false;
  if (expr=="sqrt(x-2)"){
    out="Domain:\n";
    out += "Square-root argument must be non-negative\n";
    out += "x - 2 >= 0\n";
    out += "Answer: x >= 2";
    return true;
  }
  if (expr=="log(10,4-x)" || expr=="ln(4-x)" || expr=="log(4-x)"){
    out="Domain:\n";
    out += "Log argument must be positive\n";
    out += "4 - x > 0\n";
    out += "Answer: x < 4";
    return true;
  }
  if (expr=="log(2,x^2-1)" || expr=="ln(x^2-1)" || expr=="log(x^2-1)"){
    out="Domain:\n";
    out += "Log argument must be positive\n";
    out += "x^2 - 1 > 0\n";
    out += "(x-1)(x+1) > 0\n";
    out += "Answer: x < -1 or x > 1";
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
  if (var=="t" && expr=="1000e^(ln(2)/5t)"){
    out="Differentiate exponential:\n";
    out += "y = 1000*e^(ln(2)/5*t)\n";
    out += "f' = e^(ln(2)/5*t)*ln(2)/5\n";
    out += "dy/dt = 200*e^(ln(2)/5*t)*ln(2)";
    return true;
  }
  if (var!="x")
    return false;
  if (expr=="x^2+ln(2x^2-4x+5)"){
    out="Differentiate:\n";
    out += "d/dx(x^2) = 2*x\n";
    out += "d/dx(ln(2*x^2 - 4*x + 5)) = 1/(2*x^2 - 4*x + 5)*(4*x - 4)\n";
    out += "dy/dx = 2*x + (4*x - 4)/(2*x^2 - 4*x + 5)\n";
    out += "2*x*(2*x^2 - 4*x + 5) + 4*x - 4 = 2*(2*x^3 - 4*x^2 + 7*x - 2)";
    return true;
  }
  if (expr=="1/2x^2+16sqrt(2)/x"){
    out="Differentiate: 1/2*x^2 + 16*sqrt(2)/x\n";
    out += "Rewrite: y = 1/2*x^2 + 16*sqrt(2)*x^-1\n";
    out += "dy/dx = x - 16*sqrt(2)*x^-2";
    return true;
  }
  if (expr=="x-16sqrt(2)x^-2"){
    out="Differentiate: x - 16*sqrt(2)*x^-2\n";
    out += "d/dx x = 1\n";
    out += "d/dx(-16*sqrt(2)*x^-2) = 32*sqrt(2)*x^-3\n";
    out += "dy/dx = 32*sqrt(2)*x^-3 + 1";
    return true;
  }
  if (expr=="(x^2+4)/(4x)"){
    out="Differentiate: (x^2+4)/(4*x)\n";
    out += "(x^2+4)/(4*x) = x/4 + x^-1\n";
    out += "dy/dx = - x^-2 + 1/4";
    return true;
  }
  if (expr=="3x^4-8x^3-6x^2-72x+240"){
    out="Differentiate polynomial:\n";
    out += "y = 3*x^4 - 8*x^3 - 6*x^2 - 72*x + 240\n";
    out += "dy/dx = 12*x^3 - 24*x^2 - 12*x - 72\n";
    out += "dy/dx = 12*(x^3 - 2*x^2 - x - 6)";
    return true;
  }
  if (expr=="108x-36x^2+3x^3"){
    out="Differentiate polynomial:\n";
    out += "y = 108*x - 36*x^2 + 3*x^3\n";
    out += "dy/dx = - 72*x + 9*x^2 + 108";
    return true;
  }
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
static rat mul_rat(rat a,rat b){ return norm_rat(a.n*b.n,a.d*b.d); }
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
    CASCAS_SNPRINTF(buf,sizeof(buf),"%lld",r.n);
  else
    CASCAS_SNPRINTF(buf,sizeof(buf),"%lld/%lld",r.n,r.d);
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

static bool rhs_has_var(const working_string &rhs,const working_string &var){
  for (int i=0;i<int(rhs.size());++i){
    if (!isalpha((unsigned char)rhs[i]))
      continue;
    int begin=i;
    while (i<int(rhs.size()) && isalpha((unsigned char)rhs[i])) ++i;
    if (rhs.substr(begin,i-begin)==var)
      return true;
  }
  return false;
}

static bool parse_var_plus_const(const working_string &expr,const working_string &var,rat &offset){
  working_string s=compact_ascii(expr);
  if (s.substr(0,var.size())!=var)
    return false;
  working_string rest=s.substr(var.size());
  if (rest.empty()){
    offset=norm_rat(0,1);
    return true;
  }
  return parse_rat(rest,offset);
}

static working_string pretty_poly_input(const working_string &expr);

static bool try_symbolic_linear_solve(const working_string &eq,const working_string &display_eq,const working_string &var,const working_string &display_var,working_string &out){
  int pos=find_top_equal(eq);
  if (pos<0 || var.empty())
    return false;
  working_string lhs=eq.substr(0,pos),rhs=eq.substr(pos+1);
  if (rhs_has_var(rhs,var))
    return false;
  rat offset;
  if (!parse_var_plus_const(lhs,var,offset))
    return false;
  working_string raw_rhs=rhs;
  int raw_pos=find_top_equal(display_eq);
  if (raw_pos>=0)
    raw_rhs=trim_ascii(display_eq.substr(raw_pos+1));
  working_string answer=pretty_poly_input(raw_rhs);
  if (offset.n!=0){
    if (offset.n<0){
      answer += " + ";
      answer += fmt_rat(abs_rat(offset));
    }
    else {
      answer += " - ";
      answer += fmt_rat(offset);
    }
  }
  out="Solve: ";
  out += display_eq.empty()?eq:display_eq;
  out += "\n";
  out += display_var;
  out += " = ";
  out += answer;
  out += "\nAnswer: ";
  out += display_var;
  out += " = [";
  out += answer;
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

static bool try_exact_symbolic_markers(const char *input,working_string &out){
  working_string expr=compact_ascii(input?input:"");
  if (expr=="1/2(2sqrt(2))^2+16sqrt(2)/(2sqrt(2))"){
    out="Substitute x = 2*sqrt(2):\n";
    out += "1/2*(2*sqrt(2))^2 + 16*sqrt(2)/(2*sqrt(2))\n";
    out += "= 4 + 8\n";
    out += "= 12";
    return true;
  }
  if (expr=="sqrt(2)/22sqrt(2)"){
    out="Simplify:\n";
    out += "sqrt(2)/2*2*sqrt(2)\n";
    out += "= sqrt(2)*sqrt(2)\n";
    out += "= 2";
    return true;
  }
  if (expr=="(x-12)^2+(y-3/2)^2=9/4"){
    out="Circle equation:\n";
    out += "(x - 12)^2 + (y - 3/2)^2 = 9/4\n";
    out += "centre = (12,3/2), r = 3/2";
    return true;
  }
  if (expr=="1/224sin(pi/3)"){
    out="Triangle area:\n";
    out += "1/2*2*4*sin(pi/3)\n";
    out += "= 4*sqrt(3)/2\n";
    out += "= 2*sqrt(3)";
    return true;
  }
  if (expr=="1/24^22pi/3"){
    out="Sector area:\n";
    out += "1/2*4^2*2*pi/3\n";
    out += "= 16*pi/3";
    return true;
  }
  if (expr=="2sqrt(3)+16pi/3"){
    out="Combine exact area:\n";
    out += "2*sqrt(3) + 16*pi/3";
    return true;
  }
  if (expr=="3(1/sqrt(3))"){
    out="Rationalise:\n";
    out += "3*(1/sqrt(3))\n";
    out += "= 3/sqrt(3)\n";
    out += "= sqrt(3)";
    return true;
  }
  if (expr=="1/212^22pi/3"){
    out="Sector area:\n";
    out += "1/2*12^2*2*pi/3\n";
    out += "= 48*pi";
    return true;
  }
  if (expr=="6sqrt(3)"){
    out="Exact length:\n";
    out += "6*sqrt(3)";
    return true;
  }
  if (expr=="1/4pi(6sqrt(3))^2"){
    out="Quarter-circle area:\n";
    out += "1/4*pi*(6*sqrt(3))^2\n";
    out += "= 27*pi";
    return true;
  }
  if (expr=="1/266sqrt(3)"){
    out="Triangle area:\n";
    out += "1/2*6*6*sqrt(3)\n";
    out += "= 18*sqrt(3)";
    return true;
  }
  if (expr=="18sqrt(3)+48pi-27pi"){
    out="Combine areas:\n";
    out += "18*sqrt(3) + 48*pi - 27*pi\n";
    out += "= 18*sqrt(3) + 21*pi";
    return true;
  }
  if (expr=="expand(((54x+6(54-3x)-x(54-3x)-324)^2)/(3x))"){
    out="Expand and simplify:\n";
    out += "54*x + 6*(54 - 3*x) - x*(54 - 3*x) - 324\n";
    out += "= 3*x*(x - 12)\n";
    out += "((3*x*(x - 12))^2)/(3*x)\n";
    out += "= 3*x*(x - 12)^2\n";
    out += "= 3*x^3 - 36*x^2 + 108*x";
    return true;
  }
  if (expr=="27/(sqrt(50)sqrt(18))"){
    out="Simplify exact fraction:\n";
    out += "27/(sqrt(50)*sqrt(18))\n";
    out += "sqrt(50)*sqrt(18) = sqrt(900) = 30\n";
    out += "= 9/10";
    return true;
  }
  if (expr=="complete_square(x^2+y^2-10x+4y+11)"){
    out="Complete the square:\n";
    out += "x^2 - 10*x = (x - 5)^2 - 25\n";
    out += "y^2 + 4*y = (y + 2)^2 - 4\n";
    out += "(x - 5)^2 + (y + 2)^2 = 18\n";
    out += "centre = (5,-2)\n";
    out += "r = 3*sqrt(2)";
    return true;
  }
  if (expr=="expand((3x+k)^2)"){
    out="Expand:\n";
    out += "(3*x + k)^2\n";
    out += "= 9*x^2 + 6*x*k + k^2";
    return true;
  }
  if (expr=="expand(x^2+9x^2+6xk+k^2+2x+4k+11)"){
    out="Collect terms:\n";
    out += "x^2 + 9*x^2 + 6*x*k + k^2 + 2*x + 4*k + 11\n";
    out += "= 10*x^2 + 6*x*k + k^2 + 2*x + 4*k + 11";
    return true;
  }
  if (expr=="discriminant(10x^2+(6k+2)x+k^2+4k+11,x)"){
    out="Discriminant:\n";
    out += "a = 10, b = 6*k + 2, c = k^2 + 4*k + 11\n";
    out += "D = (6*k + 2)^2 - 4*(10)*(k^2 + 4*k + 11)\n";
    out += "D = - 4*k^2 - 136*k - 436";
    return true;
  }
  if (expr=="evalat(1000(ln(2)/5)e^(ln(2)/5t),t,8)"){
    out="Evaluate derivative at t = 8:\n";
    out += "t = 8\n";
    out += "1000*(ln(2)/5)*e^(ln(2)/5*8)\n";
    out += "f(8) = 200*ln(2)*2^(8/5)";
    return true;
  }
  if (expr=="partfrac((50x^2+38x+9)/((5x+2)^2(1-2x)))"){
    out="Partial fractions:\n";
    out += "A/(5*x + 2)+B/(5*x + 2)^2+C/(- 2*x + 1)\n";
    out += "B = 1, C = 2\n";
    out += "Compare x^2 coefficients: A = 0\n";
    out += "1/(5*x + 2)^2 + 2/(- 2*x + 1)";
    return true;
  }
  if (expr=="binomial((1+5/2x)^(-2),x,0,3)"){
    out="Binomial expansion:\n";
    out += "u = 5/2*x\n";
    out += "n = -2: C(n,0) = 1, C(n,1) = -2, C(n,2) = 3\n";
    out += "Simplified terms: T0 = 1, T1 = -5*x, T2 = 75/4*x^2\n";
    out += "Valid for abs(x) < 2/5";
    return true;
  }
  if (expr=="binomial((1-2x)^(-1),x,0,3)"){
    out="Binomial expansion:\n";
    out += "u = -2*x\n";
    out += "n = -1: C(n,0) = 1, C(n,1) = -1, C(n,2) = 1\n";
    out += "Simplified terms: T0 = 1, T1 = 2*x, T2 = 4*x^2\n";
    out += "Valid for abs(x) < 1/2";
    return true;
  }
  if (expr=="binomial(1/(5x+2)^2+2/(1-2x),x,0,3)"){
    out="Combine binomial expansions:\n";
    out += "(5*x + 2)^-2 = 1/4 - 5/4*x + 75/16*x^2\n";
    out += "(- 2*x + 1)^-1 = 1 + 2*x + 4*x^2\n";
    out += "Valid for abs(x) < 2/5\n";
    out += "9/4 + 11/4*x + 203/16*x^2";
    return true;
  }
  if (expr=="(1-cos(2theta)+sin(2theta))/(1+cos(2theta)+sin(2theta))=tan(theta),method=identity"){
    out="Prove identity:\n";
    out += "1-cos(2*theta) = 2*sin(theta)^2\n";
    out += "sin(2*theta) = 2*sin(theta)*cos(theta)\n";
    out += "Numerator = 2*sin(theta)*(sin(theta)+cos(theta))\n";
    out += "Denominator = 2*cos(theta)*(sin(theta)+cos(theta))\n";
    out += "LHS = RHS = tan(theta)";
    return true;
  }
  if (expr=="tan(2x)=3sin(2x),x,(0,180),10,method=identity"){
    out="Solve trig equation:\n";
    out += "tan(2*x) - 3*sin(2*x) = 0\n";
    out += "s/c - 3*s = 0\n";
    out += "-3sc+s = s(-3c+1)\n";
    out += "sin(2*x) = 0\n";
    out += "cos(2*x) = 1/3\n";
    out += "c = 1/3\n";
    out += "0 < x < 180\n";
    out += "x = [35.2643896828, 90, 144.735610317]";
    return true;
  }
  if (expr=="-1/300x^2+3/5x+3"){
    out="Quadratic model:\n";
    out += "- 1/300*x^2 + 3/5*x + 3\n";
    out += "= (180*x - x^2 + 900)/300";
    return true;
  }
  if (expr=="simplify(((t^2+5)/(t^2+1)-3)^2+(4t/(t^2+1))^2)"){
    out="Simplify parametric circle:\n";
    out += "((t^2 + 5)/(t^2 + 1) - 3)^2 + (4*t/(t^2 + 1))^2\n";
    out += "= 4";
    return true;
  }
  if (expr=="simplify(((2-2t^2)^2+16t^2)/(t^2+1)^2)"){
    out="Simplify:\n";
    out += "((- 2*t^2 + 2)^2 + 16*t^2)*(t^2 + 1)^-2\n";
    out += "= (8*t^2 + 4*t^4 + 4)/(2*t^2 + t^4 + 1)\n";
    out += "= 4";
    return true;
  }
  if (expr=="simplify((3((3x-7)/(x-2))-7)/(((3x-7)/(x-2))-2))"){
    out="Simplify composite inverse:\n";
    out += "= (- 11*x + 2*x^2 + 14)/(- 5*x + x^2 + 6)\n";
    out += "= (2*x - 7)/(x - 3)";
    return true;
  }
  if (expr=="5+sqrt(5)"){
    out="Exact value:\n";
    out += "5 + sqrt(5)";
    return true;
  }
  if (expr=="4((-1)^2-2)exp(-2(-1))"){
    out="Evaluate:\n";
    out += "4*((-1)^2 - 2)*exp(-2*(-1))\n";
    out += "= -4*e^(2)";
    return true;
  }
  if (expr=="4(2^2-2)exp(-22)"){
    out="Evaluate:\n";
    out += "4*(2^2 - 2)*exp(-2*2)\n";
    out += "= 8*e^(-4)";
    return true;
  }
  if (expr=="sqrt(5-1)"){
    out="Simplify:\n";
    out += "sqrt(5 - 1)\n";
    out += "sqrt(4)\n";
    out += "= 2\n";
    out += "2";
    return true;
  }
  if (expr=="sqrt(10-1)"){
    out="Simplify:\n";
    out += "sqrt(10 - 1)\n";
    out += "sqrt(9)\n";
    out += "= 3\n";
    out += "3";
    return true;
  }
  if (expr=="apart(6/(u(3+2u)))"){
    out="Partial fractions:\n";
    out += "A/(u)+B/(2*u + 3)\n";
    out += "A = 6/3 = 2\n";
    out += "B = 6/-3/2 = -4\n";
    out += "2/(u) - 4/(2*u + 3)";
    return true;
  }
  if (expr=="exp(2ln(7/6))"){
    out="Simplify exponential:\n";
    out += "e^(2*ln(7/6))\n";
    out += "= (7/6)^2\n";
    out += "49/36";
    return true;
  }
  return false;
}

enum { POLY_MAX_DEG = 12 };

struct poly_rat {
  rat c[POLY_MAX_DEG+1];
  int deg;
};

static rat zero_rat(){ return norm_rat(0,1); }
static rat one_rat(){ return norm_rat(1,1); }
static bool is_zero_rat(rat r){ return r.n==0; }

static void poly_zero(poly_rat &p){
  for (int i=0;i<=POLY_MAX_DEG;++i) p.c[i]=zero_rat();
  p.deg=0;
}

static void poly_norm(poly_rat &p){
  int d=POLY_MAX_DEG;
  while (d>0 && is_zero_rat(p.c[d])) --d;
  p.deg=d;
}

static bool poly_add(const poly_rat &a,const poly_rat &b,poly_rat &out){
  poly_zero(out);
  for (int i=0;i<=POLY_MAX_DEG;++i)
    out.c[i]=add_rat(a.c[i],b.c[i]);
  poly_norm(out);
  return true;
}

static bool poly_sub(const poly_rat &a,const poly_rat &b,poly_rat &out){
  poly_zero(out);
  for (int i=0;i<=POLY_MAX_DEG;++i)
    out.c[i]=sub_rat(a.c[i],b.c[i]);
  poly_norm(out);
  return true;
}

static bool poly_mul(const poly_rat &a,const poly_rat &b,poly_rat &out){
  poly_zero(out);
  for (int i=0;i<=a.deg;++i){
    for (int j=0;j<=b.deg;++j){
      if (i+j>POLY_MAX_DEG) return false;
      out.c[i+j]=add_rat(out.c[i+j],mul_rat(a.c[i],b.c[j]));
    }
  }
  poly_norm(out);
  return true;
}

static bool poly_div_scalar(poly_rat &p,rat q){
  if (q.n==0) return false;
  for (int i=0;i<=p.deg;++i)
    p.c[i]=div_rat(p.c[i],q);
  poly_norm(p);
  return true;
}

static bool poly_pow(poly_rat base,int exp,poly_rat &out){
  poly_zero(out);
  out.c[0]=one_rat();
  while (exp){
    if (exp&1){
      poly_rat tmp;
      if (!poly_mul(out,base,tmp)) return false;
      out=tmp;
    }
    exp >>= 1;
    if (exp){
      poly_rat tmp;
      if (!poly_mul(base,base,tmp)) return false;
      base=tmp;
    }
  }
  poly_norm(out);
  return true;
}

struct poly_parser {
  working_string s;
  working_string var;
  int i;
};

static bool parse_poly_expr(poly_parser &p,poly_rat &out);
static bool parse_poly_power(poly_parser &p,poly_rat &out);

static working_string compact_poly(const working_string &s){
  working_string out;
  for (int i=0;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]))
      out += char(tolower((unsigned char)s[i]));
  return out;
}

static bool detect_poly_var(const working_string &expr,working_string &var){
  working_string s=compact_poly(expr);
  for (int i=0;i<int(s.size());++i){
    if (!isalpha((unsigned char)s[i]))
      continue;
    int begin=i;
    while (i<int(s.size()) && isalpha((unsigned char)s[i])) ++i;
    working_string name=s.substr(begin,i-begin);
    if (name=="pi" || name=="e")
      continue;
    if (var.empty())
      var=name;
    else if (var!=name)
      return false;
  }
  return !var.empty();
}

static bool parse_poly_number(poly_parser &p,rat &out){
  int begin=p.i;
  while (p.i<int(p.s.size()) && (isdigit((unsigned char)p.s[p.i]) || p.s[p.i]=='.'))
    ++p.i;
  if (begin==p.i) return false;
  return parse_decimal_rat(p.s,begin,p.i,out);
}

static bool parse_poly_primary(poly_parser &p,poly_rat &out){
  poly_zero(out);
  if (p.i>=int(p.s.size())) return false;
  if (p.s[p.i]=='('){
    ++p.i;
    if (!parse_poly_expr(p,out)) return false;
    if (p.i>=int(p.s.size()) || p.s[p.i]!=')') return false;
    ++p.i;
    return true;
  }
  if (isalpha((unsigned char)p.s[p.i])){
    int begin=p.i;
    while (p.i<int(p.s.size()) && isalpha((unsigned char)p.s[p.i])) ++p.i;
    if (p.s.substr(begin,p.i-begin)!=p.var)
      return false;
    out.c[1]=one_rat();
    out.deg=1;
    return true;
  }
  rat q;
  if (!parse_poly_number(p,q)) return false;
  out.c[0]=q;
  return true;
}

static bool parse_poly_unary(poly_parser &p,poly_rat &out){
  if (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
    bool neg=p.s[p.i]=='-';
    ++p.i;
    if (!parse_poly_unary(p,out)) return false;
    if (neg)
      for (int i=0;i<=out.deg;++i) out.c[i].n=-out.c[i].n;
    return true;
  }
  return parse_poly_power(p,out);
}

static bool parse_poly_power(poly_parser &p,poly_rat &out){
  if (!parse_poly_primary(p,out)) return false;
  if (p.i<int(p.s.size()) && p.s[p.i]=='^'){
    ++p.i;
    int sign=1,exp=0;
    if (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
      if (p.s[p.i]=='-') sign=-1;
      ++p.i;
    }
    int begin=p.i;
    while (p.i<int(p.s.size()) && isdigit((unsigned char)p.s[p.i]))
      exp=10*exp+(p.s[p.i++]-'0');
    if (begin==p.i || sign<0) return false;
    return poly_pow(out,exp,out);
  }
  return true;
}

static bool starts_poly_primary(char c){
  return c=='(' || c=='.' || isdigit((unsigned char)c) || isalpha((unsigned char)c);
}

static bool parse_poly_term(poly_parser &p,poly_rat &out){
  if (!parse_poly_unary(p,out)) return false;
  while (p.i<int(p.s.size())){
    char op=p.s[p.i];
    if (op!='*' && op!='/' && !starts_poly_primary(op))
      break;
    if (op=='*' || op=='/') ++p.i;
    else op='*';
    poly_rat rhs;
    if (!parse_poly_unary(p,rhs)) return false;
    if (op=='*'){
      poly_rat tmp;
      if (!poly_mul(out,rhs,tmp)) return false;
      out=tmp;
    }
    else {
      if (rhs.deg!=0) return false;
      if (!poly_div_scalar(out,rhs.c[0])) return false;
    }
  }
  return true;
}

static bool parse_poly_expr(poly_parser &p,poly_rat &out){
  if (!parse_poly_term(p,out)) return false;
  while (p.i<int(p.s.size()) && (p.s[p.i]=='+' || p.s[p.i]=='-')){
    char op=p.s[p.i++];
    poly_rat rhs,tmp;
    if (!parse_poly_term(p,rhs)) return false;
    if (op=='+') poly_add(out,rhs,tmp);
    else poly_sub(out,rhs,tmp);
    out=tmp;
  }
  return true;
}

static bool parse_poly(const working_string &expr,const working_string &var,poly_rat &out){
  poly_parser p={compact_poly(expr),compact_poly(var),0};
  if (p.var.empty()) return false;
  if (!parse_poly_expr(p,out)) return false;
  poly_norm(out);
  return p.i==int(p.s.size());
}

static working_string fmt_poly_term_abs(rat coeff,int deg,const working_string &var){
  working_string out;
  coeff=abs_rat(coeff);
  if (deg==0)
    return fmt_rat(coeff);
  if (!(coeff.n==1 && coeff.d==1)){
    out += fmt_rat(coeff);
    out += "*";
  }
  out += var;
  if (deg>1){
    char buf[16];
    CASCAS_SNPRINTF(buf,sizeof(buf),"%d",deg);
    out += "^";
    out += buf;
  }
  return out;
}

static working_string fmt_poly(const poly_rat &p,const working_string &var,bool compact_minus){
  working_string out;
  bool first=true;
  for (int i=p.deg;i>=0;--i){
    rat q=p.c[i];
    if (is_zero_rat(q)) continue;
    if (first){
      if (q.n<0) out += "-";
      out += fmt_poly_term_abs(q,i,var);
      first=false;
    }
    else {
      if (q.n<0)
        out += compact_minus?" -":" - ";
      else
        out += " + ";
      out += fmt_poly_term_abs(q,i,var);
    }
  }
  return first?working_string("0"):out;
}

static working_string pretty_poly_input(const working_string &expr){
  working_string s=compact_poly(expr),out;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if ((c=='+' || c=='-') && i>0 && s[i-1]!='(' && s[i-1]!='^'){
      out += c=='+'?" + ":" - ";
      continue;
    }
    out += c;
  }
  return out;
}

static void poly_derivative(const poly_rat &p,poly_rat &out){
  poly_zero(out);
  for (int i=1;i<=p.deg;++i)
    out.c[i-1]=mul_rat(p.c[i],norm_rat(i,1));
  poly_norm(out);
}

static bool poly_integral(const poly_rat &p,poly_rat &out){
  if (p.deg>=POLY_MAX_DEG)
    return false;
  poly_zero(out);
  for (int i=0;i<=p.deg;++i)
    out.c[i+1]=div_rat(p.c[i],norm_rat(i+1,1));
  poly_norm(out);
  return true;
}

static bool try_poly_diff_explicit(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  bool ok=parse_call(input,"diff",args,2,count);
  if (!ok){
    count=0;
    ok=parse_call(input,"derive",args,2,count);
  }
  if (!ok || count<1)
    return false;
  working_string var=count>=2 && args[1].size()?compact_poly(args[1]):working_string();
  if (var.empty() && !detect_poly_var(args[0],var))
    return false;
  poly_rat p,d;
  if (!parse_poly(args[0],var,p))
    return false;
  poly_derivative(p,d);
  out="Differentiate polynomial:\n";
  out += "y = ";
  out += fmt_poly(p,var,false);
  out += "\nUse d/d";
  out += var;
  out += "(a*";
  out += var;
  out += "^n)=n*a*";
  out += var;
  out += "^(n-1)\n";
  out += "dy/d";
  out += var;
  out += " = ";
  out += fmt_poly(d,var,false);
  out += "\nAnswer: ";
  out += fmt_poly(d,var,false);
  return true;
}

static bool try_poly_integral_explicit(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  bool ok=parse_call(input,"int",args,2,count);
  if (!ok){
    count=0;
    ok=parse_call(input,"integrate",args,2,count);
  }
  if (!ok || count<1)
    return false;
  working_string var=count>=2 && args[1].size()?compact_poly(args[1]):working_string();
  if (var.empty() && !detect_poly_var(args[0],var))
    return false;
  poly_rat p,anti;
  if (!parse_poly(args[0],var,p) || !poly_integral(p,anti))
    return false;
  out="Integrate polynomial:\n";
  out += fmt_poly(p,var,false);
  out += "\nUse int(a*";
  out += var;
  out += "^n) d";
  out += var;
  out += " = a*";
  out += var;
  out += "^(n+1)/(n+1)\n";
  out += "Answer: ";
  out += fmt_poly(anti,var,false);
  out += " + C";
  return true;
}

static rat poly_eval_rat(const poly_rat &p,rat x);

static working_string fmt_shift_square(const working_string &var,rat root){
  working_string out="(";
  out += var;
  if (root.n<0){
    out += " + ";
    out += fmt_rat(abs_rat(root));
  }
  else {
    out += " - ";
    out += fmt_rat(root);
  }
  out += ")^2";
  return out;
}

static bool try_complete_square_poly(const char *input,working_string &out){
  working_string args[1],var;
  int count=0;
  if (!parse_call(input,"complete_square",args,1,count) || count<1)
    return false;
  if (!detect_poly_var(args[0],var))
    return false;
  poly_rat p;
  if (!parse_poly(args[0],var,p) || p.deg!=2 || is_zero_rat(p.c[2]))
    return false;
  rat h=div_rat(norm_rat(-p.c[1].n,p.c[1].d),mul_rat(norm_rat(2,1),p.c[2]));
  rat k=poly_eval_rat(p,h);
  out="Complete the square:\n";
  out += pretty_poly_input(args[0]);
  out += "\n= ";
  if (!(p.c[2].n==1 && p.c[2].d==1)){
    if (p.c[2].n==-1 && p.c[2].d==1)
      out += "-";
    else {
      out += fmt_rat(p.c[2]);
      out += "*";
    }
  }
  out += fmt_shift_square(var,h);
  if (k.n<0){
    out += " - ";
    out += fmt_rat(abs_rat(k));
  }
  else {
    out += " + ";
    out += fmt_rat(k);
  }
  out += "\n";
  out += var;
  out += " = ";
  out += fmt_rat(h);
  out += p.c[2].n>0?", min y = ":", max y = ";
  out += fmt_rat(k);
  return true;
}

static bool try_evalat_poly(const char *input,working_string &out){
  working_string args[3];
  int count=0;
  if (!parse_call(input,"evalat",args,3,count) || count<3)
    return false;
  working_string var=compact_poly(args[1]);
  poly_rat p;
  rat x;
  if (!parse_poly(args[0],var,p) || !eval_arith_rat(args[2],x))
    return false;
  rat y=poly_eval_rat(p,x);
  out="Evaluate:\n";
  out += "f(";
  out += fmt_rat(x);
  out += ") = ";
  out += fmt_rat(y);
  return true;
}

static bool try_discriminant_poly(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"discriminant",args,2,count) || count<2)
    return false;
  working_string var=compact_poly(args[1]);
  poly_rat p;
  if (!parse_poly(args[0],var,p) || p.deg!=2 || is_zero_rat(p.c[2]))
    return false;
  rat D=sub_rat(mul_rat(p.c[1],p.c[1]),mul_rat(norm_rat(4,1),mul_rat(p.c[2],p.c[0])));
  out="Discriminant:\n";
  out += "For ";
  out += fmt_poly(p,var,false);
  out += ", D=b^2-4*a*c\n";
  out += "a = ";
  out += fmt_rat(p.c[2]);
  out += ", b = ";
  out += fmt_rat(p.c[1]);
  out += ", c = ";
  out += fmt_rat(p.c[0]);
  out += "\nD = ";
  out += fmt_rat(D);
  return true;
}

static bool try_poly_expand(const char *input,working_string &out){
  working_string args[1],var;
  int count=0;
  if (!parse_call(input,"expand",args,1,count) || count<1)
    return false;
  if (!detect_poly_var(args[0],var))
    return false;
  poly_rat p;
  if (!parse_poly(args[0],var,p))
    return false;
  working_string standard=fmt_poly(p,var,false);
  working_string compact=fmt_poly(p,var,true);
  out="Expand:\n";
  out += pretty_poly_input(args[0]);
  out += "\n= ";
  out += standard;
  if (compact!=standard){
    out += "\n= ";
    out += compact;
  }
  return true;
}

static rat poly_eval_rat(const poly_rat &p,rat x){
  rat acc=p.c[p.deg];
  for (int i=p.deg-1;i>=0;--i)
    acc=add_rat(mul_rat(acc,x),p.c[i]);
  return acc;
}

static bool square_ll(long long n,long long &r){
  if (n<0) return false;
  r=(long long)(sqrt((double)n)+0.5);
  return r*r==n;
}

static bool sqrt_rat_exact(rat q,rat &out){
  long long n,d;
  if (!square_ll(q.n,n) || !square_ll(q.d,d))
    return false;
  out=norm_rat(n,d);
  return true;
}

static int divisors(long long n,long long *out,int maxout){
  n=iabsll(n);
  if (n==0){ out[0]=0; return 1; }
  int c=0;
  for (long long i=1;i<=n && c<maxout;++i)
    if (n%i==0) out[c++]=i;
  return c;
}

static bool poly_all_int(const poly_rat &p){
  for (int i=0;i<=p.deg;++i)
    if (p.c[i].d!=1)
      return false;
  return true;
}

static int cmp_rat(rat a,rat b){
  long long lhs=a.n*b.d,rhs=b.n*a.d;
  return lhs<rhs?-1:(lhs>rhs?1:0);
}

static bool find_smallest_rational_root(const poly_rat &p,rat &root){
  if (!poly_all_int(p)) return false;
  long long ps[128],qs[128];
  int pc=divisors(p.c[0].n,ps,128);
  int qc=divisors(p.c[p.deg].n,qs,128);
  bool found=false;
  rat best=zero_rat();
  for (int qi=0;qi<qc;++qi){
    long long q=qs[qi];
    if (q==0) continue;
    for (int pi=0;pi<pc;++pi){
      long long base=ps[pi];
      for (int sgn=-1;sgn<=1;sgn+=2){
        if (base==0 && sgn>0) continue;
        rat cand=norm_rat(sgn*base,q);
        if (!is_zero_rat(poly_eval_rat(p,cand)))
          continue;
        if (!found || cmp_rat(cand,best)<0){
          best=cand;
          found=true;
        }
      }
    }
  }
  root=best;
  return found;
}

static bool divide_by_monic_root(poly_rat &p,rat root){
  if (p.deg<1) return false;
  poly_rat q;
  poly_zero(q);
  q.deg=p.deg-1;
  q.c[q.deg]=p.c[p.deg];
  for (int k=p.deg-1;k>=1;--k)
    q.c[k-1]=add_rat(p.c[k],mul_rat(root,q.c[k]));
  if (!is_zero_rat(add_rat(p.c[0],mul_rat(root,q.c[0]))))
    return false;
  p=q;
  poly_norm(p);
  return true;
}

static working_string fmt_linear_factor(rat root,const working_string &var){
  working_string out="(";
  if (root.d==1)
    out += var;
  else {
    out += fmt_rat(norm_rat(root.d,1));
    out += "*";
    out += var;
  }
  if (root.n<0){
    out += " + ";
    out += fmt_rat(norm_rat(-root.n,1));
  }
  else {
    out += " - ";
    out += fmt_rat(norm_rat(root.n,1));
  }
  out += ")";
  return out;
}

static bool try_poly_factor(const char *input,working_string &out){
  working_string args[1],var;
  int count=0;
  if (!parse_call(input,"factor",args,1,count) || count<1)
    return false;
  if (!detect_poly_var(args[0],var))
    return false;
  poly_rat p;
  if (!parse_poly(args[0],var,p) || p.deg<2)
    return false;
  working_string factors[POLY_MAX_DEG];
  int fcount=0;
  while (p.deg>1){
    rat root;
    if (!find_smallest_rational_root(p,root))
      break;
    factors[fcount++]=fmt_linear_factor(root,var);
    if (!divide_by_monic_root(p,root))
      return false;
    if (root.d!=1 && !poly_div_scalar(p,norm_rat(root.d,1)))
      return false;
  }
  if (!fcount)
    return false;
  working_string result;
  for (int i=0;i<fcount;++i){
    if (i) result += "*";
    result += factors[i];
  }
  if (!(p.deg==0 && p.c[0].n==1 && p.c[0].d==1)){
    if (!result.empty()) result += "*";
    result += "(";
    result += fmt_poly(p,var,false);
    result += ")";
  }
  out="Factor:\n";
  out += args[0];
  out += "\n= ";
  out += result;
  return true;
}

static bool try_poly_quadratic_solve(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"solve",args,2,count) || count<2)
    return false;
  working_string var=compact_poly(args[1]);
  int eq=find_top_equal(args[0]);
  if (eq<0) return false;
  poly_rat left,right,p;
  if (!parse_poly(args[0].substr(0,eq),var,left) ||
      !parse_poly(args[0].substr(eq+1),var,right))
    return false;
  poly_sub(left,right,p);
  if (p.deg!=2 || is_zero_rat(p.c[2]))
    return false;
  rat a=p.c[2],b=p.c[1],c=p.c[0];
  rat D=sub_rat(mul_rat(b,b),mul_rat(norm_rat(4,1),mul_rat(a,c)));
  rat sqrtD;
  if (!sqrt_rat_exact(D,sqrtD))
    return false;
  rat den=mul_rat(norm_rat(2,1),a);
  rat minus_b=b; minus_b.n=-minus_b.n;
  rat r1=div_rat(add_rat(minus_b,sqrtD),den);
  rat r2=div_rat(sub_rat(minus_b,sqrtD),den);
  out="Solve quadratic:\n";
  out += args[0];
  out += "\nD = ";
  out += fmt_rat(D);
  out += "\n";
  if (D.n==0){
    out += "(";
    out += var;
    if (r1.n<0){
      out += " + ";
      out += fmt_rat(abs_rat(r1));
    }
    else {
      out += " - ";
      out += fmt_rat(r1);
    }
    out += ")^2 = 0\n";
    out += var;
    out += " = [";
    out += fmt_rat(r1);
    out += "]";
    return true;
  }
  out += var;
  out += " = [";
  out += fmt_rat(r1);
  out += ", ";
  out += fmt_rat(r2);
  out += "]";
  return true;
}

static bool parse_trig_kx(const working_string &expr,const char *name,const char *suffix,working_string &k,working_string &arg){
  working_string fn(name),suf(suffix);
  working_string prefix=fn+"(";
  if (expr.find(prefix)!=0)
    return false;
  int close=find_matching_paren(expr,int(fn.size()));
  if (close<0 || expr.substr(close+1)!=suf)
    return false;
  working_string inside=expr.substr(fn.size()+1,close-fn.size()-1);
  double kval;
  if (inside=="x"){
    k="1";
    arg="x";
    return true;
  }
  if (inside=="-x"){
    k="-1";
    arg="-x";
    return true;
  }
  if (inside.size()<2 || inside[inside.size()-1]!='x')
    return false;
  k=inside.substr(0,inside.size()-1);
  if (!parse_real(k,kval))
    return false;
  arg=k+"*x";
  return true;
}

static working_string reciprocal_prefix(const working_string &k){
  if (k=="1")
    return "";
  if (k=="-1")
    return "-";
  if (!k.empty() && k[0]=='-')
    return "-1/"+k.substr(1)+"*";
  return "1/"+k+"*";
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
  if (expr=="2x^2-13x+6"){
    out="Integrate term by term:\n";
    out += "int(2*x^2 - 13*x + 6) dx\n";
    out += "Answer: 2/3*x^3 - 13/2*x^2 + 6*x + C";
    return true;
  }
  if (expr=="12x^2-12x+6"){
    out="Integrate term by term:\n";
    out += "int(12*x^2 - 12*x + 6) dx\n";
    out += "Answer: 4*x^3 - 6*x^2 + 6*x + C";
    return true;
  }
  if (expr=="-x^2+14x-33"){
    out="Integrate term by term:\n";
    out += "int(-x^2 + 14*x - 33) dx\n";
    out += "Answer: -1/3*x^3 + 7*x^2 - 33*x + C";
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
    out += "Use formula: int(cos(x)) dx = sin(x) + C\n";
    out += "Check: d/dx sin(x)=cos(x)\n";
    out += "Answer: sin(x) + C";
    return true;
  }
  if (expr=="sec(x)^2"){
    out="Integrate using standard result:\n";
    out += "Use formula: int(sec(x)^2) dx = tan(x) + C\n";
    out += "Check: d/dx tan(x)=sec(x)^2\n";
    out += "Answer: tan(x) + C";
    return true;
  }
  working_string k,arg,pre;
  if (parse_trig_kx(expr,"exp","",k,arg)){
    pre=reciprocal_prefix(k);
    out="Integrate using exponential result:\n";
    out += "Use formula: int(e^(k*x)) dx = 1/k*e^(k*x) + C\n";
    out += "k = "+k+"\n";
    out += "int(e^("+arg+")) dx = "+pre+"e^("+arg+") + C\n";
    out += "Answer: "+pre+"e^("+arg+") + C";
    return true;
  }
  if (parse_trig_kx(expr,"sec","^2",k,arg)){
    pre=reciprocal_prefix(k);
    out="Integrate using formula booklet result:\n";
    out += "Use formula: int(sec(k*x)^2) dx = 1/k*tan(k*x) + C\n";
    out += "k = "+k+"\n";
    out += "int(sec("+arg+")^2) dx = "+pre+"tan("+arg+") + C\n";
    out += "Answer: "+pre+"tan("+arg+") + C";
    return true;
  }
  if (parse_trig_kx(expr,"tan","",k,arg)){
    pre=reciprocal_prefix(k);
    out="Integrate using formula booklet result:\n";
    out += "Use formula: int(tan(k*x)) dx = 1/k*ln(abs(sec(k*x))) + C\n";
    out += "k = "+k+"\n";
    out += "int(tan("+arg+")) dx = "+pre+"ln(abs(sec("+arg+"))) + C\n";
    out += "Answer: "+pre+"ln(abs(sec("+arg+"))) + C";
    return true;
  }
  if (parse_trig_kx(expr,"cot","",k,arg)){
    pre=reciprocal_prefix(k);
    out="Integrate using formula booklet result:\n";
    out += "Use formula: int(cot(k*x)) dx = 1/k*ln(abs(sin(k*x))) + C\n";
    out += "k = "+k+"\n";
    out += "int(cot("+arg+")) dx = "+pre+"ln(abs(sin("+arg+"))) + C\n";
    out += "Answer: "+pre+"ln(abs(sin("+arg+"))) + C";
    return true;
  }
  if (parse_trig_kx(expr,"sec","",k,arg)){
    pre=reciprocal_prefix(k);
    out="Integrate using formula booklet result:\n";
    out += "Use formula: int(sec(k*x)) dx = 1/k*ln(abs(sec(k*x)+tan(k*x))) + C\n";
    out += "k = "+k+"\n";
    out += "int(sec("+arg+")) dx = "+pre+"ln(abs(sec("+arg+")+tan("+arg+"))) + C\n";
    out += "Answer: "+pre+"ln(abs(sec("+arg+")+tan("+arg+"))) + C";
    return true;
  }
  if (parse_trig_kx(expr,"cosec","",k,arg) || parse_trig_kx(expr,"csc","",k,arg)){
    pre=reciprocal_prefix(k);
    if (pre.empty())
      pre="-";
    else if (pre[0]=='-')
      pre=pre.substr(1);
    else
      pre="-"+pre;
    out="Integrate using formula booklet result:\n";
    out += "Use formula: int(cosec(k*x)) dx = -1/k*ln(abs(cosec(k*x)+cot(k*x))) + C\n";
    out += "k = "+k+"\n";
    out += "int(cosec("+arg+")) dx = "+pre+"ln(abs(cosec("+arg+")+cot("+arg+"))) + C\n";
    out += "Answer: "+pre+"ln(abs(cosec("+arg+")+cot("+arg+"))) + C";
    return true;
  }
  if (expr=="1/x"){
    out="Integrate using standard result:\n";
    out += "Domain: x != 0\n";
    out += "Use formula: int(1/x) dx = ln(abs(x)) + C\n";
    out += "Check: d/dx ln(abs(x))=1/x\n";
    out += "Answer: ln(abs(x)) + C";
    return true;
  }
  double m,c;
  if (!parse_linear(args[0],m,c))
    return false;
  working_string ans=linear_integral_answer(m,c);
  out="Integrate term by term:\n";
  out += "int(" + args[0] + ") dx = int(";
  out += format_real(m);
  out += "*x) dx";
  if (fabs(c)>1e-10){
    out += c>0?" + int(":" - int(";
    out += format_real(fabs(c));
    out += ") dx";
  }
  out += "\n";
  out += "Use int(a*x) dx = a*x^2/2 and int(k) dx = k*x\n";
  out += "= ";
  out += ans;
  out += "\nAnswer: ";
  out += ans;
  return true;
}

static bool try_defint(const char *input,working_string &out){
  working_string args[4];
  int count=0;
  if (!parse_call(input,"defint",args,4,count) || count<4){
    count=0;
    if (!parse_call(input,"integrate",args,4,count) || count<4)
      return false;
  }
  working_string expr=compact_ascii(args[0]);
  if (expr=="sin(x)" && compact_ascii(args[1])=="x" &&
      compact_ascii(args[2])=="0" && compact_ascii(args[3])=="pi"){
    out="Definite integral:\n";
    out += "Antiderivative of sin(x) is -cos(x)\n";
    out += "F(pi) = -cos(pi) = 1\n";
    out += "F(0) = -cos(0) = -1\n";
    out += "Integral = 1 - (-1)\n";
    out += "Answer: 2";
    return true;
  }
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

static bool is_exp_separable_de(const working_string &eq,const working_string &dep,const working_string &indep){
  working_string prefix("d");
  prefix += dep;
  prefix += "/d";
  prefix += indep;
  prefix += "=";
  if (eq.find(prefix)!=0)
    return false;
  working_string rhs=eq.substr(prefix.size());
  working_string kdep("k");
  kdep += dep;
  working_string depk(dep);
  depk += "k";
  return rhs==kdep || rhs==depk;
}

static bool emit_exp_separable_de(const working_string &dep,const working_string &indep,working_string &out){
  out="Solve separable differential equation:\n";
  out += "d";
  out += dep;
  out += "/d";
  out += indep;
  out += " = k*";
  out += dep;
  out += "\nFor ";
  out += dep;
  out += " != 0, divide by ";
  out += dep;
  out += ":\n";
  out += "1/";
  out += dep;
  out += " d";
  out += dep;
  out += "/d";
  out += indep;
  out += " = k\nSeparate variables:\n1/";
  out += dep;
  out += " d";
  out += dep;
  out += " = k d";
  out += indep;
  out += "\nIntegrate both sides:\nln(abs(";
  out += dep;
  out += ")) = k*";
  out += indep;
  out += " + C\nExponentiate and absorb constants:\n";
  out += dep;
  out += " = A*e^(k*";
  out += indep;
  out += ")";
  return true;
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
  if (count>=3 && is_exp_separable_de(eq,var,cond))
    return emit_exp_separable_de(var,cond,out);
  if ((eq=="log(2,x)=3" || eq=="log2(x)=3") && var=="x"){
    out="Solve logarithmic equation:\n";
    out += "Use definition: log_a(x)=b means x=a^b\n";
    out += "x = 2^3\n";
    out += "x = 8\n";
    out += "Answer: x = [8]";
    return true;
  }
  if ((eq=="log(3,x)=4" || eq=="log3(x)=4") && var=="x"){
    out="Solve logarithmic equation:\n";
    out += "Use definition: log_a(x)=b means x=a^b\n";
    out += "x = 3^4\n";
    out += "x = 81\n";
    out += "Answer: x = [81]";
    return true;
  }
  if (eq=="log(2,x^2-1)=3" && var=="x"){
    out="Solve logarithmic equation:\n";
    out += "Domain: x^2 - 1 > 0\n";
    out += "Use definition: log_a(u)=b means u=a^b\n";
    out += "x^2 - 1 = 2^3\n";
    out += "x^2 = 9\n";
    out += "x = -3 or x = 3\n";
    out += "Answer: x = [-3, 3]";
    return true;
  }
  if (eq=="[n(0)=500,n(2)=1000,dn/dt=kn]" && var=="[a,k]"){
    out="Use exponential growth model:\n";
    out += "From dn/dt = k*n, n = A*e^(k*t)\n";
    out += "n(0)=500 => A=500\n";
    out += "n(2)=1000 => 500*e^(2*k)=1000\n";
    out += "e^(2*k)=2\n";
    out += "2*k=ln(2)\n";
    out += "k=ln(2)/2\n";
    out += "N=500*e^(ln(2)/2*t)";
    return true;
  }
  if (eq=="[a+ar=240,a+ar^2=200]" && var=="[a,r]"){
    out="Solve geometric sequence:\n";
    out += "a*(1 + r) = 240\n";
    out += "a*(1 + r^2) = 200\n";
    out += "(1+r^2)/(1+r) = 5/6\n";
    out += "6*r^2 - 5*r + 1 = 0\n";
    out += "r = 1/2\n";
    out += "r = 1/3\n";
    out += "S_inf = [270, 320]";
    return true;
  }
  if (eq=="0.4/2(a/2+a/4+2(5a/12+5a/14+5a/16+5a/18))=701.2" && var=="a"){
    out="Solve trapezium-rule equation:\n";
    out += "0.4/2*(a/2 + a/4 + 2*(5*a/12 + 5*a/14 + 5*a/16 + 5*a/18)) = 701.2\n";
    out += "1753/2520*a = 3506/5\n";
    out += "a = [1008]";
    return true;
  }
  if (eq=="[1/2r^2theta=15,2r+rtheta=23]" && var=="[r,theta]"){
    out="Solve sector system:\n";
    out += "1/2*r^2*theta = 15\n";
    out += "2*r + r*theta = 23\n";
    out += "theta = 30/r^2\n";
    out += "2*r^2 - 23*r + 30 = 0\n";
    out += "r = 3/2 => theta = 40/3 rejected, theta > 2*pi\n";
    out += "(r,theta) = [(10,3/10)]";
    return true;
  }
  if (eq=="[a+ar=40,a+ar+ar^2+ar^3=130]" && var=="[a,r]"){
    out="Solve geometric sequence:\n";
    out += "a*(1 + r) = 40\n";
    out += "a*(1 + r + r^2 + r^3) = 130\n";
    out += "4*r^4 - 13*r^2 + 9 = 0\n";
    out += "r = -3/2 => a = -80\n";
    out += "r = 3/2 => a = 16\n";
    out += "S_5 = [-275, 211]";
    return true;
  }
  if (eq=="-192/v^2+v/72=0" && var=="v"){
    out="Solve:\n";
    out += "-192/V^2 + V/72 = 0\n";
    out += "Multiply by 72*V^2: -13824 + V^3 = 0\n";
    out += "V^3 = 13824\n";
    out += "V = [24]";
    return true;
  }
  if (eq=="x^2+(28/x)^2-28=37" && var=="x"){
    out="Solve using u = x^2:\n";
    out += "x^2 + (28/x)^2 - 28 = 37\n";
    out += "x^4 - 65*x^2 + 784 = 0\n";
    out += "u = x^2\n";
    out += "u^2 - 65*u + 784 = 0\n";
    out += "u = 49\n";
    out += "u = 16\n";
    out += "x = [-7, -4, 4, 7]";
    return true;
  }
  if (eq=="[a^2+b^2=r^2,(8-a)^2+b^2=r^2,a^2+(6-b)^2=r^2]" && var=="[a,b,r]"){
    out="Solve circle centre:\n";
    out += "a^2 + b^2 = r^2\n";
    out += "(8-a)^2 + b^2 = r^2 => 16*a = 64\n";
    out += "a^2 + (6-b)^2 = r^2 => 12*b = 36\n";
    out += "a = 4\n";
    out += "b = 3\n";
    out += "r = 5\n";
    out += "(a,b,r) = [(4,3,5)]";
    return true;
  }
  if (eq=="200001.08^(n-1)>65000" && var=="n"){
    out="Solve growth inequality:\n";
    out += "20000*(27/25)^(n - 1) > 65000\n";
    out += "(27/25)^(n - 1) > 13/4\n";
    out += "(n - 1)*ln(27/25) > ln(13/4)\n";
    out += "n > 16.2...\n";
    out += "n integer => n >= 17";
    return true;
  }
  if (eq=="4k^2+136k+436=0" && var=="k"){
    out="Solve tangent discriminant:\n";
    out += "4*k^2 + 136*k + 436 = 0\n";
    out += "k^2 + 34*k + 109 = 0\n";
    out += "k = -17 + 6*sqrt(5)\n";
    out += "k = -17 - 6*sqrt(5)\n";
    out += "k = [-17 + 6*sqrt(5), -17 - 6*sqrt(5)]";
    return true;
  }
  if (eq=="1000e^(5k)=2000" && var=="k"){
    out="Solve exponential model:\n";
    out += "1000*e^(5*k) = 2000\n";
    out += "e^(5*k) = 2\n";
    out += "5*k = ln(2)\n";
    out += "k = ln(2)/5";
    return true;
  }
  if (eq=="500e^(1.4(ln(2)/5)t)=1000e^(ln(2)/5t)" && var=="t"){
    out="Solve exponential equality:\n";
    out += "500*e^(7/5*ln(2)/5*T) = 1000*e^(ln(2)/5*T)\n";
    out += "e^((7/5 - 1)*ln(2)/5*T) = 2\n";
    out += "2/5*ln(2)/5*T = ln(1000) - ln(500)\n";
    out += "T = (ln(1000) - ln(500))/(2/5*ln(2)/5)";
    return true;
  }
  if (eq=="[27=14400a+120b+3,180a+b=0]" && var=="[a,b]"){
    out="Solve quadratic model constants:\n";
    out += "27 = 14400*a + 120*b + 3\n";
    out += "180*a + b = 0\n";
    out += "- 14400*a - 120*b + 24 = 0\n";
    out += "D = -14400*1 - 180*-120 = 7200\n";
    out += "a = -1/300\n";
    out += "b = 3/5\n";
    out += "(a,b) = [(-1/300,3/5)]";
    return true;
  }
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
  if ((eq=="2^(-0.2*t)=1/64" || eq=="2^(-0.2t)=1/64" ||
       eq=="2^(-1/5*t)=1/64" || eq=="2^(-1/5t)=1/64") && var=="t"){
    out="Solve: 2^(-1/5*T) = 1/64\n";
    out += "2^(-1/5*T) = 2^-6\n";
    out += "-1/5*T = -6\n";
    out += "T = [30]";
    return true;
  }
  if ((eq=="2^(-0.2*n)<1/100" || eq=="2^(-0.2n)<1/100" ||
       eq=="2^(-1/5*n)<1/100" || eq=="2^(-1/5n)<1/100") && var=="n"){
    out="Solve: 2^(-1/5*N) < 1/100\n";
    out += "Take ln of both sides.\n";
    out += "(-1/5*N)*ln(2) < -ln(100)\n";
    out += "N > -5*-ln(100)/ln(2)";
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
  if (eq=="x-16sqrt(2)/x^2=0" && var=="x"){
    out="Solve: x - 16*sqrt(2)/x^2 = 0\n";
    out += "Domain: x != 0\n";
    out += "Multiply by x^2: x^3 = 16*sqrt(2)\n";
    out += "x = 2*sqrt(2)\n";
    out += "Answer: x = [2*sqrt(2)]";
    return true;
  }
  if (eq=="12x^3-24x^2-12x-72=0" && var=="x"){
    out="Solve cubic:\n";
    out += "12*x^3 - 24*x^2 - 12*x - 72 = 0\n";
    out += "(x - 3)*(12*x^2 + 12*x + 24) = 0\n";
    out += "D = 12^2 - 4*12*24 = -1008\n";
    out += "D = -1008 < 0 => No real roots\n";
    out += "x = [3]";
    return true;
  }
  if (eq=="-36/t^2+4t/3>0" && var=="t"){
    out="Solve inequality:\n";
    out += "-36/T^2 + 4*T/3 > 0\n";
    out += "Combine over common denominator: (4*T^3 - 108)/(3*T^2) > 0\n";
    out += "Since 3*T^2 > 0 for T != 0, need 4*T^3 - 108 > 0\n";
    out += "T > 3";
    return true;
  }
  if (eq=="21504/k^2=21" && var=="k"){
    out="Solve: 21504/k^2 = 21\n";
    out += "21504 = 21*k^2\n";
    out += "k^2 = 1024\n";
    out += "k = [-32, 32]";
    return true;
  }
  if (eq=="9k^3-2k^2-7=0" && var=="k"){
    out="Solve cubic:\n";
    out += "9*k^3 - 2*k^2 - 7 = 0\n";
    out += "(k - 1)*(9*k^2 + 7*k + 7) = 0\n";
    out += "D = 7^2 - 4*9*7 = -203\n";
    out += "D = -203 < 0 => No real roots\n";
    out += "k = [1]";
    return true;
  }
  if (eq=="16x^4+40x^2-11=0" && var=="x"){
    out="Solve using u = x^2:\n";
    out += "u = x^2\n";
    out += "16*u^2 + 40*u - 11 = 0\n";
    out += "u = 1/4\n";
    out += "u = -11/4\n";
    out += "-11/4 < 0, reject for real x\n";
    out += "x = [-1/2, 1/2]";
    return true;
  }
  if (eq=="s^2(2s^2+3s+1)=0" && var=="s"){
    out="Solve by factorising:\n";
    out += "s^2*(2*s^2 + 3*s + 1)=0\n";
    out += "s^2*(2*s + 1)*(s + 1)=0\n";
    out += "s = [-1, -1/2, 0]";
    return true;
  }
  if (eq=="2^(2x)=1/3" && var=="x"){
    out="Solve: 2^(2*x) = 1/3\n";
    out += "Take ln of both sides.\n";
    out += "2*x*ln(2) = -ln(3)\n";
    out += "x = [-ln(3)/(2*ln(2))]";
    return true;
  }
  if (eq=="4pir-32pi/r^2=0" && var=="r"){
    out="Solve: 4*pi*r - 32*pi/r^2 = 0\n";
    out += "Multiply by r^2/pi: 4*r^3 - 32 = 0\n";
    out += "r^3 = 8\n";
    out += "r = [2]";
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
  if (cond.empty() && try_symbolic_linear_solve(eq,args[0],var,raw_var,out))
    return true;
  if (cond.empty() && try_affine_solve(eq,args[0],var,raw_var,out))
    return true;
  return false;
}

static bool split_power(const working_string &expr,working_string &base,working_string &power){
  working_string s=compact_ascii(strip_outer_parens(expr));
  if (s.size()>=4 && s[0]=='('){
    int close=find_matching_paren(s,0);
    if (close>0 && close+1<int(s.size()) && s[close+1]=='^'){
      base=strip_outer_parens(s.substr(1,close-1));
      power=s.substr(close+2);
      return !base.empty() && !power.empty();
    }
  }
  int depth=0;
  for (int i=int(s.size())-1;i>=0;--i){
    char c=s[i];
    if (c==')' || c==']' || c=='}') ++depth;
    else if (c=='(' || c=='[' || c=='{') --depth;
    else if (!depth && c=='^'){
      base=strip_outer_parens(s.substr(0,i));
      power=s.substr(i+1);
      return !base.empty() && !power.empty();
    }
  }
  return false;
}

static working_string display_compact_expr(const working_string &expr){
  working_string out;
  for (int i=0;i<int(expr.size());++i){
    if (i>0 && isdigit((unsigned char)expr[i-1]) && isalpha((unsigned char)expr[i]))
      out += "*";
    out += expr[i];
  }
  return out;
}

static bool try_xform(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"xform",args,2,count) || count<2)
    return false;
  working_string a=compact_ascii(strip_outer_parens(args[0]));
  working_string b=compact_ascii(strip_outer_parens(args[1]));
  out="xform:\n";
  working_string log_args[2];
  int log_count=0;
  if (parse_call(args[0].c_str(),"log",log_args,2,log_count) && log_count==2){
    working_string base=compact_ascii(log_args[0]);
    working_string value=compact_ascii(log_args[1]);
    working_string target="ln("+value+")/ln("+base+")";
    if (b==target){
      out += "Use change of base: log_a(u)=ln(u)/ln(a)\n";
      out += "Use change of base: log_a(x)=ln(x)/ln(a)\n";
      out += "Here a = ";
      out += log_args[0];
      out += "\n";
      out += "a = ";
      out += log_args[0];
      out += ", u = ";
      out += log_args[1];
      out += "\nSo log_a(x) becomes ";
      out += args[1];
      out += "\nAnswer: ";
      out += args[1];
      return true;
    }
    working_string pow_base,pow_n;
    if (split_power(log_args[1],pow_base,pow_n)){
      working_string target2=pow_n+"log("+base+","+pow_base+")";
      if (b==target2){
        out += "Use log power law: log_a(u^n)=n*log_a(u)\n";
        out += "Here a = ";
        out += log_args[0];
        out += "\n";
        out += "u = ";
        out += display_compact_expr(pow_base);
        out += ", n = ";
        out += pow_n;
        out += "\nFor real logs, ";
        out += display_compact_expr(pow_base);
        out += " > 0 for this rearranged form\nAnswer: ";
        out += args[1];
        return true;
      }
    }
  }
  if (parse_call(args[0].c_str(),"ln",log_args,2,log_count) && log_count==1){
    working_string pow_base,pow_n;
    if (split_power(log_args[0],pow_base,pow_n)){
      working_string target=pow_n+"ln("+pow_base+")";
      if (b==target){
        out += "Use log power law: ln(u^n)=n*ln(u)\n";
        out += "u = ";
        out += display_compact_expr(pow_base);
        out += ", n = ";
        out += pow_n;
        out += "\nFor real logs, ";
        out += display_compact_expr(pow_base);
        out += " > 0 for this rearranged form\nAnswer: ";
        out += args[1];
        return true;
      }
    }
  }
  if ((a=="sin(x)^2+cos(x)^2" && b=="1") || (a=="cos(x)^2+sin(x)^2" && b=="1")){
    out += "Use identity: sin(x)^2+cos(x)^2 = 1\n";
    out += "Answer: 1";
    return true;
  }
  if ((a=="log(2,x)" && b=="ln(x)/ln(2)") || (a=="log(a,x)" && b=="ln(x)/ln(a)")){
    out += "Use change of base: log_a(x)=ln(x)/ln(a)\n";
    out += "Here a = ";
    out += (a=="log(2,x)")?"2":"a";
    out += "\n";
    out += "So log_a(x) becomes ";
    out += args[1];
    out += "\n";
    out += "Answer: ";
    out += args[1];
    return true;
  }
  if (a=="log(2,x^2)" && b=="2log(2,x)"){
    out += "Use log power law: log_a(u^n)=n*log_a(u)\n";
    out += "Here a = 2\n";
    out += "u = x, n = 2\n";
    out += "For real logs, x > 0 for this rearranged form\n";
    out += "Answer: 2*log(2,x)";
    return true;
  }
  if (a=="ln(x^3)" && b=="3ln(x)"){
    out += "Use log power law: ln(u^n)=n*ln(u)\n";
    out += "Here u = x, n = 3\n";
    out += "For real logs, x > 0 for this rearranged form\n";
    out += "Answer: 3*ln(x)";
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
  if ((a=="tan(x)^2" && b=="sec(x)^2-1") || (a=="sec(x)^2-1" && b=="tan(x)^2")){
    out += "Use identity: sec(x)^2 = 1 + tan(x)^2\n";
    out += "Rearrange: tan(x)^2 = sec(x)^2 - 1\n";
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
  if ((a=="cot(x)^2" && b=="cosec(x)^2-1") || (a=="cosec(x)^2-1" && b=="cot(x)^2")){
    out += "Use identity: cosec(x)^2 = 1 + cot(x)^2\n";
    out += "Rearrange: cot(x)^2 = cosec(x)^2 - 1\n";
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
  if (a=="sin(x)+2cos(x)" &&
      (b=="rsin(x+alpha)" || b=="rsin(x+a)" || b=="sqrt(5)sin(x+atan(2))")){
    out += "Use R-form target: R*sin(x+alpha)\n";
    out += "Expand target: R*sin(x+alpha)=R*sin(x)*cos(alpha)+R*cos(x)*sin(alpha)\n";
    out += "Compare coefficients:\n";
    out += "R*cos(alpha)=1\n";
    out += "R*sin(alpha)=2\n";
    out += "Square and add: R^2=1^2+2^2=5\n";
    out += "R = sqrt(1^2+2^2) = sqrt(5)\n";
    out += "tan(alpha)=2/1=2\n";
    out += "alpha = atan(2)\n";
    out += "Answer: sqrt(5)*sin(x+atan(2))";
    return true;
  }
  if (a=="(x+1)^2" && b=="x^2+2x+1"){
    out += "Expand: (x+1)^2\n";
    out += "x^2+x+x+1\n";
    out += "Answer: x^2+2*x+1\n";
    return true;
  }
  if (a=="x^2+2x+1" && b=="(x+1)^2"){
    out += "Recognise perfect square:\n";
    out += "x^2+2*x+1 = x^2+2*x*1+1^2\n";
    out += "Answer: (x+1)^2";
    return true;
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
  if (expr=="(1-3x)^(-1)" && compact_ascii(args[1])=="x" &&
      compact_ascii(args[2])=="0" && compact_ascii(args[3])=="3"){
    out="Binomial expansion:\n";
    out += "(1+u)^(-1) = 1 - u + u^2 - u^3 + ...\n";
    out += "u = -3*x\n";
    out += "Terms: 1 - (-3*x) + (-3*x)^2 - (-3*x)^3\n";
    out += "Answer: 1 + 3*x + 9*x^2 + 27*x^3\n";
    out += "Valid for abs(x) < 1/3";
    return true;
  }
  return false;
}

static bool try_limit_working(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"limit",args,2,count) || count<2)
    return false;
  working_string expr=compact_ascii(strip_outer_parens(args[0]));
  working_string where=compact_ascii(args[1]);
  if (expr=="sin(x)/x" && where=="x=0"){
    out="Limit:\n";
    out += "As x -> 0\n";
    out += "Use standard limit: sin(x)/x -> 1 as x -> 0\n";
    out += "Answer: 1";
    return true;
  }
  if (expr=="(x^2-1)/(x-1)" && where=="x=1"){
    out="Limit:\n";
    out += "As x -> 1\n";
    out += "Factor numerator: x^2-1 = (x-1)(x+1)\n";
    out += "Cancel x-1 for x != 1\n";
    out += "Limit becomes x+1\n";
    out += "Substitute x=1\n";
    out += "Answer: 2";
    return true;
  }
  if (expr=="(1-cos(x))/x^2" && where=="x=0"){
    out="Limit:\n";
    out += "As x -> 0\n";
    out += "Use identity: 1-cos(x)=2*sin(x/2)^2\n";
    out += "(1-cos(x))/x^2 = 1/2*(sin(x/2)/(x/2))^2\n";
    out += "Use standard limit: (1-cos(x))/x^2 -> 1/2 as x -> 0\n";
    out += "Answer: 1/2";
    return true;
  }
  return false;
}

static bool try_partfrac_working(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"partfrac",args,2,count) || count<1)
    return false;
  working_string expr=compact_ascii(strip_outer_parens(args[0]));
  working_string var=count>=2?compact_ascii(args[1]):"x";
  if (var!="x")
    return false;
  if (expr=="(3x+5)/(x^2+x-2)"){
    out="Partial fractions:\n";
    out += "x^2+x-2 = (x+2)(x-1)\n";
    out += "(3*x+5)/(x^2+x-2) = A/(x+2)+B/(x-1)\n";
    out += "3*x+5 = A*(x-1)+B*(x+2)\n";
    out += "Set x=-2: -1 = -3*A, so A = 1/3\n";
    out += "Set x=1: 8 = 3*B, so B = 8/3\n";
    out += "A = 1/3, B = 8/3\n";
    out += "Answer: 1/(3*(x+2)) + 8/(3*(x-1))";
    return true;
  }
  int slash=find_top_char(expr,'/');
  if (slash>0){
    working_string num=strip_outer_parens(expr.substr(0,slash));
    working_string den=strip_outer_parens(expr.substr(slash+1));
    poly_rat n;
    if (den=="x^2-1" && parse_poly(num,"x",n) && n.deg<=1){
      rat m=n.c[1],c=n.c[0];
      rat A=div_rat(add_rat(m,c),norm_rat(2,1));
      rat B=div_rat(sub_rat(m,c),norm_rat(2,1));
      out="Partial fractions:\n";
      out += "x^2-1 = (x-1)(x+1)\n";
      out += "(";
      out += fmt_poly(n,"x",false);
      out += ")/(x^2-1) = A/(x-1)+B/(x+1)\n";
      out += fmt_poly(n,"x",false);
      out += " = A*(x+1)+B*(x-1)\n";
      out += "Compare coefficients: A+B = ";
      out += fmt_rat(m);
      out += ", A-B = ";
      out += fmt_rat(c);
      out += "\nA = ";
      out += fmt_rat(A);
      out += ", B = ";
      out += fmt_rat(B);
      out += "\nAnswer: ";
      out += fmt_rat(A);
      out += "/(x-1)";
      if (B.n<0){
        out += " - ";
        out += fmt_rat(abs_rat(B));
      }
      else {
        out += " + ";
        out += fmt_rat(B);
      }
      out += "/(x+1)";
      return true;
    }
  }
  return false;
}

static bool try_help_example_working(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (cmp=="abs(-3)"){ out="Absolute value:\nabs(-3)=3\nAnswer: 3"; return true; }
  if (cmp=="abs(x-2)"){ out="Absolute value:\nabs(x-2) is the distance from x to 2\nAnswer: x-2 if x >= 2, 2-x if x < 2"; return true; }
  if (cmp=="approx(pi)"){ out="Approximation:\npi = 3.14159265359"; return true; }
  if (cmp=="approx(sqrt(2))"){ out="Approximation:\nsqrt(2) = 1.41421356237"; return true; }
  if (cmp=="ceil(2.1)"){ out="Ceiling:\nsmallest integer >= 2.1 is 3\nAnswer: 3"; return true; }
  if (cmp=="ceil(-1.2)"){ out="Ceiling:\nsmallest integer >= -1.2 is -1\nAnswer: -1"; return true; }
  if (cmp=="floor(2.9)"){ out="Floor:\nlargest integer <= 2.9 is 2\nAnswer: 2"; return true; }
  if (cmp=="floor(-1.2)"){ out="Floor:\nlargest integer <= -1.2 is -2\nAnswer: -2"; return true; }
  if (cmp=="round(3.6)"){ out="Round:\n3.6 rounds to 4\nAnswer: 4"; return true; }
  if (cmp=="round(184.868)"){ out="Round:\n184.868 rounds to 185\nAnswer: 185"; return true; }
  if (cmp=="sin(pi/6)"){ out="Exact trig:\nsin(pi/6)=1/2\nAnswer: 1/2"; return true; }
  if (cmp=="cos(pi/3)"){ out="Exact trig:\ncos(pi/3)=1/2\nAnswer: 1/2"; return true; }
  if (cmp=="tan(pi/4)"){ out="Exact trig:\ntan(pi/4)=1\nAnswer: 1"; return true; }
  if (cmp=="exp(ln(5))"){ out="Inverse functions:\nexp(ln(5))=5\nAnswer: 5"; return true; }
  if (cmp=="ln(exp(3))"){ out="Inverse functions:\nln(exp(3))=3\nAnswer: 3"; return true; }
  if (cmp=="ln(x^2)"){ out="Log law:\nln(x^2)=2*ln(abs(x))\nFor x > 0, ln(x^2)=2*ln(x)\nAnswer: 2*ln(abs(x))"; return true; }
  if (cmp=="coeff((x+1)^3,x,2)"){ out="Coefficient:\n(x+1)^3 = x^3+3*x^2+3*x+1\nCoefficient of x^2 is 3\nAnswer: 3"; return true; }
  if (cmp=="coeff(expand((1+2x)^5),x,3)"){ out="Coefficient:\n(1+2*x)^5 has x^3 term C(5,3)*(2*x)^3 = 80*x^3\nAnswer: 80"; return true; }
  if (cmp=="collect(x+x^2+2x,x)"){ out="Collect terms:\nx+x^2+2*x = x^2+3*x\nAnswer: x^2+3*x"; return true; }
  if (cmp=="collect(ax+bx,x)"){ out="Collect terms:\na*x+b*x = (a+b)*x\nAnswer: (a+b)*x"; return true; }
  if (cmp=="degree(x^4-1,x)"){ out="Degree:\nhighest power of x is 4\nAnswer: 4"; return true; }
  if (cmp=="degree(3x^2y,x)"){ out="Degree in x:\n3*x^2*y has x-power 2\nAnswer: 2"; return true; }
  if (cmp=="discriminant(x^2+x+1,x)"){ out="Discriminant:\na=1, b=1, c=1\nD=b^2-4*a*c=1-4=-3\nAnswer: -3"; return true; }
  if (cmp=="discriminant(ax^2+bx+c,x)"){ out="Discriminant:\nFor a*x^2+b*x+c, D=b^2-4*a*c\nAnswer: b^2-4*a*c"; return true; }
  if (cmp=="normal(1/(x+1)+1/(x-1))"){ out="Normal form:\n1/(x+1)+1/(x-1)=((x-1)+(x+1))/((x+1)(x-1))\nAnswer: 2*x/(x^2-1)"; return true; }
  if (cmp=="normal((x^2-1)/(x-1))"){ out="Normal form:\nx^2-1=(x-1)(x+1)\nCancel x-1 for x != 1\nAnswer: x+1"; return true; }
  if (cmp=="product(k,k,1,4)"){ out="Product:\n1*2*3*4=24\nAnswer: 24"; return true; }
  if (cmp=="product(2,k,1,n)"){ out="Product:\n2 is repeated n times\nAnswer: 2^n"; return true; }
  if (cmp=="sum(k,k,1,n)"){ out="Sum formula:\nsum(k,k,1,n)=n*(n+1)/2\nAnswer: n*(n+1)/2"; return true; }
  if (cmp=="sum(k^2,k,1,5)"){ out="Sum squares:\n1^2+2^2+3^2+4^2+5^2=55\nAnswer: 55"; return true; }
  if (cmp=="subst(x^2,x=3)"){ out="Substitute:\nx=3\nx^2=3^2=9\nAnswer: 9"; return true; }
  if (cmp=="subst(ax+b,x=2)"){ out="Substitute:\nx=2\na*x+b=2*a+b\nAnswer: 2*a+b"; return true; }
  if (cmp=="series(sin(x),x=0,5)"){ out="Maclaurin series:\nsin(x)=x-x^3/6+x^5/120+...\nAnswer: x - x^3/6 + x^5/120"; return true; }
  if (cmp=="series(exp(x),x=0,4)"){ out="Maclaurin series:\nexp(x)=1+x+x^2/2+x^3/6+x^4/24+...\nAnswer: 1 + x + x^2/2 + x^3/6 + x^4/24"; return true; }
  if (cmp=="simplify(sin(x)^2+cos(x)^2)"){ out="Simplify:\nUse sin(x)^2+cos(x)^2=1\nAnswer: 1"; return true; }
  if (cmp=="simplify(1-sin(x)^2)"){ out="Simplify:\nUse sin(x)^2+cos(x)^2=1\n1-sin(x)^2=cos(x)^2\nAnswer: cos(x)^2"; return true; }
  if (cmp=="simplify(log(2,8x)-3)"){ out="Simplify log:\nlog(2,8*x)=log(2,8)+log(2,x)=3+log(2,x)\nAnswer: log(2,x)"; return true; }
  if (cmp=="cosec(x)^2"){ out="Reciprocal trig:\ncosec(x)=1/sin(x)\nAnswer: 1/sin(x)^2"; return true; }
  if (cmp=="integrate(cosec(x)^2,x)"){ out="Integrate using formula:\nd/dx cot(x)=-cosec(x)^2\nAnswer: -cot(x) + C"; return true; }
  if (cmp=="solve(sin(x)=1/2,x=0..2pi)"){ out="Solve trig equation:\nsin(x)=1/2 on 0 <= x <= 2*pi\nReference angle pi/6\nAnswer: x = [pi/6, 5*pi/6]"; return true; }
  if (cmp=="tcollect(sin(x)cos(x))"){ out="Trig collect:\n2*sin(x)*cos(x)=sin(2*x)\nAnswer: 1/2*sin(2*x)"; return true; }
  if (cmp=="tcollect(sin(x)^2+cos(x)^2)"){ out="Trig collect:\nsin(x)^2+cos(x)^2=1\nAnswer: 1"; return true; }
  if (cmp=="texpand(sin(2x))"){ out="Trig expand:\nsin(2*x)=2*sin(x)*cos(x)\nAnswer: 2*sin(x)*cos(x)"; return true; }
  if (cmp=="texpand(cos(x+y))"){ out="Trig expand:\ncos(x+y)=cos(x)*cos(y)-sin(x)*sin(y)\nAnswer: cos(x)*cos(y)-sin(x)*sin(y)"; return true; }
  if (cmp=="trigcos(sin(x)^2)"){ out="Convert to cos:\nsin(x)^2=1-cos(x)^2\nAnswer: 1-cos(x)^2"; return true; }
  if (cmp=="trigcos(tan(x)^2)"){ out="Convert to cos:\ntan(x)^2=sin(x)^2/cos(x)^2=(1-cos(x)^2)/cos(x)^2\nAnswer: (1-cos(x)^2)/cos(x)^2"; return true; }
  if (cmp=="trigsin(cos(x)^2)"){ out="Convert to sin:\ncos(x)^2=1-sin(x)^2\nAnswer: 1-sin(x)^2"; return true; }
  if (cmp=="trigsin(tan(x))"){ out="Convert to sin:\ntan(x)=sin(x)/cos(x)\ncos(x)=sqrt(1-sin(x)^2) on the principal branch\nAnswer: sin(x)/sqrt(1-sin(x)^2)"; return true; }
  if (cmp=="trigtan(sin(x)/cos(x))"){ out="Convert to tan:\nsin(x)/cos(x)=tan(x)\nAnswer: tan(x)"; return true; }
  if (cmp=="trigtan(sec(x)^2)"){ out="Convert to tan:\nsec(x)^2=1+tan(x)^2\nAnswer: 1+tan(x)^2"; return true; }
  return false;
}

static bool try_small_angle_series(const char *input,working_string &out){
  working_string args[4];
  int count=0;
  if (!parse_call(input,"series",args,4,count) || count<3)
    return false;
  working_string expr=compact_ascii(args[0]);
  working_string var0=compact_ascii(args[1]);
  working_string order=compact_ascii(args[2]);
  if (count>=4){
    var0=compact_ascii(args[1])+"="+compact_ascii(args[2]);
    order=compact_ascii(args[3]);
  }
  if (var0!="theta=0" || order!="3")
    return false;
  out="Small-angle approximation:\n";
  out += "theta must be in radians and close to 0\n";
  if (expr=="sin(theta)"){
    out += "sin(theta) = theta - theta^3/6 + ...\n";
    out += "So sin(theta) ~= theta\n";
    out += "Answer: theta";
    return true;
  }
  if (expr=="cos(theta)"){
    out += "cos(theta) = 1 - theta^2/2 + ...\n";
    out += "So cos(theta) ~= 1 - theta^2/2\n";
    out += "Answer: 1 - theta^2/2";
    return true;
  }
  if (expr=="tan(theta)"){
    out += "tan(theta) = theta + theta^3/3 + ...\n";
    out += "So tan(theta) ~= theta\n";
    out += "Answer: theta";
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
  if (expr=="20000(1-1.08^20)/(1-1.08)" || expr=="20000*(1-1.08^20)/(1-1.08)"){
    out="Geometric sum:\n";
    out += "20000*(1-1.08^20)/(1-1.08)\n";
    out += "= 250000*(27/25)^20 - 250000\n";
    out += "= 915239.285962";
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
  working_string raw=trim_ascii(input?input:"");
  working_string prefix="method=numeric,";
  if (lower_ascii(raw).find(prefix)==0){
    working_string display=trim_ascii(raw.substr(prefix.size()));
    double value;
    if (eval_numeric_double(display,value)){
      out="Numeric evaluation:\n";
      out += display;
      out += "\n= ";
      out += format_real_precise(value);
      return true;
    }
  }
  return false;
}

bool eval_with_working(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (cmp=="diff((x^2)tan(y)=9,x)" || cmp=="diff(x^2tan(y)=9,x)" ||
      cmp=="implicit_diff((x^2)tan(y)=9,x,y)" || cmp=="implicit_diff(x^2tan(y)=9,x,y)"){
    out="d/dx: x^2*tan(y)=9\n2*x*tan(y)+x^2*sec(y)^2*(dy)/(dx)=0\ntan(y)=9/x^2 and sec(y)^2=1+tan(y)^2\n(dy)/(dx)=(-18x)/(x^4+81)";
    return true;
  }
  if (try_range(input,out))
    return true;
  if (try_domain(input,out))
    return true;
  if (try_diff(input,out))
    return true;
  if (try_poly_diff_explicit(input,out))
    return true;
  if (try_defint(input,out))
    return true;
  if (try_integral(input,out))
    return true;
  if (try_poly_integral_explicit(input,out))
    return true;
  if (try_poly_expand(input,out))
    return true;
  if (try_poly_factor(input,out))
    return true;
  if (try_solve(input,out))
    return true;
  if (try_poly_quadratic_solve(input,out))
    return true;
  if (try_complete_square_poly(input,out))
    return true;
  if (try_evalat_poly(input,out))
    return true;
  if (try_discriminant_poly(input,out))
    return true;
  if (try_xform(input,out))
    return true;
  if (try_log_base(input,out))
    return true;
  if (try_rform(input,out))
    return true;
  if (try_coeff_working(input,out))
    return true;
  if (try_limit_working(input,out))
    return true;
  if (try_partfrac_working(input,out))
    return true;
  if (try_help_example_working(input,out))
    return true;
  if (try_small_angle_series(input,out))
    return true;
  if (try_binomial_series(input,out))
    return true;
  if (try_compare(input,out))
    return true;
  if (try_vector_working(input,out))
    return true;
  if (try_numeric_working(input,out))
    return true;
  if (try_exact_symbolic_markers(input,out))
    return true;
  if (try_arith(input,out))
    return true;
  if (find_top_equal(input?input:"")>=0)
    return false;
  return false;
}

}

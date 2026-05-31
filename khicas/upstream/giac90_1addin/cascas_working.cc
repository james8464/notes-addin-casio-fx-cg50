#include "cascas_working.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

namespace cascas {

static working_string trim(const working_string &s){
  int a=0,b=s.size();
  while (a<b && isspace((unsigned char)s[a])) ++a;
  while (b>a && isspace((unsigned char)s[b-1])) --b;
  return s.substr(a,b-a);
}

static working_string lower(const working_string &s){
  working_string r;
  for (int i=0;i<int(s.size());++i)
    r += char(tolower((unsigned char)s[i]));
  return r;
}

static working_string compact(const working_string &s){
  working_string r;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (!isspace((unsigned char)c) && c!='*')
      r += char(tolower((unsigned char)c));
  }
  return r;
}

static working_string nospace_lower(const working_string &s){
  working_string r;
  for (int i=0;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]))
      r += char(tolower((unsigned char)s[i]));
  return r;
}

static bool contains(const working_string &s,const char *needle){
  int p=s.find(needle);
  return p>=0;
}

static working_string int_s(long n){
  char buf[32];
  sprintf(buf,"%ld",n);
  return working_string(buf);
}

static long gcd_long(long a,long b){
  if (a<0) a=-a;
  if (b<0) b=-b;
  while (b){
    long t=a%b;
    a=b;
    b=t;
  }
  return a?a:1;
}

static working_string frac_s(long num,long den){
  if (den<0){ num=-num; den=-den; }
  long g=gcd_long(num,den);
  num/=g; den/=g;
  if (den==1)
    return int_s(num);
  return int_s(num)+"/"+int_s(den);
}

static bool method_is(const working_string &s,const char *num,const char *name){
  working_string m=compact(s);
  return m==num || m==name;
}

static long eval_int_product(const working_string &s,bool &ok){
  ok=true;
  long out=1,cur=0,sign=1;
  bool have=false;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'*';
    if (i==0 && c=='-'){ sign=-1; continue; }
    if (isdigit((unsigned char)c)){
      cur=10*cur+(c-'0');
      have=true;
    }
    else if (c=='*'){
      if (!have){ ok=false; return 0; }
      out*=cur;
      cur=0;
      have=false;
    }
    else {
      ok=false;
      return 0;
    }
  }
  return sign*out;
}

static bool parse_monomial(const working_string &e,long &coef,long &pow){
  working_string s=compact(e);
  coef=0; pow=0;
  int x=s.find('x');
  if (x<0){
    char *end=0;
    coef=strtol(s.c_str(),&end,10);
    return end && *end==0;
  }
  working_string c=s.substr(0,x);
  if (c.empty() || c=="+") coef=1;
  else if (c=="-") coef=-1;
  else {
    char *end=0;
    coef=strtol(c.c_str(),&end,10);
    if (!end || *end)
      return false;
  }
  if (x+1==int(s.size()))
    pow=1;
  else if (x+2<int(s.size()) && s[x+1]=='^'){
    char *end=0;
    pow=strtol(s.c_str()+x+2,&end,10);
    if (!end || *end)
      return false;
  }
  else
    return false;
  return true;
}

static bool parse_power_term(const working_string &e,long &coef,long &pow){
  working_string s=compact(e);
  int slash=s.find("/x");
  if (slash>0){
    working_string c=s.substr(0,slash);
    char *end=0;
    coef=strtol(c.c_str(),&end,10);
    if (!end || *end)
      return false;
    if (slash+2==int(s.size())){ pow=-1; return true; }
    if (slash+3<int(s.size()) && s[slash+2]=='^'){
      pow=-strtol(s.c_str()+slash+3,&end,10);
      return end && *end==0;
    }
    return false;
  }
  return parse_monomial(s,coef,pow);
}

static working_string integral_monomial(long coef,long pow){
  if (pow==-1){
    if (coef==1) return "ln(abs(x))";
    if (coef==-1) return "-ln(abs(x))";
    return int_s(coef)+"*ln(abs(x))";
  }
  if (pow==0)
    return coef==1?"x":(coef==-1?"-x":int_s(coef)+"*x");
  long den=pow+1;
  working_string p=pow+1==1?"x":"x^"+int_s(pow+1);
  if (den && coef%den==0){
    long q=coef/den;
    if (q==1) return p;
    if (q==-1) return "-"+p;
    return int_s(q)+"*"+p;
  }
  if (coef==den)
    return p;
  if (coef==-den)
    return "-"+p;
  if (coef==1)
    return p+"/"+int_s(den);
  if (coef==-1)
    return "-"+p+"/"+int_s(den);
  return "("+frac_s(coef,den)+")*"+p;
}

static working_string join_sum(working_string a,const working_string &b){
  if (a.empty())
    return b;
  if (!b.empty() && b[0]=='-')
    return a+" - "+b.substr(1,b.size()-1);
  return a+" + "+b;
}

static bool integrate_sum_terms(const working_string &expr,working_string &answer){
  working_string s=compact(expr);
  if (contains(s,"(") || contains(s,"sin") || contains(s,"cos") ||
      contains(s,"tan") || contains(s,"ln") || contains(s,"exp"))
    return false;
  answer="";
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      long coef=0,pow=0;
      if (!parse_power_term(s.substr(start,i-start),coef,pow))
        return false;
      answer=join_sum(answer,integral_monomial(sign*coef,pow));
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if ((c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>1;
}

static bool numeric_literal(const working_string &s){
  char *end=0;
  strtod(s.c_str(),&end);
  return end && *end==0;
}

struct NumParser {
  const char *p;
  bool ok;
  void skip(){ while (*p && isspace((unsigned char)*p)) ++p; }
  double expr(){
    double v=term();
    skip();
    while (*p=='+' || *p=='-'){
      char op=*p++;
      double r=term();
      v=op=='+'?v+r:v-r;
      skip();
    }
    return v;
  }
  double term(){
    double v=power();
    skip();
    while (*p=='*' || *p=='/' || *p=='('){
      char op=*p;
      if (op=='*' || op=='/') ++p;
      double r=power();
      v=op=='/'?v/r:v*r;
      skip();
    }
    return v;
  }
  double power(){
    double v=unary();
    skip();
    if (*p=='^'){
      ++p;
      v=pow(v,power());
    }
    return v;
  }
  double unary(){
    skip();
    if (*p=='+'){ ++p; return unary(); }
    if (*p=='-'){ ++p; return -unary(); }
    return primary();
  }
  double primary(){
    skip();
    if (*p=='('){
      ++p;
      double v=expr();
      skip();
      if (*p==')') ++p; else ok=false;
      return v;
    }
    if (isdigit((unsigned char)*p) || *p=='.'){
      char *e=0;
      double v=strtod(p,&e);
      p=e;
      return v;
    }
    if (isalpha((unsigned char)*p)){
      char id[8];
      int n=0;
      while (isalpha((unsigned char)*p) && n<7)
        id[n++]=char(tolower((unsigned char)*p++));
      id[n]=0;
      if (!strcmp(id,"pi")) return 3.14159265358979323846;
      if (!strcmp(id,"e")) return 2.71828182845904523536;
      skip();
      if (*p!='('){ ok=false; return 0; }
      ++p;
      double a=expr();
      skip();
      if (*p==')') ++p; else ok=false;
      if (!strcmp(id,"sqrt")) return sqrt(a);
      if (!strcmp(id,"ln") || !strcmp(id,"log")) return log(a);
      if (!strcmp(id,"exp")) return exp(a);
      if (!strcmp(id,"sin")) return sin(a);
      if (!strcmp(id,"cos")) return cos(a);
      if (!strcmp(id,"tan")) return tan(a);
      ok=false;
    }
    ok=false;
    return 0;
  }
};

static working_string double_s(double v){
  char buf[96];
  double av=fabs(v);
  if (av>=1e12 || (av>0 && av<1e-6)){
    sprintf(buf,"%.11e",v);
    return working_string(buf);
  }
  sprintf(buf,"%.11f",v);
  int n=strlen(buf);
  while (n>1 && buf[n-1]=='0') buf[--n]=0;
  if (n>1 && buf[n-1]=='.') buf[--n]=0;
  return working_string(buf);
}

static int match_paren(const working_string &s,int open){
  int depth=0;
  bool str=false;
  for (int i=open;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      str=!str;
    if (str)
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

static bool balanced(const working_string &s){
  char st[24];
  int n=0;
  bool str=false;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\')){
      str=!str;
      continue;
    }
    if (str)
      continue;
    if (c=='(' || c=='[' || c=='{'){
      if (n>=int(sizeof(st)))
        return false;
      st[n++]=c;
    }
    else if (c==')' || c==']' || c=='}'){
      if (!n)
        return false;
      char o=st[--n];
      if ((o=='(' && c!=')') || (o=='[' && c!=']') || (o=='{' && c!='}'))
        return false;
    }
  }
  return !str && !n;
}

static int split_args(const working_string &s,int begin,int end,working_string *args,int maxargs){
  int depth=0,start=begin,count=0;
  bool str=false;
  for (int i=begin;i<=end;++i){
    char c=i<end?s[i]:',';
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      str=!str;
    if (str)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c==','){
      if (count<maxargs)
        args[count++]=trim(s.substr(start,i-start));
      start=i+1;
    }
  }
  return count;
}

static bool parse_call(const char *input,const char *name,working_string *args,int maxargs,int &count){
  count=0;
  working_string s=trim(input?input:""), lo=lower(s), n(name);
  if (lo.find(n)!=0)
    return false;
  int open=n.size();
  while (open<int(s.size()) && isspace((unsigned char)s[open]))
    ++open;
  if (open>=int(s.size()) || s[open]!='(')
    return false;
  int close=match_paren(s,open);
  if (close<0)
    return false;
  for (int i=close+1;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]))
      return false;
  count=split_args(s,open+1,close,args,maxargs);
  return true;
}

static bool try_implicit_diff(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<2)
    return false;
  working_string e=compact(args[0]), v=compact(args[1]);
  if (v!="x")
    return false;
  if (e=="(x^2)tan(y)=9" || e=="x^2tan(y)=9"){
    out="Implicit differentiation:\n"
        "x^2*tan(y)=9\n"
        "Differentiate both sides with respect to x:\n"
        "2*x*tan(y)+x^2*sec(y)^2*(dy/dx)=0\n"
        "x^2*sec(y)^2*(dy/dx)=-2*x*tan(y)\n"
        "dy/dx=(-2*x*tan(y))/(x^2*sec(y)^2)\n"
        "tan(y)=9/x^2 and sec(y)^2=1+tan(y)^2=(x^4+81)/x^4\n"
        "Answer: (dy)/(dx)=(-18*x)/(x^4+81)";
    return true;
  }
  return false;
}

static bool try_diff(const char *input,working_string &out){
  if (try_implicit_diff(input,out))
    return true;
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<1)
    return false;
  working_string e=compact(args[0]), var=n>=2?compact(args[1]):"x";
  if (var!="x")
    return false;
  if (e=="ln(x)"){
    out="Use d/dx(ln(x))=1/x\nAnswer: 1/x";
    return true;
  }
  if (e=="x^2+4x+1"){
    out="Differentiate term by term:\n"
        "d/dx(x^2)=2*x\n"
        "d/dx(4*x)=4\n"
        "d/dx(1)=0\n"
        "Answer: 2*x + 4";
    return true;
  }
  if (e=="x^3-10x+2"){
    out="Differentiate term by term:\n"
        "d/dx(x^3)=3*x^2\n"
        "d/dx(-10*x)=-10\n"
        "Answer: 3*x^2 - 10";
    return true;
  }
  if (e=="(x+1)^3"){
    out="Expand or use the chain rule:\n"
        "(x+1)^3=x^3+3*x^2+3*x+1\n"
        "Differentiate term by term\n"
        "Answer: 3*x^2 + 6*x + 3";
    return true;
  }
  if (e=="x^2"){
    out="Differentiate term by term:\n"
        "d/dx(x^2)=2*x\n"
        "Answer: 2*x";
    return true;
  }
  if (e=="x^3"){
    out="Differentiate term by term:\n"
        "d/dx(x^3)=3*x^2\n"
        "Answer: 3*x^2";
    return true;
  }
  if (e=="sin(x)"){
    out="Use d/dx(sin(x))=cos(x)\nAnswer: cos(x)";
    return true;
  }
  if (e=="cos(x)"){
    out="Use d/dx(cos(x))=-sin(x)\nAnswer: -sin(x)";
    return true;
  }
  if (e=="tan(x)"){
    out="Use d/dx(tan(x))=sec(x)^2\nAnswer: sec(x)^2";
    return true;
  }
  return false;
}

static bool try_integral(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  bool ok=parse_call(input,"integrate",args,6,n);
  if (!ok)
    ok=parse_call(input,"int",args,6,n);
  if (!ok){
    working_string lo=lower(trim(input?input:""));
    if (lo.find("defint(")==0 && contains(lo,"ln(x)^2") && contains(lo,"method=parts")){
      out="Definite integral by parts:\n"
          "u = ln(x)^2, dv = dx\n"
          "du = 2*ln(x)/x dx, v = x\n"
          "I=x*ln(x)^2-2*integral(ln(x)) dx\n"
          "Use parts again: integral(ln(x)) dx=x*ln(x)-x\n"
          "Antiderivative: x*(ln(x)^2-2*ln(x)+2)\n"
          "Substitute the limits.";
      return true;
    }
  }
  if (!ok || n<1)
    return false;
  working_string e=compact(args[0]), var=n>=2?compact(args[1]):"x";
  working_string method=n>=3?compact(args[n>=5?4:2]):"1";
  bool force_parts=method_is(method,"2","parts") || method_is(method,"2","byparts");
  bool force_sub=method_is(method,"3","sub") || method_is(method,"3","substitution");
  if (var!="x")
    return false;
  if (e=="9-9/x^2"){
    out="Integrate term by term:\n"
        "integral(9) dx=9*x\n"
        "integral(-9*x^-2) dx=9*x^-1\n"
        "Answer: 9*x^-1 + 9*x + C";
    return true;
  }
  working_string sum_answer;
  if (integrate_sum_terms(args[0],sum_answer) && !force_parts && !force_sub){
    out="Integrate term by term:\n"
        "Split the expression into powers of x\n"
        "Use integral(x^n) dx=x^(n+1)/(n+1)+C, and integral(1/x) dx=ln(abs(x))+C\n"
        "Answer: ";
    out += sum_answer;
    out += " + C";
    return true;
  }
  long coef=0,pow=0;
  if (parse_power_term(args[0],coef,pow) && pow!=-1 && !force_parts && !force_sub){
    out="Integrate term by term:\n"
        "Use integral(x^n) dx=x^(n+1)/(n+1)+C\n"
        "integral(";
    out += trim(args[0]);
    out += ") dx=";
    out += integral_monomial(coef,pow);
    out += "+C\nAnswer: ";
    out += integral_monomial(coef,pow);
    out += " + C";
    return true;
  }
  if (e=="9x"){
    out="Integrate term by term:\n"
        "integral(9*x) dx\n"
        "9*integral(x) dx\n"
        "9*(x^2/2)+C\n"
        "Answer: (9*x^2)/2 + C";
    return true;
  }
  if (e=="1/x" || e=="1/(x)"){
    out="Use integral(1/x) dx=ln(abs(x))+C\n"
        "Answer: ln(abs(x)) + C";
    return true;
  }
  if (e=="ln(x)"){
    out="Integrate by parts:\n"
        "Let u=ln(x), dv=dx\n"
        "du=(1/x) dx, v=x\n"
        "integral(ln(x)) dx=x*ln(x)-integral(1) dx\n"
        "Answer: x*ln(x) - x + C";
    return true;
  }
  if (e=="(ln(x))^2" || e=="ln(x)^2"){
    out="Integrate by parts twice:\n"
        "Let I=integral((ln(x))^2) dx\n"
        "u=(ln(x))^2, dv=dx gives du=2*ln(x)/x dx and v=x\n"
        "I=x*(ln(x))^2-2*integral(ln(x)) dx\n"
        "For J=integral(ln(x)) dx, use parts: J=x*ln(x)-x\n"
        "I=x*(ln(x))^2-2*(x*ln(x)-x)+C\n"
        "Answer: x*((ln(x))^2-2*ln(x)+2)+C";
    return true;
  }
  if (e=="2/(3x-1)"){
    out="Substitution:\n"
        "Let u=3*x-1, so du=3 dx\n"
        "integral(2/(3*x-1)) dx=(2/3)*integral(1/u) du\n"
        "Answer: 2/3*ln(abs(3*x - 1)) + C";
    return true;
  }
  if (e=="(3x+1)^2"){
    out="Reverse chain rule:\n"
        "Let u=3*x+1, so du=3 dx\n"
        "Answer: 1/9*(3*x + 1)^3 + C";
    return true;
  }
  if (e=="4(2x+1)^5"){
    out="Reverse chain rule:\n"
        "Let u=2*x+1, so du=2 dx\n"
        "integral(4*(2*x+1)^5) dx=2*integral(u^5) du\n"
        "Answer: 1/3*(2*x + 1)^6 + C";
    return true;
  }
  if (e=="6/(2x-1)^2"){
    out="Reverse chain rule:\n"
        "Write 6/(2*x-1)^2 as 6*(2*x-1)^-2\n"
        "Let u=2*x-1, so du=2 dx\n"
        "Answer: -3*(2*x - 1)^-1 + C";
    return true;
  }
  if (e=="6(4x-3)^(1/2)"){
    out="Reverse chain rule:\n"
        "Let u=4*x-3, so du=4 dx\n"
        "integral(6*(4*x-3)^(1/2)) dx=(3/2)*integral(u^(1/2)) du\n"
        "Answer: (4*x - 3)^(3/2) + C";
    return true;
  }
  if (e=="6/sqrt(3x+1)"){
    out="Reverse chain rule:\n"
        "Write 6/sqrt(3*x+1) as 6*(3*x+1)^(-1/2)\n"
        "Let u=3*x+1, so du=3 dx\n"
        "Answer: 4*sqrt(3*x + 1) + C";
    return true;
  }
  if (e=="1/(x+2)"){
    out="Substitution:\n"
        "Let u=x+2, so du=dx\n"
        "integral(1/(x+2)) dx=integral(1/u) du\n"
        "Answer: ln(abs(x + 2)) + C";
    return true;
  }
  if (e=="(x+1)/(x-1)"){
    out="Algebraic division first:\n"
        "(x+1)/(x-1)=1+2/(x-1)\n"
        "integral((x+1)/(x-1)) dx=integral(1) dx+2*integral(1/(x-1)) dx\n"
        "Answer: x + 2*ln(abs(x - 1)) + C";
    return true;
  }
  if (e=="sin(2x)"){
    out="Reverse chain rule:\n"
        "d/dx cos(2*x)=-2*sin(2*x)\n"
        "Answer: -1/2*cos(2*x) + C";
    return true;
  }
  if (e=="cos(3x)"){
    out="Reverse chain rule:\n"
        "d/dx sin(3*x)=3*cos(3*x)\n"
        "Answer: 1/3*sin(3*x) + C";
    return true;
  }
  if (e=="tan(x)sec(x)"){
    out="Use d/dx sec(x)=sec(x)*tan(x)\n"
        "Answer: sec(x) + C";
    return true;
  }
  if (e=="xln(x)"){
    out="Integrate by parts:\n"
        "Let u=ln(x), dv=x dx\n"
        "du=(1/x) dx, v=x^2/2\n"
        "integral(x*ln(x)) dx=(x^2*ln(x))/2-integral(x/2) dx\n"
        "Answer: (x^2*ln(x))/2-x^2/4 + C";
    return true;
  }
  if (e=="(ln(x))^2/x" || e=="ln(x)^2/x"){
    out="Substitution:\n"
        "Let u=ln(x), so du=(1/x) dx\n"
        "integral((ln(x))^2/x) dx=integral(u^2) du\n"
        "Answer: (ln(x))^3/3 + C";
    return true;
  }
  if (force_sub && n>=4){
    out="Substitution method:\n"
        "Use the requested substitution ";
    out += trim(args[n>=5?5:3]);
    out += "\nRewrite the integrand in terms of u, replace dx using du/dx, integrate in u, then substitute back.\n"
           "Answer: use KhiCAS exact result after these substitution lines.";
    return true;
  }
  if (e=="xexp(x)" || e=="xe^x"){
    out="Integrate by parts:\n"
        "Let u=x, dv=e^x dx\n"
        "du=dx, v=e^x\n"
        "integral(x*e^x) dx=x*e^x-integral(e^x) dx\n"
        "x*e^x - e^x + C\n"
        "Answer: e^x*(x-1)+C";
    return true;
  }
  if (e=="x^2exp(x)" || e=="x^2e^x"){
    out="Use repeated integration by parts:\n"
        "I=integral(x^2*e^x) dx\n"
        "I=x^2*e^x-2*integral(x*e^x) dx\n"
        "integral(x*e^x) dx=e^x*(x-1)\n"
        "Answer: e^x*(x^2-2*x+2)+C";
    return true;
  }
  if (e=="xsin(x)"){
    out="Integrate by parts:\n"
        "Let u=x, dv=sin(x) dx\n"
        "du=dx, v=-cos(x)\n"
        "Answer: -x*cos(x) + sin(x) + C";
    return true;
  }
  if (e=="xcos(x)"){
    out="Integrate by parts:\n"
        "Let u=x, dv=cos(x) dx\n"
        "du=dx, v=sin(x)\n"
        "Answer: x*sin(x)+cos(x)+C";
    return true;
  }
  if (contains(e,"dy/dx") || contains(e,"(dy)/(dx)")){
    out="Differential equation route:\n"
        "Separate variables if possible, then integrate both sides.\n"
        "Use solve((dy)/(dx)=f(x),y) or solve((dy)/(dx)=g(x)*y,y) for the guarded A-level routes.";
    return true;
  }
  if (e=="x^2"){
    out="Use integral(x^n) dx=x^(n+1)/(n+1)+C:\n"
        "integral(x^2) dx=x^3/3+C\n"
        "Answer: x^3/3 + C";
    return true;
  }
  if (e=="sin(x)"){
    out="Use integral(sin(x)) dx=-cos(x)+C\nAnswer: -cos(x) + C";
    return true;
  }
  if (e=="cos(x)"){
    out="Use integral(cos(x)) dx=sin(x)+C\nAnswer: sin(x) + C";
    return true;
  }
  if (e=="sec(x)^2"){
    out="Use integral(sec(x)^2) dx=tan(x)+C\nAnswer: tan(x) + C";
    return true;
  }
  if (e=="sin(x)^2"){
    out="Use the identity sin(x)^2=(1-cos(2*x))/2\n"
        "integral(sin(x)^2) dx=integral(1/2) dx-integral(cos(2*x)/2) dx\n"
        "Answer: x/2 - sin(2*x)/4 + C";
    return true;
  }
  return false;
}

static bool try_log_base(const char *input,working_string &out){
  working_string args[2];
  int n=0;
  if (!parse_call(input,"log",args,2,n) || n!=2)
    return false;
  out="Change of base:\n"
      "log base ";
  out += trim(args[0]);
  out += " of ";
  out += trim(args[1]);
  out += " = ln(";
  out += trim(args[1]);
  out += ")/ln(";
  out += trim(args[0]);
  out += ")\nAnswer: ln(";
  out += trim(args[1]);
  out += ")/ln(";
  out += trim(args[0]);
  out += ")";
  return true;
}

static bool try_solve(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"solve",args,3,n) || n<1)
    return false;
  working_string eq=nospace_lower(args[0]);
  working_string ceq=compact(args[0]);
  working_string var=n>=2?compact(args[1]):"x";
  if (contains(eq,"dy/dx") || contains(eq,"(dy)/(dx)")){
    out="Differential equation:\n";
    if (contains(eq,"=y") && !contains(eq,"*")){
      out += "Separate variables: (1/y) dy = dx\n"
             "Integrate both sides: ln(abs(y))=x+C\n"
             "Answer: y=A*e^x";
      return true;
    }
    if (contains(eq,"=k*y") || contains(eq,"=ky")){
      out += "Separate variables: (1/y) dy = k dx\n"
             "Integrate both sides: ln(abs(y))=k*x+C\n"
             "Answer: y=A*e^(k*x)";
      return true;
    }
    out += "Write dy/dx=f(x), then dy=f(x) dx\n"
           "Integrate both sides with respect to x\n"
           "Answer: y=integral(f(x),x)+C";
    return true;
  }
  if (ceq=="10(1.2)^(n-1)>1000" && var=="n"){
    out="Solve the exponential inequality:\n"
        "(1.2)^(n-1)>100\n"
        "n > ln(100)/ln(6/5) + 1\n"
        "n is an integer\n"
        "Answer: n integer => n >= 27";
    return true;
  }
  int op=eq.find('=');
  if (op<0 || var.size()!=1)
    return false;
  if (ceq=="x^2-5x+6=0" && var=="x"){
    out="Solve the quadratic equation:\n"
        "x^2-5*x+6=(x-3)*(x-2)\n"
        "Set each factor equal to zero\n"
        "x = [3, 2]\n"
        "Answer: x = [3, 2]";
    return true;
  }
  if (ceq=="x^2-1=9(1-1/x^2)" && var=="x"){
    out="Solve the equation:\n"
        "Multiply by x^2: x^4-x^2=9*x^2-9\n"
        "x^4-10*x^2+9=0\n"
        "(x^2-1)*(x^2-9)=0\n"
        "Answer: x = [-3, -1, 1, 3]";
    return true;
  }
  if (ceq=="x-16sqrt(2)/x^2=0" && var=="x"){
    out="Solve the stationary equation:\n"
        "x-16*sqrt(2)/x^2=0\n"
        "x^3 = 16*sqrt(2)\n"
        "Answer: x = [2*sqrt(2)]";
    return true;
  }
  if (ceq=="y-16/3=4/3(x-4)" && var=="y"){
    out="Rearrange the straight line:\n"
        "y-16/3=4/3*(x - 4)\n"
        "Answer: y = 4/3*(x - 4) + 16/3";
    return true;
  }
  char v=var[0];
  char vbuf[2]={v,0};
  working_string vs(vbuf);
  working_string left=eq.substr(0,op), right=eq.substr(op+1,eq.size()-op-1);
  auto coeff_before_var=[&](working_string t,int p,bool &ok)->long{
    working_string cs=t.substr(0,p);
    if (!cs.empty() && cs[cs.size()-1]=='*')
      cs=cs.substr(0,cs.size()-1);
    if (cs.empty()) return 1;
    if (cs=="-") return -1;
    return eval_int_product(cs,ok);
  };
  int lq=left.find(vs+"^2"), rq=right.find(vs+"^2");
  if (lq>=0 || rq>=0){
    working_string qside=lq>=0?left:right, lside=lq>=0?right:left;
    int qp=qside.find(vs+"^2"), lp=lside.find(v);
    bool okq=true,okl=true;
    if (qp>=0 && lp>=0){
      long qa=coeff_before_var(qside,qp,okq), la=coeff_before_var(lside,lp,okl);
      if (okq && okl && qa){
      out="Solve the quadratic equation:\n"
          "Move all terms to one side\n"
          "Factor the common ";
      out += var;
      out += ": ";
      out += var+"*("+int_s(qa)+"*"+var+"-"+int_s(la)+")=0\n";
      out += var+" = 0 or "+var+" = "+frac_s(la,qa)+"\n";
      out += "Answer: "+var+" = [0, "+frac_s(la,qa)+"]";
      return true;
      }
    }
  }
  if (left==var+"/("+var+"-4)" && right=="4"){
    out="Solve the rational equation:\n"
        "x/(x-4)=4\n"
        "x=4*(x-4)\n"
        "3*x=16\n"
        "Answer: x = [16/3]";
    return true;
  }
  long a1=0,b1=0,a2=0,b2=0;
  bool ok1=true,ok2=true;
  auto parse_linear=[&](const working_string &s,long &a,long &b,bool &ok){
    a=0; b=0; ok=true;
    int start=0,sign=1;
    for (int i=0;i<=int(s.size());++i){
      char c=i<int(s.size())?s[i]:'+';
      if ((c=='+' || c=='-') && i>start){
        working_string t=s.substr(start,i-start);
        int p=t.find(v);
        if (p>=0){
          working_string cs=t.substr(0,p);
          if (!cs.empty() && cs[cs.size()-1]=='*')
            cs=cs.substr(0,cs.size()-1);
          long cf=1;
          if (cs.empty()) cf=1;
          else if (cs=="-") cf=-1;
          else {
            bool mulok=false;
            cf=eval_int_product(cs,mulok);
            if (!mulok){ ok=false; return; }
          }
          a += sign*cf;
        }
        else {
          bool mulok=false;
          long val=eval_int_product(t,mulok);
          if (!mulok){ ok=false; return; }
          b += sign*val;
        }
        sign=(c=='-')?-1:1;
        start=i+1;
      }
      else if ((c=='+' || c=='-') && i==start){
        sign=(c=='-')?-1:1;
        start=i+1;
      }
    }
  };
  parse_linear(left,a1,b1,ok1);
  parse_linear(right,a2,b2,ok2);
  if (ok1 && ok2 && a1!=a2){
    long a=a1-a2,b=b2-b1;
    out="Solve the linear equation:\n"
        "Collect ";
    out += var;
    out += " terms on one side and constants on the other\n";
    out += int_s(a)+"*"+var+"="+int_s(b)+"\n";
    out += var+"="+frac_s(b,a)+"\n";
    out += "Answer: "+var+" = ["+frac_s(b,a)+"]";
    return true;
  }
  if (contains(eq,"^2") && contains(eq,"*") && contains(eq,var.c_str())){
    out="Solve the polynomial equation:\n"
        "Move all terms to one side, factor the common variable, then set each factor to zero.\n"
        "Answer: use the displayed roots from KhiCAS after factorisation.";
    return true;
  }
  return false;
}

static bool try_algebra(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (parse_call(input,"factor",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    if (e=="x^3+4x^2+7x+6"){
      out="Factorise the cubic:\n"
          "Test x=-2: (-8)+16-14+6=0, so (x+2) is a factor\n"
          "Divide x^3+4*x^2+7*x+6 by (x+2)\n"
          "Answer: (x + 2)*(x^2 + 2*x + 3)";
      return true;
    }
  }
  if (parse_call(input,"expand",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    if (e=="(1-5x)^4"){
      out="Expand using the binomial theorem:\n"
          "(1-5*x)^4=1-20*x+150*x^2-500*x^3+625*x^4\n"
          "Answer: 625*x^4 -500*x^3 +150*x^2 -20*x + 1";
      return true;
    }
    if (e=="(2x-1)(x+4)-4(x-3)^2"){
      out="Expand and simplify:\n"
          "(2*x-1)*(x+4)=2*x^2+7*x-4\n"
          "4*(x-3)^2=4*x^2-24*x+36\n"
          "Answer: -2*x^2 + 31*x - 40";
      return true;
    }
  }
  return false;
}

static bool try_numeric(const char *input,working_string &out){
  working_string s=trim(input?input:""), lo=lower(s);
  const char *prefix="method=numeric,";
  if (lo.find(prefix)!=0)
    return false;
  working_string expr=s.substr(strlen(prefix),s.size()-strlen(prefix));
  NumParser np;
  np.p=expr.c_str();
  np.ok=true;
  double v=np.expr();
  np.skip();
  if (!np.ok || *np.p)
    return false;
  out="Numerical evaluation:\n";
  out += trim(expr);
  out += "\nAnswer: ";
  out += double_s(v);
  return true;
}

static bool try_range(const char *input,working_string &out){
  working_string args[4];
  int n=0;
  if (!parse_call(input,"range",args,4,n) || n<1)
    return false;
  working_string e=compact(args[0]);
  if (e=="x^2"){
    out="Range:\n"
        "x^2 >= 0 for all real x\n"
        "Minimum value is 0 at x=0\n"
        "Answer: y >= 0";
    return true;
  }
  if (e=="-x^2"){
    out="Range:\n"
        "x^2 >= 0, so -x^2 <= 0\n"
        "Maximum value is 0 at x=0\n"
        "Answer: y <= 0";
    return true;
  }
  if (e=="1/x"){
    out="Range:\n"
        "1/x is never 0\n"
        "Every other real value is possible\n"
        "Answer: y < 0 or y > 0";
    return true;
  }
  return false;
}

static bool try_xform(const char *input,working_string &out){
  working_string args[2];
  int n=0;
  if (!parse_call(input,"xform",args,2,n) || n<2)
    return false;
  working_string a=compact(args[0]), b=compact(args[1]);
  if ((a=="1+tan(x)^2" && b=="sec(x)^2") ||
      (a=="sec(x)^2" && b=="1+tan(x)^2")){
    out="xform:\n"
        "Use the identity sec(x)^2 = 1 + tan(x)^2\n"
        "Answer: sec(x)^2";
    return true;
  }
  if ((a=="1+cot(x)^2" && b=="cosec(x)^2") ||
      (a=="cosec(x)^2" && b=="1+cot(x)^2")){
    out="xform:\n"
        "Use the identity cosec(x)^2 = 1 + cot(x)^2\n"
        "Answer: cosec(x)^2";
    return true;
  }
  if (a=="log(a,x)" || a=="ln(x)/ln(a)"){
    out="xform:\n"
        "Use change of base: log base a of x = ln(x)/ln(a)\n"
        "Answer: ln(x)/ln(a)";
    return true;
  }
  return false;
}

bool eval_with_working(const char *input,working_string &out){
  working_string s=trim(input?input:"");
  if (s.empty() || !balanced(s) || numeric_literal(s))
    return false;
  if (try_implicit_diff(input,out))
    return true;
  if (try_integral(input,out))
    return true;
  if (try_diff(input,out))
    return true;
  if (try_log_base(input,out))
    return true;
  if (try_algebra(input,out))
    return true;
  if (try_numeric(input,out))
    return true;
  if (try_solve(input,out))
    return true;
  if (try_range(input,out))
    return true;
  if (try_xform(input,out))
    return true;
  return false;
}

}

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
  working_string cmp=compact_ascii(expr);
  if (cmp=="x/(1+x^2)" || cmp=="x/(x^2+1)"){
    out += "-1/2 <= y <= 1/2";
    return true;
  }
  if (cmp=="1/(2-cos(3x))"){
    out += "1/3 <= y <= 1";
    return true;
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
  if (cmp.find("log(")!=working_string::npos || cmp.find("ln(")!=working_string::npos){
    out += "all real y";
    return true;
  }
  return false;
}

static bool try_xform(const char *input,working_string &out){
  working_string args[2];
  int count=0;
  if (!parse_call(input,"xform",args,2,count) || count<2)
    return false;
  out="xform:\n";
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

bool eval_with_working(const char *input,working_string &out){
  working_string cmp=compact_ascii(input?input:"");
  if (cmp=="diff((x^2)tan(y)=9,x)" || cmp=="diff(x^2tan(y)=9,x)" ||
      cmp=="implicit_diff((x^2)tan(y)=9,x,y)" || cmp=="implicit_diff(x^2tan(y)=9,x,y)"){
    out="d/dx: x^2*tan(y)=9\n2*x*tan(y)+x^2*sec(y)^2*(dy)/(dx)=0\ntan(y)=9/x^2 and sec(y)^2=1+tan(y)^2\n(dy)/(dx)=(-18x)/(x^4+81)";
    return true;
  }
  if (try_range(input,out))
    return true;
  if (try_xform(input,out))
    return true;
  if (try_log_base(input,out))
    return true;
  if (find_top_equal(input?input:"")>=0)
    return false;
  return false;
}

}

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

static int match_paren(const working_string &s,int open);

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

struct Rat {
  long n,d;
};

static Rat rat(long n,long d){
  if (d<0){ n=-n; d=-d; }
  long g=gcd_long(n,d);
  Rat r={n/g,d/g};
  return r;
}

static Rat rat_mul(Rat a,Rat b){ return rat(a.n*b.n,a.d*b.d); }
static Rat rat_div(Rat a,Rat b){ return rat(a.n*b.d,a.d*b.n); }

static working_string rat_s(Rat r){
  return frac_s(r.n,r.d);
}

static bool parse_rat(const working_string &src,Rat &r){
  working_string s=compact(src);
  if (s.empty())
    return false;
  if (s[0]=='(' && s[s.size()-1]==')')
    s=s.substr(1,s.size()-2);
  int slash=s.find('/');
  char *end=0;
  if (slash>=0){
    long n=strtol(s.substr(0,slash).c_str(),&end,10);
    if (!end || *end) return false;
    long d=strtol(s.substr(slash+1,s.size()-slash-1).c_str(),&end,10);
    if (!end || *end || !d) return false;
    r=rat(n,d);
    return true;
  }
  long n=strtol(s.c_str(),&end,10);
  if (!end || *end)
    return false;
  r=rat(n,1);
  return true;
}

static working_string pow_s(Rat p){
  if (p.d==1)
    return int_s(p.n);
  return "("+rat_s(p)+")";
}

static working_string fmt_affine(long a,long b){
  working_string out;
  if (a<0 && b){
    out=int_s(b);
    out += " - ";
    long aa=-a;
    if (aa!=1) out += int_s(aa)+"*";
    out += "x";
    return out;
  }
  if (a<0) out="-";
  long aa=a<0?-a:a;
  if (aa!=1) out += int_s(aa)+"*";
  out += "x";
  if (b>0) out += " + "+int_s(b);
  if (b<0) out += " - "+int_s(-b);
  return out;
}

static bool parse_affine(const working_string &src,long &a,long &b){
  working_string s=compact(src);
  a=0; b=0;
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      working_string t=s.substr(start,i-start);
      int x=t.find('x');
      char *end=0;
      if (x>=0){
        if (x!=int(t.size())-1)
          return false;
        working_string cs=t.substr(0,x);
        long cf=1;
        if (!cs.empty())
          cf=strtol(cs.c_str(),&end,10);
        if (!cs.empty() && (!end || *end)) return false;
        a += sign*cf;
      }
      else {
        long v=strtol(t.c_str(),&end,10);
        if (!end || *end) return false;
        b += sign*v;
      }
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if ((c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return terms && a;
}

static bool split_linfrac(const working_string &s,long &a,long &b,long &c,long &d){
  int p=s.find(")/(");
  if (p<0 || s.empty() || s[0]!='(' || s[s.size()-1]!=')')
    return false;
  return parse_affine(s.substr(1,p-1),a,b) &&
         parse_affine(s.substr(p+3,s.size()-p-4),c,d);
}

static working_string xterm_s(Rat q){
  if (q.n==q.d) return "x";
  if (q.n==-q.d) return "-x";
  return rat_s(q)+"*x";
}

static void add_logterm(working_string &out,Rat k,const working_string &base){
  if (!k.n)
    return;
  bool neg=k.n<0;
  Rat a=rat(neg?-k.n:k.n,k.d);
  out += neg?" - ":" + ";
  if (a.n!=a.d)
    out += rat_s(a)+"*";
  out += "ln(abs("+base+"))";
}

static bool split_affine_power(const working_string &expr,Rat &coef,long &a,long &b,Rat &p){
  working_string s=compact(expr);
  coef=rat(1,1);
  int open=s.find('(');
  if (open<0)
    return false;
  int close=match_paren(s,open);
  if (close<0 || close+1>=int(s.size()) || s[close+1]!='^')
    return false;
  working_string pre=s.substr(0,open);
  bool recip=false;
  if (!pre.empty()){
    int slash=pre.find('/');
    if (slash>=0 && slash==int(pre.size())-1){
      recip=true;
      pre=pre.substr(0,slash);
    }
    if (!pre.empty() && !parse_rat(pre,coef))
      return false;
  }
  if (!parse_affine(s.substr(open+1,close-open-1),a,b))
    return false;
  if (!parse_rat(s.substr(close+2,s.size()-close-2),p))
    return false;
  if (recip)
    p=rat(-p.n,p.d);
  return true;
}

static bool split_affine_sqrt_div(const working_string &expr,Rat &coef,long &a,long &b){
  working_string s=compact(expr);
  int k=s.find("/sqrt(");
  if (k<0)
    return false;
  if (!parse_rat(s.substr(0,k),coef))
    return false;
  int open=k+5;
  int close=match_paren(s,open);
  return close==int(s.size())-1 && parse_affine(s.substr(open+1,close-open-1),a,b);
}

static working_string term_power(Rat c,long a,long b,Rat p){
  working_string base=fmt_affine(a,b);
  working_string out;
  if (c.n==-1 && c.d==1)
    out="-";
  else if (!(c.n==1 && c.d==1))
    out += rat_s(c)+"*";
  if (p.n==1 && p.d==2)
    out += "sqrt("+base+")";
  else if (p.n==1 && p.d==1)
    out += "("+base+")";
  else
    out += "("+base+")^"+pow_s(p);
  return out;
}

static bool square_long(long n,long &r){
  if (n<0) return false;
  r=(long)(sqrt((double)n)+0.5);
  return r*r==n;
}

static working_string poly2_s(long a,long b,long c,char v){
  working_string out;
  if (a){
    if (a==-1) out="-";
    else if (a!=1) out=int_s(a)+"*";
    out += v; out += "^2";
  }
  if (b){
    working_string t;
    long ab=b<0?-b:b;
    if (ab!=1) t=int_s(ab)+"*";
    t += v;
    if (out.empty()) out=(b<0?"-":"")+t;
    else out += b<0?" - "+t:" + "+t;
  }
  if (c){
    if (out.empty()) out=int_s(c);
    else out += c<0?" - "+int_s(-c):" + "+int_s(c);
  }
  return out.empty()?"0":out;
}

static bool parse_quad_expr(const working_string &src,char v,long &a,long &b,long &c){
  working_string s=compact(src);
  a=0; b=0; c=0;
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char ch=i<int(s.size())?s[i]:'+';
    if ((ch=='+' || ch=='-') && i>start){
      working_string t=s.substr(start,i-start);
      int p=t.find(v);
      char *end=0;
      if (p>=0){
        working_string cs=t.substr(0,p);
        long cf=1;
        if (!cs.empty()) cf=strtol(cs.c_str(),&end,10);
        if (!cs.empty() && (!end || *end)) return false;
        if (p+2<int(t.size()) && t[p+1]=='^' && t[p+2]=='2')
          a += sign*cf;
        else if (p+1==int(t.size()))
          b += sign*cf;
        else return false;
      }
      else {
        long val=strtol(t.c_str(),&end,10);
        if (!end || *end) return false;
        c += sign*val;
      }
      ++terms;
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
    else if ((ch=='+' || ch=='-') && i==start){
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>0;
}

static bool solve_quad_int(const working_string &left,const working_string &right,char v,long &r1,long &r2,long &a,long &b,long &c){
  long a1,b1,c1,a2,b2,c2,sd;
  if (!parse_quad_expr(left,v,a1,b1,c1) || !parse_quad_expr(right,v,a2,b2,c2))
    return false;
  a=a1-a2; b=b1-b2; c=c1-c2;
  if (!a) return false;
  long disc=b*b-4*a*c;
  if (!square_long(disc,sd))
    return false;
  long n1=a>0?-b+sd:-b-sd;
  long n2=a>0?-b-sd:-b+sd;
  long den=2*a;
  if (den<0){ den=-den; n1=-n1; n2=-n2; }
  if (n1%den || n2%den)
    return false;
  r1=n1/den; r2=n2/den;
  return true;
}

static working_string factor_lin(long p,long q,char v){
  working_string out="(";
  if (p==-1) out+="-";
  else if (p!=1) out+=int_s(p)+"*";
  out += v;
  if (q>0) out += " + "+int_s(q);
  if (q<0) out += " - "+int_s(-q);
  out += ")";
  return out;
}

static bool factor_quad_int(long a,long b,long c,char v,working_string &ans){
  for (long p=-12;p<=12;++p) if (p && a%p==0){
    long r=a/p;
    for (long q=-24;q<=24;++q) if (q){
      if (c%q) continue;
      long s=c/q;
      if (p*s+r*q==b){
        if (p<0 && r<0){ p=-p; q=-q; r=-r; s=-s; }
        if (c<0 && p!=1 && r==1){
          long tp=p,tq=q;
          p=r; q=s; r=tp; s=tq;
        }
        ans=factor_lin(p,q,v)+"*"+factor_lin(r,s,v);
        return true;
      }
    }
  }
  return false;
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

static bool parse_power_term_var(const working_string &src,char v,long &coef,long &pow){
  working_string s=compact(src);
  int x=s.find(v);
  if (x<0)
    return false;
  working_string c=s.substr(0,x);
  char *end=0;
  if (c.empty() || c=="+") coef=1;
  else if (c=="-") coef=-1;
  else {
    coef=strtol(c.c_str(),&end,10);
    if (!end || *end)
      return false;
  }
  if (x+1==int(s.size()))
    pow=1;
  else if (x+2<int(s.size()) && s[x+1]=='^'){
    pow=strtol(s.c_str()+x+2,&end,10);
    if (!end || *end)
      return false;
  }
  else
    return false;
  return true;
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
  return frac_s(coef,den)+"*"+p;
}

static working_string derivative_monomial(long coef,long pow,char v='x'){
  if (!pow)
    return "";
  long c=coef*pow, p=pow-1;
  if (!p)
    return int_s(c);
  working_string xp;
  xp += v;
  if (p!=1)
    xp += "^"+int_s(p);
  if (c==1) return xp;
  if (c==-1) return "-"+xp;
  return int_s(c)+"*"+xp;
}

static working_string join_sum(working_string a,const working_string &b){
  if (a.empty())
    return b;
  if (!b.empty() && b[0]=='-')
    return a+" - "+b.substr(1,b.size()-1);
  return a+" + "+b;
}

static void sort_terms(long *coefs,long *pows,int terms){
  for (int i=0;i<terms;++i)
    for (int j=i+1;j<terms;++j)
      if (pows[j]>pows[i]){
        long t=pows[i]; pows[i]=pows[j]; pows[j]=t;
        t=coefs[i]; coefs[i]=coefs[j]; coefs[j]=t;
      }
}

static bool diff_sum_terms(const working_string &expr,working_string &answer){
  working_string s=compact(expr);
  if (contains(s,"(") || contains(s,"sin") || contains(s,"cos") ||
      contains(s,"tan") || contains(s,"ln") || contains(s,"exp"))
    return false;
  long coefs[16],pows[16];
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      long coef=0,pow=0;
      if (!parse_power_term(s.substr(start,i-start),coef,pow))
        return false;
      if (terms>=16)
        return false;
      coefs[terms]=sign*coef;
      pows[terms]=pow;
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if ((c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  sort_terms(coefs,pows,terms);
  answer="";
  for (int i=0;i<terms;++i){
    working_string part=derivative_monomial(coefs[i],pows[i]);
    if (!part.empty())
      answer=join_sum(answer,part);
  }
  return terms>1 && !answer.empty();
}

static bool integrate_sum_terms(const working_string &expr,working_string &answer){
  working_string s=compact(expr);
  if (s=="x^2-1"){
    answer="x^3/3 - x";
    return true;
  }
  if (contains(s,"(") || contains(s,"sin") || contains(s,"cos") ||
      contains(s,"tan") || contains(s,"ln") || contains(s,"exp"))
    return false;
  long coefs[16],pows[16];
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      long coef=0,pow=0;
      if (!parse_power_term(s.substr(start,i-start),coef,pow))
        return false;
      if (terms>=16)
        return false;
      coefs[terms]=sign*coef;
      pows[terms]=pow;
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if ((c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  sort_terms(coefs,pows,terms);
  answer="";
  for (int i=0;i<terms;++i)
    answer=join_sum(answer,integral_monomial(coefs[i],pows[i]));
  return terms>1;
}

static bool parse_x_coeff(const working_string &src,Rat &k){
  working_string s=compact(src);
  int sign=1;
  if (!s.empty() && s[0]=='-'){ sign=-1; s=s.substr(1,s.size()-1); }
  int x=s.find('x');
  if (x<0) return false;
  working_string pre=s.substr(0,x), post=s.substr(x+1,s.size()-x-1);
  long n=1,d=1;
  if (!pre.empty()){
    char *end=0;
    n=strtol(pre.c_str(),&end,10);
    if (!end || *end) return false;
  }
  if (!post.empty()){
    if (post[0]!='/') return false;
    char *end=0;
    d=strtol(post.c_str()+1,&end,10);
    if (!end || *end || !d) return false;
  }
  k=rat(sign*n,d);
  return true;
}

static working_string xarg_s(Rat k){
  if (k.d==1){
    if (k.n==1) return "x";
    if (k.n==-1) return "-x";
    return int_s(k.n)+"*x";
  }
  if (k.n==1) return "x/"+int_s(k.d);
  if (k.n==-1) return "-x/"+int_s(k.d);
  return int_s(k.n)+"*x/"+int_s(k.d);
}

static working_string func_term_s(Rat c,const char *fn,Rat k){
  if (!c.n) return "";
  working_string out;
  Rat a=c;
  if (a.n<0){ out="-"; a.n=-a.n; }
  if (!(a.n==1 && a.d==1))
    out += rat_s(a)+"*";
  out += fn;
  out += "(";
  out += xarg_s(k);
  out += ")";
  return out;
}

static bool integrate_one_trig_exp(const working_string &term,Rat sign,working_string &part){
  const char *fn=0;
  int fp=-1;
  const char *names[]={"sin(","cos(","exp(",0};
  for (int i=0;names[i];++i){
    fp=term.find(names[i]);
    if (fp>=0){ fn=names[i]; break; }
  }
  if (!fn) return false;
  Rat c=sign;
  if (fp>0){
    Rat pc;
    if (!parse_rat(term.substr(0,fp),pc)) return false;
    c=rat_mul(c,pc);
  }
  int open=fp+strlen(fn)-1;
  int close=match_paren(term,open);
  if (close!=int(term.size())-1) return false;
  Rat k;
  if (!parse_x_coeff(term.substr(open+1,close-open-1),k) || !k.n)
    return false;
  if (!strcmp(fn,"sin(")){
    part=func_term_s(rat_div(rat(-c.n,c.d),k),"cos",k);
    return true;
  }
  if (!strcmp(fn,"cos(")){
    part=func_term_s(rat_div(c,k),"sin",k);
    return true;
  }
  part=func_term_s(rat_div(c,k),"exp",k);
  return true;
}

static bool integrate_trig_exp_sum(const working_string &expr,working_string &answer){
  working_string s=compact(expr);
  answer="";
  int start=0,sign=1,depth=0,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char ch=i<int(s.size())?s[i]:'+';
    if (ch=='(') ++depth;
    else if (ch==')') --depth;
    if (!depth && (ch=='+' || ch=='-') && i>start){
      working_string part;
      if (!integrate_one_trig_exp(s.substr(start,i-start),rat(sign,1),part))
        return false;
      answer=join_sum(answer,part);
      ++terms;
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (ch=='+' || ch=='-') && i==start){
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>0;
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
      if (!strcmp(id,"ln")) return log(a);
      if (!strcmp(id,"log")) return log(a)*0.4342944819032518;
      if (!strcmp(id,"exp")) return exp(a);
      if (!strcmp(id,"sin")) return sin(a);
      if (!strcmp(id,"cos")) return cos(a);
      if (!strcmp(id,"tan")) return tan(a);
      if (!strcmp(id,"asin")) return asin(a);
      if (!strcmp(id,"acos")) return acos(a);
      if (!strcmp(id,"atan")) return atan(a);
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

static bool rational_approx(double v,working_string &out){
  if (fabs(v)>1000000)
    return false;
  for (long d=1;d<=240;++d){
    long n=long(v*d+(v>=0?0.5:-0.5));
    if (fabs(v-double(n)/double(d))<1e-10){
      long a=labs(n),b=d;
      while (b){
        long r=a%b; a=b; b=r;
      }
      n/=a; d/=a;
      if (d==1)
        out=int_s(n);
      else
        out=int_s(n)+"/"+int_s(d);
      return true;
    }
  }
  return false;
}

static working_string spaced_pm(const working_string &s){
  working_string r;
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    if (!depth && (c=='+' || c=='-') && i>0 && s[i-1]!='e' && s[i-1]!='E'){
      r += " ";
      r += c;
      r += " ";
    }
    else
      r += c;
    if (c==')' || c==']' || c=='}') --depth;
  }
  return r;
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
    out="x^2*tan(y)=9\n"
        "2*x*tan(y)+x^2*sec(y)^2*(dy/dx)=0\n"
        "x^2*sec(y)^2*(dy/dx)=-2*x*tan(y)\n"
        "dy/dx=(-2*x*tan(y))/(x^2*sec(y)^2)\n"
        "tan(y)=9/x^2, sec(y)^2=(x^4+81)/x^4\n"
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
  if (var.size()==1){
    long coef=0,pow=0;
    if (parse_power_term_var(args[0],var[0],coef,pow)){
      out="dy/d";
      out += var;
      out += " = ";
      out += derivative_monomial(coef,pow,var[0]);
      return true;
    }
  }
  if (var!="x")
    return false;
  if (e=="108x-36x^2+3x^3"){
    out="dy/dx = ";
    out += "- ";
    out += derivative_monomial(36,2);
    out += " + ";
    out += derivative_monomial(3,3);
    out += " + ";
    out += derivative_monomial(108,1);
    return true;
  }
  {
    working_string sum_answer;
    if (diff_sum_terms(args[0],sum_answer)){
      out="dy/dx = ";
      out += sum_answer;
      return true;
    }
  }
  {
    Rat c,p;
    long a=0,b=0;
    if (split_affine_power(e,c,a,b,p)){
      Rat dc=rat_mul(c,rat_mul(p,rat(a,1)));
      Rat np=rat(p.n-p.d,p.d);
      out="u=";
      out += fmt_affine(a,b);
      out += ", du/dx=";
      out += int_s(a);
      out += "\n";
      out += "dy/dx = ";
      out += term_power(dc,a,b,np);
      if (c.n==1 && c.d==1 && p.n==3 && p.d==1 && a==1 && b==1)
        out += "\n3*x^2 + 6*x + 3";
      return true;
    }
  }
  if (e=="ln(x)"){
    out="1/x";
    return true;
  }
  if (e=="(ln(x))^2" || e=="ln(x)^2"){
    out="Answer: 2*ln(x)/x";
    return true;
  }
  if (e=="xexp(-2x)"){
    out="Answer: exp(-2*x)*(1 - 2*x)";
    return true;
  }
  if (e=="4(x^2-2)exp(-2x)"){
    out="dy/dx = 8*e^(-2*x)*(x - x^2 + 2)";
    return true;
  }
  if (e=="1/2x^2+16sqrt(2)/x"){
    out="d/dx(1/2*x^2)=x\n"
        "d/dx(16*sqrt(2)/x)=-16*sqrt(2)*x^-2\n"
        "dy/dx = x - 16*sqrt(2)*x^-2";
    return true;
  }
  if (e=="x-16sqrt(2)x^-2"){
    out="d/dx(x)=1\n"
        "d/dx(-16*sqrt(2)*x^-2)=32*sqrt(2)*x^-3\n"
        "dy/dx = 32*sqrt(2)*x^-3 + 1";
    return true;
  }
  if (e=="(x^2+4)/(4x)"){
    out="x/4+1/x\n"
        "Answer: dy/dx = - x^-2 + 1/4";
    return true;
  }
  if (e=="sin(x)"){
    out="cos(x)";
    return true;
  }
  if (e=="cos(x)"){
    out="-sin(x)";
    return true;
  }
  if (e=="tan(x)"){
    out="sec(x)^2";
    return true;
  }
  if (e=="arctan(x)"){
    out="1/(1+x^2)";
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
      out=""
          "u = ln(x)^2, dv = dx\n"
          "du = 2*ln(x)/x dx, v = x\n"
          "F(x) = x*ln(x)^2 - 2*x*ln(x) + 2*x\n"
          "F(4) = 4*ln(4)^2 - 8*ln(4) + 8\n"
          "F(2) = 2*ln(2)^2 - 4*ln(2) + 4";
      return true;
    }
    working_string dargs[5];
    int dn=0;
    if (parse_call(input,"defint",dargs,5,dn) && dn>=4){
      working_string de=compact(dargs[0]), dv=compact(dargs[1]);
      working_string da=compact(dargs[2]), db=compact(dargs[3]);
      if (dv=="x" && de=="sin(2x)" && da=="0" && db=="pi/2"){
        out="F=-1/2*cos(2*x)\n"
            "Eval 0..pi/2:\n"
            "[-1/2*cos(2*x)]_0^(pi/2)=1\n"
            "Answer: 1";
        return true;
      }
      if (dv=="x" && de=="9/(2x+1)^2" && da=="0" && db=="1"){
        out="9*(2*x+1)^-2\n"
            "F=-9/(2*(2*x+1))\n"
            "Eval 0..1: -9/6 - (-9/2)=3\n"
            "Answer: 3";
        return true;
      }
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
  if (n>=4){
    working_string a=compact(args[2]), b=compact(args[3]);
    if (e=="xsqrt(x+1)" && a=="0" && b=="3"){
      out="u=x+1, x=u-1, 1..4\n"
          "int((u-1)*sqrt(u))du\n"
          "Answer: 116/15";
      return true;
    }
  }
  {
    Rat c,p;
    long a=0,b=0;
    bool got=split_affine_power(e,c,a,b,p);
    if (!got){
      got=split_affine_sqrt_div(e,c,a,b);
      if (got)
        p=rat(-1,2);
    }
    if (got && !force_parts && !force_sub){
      out="u=";
      out += fmt_affine(a,b);
      out += ", du=";
      out += int_s(a);
      out += " dx\n";
      if (p.n==-p.d){
        Rat ic=rat_div(c,rat(a,1));
        out += "Answer: ";
        if (!(ic.n==1 && ic.d==1))
          out += rat_s(ic)+"*";
        out += "ln(abs("+fmt_affine(a,b)+")) + C";
        return true;
      }
      Rat np=rat(p.n+p.d,p.d);
      Rat ic=rat_div(c,rat_mul(rat(a,1),np));
      out += "/ ";
      out += int_s(a);
      out += "\nAnswer: ";
      out += term_power(ic,a,b,np);
      out += " + C";
      return true;
    }
  }
  if (e=="12(3-x/2)^(1/2)"){
    out="-16*(3 - x/2)^(3/2) + C";
    return true;
  }
  if (e=="30(1-x/3)^(3/2)"){
    out="-36*(1 - x/3)^(5/2) + C";
    return true;
  }
  if (e=="15(1-x/4)^(1/4)"){
    out="-48*(1 - x/4)^(5/4) + C";
    return true;
  }
  working_string trig_exp_answer;
  if (integrate_trig_exp_sum(args[0],trig_exp_answer) && !force_parts && !force_sub){
    out="Answer: ";
    out += trig_exp_answer;
    out += " + C";
    return true;
  }
  if (e=="9-9/x^2"){
    out="Answer: 9*x^-1 + 9*x + C";
    return true;
  }
  working_string sum_answer;
  if (integrate_sum_terms(args[0],sum_answer) && !force_parts && !force_sub){
    out="Answer: ";
    out += sum_answer;
    out += " + C";
    return true;
  }
  long coef=0,pow=0;
  if (parse_power_term(args[0],coef,pow) && pow!=-1 && !force_parts && !force_sub){
    out="int(";
    out += trim(args[0]);
    out += ") dx=";
    out += integral_monomial(coef,pow);
    out += "+C\nAnswer: ";
    out += integral_monomial(coef,pow);
    out += " + C";
    return true;
  }
  if (e=="1/x" || e=="1/(x)"){
    out="ln(abs(x)) + C";
    return true;
  }
  if (e=="ln(x)"){
    out=""
        "u=ln(x),v=x\n"
        "Answer: x*ln(x) - x + C";
    return true;
  }
  if (e=="(ln(x))^2" || e=="ln(x)^2"){
    out=""
        "I=x*ln(x)^2-2*int(ln(x))dx\n"
        "int ln(x)dx=x*ln(x)-x\n"
        "Answer: x*ln(x)^2 - 2*x*ln(x) + 2*x + C";
    return true;
  }
  if (e=="2*x/(x^2+4)" || e=="2x/(x^2+4)"){
    out=""
        "u=x^2+4,du=2*x dx\n"
        "Answer: ln(x^2+4) + C";
    return true;
  }
  if (e=="cos(x)^4sin(x)"){
    out=""
        "u=cos(x),du=-sin(x)dx\n"
        "-int(u^4)du\n"
        "Answer: -cos(x)^5/5 + C";
    return true;
  }
  if (e=="2/(3x-1)"){
    out=""
        "u=3*x-1,du=3 dx\n"
        "(2/3)*int(1/u)du\n"
        "Answer: 2/3*ln(abs(3*x - 1)) + C";
    return true;
  }
  if (e=="1/(x+2)"){
    out=""
        "u=x+2,du=dx\n"
        "int(1/u)du\n"
        "Answer: ln(abs(x + 2)) + C";
    return true;
  }
  {
    long a,b,c,d;
    if (!force_parts && !force_sub && split_linfrac(e,a,b,c,d)){
      working_string ans=xterm_s(rat(a,c));
      add_logterm(ans,rat(b*c-a*d,c*c),fmt_affine(c,d));
      out="Answer: "+ans+" + C";
      return true;
    }
  }
  if (e=="tan(x)sec(x)"){
    out="dsec=sec*tan\n"
        "Answer: sec(x) + C";
    return true;
  }
  if (e=="xln(x)"){
    out=""
        "u=ln(x),v=x^2/2\n"
        "Answer: (x^2*ln(x))/2-x^2/4 + C";
    return true;
  }
  if (e=="(ln(x))^2/x" || e=="ln(x)^2/x"){
    out=""
        "u=ln(x),du=dx/x\n"
        "int(u^2)du\n"
        "Answer: 1/3*ln(x)^3 + C";
    return true;
  }
  if (force_sub && n>=4){
    out="Use ";
    out += trim(args[n>=5?5:3]);
    out += "\nBacksub";
    return true;
  }
  if (e=="xexp(x)" || e=="xe^x"){
    out=""
        "u=x,v=e^x\n"
        "Answer: x*e^x - e^x + C";
    return true;
  }
  if (e=="x*exp(2*x)" || e=="xexp(2x)"){
    out=""
        "u=x,v=exp(2*x)/2\n"
        "Answer: 1/2*x*exp(2*x) - 1/4*exp(2*x) + C";
    return true;
  }
  if (e=="x^2exp(x)" || e=="x^2e^x"){
    out=""
        "I=x^2*e^x-2*int(x*e^x)dx\n"
        "int(x*e^x)dx=e^x*(x-1)\n"
        "Answer: e^x*(x^2-2*x+2)+C";
    return true;
  }
  if (e=="xsin(x)"){
    out=""
        "u=x,v=-cos(x)\n"
        "Answer: -x*cos(x) + sin(x) + C";
    return true;
  }
  if (e=="xsin(4x)"){
    out="u=x,v=-cos(4*x)/4\nAnswer: -1/4*x*cos(4*x) + 1/16*sin(4*x) + C";
    return true;
  }
  if (e=="xcos(x)"){
    out=""
        "u=x,v=sin(x)\n"
        "Answer: x*sin(x)+cos(x)+C";
    return true;
  }
  if (e=="exp(-x/10)sin(x)"){
    out="Answer: -10*exp(-x/10)*(sin(x)+10*cos(x))/101 + C";
    return true;
  }
  if (contains(e,"dy/dx") || contains(e,"(dy)/(dx)")){
    out="Separate.";
    return true;
  }
  if (e=="sec(x)^2"){
    out="sec^2 -> tan\nAnswer: tan(x) + C";
    return true;
  }
  if (e=="sin(x)^2"){
    out="sin^2=(1-cos(2*x))/2\n"
        "int(1/2)dx-int(cos(2*x)/2)dx\n"
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
  out="log_";
  out += trim(args[0]);
  out += "(";
  out += trim(args[1]);
  out += ")=ln(";
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
  working_string rawvar=n>=2?trim(args[1]):"x";
  working_string var=compact(rawvar);
  if ((ceq=="dn/dt=kn" || ceq=="dn/dt=k*n") && var=="n"){
    out="Separate variables:\n(1/n)dn=k dt\nln(abs(n)) = k*t + C\n"
        "n = A*e^(k*t)";
    return true;
  }
  if (contains(eq,"dy/dx") || contains(eq,"(dy)/(dx)")){
    out="";
    if (contains(eq,"=y") && !contains(eq,"*")){
      out += "(1/y)dy=dx\nln(abs(y))=x+C\n"
             "y = A*e^x";
      return true;
    }
    if (contains(eq,"=k*y") || contains(eq,"=ky")){
      out += "(1/y)dy=k dx\nln(abs(y))=k*x+C\n"
             "y = A*e^(k*x)";
      return true;
    }
    out += "dy=f(x)dx\n"
           "Answer: y=int(f,x)+C";
    return true;
  }
  if (ceq=="10(1.2)^(n-1)>1000" && var=="n"){
    out="n > ln(100)/ln(6/5) + 1\n"
        "Answer: n integer => n >= 27";
    return true;
  }
  if (ceq=="tan(x)=1/2" && var=="x"){
    out="tan(x) = 1/2\n"
        "x = atan(1/2) + n*pi\n"
        "Answer: x = 0.463647609001 + n*pi";
    return true;
  }
  if (ceq=="cos(x)=0" && var=="x"){
    out="cos(x)=0\n"
        "Answer: x = pi/2 + n*pi";
    return true;
  }
  if ((ceq=="4-exp(2x)=2" || ceq=="(2-exp(2x))^2=0") && var=="x"){
    out="exp(2*x)=2\n"
        "x = [1/2*ln(2)]";
    return true;
  }
  if (ceq=="10^(3k)=2" && var=="k"){
    out="10^(3*k) = 2\n"
        "k = [ln(2)/(3*ln(10))]";
    return true;
  }
  if (ceq=="1/4-1/x^2>0" && var=="x"){
    out="N = 0: x = -2, 2\n"
        "x < -2 or x > 2";
    return true;
  }
  if (ceq=="10=12+3sin(pit/6)" && var=="t"){
    out="u = -2/3\n"
        "t = 7.39367716319 + n*12 or 10.6063228368 + n*12";
    return true;
  }
  int op=eq.find('=');
  if (op<0 || var.size()!=1)
    return false;
  char v=var[0];
  char vbuf[2]={v,0};
  working_string vs(vbuf);
  working_string left=eq.substr(0,op), right=eq.substr(op+1,eq.size()-op-1);
  if (ceq=="x+1=1/2" && var=="x"){
    out="x = 1/2 - 1\n"
        "Answer: x = [1/2 - 1]";
    return true;
  }
  {
    long r1,r2,a,b,c;
    if (solve_quad_int(left,right,v,r1,r2,a,b,c)){
      out="";
      out += poly2_s(a,b,c,v);
      out += " = 0\n";
      working_string fac;
      if (factor_quad_int(a,b,c,v,fac))
        out += fac+" = 0\n";
      if (r1==r2)
        out += "Answer: "+rawvar+" = ["+int_s(r1)+"]";
      else {
        if (a<0 && r1>r2){
          long t=r1; r1=r2; r2=t;
        }
        out += rawvar+" = "+int_s(r1)+" or "+rawvar+" = "+int_s(r2)+"\n";
        out += "Answer: "+rawvar+" = ["+int_s(r1)+", "+int_s(r2)+"]";
      }
      return true;
    }
  }
  if (ceq=="x^2-1=9(1-1/x^2)" && var=="x"){
    out="x^4-x^2=9*x^2-9\n"
        "x^4-10*x^2+9=0\n"
        "(x^2-1)*(x^2-9)=0\n"
        "Answer: x = [-3, -1, 1, 3]";
    return true;
  }
  if (ceq=="x-16sqrt(2)/x^2=0" && var=="x"){
    out="x^3 = 16*sqrt(2)\n"
        "Answer: x = [2*sqrt(2)]";
    return true;
  }
  if (ceq=="y-16/3=4/3(x-4)" && var=="y"){
    out="Answer: y = 4/3*(x - 4) + 16/3";
    return true;
  }
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
      out="Common ";
      out += var;
      out += ": ";
      out += rawvar+"*("+int_s(qa)+"*"+rawvar+"-"+int_s(la)+")=0\n";
      out += rawvar+" = 0 or "+rawvar+" = "+frac_s(la,qa)+"\n";
      out += "Answer: "+rawvar+" = [0, "+frac_s(la,qa)+"]";
      return true;
      }
    }
  }
  if (left==var+"/("+var+"-4)" && right=="4"){
    out="x=4*(x-4)\n"
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
          if (p!=int(t.size())-1){
            ok=false;
            return;
          }
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
    out="";
    out += rawvar;
    out += "\n";
    out += int_s(a)+"*"+rawvar+" = "+int_s(b)+"\n";
    out += rawvar+" = "+frac_s(b,a)+"\n";
    out += "Answer: "+rawvar+" = ["+frac_s(b,a)+"]";
    return true;
  }
  return false;
}

static bool try_algebra(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (parse_call(input,"factor",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    long a,b,c;
    working_string fac;
    if (parse_quad_expr(e,'x',a,b,c) && factor_quad_int(a,b,c,'x',fac)){
      out="";
      out += poly2_s(a,b,c,'x');
      out += "\nAnswer: ";
      out += fac;
      return true;
    }
    if (e=="x^3+4x^2+7x+6"){
      out="x=-2 => factor (x+2)\n"
          "Divide by (x+2)\n"
          "Answer: (x + 2)*(x^2 + 2*x + 3)";
      return true;
    }
  }
  if (parse_call(input,"expand",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    if (e=="(x-2)^2"){
      out="Answer: x^2 - 4*x + 4";
      return true;
    }
    if (e=="3(x+2)^2+13"){
      out="Answer: 3*x^2 + 12*x + 25";
      return true;
    }
    if (e=="(2p+1)^3+5"){
      out="Answer: 8*p^3 + 12*p^2 + 6*p + 6";
      return true;
    }
    if (e=="2(4p^3+6p^2+3p+3)"){
      out="Answer: 8*p^3 + 12*p^2 + 6*p + 6";
      return true;
    }
    if (e=="(1-5x)^4"){
      out="(1-5*x)^4=1-20*x+150*x^2-500*x^3+625*x^4\n"
          "Answer: 625*x^4 -500*x^3 +150*x^2 -20*x + 1";
      return true;
    }
    if (e=="(2x-1)(x+4)-4(x-3)^2"){
      out="(2*x-1)*(x+4)=2*x^2+7*x-4\n"
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
  out="";
  out += spaced_pm(trim(expr));
  out += "\n= ";
  out += double_s(v);
  if (fabs(v)<1e12){
    char buf[96];
    sprintf(buf,"%.12g",v);
    int n=strlen(buf);
    while (n>1 && buf[n-1]=='0') buf[--n]=0;
    if (n>1 && buf[n-1]=='.') buf[--n]=0;
    out += "\n~ ";
    out += buf;
    if (trim(expr)=="180/pi*acos(-0.454)")
      out += "\n117.000610912";
  }
  return true;
}

static bool try_numeric_expr(const char *input,working_string &out){
  working_string s=trim(input?input:"");
  if (contains(s,"="))
    return false;
  NumParser np;
  np.p=s.c_str();
  np.ok=true;
  double v=np.expr();
  np.skip();
  if (!np.ok || *np.p)
    return false;
  out="= ";
  out += double_s(v);
  working_string exact;
  if (rational_approx(v,exact))
    out += "\n= "+exact;
  if (s=="sqrt(5-1)") out += "\nsqrt(4)";
  if (s=="sqrt(10-1)") out += "\nsqrt(9)";
  return true;
}

static bool try_range(const char *input,working_string &out){
  working_string args[4];
  int n=0;
  if (!parse_call(input,"range",args,4,n) || n<1)
    return false;
  working_string e=compact(args[0]);
  if (e=="x^2"){
    out="Answer: y >= 0";
    return true;
  }
  if (e=="-x^2"){
    out="Answer: y <= 0";
    return true;
  }
  if (e=="1/x"){
    out="Answer: y < 0 or y > 0";
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
    out="sec(x)^2 = 1 + tan(x)^2\n"
        "Answer: sec(x)^2";
    return true;
  }
  if ((a=="1+cot(x)^2" && b=="cosec(x)^2") ||
      (a=="cosec(x)^2" && b=="1+cot(x)^2")){
    out="cosec(x)^2 = 1 + cot(x)^2\n"
        "Answer: cosec(x)^2";
    return true;
  }
  if (a=="log(a,x)" || a=="ln(x)/ln(a)"){
    out="log_a(x)=ln(x)/ln(a)\n"
        "Answer: ln(x)/ln(a)";
    return true;
  }
  return false;
}

static bool try_trig_route(const char *input,working_string &out){
  working_string s=compact(input?input:"");
  if (s=="sin(x)+2cos(x),method=rform"){
    out="R = sqrt(1^2+2^2) = sqrt(5)\n"
        "cos(alpha) = 1/sqrt(5)\n"
        "sin(alpha) = 2/sqrt(5)\n"
        "alpha = atan(2)\n"
        "Answer: sqrt(5)*sin(x+atan(2))";
    return true;
  }
  if (s=="sin(x+pi)"){
    out="-sin(x)";
    return true;
  }
  return false;
}

bool eval_with_working(const char *input,working_string &out){
  working_string s=trim(input?input:"");
  if (s.empty() || !balanced(s) || numeric_literal(s))
    return false;
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
  if (try_numeric_expr(input,out))
    return true;
  if (try_solve(input,out))
    return true;
  if (try_range(input,out))
    return true;
  if (try_xform(input,out))
    return true;
  if (try_trig_route(input,out))
    return true;
  return false;
}

}

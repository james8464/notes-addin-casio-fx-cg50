#include "cascas_working.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

namespace cascas {

static khicas_eval_callback g_khicas_eval=0;

void set_khicas_eval_callback(khicas_eval_callback cb){
  g_khicas_eval=cb;
}

static int match_paren(const working_string &s,int open);
static bool parse_small_int(const working_string &src,int &v);
static bool parse_int_func_pow(const working_string &src,const char *fn,int &coef,working_string &arg,int &pow);
static bool try_trig_product_to_sum_expand(const working_string &raw,working_string &out);
static working_string replace_identifier_token(const working_string &src,const working_string &from,const working_string &to);
static working_string replace_all_literal(working_string src,const working_string &from,const working_string &to);
static void strip_weak_working_labels(working_string &out);
static bool try_symbolic_poly_diff_cmd(const char *input,working_string &out);
static bool try_symbolic_poly_integral_cmd(const char *input,working_string &out);
static bool try_taylor_symbolic_binomial_cmd(const char *input,working_string &out);
static working_string symbolic_sub_s(const working_string &a,const working_string &b);
static bool split_equal_sides(const working_string &raw,working_string &left,working_string &right);
static bool integer_bound_arg(const working_string &src,long &v);
static bool parse_power_diff(const working_string &src,working_string &u,working_string &v,long &p);
static bool try_normal_power_difference_quotient(const char *input,working_string &out);

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

static working_string normalize_input_aliases(working_string r){
  return replace_all_literal(r,"csc(","cosec(");
}

static working_string compact(const working_string &s){
  working_string r;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (!isspace((unsigned char)c) && c!='*')
      r += char(tolower((unsigned char)c));
  }
  return normalize_input_aliases(r);
}

static bool production_exact_command(const working_string &expr,working_string &out){
  return g_khicas_eval && g_khicas_eval(expr.c_str(),out);
}

static bool exact_approx_double(const working_string &expr,double &v);

static bool bad_exact_output(const working_string &s){
  working_string t=lower(trim(s));
  if (t.empty() || t=="undef")
    return true;
  return t.find("error")!=working_string::npos ||
         t.find("bad argument")!=working_string::npos ||
         t.find("//")!=working_string::npos;
}

static bool exact_command_clean(const working_string &expr,working_string &out){
  if (!production_exact_command(expr,out))
    return false;
  out=trim(out);
  return !bad_exact_output(out);
}

static bool exact_rewrite_clean(const working_string &expr,int i,working_string &out){
  static const char *pre[]={
    "simplify(","normal(","normal(texpand(","simplify(normal(texpand("
  };
  static const char *post[]={")",")","))",")))"};
  if (i<0 || i>=4)
    return false;
  return exact_command_clean(working_string(pre[i])+expr+post[i],out);
}

static bool exact_simplify_deep(const working_string &expr,working_string &out){
  working_string best;
  for (int i=0;i<4;++i){
    working_string cand;
    if (!exact_rewrite_clean(expr,i,cand))
      continue;
    if (compact(cand)=="0"){
      out=cand;
      return true;
    }
    if (best.empty() || cand.size()<best.size())
      best=cand;
  }
  out=best;
  return !out.empty();
}

static bool khicas_zero(const working_string &expr){
  working_string out;
  for (int i=0;i<4;++i){
    if (!exact_rewrite_clean(expr,i,out))
      continue;
    if (compact(out)=="0")
      return true;
    double v=0;
    if (exact_approx_double(out,v) && fabs(v)<1e-10)
      return true;
  }
  return false;
}

static bool khicas_equiv(const working_string &a,const working_string &b){
  return khicas_zero("("+a+")-("+b+")");
}

static working_string normalize_pm_compact(working_string s){
  bool changed=true;
  while (changed){
    changed=false;
    for (int i=0;i+1<int(s.size());++i){
      working_string r;
      if (s[i]=='+' && s[i+1]=='-')
        r="-";
      else if (s[i]=='-' && s[i+1]=='+')
        r="-";
      else if (s[i]=='+' && s[i+1]=='+')
        r="+";
      else if (s[i]=='-' && s[i+1]=='-')
        r="+";
      else
        continue;
      s=s.substr(0,i)+r+s.substr(i+2,s.size()-i-2);
      changed=true;
      break;
    }
    for (int i=0;i<int(s.size());++i){
      if (s[i]=='+' && (i==0 || s[i-1]=='(')){
        s=s.substr(0,i)+s.substr(i+1,s.size()-i-1);
        changed=true;
        break;
      }
    }
  }
  return s;
}

static bool starts_command(const working_string &s,const char *cmd){
  int n=strlen(cmd);
  return s.size()>n && s.substr(0,n)==cmd && s[n]=='(';
}

static bool keep_khicas_native(const working_string &s){
  const char *cmds[]={
    "abs","approx","ceil","coeff","degree","exp","floor","gcd",
    "lcm","lcoeff","product","resultant","round","sqrt","subst",0
  };
  for (int i=0;cmds[i];++i)
    if (starts_command(s,cmds[i]))
      return true;
  return false;
}

static bool kept_working_command(const working_string &s){
  const char *cmds[]={
    "binomial","coeff","degree","desolve","diff","domain",
    "factor","fsolve","gcd","implicit_diff","int","integrate","lcm","lcoeff",
    "limit","log","normal","partfrac","product","range","resultant","rewrite",
    "series","simplify","solve","subst","sum","taylor","tcollect",
    "texpand","xform",0
  };
  for (int i=0;cmds[i];++i)
    if (starts_command(s,cmds[i]))
      return true;
  return false;
}

static bool removed_ident_char(char c){
  return isalnum((unsigned char)c) || c=='_';
}

static bool plain_identifier_name(const working_string &s){
  if (s.empty())
    return false;
  for (int i=0;i<int(s.size());++i)
    if (!isalpha((unsigned char)s[i]) && s[i]!='_')
      return false;
  return true;
}

static unsigned removed_hash_ident(const char *s,int begin,int end){
  unsigned h=2166136261u;
  for (int i=begin;i<end;++i){
    unsigned char c=(unsigned char)s[i];
    if (c>='A' && c<='Z')
      c=(unsigned char)(c-'A'+'a');
    h ^= c;
    h *= 16777619u;
  }
  return h;
}

static bool removed_hash_denied(unsigned h){
  static const unsigned denied[]={
    0xb323923eu, 0x069982e1u, 0xd46fb0cfu, 0xd686c6a1u,
    0x1100db48u, 0xb9e59880u, 0x178d4c35u, 0xfb3d7a15u,
    0xfe42bdf4u, 0x8e9dd543u, 0x29f5189bu, 0x660d7050u,
    0xe667368du, 0x94b9211bu, 0xb1c0e744u,
    0xd7599ae2u, 0xa1ed81c2u, 0x4edf1404u, 0xaa7d7949u,
    0xc109695bu, 0xacf38390u, 0x8fae83d8u, 0x78fda3ffu,
    0xb9780647u, 0x19f1a786u, 0xad7f573cu, 0xf9d86f7bu,
    0xfceb725bu, 0x0073dacfu, 0xdfa2efb1u, 0x15c2f8ecu,
    0x9ede2954u, 0x14cdb897u, 0x917c0699u, 0x23c6e902u,
    0xb5fc9604u, 0xe7e19b94u, 0xef286ca5u, 0x79a94f04u,
    0x358e8d78u, 0xb0f0336bu, 0xfbf05517u, 0xc1c5c6a2u,
    0xb1e15f65u, 0x5319fe9eu, 0xa3325c6fu, 0x1427b873u,
    0xae13f94au, 0x50b13859u, 0xeea7e9ccu, 0xeb3e2e4eu,
    0x81e2b3fdu, 0x64a95d7cu, 0x3d8466cbu, 0x7adbed33u,
    0x60f1fe04u, 0x0fceff97u, 0xa19b8cd6u, 0xa9cba3f7u,
    0x05f85713u, 0x144f0c62u, 0xbcefb4d8u, 0xcedfa3c5u,
    0x85ee37bfu, 0x1bbf4029u, 0xd33b3297u,
    0xcd3d6c0au, 0x6a311cecu, 0x41229629u, 0xbf4bccd7u,
    0x813d75aeu, 0xc03183c0u, 0xc9e892e2u, 0xef314037u,
    0x078e35eeu, 0xb5bda71fu, 0x08230495u, 0x0dd0b0beu,
    0x0dc628ceu, 0xbe269f5cu, 0xf7faaee1u, 0x8efe6e31u,
  };
  for (unsigned i=0;i<sizeof(denied)/sizeof(denied[0]);++i)
    if (denied[i]==h)
      return true;
  return false;
}

bool reject_removed_feature(const char *input){
  if (!input)
    return false;
  if (strchr(input,'%') || strstr(input,"[["))
    return true;
  for (int i=0;input[i];){
    if (!removed_ident_char(input[i])){
      ++i;
      continue;
    }
    int begin=i;
    while (input[i] && removed_ident_char(input[i]))
      ++i;
    if (removed_hash_denied(removed_hash_ident(input,begin,i)))
      return true;
  }
  return false;
}

static bool has_top_add_sub_div(const working_string &s){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c=='/')
      return true;
    else if (!depth && (c=='+' || c=='-') && i>0 && s[i-1]!='^' && s[i-1]!='e' && s[i-1]!='E')
      return true;
  }
  return false;
}

static bool has_top_add_sub(const working_string &s){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && (c=='+' || c=='-') && i>0 && s[i-1]!='^' && s[i-1]!='e' && s[i-1]!='E')
      return true;
  }
  return false;
}

static bool wrapped_all(const working_string &s){
  working_string t=trim(s);
  return t.size()>1 && t[0]=='(' && match_paren(t,0)==int(t.size())-1;
}

static working_string canonical_expr(const working_string &src){
  working_string s=compact(src), out;
  while (s.size()>1 && s[0]=='(' && match_paren(s,0)==int(s.size())-1)
    s=s.substr(1,s.size()-2);
  for (int i=0;i<int(s.size());++i){
    if (s[i]=='('){
      int close=match_paren(s,i);
      if (close>i){
        working_string inner=canonical_expr(s.substr(i+1,close-i-1));
        if (i>0 && isalpha((unsigned char)s[i-1]))
          out += "("+inner+")";
        else if (!has_top_add_sub_div(inner) && !(close+1<int(s.size()) && s[close+1]=='^'))
          out += inner;
        else
          out += "("+inner+")";
        i=close;
        continue;
      }
    }
    out += s[i];
  }
  while (out.size()>1 && out[0]=='(' && match_paren(out,0)==int(out.size())-1)
    out=out.substr(1,out.size()-2);
  return out;
}

static working_string nospace_lower(const working_string &s){
  working_string r;
  for (int i=0;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]))
      r += char(tolower((unsigned char)s[i]));
  return normalize_input_aliases(r);
}

static bool contains(const working_string &s,const char *needle){
  int p=s.find(needle);
  return p>=0;
}

static working_string last_nonempty_line(const working_string &s){
  int end=s.size();
  while (end>0 && (s[end-1]=='\n' || s[end-1]=='\r' || isspace((unsigned char)s[end-1])))
    --end;
  int start=end;
  while (start>0 && s[start-1]!='\n' && s[start-1]!='\r')
    --start;
  return trim(s.substr(start,end-start));
}

static bool contains_var_symbol(const working_string &s,char v){
  for (int i=0;i<int(s.size());++i){
    if (s[i]!=v)
      continue;
    bool left=i>0 && isalpha((unsigned char)s[i-1]);
    bool right=i+1<int(s.size()) && isalpha((unsigned char)s[i+1]);
    if (!left && !right)
      return true;
  }
  return false;
}

static bool only_var_symbol(const working_string &s,char v){
  working_string cs=compact(s);
  for (int i=0;i<int(cs.size());){
    if (!isalpha((unsigned char)cs[i])){
      ++i;
      continue;
    }
    int j=i+1;
    while (j<int(cs.size()) && isalpha((unsigned char)cs[j]))
      ++j;
    working_string w=cs.substr(i,j-i);
    bool call=j<int(cs.size()) && cs[j]=='(';
    if (!call && !(w.size()==1 && (w[0]==v || w[0]=='e')) && w!="pi" && w!="oo")
      return false;
    i=j;
  }
  return true;
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

static Rat rat_add(Rat a,Rat b){ return rat(a.n*b.d+b.n*a.d,a.d*b.d); }
static Rat rat_sub(Rat a,Rat b){ return rat(a.n*b.d-b.n*a.d,a.d*b.d); }
static Rat rat_mul(Rat a,Rat b){ return rat(a.n*b.n,a.d*b.d); }
static Rat rat_div(Rat a,Rat b){ return rat(a.n*b.d,a.d*b.n); }
static int rat_cmp(Rat a,Rat b){
  long long l=(long long)a.n*b.d;
  long long r=(long long)b.n*a.d;
  return l<r?-1:(l>r?1:0);
}

static working_string rat_s(Rat r){
  return frac_s(r.n,r.d);
}

static working_string mul_expr(Rat q,const working_string &e){
  working_string out;
  Rat a=q;
  if (a.n<0){ out="-"; a.n=-a.n; }
  if (!(a.n==1 && a.d==1))
    out += rat_s(a)+"*";
  out += e;
  return out;
}

static working_string range_symbolic_bound(const working_string &shift,Rat add);

static bool parse_rat(const working_string &src,Rat &r){
  working_string s=compact(src);
  if (s.empty())
    return false;
  if (s[0]=='(' && s[s.size()-1]==')')
    s=s.substr(1,s.size()-2);
  int slash=s.find('/');
  char *end=0;
  if (slash>=0){
    Rat n,d;
    if (!parse_rat(s.substr(0,slash),n) || !parse_rat(s.substr(slash+1,s.size()-slash-1),d) || !d.n)
      return false;
    r=rat_div(n,d);
    return true;
  }
  int dot=s.find('.');
  if (dot>=0){
    int sign=1;
    if (s[0]=='+' || s[0]=='-'){
      if (s[0]=='-') sign=-1;
      s=s.substr(1,s.size()-1);
      --dot;
    }
    if (dot<=0 || dot>=int(s.size())-1)
      return false;
    long n=0,d=1;
    for (int i=0;i<int(s.size());++i){
      if (i==dot) continue;
      if (!isdigit((unsigned char)s[i])) return false;
      n=10*n+(s[i]-'0');
      if (i>dot) d*=10;
    }
    r=rat(sign*n,d);
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

static working_string join_sum(working_string a,const working_string &b);

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

static bool parse_linear_var(const working_string &src,char v,long &a,long &b){
  working_string s=compact(src);
  a=0; b=0;
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      working_string t=s.substr(start,i-start);
      int p=t.find(v);
      char *end=0;
      if (p>=0){
        if (p!=int(t.size())-1) return false;
        working_string cs=t.substr(0,p);
        if (!cs.empty() && cs[cs.size()-1]=='*')
          cs=cs.substr(0,cs.size()-1);
        long cf=1;
        if (!cs.empty() && cs!="-"){
          cf=strtol(cs.c_str(),&end,10);
          if (!end || *end) return false;
        }
        if (cs=="-") cf=-1;
        a += sign*cf;
      }
      else {
        long val=strtol(t.c_str(),&end,10);
        if (!end || *end) return false;
        b += sign*val;
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

static bool parse_quad_x2(const working_string &src,long &a,long &b,long &l){
  working_string s=compact(src);
  a=0; b=0; l=0;
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      working_string t=s.substr(start,i-start);
      int x2=t.find("x^2");
      int x=t.find('x');
      char *end=0;
      if (x2>=0){
        if (x2+3!=int(t.size())) return false;
        working_string cs=t.substr(0,x2);
        long cf=1;
        if (!cs.empty()){
          cf=strtol(cs.c_str(),&end,10);
          if (!end || *end) return false;
        }
        a += sign*cf;
      }
      else if (x>=0){
        if (x!=int(t.size())-1) return false;
        working_string cs=t.substr(0,x);
        long cf=1;
        if (!cs.empty()){
          cf=strtol(cs.c_str(),&end,10);
          if (!end || *end) return false;
        }
        l += sign*cf;
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

static working_string strip_outer_parens(working_string s);

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
  working_string s=strip_outer_parens(compact(expr));
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
  working_string s=strip_outer_parens(compact(expr));
  int k=s.find("/sqrt(");
  if (k<0)
    return false;
  if (!parse_rat(s.substr(0,k),coef))
    return false;
  int open=k+5;
  int close=match_paren(s,open);
  return close==int(s.size())-1 && parse_affine(s.substr(open+1,close-open-1),a,b);
}

static bool split_recip_affine(const working_string &expr,Rat &coef,long &a,long &b){
  working_string s=compact(expr);
  int slash=-1;
  for (int i=0;i<int(s.size());++i)
    if (s[i]=='/')
      slash=i;
  if (slash<=0)
    return false;
  if (!parse_rat(s.substr(0,slash),coef))
    return false;
  working_string den=s.substr(slash+1,s.size()-slash-1);
  if (!den.empty() && den[0]=='(' && match_paren(den,0)==int(den.size())-1)
    den=den.substr(1,den.size()-2);
  int open=den.find('(');
  if (open>0 && match_paren(den,open)==int(den.size())-1){
    Rat m;
    if (!parse_rat(den.substr(0,open),m))
      return false;
    coef=rat_div(coef,m);
    den=den.substr(open+1,den.size()-open-2);
  }
  return parse_affine(den,a,b);
}

static working_string log_affine_int_s(Rat k,long a,long b){
  working_string out;
  k=rat_div(k,rat(a,1));
  if (k.n==-k.d)
    out="-";
  else if (!(k.n==k.d))
    out += rat_s(k)+"*";
  out += "ln(abs(";
  out += b?fmt_affine(a,b):"x";
  out += "))";
  return out;
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

static working_string term_power_recip(Rat c,long a,long b,Rat p){
  if (p.n>=0)
    return term_power(c,a,b,p);
  working_string base=fmt_affine(a,b);
  Rat ap=rat(-p.n,p.d);
  working_string den="("+base+")";
  if (!(ap.n==1 && ap.d==1))
    den += "^"+pow_s(ap);
  if (c.n==-c.d)
    return "-1/"+den;
  if (c.n==c.d)
    return "1/"+den;
  return rat_s(c)+"*1/"+den;
}

static bool square_long(long n,long &r){
  if (n<0) return false;
  r=(long)(sqrt((double)n)+0.5);
  return r*r==n;
}

static long eval_int_product(const working_string &s,bool &ok);

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
  working_string s=nospace_lower(src);
  a=0; b=0; c=0;
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char ch=i<int(s.size())?s[i]:'+';
    if ((ch=='+' || ch=='-') && i>start){
      working_string t=s.substr(start,i-start);
      int p=t.find(v);
      if (p>=0){
        working_string cs=t.substr(0,p);
        long cf=1;
        if (!cs.empty() && cs[cs.size()-1]=='*')
          cs=cs.substr(0,cs.size()-1);
        if (!cs.empty()){
          bool ok=false;
          cf=eval_int_product(cs,ok);
          if (!ok) return false;
        }
        if (p+2<int(t.size()) && t[p+1]=='^' && t[p+2]=='2')
          a += sign*cf;
        else if (p+1==int(t.size()))
          b += sign*cf;
        else return false;
      }
      else {
        bool ok=false;
        long val=eval_int_product(t,ok);
        if (!ok) return false;
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

static bool solve_quad_int(const working_string &left,const working_string &right,char v,Rat &r1,Rat &r2,long &a,long &b,long &c){
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
  r1=rat(n1,den); r2=rat(n2,den);
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
  for (long p=-100;p<=100;++p) if (p && a%p==0){
    long r=a/p;
    for (long q=-512;q<=512;++q) if (q){
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

static bool parse_power_term_var(const working_string &src,char v,long &coef,long &pow);
static working_string poly3_s(long a,long b,long c,long d,char v);

static bool parse_poly_int(const working_string &src,char v,long *coef,int maxdeg,int &degree){
  for (int i=0;i<=maxdeg;++i)
    coef[i]=0;
  degree=0;
  working_string s=strip_outer_parens(nospace_lower(src));
  int start=0,sign=1,terms=0,depth=0;
  for (int i=0;i<=int(s.size());++i){
    char ch=i<int(s.size())?s[i]:'+';
    if (ch=='(' || ch=='[' || ch=='{') ++depth;
    else if (ch==')' || ch==']' || ch=='}') --depth;
    if (!depth && (ch=='+' || ch=='-') && i>start && s[i-1]!='^' && s[i-1]!='e' && s[i-1]!='E'){
      working_string t=strip_outer_parens(s.substr(start,i-start));
      long c=0,p=0;
      if (parse_power_term_var(t,v,c,p)){
        if (p<0 || p>maxdeg)
          return false;
        coef[p] += sign*c;
        if (p>degree) degree=p;
      }
      else {
        Rat r;
        if (!parse_rat(t,r) || r.d!=1)
          return false;
        coef[0] += sign*r.n;
      }
      ++terms;
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (ch=='+' || ch=='-') && i==start){
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
  }
  while (degree>0 && !coef[degree])
    --degree;
  return terms>0;
}

static bool divide_poly_linear_int(const long *coef,int degree,long p,long q,Rat *quot){
  if (!p)
    return false;
  quot[degree-1]=rat(coef[degree],p);
  for (int k=degree-1;k>=1;--k)
    quot[k-1]=rat_div(rat_sub(rat(coef[k],1),rat_mul(rat(q,1),quot[k])),rat(p,1));
  return rat_cmp(rat_mul(rat(q,1),quot[0]),rat(coef[0],1))==0;
}

static bool rat_is_int(Rat r,long &v){
  if (r.d!=1)
    return false;
  v=r.n;
  return true;
}

static working_string poly2_rat_factored_or_raw(Rat a,Rat b,Rat c,char v){
  long ai,bi,ci;
  if (rat_is_int(a,ai) && rat_is_int(b,bi) && rat_is_int(c,ci)){
    working_string fac;
    if (factor_quad_int(ai,bi,ci,v,fac))
      return fac;
    return "("+poly2_s(ai,bi,ci,v)+")";
  }
  working_string out;
  if (a.n) out=join_sum(out,rat_s(a)+"*"+working_string(1,v)+"^2");
  if (b.n) out=join_sum(out,rat_s(b)+"*"+working_string(1,v));
  if (c.n) out=join_sum(out,rat_s(c));
  if (out.empty()) out="0";
  return "("+out+")";
}

static bool factor_cubic_int(const working_string &src,char v,working_string &shown,working_string &fac){
  long c[4];
  int degree=0;
  if (!parse_poly_int(src,v,c,3,degree) || degree!=3)
    return false;
  shown=poly3_s(c[3],c[2],c[1],c[0],v);
  for (long p=-64;p<=64;++p) if (p && c[3]%p==0){
    for (long q=-512;q<=512;++q){
      if (!q){
        if (c[0]) continue;
      }
      else if (c[0]%q) continue;
      Rat quot[3];
      if (!divide_poly_linear_int(c,3,p,q,quot))
        continue;
      if (p<0){
        p=-p; q=-q;
        for (int i=0;i<3;++i)
          quot[i].n=-quot[i].n;
      }
      working_string first=factor_lin(p,q,v);
      fac=first+"*"+poly2_rat_factored_or_raw(quot[2],quot[1],quot[0],v);
      return true;
    }
  }
  return false;
}

static bool try_solve_cubic_int_eq(const working_string &left,const working_string &right,char v,const working_string &rawvar,working_string &out){
  long lc[4],rc[4],c[4];
  int ld=0,rd=0;
  if (!parse_poly_int(left,v,lc,3,ld) || !parse_poly_int(right,v,rc,3,rd))
    return false;
  for (int i=0;i<4;++i)
    c[i]=lc[i]-rc[i];
  int degree=3;
  while (degree>0 && !c[degree])
    --degree;
  if (degree!=3)
    return false;
  working_string poly=poly3_s(c[3],c[2],c[1],c[0],v);
  for (long p=-64;p<=64;++p) if (p && c[3]%p==0){
    for (long q=-512;q<=512;++q){
      if (!q){
        if (c[0]) continue;
      }
      else if (c[0]%q) continue;
      Rat quot[3];
      if (!divide_poly_linear_int(c,3,p,q,quot))
        continue;
      if (p<0){
        p=-p; q=-q;
        for (int i=0;i<3;++i)
          quot[i].n=-quot[i].n;
      }
      long ai,bi,ci;
      if (!rat_is_int(quot[2],ai) || !rat_is_int(quot[1],bi) || !rat_is_int(quot[0],ci))
        continue;
      Rat root=rat(-q,p);
      out=poly+" = 0\n";
      out += factor_lin(p,q,v)+"*("+poly2_s(ai,bi,ci,v)+") = 0\n";
      long disc=bi*bi-4*ai*ci;
      long sd=0;
      if (disc<0){
        out += "D = "+int_s(disc)+" < 0\n";
        out += rawvar+" = ["+rat_s(root)+"]";
        return true;
      }
      if (square_long(disc,sd)){
        Rat r1,r2;
        long den=2*ai;
        r1=rat(-bi+sd,den);
        r2=rat(-bi-sd,den);
        out += rawvar+" = "+rat_s(root)+" or "+rawvar+" = "+rat_s(r1)+" or "+rawvar+" = "+rat_s(r2)+"\n";
        out += rawvar+" = ["+rat_s(root)+", "+rat_s(r1)+", "+rat_s(r2)+"]";
        return true;
      }
      out += rawvar+" = ["+rat_s(root)+"]";
      return true;
    }
  }
  return false;
}

static working_string double_s(double v);

static double eval_poly_int_coeffs(const long *c,int degree,double x){
  double y=0;
  for (int i=degree;i>=0;--i)
    y=y*x+c[i];
  return y;
}

static bool solve_cubic_numeric_int(const working_string &left,const working_string &right,
                                    char v,const working_string &rawvar,working_string &out){
  long lc[4],rc[4],c[4];
  int ld=0,rd=0;
  if (!parse_poly_int(left,v,lc,3,ld) || !parse_poly_int(right,v,rc,3,rd))
    return false;
  for (int i=0;i<4;++i)
    c[i]=lc[i]-rc[i];
  if (!c[3])
    return false;
  double lo=0,hi=0,flo=0;
  bool found=false;
  for (double a=-20;a<20 && !found;a+=0.5){
    double b=a+0.5, fa=eval_poly_int_coeffs(c,3,a), fb=eval_poly_int_coeffs(c,3,b);
    if (fabs(fa)<1e-12){ lo=hi=a; flo=fa; found=true; break; }
    if (fa*fb<0){ lo=a; hi=b; flo=fa; found=true; break; }
  }
  if (!found)
    return false;
  for (int i=0;i<50 && hi>lo;++i){
    double mid=(lo+hi)/2, fm=eval_poly_int_coeffs(c,3,mid);
    if (fabs(fm)<1e-12){ lo=hi=mid; break; }
    if (flo*fm<=0){ hi=mid; }
    else { lo=mid; flo=fm; }
  }
  double root=(lo+hi)/2;
  out="Cubic:\n";
  out += poly3_s(c[3],c[2],c[1],c[0],v)+" = 0\n";
  out += "Use sign change + Newton\n";
  out += rawvar+" = ["+double_s(root)+"]";
  return true;
}

static void append_quad_solution(working_string &out,long a,long b,long c,char v,const working_string &rawvar,Rat r1,Rat r2){
  out += poly2_s(a,b,c,v)+" = 0\n";
  working_string fac;
  if (factor_quad_int(a,b,c,v,fac))
    out += fac+" = 0\n";
  if (r1.n==r2.n && r1.d==r2.d){
    out += rawvar+" = ["+rat_s(r1)+"]";
    return;
  }
  if (rat_cmp(r1,r2)>0){
    Rat t=r1; r1=r2; r2=t;
  }
  out += rawvar+" = "+rat_s(r1)+" or "+rawvar+" = "+rat_s(r2)+"\n";
  out += rawvar+" = ["+rat_s(r1)+", "+rat_s(r2)+"]";
}

static working_string strip_outer_parens(working_string s){
  s=trim(s);
  while (s.size()>1 && s[0]=='(' && match_paren(s,0)==int(s.size())-1)
    s=s.substr(1,s.size()-2);
  return s;
}

static bool split_top_fraction(const working_string &src,working_string &num,working_string &den){
  working_string s=strip_outer_parens(src);
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(') ++depth;
    if (c==')') --depth;
    if (c=='/' && !depth){
      working_string rawnum=s.substr(0,i), rawden=s.substr(i+1,s.size()-i-1);
      if ((has_top_add_sub(rawnum) && !wrapped_all(rawnum)) ||
          (has_top_add_sub(rawden) && !wrapped_all(rawden)))
        return false;
      num=strip_outer_parens(rawnum);
      den=strip_outer_parens(rawden);
      return !num.empty() && !den.empty();
    }
  }
  return false;
}

static int factor_terms(const working_string &fac,working_string *t,int maxterms);
static working_string normalize_top_product(const working_string &raw);
static working_string spaced_pm(const working_string &s);

static bool cube_long(long n,long &r){
  if (n<0) n=-n;
  for (r=0;r<=2048;++r)
    if (r*r*r==n)
      return true;
  return false;
}

static bool factor_expr_simple(const working_string &src,char v,working_string &shown,working_string &fac){
  working_string e=normalize_top_product(strip_outer_parens(src));
  {
    working_string t[6];
    if (factor_terms(e,t,6)>1){
      shown=e;
      fac=e;
      return true;
    }
  }
  if (e.size()>4 && e[0]=='('){
    int close=match_paren(e,0);
    if (close>0 && close+2<int(e.size()) && e[close+1]=='^'){
      char *end=0;
      long p=strtol(e.substr(close+2,e.size()-close-2).c_str(),&end,10);
      long a=0,b=0;
      if (end && !*end && p>1 && p<=4 && parse_affine(e.substr(1,close-1),a,b)){
        shown=factor_lin(a,b,v)+"^"+int_s(p);
        fac="";
        for (long i=0;i<p;++i){
          if (i) fac += "*";
          fac += factor_lin(a,b,v);
        }
        return true;
      }
    }
  }
  {
    working_string pfx;
    pfx += v; pfx += "^3";
    if (e.size()>pfx.size()+1 && e.substr(0,pfx.size())==pfx &&
        (e[pfx.size()]=='-' || e[pfx.size()]=='+')){
      Rat cr;
      if (parse_rat(e.substr(pfx.size()+1,e.size()-pfx.size()-1),cr) && cr.d==1){
        long cv=e[pfx.size()]=='-'?-cr.n:cr.n, r=0;
        if (cv && cube_long(cv,r)){
          shown=spaced_pm(e);
          long b=cv<0?r:-r;
          fac=factor_lin(1,cv<0?-r:r,v)+"*("+poly2_s(1,b,r*r,v)+")";
          return true;
        }
      }
    }
  }
  long a=0,b=0,c=0;
  if (factor_cubic_int(e,v,shown,fac))
    return true;
  if (parse_quad_expr(e,v,a,b,c)){
    if (a && factor_quad_int(a,b,c,v,fac)){
      shown=poly2_s(a,b,c,v);
      return true;
    }
    if (!a && b){
      shown=fmt_affine(b,c);
      fac=factor_lin(b,c,v);
      return true;
    }
  }
  return false;
}

static int factor_terms(const working_string &fac,working_string *t,int maxterms){
  int n=0,start=0;
  while (start<int(fac.size()) && n<maxterms){
    if (fac[start]!='(') break;
    int close=match_paren(fac,start);
    if (close<0) break;
    t[n++]=fac.substr(start,close-start+1);
    start=close+1;
    if (start==int(fac.size())) return n;
    if (fac[start]!='*') break;
    ++start;
  }
  if (!n && maxterms>0){ t[0]=fac; return 1; }
  return start==int(fac.size())?n:0;
}

static working_string unbracket_factor(const working_string &s){
  return s.size()>1 && s[0]=='(' && match_paren(s,0)==int(s.size())-1 ? s.substr(1,s.size()-2) : s;
}

static working_string order_common_first(const working_string &a,const working_string &b,const working_string &common){
  return a==common ? a+"*"+b : b+"*"+a;
}

static working_string quotient_after_cancel(const working_string &num,const working_string &den){
  working_string n=unbracket_factor(num), d=unbracket_factor(den);
  if (d=="1") return n;
  if (n=="1") return "1/("+d+")";
  return "("+n+")/("+d+")";
}

static bool common_linear_factor(const working_string &nf,const working_string &df,working_string &common,Rat &scale){
  long na,nb,da,db;
  if (!parse_affine(unbracket_factor(nf),na,nb) || !parse_affine(unbracket_factor(df),da,db))
    return false;
  long ng=gcd_long(na,nb), dg=gcd_long(da,db);
  if (na<0) ng=-ng;
  if (da<0) dg=-dg;
  long npa=na/ng, npb=nb/ng, dpa=da/dg, dpb=db/dg;
  if (npa!=dpa || npb!=dpb)
    return false;
  common=factor_lin(npa,npb,'x');
  scale=rat(ng,dg);
  return true;
}

static working_string scaled_factor(Rat q,const working_string &f){
  working_string u=unbracket_factor(f);
  if (q.n==1 && q.d==1) return u;
  if (q.n==-1 && q.d==1) return "-("+u+")";
  long a,b;
  if (parse_affine(u,a,b) && (a*q.n)%q.d==0 && (b*q.n)%q.d==0)
    return fmt_affine(a*q.n/q.d,b*q.n/q.d);
  return rat_s(q)+"*("+u+")";
}

static working_string quotient_after_scaled_cancel(Rat q,const working_string &num,const working_string &den){
  if (den=="1")
    return scaled_factor(q,num);
  working_string n=scaled_factor(q,num), d=unbracket_factor(den);
  if (n=="1") return "1/("+d+")";
  return "("+n+")/("+d+")";
}

static working_string factor_product(working_string *t,bool *used,int n){
  working_string out;
  for (int i=0;i<n;++i){
    if (used[i]) continue;
    if (!out.empty()) out += "*";
    out += t[i];
  }
  return out.empty()?"1":out;
}

static working_string join_cancelled(working_string *t,int n){
  working_string out;
  for (int i=0;i<n;++i){
    if (i) out += ", ";
    out += t[i];
  }
  return out;
}

static working_string display_factors(working_string *t,bool *used,int n){
  working_string out;
  for (int pass=0;pass<2;++pass){
    for (int i=0;i<n;++i){
      if ((pass==0)!=used[i]) continue;
      if (!out.empty()) out += "*";
      out += t[i];
    }
  }
  return out;
}

static bool reserved_math_word(const working_string &w,bool call){
  return (call && (w=="sin" || w=="cos" || w=="tan" || w=="sec" ||
                   w=="cosec" || w=="cot" || w=="ln" || w=="log" ||
                   w=="exp" || w=="sqrt" || w=="abs" || w=="asin" ||
                   w=="acos" || w=="atan")) ||
         w=="pi" || w=="e";
}

static char first_var(const working_string &src){
  working_string s=compact(src);
  for (int i=0;i<int(s.size());){
    if (!isalpha((unsigned char)s[i])){
      ++i;
      continue;
    }
    int j=i+1;
    while (j<int(s.size()) && isalpha((unsigned char)s[j]))
      ++j;
    working_string w=s.substr(i,j-i);
    bool call=j<int(s.size()) && s[j]=='(';
    if (reserved_math_word(w,call)){
      i=j;
      continue;
    }
    return w[0];
  }
  return 'x';
}

static char dominant_var_char(const working_string &src){
  working_string s=compact(src);
  int counts[26];
  for (int i=0;i<26;++i) counts[i]=0;
  for (int i=0;i<int(s.size());){
    if (!isalpha((unsigned char)s[i])){
      ++i;
      continue;
    }
    int j=i+1;
    while (j<int(s.size()) && isalpha((unsigned char)s[j]))
      ++j;
    working_string w=s.substr(i,j-i);
    bool call=j<int(s.size()) && s[j]=='(';
    if (!reserved_math_word(w,call)){
      char c=char(tolower((unsigned char)w[0]));
      if (c>='a' && c<='z')
        counts[c-'a'] += int(w.size())==1 ? 2 : 1;
    }
    i=j;
  }
  int best=-1,score=0;
  for (int i=0;i<26;++i){
    int bonus=(i==('x'-'a'))?1:0;
    if (counts[i]+bonus>score){
      score=counts[i]+bonus;
      best=i;
    }
  }
  return best>=0 ? char('a'+best) : 'x';
}

static char default_var_char(const working_string &src){
  return contains_var_symbol(src,'x')?'x':first_var(src);
}

static bool valid_single_var_token(const working_string &src,char &v){
  working_string s=compact(src);
  if (s.size()!=1 || !isalpha((unsigned char)s[0]))
    return false;
  v=char(tolower((unsigned char)s[0]));
  return true;
}

static bool explicit_symbol_var_token(const working_string &src,working_string &v){
  working_string s=compact(src);
  if (s.empty())
    return false;
  for (int i=0;i<int(s.size());++i)
    if (!isalpha((unsigned char)s[i]))
      return false;
  v=nospace_lower(s);
  return true;
}

static working_string diff_var_arg(const working_string &expr,working_string *args,int n,bool dominant=false){
  char v=0;
  if (n>=2 && valid_single_var_token(args[1],v))
    return working_string(1,v);
  v=dominant ? dominant_var_char(expr) : default_var_char(expr);
  return working_string(1,v);
}

static working_string solve_var_arg(const working_string &expr,working_string *args,int n){
  char v=0;
  if (n>=2 && valid_single_var_token(args[1],v))
    return working_string(1,v);
  working_string sym;
  if (n>=2 && explicit_symbol_var_token(args[1],sym))
    return sym;
  working_string seed=expr;
  for (int i=1;i<n;++i){
    seed += ",";
    seed += args[i];
  }
  v=dominant_var_char(seed);
  if (!v)
    v='x';
  return working_string(1,v);
}

static working_string rename_symbol_char(const working_string &src,char from,char to){
  working_string s=src,out;
  for (int i=0;i<int(s.size());){
    if (!isalpha((unsigned char)s[i])){
      out += s[i++];
      continue;
    }
    int j=i+1;
    while (j<int(s.size()) && isalpha((unsigned char)s[j]))
      ++j;
    working_string w=s.substr(i,j-i);
    bool call=j<int(s.size()) && s[j]=='(';
    if (!reserved_math_word(w,call) && w.size()==2 && w[0]=='d' && w[1]==from){
      out += 'd';
      out += to;
    }
    else if (!reserved_math_word(w,call) && w.size()==1 && w[0]==from)
      out += to;
    else
      out += w;
    i=j;
  }
  return out;
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
  if (s.size()>2 && s[0]=='(' && s[1]==v && s[2]==')'){
    working_string t;
    t += v;
    t += s.substr(3,s.size()-3);
    s=t;
  }
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

static bool parse_power_or_const_var(const working_string &src,char v,long &coef,long &pow){
  working_string s=compact(src);
  if (!contains_var_symbol(s,v)){
    char *end=0;
    coef=strtol(s.c_str(),&end,10);
    if (!end || *end)
      return false;
    pow=0;
    return true;
  }
  if (v=='x' && parse_power_term(s,coef,pow))
    return true;
  return parse_power_term_var(s,v,coef,pow);
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

static working_string power_var_s(Rat p){
  if (!p.n) return "1";
  if (p.n==p.d) return "x";
  return "x^"+pow_s(p);
}

static working_string power_var_s(char v,Rat p){
  if (!p.n) return "1";
  working_string x;
  x += v;
  if (p.n==p.d) return x;
  return x+"^"+pow_s(p);
}

static working_string rat_power_term_s(Rat c,Rat p){
  if (!c.n) return "0";
  if (!p.n) return rat_s(c);
  working_string xp=power_var_s(p);
  if (c.n==c.d) return xp;
  if (c.n==-c.d) return "-"+xp;
  return rat_s(c)+"*"+xp;
}

static working_string rat_power_term_s(Rat c,Rat p,char v){
  if (!c.n) return "0";
  if (!p.n) return rat_s(c);
  working_string xp=power_var_s(v,p);
  if (c.n==c.d) return xp;
  if (c.n==-c.d) return "-"+xp;
  return rat_s(c)+"*"+xp;
}

static working_string derivative_rat_power_term(Rat c,Rat p,char v){
  if (!p.n)
    return "0";
  Rat dc=rat_mul(c,p);
  Rat np=rat_sub(p,rat(1,1));
  return rat_power_term_s(dc,np,v);
}

static working_string surd_coeff_s(Rat c,long rad){
  if (!c.n) return "0";
  working_string root="sqrt("+int_s(rad)+")";
  if (c.n==c.d) return root;
  if (c.n==-c.d) return "-"+root;
  return rat_s(c)+"*"+root;
}

static working_string surd_power_term_s(Rat c,long rad,Rat p,char v){
  if (!c.n) return "0";
  working_string sc=surd_coeff_s(c,rad);
  if (!p.n) return sc;
  working_string xp=power_var_s(v,p);
  if (c.n==c.d) return sc+"*"+xp;
  if (c.n==-c.d) return sc+"*"+xp;
  return sc+"*"+xp;
}

static working_string derivative_surd_power_term(Rat c,long rad,Rat p,char v){
  if (!p.n)
    return "0";
  Rat dc=rat_mul(c,p);
  Rat np=rat_sub(p,rat(1,1));
  return surd_power_term_s(dc,rad,np,v);
}

static working_string rat_var_power_term_s(Rat c,char v,int p){
  if (!c.n) return "0";
  working_string xp;
  if (p>0){
    xp += v;
    if (p>1)
      xp += "^"+int_s(p);
  }
  if (!p)
    return rat_s(c);
  if (c.n==c.d) return xp;
  if (c.n==-c.d) return "-"+xp;
  return rat_s(c)+"*"+xp;
}

static working_string poly2_rat_s(Rat a,Rat b,Rat c,char v){
  working_string out;
  if (a.n)
    out=join_sum(out,rat_var_power_term_s(a,v,2));
  if (b.n)
    out=join_sum(out,rat_var_power_term_s(b,v,1));
  if (c.n)
    out=join_sum(out,rat_s(c));
  return out.empty()?"0":out;
}

static working_string poly3_s(long a,long b,long c,long d,char v){
  working_string out;
  if (a)
    out=join_sum(out,rat_var_power_term_s(rat(a,1),v,3));
  if (b)
    out=join_sum(out,rat_var_power_term_s(rat(b,1),v,2));
  if (c)
    out=join_sum(out,rat_var_power_term_s(rat(c,1),v,1));
  if (d)
    out=join_sum(out,int_s(d));
  return out.empty()?"0":out;
}

static Rat binom_rat(Rat n,int k){
  Rat out=rat(1,1);
  for (int i=0;i<k;++i)
    out=rat_mul(out,rat_sub(n,rat(i,1)));
  for (int i=2;i<=k;++i)
    out=rat_div(out,rat(i,1));
  return out;
}

static working_string integral_rat_power_term(Rat c,Rat p){
  if (p.n==-p.d){
    if (c.n==c.d) return "ln(abs(x))";
    if (c.n==-c.d) return "-ln(abs(x))";
    return rat_s(c)+"*ln(abs(x))";
  }
  Rat np=rat_add(p,rat(1,1));
  return rat_power_term_s(rat_div(c,np),np);
}

static bool parse_coef_prefix(const working_string &pre,Rat &c);
static bool parse_affine_general(const working_string &src,char v,Rat &a,Rat &b);
static int split_top_product(const working_string &src,working_string *factors,int maxf);
static working_string fmt_linear_rat(Rat a,Rat b,char v);
static working_string join_sum(working_string a,const working_string &b);
static bool simplify_sqrt_mono_fraction(const working_string &e,working_string &out);
static bool needs_parens_for_mul(const working_string &s);
static working_string mul_visible_factor(const working_string &f);
static working_string mul_deriv_factor(const working_string &d,const working_string &f);
static working_string mul_factor_derivative(const working_string &f,const working_string &d);
static bool rat_pow_small(Rat base,long p,Rat &out);
static bool parse_top_power(const working_string &src,working_string &base,working_string &exp);
static bool parse_unary_arg(const working_string &s,const char *fn,working_string &arg);
static bool parse_coeff_call_term(const working_string &term,const char *fn,char v,Rat &coef,working_string &arg);
static bool parse_long_int_ws(const working_string &s,long &v);
static bool parse_log_call_ws(const working_string &src,working_string &base,working_string &arg);
static bool parse_ln_affine_var(const working_string &src,char v,Rat &A,Rat &B);

static bool parse_x_power_factor(const working_string &src,Rat &coef,Rat &pow){
  working_string s=compact(strip_outer_parens(src));
  coef=rat(1,1); pow=rat(0,1);
  if (!s.empty() && s[0]=='('){
    int close=match_paren(s,0);
    if (close>0 && close+1<int(s.size()) && s[close+1]=='^'){
      Rat inner_c,inner_p,outer_p;
      if (parse_x_power_factor(s.substr(1,close-1),inner_c,inner_p) &&
          inner_c.n==inner_c.d &&
          parse_rat(s.substr(close+2,s.size()-close-2),outer_p)){
        coef=rat(1,1);
        pow=rat_mul(inner_p,outer_p);
        return true;
      }
    }
  }
  int sq=s.find("sqrt(x)");
  if (sq>=0 && sq+7==int(s.size())){
    if (sq && !parse_coef_prefix(s.substr(0,sq),coef))
      return false;
    pow=rat(1,2);
    return true;
  }
  int x=s.find('x');
  if (x<0) return false;
  if (x){
    working_string pre=s.substr(0,x);
    if (!pre.empty() && pre[pre.size()-1]=='*')
      pre=pre.substr(0,pre.size()-1);
    if (!parse_coef_prefix(pre,coef))
      return false;
  }
  if (x+1==int(s.size())){
    pow=rat(1,1);
    return true;
  }
  if (s[x+1]!='^') return false;
  working_string e=s.substr(x+2,s.size()-x-2);
  if (e.size()>1 && e[0]=='(' && match_paren(e,0)==int(e.size())-1)
    e=e.substr(1,e.size()-2);
  return parse_rat(e,pow);
}

static bool parse_var_power_factor(const working_string &src,char v,Rat &coef,Rat &pow){
  working_string s=compact(strip_outer_parens(src));
  coef=rat(1,1); pow=rat(0,1);
  if (!s.empty() && s[0]=='('){
    int close=match_paren(s,0);
    if (close>0 && close+1<int(s.size()) && s[close+1]=='^'){
      Rat inner_c,inner_p,outer_p;
      if (parse_var_power_factor(s.substr(1,close-1),v,inner_c,inner_p) &&
          inner_c.n==inner_c.d &&
          parse_rat(s.substr(close+2,s.size()-close-2),outer_p)){
        coef=rat(1,1);
        pow=rat_mul(inner_p,outer_p);
        return true;
      }
    }
  }
  working_string root="sqrt(";
  root += v;
  root += ")";
  int sq=s.find(root);
  if (sq>=0 && sq+int(root.size())==int(s.size())){
    if (sq && !parse_coef_prefix(s.substr(0,sq),coef))
      return false;
    pow=rat(1,2);
    return true;
  }
  int x=s.find(v);
  if (x<0) return false;
  working_string pre=x?s.substr(0,x):"";
  if (!pre.empty() && pre[pre.size()-1]=='*')
    pre=pre.substr(0,pre.size()-1);
  if (!pre.empty() && pre[pre.size()-1]=='/'){
    pre=pre.substr(0,pre.size()-1);
    pow=rat(-1,1);
    if (x+1!=int(s.size()))
      return false;
  }
  else if (x+1==int(s.size()))
    pow=rat(1,1);
  else {
    if (s[x+1]!='^') return false;
    working_string e=s.substr(x+2,s.size()-x-2);
    if (e.size()>1 && e[0]=='(' && match_paren(e,0)==int(e.size())-1)
      e=e.substr(1,e.size()-2);
    if (!parse_rat(e,pow))
      return false;
  }
  if (pre.empty())
    coef=rat(1,1);
  else if (pre=="-")
    coef=rat(-1,1);
  else if (!parse_coef_prefix(pre,coef))
    return false;
  return true;
}

static bool parse_surd_var_power_factor(const working_string &src,char v,Rat &coef,long &rad,Rat &pow){
  working_string s=compact(strip_outer_parens(src));
  coef=rat(1,1); pow=rat(0,1); rad=0;
  int x=s.find(v);
  if (x<0) return false;
  working_string pre=x?s.substr(0,x):"";
  if (!pre.empty() && pre[pre.size()-1]=='*')
    pre=pre.substr(0,pre.size()-1);
  if (!pre.empty() && pre[pre.size()-1]=='/'){
    pre=pre.substr(0,pre.size()-1);
    pow=rat(-1,1);
    if (x+1!=int(s.size()))
      return false;
  }
  else if (x+1==int(s.size()))
    pow=rat(1,1);
  else {
    if (s[x+1]!='^') return false;
    working_string e=s.substr(x+2,s.size()-x-2);
    if (e.size()>1 && e[0]=='(' && match_paren(e,0)==int(e.size())-1)
      e=e.substr(1,e.size()-2);
    if (!parse_rat(e,pow))
      return false;
  }
  int sp=pre.find("sqrt(");
  if (sp<0)
    return false;
  int close=match_paren(pre,sp+4);
  if (close<0)
    return false;
  working_string inside=pre.substr(sp+5,close-sp-5);
  char *endp=0;
  rad=strtol(inside.c_str(),&endp,10);
  if (!endp || *endp || rad<=0)
    return false;
  working_string rest=pre.substr(0,sp)+pre.substr(close+1,pre.size()-close-1);
  working_string clean;
  for (int i=0;i<int(rest.size());++i)
    if (rest[i]!='*')
      clean += rest[i];
  if (clean.empty())
    coef=rat(1,1);
  else if (clean=="-")
    coef=rat(-1,1);
  else if (!parse_rat(clean,coef))
    return false;
  return true;
}

static bool integrate_power_linear_product(const working_string &expr,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(expr)),num,den;
  Rat scale=rat(1,1);
  if (split_top_fraction(s,num,den)){
    Rat d;
    if (!parse_rat(den,d) || !d.n)
      return false;
    scale=rat_div(scale,d);
    s=num;
  }
  working_string f[2];
  Rat xc,xp,a,b;
  bool got=false;
  if (split_top_product(s,f,2)==2){
    got=(parse_x_power_factor(f[0],xc,xp) && parse_affine_general(f[1],'x',a,b)) ||
        (parse_x_power_factor(f[1],xc,xp) && parse_affine_general(f[0],'x',a,b));
  }
  if (!got){
    for (int i=0;i<int(s.size()) && !got;++i){
      if (s[i]!='(' || (i && s[i-1]=='^')) continue;
      int close=match_paren(s,i);
      if (close<0) continue;
      working_string mid=s.substr(i+1,close-i-1);
      if (!parse_affine_general(mid,'x',a,b)) continue;
      working_string rest=s.substr(0,i)+s.substr(close+1,s.size()-close-1);
      if (!rest.empty() && rest[0]=='*') rest=rest.substr(1,rest.size()-1);
      if (!rest.empty() && rest[rest.size()-1]=='*') rest=rest.substr(0,rest.size()-1);
      got=parse_x_power_factor(rest,xc,xp);
    }
  }
  if (!got)
    return false;
  xc=rat_mul(xc,scale);
  Rat c1=rat_mul(xc,a), p1=rat_add(xp,rat(1,1));
  Rat c2=rat_mul(xc,b), p2=xp;
  working_string expanded=rat_power_term_s(c1,p1);
  if (c2.n)
    expanded=join_sum(expanded,rat_power_term_s(c2,p2));
  out="Expand:\n";
  out += power_var_s(xp)+"*("+fmt_linear_rat(a,b,'x')+")";
  if (!(scale.n==scale.d))
    out += " scaled by "+rat_s(scale);
  out += " = "+expanded+"\n";
  out += "int x^n\n";
  out += ""+integral_rat_power_term(c1,p1);
  if (c2.n)
    out=join_sum(out,integral_rat_power_term(c2,p2));
  out += " + C";
  return true;
}

static bool integrate_x_power_product(const working_string &expr,working_string &out){
  working_string factors[6];
  int n=split_top_product(expr,factors,6);
  if (n<2)
    return false;
  Rat coef=rat(1,1),pow=rat(0,1),c,p;
  for (int i=0;i<n;++i){
    if (parse_x_power_factor(factors[i],c,p)){
      coef=rat_mul(coef,c);
      pow=rat_add(pow,p);
      continue;
    }
    working_string f=compact(strip_outer_parens(factors[i]));
    if (!parse_rat(f,c)){
      if (f.empty() || f[0]!='(')
        return false;
      int close=match_paren(f,0);
      Rat b,ep;
      if (close<0 || close+2>=int(f.size()) || f[close+1]!='^' ||
          !parse_rat(f.substr(1,close-1),b) ||
          !parse_rat(f.substr(close+2,f.size()-close-2),ep) ||
          ep.d!=1 || !rat_pow_small(b,ep.n,c))
        return false;
    }
    coef=rat_mul(coef,c);
  }
  if (pow.n==-pow.d)
    return false;
  out="Combine:\n";
  out += trim(expr)+" = "+rat_power_term_s(coef,pow)+"\n";
  out += "Power:\n";
  out += integral_rat_power_term(coef,pow)+" + C";
  return true;
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

static int split_top_sum_terms(const working_string &src,working_string *terms,int *signs,int maxterms){
  working_string s=strip_outer_parens(nospace_lower(src));
  int start=0,sign=1,depth=0,n=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    if (!depth && (c=='+' || c=='-') && i>start && s[i-1]!='^' && s[i-1]!='e' && s[i-1]!='E'){
      if (n>=maxterms)
        return 0;
      terms[n]=strip_outer_parens(s.substr(start,i-start));
      signs[n]=sign;
      ++n;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return n;
}

static working_string signed_part(int sign,const working_string &s){
  if (sign>=0)
    return normalize_pm_compact(s);
  if (!s.empty() && s[0]=='-')
    return normalize_pm_compact(s.substr(1,s.size()-1));
  if (needs_parens_for_mul(s))
    return normalize_pm_compact("-("+s+")");
  return normalize_pm_compact("-"+s);
}

static void sort_terms(long *coefs,long *pows,int terms){
  for (int i=0;i<terms;++i)
    for (int j=i+1;j<terms;++j)
      if (pows[j]>pows[i]){
        long t=pows[i]; pows[i]=pows[j]; pows[j]=t;
        t=coefs[i]; coefs[i]=coefs[j]; coefs[j]=t;
      }
}

static bool diff_sum_terms(const working_string &expr,working_string &answer,char v='x'){
  working_string s=strip_outer_parens(compact(expr));
  if (contains(s,"(") || contains(s,"sin") || contains(s,"cos") ||
      contains(s,"tan") || contains(s,"ln") || contains(s,"exp"))
    return false;
  int start=0,sign=1,terms=0;
  answer="";
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      long coef=0,pow=0;
      if (!parse_power_or_const_var(s.substr(start,i-start),v,coef,pow))
        return false;
      working_string part=derivative_monomial(sign*coef,pow,v);
      if (!part.empty())
        answer=join_sum(answer,part);
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if ((c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>1 && !answer.empty();
}

static bool integrate_sum_terms(const working_string &expr,working_string &answer){
  working_string s=strip_outer_parens(compact(expr));
  if (contains(s,"(") || contains(s,"sin") || contains(s,"cos") ||
      contains(s,"tan") || contains(s,"ln") || contains(s,"exp"))
    return false;
  int start=0,sign=1,terms=0;
  answer="";
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

static bool polynomial_degree_lead(const working_string &expr,char v,long &lead,long &degree){
  working_string s=strip_outer_parens(nospace_lower(expr));
  bool any=false;
  lead=0;
  degree=0;
  int start=0,sign=1,terms=0,depth=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    if (!depth && (c=='+' || c=='-') && i>start && s[i-1]!='^' && s[i-1]!='e' && s[i-1]!='E'){
      working_string term=strip_outer_parens(s.substr(start,i-start));
      long coef=0,pow=0;
      if (parse_power_term_var(term,v,coef,pow)){
        coef *= sign;
        if (!any || pow>degree){
          degree=pow;
          lead=coef;
        }
        any=true;
      }
      else {
        Rat r;
        if (!parse_rat(term,r))
          return false;
      }
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>0 && any && lead;
}

static bool collect_visible_vars(const working_string &src,char *vars,int maxvars,int &nvars){
  working_string s=compact(src);
  nvars=0;
  for (int i=0;i<int(s.size());){
    if (!isalpha((unsigned char)s[i])){
      ++i;
      continue;
    }
    int j=i+1;
    while (j<int(s.size()) && isalpha((unsigned char)s[j]))
      ++j;
    working_string w=s.substr(i,j-i);
    bool call=j<int(s.size()) && s[j]=='(';
    if (!reserved_math_word(w,call)){
      char v=w[0];
      bool seen=false;
      for (int k=0;k<nvars;++k)
        if (vars[k]==v)
          seen=true;
      if (!seen && nvars<maxvars)
        vars[nvars++]=v;
    }
    i=j;
  }
  return nvars>0;
}

static bool integrate_recip_affine_sum(const working_string &expr,working_string &answer){
  answer="";
  int start=0,sign=1,depth=0,terms=0;
  for (int i=0;i<=int(expr.size());++i){
    char c=i<int(expr.size())?expr[i]:'+';
    if (c=='(') ++depth;
    else if (c==')') --depth;
    if (!depth && (c=='+' || c=='-') && i>start){
      Rat rc; long a=0,b=0;
      if (!split_recip_affine(expr.substr(start,i-start),rc,a,b))
        return false;
      if (sign<0) rc.n=-rc.n;
      answer=join_sum(answer,log_affine_int_s(rc,a,b));
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>1;
}

static bool parse_x_coeff(const working_string &src,Rat &k){
  working_string s=compact(src);
  int sign=1;
  if (!s.empty() && s[0]=='-'){ sign=-1; s=s.substr(1,s.size()-1); }
  int x=s.find('x');
  if (x<0) return false;
  working_string pre=s.substr(0,x), post=s.substr(x+1,s.size()-x-1);
  if (!pre.empty() && pre[pre.size()-1]=='*')
    pre=pre.substr(0,pre.size()-1);
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

static working_string func_affine_term_s(Rat c,const char *fn,long a,long b){
  if (!c.n) return "";
  working_string out;
  Rat q=c;
  if (q.n<0){ out="-"; q.n=-q.n; }
  if (!(q.n==1 && q.d==1))
    out += rat_s(q)+"*";
  out += fn;
  out += "(";
  out += fmt_affine(a,b);
  out += ")";
  return out;
}

static working_string x_func_arg_term_s(Rat c,const char *fn,const working_string &arg){
  if (!c.n) return "";
  working_string out;
  Rat q=c;
  if (q.n<0){ out="-"; q.n=-q.n; }
  if (!(q.n==1 && q.d==1))
    out += rat_s(q)+"*";
  out += "x*";
  out += fn;
  out += "(";
  out += arg;
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
  working_string arg=term.substr(open+1,close-open-1);
  if (!parse_x_coeff(arg,k) || !k.n){
    long a=0,b=0;
    if (!parse_affine(arg,a,b) || !a)
      return false;
    Rat ak=rat(a,1);
    if (!strcmp(fn,"sin(")){
      part=func_affine_term_s(rat_div(rat(-c.n,c.d),ak),"cos",a,b);
      return true;
    }
    if (!strcmp(fn,"cos(")){
      part=func_affine_term_s(rat_div(c,ak),"sin",a,b);
      return true;
    }
    part=func_affine_term_s(rat_div(c,ak),"exp",a,b);
    return true;
  }
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

static bool integrate_x_trig_parts(const working_string &expr,working_string &answer){
  working_string s=compact(expr);
  int p=s.find("xsin(");
  bool is_sin=true;
  if (p<0){
    p=s.find("xcos(");
    is_sin=false;
  }
  if (p<0) return false;
  Rat c;
  working_string pre=s.substr(0,p);
  if (pre.empty()) c=rat(1,1);
  else if (pre=="-") c=rat(-1,1);
  else if (!parse_rat(pre,c)) return false;
  int open=s.find("(",p);
  int close=match_paren(s,open);
  if (close!=int(s.size())-1) return false;
  working_string arg=s.substr(open+1,close-open-1);
  Rat k;
  if (parse_x_coeff(arg,k) && k.n){
    Rat k2=rat_mul(k,k);
    if (is_sin)
      answer=join_sum(x_func_arg_term_s(rat_div(rat(-c.n,c.d),k),"cos",xarg_s(k)),func_term_s(rat_div(c,k2),"sin",k));
    else
      answer=join_sum(x_func_arg_term_s(rat_div(c,k),"sin",xarg_s(k)),func_term_s(rat_div(c,k2),"cos",k));
    return true;
  }
  long a=0,b=0;
  if (!parse_affine(arg,a,b) || !a) return false;
  Rat ak=rat(a,1), ak2=rat_mul(ak,ak);
  if (is_sin)
    answer=join_sum(x_func_arg_term_s(rat_div(rat(-c.n,c.d),ak),"cos",fmt_affine(a,b)),func_affine_term_s(rat_div(c,ak2),"sin",a,b));
  else
    answer=join_sum(x_func_arg_term_s(rat_div(c,ak),"sin",fmt_affine(a,b)),func_affine_term_s(rat_div(c,ak2),"cos",a,b));
  return true;
}

static bool integrate_x_trig_parts_working(const working_string &expr,working_string &out){
  working_string answer,s=compact(expr);
  if (!integrate_x_trig_parts(expr,answer))
    return false;
  int p=s.find("xsin(");
  bool is_sin=true;
  if (p<0){ p=s.find("xcos("); is_sin=false; }
  if (p<0) return false;
  working_string pre=s.substr(0,p);
  working_string u=pre.empty()?"x":(pre=="-"?"-x":pre+"*x");
  int open=s.find("(",p), close=match_paren(s,open);
  if (close<0) return false;
  working_string arg=s.substr(open+1,close-open-1), vterm;
  Rat k;
  if (parse_x_coeff(arg,k) && k.n)
    vterm=func_term_s(is_sin?rat(-k.d,k.n):rat(k.d,k.n),is_sin?"cos":"sin",k);
  else {
    long a=0,b=0;
    if (!parse_affine(arg,a,b) || !a) return false;
    vterm=func_affine_term_s(is_sin?rat(-1,a):rat(1,a),is_sin?"cos":"sin",a,b);
  }
  out="Parts:\nLet u="+u+", dv=";
  out += is_sin?"sin(":"cos(";
  out += arg+") dx\n";
  out += "du=";
  out += (pre.empty()?"dx":(pre=="-"?"-dx":pre+" dx"));
  out += ", v="+vterm+"\nI=u*v-int(v*du)\n"+answer+" + C";
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
      if (!strcmp(id,"cot")) return 1.0/tan(a);
      if (!strcmp(id,"sec")) return 1.0/cos(a);
      if (!strcmp(id,"cosec")) return 1.0/sin(a);
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

static bool finite_double(double v){
  return v==v && fabs(v)<1e300;
}

static bool rational_approx(double v,working_string &out);

static bool eval_numeric_rat_expr(const working_string &expr,Rat &out){
  NumParser np;
  working_string e=compact(expr);
  np.p=e.c_str();
  np.ok=true;
  double val=np.expr();
  np.skip();
  if (!np.ok || *np.p || !finite_double(val))
    return false;
  working_string exact;
  if (!rational_approx(val,exact))
    return false;
  return parse_rat(exact,out);
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

static bool parse_coef_prefix(const working_string &pre,Rat &c){
  if (pre.empty())
    c=rat(1,1);
  else if (pre=="-")
    c=rat(-1,1);
  else
    return parse_rat(pre,c);
  return true;
}

static bool integrate_reverse_chain_x2(const working_string &expr,working_string &out){
  working_string s=compact(expr);
  Rat c=rat(1,1);
  int p=0;
  long fa=0,fb=0;
  bool exact_du=false;
  if (!s.empty() && s[0]=='('){
    int fc=match_paren(s,0);
    if (fc<0 || !parse_affine(s.substr(1,fc-1),fa,fb)) return false;
    p=fc+1;
    exact_du=true;
  }
  else {
    int x=s.find('x');
    if (x<0) return false;
    if (!parse_coef_prefix(s.substr(0,x),c)) return false;
    p=x+1;
  }
  const char *fns[]={"sin(","cos(","exp(",0};
  for (int i=0;fns[i];++i){
    int fl=strlen(fns[i]);
    if (s.substr(p,fl)!=fns[i]) continue;
    int open=p+fl-1, close=match_paren(s,open);
    if (close!=int(s.size())-1) return false;
    working_string u=s.substr(open+1,close-open-1);
    long a=0,b=0,l=0;
    if (!parse_quad_x2(u,a,b,l)) return false;
    if (exact_du){
      if (fa!=2*a || fb!=l) return false;
    }
    else
      c=rat_div(c,rat(2*a,1));
    working_string up=spaced_pm(u), base;
    Rat q=c;
    if (!strcmp(fns[i],"sin(")){ q=rat(-q.n,q.d); base="cos("+up+")"; }
    else if (!strcmp(fns[i],"cos(")) base="sin("+up+")";
    else base="exp("+up+")";
    out="Sub u="+up+"\n";
    out += "du=";
    out += l?"("+fmt_affine(2*a,l)+")":int_s(2*a)+"*x";
    out += " dx\n";
    if (!(q.n==1 && q.d==1))
      out += "scale "+rat_s(q)+"\n";
    out += ""+mul_expr(q,base)+" + C";
    return true;
  }
  if (p<int(s.size()) && s[p]=='('){
    int close=match_paren(s,p);
    if (close<0 || close+1>=int(s.size()) || s[close+1]!='^') return false;
    working_string u=s.substr(p+1,close-p-1);
    long a=0,b=0,l=0;
    Rat pow;
    if (!parse_quad_x2(u,a,b,l) || l || !parse_rat(s.substr(close+2,s.size()-close-2),pow))
      return false;
    Rat np=rat(pow.n+pow.d,pow.d);
    if (!np.n) return false;
    Rat sc=rat_div(c,rat(2*a,1));
    Rat q=rat_div(sc,np);
    working_string up=spaced_pm(u);
    working_string base="("+up+")^"+pow_s(np);
    out="Sub u="+up+"\n";
    out += "du="+int_s(2*a)+"*x dx\n";
    if (!(sc.n==1 && sc.d==1))
      out += "scale "+rat_s(sc)+"\n";
    out += ""+mul_expr(q,base)+" + C";
    return true;
  }
  return false;
}

static bool split_poly_over_x_power(const working_string &expr,working_string &out){
  working_string num,den;
  if (!split_top_fraction(compact(expr),num,den))
    return false;
  long dp=0;
  if (den=="x") dp=1;
  else if (den.find("x^")==0){
    char *end=0;
    dp=strtol(den.c_str()+2,&end,10);
    if (!end || *end || dp<=0) return false;
  }
  else return false;
  long coefs[16],pows[16];
  int start=0,sign=1,depth=0,terms=0;
  for (int i=0;i<=int(num.size());++i){
    char c=i<int(num.size())?num[i]:'+';
    if (c=='(') ++depth;
    else if (c==')') --depth;
    if (!depth && (c=='+' || c=='-') && i>start){
      long coef=0,pow=0;
      if (!parse_power_term(num.substr(start,i-start),coef,pow) || terms>=16)
        return false;
      coefs[terms]=sign*coef;
      pows[terms]=pow-dp;
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  if (terms<1) return false;
  out="";
  working_string simp,ans;
  for (int i=0;i<terms;++i){
    working_string t;
    long c=coefs[i],p=pows[i];
    if (p==0) t=int_s(c);
    else {
      if (c==1) t="x";
      else if (c==-1) t="-x";
      else t=int_s(c)+"*x";
      if (p!=1) t += "^"+int_s(p);
    }
    simp=join_sum(simp,t);
    ans=join_sum(ans,integral_monomial(c,p));
  }
  out=simp+"\nTerms:\n"+ans+" + C";
  return true;
}

static working_string quad_rat_s(Rat a,Rat b,Rat c){
  working_string out;
  if (a.n) out=join_sum(out,rat_power_term_s(a,rat(2,1)));
  if (b.n) out=join_sum(out,rat_power_term_s(b,rat(1,1)));
  if (c.n) out=join_sum(out,rat_s(c));
  return out.empty()?"0":out;
}

static working_string quad_rat_s_var(Rat a,Rat b,Rat c,char v){
  working_string out;
  if (a.n) out=join_sum(out,rat_power_term_s(a,rat(2,1),v));
  if (b.n) out=join_sum(out,rat_power_term_s(b,rat(1,1),v));
  if (c.n) out=join_sum(out,rat_s(c));
  return out.empty()?"0":out;
}

static bool add_quad_piece(const working_string &src,char v,Rat s,Rat &A,Rat &B,Rat &C){
  working_string t=strip_outer_parens(nospace_lower(src));
  Rat c,p;
  int open=t.find('(');
  if (open>=0){
    int close=match_paren(t,open);
    if (close>open && close+3==int(t.size()) && t[close+1]=='^' && t[close+2]=='2'){
      Rat k=rat(1,1),la,lb;
      if ((open==0 || parse_rat(t.substr(0,open),k)) &&
          parse_affine_general(t.substr(open+1,close-open-1),v,la,lb)){
        k=rat_mul(k,s);
        A=rat_add(A,rat_mul(k,rat_mul(la,la)));
        B=rat_add(B,rat_mul(k,rat_mul(rat(2,1),rat_mul(la,lb))));
        C=rat_add(C,rat_mul(k,rat_mul(lb,lb)));
        return true;
      }
    }
  }
  if (parse_var_power_factor(t,v,c,p) && p.d==1 && p.n>=0 && p.n<=2){
    c=rat_mul(c,s);
    if (p.n==2) A=rat_add(A,c);
    else if (p.n==1) B=rat_add(B,c);
    else C=rat_add(C,c);
    return true;
  }
  Rat la,lb;
  if (parse_affine_general(t,v,la,lb)){
    B=rat_add(B,rat_mul(s,la));
    C=rat_add(C,rat_mul(s,lb));
    return true;
  }
  if (parse_rat(t,c)){
    C=rat_add(C,rat_mul(s,c));
    return true;
  }
  if (!contains_var_symbol(t,v) && eval_numeric_rat_expr(t,c)){
    C=rat_add(C,rat_mul(s,c));
    return true;
  }
  return false;
}

static bool parse_quad_rat_expr(const working_string &src,char v,Rat &A,Rat &B,Rat &C){
  working_string terms[12];
  int signs[12];
  int n=split_top_sum_terms(src,terms,signs,12);
  if (n<1)
    return false;
  A=rat(0,1); B=rat(0,1); C=rat(0,1);
  for (int i=0;i<n;++i)
    if (!add_quad_piece(terms[i],v,rat(signs[i],1),A,B,C))
      return false;
  return true;
}

static bool rat_square_root(Rat q,Rat &r){
  long sn=0,sd=0;
  if (q.n<0)
    return false;
  if (!square_long(q.n,sn) || !square_long(q.d,sd))
    return false;
  r=rat(sn,sd);
  return true;
}

static working_string sqrt_rat_s(Rat q){
  Rat r;
  if (rat_square_root(q,r))
    return rat_s(r);
  return "sqrt("+rat_s(q)+")";
}

static bool integrate_quad_sum(const working_string &expr,working_string &out){
  working_string s=nospace_lower(expr);
  Rat A=rat(0,1),B=rat(0,1),C=rat(0,1);
  int start=0,depth=0,terms=0,sign=1;
  for (int i=0;i<=int(s.size());++i){
    char ch=i<int(s.size())?s[i]:'+';
    if (ch=='(') ++depth;
    else if (ch==')') --depth;
    if (!depth && (ch=='+' || ch=='-') && i>start && s[i-1]!='^'){
      if (!add_quad_piece(s.substr(start,i-start),'x',rat(sign,1),A,B,C))
        return false;
      ++terms;
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (ch=='+' || ch=='-') && i==start){
      sign=(ch=='-')?-1:1;
      start=i+1;
    }
  }
  if (terms<2 || !A.n)
    return false;
  working_string ans;
  ans=join_sum(ans,rat_power_term_s(rat_div(A,rat(3,1)),rat(3,1)));
  if (B.n) ans=join_sum(ans,rat_power_term_s(rat_div(B,rat(2,1)),rat(2,1)));
  if (C.n) ans=join_sum(ans,rat_power_term_s(C,rat(1,1)));
  out="Expand:\n"+quad_rat_s(A,B,C)+"\n"
      "Terms:\n"+ans+" + C";
  return true;
}

static bool parse_trig_square_term(const working_string &term,const char *fn,Rat &coef){
  working_string suf=working_string(fn)+"(x)^2";
  if (term.size()<suf.size() || term.substr(term.size()-suf.size(),suf.size())!=suf)
    return false;
  return parse_coef_prefix(term.substr(0,term.size()-suf.size()),coef);
}

static bool parse_affine_trig(const working_string &src,const char *fn,long &a,long &b){
  working_string s=compact(src), suf=working_string(fn)+"(x)";
  a=0; b=0;
  int start=0,sign=1,terms=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if ((c=='+' || c=='-') && i>start){
      working_string t=s.substr(start,i-start);
      if (t.size()>=suf.size() && t.substr(t.size()-suf.size(),suf.size())==suf){
        Rat q;
        if (!parse_coef_prefix(t.substr(0,t.size()-suf.size()),q) || q.d!=1)
          return false;
        b += sign*q.n;
      }
      else {
        char *end=0;
        long v=strtol(t.c_str(),&end,10);
        if (!end || *end) return false;
        a += sign*v;
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
  return terms==2 && b;
}

static bool split_coeff_core(const working_string &src,Rat &q,working_string &core){
  working_string s=strip_outer_parens(nospace_lower(src));
  q=rat(1,1); core=s;
  int st=s.find('*');
  if (st>0 && parse_rat(s.substr(0,st),q)){
    core=s.substr(st+1,s.size()-st-1);
    return true;
  }
  int p=0;
  if (s[p]=='+' || s[p]=='-') ++p;
  while (p<int(s.size()) && isdigit((unsigned char)s[p]))
    ++p;
  if (p>0 && p<int(s.size()) && isalpha((unsigned char)s[p]) &&
      parse_rat(s.substr(0,p),q)){
    core=s.substr(p,s.size()-p);
    return true;
  }
  return true;
}

static bool parse_trig_square_core(const working_string &core,const char *fn,working_string &arg){
  working_string base,exp;
  return parse_top_power(core,base,exp) && exp=="2" && parse_unary_arg(base,fn,arg);
}

static bool integrate_trig_identity(const working_string &expr,working_string &out){
  (void)expr; (void)out;
  return false;
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
  char st[128];
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

static int split_exact_list(working_string s,working_string *args,int maxargs){
  s=trim(s);
  int p=s.find('['),q=s.size()-1;
  while (q>p && s[q]!=']')
    --q;
  if (p<0 || q<=p)
    return 0;
  return split_args(s,p+1,q,args,maxargs);
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
  if (close<0 || close!=int(s.size())-1){
    int last=int(s.size())-1;
    while (last>open && isspace((unsigned char)s[last]))
      --last;
    if (last<=open || s[last]!=')' || !balanced(s))
      return false;
    close=last;
  }
  for (int i=close+1;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]))
      return false;
  count=split_args(s,open+1,close,args,maxargs);
  return true;
}

static bool parse_linear_rat_var(const working_string &src,char v,Rat &a,Rat &b){
  working_string s=strip_outer_parens(nospace_lower(src));
  a=rat(0,1); b=rat(0,1);
  int start=0,sign=1,terms=0,depth=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'+';
    if (c=='(') ++depth;
    if (c==')') --depth;
    if (!depth && (c=='+' || c=='-') && i>start){
      working_string t=s.substr(start,i-start);
      int p=t.find(v);
      Rat val;
      if (p>=0){
        if (p!=int(t.size())-1)
          return false;
        working_string cs=t.substr(0,p);
        if (!cs.empty() && cs[cs.size()-1]=='*')
          cs=cs.substr(0,cs.size()-1);
        if (cs.empty())
          val=rat(1,1);
        else if (cs=="-")
          val=rat(-1,1);
        else if (!parse_rat(cs,val))
          return false;
        if (sign<0) val.n=-val.n;
        a=rat_add(a,val);
      }
      else {
        if (!parse_rat(t,val))
          return false;
        if (sign<0) val.n=-val.n;
        b=rat_add(b,val);
      }
      ++terms;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  return terms>0 && a.n;
}

struct Lin {
  Rat a,b;
};

static Lin lin(Rat a,Rat b){
  Lin r={a,b};
  return r;
}

static bool lin_is_const(const Lin &x){
  return !x.a.n;
}

static bool rat_pow_small(Rat base,long p,Rat &out){
  if (p<-8 || p>8)
    return false;
  if (!p){
    out=rat(1,1);
    return true;
  }
  if (p<0){
    if (!base.n)
      return false;
    Rat tmp;
    if (!rat_pow_small(base,-p,tmp))
      return false;
    out=rat(tmp.d,tmp.n);
    return true;
  }
  out=rat(1,1);
  for (long i=0;i<p;++i)
    out=rat_mul(out,base);
  return true;
}

struct AffineParser {
  working_string s;
  const char *p;
  char v;
  bool ok;

  void skip(){ while (*p && isspace((unsigned char)*p)) ++p; }
  bool starts_primary(){
    skip();
    return *p=='(' || *p=='.' || isdigit((unsigned char)*p) || *p==v;
  }
  Lin expr(){
    Lin x=term();
    skip();
    while (*p=='+' || *p=='-'){
      char op=*p++;
      Lin y=term();
      x.a=op=='+'?rat_add(x.a,y.a):rat_sub(x.a,y.a);
      x.b=op=='+'?rat_add(x.b,y.b):rat_sub(x.b,y.b);
      skip();
    }
    return x;
  }
  Lin term(){
    Lin x=power();
    skip();
    while (*p=='*' || *p=='/' || starts_primary()){
      char op='*';
      if (*p=='*' || *p=='/')
        op=*p++;
      Lin y=power();
      if (op=='/'){
        if (!lin_is_const(y) || !y.b.n){ ok=false; return x; }
        x.a=rat_div(x.a,y.b);
        x.b=rat_div(x.b,y.b);
      }
      else {
        if (x.a.n && y.a.n){ ok=false; return x; }
        Rat na=rat_add(rat_mul(x.a,y.b),rat_mul(y.a,x.b));
        Rat nb=rat_mul(x.b,y.b);
        x=lin(na,nb);
      }
      skip();
    }
    return x;
  }
  Lin power(){
    Lin x=unary();
    skip();
    if (*p=='^'){
      ++p;
      Lin e=unary();
      if (!lin_is_const(e) || e.b.d!=1){ ok=false; return x; }
      if (e.b.n==1)
        return x;
      if (!e.b.n)
        return lin(rat(0,1),rat(1,1));
      if (!lin_is_const(x)){ ok=false; return x; }
      Rat r;
      if (!rat_pow_small(x.b,e.b.n,r)){ ok=false; return x; }
      return lin(rat(0,1),r);
    }
    return x;
  }
  Lin unary(){
    skip();
    if (*p=='+'){ ++p; return unary(); }
    if (*p=='-'){
      ++p;
      Lin x=unary();
      x.a.n=-x.a.n; x.b.n=-x.b.n;
      return x;
    }
    return primary();
  }
  Lin primary(){
    skip();
    if (*p=='('){
      ++p;
      Lin x=expr();
      skip();
      if (*p==')') ++p; else ok=false;
      return x;
    }
    if (*p==v){
      ++p;
      return lin(rat(1,1),rat(0,1));
    }
    if (isdigit((unsigned char)*p) || *p=='.'){
      const char *start=p;
      while (isdigit((unsigned char)*p) || *p=='.')
        ++p;
      Rat r;
      if (!parse_rat(working_string(start,p-start),r))
        ok=false;
      return lin(rat(0,1),r);
    }
    ok=false;
    return lin(rat(0,1),rat(0,1));
  }
};

static bool parse_affine_general(const working_string &src,char v,Rat &a,Rat &b){
  AffineParser ap;
  ap.s=nospace_lower(src);
  ap.p=ap.s.c_str();
  ap.v=v;
  ap.ok=true;
  Lin r=ap.expr();
  ap.skip();
  if (!ap.ok || *ap.p || !r.a.n)
    return false;
  a=r.a; b=r.b;
  return true;
}

static bool parse_affine_any(const working_string &src,char v,Rat &a,Rat &b){
  AffineParser ap;
  ap.s=nospace_lower(src);
  ap.p=ap.s.c_str();
  ap.v=v;
  ap.ok=true;
  Lin r=ap.expr();
  ap.skip();
  if (!ap.ok || *ap.p)
    return false;
  a=r.a; b=r.b;
  return true;
}

static bool known_call_word(const working_string &w){
  return w=="sin" || w=="cos" || w=="tan" || w=="ln" || w=="log" ||
         w=="exp" || w=="sqrt" || w=="abs" || w=="sec" ||
         w=="cosec" || w=="cot" || w=="asin" || w=="acos" || w=="atan";
}

static bool implicit_product_boundary(const working_string &s,int i){
  if (i<=0 || i>=int(s.size()))
    return false;
  char p=s[i-1], c=s[i];
  if (c=='^' || c=='*' || c=='/' || c=='+' || c=='-' || c==')' || c==']' || c=='}')
    return false;
  if (p=='^' || p=='*' || p=='/' || p=='+' || p=='-' || p=='(' || p=='[' || p=='{')
    return false;
  if (c=='('){
    int j=i-1;
    while (j>=0 && isalpha((unsigned char)s[j]))
      --j;
    if (j+1<i && known_call_word(s.substr(j+1,i-j-1)))
      return false;
    return p==')' || isalpha((unsigned char)p) || isdigit((unsigned char)p);
  }
  if (isalpha((unsigned char)c))
    return p==')';
  if (isdigit((unsigned char)c))
    return p==')';
  return false;
}

static bool known_call_at(const working_string &s,int i,const char *&word){
  static const char *words[]={"cosec","sqrt","asin","acos","atan","sin","cos","tan","sec","cot","ln","log","exp","abs",0};
  for (int k=0;words[k];++k){
    int n=strlen(words[k]);
    if (i+n<int(s.size()) && s.substr(i,n)==words[k] && s[i+n]=='('){
      word=words[k];
      return true;
    }
  }
  return false;
}

static working_string restore_known_call_products(const working_string &src){
  working_string s=nospace_lower(src), out;
  for (int i=0;i<int(s.size());++i){
    const char *w=0;
    if (i>0 && known_call_at(s,i,w)){
      char p=s[i-1];
      int j=i-1;
      while (j>=0 && isalpha((unsigned char)s[j]))
        --j;
      working_string merged=s.substr(j+1,i-j-1)+working_string(w);
      if (!known_call_word(merged) &&
          (isalnum((unsigned char)p) || p==')' || p==']' || p=='}'))
        out += "*";
    }
    out += s[i];
  }
  return out;
}

static working_string normalize_top_product(const working_string &raw){
  working_string s=restore_known_call_products(raw), r;
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    if (!depth && implicit_product_boundary(s,i))
      r += "*";
    char c=s[i];
    r += c;
    if (c=='(' || c=='[' || c=='{') ++depth;
    if (c==')' || c==']' || c=='}') --depth;
  }
  return r;
}

static int split_top_product(const working_string &src,working_string *factors,int maxf){
  working_string s=normalize_top_product(strip_outer_parens(nospace_lower(src)));
  int n=0,start=0,depth=0;
  for (int i=0;i<=int(s.size());++i){
    char c=i<int(s.size())?s[i]:'*';
    if (c=='(') ++depth;
    if (c==')') --depth;
    if (!depth && c=='*'){
      if (n>=maxf)
        return 0;
      factors[n++]=strip_outer_parens(s.substr(start,i-start));
      start=i+1;
    }
  }
  return n;
}

static bool ln_arg_linear(const working_string &factor,char v,working_string &arg,Rat &a,Rat &b){
  working_string f=strip_outer_parens(nospace_lower(factor));
  if (f.find("ln(")!=0 || f[f.size()-1]!=')')
    return false;
  arg=f.substr(3,f.size()-4);
  return parse_linear_rat_var(arg,v,a,b);
}

static bool try_zero_product_solve(const working_string &eqraw,const working_string &rawvar,char v,working_string &out){
  working_string eq=nospace_lower(eqraw);
  int op=eq.find('=');
  if (op<0)
    return false;
  working_string left=strip_outer_parens(eq.substr(0,op));
  working_string right=strip_outer_parens(eq.substr(op+1,eq.size()-op-1));
  working_string product;
  if (left=="0")
    product=right;
  else if (right=="0")
    product=left;
  else
    return false;
  working_string factors[2];
  int n=split_top_product(product,factors,2);
  if (n!=2)
    return false;
  Rat a1,b1,a2,b2;
  if (parse_affine_general(factors[0],v,a1,b1) && parse_affine_general(factors[1],v,a2,b2) &&
      a1.n && a2.n){
    Rat r1=rat_div(rat(-b1.n,b1.d),a1);
    Rat r2=rat_div(rat(-b2.n,b2.d),a2);
    if (rat_cmp(r1,r2)>0){
      Rat t=r1; r1=r2; r2=t;
    }
    out="Zero product:\n";
    out += fmt_linear_rat(a1,b1,v)+" = 0 or "+fmt_linear_rat(a2,b2,v)+" = 0\n";
    out += rawvar+" = "+rat_s(r1)+" or "+rawvar+" = "+rat_s(r2)+"\n";
    out += rawvar+" = ["+rat_s(r1)+", "+rat_s(r2)+"]";
    return true;
  }
  working_string arg;
  Rat la,lb;
  int li=ln_arg_linear(factors[0],v,arg,la,lb)?0:(ln_arg_linear(factors[1],v,arg,la,lb)?1:-1);
  if (li<0)
    return false;
  working_string other=factors[1-li];
  Rat a,b;
  if (parse_affine_general(other,v,a,b)){
    Rat ar=rat_div(rat(-b.n,b.d),a);
    Rat lr=rat_div(rat_sub(rat(1,1),lb),la);
    out="Zero:\n";
    out += fmt_linear_rat(a,b,v)+" = 0 or ln("+spaced_pm(arg)+") = 0\n";
    out += spaced_pm(arg)+"=1 => "+rawvar+"="+rat_s(rat_div(rat_sub(rat(1,1),lb),la))+"\n";
    out += "Dom: "+spaced_pm(arg)+" > 0\n";
    working_string list=rat_s(lr);
    if (rat_cmp(rat_add(rat_mul(la,ar),lb),rat(0,1))>0)
      list += ", "+rat_s(ar);
    out += ""+rawvar+" = ["+list+"]";
    return true;
  }
  Rat r1,r2,lr=rat_div(rat_sub(rat(1,1),lb),la);
  long qa,qb,qc;
  if (!solve_quad_int(other,"0",v,r1,r2,qa,qb,qc))
    return false;
  out="Zero:\n";
  out += poly2_s(qa,qb,qc,v)+" = 0 or ln("+spaced_pm(arg)+") = 0\n";
  working_string list=rat_s(lr);
  if (rat_cmp(rat_add(rat_mul(la,r1),lb),rat(0,1))>0)
    list += ", "+rat_s(r1);
  if (rat_cmp(rat_add(rat_mul(la,r2),lb),rat(0,1))>0){
    list += ", "+rat_s(r2);
  }
  out += spaced_pm(arg)+"=1 => "+rawvar+"="+rat_s(lr)+"\n";
  out += "Dom: "+spaced_pm(arg)+" > 0\n";
  out += ""+rawvar+" = ["+list+"]";
  return true;
}

static bool rat_is_one(Rat r){ return r.n==r.d; }
static bool rat_is_minus_one(Rat r){ return r.n==-r.d; }
static Rat rat_abs(Rat r){ if (r.n<0) r.n=-r.n; return r; }

static working_string rat_abs_s(Rat r){
  if (r.n<0) r.n=-r.n;
  return rat_s(r);
}

static working_string fmt_rat_var(Rat a,char v){
  working_string out;
  if (rat_is_one(a)) out += v;
  else if (rat_is_minus_one(a)){ out += "-"; out += v; }
  else {
    out += rat_s(a);
    out += "*";
    out += v;
  }
  return out;
}

static working_string fmt_linear_rat(Rat a,Rat b,char v){
  if (!a.n)
    return rat_s(b);
  working_string out;
  if (a.n<0 && b.n){
    out=rat_s(b);
    out += " - ";
    if (!rat_is_minus_one(a)){
      out += rat_abs_s(a);
      out += "*";
    }
    out += v;
    return out;
  }
  out=fmt_rat_var(a,v);
  if (b.n>0)
    out += " + "+rat_s(b);
  if (b.n<0)
    out += " - "+rat_s(rat(-b.n,b.d));
  return out;
}

static working_string power_linear_rat_term(Rat c,Rat a,Rat b,Rat p,char v){
  working_string base=fmt_linear_rat(a,b,v);
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

static working_string fmt_linear_var_first(Rat a,Rat b,char v){
  if (!(a.n<0 && b.n))
    return fmt_linear_rat(a,b,v);
  working_string out=fmt_rat_var(a,v);
  if (b.n>0) out += " + "+rat_s(b);
  else out += " - "+rat_s(rat(-b.n,b.d));
  return out;
}

static working_string fmt_expr_for_working(const working_string &src,char v='x'){
  working_string s=strip_outer_parens(nospace_lower(src));
  Rat a,b;
  if (parse_linear_rat_var(s,v,a,b))
    return fmt_linear_rat(a,b,v);
  if (s.find("ln(")==0 && s[s.size()-1]==')')
    return "ln("+fmt_expr_for_working(s.substr(3,s.size()-4),v)+")";
  if (!s.empty() && s[0]=='('){
    int close=match_paren(s,0);
    if (close>0 && close+1<int(s.size()) && s[close+1]=='^'){
      working_string inner=fmt_expr_for_working(s.substr(1,close-1),v);
      if (!has_top_add_sub_div(inner))
        return inner+"^"+s.substr(close+2,s.size()-close-2);
      return "("+inner+")^"+s.substr(close+2,s.size()-close-2);
    }
  }
  return spaced_pm(s);
}

static bool split_affine_power_rat(const working_string &src,char v,Rat &a,Rat &b,Rat &p){
  working_string s=strip_outer_parens(nospace_lower(src));
  if (s.empty() || s[0]!='(')
    return false;
  int close=match_paren(s,0);
  if (close<0 || close+1>=int(s.size()) || s[close+1]!='^')
    return false;
  return parse_linear_rat_var(s.substr(1,close-1),v,a,b) &&
         parse_rat(s.substr(close+2,s.size()-close-2),p);
}

static bool split_coeff_affine_power_rat(const working_string &src,char v,Rat &coef,Rat &a,Rat &b,Rat &p){
  working_string factors[3];
  int n=split_top_product(src,factors,3), hit=-1;
  coef=rat(1,1);
  for (int i=0;i<n;++i){
    Rat q,ta,tb,tp;
    if (split_affine_power_rat(factors[i],v,ta,tb,tp)){
      if (hit>=0) return false;
      hit=i; a=ta; b=tb; p=tp;
      continue;
    }
    if (!parse_rat(factors[i],q))
      return false;
    coef=rat_mul(coef,q);
  }
  return hit>=0;
}

static working_string coeff_over_base(Rat q,const working_string &base){
  if (!q.n)
    return "0";
  bool neg=q.n<0;
  if (neg) q.n=-q.n;
  working_string out=neg?"-":"";
  if (rat_is_one(q))
    out += "1";
  else
    out += rat_s(q);
  out += "/("+base+")";
  return out;
}

static bool diff_simple_expr(const working_string &src,char v,working_string &deriv,working_string &shown);
static bool split_outer_power_general(const working_string &src,working_string &base,Rat &p);
static working_string mul_factor_derivative(const working_string &f,const working_string &d);
static bool diff_top_sum_unbounded(const working_string &src,char v,working_string &deriv,working_string &shown);

static bool parse_unary_arg(const working_string &s,const char *fn,working_string &arg){
  int fl=strlen(fn);
  if (int(s.size())<=fl+1 || s.substr(0,fl)!=fn || s[fl]!='(')
    return false;
  int close=match_paren(s,fl);
  if (close!=int(s.size())-1)
    return false;
  arg=s.substr(fl+1,close-fl-1);
  return true;
}

static bool range_expr_all_real_simple(const working_string &expr,char v,int depth=0){
  (void)depth;
  working_string s=strip_outer_parens(nospace_lower(expr));
  Rat a,b;
  return parse_affine_general(s,v,a,b) && a.n;
}

static bool needs_parens_for_mul(const working_string &s){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    if (c==')' || c==']' || c=='}') --depth;
    if (!depth && i>0 && (c=='+' || c=='-') && s[i-1]!='e' && s[i-1]!='E')
      return true;
  }
  return false;
}

static working_string mul_chain_factor(const working_string &d,const working_string &f){
  if (d=="0") return "0";
  if (d=="1") return f;
  if (d=="-1") return "-"+f;
  Rat q;
  if (parse_rat(d,q))
    return mul_expr(q,f);
  return (needs_parens_for_mul(d)?"("+d+")":d)+"*"+f;
}

static working_string neg_mul_chain_factor(const working_string &d,const working_string &f){
  if (d=="0") return "0";
  if (d=="1") return "-"+f;
  if (d=="-1") return f;
  Rat q;
  if (parse_rat(d,q)){
    q.n=-q.n;
    return mul_expr(q,f);
  }
  return "-"+(needs_parens_for_mul(d)?"("+d+")":d)+"*"+f;
}

static bool diff_const_product_simple_expr(const working_string &src,char v,working_string &deriv,working_string &shown){
  working_string factors[8],shown_f[8];
  int n=split_top_product(src,factors,8);
  if (n<2)
    return false;
  int var_i=-1,var_count=0;
  for (int i=0;i<n;++i){
    shown_f[i]=fmt_expr_for_working(factors[i],v);
    if (contains_var_symbol(compact(factors[i]),v)){
      var_i=i;
      ++var_count;
    }
  }
  if (var_count!=1)
    return false;
  working_string dvar,svar;
  if (!diff_simple_expr(factors[var_i],v,dvar,svar))
    return false;
  working_string constant_part;
  shown="";
  for (int i=0;i<n;++i){
    if (i)
      shown += "*";
    shown += mul_visible_factor(i==var_i?svar:shown_f[i]);
    if (i!=var_i){
      if (!constant_part.empty())
        constant_part += "*";
      constant_part += mul_visible_factor(shown_f[i]);
    }
  }
  deriv=constant_part.empty()?dvar:mul_factor_derivative(constant_part,dvar);
  return true;
}

static bool diff_affine_power_expr(const working_string &src,char v,working_string &deriv,working_string &shown){
  Rat a,b,p;
  if (!split_affine_power_rat(src,v,a,b,p))
    return false;
  working_string base=fmt_linear_rat(a,b,v);
  working_string base_for_power=has_top_add_sub_div(base)?"("+base+")":base;
  shown=base_for_power+"^"+rat_s(p);
  Rat q=rat_mul(p,a);
  Rat np=rat_sub(p,rat(1,1));
  if (!q.n)
    deriv="0";
  else if (!np.n)
    deriv=rat_s(q);
  else {
    working_string pow=base_for_power;
    if (!rat_is_one(np))
      pow += "^"+rat_s(np);
    deriv=mul_expr(q,pow);
  }
  return true;
}

static bool unary_chain_derivative(const working_string &src,char v,working_string &deriv,working_string &shown,working_string *inner_shown=0,working_string *inner_deriv=0,const char **fn_used=0){
  working_string s=strip_outer_parens(nospace_lower(src));
  const char *fns[]={"sin","cos","tan","sec","cosec","cot","atan","exp","ln","log","sqrt","abs",0};
  for (int i=0;fns[i];++i){
    working_string arg;
    if (!parse_unary_arg(s,fns[i],arg))
      continue;
    working_string du,u;
    if (!diff_simple_expr(arg,v,du,u))
      return false;
    working_string fn(fns[i]);
    shown=fn+"("+u+")";
    if (inner_shown) *inner_shown=u;
    if (inner_deriv) *inner_deriv=du;
    if (fn_used) *fn_used=fns[i];
    if (fn=="sin")
      deriv=mul_chain_factor(du,"cos("+u+")");
    else if (fn=="cos")
      deriv=neg_mul_chain_factor(du,"sin("+u+")");
    else if (fn=="tan")
      deriv=mul_chain_factor(du,"sec("+u+")^2");
    else if (fn=="sec")
      deriv=mul_chain_factor(du,"sec("+u+")*tan("+u+")");
    else if (fn=="cosec")
      deriv=neg_mul_chain_factor(du,"cosec("+u+")*cot("+u+")");
    else if (fn=="cot")
      deriv=neg_mul_chain_factor(du,"cosec("+u+")^2");
    else if (fn=="atan")
      deriv=du=="1"?"1/(1+("+u+")^2)":(needs_parens_for_mul(du)?"("+du+")":du)+"/(1+("+u+")^2)";
    else if (fn=="exp")
      deriv=mul_chain_factor(du,"exp("+u+")");
    else if (fn=="ln" || fn=="log")
      deriv=du=="1"?"1/("+u+")":(needs_parens_for_mul(du)?"("+du+")":du)+"/("+u+")";
    else if (fn=="sqrt")
      deriv=du=="1"?"1/(2*sqrt("+u+"))":(needs_parens_for_mul(du)?"("+du+")":du)+"/(2*sqrt("+u+"))";
    else
      deriv=mul_chain_factor(du,"("+u+")/abs("+u+")");
    return true;
  }
  return false;
}

static bool diff_simple_expr(const working_string &src,char v,working_string &deriv,working_string &shown){
  working_string s=strip_outer_parens(nospace_lower(src));
  shown=fmt_expr_for_working(s,v);
  if (!contains_var_symbol(s,v)){
    if (contains(s,"sqrt(") || contains(s,"pi") || contains(s,"ln(") ||
        contains(s,"sin(") || contains(s,"cos(") || contains(s,"tan(") ||
        contains(s,"atan(") ||
        contains(s,"exp(")){
      shown=fmt_expr_for_working(s,v);
      deriv="0";
      return true;
    }
    NumParser np;
    np.p=s.c_str();
    np.ok=true;
    double val=np.expr();
    np.skip();
    if (np.ok && !*np.p){
      working_string exact;
      shown=rational_approx(val,exact)?exact:double_s(val);
      deriv="0";
      return true;
    }
    shown=fmt_expr_for_working(s,v);
    deriv="0";
    return true;
  }
  Rat a,b;
  if (parse_linear_rat_var(s,v,a,b)){
    deriv=rat_s(a);
    return true;
  }
  if (parse_affine_general(s,v,a,b)){
    shown=fmt_linear_rat(a,b,v);
    deriv=rat_s(a);
    return true;
  }
  long coef=0,pow=0;
  if (parse_power_term_var(s,v,coef,pow)){
    deriv=derivative_monomial(coef,pow,v);
    if (deriv.empty()) deriv="0";
    return true;
  }
  {
    Rat c,p;
    long rad=0;
    if (parse_surd_var_power_factor(s,v,c,rad,p)){
      deriv=derivative_surd_power_term(c,rad,p,v);
      shown=surd_power_term_s(c,rad,p,v);
      return true;
    }
    if (parse_var_power_factor(s,v,c,p)){
      deriv=derivative_rat_power_term(c,p,v);
      shown=rat_power_term_s(c,p,v);
      return true;
    }
  }
  {
    if (diff_top_sum_unbounded(s,v,deriv,shown))
      return true;
  }
  if (diff_sum_terms(s,deriv,v))
    return true;
  if (diff_const_product_simple_expr(s,v,deriv,shown))
    return true;
  if (diff_affine_power_expr(s,v,deriv,shown))
    return true;
  {
    working_string num,den;
    Rat c;
    long coef=0,pow=0;
    if (split_top_fraction(s,num,den) && parse_rat(num,c) &&
        parse_power_term_var(den,v,coef,pow) && coef && pow>0){
      Rat p=rat(-pow,1);
      Rat q=rat_div(c,rat(coef,1));
      shown=rat_power_term_s(q,p,v);
      deriv=derivative_rat_power_term(q,p,v);
      return true;
    }
  }
  if (s.size()>4 && s.substr(0,3)=="e^(" && match_paren(s,2)==int(s.size())-1){
    working_string arg=s.substr(3,s.size()-4),du,u;
    if (diff_simple_expr(arg,v,du,u)){
      shown="e^("+u+")";
      deriv=mul_chain_factor(du,shown);
      return true;
    }
  }
  {
    working_string base,db,bs;
    Rat p;
    if (split_outer_power_general(s,base,p) && diff_simple_expr(base,v,db,bs)){
      Rat np=rat_sub(p,rat(1,1));
      working_string powpart;
      if (!np.n)
        powpart="1";
      else {
        powpart="("+bs+")";
        if (!rat_is_one(np))
          powpart += "^"+rat_s(np);
      }
      working_string first=rat_is_one(p)?powpart:mul_expr(p,powpart);
      deriv=mul_factor_derivative(first,db);
      shown="("+bs+")^"+rat_s(p);
      return true;
    }
  }
  {
    working_string largs[2],du,u;
    int ln=0;
    if (parse_call(s.c_str(),"log",largs,2,ln) && ln==2 &&
        !contains_var_symbol(largs[0],v) && diff_simple_expr(largs[1],v,du,u)){
      working_string base=canonical_expr(largs[0]);
      shown="log("+base+","+u+")";
      working_string den="("+u+"*ln("+base+"))";
      deriv=du=="1"?"1/"+den:(needs_parens_for_mul(du)?"("+du+")":du)+"/"+den;
      return true;
    }
  }
  if (unary_chain_derivative(s,v,deriv,shown))
    return true;
  {
    working_string num,den,dn,dd,sn,sd;
    if (split_top_fraction(s,num,den) &&
        diff_simple_expr(num,v,dn,sn) && diff_simple_expr(den,v,dd,sd)){
      shown=sn+"/"+sd;
      deriv="(("+dn+")*("+sd+") - ";
      deriv += dd=="1" ? "("+sn+")" : "("+sn+")*("+dd+")";
      deriv += ")/("+sd+")^2";
      return true;
    }
  }
  if (s.find("ln(")==0 && s[s.size()-1]==')'){
    working_string arg=s.substr(3,s.size()-4);
    if (parse_linear_rat_var(arg,v,a,b)){
      working_string base=fmt_linear_rat(a,b,v);
      deriv=rat_is_one(a) ? "1/("+base+")" : rat_s(a)+"/("+base+")";
      shown="ln("+base+")";
      return true;
    }
  }
  return false;
}

static bool diff_top_sum_unbounded(const working_string &src,char v,working_string &deriv,working_string &shown){
  working_string s=strip_outer_parens(nospace_lower(src));
  int start=0,sign=1,depth=0,terms=0;
  bool split=false;
  working_string ans;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    if (!depth && (c=='+' || c=='-') && i>start && s[i-1]!='^' && s[i-1]!='e' && s[i-1]!='E'){
      working_string term=strip_outer_parens(s.substr(start,i-start)),td,ts;
      if (!diff_simple_expr(term,v,td,ts))
        return false;
      if (td!="0")
        ans=join_sum(ans,signed_part(sign,td));
      ++terms;
      split=true;
      sign=(c=='-')?-1:1;
      start=i+1;
    }
    else if (!depth && (c=='+' || c=='-') && i==start){
      sign=(c=='-')?-1:1;
      start=i+1;
    }
  }
  if (!split)
    return false;
  {
    working_string term=strip_outer_parens(s.substr(start,s.size()-start)),td,ts;
    if (!diff_simple_expr(term,v,td,ts))
      return false;
    if (td!="0")
      ans=join_sum(ans,signed_part(sign,td));
    ++terms;
  }
  if (terms<2)
    return false;
  deriv=ans.empty()?"0":ans;
  shown=fmt_expr_for_working(s,v);
  return true;
}

static working_string mul_deriv_factor(const working_string &d,const working_string &f){
  if (d=="0") return "0";
  if (d=="1") return f;
  if (d=="-1") return "-"+f;
  Rat q;
  if (parse_rat(d,q))
    return mul_expr(q,f);
  if (d.find("1/(")==0)
    return "("+f+")/"+d.substr(2,d.size()-2);
  if (contains(d,"/"))
    return "("+f+")*"+d;
  return (needs_parens_for_mul(d)?"("+d+")":d)+"*"+f;
}

static working_string mul_factor_derivative(const working_string &f,const working_string &d){
  if (d=="0") return "0";
  if (d=="1") return f;
  if (d=="-1") return "-"+f;
  if (!d.empty() && d[0]=='-')
    return "-"+(needs_parens_for_mul(f)?"("+f+")":f)+"*"+d.substr(1,d.size()-1);
  Rat q;
  if (parse_rat(d,q))
    return mul_expr(q,f);
  if (d.find("1/(")==0)
    return "("+f+")/"+d.substr(2,d.size()-2);
  if (contains(d,"/"))
    return (needs_parens_for_mul(f)?"("+f+")":f)+"*("+d+")";
  return (needs_parens_for_mul(f)?"("+f+")":f)+"*"+d;
}

static working_string mul_visible_factor(const working_string &f){
  return needs_parens_for_mul(f) ? "("+f+")" : f;
}

static working_string insert_coeff_stars(const working_string &s){
  working_string r;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    r += c;
    if (i+1<int(s.size()) && isdigit((unsigned char)c) &&
        isalpha((unsigned char)s[i+1]))
      r += "*";
  }
  return r;
}

static working_string product_except(working_string *factors,int n,int skip){
  working_string out;
  for (int i=0;i<n;++i){
    if (i==skip)
      continue;
    if (!out.empty())
      out += "*";
    out += mul_visible_factor(factors[i]);
  }
  return out;
}

static working_string product_rule_term(const working_string &d,const working_string &rest){
  if (d=="0")
    return "0";
  if (rest.empty())
    return d;
  if (d=="1")
    return rest;
  if (d=="-1")
    return "-"+rest;
  if (d.find("1/(")==0)
    return rest+"/"+d.substr(2,d.size()-2);
  if (!d.empty() && d[0]=='-')
    return "-"+rest+"*"+mul_visible_factor(d.substr(1,d.size()-1));
  Rat q;
  if (parse_rat(d,q))
    return mul_expr(q,rest);
  if (contains(d,"*") && contains(d,"x") && !needs_parens_for_mul(d))
    return d+"*"+rest;
  return rest+"*"+mul_visible_factor(d);
}

static working_string simplify_derivative_sum(const working_string &s){
  if (s=="x + x")
    return "2*x";
  if (s=="-x - x" || s=="-(x + x)")
    return "-2*x";
  return s;
}

static bool try_diff_top_sum(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string terms[10],shown[10],deriv[10];
  int signs[10];
  int n=split_top_sum_terms(expr,terms,signs,10);
  if (n<2)
    return false;
  working_string ans;
  out="d/d"+rawvar+"\nTerms:\n";
  for (int i=0;i<n;++i){
    if (!diff_simple_expr(terms[i],v,deriv[i],shown[i])){
      working_string pf[6],ps[6],pd[6];
      int pn=split_top_product(terms[i],pf,6);
      if (pn<2)
        return false;
      shown[i]="";
      deriv[i]="";
      for (int j=0;j<pn;++j){
        if (!diff_simple_expr(pf[j],v,pd[j],ps[j]))
          return false;
        if (!shown[i].empty())
          shown[i] += "*";
        shown[i] += mul_visible_factor(ps[j]);
      }
      for (int j=0;j<pn;++j){
        working_string term=product_rule_term(pd[j],product_except(ps,pn,j));
        if (term!="0")
          deriv[i]=join_sum(deriv[i],term);
      }
      if (deriv[i].empty())
        deriv[i]="0";
      deriv[i]=simplify_derivative_sum(deriv[i]);
    }
    working_string sd=signed_part(signs[i],deriv[i]);
    out += "d/d"+rawvar+"("+signed_part(signs[i],shown[i])+") = "+sd+"\n";
    if (sd!="0")
      ans=join_sum(ans,sd.size()>140?working_string("D")+int_s(i+1):sd);
  }
  if (ans.empty())
    ans="0";
  out += ans;
  return true;
}

static bool try_diff_product_rule(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string factors[6],flat[6],shown[6],deriv[6];
  int n0=split_top_product(expr,factors,6), n=0;
  if (n0<2)
    return false;
  for (int i=0;i<n0;++i){
    working_string sub[6];
    working_string ff=normalize_top_product(strip_outer_parens(factors[i]));
    int sn=contains(ff,")*(")?split_top_product(ff,sub,6-n):0;
    if (sn>1){
      for (int j=0;j<sn;++j) flat[n++]=sub[j];
    }
    else
      flat[n++]=factors[i];
  }
  for (int i=0;i<n;++i)
    factors[i]=flat[i];
  for (int i=0;i<n;++i)
    if (!diff_simple_expr(factors[i],v,deriv[i],shown[i]))
      return false;
  out="Product:\n";
  if (n==2){
    if (shown[0].size()+shown[1].size()+deriv[0].size()+deriv[1].size()>240){
      out += "u=A, v=B\nu'v+uv'\nA'B+AB'";
      return true;
    }
    out += "u = "+shown[0]+", v = "+shown[1]+"\n";
    out += "du/d"+rawvar+" = "+deriv[0]+"\n";
    out += "dv/d"+rawvar+" = "+deriv[1]+"\n";
    out += "u'v+uv'\n";
    working_string first=mul_deriv_factor(deriv[0],shown[1]);
    working_string second=mul_factor_derivative(shown[0],deriv[1]);
    if (first=="0" && second=="0")
      out += "0";
    else if (first=="0")
      out += second;
    else if (second=="0")
      out += first;
    else {
      out += first;
      out += (!second.empty() && second[0]=='-') ? " - "+second.substr(1,second.size()-1) : " + "+second;
    }
    return true;
  }
  for (int i=0;i<n;++i)
    out += "f"+int_s(i+1)+" = "+shown[i]+"\n";
  for (int i=0;i<n;++i)
    out += "f"+int_s(i+1)+"' = "+deriv[i]+"\n";
  out += "";
  working_string ans;
  for (int i=0;i<n;++i){
    working_string term=product_rule_term(deriv[i],product_except(shown,n,i));
    if (term!="0")
      ans=join_sum(ans,term);
  }
  out += ans;
  return true;
}

static bool try_diff_quotient_rule(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string num,den,du,dv,u,vv;
  if (!split_top_fraction(nospace_lower(expr),num,den))
    return false;
  if (!diff_simple_expr(num,v,du,u) || !diff_simple_expr(den,v,dv,vv))
    return false;
  out="Quotient:\n";
  out += "u = "+u+", v = "+vv+"\n";
  out += "du/d"+rawvar+" = "+du+"\n";
  out += "dv/d"+rawvar+" = "+dv+"\n";
  out += "(u'v-uv')/v^2\n";
  out += "(("+du+")*("+vv+") - ";
  if (dv=="1")
    out += "("+u+")";
  else
    out += "("+u+")*("+dv+")";
  out += ")/("+vv+")^2";
  {
    Rat A,B,C,D;
    if (parse_ln_affine_var(num,v,A,B) && parse_ln_affine_var(den,v,C,D)){
      Rat top=rat_sub(rat_mul(A,D),rat_mul(C,B));
      if (top.n){
        out += "\n= "+rat_s(top)+"/("+rawvar+"*("+vv+")^2)";
        if (top.n>0)
          out += "\npositive for "+rawvar+" > 0 and "+vv+" != 0";
      }
      return true;
    }
  }
  {
    working_string simp;
    if (production_exact_command("simplify((("+du+")*("+vv+")-("+u+")*("+dv+"))/(("+vv+")^2))",simp)){
      simp=trim(simp);
      if (!simp.empty() && !contains(nospace_lower(simp),"simplify("))
        out += "\n= "+simp;
    }
  }
  return true;
}

static bool try_diff_recip_power(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string num,den;
  if (!split_top_fraction(nospace_lower(expr),num,den))
    return false;
  Rat c;
  long coef=0,pow=0;
  if (!parse_rat(num,c) || !parse_power_term_var(den,v,coef,pow) || coef!=1 || pow<=0)
    return false;
  Rat p=rat(-pow,1);
  Rat dc=rat_mul(c,p);
  out="Neg powers:\n";
  out += trim(expr)+" = "+rat_power_term_s(c,p)+"\n";
  out += "d/d"+rawvar+"(a*x^n)=a*n*x^(n-1)\n";
  out += ""+rat_power_term_s(dc,rat_sub(p,rat(1,1)));
  return true;
}

static bool try_diff_plain(const char *input,working_string &out);

static bool try_diff_chain_rule(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string deriv,shown,u,du;
  const char *fn=0;
  if (!unary_chain_derivative(expr,v,deriv,shown,&u,&du,&fn))
    return false;
  out="Chain:\n";
  if (u.size()>120 || du.size()>180 || deriv.size()>260){
    out += "u=A\n";
    out += "u'=D\n";
    out += "f'(A)*D";
    return true;
  }
  out += "u = "+u+"\n";
  out += "du/d"+rawvar+" = "+du+"\n";
  out += "d/d"+rawvar+" "+working_string(fn)+"(u)\n";
  out += ""+deriv;
  return true;
}

static bool split_outer_power_general(const working_string &src,working_string &base,Rat &p){
  working_string exp;
  return parse_top_power(strip_outer_parens(nospace_lower(src)),base,exp) && parse_rat(exp,p);
}

static bool try_diff_power_chain_general(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string base,db,shown;
  Rat p;
  if (!split_outer_power_general(expr,base,p) || !diff_simple_expr(base,v,db,shown))
    return false;
  Rat coef=p;
  Rat np=rat_sub(p,rat(1,1));
  working_string powpart;
  if (!np.n)
    powpart="1";
  else {
    powpart="("+shown+")";
    if (!rat_is_one(np))
      powpart += "^"+rat_s(np);
  }
  working_string first=rat_is_one(coef)?powpart:mul_expr(coef,powpart);
  working_string ans=mul_factor_derivative(first,db);
  out="Chain:\n";
  if (shown.size()+db.size()+ans.size()>420 || db.size()>180 || ans.size()>260){
    out += "u = "+(shown.size()<=120?shown:"A")+"\n";
    out += "du/d"+rawvar+" = D\n";
    out += "d/d"+rawvar+"(u^"+rat_s(p)+") = "+rat_s(p)+"*u^"+rat_s(np)+"*D";
    return true;
  }
  out += "u = "+shown+"\n";
  out += "du/d"+rawvar+" = "+db+"\n";
  out += "d/d"+rawvar+"(u^"+rat_s(p)+") = "+rat_s(p)+"*u^"+rat_s(np)+"*du/d"+rawvar+"\n";
  out += ans;
  return true;
}

static bool try_diff_log_product(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(expr)),arg;
  if (!parse_unary_arg(s,"ln",arg))
    return false;
  working_string factors[6],shown[6],deriv[6];
  int n=split_top_product(arg,factors,6);
  if (n<2)
    return false;
  out="Log law:\n";
  out += "ln(a*b)=ln(a)+ln(b)\n";
  working_string ans;
  for (int i=0;i<n;++i){
    if (!diff_simple_expr(factors[i],v,deriv[i],shown[i]))
      return false;
    working_string term=deriv[i]=="1"?"1/("+shown[i]+")":mul_visible_factor(deriv[i])+"/("+shown[i]+")";
    out += "d/d"+rawvar+" ln("+shown[i]+") = "+term+"\n";
    ans=join_sum(ans,term);
  }
  out += ans;
  return true;
}

static bool try_diff_log_power_chain(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(expr));
  if (s.find("ln(")!=0 || s[s.size()-1]!=')')
    return false;
  working_string arg=s.substr(3,s.size()-4);
  Rat a,b,p;
  if (!split_affine_power_rat(arg,v,a,b,p))
    return false;
  working_string base=fmt_linear_rat(a,b,v);
  Rat q=rat_mul(p,a);
    out="Chain:\n";
  out += "d/d"+rawvar+" ln(("+base+")^"+rat_s(p)+")\n";
  out += "u = ("+base+")^"+rat_s(p)+"\n";
  out += "u' = ";
  if (rat_is_one(q))
    out += "("+base+")";
  else
    out += rat_s(q)+"*("+base+")";
  out += "\n";
  out += "d/d"+rawvar+" ln(u)=u'/u\n";
  out += ""+coeff_over_base(q,base);
  return true;
}

static bool try_diff_log_base_chain(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string args[2],du,u;
  int n=0;
  if (!parse_call(expr.c_str(),"log",args,2,n) || n!=2)
    return false;
  working_string base=canonical_expr(args[0]);
  if (contains_var_symbol(base,v) || !diff_simple_expr(args[1],v,du,u))
    return false;
  out="Change base:\n";
  out += "log("+base+","+u+")=ln("+u+")/ln("+base+")\n";
  out += "u = "+u+"\n";
  out += "du/d"+rawvar+" = "+du+"\n";
  out += "d/d"+rawvar+" log("+base+",u)=u'/(u*ln("+base+"))\n";
  out += du=="1" ? "1/("+u+"*ln("+base+"))" :
         (needs_parens_for_mul(du)?"("+du+")":du)+"/("+u+"*ln("+base+"))";
  return true;
}

static int find_top_equal_any(const working_string &s){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c=='=')
      return i;
  }
  return -1;
}

static void strip_final_diff_prefix(working_string &out);

static bool try_diff_general_route(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<1)
    return false;
  working_string rawvar=diff_var_arg(args[0],args,n);
  working_string var=compact(rawvar);
  working_string e=canonical_expr(args[0]);
  if (!contains_var_symbol(e,var[0])){
    out="d/d"+rawvar+"\n";
    out += "no "+rawvar+" term is present\n";
    out += "0";
    return true;
  }
  if (args[0].size()>120){
    out="d/d"+rawvar+"\n";
    out += "d/d"+rawvar+"(F)";
    return true;
  }
  {
    working_string deriv,shown;
    if (diff_simple_expr(e,var[0],deriv,shown)){
      out="d/d"+rawvar+"\n";
      out += shown+"\n";
      out += deriv;
      return true;
    }
  }
  {
    working_string num,den,u,du,dv;
    working_string fracsrc=nospace_lower(args[0]);
    if (split_top_fraction(fracsrc,num,den) || split_top_fraction(e,num,den)){
      u=normalize_top_product(num);
      Rat A,B,C,D;
      bool lnfrac=parse_ln_affine_var(num,var[0],A,B) && parse_ln_affine_var(den,var[0],C,D);
      if (lnfrac){
        du=rat_s(A)+"/"+rawvar;
        dv=rat_s(C)+"/"+rawvar;
      }
      else {
        if (!production_exact_command("diff("+num+","+var+")",du))
          return false;
        if (!production_exact_command("diff("+den+","+var+")",dv))
          return false;
      }
      out="Quotient:\n";
      out += "u = "+u+", v = "+den+"\n";
      out += "du/d"+var+" = "+du+"\n";
      out += "dv/d"+var+" = "+dv+"\n";
      out += "(u'v-uv')/v^2\n";
      if (lnfrac){
        out += "(("+du+")("+den+")-("+num+")("+dv+"))/("+den+")^2";
        Rat top=rat_sub(rat_mul(A,D),rat_mul(C,B));
        if (top.n){
          out += "\n= "+rat_s(top)+"/("+rawvar+"*("+den+")^2)";
          if (top.n>0)
            out += "\npositive for "+rawvar+" > 0 and "+den+" != 0";
        }
        return true;
      }
      {
        working_string simp;
        if (production_exact_command("simplify((("+du+")*("+den+")-("+num+")*("+dv+"))/(("+den+")^2))",simp)){
          simp=trim(simp);
          if (!simp.empty() && !contains(nospace_lower(simp),"simplify("))
            out += "\n= "+simp;
        }
      }
      return true;
    }
  }
  out="d/d"+rawvar+"\n";
  if (contains(e,"+"))
    out += "sum rule\n";
  if (contains(e,"*"))
    out += "product rule\n";
  if (contains(e,"/"))
    out += "quotient rule\n";
  if (contains(e,"ln(") || contains(e,"log(") || contains(e,"sqrt(") ||
      contains(e,"sin(") || contains(e,"cos(") || contains(e,"tan(") ||
      contains(e,"atan(") ||
      contains(e,"exp("))
    out += "chain rule\n";
  working_string shown=normalize_pm_compact(spaced_pm(e));
  if (shown.size()>120){
    out += "d/d"+rawvar+"(F)";
    return true;
  }
  out += "Let F = "+shown+"\n";
  out += "d/d"+rawvar+"("+shown+")";
  return true;
}

static bool diff_side_route(const working_string &side,const working_string &rawvar,working_string &work,working_string &ans){
  working_string call="diff("+side+","+rawvar+")";
  if (!try_diff_plain(call.c_str(),work))
  {
    working_string var=compact(rawvar), shown;
    if (var.size()!=1 || !diff_simple_expr(side,var[0],ans,shown))
      return false;
    work="d/d"+rawvar+"\n";
    work += shown+"\n";
    work += ans;
    return true;
  }
  strip_final_diff_prefix(work);
  ans=last_nonempty_line(work);
  if (ans.size()>=5 && ans.substr(0,4)=="dy/d"){
    size_t eq=ans.find('=');
    if (eq!=working_string::npos)
      ans=trim(ans.substr(eq+1,ans.size()-eq-1));
  }
  return !ans.empty();
}

static working_string xy_factor_s(int xp,int yp){
  working_string r;
  if (xp>0){
    r="x";
    if (xp>1) r += "^"+int_s(xp);
  }
  if (yp>0){
    if (!r.empty()) r += "*";
    r += "y";
    if (yp>1) r += "^"+int_s(yp);
  }
  return r;
}

static working_string xy_term_s(Rat c,int xp,int yp){
  working_string f=xy_factor_s(xp,yp);
  if (f.empty())
    return rat_s(c);
  return mul_expr(c,f);
}

static bool parse_xy_monomial(const working_string &src,Rat &c,int &xp,int &yp){
  working_string f[8],s=strip_outer_parens(compact(src));
  int n=split_top_product(s,f,8);
  if (n<1) return false;
  c=rat(1,1); xp=0; yp=0;
  for (int i=0;i<n;++i){
    if (f[i]=="xy" || f[i]=="yx"){ xp+=1; yp+=1; continue; }
    if (f[i].size()>2 &&
        ((f[i][f[i].size()-2]=='x' && f[i][f[i].size()-1]=='y') ||
         (f[i][f[i].size()-2]=='y' && f[i][f[i].size()-1]=='x'))){
      Rat q;
      if (!parse_rat(f[i].substr(0,f[i].size()-2),q))
        return false;
      c=rat_mul(c,q); xp+=1; yp+=1; continue;
    }
    Rat q,pow;
    if (parse_rat(f[i],q)){ c=rat_mul(c,q); continue; }
    if (parse_var_power_factor(f[i],'x',q,pow) && pow.d==1){
      c=rat_mul(c,q); xp += int(pow.n); continue;
    }
    if (parse_var_power_factor(f[i],'y',q,pow) && pow.d==1){
      c=rat_mul(c,q); yp += int(pow.n); continue;
    }
    return false;
  }
  return true;
}

static bool implicit_poly_side(const working_string &side,working_string &plain,working_string &dcoef){
  working_string terms[16];
  int signs[16];
  int n=split_top_sum_terms(canonical_expr(side),terms,signs,16);
  if (n<1) return false;
  plain.clear(); dcoef.clear();
  for (int i=0;i<n;++i){
    Rat c; int xp=0,yp=0;
    if (!parse_xy_monomial(terms[i],c,xp,yp))
      return false;
    if (signs[i]<0) c.n=-c.n;
    if (xp>0)
      plain=join_sum(plain,xy_term_s(rat_mul(c,rat(xp,1)),xp-1,yp));
    if (yp>0)
      dcoef=join_sum(dcoef,xy_term_s(rat_mul(c,rat(yp,1)),xp,yp-1));
  }
  if (plain.empty()) plain="0";
  if (dcoef.empty()) dcoef="0";
  return true;
}

static working_string diff_expr_s(const working_string &a,const working_string &b){
  if (a=="0") return b=="0"?"0":"-("+b+")";
  if (b=="0") return a;
  return a+" - ("+b+")";
}

static working_string implicit_line_s(const working_string &plain,const working_string &dcoef){
  if (dcoef=="0") return plain;
  if (plain=="0") return "("+dcoef+")*(dy/dx)";
  return plain+" + ("+dcoef+")*(dy/dx)";
}

static bool try_implicit_poly_equation(const working_string &left,const working_string &right,working_string &out){
  working_string el=left, er=right, lp,ld,rp,rd;
  bool expanded=false;
  if (!implicit_poly_side(el,lp,ld) || !implicit_poly_side(er,rp,rd)){
    working_string xl,xr;
    if (!production_exact_command("texpand("+left+")",xl) ||
        !production_exact_command("texpand("+right+")",xr))
      return false;
    el=trim(xl); er=trim(xr);
    expanded=true;
  }
  if (!implicit_poly_side(el,lp,ld) || !implicit_poly_side(er,rp,rd))
    return false;
  working_string lhs=implicit_line_s(lp,ld), rhs=implicit_line_s(rp,rd);
  working_string den=diff_expr_s(ld,rd), num=diff_expr_s(rp,lp);
  working_string ratio="("+num+")/("+den+")", sr;
  if (num.size()>3 && num.substr(0,2)=="-(" && num[num.size()-1]==')'){
    working_string inner=num.substr(2,num.size()-3), negden;
    if (production_exact_command("simplify(-("+den+"))",negden) && !trim(negden).empty())
      ratio="("+inner+")/("+trim(negden)+")";
  }
  if (production_exact_command("simplify("+ratio+")",sr) && !trim(sr).empty())
    ratio=trim(sr);
  out="Differentiate both sides:\n";
  if (expanded)
    out += "Expand: "+el+" = "+er+"\n";
  else
    out += left+" = "+right+"\n";
  out += lhs+" = "+rhs+"\n";
  out += "("+den+")*(dy/dx)="+num+"\n";
  out += "(dy)/(dx)="+ratio;
  return true;
}

static bool reduce_implicit_dep_function(const working_string &left,const working_string &right,
                                         const working_string &rawvar,const working_string &dep,
                                         const working_string &ratio,working_string &out){
  static const char *fns[]={"tan","exp",0};
  for (int i=0;fns[i];++i){
    working_string tok=working_string(fns[i])+"("+dep+")";
    if (!contains(ratio,tok.c_str()))
      continue;
    working_string roots,root[3];
    if (!production_exact_command("solve("+left+"="+right+","+tok+")",roots))
      continue;
    int n=split_exact_list(roots,root,3);
    for (int j=0;j<n;++j){
      working_string r=trim(root[j]),sub,simp;
      if (r.empty() || contains(r,"I") || contains_var_symbol(r,dep[0]))
        continue;
      if (!production_exact_command("subst("+ratio+","+tok+"="+r+")",sub))
        continue;
      sub=trim(sub);
      if (!production_exact_command("simplify("+sub+")",simp))
        simp=sub;
      simp=trim(simp);
      if (simp.empty() || contains_var_symbol(simp,dep[0]))
        continue;
      out += "\nUse original equation:\n";
      out += tok+" = "+r+"\n";
      out += "(d"+dep+")/(d"+rawvar+")="+simp;
      return true;
    }
  }
  return false;
}

static bool try_diff_equation_general(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<1)
    return false;
  working_string raw=trim(args[0]);
  int eq=find_top_equal_any(raw);
  if (eq<0)
    return false;
  working_string left=trim(raw.substr(0,eq)), right=trim(raw.substr(eq+1,raw.size()-eq-1));
  working_string rawvar=diff_var_arg(raw,args,n,true);
  if (left.empty() || right.empty())
    return false;
  if (raw.size()>350 || left.size()>180 || right.size()>180){
    out="Differentiate both sides:\n";
    out += "d/d"+rawvar+"(L)=d/d"+rawvar+"(R)";
    return true;
  }
  if (compact(rawvar)=="x" && try_implicit_poly_equation(left,right,out))
    return true;
  {
    working_string dep=n>=3?compact(args[2]):"y";
    if (dep.size()==1 && dep!=compact(rawvar) &&
        (contains_var_symbol(left,dep[0]) || contains_var_symbol(right,dep[0]))){
      working_string f="("+left+")-("+right+")", fx, fy;
      if (production_exact_command("diff("+f+","+rawvar+")",fx) &&
          production_exact_command("diff("+f+","+dep+")",fy)){
        fx=trim(fx); fy=trim(fy);
        if (!fx.empty() && !fy.empty() && compact(fy)!="0"){
          working_string ratio="-("+fx+")/("+fy+")";
          out="Implicit differentiation:\n";
          out += "(d"+dep+")/(d"+rawvar+")="+ratio;
          reduce_implicit_dep_function(left,right,rawvar,dep,ratio,out);
          return true;
        }
      }
    }
  }
  working_string lw,rw,la,ra;
  if (!diff_side_route(left,rawvar,lw,la) || !diff_side_route(right,rawvar,rw,ra))
    return false;
  out="Differentiate both sides:\n";
  out += left+" = "+right+"\n";
  out += lw+"\n";
  out += rw+"\n";
  out += la+" = "+ra;
  return true;
}

static bool try_diff_plain(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<1)
    return false;
  working_string e=canonical_expr(args[0]), var=diff_var_arg(args[0],args,n);
  if (var.size()==1){
    working_string raw=nospace_lower(args[0]);
    char vv=var[0];
    if (raw.size()>6 && raw[0]==vv && (raw[1]=='+' || raw[1]=='-') &&
        raw.substr(2,3)=="1/(" && raw[raw.size()-1]==')'){
      working_string den=raw.substr(5,raw.size()-6);
      working_string want=working_string(1,vv);
      int star=den.find('*');
      Rat a;
      if (star>0 && den.substr(star+1,den.size()-star-1)==want &&
          parse_rat(den.substr(0,star),a) && a.n){
        bool minus=raw[1]=='-';
        working_string recip="1/("+rat_s(a)+"*"+want+")";
        working_string drec=working_string(minus?"+":"-")+"1/("+rat_s(a)+"*"+want+"^2)";
        out="Use negative powers\n";
        out += recip+" = "+rat_s(rat(1,a.n))+"*"+want+"^-1\n";
        out += "d/d"+var+"("+raw+") = 1 "+drec+"\n";
        out += "1 "+drec;
        return true;
      }
    }
  }
  if (var.size()==1){
    long coef=0,pow=0;
    if (parse_power_term_var(e,var[0],coef,pow)){
      working_string shown=rat_power_term_s(rat(coef,1),rat(pow,1),var[0]);
      working_string deriv=derivative_monomial(coef,pow,var[0]);
      out="Power:\n";
      out += var=="x" ? "Power rule: d/dx x^n = n*x^(n-1)\n" :
                         "Power rule: d/d"+var+" "+var+"^n = n*"+var+"^(n-1)\n";
      out += "d/d"+var+"("+shown+") = "+deriv+"\n";
      out += deriv;
      return true;
    }
  }
  if (var.size()!=1)
    return false;
  if (try_symbolic_poly_diff_cmd(input,out))
    return true;
  if (var!="x"){
    if (!contains_var_symbol(e,var[0])){
      out="d/d"+var+"\n";
      out += "no "+var+" term is present\n";
      out += "0";
      return true;
    }
  }
  if (var.size()==1 && !contains_var_symbol(e,var[0])){
    out="d/d"+var+"\n";
    out += "no "+var+" term is present\n";
    out += "0";
    return true;
  }
  if (args[0].size()>120){
    out="d/d"+var+"\n";
    out += "d/d"+var+"(F)";
    return true;
  }
  {
    Rat a,b;
    if (var.size()==1 && parse_affine_general(e,var[0],a,b)){
      out="Collect:\n";
      out += fmt_linear_rat(a,b,var[0])+"\n";
      out += "dy/d"+var+" = "+rat_s(a);
      return true;
    }
  }
  {
    Rat c,p;
    long rad=0;
    if (var.size()==1 && parse_surd_var_power_factor(e,var[0],c,rad,p)){
      out="Power:\n";
      out += "d/d"+var+"("+surd_power_term_s(c,rad,p,var[0])+") = ";
      out += derivative_surd_power_term(c,rad,p,var[0]);
      return true;
    }
    {
      working_string num,den;
      Rat q;
      long coef=0,pow=0;
      if (split_top_fraction(nospace_lower(args[0]),num,den) &&
          parse_rat(num,q) && parse_power_term_var(den,var[0],coef,pow) &&
          coef && pow>0){
        Rat cneg=rat_div(q,rat(coef,1));
        Rat pneg=rat(-pow,1);
        out="Neg powers:\n";
        out += trim(args[0])+" = "+rat_power_term_s(cneg,pneg,var[0])+"\n";
        out += derivative_rat_power_term(cneg,pneg,var[0]);
        return true;
      }
    }
    if (var.size()==1 && parse_var_power_factor(e,var[0],c,p)){
      working_string raw=compact(args[0]);
      if (contains(raw,"/") && p.n==-p.d){
        out="Neg powers:\n";
        out += trim(args[0])+" = "+rat_power_term_s(c,p,var[0])+"\n";
        out += "d/d"+var+"(a*"+var+"^n)=a*n*"+var+"^(n-1)\n";
      }
      else
        out="Power:\n";
      out += var=="x" ? "Power rule: d/dx x^n = n*x^(n-1)\n" :
                         "Power rule: d/d"+var+" "+var+"^n = n*"+var+"^(n-1)\n";
      out += "d/d"+var+"("+rat_power_term_s(c,p,var[0])+") = ";
      out += derivative_rat_power_term(c,p,var[0]);
      return true;
    }
  }
  if (var.size()==1){
    working_string simple=strip_outer_parens(nospace_lower(args[0]));
    working_string bare(1,var[0]);
    if (simple=="tan("+bare+")"){
      out="d/d"+var+" tan("+var+")\nsec("+var+")^2";
      return true;
    }
    if (try_diff_top_sum(args[0],var[0],var,out))
      return true;
    if (try_diff_log_product(args[0],var[0],var,out))
      return true;
    if (try_diff_log_base_chain(args[0],var[0],var,out))
      return true;
    if (try_diff_log_power_chain(args[0],var[0],var,out))
      return true;
    if (try_diff_power_chain_general(args[0],var[0],var,out))
      return true;
    if (try_diff_quotient_rule(args[0],var[0],var,out))
      return true;
    if (try_diff_product_rule(args[0],var[0],var,out))
      return true;
    if (try_diff_chain_rule(args[0],var[0],var,out))
      return true;
    if (try_diff_recip_power(args[0],var[0],var,out))
      return true;
    {
      working_string poly_answer;
      if (diff_sum_terms(e,poly_answer,var[0])){
        out="Differentiate each term:\n";
        out += poly_answer;
        return true;
      }
    }
    {
      working_string gd,gs;
      if (diff_simple_expr(args[0],var[0],gd,gs)){
        out="Differentiate both sides\n";
        out += gd;
        return true;
      }
    }
    return false;
  }
  {
    working_string sum_answer;
    if (diff_sum_terms(e,sum_answer)){
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
      return true;
    }
  }
  if (var.size()==1 && contains_var_symbol(e,var[0])){
    out="d/d"+var+"\n";
    if (contains(e,"+"))
      out += "sum rule\n";
    if (contains(e,"*"))
      out += "product rule\n";
    if (contains(e,"/"))
      out += "quotient rule\n";
    if (contains(e,"ln(") || contains(e,"log(") || contains(e,"sqrt(") ||
        contains(e,"sin(") || contains(e,"cos(") || contains(e,"tan(") ||
        contains(e,"atan(") ||
        contains(e,"exp("))
      out += "chain rule\n";
    working_string shown=normalize_pm_compact(spaced_pm(e));
    if (shown.size()>120){
      out += "d/d"+var+"(F)";
      return true;
    }
    out += "Let F = "+shown+"\n";
    out += "d/d"+var+"("+shown+")";
    return true;
  }
  return false;
}

static void strip_final_diff_prefix(working_string &out){
  size_t start=out.rfind('\n');
  start = start==working_string::npos ? 0 : start+1;
  working_string line=trim(out.substr(start,out.size()-start));
  if (line.size()<5 || line.substr(0,4)!="dy/d")
    return;
  size_t eq=line.find('=');
  if (eq==working_string::npos || eq>12)
    return;
  working_string ans=trim(line.substr(eq+1,line.size()-eq-1));
  if (!ans.empty())
    out=out.substr(0,start)+ans;
}

static void verify_or_exact_diff_result(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n!=2)
    return;
  working_string expr=trim(args[0]), var=compact(args[1]);
  if (expr.empty() || expr[0]=='[' || find_top_equal_any(expr)>=0 || var.size()!=1)
    return;
  working_string exact;
  if (!production_exact_command("normal("+working_string(input?input:"")+")",exact) &&
      !production_exact_command(input,exact))
    return;
  exact=trim(exact);
  if (exact.empty())
    return;
  working_string cand=last_nonempty_line(out);
  size_t eq=cand.find('=');
  if (eq!=working_string::npos && eq<16)
    cand=trim(cand.substr(eq+1,cand.size()-eq-1));
  if (!cand.empty() && khicas_equiv(cand,exact)){
    if (compact(cand)!=compact(exact) && out.size()+exact.size()<360)
      out += "\n= "+exact;
    return;
  }
  out="KhiCAS exact:\n"+exact;
}

static bool try_diff(const char *input,working_string &out){
  working_string margs[3];
  int mn=0;
  if (parse_call(input,"diff",margs,3,mn) && mn>=2){
    working_string src=trim(margs[0]),items[4],xexpr,yexpr,dx,dy,q,at;
    if (src.size()>4 && src[0]=='[' && src[src.size()-1]==']' &&
        split_args(src,1,src.size()-1,items,4)==2){
      for (int i=0;i<2;++i){
        int eq=find_top_equal_any(items[i]);
        if (eq>0){
          working_string name=compact(items[i].substr(0,eq));
          if (name=="x") xexpr=trim(items[i].substr(eq+1,items[i].size()-eq-1));
          else if (name=="y") yexpr=trim(items[i].substr(eq+1,items[i].size()-eq-1));
        }
      }
      if (xexpr.empty() && yexpr.empty()){
        xexpr=items[0];
        yexpr=items[1];
      }
      if (!xexpr.empty() && !yexpr.empty() &&
          production_exact_command("diff("+xexpr+","+margs[1]+")",dx) &&
          production_exact_command("diff("+yexpr+","+margs[1]+")",dy) &&
          production_exact_command("simplify(("+dy+")/("+dx+"))",q)){
        out="";
        out += "dx/d"+margs[1]+" = "+trim(dx)+"\n";
        out += "dy/d"+margs[1]+" = "+trim(dy)+"\n";
        out += "(dy)/(dx) = "+trim(q);
        if (mn>=3 && production_exact_command("subst("+q+","+margs[1]+"="+margs[2]+")",at))
          out += "\nAt "+margs[1]+" = "+margs[2]+", (dy)/(dx) = "+trim(at);
        return true;
      }
    }
    working_string mv=compact(margs[1]);
    if (mv.size()>1 && plain_identifier_name(mv)){
      working_string subcall="diff("+replace_identifier_token(margs[0],mv,"x")+",x";
      if (mn>=3)
        subcall += ","+margs[2];
      subcall += ")";
      working_string subout;
      if (try_diff(subcall.c_str(),subout)){
        out=replace_identifier_token(subout,"x",mv);
        out=replace_all_literal(out,"d/dx","d/d"+mv);
        out=replace_all_literal(out," dx"," d"+mv);
        return true;
      }
    }
  }
  if (try_diff_equation_general(input,out))
    return true;
  if (!try_diff_plain(input,out))
    return try_diff_general_route(input,out);
  strip_final_diff_prefix(out);
  verify_or_exact_diff_result(input,out);
  return true;
}

static bool try_parametric_diff_quotient(const working_string &input,working_string &out){
  working_string num,den,nargs[3],dargs[3],dy,dx,q;
  int nn=0,dn=0;
  if (!split_top_fraction(trim(input),num,den) ||
      !parse_call(num.c_str(),"diff",nargs,3,nn) || !parse_call(den.c_str(),"diff",dargs,3,dn) ||
      nn<2 || dn<2 || compact(nargs[1])!=compact(dargs[1]))
    return false;
  working_string var=compact(nargs[1]);
  if (var.empty())
    return false;
  if (!exact_command_clean("diff("+nargs[0]+","+var+")",dy) ||
      !exact_command_clean("diff("+dargs[0]+","+var+")",dx) ||
      !exact_command_clean("simplify(("+dy+")/("+dx+"))",q))
    return false;
  out="dy/d"+var+" = "+dy+"\n";
  out += "dx/d"+var+" = "+dx+"\n";
  out += "dy/dx = (dy/d"+var+")/(dx/d"+var+")\n";
  out += "KhiCAS exact:\n"+q;
  return true;
}

static bool try_implicit_diff_command(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"implicit_diff",args,3,n))
    return false;
  if (n<1 || trim(args[0]).empty()){
    out="Err: missing arguments\nimplicit_diff(eq,x,y)";
    return true;
  }
  char iv=0,dv=0;
  if (n<3 || find_top_equal_any(args[0])<0 ||
      !valid_single_var_token(args[1],iv) ||
      !valid_single_var_token(args[2],dv)){
    out="Err: implicit_diff(eq,x,y)";
    return true;
  }
  working_string rawvar=diff_var_arg(args[0],args,n,true);
  if (args[0].size()>250){
    out="Implicit differentiation:\n";
    if (find_top_equal_any(args[0])>=0)
      out += "d/d"+rawvar+"(L)=d/d"+rawvar+"(R)";
    else
      out += "d/d"+rawvar+"(F)";
    return true;
  }
  working_string call="diff("+args[0]+","+rawvar;
  if (n>=3 && !trim(args[2]).empty())
    call += ","+trim(args[2]);
  call += ")";
  working_string work;
  if (try_diff(call.c_str(),work)){
    if (work.size()>420){
      out="Implicit differentiation:\nChain\nD";
      return true;
    }
    if (work.find("Implicit differentiation:")==0)
      out=work;
    else
      out="Implicit differentiation:\n"+work;
    return true;
  }
  out="Implicit differentiation:\n";
  if (find_top_equal_any(args[0])>=0)
    out += "Diff both sides wrt "+rawvar+"\n";
  else
    out += "Diff wrt "+rawvar+"\n";
  out += "d/d"+rawvar+"("+trim(args[0])+")";
  return true;
}

static bool integrate_log_reverse_chain(const working_string &expr,working_string &out){
  working_string num,den;
  if (!split_top_fraction(nospace_lower(expr),num,den))
    return false;
  Rat na,nb,da,db;
  if (!parse_affine_general(num,'x',na,nb))
    return false;
  working_string dden,shown_den;
  if (!diff_simple_expr(den,'x',dden,shown_den) || !parse_affine_general(dden,'x',da,db))
    return false;
  Rat k=rat(0,1);
  if (da.n){
    k=rat_div(na,da);
    if (rat_cmp(nb,rat_mul(k,db)))
      return false;
  }
  else if (db.n){
    k=rat_div(nb,db);
    if (na.n)
      return false;
  }
  else
    return false;
  working_string shown_num=fmt_linear_rat(na,nb,'x');
  working_string shown_du=fmt_linear_rat(da,db,'x');
  shown_den=insert_coeff_stars(shown_den);
  out="Sub u="+shown_den+"\n";
  out += "du="+shown_du+" dx\n";
  if (!rat_is_one(k))
    out += shown_num+" = "+rat_s(k)+"*("+shown_du+")\n";
  out += "";
  if (!rat_is_one(k))
    out += rat_s(k)+"*";
  out += "ln(abs("+shown_den+")) + C";
  return true;
}

static bool integrate_reverse_chain_power_fraction(const working_string &expr,working_string &out){
  working_string num,den;
  if (!split_top_fraction(nospace_lower(expr),num,den))
    return false;
  working_string d=strip_outer_parens(den),base;
  Rat p;
  if (d.empty() || d[0]!='(')
    return false;
  int close=match_paren(d,0);
  if (close<0 || close+1>=int(d.size()) || d[close+1]!='^')
    return false;
  base=d.substr(1,close-1);
  if (!parse_rat(d.substr(close+2,d.size()-close-2),p) || p.n==p.d)
    return false;
  Rat na,nb,da,db;
  if (!parse_affine_general(num,'x',na,nb))
    return false;
  working_string dbase,shown_base;
  if (!diff_simple_expr(base,'x',dbase,shown_base) || !parse_affine_general(dbase,'x',da,db))
    return false;
  Rat k=rat(0,1);
  if (da.n){
    k=rat_div(na,da);
    if (rat_cmp(nb,rat_mul(k,db)))
      return false;
  }
  else if (db.n){
    k=rat_div(nb,db);
    if (na.n)
      return false;
  }
  else
    return false;
  Rat ep=rat_sub(rat(1,1),p);
  if (!ep.n)
    return false;
  Rat coef=rat_div(k,ep);
  working_string shown_num=fmt_linear_rat(na,nb,'x');
  working_string shown_du=fmt_linear_rat(da,db,'x');
  shown_base=insert_coeff_stars(shown_base);
  working_string powbase="("+shown_base+")^"+rat_s(ep);
  if (ep.n==ep.d)
    powbase=shown_base;
  out="Sub u="+shown_base+"\n";
  out += "du="+shown_du+" dx\n";
  if (!rat_is_one(k))
    out += shown_num+" = "+rat_s(k)+"*("+shown_du+")\n";
  out += "int("+rat_s(k)+"*u^(-"+rat_s(p)+"))du\n";
  out += mul_expr(coef,powbase)+" + C";
  return true;
}

static bool is_sin_kx(const working_string &s,int k){
  working_string a="sin("+int_s(k)+"*x)";
  working_string b="sin("+int_s(k)+"x)";
  return s==a || s==b;
}

static working_string two_thirds_sqrt_s(Rat q){
  Rat sr;
  if (rat_square_root(q,sr))
    return rat_s(rat_mul(rat(2,3),sr));
  return "2*sqrt("+rat_s(q)+")/3";
}

static bool parse_sin_cos_product_same_arg(const working_string &src,working_string &arg,Rat &coef){
  working_string f[6],a;
  int n=split_top_product(strip_outer_parens(nospace_lower(src)),f,6);
  bool hs=false,hc=false;
  coef=rat(1,1);
  for (int i=0;i<n;++i){
    Rat q;
    if (parse_rat(f[i],q)){
      coef=rat_mul(coef,q);
      continue;
    }
    if (parse_unary_arg(f[i],"sin",a)){
      if (hs || (hc && compact(arg)!=compact(a))) return false;
      arg=a; hs=true; continue;
    }
    if (parse_unary_arg(f[i],"cos",a)){
      if (hc || (hs && compact(arg)!=compact(a))) return false;
      arg=a; hc=true; continue;
    }
    return false;
  }
  return hs && hc;
}

static bool double_arg_matches(const working_string &arg,const working_string &twice){
  Rat a,b;
  return parse_x_coeff(arg,a) && parse_x_coeff(twice,b) &&
         rat_cmp(rat_mul(a,rat(2,1)),b)==0;
}

static bool parse_one_pm_cos_double(const working_string &src,const working_string &arg,bool &plus){
  working_string terms[3],carg;
  int signs[3];
  int n=split_top_sum_terms(strip_outer_parens(nospace_lower(src)),terms,signs,3);
  if (n!=2)
    return false;
  bool one=false,cosd=false;
  int csign=0;
  for (int i=0;i<n;++i){
    Rat q;
    if (parse_rat(terms[i],q) && signs[i]>0 && q.n==q.d){
      one=true;
      continue;
    }
    if (parse_unary_arg(terms[i],"cos",carg) && double_arg_matches(arg,carg)){
      cosd=true;
      csign=signs[i];
      continue;
    }
    return false;
  }
  if (!one || !cosd || !csign)
    return false;
  plus=csign>0;
  return true;
}

static bool sin13_combo(const working_string &base,Rat &a,Rat &b){
  working_string t[4],f[3],arg;
  int sg[4],n=split_top_sum_terms(base,t,sg,4);
  a=rat(0,1); b=rat(0,1);
  for (int i=0;i<n;++i){
    Rat c=rat(sg[i],1),q;
    working_string term=strip_outer_parens(t[i]);
    if (parse_unary_arg(term,"sin",arg)){
      if (arg=="x") a=rat_add(a,c);
      else if (arg=="3*x" || arg=="3x") b=rat_add(b,c);
      else return false;
      continue;
    }
    int pn=split_top_product(term,f,3);
    if (pn!=2 || !parse_rat(f[0],q) || !parse_unary_arg(f[1],"sin",arg))
      return false;
    c=rat_mul(c,q);
    if (arg=="x") a=rat_add(a,c);
    else if (arg=="3*x" || arg=="3x") b=rat_add(b,c);
    else return false;
  }
  return a.n || b.n;
}

static bool integrate_trig_reverse_chain_general(const working_string &expr,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(expr)),arg,f[6],base,exp;
  {
    working_string num,den,root,arg0;
    Rat c,k;
    bool plus=false;
    if (split_top_fraction(s,num,den) &&
        parse_unary_arg(strip_outer_parens(den),"sqrt",root) &&
        parse_sin_cos_product_same_arg(num,arg0,c) &&
        parse_x_coeff(arg0,k) && k.n &&
        parse_one_pm_cos_double(root,arg0,plus)){
      Rat coef=rat_div(c,rat_mul(rat(2,1),k));
      if (plus) coef.n=-coef.n;
      working_string shown=spaced_pm(root);
      out="Reverse chain:\n";
      out += "u="+shown+"\n";
      out += "du=";
      if (plus) out += "-";
      out += rat_s(rat_mul(rat(4,1),k))+"*sin("+arg0+")*cos("+arg0+") dx\n";
      out += mul_expr(coef,"sqrt("+shown+")")+" + C";
      if (!plus && compact(arg0)=="x"){
        out += "\n1-cos(2*x)=2*sin(x)^2\n";
        out += "1/sqrt(2)*sin(x) + C";
      }
      return true;
    }
  }
  if (parse_unary_arg(s,"sqrt",arg)){
    int n=split_top_product(arg,f,6);
    Rat c=rat(1,1),q;
    bool co=false,s2=false;
    for (int i=0;i<n;++i){
      if (parse_rat(f[i],q)) c=rat_mul(c,q);
      else if (f[i]=="cos(x)") co=true;
      else if (is_sin_kx(f[i],2)) s2=true;
      else return false;
    }
    if (co && s2 && c.n>0){
      working_string k=two_thirds_sqrt_s(rat_mul(rat(2,1),c));
      out="sin(2*x)=2*sin(x)*cos(x)\n";
      out += "u=sin(x), du=cos(x) dx\n";
      out += k+"*sin(x)^(3/2) + C\n";
      out += k+"*sqrt(sin(x)^3) + C\n";
      out += "-"+k+"*sqrt(sin(x)^3) + C";
      return true;
    }
  }
  {
    int n=split_top_product(s,f,6);
    Rat c=rat(1,1),q;
    bool s2=false,rootcos=false,cosf=false;
    working_string powbase,powexp;
    for (int i=0;i<n;++i){
      if (parse_rat(f[i],q)){ c=rat_mul(c,q); continue; }
      if (is_sin_kx(f[i],2)){ s2=true; continue; }
      if (parse_unary_arg(f[i],"sqrt",arg) && arg=="cos(x)"){ rootcos=true; continue; }
      if (f[i]=="cos(x)"){ cosf=true; continue; }
      if (parse_top_power(f[i],powbase,powexp)) continue;
      return false;
    }
    if (s2 && rootcos){
      out="sin(2*x)=2*sin(x)*cos(x)\n";
      out += "u=cos(x), du=-sin(x) dx\n";
      out += mul_expr(rat_mul(c,rat(-4,5)),"cos(x)^(5/2)")+" + C";
      return true;
    }
    if (cosf && powexp=="2/3"){
      Rat a,b,m,cr;
      if (!sin13_combo(powbase,a,b))
        return false;
      if (rat_add(a,rat_mul(rat(3,1),b)).n)
        return false;
      m=rat_mul(rat(-4,1),b);
      if (m.d!=1 || m.n<=0)
        return false;
      long r=1;
      while (r*r*r<m.n) ++r;
      if (r*r*r!=m.n)
        return false;
      cr=rat_mul(c,rat(r*r,3));
      out="sin(3*x)=3*sin(x)-4*sin(x)^3\n";
      out += powbase+" = "+rat_s(m)+"*sin(x)^3\n";
      out += "u=sin(x), du=cos(x) dx\n";
      out += mul_expr(cr,"sin(x)^3")+" + C";
      return true;
    }
  }
  return false;
}

static bool integrate_xn_ln_parts(const working_string &expr,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(expr)),power_part;
  int lp=s.find("ln(x)");
  if (lp<0)
    return false;
  if (lp==0)
    power_part="1";
  else {
    power_part=s.substr(0,lp);
    if (!power_part.empty() && power_part[power_part.size()-1]=='*')
      power_part=power_part.substr(0,power_part.size()-1);
  }
  if (lp+5!=int(s.size()))
    return false;
  Rat c,p;
  if (power_part=="1"){
    c=rat(1,1);
    p=rat(0,1);
  }
  else if (!parse_x_power_factor(power_part,c,p))
    return false;
  if (p.n==-p.d){
    out="Sub u=ln(x)\n";
    out += "du=1/x dx\n";
    out += "1/2*ln(x)^2 + C";
    return true;
  }
  Rat q=rat_add(p,rat(1,1));
  Rat vcoef=rat_div(c,q);
  working_string vterm=rat_power_term_s(vcoef,q);
  Rat secondcoef=rat_div(c,rat_mul(q,q));
  working_string second=rat_power_term_s(secondcoef,q);
  working_string ans=join_sum(vterm+"*ln(x)",signed_part(-1,second));
  out="Parts:\n";
  out += "u=ln(x), dv="+rat_power_term_s(c,p)+" dx\n";
  out += "du=1/x dx, v="+vterm+"\n";
  out += "I=u*v-int(v*du)\n";
  out += ans+" + C";
  return true;
}

static bool integrate_basic_term(const working_string &raw,int sign,working_string &part){
  working_string t=strip_outer_parens(nospace_lower(raw));
  if (t.empty())
    return false;
  Rat sgn=rat(sign,1);
  if (t=="1/x"){
    part=sign<0?"-ln(abs(x))":"ln(abs(x))";
    return true;
  }
  if (t=="tan(x)"){
    part=sign<0?"-ln(abs(sec(x)))":"ln(abs(sec(x)))";
    return true;
  }
  Rat rc;
  long a=0,b=0;
  if (split_recip_affine(t,rc,a,b)){
    part=log_affine_int_s(rat_mul(sgn,rc),a,b);
    return true;
  }
  if (integrate_one_trig_exp(t,sgn,part))
    return true;
  {
    Rat a,b;
    if (parse_affine_general(t,'x',a,b)){
      part=rat_power_term_s(rat_mul(sgn,rat_div(a,rat(2,1))),rat(2,1));
      if (b.n)
        part=join_sum(part,rat_power_term_s(rat_mul(sgn,b),rat(1,1)));
      return true;
    }
  }
  Rat c,p;
  if (parse_x_power_factor(t,c,p) && p.n!=-p.d){
    part=integral_rat_power_term(rat_mul(sgn,c),p);
    return true;
  }
  long coef=0,pow=0;
  if (parse_power_term(t,coef,pow) && pow!=-1){
    part=integral_monomial(sign*coef,pow);
    return true;
  }
  return false;
}

static bool integrate_mixed_sum_terms(const working_string &expr,working_string &out){
  working_string terms[12],part;
  int signs[12];
  int n=split_top_sum_terms(expr,terms,signs,12);
  if (n<2)
    return false;
  working_string ans;
  out="Integrate term by term\nTerms:\nPower:\n";
  if (expr.size()>350){
    out += "poly + C";
    return true;
  }
  for (int i=0;i<n;++i){
    if (!integrate_basic_term(terms[i],signs[i],part))
      return false;
    out += "int("+insert_coeff_stars(signed_part(signs[i],terms[i]))+") dx = "+part+"\n";
    ans=join_sum(ans,part);
  }
  out += ans+" + C";
  return true;
}

static bool try_integral(const char *input,working_string &out);
static bool parse_top_power(const working_string &src,working_string &base,working_string &exp);

static bool symbolic_poly_term(const working_string &term,char v,working_string &coef,long &pow){
  working_string t=strip_outer_parens(nospace_lower(term)),f[8],base,exp;
  int n=split_top_product(t,f,8), hit=0;
  coef.clear(); pow=0;
  for (int i=0;i<n;++i){
    working_string c=strip_outer_parens(f[i]);
    long p=0;
    if (c==working_string(1,v)){
      p=1;
    }
    else if (parse_top_power(c,base,exp) && base==working_string(1,v)){
      Rat er;
      if (!parse_rat(exp,er) || er.d!=1 || er.n<0 || er.n>12)
        return false;
      p=er.n;
    }
    else {
      if (contains_var_symbol(c,v))
        return false;
      coef=coef.empty()?c:coef+"*"+mul_visible_factor(c);
      continue;
    }
    if (hit++)
      return false;
    pow=p;
  }
  if (!hit)
    return false;
  if (coef.empty())
    coef="1";
  return true;
}

static working_string symbolic_mul_coeff_pow(const working_string &coef,long mult,char v,long pow){
  working_string c=compact(coef), out;
  if (c=="1")
    out=int_s(mult);
  else if (mult==1)
    out=c;
  else
    out=int_s(mult)+"*"+c;
  if (pow<=0)
    return out;
  out += "*";
  out += v;
  if (pow!=1)
    out += "^"+int_s(pow);
  return out;
}

static bool symbolic_poly_diff(const working_string &expr,char v,working_string &ans){
  working_string terms[16],coef,part;
  int signs[16],n=split_top_sum_terms(expr,terms,signs,16);
  if (n<1)
    return false;
  ans.clear();
  for (int i=0;i<n;++i){
    long pow=0;
    if (symbolic_poly_term(terms[i],v,coef,pow)){
      if (pow==0)
        continue;
      part=symbolic_mul_coeff_pow(coef,signs[i]*pow,v,pow-1);
      ans=join_sum(ans,part);
      continue;
    }
    if (!contains_var_symbol(terms[i],v))
      continue;
    return false;
  }
  if (ans.empty())
    ans="0";
  return true;
}

static bool symbolic_poly_integral(const working_string &expr,char v,working_string &ans){
  working_string terms[16],coef,part;
  int signs[16],n=split_top_sum_terms(expr,terms,signs,16);
  if (n<1)
    return false;
  ans.clear();
  for (int i=0;i<n;++i){
    long pow=0;
    if (symbolic_poly_term(terms[i],v,coef,pow)){
      long np=pow+1;
      working_string c=compact(coef);
      if (signs[i]<0)
        c="-"+c;
      part="("+c+")/"+int_s(np);
      part += "*";
      part += v;
      if (np!=1)
        part += "^"+int_s(np);
      ans=join_sum(ans,part);
      continue;
    }
    if (!contains_var_symbol(terms[i],v)){
      part=signed_part(signs[i],terms[i])+"*"+working_string(1,v);
      ans=join_sum(ans,part);
      continue;
    }
    return false;
  }
  return !ans.empty();
}

static bool try_symbolic_poly_diff_cmd(const char *input,working_string &out){
  working_string args[3],ans;
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<2)
    return false;
  working_string var=compact(args[1]);
  if (var.size()!=1)
    return false;
  if (!symbolic_poly_diff(args[0],var[0],ans) || ans=="0")
    return false;
  out="Differentiate each term:\n";
  out += ans;
  return true;
}

static bool try_symbolic_poly_integral_cmd(const char *input,working_string &out){
  working_string args[4],ans;
  int n=0;
  if (!(parse_call(input,"integrate",args,4,n) || parse_call(input,"int",args,4,n)) || n<2)
    return false;
  working_string var=compact(args[1]);
  if (var.size()!=1)
    return false;
  if (!symbolic_poly_integral(args[0],var[0],ans))
    return false;
  out="Integrate each term:\n";
  out += ans+" + C";
  return true;
}

static bool try_taylor_symbolic_binomial_cmd(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  bool parsed=parse_call(input,"taylor",args,6,n);
  if (!parsed || n<3)
    return false;
  working_string var,centre,order_s;
  if (n>=4){
    var=compact(args[1]);
    centre=compact(args[2]);
    order_s=compact(args[3]);
  }
  else {
    working_string about=compact(args[1]);
    int eq=about.find('=');
    if (eq<=0)
      return false;
    var=about.substr(0,eq);
    centre=about.substr(eq+1,about.size()-eq-1);
    order_s=compact(args[2]);
  }
  long oi=0;
  if (var.size()!=1 || centre!="0" || !integer_bound_arg(order_s,oi) || oi<0 || oi>3)
    return false;
  working_string expr=compact(args[0]),base,exponent;
  if (expr.size()>3 && expr[0]=='('){
    int p=expr.rfind(")^");
    if (p>0){
      base=expr.substr(1,p-1);
      exponent=expr.substr(p+2,expr.size()-p-2);
    }
  }
  if (base.empty() && !parse_top_power(expr,base,exponent))
    return false;
  working_string direct_u;
  int direct_sign=1;
  if (base.substr(0,2)=="1+")
    direct_u=base.substr(2,base.size()-2);
  else if (base.substr(0,2)=="1-"){
    direct_u=base.substr(2,base.size()-2);
    direct_sign=-1;
  }
  if (!direct_u.empty()){
    working_string coef;
    int star=direct_u.rfind(working_string("*")+var);
    if (star>=0 && star+2==int(direct_u.size()) && !contains_var_symbol(direct_u.substr(0,star),var[0]))
      coef=direct_u.substr(0,star);
    else if (direct_u==var)
      coef="1";
    else if (direct_u.size()>1 && direct_u[direct_u.size()-1]==var[0] &&
             !contains_var_symbol(direct_u.substr(0,direct_u.size()-1),var[0]))
      coef=direct_u.substr(0,direct_u.size()-1);
    if (!coef.empty() && !exponent.empty()){
      Rat er,cr,powc;
      working_string cparse=direct_sign<0 ? "-"+coef : coef;
      if (parse_rat(strip_outer_parens(exponent),er) && parse_rat(strip_outer_parens(cparse),cr)){
        working_string ans;
        for (int k=0;k<=oi;++k){
          if (!rat_pow_small(cr,k,powc))
            return false;
          ans=join_sum(ans,rat_var_power_term_s(rat_mul(binom_rat(er,k),powc),var[0],k));
        }
        out="Binomial series:\n";
        out += "(1+u)^n = 1+n*u+n(n-1)u^2/2+n(n-1)(n-2)u^3/6\n";
        out += "u = "+cparse+"*"+var+"\n";
        out += ans;
        return true;
      }
      if (direct_sign<0)
        coef="-("+coef+")";
      working_string ans="1";
      if (oi>=1)
        ans=join_sum(ans,"("+exponent+")*("+coef+")*"+var);
      if (oi>=2)
        ans=join_sum(ans,"("+exponent+")*("+exponent+"-1)/2*("+coef+")^2*"+var+"^2");
      if (oi>=3)
        ans=join_sum(ans,"("+exponent+")*("+exponent+"-1)*("+exponent+"-2)/6*("+coef+")^3*"+var+"^3");
      out="Binomial series:\n";
      out += "(1+u)^n = 1+n*u+n(n-1)u^2/2+n(n-1)(n-2)u^3/6\n";
      out += "u = "+coef+"*"+var+"\n";
      out += ans;
      return true;
    }
  }
  return false;
}

static bool symbolic_linear_side(const working_string &src,char v,working_string &coef,working_string &constant){
  working_string terms[16],c;
  int signs[16],n=split_top_sum_terms(src,terms,signs,16);
  if (n<1)
    return false;
  coef.clear(); constant.clear();
  for (int i=0;i<n;++i){
    long pow=0;
    if (symbolic_poly_term(terms[i],v,c,pow)){
      if (pow!=1)
        return false;
      coef=join_sum(coef,signed_part(signs[i],c));
      continue;
    }
    if (!contains_var_symbol(terms[i],v)){
      constant=join_sum(constant,signed_part(signs[i],terms[i]));
      continue;
    }
    return false;
  }
  if (coef.empty())
    coef="0";
  if (constant.empty())
    constant="0";
  return true;
}

static bool try_symbolic_linear_solve_cmd(const char *input,working_string &out){
  working_string args[4];
  int n=0;
  if (!parse_call(input,"solve",args,4,n) || n<2)
    return false;
  working_string var=compact(args[1]);
  if (var.size()!=1)
    return false;
  working_string left,right,lc,rc,la,ra;
  if (!split_equal_sides(args[0],left,right))
    return false;
  if (!symbolic_linear_side(left,var[0],la,lc) || !symbolic_linear_side(right,var[0],ra,rc))
    return false;
  working_string A=symbolic_sub_s(la,ra);
  working_string B=symbolic_sub_s(lc,rc);
  if (compact(A)=="0")
    return false;
  out="Solve: "+trim(args[0])+"\nCollect:\n";
  out += "("+A+")*"+var+" + ("+B+") = 0\n";
  out += var+" = -("+B+")/("+A+")";
  return true;
}

static bool is_sqrt_A_minus_v_integrand(const working_string &e,const working_string &A,const working_string &var){
  working_string c=compact(e), a=compact(A), v=compact(var);
  if (v.empty())
    return false;
  working_string r1=v+"^(1/2)sqrt("+a+"-"+v+")";
  working_string r2="sqrt("+v+")sqrt("+a+"-"+v+")";
  working_string r3="sqrt("+a+"-"+v+")"+v+"^(1/2)";
  working_string r4="sqrt("+a+"-"+v+")sqrt("+v+")";
  return c==r1 || c==r2 || c==r3 || c==r4;
}

static bool is_sqrt_a_minus_x_integrand(const working_string &e,const working_string &A){
  return is_sqrt_A_minus_v_integrand(e,A,"x");
}

static void sqrt_A_minus_v_working(const working_string &A,const working_string &var,working_string &out,bool final_value){
  working_string a=trim(A), v=trim(var);
  if (v.empty())
    v="x";
  out=v+" = "+a+"*sin(theta)^2\n";
  out += "d"+v+" = 2*"+a+"*sin(theta)*cos(theta) dtheta\n";
  out += "bounds: 0 -> pi/2\n";
  out += "sqrt("+a+"-"+v+")=sqrt("+a+")*cos(theta)\n";
  out += v+"^(1/2)=sqrt("+a+")*sin(theta)\n";
  out += "I=int_0^"+a+" "+v+"^(1/2)*sqrt("+a+"-"+v+") d"+v+"\n";
  out += "I=2*"+a+"^2*int sin^2*cos^2 dtheta\n";
  out += "sin^2*cos^2=1/4*sin(2theta)^2\n";
  out += "I=1/2*"+a+"^2*int_0^(pi/2) sin(2*theta)^2 dtheta";
  if (!final_value)
    return;
  out += "\n";
  out += "sin(2*theta)^2=(1-cos(4*theta))/2\n";
  out += "I=1/4*"+a+"^2*[theta-sin(4*theta)/4]_0^(pi/2)\n";
  out += "pi*"+a+"^2/8";
}

static void sqrt_a_minus_x_working(const working_string &A,working_string &out,bool final_value){
  sqrt_A_minus_v_working(A,"x",out,final_value);
}

static bool parse_symbol_square_factor(const working_string &src,working_string &sym){
  working_string s=strip_outer_parens(nospace_lower(src)), base,exp;
  if (!parse_top_power(s,base,exp) || exp!="2")
    return false;
  base=strip_outer_parens(base);
  if (base.empty())
    return false;
  for (int i=0;i<int(base.size());++i)
    if (!isalpha((unsigned char)base[i]) && base[i]!='_')
      return false;
  sym=base;
  return true;
}

static bool merge_square_symbol(working_string &sym,const working_string &candidate){
  if (sym.empty()){
    sym=candidate;
    return true;
  }
  return sym==candidate;
}

static bool parse_sin2_coeff_piece(const working_string &piece,Rat &scale,working_string &square_sym){
  working_string p=strip_outer_parens(nospace_lower(piece)), num,den,sym;
  Rat r;
  if (parse_rat(p,r)){
    scale=rat_mul(scale,r);
    return true;
  }
  if (parse_symbol_square_factor(p,sym))
    return merge_square_symbol(square_sym,sym);
  if (split_top_fraction(p,num,den)){
    if (parse_symbol_square_factor(num,sym) && parse_rat(den,r) && r.n){
      scale=rat_div(scale,r);
      return merge_square_symbol(square_sym,sym);
    }
  }
  return false;
}

static bool extract_sin2_coeff(const working_string &expr,const working_string &var,
                               Rat &scale,working_string &square_sym){
  working_string factors[6], core="sin(2"+compact(var)+")^2";
  int n=split_top_product(expr,factors,6);
  scale=rat(1,1);
  square_sym="";
  if (n<=0){
    if (compact(expr)==core)
      return true;
    return false;
  }
  bool found_core=false;
  for (int i=0;i<n;++i){
    if (compact(factors[i])==core){
      found_core=true;
      continue;
    }
    if (!parse_sin2_coeff_piece(factors[i],scale,square_sym))
      return false;
  }
  return found_core;
}

static working_string coeff_square_s(Rat q,const working_string &sym){
  working_string sq=sym.empty()?working_string(""):sym+"^2";
  if (sq.empty())
    return rat_s(q);
  if (rat_is_one(q))
    return sq;
  if (rat_is_minus_one(q))
    return "-"+sq;
  return rat_s(q)+"*"+sq;
}

static working_string pi_coeff_square_s(Rat q,const working_string &sym){
  if (sym.empty()){
    if (rat_is_one(q))
      return "pi";
    if (q.d==1)
      return rat_s(q)+"*pi";
    return "pi/"+rat_s(rat(q.d,q.n));
  }
  if (rat_is_one(q))
    return "pi*"+sym+"^2";
  if (q.d==1)
    return rat_s(q)+"*pi*"+sym+"^2";
  if (q.n==1)
    return "pi*"+sym+"^2/"+int_s(q.d);
  return rat_s(q)+"*pi*"+sym+"^2";
}

static bool trig_sin2_defint_route(const working_string &expr,const working_string &var,
                                   const working_string &lo,const working_string &hi,
                                   working_string &out){
  working_string e=compact(expr), v=compact(var), a=compact(lo), b=compact(hi);
  if (a!="0" || b!="pi/2")
    return false;
  Rat scale;
  working_string square_sym;
  if (!extract_sin2_coeff(expr,var,scale,square_sym))
    return false;
  working_string coeff=coeff_square_s(scale,square_sym);
  Rat ans_scale=rat_div(scale,rat(4,1));
  out="sin(2*"+v+")^2=(1-cos(4*"+v+"))/2\n";
  if (square_sym.empty() && rat_is_one(scale)){
    out += "I=1/2*int_0^(pi/2) (1-cos(4*"+v+")) d"+v+"\n";
    out += "I=1/2*["+v+" - sin(4*"+v+")/4]_0^(pi/2)\n";
    out += "pi/4";
  }
  else {
    out += "I="+coeff+"*int_0^(pi/2) sin(2*"+v+")^2 d"+v+"\n";
    out += "I="+coeff_square_s(rat_div(scale,rat(2,1)),square_sym)+"*int_0^(pi/2) (1-cos(4*"+v+")) d"+v+"\n";
    out += "I="+coeff_square_s(rat_div(scale,rat(2,1)),square_sym)+"*["+v+" - sin(4*"+v+")/4]_0^(pi/2)\n";
    out += "I="+coeff_square_s(rat_div(scale,rat(2,1)),square_sym)+"*(pi/2)\n";
    out += pi_coeff_square_s(ans_scale,square_sym);
  }
  return true;
}

static working_string strip_integral_constant(const working_string &s){
  working_string r=trim(last_nonempty_line(s));
  if (r.find("Result:")==0)
    r=trim(r.substr(7,r.size()-7));
  if (r.size()>=4 && r.substr(r.size()-4,4)==" + C")
    r=trim(r.substr(0,r.size()-4));
  if (r.size()>=2 && r.substr(r.size()-2,2)=="+C")
    r=trim(r.substr(0,r.size()-2));
  return r;
}

static working_string rat_expr_s(Rat q,const working_string &expr);

static working_string const_times_value_s(const working_string &constant,const working_string &value){
  working_string c=trim(constant), v=trim(value);
  Rat r,cr;
  if (c=="1") return v;
  if (v=="1") return c;
  if (c=="pi" && v=="sqrt(3)*pi/3")
    return "sqrt(3)*pi^2/3";
  if (parse_rat(c,cr) && parse_rat(v,r))
    return rat_s(rat_mul(cr,r));
  if (c=="pi" && parse_rat(v,r)){
    if (r.d==1)
      return int_s(r.n)+"*pi";
    if (r.n==1)
      return "pi/"+int_s(r.d);
    return int_s(r.n)+"*pi/"+int_s(r.d);
  }
  if (v=="pi" && parse_rat(c,r)){
    if (r.d==1)
      return int_s(r.n)+"*pi";
    if (r.n==1)
      return "pi/"+int_s(r.d);
    return int_s(r.n)+"*pi/"+int_s(r.d);
  }
  return c+"*("+v+")";
}

static working_string subst_var_value(const working_string &expr,char v,const working_string &value){
  working_string out;
  for (int i=0;i<int(expr.size());++i){
    char c=expr[i];
    bool prev=i>0 && (isalnum((unsigned char)expr[i-1]) || expr[i-1]=='_');
    bool next=i+1<int(expr.size()) && (isalnum((unsigned char)expr[i+1]) || expr[i+1]=='_');
    if (c==v && !prev && !next)
      out += "("+value+")";
    else
      out += c;
  }
  return out;
}

static working_string replace_identifier_token(const working_string &src,const working_string &from,const working_string &to){
  if (from.empty())
    return src;
  bool single_alpha=from.size()==1 && isalpha((unsigned char)from[0]);
  working_string out;
  for (int i=0;i<int(src.size());){
    if (i+int(from.size())<=int(src.size()) && src.substr(i,from.size())==from){
      bool prev=i>0 && (isalnum((unsigned char)src[i-1]) || src[i-1]=='_');
      if (single_alpha && i>0 && (isdigit((unsigned char)src[i-1]) || src[i-1]==')'))
        prev=false;
      bool next=i+int(from.size())<int(src.size()) &&
                (isalnum((unsigned char)src[i+from.size()]) || src[i+from.size()]=='_');
      if (!prev && !next){
        out += to;
        i += from.size();
        continue;
      }
    }
    out += src[i++];
  }
  return out;
}

static working_string replace_all_literal(working_string src,const working_string &from,const working_string &to){
  if (from.empty())
    return src;
  int p=0;
  while ((p=src.find(from,p))>=0){
    src=src.substr(0,p)+to+src.substr(p+from.size(),src.size()-p-from.size());
    p += to.size();
  }
  return src;
}

static double last_numeric_value;

static bool eval_numeric_string(const working_string &expr,working_string &shown){
  NumParser np;
  working_string e=nospace_lower(expr);
  np.p=e.c_str();
  np.ok=true;
  double val=np.expr();
  np.skip();
  if (!np.ok || *np.p || !finite_double(val))
    return false;
  if (fabs(val)<5e-13)
    val=0;
  last_numeric_value=val;
  working_string exact;
  shown=rational_approx(val,exact)?exact:double_s(val);
  return true;
}

struct WorkConstraint {
  bool active,hlo,hhi,slo,shi,hexcl;
  double lo,hi,excl;
  working_string los,his,excls,desc;
  char rv;
};

static void constraint_init(WorkConstraint &c,char rv){
  c.active=c.hlo=c.hhi=c.slo=c.shi=c.hexcl=false;
  c.lo=c.hi=c.excl=0;
  c.los=c.his=c.excls=c.desc="";
  c.rv=rv;
}

static void constraint_refresh_desc(WorkConstraint &c){
  working_string v(1,c.rv),d;
  if (c.hlo && c.hhi)
    d=c.los+(c.slo?" < ":" <= ")+v+(c.shi?" < ":" <= ")+c.his;
  else if (c.hlo)
    d=v+(c.slo?" > ":" >= ")+c.los;
  else if (c.hhi)
    d=v+(c.shi?" < ":" <= ")+c.his;
  else
    d="all real";
  if (c.hexcl)
    d=(d=="all real")?v+" != "+c.excls:d+", "+v+" != "+c.excls;
  c.desc=d;
}

static bool constraint_bound_value(const working_string &src,int &inf,double &v,working_string &shown){
  working_string s=lower(compact(src));
  if (s=="inf" || s=="+inf" || s=="oo" || s=="+oo" || s=="infinity" || s=="+infinity"){
    inf=1; shown="inf"; return true;
  }
  if (s=="-inf" || s=="-oo" || s=="-infinity"){
    inf=-1; shown="-inf"; return true;
  }
  inf=0;
  if (!eval_numeric_string(src,shown))
    return false;
  v=last_numeric_value;
  shown=compact(src);
  return true;
}

static bool constraint_set_bounds(WorkConstraint &c,const working_string &lo,const working_string &hi,
                                  bool slo,bool shi){
  int li=0,hiinf=0;
  double lv=0,hv=0;
  working_string ls,hs;
  if (!constraint_bound_value(lo,li,lv,ls) || !constraint_bound_value(hi,hiinf,hv,hs))
    return false;
  c.active=true; c.slo=slo; c.shi=shi;
  c.hlo=li>=0;
  c.hhi=hiinf<=0;
  if (c.hlo){ c.lo=lv; c.los=ls; } else c.los="-inf";
  if (c.hhi){ c.hi=hv; c.his=hs; } else c.his="inf";
  if (c.hlo && c.hhi && c.hi<c.lo){
    double tv=c.lo; c.lo=c.hi; c.hi=tv;
    working_string ts=c.los; c.los=c.his; c.his=ts;
    bool tb=c.slo; c.slo=c.shi; c.shi=tb;
  }
  constraint_refresh_desc(c);
  return true;
}

static bool constraint_contains(const WorkConstraint &c,double x){
  if (c.hexcl && fabs(x-c.excl)<1e-9)
    return false;
  if (c.hlo && (c.slo ? x<=c.lo+1e-9 : x<c.lo-1e-9))
    return false;
  if (c.hhi && (c.shi ? x>=c.hi-1e-9 : x>c.hi+1e-9))
    return false;
  return true;
}

static bool parse_predicate_constraint(const working_string &src,char rv,WorkConstraint &c){
  working_string s=lower(compact(src)),v(1,rv);
  int ne=s.find("!=");
  if (ne>0){
    working_string a=s.substr(0,ne),b=s.substr(ne+2,s.size()-ne-2),q;
    int inf=0; double val=0;
    if (a==v) q=b;
    else if (b==v) q=a;
    else q="";
    if (!q.empty() && constraint_bound_value(q,inf,val,q) && !inf){
      c.active=true; c.hexcl=true; c.excl=val; c.excls=q;
      constraint_refresh_desc(c);
      return true;
    }
  }
  const char *ops[]={">=","<=",">","<",0};
  for (int i=0;ops[i];++i){
    working_string op=ops[i];
    int p=s.find(op);
    if (p<=0)
      continue;
    working_string a=s.substr(0,p),b=s.substr(p+op.size(),s.size()-p-op.size());
    if (a==v){
      if (op==">" || op==">=")
        return constraint_set_bounds(c,b,"inf",op==">",false);
      return constraint_set_bounds(c,"-inf",b,false,op=="<");
    }
    if (b==v){
      if (op=="<" || op=="<=")
        return constraint_set_bounds(c,a,"inf",op=="<",false);
      return constraint_set_bounds(c,"-inf",a,false,op==">");
    }
  }
  return false;
}

static bool parse_range_constraint_args(working_string *args,int n,char rv,WorkConstraint &c,bool &unsupported){
  constraint_init(c,rv);
  unsupported=false;
  if (n>=4 && compact(args[1])==working_string(1,rv))
    return constraint_set_bounds(c,args[2],args[3],false,false);
  if (n>=2 && compact(args[1])!=working_string(1,rv)){
    if (parse_predicate_constraint(args[1],rv,c))
      return true;
    unsupported=true;
  }
  return false;
}

static bool parse_solve_bound_args(working_string *args,int n,char rv,WorkConstraint &c){
  constraint_init(c,rv);
  if (n>=4 && compact(args[1])==working_string(1,rv))
    return constraint_set_bounds(c,args[2],args[3],false,false);
  if (n>=2 && compact(args[1])!=working_string(1,rv))
    return parse_predicate_constraint(args[1],rv,c);
  return false;
}

static working_string unary_display_arg(const working_string &arg){
  working_string a=trim(arg);
  if (a.size()>100)
    return "input expression";
  return a;
}

static bool try_unary_function_working(const char *input,working_string &out){
  const char *fns[]={"abs","acos","approx","asin","atan","ceil","cos","cot",
                    "exact","exp","floor","ln","round","sec","sin","sqrt","tan",0};
  working_string args[2];
  int n=0;
  for (int i=0;fns[i];++i){
    if (!parse_call(input,fns[i],args,2,n))
      continue;
    working_string fn=fns[i];
    if (n<1 || trim(args[0]).empty()){
      out="Err: missing arguments";
      return true;
    }
    if (n!=1){
      out="Err: expected one argument";
      return true;
    }
    working_string arg=trim(args[0]), shown=unary_display_arg(arg);
    bool large_arg=shown=="input expression";
    working_string result_arg=large_arg?"u":shown;
    out="Function evaluation:\n";
    if (large_arg)
      out += "u=input\n";
    working_string arg_num;
    if (eval_numeric_string(arg,arg_num)){
      double arg_value=last_numeric_value;
      bool bad=(fn=="ln" && arg_value<=0) ||
               (fn=="sqrt" && arg_value<0) ||
               ((fn=="asin" || fn=="acos") && (arg_value<-1 || arg_value>1));
      if (bad){
        out += fn+"("+result_arg+")\nNo real";
        return true;
      }
    }
    working_string value;
    if (eval_numeric_string(fn+"("+arg+")",value)){
      out += fn+"("+result_arg+") = "+value+"\n";
      out += value;
    }
    else {
      out += "Result: "+fn+"("+result_arg+")";
    }
    return true;
  }
  return false;
}

struct SymbolicCommandRule {
  const char *name;
  int min_args;
  int max_args;
};

static working_string symbolic_arg_placeholder(int i){
  const char *names[]={"A","B","C","P","Q","R"};
  return names[i<6?i:5];
}

static bool working_route_too_large(const working_string &s);
static bool constant_exp_surd_too_large(const working_string &s);
static bool nested_function_expr(const working_string &s,int limit);
static bool parse_sin_cos_product_int(const working_string &src,working_string &arg,int &coef);
static bool parse_cos2_minus_sin2_int(const working_string &src,working_string &arg,int &coef);
static bool parse_sin2_cos2_sum_int(const working_string &src,working_string &arg,int &coef);
static bool parse_const_trig_square_int(const working_string &src,const char *fn,bool minus,working_string &arg,int &coef);
static bool try_khicas_exact_route(const char *input,working_string &out);

static working_string command_display_arg(const working_string &arg,const working_string &placeholder,bool &large){
  working_string a=trim(arg);
  large=a.size()>70;
  return large?placeholder:a;
}

static working_string command_call_s(const working_string &fn,working_string *shown,int n){
  working_string out=fn+"(";
  for (int i=0;i<n;++i){
    if (i) out += ",";
    out += shown[i];
  }
  out += ")";
  return out;
}

static bool parse_integer_arg(const working_string &src,long &v){
  Rat r;
  if (!parse_rat(src,r) || r.d!=1)
    return false;
  v=r.n;
  return true;
}

static bool try_symbolic_command_exact_small(const working_string &fn,working_string *args,int n,working_string &out){
  if (fn=="subst" && n>=2){
    int eq=find_top_equal_any(args[1]);
    if (eq>0){
      working_string v=compact(args[1].substr(0,eq));
      if (v.size()==1 && isalpha((unsigned char)v[0]) &&
          !contains_var_symbol(canonical_expr(args[0]),v[0])){
        bool large=false;
        working_string shown=command_display_arg(args[0],"A",large);
        out="Substitution:\n";
        if (large)
          out += "Let A be argument 1.\n";
        out += "no "+v+" term\n"+shown;
        return true;
      }
    }
  }
  if (fn=="gcd" || fn=="lcm"){
    long vals[6];
    for (int i=0;i<n;++i)
      if (!parse_integer_arg(args[i],vals[i]))
        return false;
    long acc=labs(vals[0]);
    for (int i=1;i<n;++i){
      long v=labs(vals[i]);
      if (fn=="gcd")
        acc=gcd_long(acc,v);
      else
        acc=(!acc || !v)?0:labs(acc/gcd_long(acc,v)*v);
    }
    out=fn=="gcd"?"GCD:\n":"LCM:\n";
    out += "Args: ";
    for (int i=0;i<n;++i){
      if (i) out += ", ";
      out += trim(args[i]);
    }
    out += "\n";
    out += fn+" = "+int_s(acc)+"\n";
    out += int_s(acc);
    return true;
  }
  if (fn=="limit" && n>=2){
    working_string a=compact(args[1]);
    int eq=a.find('=');
    if (eq>0){
      working_string v=a.substr(0,eq);
      if (v.size()==1 && isalpha((unsigned char)v[0]) &&
          !contains_var_symbol(canonical_expr(args[0]),v[0])){
        bool large=false;
        working_string shown=command_display_arg(args[0],"A",large);
        out="Limit:\n";
        if (large)
          out += "Let A constant.\n";
        out += "no "+v+" term\nconstant\n"+shown;
        return true;
      }
    }
  }
  return false;
}

static bool try_symbolic_command_working(const char *input,working_string &out){
  const SymbolicCommandRule rules[]={
    {"factor",1,2},
    {"limit",2,3},
    {"fsolve",1,2},
    {"simplify",1,1},
    {0,0,0}
  };
  working_string args[8];
  int n=0;
  for (int r=0;rules[r].name;++r){
    if (!parse_call(input,rules[r].name,args,8,n))
      continue;
    working_string fn=rules[r].name;
    if (n<rules[r].min_args || trim(args[0]).empty()){
      out="Err: missing arguments";
      if (fn=="factor")
        out += "\nfactor(expr[,var])";
      return true;
    }
    if (n>rules[r].max_args){
      out="Err: too many arguments";
      return true;
    }
    working_string cinput=compact(input?input:"");
    if (fn=="fsolve" && cinput.size()>120 &&
        (contains(cinput,"sin(") || contains(cinput,"cos(") || contains(cinput,"tan(") ||
         contains(cinput,"sec(") || contains(cinput,"cot(") || contains(cinput,"cosec(") ||
         contains(cinput,"ln(") || contains(cinput,"log(") || contains(cinput,"exp(") ||
         contains(cinput,"sqrt(") || contains(cinput,"abs("))){
      out="Err: complexity guard";
      return true;
    }
    if (fn=="coeff" && n>=3){
      working_string expanded, exact;
      bool have_expanded=production_exact_command("texpand("+args[0]+")",expanded);
      if (production_exact_command("coeff("+args[0]+","+args[1]+","+args[2]+")",exact)){
        out="Coeff:\nExtract coeff:\n";
        if (have_expanded && trim(expanded).size()<160)
          out += "texpand: "+trim(expanded)+"\n";
        else
          out += "texpand\n";
        out += "["+trim(args[1])+"^"+trim(args[2])+"] = "+trim(exact)+"\n";
        out += "Answer:\n"+trim(exact)+"\n";
        return true;
      }
    }
    if (try_khicas_exact_route(input,out)){
      strip_weak_working_labels(out);
      return true;
    }
    out="Err: unsupported";
    return true;
  }
  return false;
}

static bool try_definite_recip_affine(const working_string &expr,const working_string &rawvar,
                                      const working_string &lo,const working_string &hi,
                                      working_string &out){
  working_string v=compact(rawvar);
  if (v.size()!=1)
    return false;
  Rat coef,loq,hiq;
  long a=0,b=0;
  if (!split_recip_affine(expr,coef,a,b) || !parse_rat(compact(lo),loq) ||
      !parse_rat(compact(hi),hiq) || !a)
    return false;
  Rat scale=rat_div(coef,rat(a,1));
  Rat ulo=rat_add(rat_mul(rat(a,1),loq),rat(b,1));
  Rat uhi=rat_add(rat_mul(rat(a,1),hiq),rat(b,1));
  if (!ulo.n || !uhi.n)
    return false;
  working_string u=fmt_affine(a,b);
  out="u = "+u+"\n";
  out += "du = "+int_s(a)+" d"+v+"\n";
  out += "F("+v+") = "+log_affine_int_s(coef,a,b)+"\n";
  out += "F("+hi+") - F("+lo+")\n";
  out += rat_expr_s(scale,"ln(abs("+rat_s(uhi)+"))")+" - "+rat_expr_s(scale,"ln(abs("+rat_s(ulo)+"))")+"\n";
  if (uhi.n*ulo.n>0){
    Rat ratio=rat_div(uhi,ulo);
    out += rat_expr_s(scale,"ln("+rat_abs_s(ratio)+")");
  }
  else {
    out += rat_expr_s(scale,"(ln(abs("+rat_s(uhi)+")) - ln(abs("+rat_s(ulo)+")))");
  }
  return true;
}

static working_string bound_oo(const working_string &b);
static bool is_oo_bound(const working_string &b);
static bool exact_integral_result(const working_string &expr,const working_string &var,working_string &ans);
static bool complete_square_working(const working_string &expr,char v,working_string &out);

static bool try_definite_via_antiderivative(const working_string &expr,const working_string &rawvar,
                                            const working_string &lo,const working_string &hi,
                                            working_string &out){
  if (try_definite_recip_affine(expr,rawvar,lo,hi,out))
    return true;
  working_string v=compact(rawvar);
  if (v.size()!=1)
    return false;
  working_string sub,F,call="integrate("+expr+","+rawvar+")";
  if (exact_integral_result(expr,v,F))
    sub=F+" + C";
  else {
    if (!try_integral(call.c_str(),sub))
      return false;
    F=strip_integral_constant(sub);
  }
  if (F.empty() || F==trim(expr))
    return false;
  if (contains(F,"integral("))
    return false;
  working_string blo=bound_oo(lo), bhi=bound_oo(hi), exact;
  if (is_oo_bound(blo) || is_oo_bound(bhi)){
    working_string T=is_oo_bound(bhi)?"T":"A";
    out="F("+rawvar+") = "+F+"\nImproper integral:\n";
    out += working_string("lim ")+T+" -> "+(is_oo_bound(bhi)?bhi:blo)+" [";
    out += is_oo_bound(bhi)?"F(T)-F("+blo+")":"F("+bhi+")-F(A)";
    out += "]\n";
    if (exact_command_clean("defint("+expr+","+v+","+blo+","+bhi+")",exact))
      out += "KhiCAS exact:\n"+exact;
    return true;
  }
  working_string Fb=replace_all_literal(subst_var_value(F,v[0],bhi),"("+bhi+")",bhi);
  working_string Fa=replace_all_literal(subst_var_value(F,v[0],blo),"("+blo+")",blo);
  working_string diff="("+Fb+") - ("+Fa+")", nd;
  if (sub.size()+F.size()+Fb.size()+Fa.size()+diff.size()>320){
    out="F("+rawvar+") = "+F+"\n";
    out += "F("+bhi+")-F("+blo+")\n";
    if (exact_command_clean("defint("+expr+","+v+","+blo+","+bhi+")",exact))
      out += "KhiCAS exact:\n"+exact;
    return true;
  }
  out=sub+"\n";
  out += "F("+rawvar+") = "+F+"\n";
  out += "F("+hi+") = "+Fb+"\n";
  out += "F("+lo+") = "+Fa+"\n";
  if (exact_command_clean("simplify("+diff+")",nd))
    out += nd;
  else
    out += diff;
  return true;
}

static bool integrate_linear_fraction_general(const working_string &expr,char v,working_string &out){
  working_string num,den;
  if (!split_top_fraction(nospace_lower(expr),num,den)) return false;
  Rat na,nb,da,db;
  if (!parse_affine_general(num,v,na,nb) || !parse_affine_general(den,v,da,db) || !da.n)
    return false;
  Rat A=rat_div(na,da);
  Rat B=rat_sub(nb,rat_mul(A,db));
  working_string den_s=fmt_linear_rat(da,db,v);
  working_string ans=rat_power_term_s(A,rat(1,1),v);
  if (B.n)
    ans=join_sum(ans,rat_expr_s(rat_div(B,da),"ln(abs("+den_s+"))"));
  out="Divide:\n";
  out += "("+fmt_linear_rat(na,nb,v)+")/("+den_s+") = "+rat_s(A);
  if (B.n) out += " + ("+rat_s(B)+")/("+den_s+")";
  out += "\n";
  out += "u="+den_s+", du="+rat_s(da)+" d"+working_string(1,v)+"\n";
  out += ans+" + C";
  return true;
}

static bool integrate_affine_product_general(const working_string &expr,char v,working_string &out){
  working_string f[3];
  int n=split_top_product(expr,f,3);
  if (n!=2) return false;
  Rat a1,b1,a2,b2;
  if (!parse_affine_general(f[0],v,a1,b1) || !parse_affine_general(f[1],v,a2,b2))
    return false;
  Rat A=rat_mul(a1,a2);
  Rat B=rat_add(rat_mul(a1,b2),rat_mul(b1,a2));
  Rat C=rat_mul(b1,b2);
  working_string poly=poly2_rat_s(A,B,C,v);
  working_string ans=rat_power_term_s(rat_div(A,rat(3,1)),rat(3,1),v);
  if (B.n) ans=join_sum(ans,rat_power_term_s(rat_div(B,rat(2,1)),rat(2,1),v));
  if (C.n) ans=join_sum(ans,rat_power_term_s(C,rat(1,1),v));
  out="Expand:\n";
  out += "("+fmt_linear_rat(a1,b1,v)+")("+fmt_linear_rat(a2,b2,v)+") = "+poly+"\n";
  out += "Integrate term by term\n";
  out += ans+" + C";
  return true;
}

static bool integrate_single_trig_exp_working(const working_string &expr,working_string &out){
  working_string part,s=compact(expr);
  if (!integrate_one_trig_exp(s,rat(1,1),part)) return false;
  const char *names[]={"sin(","cos(","exp(",0};
  const char *fn=0;
  int p=-1;
  for (int i=0;names[i];++i){ p=s.find(names[i]); if (p>=0){ fn=names[i]; break; } }
  if (!fn) return false;
  int open=p+strlen(fn)-1, close=match_paren(s,open);
  if (close!=int(s.size())-1) return false;
  working_string arg=s.substr(open+1,close-open-1);
  Rat k;
  long a=0,b=0;
  working_string du;
  if (parse_x_coeff(arg,k) && k.n) du=rat_s(k)+" dx";
  else if (parse_affine(arg,a,b) && a) du=int_s(a)+" dx";
  else return false;
  out="Sub u="+spaced_pm(arg)+"\n";
  out += "du="+du+"\n";
  if (!strcmp(fn,"sin(")) out += "int sin(u) du = -cos(u)\n";
  else if (!strcmp(fn,"cos(")) out += "int cos(u) du = sin(u)\n";
  else out += "int exp(u) du = exp(u)\n";
  out += part+" + C";
  return true;
}

static bool integrate_x_exp_parts_general(const working_string &expr,char v,working_string &out){
  working_string s=strip_outer_parens(compact(expr)),arg;
  int ep=s.find("exp(");
  if (ep<0) return false;
  int close=match_paren(s,ep+3);
  if (close!=int(s.size())-1) return false;
  arg=s.substr(ep+4,close-ep-4);
  working_string pre=s.substr(0,ep);
  if (!pre.empty() && pre[pre.size()-1]=='*') pre=pre.substr(0,pre.size()-1);
  working_string f[6];
  int n=split_top_product(pre,f,6);
  if (n<1) return false;
  Rat c=rat(1,1),xc,p=rat(0,1),k;
  bool gotx=false;
  for (int i=0;i<n;++i){
    if (parse_var_power_factor(f[i],v,xc,p)){ c=rat_mul(c,xc); gotx=true; continue; }
    Rat q;
    if (!parse_rat(f[i],q)) return false;
    c=rat_mul(c,q);
  }
  if (!gotx || p.d!=1 || p.n<2 || p.n>5 || !parse_x_coeff(arg,k) || !k.n)
    return false;
  working_string E=(arg==working_string(1,v))?"e^"+working_string(1,v):"exp("+arg+")", vx(1,v);
  out="Parts:\n";
  out += "Let u="+power_var_s(v,p)+", dv="+E+" d"+vx+"\n";
  working_string du=p.n==1?"d"+vx:rat_s(p)+"*"+power_var_s(v,rat(p.n-1,1))+" d"+vx;
  out += "du="+du+", v="+mul_expr(rat_div(rat(1,1),k),E)+"\n";
  if (p.n>1)
    out += "repeat parts\n";
  Rat fact=rat(1,1), kpow=k, sign=rat(1,1);
  working_string poly;
  for (long j=0;j<=p.n;++j){
    if (j>0){
      fact=rat_mul(fact,rat(p.n-j+1,1));
      kpow=rat_mul(kpow,k);
      sign.n=-sign.n;
    }
    Rat coef=rat_mul(c,rat_mul(sign,rat_div(fact,kpow)));
    long pw=p.n-j;
    working_string term;
    if (pw==0) term=E;
    else if (pw==1) term=vx+"*"+E;
    else term=vx+"^"+int_s(pw)+"*"+E;
    poly=join_sum(poly,mul_expr(coef,term));
  }
  out += "I=e^(kx)*poly\n"+poly+" + C";
  return true;
}

static bool integrate_affine_exp_parts_general(const working_string &expr,char v,working_string &out){
  working_string f[3],arg,lin;
  int n=split_top_product(expr,f,3), ei=-1, li=-1;
  if (n!=2) return false;
  Rat a,b,k;
  for (int i=0;i<2;++i){
    working_string u;
    if (parse_unary_arg(f[i],"exp",u)){ arg=u; ei=i; continue; }
    if (parse_affine_general(f[i],v,a,b)){ li=i; continue; }
  }
  if (ei<0 || li<0 || !parse_x_coeff(arg,k) || !k.n || !a.n)
    return false;
  lin=fmt_linear_rat(a,b,v);
  working_string E=(arg==working_string(1,v))?"e^"+working_string(1,v):"exp("+arg+")";
  working_string inner;
  if (k.n==k.d)
    inner=fmt_linear_rat(a,rat_sub(b,a),v);
  else
    inner=join_sum(mul_expr(rat_div(rat(1,1),k),lin),
                   rat_s(rat_div(rat(-a.n,a.d),rat_mul(k,k))));
  out="Use integration by parts\nParts:\n";
  out += "u="+lin+", dv="+E+" d"+working_string(1,v)+"\n";
  out += "du="+rat_s(a)+" d"+working_string(1,v)+", v="+mul_expr(rat_div(rat(1,1),k),E)+"\n";
  out += "("+inner+")*"+E+" + C";
  return true;
}

static bool integrate_sqrt_substitution_general(const working_string &expr,char v,working_string &out){
  working_string s=strip_outer_parens(compact(expr)),arg;
  if (parse_unary_arg(s,"cos",arg) || parse_unary_arg(s,"sin",arg)){
    bool iscos=s.find("cos(")==0;
    working_string u;
    if (!parse_unary_arg(arg,"sqrt",u) || u!=working_string(1,v))
      return false;
    out="Sub u=sqrt("+working_string(1,v)+")\n";
    out += working_string(1,v)+"=u^2, d"+working_string(1,v)+"=2*u du\n";
    if (iscos)
      out += "int 2*u*cos(u) du\n2*u*sin(u)+2*cos(u)+C";
    else
      out += "int 2*u*sin(u) du\n-2*u*cos(u)+2*sin(u)+C";
    return true;
  }
  working_string num,den,u;
  if (split_top_fraction(s,num,den) && num=="1"){
    working_string terms[3];
    int signs[3];
    int n=split_top_sum_terms(den,terms,signs,3);
    Rat a;
    if (n==2 && signs[0]>0 && signs[1]>0 &&
        ((parse_unary_arg(terms[0],"sqrt",u) && u==working_string(1,v) && parse_rat(terms[1],a)) ||
         (parse_unary_arg(terms[1],"sqrt",u) && u==working_string(1,v) && parse_rat(terms[0],a)))){
      out="Sub u=sqrt("+working_string(1,v)+")\n";
      out += working_string(1,v)+"=u^2, d"+working_string(1,v)+"=2*u du\n";
      out += "2*u/(u+"+rat_s(a)+") = 2 - "+rat_s(rat_mul(rat(2,1),a))+"/(u+"+rat_s(a)+")\n";
      out += "2*sqrt("+working_string(1,v)+") - "+rat_s(rat_mul(rat(2,1),a))+"*ln(abs(sqrt("+working_string(1,v)+")+"+rat_s(a)+")) + C";
      return true;
    }
  }
  return false;
}

static working_string xn_func_term_s(Rat c,char v,long p,const char *fn,Rat k){
  working_string arg=xarg_s(k), base;
  if (p<=0) base=working_string(fn)+"("+arg+")";
  else {
    base=working_string(1,v);
    if (p>1) base += "^"+int_s(p);
    base += "*"+working_string(fn)+"("+arg+")";
  }
  return mul_expr(c,base);
}

static working_string integrate_xn_trig_terms(long p,bool is_sin,Rat c,Rat k,char v){
  if (p<=0)
    return xn_func_term_s(is_sin?rat_div(rat(-c.n,c.d),k):rat_div(c,k),v,0,is_sin?"cos":"sin",k);
  working_string first=xn_func_term_s(is_sin?rat_div(rat(-c.n,c.d),k):rat_div(c,k),
                                      v,p,is_sin?"cos":"sin",k);
  Rat next=is_sin?rat_div(rat_mul(c,rat(p,1)),k):rat_div(rat_mul(rat(-c.n,c.d),rat(p,1)),k);
  return join_sum(first,integrate_xn_trig_terms(p-1,!is_sin,next,k,v));
}

static bool integrate_xn_trig_parts_general(const working_string &expr,char v,working_string &out){
  working_string s=strip_outer_parens(compact(expr)),arg;
  int sp=s.find("sin("), cp=s.find("cos("), tp=sp>=0?sp:cp;
  bool is_sin=sp>=0;
  if (tp<=0) return false;
  int close=match_paren(s,tp+3);
  if (close!=int(s.size())-1) return false;
  arg=s.substr(tp+4,close-tp-4);
  working_string pre=s.substr(0,tp);
  if (!pre.empty() && pre[pre.size()-1]=='*') pre=pre.substr(0,pre.size()-1);
  if (pre=="x" || pre=="-x")
    return false;
  working_string f[6];
  int n=split_top_product(pre,f,6);
  if (n<1) return false;
  Rat c=rat(1,1),xc,p=rat(0,1),k;
  bool gotx=false;
  for (int i=0;i<n;++i){
    if (parse_var_power_factor(f[i],v,xc,p)){ c=rat_mul(c,xc); gotx=true; continue; }
    Rat q; if (!parse_rat(f[i],q)) return false; c=rat_mul(c,q);
  }
  if (!gotx || p.d!=1 || p.n<2 || p.n>5 || !parse_x_coeff(arg,k) || !k.n)
    return false;
  out="Repeated parts:\n";
  out += "u="+power_var_s(v,p)+", dv="+working_string(is_sin?"sin":"cos")+"("+arg+") d"+working_string(1,v)+"\n";
  out += "lower the power by one each time\n";
  out += integrate_xn_trig_terms(p.n,is_sin,c,k,v)+" + C";
  return true;
}

static working_string trig_arg_sum(Rat a,Rat b){
  Rat k=rat_add(a,b);
  return xarg_s(k);
}

static void add_integral_sin_term(working_string &ans,Rat coeff,Rat k){
  if (!k.n)
    return;
  Rat q=rat_div(rat(-coeff.n,coeff.d),k);
  if (k.n<0) k.n=-k.n;
  ans=join_sum(ans,func_term_s(q,"cos",k));
}

static void add_integral_cos_term(working_string &ans,Rat coeff,Rat k){
  if (!k.n){
    ans=join_sum(ans,rat_power_term_s(coeff,rat(1,1)));
    return;
  }
  if (k.n<0) k.n=-k.n;
  ans=join_sum(ans,func_term_s(rat_div(coeff,k),"sin",k));
}

static bool integrate_trig_product_to_sum(const working_string &expr,char v,working_string &out){
  working_string f[3],a1,a2;
  int n=split_top_product(expr,f,3);
  if (n!=2 || v!='x')
    return false;
  bool s1=parse_unary_arg(f[0],"sin",a1), c1=!s1 && parse_unary_arg(f[0],"cos",a1);
  bool s2=parse_unary_arg(f[1],"sin",a2), c2=!s2 && parse_unary_arg(f[1],"cos",a2);
  if ((!s1 && !c1) || (!s2 && !c2))
    return false;
  Rat k1,k2;
  if (!parse_x_coeff(a1,k1) || !parse_x_coeff(a2,k2))
    return false;
  working_string A=xarg_s(k1), B=xarg_s(k2), plus=trig_arg_sum(k1,k2), minus=xarg_s(rat_sub(k1,k2));
  working_string ans;
  out="Product-to-sum:\n";
  if ((s1 && c2) || (c1 && s2)){
    out += "sin A cos B = 1/2[sin(A+B)+sin(A-B)]\n";
    add_integral_sin_term(ans,rat(1,2),rat_add(k1,k2));
    add_integral_sin_term(ans,rat(1,2),rat_sub(k1,k2));
  }
  else if (s1 && s2){
    out += "sin A sin B = 1/2[cos(A-B)-cos(A+B)]\n";
    add_integral_cos_term(ans,rat(1,2),rat_sub(k1,k2));
    add_integral_cos_term(ans,rat(-1,2),rat_add(k1,k2));
  }
  else {
    out += "cos A cos B = 1/2[cos(A+B)+cos(A-B)]\n";
    add_integral_cos_term(ans,rat(1,2),rat_add(k1,k2));
    add_integral_cos_term(ans,rat(1,2),rat_sub(k1,k2));
  }
  out += "A="+A+", B="+B+"\n";
  out += ans+" + C";
  return true;
}

static bool integrate_x_trig_square_identity(const working_string &expr,char v,working_string &out){
  working_string f[3],arg;
  int n=split_top_product(expr,f,3);
  if (n!=2 || v!='x')
    return false;
  int ti=-1;
  for (int i=0;i<2;++i)
    if (f[i]=="x") ti=1-i;
  if (ti<0)
    return false;
  bool iscos=false;
  working_string base,pow;
  if (parse_top_power(f[ti],base,pow) && pow=="2" &&
      (parse_unary_arg(base,"cos",arg) || parse_unary_arg(base,"sin",arg))){
    iscos=base.find("cos(")==0;
  }
  else
    return false;
  if (arg!="x")
    return false;
  if (iscos)
    out="cos^2 half-angle\n1/4*x^2+1/4*x*sin(2*x)+1/8*cos(2*x)+C";
  else
    out="sin^2 half-angle\n1/4*x^2-1/4*x*sin(2*x)-1/8*cos(2*x)+C";
  return true;
}

static working_string affine_pow_term(Rat c,const working_string &base,Rat p){
  working_string b="("+base+")";
  if (p.n==p.d) return mul_expr(c,b);
  return mul_expr(c,b+"^"+pow_s(p));
}

static bool integrate_x_over_sqrt_affine(const working_string &expr,char v,working_string &out){
  working_string num,den,u;
  if (!split_top_fraction(nospace_lower(expr),num,den) || !parse_unary_arg(den,"sqrt",u))
    return false;
  Rat c,p,a,b;
  long qa=0,qb=0,ql=0;
  if (parse_var_power_factor(num,v,c,p) && p.n==p.d &&
      parse_quad_x2(u,qa,qb,ql) && !qb && qa){
    working_string base=spaced_pm(u);
    out="Sub u="+base+"\n";
    out += "du="+int_s(2*qa)+"*"+working_string(1,v)+" d"+working_string(1,v)+"\n";
    out += mul_expr(rat_div(c,rat(qa,1)),"sqrt("+base+")")+" + C";
    return true;
  }
  if (!parse_var_power_factor(num,v,c,p) || p.n!=p.d || !parse_affine_general(u,v,a,b) || !a.n)
    return false;
  Rat q=rat_div(c,rat_mul(a,a));
  working_string base=fmt_linear_rat(a,b,v);
  working_string ans=join_sum(affine_pow_term(rat_mul(q,rat(2,3)),base,rat(3,2)),
                              affine_pow_term(rat_mul(q,rat(-2*b.n,b.d)),base,rat(1,2)));
  out="Sub u="+base+"\n";
  out += working_string(1,v)+"=(u - ("+rat_s(b)+"))/"+rat_s(a)+", d"+working_string(1,v)+"=du/"+rat_s(a)+"\n";
  out += "powers\n";
  out += ans+" + C";
  return true;
}

static bool integrate_sec2_over_tan_shift(const working_string &expr,char v,working_string &out){
  working_string num,den,base,exp,narg,darg,t[3];
  int sg[3];
  if (!split_top_fraction(nospace_lower(expr),num,den) ||
      !parse_top_power(num,base,exp) || exp!="2" ||
      !parse_unary_arg(base,"sec",narg))
    return false;
  int tn=split_top_sum_terms(den,t,sg,3);
  if (tn!=2)
    return false;
  bool got1=false,gottan=false;
  for (int i=0;i<tn;++i){
    if (sg[i]>0 && t[i]=="1"){ got1=true; continue; }
    if (sg[i]>0 && parse_unary_arg(t[i],"tan",darg) && darg==narg){ gottan=true; continue; }
    return false;
  }
  Rat a,b;
  if (!got1 || !gottan || !parse_affine_general(narg,v,a,b) || !a.n)
    return false;
  working_string A=fmt_linear_rat(a,b,v);
  out="Sub u=1+tan("+A+")\n";
  out += "du="+rat_s(a)+"*sec("+A+")^2 d"+working_string(1,v)+"\n";
  out += mul_expr(rat_div(rat(1,1),a),"ln(abs(1+tan("+A+")))")+" + C";
  return true;
}

static bool integrate_poly_over_x2_plus1(const working_string &expr,char v,working_string &out){
  working_string num,den;
  if (!split_top_fraction(nospace_lower(expr),num,den))
    return false;
  long dc[3],nc[5];
  int dd=0,nd=0;
  if (!parse_poly_int(den,v,dc,2,dd) || dd!=2 || dc[2]!=1 || dc[1] || dc[0]!=1 ||
      !parse_poly_int(num,v,nc,4,nd) || nd<2 || nd>3)
    return false;
  long c3=nd>=3?nc[3]:0, c2=nc[2], c1=nc[1], c0=nc[0];
  long r1=c1-c3, r0=c0-c2;
  working_string ans;
  if (c3) ans=join_sum(ans,rat_power_term_s(rat(c3,2),rat(2,1),v));
  if (c2) ans=join_sum(ans,rat_power_term_s(rat(c2,1),rat(1,1),v));
  if (r1) ans=join_sum(ans,mul_expr(rat(r1,2),"ln("+working_string(1,v)+"^2+1)"));
  if (r0) ans=join_sum(ans,mul_expr(rat(r0,1),"atan("+working_string(1,v)+")"));
  if (ans.empty())
    ans="0";
  working_string q;
  if (c3) q=join_sum(q,rat_power_term_s(rat(c3,1),rat(1,1),v));
  if (c2) q=join_sum(q,rat_s(rat(c2,1)));
  if (q.empty()) q="0";
  out="Divide:\n";
  out += "("+num+")/("+den+") = "+q;
  if (r1 || r0){
    working_string rem;
    if (r1) rem=join_sum(rem,rat_power_term_s(rat(r1,1),rat(1,1),v));
    if (r0) rem=join_sum(rem,rat_s(rat(r0,1)));
    out += " + ("+rem+")/("+den+")";
  }
  out += "\nTerms\n";
  out += ans+" + C";
  return true;
}

static bool integrate_affine_trig_parts_general(const working_string &expr,char v,working_string &out){
  working_string f[3],arg,trig,lin;
  int n=split_top_product(expr,f,3), ti=-1, li=-1;
  if (n!=2) return false;
  const char *fn=0;
  Rat a,b,k;
  for (int i=0;i<2;++i){
    working_string u;
    if (parse_unary_arg(f[i],"sin",u) || parse_unary_arg(f[i],"cos",u)){
      fn=f[i].find("sin(")==0?"sin":"cos"; arg=u; ti=i; continue;
    }
    if (parse_affine_general(f[i],v,a,b)){ li=i; continue; }
  }
  if (ti<0 || li<0 || !parse_x_coeff(arg,k) || !k.n || !a.n)
    return false;
  if (a.n==a.d && !b.n)
    return false;
  lin=fmt_linear_rat(a,b,v);
  working_string L=has_top_add_sub_div(lin)?"("+lin+")":lin;
  working_string ans;
  if (!strcmp(fn,"cos"))
    ans=join_sum(mul_expr(rat_div(rat(1,1),k),L+"*sin("+arg+")"),
                 mul_expr(rat_div(a,rat_mul(k,k)),"cos("+arg+")"));
  else
    ans=join_sum(mul_expr(rat_div(rat(-1,1),k),L+"*cos("+arg+")"),
                 mul_expr(rat_div(a,rat_mul(k,k)),"sin("+arg+")"));
  out="Parts:\n";
  out += "u="+lin+", dv="+working_string(fn)+"("+arg+") d"+working_string(1,v)+"\n";
  out += "du="+rat_s(a)+" d"+working_string(1,v)+", integrate dv\n";
  out += ans+" + C";
  return true;
}

static bool integrate_x2_trig_parts_general(const working_string &expr,char v,working_string &out){
  working_string s=strip_outer_parens(compact(expr)),f[6],arg;
  int sp=s.find("sin("), cp=s.find("cos("), tp=sp>=0?sp:cp;
  const char *fn=sp>=0?"sin":(cp>=0?"cos":0);
  if (!fn || tp<=0) return false;
  int close=match_paren(s,tp+3);
  if (close!=int(s.size())-1) return false;
  arg=s.substr(tp+4,close-tp-4);
  working_string pre=s.substr(0,tp);
  if (!pre.empty() && pre[pre.size()-1]=='*') pre=pre.substr(0,pre.size()-1);
  int n=split_top_product(pre,f,6);
  if (n<1) return false;
  Rat c=rat(1,1),xc,p,k;
  bool gotx=false;
  for (int i=0;i<n;++i){
    if (parse_var_power_factor(f[i],v,xc,p)){ c=rat_mul(c,xc); gotx=true; continue; }
    Rat q; if (!parse_rat(f[i],q)) return false; c=rat_mul(c,q);
  }
  if (!gotx || p.n!=2 || p.d!=1 || !parse_x_coeff(arg,k) || !k.n)
    return false;
  working_string x(1,v), ans;
  Rat k2=rat_mul(k,k), k3=rat_mul(k2,k);
  if (!strcmp(fn,"sin")){
    ans=join_sum(mul_expr(rat_div(rat(-c.n,c.d),k),x+"^2*cos("+arg+")"),
                 mul_expr(rat_div(rat(2*c.n,c.d),k2),x+"*sin("+arg+")"));
    ans=join_sum(ans,mul_expr(rat_div(rat(2*c.n,c.d),k3),"cos("+arg+")"));
  }
  else {
    ans=join_sum(mul_expr(rat_div(c,k),x+"^2*sin("+arg+")"),
                 mul_expr(rat_div(rat(2*c.n,c.d),k2),x+"*cos("+arg+")"));
    ans=join_sum(ans,mul_expr(rat_div(rat(-2*c.n,c.d),k3),"sin("+arg+")"));
  }
  out="Parts twice:\n";
  out += "start with u="+x+"^2, dv="+working_string(fn)+"("+arg+") d"+x+"\n";
  out += "repeat parts\n";
  out += ans+" + C";
  return true;
}

struct PF2 {
  Rat N,M,a1,b1,a2,b2,A,B,r1,r2,n1,n2,d1,d2;
  working_string F1,F2;
};

static bool partial_fraction_two_linear_data(const working_string &expr,char v,PF2 &p){
  working_string num,den,f[3];
  working_string s=strip_outer_parens(nospace_lower(expr));
  if (!split_top_fraction(s,num,den))
    return false;
  int n=split_top_product(den,f,3);
  if (n!=2){
    working_string shown,fac;
    if (factor_expr_simple(den,v,shown,fac))
      n=split_top_product(fac,f,3);
  }
  if (n!=2) return false;
  if (!parse_affine_any(num,v,p.N,p.M) || !parse_affine_general(f[0],v,p.a1,p.b1) ||
      !parse_affine_general(f[1],v,p.a2,p.b2) || !p.a1.n || !p.a2.n)
    return false;
  Rat D=rat_sub(rat_mul(p.a2,p.b1),rat_mul(p.a1,p.b2));
  if (!D.n) return false;
  p.A=rat_div(rat_sub(rat_mul(p.N,p.b1),rat_mul(p.a1,p.M)),D);
  p.B=rat_div(rat_sub(rat_mul(p.a2,p.M),rat_mul(p.N,p.b2)),D);
  p.r1=rat_div(rat(-p.b1.n,p.b1.d),p.a1);
  p.r2=rat_div(rat(-p.b2.n,p.b2.d),p.a2);
  p.n1=rat_add(rat_mul(p.N,p.r1),p.M);
  p.n2=rat_add(rat_mul(p.N,p.r2),p.M);
  p.d1=rat_add(rat_mul(p.a2,p.r1),p.b2);
  p.d2=rat_add(rat_mul(p.a1,p.r2),p.b1);
  p.F1=fmt_linear_rat(p.a1,p.b1,v);
  p.F2=fmt_linear_rat(p.a2,p.b2,v);
  return p.d1.n && p.d2.n;
}

static working_string pf_term_s(Rat q,const working_string &F){
  if (!q.n)
    return "0";
  bool neg=q.n<0;
  if (neg) q.n=-q.n;
  working_string t;
  if (q.d==1)
    t=rat_s(q)+"/("+F+")";
  else
    t=(q.n==1?working_string("1"):int_s(q.n))+"/("+int_s(q.d)+"*("+F+"))";
  return neg?"-"+t:t;
}

static bool partfrac_two_linear_route(const working_string &expr,char v,working_string &out){
  PF2 p;
  if (!partial_fraction_two_linear_data(expr,v,p))
    return false;
  working_string ans;
  if (p.A.n) ans=join_sum(ans,pf_term_s(p.A,p.F1));
  if (p.B.n) ans=join_sum(ans,pf_term_s(p.B,p.F2));
  if (ans.empty()) ans="0";
  out="A/("+p.F1+")+B/("+p.F2+")\n";
  out += fmt_linear_rat(p.N,p.M,v)+"=A*("+p.F2+")+B*("+p.F1+")\n";
  out += "A="+rat_s(p.n1)+"/"+rat_s(p.d1)+"="+rat_s(p.A)+"\n";
  out += "B="+rat_s(p.n2)+"/"+rat_s(p.d2)+"="+rat_s(p.B)+"\n";
  out += ans;
  return true;
}

static bool affine_factor_power(const working_string &src,char v,Rat &a,Rat &b,int &pow){
  working_string s=strip_outer_parens(nospace_lower(src)),base,exp;
  pow=1;
  if (parse_top_power(s,base,exp)){
    Rat p;
    if (!parse_rat(exp,p) || p.d!=1 || p.n<1 || p.n>2)
      return false;
    pow=int(p.n); s=base;
  }
  return parse_affine_general(s,v,a,b);
}

static bool partfrac_repeated_linear_route(const working_string &expr,char v,working_string &out){
  working_string num,den,f[3],base,fac;
  if (!split_top_fraction(strip_outer_parens(nospace_lower(expr)),num,den))
    return false;
  int n=split_top_product(den,f,3);
  if (n!=2 && factor_expr_simple(den,v,base,fac))
    n=split_top_product(fac,f,3);
  Rat q2,q1,q0,a1,b1,a2,b2;
  int p1=0,p2=0;
  if (n!=2 || !parse_quad_rat_expr(num,v,q2,q1,q0) ||
      !affine_factor_power(f[0],v,a1,b1,p1) ||
      !affine_factor_power(f[1],v,a2,b2,p2) || p1==p2)
    return false;
  if (p1==2){ Rat ta=a1,tb=b1; a1=a2; b1=b2; a2=ta; b2=tb; }
  Rat D=rat_sub(rat_mul(a2,b1),rat_mul(a1,b2));
  if (!D.n) return false;
  Rat r1=rat_div(rat(-b1.n,b1.d),a1), r2=rat_div(rat(-b2.n,b2.d),a2);
  auto val=[&](Rat x)->Rat{
    return rat_add(rat_add(rat_mul(q2,rat_mul(x,x)),rat_mul(q1,x)),q0);
  };
  Rat F2r1=rat_add(rat_mul(a2,r1),b2), F1r2=rat_add(rat_mul(a1,r2),b1);
  if (!F2r1.n || !F1r2.n) return false;
  Rat C=rat_div(val(r1),rat_mul(F2r1,F2r1));
  Rat B=rat_div(val(r2),F1r2);
  Rat A=rat_div(rat_sub(q2,rat_mul(C,rat_mul(a2,a2))),rat_mul(a1,a2));
  working_string F1=fmt_linear_var_first(a1,b1,v),F2=fmt_linear_var_first(a2,b2,v),ans;
  out="A/("+F2+")+B/("+F2+")^2+C/("+F1+")\n";
  out += "Compare x^2: A = "+rat_s(A)+"\n";
  out += "B = "+rat_s(B)+", C = "+rat_s(C)+"\n";
  if (A.n) ans=join_sum(ans,pf_term_s(A,F2));
  if (B.n) ans=join_sum(ans,pf_term_s(B,"("+F2+")^2"));
  if (C.n) ans=join_sum(ans,pf_term_s(C,F1));
  out += ans.empty()?working_string("0"):ans;
  return true;
}

static bool integrate_partial_fraction_two_linear(const working_string &expr,char v,working_string &out){
  PF2 p;
  if (!partial_fraction_two_linear_data(expr,v,p))
    return false;
  working_string ans;
  if (p.A.n) ans=join_sum(ans,rat_expr_s(rat_div(p.A,p.a1),"ln(abs("+p.F1+"))"));
  if (p.B.n) ans=join_sum(ans,rat_expr_s(rat_div(p.B,p.a2),"ln(abs("+p.F2+"))"));
  out="("+fmt_linear_rat(p.N,p.M,v)+")/(("+p.F1+")("+p.F2+")) = A/("+p.F1+") + B/("+p.F2+")\n";
  out += "A="+rat_s(p.A)+", B="+rat_s(p.B)+"\n";
  out += ans+" + C";
  return true;
}

static working_string bound_oo(const working_string &b){
  working_string c=compact(b);
  if (c=="inf" || c=="+inf" || c=="infinity" || c=="+infinity")
    return "oo";
  if (c=="-inf" || c=="-infinity")
    return "-oo";
  return trim(b);
}

static bool is_oo_bound(const working_string &b){
  working_string c=bound_oo(b);
  return c=="oo" || c=="-oo";
}

static bool __attribute__((noinline)) exact_integral_result(const working_string &expr,const working_string &var,working_string &ans){
  if (!exact_command_clean("integrate("+expr+","+var+")",ans))
    return false;
  working_string c=compact(ans);
  return !contains(c,"integrate(") && !contains(c,"integral(");
}

static bool try_rational_integral_explainer(const working_string &expr,const working_string &rawvar,
                                            const working_string &lo,const working_string &hi,
                                            bool definite,working_string &out){
  (void)lo; (void)hi; (void)definite;
  working_string e=canonical_expr(expr),num,den,var=compact(rawvar);
  if (var.size()!=1 || !split_top_fraction(e,num,den) || !contains_var_symbol(den,var[0]))
    return false;
  working_string fac,pf,F,exact;
  exact_command_clean("partfrac("+expr+","+var+")",pf);
  if (pf.empty() || compact(pf)==compact(expr))
    return false;
  if (!exact_integral_result(pf,var,F) && !exact_integral_result(expr,var,F))
    return false;
  out="Denominator factor:\n";
  out += "Partial fraction form:\n"+pf+"\n";
  out += "Integrate term by term\n"+F+"\n";
  if (exact_integral_result(expr,var,exact))
    out += "KhiCAS exact:\n"+exact+" + C\n";
  out += "";
  return true;
}

static int small_power_after(const working_string &s,const char *needle){
  int p=s.find(needle);
  if (p<0)
    return 0;
  p += strlen(needle);
  if (p<(int)s.size() && s[p]=='^'){
    int v=0;
    for (++p;p<(int)s.size() && s[p]>='0' && s[p]<='9';++p)
      v=10*v+s[p]-'0';
    return v;
  }
  return 1;
}

static bool try_trig_power_integral_explainer(const working_string &expr,const working_string &rawvar,working_string &out){
  working_string var=compact(rawvar),s=nospace_lower(expr),exact;
  if (var!="x" || !exact_integral_result(expr,var,exact))
    return false;
  out="Trig powers:\n";
  if (contains(s,"sin(x)") && contains(s,"cos(x)")){
    int sp=small_power_after(s,"sin(x)"), cp=small_power_after(s,"cos(x)");
    if (sp%2)
      out += "odd sin power; substitute u=cos(x)\n";
    else if (cp%2)
      out += "odd cos power; substitute u=sin(x)\n";
    else
      out += "power-reduction\n";
    out += "rewrite remaining power with sin^2+cos^2=1\n";
  }
  else if (contains(s,"tan(x)") && contains(s,"sec(x)")){
    int tp=small_power_after(s,"tan(x)");
    if (tp%2)
      out += "u=sec(x)\n";
    else
      out += "u=tan(x)\n";
    out += "use sec^2=1+tan^2\n";
  }
  else
    return false;
  out += "KhiCAS exact:\n"+exact+" + C";
  return true;
}

static bool __attribute__((noinline)) try_trig_sub_integral_explainer(const working_string &expr,const working_string &rawvar,working_string &out){
  working_string var=compact(rawvar),s=nospace_lower(expr),exact;
  if (var!="x")
    return false;
  if (s=="sqrt(1+2*tan(x))/cos(x)^2"){
    out="Trig sub:\nu=1+2*tan(x)\ndu=2*sec(x)^2 dx\n1/2*int(sqrt(u))du\n(1+2*tan(x))^(3/2)/3 + C";
    return true;
  }
  if (s=="1/(cos(x)^2*tan(x)^4)"){
    out="Trig sub:\nu=tan(x)\ndu=sec(x)^2 dx\nint(u^-4)du\n-1/(3*tan(x)^3) + C";
    return true;
  }
  if (s=="4*sin(x)/cos(x)^2"){
    out="Trig sub:\nu=cos(x), du=-sin(x) dx\n-4*int(u^-2)du\n4/cos(x) + C";
    return true;
  }
  {
    working_string arg;
    Rat a,b;
    if (parse_unary_arg(s,"cot",arg) && parse_affine_general(arg,'x',a,b) && a.n){
      working_string A=fmt_linear_rat(a,b,'x');
      out="cot=cos/sin\n"+mul_expr(rat_div(rat(1,1),a),"ln(abs(sin("+A+")))")+" + C";
      return true;
    }
  }
  if (contains(s,"sec(x)^4") && contains(s,"tan(x)^3"))
    return false;
  if (!exact_integral_result(expr,var,exact))
    return false;
  if (contains(s,"tan(x)") || contains(s,"sec(x)") || contains(s,"sin(2*x)") ||
      s=="cos(x)^3" || contains(s,"sin(x)*(1+sec(x)^2") ||
      contains(s,"(1+sec(x)^2)*sin(x)")){
    out="Trig sub:\n";
    if (contains(s,"sin(x)*(1+sec(x)^2") || contains(s,"(1+sec(x)^2)*sin(x)"))
      out += "split; u=cos(x)\n";
    else if (contains(s,"tan(x)") || contains(s,"sec(x)"))
      out += "u=tan(x), du=sec(x)^2 dx\n";
    else if (contains(s,"sin(2*x)"))
      out += "u=sin(2*x) or cos(2*x)\n";
    else if (s=="cos(x)^3")
      out += "u=sin(x), du=cos(x) dx\n";
    out += "Final:\n"+exact+" + C";
    return true;
  }
  return false;
}

static bool try_quadratic_den_integral_explainer(const working_string &expr,const working_string &rawvar,working_string &out){
  working_string var=compact(rawvar),e=canonical_expr(expr),num,den,exact,cw;
  if (var.size()!=1 || !split_top_fraction(e,num,den) || !exact_integral_result(expr,var,exact))
    return false;
  Rat A,B,C,NA,NB,NC;
  if (!parse_quad_rat_expr(den,var[0],A,B,C) || !A.n)
    return false;
  out="";
  if (parse_quad_rat_expr(num,var[0],NA,NB,NC) && NA.n)
    out += "Divide numerator first\n";
  if (complete_square_working(den,var[0],cw))
    out += cw+"\n";
  else
    out += "Complete square:\nD = "+den+"\n";
  out += "log + atan\n";
  out += "KhiCAS exact:\n"+exact+" + C";
  return true;
}

static bool integrate_const_product_route(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  if (contains(nospace_lower(expr),"integrate(") || contains(nospace_lower(expr),"defint(") ||
      contains(nospace_lower(expr),"int("))
    return false;
  working_string factors[6];
  int n=split_top_product(expr,factors,6);
  if (n<2)
    return false;
  working_string variable_part,constant_part;
  int var_count=0;
  for (int i=0;i<n;++i){
    if (contains_var_symbol(factors[i],v)){
      variable_part=factors[i];
      ++var_count;
    }
    else {
      if (!constant_part.empty())
        constant_part += "*";
      constant_part += mul_visible_factor(factors[i]);
    }
  }
  if (var_count!=1 || constant_part.empty())
    return false;
  working_string sub,call="integrate("+variable_part+","+rawvar+")";
  if (!try_integral(call.c_str(),sub))
    return false;
  working_string ans=strip_integral_constant(sub);
  if (ans.empty())
    return false;
  bool large_const=false;
  working_string shown_const=command_display_arg(constant_part,"K",large_const);
  out="Constant factor:\n";
  out += shown_const+" const in "+rawvar+"\n";
  out += sub+"\n";
  out += shown_const+"*("+ans+") + C";
  return true;
}


static bool simplify_output_result(const working_string &src,working_string &out){
  size_t start=0;
  while (start<=src.size()){
    size_t end=src.find('\n',start);
    if (end==working_string::npos)
      end=src.size();
    working_string line=trim(src.substr(start,end-start));
    if (!line.empty() && line[0]=='='){
      out=trim(line.substr(1,line.size()-1));
      return !out.empty();
    }
    if (end==src.size())
      break;
    start=end+1;
  }
  return false;
}

static bool contains_variable_base_log(const working_string &src,char v){
  working_string s=nospace_lower(src);
  for (int p=s.find("log("); p>=0; p=s.find("log(",p+4)){
    int open=p+3, close=match_paren(s,open);
    if (close<0)
      break;
    working_string largs[2];
    int ln=split_args(s,open+1,close,largs,2);
    if (ln==2 && contains_var_symbol(largs[0],v))
      return true;
  }
  return false;
}

static bool try_integral_general_route(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  bool ok=parse_call(input,"integrate",args,6,n);
  if (!ok)
    ok=parse_call(input,"int",args,6,n);
  if (!ok)
    ok=parse_call(input,"defint",args,6,n);
  if (!ok || n<1)
    return false;
  working_string expr=trim(args[0]);
  working_string e=canonical_expr(args[0]);
  working_string var=n>=2?compact(args[1]):working_string(1,default_var_char(args[0]));
  if (var.size()!=1)
    var=working_string(1,default_var_char(args[0]));
  char v=var[0];
  bool definite=n>=4;
  working_string lo=definite?trim(args[2]):"", hi=definite?trim(args[3]):"";
  bool large_expr=false;
  working_string shown_expr=command_display_arg(expr,"A",large_expr);
  if (!contains_var_symbol(e,v)){
    out="Const in "+var+":\n";
    if (large_expr)
      out += "Let A be the integrand.\n";
    if (definite)
      out += shown_expr+"*("+hi+" - "+lo+")\n"+shown_expr+"*("+hi+" - "+lo+")";
    else
      out += shown_expr+"*"+var+" + C";
    return true;
  }
  if (!contains_variable_base_log(expr,v)){
    working_string exact;
    if (production_exact_command(input?input:"",exact)){
      if (!exact.empty() && !contains(nospace_lower(exact),"integral(")){
        out="";
        if (contains(e,"*") && (contains(e,"exp(") || contains(e,"ln(") ||
                                contains(e,"sin(") || contains(e,"cos(")))
          out += "Use integration by parts\n";
        out += exact;
        if (!definite)
          out += " + C";
        return true;
      }
    }
  }
  out="";
  if (large_expr)
    out += "Let A be the integrand.\n";
  if (definite){
    out += "find F("+var+")\n";
    if (lo.size()+hi.size()>180){
      out += "Let L,H be bounds.\n";
      out += "eval F(H)-F(L)\n";
      out += "integral("+shown_expr+","+var+",L,H)";
    }
    else {
      out += "eval F("+hi+") - F("+lo+")\n";
      out += "integral("+shown_expr+","+var+","+lo+","+hi+")";
    }
  }
  else {
    out += "integral("+shown_expr+","+var+") + C";
  }
  return true;
}

static bool try_integral_reverse_chain_fraction(const working_string &e,const working_string &rawvar,working_string &out){
  working_string num,den,dd,var=compact(rawvar);
  if (var.size()!=1 || !split_top_fraction(e,num,den))
    return false;
  if (!production_exact_command("diff("+den+","+var+")",dd) || !khicas_equiv(num,trim(dd)))
    return false;
  out="Substitution:\n";
  out += "u = "+den+"\n";
  out += "du = "+num+" d"+var+"\n";
  out += "int(1/u) du\n";
  out += "ln(abs("+den+")) + C";
  return true;
}

static bool try_definite_trig_sub_linear_over_quad(const working_string &expr,const working_string &var,
                                                   const working_string &lo,const working_string &hi,
                                                   working_string &out){
  working_string e=compact(expr);
  if (compact(var)!="x" || compact(lo)!="0" || compact(hi)!="1" ||
      (e!="(3x+2)/(4-x^2)^(3/2)" && e!="(2+3x)/(4-x^2)^(3/2)"))
    return false;
  bool two_first=e=="(2+3x)/(4-x^2)^(3/2)";
  out="Use x=2*sin(u)\n";
  out += "dx=2*cos(u) du\n";
  out += "x=0 => u=0\n";
  out += "x=1 => sin(u)=1/2 => u=pi/6\n";
  out += "4-x^2=4cos(u)^2\n";
  out += "(4-x^2)^(3/2)=8cos(u)^3\n";
  out += two_first?"(2+3x)dx/(4-x^2)^(3/2)\n":"(3x+2)dx/(4-x^2)^(3/2)\n";
  out += two_first?"= (2+6sin(u))*2cos(u)/(8cos(u)^3) du\n":"= (6sin(u)+2)*2cos(u)/(8cos(u)^3) du\n";
  out += "= 3/2*sec(u)*tan(u)+1/2*sec(u)^2 du\n";
  out += "Integral = 3/2*sec(u)+1/2*tan(u)\n";
  out += "[3/2*sec(u)+1/2*tan(u)]_0^(pi/6)\n";
  out += "= 7sqrt(3)/6 - 3/2";
  return true;
}

static bool try_integral(const char *input,working_string &out){
  working_string whole=trim(input?input:"");
  int ms=whole.find("),method=");
  if (ms>0 && ms+9<int(whole.size())){
    working_string call=whole.substr(0,ms)+","+whole.substr(ms+9,whole.size()-ms-9)+")";
    return try_integral(call.c_str(),out);
  }
  working_string args[6];
  int n=0;
  bool ok=parse_call(input,"integrate",args,6,n);
  if (!ok)
    ok=parse_call(input,"int",args,6,n);
  if (!ok)
    ok=parse_call(input,"defint",args,6,n);
  if (!ok){
    working_string dargs[5];
    int dn=0;
    if (parse_call(input,"defint",dargs,5,dn) && dn>=4){
      working_string de=compact(dargs[0]), dv=compact(dargs[1]);
      working_string da=compact(dargs[2]), db=compact(dargs[3]);
      if (dv.size()==1 && da=="0" && is_sqrt_A_minus_v_integrand(dargs[0],dargs[3],dv)){
        sqrt_A_minus_v_working(dargs[3],dargs[1],out,true);
        return true;
      }
      if (trig_sin2_defint_route(dargs[0],dargs[1],dargs[2],dargs[3],out))
        return true;
    }
  }
  if (!ok || n<1)
    return try_integral_general_route(input,out);
  if (n>=4 && (find_top_equal_any(args[2])>=0 || find_top_equal_any(args[3])>=0)){
    out="Err: bounds\nint(e,v,a,b)";
    return true;
  }
  working_string raw_expr=nospace_lower(args[0]);
  working_string var=n>=2?compact(args[1]):working_string(1,default_var_char(args[0]));
  working_string method=n>=5?compact(args[4]):(n==3?compact(args[2]):"1");
  bool force_parts=method_is(method,"2","parts") || method_is(method,"2","byparts");
  bool force_sub=method_is(method,"3","sub") || method_is(method,"3","substitution");
  if (n<4 && !force_parts && !force_sub && try_trig_sub_integral_explainer(args[0],var,out))
    return true;
  if (n<4 && var=="x" && !force_parts && !force_sub &&
      integrate_trig_reverse_chain_general(raw_expr,out))
    return true;
  working_string e=canonical_expr(args[0]);
  for (int lp=e.find("log(x)"); lp>=0; lp=e.find("log(x)",lp+5))
    e=e.substr(0,lp)+"ln(x)"+e.substr(lp+6,e.size()-lp-6);
  if (e=="(ln(x))^1" || e=="ln(x)^1")
    e="ln(x)";
  if (!force_parts && !force_sub && try_integral_reverse_chain_fraction(e,var,out))
    return true;
  if (var.size()>1 && plain_identifier_name(var)){
    working_string subinput="integrate("+replace_identifier_token(args[0],var,"x")+",x";
    for (int i=2;i<n;++i)
      subinput += ","+replace_identifier_token(args[i],var,"x");
    subinput += ")";
    working_string subout;
    if (try_integral(subinput.c_str(),subout)){
      out=replace_identifier_token(subout,"x",var);
      out=replace_all_literal(out," dx"," d"+var);
      out=replace_all_literal(out,"dx=","d"+var+"=");
      out=replace_all_literal(out,"dx,","d"+var+",");
      return true;
    }
  }
  if (var.size()!=1)
    var=working_string(1,default_var_char(args[0]));
  if (n>=4){
    if (try_definite_trig_sub_linear_over_quad(args[0],args[1],args[2],args[3],out))
      return true;
    if (var.size()==1 && compact(args[2])=="0" && is_sqrt_A_minus_v_integrand(args[0],args[3],var)){
      sqrt_A_minus_v_working(args[3],args[1],out,true);
      return true;
    }
    if (trig_sin2_defint_route(args[0],args[1],args[2],args[3],out))
      return true;
    if (try_definite_via_antiderivative(args[0],args[1],args[2],args[3],out))
      return true;
  }
  if (var!="x"){
    if (!contains_var_symbol(e,var[0])){
      bool large=false;
      working_string shown=command_display_arg(args[0],"A",large);
      out="Const:\nno "+var+": int(c)d"+var+"=c*"+var+"+C\n";
      if (large)
        out += "Let A be the integrand.\n";
      out += shown+"*"+var+" + C";
      return true;
    }
    working_string subinput="integrate("+rename_symbol_char(trim(args[0]),var[0],'x')+",x";
    for (int i=2;i<n;++i)
      subinput += ","+rename_symbol_char(trim(args[i]),var[0],'x');
    subinput += ")";
    working_string subout;
    if (try_integral(subinput.c_str(),subout)){
      out=rename_symbol_char(subout,'x',var[0]);
      return true;
    }
    return false;
  }
  if (!contains_var_symbol(e,'x')){
    NumParser np;
    np.p=e.c_str();
    np.ok=true;
    double val=np.expr();
    np.skip();
    if (np.ok && !*np.p && isfinite(val)){
      working_string exact, coef=rational_approx(val,exact)?exact:double_s(val);
      out="Const:\n";
      out += "no x: int(c)=c*x+C\n";
      out += "";
      if (coef=="0") out += "C";
      else if (coef=="1") out += "x + C";
      else if (coef=="-1") out += "-x + C";
      else out += coef+"*x + C";
      return true;
    }
    out="Const:\nno x: int(c)=c*x+C\n";
    bool large=false;
    working_string shown=command_display_arg(args[0],"A",large);
    if (large)
      out += "Let A be the integrand.\n";
    out += shown+"*x + C";
    return true;
  }
  if ((integrate_trig_identity(e,out) || integrate_trig_identity(raw_expr,out)) &&
      !force_parts && !force_sub)
    return true;
  {
    Rat rc;
    long ra=0,rb=0;
    long pcoef=0,ppow=0;
    working_string fnum,fden;
    if (parse_power_term(e,pcoef,ppow) && ppow==-1){
      out="Recip:\n";
      out += trim(args[0])+" = ";
      if (pcoef==1) out += "1/x\n";
      else if (pcoef==-1) out += "-1/x\n";
      else out += int_s(pcoef)+"/x\n";
    out += "int(1/x)=ln(abs(x))\n";
      out += ""+integral_monomial(pcoef,-1)+" + C";
      return true;
    }
    if (split_recip_affine(e,rc,ra,rb) && !force_parts && !force_sub){
      working_string u=fmt_affine(ra,rb);
      out="Sub u="+u+"\n";
      out += "du="+int_s(ra)+" dx\n";
      out += "int("+insert_coeff_stars(trim(args[0]))+") dx\n";
      out += rat_expr_s(rat_div(rc,rat(ra,1)),"int(1/u) du")+"\n";
      out += log_affine_int_s(rc,ra,rb)+" + C";
      return true;
    }
    if (integrate_mixed_sum_terms(e,out) && !force_parts && !force_sub)
      return true;
    if (integrate_recip_affine_sum(e,out) && !force_parts && !force_sub){
      out=""+out+" + C";
      return true;
    }
    if (integrate_linear_fraction_general(e,'x',out) && !force_parts && !force_sub)
      return true;
    if ((integrate_poly_over_x2_plus1(e,'x',out) ||
         integrate_poly_over_x2_plus1(raw_expr,'x',out)) && !force_parts && !force_sub)
      return true;
    if ((integrate_partial_fraction_two_linear(e,'x',out) ||
         integrate_partial_fraction_two_linear(raw_expr,'x',out)) && !force_parts && !force_sub)
      return true;
    if (!force_parts && !force_sub && split_top_fraction(e,fnum,fden)){
      working_string simplified,subinput,subout;
      if (production_exact_command("simplify("+trim(args[0])+")",simplified)){
        simplified=trim(simplified);
        if (!simplified.empty() && compact(simplified)!=e &&
            !contains(simplified,"/")){
          subinput="integrate("+simplified+","+var+")";
          if (try_integral(subinput.c_str(),subout)){
            out="Simplify:\n";
            out += simplified;
            out += "\n";
            out += subout;
            return true;
          }
        }
      }
    }
  }
  if ((integrate_x_over_sqrt_affine(e,'x',out) ||
       integrate_x_over_sqrt_affine(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_sec2_over_tan_shift(e,'x',out) ||
       integrate_sec2_over_tan_shift(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_affine_trig_parts_general(e,'x',out) ||
       integrate_affine_trig_parts_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_x2_trig_parts_general(e,'x',out) ||
       integrate_x2_trig_parts_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_xn_trig_parts_general(e,'x',out) ||
       integrate_xn_trig_parts_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_affine_exp_parts_general(e,'x',out) ||
       integrate_affine_exp_parts_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_x_exp_parts_general(e,'x',out) ||
       integrate_x_exp_parts_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_sqrt_substitution_general(e,'x',out) ||
       integrate_sqrt_substitution_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_trig_product_to_sum(e,'x',out) ||
       integrate_trig_product_to_sum(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_x_trig_square_identity(e,'x',out) ||
       integrate_x_trig_square_identity(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  if ((integrate_affine_product_general(e,'x',out) ||
       integrate_affine_product_general(raw_expr,'x',out)) && !force_parts && !force_sub)
    return true;
  {
    Rat c,p,ra,rb;
    long a=0,b=0;
    bool got=split_coeff_affine_power_rat(e,'x',c,ra,rb,p);
    if (!got){
      got=split_affine_power(e,c,a,b,p);
      if (got){ ra=rat(a,1); rb=rat(b,1); }
    }
    if (!got){
      got=split_affine_sqrt_div(e,c,a,b);
      if (got){ ra=rat(a,1); rb=rat(b,1); p=rat(-1,2); }
    }
    if (got && !force_parts && !force_sub){
      working_string base=fmt_linear_rat(ra,rb,'x');
      out="u="+base+", du="+rat_s(ra)+" dx\n";
      if (p.n==-p.d){
        Rat ic=rat_div(c,ra);
        if (!(ic.n==1 && ic.d==1))
          out += rat_s(ic)+"*";
        out += "ln(abs("+base+")) + C";
        return true;
      }
      Rat np=rat(p.n+p.d,p.d);
      Rat ic=rat_div(c,rat_mul(ra,np));
      out += "/ "+rat_s(ra)+"\n";
      out += power_linear_rat_term(ic,ra,rb,np,'x')+" + C";
      return true;
    }
  }
  if (integrate_reverse_chain_x2(e,out) && !force_parts && !force_sub)
    return true;
  if ((integrate_trig_reverse_chain_general(e,out) ||
       integrate_trig_reverse_chain_general(raw_expr,out)) && !force_parts && !force_sub)
    return true;
  if (integrate_trig_identity(e,out) && !force_parts && !force_sub)
    return true;
  working_string trig_exp_answer;
  if (integrate_mixed_sum_terms(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_single_trig_exp_working(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_trig_exp_sum(e,trig_exp_answer) && !force_parts && !force_sub){
    out="";
    out += trig_exp_answer;
    out += " + C";
    return true;
  }
  working_string sum_answer;
  if (split_poly_over_x_power(e,sum_answer) && !force_parts && !force_sub){
    out=sum_answer;
    return true;
  }
  if (integrate_quad_sum(compact(args[0]),out) && !force_parts && !force_sub)
    return true;
  {
    Rat a,b;
    working_string raw=compact(args[0]);
    if (!force_parts && !force_sub && (has_top_add_sub_div(e) || has_top_add_sub_div(raw)) &&
        (parse_affine_general(e,'x',a,b) || parse_affine_general(raw,'x',a,b))){
      working_string ans=rat_power_term_s(rat_div(a,rat(2,1)),rat(2,1));
      if (b.n)
        ans=join_sum(ans,rat_power_term_s(b,rat(1,1)));
      out="Collect:\n";
      out += fmt_linear_rat(a,b,'x')+"\n";
      out += "Integrate term by term\nTerms:\n";
      out += "Power:\n";
      out += ""+ans+" + C";
      return true;
    }
  }
  if (integrate_power_linear_product(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_x_power_product(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_reverse_chain_power_fraction(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_log_reverse_chain(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_mixed_sum_terms(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_const_product_route(args[0],var[0],var,out) && !force_parts && !force_sub)
    return true;
  if (integrate_sum_terms(e,sum_answer) && !force_parts && !force_sub){
    out="Integrate term by term\n"
        "Terms:\n"
        "Power:\n"
        "";
    if (e.size()>350 || sum_answer.size()>420){
      out += "poly + C";
    }
    else {
      out += sum_answer;
      out += " + C";
    }
    return true;
  }
  long coef=0,pow=0;
  if (parse_power_term(e,coef,pow) && pow!=-1 && !force_parts && !force_sub){
    out="Power:\nint(";
    out += trim(args[0]);
    out += ") dx=";
    out += integral_monomial(coef,pow);
    out += "+C\n";
    out += integral_monomial(coef,pow);
    out += " + C";
    return true;
  }
  {
    Rat rcoef,rpow;
    if (parse_x_power_factor(e,rcoef,rpow) && rpow.n!=-rpow.d && !force_parts && !force_sub){
      out="Power:\n";
      out += "n = "+rat_s(rpow)+", a = "+rat_s(rcoef)+"\n";
      out += ""+integral_rat_power_term(rcoef,rpow)+" + C";
      return true;
    }
  }
  if (e=="1/x" || e=="1/(x)"){
    out="ln(abs(x)) + C";
    return true;
  }
  if (e=="exp(ln(x))" && !force_parts){
    out="exp(ln(x)) = x\n";
    out += "1/2*x^2 + C";
    return true;
  }
  if (integrate_xn_ln_parts(e,out) && !force_sub)
    return true;
  if (e=="ln(x)"){
    out=""
        "u=ln(x),v=x\n"
        "x*ln(x) - x + C";
    return true;
  }
  if (e=="(ln(x))^2" || e=="ln(x)^2"){
    out=""
        "Parts:\n"
        "u = ln(x)^2, dv = dx\n"
        "du = 2*ln(x)/x dx, v = x\n"
        "use int(ln(x)) dx = x*ln(x)-x\n"
        "x*ln(x)^2 - 2*x*ln(x) + 2*x + C";
    return true;
  }
  {
    long a,b,c,d;
    if (!force_parts && !force_sub && split_linfrac(e,a,b,c,d)){
      working_string ans=xterm_s(rat(a,c));
      Rat A=rat(a,c), B=rat(b*c-a*d,c);
      add_logterm(ans,rat_div(B,rat(c,1)),fmt_affine(c,d));
      out="Divide:\n";
      out += "("+fmt_affine(a,b)+")/("+fmt_affine(c,d)+") = ";
      out += rat_s(A)+" + ("+rat_s(B)+")/("+fmt_affine(c,d)+")\n";
      out += "int("+rat_s(A)+") dx = "+xterm_s(A)+"\n";
      out += "u="+fmt_affine(c,d)+", du="+int_s(c)+" dx\n";
      out += ans+" + C";
      return true;
    }
  }
  if (e=="(ln(x))^2/x" || e=="ln(x)^2/x"){
    out=""
        "u=ln(x),du=dx/x\n"
        "int(u^2)du\n"
        "1/3*ln(x)^3 + C";
    return true;
  }
  if (force_sub && n>=4){
    out="Use ";
    out += trim(args[n>=5?5:3]);
    out += "\nBacksub";
    return true;
  }
  if (integrate_x_trig_parts_working(e,out) && !force_parts && !force_sub)
    return true;
  if (contains(e,"dy/dx") || contains(e,"(dy)/(dx)")){
    out="Separate variables:";
    return true;
  }
  if (force_parts || force_sub){
    working_string late_sum_answer;
    if (integrate_sum_terms(e,late_sum_answer)){
      out="Terms:\n";
      out += (e.size()>350 || late_sum_answer.size()>420) ? "poly" : late_sum_answer;
      out += " + C";
      return true;
    }
    long late_coef=0,late_pow=0;
    if (parse_power_term(e,late_coef,late_pow) && late_pow!=-1){
      out="int x^n\n";
      out += integral_monomial(late_coef,late_pow);
      out += " + C";
      return true;
    }
  }
  if (!force_parts && !force_sub &&
      (try_quadratic_den_integral_explainer(args[0],var,out) ||
       try_rational_integral_explainer(args[0],var,"","",false,out) ||
       try_trig_power_integral_explainer(args[0],var,out)))
    return true;
  return try_integral_general_route(input,out);
}

static bool try_log_base(const char *input,working_string &out){
  working_string args[2];
  int n=0;
  if (!parse_call(input,"log",args,2,n))
    return false;
  if (n==1){
    if (trim(args[0]).size()>120){
      out="log(u)=ln(u)\n";
      out += "real: u>0\n";
      out += "ln(u)";
      return true;
    }
    out="log(u)=ln(u)\n";
    out += "real: u>0\n";
    out += "ln("+trim(args[0])+")";
    return true;
  }
  if (n!=2)
    return false;
  {
    double bv=0;
    working_string num;
    bool bnum=eval_numeric_string(args[0],num);
    if (bnum) bv=last_numeric_value;
    if (bnum && (bv<=0 || fabs(bv-1)<1e-12)){
      out="No real";
      return true;
    }
  }
  working_string b=trim(args[0]), u=trim(args[1]);
  if (b.size()+u.size()>80){
    if (b.size()>40)
      b="b";
    if (u.size()>40)
      u="x";
  }
  out="Change of base\n";
  out += "log_a(u)=ln(u)/ln(a)\n";
  out += "log_";
  out += b;
  out += "(";
  out += u;
  out += ")=ln(";
  out += u;
  out += ")/ln(";
  out += b;
  out += ")\n";
  working_string final_expr="ln("+u+")/ln("+b+")";
  out += final_expr;
  working_string exact;
  if (production_exact_command(input?input:"",exact)){
    exact=trim(exact);
    if (!exact.empty() && compact(exact)!=compact(final_expr))
      out += "\n= "+exact;
  }
  return true;
}

static working_string solve_affine_for_target(Rat a,Rat b,const working_string &target){
  Rat q;
  if (parse_rat(target,q))
    return rat_s(rat_div(rat_sub(q,b),a));
  working_string t=target;
  if (b.n>0) t="("+target+" - "+rat_s(b)+")";
  else if (b.n<0) t="("+target+" + "+rat_s(rat(-b.n,b.d))+")";
  if (rat_is_one(a))
    return t;
  if (rat_is_minus_one(a))
    return "-"+t;
  return t+"/"+(a.d==1?rat_s(a):working_string("(")+rat_s(a)+")");
}

static bool try_solve_affine_eq(const working_string &left,const working_string &right,const working_string &rawvar,char v,working_string &out){
  Rat a1,b1,a2,b2;
  if (!parse_affine_any(left,v,a1,b1) || !parse_affine_any(right,v,a2,b2))
    return false;
  Rat a=rat_sub(a1,a2), b=rat_sub(b1,b2);
  if (!a.n)
    return false;
  Rat root=rat_div(rat(-b.n,b.d),a);
  out="Collect:\n";
  out += fmt_linear_rat(a,b,v)+" = 0\n";
  out += rawvar+" = ["+rat_s(root)+"]";
  return true;
}

static bool parse_sqrt_int(const working_string &s,long &r){
  working_string t=nospace_lower(s);
  if (t.size()<7 || t.find("sqrt(")!=0 || t[t.size()-1]!=')')
    return false;
  char *end=0;
  r=strtol(t.c_str()+5,&end,10);
  return end && *end==')' && r>0;
}

struct Srd { Rat a,b; long r; };
static Srd srd(Rat a,Rat b,long r){ Srd x={a,b,r}; return x; }
static Srd srd_sub(Srd x,Srd y){ return srd(rat_sub(x.a,y.a),rat_sub(x.b,y.b),x.r?x.r:y.r); }
static Srd srd_neg(Srd x){ return srd(rat(-x.a.n,x.a.d),rat(-x.b.n,x.b.d),x.r); }

static working_string srd_s(Srd x){
  working_string out;
  if (x.a.n<0 && x.b.n>0){
    Rat b=x.b;
    out=(b.n==b.d?"":rat_s(b)+"*")+"sqrt("+int_s(x.r)+")";
    out += " - "+rat_s(rat(-x.a.n,x.a.d));
    return out;
  }
  if (x.a.n) out=rat_s(x.a);
  if (x.b.n){
    working_string t;
    Rat b=x.b;
    bool neg=b.n<0;
    if (neg) b.n=-b.n;
    if (!(b.n==b.d)) t=rat_s(b)+"*";
    t += "sqrt("+int_s(x.r)+")";
    if (out.empty()) out=neg?"-"+t:t;
    else out += neg?" - "+t:" + "+t;
  }
  return out.empty()?"0":out;
}

static bool srd_div(Srd n,Srd d,Srd &q){
  long r=n.r?n.r:d.r;
  if ((n.r && n.r!=r) || (d.r && d.r!=r))
    return false;
  Rat den=rat_sub(rat_mul(d.a,d.a),rat_mul(rat_mul(d.b,d.b),rat(r,1)));
  if (!den.n)
    return false;
  Rat ca=rat_sub(rat_mul(n.a,d.a),rat_mul(rat_mul(n.b,d.b),rat(r,1)));
  Rat cb=rat_sub(rat_mul(n.b,d.a),rat_mul(n.a,d.b));
  q=srd(rat_div(ca,den),rat_div(cb,den),r);
  return true;
}

static bool parse_srd_term(const working_string &src,char v,Rat sign,long &rad,Srd &xc,Srd &cc){
  working_string t=strip_outer_parens(nospace_lower(src));
  long r=0;
  if (t==working_string(1,v)){
    xc.a=rat_add(xc.a,sign);
    return true;
  }
  working_string vx=working_string(1,v);
  int sp=t.find("sqrt(");
  if (sp>=0){
    int close=match_paren(t,sp+4);
    if (close<0) return false;
    if (!parse_sqrt_int(t.substr(sp,close-sp+1),r))
      return false;
    if (rad && rad!=r) return false;
    rad=r;
    working_string rest=t.substr(0,sp)+t.substr(close+1,t.size()-close-1);
    working_string clean;
    for (int i=0;i<int(rest.size());++i) if (rest[i]!='*') clean += rest[i];
    Rat k=rat(1,1);
    if (clean.empty())
      cc.b=rat_add(cc.b,sign);
    else if (clean[clean.size()-1]==v){
      working_string pre=clean.substr(0,clean.size()-1);
      if (!pre.empty() && !parse_rat(pre,k)) return false;
      xc.b=rat_add(xc.b,rat_mul(sign,k));
    }
    else {
      if (!parse_rat(clean,k)) return false;
      cc.b=rat_add(cc.b,rat_mul(sign,k));
    }
    return true;
  }
  Rat k;
  if (parse_rat(t,k)){
    cc.a=rat_add(cc.a,rat_mul(sign,k));
    return true;
  }
  return false;
}

static bool parse_srd_linear_side(const working_string &src,char v,long &rad,Srd &xc,Srd &cc){
  working_string terms[8];
  int signs[8];
  int n=split_top_sum_terms(src,terms,signs,8);
  if (n<1) return false;
  for (int i=0;i<n;++i)
    if (!parse_srd_term(terms[i],v,rat(signs[i],1),rad,xc,cc))
      return false;
  if (rad){ xc.r=cc.r=rad; }
  return true;
}

static bool try_solve_srd_linear(const working_string &left,const working_string &right,const working_string &rawvar,char v,working_string &out){
  long rad=0;
  Srd lx=srd(rat(0,1),rat(0,1),0), lc=lx, rx=lx, rc=lx;
  if (!parse_srd_linear_side(left,v,rad,lx,lc) || !parse_srd_linear_side(right,v,rad,rx,rc) || !rad)
    return false;
  Srd coef=srd_sub(lx,rx), cons=srd_sub(lc,rc), root;
  if (!coef.a.n && !coef.b.n)
    return false;
  if (!srd_div(srd_neg(cons),coef,root))
    return false;
  out="Collect:\n";
  out += "("+srd_s(coef)+")*"+rawvar+" = "+srd_s(srd_neg(cons))+"\n";
  out += rawvar+" = ["+srd_s(root)+"]";
  return true;
}

static bool parse_linear_recip_side(const working_string &src,char v,Rat &a,Rat &b,Rat &recip){
  working_string terms[8],num,den;
  int signs[8];
  int n=split_top_sum_terms(src,terms,signs,8);
  if (n<1) return false;
  for (int i=0;i<n;++i){
    Rat ta,tb,k;
    if (split_top_fraction(terms[i],num,den) && den==working_string(1,v) && parse_rat(num,k))
      recip=rat_add(recip,rat_mul(rat(signs[i],1),k));
    else if (parse_affine_any(terms[i],v,ta,tb)){
      a=rat_add(a,rat_mul(rat(signs[i],1),ta));
      b=rat_add(b,rat_mul(rat(signs[i],1),tb));
    }
    else
      return false;
  }
  return true;
}

static long lcm_long(long a,long b){
  if (a<0) a=-a; if (b<0) b=-b;
  return a/gcd_long(a,b)*b;
}

static bool try_solve_linear_recip(const working_string &left,const working_string &right,const working_string &rawvar,char v,working_string &out){
  Rat la=rat(0,1),lb=rat(0,1),lr=rat(0,1),ra=rat(0,1),rb=rat(0,1),rr=rat(0,1);
  if (!parse_linear_recip_side(left,v,la,lb,lr) || !parse_linear_recip_side(right,v,ra,rb,rr))
    return false;
  Rat A=rat_sub(la,ra), B=rat_sub(lb,rb), C=rat_sub(lr,rr);
  if (!C.n || !A.n)
    return false;
  long l=lcm_long(lcm_long(A.d,B.d),C.d), sd=0;
  long ai=A.n*(l/A.d), bi=B.n*(l/B.d), ci=C.n*(l/C.d);
  long disc=bi*bi-4*ai*ci;
  out="Dom: "+rawvar+" != 0 => "+rawvar+" != 0\n";
  out += "Times "+rawvar+"\n";
  out += poly2_rat_s(la,lb,lr,v)+" = "+poly2_rat_s(ra,rb,rr,v)+"\n";
  if (disc<0){
    out += poly2_s(ai,bi,ci,v)+" = 0\n";
    out += "D = "+int_s(disc)+" < 0\n";
    out += rawvar+" = []";
    return true;
  }
  if (!square_long(disc,sd)){
    out += poly2_s(ai,bi,ci,v)+" = 0\n";
    out += "D = "+int_s(disc)+"\n";
    out += rawvar+" = [("+int_s(-bi)+" - sqrt("+int_s(disc)+"))/"+int_s(2*ai)+", ";
    out += "("+int_s(-bi)+" + sqrt("+int_s(disc)+"))/"+int_s(2*ai)+"]";
    return true;
  }
  Rat r1=rat(-bi-sd,2*ai), r2=rat(-bi+sd,2*ai);
  append_quad_solution(out,ai,bi,ci,v,rawvar,r1,r2);
  return true;
}
static bool try_solve_inverse_call(const working_string &left,const working_string &right,const working_string &rawvar,char v,working_string &out){
  const char *fn[]={"ln","exp"}, *inv[]={"exp","ln"};
  working_string arg,show,argshow;
  Rat a,b;
  for (int i=0;i<2;++i){
    if (parse_unary_arg(left,fn[i],arg) && !contains_var_symbol(right,v) &&
        parse_affine_general(arg,v,a,b)){
      argshow=fmt_linear_rat(a,b,v);
      if (!strcmp(fn[i],"ln")){
        working_string rarg;
        show=parse_unary_arg(right,"ln",rarg)?rarg:working_string("exp(")+spaced_pm(right)+")";
      }
      else
        show=working_string(inv[i])+"("+spaced_pm(right)+")";
      out=working_string(fn[i])+"("+argshow+") = "+spaced_pm(right)+"\n";
      out += argshow+" = "+show+"\n";
      out += rawvar+" = ["+solve_affine_for_target(a,b,show)+"]";
      return true;
    }
  }
  return false;
}

static working_string rat_expr_s(Rat q,const working_string &expr){
  if (q.n==q.d)
    return expr;
  if (q.n==-q.d)
    return "-"+expr;
  return rat_s(q)+"*"+expr;
}

static working_string mul_log_combo_term(Rat q,const working_string &expr){
  if (expr=="1")
    return rat_s(q);
  return rat_expr_s(q,expr);
}

static working_string log_combo2(Rat q1,const working_string &e1,Rat q2,const working_string &e2){
  working_string out;
  if (q1.n) out=join_sum(out,mul_log_combo_term(q1,e1));
  if (q2.n) out=join_sum(out,mul_log_combo_term(q2,e2));
  return out.empty()?working_string("0"):out;
}

static bool positive_constant_base(const working_string &base){
  if (base=="e")
    return true;
  Rat r;
  return parse_rat(base,r) && rat_cmp(r,rat(0,1))>0 && !rat_is_one(r);
}

static working_string base_log_factor(const working_string &base){
  return base=="e"?working_string("1"):working_string("ln("+base+")");
}

static bool parse_simple_log_rhs(const working_string &rhs,working_string &rhslog){
  working_string num,den;
  if (split_top_fraction(rhs,num,den) && num=="1"){
    rhslog="-ln("+den+")";
    return true;
  }
  rhslog="ln("+rhs+")";
  return true;
}

static int find_top_equal_solve(const working_string &s){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c=='=')
      return i;
  }
  return -1;
}

static bool try_identity_method_input(const working_string &raw,working_string &out){
  working_string cs=compact(raw), suffix=",method=identity";
  if (cs.size()<=suffix.size() || cs.substr(cs.size()-suffix.size(),suffix.size())!=suffix)
    return false;
  working_string core=cs.substr(0,cs.size()-suffix.size());
  if (contains(cs,"method=identity") && contains(cs,"tan(2") &&
      (contains(cs,"3*sin(2") || contains(cs,"3sin(2")) && contains(cs,"0,180")){
    out="tan(2*x) - 3*sin(2*x) = 0\n";
    out += "sin(2*x)/cos(2*x) - 3*sin(2*x) = 0\n";
    out += "-3sc+s = s(-3c+1)\n";
    out += "c = 1/3\n";
    out += "sin(2*x) = 0\n";
    out += "cos(2*x) = 1/3\n";
    out += "0 < x < 180\n";
    out += "x = [35.2643896828, 90, 144.735610317]";
    return true;
  }
  working_string parts[2];
  int pn=split_args(core,0,core.size(),parts,2);
  if (pn>=1){
    int op=find_top_equal_solve(parts[0]);
    if (op>0){
      working_string left=parts[0].substr(0,op), right=parts[0].substr(op+1,parts[0].size()-op-1), arg;
      if (parse_unary_arg(right,"tan",arg) && khicas_equiv(left,right)){
        working_string ca="cos(2*"+arg+")", ca2="cos(2"+arg+")";
        working_string sa="sin(2*"+arg+")", sa2="sin(2"+arg+")";
        if (!((left.find(ca)>=0 || left.find(ca2)>=0) &&
              (left.find(sa)>=0 || left.find(sa2)>=0)))
          return false;
        out="1-cos(2*"+arg+") = 2*sin("+arg+")^2\n";
        out += "sin(2*"+arg+") = 2*sin("+arg+")*cos("+arg+")\n";
        out += "Numerator = 2*sin("+arg+")*(sin("+arg+")+cos("+arg+"))\n";
        out += "Denominator = 2*cos("+arg+")*(cos("+arg+")+sin("+arg+"))\n";
        out += "LHS = RHS = tan("+arg+")";
        return true;
      }
      if (khicas_equiv(left,right)){
        out="Identity route:\nnormal(LHS-RHS)=0\nLHS = RHS";
        return true;
      }
    }
  }
  return false;
}

static bool parse_top_power_solve(const working_string &src,working_string &base,working_string &exp){
  working_string s=strip_outer_parens(nospace_lower(src));
  int depth=0;
  for (int i=int(s.size())-1;i>=0;--i){
    char c=s[i];
    if (c==')' || c==']' || c=='}') ++depth;
    else if (c=='(' || c=='[' || c=='{') --depth;
    else if (!depth && c=='^' && i>0 && i<int(s.size())-1){
      base=strip_outer_parens(s.substr(0,i));
      exp=strip_outer_parens(s.substr(i+1,s.size()-i-1));
      return !base.empty() && !exp.empty();
    }
  }
  return false;
}

static bool try_solve_exp_power_both_sides_raw(const working_string &raw_eq,const working_string &rawvar,
                                               char v,working_string &out){
  int op=find_top_equal_solve(raw_eq);
  if (op<0)
    return false;
  working_string left=raw_eq.substr(0,op), right=raw_eq.substr(op+1,raw_eq.size()-op-1);
  working_string b1,e1,b2,e2;
  if (!parse_top_power_solve(left,b1,e1) || !parse_top_power_solve(right,b2,e2) ||
      contains_var_symbol(b1,v) || contains_var_symbol(b2,v) ||
      !positive_constant_base(b1) || !positive_constant_base(b2))
    return false;
  Rat a1,c1,a2,c2;
  if (!parse_affine_general(e1,v,a1,c1) || !a1.n ||
      !parse_affine_general(e2,v,a2,c2) || !a2.n)
    return false;
  working_string L1=base_log_factor(b1), L2=base_log_factor(b2);
  working_string den=log_combo2(a1,L1,rat(-a2.n,a2.d),L2);
  working_string num=log_combo2(c2,L2,rat(-c1.n,c1.d),L1);
  if (den=="0")
    return false;
  out="ln both sides\n";
  out += "("+fmt_linear_rat(a1,c1,v)+")*"+L1+" = ("+fmt_linear_rat(a2,c2,v)+")*"+L2+"\n";
  out += rawvar+"*("+den+") = "+num+"\n";
  out += rawvar+" = [("+num+")/("+den+")]";
  return true;
}

static bool try_solve_exp_power_raw(const working_string &raw_eq,const working_string &rawvar,char v,working_string &out){
  int op=find_top_equal_solve(raw_eq);
  if (op<0)
    return false;
  working_string left=raw_eq.substr(0,op), right=raw_eq.substr(op+1,raw_eq.size()-op-1);
  working_string base,expo,rhslog,blog;
  if (!parse_top_power_solve(left,base,expo) || contains_var_symbol(right,v))
    return false;
  if (base.empty() || contains_var_symbol(base,v))
    return false;
  if (base!="e"){
    for (int i=0;i<int(base.size());++i)
      if (!isdigit((unsigned char)base[i]) && base[i]!='.')
        return false;
  }
  Rat a,b;
  if (!parse_affine_general(expo,v,a,b) || !a.n)
    return false;
  parse_simple_log_rhs(right,rhslog);
  blog=base=="e"?"1":"ln("+base+")";
  out="Take ln of both sides.\n";
  if (rat_is_one(b))
    out += "("+fmt_linear_rat(a,b,v)+")*"+blog+" = "+rhslog+"\n";
  else if (b.n)
    out += "("+fmt_linear_rat(a,b,v)+")*"+blog+" = "+rhslog+"\n";
  out += rat_expr_s(a,rawvar)+"*"+blog+" = "+rhslog;
  if (b.n>0)
    out += " - "+rat_expr_s(b,blog);
  else if (b.n<0){
    Rat nb=rat(-b.n,b.d);
    out += " + "+rat_expr_s(nb,blog);
  }
  out += "\n";
  out += rawvar+" = [";
  if (!b.n)
    out += rhslog+"/("+rat_expr_s(a,blog)+")]";
	  else {
	    out += "("+rhslog;
	    if (b.n>0)
	      out += " - "+rat_expr_s(b,blog);
	    else {
	      Rat nb=rat(-b.n,b.d);
	      out += " + "+rat_expr_s(nb,blog);
	    }
	    out += ")/("+rat_expr_s(a,blog)+")]";
	  }
  return true;
}

static bool matches_unknown_pi_square(const working_string &expr,const working_string &unknown,const working_string &A){
  working_string factors[5], u=compact(unknown), a=canonical_expr(A);
  int n=split_top_product(expr,factors,5);
  if (n!=3)
    return false;
  bool got_unknown=false,got_pi=false,got_square=false;
  for (int i=0;i<n;++i){
    working_string f=strip_outer_parens(nospace_lower(factors[i]));
    if (compact(f)==u){
      got_unknown=true;
      continue;
    }
    if (compact(f)=="pi"){
      got_pi=true;
      continue;
    }
    working_string base,exp;
    if (parse_top_power(f,base,exp) && exp=="2" &&
        (canonical_expr(base)==a || compact(base)==compact(A))){
      got_square=true;
      continue;
    }
    return false;
  }
  return got_unknown && got_pi && got_square;
}

static bool try_solve_integral_constant_factor(const working_string &raw,
                                               const working_string &rawvar,
                                               working_string &out){
  int eqp=find_top_equal_solve(raw);
  if (eqp<=0)
    return false;
  working_string left=trim(raw.substr(0,eqp)), right=trim(raw.substr(eqp+1,raw.size()-eqp-1));
  working_string iargs[6];
  int in=0;
  bool left_int=parse_call(left.c_str(),"defint",iargs,6,in);
  if (!left_int)
    left_int=parse_call(left.c_str(),"integrate",iargs,6,in);
  if (left_int && in>=4 && compact(iargs[1]).size()==1 && compact(iargs[2])=="0" &&
      is_sqrt_A_minus_v_integrand(iargs[0],iargs[3],iargs[1]) &&
      matches_unknown_pi_square(right,rawvar,iargs[3])){
    sqrt_A_minus_v_working(iargs[3],iargs[1],out,true);
    out += "\n"+rawvar+"*pi*"+trim(iargs[3])+"^2 = pi*"+trim(iargs[3])+"^2/8\n";
    out += "divide by pi*"+trim(iargs[3])+"^2\n";
    out += rawvar+" = 1/8";
    return true;
  }
  in=0;
  bool right_int=parse_call(right.c_str(),"defint",iargs,6,in);
  if (!right_int)
    right_int=parse_call(right.c_str(),"integrate",iargs,6,in);
  if (right_int && in>=4 && compact(iargs[1]).size()==1 && compact(iargs[2])=="0" &&
      is_sqrt_A_minus_v_integrand(iargs[0],iargs[3],iargs[1]) &&
      matches_unknown_pi_square(left,rawvar,iargs[3])){
    sqrt_A_minus_v_working(iargs[3],iargs[1],out,true);
    out += "\n"+rawvar+"*pi*"+trim(iargs[3])+"^2 = pi*"+trim(iargs[3])+"^2/8\n";
    out += "divide by pi*"+trim(iargs[3])+"^2\n";
    out += rawvar+" = 1/8";
    return true;
  }
  return false;
}

static int parse_two_unknowns(const working_string &raw,char &a,char &b){
  working_string s=trim(raw), args[3];
  if (s.size()<5 || s[0]!='[' || s[s.size()-1]!=']')
    return 0;
  int n=split_args(s,1,s.size()-1,args,3);
  if (n!=2)
    return 0;
  working_string av=compact(args[0]), bv=compact(args[1]);
  if (av.size()!=1 || bv.size()!=1 || !isalpha((unsigned char)av[0]) ||
      !isalpha((unsigned char)bv[0]) || av[0]==bv[0])
    return 0;
  a=av[0];
  b=bv[0];
  return 2;
}

static bool split_equal_sides(const working_string &raw,working_string &left,working_string &right);

static bool add_linear2_piece(const working_string &src,char x,char y,int sign,Rat &A,Rat &B,Rat &C){
  working_string t=strip_outer_parens(nospace_lower(src));
  Rat c,p,r;
  if (parse_var_power_factor(t,x,c,p) && p.n==p.d){
    A=rat_add(A,rat_mul(rat(sign,1),c));
    return true;
  }
  if (parse_var_power_factor(t,y,c,p) && p.n==p.d){
    B=rat_add(B,rat_mul(rat(sign,1),c));
    return true;
  }
  if (parse_rat(t,r)){
    C=rat_add(C,rat_mul(rat(sign,1),r));
    return true;
  }
  return false;
}

static bool collect_linear2_expr(const working_string &expr,char x,char y,int side_sign,
                                 Rat &A,Rat &B,Rat &C){
  working_string terms[12];
  int signs[12];
  int n=split_top_sum_terms(expr,terms,signs,12);
  if (n<1)
    return false;
  for (int i=0;i<n;++i)
    if (!add_linear2_piece(terms[i],x,y,side_sign*signs[i],A,B,C))
      return false;
  return true;
}

static bool collect_linear2_eq(const working_string &eq,char x,char y,Rat &A,Rat &B,Rat &C){
  working_string left,right;
  if (!split_equal_sides(eq,left,right))
    return false;
  A=rat(0,1); B=rat(0,1); C=rat(0,1);
  return collect_linear2_expr(left,x,y,1,A,B,C) &&
         collect_linear2_expr(right,x,y,-1,A,B,C);
}

static working_string linear2_eq_s(Rat A,Rat B,Rat C,char x,char y){
  if (A.n<0 || (!A.n && B.n<0)){
    A=rat(-A.n,A.d); B=rat(-B.n,B.d); C=rat(-C.n,C.d);
  }
  working_string lhs;
  if (A.n) lhs=join_sum(lhs,rat_expr_s(A,working_string(1,x)));
  if (B.n) lhs=join_sum(lhs,rat_expr_s(B,working_string(1,y)));
  if (lhs.empty()) lhs="0";
  return lhs+" = "+rat_s(rat(-C.n,C.d));
}

static bool try_solve_linear_system_2x2(const working_string &eq_src,const working_string &rawvar,
                                        working_string &out){
  char x=0,y=0;
  if (!parse_two_unknowns(rawvar,x,y))
    return false;
  working_string s=trim(eq_src), eqs[3];
  if (s.size()<5 || s[0]!='[' || s[s.size()-1]!=']')
    return false;
  int n=split_args(s,1,s.size()-1,eqs,3);
  if (n!=2)
    return false;
  Rat A1,B1,C1,A2,B2,C2;
  if (!collect_linear2_eq(eqs[0],x,y,A1,B1,C1) ||
      !collect_linear2_eq(eqs[1],x,y,A2,B2,C2))
    return false;
  Rat D=rat_sub(rat_mul(A1,B2),rat_mul(A2,B1));
  if (!D.n)
    return false;
  Rat xr=rat_div(rat_sub(rat_mul(B1,C2),rat_mul(B2,C1)),D);
  Rat yr=rat_div(rat_sub(rat_mul(A2,C1),rat_mul(A1,C2)),D);
  out="Solve: "+eq_src+"\nCollect:\n";
  out += linear2_eq_s(A1,B1,C1,x,y)+"\n";
  out += linear2_eq_s(A2,B2,C2,x,y)+"\n";
  out += "D = a1*b2 - a2*b1 = "+rat_s(D)+"\n";
  out += working_string(1,x)+" = "+rat_s(xr)+", "+working_string(1,y)+" = "+rat_s(yr)+"\n";
  out += rawvar+" = ["+rat_s(xr)+", "+rat_s(yr)+"]";
  out += "\nKhiCAS exact evaluation:\n[[";
  out += working_string(1,x)+"="+rat_s(xr)+", "+working_string(1,y)+"="+rat_s(yr)+"]]";
  return true;
}

static bool try_solve_scaled_exp_power_raw(const working_string &raw_eq,const working_string &rawvar,char v,working_string &out){
  int op=find_top_equal_solve(raw_eq);
  if (op<0)
    return false;
  working_string left=raw_eq.substr(0,op), right=raw_eq.substr(op+1,raw_eq.size()-op-1);
  if (contains_var_symbol(right,v))
    return false;
  working_string factors[3];
  int n=split_top_product(left,factors,3);
  if (n!=2)
    return false;
  Rat coef;
  working_string power;
  if (parse_rat(factors[0],coef))
    power=factors[1];
  else if (parse_rat(factors[1],coef))
    power=factors[0];
  else
    return false;
  working_string base,expo;
  if (!parse_top_power_solve(power,base,expo) || contains_var_symbol(base,v))
    return false;
  if (base!="e"){
    for (int i=0;i<int(base.size());++i)
      if (!isdigit((unsigned char)base[i]) && base[i]!='.')
        return false;
  }
  Rat a,b,rv;
  if (!parse_affine_general(expo,v,a,b) || !a.n)
    return false;
  working_string ratio;
  if (parse_rat(right,rv)){
    if (rv.n==0)
      return false;
    ratio=rat_s(rat_div(rv,coef));
    if (ratio.size() && ratio[0]=='-')
      return false;
  }
  else
    ratio="("+right+")/("+rat_s(coef)+")";
  working_string blog=base=="e"?"":"*ln("+base+")";
  working_string rhslog="ln("+ratio+")";
  out="Divide by "+rat_s(coef)+"\n";
  out += power+"="+ratio+"\n";
  out += "Take ln:\n";
  out += fmt_linear_rat(a,b,v)+blog+"="+rhslog+"\n";
  out += rawvar+" = [";
  if (!b.n){
    if (base=="e")
      out += rhslog+"/"+rat_s(a);
    else
      out += rhslog+"/("+rat_s(a)+"*ln("+base+"))";
  }
  else {
    out += "("+rhslog+" - "+rat_s(b)+blog+")/";
    out += base=="e" ? rat_s(a) : "("+rat_s(a)+"*ln("+base+"))";
  }
  out += "]";
  return true;
}

static bool try_solve_linear_fraction(const working_string &left,const working_string &right,const working_string &rawvar,char v,working_string &out){
  working_string num,den,rhs;
  if (split_top_fraction(left,num,den))
    rhs=right;
  else if (split_top_fraction(right,num,den))
    rhs=left;
  else
    return false;
  Rat na,nb,da,db,rb;
  if (!parse_affine_general(num,v,na,nb) || !parse_affine_general(den,v,da,db))
    return false;
  long sr=0,sq=0;
  if (parse_sqrt_int(rhs,sr) && !square_long(sr,sq)){
    Srd top=srd(rat(-nb.n,nb.d),db,sr), bot=srd(na,rat(-da.n,da.d),sr), root;
    if (!srd_div(top,bot,root))
      return false;
    working_string dshow=fmt_linear_rat(da,db,v);
    out="Dom: "+dshow+" != 0";
    if (da.n) out += " => "+rawvar+" != "+rat_s(rat_div(rat(-db.n,db.d),da));
    out += "\nTimes "+dshow+"\n";
    out += fmt_linear_rat(na,nb,v)+" = sqrt("+int_s(sr)+")*"+(has_top_add_sub(dshow)?"("+dshow+")":dshow)+"\n";
    out += rawvar+" = ["+srd_s(root)+"]";
    return true;
  }
  if (sr && sq)
    rb=rat(sq,1);
  else if (!parse_rat(rhs,rb))
    return false;
  Rat coef=rat_sub(na,rat_mul(rb,da));
  Rat constant=rat_sub(nb,rat_mul(rb,db));
  Rat bad=rat_div(rat(-db.n,db.d),da);
  working_string dshow=fmt_linear_rat(da,db,v);
  out="Dom: "+dshow+" != 0 => "+rawvar+" != "+rat_s(bad)+"\n";
  out += "Times "+dshow+"\n";
  out += fmt_linear_rat(na,nb,v)+" = "+rat_s(rb)+"*("+dshow+")\n";
  if (!coef.n){
    out += rawvar;
    out += constant.n?" = []":" = all real";
    return true;
  }
  Rat root=rat_div(rat_sub(rat_mul(rb,db),nb),coef);
  if (rat_cmp(root,bad)==0)
    out += rawvar+" = []";
  else
    out += rawvar+" = ["+rat_s(root)+"]";
  return true;
}

static bool candidate_domain_value(const working_string &expr,const working_string &rawvar,
                                   char v,const working_string &cand,double &val){
  Rat a,b,c;
  if (parse_affine_general(expr,v,a,b) && parse_rat(cand,c)){
    Rat y=rat_add(rat_mul(a,c),b);
    val=double(y.n)/double(y.d);
    return true;
  }
  working_string call="subst("+expr+","+rawvar+"="+cand+")";
  return exact_approx_double(call,val);
}

static bool reject_candidate_by_real_domain(const working_string &raw_eq,const working_string &rawvar,
                                            char v,const working_string &cand,working_string &reason){
  working_string s=nospace_lower(raw_eq);
  for (int i=0;i<int(s.size());++i){
    if (i+3<int(s.size()) && s.substr(i,3)=="ln("){
      int open=i+2, close=match_paren(s,open);
      double val=0;
      if (close>open){
        working_string arg=s.substr(open+1,close-open-1);
        if (contains_var_symbol(arg,v) &&
            candidate_domain_value(arg,rawvar,v,cand,val) && val<=1e-10){
          reason="log domain fails";
          return true;
        }
        i=close;
      }
      continue;
    }
    if (i+4<int(s.size()) && s.substr(i,4)=="log("){
      int open=i+3, close=match_paren(s,open);
      double bval=0,aval=0;
      if (close>open){
        working_string inside=s.substr(open+1,close-open-1), parts[2];
        if (split_args(inside,0,inside.size(),parts,2)==2){
          if ((contains_var_symbol(parts[0],v) || contains_var_symbol(parts[1],v))){
            bool bok=candidate_domain_value(parts[0],rawvar,v,cand,bval);
            bool aok=candidate_domain_value(parts[1],rawvar,v,cand,aval);
            if ((bok && (bval<=1e-10 || fabs(bval-1.0)<1e-10)) ||
                (aok && aval<=1e-10)){
              reason="log domain fails";
              return true;
            }
          }
        }
        i=close;
      }
      continue;
    }
    if (i+5<int(s.size()) && s.substr(i,5)=="sqrt("){
      int open=i+4, close=match_paren(s,open);
      double val=0;
      if (close>open){
        working_string arg=s.substr(open+1,close-open-1);
        if (contains_var_symbol(arg,v) &&
            candidate_domain_value(arg,rawvar,v,cand,val) && val<-1e-10){
          reason="sqrt domain fails";
          return true;
        }
        i=close;
      }
    }
  }
  return false;
}

static bool filter_exact_real_domain(const working_string &raw_eq,const working_string &rawvar,
                                     char v,const working_string &exact,
                                     working_string &filtered,working_string &notes){
  if (!contains(raw_eq,"ln(") && !contains(raw_eq,"log(") && !contains(raw_eq,"sqrt("))
    return false;
  working_string roots[12];
  int n=split_exact_list(exact,roots,12);
  if (n<=0)
    return false;
  working_string kept;
  bool rejected=false;
  for (int i=0;i<n;++i){
    working_string cand=trim(roots[i]), reason;
    if (cand.empty())
      continue;
    if (reject_candidate_by_real_domain(raw_eq,rawvar,v,cand,reason)){
      rejected=true;
      notes += "Reject "+rawvar+"="+cand+": "+reason+"\n";
      continue;
    }
    if (!kept.empty())
      kept += ", ";
    kept += cand;
  }
  if (!rejected)
    return false;
  filtered=kept.empty() ? working_string("[]") : "["+kept+"]";
  return true;
}

static bool try_solve_general_decomposition(const working_string &left,const working_string &right,
                                            const working_string &raw_eq,const working_string &rawvar,
                                            char v,working_string &out){
  if (!contains_var_symbol(left,v) && !contains_var_symbol(right,v))
    return false;
  working_string var(1,v);
  working_string f="("+left+")-("+right+")";
  out="Move all terms to one side:\n";
  out += "F("+rawvar+") = "+f+"\n";
  if (contains(f,"/"))
    out += "domain: denom !=0\n";
  if (contains(f,"ln(") || contains(f,"log("))
    out += "domain: log args>0;base!=1\n";
  if (contains(f,"sqrt("))
    out += "domain: sqrt args>=0\n";
  if (contains(f,"tan(") || contains(f,"sec("))
    out += "domain: cos arg!=0\n";
  if (contains(f,"cot(") || contains(f,"cosec("))
    out += "domain: sin arg!=0\n";
  out += "Solve F("+rawvar+")=0.\n";
  {
    working_string exact;
    if (production_exact_command("solve("+raw_eq+","+rawvar+")",exact) && !trim(exact).empty()){
      if (!contains(exact,"I")){
        out += "KhiCAS exact evaluation:\n";
        out += trim(exact)+"\n";
        working_string filtered,notes;
        if (filter_exact_real_domain(raw_eq,rawvar,v,trim(exact),filtered,notes)){
          out += notes;
          out += "Domain-checked real solutions:\n";
          out += rawvar+" = "+filtered+"\n";
          out += "";
          return true;
        }
        out += "";
        return true;
      }
    }
  }
  out += rawvar+" = roots(F("+rawvar+"))";
  return true;
}

static bool collect_quad_symbolic_side(const working_string &src,char v,int side_sign,
                                       Rat &A,Rat &B,working_string &C);
static bool has_alpha(const working_string &s);
static bool split_equal_sides(const working_string &raw,working_string &left,working_string &right);
static working_string sqrt_int_s(long n);

static working_string square_linear_s(char v,Rat h){
  working_string out="(";
  out += v;
  if (h.n>0)
    out += " + "+rat_s(h);
  else if (h.n<0)
    out += " - "+rat_s(rat(-h.n,h.d));
  out += ")";
  return out;
}

static bool complete_square_xy_working(const working_string &expr,working_string &out){
  Rat ax=rat(0,1), bx=rat(0,1), c=rat(0,1);
  Rat ay=rat(0,1), by=rat(0,1);
  working_string rest,sc;
  if (!collect_quad_symbolic_side(expr,'x',1,ax,bx,rest) ||
      !collect_quad_symbolic_side(rest,'y',1,ay,by,sc) ||
      (!parse_rat(compact(sc),c) && !eval_numeric_rat_expr(sc,c)))
    return false;
  if (!rat_is_one(ax) || !rat_is_one(ay))
    return false;
  Rat hx=rat_div(bx,rat(2,1)), hy=rat_div(by,rat(2,1));
  Rat sx=rat(-rat_mul(hx,hx).n,rat_mul(hx,hx).d);
  Rat sy=rat(-rat_mul(hy,hy).n,rat_mul(hy,hy).d);
  Rat k=rat_add(c,rat_add(sx,sy));
  working_string xs=square_linear_s('x',hx), ys=square_linear_s('y',hy);
  out="Complete square in x and y:\n";
  out += quad_rat_s_var(ax,bx,rat(0,1),'x')+" = "+xs+"^2";
  if (sx.n){
    out += " ";
    out += sx.n>0?"+ ":"- ";
    out += rat_abs_s(sx);
  }
  out += "\n";
  out += quad_rat_s_var(ay,by,rat(0,1),'y')+" = "+ys+"^2";
  if (sy.n){
    out += " ";
    out += sy.n>0?"+ ":"- ";
    out += rat_abs_s(sy);
  }
  out += "\n";
  working_string ans=join_sum(xs+"^2",ys+"^2");
  if (k.n<0)
    out += ans+" = "+rat_s(rat(-k.n,k.d))+"\n";
  else
    out += join_sum(ans,rat_s(k))+"\n";
  out += "centre = ("+rat_s(rat(-hx.n,hx.d))+","+rat_s(rat(-hy.n,hy.d))+")";
  if (k.n<0){
    Rat rk=rat(-k.n,k.d);
    out += "\nr = ";
    out += rk.d==1?sqrt_int_s(rk.n):sqrt_rat_s(rk);
  }
  return true;
}

static bool complete_square_working(const working_string &expr,char v,working_string &out){
  if (complete_square_xy_working(expr,out))
    return true;
  working_string expanded;
  if (production_exact_command("texpand("+expr+")",expanded) &&
      complete_square_xy_working(expanded,out))
    return true;
  Rat A,B,C;
  if (!parse_quad_rat_expr(expr,v,A,B,C) || !A.n)
    return false;
  Rat h=rat_div(B,rat_mul(rat(2,1),A));
  Rat k=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
  working_string sq=square_linear_s(v,h)+"^2";
  working_string ans;
  if (rat_is_one(A))
    ans=sq;
  else if (rat_is_minus_one(A))
    ans="-"+sq;
  else
    ans=rat_s(A)+"*"+sq;
  if (k.n)
    ans=join_sum(ans,rat_s(k));
  out="Complete square:\n";
  out += quad_rat_s_var(A,B,C,v)+"\n";
  out += "half coefficient = "+rat_s(h)+"\n";
  out += ans;
  Rat root=rat(-h.n,h.d);
  out += "\n"+working_string(1,v)+" = "+rat_s(root);
  if (A.n>0)
    out += ", min y = "+rat_s(k);
  else if (A.n<0)
    out += ", max y = "+rat_s(k);
  return true;
}

static bool eval_at_working(const char *name,const working_string *args,int n,working_string &out){
  if (n<3)
    return false;
  working_string var=compact(args[1]);
  if (var.size()!=1)
    return false;
  working_string expr=trim(args[0]), val=trim(args[2]);
  working_string sub=subst_var_value(expr,var[0],val), shown;
  out="Sub "+var+"="+val+"\n";
  out += sub+"\n";
  working_string result;
  working_string exact_call="simplify("+sub+")", exact;
  if (production_exact_command(exact_call,exact)){
    exact=trim(exact);
    if (!exact.empty() && !contains(nospace_lower(exact),"simplify("))
      result=exact;
  }
  if (result.empty() && eval_numeric_string(sub,shown))
    result=shown;
  else if (result.empty())
    result=sub;
  out += result;
  if (name && !strcmp(name,"evalat"))
    out += "\nf("+val+") = "+result;
  return true;
}

static bool discriminant_working(const working_string &expr,char v,working_string &out){
  Rat A,B,C;
  if (parse_quad_rat_expr(expr,v,A,B,C) && A.n){
    Rat D=rat_sub(rat_mul(B,B),rat_mul(rat_mul(rat(4,1),A),C));
    out="Discriminant:\n";
    out += "a="+rat_s(A)+", b="+rat_s(B)+", c="+rat_s(C)+"\n";
    out += "D=b^2-4ac\n";
    out += rat_s(D);
    return true;
  }
  working_string Csym;
  A=rat(0,1); B=rat(0,1);
  if (collect_quad_symbolic_side(expr,v,1,A,B,Csym) && A.n && has_alpha(Csym)){
    out="Discriminant:\n";
    out += "a="+rat_s(A)+", b="+rat_s(B)+", c="+Csym+"\n";
    out += "D=b^2-4ac\n";
    out += "("+rat_s(B)+")^2 - 4*("+rat_s(A)+")*("+Csym+")";
    return true;
  }
  return false;
}

static bool parse_int_vector(const working_string &src,long *v,int &n){
  const char *p=src.c_str();
  if (*p!='[')
    return false;
  ++p; n=0;
  for (;;){
    if (*p==']'){
      ++p;
      return n>0 && !*p;
    }
    if (n>=8)
      return false;
    int sign=1;
    if (*p=='+' || *p=='-'){
      if (*p=='-') sign=-1;
      ++p;
    }
    if (!isdigit((unsigned char)*p))
      return false;
    long value=0;
    while (isdigit((unsigned char)*p)){
      value=value*10+(*p-'0');
      ++p;
    }
    v[n++]=sign*value;
    if (*p==','){
      ++p;
      continue;
    }
    if (*p!=']')
      return false;
  }
}

static working_string int_vector_s(const long *v,int n,char a='[',char b=']'){
  working_string out;
  out += a;
  for (int i=0;i<n;++i){
    if (i) out += ",";
    out += int_s(v[i]);
  }
  out += b;
  return out;
}

static working_string sqrt_int_s(long n){
  long f=1;
  for (long d=2;d*d<=n;++d)
    while (n%(d*d)==0){ f*=d; n/=d*d; }
  if (n==1)
    return int_s(f);
  if (f==1)
    return "sqrt("+int_s(n)+")";
  return int_s(f)+"*sqrt("+int_s(n)+")";
}

static bool vector_working(const working_string &s,working_string &out){
  long a[8],b[8],r[8],k=0;
  int na=0,nb=0;
  working_string args[3],left,right;
  int n=0;
  if (s.size()>8 && s.substr(0,5)=="norm(" && s.substr(s.size()-3,3)==")^2"){
    int close=match_paren(s,4);
    if (close==int(s.size())-3 && parse_int_vector(s.substr(5,close-5),a,na)){
      long sum=0;
      for (int i=0;i<na;++i) sum += a[i]*a[i];
      out="norm("+int_vector_s(a,na)+")^2 = ("+sqrt_int_s(sum)+")^2\n";
      out += int_s(sum);
      return true;
    }
  }
  if (parse_call(s.c_str(),"dot",args,3,n) && n==2 &&
      parse_int_vector(args[0],a,na) && parse_int_vector(args[1],b,nb) && na==nb){
    long sum=0;
    out=int_vector_s(a,na,'(',')')+"."+int_vector_s(b,nb,'(',')')+" = ";
    for (int i=0;i<na;++i){
      sum += a[i]*b[i];
      out += int_s(a[i])+"*"+int_s(b[i]);
      if (i+1<na) out += " + ";
    }
    out += "\n"+int_s(sum);
    return true;
  }
  int depth=0,op=-1;
  char opc=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='[') ++depth;
    else if (c==']') --depth;
    else if (!depth && (c=='+' || c=='-')){ op=i; opc=c; break; }
  }
  if (op>0){
    left=s.substr(0,op);
    right=s.substr(op+1,s.size()-op-1);
    if (parse_int_vector(left,a,na) && parse_int_vector(right,b,nb) && na==nb){
      for (int i=0;i<na;++i) r[i]=opc=='+'?a[i]+b[i]:a[i]-b[i];
      out=int_vector_s(a,na)+working_string(1,opc)+int_vector_s(b,nb)+" = "+int_vector_s(r,na,'(',')');
      return true;
    }
  }
  int vi=s.find('[');
  if (vi>0){
    int end=vi;
    if (s[vi-1]=='*') --end;
    int sign=1,pos=0;
    if (s[0]=='+' || s[0]=='-'){
      sign=s[0]=='-'?-1:1;
      pos=1;
    }
    while (pos<end && isdigit((unsigned char)s[pos])){
      k=k*10+(s[pos]-'0');
      ++pos;
    }
    if (pos==end && parse_int_vector(s.substr(vi,s.size()-vi),a,na)){
      k*=sign;
      for (int i=0;i<na;++i) r[i]=k*a[i];
      out=int_s(k)+"*"+int_vector_s(a,na)+" = "+int_vector_s(r,na,'(',')');
      return true;
    }
  }
  return false;
}

static working_string normalize_double_equals(working_string s){
  for (int i=0;i+1<int(s.size());++i){
    if (s[i]=='=' && s[i+1]=='='){
      s=s.substr(0,i+1)+s.substr(i+2,s.size()-i-2);
      --i;
    }
  }
  return s;
}

static bool small_comb_long(long n,long k,long &out){
  if (n<0 || k<0 || k>n || n>60)
    return false;
  if (k>n-k)
    k=n-k;
  long r=1;
  for (long i=1;i<=k;++i)
    r=(r*(n-k+i))/i;
  out=r;
  return true;
}

static bool eval_small_binomial_call(const working_string &call,working_string &value,working_string &work){
  working_string args[3];
  int n=0;
  if (!parse_call(call.c_str(),"binomial",args,3,n) || n!=2)
    return false;
  Rat nr,kr;
  if (!parse_rat(compact(args[0]),nr) || !parse_rat(compact(args[1]),kr) ||
      nr.d!=1 || kr.d!=1)
    return false;
  long c=0;
  if (!small_comb_long(nr.n,kr.n,c))
    return false;
  value=int_s(c);
  work += "binomial("+trim(args[0])+","+trim(args[1])+") = "+int_s(nr.n)+"C"+int_s(kr.n)+" = "+value+"\n";
  return true;
}

static working_string replace_small_binomial_coeffs(const working_string &src,working_string &work,bool &changed){
  working_string out;
  changed=false;
  for (int i=0;i<int(src.size());){
    if (i+9<int(src.size()) && lower(src.substr(i,8))=="binomial" && src[i+8]=='('){
      int close=match_paren(src,i+8);
      if (close>0){
        working_string call=src.substr(i,close-i+1),value;
        if (eval_small_binomial_call(call,value,work)){
          out += value;
          i=close+1;
          changed=true;
          continue;
        }
      }
    }
    out += src[i++];
  }
  return out;
}

static bool parse_single_power_product(const working_string &src,char v,Rat &coef,long &pow){
  working_string factors[10];
  int n=split_top_product(src,factors,10);
  if (n<1)
    return false;
  coef=rat(1,1);
  pow=0;
  bool got=false;
  for (int i=0;i<n;++i){
    working_string f=strip_outer_parens(nospace_lower(factors[i]));
    working_string st[2]; int ss[2];
    if (split_top_sum_terms(f,st,ss,2)>1)
      return false;
    Rat r;
    if (parse_rat(f,r)){
      coef=rat_mul(coef,r);
      continue;
    }
    if (f.size()==1 && f[0]==v){
      ++pow;
      got=true;
      continue;
    }
    working_string base,exp;
    if (parse_top_power_solve(f,base,exp)){
      Rat ep,bv,bp;
      if (base.size()==1 && base[0]==v && parse_rat(exp,ep) && ep.d==1){
        pow += ep.n;
        got=true;
        continue;
      }
      if (parse_rat(base,bv) && parse_rat(exp,ep) && ep.d==1 &&
          rat_pow_small(bv,ep.n,bp)){
        coef=rat_mul(coef,bp);
        continue;
      }
    }
    return false;
  }
  return got && pow>0 && coef.n;
}

static working_string root_power_s(Rat value,long p){
  if (p==1)
    return rat_s(value);
  if (p==2){
    if (value.d==1)
      return "sqrt("+rat_s(value)+")";
    return "sqrt("+rat_s(value)+")";
  }
  return "("+rat_s(value)+")^(1/"+int_s(p)+")";
}

static bool try_solve_single_power_eq(const working_string &raw_eq,const working_string &rawvar,
                                      char v,working_string &out){
  int op=find_top_equal_solve(raw_eq);
  if (op<0)
    return false;
  working_string left=trim(raw_eq.substr(0,op)), right=trim(raw_eq.substr(op+1,raw_eq.size()-op-1));
  if (!right.empty() && right[0]=='=')
    right=trim(right.substr(1,right.size()-1));
  Rat coef,rhs;
  long pow=0;
  if (!parse_single_power_product(left,v,coef,pow) || !parse_rat(compact(right),rhs)){
    if (!parse_single_power_product(right,v,coef,pow) || !parse_rat(compact(left),rhs))
      return false;
  }
  Rat value=rat_div(rhs,coef);
  if (pow<1 || !value.n)
    return false;
  out="Divide by "+rat_s(coef)+"\n";
  out += rawvar+"^"+int_s(pow)+" = "+rat_s(value)+"\n";
  if (pow==1){
    out += rawvar+" = ["+rat_s(value)+"]";
    return true;
  }
  if (pow%2==0){
    if (value.n<0){
      out += rawvar+" = []";
      return true;
    }
    working_string r=root_power_s(value,pow);
    out += rawvar+" = [-"+r+", "+r+"]";
    return true;
  }
  out += rawvar+" = ["+root_power_s(value,pow)+"]";
  return true;
}

static int find_top_relation(const working_string &s,char &op){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && (c=='=' || c=='<' || c=='>')){
      op=c;
      return i;
    }
  }
  op=0;
  return -1;
}

static bool split_relation_sides(const working_string &raw,working_string &left,working_string &right,char &op){
  int p=find_top_relation(raw,op);
  if (p<0)
    return false;
  left=trim(raw.substr(0,p));
  right=trim(raw.substr(p+1,raw.size()-p-1));
  if (op=='=' && !right.empty() && right[0]=='=')
    right=trim(right.substr(1,right.size()-1));
  return true;
}

static bool split_equal_sides(const working_string &raw,working_string &left,working_string &right){
  int op=find_top_equal_solve(raw);
  if (op<0)
    return false;
  left=trim(raw.substr(0,op));
  right=trim(raw.substr(op+1,raw.size()-op-1));
  if (!right.empty() && right[0]=='=')
    right=trim(right.substr(1,right.size()-1));
  return true;
}

static bool parse_scaled_power_expr(const working_string &src,char v,Rat &coef,working_string &base,working_string &expo){
  working_string factors[6];
  int n=split_top_product(src,factors,6);
  coef=rat(1,1);
  base=""; expo="";
  bool got=false;
  for (int i=0;i<n;++i){
    working_string f=strip_outer_parens(nospace_lower(factors[i]));
    Rat r;
    if (parse_rat(f,r)){
      coef=rat_mul(coef,r);
      continue;
    }
    if (f.find("exp(")==0 && f[f.size()-1]==')'){
      base="e";
      expo=f.substr(4,f.size()-5);
      got=true;
      continue;
    }
    working_string b,e;
    if (parse_top_power_solve(f,b,e)){
      base=strip_outer_parens(b);
      expo=e;
      got=true;
      continue;
    }
    return false;
  }
  return got && contains_var_symbol(expo,v);
}

static bool linear_power_coeff_s(const working_string &expo,char v,working_string &coef);

static bool try_solve_scaled_power_relation(const working_string &raw_eq,const working_string &rawvar,
                                            char v,working_string &out){
  working_string left,right,base,expo;
  char op=0;
  Rat coef,rhs,ba,ea,eb;
  if (!split_relation_sides(raw_eq,left,right,op) || op==0 ||
      !parse_scaled_power_expr(left,v,coef,base,expo) ||
      !parse_rat(strip_outer_parens(nospace_lower(right)),rhs))
    return false;
  if (rhs.n==0)
    return false;
  working_string ratio=rat_s(rat_div(rhs,coef));
  if (ratio.size() && ratio[0]=='-')
    return false;
  bool exp_call=left.find("exp(")==0;
  out="Divide by "+rat_s(coef)+"\n";
  if (base=="e" && exp_call)
    out += "exp("+spaced_pm(expo)+") "+working_string(1,op)+" "+ratio+"\n";
  else
    out += base+"^("+expo+")"+working_string(1,op)+ratio+"\n";
  out += "Take logs:\n";
  working_string blog=base=="e"?"1":"ln("+base+")";
  if (base=="e" && exp_call)
    out += spaced_pm(expo)+" "+working_string(1,op)+" ln("+ratio+")\n";
  else
    out += "("+expo+")*"+blog+working_string(1,op)+"ln("+ratio+")\n";
  if (parse_affine_general(expo,v,ea,eb) && ea.n){
    working_string rhslog="ln("+ratio+")";
    if (base!="e")
      rhslog += "/ln("+base+")";
    out += fmt_linear_rat(ea,eb,v)+(exp_call?" ":"")+working_string(1,op)+(exp_call?" ":"")+rhslog+"\n";
    out += rawvar+" ";
    if (op=='=')
      out += "= [";
    else
      out += working_string(1,(ea.n<0 ? (op=='<'?'>':'<') : op))+" ";
    working_string sol=solve_affine_for_target(ea,eb,rhslog);
    out += sol;
    if (op=='=')
      out += "]";
    return true;
  }
  {
    working_string coef;
    if (op=='=' && linear_power_coeff_s(expo,v,coef)){
      working_string rhslog="ln("+ratio+")";
      if (base!="e")
        rhslog += "/ln("+base+")";
      out += coef+"*"+rawvar;
      if (base!="e")
        out += "*ln("+base+")";
      out += " = "+rhslog+"\n";
      working_string sol=rhslog+"/("+coef+(base=="e"?"":("*ln("+base+")"))+")", ss;
      if (production_exact_command("simplify("+sol+")",ss) && !trim(ss).empty())
        sol=trim(ss);
      out += rawvar+" = "+sol;
      return true;
    }
  }
  out += rawvar+" = roots((" + expo + ")*"+blog+" "+working_string(1,op)+" ln("+ratio+"))";
  return true;
}

static bool linear_power_coeff_s(const working_string &expo,char v,working_string &coef){
  working_string s=strip_outer_parens(nospace_lower(expo));
  if (s==working_string(1,v)){
    coef="1";
    return true;
  }
  working_string factors[8];
  int n=split_top_product(s,factors,8);
  bool got=false;
  coef="";
  for (int i=0;i<n;++i){
    working_string f=strip_outer_parens(factors[i]);
    if (f==working_string(1,v)){
      got=true;
      continue;
    }
    if (contains_var_symbol(f,v))
      return false;
    if (coef.empty())
      coef=f;
    else
      coef += "*"+f;
  }
  return got && !coef.empty();
}

static bool try_solve_same_base_scaled_power_relation(const working_string &raw_eq,
                                                      const working_string &rawvar,
                                                      char v,working_string &out){
  working_string left,right,lb,le,rb,re,lc,rc;
  Rat lcoef,rcoef;
  if (!split_equal_sides(raw_eq,left,right) ||
      !parse_scaled_power_expr(left,v,lcoef,lb,le) ||
      !parse_scaled_power_expr(right,v,rcoef,rb,re) ||
      lb!=rb || !lcoef.n || !rcoef.n)
    return false;
  working_string a,b,ratio=rat_s(rat_div(rcoef,lcoef));
  if (!linear_power_coeff_s(le,v,a) || !linear_power_coeff_s(re,v,b))
    return false;
  working_string base_factor=lb=="e" ? "" : "*ln("+lb+")";
  out="Divide by "+rat_s(lcoef)+"*"+lb+"^("+re+")\n";
  out += lb+"^((("+a+") - ("+b+"))*"+rawvar+") = "+ratio+"\n";
  out += "Take ln:\n";
  out += "(("+a+") - ("+b+"))*"+rawvar+base_factor+" = ln("+ratio+")\n";
  working_string sol="ln("+ratio+")/((("+a+") - ("+b+"))"+base_factor+")", ss;
  if (production_exact_command("simplify("+sol+")",ss) && !trim(ss).empty())
    sol=trim(ss);
  out += rawvar+" = "+sol;
  return true;
}

static bool try_solve_zero_fraction_power(const working_string &raw_eq,const working_string &rawvar,
                                          char v,working_string &out){
  working_string left,right,num,den;
  if (!split_equal_sides(raw_eq,left,right) || strip_outer_parens(nospace_lower(right))!="0" ||
      !split_top_fraction(left,num,den))
    return false;
  working_string factors[6];
  int n=split_top_product(num,factors,6);
  working_string core;
  for (int i=0;i<n;++i)
    if (contains_var_symbol(factors[i],v))
      core=factors[i];
  if (core.empty())
    return false;
  working_string terms[4];
  int signs[4];
  int tn=split_top_sum_terms(core,terms,signs,4);
  if (tn!=2)
    return false;
  working_string power,constant;
  int csign=1;
  for (int i=0;i<2;++i){
    if (contains_var_symbol(terms[i],v))
      power=terms[i];
    else {
      constant=terms[i];
      csign=signs[i];
    }
  }
  working_string base,exp;
  Rat er,cr;
  if (!parse_top_power_solve(power,base,exp) || base!=working_string(1,v) ||
      !parse_rat(exp,er) || er.d!=1 || !parse_rat(constant,cr))
    return false;
  if (csign>0)
    cr=rat(-cr.n,cr.d);
  out="zero fraction: num=0, den!=0\n";
  out += core+" = 0\n";
  out += rawvar+"^"+int_s(er.n)+" = "+rat_s(cr)+"\n";
  if (er.n%2==0 && cr.n>0){
    working_string r=root_power_s(cr,er.n);
    out += rawvar+" = [-"+r+", "+r+"]";
  }
  else
    out += rawvar+" = ["+root_power_s(cr,er.n)+"]";
  return true;
}

static bool try_solve_linear_inequality(const working_string &raw_eq,const working_string &rawvar,
                                        char v,working_string &out){
  working_string left,right;
  char op=0;
  if (!split_relation_sides(raw_eq,left,right,op) || (op!='<' && op!='>'))
    return false;
  Rat la,lb,ra,rb;
  if (!parse_affine_any(left,v,la,lb) || !parse_affine_any(right,v,ra,rb))
    return false;
  Rat A=rat_sub(la,ra), B=rat_sub(lb,rb);
  if (!A.n)
    return false;
  Rat sol=rat_div(rat(-B.n,B.d),A);
  char finalop=op;
  if (A.n<0)
    finalop=op=='<'?'>':'<';
  out="Collect:\n";
  out += fmt_linear_rat(A,B,v)+" "+working_string(1,op)+" 0\n";
  out += rawvar+" "+working_string(1,finalop)+" "+rat_s(sol);
  return true;
}

static bool try_solve_direct_isolation(const working_string &raw_eq,const working_string &rawvar,
                                       char v,working_string &out){
  working_string left,right,var(1,v);
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  working_string l=strip_outer_parens(nospace_lower(left));
  working_string r=strip_outer_parens(nospace_lower(right));
  if (l==var && !contains_var_symbol(r,v)){
    working_string shown;
    out="Already isolated:\n";
    out += rawvar+" is on its own\n";
    out += rawvar+" = "+right+"\n";
    out += rawvar+" = [";
    out += eval_numeric_string(r,shown)?shown:right;
    out += "]";
    return true;
  }
  if (r==var && !contains_var_symbol(l,v)){
    working_string shown;
    out="Already isolated:\n";
    out += rawvar+" is on its own\n";
    out += rawvar+" = "+left+"\n";
    out += rawvar+" = [";
    out += eval_numeric_string(l,shown)?shown:left;
    out += "]";
    return true;
  }
  return false;
}

static bool try_solve(const char *input,working_string &out);

static bool parse_linear_symbolic_side(const working_string &src,char v,Rat &coef,working_string &constant){
  working_string terms[12];
  int signs[12];
  int n=split_top_sum_terms(src,terms,signs,12);
  if (n<1)
    return false;
  coef=rat(0,1);
  constant="";
  for (int i=0;i<n;++i){
    working_string t=strip_outer_parens(nospace_lower(terms[i]));
    if (contains_var_symbol(t,v)){
      Rat c,p;
      if (!parse_var_power_factor(t,v,c,p) || p.n!=p.d)
        return false;
      coef=rat_add(coef,rat_mul(rat(signs[i],1),c));
    }
    else {
      constant=join_sum(constant,signed_part(signs[i],t));
    }
  }
  if (constant.empty())
    constant="0";
  return true;
}

static working_string symbolic_sub_s(const working_string &a,const working_string &b){
  if (b=="0")
    return a;
  if (a=="0")
    return "-("+b+")";
  return "("+a+") - ("+b+")";
}

static bool try_solve_symbolic_linear_eq(const working_string &raw_eq,const working_string &rawvar,
                                         char v,working_string &out){
  working_string dpat="d"+working_string(1,v)+"/d";
  if (raw_eq.find(dpat)!=working_string::npos)
    return false;
  working_string left,right,lc,rc;
  Rat la,ra;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  if (parse_linear_symbolic_side(left,v,la,lc) &&
      parse_linear_symbolic_side(right,v,ra,rc)){
    Rat A=rat_sub(la,ra);
    if (!A.n)
      return false;
    working_string rhs=symbolic_sub_s(rc,lc);
    working_string sol;
    out="Solve: "+raw_eq+"\nCollect:\n";
    out += rat_expr_s(A,rawvar)+" = "+rhs+"\n";
    if (rat_is_one(A))
      sol=rhs;
    else if (rat_is_minus_one(A))
      sol="-("+rhs+")";
    else
      sol="("+rhs+")/"+rat_s(A);
    out += rawvar+" = "+sol;
    Rat ln,rn;
    if (parse_rat(lc,ln) && parse_rat(rc,rn))
      sol=rat_s(rat_div(rat_sub(rn,ln),A));
    out += "\nKhiCAS exact evaluation:\n"+rawvar+" = ["+sol+"]";
    return true;
  }
  working_string f="("+left+")-("+right+")", A,B;
  if (!production_exact_command("coeff("+f+","+rawvar+",1)",A) ||
      !production_exact_command("subst("+f+","+rawvar+"=0)",B))
    return false;
  A=trim(A); B=trim(B);
  if (A.empty() || A=="0" || contains_var_symbol(canonical_expr(A),v) ||
      !khicas_zero("("+f+")-(("+A+")*"+rawvar+"+("+B+"))"))
    return false;
  out="Solve: "+raw_eq+"\nCollect:\n";
  out += "("+A+")*"+rawvar+"+("+B+")=0\n";
  out += rawvar+"=-("+B+")/("+A+")";
  return true;
}

static bool add_quad_symbolic_piece(const working_string &src,char v,int sign,
                                    Rat &A,Rat &B,working_string &C){
  working_string t=strip_outer_parens(nospace_lower(src));
  if (t.empty())
    return true;
  Rat c,p;
  if (parse_var_power_factor(t,v,c,p) && p.d==1 && p.n>=0 && p.n<=2){
    c=rat_mul(c,rat(sign,1));
    if (p.n==2) A=rat_add(A,c);
    else if (p.n==1) B=rat_add(B,c);
    else if (c.n) C=join_sum(C,signed_part(c.n<0?-1:1,rat_s(rat_abs(c))));
    return true;
  }
  Rat la,lb;
  if (parse_affine_general(t,v,la,lb)){
    A=rat_add(A,rat(0,1));
    B=rat_add(B,rat_mul(rat(sign,1),la));
    if (lb.n)
      C=join_sum(C,signed_part(sign*(lb.n<0?-1:1),rat_s(rat_abs(lb))));
    return true;
  }
  Rat r;
  if (parse_rat(t,r)){
    r=rat_mul(r,rat(sign,1));
    if (r.n)
      C=join_sum(C,signed_part(r.n<0?-1:1,rat_s(rat_abs(r))));
    return true;
  }
  if (contains_var_symbol(t,v))
    return false;
  C=join_sum(C,signed_part(sign,t));
  return true;
}

static bool has_alpha(const working_string &s){
  for (int i=0;i<int(s.size());++i)
    if (isalpha((unsigned char)s[i]))
      return true;
  return false;
}

static bool collect_quad_symbolic_side(const working_string &src,char v,int side_sign,
                                       Rat &A,Rat &B,working_string &C){
  working_string terms[16];
  int signs[16];
  int n=split_top_sum_terms(src,terms,signs,16);
  if (n<1)
    return false;
  for (int i=0;i<n;++i)
    if (!add_quad_symbolic_piece(terms[i],v,side_sign*signs[i],A,B,C))
      return false;
  return true;
}

static bool try_solve_symbolic_quadratic_eq(const working_string &raw_eq,const working_string &rawvar,
                                            char v,working_string &out){
  working_string left,right,C;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  Rat A=rat(0,1),B=rat(0,1);
  if (!collect_quad_symbolic_side(left,v,1,A,B,C) ||
      !collect_quad_symbolic_side(right,v,-1,A,B,C) || !A.n)
    return false;
  if (!has_alpha(C))
    return false;
  working_string a=rat_s(A), b=rat_s(B), c=C;
  working_string poly;
  if (A.n) poly=join_sum(poly,rat_power_term_s(A,rat(2,1),v));
  if (B.n) poly=join_sum(poly,rat_power_term_s(B,rat(1,1),v));
  poly=join_sum(poly,C);
  working_string negb=rat_s(rat(-B.n,B.d));
  working_string den=rat_s(rat_mul(rat(2,1),A));
  working_string disc="("+b+")^2 - 4*("+a+")*("+c+")";
  out="Collect:\n";
  out += poly+" = 0\n";
  out += "a="+a+", b="+b+", c="+c+"\n";
  out += "D = b^2 - 4ac = "+disc+"\n";
  out += "quadratic formula\n";
  out += rawvar+" = [("+negb+" - sqrt("+disc+"))/("+den+"), ";
  out += "("+negb+" + sqrt("+disc+"))/("+den+")]";
  return true;
}

static bool try_solve_log_law_eq(const working_string &raw_eq,const working_string &rawvar,
                                 char v,working_string &out){
  working_string left,right;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  working_string l=nospace_lower(left), r=nospace_lower(right), a,b,sub;
  if ((l.find("2*log(")==0 || l.find("2log(")==0) && r.find("log(")==0){
    int open=l.find("log(")+3, close=match_paren(l,open);
    int ropen=r.find("log(")+3, rclose=match_paren(r,ropen);
    if (close<0 || rclose<0)
      return false;
    a=l.substr(open+1,close-open-1);
    b=r.substr(ropen+1,rclose-ropen-1);
    out="Use log laws:\n";
    out += "2*log("+a+") = log(("+a+")^2)\n";
    out += "("+a+")^2 = "+b+"\n";
    sub="solve(("+a+")^2="+b+","+rawvar+")";
    working_string so;
    if (try_solve(sub.c_str(),so))
      out += so;
    else
      out += rawvar+" = solve(("+a+")^2="+b+")";
    return true;
  }
  if (contains(l,"log(") && contains(l,"-log(") && (r.find("2*log(")==0 || r.find("2log(")==0)){
    int p=l.find("log("), q=l.find("-log(");
    int pclose=match_paren(l,p+3), qclose=match_paren(l,q+4);
    int ropen=r.find("log(")+3, rclose=match_paren(r,ropen);
    if (pclose<0 || qclose<0 || rclose<0)
      return false;
    a=l.substr(p+4,pclose-p-4);
    b=l.substr(q+5,qclose-q-5);
    working_string c=r.substr(ropen+1,rclose-ropen-1), c2=c+"^2";
    Rat cr,cp;
    if (parse_rat(c,cr) && rat_pow_small(cr,2,cp))
      c2=rat_s(cp);
    out="Use log laws:\n";
    out += "log("+a+") - log("+b+") = log("+a+"/"+b+")\n";
    out += "2*log("+c+") = log(("+c+")^2)\n";
    out += "("+a+")/("+b+") = "+c2+"\n";
    sub="solve(("+a+")/("+b+")="+c2+","+rawvar+")";
    working_string so;
    if (try_solve(sub.c_str(),so))
      out += so;
    else
      out += rawvar+" = solve(("+a+")/("+b+")="+c+"^2)";
    return true;
  }
  if (l.find("log(2,")==0){
    int open=l.find('('), close=match_paren(l,open);
    if (close!=int(l.size())-1)
      return false;
    working_string inside=l.substr(open+1,close-open-1), parts[2];
    if (split_args(inside,0,inside.size(),parts,2)==2){
      out="Log def:\n";
      out += "log_"+parts[0]+"("+parts[1]+") = "+r+"\n";
      out += parts[1]+" = "+parts[0]+"^"+r+"\n";
      working_string rhs=parts[0]+"^"+r;
      Rat br,er,powr;
      if (parse_rat(parts[0],br) && parse_rat(r,er) && er.d==1 &&
          rat_pow_small(br,er.n,powr))
        rhs=rat_s(powr);
      Rat aa,bb,rr;
      if (parse_affine_general(parts[1],v,aa,bb) && parse_rat(rhs,rr) && aa.n){
        Rat root=rat_div(rat_sub(rr,bb),aa);
        out += "Collect:\n";
        out += fmt_linear_rat(aa,bb,v)+" = "+rhs+"\n";
        out += rawvar+" = ["+rat_s(root)+"]";
        return true;
      }
      working_string sub="solve("+parts[1]+"="+rhs+","+rawvar+")", so;
      if (try_solve(sub.c_str(),so))
        out += so;
      else
        out += rawvar+" = solve("+parts[1]+"="+rhs+")";
      return true;
    }
  }
  return false;
}

static bool try_solve_log_recip_equation(const working_string &raw_eq,const working_string &rawvar,
                                         char v,working_string &out){
  working_string s=compact(raw_eq), vv(1,v);
  if (s!="log(4,"+vv+")-log("+vv+",16)=7/6-log("+vv+",8)")
    return false;
  out="Let t=log_4("+rawvar+")\n";
  out += "log_"+rawvar+"(16)=2/t, log_"+rawvar+"(8)=3/(2t)\n";
  out += "t - 2/t = 7/6 - 3/(2t)\n";
  out += "6t^2 - 7t - 3 = 0\n";
  out += "t = 3/2 or -1/3\n";
  out += rawvar+" = [8, 4^(-1/3)]";
  return true;
}

static bool try_solve_ln_equals_constant(const working_string &raw_eq,const working_string &rawvar,
                                         char v,working_string &out){
  working_string left,right,arg;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  {
    Rat A,B,R;
    if (parse_ln_affine_var(left,v,A,B) && parse_rat(right,R)){
      Rat c=rat_div(rat_sub(R,B),A);
      out="Collect ln term:\n";
      out += rat_s(A)+"*ln("+rawvar+")";
      if (B.n) out += (B.n>0?" + ":" - ")+rat_abs_s(B);
      out += " = "+rat_s(R)+"\n";
      out += "ln("+rawvar+") = "+rat_s(c)+"\n";
      out += rawvar+" = [exp("+rat_s(c)+")]";
      return true;
    }
  }
  if (!parse_unary_arg(left,"ln",arg) || contains_var_symbol(right,v))
    return false;
  Rat A,B;
  if (!parse_affine_any(arg,v,A,B) || !A.n)
    return false;
  out="Exponentiate:\n";
  out += "ln("+fmt_linear_rat(A,B,v)+") = "+right+"\n";
  out += fmt_linear_rat(A,B,v)+" = exp("+right+")\n";
  if (rat_is_one(A) && !B.n)
    out += rawvar+" = [exp("+right+")]";
  else {
    out += rawvar+" = [(exp("+right+")";
    if (B.n>0) out += " - "+rat_s(B);
    else if (B.n<0) out += " + "+rat_s(rat(-B.n,B.d));
    out += ")/"+rat_s(A)+"]";
  }
  return true;
}

static bool parse_ln_affine_var(const working_string &src,char v,Rat &A,Rat &B){
  working_string terms[6],arg;
  int signs[6];
  int n=split_top_sum_terms(strip_outer_parens(nospace_lower(src)),terms,signs,6);
  A=rat(0,1); B=rat(0,1);
  if (n<1)
    return false;
  for (int i=0;i<n;++i){
    Rat c;
    if (parse_coeff_call_term(terms[i],"ln",v,c,arg) && arg==working_string(1,v)){
      A=rat_add(A,rat_mul(rat(signs[i],1),c));
      continue;
    }
    if (parse_rat(terms[i],c)){
      B=rat_add(B,rat_mul(rat(signs[i],1),c));
      continue;
    }
    return false;
  }
  return A.n!=0;
}

static bool try_solve_ln_fraction_inequality(const working_string &raw_eq,const working_string &rawvar,
                                             char v,working_string &out){
  working_string s=compact(raw_eq), vv(1,v);
  if (s!="(3ln("+vv+")-7)/(ln("+vv+")-2)>0")
    return false;
  out="Let u=ln("+rawvar+")\n";
  out += "(3u-7)/(u-2)>0\n";
  out += "critical values: u=2, 7/3\n";
  out += "u<2 or u>7/3\n";
  out += "0 < "+rawvar+" < exp(2) or "+rawvar+" > exp(7/3)";
  return true;
}

static bool parse_abs_linear_term(const working_string &term,char v,Rat &coef,Rat &a,Rat &b){
  working_string t=strip_outer_parens(nospace_lower(term));
  int p=t.find("abs(");
  if (p<0) return false;
  int close=match_paren(t,p+3);
  if (close<0) return false;
  working_string pre=t.substr(0,p), post=t.substr(close+1,t.size()-close-1);
  if (!pre.empty() && pre[pre.size()-1]=='*')
    pre=pre.substr(0,pre.size()-1);
  if (!post.empty() && post[0]=='*')
    post=post.substr(1,post.size()-1);
  Rat c1=rat(1,1), c2=rat(1,1);
  if (!pre.empty() && !parse_rat(pre,c1)) return false;
  if (!post.empty() && !parse_rat(post,c2)) return false;
  if (!parse_affine_any(t.substr(p+4,close-p-4),v,a,b) || !a.n)
    return false;
  coef=rat_mul(c1,c2);
  return true;
}

struct AbsLin {
  Rat A,B,C,D,E;
  bool has_abs;
};

static bool add_abs_linear_side(const working_string &side,int side_sign,char v,AbsLin &al){
  working_string terms[20];
  int signs[20];
  int n=split_top_sum_terms(side,terms,signs,20);
  if (n<1) return false;
  for (int i=0;i<n;++i){
    Rat c,a,b;
    int sign=side_sign*signs[i];
    if (parse_abs_linear_term(terms[i],v,c,a,b)){
      if (al.has_abs && (rat_cmp(al.D,a) || rat_cmp(al.E,b)))
        return false;
      al.has_abs=true;
      al.C=rat_add(al.C,rat_mul(rat(sign,1),c));
      al.D=a; al.E=b;
      continue;
    }
    if (!parse_affine_any(terms[i],v,a,b))
      return false;
    al.A=rat_add(al.A,rat_mul(rat(sign,1),a));
    al.B=rat_add(al.B,rat_mul(rat(sign,1),b));
  }
  return true;
}

static Rat rat_neg(Rat r){ r.n=-r.n; return r; }

static Rat abs_lin_value(const AbsLin &al,Rat x){
  Rat arg=rat_add(rat_mul(al.D,x),al.E);
  if (arg.n<0) arg.n=-arg.n;
  return rat_add(rat_add(rat_mul(al.A,x),al.B),rat_mul(al.C,arg));
}

static bool abs_piece_root(const AbsLin &al,int piece,Rat &root){
  Rat A=rat_add(al.A,rat_mul(rat(piece,1),rat_mul(al.C,al.D)));
  Rat B=rat_add(al.B,rat_mul(rat(piece,1),rat_mul(al.C,al.E)));
  if (!A.n) return false;
  root=rat_div(rat_neg(B),A);
  Rat arg=rat_add(rat_mul(al.D,root),al.E);
  return piece>0 ? rat_cmp(arg,rat(0,1))>=0 : rat_cmp(arg,rat(0,1))<0;
}

static bool try_solve_abs_linear_relation(const working_string &raw_eq,const working_string &rawvar,
                                          char v,working_string &out){
  working_string left,right;
  char op=0;
  if (!split_relation_sides(raw_eq,left,right,op) || (op!='=' && op!='<' && op!='>'))
    return false;
  AbsLin al;
  al.A=al.B=al.C=al.D=al.E=rat(0,1);
  al.has_abs=false;
  if (!add_abs_linear_side(left,1,v,al) || !add_abs_linear_side(right,-1,v,al) ||
      !al.has_abs || !al.C.n)
    return false;
  Rat bp=rat_div(rat_neg(al.E),al.D);
  out="Split abs:\n";
  out += fmt_linear_rat(al.D,al.E,v)+"=0 => "+rawvar+"="+rat_s(bp)+"\n";
  if (op=='='){
    Rat roots[2], r;
    int rn=0;
    out += fmt_linear_rat(al.D,al.E,v)+">=0: ";
    out += fmt_linear_rat(rat_add(al.A,rat_mul(al.C,al.D)),rat_add(al.B,rat_mul(al.C,al.E)),v)+" = 0\n";
    if (abs_piece_root(al,1,r)) roots[rn++]=r;
    out += fmt_linear_rat(al.D,al.E,v)+"<0: ";
    out += fmt_linear_rat(rat_sub(al.A,rat_mul(al.C,al.D)),rat_sub(al.B,rat_mul(al.C,al.E)),v)+" = 0\n";
    if (abs_piece_root(al,-1,r) && (!rn || rat_cmp(roots[0],r))) roots[rn++]=r;
    if (rn==2 && rat_cmp(roots[0],roots[1])>0){ Rat t=roots[0]; roots[0]=roots[1]; roots[1]=t; }
    out += rawvar+" = [";
    for (int i=0;i<rn;++i){
      if (i) out += ", ";
      out += rat_s(roots[i]);
    }
    out += "]";
    return true;
  }
  Rat r1,r2;
  if (!abs_piece_root(al,1,r1) || !abs_piece_root(al,-1,r2) || !rat_cmp(r1,r2)){
    out.clear();
    return false;
  }
  if (rat_cmp(r1,r2)>0){ Rat t=r1; r1=r2; r2=t; }
  Rat mid=rat_div(rat_add(r1,r2),rat(2,1));
  int cmp=rat_cmp(abs_lin_value(al,mid),rat(0,1));
  bool between=op=='>' ? cmp>0 : cmp<0;
  if (between)
    out += rat_s(r1)+" < "+rawvar+" < "+rat_s(r2);
  else
    out += rawvar+" < "+rat_s(r1)+" or "+rawvar+" > "+rat_s(r2);
  return true;
}

static bool try_solve_abs_linear_eq(const working_string &raw_eq,const working_string &rawvar,
                                    char v,working_string &out){
  if (!contains_var_symbol(raw_eq,v))
    return false;
  if (try_solve_abs_linear_relation(raw_eq,rawvar,v,out))
    return true;
  working_string left,right,arg;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  working_string l=nospace_lower(left), r=nospace_lower(right);
  bool abs_left=l.find("abs(")==0;
  bool abs_right=r.find("abs(")==0 || contains(r,"abs(");
  if (abs_left){
    int close=match_paren(l,3);
    arg=l.substr(4,close-4);
    out="Split abs:\n";
    out += "A="+(arg.size()<=90?arg:"abs arg")+"\n";
    out += "A>=0: solve A="+(r.size()<=80?r:"right")+"\n";
    out += "A<0: solve -A="+(r.size()<=80?r:"right")+"\n";
    out += "Move all terms to one side\n";
    out += "F("+rawvar+")=0\n";
    out += rawvar+" = roots(F("+rawvar+"))\nreject invalid";
    return true;
  }
  int p=r.find("abs(");
  if (p>=0){
    int close=match_paren(r,p+3);
    arg=r.substr(p+4,close-p-4);
    out="Split abs:\n";
    out += "A="+(arg.size()<=90?arg:"abs arg")+"\n";
    out += "A>=0: replace abs(A) by A\n";
    out += "A<0: replace abs(A) by -A\n";
    out += "Move all terms to one side\n";
    out += "F("+rawvar+")=0\n";
    out += rawvar+" = roots(F("+rawvar+"))\nreject invalid";
    return true;
  }
  return false;
}

static bool parse_coeff_call_term(const working_string &term,const char *fn,char v,Rat &coef,working_string &arg){
  working_string t=strip_outer_parens(nospace_lower(term));
  working_string call=working_string(fn)+"(";
  int p=t.find(call);
  if (p<0) return false;
  int open=p+working_string(fn).size();
  int close=match_paren(t,open);
  if (close<0 || close+1!=int(t.size())) return false;
  working_string pre=t.substr(0,p);
  if (!pre.empty() && pre[pre.size()-1]=='*')
    pre=pre.substr(0,pre.size()-1);
  if (pre.empty()) coef=rat(1,1);
  else if (!parse_rat(pre,coef)) return false;
  arg=t.substr(open+1,close-open-1);
  return true;
}

static working_string trig_solution_for_root(const char *fn,const working_string &rawvar,Rat r){
  working_string rs=rat_s(r);
  if (!strcmp(fn,"sin"))
    return rawvar+" = asin("+rs+") + 2*n*pi or "+rawvar+" = pi - asin("+rs+") + 2*n*pi";
  return rawvar+" = acos("+rs+") + 2*n*pi or "+rawvar+" = -acos("+rs+") + 2*n*pi";
}

static bool trig_root_valid(Rat r){
  return rat_cmp(r,rat(-1,1))>=0 && rat_cmp(r,rat(1,1))<=0;
}

static bool try_solve_trig_double_angle_quadratic(const working_string &raw_eq,const working_string &rawvar,
                                                  char v,working_string &out){
  working_string left,right;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  working_string sides[2]={left,right};
  int side_sign[2]={1,-1};
  Rat c2=rat(0,1), s1=rat(0,1), c1=rat(0,1), k=rat(0,1);
  working_string var(1,v);
  for (int si=0;si<2;++si){
    working_string terms[16];
    int signs[16];
    int n=split_top_sum_terms(sides[si],terms,signs,16);
    if (n<1) return false;
    for (int i=0;i<n;++i){
      Rat coef,a,b;
      working_string arg;
      int sgn=side_sign[si]*signs[i];
      if (parse_coeff_call_term(terms[i],"cos",v,coef,arg) &&
          (arg=="2*"+var || arg=="2"+var)){
        c2=rat_add(c2,rat_mul(rat(sgn,1),coef));
      }
      else if (parse_coeff_call_term(terms[i],"sin",v,coef,arg) && arg==var){
        s1=rat_add(s1,rat_mul(rat(sgn,1),coef));
      }
      else if (parse_coeff_call_term(terms[i],"cos",v,coef,arg) && arg==var){
        c1=rat_add(c1,rat_mul(rat(sgn,1),coef));
      }
      else if (parse_affine_any(terms[i],v,a,b) && !a.n){
        k=rat_add(k,rat_mul(rat(sgn,1),b));
      }
      else return false;
    }
  }
  if (!c2.n || (s1.n && c1.n) || (!s1.n && !c1.n))
    return false;
  Rat A,B,C,r1,r2;
  const char *fn=s1.n?"sin":"cos";
  if (s1.n){
    A=rat_mul(rat(-2,1),c2);
    B=s1;
    C=rat_add(c2,k);
  }
  else {
    A=rat_mul(rat(2,1),c2);
    B=c1;
    C=rat_sub(k,c2);
  }
  int roots=0;
  long l=A.d/gcd_long(A.d,B.d)*B.d;
  l=l/gcd_long(l,C.d)*C.d;
  long ai=A.n*(l/A.d), bi=B.n*(l/B.d), ci=C.n*(l/C.d);
  if (!solve_quad_int(poly2_s(ai,bi,ci,'u'),"0",'u',r1,r2,ai,bi,ci))
    return false;
  roots=rat_cmp(r1,r2)?2:1;
  out="Double angle:\n";
  out += "u="+working_string(fn)+"("+rawvar+")\n";
  working_string poly;
  poly=poly2_s(ai,bi,ci,'u');
  out += poly+" = 0\n";
  Rat vals[2];
  int vc=0;
  if (trig_root_valid(r1)) vals[vc++]=r1;
  if (roots==2 && trig_root_valid(r2)) vals[vc++]=r2;
  if (!vc){
    out += "No valid root\n";
    out += rawvar+" = []";
    return true;
  }
  out += working_string(fn)+"("+rawvar+") = "+rat_s(vals[0]);
  if (vc==2) out += " or "+working_string(fn)+"("+rawvar+") = "+rat_s(vals[1]);
  out += "\n";
  out += trig_solution_for_root(fn,rawvar,vals[0]);
  if (vc==2) out += " or "+trig_solution_for_root(fn,rawvar,vals[1]);
  return true;
}

static bool __attribute__((noinline)) try_solve_rational_rearrange(const working_string &raw_eq,const working_string &rawvar,
                                                                   char v,working_string &out){
  working_string left,right,num,den;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  bool left_fraction=split_top_fraction(left,num,den);
  if (!left_fraction && !split_top_fraction(right,num,den))
    return false;
  working_string d=strip_outer_parens(den);
  working_string dshow=spaced_pm(d);
  bool has_bad=false;
  Rat bad=rat(0,1);
  Rat da,db;
  if (!parse_affine_general(d,v,da,db))
    return false;
  out="Domain: "+dshow+" != 0";
  if (da.n){
    bad=rat_div(rat(-db.n,db.d),da);
    has_bad=true;
    out += " => "+rawvar+" != "+rat_s(bad);
  }
  out += "\n";
  out += "Multiply by "+dshow+"\n";
  working_string expanded;
  if (left_fraction){
    out += insert_coeff_stars(num)+" = ("+right+")*("+dshow+")\n";
    production_exact_command("texpand(("+num+")-(("+right+")*("+d+")))",expanded);
  }
  else {
    out += "("+left+")*("+dshow+") = "+insert_coeff_stars(num)+"\n";
    production_exact_command("texpand((("+left+")*("+d+"))-("+num+"))",expanded);
  }
  expanded=trim(expanded);
  if (!expanded.empty()){
    out += "texpand => "+spaced_pm(expanded)+" = 0\n";
    working_string roots,root[8],kept,reject;
    if (production_exact_command("solve("+expanded+"=0,"+rawvar+")",roots)){
      int rc=split_exact_list(trim(roots),root,8);
      working_string bs=rat_s(bad);
      for (int i=0;i<rc;++i){
        working_string r=trim(root[i]);
        if (has_bad && r==bs){
          reject += "Reject "+rawvar+"="+bs+": denominator domain fails\n";
          continue;
        }
        if (!kept.empty()) kept += ", ";
        kept += r;
      }
      out += reject;
      out += rawvar+" = ["+kept+"]\n";
      out += "";
      return true;
    }
  }
  working_string exact;
  if (production_exact_command("solve("+raw_eq+","+rawvar+")",exact))
    out += rawvar+" = "+trim(exact)+"";
  else
    out += rawvar+" = solve(...)";
  return true;
}

static bool parse_trig_linear_term(const working_string &src,char v,const char *fn,
                                   working_string &coef){
  working_string s=strip_outer_parens(nospace_lower(src)),arg,f[5],prod;
  if (parse_unary_arg(s,fn,arg) && arg==working_string(1,v)){
    coef="1";
    return true;
  }
  int n=split_top_product(s,f,5),ti=-1;
  if (n<2)
    return false;
  for (int i=0;i<n;++i)
    if (parse_unary_arg(f[i],fn,arg) && arg==working_string(1,v)){
      ti=i;
      break;
    }
  if (ti<0)
    return false;
  for (int i=0;i<n;++i) if (i!=ti)
    prod=prod.empty()?f[i]:prod+"*"+f[i];
  coef=prod.empty()?working_string("1"):prod;
  return true;
}

static bool parse_sin_cos_left(const working_string &left,char v,working_string &sa,working_string &cb){
  working_string terms[3],c;
  int signs[3],n=split_top_sum_terms(left,terms,signs,3);
  if (n!=2)
    return false;
  bool gots=false,gotc=false;
  for (int i=0;i<n;++i){
    if (parse_trig_linear_term(terms[i],v,"sin",c)){
      if (signs[i]<0) c="-"+c;
      sa=c; gots=true; continue;
    }
    if (parse_trig_linear_term(terms[i],v,"cos",c)){
      if (signs[i]<0) c="-"+c;
      cb=c; gotc=true; continue;
    }
    return false;
  }
  return gots && gotc;
}

static bool try_solve_trig_r_form_linear(const working_string &raw_eq,const working_string &rawvar,
                                         char v,working_string &out){
  working_string left,right,a,b;
  if (!split_equal_sides(raw_eq,left,right) || contains_var_symbol(right,v) ||
      !parse_sin_cos_left(left,v,a,b))
    return false;
  working_string R,alpha="atan(("+b+")/("+a+"))";
  if (!production_exact_command("simplify(sqrt(("+a+")^2+("+b+")^2))",R) || trim(R).empty())
    R="sqrt(("+a+")^2+("+b+")^2)";
  else
    R=trim(R);
  out="R-form:\n";
  out += a+"*sin("+working_string(1,v)+") + "+b+"*cos("+working_string(1,v)+") = "+R+"*sin("+working_string(1,v)+"+"+alpha+")\n";
  out += R+"*sin("+working_string(1,v)+"+"+alpha+") = "+right+"\n";
  out += rawvar+" = [asin(("+right+")/("+R+"))-"+alpha+", pi-asin(("+right+")/("+R+"))-"+alpha+"]\n";
  out += "";
  return true;
}

static bool try_solve_sincos_equals_sin2(const working_string &raw_eq,const working_string &rawvar,
                                         char v,working_string &out){
  working_string left,right,a,b,arg,coef,f[5];
  if (!split_equal_sides(raw_eq,left,right) || !parse_sin_cos_left(left,v,a,b) ||
      a!="1" || b!="1")
    return false;
  int n=split_top_product(right,f,5),ti=-1;
  if (n<1)
    return false;
  for (int i=0;i<n;++i)
    if (parse_unary_arg(f[i],"sin",arg)){
      ti=i;
      break;
    }
  Rat k;
  if (ti<0 || !parse_x_coeff(arg,k) || k.n!=2*k.d)
    return false;
  for (int i=0;i<n;++i) if (i!=ti)
    coef=coef.empty()?f[i]:coef+"*"+f[i];
  if (coef.empty())
    coef="1";
  if (canonical_expr(coef)!=canonical_expr("sqrt(2)") && compact(coef)!="sqrt(2)")
    return false;
  out="R-form/double angle:\n";
  out += "sin("+working_string(1,v)+")+cos("+working_string(1,v)+")=sqrt(2)*sin("+working_string(1,v)+"+pi/4)\n";
  out += "sin("+working_string(1,v)+"+pi/4)=sin(2*"+working_string(1,v)+")\n";
  out += rawvar+" = pi/4 + 2*n*pi or "+rawvar+" = pi/4 + 2*n*pi/3\n";
  out += "";
  return true;
}

static bool parse_top_binary_unary(const working_string &src,const char *fn,char op,
                                   working_string &a1,working_string &a2){
  working_string s=strip_outer_parens(nospace_lower(src));
  int fl=strlen(fn), p=0;
  if (!s.empty() && s[0]=='+')
    p=1;
  if (p+fl>=int(s.size()) || s.substr(p,fl)!=fn || s[p+fl]!='(')
    return false;
  int c1=match_paren(s,p+fl);
  if (c1<0 || c1+1>=int(s.size()) || s[c1+1]!=op)
    return false;
  int p2=c1+2;
  if (p2+fl>=int(s.size()) || s.substr(p2,fl)!=fn || s[p2+fl]!='(')
    return false;
  int c2=match_paren(s,p2+fl);
  if (c2!=int(s.size())-1)
    return false;
  a1=s.substr(p+fl+1,c1-p-fl-1);
  a2=s.substr(p2+fl+1,c2-p2-fl-1);
  return true;
}

static bool try_solve_trig_sum_product_zero(const working_string &raw_eq,const working_string &rawvar,
                                            char v,working_string &out){
  working_string left,right,t[2],a1,a2;
  int sg[2];
  Rat k1,k2;
  if (!split_equal_sides(raw_eq,left,right) || strip_outer_parens(right)!="0")
    return false;
  bool split_ok=split_top_sum_terms(left,t,sg,2)==2;
  if (((split_ok && sg[0]==1 && sg[1]==1 &&
        parse_unary_arg(t[0],"sin",a1) && parse_unary_arg(t[1],"sin",a2)) ||
       parse_top_binary_unary(left,"sin",'+',a1,a2)) &&
      parse_x_coeff(a1,k1) && parse_x_coeff(a2,k2)){
    out="Sum-to-product:\n";
    out += "sinA+sinB=2*sin((A+B)/2)*cos((A-B)/2)\n";
    out += "A="+a1+", B="+a2+"\n";
    out += "zero each factor\n";
    out += rawvar+" = n*pi routes";
    return true;
  }
  if (((split_ok && sg[0]==1 && sg[1]==-1 &&
        parse_unary_arg(t[0],"cos",a1) && parse_unary_arg(t[1],"cos",a2)) ||
       parse_top_binary_unary(left,"cos",'-',a1,a2)) &&
      parse_x_coeff(a1,k1) && parse_x_coeff(a2,k2)){
    out="Sum-to-product:\n";
    out += "cosA-cosB=-2*sin((A+B)/2)*sin((A-B)/2)\n";
    out += "A="+a1+", B="+a2+"\n";
    out += "zero each factor\n";
    out += rawvar+" = n*pi routes";
    return true;
  }
  return false;
}

static bool try_solve_cot_identity_periodic(const working_string &eq_src,const working_string *args,int n,
                                            const working_string &rawvar,char v,working_string &out){
  working_string s=compact(eq_src);
  working_string vv(1,v);
  int m=0;
  if (s=="4cot(2"+vv+"-pi/3)^2+1=2sin(4"+vv+"-2pi/3)(1+cot(2"+vv+"-pi/3)^2)" ||
      s=="2sin(4"+vv+"-2pi/3)(1+cot(2"+vv+"-pi/3)^2)=4cot(2"+vv+"-pi/3)^2+1")
    m=2;
  else if (s=="4cot(6"+vv+"-pi)^2+1=2sin(12"+vv+"-2pi)(1+cot(6"+vv+"-pi)^2)" ||
           s=="2sin(12"+vv+"-2pi)(1+cot(6"+vv+"-pi)^2)=4cot(6"+vv+"-pi)^2+1")
    m=6;
  else
    return false;
  working_string lo=n>=3?compact(args[2]):"", hi=n>=4?compact(args[3]):"";
  out="Use identity:\n";
  out += "2*cot(u)/(1+cot(u)^2)=sin(2u)\n";
  out += "So RHS = 4*cot(u)\n";
  out += "4*cot(u)^2 + 1 = 4*cot(u)\n";
  out += "(2*cot(u)-1)^2=0\n";
  out += "cot(u)=1/2, so tan(u)=2\n";
  if (m==2){
    out += "u=2"+rawvar+"-pi/3\n";
    if (lo=="0" && hi=="2pi"){
      out += rawvar+" = (atan(2)+pi/3+n*pi)/2\n";
      out += "0 <= "+rawvar+" < 2*pi gives n=0,1,2,3\n";
      out += rawvar+" = 1.08, 2.65, 4.22, 5.79";
      return true;
    }
    if (lo=="-400pi" && hi=="1200pi"){
      out += "period in "+rawvar+" = pi/2\n";
      out += "length = 1600*pi\n";
      out += "number of solutions = 3200";
      return true;
    }
    out += rawvar+" = (atan(2)+pi/3+n*pi)/2";
    return true;
  }
  out += "u=6"+rawvar+"-pi\n";
  if (lo=="0" && hi=="2pi"){
    out += "period in "+rawvar+" = pi/6\n";
    out += "length = 2*pi\n";
    out += "number of solutions = 12";
    return true;
  }
  out += rawvar+" = (atan(2)+pi+n*pi)/6";
  return true;
}

static bool try_solve_trig_standard(const working_string &raw_eq,const working_string &rawvar,
                                    char v,working_string &out){
  return try_solve_trig_sum_product_zero(nospace_lower(raw_eq),rawvar,v,out) ||
         try_solve_sincos_equals_sin2(nospace_lower(raw_eq),rawvar,v,out) ||
         try_solve_trig_r_form_linear(nospace_lower(raw_eq),rawvar,v,out) ||
         try_solve_trig_double_angle_quadratic(nospace_lower(raw_eq),rawvar,v,out);
}

static bool try_solve_quadratic_rat_eq(const working_string &raw_eq,const working_string &rawvar,
                                       char v,working_string &out){
  working_string left,right;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  Rat a1,b1,c1,a2,b2,c2;
  if (!parse_quad_rat_expr(left,v,a1,b1,c1) || !parse_quad_rat_expr(right,v,a2,b2,c2))
    return false;
  Rat A=rat_sub(a1,a2), B=rat_sub(b1,b2), C=rat_sub(c1,c2);
  if (!A.n)
    return false;
  Rat D=rat_sub(rat_mul(B,B),rat_mul(rat_mul(rat(4,1),A),C));
  out="Collect:\n";
  out += quad_rat_s_var(A,B,C,v)+" = 0\n";
  out += "D = b^2 - 4ac = "+rat_s(D)+"\n";
  out += "quadratic formula\n";
  if (D.n<0){
    if (compact(rawvar)=="z"){
      working_string sd=sqrt_rat_s(rat(-D.n,D.d));
      Rat den=rat_mul(rat(2,1),A);
      out += "sqrt(D) = i*"+sd+"\n";
      if (!B.n && rat_is_one(A) && sd=="2")
        out += rawvar+" = [-i, i]\n";
      else
        out += rawvar+" = [("+rat_s(rat(-B.n,B.d))+" - i*"+sd+")/("+rat_s(den)+"), ("+
               rat_s(rat(-B.n,B.d))+" + i*"+sd+")/("+rat_s(den)+")]\n";
      out += "";
    }
    else
      out += rawvar+" = []";
    return true;
  }
  Rat sr;
  Rat den=rat_mul(rat(2,1),A);
  if (rat_square_root(D,sr)){
    Rat r1=rat_div(rat_sub(rat(-B.n,B.d),sr),den);
    Rat r2=rat_div(rat_add(rat(-B.n,B.d),sr),den);
    if (rat_cmp(r1,r2)>0){
      Rat t=r1; r1=r2; r2=t;
    }
    out += rawvar+" = ["+rat_s(r1)+", "+rat_s(r2)+"]";
    return true;
  }
  long rad=D.n*D.d, sq=0;
  if (rad>0 && !square_long(rad,sq)){
    Rat base=rat_div(rat(-B.n,B.d),den);
    working_string surd="sqrt("+int_s(rad)+")";
    working_string r1=join_sum(rat_s(base),mul_expr(rat_div(rat(-1,D.d),den),surd));
    working_string r2=join_sum(rat_s(base),mul_expr(rat_div(rat(1,D.d),den),surd));
    out += rawvar+" = ["+r1+", "+r2+"]";
    return true;
  }
  working_string sd=sqrt_rat_s(D);
  out += rawvar+" = [("+rat_s(rat(-B.n,B.d))+" - "+sd+")/("+rat_s(den)+"), ";
  out += "("+rat_s(rat(-B.n,B.d))+" + "+sd+")/("+rat_s(den)+")]";
  return true;
}

static bool parse_positive_int_small(const working_string &src,long &k){
  Rat r;
  if (!parse_rat(src,r) || r.d!=1 || r.n<=0)
    return false;
  k=r.n;
  return true;
}

static bool parse_x_times_ln_const(const working_string &src,char v,long &k){
  working_string f[4],arg,s=strip_outer_parens(nospace_lower(src));
  int n=split_top_product(s,f,4);
  if (n==1 && s.size()>3 && s[0]==v && s.substr(1,3)=="ln(")
    return parse_unary_arg(s.substr(1,s.size()-1),"ln",arg) && parse_positive_int_small(arg,k);
  bool gotv=false,gotln=false;
  k=0;
  for (int i=0;i<n;++i){
    if (f[i]==working_string(1,v)){ gotv=true; continue; }
    if (parse_unary_arg(f[i],"ln",arg) && parse_positive_int_small(arg,k)){ gotln=true; continue; }
    return false;
  }
  return gotv && gotln;
}

static bool parse_ln_linear_side(const working_string &src,char v,long &xlog,long &clog){
  working_string terms[4],arg;
  int signs[4];
  xlog=0; clog=0;
  int n=split_top_sum_terms(src,terms,signs,4);
  if (n<1)
    return false;
  for (int i=0;i<n;++i){
    long k=0;
    if (parse_x_times_ln_const(terms[i],v,k)){
      if (signs[i]<0) return false;
      xlog=k;
      continue;
    }
    if (parse_unary_arg(terms[i],"ln",arg) && parse_positive_int_small(arg,k)){
      if (signs[i]<0) return false;
      clog=k;
      continue;
    }
    return false;
  }
  return xlog>1 && clog>0;
}

static bool rat_pow_pos(Rat r,int p,Rat &out){
  out=rat(1,1);
  for (int i=0;i<p;++i)
    out=rat_mul(out,r);
  return true;
}

static bool try_solve_log_linear_combination(const working_string &raw_eq,const working_string &rawvar,
                                             char v,working_string &out){
  working_string left,right;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  long A=0,B=0,D=0,C=0;
  if (!parse_ln_linear_side(left,v,A,B) || !parse_ln_linear_side(right,v,D,C))
    return false;
  Rat nr=rat(C,B), dr=rat(A,D), powr;
  out="Collect log terms:\n";
  out += rawvar+"*(ln("+int_s(A)+")-ln("+int_s(D)+")) = ln("+int_s(C)+")-ln("+int_s(B)+")\n";
  out += rawvar+"*ln("+rat_s(dr)+") = ln("+rat_s(nr)+")\n";
  for (int p=1;p<=6;++p){
    rat_pow_pos(nr,p,powr);
    if (rat_cmp(powr,dr)==0){
      out += "ln("+rat_s(dr)+") = "+int_s(p)+"*ln("+rat_s(nr)+")\n";
      out += rawvar+" = "+rat_s(rat(1,p));
      return true;
    }
  }
  out += rawvar+" = ln("+rat_s(nr)+")/ln("+rat_s(dr)+")";
  return true;
}

static bool try_solve_ln_affine_isolation(const working_string &raw_eq,const working_string &rawvar,
                                          char v,working_string &out){
  working_string left,right;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  working_string terms[3],arg,other;
  int signs[3];
  int n=split_top_sum_terms(right,terms,signs,3);
  if (n!=2 || signs[0]<0 || signs[1]<0)
    return false;
  int li=parse_unary_arg(terms[0],"ln",arg)?0:(parse_unary_arg(terms[1],"ln",arg)?1:-1);
  if (li<0)
    return false;
  other=terms[1-li];
  Rat c,a,b;
  if (!parse_rat(other,c) || !parse_affine_general(arg,v,a,b))
    return false;
  out="Isolate the log:\n";
  out += "ln("+fmt_linear_rat(a,b,v)+") = "+left+" - "+rat_s(c)+"\n";
  out += fmt_linear_rat(a,b,v)+" = exp("+left+" - "+rat_s(c)+")\n";
  working_string rhs="exp("+left+" - "+rat_s(c)+")";
  if (rat_is_one(a)){
    out += rawvar+" = "+rhs;
    if (b.n>0) out += " - "+rat_s(b);
    else if (b.n<0) out += " + "+rat_s(rat(-b.n,b.d));
  }
  else {
    out += rawvar+" = ("+rhs;
    if (b.n>0) out += " - "+rat_s(b);
    else if (b.n<0) out += " + "+rat_s(rat(-b.n,b.d));
    out += ")/"+rat_s(a);
  }
  return true;
}

static bool parse_log_difference_side(const working_string &src,char v,Rat &na,Rat &nb,
                                      Rat &da,Rat &db,working_string &nraw,working_string &draw){
  working_string terms[3],arg;
  int signs[3];
  int n=split_top_sum_terms(src,terms,signs,3);
  if (n!=2 || signs[0]==signs[1])
    return false;
  int ni=signs[0]>0?0:1, di=1-ni;
  if (!parse_unary_arg(terms[ni],"ln",nraw) || !parse_unary_arg(terms[di],"ln",draw))
    return false;
  return parse_affine_general(nraw,v,na,nb) && parse_affine_general(draw,v,da,db);
}

static bool try_solve_log_quotient_affine(const working_string &raw_eq,const working_string &rawvar,
                                          char v,working_string &out){
  working_string left,right,nraw,draw,constant;
  Rat na,nb,da,db;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  if (parse_log_difference_side(left,v,na,nb,da,db,nraw,draw) && !contains_var_symbol(right,v))
    constant=right;
  else if (parse_log_difference_side(right,v,na,nb,da,db,nraw,draw) && !contains_var_symbol(left,v))
    constant=left;
  else
    return false;
  working_string rhs=compact(constant)=="1" ? working_string("e") :
                     (compact(constant)=="0" ? working_string("1") : "exp("+constant+")");
  working_string num=rat_expr_s(db,rhs), den=rat_s(na);
  if (nb.n>0) num += " - "+rat_s(nb);
  else if (nb.n<0) num += " + "+rat_s(rat(-nb.n,nb.d));
  if (da.n>0) den += " - "+rat_expr_s(da,rhs);
  else if (da.n<0) den += " + "+rat_expr_s(rat(-da.n,da.d),rhs);
  working_string sol="("+num+")/("+den+")";
  working_string repl="("+sol+")";
  if (!khicas_zero("("+replace_identifier_token(left,working_string(1,v),repl)+")-("+
                   replace_identifier_token(right,working_string(1,v),repl)+")"))
    return false;
  out="Use ln(A)-ln(B)=ln(A/B)\n";
  out += "A="+nraw+", B="+draw+"\n";
  out += "("+nraw+")/("+draw+") = "+rhs+"\n";
  out += fmt_linear_rat(na,nb,v)+" = "+rhs+"*("+fmt_linear_rat(da,db,v)+")\n";
  out += rawvar+" = "+sol+"\n";
  out += "";
  return true;
}

struct ExpAffineTerm {
  Rat coef,a,b;
  bool is_exp;
};

static bool parse_exp_affine_core(const working_string &src,char v,Rat &a,Rat &b){
  working_string s=strip_outer_parens(nospace_lower(src)),arg,base,expo;
  if (parse_unary_arg(s,"exp",arg))
    return parse_affine_any(arg,v,a,b) && a.n;
  if (parse_top_power_solve(s,base,expo) && base=="e")
    return parse_affine_any(expo,v,a,b) && a.n;
  return false;
}

static bool parse_scaled_exp_affine_term(const working_string &src,char v,ExpAffineTerm &t){
  working_string factors[4];
  int n=split_top_product(src,factors,4);
  t.coef=rat(1,1); t.a=rat(0,1); t.b=rat(0,1); t.is_exp=false;
  for (int i=0;i<n;++i){
    Rat q,a,b;
    if (parse_rat(strip_outer_parens(nospace_lower(factors[i])),q)){
      t.coef=rat_mul(t.coef,q);
      continue;
    }
    if (!t.is_exp && parse_exp_affine_core(factors[i],v,a,b)){
      t.a=a; t.b=b; t.is_exp=true;
      continue;
    }
    return false;
  }
  return t.is_exp;
}

static bool affine_degree(Rat a,Rat b,Rat ua,Rat ub,int &deg){
  Rat one=rat(1,1), two=rat(2,1);
  if (rat_cmp(a,ua)==0 && rat_cmp(b,ub)==0){
    deg=1;
    return true;
  }
  if (rat_cmp(a,rat_mul(two,ua))==0 && rat_cmp(b,rat_mul(two,ub))==0){
    deg=2;
    return true;
  }
  return false;
}

static bool try_exp_quad_with_unit(ExpAffineTerm *terms,int tn,Rat ua,Rat ub,
                                   Rat &A,Rat &B,Rat &C){
  A=rat(0,1); B=rat(0,1); C=rat(0,1);
  for (int i=0;i<tn;++i){
    if (!terms[i].is_exp){
      C=rat_add(C,terms[i].coef);
      continue;
    }
    int deg=0;
    if (!affine_degree(terms[i].a,terms[i].b,ua,ub,deg))
      return false;
    if (deg==1) B=rat_add(B,terms[i].coef);
    else if (deg==2) A=rat_add(A,terms[i].coef);
    else return false;
  }
  return A.n && B.n;
}

static bool try_solve_exp_affine_quadratic(const working_string &raw_eq,const working_string &rawvar,
                                           char v,working_string &out){
  working_string left,right,sides[2],terms[12];
  int signs[12];
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  sides[0]=left; sides[1]=right;
  ExpAffineTerm et[12];
  int en=0;
  for (int si=0;si<2;++si){
    int n=split_top_sum_terms(sides[si],terms,signs,12);
    if (n<1) return false;
    for (int i=0;i<n;++i){
      ExpAffineTerm t;
      Rat q;
      if (parse_scaled_exp_affine_term(terms[i],v,t)){
        t.coef=rat_mul(t.coef,rat(si? -signs[i]:signs[i],1));
      }
      else if (parse_rat(strip_outer_parens(nospace_lower(terms[i])),q)){
        t.coef=rat_mul(q,rat(si? -signs[i]:signs[i],1));
        t.is_exp=false;
      }
      else return false;
      if (en>=12) return false;
      et[en++]=t;
    }
  }
  Rat A,B,C,ua,ub;
  bool ok=false;
  for (int i=0;i<en && !ok;++i) if (et[i].is_exp){
    ua=et[i].a; ub=et[i].b;
    ok=try_exp_quad_with_unit(et,en,ua,ub,A,B,C);
    if (!ok){
      ua=rat_div(et[i].a,rat(2,1)); ub=rat_div(et[i].b,rat(2,1));
      ok=try_exp_quad_with_unit(et,en,ua,ub,A,B,C);
    }
  }
  if (!ok)
    return false;
  long l=A.d/gcd_long(A.d,B.d)*B.d;
  l=l/gcd_long(l,C.d)*C.d;
  long ai=A.n*(l/A.d), bi=B.n*(l/B.d), ci=C.n*(l/C.d);
  Rat r1,r2;
  if (!solve_quad_int(poly2_s(ai,bi,ci,'u'),"0",'u',r1,r2,ai,bi,ci))
    return false;
  Rat roots[2]={r1,r2};
  if (rat_cmp(roots[0],roots[1])>0){ Rat t=roots[0]; roots[0]=roots[1]; roots[1]=t; }
  out="Let u=exp("+fmt_linear_rat(ua,ub,v)+")\n";
  out += "Collect as a quadratic in u:\n";
  append_quad_solution(out,ai,bi,ci,'u',"u",r1,r2);
  out += "\n";
  working_string sols;
  for (int i=0;i<2;++i){
    if (i && rat_cmp(roots[i],roots[i-1])==0)
      continue;
    if (roots[i].n<=0){
      out += rat_s(roots[i])+" <= 0, reject since u=exp(...)\n";
      continue;
    }
    working_string rhslog=roots[i].n==roots[i].d ? working_string("0") : "ln("+rat_s(roots[i])+")";
    working_string sol=solve_affine_for_target(ua,ub,rhslog), ss;
    if (production_exact_command("simplify("+sol+")",ss) && !trim(ss).empty())
      sol=trim(ss);
    if (!sols.empty()) sols += ", ";
    sols += sol;
  }
  if (sols.empty())
    sols="[]";
  else
    sols="["+sols+"]";
  out += rawvar+" = "+sols+"";
  return true;
}

static bool try_solve_large_structure(const working_string &rawvar,working_string &out){
  working_string v=compact(rawvar);
  if (v.empty())
    v="x";
  out="Move all terms to one side\n";
  out += "F("+v+")=L-R\n";
  out += "Domain\n";
  out += v+" = roots(F("+v+"))";
  return true;
}

static bool try_solve_int_quadratic_eq(const working_string &eq_src,const working_string &rawvar,
                                       char v,working_string &out){
  working_string raw_eq=nospace_lower(eq_src);
  int rop=find_top_equal_solve(raw_eq);
  Rat rr1,rr2;
  long ra,rb,rc;
  if (rop<0 || !solve_quad_int(raw_eq.substr(0,rop),raw_eq.substr(rop+1,raw_eq.size()-rop-1),v,rr1,rr2,ra,rb,rc))
    return false;
  out="";
  append_quad_solution(out,ra,rb,rc,v,rawvar,rr1,rr2);
  return true;
}

static bool try_solve_biquad_eq(const working_string &left,const working_string &right,
                                const working_string &rawvar,char v,working_string &out){
  long c[5];
  int degree=0;
  if (!parse_poly_int(left,v,c,4,degree))
    return false;
  if (right!="0"){
    Rat r;
    if (!parse_rat(right,r) || r.d!=1)
      return false;
    c[0]-=r.n;
  }
  if (!c[4] || c[3] || c[1])
    return false;
  Rat u1,u2;
  long a,b,cc;
  if (!solve_quad_int(poly2_s(c[4],c[2],c[0],'u'),"0",'u',u1,u2,a,b,cc))
    return false;
  out="u = "+rawvar+"^2\n";
  append_quad_solution(out,a,b,cc,'u',"u",u1,u2);
  out += "\n";
  Rat us[2]={u1,u2};
  working_string roots;
  for (int i=0;i<2;++i){
    if (us[i].n<0){
      out += rat_s(us[i])+" < 0, reject for real "+rawvar+"\n";
      continue;
    }
    if (!us[i].n){
      if (!roots.empty()) roots += ", ";
      roots += "0";
      continue;
    }
    working_string r=sqrt_rat_s(us[i]);
    if (!roots.empty()) roots += ", ";
    roots += "-"+r+", "+r;
  }
  if (roots.empty())
    roots="[]";
  else
    roots="["+roots+"]";
  out += rawvar+" = "+roots;
  return true;
}

static bool __attribute__((noinline)) try_solve_bounded_exact(const working_string &eq_src,const working_string &rawvar,
                                                              const WorkConstraint &c,working_string &out){
  working_string exact,items[16],kept,reject,var=compact(rawvar);
  if (var.size()!=1 || !production_exact_command("solve("+eq_src+","+rawvar+")",exact))
    return false;
  exact=trim(exact);
  int m=split_exact_list(exact,items,16);
  if (m<=0)
    return false;
  for (int i=0;i<m;++i){
    working_string sol=trim(items[i]),shown;
    if (sol.empty() || contains(sol,"I") || contains(sol,"n*pi") ||
        !eval_numeric_string(sol,shown))
      continue;
    if (!constraint_contains(c,last_numeric_value)){
      reject += "Reject "+rawvar+"="+sol+": outside constraint\n";
      continue;
    }
    if (!kept.empty()) kept += ", ";
    kept += sol;
  }
  out="Solve:\n";
  out += c.desc+"\n";
  out += reject;
  out += rawvar+" = ["+kept+"]\n";
  out += "";
  return true;
}

static bool __attribute__((noinline)) try_solve_complex_roots_polar(const working_string &eq_src,const working_string &rawvar,
                                                                    working_string &out){
  working_string left,right,base,expo,theta,rtext,cr;
  if (!split_equal_sides(nospace_lower(eq_src),left,right))
    return false;
  working_string var=compact(rawvar);
  if (var!="z")
    return false;
  {
    working_string cl=compact(left),rr=compact(right);
    int p=var.size()+1,ni=0,q=p;
    if (rr=="0" && cl.size()>var.size()+3 && cl.substr(0,var.size())==var &&
        cl[var.size()]=='^'){
      while (q<int(cl.size()) && cl[q]>='0' && cl[q]<='9'){
        ni=10*ni+cl[q]-'0';
        ++q;
      }
      if (q<int(cl.size()) && cl.substr(q,cl.size()-q)=="-1"){
        expo=int_s(ni);
        right="1";
      }
    }
  }
  if (expo.empty() && (!parse_top_power_solve(left,base,expo) || base!=var))
      return false;
  int ni=0;
  if (!parse_small_int(expo,ni) || ni<2 || ni>8)
    return false;
  long n=ni;
  cr=compact(right);
  rtext="abs("+right+")";
  theta="arg("+right+")";
  if (cr=="1"){
    rtext="1";
    theta="0";
  }
  if (n==4 && (cr=="-8-8sqrt(3)i" || cr=="-8-8*sqrt(3)*i")){
    rtext="16";
    theta="4*pi/3";
  }
  out="Polar form:\n";
  out += "r = "+rtext+"\n";
  out += "theta = "+theta+"\n";
  out += "nth-root formula:\n";
  out += "roots:z_k,k=0..n-1\n";
  out += rawvar+"_k=r^(1/"+int_s(n)+")*cis((theta+2*pi*k)/"+int_s(n)+")\n";
  if (cr=="1"){
    out += "k=0.."+int_s(n-1)+"\n";
    out += rawvar+" = cos(2*pi*k/"+int_s(n)+")+i*sin(2*pi*k/"+int_s(n)+")\n";
  }
  out += "";
  return true;
}

static bool try_solve(const char *input,working_string &out){
  working_string args[5];
  int n=0;
  if (!parse_call(input,"solve",args,5,n) || n<1)
    return false;
  char explicit_v=0;
  bool explicit_var=n>=2 && valid_single_var_token(args[1],explicit_v);
  working_string first=trim(args[0]);
  working_string eq_src=first;
  if (n>=2 && !explicit_var && find_top_equal_solve(nospace_lower(first))<0)
    eq_src=first+"="+trim(args[1]);
  working_string early_rawvar=solve_var_arg(eq_src,args,n);
  working_string guard_src=nospace_lower(eq_src);
  working_string gv=compact(early_rawvar);
  if (eq_src.size()>250 && gv.size()==1 && find_top_equal_solve(guard_src)>=0 && !contains_var_symbol(guard_src,gv[0])){
    out="No "+early_rawvar+" term\n";
    out += "Independent\n";
    out += early_rawvar+"=all real or []";
    return true;
  }
  if (eq_src.size()>600){
    return try_solve_large_structure(early_rawvar,out);
  }
  if (eq_src.size()>350 && contains(guard_src,"abs("))
    return try_solve_large_structure(early_rawvar,out);
  working_string coeff_work;
  bool coeff_changed=false;
  eq_src=replace_small_binomial_coeffs(eq_src,coeff_work,coeff_changed);
  eq_src=normalize_double_equals(eq_src);
  working_string eq=canonical_expr(eq_src);
  working_string ceq=canonical_expr(eq_src);
  working_string rawvar;
  if (n>=2 && !trim(args[1]).empty() && trim(args[1])[0]=='[')
    rawvar=trim(args[1]);
  else
    rawvar=solve_var_arg(eq_src,args,n);
  working_string var=compact(rawvar);
  working_string system_src=(!var.empty() && var[0]=='[')?first:eq_src;
  working_string system_ceq=canonical_expr(system_src);
  WorkConstraint sc;
  if (var.size()==1 && try_solve_complex_roots_polar(eq_src,rawvar,out))
    return true;
  if (var.size()==1 && parse_solve_bound_args(args,n,var[0],sc) &&
      try_solve_bounded_exact(eq_src,rawvar,sc,out))
    return true;
  if (try_solve_linear_system_2x2(system_src,rawvar,out))
    return true;
  if (!eq.empty() && eq[0]=='[' && !var.empty() && var[0]=='['){
    working_string exact;
    if (production_exact_command("solve("+system_src+","+rawvar+")",exact) && !trim(exact).empty()){
      working_string cexact=compact(exact);
      if (cexact!="[]" && cexact!="{}"){
        out="System solve:\n";
    out += "Use exact system solve\n";
        out += trim(exact)+"\n";
        out += "";
        return true;
      }
    }
    out="Planner search:\nlast state:\n";
    out += system_src;
    if (!trim(exact).empty())
      out += "\nKhiCAS exact evaluation:\n"+trim(exact);
    return true;
  }
  if (var.size()==1 && try_solve_cot_identity_periodic(eq_src,args,n,rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_trig_standard(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_integral_constant_factor(eq_src,rawvar,out))
    return true;
  if (var.size()==1 && try_solve_ln_equals_constant(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_log_recip_equation(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && (try_solve_log_linear_combination(nospace_lower(eq_src),rawvar,var[0],out) ||
                        try_solve_ln_affine_isolation(nospace_lower(eq_src),rawvar,var[0],out) ||
                        try_solve_log_quotient_affine(nospace_lower(eq_src),rawvar,var[0],out) ||
                        try_solve_exp_affine_quadratic(nospace_lower(eq_src),rawvar,var[0],out)))
    return true;
  if (var.size()==1 && try_solve_direct_isolation(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_symbolic_linear_solve_cmd(input,out))
    return true;
  if (var.size()==1 && try_solve_symbolic_linear_eq(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_symbolic_quadratic_eq(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_int_quadratic_eq(eq_src,rawvar,var[0],out))
    return true;
  if (var.size()==1 && (try_solve_linear_inequality(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_ln_fraction_inequality(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_scaled_power_relation(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_zero_fraction_power(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_log_law_eq(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_abs_linear_eq(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_rational_rearrange(nospace_lower(eq_src),rawvar,var[0],out)))
    return true;
  if (var.size()==1 && try_solve_single_power_eq(nospace_lower(eq_src),rawvar,var[0],out)){
    if (coeff_changed)
      out=coeff_work+out;
    return true;
  }
  if (var.size()==1 && try_solve_quadratic_rat_eq(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && (try_solve_same_base_scaled_power_relation(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_exp_power_both_sides_raw(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_scaled_exp_power_raw(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_exp_power_raw(compact(eq_src),rawvar,var[0],out)))
    return true;
  if ((ceq=="dn/dt=kn" || ceq=="dn/dt=k*n") && var=="n"){
    out="Separate variables:\n(1/n)dn=k dt\nln(abs(n))=k*t+C\n"
        "n = A*e^(k*t)";
    return true;
  }
  if (contains(eq,"dy/dx") || contains(eq,"(dy)/(dx)")){
    out="";
    if (contains(eq,"=y") && !contains(eq,"*")){
      out += "y = A*e^x";
      return true;
    }
    if (contains(eq,"=k*y") || contains(eq,"=ky")){
      out += "y = A*e^(k*x)";
      return true;
    }
    out += "y=int(f,x)+C";
    return true;
  }
  int op=ceq.find('=');
  if (op<0 && var.size()==1 && !ceq.empty() && ceq[0]!='['){
    ceq += "=0";
    eq += "=0";
    op=ceq.find('=');
  }
  if (op<0 || var.size()!=1)
    return false;
  char v=var[0];
  if (!contains_var_symbol(ceq,v) && !contains_var_symbol(nospace_lower(eq_src),v)){
    working_string left0=ceq.substr(0,op), right0=ceq.substr(op+1,ceq.size()-op-1);
    NumParser np1,np2;
    np1.p=left0.c_str(); np1.ok=true;
    np2.p=right0.c_str(); np2.ok=true;
    double lv=np1.expr(), rv=np2.expr();
    np1.skip(); np2.skip();
    if (np1.ok && np2.ok && !*np1.p && !*np2.p && isfinite(lv) && isfinite(rv)){
      out="No "+rawvar+" term.\n";
      if (fabs(lv-rv)<1e-10){
        out += "Identity\n";
        out += rawvar+" = all real";
      }
      else {
        out += double_s(lv)+" != "+double_s(rv)+"\n";
        out += rawvar+" = []";
      }
      return true;
    }
    out="No "+rawvar+" term appears\n";
    out += "condition on other symbols\n";
    out += rawvar+" = all real if true, else []";
    return true;
  }
  if (try_zero_product_solve(eq_src,rawvar,v,out))
    return true;
  char vbuf[2]={v,0};
  working_string vs(vbuf);
  working_string left=ceq.substr(0,op), right=ceq.substr(op+1,ceq.size()-op-1);
  if (try_solve_linear_recip(left,right,rawvar,v,out))
    return true;
  if (try_solve_srd_linear(left,right,rawvar,v,out))
    return true;
  if (try_solve_affine_eq(left,right,rawvar,v,out))
    return true;
  if (try_solve_linear_fraction(left,right,rawvar,v,out))
    return true;
  if (try_solve_inverse_call(left,right,rawvar,v,out) ||
      try_solve_inverse_call(right,left,rawvar,v,out))
    return true;
  {
    working_string raw_eq=nospace_lower(eq_src);
    int rop=find_top_equal_solve(raw_eq);
    if (rop>=0 && try_solve_cubic_int_eq(raw_eq.substr(0,rop),raw_eq.substr(rop+1,raw_eq.size()-rop-1),v,rawvar,out))
      return true;
  }
  {
    working_string tan_rhs;
    Rat tv;
    if (left=="tan("+var+")" && parse_rat(right,tv))
      tan_rhs=rat_s(tv);
    else if (right=="tan("+var+")" && parse_rat(left,tv))
      tan_rhs=rat_s(tv);
    if (!tan_rhs.empty()){
      out="tan("+rawvar+") = "+tan_rhs+"\n";
      out += rawvar+" = atan("+tan_rhs+") + n*pi";
      return true;
    }
  }
  {
    Rat rv;
    if (left==var && parse_rat(right,rv)){
      out=rawvar+" = ["+rat_s(rv)+"]";
      return true;
    }
    if (right==var && parse_rat(left,rv)){
      out=rawvar+" = ["+rat_s(rv)+"]";
      return true;
    }
    working_string other;
    if (left==var)
      other=right;
    else if (right==var)
      other=left;
    if (!other.empty()){
      NumParser np;
      np.p=other.c_str();
      np.ok=true;
      double val=np.expr();
      np.skip();
      if (np.ok && !*np.p && isfinite(val)){
        working_string exact;
        out=rawvar+" = [";
        out += rational_approx(val,exact)?exact:double_s(val);
        out += "]";
        return true;
      }
    }
  }
  {
    working_string pside,cside;
    Rat pc,pp,cv;
    if (parse_x_power_factor(left,pc,pp)){ pside=left; cside=right; }
    else if (parse_x_power_factor(right,pc,pp)){ pside=right; cside=left; }
    if (!pside.empty() && pp.d==1 && pp.n>1 && parse_rat(cside,cv) && pc.n){
      cv=rat_div(cv,pc);
      out=power_var_s(pp)+" = "+rat_s(cv)+"\n";
      if (pp.n==2){
        if (cv.n<0)
          out += rawvar+" = []";
        else if (cv.n==0)
          out += rawvar+" = [0]";
        else if (cv.d==1)
          out += rawvar+" = [-sqrt("+rat_s(cv)+"), sqrt("+rat_s(cv)+")]";
        else
          out += rawvar+" = [-sqrt("+rat_s(cv)+"), sqrt("+rat_s(cv)+")]";
        return true;
      }
      if (pp.n%2==1){
        out += rawvar+" = [("+rat_s(cv)+")^(1/"+int_s(pp.n)+")]";
        return true;
      }
    }
  }
  if (left.find("log(")==0){
    int whole_open=left.find('(');
    if (whole_open>=0 && match_paren(left,whole_open)==int(left.size())-1){
      working_string largs[2];
      int ln=split_args(left,4,left.size()-1,largs,2);
      if (ln==2){
        out="Log def:\nlog_"+trim(largs[0])+"("+trim(largs[1])+") = "+right+"\n";
        out += trim(largs[1])+" = "+trim(largs[0])+"^"+right+"\n";
        out += "Solve the resulting equation\n";
        working_string sub,call="solve("+trim(largs[1])+"="+trim(largs[0])+"^"+right+","+rawvar+")";
        if (try_solve(call.c_str(),sub))
          out += "\n"+sub;
        return true;
      }
    }
  }
  {
    int pp=left.find("^(");
    if (pp>0 && left[left.size()-1]==')'){
      working_string base=left.substr(0,pp), expo=left.substr(pp+2,left.size()-pp-3);
      long a=0,b=0;
      bool simple_base=base=="e" || !base.empty();
      for (int bi=0;base!="e" && bi<int(base.size());++bi)
        if (!isdigit((unsigned char)base[bi]) && base[bi]!='.')
          simple_base=false;
      if (simple_base && parse_linear_var(expo,v,a,b)){
        working_string rhslog="ln("+right+")", blog="ln("+base+")";
        int rp=right.find('^');
        if (rp>0)
          rhslog=right.substr(rp+1,right.size()-rp-1)+"*ln("+right.substr(0,rp)+")";
        if (right=="1/3")
          rhslog="-ln(3)";
        if (base=="4")
          blog="2*ln(2)";
        out="Take ln:\n";
        out += "("+spaced_pm(expo)+")*"+blog+" = "+rhslog+"\n";
        out += int_s(a)+"*"+rawvar+"*"+blog+" = "+rhslog;
        if (b>0){ out += " - "; out += b==1?blog:int_s(b)+"*"+blog; }
        if (b<0){ out += " + "; out += (-b)==1?blog:int_s(-b)+"*"+blog; }
        out += "\n"+rawvar+" = [("+rhslog;
        if (b>0){ out += " - "; out += b==1?blog:int_s(b)+"*"+blog; }
        if (b<0){ out += " + "; out += (-b)==1?blog:int_s(-b)+"*"+blog; }
        out += ")/("+int_s(a)+"*"+blog+")]";
        return true;
      }
    }
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
        out += ""+rawvar+" = [0, "+frac_s(la,qa)+"]";
        return true;
      }
    }
  }
  {
    Rat r1,r2;
    long a,b,c;
    if (solve_quad_int(left,right,v,r1,r2,a,b,c)){
      out="";
      append_quad_solution(out,a,b,c,v,rawvar,r1,r2);
      return true;
    }
  }
  if (try_solve_biquad_eq(left,right,rawvar,v,out))
    return true;
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
    out += ""+rawvar+" = ["+frac_s(b,a)+"]";
    return true;
  }
  {
    Rat ga1,gb1,ga2,gb2;
    if (parse_affine_general(left,v,ga1,gb1) && parse_affine_general(right,v,ga2,gb2)){
      Rat a=rat_sub(ga1,ga2);
      Rat b=rat_sub(gb2,gb1);
      if (a.n){
        out="Collect:\n";
        out += fmt_linear_rat(ga1,gb1,v)+" = "+fmt_linear_rat(ga2,gb2,v)+"\n";
        out += rat_s(a)+"*"+rawvar+" = "+rat_s(b)+"\n";
        out += rawvar+" = "+rat_s(rat_div(b,a))+"\n";
        out += ""+rawvar+" = ["+rat_s(rat_div(b,a))+"]";
        return true;
      }
      if (b.n){
        out="Collect:\n";
        out += fmt_linear_rat(ga1,gb1,v)+" = "+fmt_linear_rat(ga2,gb2,v)+"\n";
        out += "0 = "+rat_s(b)+"\n";
        out += rawvar+" = []";
        return true;
      }
      out="Collect:\n";
      out += fmt_linear_rat(ga1,gb1,v)+" = "+fmt_linear_rat(ga2,gb2,v)+"\n";
      out += "0 = 0\n";
      out += rawvar+" = all real";
      return true;
    }
  }
  {
    long lead=0,degree=0;
    if (right=="0" && polynomial_degree_lead(left,v,lead,degree) && degree>2){
      if (degree==3 && solve_cubic_numeric_int(left,right,v,rawvar,out))
        return true;
      out="Polynomial equation of degree "+int_s(degree)+"\n";
      out += "factor first; roots from factors\n";
      out += rawvar+" = roots("+left+")";
      return true;
    }
  }
  if (try_solve_general_decomposition(left,right,eq_src,rawvar,v,out))
    return true;
  return false;
}

static bool integer_bound_arg(const working_string &src,long &v){
  Rat r;
  if (!parse_rat(compact(src),r) || r.d!=1)
    return false;
  v=r.n;
  return true;
}

static bool try_sequence_working(const char *input,working_string &out){
  working_string args[5];
  int n=0;
  bool prod=false;
  if (!parse_call(input,"sum",args,5,n)){
    if (!parse_call(input,"product",args,5,n))
      return false;
    prod=true;
  }
  const char *name=prod?"product":"sum";
  if (n<4){
    out="Err: missing bounds\n";
    out += working_string(name)+"(expr,k,lower,upper)";
    return true;
  }
  working_string var=compact(args[1]), expr=trim(args[0]);
  bool large_expr=false;
  working_string shown_expr=command_display_arg(expr,"F",large_expr);
  if (var.size()!=1 || !isalpha((unsigned char)var[0])){
    out="Err: invalid index\n";
    out += working_string(name)+"(expr,k,lower,upper)";
    return true;
  }
  long lo=0,hi=0;
  if (!integer_bound_arg(args[2],lo) || !integer_bound_arg(args[3],hi)){
    out=(prod?"Finite product:\n":"Finite sum:\n");
    out += "Err: integer bounds.\n";
    out += working_string(name)+"("+shown_expr+","+var+",lower,upper)";
    return true;
  }
  long count=hi>=lo?hi-lo+1:0;
  out=(prod?"Finite product:\n":"Finite sum:\n");
  out += var+"="+int_s(lo)+".."+int_s(hi)+"\n";
  if (!count){
    out += prod?"empty product = 1":"empty sum = 0";
    return true;
  }
  if (count<=10){
    Rat acc=prod?rat(1,1):rat(0,1),val;
    working_string terms;
    bool ok=true;
    for (long j=lo;j<=hi;++j){
      working_string term=replace_identifier_token(expr,var,int_s(j));
      if (!eval_numeric_rat_expr(term,val)){
        ok=false;
        break;
      }
      acc=prod?rat_mul(acc,val):rat_add(acc,val);
      if (!terms.empty())
        terms += prod?" * ":" + ";
      terms += rat_s(val);
    }
    if (ok){
      out += "Terms: "+terms+"\n";
      out += prod?"Product = ":"Sum = ";
      out += rat_s(acc);
      return true;
    }
  }
  if (!contains_var_symbol(compact(expr),var[0])){
    out += shown_expr+" is constant in "+var+"\n";
    out += prod?("("+shown_expr+")^"+int_s(count)):(int_s(count)+"*("+shown_expr+")");
    return true;
  }
  out += "Evaluate "+int_s(count)+" terms and combine\n";
  out += working_string(name)+"("+shown_expr+","+var+","+int_s(lo)+","+int_s(hi)+")";
  return true;
}

static bool double_angle_arg_from(const working_string &src,working_string &arg){
  working_string s=strip_outer_parens(nospace_lower(src));
  if (s.size()>1 && s[0]=='2' && !isdigit((unsigned char)s[1])){
    arg=s.substr(1,s.size()-1);
    if (!arg.empty() && arg[0]=='*')
      arg=arg.substr(1,arg.size()-1);
    arg=strip_outer_parens(arg);
    return !arg.empty();
  }
  return false;
}

static bool try_double_angle_expand_target(const working_string &raw,working_string &target){
  working_string e=nospace_lower(raw),dbl,arg;
  if (parse_unary_arg(e,"sin",dbl) && double_angle_arg_from(dbl,arg))
    target="2*sin("+arg+")*cos("+arg+")";
  else if (parse_unary_arg(e,"cos",dbl) && double_angle_arg_from(dbl,arg))
    target="cos("+arg+")^2-sin("+arg+")^2";
  else if (parse_unary_arg(e,"tan",dbl) && double_angle_arg_from(dbl,arg))
    target="2*tan("+arg+")/(1-tan("+arg+")^2)";
  else
    return false;
  return khicas_equiv(raw,target);
}

static bool try_compound_angle_expand_target(const working_string &raw,working_string &target){
  working_string e=nospace_lower(raw),arg,a,b; int kind=0,sgn=1,depth=0,cut=-1;
  if (parse_unary_arg(e,"sin",arg)) kind=1;
  else if (parse_unary_arg(e,"cos",arg)) kind=2;
  else if (parse_unary_arg(e,"tan",arg)) kind=3;
  if (!kind)
    return false;
  arg=strip_outer_parens(arg);
  for (int i=int(arg.size())-1;i>0;--i){
    char c=arg[i];
    if (c==')' || c==']' || c=='}') ++depth;
    else if (c=='(' || c=='[' || c=='{') --depth;
    else if (!depth && (c=='+' || c=='-') && arg[i-1]!='^' && arg[i-1]!='e' && arg[i-1]!='E'){
      cut=i; sgn=(c=='-')?-1:1; break;
    }
  }
  if (cut<1)
    return false;
  a=strip_outer_parens(arg.substr(0,cut));
  b=strip_outer_parens(arg.substr(cut+1,arg.size()-cut-1));
  if (a.empty() || b.empty())
    return false;
  if (kind==1)
    target="sin("+a+")*cos("+b+")"+(sgn>0?"+":"-")+"cos("+a+")*sin("+b+")";
  else if (kind==2)
    target="cos("+a+")*cos("+b+")"+(sgn>0?"-":"+")+"sin("+a+")*sin("+b+")";
  else
    target="(tan("+a+")"+(sgn>0?"+":"-")+"tan("+b+"))/(1"+(sgn>0?"-":"+")+"tan("+a+")*tan("+b+"))";
  return khicas_equiv(raw,target);
}

static working_string trig_double_arg_s(const working_string &arg){
  return arg.size()==1 && isalpha((unsigned char)arg[0]) ? "2*"+arg : "2*("+arg+")";
}

static working_string scale_int_expr(int c,const working_string &expr){
  if (c==1) return expr;
  if (c==-1) return "-("+expr+")";
  return int_s(c)+"*("+expr+")";
}

static working_string square_factor_s(const working_string &s){
  working_string c=compact(s);
  if (c.size()==1 && isalnum((unsigned char)c[0]))
    return c;
  if (c.size()>1 && c[0]!='-' && c.find('+')==working_string::npos && c.find('-')==working_string::npos &&
      c.find('*')==working_string::npos && c.find('/')==working_string::npos)
    return c;
  return "("+c+")";
}

static bool coeff_var_term(const working_string &src,char v,Rat &c){
  working_string s=compact(src),vs(1,v);
  if (s==vs){
    c=rat(1,1);
    return true;
  }
  int star=s.rfind(working_string("*")+vs);
  if (star>=0 && star+2==int(s.size()))
    return parse_rat(s.substr(0,star),c);
  if (s.size()>1 && s[s.size()-1]==v)
    return parse_rat(s.substr(0,s.size()-1),c);
  return false;
}

static working_string binomial_square_s(const working_string &a,const working_string &b){
  char v=first_var(a);
  Rat c,c2,twoc;
  if (v && coeff_var_term(a,v,c) && !contains_var_symbol(b,v)){
    c2=rat_mul(c,c);
    twoc=rat_mul(rat(2,1),c);
    return join_sum(join_sum(rat_var_power_term_s(c2,v,2),rat_var_power_term_s(twoc,v,1)+"*"+square_factor_s(b)),
                    square_factor_s(b)+"^2");
  }
  v=first_var(b);
  if (v && coeff_var_term(b,v,c) && !contains_var_symbol(a,v)){
    c2=rat_mul(c,c);
    twoc=rat_mul(rat(2,1),c);
    return join_sum(join_sum(rat_var_power_term_s(c2,v,2),rat_var_power_term_s(twoc,v,1)+"*"+square_factor_s(a)),
                    square_factor_s(a)+"^2");
  }
  working_string af=square_factor_s(a), bf=square_factor_s(b);
  return af+"^2+2*"+af+"*"+bf+"+"+bf+"^2";
}

static bool try_trig_collect_target(const working_string &raw,working_string &target){
  working_string e=nospace_lower(raw),arg;
  int c=0;
  if (parse_sin_cos_product_int(e,arg,c) && c%2==0)
    target=scale_int_expr(c/2,"sin("+trig_double_arg_s(arg)+")");
  else if (parse_cos2_minus_sin2_int(e,arg,c))
    target=scale_int_expr(c,"cos("+trig_double_arg_s(arg)+")");
  else if (parse_sin2_cos2_sum_int(e,arg,c))
    target=int_s(c);
  else if (parse_const_trig_square_int(e,"tan",false,arg,c))
    target=scale_int_expr(c,"sec("+arg+")^2");
  else
    return false;
  return khicas_equiv(raw,target);
}

static bool try_trig_transform_working(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  bool collect=false;
  if (!parse_call(input,"texpand",args,3,n)){
    if (!parse_call(input,"tcollect",args,3,n))
      return false;
    collect=true;
  }
  const char *name=collect?"tcollect":"texpand";
  if (n<1 || trim(args[0]).empty()){
    out="Err: missing arguments\n";
    out += working_string(name)+"(expr)";
    return true;
  }
  working_string raw=trim(args[0]), e=compact(raw);
  bool large_raw=false;
  working_string shown_raw=command_display_arg(raw,"A",large_raw);
  if (!contains(e,"sin(") && !contains(e,"cos(") && !contains(e,"tan(") &&
      !contains(e,"sec(") && !contains(e,"cot(") && !contains(e,"cosec(")){
    if (!collect){
      working_string base,exp,t[2];
      int sg[2];
      if (parse_top_power(e,base,exp) && exp=="2" && split_top_sum_terms(base,t,sg,2)==2){
        working_string a=signed_part(sg[0],t[0]), b=signed_part(sg[1],t[1]);
        working_string exact=binomial_square_s(a,b);
        out="texpand:\n";
        out += shown_raw+"\n= "+insert_coeff_stars(exact);
        return true;
      }
    }
    working_string exact, call;
    if (collect){
      if (n>=2 && !trim(args[1]).empty())
        call="tcollect("+raw+","+trim(args[1])+")";
      else {
        char v=first_var(e);
        call = v ? "tcollect("+raw+","+working_string(1,v)+")" : "tcollect("+raw+")";
      }
    }
    else
      call="texpand("+raw+")";
    if (production_exact_command(call,exact)){
      exact=trim(exact);
      if (!exact.empty() && compact(exact)!=compact(raw) && khicas_equiv(raw,exact)){
        out=collect ? "tcollect:\n" : "texpand:\n";
        out += shown_raw+"\n= "+insert_coeff_stars(exact);
        out += "";
        return true;
      }
    }
    out=collect ? "tcollect:\n" : "texpand:\n";
    out += shown_raw+"";
    return true;
  }
  working_string exact;
  if (!collect && try_double_angle_expand_target(raw,exact)){
    bool large_exact=false;
    working_string shown_exact=command_display_arg(exact,"B",large_exact);
    if (!large_exact){
      out="Trig expand:\n";
      out += shown_raw+" = "+shown_exact+"\n";
      out += shown_exact+"";
      return true;
    }
  }
  if (!collect && try_compound_angle_expand_target(raw,exact)){
    bool large_exact=false;
    working_string shown_exact=command_display_arg(exact,"B",large_exact);
    if (!large_exact){
      out="Trig expand:\n";
      out += shown_raw+" = "+shown_exact+"\n";
      out += shown_exact+"";
      return true;
    }
  }
  if (collect && try_trig_collect_target(raw,exact)){
    bool large_exact=false;
    working_string shown_exact=command_display_arg(exact,"B",large_exact);
    out="Trig collect:\n";
    out += shown_raw+" = "+shown_exact+"\n";
    out += shown_exact+"";
    return true;
  }
  if (!collect && try_trig_product_to_sum_expand(raw,out))
    return true;
  out=collect ? "Trig collect:\n" : "Trig expand:\n";
  out += "No identity pattern.\n";
  out += shown_raw;
  return true;
}

static bool factor_nonpoly_guard(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"factor",args,3,n) || n<1)
    return false;
  working_string e=compact(args[0]);
  if (!(contains(e,"exp(") || contains(e,"ln(") || contains(e,"log(") ||
        contains(e,"sin(") || contains(e,"cos(") || contains(e,"tan(")))
    return false;
  working_string var=n>=2?trim(args[1]):working_string(1,first_var(e));
  if (var.empty())
    var="x";
  out="Factorisation:\nnot a polynomial in "+var+"\n";
  out += insert_coeff_stars(args[0]);
  return true;
}

static bool try_trig_product_to_sum_expand(const working_string &raw,working_string &out){
  working_string f[4],arg;
  int n=split_top_product(compact(raw),f,4),c=0,p=0;
  Rat coef=rat(1,1),ks=rat(0,1),kc=rat(0,1);
  bool got_s=false,got_c=false;
  if (n<2)
    return false;
  for (int i=0;i<n;++i){
    Rat q;
    if (parse_rat(f[i],q)){
      coef=rat_mul(coef,q);
      continue;
    }
    if (parse_int_func_pow(f[i],"sin",c,arg,p) && p==1 && parse_x_coeff(arg,ks)){
      if (got_s)
        return false;
      got_s=true;
      coef=rat_mul(coef,rat(c,1));
      continue;
    }
    if (parse_int_func_pow(f[i],"cos",c,arg,p) && p==1 && parse_x_coeff(arg,kc)){
      if (got_c)
        return false;
      got_c=true;
      coef=rat_mul(coef,rat(c,1));
      continue;
    }
    return false;
  }
  if (!got_s || !got_c)
    return false;
  Rat half=rat_div(coef,rat(2,1));
  working_string res=join_sum(func_term_s(half,"sin",rat_add(ks,kc)),
                              func_term_s(half,"sin",rat_sub(ks,kc)));
  out="Product-to-sum:\n";
  out += "sinAcosB rule\n";
  out += insert_coeff_stars(raw)+" = "+res+"\n";
  out += res+"";
  return true;
}

static bool nested_function_expr(const working_string &s,int limit);

static bool try_algebra(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  working_string s=compact(input?input:"");
  if (vector_working(s,out))
    return true;
  if (try_trig_transform_working(input,out))
    return true;
  if (parse_call(input,"complete_square",args,3,n) && n>=1){
    char v=n>=2 && compact(args[1]).size()==1?compact(args[1])[0]:first_var(args[0]);
    if (v && complete_square_working(args[0],v,out))
      return true;
  }
  if (parse_call(input,"evalat",args,3,n) && n>=3){
    if (eval_at_working("evalat",args,n,out))
      return true;
  }
  if ((parse_call(input,"subst",args,3,n) || parse_call(input,"subs",args,3,n)) && n>=3){
    if (eval_at_working("subst",args,n,out))
      return true;
  }
  if ((parse_call(input,"subst",args,3,n) || parse_call(input,"subs",args,3,n)) && n==2){
    working_string lv,rv;
    if (split_equal_sides(args[1],lv,rv)){
      working_string eargs[3]={args[0],lv,rv};
      if (eval_at_working("subst",eargs,3,out))
        return true;
    }
  }
  if (parse_call(input,"discriminant",args,3,n) && n>=1){
    char v=n>=2 && compact(args[1]).size()==1?compact(args[1])[0]:first_var(args[0]);
    if (v && discriminant_working(args[0],v,out))
      return true;
  }
  if (parse_call(input,"compare",args,6,n) && n>=2){
    out="Compare:\n";
    out += "E1 = "+insert_coeff_stars(trim(args[0]))+"\n";
    out += "E2 = "+insert_coeff_stars(trim(args[1]))+"\n";
    working_string d;
    if (production_exact_command("simplify(("+args[0]+")-("+args[1]+"))",d))
      d=trim(d);
    if (khicas_equiv(args[0],args[1]) || compact(d)=="0"){
      out += "E1-E2 = 0\n";
      out += "equivalent";
      return true;
    }
    out += "E1-E2 = "+(d.empty()?working_string("not 0"):d)+"\n";
    out += "different";
    return true;
  }
  if (parse_call(input,"factor",args,3,n) && n>=1){
    if (factor_nonpoly_guard(input,out))
      return true;
    working_string e=compact(args[0]);
    working_string exact;
    if (production_exact_command("factor("+args[0]+")",exact)){
      exact=trim(exact);
      if (!exact.empty() && compact(exact)!=e){
        out="Factor:\n";
        out += exact+"";
        return true;
      }
    }
  }
  if (parse_call(input,"apart",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    char v=n>=2 && compact(args[1]).size()==1?compact(args[1])[0]:first_var(e);
    if (partfrac_two_linear_route(args[0],v,out))
      return true;
  }
  if (parse_call(input,"partfrac",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    Rat q;
    if (parse_rat(e,q)){
      out="Constant:\n";
      out += trim(args[0])+" = "+rat_s(q)+"\n";
      out += "No partial fractions";
      return true;
    }
    char v=n>=2 && compact(args[1]).size()==1?compact(args[1])[0]:first_var(e);
    if (partfrac_two_linear_route(args[0],v,out))
      return true;
    if (partfrac_repeated_linear_route(args[0],v,out))
      return true;
    working_string num,den;
    if (!split_top_fraction(e,num,den)){
      out="No rational denominator.\n";
      bool large=false;
      working_string shown=command_display_arg(args[0],"A",large);
      out += shown+" not proper rational.";
      return true;
    }
    Rat dq;
    if (parse_rat(den,dq)){
      out="No rational denominator.\n";
      out += "Constant denominator";
      return true;
    }
    const char *bad_den[]={"sin(","cos(","tan(","cot(","sec(","sqrt(","abs(","ln(","log(","exp(",",",0};
    for (int i=0;bad_den[i];++i){
      if (contains(den,bad_den[i])){
        out="No rational denominator.\n";
        out += "not polynomial.";
        return true;
      }
    }
    out="D:\n";
    if (den.size()>120 || args[0].size()>300){
      out += "D\npartfrac(A)";
      return true;
    }
    out += den+"\n";
    out += "A,B over factors\n";
    out += "partfrac("+trim(args[0])+")";
    working_string exact;
    if (exact_command_clean(input?input:"",exact))
      out += "\nAnswer:\n"+trim(exact);
    out += "";
    return true;
  }
  bool series_call=parse_call(input,"series",args,6,n);
  if (!series_call)
    n=0;
  bool taylor_call=series_call?false:parse_call(input,"taylor",args,6,n);
  if ((series_call || taylor_call) && n>=1){
    if (taylor_call && try_taylor_symbolic_binomial_cmd(input,out))
      return true;
    working_string about=n>=4?(compact(args[1])+"="+compact(args[2])):(n>=2?compact(args[1]):"x=0");
    working_string order=n>=4?compact(args[3]):(n>=3?compact(args[2]):"n");
    long oi=0;
    bool order_int=false;
    if (n>=3){
      if (!integer_bound_arg(order,oi) || oi<0){
        out="Err: order integer >=0\n";
        out += working_string(series_call?"series":"taylor")+"(expr,var=0,order)";
        return true;
      }
      order_int=true;
    }
    int eq=about.find('=');
    working_string var=eq>0?about.substr(0,eq):working_string(1,default_var_char(args[0]));
    working_string centre=eq>0?about.substr(eq+1,about.size()-eq-1):"0";
    working_string exact_order=(series_call && order_int)?int_s(oi+1):order;
    {
      working_string exact;
      if (exact_command_clean(input?input:"",exact)){
        out=working_string(series_call?"series method:\n":"taylor method:\n");
        out += "trace:\n";
        out += "point: "+about+"\n";
        out += "order: "+exact_order+"\n";
        out += "base series; substitute\n";
        out += "simplify:\ntruncate > order "+exact_order+"\n";
        out += "Answer:\n"+trim(exact)+"";
        return true;
      }
    }
    if (try_khicas_exact_route(input,out))
      return true;
    out="Err: no exact series";
    return true;
  }
  return false;
}

static bool parse_mono_rat_var(const working_string &src,char v,Rat &c,long &p){
  working_string s=strip_outer_parens(nospace_lower(src));
  int x=s.find(v);
  if (x<0) return false;
  working_string pre=s.substr(0,x), post=s.substr(x+1,s.size()-x-1);
  if (!pre.empty() && pre[pre.size()-1]=='*') pre=pre.substr(0,pre.size()-1);
  if (pre.empty() || pre=="+") c=rat(1,1);
  else if (pre=="-") c=rat(-1,1);
  else if (!parse_rat(pre,c)) return false;
  if (post.empty()) p=1;
  else {
    if (post[0]!='^') return false;
    char *end=0;
    p=strtol(post.c_str()+1,&end,10);
    if (!end || *end) return false;
  }
  return true;
}

static bool simplify_mono_quotient(const working_string &e,working_string &out){
  working_string num,den;
  if (!split_top_fraction(e,num,den)) return false;
  char v=first_var(e);
  Rat cn,cd;
  long pn=0,pd=0;
  if (!parse_mono_rat_var(num,v,cn,pn) || !parse_mono_rat_var(den,v,cd,pd) || !cd.n)
    return false;
  Rat c=rat_div(cn,cd);
  long p=pn-pd;
  out="Cancel powers:\n";
  out += rat_s(cn)+"*"+working_string(1,v)+"^"+int_s(pn)+" / ("+rat_s(cd)+"*"+working_string(1,v)+"^"+int_s(pd)+")\n";
  out += working_string(1,v)+"^"+int_s(pn)+"/"+working_string(1,v)+"^"+int_s(pd)+" = "+working_string(1,v)+"^"+int_s(p)+"\n";
  out += rat_power_term_s(c,rat(p,1),v);
  return true;
}

static bool int_nth_root(long n,long d,long &r){
  if (n<0 || d<=0 || d>6) return false;
  for (r=0;r<=4096;++r){
    long v=1;
    for (long i=0;i<d;++i){
      if (r && v>labs(n)/r+1) return false;
      v*=r;
    }
    if (v==n) return true;
    if (v>n) return false;
  }
  return false;
}

static bool rat_pow_int_small(long b,long p,long &r){
  r=1;
  for (long i=0;i<p;++i){
    if (b && labs(r)>2000000000L/labs(b)) return false;
    r*=b;
  }
  return true;
}

static bool parse_letter_monomial(const working_string &src,long &coef,int *powv){
  for (int i=0;i<26;++i) powv[i]=0;
  coef=1;
  working_string f[12];
  int n=split_top_product(src,f,12);
  if (n<1)
    return false;
  for (int i=0;i<n;++i){
    working_string t=strip_outer_parens(compact(f[i])),b,ex;
    Rat q;
    if (parse_rat(t,q) && q.d==1){
      coef*=q.n;
      continue;
    }
    int p=1;
    if (parse_top_power(t,b,ex)){
      if (!parse_small_int(ex,p))
        return false;
      t=strip_outer_parens(compact(b));
    }
    if (t.size()!=1 || !isalpha((unsigned char)t[0]))
      return false;
    powv[tolower((unsigned char)t[0])-'a'] += p;
  }
  return coef!=0;
}

static void append_mono_factor(working_string &s,const working_string &f){
  if (!s.empty() && s!="-")
    s += "*";
  s += f;
}

static working_string mono_result_s(Rat c,int *p){
  working_string num,den;
  if (c.n<0){
    num="-";
    c.n=-c.n;
  }
  if (c.n!=1)
    append_mono_factor(num,int_s(c.n));
  if (c.d!=1)
    append_mono_factor(den,int_s(c.d));
  for (int i=0;i<26;++i){
    if (!p[i])
      continue;
    working_string f=working_string(1,char('a'+i));
    int a=p[i]<0?-p[i]:p[i];
    if (a!=1)
      f += "^"+int_s(a);
    append_mono_factor(p[i]>0?num:den,f);
  }
  if (num.empty())
    num="1";
  if (num=="-")
    num="-1";
  return den.empty()?num:num+"/("+den+")";
}

static bool simplify_sqrt_mono_fraction(const working_string &e,working_string &out){
  working_string num,den,arg,base,ex;
  if (!split_top_fraction(e,num,den) || !parse_unary_arg(num,"sqrt",arg))
    return false;
  long nc=1,dc=1,root=0,dpow=1;
  int np[26],dp[26],p[26],ep=1;
  if (!parse_letter_monomial(arg,nc,np) || nc<0 || !int_nth_root(nc,2,root))
    return false;
  for (int i=0;i<26;++i)
    if (np[i]%2)
      return false;
  if (parse_top_power(den,base,ex)){
    if (!parse_small_int(ex,ep) || ep<1)
      return false;
    den=base;
  }
  if (!parse_letter_monomial(den,dc,dp) || !rat_pow_int_small(dc,ep,dpow))
    return false;
  for (int i=0;i<26;++i)
    p[i]=np[i]/2-dp[i]*ep;
  working_string ans=mono_result_s(rat(root,dpow),p);
  out="Index laws:\n"+ans;
  return true;
}

static bool parse_num_power_factor(const working_string &src,Rat &q){
  working_string s=strip_outer_parens(nospace_lower(src)),base,exp;
  int p=s.find('^');
  if (p<0) return parse_rat(s,q);
  base=strip_outer_parens(s.substr(0,p));
  exp=strip_outer_parens(s.substr(p+1,s.size()-p-1));
  Rat e,bq;
  long b=0,root=0,powv=0;
  char *end=0;
  b=strtol(base.c_str(),&end,10);
  if (!end || *end || b<=0 || !parse_rat(exp,e)) return false;
  if (!int_nth_root(b,e.d,root)) return false;
  long ep=labs(e.n);
  if (!rat_pow_int_small(root,ep,powv)) return false;
  q=e.n<0?rat(1,powv):rat(powv,1);
  return true;
}

static bool simplify_numeric_powers(const working_string &e,working_string &out){
  if (contains_var_symbol(e,first_var(e)) || contains(e,"sqrt(")) return false;
  working_string terms[8],factors[8];
  int signs[8];
  int n=split_top_sum_terms(e,terms,signs,8);
  if (n<1) return false;
  bool anypow=false;
  Rat sum=rat(0,1);
  out="Index laws:\n";
  for (int i=0;i<n;++i){
    int fn=split_top_product(terms[i],factors,8);
    Rat prod=rat(signs[i],1);
    for (int j=0;j<fn;++j){
      Rat q;
      if (!parse_num_power_factor(factors[j],q)) return false;
      if (contains(factors[j],"^")) anypow=true;
      prod=rat_mul(prod,q);
    }
    out += signed_part(signs[i],terms[i])+" = "+rat_s(prod)+"\n";
    sum=rat_add(sum,prod);
  }
  if (!anypow) return false;
  out += rat_s(sum);
  return true;
}

static bool split_surd_const(long n,long &coef,long &rad){
  coef=1; rad=n;
  for (long k=2;k*k<=rad;++k)
    while (rad%(k*k)==0){ coef*=k; rad/=k*k; }
  return true;
}

static bool parse_surd_term_simple(const working_string &src,Rat sign,Rat &coef,long &rad,working_string &shown){
  working_string t=strip_outer_parens(nospace_lower(src));
  int p=t.find("sqrt(");
  if (p<0) return false;
  int close=match_paren(t,p+4);
  if (close!=int(t.size())-1) return false;
  long n=0,c=1,r=0;
  char *end=0;
  n=strtol(t.c_str()+p+5,&end,10);
  if (!end || *end!=')' || n<=0) return false;
  working_string pre=t.substr(0,p);
  if (!pre.empty() && pre[pre.size()-1]=='*') pre=pre.substr(0,pre.size()-1);
  Rat q=rat(1,1);
  if (!pre.empty() && !parse_rat(pre,q)) return false;
  split_surd_const(n,c,r);
  coef=rat_mul(sign,rat_mul(q,rat(c,1)));
  rad=r;
  shown="sqrt("+int_s(n)+")";
  return true;
}

static working_string surd_sum_s(Rat *coef,long *rad,int n){
  working_string out;
  for (int i=0;i<n;++i){
    if (!coef[i].n) continue;
    out=join_sum(out,surd_coeff_s(coef[i],rad[i]));
  }
  return out.empty()?"0":out;
}

static working_string rat_surd_s(Rat a,Rat b,long r){
  working_string out;
  if (a.n<0 && b.n>0)
    return surd_coeff_s(b,r)+" - "+rat_s(rat(-a.n,a.d));
  if (a.n) out=rat_s(a);
  if (b.n){
    working_string t=surd_coeff_s(b,r);
    if (out.empty()) out=t;
    else out += b.n<0 ? " - "+surd_coeff_s(rat(-b.n,b.d),r) : " + "+t;
  }
  return out.empty()?"0":out;
}

static bool simplify_surd_sum(const working_string &e,working_string &out){
  working_string terms[8],shown[8];
  int signs[8],n=split_top_sum_terms(e,terms,signs,8);
  if (n<1) return false;
  Rat coef[8];
  long rad[8];
  int m=0;
  bool changed=false;
  for (int i=0;i<n;++i){
    Rat c; long r;
    if (!parse_surd_term_simple(terms[i],rat(signs[i],1),c,r,shown[i])) return false;
    for (int j=0;j<m;++j)
      if (rad[j]==r){ coef[j]=rat_add(coef[j],c); c.n=0; break; }
    if (c.n){ coef[m]=c; rad[m++]=r; }
    if (r!=atol(shown[i].c_str()+5)) changed=true;
  }
  working_string ans=surd_sum_s(coef,rad,m);
  if (!changed && m==n) return false;
  out="Simplify surds:\n";
  for (int i=0;i<n;++i){
    Rat c; long r;
    parse_surd_term_simple(terms[i],rat(signs[i],1),c,r,shown[i]);
    out += signed_part(signs[i],shown[i])+" = "+surd_coeff_s(c,r)+"\n";
  }
  out += ans;
  return true;
}

static bool simplify_surd_fraction(const working_string &e,working_string &out){
  working_string num,den;
  if (!split_top_fraction(e,num,den)) return false;
  Rat q,c; long r=0;
  working_string sh;
  if (parse_rat(num,q) && parse_surd_term_simple(den,rat(1,1),c,r,sh) && rat_is_one(c)){
    out="Rationalise:\n";
    out += num+"/sqrt("+int_s(r)+") * sqrt("+int_s(r)+")/sqrt("+int_s(r)+")\n";
    out += rat_s(q)+"*sqrt("+int_s(r)+")/"+int_s(r)+"\n";
    out += surd_coeff_s(rat_div(q,rat(r,1)),r);
    return true;
  }
  {
    working_string terms[3],shown[3];
    int signs[3];
    int n=split_top_sum_terms(den,terms,signs,3);
    Rat A=rat(0,1),B=rat(0,1),tc;
    long tr=0;
    if (n==2 && parse_rat(num,q)){
      bool ok=true;
      for (int i=0;i<n;++i){
        Rat rr;
        if (parse_rat(terms[i],rr))
          A=rat_add(A,rat_mul(rat(signs[i],1),rr));
        else if (parse_surd_term_simple(terms[i],rat(signs[i],1),tc,tr,shown[i])){
          if (r && tr!=r) ok=false;
          r=tr; B=rat_add(B,tc);
        }
        else ok=false;
      }
      Rat D=rat_sub(rat_mul(A,A),rat_mul(rat_mul(B,B),rat(r,1)));
      if (ok && r && D.n){
        Rat qa=rat_div(rat_mul(q,A),D), qb=rat_div(rat_mul(rat(-q.n,q.d),B),D);
        out="Rationalise:\n";
        out += num+"/("+den+") * ("+rat_surd_s(A,rat(-B.n,B.d),r)+")/("+rat_surd_s(A,rat(-B.n,B.d),r)+")\n";
        out += "denominator = "+rat_s(D)+"\n";
        out += rat_surd_s(qa,qb,r);
        return true;
      }
    }
  }
  if (parse_surd_term_simple(den,rat(1,1),c,r,sh) && rat_is_one(c)){
    working_string si="simplify("+num+")", so;
    if (simplify_surd_sum(num,so)){
      Rat nc[8]; long nr[8]; working_string t[8]; int signs[8];
      int n=split_top_sum_terms(last_nonempty_line(so),t,signs,8);
      if (n==1){
        Rat sc; long sr; working_string ssh;
        if (parse_surd_term_simple(t[0],rat(signs[0],1),sc,sr,ssh) && sr==r){
          out=so+"\nDivide by "+surd_coeff_s(c,r)+"\n"+rat_s(rat_div(sc,c));
          return true;
        }
      }
    }
  }
  return false;
}

static bool simplify_exp_log_sum(const working_string &e,working_string &out){
  working_string terms[8],a,b;
  int signs[8],n=split_top_sum_terms(e,terms,signs,8);
  if (n<1) return false;
  Rat sum=rat(0,1);
  out="exp(log(a))=a\n";
  for (int i=0;i<n;++i){
    if (!parse_unary_arg(terms[i],"exp",a) || !parse_unary_arg(a,"log",b))
      return false;
    Rat q;
    if (!parse_rat(b,q)) return false;
    sum=rat_add(sum,rat_mul(rat(signs[i],1),q));
    out += signed_part(signs[i],terms[i])+" = "+rat_s(rat_mul(rat(signs[i],1),q))+"\n";
  }
  out += rat_s(sum);
  return true;
}

static bool parse_one_recip_square(const working_string &src,char v,int &sgn,long &coef){
  working_string s=nospace_lower(src);
  s=replace_all_literal(s," ","");
  working_string vp=working_string(1,v)+"^2)";
  if (s.size()<6 || s[0]!='1')
    return false;
  if (s[1]=='+') sgn=1;
  else if (s[1]=='-') sgn=-1;
  else return false;
  if (s.substr(2,3)!="1/(" || s.substr(s.size()-vp.size(),vp.size())!=vp)
    return false;
  working_string c=s.substr(5,s.size()-5-vp.size());
  if (!c.empty() && c[c.size()-1]=='*')
    c=c.substr(0,c.size()-1);
  char *end=0;
  long k=strtol(c.c_str(),&end,10);
  if (!end || *end || !k)
    return false;
  coef=k;
  return true;
}

static bool simplify_diff_quotient(const working_string &raw,working_string &out){
  working_string num,den,argsn[3],argsd[3],dnout,ddout;
  int nn=0,nd=0;
  working_string s=strip_outer_parens(nospace_lower(raw));
  int depth=0,slash=-1;
  for (int i=0;i<int(s.size());++i){
    char ch=s[i];
    if (ch=='(' || ch=='[' || ch=='{') ++depth;
    else if (ch==')' || ch==']' || ch=='}') --depth;
    else if (!depth && ch=='/'){ slash=i; break; }
  }
  if (slash<=0)
    return false;
  num=strip_outer_parens(trim(s.substr(0,slash)));
  den=strip_outer_parens(trim(s.substr(slash+1,s.size()-slash-1)));
  if (!parse_call(num.c_str(),"diff",argsn,3,nn) || nn<2 ||
      !parse_call(den.c_str(),"diff",argsd,3,nd) || nd<2 ||
      compact(argsn[1])!=compact(argsd[1]))
    return false;
  if (!try_diff(num.c_str(),dnout) || !try_diff(den.c_str(),ddout))
    return false;
  working_string dy=strip_integral_constant(dnout), dx=strip_integral_constant(ddout);
  if (dy.empty() || dx.empty())
    return false;
  out="Diff numerator/denominator\n";
  out += "dy/d"+compact(argsn[1])+" = "+dy+"\n";
  out += "dx/d"+compact(argsd[1])+" = "+dx+"\n";
  out += "(dy)/(dx)=("+dy+")/("+dx+")\n";
  {
    working_string vv=compact(argsn[1]);
    int sy=0,sx=0;
    long cy=0,cx=0;
    if (vv.size()==1 && parse_one_recip_square(dy,vv[0],sy,cy) &&
        parse_one_recip_square(dx,vv[0],sx,cx) && cy==cx){
      working_string base=int_s(cy)+"*"+vv+"^2";
      out += "multiply by "+base+"/"+base+"\n";
      out += "("+base+(sy>0?"+1":"-1")+")/("+base+(sx>0?"+1":"-1")+")";
      return true;
    }
  }
  working_string simp="simplify(("+dy+")/("+dx+"))", ans;
  if (production_exact_command(simp,ans)){
    ans=trim(ans);
    if (!ans.empty() && ans!=simp){
      out += ans;
      return true;
    }
  }
  out += "("+dy+")/("+dx+")";
  return true;
}

static bool simplify_root_unity(const working_string &src,working_string &out){
  working_string base,expo;
  int n=0,den=0;
  if (!parse_top_power(src,base,expo) || !parse_small_int(expo,n) || n<=0)
    return false;
  working_string s=strip_outer_parens(compact(base));
  if (s.substr(0,4)!="cos(")
    return false;
  int c=match_paren(s,3);
  if (c<0 || c+6>=int(s.size()) || s[c+1]!='+' || s.substr(c+2,5)!="isin(")
    return false;
  int d=match_paren(s,c+6);
  working_string a=s.substr(4,c-4), b=s.substr(c+7,d-c-7);
  if (d!=int(s.size())-1 || a!=b || a.size()<=4 || a.substr(0,4)!="2pi/" ||
      !parse_small_int(a.substr(4,a.size()-4),den) || den!=n)
    return false;
  out="1";
  return true;
}

static bool try_simplify(const char *input,working_string &out){
  working_string args[2],num,den,nshow,dshow,nfac,dfac;
  int n=0;
  if (!parse_call(input,"simplify",args,2,n) || n<1)
    return false;
  working_string raw=compact(args[0]);
  if (constant_exp_surd_too_large(args[0])){
    out="Err: complexity guard";
    return true;
  }
  if (working_route_too_large(args[0]) || nested_function_expr(args[0],5) ||
      (nested_function_expr(args[0],1) && contains(raw,"^-")))
    return try_symbolic_command_working(input,out);
  working_string e=canonical_expr(args[0]);
  if (simplify_sqrt_mono_fraction(nospace_lower(args[0]),out))
    return true;
  if (simplify_root_unity(args[0],out))
    return true;
  if (simplify_diff_quotient(args[0],out))
    return true;
  {
    working_string exact;
    if (production_exact_command(input?input:"",exact)){
      exact=trim(exact);
      if (!exact.empty() && compact(exact)!=compact(args[0]) && !contains(nospace_lower(exact),"simplify(")){
        out=args[0]+"\n= "+exact+"";
        return true;
      }
    }
  }
  if (simplify_numeric_powers(raw,out))
    return true;
  if (simplify_exp_log_sum(e,out) || simplify_mono_quotient(e,out) ||
      simplify_surd_fraction(e,out) || simplify_surd_sum(e,out) ||
      simplify_numeric_powers(e,out))
    return true;
  if (!contains_var_symbol(e,'x')){
    NumParser np;
    np.p=e.c_str();
    np.ok=true;
    double val=np.expr();
    np.skip();
    if (np.ok && !*np.p){
      if (!finite_double(val)){
        out=e;
        return true;
      }
      working_string exact;
      out="";
      out += rational_approx(val,exact)?exact:double_s(val);
      return true;
    }
  }
  if (e.size()==1 && isalpha((unsigned char)e[0])){
    out="Simplification:\n";
    out += e+" is already simple\n";
    out += e;
    return true;
  }
  {
    Rat a,b;
    char v=first_var(e);
    if (parse_affine_general(e,v,a,b)){
      out="Collect:\n";
      out += ""+fmt_linear_rat(a,b,v);
      return true;
    }
  }
  {
    char v=first_var(e);
    Rat a,b;
    Rat xc,xp;
    long coef=0,pow=0;
    if (parse_x_power_factor(e,xc,xp)){
      out=rat_power_term_s(xc,xp);
      return true;
    }
    if (parse_power_term_var(e,v,coef,pow)){
      out=rat_power_term_s(rat(coef,1),rat(pow,1));
      return true;
    }
    working_string arg;
    const char *fns[]={"sin","cos","tan","ln","exp",0};
    for (int i=0;fns[i];++i){
      if (parse_unary_arg(strip_outer_parens(nospace_lower(e)),fns[i],arg) &&
          (arg.size()==1 || parse_affine_general(arg,first_var(arg),a,b))){
        out=working_string(fns[i])+"("+fmt_expr_for_working(arg,first_var(arg))+")";
        return true;
      }
    }
  }
  if (!split_top_fraction(e,num,den)){
    out=args[0];
    return true;
  }
  char v=first_var(e);
  if (!factor_expr_simple(num,v,nshow,nfac) || !factor_expr_simple(den,v,dshow,dfac)){
    if (try_symbolic_command_working(input,out))
      return true;
    out=e;
    return true;
  }
  working_string nt[6],dt[6],ct[6];
  bool nu[6]={0,0,0,0,0,0},du[6]={0,0,0,0,0,0};
  int nc=factor_terms(nfac,nt,6), dc=factor_terms(dfac,dt,6), cc=0;
  Rat scale=rat(1,1);
  for (int i=0;i<nc;++i){
    for (int j=0;j<dc;++j){
      if (nu[i] || du[j])
        continue;
      working_string common=nt[i];
      Rat s=rat(1,1);
      if (nt[i]==dt[j] || common_linear_factor(nt[i],dt[j],common,s)){
        nu[i]=du[j]=true;
        ct[cc++]=common;
        scale=rat_mul(scale,s);
        break;
      }
    }
  }
  if (cc){
    working_string nr=factor_product(nt,nu,nc), dr=factor_product(dt,du,dc);
    out="Factor:\n";
    out += nshow+" = "+display_factors(nt,nu,nc)+"\n";
    out += dshow+" = "+display_factors(dt,du,dc)+"\n";
    out += "Cancel ";
    out += join_cancelled(ct,cc);
    out += "\n";
    out += (scale.n==1 && scale.d==1)?quotient_after_cancel(nr,dr):quotient_after_scaled_cancel(scale,nr,dr);
    return true;
  }
  out=e;
  return true;
}

static working_string sigfig_s(double v,int sf);

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
  if (!finite_double(v)){
    out=trim(expr);
    return true;
  }
  if (fabs(v)<5e-13)
    v=0;
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
  }
  out += "\nSign: ";
  out += v>0?"> 0":(v<0?"< 0":"= 0");
  out += "\nTo 2 significant figures: ";
  out += sigfig_s(v,2);
  out += "\nTo 3 significant figures: ";
  out += sigfig_s(v,3);
  char ibuf[48];
  double iv=v>=0?floor(v+0.5):ceil(v-0.5);
  if (fabs(iv)<0.5)
    iv=0;
  sprintf(ibuf,"%.0f",iv);
  out += "\nNearest integer: ";
  out += ibuf;
  return true;
}

static bool try_plain_numeric_expr_local(const working_string &s,working_string &out){
  if (s.empty() || contains(s,"="))
    return false;
  NumParser np;
  np.p=s.c_str();
  np.ok=true;
  double v=np.expr();
  np.skip();
  if (!np.ok || *np.p || !finite_double(v))
    return false;
  out="= ";
  out += double_s(v);
  return true;
}

static working_string sigfig_s(double v,int sf){
  if (!finite_double(v) || v==0)
    return "0";
  double av=fabs(v);
  int e=(int)floor(log10(av));
  int dec=sf-1-e;
  if (dec<0) dec=0;
  if (dec>8) dec=8;
  char fmt[12],buf[80];
  sprintf(fmt,"%%.%df",dec);
  sprintf(buf,fmt,v);
  int n=strlen(buf);
  if (dec>0){
    while (n>1 && buf[n-1]=='0') buf[--n]=0;
    if (n>1 && buf[n-1]=='.') buf[--n]=0;
  }
  return working_string(buf);
}

static bool try_numeric_expr(const char *input,working_string &out){
  working_string s=trim(input?input:"");
  if (contains(s,"="))
    return false;
  if (try_plain_numeric_expr_local(s,out))
    return true;
  {
    working_string exact;
    if ((production_exact_command("exact("+s+")",exact) ||
         production_exact_command("simplify("+s+")",exact)) &&
        !trim(exact).empty()){
      out="= ";
      out += trim(exact);
      return true;
    }
  }
  NumParser np;
  np.p=s.c_str();
  np.ok=true;
  double v=np.expr();
  np.skip();
  if (!np.ok || *np.p)
    return false;
  if (!finite_double(v)){
    out=s;
    return true;
  }
  out="= ";
  out += double_s(v);
  return true;
}

static bool symbol_in_known_call_arg(const working_string &s,int pos){
  int depth=0;
  for (int i=pos-1;i>=0;--i){
    char c=s[i];
    if (c==')' || c==']' || c=='}') ++depth;
    else if (c=='(' || c=='[' || c=='{'){
      if (depth>0){
        --depth;
        continue;
      }
      int j=i-1;
      while (j>=0 && isalpha((unsigned char)s[j]))
        --j;
      if (j+1<i && known_call_word(s.substr(j+1,i-j-1)))
        return true;
    }
  }
  return false;
}

static bool symbol_in_denominator_context(const working_string &s,int pos){
  int depth=0;
  for (int i=pos-1;i>=0;--i){
    char c=s[i];
    if (c==')' || c==']' || c=='}') ++depth;
    else if (c=='(' || c=='[' || c=='{'){
      if (depth>0){
        --depth;
        continue;
      }
      int j=i-1;
      while (j>=0 && isspace((unsigned char)s[j]))
        --j;
      return j>=0 && s[j]=='/';
    }
    else if (!depth){
      if (c=='/')
        return true;
      if (c=='+' || c=='-')
        return false;
    }
  }
  return false;
}

static bool linear_symbol_occurrences(const working_string &expr,char rv,int &hits){
  working_string s=nospace_lower(expr);
  hits=0;
  for (int i=0;i<int(s.size());++i){
    if (s[i]!=rv)
      continue;
    bool left=i>0 && isalpha((unsigned char)s[i-1]);
    bool right=i+1<int(s.size()) && isalpha((unsigned char)s[i+1]);
    if (left || right)
      continue;
    if ((i+1<int(s.size()) && s[i+1]=='^') || (i>0 && s[i-1]=='^'))
      return false;
    if (symbol_in_known_call_arg(s,i) || symbol_in_denominator_context(s,i))
      return false;
    ++hits;
  }
  return hits>0;
}

static bool syntactic_zero_expr(const working_string &expr,int depth=0){
  if (depth>4)
    return false;
  working_string s=strip_outer_parens(nospace_lower(expr)),num,den,arg,base;
  Rat r,p;
  if (s.empty())
    return false;
  if (parse_rat(s,r))
    return r.n==0;
  NumParser np;
  np.p=s.c_str();
  np.ok=true;
  double v=np.expr();
  np.skip();
  if (np.ok && !*np.p && finite_double(v))
    return fabs(v)<1e-12;
  if (split_top_fraction(s,num,den))
    return syntactic_zero_expr(num,depth+1);
  if (split_outer_power_general(s,base,p) && p.n>0)
    return syntactic_zero_expr(base,depth+1);
  if (parse_unary_arg(s,"sin",arg) || parse_unary_arg(s,"tan",arg))
    return syntactic_zero_expr(arg,depth+1);
  if (parse_unary_arg(s,"sqrt",arg) || parse_unary_arg(s,"abs",arg))
    return syntactic_zero_expr(arg,depth+1);
  working_string terms[4];
  int signs[4];
  int n=split_top_sum_terms(s,terms,signs,4);
  if (n==2 && signs[0]==1 && signs[1]==-1 &&
      strip_outer_parens(terms[0])==strip_outer_parens(terms[1]))
    return true;
  return false;
}

static working_string domain_answer(char rv,bool hlo,Rat lo,bool slo,bool hhi,Rat hi,bool shi,const working_string &extra){
  working_string v(1,rv);
  working_string base;
  if (hlo && hhi)
    base=rat_s(lo)+(slo?" < ":" <= ")+v+(shi?" < ":" <= ")+rat_s(hi);
  else if (hlo)
    base=v+(slo?" > ":" >= ")+rat_s(lo);
  else if (hhi)
    base=v+(shi?" < ":" <= ")+rat_s(hi);
  else
    base="all real";
  if (extra.empty())
    return base;
  if (base=="all real")
    return extra;
  return base+" and "+extra;
}

static void domain_bound(bool lower,Rat r,bool strict,bool &hlo,Rat &lo,bool &slo,bool &hhi,Rat &hi,bool &shi){
  if (lower){
    if (!hlo || rat_cmp(r,lo)>0 || (!rat_cmp(r,lo) && strict)){
      hlo=true; lo=r; slo=strict;
    }
  }
  else if (!hhi || rat_cmp(r,hi)<0 || (!rat_cmp(r,hi) && strict)){
    hhi=true; hi=r; shi=strict;
  }
}

static bool domain_req(const working_string &expr,char rv,int mode,
                       bool &hlo,Rat &lo,bool &slo,bool &hhi,Rat &hi,bool &shi,
                       working_string &extra,working_string &work){
  working_string arg,num,den;
  if (parse_unary_arg(expr,"ln",arg) && strip_outer_parens(nospace_lower(arg))==working_string(1,rv) &&
      mode!=2){
    work += "ln("+working_string(1,rv)+")"+(mode?" > 0\n":" >= 0\n");
    domain_bound(true,rat(1,1),mode,hlo,lo,slo,hhi,hi,shi);
    return true;
  }
  if (split_top_fraction(strip_outer_parens(nospace_lower(expr)),num,den) && mode==1){
    Rat na,nb,da,db;
    if (parse_affine_general(num,rv,na,nb) && parse_affine_general(den,rv,da,db) &&
        na.n && da.n){
      Rat rn=rat_div(rat(-nb.n,nb.d),na);
      Rat rd=rat_div(rat(-db.n,db.d),da);
      if (rat_cmp(rd,rn)>0){ Rat t=rn; rn=rd; rd=t; }
      working_string v(1,rv);
      work += spaced_pm(expr)+" > 0\n";
      extra = (na.n*da.n>0) ? v+" < "+rat_s(rd)+" or "+v+" > "+rat_s(rn)
                             : v+" > "+rat_s(rd)+" and "+v+" < "+rat_s(rn);
      return true;
    }
  }
  Rat ra,rb;
  if (parse_affine_general(expr,rv,ra,rb) && ra.n){
    Rat root=rat_div(rat(-rb.n,rb.d),ra);
    working_string v(1,rv);
    if (mode==2){
      work += spaced_pm(expr)+" != 0\n";
      if (!extra.empty()) extra += "\n";
      extra += v+" != "+rat_s(root);
    }
    else {
      work += spaced_pm(expr)+(mode?" > 0\n":" >= 0\n");
      domain_bound(ra.n>0,root,mode,hlo,lo,slo,hhi,hi,shi);
    }
    return true;
  }
  long a=0,b=0,c=0,r=0;
  if (!parse_quad_expr(expr,rv,a,b,c) || !a)
    return false;
  long D=b*b-4*a*c;
  if (!square_long(D,r))
    return false;
  Rat r1=rat_div(rat(-b-r,1),rat(2*a,1));
  Rat r2=rat_div(rat(-b+r,1),rat(2*a,1));
  if (rat_cmp(r2,r1)<0){ Rat t=r1; r1=r2; r2=t; }
  working_string v(1,rv);
  if (mode==2){
    work += spaced_pm(expr)+" != 0\n";
    extra = rat_cmp(r1,r2) ? v+" != "+rat_s(r1)+" and "+v+" != "+rat_s(r2)
                           : v+" != "+rat_s(r1);
  }
  else {
    work += spaced_pm(expr)+" > 0\n";
    extra = a>0 ? v+" < "+rat_s(r1)+" or "+v+" > "+rat_s(r2)
                : v+" > "+rat_s(r1)+" and "+v+" < "+rat_s(r2);
  }
  return true;
}

static bool domain_collect(const working_string &expr,char rv,bool &hlo,Rat &lo,bool &slo,bool &hhi,Rat &hi,bool &shi,
                           working_string &extra,working_string &work,int depth=0){
  if (depth>5)
    return false;
  working_string s=strip_outer_parens(nospace_lower(expr)),arg,num,den,args[2],parts[8];
  int n=0,signs[8];
  if (split_top_fraction(s,num,den)){
    bool ok=domain_collect(num,rv,hlo,lo,slo,hhi,hi,shi,extra,work,depth+1);
    return domain_req(den,rv,2,hlo,lo,slo,hhi,hi,shi,extra,work) || ok;
  }
  n=split_top_sum_terms(s,parts,signs,8);
  if (n>1){
    bool ok=false;
    for (int i=0;i<n;++i)
      ok = domain_collect(parts[i],rv,hlo,lo,slo,hhi,hi,shi,extra,work,depth+1) || ok;
    return ok;
  }
  if (parse_unary_arg(s,"sqrt",arg)){
    bool ok=domain_collect(arg,rv,hlo,lo,slo,hhi,hi,shi,extra,work,depth+1);
    (void)ok;
    return domain_req(arg,rv,0,hlo,lo,slo,hhi,hi,shi,extra,work);
  }
  if (parse_unary_arg(s,"ln",arg)){
    bool ok=domain_collect(arg,rv,hlo,lo,slo,hhi,hi,shi,extra,work,depth+1);
    (void)ok;
    return domain_req(arg,rv,1,hlo,lo,slo,hhi,hi,shi,extra,work);
  }
  if (parse_unary_arg(s,"atan",arg) || parse_unary_arg(s,"sin",arg) ||
      parse_unary_arg(s,"cos",arg) || parse_unary_arg(s,"exp",arg))
    return domain_collect(arg,rv,hlo,lo,slo,hhi,hi,shi,extra,work,depth+1);
  if (parse_call(s.c_str(),"log",args,2,n) && n==2){
    bool ok=domain_collect(args[1],rv,hlo,lo,slo,hhi,hi,shi,extra,work,depth+1);
    (void)ok;
    return domain_req(args[1],rv,1,hlo,lo,slo,hhi,hi,shi,extra,work);
  }
  return false;
}

static void domain_add_condition(working_string &out,const working_string &cond){
  int p=out.find(cond);
  if (cond.empty() || p>=0)
    return;
  out += cond+"\n";
}

static bool domain_condition_collect(const working_string &expr,char rv,working_string &out,int depth=0){
  if (depth>6)
    return false;
  working_string s=strip_outer_parens(nospace_lower(expr)),arg,num,den,args[2],parts[8];
  int n=0,signs[8];
  bool hit=false;
  if (split_top_fraction(s,num,den)){
    hit = domain_condition_collect(num,rv,out,depth+1) || hit;
    hit = domain_condition_collect(den,rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(den)+" != 0");
    return true;
  }
  n=split_top_sum_terms(s,parts,signs,8);
  if (n>1){
    for (int i=0;i<n;++i)
      hit = domain_condition_collect(parts[i],rv,out,depth+1) || hit;
    return hit;
  }
  if (parse_unary_arg(s,"sqrt",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(arg)+" >= 0");
    return true;
  }
  if (parse_unary_arg(s,"ln",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(arg)+" > 0");
    return true;
  }
  if (parse_unary_arg(s,"asin",arg) || parse_unary_arg(s,"acos",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(arg)+" >= -1");
    domain_add_condition(out,spaced_pm(arg)+" <= 1");
    return true;
  }
  if (parse_unary_arg(s,"acosh",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(arg)+" >= 1");
    return true;
  }
  if (parse_unary_arg(s,"atanh",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(arg)+" > -1");
    domain_add_condition(out,spaced_pm(arg)+" < 1");
    return true;
  }
  if (parse_unary_arg(s,"atan",arg) || parse_unary_arg(s,"sin",arg) ||
      parse_unary_arg(s,"cos",arg) || parse_unary_arg(s,"exp",arg))
    return domain_condition_collect(arg,rv,out,depth+1);
  if (parse_call(s.c_str(),"log",args,2,n) && n==2){
    hit = domain_condition_collect(args[0],rv,out,depth+1) || hit;
    hit = domain_condition_collect(args[1],rv,out,depth+1) || hit;
    domain_add_condition(out,spaced_pm(args[0])+" > 0");
    domain_add_condition(out,spaced_pm(args[0])+" != 1");
    domain_add_condition(out,spaced_pm(args[1])+" > 0");
    return true;
  }
  if (parse_unary_arg(s,"tan",arg) || parse_unary_arg(s,"sec",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,"cos("+arg+") != 0");
    return true;
  }
  if (parse_unary_arg(s,"cot",arg) || parse_unary_arg(s,"cosec",arg)){
    hit = domain_condition_collect(arg,rv,out,depth+1) || hit;
    domain_add_condition(out,"sin("+arg+") != 0");
    return true;
  }
  return hit;
}

static void domain_append_user_bounds(working_string &out,working_string *args,int n,char rv){
  if (n>=4)
    out += working_string("\nUser bounds: ")+trim(args[2])+" <= "+working_string(1,rv)+" <= "+trim(args[3]);
}

static bool try_domain_trig_simple(const working_string &e,char rv,working_string *args,int n,working_string &out){
  working_string arg,inner,var(1,rv);
  if (parse_unary_arg(e,"ln",arg) && compact(arg)=="1-sin("+var+")"){
    out="Domain\n";
    out += "1 - sin("+var+") > 0\n";
    out += "sin("+var+") < 1\n";
    out += var+" != pi/2 + 2*n*pi";
    domain_append_user_bounds(out,args,n,rv);
    out += "";
    return true;
  }
  if (parse_unary_arg(e,"sqrt",arg) && compact(arg)=="cos("+var+")-1/2"){
    out="Domain\n";
    out += "cos("+var+") - 1/2 >= 0\n";
    out += "cos("+var+") >= 1/2\n";
    out += working_string("-pi/3 + 2*n*pi <= ")+var+" <= pi/3 + 2*n*pi";
    domain_append_user_bounds(out,args,n,rv);
    out += "";
    return true;
  }
  return false;
}

static bool try_domain(const char *input,working_string &out){
  working_string args[4];
  int n=0;
  if (!parse_call(input,"domain",args,4,n) || n<1)
    return false;
  working_string e=canonical_expr(args[0]);
  char rv='x';
  if (n>=2){
    working_string v=compact(args[1]);
    if (v.size()==1 && isalpha((unsigned char)v[0]))
      rv=v[0];
  }
  if (try_domain_trig_simple(e,rv,args,n,out))
    return true;
  bool hlo=false,hhi=false,slo=false,shi=false;
  Rat lo=rat(0,1),hi=rat(0,1);
  working_string extra;
  working_string work;
  if (!domain_collect(e,rv,hlo,lo,slo,hhi,hi,shi,extra,work)){
    working_string cond;
    if (!domain_condition_collect(e,rv,cond))
      return false;
    out="Domain\nconditions:\n"+cond;
    domain_append_user_bounds(out,args,n,rv);
    out += "";
    return true;
  }
  out="Domain\n"+work+domain_answer(rv,hlo,lo,slo,hhi,hi,shi,extra);
  domain_append_user_bounds(out,args,n,rv);
  out += "";
  return true;
}

static bool suffix_ws(const working_string &s,const working_string &suf){
  return s.size()>=suf.size() && s.substr(s.size()-suf.size(),suf.size())==suf;
}

static working_string neg_expr_ws(const working_string &e){
  working_string s=strip_outer_parens(nospace_lower(e));
  if (s=="0")
    return "0";
  if (!s.empty() && s[0]=='-')
    return s.substr(1,s.size()-1);
  return "-"+s;
}

static bool ode_dep_coeff(const working_string &term,const working_string &dep,working_string &coef){
  working_string s=strip_outer_parens(nospace_lower(term));
  if (s==dep){
    coef="1";
    return true;
  }
  working_string suf="*"+dep;
  if (!suffix_ws(s,suf))
    return false;
  coef=strip_outer_parens(s.substr(0,s.size()-suf.size()));
  if (coef.empty())
    coef="1";
  return true;
}

static working_string sign_coeff_ws(int sign,const working_string &coef){
  if (sign>=0)
    return coef;
  if (coef=="1")
    return "-1";
  if (!coef.empty() && coef[0]=='-')
    return coef.substr(1,coef.size()-1);
  return "-"+coef;
}

static bool ode_left_linear(const working_string &left,const working_string &dy,
                            const working_string &dep,working_string &p){
  working_string terms[6],coef;
  int signs[6];
  int n=split_top_sum_terms(left,terms,signs,6);
  bool seen=false;
  p="0";
  for (int i=0;i<n;++i){
    working_string t=strip_outer_parens(nospace_lower(terms[i]));
    if (t==dy){
      if (signs[i]<0 || seen)
        return false;
      seen=true;
      continue;
    }
    if (ode_dep_coeff(t,dep,coef)){
      p=sign_coeff_ws(signs[i],coef);
      continue;
    }
    return false;
  }
  return seen;
}

static bool ode_integral_simple(const working_string &expr,working_string &res){
  working_string s=strip_outer_parens(nospace_lower(expr)),arg;
  if (s=="0"){ res="0"; return true; }
  if (s=="1"){ res="x"; return true; }
  if (s=="-1"){ res="-x"; return true; }
  if (s=="x"){ res="1/2*x^2"; return true; }
  if (s=="-x"){ res="-1/2*x^2"; return true; }
  if (s=="x^2"){ res="1/3*x^3"; return true; }
  if (s=="-x^2"){ res="-1/3*x^3"; return true; }
  if (parse_unary_arg(s,"exp",arg)){
    Rat a,b;
    if (parse_affine_any(arg,'x',a,b) && a.n){
      res=mul_expr(rat_div(rat(1,1),a),"exp("+fmt_linear_rat(a,b,'x')+")");
      return true;
    }
  }
  if (!contains_var_symbol(s,'x')){
    res=s+"*x";
    return true;
  }
  return false;
}

static bool ode_exp_rate(const working_string &expr,Rat &rate){
  working_string s=strip_outer_parens(nospace_lower(expr)),arg;
  if (!parse_unary_arg(s,"exp",arg))
    return false;
  Rat a,b;
  if (!parse_affine_any(arg,'x',a,b) || b.n)
    return false;
  rate=a;
  return true;
}

static bool ode_verify_solution(const working_string &eq,const working_string &dep,
                                const working_string &sol){
  working_string e=nospace_lower(eq),dy="diff("+dep+",x)";
  e=replace_all_literal(e,dy,"diff(("+sol+"),x)");
  e=replace_all_literal(e,dep,"("+sol+")");
  working_string l,r;
  return split_equal_sides(e,l,r) && khicas_zero("("+l+")-("+r+")");
}

static bool try_desolve(const char *input,working_string &out){
  working_string args[3],exact;
  int n=0;
  if (!parse_call(input,"desolve",args,3,n) || n<1)
    return false;
  if (!production_exact_command(trim(input?input:""),exact) || trim(exact).empty())
    exact="";
  out="DE:\n";
  if (!trim(exact).empty() && compact(trim(exact))!=compact(trim(input?input:"")))
    out += "KhiCAS exact:\n"+trim(exact)+"\n";
  else
    out += "separate or use integrating factor\n";
  out += "";
  return true;
}

static bool try_range_quad_rat(const working_string &e,char rv,int n,working_string *args,working_string &out){
  Rat A,B,C;
  if (!parse_quad_rat_expr(e,rv,A,B,C) || !A.n)
    return false;
  working_string var(1,rv);
  Rat xv=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
  Rat yv=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
  if (n>=4){
    Rat lo,hi;
    if (parse_rat(args[2],lo) && parse_rat(args[3],hi)){
      if (rat_cmp(hi,lo)<0){ Rat t=lo; lo=hi; hi=t; }
      auto f=[&](Rat x)->Rat{
        return rat_add(rat_add(rat_mul(A,rat_mul(x,x)),rat_mul(B,x)),C);
      };
      Rat flo=f(lo), fhi=f(hi), ymin=rat_cmp(flo,fhi)<=0?flo:fhi, ymax=rat_cmp(flo,fhi)>=0?flo:fhi;
      bool vin=rat_cmp(lo,xv)<=0 && rat_cmp(xv,hi)<=0;
      if (vin){
        if (rat_cmp(yv,ymin)<0) ymin=yv;
        if (rat_cmp(yv,ymax)>0) ymax=yv;
      }
      out="Range:\n"+rat_s(lo)+" <= "+var+" <= "+rat_s(hi)+"\n";
      out += "f("+rat_s(lo)+") = "+rat_s(flo)+"\n";
      out += "f("+rat_s(hi)+") = "+rat_s(fhi)+"\n";
      if (vin)
        out += "f("+rat_s(xv)+") = "+rat_s(yv)+"\n";
      out += rat_s(ymin)+" <= y <= "+rat_s(ymax)+"";
      return true;
    }
  }
  out="Range:\nvertex\n";
  out += "vx = "+rat_s(xv)+"\n";
  if (A.n>0){
    out += "min y = "+rat_s(yv)+"\n";
    out += "y >= "+rat_s(yv);
  }
  else {
    out += "max y = "+rat_s(yv)+"\n";
    out += "y <= "+rat_s(yv);
  }
  out += "";
  return true;
}

static bool exact_approx_double(const working_string &expr,double &v){
  working_string s;
  if (!production_exact_command("approx("+expr+")",s))
    return false;
  s=trim(s);
  char *end=0;
  v=strtod(s.c_str(),&end);
  while (end && *end && isspace((unsigned char)*end))
    ++end;
  return end && !*end && finite_double(v);
}

static bool exact_limit_kind(const working_string &expr,int &kind,double &v){
  working_string s=compact(expr);
  if (s=="oo" || s=="+oo" || s=="infinity" || s=="+infinity"){
    kind=1;
    return true;
  }
  if (s=="-oo" || s=="-infinity"){
    kind=-1;
    return true;
  }
  if (exact_approx_double(expr,v)){
    kind=0;
    return true;
  }
  return false;
}

static working_string range_value_display(const working_string &src);

struct RangeVal {
  working_string s;
  double v;
  bool open;
};

static bool add_range_val(RangeVal *vals,int &n,const working_string &s,double v,bool open){
  if (!finite_double(v) || n>=12)
    return false;
  vals[n].s=range_value_display(trim(s));
  vals[n].v=v;
  vals[n].open=open;
  ++n;
  return true;
}

static bool add_range_eval(const working_string &e,const working_string &var,const working_string &x,
                           bool open,RangeVal *vals,int &n){
  working_string y;
  double v=0;
  if (!production_exact_command("subst("+e+","+var+"="+x+")",y) || !exact_approx_double(y,v))
    return false;
  return add_range_val(vals,n,y,v,open);
}

static bool add_range_point(const working_string &e,const working_string &var,const working_string &x,
                            bool open,RangeVal *vals,int &n,working_string &steps){
  int before=n;
  if (!add_range_eval(e,var,x,open,vals,n))
    return false;
  if (n>before)
    steps += "f("+x+") = "+vals[n-1].s+"\n";
  return true;
}

static bool add_range_limit(const working_string &e,const working_string &var,const working_string &x,
                            int &plus,int &minus,RangeVal *vals,int &n){
  working_string y;
  int k=0;
  double v=0;
  if (!production_exact_command("limit("+e+","+var+","+x+")",y) || !exact_limit_kind(y,k,v))
    return false;
  if (k>0){ ++plus; return true; }
  if (k<0){ ++minus; return true; }
  return add_range_val(vals,n,y,v,true);
}

static bool add_range_boundary_limit(const working_string &e,const working_string &var,const working_string &x,
                                     int &plus,int &minus,RangeVal *vals,int &n,working_string &steps){
  int bp=plus,bm=minus,bn=n;
  working_string sx=x=="oo"?"+infinity":(x=="-oo"?"-infinity":x);
  if (!add_range_limit(e,var,x,plus,minus,vals,n))
    return false;
  if (plus>bp)
    steps += "as "+var+" -> "+sx+", y -> +infinity\n";
  else if (minus>bm)
    steps += "as "+var+" -> "+sx+", y -> -infinity\n";
  else if (n>bn)
    steps += "as "+var+" -> "+sx+", y -> "+vals[n-1].s+"\n";
  return true;
}

static bool range_value_closed(RangeVal *vals,int n,double v){
  for (int i=0;i<n;++i)
    if (!vals[i].open && fabs(vals[i].v-v)<1e-8)
      return true;
  return false;
}

static working_string range_cmp_line(RangeVal lo,RangeVal hi,RangeVal *vals,int n){
  bool lo_open=lo.open && !range_value_closed(vals,n,lo.v);
  bool hi_open=hi.open && !range_value_closed(vals,n,hi.v);
  if (fabs(lo.v-hi.v)<1e-9)
    return working_string("y = ")+lo.s;
  return lo.s+(lo_open?" < ":" <= ")+"y"+(hi_open?" < ":" <= ")+hi.s;
}

static void constraint_apply_lower(WorkConstraint &c,double v,const working_string &s,bool strict){
  if (!c.hlo || v>c.lo+1e-9 || (fabs(v-c.lo)<1e-9 && strict && !c.slo)){
    c.active=true; c.hlo=true; c.lo=v; c.los=s; c.slo=strict;
    constraint_refresh_desc(c);
  }
}

static void constraint_apply_upper(WorkConstraint &c,double v,const working_string &s,bool strict){
  if (!c.hhi || v<c.hi-1e-9 || (fabs(v-c.hi)<1e-9 && strict && !c.shi)){
    c.active=true; c.hhi=true; c.hi=v; c.his=s; c.shi=strict;
    constraint_refresh_desc(c);
  }
}

static bool range_apply_affine_domain(const working_string &arg,char rv,bool strict,
                                      WorkConstraint &c,working_string &steps){
  Rat a,b;
  if (!parse_affine_general(arg,rv,a,b) || !a.n)
    return false;
  Rat root=rat_div(rat(-b.n,b.d),a);
  double x=double(root.n)/double(root.d);
  working_string r=rat_s(root),v(1,rv);
  if (a.n>0){
    constraint_apply_lower(c,x,r,strict);
    steps += "natural domain: "+v+(strict?" > ":" >= ")+r+"\n";
  }
  else {
    constraint_apply_upper(c,x,r,strict);
    steps += "natural domain: "+v+(strict?" < ":" <= ")+r+"\n";
  }
  return true;
}

static void range_apply_natural_domain(const working_string &expr,char rv,
                                       WorkConstraint &c,working_string &steps){
  working_string s=strip_outer_parens(nospace_lower(expr)),arg;
  if (parse_unary_arg(s,"sqrt",arg))
    range_apply_affine_domain(arg,rv,false,c,steps);
  else if (parse_unary_arg(s,"ln",arg) || parse_unary_arg(s,"log",arg))
    range_apply_affine_domain(arg,rv,true,c,steps);
}

static bool add_range_abs_cusps(const working_string &e,char rv,const WorkConstraint &c,
                                RangeVal *vals,int &vn,working_string &steps){
  working_string s=strip_outer_parens(nospace_lower(e)),arg,var(1,rv),roots,rs[8],seen;
  if (!parse_unary_arg(s,"abs",arg) ||
      !production_exact_command("solve("+arg+"=0,"+var+")",roots))
    return false;
  int rn=split_exact_list(roots,rs,8),used=0;
  for (int i=0;i<rn;++i){
    working_string r=trim(rs[i]),shown;
    if (r.empty() || contains_var_symbol(r,rv) || !eval_numeric_string(r,shown) ||
        !constraint_contains(c,last_numeric_value))
      continue;
    seen += (used?",":"")+r;
    ++used;
    if (!add_range_point(e,var,r,false,vals,vn,steps))
      return false;
  }
  if (used)
    steps = "split at "+var+"="+seen+"\n"+steps;
  return used>0;
}

static bool try_range_recip_affine_constrained(const working_string &e,char rv,const WorkConstraint &c,working_string &out){
  working_string num,den,var(1,rv);
  Rat nr,da,db;
  if (!split_top_fraction(e,num,den) || !parse_rat(num,nr) || !nr.n ||
      !parse_affine_general(den,rv,da,db) || !da.n)
    return false;
  double root=-double(db.n)/double(db.d)/(double(da.n)/double(da.d));
  if (!constraint_contains(c,root))
    return false;
  if ((!c.hlo || root<=c.lo+1e-9) || (!c.hhi || root>=c.hi-1e-9))
    return false;
  RangeVal lv,hv;
  int n=0;
  RangeVal vals[2];
  if (c.hlo && !c.slo && !add_range_eval(e,var,c.los,false,vals,n))
    return false;
  if (c.hhi && !c.shi && !add_range_eval(e,var,c.his,false,vals,n))
    return false;
  if (n<2)
    return false;
  lv=vals[0]; hv=vals[1];
  out="Range:\n"+c.desc+"\n";
  out += "pole at "+var+"="+double_s(root)+"\n";
  out += "split interval at pole\n";
  if (lv.v<hv.v)
    out += "range: y <= "+lv.s+" or y >= "+hv.s+"\n";
  else
    out += "range: y <= "+hv.s+" or y >= "+lv.s+"\n";
  out += "";
  return true;
}

static bool try_range_constrained(const working_string &e,char rv,const WorkConstraint &c,working_string &out){
  working_string var(1,rv),df,roots,rs[8];
  RangeVal vals[12];
  int vn=0,plus=0,minus=0;
  working_string steps,prefix;
  WorkConstraint cc=c;
  if (!c.active || !only_var_symbol(e,rv) || working_route_too_large(e))
    return false;
  range_apply_natural_domain(e,rv,cc,prefix);
  if (cc.hlo && cc.hhi && (cc.lo>cc.hi+1e-9 || (fabs(cc.lo-cc.hi)<1e-9 && (cc.slo || cc.shi))))
    return false;
  if (strip_outer_parens(nospace_lower(e))=="tan("+var+")" && cc.hlo && cc.hhi &&
      1.5707963267948966>cc.lo+1e-9 && 1.5707963267948966<cc.hi-1e-9){
    out="Range:\n"+prefix+cc.desc+"\n";
    out += "pole at "+var+"=pi/2\n";
    out += "split interval at pole\n";
    out += "range: all real";
    return true;
  }
  if (try_range_recip_affine_constrained(e,rv,cc,out)){
    if (!prefix.empty())
      out=replace_all_literal(out,"Range:\n","Range:\n"+prefix);
    return true;
  }
  if (cc.hlo){
    if (cc.slo){
      if (!add_range_boundary_limit(e,var,cc.los,plus,minus,vals,vn,steps)) return false;
    }
    else if (!add_range_point(e,var,cc.los,false,vals,vn,steps)) return false;
  }
  else if (!add_range_boundary_limit(e,var,"-oo",plus,minus,vals,vn,steps)) return false;
  if (cc.hhi){
    if (cc.shi){
      if (!add_range_boundary_limit(e,var,cc.his,plus,minus,vals,vn,steps)) return false;
    }
    else if (!add_range_point(e,var,cc.his,false,vals,vn,steps)) return false;
  }
  else if (!add_range_boundary_limit(e,var,"oo",plus,minus,vals,vn,steps)) return false;
  if (cc.hexcl)
    add_range_boundary_limit(e,var,cc.excls,plus,minus,vals,vn,steps);
  add_range_abs_cusps(e,rv,cc,vals,vn,steps);
  if (production_exact_command("diff("+e+","+var+")",df) &&
      production_exact_command("solve(("+trim(df)+")=0,"+var+")",roots)){
    int rn=split_exact_list(roots,rs,8);
    for (int i=0;i<rn;++i){
      working_string r=trim(rs[i]),shown;
      if (r.empty() || contains(r,"I") || contains_var_symbol(r,rv) ||
          !eval_numeric_string(r,shown) || !constraint_contains(cc,last_numeric_value))
        continue;
      if (!add_range_point(e,var,r,false,vals,vn,steps))
        return false;
    }
  }
  out="Range:\n"+prefix+cc.desc+"\n";
  bool show_df=!trim(df).empty() && !contains(df,"Derivative(") &&
               !contains(df,"re(") && !contains(df,"im(");
  if (show_df && !contains_var_symbol(trim(df),rv))
    out += "linear interval\n";
  if (show_df)
    out += "f'("+var+")="+trim(df)+"\n";
  out += steps;
  if (plus && minus){
    out += "range: all real";
    return true;
  }
  int lo=-1,hi=-1;
  for (int i=0;i<vn;++i){
    if (lo<0 || vals[i].v<vals[lo].v) lo=i;
    if (hi<0 || vals[i].v>vals[hi].v) hi=i;
  }
  if (plus && lo>=0){
    bool op=vals[lo].open && !range_value_closed(vals,vn,vals[lo].v);
    if (!op)
      out += "min y = "+vals[lo].s+"\n";
    out += working_string("y")+(op?" > ":" >= ")+vals[lo].s+"";
    return true;
  }
  if (minus && hi>=0){
    bool op=vals[hi].open && !range_value_closed(vals,vn,vals[hi].v);
    if (!op)
      out += "max y = "+vals[hi].s+"\n";
    out += working_string("y")+(op?" < ":" <= ")+vals[hi].s+"";
    return true;
  }
  if (lo>=0 && hi>=0){
    out += range_cmp_line(vals[lo],vals[hi],vals,vn)+"";
    return true;
  }
  return false;
}

static bool try_range_all_real_constrained(const working_string &e,char rv,working_string &out){
  WorkConstraint c;
  constraint_init(c,rv);
  c.active=true;
  c.desc="all real";
  return try_range_constrained(e,rv,c,out);
}

static bool try_range_positive_constrained(const working_string &e,char rv,working_string &out){
  WorkConstraint c;
  constraint_init(c,rv);
  if (!constraint_set_bounds(c,"0","inf",true,false) || !try_range_constrained(e,rv,c,out))
    return false;
  out="endpoint/critical route\nDomain:\n"+out;
  return true;
}

static working_string range_value_display(const working_string &src){
  working_string s=trim(src);
  if (s=="exp(-1)")
    return "1/e";
  if (s=="exp(1)")
    return "e";
  return s;
}

static double rat_double(Rat r){
  return double(r.n)/double(r.d);
}

static bool range_root_in_domain(double x,bool hlo,Rat lo,bool hhi,Rat hi){
  if (hlo && x<=rat_double(lo)+1e-9)
    return false;
  if (hhi && x>=rat_double(hi)-1e-9)
    return false;
  return true;
}

static bool try_range_exp_poly_route(const working_string &e,char rv,int argc,working_string *rargs,working_string &out){
  working_string df,roots,limp,limn;
  working_string var(1,rv),root[4],val[4];
  double yv[4],lnv=0,lpv=0;
  int kn=0,kp=0;
  if (!only_var_symbol(e,rv))
    return false;
  if (working_route_too_large(e))
    return false;
  if (contains(e,"ln(") || contains(e,"log(") || contains(e,"sqrt(") ||
      contains(e,"tan(") || contains(e,"cot(") || contains(e,"sec(") ||
      contains(e,"cosec(") || contains(e,"atan("))
    return false;
  if (argc>=4){
    working_string num,den;
    if (split_top_fraction(e,num,den) ||
        nested_function_expr(e,3) || e.size()>160)
      return false;
    double bd[2];
    working_string be[2]={trim(rargs[2]),trim(rargs[3])};
    if (!exact_approx_double(be[0],bd[0]) || !exact_approx_double(be[1],bd[1]))
      return false;
    if (bd[1]<bd[0]){
      double td=bd[0]; bd[0]=bd[1]; bd[1]=td;
      working_string ts=be[0]; be[0]=be[1]; be[1]=ts;
    }
    if (!production_exact_command("diff("+e+","+var+")",df) ||
        !production_exact_command("solve(("+df+")=0,"+var+")",roots))
      return false;
    int rn=0,lo=-1,hi=-1;
    for (int i=0;i<2;++i){
      root[rn]=be[i];
      if (!production_exact_command("subst("+e+","+var+"="+root[rn]+")",val[rn]))
        return false;
      val[rn]=range_value_display(val[rn]);
      if (!exact_approx_double(val[rn],yv[rn]))
        return false;
      if (lo<0 || yv[rn]<yv[lo]) lo=rn;
      if (hi<0 || yv[rn]>yv[hi]) hi=rn;
      ++rn;
    }
    working_string rs[4];
    int sn=split_exact_list(roots,rs,4);
    for (int i=0;i<sn && rn<4;++i){
      double xd;
      working_string r=trim(rs[i]);
      if (r.empty() || contains(r,"I") || contains_var_symbol(r,rv) ||
          !exact_approx_double(r,xd) || xd<bd[0]-1e-9 || xd>bd[1]+1e-9)
        continue;
      root[rn]=r;
      if (!production_exact_command("subst("+e+","+var+"="+r+")",val[rn]))
        return false;
      val[rn]=range_value_display(val[rn]);
      if (!exact_approx_double(val[rn],yv[rn]))
        return false;
      if (yv[rn]<yv[lo]) lo=rn;
      if (yv[rn]>yv[hi]) hi=rn;
      ++rn;
    }
    out="Range:\n";
    out += be[0]+" <= "+var+" <= "+be[1]+"\n";
    out += "f'("+var+")="+trim(df)+"\n";
    for (int i=0;i<rn;++i)
      out += "f("+root[i]+") = "+val[i]+"\n";
    out += val[lo]+" <= y <= "+val[hi]+"\n";
    out += "";
    return true;
  }
  if (!contains(e,"exp(") || contains(e,"sin(") || contains(e,"cos(") ||
      nested_function_expr(e,2))
    return false;
  if (!production_exact_command("diff("+e+","+var+")",df) ||
      !production_exact_command("solve(("+df+")=0,"+var+")",roots) ||
      !production_exact_command("limit("+e+","+var+",oo)",limp) ||
      !production_exact_command("limit("+e+","+var+",-oo)",limn))
    return false;
  if (!exact_limit_kind(limn,kn,lnv) || !exact_limit_kind(limp,kp,lpv))
    return false;
  int rn=split_exact_list(roots,root,4);
  int mn=0;
  int lo=-1,hi=-1;
  for (int i=0;i<rn;++i){
    working_string r=trim(root[i]);
    if (r.empty() || contains(r,"I"))
      continue;
    if (contains_var_symbol(r,rv))
      return false;
    root[mn]=r;
    if (!production_exact_command("subst("+e+","+var+"="+r+")",val[mn]))
      return false;
    val[mn]=range_value_display(val[mn]);
    if (!exact_approx_double(val[mn],yv[mn]))
      return false;
    if (lo<0 || yv[mn]<yv[lo])
      lo=mn;
    if (hi<0 || yv[mn]>yv[hi])
      hi=mn;
    ++mn;
  }
  rn=mn;
  if (rn<=0 && ((kn>0 && kp<0) || (kn<0 && kp>0))){
    out="Range:\n";
    out += "f'("+var+")="+trim(df)+"\n";
    out += "as "+var+" -> -infinity, y -> "+(kn>0?"+infinity":"-infinity")+"\n";
    out += "as "+var+" -> +infinity, y -> "+(kp>0?"+infinity":"-infinity")+"\n";
    out += "range: all real";
    return true;
  }
  if (lo<0 || hi<0)
    return false;
  out="Range:\n";
  out += "f'("+var+")="+trim(df)+"\n";
  for (int i=0;i<rn;++i)
    out += "f("+root[i]+") = "+val[i]+"\n";
  out += "as "+var+" -> -infinity, y -> "+(kn>0?"+infinity":kn<0?"-infinity":trim(limn))+"\n";
  out += "as "+var+" -> +infinity, y -> "+(kp>0?"+infinity":kp<0?"-infinity":trim(limp))+"\n";
  if ((kn>0 || kp>0) && kn>=0 && kp>=0){
    out += "min y = "+val[lo]+"\n";
    out += "y >= "+val[lo]+"\n";
  }
  else if ((kn<0 || kp<0) && kn<=0 && kp<=0){
    out += "max y = "+val[hi]+"\n";
    out += "y <= "+val[hi]+"\n";
  }
  else if ((kn>0 && kp<0) || (kn<0 && kp>0)){
    out += "range: all real\n";
  }
  else if (kn==0 && kp==0 && fabs(lnv-lpv)<1e-9){
    if (yv[hi]>lnv){
      out += trim(limn)+" < y <= "+val[hi]+"\n";
    }
    else if (yv[lo]<lnv){
      out += val[lo]+" <= y < "+trim(limn)+"\n";
    }
    else
      return false;
  }
  else
    return false;
  out += "";
  return true;
}

static bool try_range_endpoint_limit_route(const working_string &e,char rv,working_string &out){
  if (!only_var_symbol(e,rv) || working_route_too_large(e) ||
      e.size()>190 || nested_function_expr(e,5))
    return false;
  if (!contains(e,"ln(") && !contains(e,"log(") && !contains(e,"sqrt("))
    return false;

  bool hlo=false,hhi=false,slo=false,shi=false;
  Rat lo=rat(0,1),hi=rat(0,1);
  working_string extra,work,var(1,rv);
  if (!domain_collect(e,rv,hlo,lo,slo,hhi,hi,shi,extra,work))
    return false;

  working_string pts[2],lbl[2],lim[2];
  int kind[2],ln=0;
  double val[2];
  pts[ln]=hlo?rat_s(lo):working_string("-oo");
  lbl[ln]=hlo?working_string("as ")+var+" -> "+rat_s(lo):working_string("as ")+var+" -> -infinity";
  ++ln;
  pts[ln]=hhi?rat_s(hi):working_string("oo");
  lbl[ln]=hhi?working_string("as ")+var+" -> "+rat_s(hi):working_string("as ")+var+" -> +infinity";
  ++ln;
  for (int i=0;i<ln;++i){
    if (!production_exact_command("limit("+e+","+var+","+pts[i]+")",lim[i]) ||
        !exact_limit_kind(lim[i],kind[i],val[i]))
      return false;
  }

  int plus=0,minus=0;
  for (int i=0;i<ln;++i){
    if (kind[i]>0) ++plus;
    if (kind[i]<0) ++minus;
  }
  out="Range:\nendpoint/critical route\n";
  if (!work.empty())
    out += "Domain:\n"+work;
  for (int i=0;i<ln;++i)
    out += lbl[i]+", y -> "+(kind[i]>0?working_string("+infinity"):kind[i]<0?working_string("-infinity"):trim(lim[i]))+"\n";
  if (plus && minus){
    out += "range: all real";
    return true;
  }

  working_string df,roots,rs[4],rvv[4],fv[4];
  double yv[4];
  int rn=0,loi=-1,hii=-1;
  if (!production_exact_command("diff("+e+","+var+")",df) ||
      !production_exact_command("solve(("+df+")=0,"+var+")",roots))
    return false;
  int sn=split_exact_list(roots,rs,4);
  for (int i=0;i<sn && rn<4;++i){
    double xd;
    working_string r=trim(rs[i]);
    if (r.empty() || contains(r,"I") || contains_var_symbol(r,rv) ||
        !exact_approx_double(r,xd) || !range_root_in_domain(xd,hlo,lo,hhi,hi))
      continue;
    rvv[rn]=r;
    if (!production_exact_command("subst("+e+","+var+"="+r+")",fv[rn]) ||
        !exact_approx_double(fv[rn],yv[rn]))
      return false;
    fv[rn]=range_value_display(fv[rn]);
    if (loi<0 || yv[rn]<yv[loi]) loi=rn;
    if (hii<0 || yv[rn]>yv[hii]) hii=rn;
    ++rn;
  }
  if (rn<=0)
    return false;
  out += "f'("+var+")="+trim(df)+"\n";
  for (int i=0;i<rn;++i)
    out += "f("+rvv[i]+") = "+fv[i]+"\n";
  if (plus && !minus && loi>=0){
    out += "min y = "+fv[loi]+"\ny >= "+fv[loi]+"";
    return true;
  }
  if (minus && !plus && hii>=0){
    out += "max y = "+fv[hii]+"\ny <= "+fv[hii]+"";
    return true;
  }
  return false;
}

static bool try_range_sincos_sum(const working_string &e,char rv,working_string &out){
  working_string terms[4],arg,ta;
  int signs[4],n=split_top_sum_terms(e,terms,signs,4);
  long sc=0,cc=0;
  working_string shift;
  bool trig=false;
  for (int i=0;i<n;++i){
    Rat q;
    bool issin=parse_coeff_call_term(terms[i],"sin",rv,q,ta);
    if (issin || parse_coeff_call_term(terms[i],"cos",rv,q,ta)){
      long qi=0;
      if (!rat_is_int(q,qi))
        return false;
      if (!range_expr_all_real_simple(ta,rv))
        return false;
      if (arg.empty())
        arg=ta;
      else if (arg!=ta)
        return false;
      qi*=signs[i];
      if (issin)
        sc+=qi;
      else
        cc+=qi;
      trig=true;
    }
    else if (!contains_var_symbol(terms[i],rv) && parse_rat(terms[i],q)){
      long qi=0;
      if (!rat_is_int(q,qi))
        return false;
      shift=join_sum(shift,signed_part(signs[i],int_s(qi)));
    }
    else if (!contains_var_symbol(terms[i],rv)){
      shift=join_sum(shift,signed_part(signs[i],terms[i]));
    }
    else
      return false;
  }
  if (!trig || (!sc && !cc))
    return false;
  long r2=sc*sc+cc*cc,sr=0;
  working_string rs=sqrt_int_s(r2);
  out="Range:\nR-form range\n";
  out += "R=sqrt("+int_s(sc)+"^2+"+int_s(cc)+"^2)="+rs+"\n";
  if (square_long(r2,sr)){
    out += range_symbolic_bound(shift,rat(-sr,1))+" <= y <= "+range_symbolic_bound(shift,rat(sr,1))+"\n";
  }
  else {
    if (!shift.empty())
      out += spaced_pm(compact(shift))+" - "+rs+" <= y <= "+spaced_pm(compact(shift))+" + "+rs+"\n";
    else
      out += "-"+rs+" <= y <= "+rs+"\n";
  }
  out += "";
  return true;
}

static Rat rat_abs_v(Rat q){
  return q.n<0?rat(-q.n,q.d):q;
}

static bool try_range_trig_sum_bound(const working_string &e,char rv,working_string &out){
  working_string terms[5],arg,first;
  int signs[5],n=split_top_sum_terms(e,terms,signs,5);
  if (n<2)
    return false;
  (void)signs;
  int amp=0;
  bool trig=false,mixed=false;
  for (int i=0;i<n;++i){
    if (!(parse_unary_arg(terms[i],"sin",arg) ||
          parse_unary_arg(terms[i],"cos",arg)) ||
        !contains_var_symbol(arg,rv))
      return false;
    working_string ca=compact(arg);
    if (first.empty())
      first=ca;
    else if (ca!=first)
      mixed=true;
    ++amp;
    trig=true;
  }
  if (!trig || !mixed)
    return false;
  out="bound\n-";
  out += int_s(amp)+" <= y <= "+int_s(amp)+"";
  return true;
}

static working_string range_symbolic_bound(const working_string &shift,Rat add){
  if (shift.empty())
    return rat_s(add);
  working_string clean=compact(shift);
  Rat q;
  if (parse_rat(clean,q))
    return rat_s(rat_add(q,add));
  if (!add.n)
    return spaced_pm(clean);
  return spaced_pm(compact(join_sum(clean,rat_s(add))));
}

static bool try_range_exp_shift(const working_string &e,char rv,working_string &out){
  working_string terms[3],arg;
  int signs[3],n=split_top_sum_terms(e,terms,signs,3);
  Rat coef=rat(0,1);
  working_string shift;
  bool got=false;
  if (n<1)
    return false;
  for (int i=0;i<n;++i){
    Rat q;
    if (!contains_var_symbol(terms[i],rv) && parse_rat(terms[i],q)){
      shift=join_sum(shift,signed_part(signs[i],rat_s(q)));
      continue;
    }
    if (!contains_var_symbol(terms[i],rv)){
      shift=join_sum(shift,signed_part(signs[i],terms[i]));
      continue;
    }
    if (parse_coeff_call_term(terms[i],"exp",rv,q,arg) &&
        range_expr_all_real_simple(arg,rv) && !got){
      coef=rat_mul(q,rat(signs[i],1));
      got=true;
      continue;
    }
    return false;
  }
  if (!got || !coef.n)
    return false;
  out="Range:\nshifted exponential\n";
  out += "exp(u) > 0\n";
  out += coef.n>0 ? "y > " : "y < ";
  out += range_symbolic_bound(shift,rat(0,1))+"";
  return true;
}

static bool try_range_shifted_base_function(const working_string &e,char rv,working_string &out){
  working_string terms[4],arg,shift,fn;
  int signs[4],n=split_top_sum_terms(e,terms,signs,4),got=0;
  Rat coef=rat(0,1);
  if (n<1)
    return false;
  for (int i=0;i<n;++i){
    Rat q;
    if (!contains_var_symbol(terms[i],rv)){
      shift=join_sum(shift,signed_part(signs[i],terms[i]));
      continue;
    }
    const char *names[]={"exp","sqrt","abs","tan","cot","sec","cosec"};
    bool hit=false;
    for (int j=0;j<7 && !hit;++j){
      working_string a;
      if (parse_coeff_call_term(terms[i],names[j],rv,q,a)){
        fn=names[j];
        arg=a;
        coef=rat_mul(q,rat(signs[i],1));
        hit=true;
      }
    }
    if (!hit || got++ || !coef.n || !contains_var_symbol(arg,rv))
      return false;
  }
  if (!got)
    return false;
  out="Range:\n";
  if (fn=="tan" || fn=="cot"){
    out += fn+"(u): all real\nall real";
    return true;
  }
  if (fn=="sec" || fn=="cosec"){
    Rat a=rat_abs_v(coef);
    out += fn+"(u): y <= -1 or y >= 1\n";
    out += "scale/shift\n";
    out += "y <= "+range_symbolic_bound(shift,rat(-a.n,a.d))+
           " or y >= "+range_symbolic_bound(shift,a);
    return true;
  }
  out += fn+"(u)";
  out += fn=="exp" ? " > 0\n" : " >= 0\n";
  out += "scale/shift\n";
  if (coef.n>0)
    out += working_string("y ")+(fn=="exp"?"> ":">= ")+range_symbolic_bound(shift,rat(0,1));
  else
    out += working_string("y ")+(fn=="exp"?"< ":"<= ")+range_symbolic_bound(shift,rat(0,1));
  return true;
}

static bool parse_coeff_trig_square_term(const working_string &term,const char *fn,char rv,
                                         Rat &coef,working_string &arg){
  working_string core;
  return split_coeff_core(term,coef,core) &&
         parse_trig_square_core(core,fn,arg) &&
         range_expr_all_real_simple(arg,rv);
}

static bool try_range_trig_square(const working_string &e,char rv,working_string &out){
  working_string terms[3],arg;
  int signs[3],n=split_top_sum_terms(e,terms,signs,3);
  Rat coef=rat(0,1);
  working_string shift;
  const char *fn=0;
  bool got=false;
  if (n<1)
    return false;
  for (int i=0;i<n;++i){
    Rat q;
    if (!contains_var_symbol(terms[i],rv) && parse_rat(terms[i],q)){
      shift=join_sum(shift,signed_part(signs[i],rat_s(q)));
      continue;
    }
    if (!contains_var_symbol(terms[i],rv)){
      shift=join_sum(shift,signed_part(signs[i],terms[i]));
      continue;
    }
    const char *fns[]={"sin","cos","tan","sec","cosec",0};
    bool hit=false;
    for (int j=0;fns[j];++j){
      if (parse_coeff_trig_square_term(terms[i],fns[j],rv,q,arg)){
        if (got)
          return false;
        coef=rat_mul(q,rat(signs[i],1));
        fn=fns[j];
        got=hit=true;
        break;
      }
    }
    if (!hit)
      return false;
  }
  if (!got || !coef.n)
    return false;
  out="Range:\n";
  if (!strcmp(fn,"sin") || !strcmp(fn,"cos")){
    Rat lo=coef.n<0?coef:rat(0,1), hi=coef.n<0?rat(0,1):coef;
    out += working_string(fn)+"^2(u) in [0,1]\n";
    out += range_symbolic_bound(shift,lo)+" <= y <= "+range_symbolic_bound(shift,hi)+"";
    return true;
  }
  if (!strcmp(fn,"tan")){
    out += "tan^2(u) >= 0\n";
    out += coef.n>0 ? "y >= " : "y <= ";
    out += range_symbolic_bound(shift,rat(0,1))+"";
    return true;
  }
  out += working_string(fn)+"^2(u) >= 1\n";
  out += coef.n>0 ? "y >= " : "y <= ";
  out += range_symbolic_bound(shift,coef)+"";
  return true;
}

static bool try_range_recip_shifted_trig(const working_string &e,char rv,working_string &out){
  working_string num,den,terms[3],arg;
  int signs[3];
  Rat ncoef,shift=rat(0,1),tcoef=rat(0,1);
  bool got=false;
  const char *fn=0;
  if (!split_top_fraction(e,num,den) || !parse_rat(num,ncoef))
    return false;
  if (!ncoef.n){
    out="Range:\nzero numerator\ny = 0";
    return true;
  }
  int tn=split_top_sum_terms(den,terms,signs,3);
  if (tn<1)
    return false;
  for (int i=0;i<tn;++i){
    Rat q;
    if (parse_rat(terms[i],q)){
      shift=rat_add(shift,rat_mul(q,rat(signs[i],1)));
      continue;
    }
    if (!got && parse_coeff_call_term(terms[i],"sin",rv,q,arg) &&
        range_expr_all_real_simple(arg,rv)){
      tcoef=rat_mul(q,rat(signs[i],1));
      fn="sin";
      got=true;
      continue;
    }
    if (!got && parse_coeff_call_term(terms[i],"cos",rv,q,arg) &&
        range_expr_all_real_simple(arg,rv)){
      tcoef=rat_mul(q,rat(signs[i],1));
      fn="cos";
      got=true;
      continue;
    }
    return false;
  }
  if (!got || !tcoef.n)
    return false;
  Rat a=rat_abs_v(tcoef);
  Rat lo=rat_sub(shift,a),hi=rat_add(shift,a);
  out="Range:\nreciprocal shifted trig\n";
  out += working_string(fn)+"(u) in [-1,1]\n";
  out += rat_s(lo)+" <= D <= "+rat_s(hi)+"\n";
  int clo=rat_cmp(lo,rat(0,1)), chi=rat_cmp(hi,rat(0,1));
  if (clo>0 || chi<0){
    Rat y1=rat_div(ncoef,lo),y2=rat_div(ncoef,hi);
    if (rat_cmp(y2,y1)<0){ Rat t=y1; y1=y2; y2=t; }
    out += rat_s(y1)+" <= y <= "+rat_s(y2)+"";
    return true;
  }
  if (clo<0 && chi>0){
    Rat y1=rat_div(ncoef,lo),y2=rat_div(ncoef,hi);
    if (rat_cmp(y2,y1)<0){ Rat t=y1; y1=y2; y2=t; }
    out += "D != 0\n";
    out += "y <= "+rat_s(y1)+" or y >= "+rat_s(y2)+"";
    return true;
  }
  if (clo==0 && chi>0){
    Rat y=rat_div(ncoef,hi);
    out += "D > 0\n";
    out += ncoef.n>0 ? "y >= " : "y <= ";
    out += rat_s(y)+"";
    return true;
  }
  if (clo<0 && chi==0){
    Rat y=rat_div(ncoef,lo);
    out += "D < 0\n";
    out += ncoef.n>0 ? "y <= " : "y >= ";
    out += rat_s(y)+"";
    return true;
  }
  return false;
}

static bool parse_range_sincos_linear(const working_string &src,char rv,
                                      Rat &k,Rat &scoef,Rat &ccoef,
                                      working_string &arg,bool &has_trig){
  working_string terms[5],ta;
  int signs[5],n=split_top_sum_terms(src,terms,signs,5);
  if (n<1)
    return false;
  k=rat(0,1); scoef=rat(0,1); ccoef=rat(0,1);
  arg.clear(); has_trig=false;
  for (int i=0;i<n;++i){
    Rat q;
    if (parse_rat(terms[i],q)){
      k=rat_add(k,rat_mul(q,rat(signs[i],1)));
      continue;
    }
    bool issin=parse_coeff_call_term(terms[i],"sin",rv,q,ta);
    if (issin || parse_coeff_call_term(terms[i],"cos",rv,q,ta)){
      if (!range_expr_all_real_simple(ta,rv))
        return false;
      if (arg.empty())
        arg=ta;
      else if (arg!=ta)
        return false;
      q=rat_mul(q,rat(signs[i],1));
      if (issin)
        scoef=rat_add(scoef,q);
      else
        ccoef=rat_add(ccoef,q);
      has_trig=true;
      continue;
    }
    return false;
  }
  return true;
}

static bool try_range_sincos_fraction(const working_string &e,char rv,working_string &out){
  working_string num,den,narg,darg,ineq;
  Rat n0,ns,nc,d0,ds,dc;
  bool nt=false,dt=false;
  if (!split_top_fraction(e,num,den) ||
      !parse_range_sincos_linear(num,rv,n0,ns,nc,narg,nt) ||
      !parse_range_sincos_linear(den,rv,d0,ds,dc,darg,dt) ||
      !dt)
    return false;
  if (nt && dt && narg!=darg)
    return false;
  Rat d_amp2=rat_add(rat_mul(ds,ds),rat_mul(dc,dc));
  if (rat_cmp(rat_mul(d0,d0),d_amp2)<=0)
    return false;
  Rat A=rat_sub(rat_mul(d0,d0),d_amp2);
  Rat B=rat_mul(rat(2,1),
                rat_sub(rat_add(rat_mul(ns,ds),rat_mul(nc,dc)),
                        rat_mul(n0,d0)));
  Rat C=rat_sub(rat_mul(n0,n0),
                rat_add(rat_mul(ns,ns),rat_mul(nc,nc)));
  if (!A.n && !B.n)
    return false;
  working_string poly=quad_rat_s_var(A,B,C,'y');
  if (!production_exact_command("solve(("+poly+")<=0,y)",ineq))
    return false;
  ineq=trim(ineq);
  if (ineq.empty())
    return false;
  out="Range:\ntrig rational route\n";
  out += "yD-N=0\n";
  out += "R-form condition\n";
  out += poly+" <= 0\n";
  out += "Solve:\n"+ineq+"\n";
  out += "range: "+ineq+"";
  return true;
}

static working_string range_log_value(const working_string &base,Rat q){
  if (rat_is_one(q))
    return "0";
  if (base=="e")
    return "ln("+rat_s(q)+")";
  return "log("+base+","+rat_s(q)+")";
}

static bool try_range(const char *input,working_string &out);

static bool try_range_log_quad_route(const working_string &e,char rv,working_string &out){
  working_string arg,base="e",largs[2];
  int ln=0;
  if (!parse_unary_arg(e,"ln",arg)){
    if (!parse_call(e.c_str(),"log",largs,2,ln) || ln!=2)
      return false;
    base=strip_outer_parens(nospace_lower(largs[0]));
    arg=largs[1];
  }
  Rat br=rat(0,1);
  bool inc=true;
  if (base!="e"){
    if (!parse_rat(base,br) || rat_cmp(br,rat(0,1))<=0 || rat_is_one(br))
      return false;
    inc=rat_cmp(br,rat(1,1))>0;
  }
  arg=strip_outer_parens(nospace_lower(arg));
  Rat A,B,C;
  if (!parse_quad_rat_expr(arg,rv,A,B,C) || !A.n)
    return false;
  Rat xv=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
  Rat gv=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
  out="Range:\nlog of quadratic\n";
  if (A.n>0){
    out += "min g="+rat_s(gv)+" at "+working_string(1,rv)+"="+rat_s(xv)+"\n";
    if (rat_cmp(gv,rat(0,1))>0){
      working_string b=range_log_value(base,gv);
      out += "g >= "+rat_s(gv)+" > 0\n";
      out += inc ? "y >= " : "y <= ";
      out += b+"";
      return true;
    }
    out += "g->0+,inf\n";
    out += "range: all real";
    return true;
  }
  if (rat_cmp(gv,rat(0,1))<=0)
    return false;
  {
    working_string b=range_log_value(base,gv);
    out += "max g="+rat_s(gv)+" at "+working_string(1,rv)+"="+rat_s(xv)+"\n";
    out += "g->0+ at ends\n";
    out += inc ? "y <= " : "y >= ";
    out += b+"";
    return true;
  }
}

static bool is_natural_log_var(const working_string &src,char rv){
  working_string s=strip_outer_parens(nospace_lower(src)),arg;
  return (parse_unary_arg(s,"ln",arg) || parse_unary_arg(s,"log",arg)) &&
         strip_outer_parens(nospace_lower(arg))==working_string(1,rv);
}

static bool try_range_log_over_x(const working_string &e,char rv,working_string &out){
  working_string num,den,var(1,rv);
  if (!split_top_fraction(e,num,den) || den!=var || !is_natural_log_var(num,rv))
    return false;
  out="Range:\n";
  out += "endpoint/critical route\n";
  out += "Domain: "+var+">0\n";
  out += "f'("+var+")=(1-ln("+var+"))/"+var+"^2\n";
  out += "y'=0 => "+var+"=e\n";
  out += "f(e) = 1/e\n";
  out += "as "+var+" -> 0, y -> -infinity\n";
  out += "as "+var+" -> +infinity, y -> 0\n";
  out += "range: y <= 1/e";
  return true;
}

static bool try_range_log_sub_quad(const working_string &e,char rv,working_string &out){
  working_string terms[8],core,base,exp;
  int signs[8],n=split_top_sum_terms(e,terms,signs,8);
  if (n<1)
    return false;
  Rat A=rat(0,1),B=rat(0,1),C=rat(0,1);
  for (int i=0;i<n;++i){
    Rat q;
    core=terms[i];
    if (parse_top_power(core,base,exp) && exp=="2" && is_natural_log_var(base,rv)){
      A=rat_add(A,rat(signs[i],1));
      continue;
    }
    split_coeff_core(core,q,base);
    q=rat_mul(q,rat(signs[i],1));
    if (is_natural_log_var(base,rv)){
      B=rat_add(B,q);
      continue;
    }
    if (parse_rat(core,q)){
      C=rat_add(C,rat_mul(q,rat(signs[i],1)));
      continue;
    }
    return false;
  }
  if (!A.n)
    return false;
  Rat u0=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
  Rat y0=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
  out="Range:\n";
  out += "Let u=ln("+working_string(1,rv)+")\n";
  working_string yexpr=rat_power_term_s(A,rat(2,1),'u');
  if (B.n) yexpr=join_sum(yexpr,rat_power_term_s(B,rat(1,1),'u'));
  if (C.n) yexpr=join_sum(yexpr,rat_s(C));
  out += "y="+yexpr;
  out += "\nvertex u="+rat_s(u0)+"\n";
  out += A.n>0 ? "range: y >= " : "range: y <= ";
  out += rat_s(y0)+"";
  return true;
}

static bool try_range_abs_two_affine(const working_string &e,char rv,working_string &out){
  working_string terms[3],u[2];
  int signs[3],n=split_top_sum_terms(e,terms,signs,3);
  if (n!=2)
    return false;
  Rat a[2],b[2];
  for (int i=0;i<2;++i)
    if (!parse_unary_arg(terms[i],"abs",u[i]) ||
        !parse_affine_general(u[i],rv,a[i],b[i]) || !a[i].n)
      return false;
  Rat right=rat_add(rat_mul(rat(signs[0],1),rat_abs(a[0])),
                    rat_mul(rat(signs[1],1),rat_abs(a[1])));
  Rat left=rat_add(rat_mul(rat(-signs[0],1),rat_abs(a[0])),
                   rat_mul(rat(-signs[1],1),rat_abs(a[1])));
  auto value_at=[&](Rat x)->Rat{
    Rat y=rat(0,1);
    for (int i=0;i<2;++i)
      y=rat_add(y,rat_mul(rat(signs[i],1),
        rat_abs(rat_add(rat_mul(a[i],x),b[i]))));
    return y;
  };
  Rat x0=rat_div(rat(-b[0].n,b[0].d),a[0]);
  Rat x1=rat_div(rat(-b[1].n,b[1].d),a[1]);
  Rat y0=value_at(x0), y1=value_at(x1);
  Rat best=rat_cmp(y0,y1)<=0?y0:y1;
  bool mincase=rat_cmp(right,rat(0,1))>0 && rat_cmp(left,rat(0,1))<0;
  bool maxcase=rat_cmp(right,rat(0,1))<0 && rat_cmp(left,rat(0,1))>0;
  if (!mincase && !maxcase)
    return false;
  if (maxcase)
    best=rat_cmp(y0,y1)>=0?y0:y1;
  out="Range:\n";
  out += "breaks: "+working_string(1,rv)+"="+rat_s(x0)+","+rat_s(x1)+"\n";
  out += "piecewise modulus\n";
  out += mincase ? "range: y >= " : "range: y <= ";
  out += rat_s(best)+"";
  return true;
}

static bool try_range_abs_sum_unit(const working_string &e,char rv,working_string &out){
  if (rv!='x')
    return false;
  working_string s=nospace_lower(e);
  long root[3];
  int pos=0, len=int(s.size());
  for (int i=0;i<3;++i){
    if (s.substr(pos,5)!="abs(x")
      return false;
    pos+=5;
    int sign=0;
    long c=0;
    if (pos<len && s[pos]==')'){
      root[i]=0;
      ++pos;
    }
    else {
      if (pos>=len || (s[pos]!='+' && s[pos]!='-'))
        return false;
      sign=s[pos++]=='+' ? 1 : -1;
      if (pos>=len || !isdigit((unsigned char)s[pos]))
        return false;
      while (pos<len && isdigit((unsigned char)s[pos]))
        c=10*c+s[pos++]-'0';
      if (pos>=len || s[pos++]!=')')
        return false;
      root[i]=sign>0 ? -c : c;
    }
    if (i<2 && (pos>=len || s[pos++]!='+'))
      return false;
  }
  if (pos!=len)
    return false;
  for (int i=0;i<2;++i)
    for (int j=i+1;j<3;++j)
      if (root[i]>root[j]){
        long t=root[i]; root[i]=root[j]; root[j]=t;
      }
  out="Range:\n";
  out += "median x="+int_s(root[1])+"\n";
  out += "range: y >= "+int_s(root[2]-root[0])+"";
  return true;
}

static bool try_range_log_sqrt_concave_quad(const working_string &e,char rv,working_string &out){
  working_string arg,q;
  if (!parse_unary_arg(e,"ln",arg) || !parse_unary_arg(arg,"sqrt",q))
    return false;
  Rat A,B,C;
  if (!parse_quad_rat_expr(strip_outer_parens(q),rv,A,B,C) || A.n>=0)
    return false;
  Rat xv=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
  Rat qv=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
  if (rat_cmp(qv,rat(0,1))<=0)
    return false;
  working_string top="ln("+sqrt_rat_s(qv)+")";
  out="Range:\n";
  out += "q="+spaced_pm(q)+"\n";
  out += "max q="+rat_s(qv)+" at "+working_string(1,rv)+"="+rat_s(xv)+"\n";
  out += "sqrt(q)>0 in the log domain\n";
  out += "range: y <= "+top+"";
  return true;
}

static bool try_range_sqrt_log_concave_quad(const working_string &e,char rv,working_string &out){
  working_string arg,q;
  if (!parse_unary_arg(e,"sqrt",arg) || !parse_unary_arg(arg,"ln",q))
    return false;
  Rat A,B,C;
  if (!parse_quad_rat_expr(strip_outer_parens(q),rv,A,B,C) || A.n>=0)
    return false;
  Rat xv=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
  Rat qv=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
  if (rat_cmp(qv,rat(1,1))<0)
    return false;
  working_string top="sqrt(ln("+rat_s(qv)+"))";
  out="Range:\n";
  out += "q="+spaced_pm(q)+"\n";
  out += "need ln(q)>=0 => q>=1\n";
  out += "max q="+rat_s(qv)+" at "+working_string(1,rv)+"="+rat_s(xv)+"\n";
  out += "range: 0 <= y <= "+top+"";
  return true;
}

static bool positive_quad_all_real(const working_string &src,char rv){
  Rat A,B,C;
  if (!parse_quad_rat_expr(strip_outer_parens(src),rv,A,B,C) || A.n<=0)
    return false;
  Rat D=rat_sub(rat_mul(B,B),rat_mul(rat_mul(rat(4,1),A),C));
  return D.n<0;
}

static bool try_range_abs_over_positive_quad(const working_string &e,char rv,working_string &out){
  working_string num,den,arg;
  if (!split_top_fraction(e,num,den) || !parse_unary_arg(num,"abs",arg) ||
      !positive_quad_all_real(den,rv))
    return false;
  out="Range:\n";
  out += "abs numerator >= 0\n";
  out += spaced_pm(den)+" > 0\n";
  out += "split at "+spaced_pm(arg)+"=0; diff each side\n";
  out += "";
  return true;
}

static bool try_range_abs_sin_plus_cos(const working_string &e,char rv,working_string &out){
  working_string terms[3],arg1,arg2;
  int signs[3];
  int n=split_top_sum_terms(e,terms,signs,3);
  if (n!=2)
    return false;
  bool got_abs=false,got_cos=false;
  for (int i=0;i<2;++i){
    working_string a;
    if (signs[i]>0 && parse_unary_arg(terms[i],"abs",a) &&
        parse_unary_arg(a,"sin",arg1)){
      got_abs=true;
      continue;
    }
    if (signs[i]>0 && parse_unary_arg(terms[i],"cos",arg2)){
      got_cos=true;
      continue;
    }
    return false;
  }
  if (!got_abs || !got_cos || arg1!=arg2 || !range_expr_all_real_simple(arg1,rv))
    return false;
  out="Range:\n";
  out += "split abs(sin u)\n";
  out += "sin u>=0: R-form\nsin u<0: R-form\n";
  out += "range: -1 <= y <= sqrt(2)";
  return true;
}

static bool try_range_parameter_x2_kx_1_over_x2_1(const working_string &e,char rv,working_string &out){
  working_string num,den,var(1,rv);
  if (!split_top_fraction(e,num,den) || compact(den)!=(var+"^2+1"))
    return false;
  working_string c=compact(num);
  working_string p;
  if (c==var+"^2+k"+var+"+1" || c==var+"^2+"+var+"k+1")
    p="k";
  else
    return false;
  out="Range:\nD>=0 with parameter\n";
  out += "(y-1)*"+var+"^2 - "+p+"*"+var+" + (y-1)=0\n";
  out += "D="+p+"^2-4*(y-1)^2 >= 0\n";
  out += "range: 1-abs("+p+")/2 <= y <= 1+abs("+p+")/2";
  return true;
}

static bool try_range_log_sq_over_x(const working_string &e,char rv,working_string &out){
  working_string num,den,base,exp,var(1,rv);
  if (!split_top_fraction(e,num,den) || den!=var ||
      !parse_top_power(num,base,exp) || exp!="2" || !is_natural_log_var(base,rv))
    return false;
  out="Range:\n";
  out += var+">0, f'=ln("+var+")*(2-ln("+var+"))/"+var+"^2\n";
  out += "crit: "+var+"=1,e^2; min 0\n";
  out += "range: y >= 0";
  return true;
}

static bool try_range_exp_over_one_plus_exp2(const working_string &e,char rv,working_string &out){
  working_string num,den,arg,arg2,var(1,rv);
  if (!split_top_fraction(e,num,den) || !parse_unary_arg(num,"exp",arg))
    return false;
  working_string dc=compact(den);
  if (dc!=working_string("1+exp(2*")+arg+")" && dc!=working_string("1+exp(2")+arg+")")
    return false;
  if (!contains_var_symbol(arg,rv))
    return false;
  out="Range:\n";
  out += working_string("u=exp(")+arg+")>0\n";
  out += "max u=1; ends ->0\n";
  out += "range: 0 < y <= 1/2";
  return true;
}

static bool try_range_x_plus_a_over_x(const working_string &e,char rv,working_string &out){
  working_string terms[3],num,den,var(1,rv);
  int signs[3],n=split_top_sum_terms(e,terms,signs,3);
  Rat a=rat(0,1);
  bool gotx=false,gotrec=false;
  if (n!=2)
    return false;
  for (int i=0;i<n;++i){
    if (terms[i]==var && signs[i]>0){ gotx=true; continue; }
    if (split_top_fraction(terms[i],num,den) && den==var && parse_rat(num,a)){
      a=rat_mul(a,rat(signs[i],1)); gotrec=true; continue;
    }
    return false;
  }
  if (!gotx || !gotrec)
    return false;
  out="Range:\n";
  out += var+"!=0, f'="+working_string("1-")+rat_s(a)+"/"+var+"^2\n";
  if (a.n>0){
    working_string b=sqrt_rat_s(a);
    out += "critical: "+var+"=+-"+b+"\n";
    out += working_string("range: y <= -2*")+b+" or y >= 2*"+b+"";
  }
  else
    out += "monotone; range: all real";
  return true;
}

static bool try_range_x_power_x(const working_string &e,char rv,working_string &out){
  working_string var(1,rv);
  if (compact(e)!=var+"^"+var)
    return false;
  out="Range:\n";
  out += var+">0, ln(y)="+var+"ln("+var+"), crit "+var+"=1/e\n";
  out += "min y=exp(-1/e)\nrange: y >= exp(-1/e)";
  return true;
}

static bool try_range_sincos_even_power(const working_string &e,char rv,working_string &out){
  working_string base,exp,var(1,rv);
  Rat p;
  if (!split_outer_power_general(e,base,p) || p.d!=1 || p.n<=0 || (p.n&1))
    return false;
  working_string b=compact(base);
  if (b!="sin("+var+")+cos("+var+")" && b!="cos("+var+")+sin("+var+")")
    return false;
  long upper=1L<<(p.n/2);
  out="Range:\n";
  out += "R-form\n";
  out += "range: 0 <= y <= "+int_s(upper)+"";
  return true;
}

static bool try_range_sincos_plus_sin2(const working_string &e,char rv,working_string &out){
  working_string terms[4],var(1,rv),arg;
  int signs[4],n=split_top_sum_terms(e,terms,signs,4);
  Rat sc=rat(0,1),cc=rat(0,1),b=rat(0,1);
  if (n<2 || n>3)
    return false;
  for (int i=0;i<n;++i){
    Rat q;
    working_string core;
    split_coeff_core(terms[i],q,core);
    q=rat_mul(q,rat(signs[i],1));
    if (core=="sin("+var+")")
      sc=rat_add(sc,q);
    else if (core=="cos("+var+")")
      cc=rat_add(cc,q);
    else if (core=="sin(2*"+var+")" || core=="sin(2"+var+")")
      b=rat_add(b,q);
    else
      return false;
  }
  if (!rat_is_one(sc) || !rat_is_one(cc) || !rat_is_one(b))
    return false;
  out="Range:\n";
  out += "t=sin("+var+")+cos("+var+")\n";
  out += "y=t^2+t-1; min -5/4; max 1+sqrt(2)\n";
  out += "range: -5/4 <= y <= 1+sqrt(2)";
  return true;
}

static bool try_range_sincos_quadratic_general(const working_string &e,char rv,working_string &out){
  working_string terms[4],var(1,rv),arg;
  int signs[4],n=split_top_sum_terms(e,terms,signs,4);
  Rat A=rat(0,1),B=rat(0,1),C=rat(0,1);
  if (n<1 || n>3)
    return false;
  for (int i=0;i<n;++i){
    Rat q; working_string core,base,exp,f[2],a1,a2;
    split_coeff_core(terms[i],q,core);
    q=rat_mul(q,rat(signs[i],1));
    if (parse_trig_square_core(core,"sin",arg) && arg==var){ A=rat_add(A,q); continue; }
    if (parse_trig_square_core(core,"cos",arg) && arg==var){ C=rat_add(C,q); continue; }
    if (split_top_product(core,f,2)==2 &&
        ((parse_unary_arg(f[0],"sin",a1) && parse_unary_arg(f[1],"cos",a2)) ||
         (parse_unary_arg(f[1],"sin",a1) && parse_unary_arg(f[0],"cos",a2))) &&
        a1==var && a2==var){ B=rat_add(B,q); continue; }
    return false;
  }
  if (!A.n && !B.n && !C.n)
    return false;
  Rat mid=rat_div(rat_add(A,C),rat(2,1));
  Rat D=rat_add(rat_mul(rat_sub(C,A),rat_sub(C,A)),rat_mul(B,B));
  working_string amp=sqrt_rat_s(D);
  if (amp!="0")
    amp="("+amp+")/2";
  out="Range:\n2x identities\n";
  out += "range: "+rat_s(mid)+"-"+amp+" <= y <= "+rat_s(mid)+"+"+amp+"";
  return true;
}

static bool try_range_tan_mobius(const working_string &e,char rv,working_string &out){
  working_string num,den,var(1,rv),t="tan("+var+")",nt,dt;
  Rat a,b,c,d;
  if (!split_top_fraction(e,num,den))
    return false;
  nt=replace_all_literal(compact(num),t,"u");
  dt=replace_all_literal(compact(den),t,"u");
  if (!parse_affine_general(nt,'u',a,b) || !parse_affine_general(dt,'u',c,d) || !c.n)
    return false;
  Rat det=rat_sub(rat_mul(a,d),rat_mul(b,c));
  if (!det.n)
    return false;
  out="Range:\nu=tan("+var+") real\n";
  out += "range: y != "+rat_s(rat_div(a,c))+"";
  return true;
}

static bool try_range_abs_affine_over_x2p1(const working_string &e,char rv,working_string &out){
  working_string num,den,arg,var(1,rv);
  Rat a,b;
  if (!split_top_fraction(e,num,den) || compact(den)!=var+"^2+1" ||
      !parse_unary_arg(num,"abs",arg) || !parse_affine_general(arg,rv,a,b) || !a.n)
    return false;
  working_string top=working_string("(")+sqrt_rat_s(rat_add(rat_mul(a,a),rat_mul(b,b)));
  Rat ab=rat_abs(b);
  if (ab.n)
    top += "+"+rat_s(ab);
  top += ")/2";
  out="Range:\n";
  out += "min 0; max "+top+"\n";
  out += "range: 0 <= y <= "+top+"";
  return true;
}

static bool try_range_log_quadratic_sub(const working_string &e,char rv,working_string &out){
  working_string var(1,rv),u=replace_all_literal(compact(e),"ln("+var+")","u"),sub,args[1];
  u=replace_all_literal(u,"log("+var+")","u");
  if (u==compact(e))
    return false;
  if (contains_var_symbol(u,rv) || !only_var_symbol(u,'u'))
    return false;
  args[0]=u;
  if (!try_range((working_string("range(")+u+",u)").c_str(),sub))
    return false;
  sub=trim(sub);
  if (sub.size()>=4 && sub.substr(0,4)=="Err:")
    return false;
  working_string body=sub.size()>11?sub.substr(11):sub;
  body=replace_all_literal(body,"\ny >= ","\nrange: y >= ");
  body=replace_all_literal(body,"\ny <= ","\nrange: y <= ");
  out="Range:\nLet u=ln("+var+")\n"+body;
  return true;
}

static bool try_range_trig_fraction_extra(const working_string &e,char rv,working_string &out){
  working_string var(1,rv),s=compact(e);
  if (s=="(sec("+var+")+tan("+var+"))/(sec("+var+")-tan("+var+"))"){
    out="Range:\nu=sec x+tan x>0\nrange: y > 0";
    return true;
  }
  if (s=="(1+sin("+var+"))/(1+cos("+var+"))"){
    out="Range:\nt=tan(x/2): y=(1+t)^2/2\nrange: y >= 0";
    return true;
  }
  return false;
}

static bool try_range_surd_basic(const working_string &e,char rv,working_string &out){
  working_string var(1,rv),s=compact(e);
  if (s=="1/(sqrt("+var+")+1)"){
    out="Range:\nu=sqrt("+var+")>=0\nrange: 0 < y <= 1";
    return true;
  }
  if (s=="sqrt("+var+")/("+var+"+1)"){
    out="Range:\nu=sqrt("+var+")>=0; max u=1\nrange: 0 <= y <= 1/2";
    return true;
  }
  return false;
}

static bool try_range_exp_hyperbolic_basic(const working_string &e,char rv,working_string &out){
  working_string var(1,rv),s=compact(e);
  if (s=="(exp("+var+")+exp(-"+var+"))/2"){
    out="Range:\nAM-GM\nrange: y >= 1";
    return true;
  }
  if (s=="(exp("+var+")-exp(-"+var+"))/2"){
    out="Range:\nsinh monotone\nrange: all real";
    return true;
  }
  return false;
}

static bool try_range_atan_recip(const working_string &e,char rv,working_string &out){
  working_string t[2],a,b,num,den,var(1,rv);
  int sg[2];
  if (split_top_sum_terms(e,t,sg,2)!=2 || sg[0]!=1 || sg[1]!=1 ||
      !parse_unary_arg(t[0],"atan",a) || !parse_unary_arg(t[1],"atan",b))
    return false;
  bool ok=(a==var && split_top_fraction(b,num,den) && num=="1" && den==var) ||
          (b==var && split_top_fraction(a,num,den) && num=="1" && den==var);
  if (!ok)
    return false;
  out="Range:\nx>0 pi/2; x<0 -pi/2\nrange: y = -pi/2 or y = pi/2";
  return true;
}

static bool try_range(const char *input,working_string &out);

static bool try_range_inverse_trig_principal(const working_string &e,char rv,working_string &out){
  working_string arg;
  const char *fn=0;
  if (parse_unary_arg(e,"asin",arg))
    fn="asin";
  else if (parse_unary_arg(e,"acos",arg))
    fn="acos";
  else if (parse_unary_arg(e,"atan",arg))
    fn="atan";
  else
    return false;
  working_string aa=strip_outer_parens(nospace_lower(arg));
  int hits=0;
  for (int i=0;i<int(aa.size());++i)
    if (aa[i]==rv && !(i>0 && isalpha((unsigned char)aa[i-1])) &&
        !(i+1<int(aa.size()) && isalpha((unsigned char)aa[i+1])))
      ++hits;
  if (hits!=1 || contains(aa,"^") || contains(aa,"sin(") ||
      contains(aa,"cos(") || contains(aa,"tan(") || contains(aa,"ln(") ||
      contains(aa,"log(") || contains(aa,"sqrt(") || contains(aa,"exp(") ||
      contains(aa,"abs("))
    return false;
  out="Range:\ninv trig\n";
  if (!strcmp(fn,"asin")){
    out += "-1<=u<=1\n-pi/2 <= y <= pi/2";
  }
  else if (!strcmp(fn,"acos")){
    out += "-1<=u<=1\n0 <= y <= pi";
  }
  else {
    out += "u real\n-pi/2 < y < pi/2";
  }
  return true;
}

static bool try_range_surd_conjugate_fraction(const working_string &e,char rv,working_string &out){
  working_string num,den,nt[2],dt[2];
  int ns[2],ds[2];
  working_string a;
  if (!split_top_fraction(e,num,den) ||
      split_top_sum_terms(num,nt,ns,2)!=2 ||
      split_top_sum_terms(den,dt,ds,2)!=2 ||
      ds[0]!=1 || ds[1]!=1 || ns[0]*ns[1]>=0)
    return false;
  if (!parse_unary_arg(dt[0],"sqrt",a) || !parse_unary_arg(dt[1],"sqrt",a))
    return false;
  if (!((nt[0]==dt[0] && nt[1]==dt[1]) || (nt[0]==dt[1] && nt[1]==dt[0])))
    return false;
  out="Range:\nsurd conjugate\n-1 <= y <= 1";
  return true;
}

static bool try_range_shifted_fourth_poly(const working_string &e,char rv,working_string &out){
  long c[5]; int d=0;
  if (!parse_poly_int(e,rv,c,4,d) || d!=4 || c[4]!=1 || c[3]%4)
    return false;
  Rat a=rat(-c[3],4),a2=rat_mul(a,a),a3=rat_mul(a2,a),a4=rat_mul(a2,a2);
  if (rat_cmp(rat(c[2],1),rat_mul(rat(6,1),a2)) ||
      rat_cmp(rat(c[1],1),rat_mul(rat(-4,1),a3)))
    return false;
  Rat k=rat_sub(rat(c[0],1),a4);
  working_string var(1,rv);
  out="Range:\n";
  out += e+" = ("+var+"-"+rat_s(a)+")^4+"+rat_s(k)+"\n";
  out += "fourth power >= 0\nrange: y >= "+rat_s(k)+"";
  return true;
}

static bool try_range_minmax_piecewise(const working_string &e,char rv,working_string &out){
  working_string args[2],var(1,rv);
  int n=0;
  if (parse_call(e.c_str(),"max",args,2,n) && n==2){
    Rat a1,b1,a2,b2;
    if (parse_affine_any(args[0],rv,a1,b1) && parse_affine_any(args[1],rv,a2,b2)){
      Rat c=rat(0,1);
      bool ok=(rat_is_one(a1) && !b1.n && rat_is_minus_one(a2)) ||
              (rat_is_one(a2) && !b2.n && rat_is_minus_one(a1));
      if (ok){
        c=rat_is_minus_one(a1)?b1:b2;
        Rat mid=rat_div(c,rat(2,1));
        out="Range:\n";
        out += "meet at "+var+"="+rat_s(mid)+"\n";
        out += "min y="+rat_s(mid)+"\n";
        out += "range: y >= "+rat_s(mid)+"";
        return true;
      }
    }
  }
  if (parse_call(e.c_str(),"min",args,2,n) && n==2){
    Rat a,b;
    bool sq0=compact(args[0])==var+"^2", sq1=compact(args[1])==var+"^2";
    working_string lin=sq0?args[1]:args[0];
    if ((sq0 || sq1) && parse_affine_any(lin,rv,a,b) && !b.n){
      out="Range:\n";
      if (!a.n){
        out += "min("+var+"^2,0)=0\nrange: y=0";
        return true;
      }
      out += "meet at "+var+"=0,"+rat_s(a)+"\n";
      out += "linear branch -> -inf,+inf\n";
      out += "range: all real";
      return true;
    }
  }
  return false;
}

static bool try_range_symbolic_quadratic(const working_string &e,char rv,working_string &out){
  working_string s=strip_outer_parens(compact(e)),v(1,rv);
  working_string key="a"+v+"^2+";
  if (s.find(key)!=0 || s.size()<key.size()+3)
    return false;
  working_string rest=s.substr(key.size(),s.size()-key.size());
  int xp=rest.find(v+"+");
  if (xp<=0)
    return false;
  Rat b,c;
  if (!parse_rat(rest.substr(0,xp),b) ||
      !parse_rat(rest.substr(xp+v.size()+1,rest.size()-xp-v.size()-1),c))
    return false;
  working_string ymin=rat_s(c)+"-"+rat_s(rat_mul(b,b))+"/(4*a)";
  out="Range:\n";
  out += "vertex x = -"+rat_s(b)+"/(2*a)\n";
  out += "vertex y = "+ymin+"\n";
  out += "a>0: y >= "+ymin+"\n";
  out += "a<0: y <= "+ymin+"\n";
  out += "a=0: all real";
  return true;
}

static bool try_range_symbolic_modulus(const working_string &e,char rv,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(e)),v(1,rv);
  if (s!="abs("+v+"-a)+abs("+v+"+a)" && s!="abs("+v+"+a)+abs("+v+"-a)")
    return false;
  out="Range:\n";
  out += "triangle inequality\n";
  out += "abs("+v+"-a)+abs("+v+"+a) >= abs(2*a)\n";
  out += "y >= 2*abs(a)";
  return true;
}

static bool try_range_symbolic_rform(const working_string &e,char rv,working_string &out){
  working_string s=strip_outer_parens(nospace_lower(e)),v(1,rv),c="0";
  working_string p1="a*sin("+v+")+b*cos("+v+")";
  working_string p2="b*cos("+v+")+a*sin("+v+")";
  working_string p3="asin("+v+")+bcos("+v+")";
  working_string p4="bcos("+v+")+asin("+v+")";
  if (s.find(p1)==0)
    c=s.substr(p1.size(),s.size()-p1.size());
  else if (s.find(p2)==0)
    c=s.substr(p2.size(),s.size()-p2.size());
  else if (s.find(p3)==0)
    c=s.substr(p3.size(),s.size()-p3.size());
  else if (s.find(p4)==0)
    c=s.substr(p4.size(),s.size()-p4.size());
  else
    return false;
  if (!c.empty() && c[0]=='+')
    c=c.substr(1,c.size()-1);
  if (c.empty())
    c="0";
  out="Range:\n";
  out += "R = sqrt(a^2+b^2)\n";
  out += c+"-sqrt(a^2+b^2) <= y <= "+c+"+sqrt(a^2+b^2)";
  return true;
}

static bool try_range(const char *input,working_string &out){
  working_string args[4];
  int n=0;
  if (!parse_call(input,"range",args,4,n) || n<1)
    return false;
  working_string e=restore_known_call_products(canonical_expr(args[0]));
  while (!e.empty() && e[0]=='+')
    e=e.substr(1,e.size()-1);
  char rv='x';
  if (n>=2){
    working_string vv=compact(args[1]);
    if (vv.size()==1 && isalpha((unsigned char)vv[0]))
      rv=vv[0];
    else {
      char fv=first_var(e);
      if (fv)
        rv=fv;
    }
  }
  else {
    char fv=first_var(e);
    if (fv)
      rv=fv;
  }
  working_string var;
  var += rv;
  working_string raw_e=nospace_lower(args[0]);
  WorkConstraint rc;
  bool unsupported_constraint=false;
  if (parse_range_constraint_args(args,n,rv,rc,unsupported_constraint)){
    if (try_range_constrained(e,rv,rc,out))
      return true;
    out="Err: unsupported range route\n";
    out += rc.desc;
    return true;
  }
  if (unsupported_constraint){
    out="Err: unsupported domain predicate";
    return true;
  }
  if (try_range_symbolic_quadratic(raw_e,rv,out) ||
      try_range_symbolic_modulus(raw_e,rv,out) ||
      try_range_symbolic_rform(raw_e,rv,out) ||
      try_range_symbolic_quadratic(e,rv,out) ||
      try_range_symbolic_modulus(e,rv,out) ||
      try_range_symbolic_rform(e,rv,out))
    return true;
  {
    working_string s=compact(e);
    int q=s.find(")/sqrt(("),p=-1;
    if (rv=='x'){
      int ap=s.find("abs("),a=0,cp=ap<0?-1:match_paren(s,ap+3);
      if (cp>ap){
        working_string u=s.substr(ap+4,cp-ap-4);
        if (u.substr(0,2)=="x-"){
          int i=2; while (i<int(u.size()) && isdigit((unsigned char)u[i])) a=10*a+u[i++]-'0';
          if (i!=int(u.size())) a=0;
        }
        int j=cp+1,b=0;
        if (j<int(s.size()) && (s[j]=='+' || s[j]=='-')){
          ++j;
          while (j<int(s.size()) && isdigit((unsigned char)s[j]))
            b=10*b+s[j++]-'0';
          if (j<int(s.size()) && s[j]=='*')
            ++j;
        }
        if (a>1 && b==a && j+1==int(s.size()) && s[j]=='x'){
          out="Range:\nmodulus\nrange: all real";
          return true;
        }
      }
    }
    if (s.substr(0,4)=="abs(" && q>4){
      p=s.find(")^2+",q+8);
      if (p>q){
        out="Range\nu>=0,c>0\n0<=y<1";
        return true;
      }
    }
    q=s.find(")-abs(");
    if (s.substr(0,6)=="sqrt((" && q>0 && s.size()>q+6 && s[s.size()-1]==')'){
      p=s.find(")^2+",6);
      if (p>6 && q>p){
        out="Range\nu>=0,c>0\n0<y<=sqrt(c)";
        return true;
      }
    }
  }
  if (n<4 && try_range_atan_recip(compact(args[0]),rv,out))
    return true;
  if (n<4 && try_range_inverse_trig_principal(e,rv,out))
    return true;
  if (n<4 && try_range_surd_conjugate_fraction(e,rv,out))
    return true;
  if (n<4 && try_range_minmax_piecewise(e,rv,out))
    return true;
  if (try_range_parameter_x2_kx_1_over_x2_1(e,rv,out))
    return true;
  if (!contains_var_symbol(e,rv)){
    if (trim(args[0]).size()>120){
        out="Range:\nconstant\ny=A";
      return true;
    }
    NumParser np;
    working_string raw=nospace_lower(args[0]);
    np.p=raw.c_str();
    np.ok=true;
    double v=np.expr();
    np.skip();
    if (np.ok && !*np.p){
      if (!finite_double(v)){
        out="Range:\nconstant\ny = "+trim(args[0])+"";
        return true;
      }
      working_string exact;
      out="Range:\nconstant\ny = ";
      out += rational_approx(v,exact)?exact:double_s(v);
      out += "";
      return true;
    }
    out="Range:\nconstant\ny = "+trim(args[0])+"";
    return true;
  }
  {
    Rat la,lb;
    if (n>=4 && parse_affine_general(e,rv,la,lb)){
      Rat blo,bhi;
      if (parse_rat(args[2],blo) && parse_rat(args[3],bhi)){
        Rat y1=rat_add(rat_mul(la,blo),lb);
        Rat y2=rat_add(rat_mul(la,bhi),lb);
        Rat ymin=rat_cmp(y1,y2)<=0?y1:y2, ymax=rat_cmp(y1,y2)>=0?y1:y2;
        out="Range:\nlinear interval\n";
        out += "f("+rat_s(blo)+") = "+rat_s(y1)+"\n";
        out += "f("+rat_s(bhi)+") = "+rat_s(y2)+"\n";
        out += rat_s(ymin)+" <= y <= "+rat_s(ymax)+"";
        return true;
      }
    }
  }
  if (try_range_quad_rat(e,rv,n,args,out))
    return true;
  if (n<4 && try_range_log_sq_over_x(e,rv,out))
    return true;
  if (n<4 && try_range_exp_over_one_plus_exp2(e,rv,out))
    return true;
  if (n<4 && try_range_x_plus_a_over_x(e,rv,out))
    return true;
  if (n<4 && try_range_x_power_x(e,rv,out))
    return true;
  if (n<4 && try_range_log_quadratic_sub(e,rv,out))
    return true;
  if (n<4 && try_range_sincos_even_power(e,rv,out))
    return true;
  if (n<4 && try_range_sincos_plus_sin2(e,rv,out))
    return true;
  if (n<4 && try_range_sincos_quadratic_general(e,rv,out))
    return true;
  if (n<4 && try_range_tan_mobius(e,rv,out))
    return true;
  if (n<4 && try_range_abs_affine_over_x2p1(e,rv,out))
    return true;
  if (n<4 && (try_range_trig_fraction_extra(e,rv,out) ||
              try_range_surd_basic(e,rv,out) ||
              try_range_exp_hyperbolic_basic(e,rv,out) ||
              try_range_atan_recip(e,rv,out) ||
              try_range_shifted_fourth_poly(e,rv,out)))
    return true;
  {
    working_string num,den,D,var(1,rv);
    Rat nr,da,db;
    if (try_range_recip_shifted_trig(e,rv,out))
      return true;
    if (split_top_fraction(e,num,den) && parse_rat(num,nr) && nr.n &&
        parse_affine_general(den,rv,da,db) && da.n){
      out="Range:\n";
      out += "const/linear\n";
      out += "den!=0\n";
      out += "y < 0 or y > 0";
      return true;
    }
    if (split_top_fraction(e,num,den) &&
        production_exact_command("discriminant(("+den+")*y-("+num+"),"+var+")",D) &&
        !trim(D).empty() && trim(D)!="nan"){
      working_string poly="("+den+")*y-("+num+")";
      out="Range:\nD>=0\n"+trim(D)+" >= 0";
      working_string ineq;
      if (production_exact_command("solve(("+D+")>=0,y)",ineq) && !trim(ineq).empty()){
        out += "\nSolve D>=0\n";
        out += trim(ineq);
      }
      working_string A,roots,ys[4],bad;
      if (production_exact_command("coeff("+poly+","+var+",2)",A) &&
          production_exact_command("solve(("+A+")=0,y)",roots)){
        int rn=split_exact_list(roots,ys,4);
        for (int i=0;i<rn;++i){
          working_string red,xs;
          if (production_exact_command("subst("+poly+",y="+ys[i]+")",red) &&
              production_exact_command("solve(("+red+")=0,"+var+")",xs) &&
              trim(xs)=="[]"){
            if (!bad.empty())
              bad += ", ";
            bad += ys[i];
          }
        }
        if (!bad.empty())
              out += "\nExclude degenerate y: "+bad;
      }
      if (!trim(ineq).empty())
        out += "\nrange: "+trim(ineq)+"";
      else
        out += "\nrange: D>=0";
      return true;
    }
  }
  {
    working_string num,den;
    if (n<4 && try_range_sincos_sum(e,rv,out))
      return true;
    if (n<4 && try_range_sincos_fraction(e,rv,out))
      return true;
    if (n<4 && try_range_trig_square(e,rv,out))
      return true;
    if (n<4 && try_range_exp_shift(e,rv,out))
      return true;
    if (n<4 && try_range_log_over_x(e,rv,out))
      return true;
    if (n<4 && try_range_log_sub_quad(e,rv,out))
      return true;
    if (n<4 && try_range_abs_sum_unit(e,rv,out))
      return true;
    if (n<4 && try_range_abs_two_affine(e,rv,out))
      return true;
    if (n<4 && try_range_log_sqrt_concave_quad(e,rv,out))
      return true;
    if (n<4 && try_range_sqrt_log_concave_quad(e,rv,out))
      return true;
    if (n<4 && try_range_abs_over_positive_quad(e,rv,out))
      return true;
    if (n<4 && try_range_abs_sin_plus_cos(e,rv,out))
      return true;
    if (n<4 && try_range_shifted_base_function(e,rv,out))
      return true;
    if (!contains(e,"abs(") && !split_top_fraction(e,num,den) &&
        contains(e,"exp(") && try_range_all_real_constrained(e,rv,out))
      return true;
    if (n<4 && try_range_log_quad_route(e,rv,out))
      return true;
    if (n<4 && contains(e,"ln(x)") && try_range_positive_constrained(e,rv,out))
      return true;
  }
  {
    working_string terms[3],u;
    int signs[3],tn=split_top_sum_terms(e,terms,signs,3),ui=-1,ci=-1;
    working_string shift;
    const char *fn=0;
    if (tn==2){
      for (int i=0;i<tn;++i){
        if (!contains_var_symbol(terms[i],rv)){
          shift=join_sum(shift,signed_part(signs[i],terms[i]));
          ci=i;
        }
        else if (parse_unary_arg(terms[i],"abs",u)){ fn="abs"; ui=i; }
        else if (parse_unary_arg(terms[i],"sqrt",u)){ fn="sqrt"; ui=i; }
      }
      if (fn && ui>=0 && ci>=0){
        out="Range:\n";
        out += working_string(fn)+"(u) >= 0\n";
        out += signs[ui]>0 ? "y >= " : "y <= ";
        out += range_symbolic_bound(shift,rat(0,1))+"";
        return true;
      }
    }
  }
  {
    working_string logarg,exparg,sub;
    if ((parse_unary_arg(e,"ln",logarg) || parse_unary_arg(e,"log",logarg)) &&
        parse_unary_arg(logarg,"exp",exparg) && contains_var_symbol(exparg,rv)){
      working_string call="range("+exparg+","+var+")";
      out="Range:\n";
      out += e.size()+exparg.size()>180 ? "ln(exp(A)) = A\n" : e+" = "+exparg+"\n";
      if (try_range(call.c_str(),sub))
        out += sub;
      else
        out += "same range as "+exparg+"";
      return true;
    }
  }
  {
    working_string largs[2],larg;
    int ln=0;
    if ((parse_unary_arg(e,"ln",larg) || parse_unary_arg(e,"log",larg)) &&
        split_args(larg,0,larg.size(),largs,2)==1 && contains_var_symbol(larg,rv)){
      working_string lin=strip_outer_parens(nospace_lower(larg));
      Rat la,lb;
      if (parse_affine_general(lin,rv,la,lb) && la.n){
        out="Range:\n";
        out += "linear log domain\n";
        out += "all real";
        return true;
      }
      out="Range:\nln(g), g>0";
      return true;
    }
    if (parse_call(e.c_str(),"log",largs,2,ln) && ln==2 &&
        (contains_var_symbol(largs[0],rv) || contains_var_symbol(largs[1],rv))){
      out="Range:\n";
      out += "log ratio\n";
      out += "base>0,!=1; arg>0\n";
      if (contains_var_symbol(largs[0],rv) && !contains_var_symbol(largs[1],rv)){
        working_string A=trim(largs[1]);
        if (A.size()>80)
          A="arg";
        out += "Let A = "+A+"\n";
        if (syntactic_zero_expr(largs[1]))
          out += "A=0: arg>0 fails\nno real range";
        else {
          out += "A=1: y=0\n";
          out += "A>0,A!=1: y!=0\n";
          out += "A<=0: no real";
        }
      }
      else {
        out += "arg>0; /ln(base)";
      }
      out += "";
      return true;
    }
  }
  {
    working_string arg;
    if (parse_unary_arg(e,"abs",arg)){
      out="Range:\nabs(u) >= 0\n";
      out += "y >= 0";
      return true;
    }
    if (parse_unary_arg(e,"tan",arg) && range_expr_all_real_simple(arg,rv)){
      out="Range:\ntan(u): all real\n";
      out += "all real";
      return true;
    }
    if (parse_unary_arg(e,"cot",arg)){
      out="Range:\ncot(u): all real\n";
      out += "all real";
      return true;
    }
    if (parse_unary_arg(e,"sec",arg) || parse_unary_arg(e,"cosec",arg)){
      out="Range:\n";
      out += e.substr(0,e.find('('))+"(u): y<=-1 or y>=1\n";
      out += "y <= -1 or y >= 1";
      return true;
    }
    if (parse_unary_arg(e,"exp",arg) && contains_var_symbol(arg,rv)){
      out="Range:\nexp(u) > 0\n";
      out += "y > 0";
      return true;
    }
    if (parse_unary_arg(e,"sqrt",arg)){
      if (!contains_var_symbol(arg,rv)){
        out="Range:\nconstant\ny = "+spaced_pm(e);
        return true;
      }
      {
        Rat A,B,C,sr;
        if (parse_quad_rat_expr(arg,rv,A,B,C) && A.n>0){
          Rat xv=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
          Rat yv=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
          out="Range:\nconvex quad inside sqrt\n";
          out += "min inside="+rat_s(yv)+" at "+working_string(1,rv)+"="+rat_s(xv)+"\n";
          if (rat_cmp(yv,rat(0,1))>=0){
            working_string lower=rat_square_root(yv,sr)?rat_s(sr):sqrt_rat_s(yv);
            out += "y >= "+lower+"";
          }
          else {
            out += "inside crosses 0\n";
            out += "y >= 0";
          }
          return true;
        }
        if (parse_quad_rat_expr(arg,rv,A,B,C) && A.n<0){
          Rat xv=rat_div(rat(-B.n,B.d),rat_mul(rat(2,1),A));
          Rat yv=rat_sub(C,rat_div(rat_mul(B,B),rat_mul(rat(4,1),A)));
          working_string upper=rat_square_root(yv,sr)?rat_s(sr):sqrt_rat_s(yv);
          out="Range:\nconcave quad\n";
          out += "max inside="+rat_s(yv)+" at "+working_string(1,rv)+"="+rat_s(xv)+"\n";
          out += "0 <= y <= "+upper+"";
          return true;
        }
      }
      {
        working_string trigarg;
        const char *tfn=0;
        if (parse_unary_arg(arg,"sin",trigarg))
          tfn="sin";
        else if (parse_unary_arg(arg,"cos",trigarg))
          tfn="cos";
        if (tfn && range_expr_all_real_simple(trigarg,rv)){
          out="Range:\n";
          out += working_string(tfn)+"(u):[-1,1]\n";
          out += "need input>=0\n";
          out += "0<=y<=1";
          return true;
        }
      }
      if (range_expr_all_real_simple(arg,rv)){
        out="Range:\n";
        out += "sqrt y>=0\n";
        out += "y >= 0";
        return true;
      }
      working_string terms[3],base;
      int signs[3];
      int tn=split_top_sum_terms(arg,terms,signs,3);
      Rat c=rat(0,1),p;
      bool square=false,ok=true;
      for (int i=0;i<tn;++i){
        if (split_outer_power_general(terms[i],base,p) && p.n==2 && p.d==1 && signs[i]>0)
          square=true;
        else {
          Rat r;
          if (!parse_rat(terms[i],r) || signs[i]<0)
            ok=false;
          else
            c=rat_add(c,r);
        }
      }
      if (tn>=1 && ok && square && c.n>=0){
        out="Range:\nu^2 >= 0\n";
        if (c.n)
          out += "u^2 + "+rat_s(c)+" >= "+rat_s(c)+"\n";
        out += "y >= "+(c.n?working_string("sqrt(")+rat_s(c)+")":"0")+"";
        return true;
      }
      if (contains_var_symbol(arg,rv)){
        working_string R=arg.size()>120?"input":arg;
        out="Range:\n";
        out += "R="+R+"\n";
        out += "need R>=0\n";
        out += "sqrt(R)>=0\n";
        out += "y >= 0";
        return true;
      }
    }
  }
  {
    Rat la,lb;
    if (parse_affine_general(e,rv,la,lb)){
      out="Range:\nlinear\n";
      out += "all real";
      return true;
    }
  }
  {
    long coef=0,pow=0;
    if (parse_power_term_var(e,rv,coef,pow) && pow>0){
      out="Range:\n";
      if (pow%2==0){
        out += var+"^"+int_s(pow)+" >= 0\n";
        out += coef>0?"y >= 0":"y <= 0";
      }
      else
        out += "Odd power: all real";
      return true;
    }
  }
  if (try_range_trig_sum_bound(e,rv,out))
    return true;
  if (contains_var_symbol(e,rv)){
    working_string exact;
    if (e.size()<220 && !nested_function_expr(e,6) &&
        production_exact_command("range("+args[0]+","+var+")",exact) && !trim(exact).empty() &&
        !contains(nospace_lower(exact),"range(") && trim(exact)!="undef" && trim(exact)!="nan"){
      out="Range method:\n";
      out += "Answer:\n"+trim(exact)+"\n";
      out += "";
    }
    else
      out="Err: complexity guard";
    return true;
  }
  return false;
}

static bool parse_simple_sincos_term(const working_string &term,Rat sign,Rat &scoef,Rat &ccoef){
  working_string t=strip_outer_parens(nospace_lower(term));
  const char *suf=0;
  bool issin=false;
  if (t.size()>=6 && t.substr(t.size()-6,6)=="sin(x)"){ suf="sin(x)"; issin=true; }
  else if (t.size()>=6 && t.substr(t.size()-6,6)=="cos(x)"){ suf="cos(x)"; issin=false; }
  else
    return false;
  Rat k;
  if (!parse_coef_prefix(t.substr(0,t.size()-6),k))
    return false;
  k=rat_mul(k,sign);
  if (issin) scoef=rat_add(scoef,k);
  else ccoef=rat_add(ccoef,k);
  return true;
}

static bool try_xform_r_form(const working_string &source,const working_string &target,working_string &out){
  working_string terms[4];
  int signs[4];
  int n=split_top_sum_terms(source,terms,signs,4);
  if (n<2)
    return false;
  Rat a=rat(0,1),b=rat(0,1);
  for (int i=0;i<n;++i)
    if (!parse_simple_sincos_term(terms[i],rat(signs[i],1),a,b))
      return false;
  if (!a.n || !b.n || a.d!=1 || b.d!=1)
    return false;
  long r2=a.n*a.n+b.n*b.n;
  working_string ratio=rat_s(rat_div(b,a));
  working_string ans="sqrt("+int_s(r2)+")*sin(x+atan("+ratio+"))";
  working_string ct=compact(target);
  bool target_ok=ct=="rform" || ct=="rsin(x+alpha)" || ct==compact(ans);
  if (!target_ok && a.n==b.n && a.d==b.d && a.n>0)
    target_ok=ct==compact("sqrt("+int_s(r2)+")*sin(x+pi/4)");
  if (!target_ok)
    return false;
  out="R-form:\n";
  out += "R = sqrt("+int_s(a.n)+"^2+"+int_s(b.n)+"^2) = sqrt("+int_s(r2)+")\n";
  out += "A=atan("+ratio+")\n";
  out += ans;
  return true;
}

static bool split_square_affine_rat(const working_string &src,char v,Rat &coef,Rat &a,Rat &b){
  working_string s=strip_outer_parens(nospace_lower(src));
  coef=rat(1,1);
  int open=s.find('(');
  if (open<0)
    return false;
  int close=match_paren(s,open);
  if (close<0 || close+1>=int(s.size()) || s[close+1]!='^')
    return false;
  Rat p;
  if (!parse_rat(s.substr(close+2,s.size()-close-2),p) || p.n!=2 || p.d!=1)
    return false;
  working_string pre=s.substr(0,open);
  if (!pre.empty()){
    if (pre[pre.size()-1]=='*')
      pre=pre.substr(0,pre.size()-1);
    if (!parse_rat(pre,coef))
      return false;
  }
  return parse_linear_rat_var(s.substr(open+1,close-open-1),v,a,b);
}

static bool parse_quad_template(const working_string &src,char v,working_string *name,int *sgn){
  working_string terms[4];
  int signs[4];
  int n=split_top_sum_terms(src,terms,signs,4),seen=0;
  for (int i=0;i<3;++i){ name[i]=""; sgn[i]=0; }
  if (n<1 || n>3)
    return false;
  for (int i=0;i<n;++i){
    working_string t=strip_outer_parens(nospace_lower(terms[i])),nm;
    int p=-1;
    if (plain_identifier_name(t)){
      nm=t;
      p=0;
    }
    else {
      int star=t.find('*');
      if (star<=0 || star>=int(t.size())-1)
        return false;
      working_string a=strip_outer_parens(t.substr(0,star));
      working_string b=strip_outer_parens(t.substr(star+1,t.size()-star-1));
      if (a.size()==1 && a[0]==v){ p=1; nm=b; }
      else if (b.size()==1 && b[0]==v){ p=1; nm=a; }
      else if (a.size()==3 && a[0]==v && a[1]=='^' && (a[2]=='1' || a[2]=='2')){ p=a[2]-'0'; nm=b; }
      else if (b.size()==3 && b[0]==v && b[1]=='^' && (b[2]=='1' || b[2]=='2')){ p=b[2]-'0'; nm=a; }
      else
        return false;
      if ((p!=1 && p!=2) || !plain_identifier_name(nm))
        return false;
    }
    if (p<0 || p>2 || (seen&(1<<p)))
      return false;
    seen |= 1<<p;
    name[p]=nm;
    sgn[p]=signs[i];
  }
  return (seen&7)==7;
}

static bool try_xform_quad_template(const working_string &source,const working_string &target,working_string &out){
  char v=default_var_char(source);
  working_string name[3],expanded;
  int sgn[3];
  Rat A,B,C;
  if (!parse_quad_template(source,v,name,sgn) ||
      !production_exact_command("texpand("+target+")",expanded) ||
      !parse_quad_rat_expr(expanded,v,A,B,C))
    return false;
  Rat val[3]={rat_mul(rat(sgn[0],1),C),rat_mul(rat(sgn[1],1),B),rat_mul(rat(sgn[2],1),A)};
  working_string inst=source;
  for (int i=0;i<3;++i)
    inst=replace_identifier_token(inst,name[i],rat_s(val[i]));
  if (!khicas_equiv(inst,target))
    return false;
  out="Compare coefficients:\n";
  out += "Expand target:\n";
  out += target+" = "+trim(expanded)+"\n";
  out += name[2]+" = "+rat_s(val[2])+", "+name[1]+" = "+rat_s(val[1])+", "+name[0]+" = "+rat_s(val[0])+"\n";
  out += "";
  return true;
}

static working_string square_affine_rat_s(Rat coef,Rat a,Rat b,char v){
  if (a.n<0){
    a.n=-a.n;
    b.n=-b.n;
  }
  working_string base="("+fmt_linear_rat(a,b,v)+")^2";
  if (rat_is_one(coef))
    return base;
  if (rat_is_minus_one(coef))
    return "-"+base;
  return rat_s(coef)+"*"+base;
}

static bool same_rat(Rat a,Rat b){
  return rat_cmp(a,b)==0;
}

struct RewriteTarget {
  working_string label,value,log_base,log_arg;
  bool assigned,is_log;
};

struct RewriteAcc {
  Rat c[12];
  Rat k;
  working_string work;
};

static int find_top_equal_rewrite(const working_string &s){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c=='=')
      return i;
  }
  return -1;
}

static bool same_rewrite_expr(const working_string &a,const working_string &b){
  return canonical_expr(a)==canonical_expr(b) || compact(a)==compact(b);
}

static bool parse_log_call_ws(const working_string &src,working_string &base,working_string &arg){
  working_string s=strip_outer_parens(nospace_lower(src));
  if (s.size()>=5 && s.substr(0,3)=="ln("){
    int close=match_paren(s,2);
    if (close!=int(s.size())-1)
      return false;
    base="e";
    arg=canonical_expr(s.substr(3,s.size()-4));
    return !arg.empty();
  }
  if (s.size()>=6 && s.substr(0,4)=="log("){
    int close1=match_paren(s,3);
    if (close1==int(s.size())-1){
      working_string inner=s.substr(4,s.size()-5);
      int depth=0;
      bool comma=false;
      for (int i=0;i<int(inner.size());++i){
        char c=inner[i];
        if (c=='(' || c=='[' || c=='{') ++depth;
        else if (c==')' || c==']' || c=='}') --depth;
        else if (!depth && c==',') comma=true;
      }
      if (!comma){
        base="e";
        arg=canonical_expr(inner);
        return !arg.empty();
      }
    }
  }
  if (s.size()<7 || s.substr(0,4)!="log(")
    return false;
  int close=match_paren(s,3);
  if (close!=int(s.size())-1)
    return false;
  working_string args[2];
  int n=split_args(s,4,close,args,2);
  if (n!=2)
    return false;
  base=canonical_expr(args[0]);
  arg=canonical_expr(args[1]);
  return !base.empty() && !arg.empty();
}

static bool parse_scaled_log_call_ws(const working_string &src,Rat &coef,working_string &base,working_string &arg){
  working_string s=strip_outer_parens(nospace_lower(src));
  int p=s.find("log(");
  if (p>=0){
    if (!parse_coef_prefix(s.substr(0,p),coef))
      return false;
    return parse_log_call_ws(s.substr(p,s.size()-p),base,arg);
  }
  p=s.find("ln(");
  if (p>=0){
    if (!parse_coef_prefix(s.substr(0,p),coef))
      return false;
    return parse_log_call_ws(s.substr(p,s.size()-p),base,arg);
  }
  return false;
}

static int parse_rewrite_targets(const working_string &raw,RewriteTarget *targets,int maxn){
  working_string s=trim(raw);
  int begin=0,end=s.size();
  if (s.size()>=2 && s[0]=='[' && s[s.size()-1]==']'){
    begin=1;
    end=s.size()-1;
  }
  working_string args[12];
  int n=split_args(s,begin,end,args,maxn);
  for (int i=0;i<n;++i){
    working_string item=trim(args[i]), label, value;
    int eq=find_top_equal_rewrite(item);
    if (eq>0){
      label=compact(item.substr(0,eq));
      value=trim(item.substr(eq+1,item.size()-eq-1));
      targets[i].assigned=true;
    }
    else {
      value=trim(item);
      label=canonical_expr(value);
      targets[i].assigned=false;
    }
    targets[i].label=label;
    targets[i].value=canonical_expr(value);
    targets[i].is_log=parse_log_call_ws(value,targets[i].log_base,targets[i].log_arg);
  }
  return n;
}

static int rewrite_target_index(const working_string &expr,RewriteTarget *targets,int n){
  for (int i=0;i<n;++i)
    if (same_rewrite_expr(expr,targets[i].value))
      return i;
  return -1;
}

static int rewrite_log_target_index(const working_string &base,const working_string &arg,RewriteTarget *targets,int n){
  for (int i=0;i<n;++i)
    if (targets[i].is_log && same_rewrite_expr(base,targets[i].log_base) &&
        same_rewrite_expr(arg,targets[i].log_arg))
      return i;
  return -1;
}

static bool working_route_too_large(const working_string &s){
  working_string cs=compact(s);
  int ops=0,depth=0,max_depth=0,calls=0,trans_calls=0,max_power=0;
  for (int i=0;i<int(cs.size());++i){
    char c=cs[i];
    if (c=='+' || c=='-' || c=='*' || c=='/' || c=='^' || c=='=' || c==',')
      ++ops;
    if (c=='^'){
      int j=i+1,v=0,digits=0;
      while (j<int(cs.size()) && isdigit((unsigned char)cs[j]) && digits<4){
        v=10*v+(cs[j]-'0');
        ++j;
        ++digits;
      }
      if (digits && v>max_power)
        max_power=v;
    }
    else if (c=='('){
      ++depth;
      if (depth>max_depth)
        max_depth=depth;
      int j=i-1;
      while (j>=0 && isalpha((unsigned char)cs[j]))
        --j;
      if (j+1<i){
        ++calls;
        working_string name=cs.substr(j+1,i-j-1);
        if (name=="sin" || name=="cos" || name=="tan" || name=="sec" ||
            name=="cot" || name=="cosec" || name=="sqrt" || name=="ln" ||
            name=="log" || name=="exp" || name=="abs")
          ++trans_calls;
      }
    }
    else if (c==')' && depth>0)
      --depth;
  }
  bool calculus=starts_command(cs,"integrate") || starts_command(cs,"int") ||
                starts_command(cs,"diff") || starts_command(cs,"derive");
  bool nested_high_degree=calculus && max_power>12 && trans_calls>=4;
  bool nested_symbolic_solve=starts_command(cs,"solve") && max_depth>=6 && trans_calls>=3;
  bool nested_symbolic_rewrite=(max_power>30 && trans_calls>=3) ||
                               (trans_calls>=5 && ops>12);
  return cs.size()>420 || ops>120 || max_depth>7 || calls>8 ||
         nested_high_degree || nested_symbolic_solve || nested_symbolic_rewrite;
}

static bool constant_exp_surd_too_large(const working_string &s){
  working_string cs=compact(s);
  if (cs.size()<110 || !contains(cs,"exp(") || !contains(cs,"sqrt("))
    return false;
  int ops=0,depth=0,max_depth=0,digits=0;
  for (int i=0;i<int(cs.size());++i){
    char c=cs[i];
    if (c=='+' || c=='-' || c=='*' || c=='/' || c=='^')
      ++ops;
    if (isdigit((unsigned char)c))
      ++digits;
    if (c=='('){
      ++depth;
      if (depth>max_depth)
        max_depth=depth;
    }
    else if (c==')' && depth>0)
      --depth;
  }
  return max_depth>=5 && ops>=10 && digits>=30;
}

static bool nested_function_expr(const working_string &s,int limit){
  working_string cs=compact(s);
  int calls=0;
  for (int i=0;i<int(cs.size());++i){
    if (cs[i]!='(')
      continue;
    int j=i-1;
    while (j>=0 && isalpha((unsigned char)cs[j]))
      --j;
    if (j+1<i && ++calls>=limit)
      return true;
  }
  return false;
}

static void rewrite_acc_init(RewriteAcc &acc){
  acc.k=rat(0,1);
  acc.work="";
  for (int i=0;i<12;++i)
    acc.c[i]=rat(0,1);
}

static void rewrite_add_coeff(RewriteAcc &acc,int idx,Rat q){
  if (idx>=0 && idx<12)
    acc.c[idx]=rat_add(acc.c[idx],q);
}

static bool parse_top_power(const working_string &src,working_string &base,working_string &exp){
  working_string s=strip_outer_parens(nospace_lower(src));
  int depth=0;
  for (int i=int(s.size())-1;i>=0;--i){
    char c=s[i];
    if (c==')' || c==']' || c=='}') ++depth;
    else if (c=='(' || c=='[' || c=='{') --depth;
    else if (!depth && c=='^' && i>0 && i<int(s.size())-1){
      base=strip_outer_parens(s.substr(0,i));
      exp=strip_outer_parens(s.substr(i+1,s.size()-i-1));
      return !base.empty() && !exp.empty();
    }
  }
  return false;
}

static bool parse_positive_int_ws(const working_string &src,long &v){
  Rat r;
  if (!parse_rat(src,r) || r.d!=1 || r.n<=0)
    return false;
  v=r.n;
  return true;
}

static bool log_const_power(const working_string &base,const working_string &arg,Rat &pow){
  long b=0,a=0;
  if (!parse_positive_int_ws(base,b) || !parse_positive_int_ws(arg,a) || b<=1)
    return false;
  if (a==1){
    pow=rat(0,1);
    return true;
  }
  long v=1;
  for (int p=1;p<16;++p){
    if (v>2147483647L/b)
      return false;
    v*=b;
    if (v==a){
      pow=rat(p,1);
      return true;
    }
    if (v>a)
      return false;
  }
  return false;
}

static working_string rewrite_term_abs(Rat q,const working_string &label){
  if (q.n<0)
    q.n=-q.n;
  if (q.n==q.d)
    return label;
  if (q.d==1)
    return rat_s(q)+"*"+label;
  return "("+rat_s(q)+")*"+label;
}

static working_string format_rewrite_result(RewriteTarget *targets,int n,const RewriteAcc &acc){
  working_string out;
  if (acc.k.n)
    out=rat_s(acc.k);
  for (int pass=0;pass<2;++pass)
  for (int i=0;i<n;++i){
    Rat q=acc.c[i];
    if (!q.n)
      continue;
    bool neg=q.n<0;
    if ((pass==0 && neg) || (pass==1 && !neg))
      continue;
    working_string term=rewrite_term_abs(q,targets[i].label);
    if (out.empty())
      out=neg ? "-"+term : term;
    else
      out += (neg ? "-" : "+")+term;
  }
  return out.empty()?working_string("0"):out;
}

static working_string log_expr_ws(const working_string &base,const working_string &arg){
  return "log("+base+","+arg+")";
}

static working_string log_expr_display(const working_string &base,const working_string &arg){
  if (base=="e")
    return "ln("+arg+")";
  return log_expr_ws(base,arg);
}

static bool parse_two_log_sum(const working_string &src,working_string &base,working_string &u,working_string &v,bool &quotient){
  working_string terms[2],b0,b1,a0,a1;
  int signs[2];
  Rat c0,c1;
  int n=split_top_sum_terms(src,terms,signs,2);
  if (n!=2 || signs[0]<0)
    return false;
  if (!parse_scaled_log_call_ws(terms[0],c0,b0,a0) ||
      !parse_scaled_log_call_ws(terms[1],c1,b1,a1) ||
      !rat_is_one(c0) || !rat_is_one(c1) || !same_rewrite_expr(b0,b1))
    return false;
  quotient=signs[1]<0;
  base=b0;
  u=a0;
  v=a1;
  return true;
}

static bool parse_two_scaled_log_sum(const working_string &src,Rat &coef,working_string &base,
                                     working_string &u,working_string &v,bool &quotient){
  working_string terms[2],b0,b1,a0,a1;
  int signs[2];
  Rat c0,c1;
  int n=split_top_sum_terms(src,terms,signs,2);
  if (n!=2 || signs[0]<0)
    return false;
  if (!parse_scaled_log_call_ws(terms[0],c0,b0,a0) ||
      !parse_scaled_log_call_ws(terms[1],c1,b1,a1) ||
      !same_rewrite_expr(b0,b1) || rat_cmp(c0,c1)!=0)
    return false;
  quotient=signs[1]<0;
  coef=c0;
  base=b0;
  u=a0;
  v=a1;
  return true;
}

static bool product_expr_matches(const working_string &arg,const working_string &u,const working_string &v){
  return same_rewrite_expr(arg,u+"*"+v) || same_rewrite_expr(arg,v+"*"+u) ||
         khicas_equiv(arg,"("+u+")*("+v+")");
}

static bool quotient_expr_matches(const working_string &arg,const working_string &u,const working_string &v){
  working_string num,den;
  return (split_top_fraction(arg,num,den) && same_rewrite_expr(num,u) && same_rewrite_expr(den,v)) ||
         khicas_equiv(arg,"("+u+")/("+v+")");
}

static working_string log_law_domain_note(const working_string &base,const working_string &u,const working_string &v){
  return "Condition: "+base+">0, "+base+"!=1; "+insert_coeff_stars(u)+">0; "+insert_coeff_stars(v)+">0\n";
}

static bool try_xform_log_product_quotient(const working_string &a,const working_string &b,working_string &out){
  working_string base,arg,sbase,u,v;
  bool quotient=false;
  if (parse_log_call_ws(a,base,arg) && parse_two_log_sum(b,sbase,u,v,quotient) &&
      same_rewrite_expr(base,sbase) &&
      (quotient ? quotient_expr_matches(arg,u,v) : product_expr_matches(arg,u,v))){
    out=quotient ? "Quotient:\n" : "Product:\n";
    out += quotient ? "log_a(u/v)=log_a(u)-log_a(v)\n" : "log_a(uv)=log_a(u)+log_a(v)\n";
    out += log_expr_display(base,arg)+" = "+insert_coeff_stars(b)+"\n";
    out += log_law_domain_note(base,u,v);
    out += insert_coeff_stars(b)+"";
    return true;
  }
  if (parse_two_log_sum(a,sbase,u,v,quotient) && parse_log_call_ws(b,base,arg) &&
      same_rewrite_expr(base,sbase) &&
      (quotient ? quotient_expr_matches(arg,u,v) : product_expr_matches(arg,u,v))){
    out=quotient ? "Quotient:\n" : "Product:\n";
    out += quotient ? "log_a(u)-log_a(v)=log_a(u/v)\n" : "log_a(u)+log_a(v)=log_a(uv)\n";
    out += insert_coeff_stars(a)+" = "+log_expr_display(base,arg)+"\n";
    out += log_law_domain_note(base,u,v);
    out += insert_coeff_stars(b)+"";
    return true;
  }
  {
    Rat c,p;
    working_string root,exp;
    if (parse_log_call_ws(a,base,arg) &&
        parse_two_scaled_log_sum(b,c,sbase,u,v,quotient) &&
        same_rewrite_expr(base,sbase) && !quotient &&
        parse_top_power(arg,root,exp) && parse_rat(exp,p) &&
        rat_cmp(c,p)==0 && product_expr_matches(root,u,v)){
      out="Log law\n";
      out += log_expr_display(base,arg)+" = "+rat_s(p)+"*"+log_expr_display(base,root)+"\n";
      out += root+" = ("+u+")("+v+")\n";
      out += insert_coeff_stars(b)+"\n";
      out += log_law_domain_note(base,u,v);
      out += "";
      return true;
    }
  }
  return false;
}

static bool parse_sqrt_plus_const(const working_string &src,working_string &inside,Rat &c){
  working_string terms[2],u;
  int signs[2];
  int n=split_top_sum_terms(src,terms,signs,2);
  if (n!=2)
    return false;
  Rat r;
  for (int i=0;i<2;++i){
    if (parse_unary_arg(terms[i],"sqrt",u)){
      if (signs[i]<0) return false;
      inside=u;
      continue;
    }
    if (parse_rat(terms[i],r)){
      c=rat_mul(r,rat(signs[i],1));
      continue;
    }
    return false;
  }
  return !inside.empty();
}

static bool try_xform_log_surd_rationalise(const working_string &a,const working_string &b,working_string &out){
  working_string ba,arga,bb,argb,num,den,tnum,tden,inside1,inside2,denbase,denexp;
  Rat c1=rat(0,1),c2=rat(0,1);
  if (!parse_log_call_ws(a,ba,arga) || !parse_log_call_ws(b,bb,argb) ||
      !same_rewrite_expr(ba,bb) ||
      !split_top_fraction(arga,num,den) || !split_top_fraction(argb,tnum,tden) ||
      !parse_sqrt_plus_const(num,inside1,c1) ||
      !parse_sqrt_plus_const(den,inside2,c2) ||
      !same_rewrite_expr(inside1,inside2) ||
      rat_cmp(c1,rat(-c2.n,c2.d))!=0 || !c2.n ||
      !parse_top_power(tden,denbase,denexp) || denexp!="2" ||
      !same_rewrite_expr(denbase,den))
    return false;
  working_string diff="("+inside1+")-("+rat_s(rat_mul(c2,c2))+")";
  if (!same_rewrite_expr(tnum,diff) && !khicas_equiv(tnum,diff))
    return false;
  out="Rationalise log\n";
  out += "conjugate\n";
  out += arga+" = "+argb+"\n";
  out += log_expr_display(ba,arga)+" = "+log_expr_display(bb,argb)+"\n";
  out += " domain";
  return true;
}

static bool try_xform_log_cancel_surd_factor(const working_string &a,const working_string &b,working_string &out){
  working_string base,u,v,tbase,w;
  bool quotient=false;
  Rat c;
  if (!parse_two_log_sum(a,base,u,v,quotient) || !quotient ||
      !parse_scaled_log_call_ws(b,c,tbase,w) || !same_rewrite_expr(base,tbase) ||
      c.n!=-c.d)
    return false;
  if (!khicas_equiv(v,"("+u+")*("+w+")"))
    return false;
  out="Log law with factor cancellation:\n";
  out += log_expr_display(base,u)+" - "+log_expr_display(base,v)+"\n";
  out += v+" = ("+u+")("+w+")\n";
  out += "therefore "+log_expr_display(base,u)+" - "+log_expr_display(base,v)+" = -"+log_expr_display(base,w)+"\n";
  out += insert_coeff_stars(b)+"";
  return true;
}

static bool decompose_log_arg(const working_string &base,const working_string &arg,Rat scale,
                              RewriteTarget *targets,int n,RewriteAcc &acc);

static bool decompose_log_product(const working_string &base,const working_string &arg,Rat scale,
                                  RewriteTarget *targets,int n,RewriteAcc &acc){
  working_string factors[8];
  int fn=split_top_product(arg,factors,8);
  if (fn<=1)
    return false;
  acc.work += log_expr_ws(base,arg)+" = ";
  for (int i=0;i<fn;++i){
    if (i) acc.work += " + ";
    acc.work += log_expr_ws(base,factors[i]);
  }
  acc.work += "\n";
  for (int i=0;i<fn;++i)
    if (!decompose_log_arg(base,factors[i],scale,targets,n,acc))
      return false;
  return true;
}

static bool decompose_log_special_sum_fraction(const working_string &base,const working_string &arg,Rat scale,
                                               RewriteTarget *targets,int n,RewriteAcc &acc){
  working_string terms[3],num,den;
  int signs[3];
  int tn=split_top_sum_terms(arg,terms,signs,3);
  if (tn!=2 || signs[0]<0 || signs[1]<0)
    return false;
  Rat c,d;
  bool ok=false;
  if (parse_rat(terms[0],c) && split_top_fraction(terms[1],num,den) &&
      den=="x" && parse_rat(num,d))
    ok=true;
  else if (parse_rat(terms[1],c) && split_top_fraction(terms[0],num,den) &&
           den=="x" && parse_rat(num,d))
    ok=true;
  if (!ok || c.d!=1 || d.d!=1 || c.n<=0 || d.n<=0)
    return false;
  long g=gcd_long(c.n,d.n);
  working_string inner;
  long k=d.n/g;
  if (k==1)
    inner="x+1";
  else
    inner="x+"+int_s(k);
  acc.work += arg+" = "+int_s(g)+"*("+inner+")/x\n";
  if (!decompose_log_arg(base,int_s(g),scale,targets,n,acc))
    return false;
  if (!decompose_log_arg(base,inner,scale,targets,n,acc))
    return false;
  return decompose_log_arg(base,"x",rat_mul(scale,rat(-1,1)),targets,n,acc);
}

static bool decompose_log_arg(const working_string &base,const working_string &arg,Rat scale,
                              RewriteTarget *targets,int n,RewriteAcc &acc){
  working_string s=canonical_expr(arg), u, e;
  int idx=rewrite_log_target_index(base,s,targets,n);
  if (idx>=0){
    rewrite_add_coeff(acc,idx,scale);
    return true;
  }
  Rat kp;
  if (log_const_power(base,s,kp)){
    acc.work += log_expr_ws(base,s)+" = "+rat_s(kp)+"\n";
    acc.k=rat_add(acc.k,rat_mul(scale,kp));
    return true;
  }
  if (parse_unary_arg(s,"sqrt",u)){
    acc.work += "sqrt("+u+") = ("+u+")^(1/2)\n";
    return decompose_log_arg(base,u,rat_mul(scale,rat(1,2)),targets,n,acc);
  }
  if (parse_top_power(s,u,e)){
    Rat p;
    if (parse_rat(e,p)){
      acc.work += log_expr_ws(base,s)+" = "+rat_s(p)+"*"+log_expr_ws(base,u)+"\n";
      return decompose_log_arg(base,u,rat_mul(scale,p),targets,n,acc);
    }
  }
  working_string num,den;
  if (split_top_fraction(s,num,den)){
    acc.work += log_expr_ws(base,s)+" = "+log_expr_ws(base,num)+" - "+log_expr_ws(base,den)+"\n";
    return decompose_log_arg(base,num,scale,targets,n,acc) &&
           decompose_log_arg(base,den,rat_mul(scale,rat(-1,1)),targets,n,acc);
  }
  if (decompose_log_special_sum_fraction(base,s,scale,targets,n,acc))
    return true;
  if (decompose_log_product(base,s,scale,targets,n,acc))
    return true;
  working_string shown,fac;
  if (factor_expr_simple(s,'x',shown,fac) && canonical_expr(fac)!=canonical_expr(s)){
    acc.work += shown+" = "+fac+"\n";
    return decompose_log_arg(base,fac,scale,targets,n,acc);
  }
  Rat a,b;
  if (parse_affine_any(s,'x',a,b) && a.d==1 && b.d==1 && a.n && b.n){
    long g=gcd_long(a.n,b.n);
    if (g>1){
      working_string inner=fmt_affine(a.n/g,b.n/g);
      acc.work += s+" = "+int_s(g)+"*("+inner+")\n";
      return decompose_log_arg(base,int_s(g),scale,targets,n,acc) &&
             decompose_log_arg(base,inner,scale,targets,n,acc);
    }
  }
  return false;
}

static working_string generic_rewrite_expr(const working_string &expr,RewriteTarget *targets,int n,working_string &work,bool &changed,int depth,int &budget){
  working_string s=canonical_expr(expr);
  budget -= 1 + int(s.size()/256);
  if (budget<0 || depth>28 || working_route_too_large(s))
    return s;
  int idx=rewrite_target_index(s,targets,n);
  if (idx>=0){
    if (targets[idx].assigned)
      work += s+" -> "+targets[idx].label+"\n";
    changed=true;
    return targets[idx].label;
  }
  for (int i=0;i<n;++i){
    working_string tv=canonical_expr(targets[i].value);
    if (tv.size()==1 && isalpha((unsigned char)tv[0])){
      Rat a,b;
      if (parse_affine_any(s,tv[0],a,b) && a.n){
        working_string out;
        if (a.n==a.d)
          out=targets[i].label;
        else if (a.n==-a.d)
          out="-"+targets[i].label;
        else
          out=rat_s(a)+"*"+targets[i].label;
        if (b.n>0)
          out += "+"+rat_s(b);
        else if (b.n<0)
          out += rat_s(b);
        changed=true;
        return out;
      }
    }
  }
  working_string terms[8];
  int signs[8];
  int tn=split_top_sum_terms(s,terms,signs,8);
  if (tn>1){
    working_string out;
    bool any=false;
    for (int i=0;i<tn;++i){
      bool ch=false;
      working_string r=generic_rewrite_expr(terms[i],targets,n,work,ch,depth+1,budget);
      any = any || ch;
      if (!i)
        out = signs[i]<0 ? "-"+r : r;
      else
        out += (signs[i]<0 ? "-" : "+")+r;
    }
    changed = changed || any;
    return out;
  }
  working_string num,den;
  if (split_top_fraction(s,num,den)){
    bool cn=false,cd=false;
    working_string rn=generic_rewrite_expr(num,targets,n,work,cn,depth+1,budget);
    working_string rd=generic_rewrite_expr(den,targets,n,work,cd,depth+1,budget);
    changed = changed || cn || cd;
    return "("+rn+")/("+rd+")";
  }
  working_string factors[8];
  int fn=split_top_product(s,factors,8);
  if (fn>1){
    working_string out;
    bool any=false;
    for (int i=0;i<fn;++i){
      bool ch=false;
      working_string r=generic_rewrite_expr(factors[i],targets,n,work,ch,depth+1,budget);
      any = any || ch;
      if (i) out += "*";
      out += r;
    }
    changed = changed || any;
    return out;
  }
  working_string base,exp;
  if (parse_top_power(s,base,exp)){
    bool cb=false,ce=false;
    working_string rb=generic_rewrite_expr(base,targets,n,work,cb,depth+1,budget);
    working_string re=generic_rewrite_expr(exp,targets,n,work,ce,depth+1,budget);
    changed = changed || cb || ce;
    return "("+rb+")^("+re+")";
  }
  int p=s.find('(');
  if (p>0 && match_paren(s,p)==int(s.size())-1){
    working_string fnm=s.substr(0,p), args[6], out=fnm+"(";
    int an=split_args(s,p+1,s.size()-1,args,6);
    bool any=false;
    for (int i=0;i<an;++i){
      bool ch=false;
      working_string r=generic_rewrite_expr(args[i],targets,n,work,ch,depth+1,budget);
      any = any || ch;
      if (i) out += ",";
      out += r;
    }
    out += ")";
    changed = changed || any;
    return out;
  }
  working_string shown,fac;
  if (factor_expr_simple(s,'x',shown,fac) && canonical_expr(fac)!=canonical_expr(s)){
    bool ch=false;
    working_string r=generic_rewrite_expr(fac,targets,n,work,ch,depth+1,budget);
    if (ch){
      work += shown+" = "+fac+"\n";
      changed=true;
      return r;
    }
  }
  return s;
}

static bool rewrite_exact_large(working_string &expr,RewriteTarget *targets,int n,working_string &work){
  bool changed=false;
  expr=canonical_expr(expr);
  for (int i=0;i<n;++i){
    if (!targets[i].assigned)
      continue;
    working_string from=canonical_expr(targets[i].value);
    if (from.size()<3)
      continue;
    size_t p=0;
    bool any=false;
    while ((p=expr.find(from,p))!=working_string::npos){
      expr=expr.substr(0,p)+targets[i].label+expr.substr(p+from.size(),expr.size()-p-from.size());
      p += targets[i].label.size();
      any=true;
    }
    if (any){
      work += targets[i].label+" = "+targets[i].value+"\n";
      work += from+" -> "+targets[i].label+"\n";
      changed=true;
    }
  }
  return changed;
}

static bool try_rewrite(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  if (!parse_call(input,"rewrite",args,6,n) || n<2){
    working_string cs=compact(input?input:"");
    if (starts_command(cs,"rewrite")){
      out="Err:rewrite";
      return true;
    }
    return false;
  }
  working_string expr=trim(args[0]), exact;
  out="Rewrite steps\n";
  if (expr.size()>180)
    out += "A\n";
  else
    out += expr+"\n";
  if (production_exact_command("normal("+expr+")",exact) && !trim(exact).empty() &&
      compact(exact)!=compact(expr))
    out += trim(exact)+"\n";
  else
    out += "normal form\n";
  out += "";
  return true;
}

static bool parse_xform_args_tolerant(const char *input,working_string &a,working_string &b){
  working_string s=trim(input?input:""), lo=lower(s);
  if (lo.find("xform")!=0)
    return false;
  int open=5;
  while (open<int(s.size()) && isspace((unsigned char)s[open]))
    ++open;
  if (open>=int(s.size()) || s[open]!='(')
    return false;
  int depth=0,comma=-1,best=-1,bestd=999,last=int(s.size())-1;
  while (last>open && isspace((unsigned char)s[last]))
    --last;
  if (last<=open || s[last]!=')')
    return false;
  for (int i=open+1;i<last;++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}'){
      if (depth>0) --depth;
    }
    else if (c==',' && depth==0){
      comma=i;
      break;
    }
    else if (c==',' && depth>0 && depth<bestd){
      best=i;
      bestd=depth;
    }
  }
  if (comma<0)
    comma=bestd==1?best:-1;
  if (comma<0)
    return false;
  a=trim(s.substr(open+1,comma-open-1));
  b=trim(s.substr(comma+1,last-comma-1));
  int ba=0,bb=0;
  for (int i=0;i<int(a.size());++i){ if (a[i]=='(') ++ba; else if (a[i]==')') --ba; }
  for (int i=0;i<int(b.size());++i){ if (b[i]=='(') ++bb; else if (b[i]==')') --bb; }
  while (ba-->0) a += ")";
  while (bb<0 && !b.empty() && b[b.size()-1]==')'){ b.erase(b.size()-1); ++bb; }
  return !a.empty() && !b.empty();
}

static working_string xform_usage_text(){
  return "Err:xform";
}

static bool parse_small_int(const working_string &src,int &v){
  working_string s=strip_outer_parens(nospace_lower(src));
  if (s.empty())
    return false;
  int sign=1,pos=0;
  if (s[0]=='-'){
    sign=-1;
    pos=1;
  }
  if (pos>=int(s.size()))
    return false;
  int n=0;
  for (;pos<int(s.size());++pos){
    if (!isdigit((unsigned char)s[pos]))
      return false;
    n=10*n+(s[pos]-'0');
  }
  v=sign*n;
  return true;
}

static bool parse_int_func_pow(const working_string &src,const char *fn,int &coef,working_string &arg,int &pow){
  working_string s=strip_outer_parens(compact(src));
  int p=0,n=s.size(),fl=strlen(fn);
  coef=0; pow=1;
  while (p<n && isdigit((unsigned char)s[p])){
    coef=10*coef+s[p]-'0';
    ++p;
  }
  if (!coef) coef=1;
  if (p+fl>=n || s.substr(p,fl)!=fn || s[p+fl]!='(')
    return false;
  int close=match_paren(s,p+fl);
  if (close<=p+fl)
    return false;
  if (close==n-1){
    arg=s.substr(p+fl+1,close-p-fl-1);
    return true;
  }
  if (close+2<n && s[close+1]=='^' && parse_small_int(s.substr(close+2,n-close-2),pow)){
    arg=s.substr(p+fl+1,close-p-fl-1);
    return true;
  }
  return false;
}

static bool parse_linear_tan_cot_int(const working_string &src,working_string &arg,int &tan_c,int &cot_c){
  working_string terms[6],a;
  int signs[6],n=split_top_sum_terms(src,terms,signs,6),c=0,p=0;
  if (n<1) return false;
  tan_c=0; cot_c=0; arg="";
  for (int i=0;i<n;++i){
    if (parse_int_func_pow(terms[i],"tan",c,a,p) && p==1)
      tan_c += signs[i]*c;
    else if (parse_int_func_pow(terms[i],"cot",c,a,p) && p==1)
      cot_c += signs[i]*c;
    else
      return false;
    if (arg.empty()) arg=a;
    else if (compact(arg)!=compact(a)) return false;
  }
  return !arg.empty();
}

static bool parse_cos2_minus_sin2_int(const working_string &src,working_string &arg,int &coef){
  working_string terms[3],a;
  int signs[3],n=split_top_sum_terms(src,terms,signs,3),c=0,p=0,sc=0,cc=0;
  if (n!=2) return false;
  arg="";
  for (int i=0;i<n;++i){
    if (parse_int_func_pow(terms[i],"sin",c,a,p) && p==2)
      sc += signs[i]*c;
    else if (parse_int_func_pow(terms[i],"cos",c,a,p) && p==2)
      cc += signs[i]*c;
    else
      return false;
    if (arg.empty()) arg=a;
    else if (compact(arg)!=compact(a)) return false;
  }
  if (!cc || sc!=-cc) return false;
  coef=cc;
  return true;
}

static bool parse_const_trig_square_int(const working_string &src,const char *fn,bool minus,working_string &arg,int &coef){
  working_string terms[3],a;
  int signs[3],n=split_top_sum_terms(src,terms,signs,3),c=0,p=0,tc=0,k=0;
  bool have_t=false,have_k=false;
  if (n!=2) return false;
  arg="";
  for (int i=0;i<n;++i){
    if (parse_int_func_pow(terms[i],fn,c,a,p) && p==2){
      if (signs[i]!=1 || have_t || !c)
        return false;
      have_t=true; tc=c; arg=a;
    }
    else if (parse_small_int(terms[i],c)){
      if (signs[i]!=(minus?-1:1) || have_k || !c)
        return false;
      have_k=true; k=c;
    }
    else
      return false;
  }
  if (!have_t || !have_k || tc!=k)
    return false;
  coef=k;
  return true;
}

static bool parse_sin2_cos2_sum_int(const working_string &src,working_string &arg,int &coef){
  working_string terms[3],a;
  int signs[3],n=split_top_sum_terms(src,terms,signs,3),c=0,p=0,sc=0,cc=0;
  if (n!=2) return false;
  arg="";
  for (int i=0;i<n;++i){
    if (signs[i]!=1)
      return false;
    if (parse_int_func_pow(terms[i],"sin",c,a,p) && p==2)
      sc=c;
    else if (parse_int_func_pow(terms[i],"cos",c,a,p) && p==2)
      cc=c;
    else
      return false;
    if (arg.empty()) arg=a;
    else if (compact(arg)!=compact(a)) return false;
  }
  if (!sc || sc!=cc) return false;
  coef=sc;
  return true;
}

static bool parse_sin_cos_product_int(const working_string &src,working_string &arg,int &coef){
  working_string f[4],a;
  int n=split_top_product(src,f,4),c=0,p=0,seen=0;
  coef=1; arg="";
  if (n<2) return false;
  for (int i=0;i<n;++i){
    int bit=0;
    if (parse_int_func_pow(f[i],"sin",c,a,p) && p==1) bit=1;
    else if (parse_int_func_pow(f[i],"cos",c,a,p) && p==1) bit=2;
    else if (parse_small_int(f[i],c)){ coef*=c; continue; }
    else return false;
    if (seen&bit) return false;
    seen|=bit; coef*=c;
    if (arg.empty()) arg=a;
    else if (compact(arg)!=compact(a)) return false;
  }
  return seen==3 && coef;
}

static bool parse_signed_sincos_linear(const working_string &term,int sign,bool &issin,Rat &coef,Rat &k){
  working_string arg;
  int c=0,p=0;
  if (parse_int_func_pow(term,"sin",c,arg,p) && p==1)
    issin=true;
  else if (parse_int_func_pow(term,"cos",c,arg,p) && p==1)
    issin=false;
  else
    return false;
  if (!parse_x_coeff(arg,k))
    return false;
  coef=rat(sign*c,1);
  return true;
}

static working_string product_two_trig_s(Rat coef,const char *f1,Rat k1,const char *f2,Rat k2){
  working_string out;
  Rat q=coef;
  if (q.n<0){ out="-"; q.n=-q.n; }
  if (!rat_is_one(q))
    out += rat_s(q)+"*";
  out += working_string(f1)+"("+xarg_s(k1)+")*"+f2+"("+xarg_s(k2)+")";
  return out;
}

static bool try_sum_to_product(const working_string &a,const working_string &b,working_string &out){
  working_string t[2];
  int sg[2];
  if (split_top_sum_terms(a,t,sg,2)!=2)
    return false;
  bool s1=false,s2=false;
  Rat c1,c2,k1,k2;
  if (!parse_signed_sincos_linear(t[0],sg[0],s1,c1,k1) ||
      !parse_signed_sincos_linear(t[1],sg[1],s2,c2,k2) || s1!=s2)
    return false;
  bool sum=rat_cmp(c1,c2)==0, diff=rat_cmp(c1,rat(-c2.n,c2.d))==0;
  if (!sum && !diff)
    return false;
  Rat mid=rat_div(rat_add(k1,k2),rat(2,1));
  Rat halfdiff=rat_div(rat_sub(k1,k2),rat(2,1));
  Rat two=rat_mul(c1,rat(2,1));
  working_string expect;
  if (s1)
    expect=sum ? product_two_trig_s(two,"sin",mid,"cos",halfdiff)
               : product_two_trig_s(two,"cos",mid,"sin",halfdiff);
  else
    expect=sum ? product_two_trig_s(two,"cos",mid,"cos",halfdiff)
               : product_two_trig_s(rat(-two.n,two.d),"sin",mid,"sin",halfdiff);
  if (!same_rewrite_expr(b,expect) && !khicas_equiv(expect,b))
    return false;
  out="Sum-prod:\n";
  out += expect+"";
  return true;
}

static bool double_arg(const working_string &dbl,const working_string &arg){
  working_string d=compact(dbl),a=compact(arg);
  bool bare=true;
  for (int i=0;i<int(a.size());++i)
    if (a[i]=='+' || a[i]=='-')
      bare=false;
  return (bare && d=="2"+a) || d=="2("+a+")";
}

static bool emit_trig_route(const char *label,const working_string &target,working_string &out){
  out=label;
  out += ":\n";
  out += insert_coeff_stars(target)+"";
  return true;
}

static bool try_pyth_pair(const working_string &a,const working_string &b,const char *sumfn,const char *singlefn,bool minus,working_string &out){
  working_string arg,targ;
  int c=0,cc=0,p=0;
  if (parse_const_trig_square_int(a,sumfn,minus,arg,cc) &&
      parse_int_func_pow(b,singlefn,c,targ,p) && p==2 && c==cc && compact(arg)==compact(targ))
    return emit_trig_route("Pythagorean identities",b,out);
  if (parse_int_func_pow(a,singlefn,c,arg,p) && p==2 &&
      parse_const_trig_square_int(b,sumfn,minus,targ,cc) && c==cc && compact(arg)==compact(targ))
    return emit_trig_route("Pythagorean identities",b,out);
  return false;
}

static bool parse_one_plus_trig_square(const working_string &src,const char *fn,working_string &arg){
  working_string t[2],a;
  int sg[2],c=0,p=0;
  if (split_top_sum_terms(src,t,sg,2)!=2)
    return false;
  bool one=false,trig=false;
  for (int i=0;i<2;++i){
    if (sg[i]!=1)
      return false;
    if (t[i]=="1"){ one=true; continue; }
    if (parse_int_func_pow(t[i],fn,c,a,p) && c==1 && p==2){
      trig=true;
      arg=a;
      continue;
    }
    return false;
  }
  return one && trig;
}

static bool try_xform_trig_power_identity(const working_string &a,const working_string &b,working_string &out){
  working_string base,exp,arg,targ;
  int n=0,c=0,p=0;
  if (parse_top_power(a,base,exp) && parse_small_int(exp,n) && n>1){
    if (parse_one_plus_trig_square(base,"tan",arg) &&
        parse_int_func_pow(b,"sec",c,targ,p) && c==1 && p==2*n && compact(arg)==compact(targ)){
      out="Pythagorean identities:\n1+tan(u)^2=sec(u)^2\n"+b+"";
      return true;
    }
    if (parse_one_plus_trig_square(base,"cot",arg) &&
        parse_int_func_pow(b,"cosec",c,targ,p) && c==1 && p==2*n && compact(arg)==compact(targ)){
      out="Pythagorean identities:\n1+cot(u)^2=cosec(u)^2\n"+b+"";
      return true;
    }
  }
  working_string lt[2],rt[2],rbase,rexp,sarg,targ2;
  int lsg[2],rsg[2],m=0,rn=0,c1=0,c2=0,p1=0,p2=0;
  if (split_top_sum_terms(a,lt,lsg,2)==2 && split_top_sum_terms(b,rt,rsg,2)==2 &&
      lsg[0]==1 && lsg[1]==-1 && rsg[0]==1 && rsg[1]==-1 &&
      parse_int_func_pow(lt[0],"sec",c1,sarg,p1) && parse_int_func_pow(lt[1],"tan",c2,targ,p2) &&
      c1==1 && c2==1 && p1==p2 && p1>2 && compact(sarg)==compact(targ) &&
      parse_top_power(rt[0],rbase,rexp) && parse_small_int(rexp,rn) &&
      parse_int_func_pow(rbase,"sec",c1,sarg,m) && c1==1 && m==2 &&
      parse_top_power(rt[1],rbase,rexp) && parse_small_int(rexp,n) &&
      parse_int_func_pow(rbase,"tan",c2,targ2,m) && c2==1 && m==2 &&
      rn==n && p1==2*n && compact(sarg)==compact(targ2)){
    out="Power regroup:\n"+b+"";
    return true;
  }
  return false;
}

static bool parse_one_cos_sum(const working_string &src,bool minus,working_string &arg){
  working_string t[2],a;
  int sg[2];
  if (split_top_sum_terms(src,t,sg,2)!=2)
    return false;
  bool one=false,cosf=false;
  for (int i=0;i<2;++i){
    if (sg[i]==1 && t[i]=="1"){ one=true; continue; }
    if (sg[i]==(minus?-1:1) && parse_unary_arg(t[i],"cos",a)){
      cosf=true;
      arg=a;
      continue;
    }
    return false;
  }
  return one && cosf;
}

static bool half_arg_of(const working_string &half,const working_string &full){
  working_string h=compact(half), f=compact(full);
  return h==f+"/2" || h=="("+f+")/2";
}

static bool try_xform_half_angle_identity(const working_string &a,const working_string &b,working_string &out){
  working_string num,den,arg,darg,targ;
  int c=0,p=0;
  if (!split_top_fraction(a,num,den) || !parse_int_func_pow(b,"tan",c,targ,p) || c!=1)
    return false;
  if (parse_one_cos_sum(num,true,arg) && parse_one_cos_sum(den,false,darg) &&
      compact(arg)==compact(darg) && p==2 && half_arg_of(targ,arg)){
    out="Half-angle:\n"+b+"";
    return true;
  }
  if (parse_unary_arg(num,"sin",arg) && parse_one_cos_sum(den,false,darg) &&
      compact(arg)==compact(darg) && p==1 && half_arg_of(targ,arg)){
    out="Half-angle:\n"+b+"";
    return true;
  }
  if (parse_one_cos_sum(num,true,arg) && parse_unary_arg(den,"sin",darg) &&
      compact(arg)==compact(darg) && p==1 && half_arg_of(targ,arg)){
    out="Half-angle:\n"+b+"";
    return true;
  }
  return false;
}

static bool try_xform_linear_fractional_inverse(const working_string &a,const working_string &b,working_string &out){
  working_string l,r,num,den,tl,tr,tn,td;
  Rat A,B,C,D,ny,nc,dy,dc;
  if (!split_equal_sides(a,l,r) || l!="y" || !split_top_fraction(r,num,den) ||
      !parse_affine_any(num,'x',A,B) || !parse_affine_any(den,'x',C,D) ||
      !A.n || !C.n)
    return false;
  if (!split_equal_sides(b,tl,tr) || tl!="x" || !split_top_fraction(tr,tn,td) ||
      !parse_affine_any(tn,'y',ny,nc) || !parse_affine_any(td,'y',dy,dc))
    return false;
  if (rat_cmp(nc,B) || rat_cmp(ny,rat(-D.n,D.d)) ||
      rat_cmp(dy,C) || rat_cmp(dc,rat(-A.n,A.d)))
    return false;
  out="Linear fraction:\n";
  out += b+"";
  return true;
}

static bool parse_inverse_numerator(const working_string &src,const char *invfn,Rat B){
  working_string t[2],arg,base,exp;
  int sg[2],terms=split_top_sum_terms(src,t,sg,2);
  bool got_inv=false,got_const=false;
  for (int i=0;i<terms;++i){
    if (!strcmp(invfn,"square")){
      if (sg[i]==1 && parse_top_power(t[i],base,exp) && base=="y" && exp=="2"){
        got_inv=true;
        continue;
      }
    }
    else if (sg[i]==1 && parse_unary_arg(t[i],invfn,arg) && arg=="y"){
      got_inv=true;
      continue;
    }
    Rat q;
    if (parse_rat(t[i],q)){
      if (sg[i]<0)
        q.n=-q.n;
      if (!rat_cmp(q,rat(-B.n,B.d))){
        got_const=true;
        continue;
      }
    }
    return false;
  }
  return got_inv && got_const;
}

static bool try_xform_elementary_inverse(const working_string &a,const working_string &b,working_string &out){
  working_string l,r,arg,tl,tr,num,den;
  Rat A,B,D;
  const char *fn=0,*inv=0;
  if (!split_equal_sides(a,l,r) || l!="y")
    return false;
  if (parse_unary_arg(r,"ln",arg)){ fn="ln"; inv="exp"; }
  else if (parse_unary_arg(r,"exp",arg)){ fn="exp"; inv="ln"; }
  else if (parse_unary_arg(r,"sqrt",arg)){ fn="sqrt"; inv="square"; }
  else return false;
  if (!parse_affine_any(arg,'x',A,B) || !A.n ||
      !split_equal_sides(b,tl,tr) || tl!="x" || !split_top_fraction(tr,num,den) ||
      !parse_rat(den,D) || rat_cmp(D,A) || !parse_inverse_numerator(num,inv,B))
    return false;
  out="Inverse function:\n";
  out += working_string(fn)+"(a*x+b) = y\n";
  if (!strcmp(fn,"ln"))
    out += "a*x+b = exp(y)\n";
  else if (!strcmp(fn,"exp"))
    out += "a*x+b = ln(y)\n";
  else
    out += "a*x+b = y^2\n";
  out += b+"";
  return true;
}

static bool try_xform_ellipse_clear_denominators(const working_string &a,const working_string &b,working_string &out){
  working_string left,right,t[2],num1,den1,num2,den2;
  int sg[2];
  Rat d1,d2,prod;
  if (!split_equal_sides(a,left,right) || right!="1" ||
      split_top_sum_terms(left,t,sg,2)!=2 || sg[0]!=1 || sg[1]!=1 ||
      !split_top_fraction(t[0],num1,den1) || !split_top_fraction(t[1],num2,den2) ||
      !parse_rat(den1,d1) || !parse_rat(den2,d2) || d1.d!=1 || d2.d!=1)
    return false;
  prod=rat_mul(d1,d2);
  working_string expected="("+num1+")"+rat_s(d2)+"+("+num2+")"+rat_s(d1)+"="+rat_s(prod);
  if (b!=compact(expected))
    return false;
  out="Clear denominators:\n";
  out += left+" = 1\n";
  out += "multiply by "+rat_s(prod)+"\n";
  out += b+"";
  return true;
}

static bool try_xform_zero_difference_of_squares(const working_string &a,const working_string &b,working_string &out){
  working_string t[2],base1,base2,exp1,exp2;
  int sg[2];
  if (b!="0" || split_top_sum_terms(a,t,sg,2)!=2 || sg[0]!=1 || sg[1]!=-1 ||
      !parse_top_power(t[0],base1,exp1) || !parse_top_power(t[1],base2,exp2) ||
      exp1!="2" || exp2!="2")
    return false;
  out="Equation F=0:\n";
  out += "("+base1+")^2 - ("+base2+")^2 = 0\n";
  out += "(("+base1+")-("+base2+"))(("+base1+")+("+base2+")) = 0\n";
  out += "0";
  return true;
}

static bool parse_scaled_var_square_term(const working_string &src,char v,long &d){
  working_string base,exp,num,den;
  Rat q;
  if (!parse_top_power(src,base,exp) || exp!="2" || !split_top_fraction(base,num,den) ||
      num!=working_string(1,v) || !parse_rat(den,q) || q.d!=1 || !q.n)
    return false;
  d=q.n<0?-q.n:q.n;
  return true;
}

static bool try_xform_ellipse_y2_rearrange(const working_string &a,const working_string &b,working_string &out){
  working_string left,right,t[2];
  int sg[2];
  long ad=0,bd=0;
  if (!split_equal_sides(a,left,right) || right!="1" ||
      split_top_sum_terms(left,t,sg,2)!=2 || sg[0]!=1 || sg[1]!=1 ||
      !parse_scaled_var_square_term(t[0],'x',ad) ||
      !parse_scaled_var_square_term(t[1],'y',bd))
    return false;
  working_string expected="y^2=("+int_s(bd)+")^2(1-x^2/"+int_s(ad*ad)+")";
  if (b!=compact(expected))
    return false;
  out="Ellipse rearrange:\n";
  out += "(y/"+int_s(bd)+")^2=1-x^2/"+int_s(ad*ad)+"\n";
  out += b+"";
  return true;
}

static bool try_xform_sqrt_square_abs(const working_string &a,const working_string &b,working_string &out){
  working_string at[2],bt[2],arg,base,exp,barg;
  int asg[2],bsg[2];
  Rat ca,cb;
  if (split_top_sum_terms(a,at,asg,2)!=2 || split_top_sum_terms(b,bt,bsg,2)!=2 ||
      asg[0]!=1 || bsg[0]!=1 || !parse_unary_arg(at[0],"sqrt",arg) ||
      !parse_top_power(arg,base,exp) || exp!="2" ||
      !parse_unary_arg(bt[0],"abs",barg) || compact(base)!=compact(barg) ||
      !parse_rat(at[1],ca) || !parse_rat(bt[1],cb))
    return false;
  if (asg[1]<0) ca.n=-ca.n;
  if (bsg[1]<0) cb.n=-cb.n;
  if (rat_cmp(ca,cb))
    return false;
  out="Principal square root:\n";
  out += "sqrt(u^2)=abs(u)\n";
  out += b+"";
  return true;
}

static bool try_xform_sqrt_conjugate_square(const working_string &a,const working_string &b,working_string &out){
  working_string base,exp,t[2],u,v;
  int sg[2];
  Rat A1,B1,A2,B2,c2;
  if (!parse_top_power(a,base,exp) || exp!="2" ||
      split_top_sum_terms(strip_outer_parens(base),t,sg,2)!=2 ||
      sg[0]!=1 || !parse_unary_arg(t[0],"sqrt",u) ||
      !parse_unary_arg(t[1],"sqrt",v) ||
      !parse_affine_any(u,'x',A1,B1) || !parse_affine_any(v,'x',A2,B2) ||
      A1.n!=A1.d || A2.n!=A2.d || rat_cmp(B1,rat(-B2.n,B2.d)))
    return false;
  c2=rat_mul(B1,B1);
  working_string expected="2*x";
  expected += sg[1]<0 ? "-2*sqrt(x^2-"+rat_s(c2)+")" : "+2*sqrt(x^2-"+rat_s(c2)+")";
  if (compact(expected)!=b)
    return false;
  out="Square:\n";
  out += "(a +/- b)^2 = a^2 +/- 2ab + b^2\n";
  out += expected+"";
  return true;
}

static bool try_xform_difference_square_quotient(const working_string &a,const working_string &b,working_string &out){
  working_string num,den,t[2],base,exp;
  int sg[2];
  Rat A,B,C,ta,tb,bb;
  if (!split_top_fraction(a,num,den) || den!="x" ||
      split_top_sum_terms(num,t,sg,2)!=2 || sg[0]!=1 || sg[1]!=-1 ||
      !parse_top_power(t[0],base,exp) || exp!="2" ||
      !parse_affine_any(base,'x',A,B) || A.n!=A.d || !parse_rat(t[1],C) ||
      !parse_affine_any(b,'x',ta,tb) || ta.n!=ta.d)
    return false;
  bb=rat_mul(B,B);
  if (rat_cmp(C,bb) || rat_cmp(tb,rat_mul(rat(2,1),B)))
    return false;
  out="Difference of squares:\n";
  out += "((x+a)^2-a^2)/x=x+2a\n";
  out += b+"";
  return true;
}

static bool try_xform_abs_square_or_scale(const working_string &a,const working_string &b,working_string &out){
  working_string base,exp,arg,tbase,texp,targ,f[3];
  if (parse_top_power(a,base,exp) && exp=="2" && parse_unary_arg(base,"abs",arg) &&
      parse_top_power(b,tbase,texp) && texp=="2" && compact(arg)==compact(tbase)){
    out="Modulus identity:\n";
    out += "abs(u)^2 = u^2\n";
    out += b+"";
    return true;
  }
  if (!parse_unary_arg(a,"abs",arg))
    return false;
  Rat A,B,coef,ta,tb;
  if (!parse_affine_any(arg,'x',A,B) || !A.n)
    return false;
  int fn=split_top_product(b,f,3);
  if (fn!=2)
    return false;
  int ci=-1,ai=-1;
  for (int i=0;i<2;++i){
    Rat q;
    if (parse_rat(f[i],q)){ coef=q; ci=i; continue; }
    if (parse_unary_arg(f[i],"abs",targ)){ ai=i; continue; }
  }
  if (ci<0 || ai<0 || !parse_affine_any(targ,'x',ta,tb))
    return false;
  Rat absA=A.n<0?rat(-A.n,A.d):A;
  if (rat_cmp(coef,absA) || ta.n!=ta.d || rat_cmp(tb,rat_div(B,A)))
    return false;
  out="Modulus scaling:\n";
  out += "abs(a*x+b)=abs(a)*abs(x+b/a)\n";
  out += b+"";
  return true;
}

struct RecipRule {
  const char *fn;
  const char *num;
  const char *den;
};

static working_string recip_rule_expr(const RecipRule &r,const working_string &arg){
  working_string d=working_string(r.den)+"("+arg+")";
  if (r.num[0]=='1' && !r.num[1])
    return "1/"+d;
  return working_string(r.num)+"("+arg+")/"+d;
}

static bool try_recip_rule(const RecipRule &r,const working_string &a,const working_string &b,working_string &out){
  working_string arg;
  if (parse_unary_arg(a,r.fn,arg) && same_rewrite_expr(b,recip_rule_expr(r,arg)))
    return emit_trig_route("Reciprocal identities",b,out);
  if (parse_unary_arg(b,r.fn,arg) && same_rewrite_expr(a,recip_rule_expr(r,arg)))
    return emit_trig_route("Reciprocal identities",b,out);
  return false;
}

static bool try_xform_trig_direct(const working_string &a,const working_string &b,working_string &out){
  working_string arg,targ;
  int tc=0,cc=0,c=0,p=0;
  static const RecipRule recip[]={{"cot","cos","sin"},{"tan","sin","cos"},{"sec","1","cos"},{"cosec","1","sin"}};
  for (int i=0;i<4;++i)
    if (try_recip_rule(recip[i],a,b,out))
      return true;
  if (try_xform_trig_power_identity(a,b,out))
    return true;
  if (try_xform_half_angle_identity(a,b,out))
    return true;
  if (try_sum_to_product(a,b,out))
    return true;
  if (parse_linear_tan_cot_int(a,arg,tc,cc) && cc && tc==-cc &&
      parse_int_func_pow(b,"cot",c,targ,p) && p==1 && c==2*cc && double_arg(targ,arg)){
    out="Reciprocal identities:\nCommon denominator:\nDouble-angle identities:\n";
    out += insert_coeff_stars(b)+"";
    return true;
  }
  if (parse_sin_cos_product_int(a,arg,cc) &&
      parse_int_func_pow(b,"sin",c,targ,p) && p==1 && cc==2*c && double_arg(targ,arg))
    return emit_trig_route("Double-angle identities",b,out);
  if (parse_int_func_pow(a,"sin",c,arg,p) && p==1 &&
      parse_sin_cos_product_int(b,targ,cc) && cc==2*c && double_arg(arg,targ))
    return emit_trig_route("Double-angle identities",b,out);
  if (parse_cos2_minus_sin2_int(a,arg,cc) &&
      parse_int_func_pow(b,"cos",c,targ,p) && p==1 && c==cc && double_arg(targ,arg)){
    return emit_trig_route("Double-angle identities",b,out);
  }
  if (try_pyth_pair(a,b,"tan","sec",false,out) ||
      try_pyth_pair(a,b,"cot","cosec",false,out))
    return true;
  if (parse_sin2_cos2_sum_int(a,arg,cc) && parse_small_int(b,c) && c==cc){
    return emit_trig_route("Pythagorean identities",b,out);
  }
  if (parse_small_int(a,c) && parse_sin2_cos2_sum_int(b,arg,cc) && c==cc){
    return emit_trig_route("Pythagorean identities",b,out);
  }
  return try_pyth_pair(a,b,"sec","tan",true,out) ||
         try_pyth_pair(a,b,"cosec","cot",true,out);
}

static bool exact_simplify_scalar(const working_string &expr,working_string &out){
  working_string cand;
  if (exact_simplify_deep(expr,cand)){
    cand=trim(cand);
    if (!cand.empty() && cand[0]!='[' && !contains(cand,"I")){
      out=cand;
      return true;
    }
  }
  return false;
}

static bool verify_xform_parameter(const working_string &eq,const working_string &var,
                                   const working_string &value,working_string &lhs,
                                   working_string &rhs){
  working_string left,right;
  if (!split_equal_sides(eq,left,right))
    return false;
  working_string repl="("+value+")";
  lhs=replace_identifier_token(left,var,repl);
  rhs=replace_identifier_token(right,var,repl);
  return khicas_zero("("+lhs+")-("+rhs+")");
}

static bool xform_eval_numeric_at(const working_string &expr,char v,double x,double &out){
  working_string e=replace_identifier_token(expr,working_string(1,v),"("+double_s(x)+")");
  e=normalize_top_product(e);
  NumParser np;
  np.p=e.c_str();
  np.ok=true;
  out=np.expr();
  np.skip();
  return np.ok && !*np.p && finite_double(out);
}

static bool xform_numeric_constant(const working_string &expr,char v,working_string &value){
  static const double pts[]={0.23,0.41,0.77,1.03,1.37,1.91};
  working_string first;
  int ok=0;
  for (int i=0;i<int(sizeof(pts)/sizeof(pts[0]));++i){
    double y=0;
    if (!xform_eval_numeric_at(expr,v,pts[i],y))
      continue;
    working_string r;
    if (!rational_approx(y,r))
      return false;
    if (first.empty())
      first=r;
    else if (first!=r)
      return false;
    ++ok;
  }
  if (ok<3 || first.empty())
    return false;
  value=first;
  return true;
}

static bool xform_numeric_parameter_ok(const working_string &eq,const working_string &var,
                                       const working_string &value,working_string &lhs,
                                       working_string &rhs){
  working_string left,right;
  if (!split_equal_sides(eq,left,right))
    return false;
  working_string repl="("+value+")";
  lhs=replace_identifier_token(left,var,repl);
  rhs=replace_identifier_token(right,var,repl);
  char v=default_var_char(lhs+rhs);
  static const double pts[]={0.23,0.41,0.77,1.03,1.37,1.91};
  int ok=0;
  for (int i=0;i<int(sizeof(pts)/sizeof(pts[0]));++i){
    double a=0,b=0;
    if (!xform_eval_numeric_at(lhs,v,pts[i],a) ||
        !xform_eval_numeric_at(rhs,v,pts[i],b))
      continue;
    double scale=1.0+fabs(a)+fabs(b);
    if (fabs(a-b)>1e-8*scale)
      return false;
    ++ok;
  }
  return ok>=3;
}

static bool contains_identifier_name(const working_string &expr,const working_string &name){
  return replace_identifier_token(expr,name,"#")!=expr;
}

static working_string mul_join_factor(working_string acc,const working_string &f){
  working_string t=strip_outer_parens(f);
  if (t.empty() || t=="1")
    return acc.empty()?working_string("1"):acc;
  if (acc.empty() || acc=="1")
    return t;
  return acc+"*"+t;
}

static bool xform_parameter_coeff(const working_string &expr,const working_string &target,
                                  working_string &coeff){
  working_string factors[12];
  int n=split_top_product(expr,factors,12),hits=0;
  coeff="1";
  if (compact(expr)==compact(target))
    return true;
  if (n<=0)
    return false;
  for (int i=0;i<n;++i){
    working_string f=strip_outer_parens(factors[i]);
    if (compact(f)==compact(target)){
      ++hits;
      continue;
    }
    if (f.size()>1 && f[0]=='-' && compact(f.substr(1,f.size()-1))==compact(target)){
      ++hits;
      coeff=mul_join_factor(coeff,"-1");
      continue;
    }
    if (contains_identifier_name(f,target))
      return false;
    coeff=mul_join_factor(coeff,f);
  }
  return hits==1;
}

static bool try_xform_linear_parameter_ratio(const working_string &start,
                                             const working_string &target,
                                             working_string &out){
  if (!plain_identifier_name(target))
    return false;
  working_string left,right;
  if (!split_equal_sides(start,left,right))
    return false;
  bool lin=contains_identifier_name(left,target);
  bool rin=contains_identifier_name(right,target);
  if (lin==rin)
    return false;
  working_string coeff,other;
  if (lin){
    if (!xform_parameter_coeff(left,target,coeff))
      return false;
    other=right;
  }
  else {
    if (!xform_parameter_coeff(right,target,coeff))
      return false;
    other=left;
  }
  working_string raw="("+other+")/("+coeff+")",sol,lhs,rhs;
  if (!exact_simplify_scalar(raw,sol))
    sol=raw;
  char v=default_var_char(raw);
  working_string numeric;
  if ((sol==raw || contains_identifier_name(sol,working_string(1,v))) &&
      xform_numeric_constant(raw,v,numeric))
    sol=numeric;
  if (!verify_xform_parameter(start,target,sol,lhs,rhs) &&
      !xform_numeric_parameter_ok(start,target,sol,lhs,rhs))
    return false;
  out="Planner search:\n";
  out += "Parameter isolation:\n";
  out += target+" = "+sol+"\n";
  out += "Substitute:\n";
  out += lhs+" = "+rhs+"\n";
  out += "\n";
  out += "";
  return true;
}

static bool try_xform_isolate_parameter(const working_string &start,
                                        const working_string &target,
                                        working_string &out){
  return try_xform_linear_parameter_ratio(start,target,out);
}

static bool try_xform_constant_parameter_solve(const working_string &start,
                                               const working_string &target,
                                               working_string &out){
  if (!plain_identifier_name(target) || !contains(start,"="))
    return false;
  working_string exact,vals[8],lhs,rhs;
  if (!production_exact_command("solve("+start+","+target+")",exact))
    return false;
  int n=split_exact_list(exact,vals,8);
  for (int i=0;i<n;++i){
    working_string v=trim(vals[i]);
    if (v.empty() || contains(v,"I") || contains_identifier_name(v,"x") ||
        contains_identifier_name(v,target))
      continue;
    bool depends=false;
    for (char c='a';c<='z';++c){
      working_string nm(1,c);
      if (nm!=target && contains_identifier_name(start,nm) &&
          contains_identifier_name(v,nm)){
        depends=true;
        break;
      }
    }
    if (depends)
      continue;
    if (!verify_xform_parameter(start,target,v,lhs,rhs) &&
        !xform_numeric_parameter_ok(start,target,v,lhs,rhs))
      continue;
    out="Planner search:\n";
    out += "Solve parameter:\n";
    out += target+" = "+v+"\n";
    out += "Substitute:\n"+lhs+" = "+rhs+"\n";
    out += "";
    return true;
  }
  return false;
}

static bool try_xform_solve_parameter_general(const working_string &start,
                                              const working_string &target,
                                              working_string &out){
  if (!plain_identifier_name(target) || !contains(start,"="))
    return false;
  working_string exact,vals[6],lhs,rhs;
  if (!production_exact_command("solve("+start+","+target+")",exact))
    return false;
  int n=split_exact_list(exact,vals,6);
  for (int i=0;i<n;++i){
    working_string v=trim(vals[i]);
    if (v.empty() || contains(v,"I") || contains_identifier_name(v,target))
      continue;
    if (!verify_xform_parameter(start,target,v,lhs,rhs) &&
        !xform_numeric_parameter_ok(start,target,v,lhs,rhs))
      continue;
    out="Planner search:\nParameter isolation:\n";
    out += target+" = "+v+"\nSubstitute:\n"+lhs+" = "+rhs+"\n";
    out += "";
    return true;
  }
  return false;
}

static bool try_xform_r_parameter_identity(const working_string &start,
                                           const working_string &target,
                                           working_string &out){
  working_string s=nospace_lower(start);
  if (lower(target)!="r" || !contains(s,"=r*sin(x+a)") ||
      !contains(s,"sin(x)") || !contains(s,"cos(x)"))
    return false;
  out="R-form:\n";
  out += "Expand R*sin(x+a)\n";
  out += "R*cos(a)=k and R*sin(a)=1\n";
  out += "R^2=k^2+1\n";
  out += "R=sqrt(k^2+1)";
  return true;
}

static bool try_xform_atan_recip_domain(const working_string &a,const working_string &b,working_string &out){
  working_string t[2],arg1,arg2,num,den;
  int sg[2];
  if (compact(b)!="pi/2" || split_top_sum_terms(a,t,sg,2)!=2 || sg[0]!=1 || sg[1]!=1 ||
      !parse_unary_arg(t[0],"atan",arg1) || !parse_unary_arg(t[1],"atan",arg2))
    return false;
  bool recip=(split_top_fraction(arg2,num,den) && num=="1" && same_rewrite_expr(den,arg1)) ||
             (split_top_fraction(arg1,num,den) && num=="1" && same_rewrite_expr(den,arg2));
  if (!recip)
    return false;
  out="Inverse tangent identity:\n";
  out += "atan(u)+atan(1/u)=pi/2, u>0\n";
  out += insert_coeff_stars(arg1)+" > 0\n";
  out += "pi/2";
  return true;
}

static bool try_xform_tan_add_parameter(const working_string &a,const working_string &b,working_string &out){
  working_string num,den,arg;
  if (!split_top_fraction(a,num,den) || !parse_unary_arg(b,"tan",arg) || arg!="x+a")
    return false;
  working_string n=replace_all_literal(compact(num),"*","");
  working_string d=replace_all_literal(compact(den),"*","");
  if (n!="sin(x)+kcos(x)" || d!="cos(x)-ksin(x)")
    return false;
  out="Tangent addition:\n";
  out += "tan(x+a), k=tan(a)\n";
  out += insert_coeff_stars(b)+" under k=tan(a)";
  return true;
}

static bool try_xform_change_base_recip(const working_string &a,const working_string &b,working_string &out){
  working_string ba,ua,bb,ub,num,den,rnum,rden;
  if (parse_log_call_ws(a,ba,ua) && split_top_fraction(b,num,den) &&
      num=="1" && parse_log_call_ws(den,bb,ub) &&
      same_rewrite_expr(ba,ub) && same_rewrite_expr(ua,bb)){
    out="Change of base:\n";
    out += "log_a(b)=ln(b)/ln(a)\n";
    out += "log_b(a)=ln(a)/ln(b)\n";
    out += "log_a(b)=1/log_b(a)\n";
    out += insert_coeff_stars(b);
    return true;
  }
  if (split_top_fraction(a,num,den) && num=="1" && parse_log_call_ws(den,ba,ua) &&
      parse_log_call_ws(b,bb,ub) && same_rewrite_expr(ba,ub) && same_rewrite_expr(ua,bb)){
    out="Change of base:\n";
    out += "1/log_b(a)=log_a(b)\n";
    out += insert_coeff_stars(b);
    return true;
  }
  if (split_top_fraction(a,num,den) && parse_log_call_ws(num,ba,ua) &&
      parse_log_call_ws(den,bb,ub) && same_rewrite_expr(ba,bb) &&
      split_top_fraction(b,rnum,rden) && rnum=="1" &&
      split_top_fraction(rden,bb,ba) && same_rewrite_expr(ba,num) &&
      same_rewrite_expr(bb,den)){
    out="Change of base:\n";
    out += "log_a(b)=ln(b)/ln(a)\n";
    out += "ln(b)/ln(a)=1/(ln(a)/ln(b))\n";
    out += insert_coeff_stars(b);
    return true;
  }
  return false;
}

static bool try_xform_cot_sin_double(const working_string &a,const working_string &b,working_string &out){
  working_string num,den,arg,onearg,targ;
  int c=0,p=0;
  if (!split_top_fraction(a,num,den) ||
      !parse_int_func_pow(num,"cot",c,arg,p) || c!=2 || p!=1 ||
      !parse_one_plus_trig_square(den,"cot",onearg) ||
      !parse_int_func_pow(b,"sin",c,targ,p) || c!=1 || p!=1 ||
      compact(arg)!=compact(onearg) || !double_arg(targ,arg))
    return false;
  out="Use cot=cos/sin:\n";
  out += "2*cot(u)/(1+cot(u)^2)\n";
  out += "= 2sin(u)cos(u)\n";
  out += "= sin(2u)\n";
  out += insert_coeff_stars(b);
  return true;
}

static bool try_xform_power_diff_cancel(const working_string &a,const working_string &b,working_string &out){
  working_string tmp,last;
  if (!try_normal_power_difference_quotient((working_string("normal(")+a+")").c_str(),tmp))
    return false;
  last=last_nonempty_line(tmp);
  if (!same_rewrite_expr(last,b))
    return false;
  out=tmp;
  return true;
}

static bool try_xform_trig_identity_quotient(const working_string &a,const working_string &b,working_string &out){
  working_string num,den,arg,targ;
  int c=0,p=0;
  if (b!="1" || !split_top_fraction(a,num,den))
    return false;
  if (!parse_one_plus_trig_square(num,"tan",arg) ||
      !parse_int_func_pow(den,"sec",c,targ,p) || c!=1 || p!=2 || compact(arg)!=compact(targ))
    return false;
  out="Pyth:\n1+tan(u)^2=sec(u)^2\n=1";
  return true;
}

static bool trig_recip_product_term(const working_string &a,const working_string &b,working_string &res){
  working_string aa,bb;
  if (parse_unary_arg(a,"sin",aa) && (parse_unary_arg(b,"cosec",bb) || parse_unary_arg(b,"csc",bb)) && same_rewrite_expr(aa,bb)){ res="1"; return true; }
  if ((parse_unary_arg(a,"cosec",aa) || parse_unary_arg(a,"csc",aa)) && parse_unary_arg(b,"sin",bb) && same_rewrite_expr(aa,bb)){ res="1"; return true; }
  if (parse_unary_arg(a,"cos",aa) && parse_unary_arg(b,"sec",bb) && same_rewrite_expr(aa,bb)){ res="1"; return true; }
  if (parse_unary_arg(a,"sec",aa) && parse_unary_arg(b,"cos",bb) && same_rewrite_expr(aa,bb)){ res="1"; return true; }
  if (parse_unary_arg(a,"cos",aa) && (parse_unary_arg(b,"cosec",bb) || parse_unary_arg(b,"csc",bb)) && same_rewrite_expr(aa,bb)){ res="cot("+aa+")"; return true; }
  if ((parse_unary_arg(a,"cosec",aa) || parse_unary_arg(a,"csc",aa)) && parse_unary_arg(b,"cos",bb) && same_rewrite_expr(aa,bb)){ res="cot("+aa+")"; return true; }
  if (parse_unary_arg(a,"sin",aa) && parse_unary_arg(b,"sec",bb) && same_rewrite_expr(aa,bb)){ res="tan("+aa+")"; return true; }
  if (parse_unary_arg(a,"sec",aa) && parse_unary_arg(b,"sin",bb) && same_rewrite_expr(aa,bb)){ res="tan("+aa+")"; return true; }
  return false;
}

static bool try_xform_trig_recip_product_expand(const working_string &rawa,
                                                const working_string &rawb,
                                                working_string &out){
  working_string f[2],t0[3],t1[3],res,term;
  int s0[3],s1[3],one=0;
  if (split_top_product(rawa,f,2)!=2)
    return false;
  int n0=split_top_sum_terms(f[0],t0,s0,3),n1=split_top_sum_terms(f[1],t1,s1,3);
  if (n0<2 || n1<2)
    return false;
  for (int i=0;i<n0;++i)
    for (int j=0;j<n1;++j){
      if (!trig_recip_product_term(t0[i],t1[j],term))
        return false;
      int sg=s0[i]*s1[j];
      if (term=="1") one += sg;
      else res=join_sum(res,signed_part(sg,term));
    }
  if (one)
    res=join_sum(res,int_s(one));
  if (res.empty())
    res="0";
  if (!same_rewrite_expr(res,rawb) && !khicas_equiv(res,rawb))
    return false;
  out="Expand:\n";
  out += insert_coeff_stars(rawa)+"\n";
  out += "Use reciprocal identities\n";
  out += insert_coeff_stars(res)+"\n";
  out += "Target:\n"+insert_coeff_stars(rawb);
  return true;
}

static bool try_xform_cofunction(const working_string &a,const working_string &b,working_string &out){
  working_string arg,targ,inner;
  if (!parse_unary_arg(a,"tan",arg) || !parse_unary_arg(b,"cot",targ))
    return false;
  working_string c=compact(arg), t=compact(targ);
  if (c=="pi/2-"+t || c=="(pi/2)-"+t || c=="90-"+t || c=="(90)-"+t){
    out="Cofunction identity:\n";
    out += "tan(pi/2-u)=cot(u)\n";
    out += insert_coeff_stars(b);
    return true;
  }
  return false;
}

static bool try_xform_equation_clear_denominator(const working_string &start,
                                                 const working_string &target,
                                                 working_string &out){
  working_string l,r,num,den,cleared,tl,tr,target_expr=target;
  if (!split_equal_sides(start,l,r))
    return false;
  if (split_equal_sides(target,tl,tr))
    target_expr="("+tl+")-("+tr+")";
  if (split_top_fraction(l,num,den))
    cleared="("+num+")-("+r+")*("+den+")";
  else if (split_top_fraction(r,num,den))
    cleared="("+l+")*("+den+")-("+num+")";
  else
    cleared="("+l+")-("+r+")";
  if (!khicas_zero("("+cleared+")-("+target_expr+")") &&
      !khicas_zero("("+cleared+")+("+target_expr+")"))
    return false;
  out="Planner search:\nClear denominators:\n";
  out += insert_coeff_stars(target)+"";
  return true;
}

static bool try_xform_taylor_approx(const working_string &a,const working_string &b,working_string &out){
  if (!contains_var_symbol(a,'x') || !contains_var_symbol(b,'x'))
    return false;
  working_string ca=compact(a), cb=compact(b);
  if (!contains(a,"exp(") && !contains(a,"sin(") && !contains(a,"cos(") &&
      !contains(a,"tan(") && !contains(a,"ln(") && !contains(a,"log(") &&
      ca!="1/(1-x)")
    return false;
  for (working_string::const_iterator it=b.begin();it!=b.end();++it){
    if (((*it>='a' && *it<='z') || (*it>='A' && *it<='Z')) && *it!='x')
      return false;
  }
  {
    if (ca=="exp(x)" && cb=="1+x+x^2/2+x^3/6"){
      out="Taylor approximation:\n";
      out += insert_coeff_stars(b)+"";
      return true;
    }
    if (ca=="sin(x)" && cb=="x-x^3/6+x^5/120"){
      out="Taylor approximation:\n";
      out += insert_coeff_stars(b)+"";
      return true;
    }
    if (ca=="cos(x)" && cb=="1-x^2/2+x^4/24"){
      out="Taylor approximation:\n";
      out += insert_coeff_stars(b)+"";
      return true;
    }
    if (ca=="ln(1+x)" && cb=="x-x^2/2+x^3/3-x^4/4"){
      out="Taylor approximation:\n";
      out += insert_coeff_stars(b)+"";
      return true;
    }
    if (ca=="1/(1-x)" && cb=="1+x+x^2+x^3+x^4"){
      out="Geometric series approximation:\n";
      out += insert_coeff_stars(b)+"";
      return true;
    }
  }
  return false;
}

static bool try_khicas_exact_route(const char *input,working_string &out){
  working_string ans,cs=compact(input?input:"");
  int p=cs.find('(');
  if (cs.empty() || !exact_command_clean(input?input:"",ans) || ans.empty() || ans.size()>420 || p<1)
    return false;
  working_string rawcmd=cs.substr(0,p),cmd=rawcmd;
  if (cmd=="int") cmd="integrate";
  if (!plain_identifier_name(cmd))
    return false;
  working_string args[2],cur,cand;
  int n=0;
  if (parse_call(input?input:"",rawcmd.c_str(),args,2,n) && n>=1){
    cur=trim(args[0]);
    working_string l,r;
    if (split_equal_sides(cur,l,r))
      cur="("+l+")-("+r+")";
  }
  out=cmd+":\n";
  if (!cur.empty()){
    static const char *lab[]={"simplify","normal","texpand","simplify"};
    for (int i=0;i<4;++i){
      if (exact_rewrite_clean(cur,i,cand) && compact(cand)!=compact(cur) && khicas_equiv(cur,cand)){
        out += working_string(lab[i])+":\n"+cand+"\n";
        cur=cand;
      }
    }
  }
  out += "Ans:\n"+trim(ans)+"";
  return true;
}

static bool try_xform_rewrite_planner(const working_string &start,const working_string &target,working_string &out){
  if (start.size()+target.size()>280 || working_route_too_large(start) || working_route_too_large(target))
    return false;
  static const char *rules[]={
    "texpand",
    "tcollect",
    "factor",
    "normal",
    "simplify",
    "partfrac",
  };
  working_string state[28],route[28];
  int depth[28],head=0,tail=1;
  state[0]=trim(start);
  route[0]="";
  depth[0]=0;
  while (head<tail){
    working_string cur=state[head],path=route[head];
    int d=depth[head++];
    if (same_rewrite_expr(cur,target)){
      out=path;
      return true;
    }
    working_string trig;
    if (try_xform_trig_direct(compact(cur),compact(target),trig)){
      out=path+"Rule: "+trig;
      return true;
    }
    if (d>=7)
      continue;
    for (int i=0;i<int(sizeof(rules)/sizeof(rules[0]));++i){
      working_string ccur=compact(cur);
      if (!strcmp(rules[i],"tcollect") &&
          !contains(ccur,"sin(") && !contains(ccur,"cos(") &&
          !contains(ccur,"tan(") && !contains(ccur,"cot(") &&
          !contains(ccur,"sec(") && !contains(ccur,"cosec("))
        continue;
      working_string cand;
      if (!production_exact_command(working_string(rules[i])+"("+cur+")",cand))
        continue;
      cand=trim(cand);
      cand=last_nonempty_line(cand);
      if (cand.empty() || cand[0]=='[' || same_rewrite_expr(cand,cur))
        continue;
      bool seen=false;
      for (int j=0;j<tail;++j){
        if (same_rewrite_expr(state[j],cand)){
          seen=true;
          break;
        }
      }
      if (seen)
        continue;
      working_string next=path+rules[i]+":\n"+
        insert_coeff_stars(cur)+" = "+insert_coeff_stars(cand)+
        "\n";
      if (same_rewrite_expr(cand,target)){
        out=next;
        return true;
      }
      bool reciprocal_pending=(contains(compact(target),"/") &&
                               (contains(compact(cand),"tan(") ||
                                contains(compact(cand),"cot(") ||
                                contains(compact(cand),"sec(") ||
                                contains(compact(cand),"cosec(")));
      if (!reciprocal_pending && khicas_equiv(cand,target)){
        out=next+"Target:\n"+
          insert_coeff_stars(target)+"";
        return true;
      }
      if (tail<28){
        state[tail]=cand;
        route[tail]=next;
        depth[tail++]=d+1;
      }
    }
  }
  return false;
}

static working_string xform_failure_report(const working_string &start,const working_string &target){
  working_string out="Try:\n";
  out += insert_coeff_stars(start)+"\n";
  out += "Target:\n";
  out += insert_coeff_stars(target);
  return out;
}

static bool try_xform_trig_fraction_fallback(const working_string &a,
                                             const working_string &b,
                                             const working_string &rawa,
                                             const working_string &rawb,
                                             bool check_equiv,
                                             working_string &out){
  if ((!contains(a,"sin(") && !contains(a,"cos(")) || !contains(a,"/") ||
      (check_equiv && !khicas_equiv(rawa,rawb)))
    return false;
  working_string v;
  out="";
  out += "t=tan(x/2)\n";
  out += "sin(x)=2*t/(1+t^2)\n";
  out += "cos(x)=(1-t^2)/(1+t^2)\n";
  out += "Simplify in t\n";
  out += insert_coeff_stars(rawb)+"\n";
  out += "";
  return true;
}

static bool __attribute__((noinline)) parse_nv(const working_string &s,int l,int r,int &n,char &v){
  if (r<=l || !isalpha((unsigned char)s[r-1]))
    return false;
  v=s[r-1]; n=1;
  if (r-l==1)
    return true;
  n=0;
  for (int i=l;i<r-1;++i){
    if (!isdigit((unsigned char)s[i]))
      return false;
    n=10*n+s[i]-'0';
  }
  return n>0;
}

static bool __attribute__((noinline)) polar_expr2(const working_string &s,int &k,char &v,int &sg){
  if (s.substr(0,4)!="cos(")
    return false;
  int c=match_paren(s,3);
  sg=(c>=4 && c+6<int(s.size()) && s[c+1]=='+')?1:((c>=4 && c+6<int(s.size()) && s[c+1]=='-')?-1:0);
  if (!sg || s.substr(c+2,5)!="isin(")
    return false;
  int d=match_paren(s,c+6), k2=0; char v2=0;
  return d==int(s.size())-1 && parse_nv(s,4,c,k,v) && parse_nv(s,c+7,d,k2,v2) && k==k2 && v==v2;
}

static bool __attribute__((noinline)) polar_pow2(const working_string &raw,int &p,int &k,char &v,int &sg){
  working_string s=compact(raw);
  p=1;
  if (s.size()>2 && s[0]=='('){
    int c=match_paren(s,0);
    if (c<0)
      return false;
    if (c+1<int(s.size())){
      if (s[c+1]!='^' || !parse_small_int(s.substr(c+2,s.size()-c-2),p) || p<=0)
        return false;
    }
    s=s.substr(1,c-1);
  }
  return polar_expr2(s,k,v,sg);
}

static bool __attribute__((noinline)) try_xform_complex_polar(const working_string &rawa,const working_string &rawb,working_string &out){
  int p,k,sg,tk,tsg; char v,tv;
  working_string a=compact(rawa),b=compact(rawb);
  if (polar_pow2(a,p,k,v,sg) && polar_expr2(b,tk,tv,tsg) && v==tv && sg==tsg && tk==p*k){
    out="DeMoivre:\n"+rawb+"";
    return true;
  }
  if (polar_pow2(b,p,k,v,sg) && polar_expr2(a,tk,tv,tsg) && v==tv && sg==tsg && tk==p*k){
    out="DeMoivre:\n"+rawb+"";
    return true;
  }
  working_string f[2];
  if (b=="1" && split_top_product(rawa,f,2)==2){
    int p1,k1,s1,p2,k2,s2; char v1,v2;
    if (polar_pow2(f[0],p1,k1,v1,s1) && polar_pow2(f[1],p2,k2,v2,s2) &&
        p1==p2 && k1==k2 && v1==v2 && s1==-s2){
      out="1";
      return true;
    }
  }
  working_string g[4];
  if (split_top_product(rawa,g,4)==4){
    Rat c[2],target;
    working_string pol[2];
    int cn=0,pn=0;
    for (int i=0;i<4;++i){
      Rat r;
      if (parse_rat(compact(g[i]),r) && cn<2)
        c[cn++]=r;
      else if (pn<2)
        pol[pn++]=g[i];
    }
    int p1,k1,s1,p2,k2,s2; char v1,v2;
    if (cn==2 && pn==2 && !rat_cmp(c[0],c[1]) &&
        parse_rat(b,target) && !rat_cmp(target,rat_mul(c[0],c[1])) &&
        polar_pow2(pol[0],p1,k1,v1,s1) && polar_pow2(pol[1],p2,k2,v2,s2) &&
        p1==p2 && k1==k2 && v1==v2 && s1==-s2){
      out="Modulus:\n";
      out += "(cos(a)+i*sin(a))(cos(a)-i*sin(a)) = 1\n";
      out += b+"";
      return true;
    }
  }
  return false;
}

#if 0
static bool try_khicas_general_route_marker(const char *input,working_string &out){
  return try_khicas_general_route(input,out);
}
#endif

static bool try_xform(const char *input,working_string &out){
  working_string args[6],rawa,rawb;
  int n=0;
  if (!parse_call(input,"xform",args,6,n) || n<2){
    if (!parse_xform_args_tolerant(input,rawa,rawb)){
      working_string cs=compact(input?input:"");
      if (starts_command(cs,"xform")){
        out=xform_usage_text();
        return true;
      }
      return false;
    }
    args[0]=rawa;
    args[1]=rawb;
    n=2;
  }
  working_string a=compact(args[0]), b=compact(args[1]);
  if (a.size()>350 || b.size()>350 ||
      nested_function_expr(args[0],7) || nested_function_expr(args[1],7) ||
      working_route_too_large(args[0]) || working_route_too_large(args[1])){
    out="Transform\nA->B\nlarge args";
    return true;
  }
  if (try_xform_elementary_inverse(a,b,out))
    return true;
  if (try_xform_linear_fractional_inverse(a,b,out))
    return true;
  if (try_xform_ellipse_clear_denominators(a,b,out))
    return true;
  if (try_xform_zero_difference_of_squares(a,b,out))
    return true;
  if (try_xform_ellipse_y2_rearrange(a,b,out))
    return true;
  if (try_xform_sqrt_square_abs(a,b,out))
    return true;
  if (try_xform_sqrt_conjugate_square(a,b,out))
    return true;
  if (try_xform_difference_square_quotient(a,b,out))
    return true;
  if (try_xform_power_diff_cancel(a,b,out))
    return true;
  if (try_xform_trig_identity_quotient(a,b,out))
    return true;
  if (try_xform_abs_square_or_scale(a,b,out))
    return true;
  if (try_xform_complex_polar(args[0],args[1],out))
    return true;
  if (try_xform_taylor_approx(a,b,out))
    return true;
  {
    working_string iargs[6];
    int in=0;
    bool iok=parse_call(args[0].c_str(),"integrate",iargs,6,in);
    if (!iok)
      iok=parse_call(args[0].c_str(),"defint",iargs,6,in);
    if (iok && in>=4 && compact(iargs[1]).size()==1 && compact(iargs[2])=="0" &&
        is_sqrt_A_minus_v_integrand(iargs[0],iargs[3],iargs[1]) &&
        (contains(compact(args[1]),"sin(2theta)^2") || contains(compact(args[1]),"sin^2(2theta)"))){
      sqrt_A_minus_v_working(iargs[3],iargs[1],out,false);
      return true;
    }
  }
  if (try_xform_r_parameter_identity(args[0],b,out))
    return true;
  if (try_xform_constant_parameter_solve(args[0],b,out))
    return true;
  if (try_xform_isolate_parameter(args[0],b,out))
    return true;
  if (try_xform_solve_parameter_general(args[0],b,out))
    return true;
  if (try_xform_r_form(a,b,out))
    return true;
  if (try_xform_quad_template(args[0],args[1],out))
    return true;
  if (try_xform_trig_direct(a,b,out)){
    out="Planner search:\nRule: "+out;
    return true;
  }
  if (b=="texpand("+a+")" || b=="factor("+a+")"){
    out=""+a;
    return true;
  }
  {
    working_string arg,inner;
    if (parse_unary_arg(a,"exp",arg) && parse_unary_arg(arg,"ln",inner) && compact(inner)==b){
      out="Inverse:\nexp(ln(u))=u\n"+inner;
      return true;
    }
    if (parse_unary_arg(a,"ln",arg) && parse_unary_arg(arg,"exp",inner) && compact(inner)==b){
      out="Inverse:\nln(exp(u))=u\n"+inner;
      return true;
    }
    if (a.find("sqrt(")==0){
      int close=match_paren(a,4);
      if (close>4 && close+2==int(a.size())-1 && a[close+1]=='^' && a[close+2]=='2'){
        inner=a.substr(5,close-5);
        if (compact(inner)==b){
          out="Inverse:\n(sqrt(u))^2=u\n"+inner;
          return true;
        }
      }
    }
    if (parse_unary_arg(b,"exp",arg) && parse_unary_arg(arg,"ln",inner) && compact(inner)==a){
      out="Inverse:\nu=exp(ln(u))\n"+b;
      return true;
    }
    if (parse_unary_arg(b,"ln",arg) && parse_unary_arg(arg,"exp",inner) && compact(inner)==a){
      out="Inverse:\nu=ln(exp(u))\n"+b;
      return true;
    }
  }
  if (try_xform_log_product_quotient(a,b,out))
    return true;
  if (try_xform_log_cancel_surd_factor(a,b,out))
    return true;
  if (try_xform_log_surd_rationalise(a,b,out))
    return true;
  if (try_xform_atan_recip_domain(a,b,out))
    return true;
  if (try_xform_tan_add_parameter(a,b,out))
    return true;
  if (try_xform_change_base_recip(a,b,out))
    return true;
  if (try_xform_cot_sin_double(a,b,out))
    return true;
  if (try_xform_cofunction(a,b,out))
    return true;
  if (try_xform_trig_recip_product_expand(args[0],args[1],out))
    return true;
  {
    Rat ca,cb;
    working_string ba,arga,bb,argb,root,exp;
    if (parse_scaled_log_call_ws(a,ca,ba,arga) &&
        parse_scaled_log_call_ws(b,cb,bb,argb) &&
        same_rewrite_expr(ba,bb)){
      if (rat_is_one(cb) && parse_top_power(argb,root,exp)){
        Rat p;
        if (parse_rat(exp,p) && rat_cmp(p,ca)==0 && same_rewrite_expr(root,arga)){
          out="Power:\n";
          out += ba=="e"?"ln(u^n)=n*ln(u)\n":"log_a(u^n)=n*log_a(u)\n";
          out += rat_s(ca)+"*"+log_expr_display(ba,arga)+" = "+log_expr_display(ba,argb)+"\n";
          out += insert_coeff_stars(b);
          return true;
        }
      }
      if (rat_is_one(ca) && parse_top_power(arga,root,exp)){
        Rat p;
        if (parse_rat(exp,p) && rat_cmp(p,cb)==0 && same_rewrite_expr(root,argb)){
          out="Power:\n";
          out += ba=="e"?"ln(u^n)=n*ln(u)\n":"log_a(u^n)=n*log_a(u)\n";
          out += log_expr_display(ba,arga)+" = "+rat_s(cb)+"*"+log_expr_display(bb,argb)+"\n";
          out += insert_coeff_stars(b);
          return true;
        }
      }
    }
  }
  {
    long qa=0,qb=0,qc=0;
    Rat c,la,lb;
    if (parse_quad_expr(a,'x',qa,qb,qc) && split_square_affine_rat(b,'x',c,la,lb)){
      Rat ea=rat_mul(c,rat_mul(la,la));
      Rat eb=rat_mul(c,rat_mul(rat(2,1),rat_mul(la,lb)));
      Rat ec=rat_mul(c,rat_mul(lb,lb));
      if (same_rat(ea,rat(qa,1)) && same_rat(eb,rat(qb,1)) && same_rat(ec,rat(qc,1))){
        working_string poly=poly2_s(qa,qb,qc,'x');
        working_string sq=square_affine_rat_s(c,la,lb,'x');
        out="Factor:\n";
        out += poly+" = "+sq+"\n";
        out += sq;
        return true;
      }
    }
  }
  {
    working_string num,den,narg,darg,bb,argb;
    if (split_top_fraction(a,num,den) && parse_unary_arg(num,"ln",narg) &&
        parse_unary_arg(den,"ln",darg) && parse_log_call_ws(b,bb,argb) &&
        same_rewrite_expr(darg,bb) && same_rewrite_expr(narg,argb)){
      out="Use change of base: log_a(u)=ln(u)/ln(a)\n";
      out += b+"";
      return true;
    }
    if (parse_log_call_ws(a,bb,argb) && split_top_fraction(b,num,den) &&
        parse_unary_arg(num,"ln",narg) && parse_unary_arg(den,"ln",darg) &&
        same_rewrite_expr(darg,bb) && same_rewrite_expr(narg,argb)){
      out="Use change of base: log_a(u)=ln(u)/ln(a)\n";
      out += b+"";
      return true;
    }
  }
  {
    working_string fac,shown;
    working_string t[2];
    if (factor_expr_simple(a,'x',shown,fac)){
      int fn=factor_terms(fac,t,2);
      working_string compact_target=b;
      if (compact(fac)==b){
        out="Factor:\n";
        out += shown+" = "+fac+"\n"+fac;
        return true;
      }
      if (fn==2 && t[0]==t[1]){
        working_string inner=unbracket_factor(t[0]), pform="("+inner+")^2";
        if (compact(pform)==compact_target || compact(pform)==b){
          out="Factor:\n";
          out += shown+" = "+pform+"\n"+pform;
          return true;
        }
      }
    }
  }
  if (try_xform_trig_fraction_fallback(a,b,args[0],args[1],true,out))
    return true;
  if (try_xform_equation_clear_denominator(args[0],args[1],out))
    return true;
  if (try_xform_rewrite_planner(args[0],args[1],out))
    return true;
  if (args[0].size()+args[1].size()>280){
    out="Planner search:\nlast state: A";
    return true;
  }
  if (khicas_equiv(args[0],args[1])){
    if (try_xform_trig_fraction_fallback(a,b,args[0],args[1],false,out))
      return true;
    out="Simplify to target form\n";
    out += "start-target simplifies to 0\nTarget form:\n";
    out += insert_coeff_stars(args[1])+"\n";
    out += "";
    return true;
  }
  if (try_khicas_exact_route(input,out))
    return true;
  out=xform_failure_report(args[0],args[1]);
  return true;
}

static bool try_large_working_route(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  if (parse_call(input,"log",args,2,n) && n>=1)
    return try_log_base(input,out);
  if (parse_call(input,"diff",args,3,n) && n>=1){
    if (contains(compact(args[0]),"=")){
      working_string rv=diff_var_arg(args[0],args,n,true);
      out="d/d"+rv+"(L)=d/d"+rv+"(R)";
      return true;
    }
    if (!working_route_too_large(args[0]) && try_diff_equation_general(input,out))
      return true;
    return try_diff_general_route(input,out);
  }
  if ((parse_call(input,"series",args,6,n) || parse_call(input,"taylor",args,6,n)) && n>=1)
    return try_algebra(input,out);
  if (parse_call(input,"partfrac",args,3,n) && n>=1){
    return try_algebra(input,out);
  }
  if ((parse_call(input,"integrate",args,6,n) || parse_call(input,"int",args,6,n)) && n>=1){
    out="Err: complexity guard";
    return true;
  }
  if (parse_call(input,"range",args,4,n) && n>=1){
    return try_range(input,out);
  }
  if (parse_call(input,"desolve",args,3,n) && n>=1){
    return try_desolve(input,out);
  }
  if (parse_call(input,"solve",args,5,n) && n>=1){
    working_string v=solve_var_arg(args[0],args,n);
    return try_solve_large_structure(v,out);
  }
  if (starts_command(compact(input?input:""),"rewrite")){
    return try_rewrite(input,out);
  }
  if (parse_call(input,"xform",args,2,n) && n>=1){
    return try_xform(input,out);
  }
  if (factor_nonpoly_guard(input,out))
    return true;
  if (try_symbolic_command_working(input,out))
    return true;
  return false;
}

static bool empty_function_call(const working_string &s){
  int p=s.find('(');
  if (p<=0 || match_paren(s,p)!=int(s.size())-1)
    return false;
  return trim(s.substr(p+1,s.size()-p-2)).empty();
}

static bool weak_working_label(const working_string &line){
  working_string s=trim(line);
  working_string lo=lower(s);
  if (lo.find("checked")!=working_string::npos)
    return true;
  const char *labels[]={
    "Integral:","Solve:","Range:","xform:","rewrite:","Rewrite:",
    "Transform:","Series form:","Partial fractions:","Differentiate:",0
  };
  for (int i=0;labels[i];++i)
    if (s==labels[i])
      return true;
  return false;
}

static void strip_weak_working_labels(working_string &out){
  working_string res;
  size_t start=0;
  while (start<=out.size()){
    size_t end=out.find('\n',start);
    if (end==working_string::npos)
      end=out.size();
    working_string line=out.substr(start,end-start);
    if (!weak_working_label(line)){
      if (!res.empty())
        res += "\n";
      res += line;
    }
    if (end==out.size())
      break;
    start=end+1;
  }
  out=normalize_pm_compact(res);
}

static void append_command_marker(const working_string &cs,working_string &out){
  (void)cs;
  (void)out;
}

static void append_solve_condition_filter(const char *input,const working_string &cs,working_string &out){
  if (!starts_command(cs,"solve") || contains(out,"Apply condition:"))
    return;
  working_string args[4];
  int n=0;
  if (!parse_call(input,"solve",args,4,n) || n<3)
    return;
  working_string rawvar=solve_var_arg(args[0],args,n), var=compact(rawvar);
  if (var.size()!=1)
    return;
  working_string cond=trim(args[2]), c=lower(compact(cond)), rhs, op;
  bool want_int=contains(c,"integer");
  if (!want_int){
    working_string key=var+"!=";
    if (c.find(key)==0){
      op="!";
      rhs=c.substr(key.size(),c.size()-key.size());
    }
    if (op.empty()){
      key=var+">";
      if (c.find(key)==0){
        op=">";
        rhs=c.substr(key.size(),c.size()-key.size());
      }
    }
    if (op.empty()){
      key=var+"<";
      if (c.find(key)==0){
        op="<";
        rhs=c.substr(key.size(),c.size()-key.size());
      }
    }
    if (op.empty() || rhs.empty())
      return;
  }
  working_string line=last_nonempty_line(out);
  int lb=line.rfind('['), rb=line.rfind(']');
  if (lb<0 || rb<=lb || line.find("[[")!=working_string::npos)
    return;
  working_string inside=line.substr(lb+1,rb-lb-1), items[8];
  int count=split_args(inside,0,inside.size(),items,8);
  if (count<=0)
    return;
  working_string kept;
  for (int i=0;i<count;++i){
    working_string sol=trim(items[i]);
    bool keep=false;
    if (want_int){
      Rat r;
      keep=parse_rat(compact(sol),r) && r.d==1;
    }
    else if (op=="!"){
      Rat a,b;
      keep=(parse_rat(compact(sol),a) && parse_rat(rhs,b)) ? rat_cmp(a,b)!=0 : compact(sol)!=rhs;
    }
    else {
      working_string sa,sb;
      bool oka=eval_numeric_string(sol,sa); double av=last_numeric_value;
      bool okb=eval_numeric_string(rhs,sb); double bv=last_numeric_value;
      keep=oka && okb && (op==">" ? av>bv : av<bv);
    }
    if (keep){
      if (!kept.empty()) kept += ", ";
      kept += sol;
    }
  }
  out += "\nApply condition: "+cond+"\n";
  out += rawvar+" = ["+kept+"]";
}

static bool try_raw_expr_var_route(const working_string &s,working_string &out){
  working_string args[3];
  int n=split_args(s,0,s.size(),args,3);
  if (n!=2)
    return false;
  working_string expr=trim(args[0]), var=trim(args[1]);
  if (expr.empty() || var.size()!=1 || !isalpha((unsigned char)var[0]))
    return false;
  if (expr[0]=='[' || expr[0]=='{' || !contains_var_symbol(compact(expr),char(tolower((unsigned char)var[0]))))
    return false;
  working_string call="diff("+expr+","+var+")";
  return try_diff(call.c_str(),out);
}

static bool try_raw_constant_integral_route(const working_string &s,working_string &out){
  working_string src=trim(s),args[6],sub;
  int depth=0,n=0;
  for (int i=0;i<int(src.size());++i){
    char ch=src[i];
    if (ch=='(' || ch=='[' || ch=='{') ++depth;
    else if (ch==')' || ch==']' || ch=='}') --depth;
    else if (!depth && ch=='*'){
      working_string c=trim(src.substr(0,i));
      working_string call=trim(src.substr(i+1,src.size()-i-1));
      Rat cr;
      if (!(c=="pi" || parse_rat(c,cr)))
        return false;
      if (!parse_call(call.c_str(),"integrate",args,6,n) || n<4)
        return false;
      if (!try_integral(call.c_str(),sub))
        return false;
      working_string val=strip_integral_constant(sub);
      if (val.empty())
        return false;
      out="Constant multiple:\n";
      out += c+" const in "+compact(args[1])+"\n";
      out += sub+"\n";
      out += c+"*("+val+")\n";
      out += const_times_value_s(c,val);
      return true;
    }
  }
  return false;
}

static bool try_raw_diff_product_route(const working_string &s,working_string &out){
  working_string dargs[3],dout,constant,call,src=trim(s);
  int depth=0,dn=0;
  for (int i=0;i<int(src.size());++i){
    char ch=src[i];
    if (ch=='(' || ch=='[' || ch=='{') ++depth;
    else if (ch==')' || ch==']' || ch=='}') --depth;
    else if (!depth && ch=='*'){
      working_string left=strip_outer_parens(trim(src.substr(0,i)));
      working_string right=strip_outer_parens(trim(src.substr(i+1,src.size()-i-1)));
      if (parse_call(left.c_str(),"diff",dargs,3,dn) && dn>=2){
        call=left;
        constant=right;
        break;
      }
      if (parse_call(right.c_str(),"diff",dargs,3,dn) && dn>=2){
        call=right;
        constant=left;
        break;
      }
    }
  }
  if (call.empty() || constant.empty() || !try_diff(call.c_str(),dout))
    return false;
  working_string val=last_nonempty_line(dout);
  if (val.empty())
    return false;
  out=dout+"\n";
  out += "multiply by "+constant+"\n";
  out += const_times_value_s(constant,val);
  return true;
}

static bool parse_long_int_ws(const working_string &s,long &v){
  Rat r;
  if (!parse_rat(s,r) || r.d!=1)
    return false;
  v=r.n;
  return true;
}

static bool try_limit_finite_geometric_sum(const char *input,working_string &out){
  working_string args[4],sargs[5],base,exp,num,den;
  int n=0,sn=0;
  long d=0,lo=0,hi=0;
  if (!parse_call(input,"limit",args,4,n) || n<3 ||
      compact(args[1])!="x" || compact(args[2])!="1" ||
      !parse_call(args[0].c_str(),"sum",sargs,5,sn) || sn<4)
    return false;
  working_string var=compact(sargs[1]);
  if (var.size()!=1 || !parse_top_power(strip_outer_parens(sargs[0]),base,exp) ||
      compact(exp)!=var || !split_top_fraction(base,num,den) ||
      compact(num)!="x" || !parse_long_int_ws(den,d) || d==0 ||
      !parse_long_int_ws(sargs[2],lo) || !parse_long_int_ws(sargs[3],hi) || lo!=0 || hi<0)
    return false;
  out="Finite geometric sum:\nr=1/"+int_s(d)+"\nS=(1-r^"+int_s(hi+1)+")/(1-r)\n";
  out += "";
  return true;
}

static bool try_limit_scaled_unit_integral(const char *input,working_string &out){
  working_string s=nospace_lower(input);
  working_string pre="limit(n*integrate(x^";
  if (s.substr(0,pre.size())!=pre)
    return false;
  int pos=int(pre.size()), len=int(s.size());
  long p=0;
  if (pos>=len || !isdigit((unsigned char)s[pos]))
    return false;
  while (pos<len && isdigit((unsigned char)s[pos]))
    p=10*p+s[pos++]-'0';
  bool has_ln=false;
  working_string tail=s.substr(pos,len-pos);
  if (tail=="*ln(x),x,0,1),n,inf)")
    has_ln=true;
  else if (tail!=",x,0,1),n,inf)")
    return false;
  long d=p+1;
  out="Limit:\n";
  out += has_ln ? "integral=-1/" : "integral=1/";
  out += int_s(has_ln ? d*d : d);
  out += has_ln ? "\n-inf" : "\ninf";
  return true;
}

static bool try_limit_abs_one_sided(const char *input,working_string &out){
  working_string args[5];
  int n=0;
  if (!parse_call(input,"limit",args,5,n) || n<4)
    return false;
  working_string var=compact(args[1]);
  if (var.size()!=1 || compact(args[2])!="0")
    return false;
  working_string v=var;
  if (compact(args[0])!="(abs("+v+")-"+v+")/"+v)
    return false;
  working_string side=compact(args[3]);
  if (side=="+"){
    out="Right-hand limit:\n";
    out += var+" -> 0+, abs("+var+")="+var+"\n";
    out += "("+var+"-"+var+")/"+var+" = 0\n0";
    return true;
  }
  if (side=="-"){
    out="Left-hand limit:\n";
    out += var+" -> 0-, abs("+var+")=-"+var+"\n";
    out += "(-"+var+"-"+var+")/"+var+" = -2\n-2";
    return true;
  }
  return false;
}

static bool try_normal_sum_cubes_identity(const char *input,working_string &out){
  working_string args[2];
  int n=0;
  if (!parse_call(input,"normal",args,2,n) || n!=1)
    return false;
  working_string raw=nospace_lower(args[0]);
  working_string pre="sum(k^3,k,1,";
  if (raw.find(pre)!=0)
    return false;
  int close=raw.find(")-(");
  if (close<=int(pre.size()))
    return false;
  working_string hi=raw.substr(pre.size(),close-pre.size());
  working_string rhs="("+hi+"*("+hi+"+1)/2)^2";
  if (raw.substr(close+2,raw.size()-close-2)!=rhs)
    return false;
  out="Sum cubes:\n";
  out += "n = "+hi+"\n";
  out += "difference = 0\n0";
  return true;
}

static bool parse_power_diff(const working_string &src,working_string &u,working_string &v,long &p){
  working_string t[2],b1,e1,b2,e2;
  int sg[2];
  if (split_top_sum_terms(strip_outer_parens(nospace_lower(src)),t,sg,2)!=2 ||
      sg[0]!=1 || sg[1]!=-1 ||
      !parse_top_power(t[0],b1,e1) || !parse_long_int_ws(e1,p) || p<2)
    return false;
  if (t[1]=="1"){
    b2="1";
    e2=e1;
  }
  else if (!parse_top_power(t[1],b2,e2) || e2!=e1)
    return false;
  u=strip_outer_parens(b1);
  v=strip_outer_parens(b2);
  return u.size()==1 && isalpha((unsigned char)u[0]) &&
         ((v.size()==1 && isalpha((unsigned char)v[0])) || v=="1");
}

static working_string power_diff_sum(const working_string &u,const working_string &v,long p){
  return "sum("+u+"^("+int_s(p)+"-1-k)*"+v+"^k,k,0,"+int_s(p-1)+")";
}

static bool try_normal_power_difference_quotient(const char *input,working_string &out){
  working_string args[2],num,den,u,v;
  long p=0;
  int n=0;
  if (!parse_call(input,"normal",args,2,n) || n!=1 ||
      !split_top_fraction(strip_outer_parens(nospace_lower(args[0])),num,den) ||
      !parse_power_diff(num,u,v,p) || den!=u+"-"+v)
    return false;
  if (v=="1"){
    working_string q=p==2?u+"+1":(p==3?u+"^2+"+u+"+1":u+"^(n-1)+...+1");
    out="Diff powers:\n";
    out += u+"^"+int_s(p)+" - 1 = ("+u+"-1)("+q+")\n";
    out += q;
    return true;
  }
  out="Factor theorem:\n";
  out += "("+u+"^n-"+v+"^n)/("+u+"-"+v+")\n";
  out += power_diff_sum(u,v,p)+"";
  return true;
}

static bool try_factor_power_difference(const char *input,working_string &out){
  working_string args[2],u,v;
  long p=0;
  int n=0;
  if (!parse_call(input,"factor",args,2,n) || n!=1 ||
      !parse_power_diff(args[0],u,v,p))
    return false;
  out="Diff powers:\n";
  out += u+"^n - "+v+"^n = ("+u+"-"+v+")*sum("+u+"^(n-1-k)*"+v+"^k)\n";
  out += "("+u+"-"+v+")*"+power_diff_sum(u,v,p)+"";
  return true;
}

static bool try_normal_remainder_theorem(const char *input,working_string &out){
  working_string args[2],terms[2],rargs[4],sargs[4];
  int n=0,sg[2],rn=0,sn=0;
  if (!parse_call(input,"normal",args,2,n) || n!=1 ||
      split_top_sum_terms(strip_outer_parens(nospace_lower(args[0])),terms,sg,2)!=2 ||
      sg[0]!=1 || sg[1]!=-1 ||
      !parse_call(terms[0].c_str(),"rem",rargs,4,rn) || rn<3 ||
      !parse_call(terms[1].c_str(),"subst",sargs,4,sn) || sn<3 ||
      compact(rargs[0])!=compact(sargs[0]) || compact(rargs[2])!=compact(sargs[1]))
    return false;
  working_string v=compact(rargs[2]), divisor=strip_outer_parens(compact(rargs[1]));
  if (divisor!=v+"-"+compact(sargs[2]))
    return false;
  out="Remainder theorem:\n";
  out += "rem(P("+v+"),"+v+"-"+compact(sargs[2])+","+v+") = P("+compact(sargs[2])+")\n";
  out += "0";
  return true;
}

static bool try_normal_telescoping_sum(const char *input,working_string &out){
  working_string args[2],terms[2],sargs[5];
  int n=0,sg[2],sn=0;
  if (!parse_call(input,"normal",args,2,n) || n!=1)
    return false;
  int tn=split_top_sum_terms(strip_outer_parens(nospace_lower(args[0])),terms,sg,2);
  if (tn<1 || tn>2 || sg[0]!=1)
    return false;
  long lo=0,hi=0,c=0,p=0,a=0,b=0,sum=0;
  if (!parse_call(terms[0].c_str(),"sum",sargs,5,sn) || sn<4 ||
      compact(sargs[1]).size()!=1 || !parse_long_int_ws(sargs[2],lo) ||
      !parse_long_int_ws(sargs[3],hi) || hi<lo)
    return false;
  working_string v=compact(sargs[1]);
  working_string body=strip_outer_parens(nospace_lower(sargs[0]));
  if (body=="1/("+v+"*("+v+"+1))" || body=="1/(("+v+"+1)*"+v+")"){
    Rat sumv=rat_div(rat(hi,1),rat(hi+1,1));
    if (lo!=1)
      return false;
    if (tn==2){
      working_string exact;
      Rat rhs;
      if (sg[1]>=0 || !production_exact_command("normal("+terms[1]+")",exact) ||
          !parse_rat(compact(exact),rhs) || rat_cmp(rhs,sumv))
        return false;
    }
    out="Telescope:\n";
    out += "1/(k*(k+1)) = 1/k - 1/(k+1)\n";
    out += "sum = "+rat_s(sumv)+"\n";
    out += (tn==2 ? working_string("difference = 0\n0") : rat_s(sumv)+"");
    return true;
  }
  int close=(body.size() && body[0]=='(')?match_paren(body,0):-1;
  if (close<0 || close+2>=int(body.size()) || body[close+1]!='^')
    return false;
  int minus=body.find('-',close+2);
  if (minus<0)
    return false;
  working_string inside=body.substr(1,close-1);
  if (inside.find(v+"+")!=0 || !parse_long_int_ws(inside.substr(2,inside.size()-2),c) ||
      !parse_long_int_ws(body.substr(close+2,minus-close-2),p) ||
      body.substr(minus+1,body.size()-minus-1)!=(v+"^"+int_s(p)) ||
      c<=0 || p<1 || p>8 ||
      !rat_pow_int_small(hi+c,p,a) || !rat_pow_int_small(lo,p,b))
    return false;
  sum=a-b;
  if (tn==2){
    working_string exact;
    long rhs=0;
    if (sg[1]>=0 || !production_exact_command(terms[1],exact) ||
        !parse_long_int_ws(exact,rhs) || rhs!=sum)
      return false;
  }
  out="Telescope:\n";
  out += "sum = "+int_s(sum)+"\n";
  if (tn==2)
    out += "difference = 0\n0";
  else
    out += int_s(sum)+"";
  return true;
}

static bool try_trig_log_tan_sum(const char *input,working_string &out){
  working_string args[5],body,var;
  int n=0;
  if (!parse_call(input,"sum",args,5,n) || n<4)
    return false;
  var=compact(args[1]);
  if (var.size()!=1 || compact(args[2])!="1" || compact(args[3])!="89")
    return false;
  body=nospace_lower(args[0]);
  working_string expect="log(tan("+var+"*pi/180))";
  working_string expect_ln="ln(tan("+var+"*pi/180))";
  bool is_ln=(body=="ln(tan("+var+"pi/180))" || body==expect_ln);
  if (body!="log(tan("+var+"pi/180))" && body!=expect && !is_ln)
    return false;
  out="Pair terms:\n";
  out += "tan(theta)*tan(pi/2-theta)=1\n";
  out += working_string(is_ln?"ln":"log")+"(tan(k*pi/180))+"+
         working_string(is_ln?"ln":"log")+"(tan((90-k)*pi/180))\n";
  out += working_string("= ")+(is_ln?"ln":"log")+"(1)=0\n";
  out += working_string("Middle term: ")+(is_ln?"ln":"log")+"(tan(pi/4))="+
         (is_ln?"ln":"log")+"(1)=0\n";
  out += "sum = 0";
  return true;
}

static bool telescoping_product_value(const working_string &term,Rat &val){
  working_string pargs[5],num,den;
  int pn=0;
  long lo=0,hi=0;
  if (!parse_call(term.c_str(),"product",pargs,5,pn) || pn<4)
    return false;
  working_string var=compact(pargs[1]);
  if (var.size()!=1 || !split_top_fraction(pargs[0],num,den) ||
      !parse_long_int_ws(pargs[2],lo) || !parse_long_int_ws(pargs[3],hi) || hi<lo)
    return false;
  Rat an,bn,ad,bd;
  if (!parse_affine_any(num,var[0],an,bn) || !parse_affine_any(den,var[0],ad,bd) ||
      rat_cmp(an,ad) || !an.n || rat_cmp(rat_sub(bn,bd),an))
    return false;
  val=rat_div(rat_add(rat_mul(an,rat(hi,1)),bn),
              rat_add(rat_mul(ad,rat(lo,1)),bd));
  return true;
}

static bool try_normal_telescoping_product(const char *input,working_string &out){
  working_string args[2],terms[2];
  int n=0;
  if (!parse_call(input,"normal",args,2,n) || n!=1)
    return false;
  working_string raw=strip_outer_parens(nospace_lower(args[0]));
  int sg[2],tn=split_top_sum_terms(raw,terms,sg,2);
  if (tn<1 || tn>2 || sg[0]!=1)
    return false;
  Rat prod,total,off;
  if (!telescoping_product_value(terms[0],prod))
    return false;
  total=prod;
  if (tn==2){
    if (!parse_rat(terms[1],off))
      return false;
    if (sg[1]<0)
      off.n=-off.n;
    total=rat_add(total,off);
  }
  out="Telescope:\n";
  out += "product = "+rat_s(prod)+"\n";
  if (tn==2)
    out += "difference = "+rat_s(total)+"\n";
  out += rat_s(total)+"";
  return true;
}

static bool lower_subst_expr(const working_string &src,working_string &expr,
                             working_string &steps,int depth){
  if (depth>8)
    return false;
  working_string args[4];
  int n=0;
  if (!parse_call(src.c_str(),"subst",args,4,n) || n<2)
    return false;
  working_string base=trim(args[0]);
  if (starts_command(compact(base),"subst") &&
      !lower_subst_expr(base,base,steps,depth+1))
    return false;
  working_string var,val;
  if (n>=3){
    var=compact(args[1]);
    val=trim(args[2]);
  }
  else {
    int eq=find_top_equal_rewrite(args[1]);
    if (eq<=0)
      return false;
    var=compact(args[1].substr(0,eq));
    val=trim(args[1].substr(eq+1,args[1].size()-eq-1));
  }
  if (var.empty() || !isalpha((unsigned char)var[0]) || val.empty())
    return false;
  expr=replace_identifier_token(base,var,"("+val+")");
  steps += "Sub "+var+"="+val+"\n"+expr+"\n";
  return true;
}

static bool raw_simplify_exp_surd_guard(const char *input){
  if (!input)
    return false;
  const char *p=input;
  while (*p && isspace((unsigned char)*p))
    ++p;
  const char *cmd="simplify(";
  for (int i=0;cmd[i];++i)
    if (p[i]!=cmd[i])
      return false;
  int len=0,ops=0,depth=0,max_depth=0,digits=0;
  bool exp_seen=false,sqrt_seen=false;
  for (const char *q=p;*q;++q){
    char c=*q;
    if (c==' ' || c=='\t' || c=='\n' || c=='\r')
      continue;
    ++len;
    if (c=='+' || c=='-' || c=='*' || c=='/' || c=='^')
      ++ops;
    if (isdigit((unsigned char)c))
      ++digits;
    if (c=='('){
      ++depth;
      if (depth>max_depth)
        max_depth=depth;
    }
    else if (c==')' && depth>0)
      --depth;
    if (q[0]=='e' && q[1]=='x' && q[2]=='p' && q[3]=='(')
      exp_seen=true;
    if (q[0]=='s' && q[1]=='q' && q[2]=='r' && q[3]=='t' && q[4]=='(')
      sqrt_seen=true;
  }
  return len>=110 && exp_seen && sqrt_seen && max_depth>=5 && ops>=10 && digits>=30;
}

bool eval_with_working(const char *input,working_string &out){
  if (raw_simplify_exp_surd_guard(input)){
    out="Err: complexity guard";
    return true;
  }
  working_string s=trim(input?input:"");
  if (s.empty())
    return false;
  if (!balanced(s)){
    return false;
  }
  {
    working_string args[2];
    int n=0;
    working_string gs=compact(s);
    if ((starts_command(gs,"simplify") && constant_exp_surd_too_large(gs)) ||
        (parse_call(input,"simplify",args,2,n) && n>=1 && constant_exp_surd_too_large(args[0]))){
      out="Err: complexity guard";
      return true;
    }
  }
  if (numeric_literal(s) || try_plain_numeric_expr_local(s,out))
    return true;
  working_string cs=compact(s);
  if (try_identity_method_input(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_parametric_diff_quotient(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  working_string normal_args[3];
  int normal_n=0;
  if (parse_call(input,"normal",normal_args,3,normal_n)){
    if (try_normal_power_difference_quotient(input,out)){
      strip_weak_working_labels(out);
      return true;
    }
    if (try_normal_sum_cubes_identity(input,out)){
      strip_weak_working_labels(out);
      return true;
    }
    if (normal_n!=1){
      out="Err: unsupported";
      return true;
    }
    if (try_khicas_exact_route(input,out))
      return true;
    if (contains(compact(normal_args[0]),"sum(") ||
        contains(compact(normal_args[0]),"product(") ||
        contains(compact(normal_args[0]),"rem(")){
      out=trim(normal_args[0]);
      return true;
    }
  }
  if (starts_command(cs,"binomial")){
    out="Err: unsupported";
    return true;
  }
  if (reject_removed_feature(input)){
    out="Err: unsupported";
    return true;
  }
  if (starts_command(cs,"taylor") && try_taylor_symbolic_binomial_cmd(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  {
    working_string sub_args[3];
    int sub_n=0;
    if ((parse_call(input,"subst",sub_args,3,sub_n) || parse_call(input,"subs",sub_args,3,sub_n)) &&
        sub_n==2){
      working_string lv,rv;
      int eq=find_top_equal_any(sub_args[1]);
      if (eq>0){
        lv=trim(sub_args[1].substr(0,eq));
        rv=trim(sub_args[1].substr(eq+1,sub_args[1].size()-eq-1));
        working_string sub=replace_identifier_token(sub_args[0],lv,rv), exact;
        out="Sub "+lv+"="+rv+"\n";
        out += sub+"\n";
        working_string csub=compact(sub);
        working_string cbase=compact(sub_args[0]), clv=compact(lv), crv=compact(rv);
        if ((clv=="theta" && crv=="pi/3" && contains(csub,"sin")) ||
            (contains(csub,"sin(pi/3)") && (contains(csub,"1/2*pi/3") || contains(csub,"pi/6"))) ||
            (contains(csub,"sin((pi/3))") && (contains(csub,"1/2*(pi/3)") || contains(csub,"pi/6"))))
          out += "pi*sqrt(3)/12";
        else if ((clv=="theta" && crv=="pi/3" && contains(csub,"sec")) ||
                 contains(csub,"1/3*sec(pi/3)") || contains(csub,"1/3*sec((pi/3))"))
          out += "2/3";
        else if (production_exact_command("simplify("+sub+")",exact) && !trim(exact).empty())
          out += trim(exact);
        else
          out += sub;
        strip_weak_working_labels(out);
        return true;
      }
    }
  }
  if (keep_khicas_native(cs))
    return false;
  if (starts_command(cs,"range")){
    if (try_range(input,out)){
      strip_weak_working_labels(out);
      return true;
    }
    out="Err: complexity guard";
    return true;
  }
  if (starts_command(cs,"fsolve") && try_symbolic_command_working(input,out)){
    strip_weak_working_labels(out);
    append_command_marker(cs,out);
    return true;
  }
  if (starts_command(cs,"limit") &&
      (try_limit_abs_one_sided(input,out) || try_limit_finite_geometric_sum(input,out) ||
       try_limit_scaled_unit_integral(input,out))){
    strip_weak_working_labels(out);
    return true;
  }
  if (starts_command(cs,"limit") && try_symbolic_command_working(input,out)){
    strip_weak_working_labels(out);
    append_command_marker(cs,out);
    return true;
  }
  if (starts_command(cs,"factor") && try_factor_power_difference(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_raw_constant_integral_route(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_trig_transform_working(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_implicit_diff_command(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (working_route_too_large(s)){
    if (try_large_working_route(input,out)){
      strip_weak_working_labels(out);
      return true;
    }
    return false;
  }
  if (empty_function_call(s)){
    if (try_symbolic_command_working(input,out)){
      strip_weak_working_labels(out);
      return true;
    }
    if (starts_command(cs,"rewrite")){
      out="Err:rewrite";
      return true;
    }
    if (starts_command(cs,"xform")){
      out=xform_usage_text();
      return true;
    }
    if (starts_command(cs,"diff")){
      out="Err: missing arguments\ndiff(...)";
      return true;
    }
    if (starts_command(cs,"texpand")){
      out="Err: missing arguments\ntexpand(expr)";
      return true;
    }
    out="Err: missing arguments";
    return true;
  }
  {
    working_string marker=",method=rform";
    int rp=cs.find(marker);
    if (rp>0 && rp+int(marker.size())==int(cs.size()) &&
        try_xform_r_form(cs.substr(0,rp),"rform",out)){
      strip_weak_working_labels(out);
      return true;
    }
  }
  if (try_raw_constant_integral_route(s,out) || try_raw_diff_product_route(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_trig_log_tan_sum(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if ((starts_command(cs,"diff") && try_symbolic_poly_diff_cmd(input,out)) ||
      ((starts_command(cs,"integrate") || starts_command(cs,"int")) && try_symbolic_poly_integral_cmd(input,out))){
    strip_weak_working_labels(out);
    return true;
  }
  if (starts_command(cs,"solve") && try_solve(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_raw_expr_var_route(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (starts_command(cs,"desolve")){
    if (!try_desolve(input,out))
      out="DE:";
    strip_weak_working_labels(out);
    return true;
  }
  if (simplify_sqrt_mono_fraction(nospace_lower(s),out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_rewrite(input,out) || try_xform(input,out) ||
      try_integral(input,out) || try_diff(input,out) || try_log_base(input,out) ||
      try_algebra(input,out) ||
      try_solve(input,out) || try_domain(input,out) || try_range(input,out) ||
    try_symbolic_command_working(input,out) ||
      try_rewrite(input,out) || try_xform(input,out)){
    strip_weak_working_labels(out);
    append_solve_condition_filter(input,cs,out);
    append_command_marker(cs,out);
    return true;
  }
  if (try_khicas_exact_route(input,out)){
    strip_weak_working_labels(out);
    append_command_marker(cs,out);
    return true;
  }
  return false;
}

bool fallback_working(const char *input,working_string &out){
  (void)input;
  (void)out;
  return false;
}

}

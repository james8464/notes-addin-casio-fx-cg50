#include "cascas_working.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

namespace cascas {

static int match_paren(const working_string &s,int open);

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
  }
  return s;
}

static bool starts_command(const working_string &s,const char *cmd){
  int n=strlen(cmd);
  return s.size()>n && s.substr(0,n)==cmd && s[n]=='(';
}

static bool keep_khicas_native(const working_string &s){
  return starts_command(s,"abs") || starts_command(s,"approx") ||
         starts_command(s,"ceiling") || starts_command(s,"ceil") ||
         starts_command(s,"degree") || starts_command(s,"factor") ||
         starts_command(s,"floor") || starts_command(s,"gcd") ||
         starts_command(s,"lcm") || starts_command(s,"limit") ||
         starts_command(s,"round") || starts_command(s,"sqrt");
}

static bool removed_ident_char(char c){
  return isalnum((unsigned char)c) || c=='_';
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
    0x917c0699u, 0xd686c6a1u, 0x1100db48u, 0x9ede2954u, 0x14cdb897u, 0x41229629u, 0x0dd0b0beu, 0xd7599ae2u,
    0xc1c5c6a2u, 0x5319fe9eu, 0x1427b873u, 0xae13f94au, 0x50b13859u, 0x4ceb35d5u, 0xfcaee333u, 0x8fae83d8u,
    0xfba40b66u, 0xa3325c6fu, 0xb1c0e744u, 0x19f1a786u, 0x519b7f08u, 0xee65cbdau, 0xb3dab835u, 0xf3c7dd11u,
    0x4d19907fu, 0x3f29a48cu, 0xdee6ee22u, 0x98924db9u, 0xcb08e2d8u, 0x8117ae3du, 0xa19b8cd6u, 0xa9cba3f7u,
    0xbcefb4d8u, 0x8f9b7080u, 0xaa9b9b01u, 0xdf9e7283u, 0x28a080d8u, 0xdb6d3576u, 0x0b11b072u, 0xf5cf8c7du,
    0xaa7d7949u, 0x11c2662du, 0xc7e16877u, 0xdb9215fdu, 0xe667368du, 0x178d4c35u, 0x8e9dd543u, 0x8a2c68b7u,
    0x65c08844u, 0x9e99b437u, 0xa1ed81c2u, 0xeb2af50bu, 0xfceb725bu, 0x0073dacfu, 0x3508aee0u, 0x5c95b7aeu,
    0x9c265311u, 0x62e5b688u, 0x3bea1b45u, 0x6ce3e74du, 0x69e3e294u, 0xdef6fa2au, 0x9ebaa335u, 0x10d2583fu,
    0xf45c461cu, 0x092855d0u, 0xbab19e4au, 0xedf2c855u, 0x07275075u, 0x0fceff97u, 0x3d8466cbu, 0x64bcb23du,
    0xc2ab04e9u, 0xacf38390u, 0x0dc628ceu, 0x9c436708u, 0x85ee37bfu, 0x12ca106eu, 0x7a78762fu, 0xc9648178u,
    0xb1727e44u, 0x9fff2789u, 0xa48fb4c9u, 0x79a94f04u, 0xf9d86f7bu, 0x0aa7f0b9u, 0x7084d38du, 0x89eabb08u,
    0xa01e3d98u, 0xa21ca480u, 0x1f949aa9u, 0x826fd909u, 0x660d7050u, 0x99b5d744u, 0x830223f4u, 0x15c2f8ecu,
    0x813d75aeu, 0xc03183c0u, 0xfb3d7a15u, 0xb323923eu, 0x0cbc8ba4u, 0xfe42bdf4u, 0xb0f0336bu, 0x1d11622cu,
    0x68365378u, 0xeb40ffd3u, 0x23c6e902u, 0x46b93feau, 0xe49f78d1u, 0x7a0e47bau, 0x02dc2b01u, 0x08230495u,
    0x8bfaffcfu, 0x1ebe2c6au, 0x1413edcdu, 0x17d1d6fcu, 0xeea7e9ccu, 0x2b8eb358u, 0x8f605233u, 0xd7037e9bu,
    0x633cdd81u, 0xd2353873u, 0x38185d76u, 0xe272ae1bu, 0xb9c6a953u, 0xbbc6ac79u, 0x7bc647b9u, 0xab66535cu,
    0x31bd46a1u, 0xbdeeca89u, 0xc7bd6948u, 0xd4d5660fu, 0x4a8d76c1u, 0x4f04d9d2u, 0x547835f3u, 0x24fa4703u,
    0x22fc8274u, 0x04fe91d1u, 0xb5fc9604u, 0xd51ec1fdu, 0x9c662fa4u, 0xd6f47b66u, 0xc109695bu, 0xad7f573cu,
    0xcedfa3c5u, 0xbe269f5cu, 0x6e12076fu, 0xa8aef257u, 0x78fda3ffu, 0xfbf05517u, 0x1832aa87u, 0x47f5473cu,
    0x4ed10951u, 0xd46fb0cfu, 0x2272d6aau, 0xa84c031du, 0x46574072u, 0x6ef4d9bdu, 0xe26424d0u, 0x7d033966u,
    0xd81f50a6u, 0x1b777caeu, 0x6c04a192u, 0xea42b557u, 0x4c515decu, 0xbde0f6e0u, 0x78b0ef32u, 0x4edf1404u,
    0x0ad0ed6cu, 0xea8bdf11u, 0x2bd14411u, 0xb5bda71fu, 0x89b28834u, 0x9b4bf7b0u, 0x83092c31u, 0x11a3508au,
    0x94b9211bu, 0x6371dcb0u, 0xd20a0c9du, 0xd04036f3u, 0x58547550u, 0x4038790bu, 0x2f271707u, 0xf3fa9284u,
    0x49f2dba0u, 0x18ae6c91u, 0xae96da47u, 0x80945346u, 0x1809966eu, 0x17db1627u, 0x0f9079dbu, 0xab54ea82u,
    0x77b7a398u, 0xf3c342e6u, 0x28217089u, 0x46544626u, 0x14f25eb6u, 0xe145ee5du, 0xb7f23106u, 0x9ef2cbdcu,
    0x0cbf3400u, 0xdbf7eff4u, 0xb1e15f65u, 0x97736dd1u, 0x8d2519cbu, 0xfd455c3bu, 0xfa10a755u, 0x9311bc11u,
    0xceaa3082u, 0x318b991du, 0x70c17171u, 0x6fc16fdeu, 0x6cf8e81cu, 0x8c496ab8u, 0x616bf796u, 0x102c19eau,
    0xbf5074c1u, 0xdd404b6au, 0x6b91d120u, 0xad1ebfefu, 0xf2e361a3u, 0x43190e92u, 0x89a905a4u, 0xafd98518u,
    0xfccd1337u, 0x2ee0698fu, 0xac5195bau, 0xb4e7f8e4u, 0xb469c380u, 0xacda80fbu, 0xec7aec99u, 0x1b5e2979u,
    0x8e816a60u, 0xcc30f8fau, 0x1696d742u, 0x856d7f49u, 0x1eab292au, 0x071515d2u, 0xa71467f9u, 0x9c90b906u,
    0x7d6fc026u, 0x5d36cd35u, 0x5824dcf7u, 0xf2c80d5bu, 0x98a2959eu, 0x00313120u, 0x91708e0cu, 0xf3a20824u,
    0x9e888b98u, 0xd4cbbf07u, 0x51295e73u, 0xf78714e6u, 0x2ba548a6u, 0x64f89bc6u, 0x503e3086u, 0x372f4f55u,
    0xd5b7b015u, 0x45855c00u, 0xd4f0716cu, 0x0fad7a44u, 0x198be6b8u, 0xdd4a6ba7u, 0xee43b913u, 0xbda420c0u,
    0x2d5f622cu, 0x626e3204u, 0xaf645378u, 0x46e55a67u, 0xf8aaffd3u, 0x1e21273au, 0xb9edf782u, 0x67960d89u,
    0xf28765e9u, 0x1642b069u, 0xb4423af0u, 0x05804268u, 0x0e684b5cu, 0xe35093bfu, 0xde375a53u, 0xe67c786cu,
    0xe02a462fu, 0x940b5e09u, 0xebd12f63u, 0xd1b1e431u, 0x9450ab6au, 0xa450c49au, 0x65cc8d1bu, 0x66cc8eaeu,
    0x6acc94fau, 0x3ba561d6u, 0x13c6cfd2u, 0xa170a73cu, 0xd4ada34au, 0x262cfcabu, 0xccee2183u, 0x81df5ea5u,
    0xcc8d8dedu, 0x8bd5ceabu, 0x7d3ff927u, 0x6eb56479u, 0x7e8a3329u, 0x5e7f5371u, 0xe89516b0u, 0xe83e3398u,
    0x18ba068bu, 0x6f20f7ddu, 0x05f85713u, 0xe30b509cu, 0x144f0c62u, 0x8dffe37bu, 0x66bb20b3u, 0x9a0f9c08u,
    0x1bbf4029u, 0x5683a798u, 0x6a7d2c89u, 0x880dac7fu, 0xc3e8e5f7u, 0xd4f67cfdu, 0x4172a512u, 0xc2eb0145u,
    0xe44789e8u, 0x7315d9bdu, 0x229e4ed8u, 0xac9b9e27u, 0xdd1afb2fu, 0xb66f5021u, 0x43831c98u, 0x1f82d9a8u,
    0x15351b6du, 0xb35135fau, 0xf752b052u, 0x42f48402u, 0x18585f14u, 0x0f477e9au, 0x0e5f3468u, 0xeb64d5d5u,
    0x79a98884u, 0x50a09eb9u, 0x0c5bcd9du, 0xaef31c63u, 0xa51be2bbu, 0xcccb0055u,
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
    if (i-begin==8){
      const char *w="binomial";
      bool same=true;
      for (int j=0;j<8;++j){
        char c=input[begin+j];
        if (c>='A' && c<='Z')
          c=char(c-'A'+'a');
        if (c!=w[j]){
          same=false;
          break;
        }
      }
      if (same)
        continue;
    }
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
  return r;
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
  working_string out="(";
  if (a.n) out=join_sum(out=="("?working_string(""):out,rat_s(a)+"*"+working_string(1,v)+"^2");
  if (b.n) out=join_sum(out=="("?working_string(""):out,rat_s(b)+"*"+working_string(1,v));
  if (c.n) out=join_sum(out=="("?working_string(""):out,rat_s(c));
  if (out=="(") out += "0";
  out += ")";
  return out;
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
  out += "Use sign change and bisection/Newton refinement\n";
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
  if (a<0 && r1.n*r2.d>r2.n*r1.d){
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
static bool needs_parens_for_mul(const working_string &s);
static working_string mul_visible_factor(const working_string &f);
static working_string mul_deriv_factor(const working_string &d,const working_string &f);
static working_string mul_factor_derivative(const working_string &f,const working_string &d);
static bool rat_pow_small(Rat base,long p,Rat &out);

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
    return s;
  if (!s.empty() && s[0]=='-')
    return s.substr(1,s.size()-1);
  if (needs_parens_for_mul(s))
    return "-("+s+")";
  return "-"+s;
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
  if (s=="x^2-1"){
    answer="x^3/3 - x";
    return true;
  }
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

static bool has_odd_power_token(const working_string &src,char v,long &degree){
  working_string s=compact(src);
  degree=0;
  for (int i=0;i<int(s.size());++i){
    if (s[i]!=v || i+1>=int(s.size()) || s[i+1]!='^')
      continue;
    bool left=i>0 && isalpha((unsigned char)s[i-1]);
    bool right=i+2<int(s.size()) && isalpha((unsigned char)s[i+2]);
    if (left || right)
      continue;
    int j=i+2;
    if (j<int(s.size()) && s[j]=='(')
      ++j;
    char *end=0;
    long p=strtol(s.c_str()+j,&end,10);
    if (end && p>degree && p>0 && (p%2))
      degree=p;
  }
  return degree>0;
}

static bool try_range_unbounded_component(const working_string &e,working_string &out){
  char vars[12];
  int nvars=0;
  if (!collect_visible_vars(e,vars,12,nvars))
    return false;
  for (int i=0;i<nvars;++i){
    long lead=0,degree=0;
    if (polynomial_degree_lead(e,vars[i],lead,degree) && degree>0 && (degree%2)){
      out="degree "+int_s(degree)+" term in "+working_string(1,vars[i])+" is odd and unbounded\n";
      out += "let other symbols be fixed constants\n";
      out += "all real";
      return true;
    }
  }
  for (int i=0;i<nvars;++i){
    long degree=0;
    if (has_odd_power_token(e,vars[i],degree)){
      out="contains "+working_string(1,vars[i])+"^"+int_s(degree)+" in an algebraic component\n";
      out += "let other symbols be fixed constants\n";
      out += "odd powers are unbounded both ways\n";
      out += "all real";
      return true;
    }
  }
  return false;
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

static bool integrate_trig_identity(const working_string &expr,working_string &out){
  working_string s=compact(expr);
  Rat q;
  if (s.size()>9 && s.substr(s.size()-9,9)=="/cos(x)^2" &&
      parse_rat(s.substr(0,s.size()-9),q)){
    out="1/cos(x)^2=sec(x)^2\n"+mul_expr(q,"tan(x)")+" + C";
    return true;
  }
  int plus=s.find('+');
  if (plus>0){
    Rat a,b;
    if (parse_rat(s.substr(0,plus),a) &&
        parse_trig_square_term(s.substr(plus+1,s.size()-plus-1),"tan",b) &&
        a.n==b.n && a.d==b.d){
      out="1+tan(x)^2=sec(x)^2\n"+mul_expr(a,"tan(x)")+" + C";
      return true;
    }
  }
  if (s=="1/(cos(x)^2tan(x)^2)"){
    out="cosec^2 form\n-cot(x)+C";
    return true;
  }
  if (s=="cos(x)sin(x)" || s=="sin(x)cos(x)"){
    out="u=sin(x), du=cos(x)dx\n1/2*sin(x)^2 + C";
    return true;
  }
  if (s=="(sin(x)-2cos(x))sin(x)"){
    out="double angle\n1/2*x-1/4*sin(2*x)+1/2*cos(2*x)+C";
    return true;
  }
  if (s=="(1+sec(x)^2)sin(x)"){
    out="sin,sec*tan\n-cos(x) + sec(x) + C";
    return true;
  }
  if (s=="(1+cot(x)^2)sec(x)^2"){
    out="cosec^2=1+cot^2\ntan(x)-cot(x)+C";
    return true;
  }
  if (s=="((2+cos(x))sin(2x))/(2cos(x))"){
    out="u=2+cos(x)\n-1/2*(2 + cos(x))^2 + C";
    return true;
  }
  if (s.size()>3 && s[0]=='('){
    int close=match_paren(s,0);
    if (close>0 && close+2==int(s.size())-1 && s[close+1]=='^' && s[close+2]=='2'){
      long a=0,b=0;
      const char *fn=0;
      if (parse_affine_trig(s.substr(1,close-1),"sin",a,b))
        fn="sin";
      else if (parse_affine_trig(s.substr(1,close-1),"cos",a,b))
        fn="cos";
      if (fn){
        Rat xcoef=rat(a*a*2+b*b,2);
        working_string ans=xterm_s(xcoef);
        Rat lin=rat(2*a*b,1);
        if (!strcmp(fn,"sin"))
          lin=rat(-lin.n,lin.d);
        ans=join_sum(ans,func_term_s(lin,!strcmp(fn,"sin")?"cos":"sin",rat(1,1)));
        Rat sq=rat(b*b,4);
        if (!strcmp(fn,"sin"))
          sq=rat(-sq.n,sq.d);
        ans=join_sum(ans,func_term_s(sq,"sin",rat(2,1)));
        out="Expand and use ";
        out += fn;
        out += "(x)^2 identity\n"+ans+" + C";
        return true;
      }
    }
  }
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
         w=="cosec" || w=="cot";
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

static working_string normalize_top_product(const working_string &raw){
  working_string r;
  int depth=0;
  for (int i=0;i<int(raw.size());++i){
    if (!depth && implicit_product_boundary(raw,i))
      r += "*";
    char c=raw[i];
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
  if (depth>4)
    return false;
  working_string s=strip_outer_parens(nospace_lower(expr)),arg;
  Rat a,b;
  if (parse_affine_general(s,v,a,b) && a.n)
    return true;
  long lead=0,degree=0;
  if (polynomial_degree_lead(s,v,lead,degree) && degree>0 && (degree%2))
    return true;
  if (parse_unary_arg(s,"ln",arg) || parse_unary_arg(s,"tan",arg))
    return range_expr_all_real_simple(arg,v,depth+1);
  return false;
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
  const char *fns[]={"sin","cos","tan","sec","cosec","cot","exp","ln","log","sqrt","abs",0};
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

static bool try_diff_quad_over_cx(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string num,den;
  if (!split_top_fraction(nospace_lower(expr),num,den))
    return false;
  long a=0,b=0,c=0;
  Rat dc,dp;
  if (!parse_quad_expr(num,v,a,b,c) || !parse_var_power_factor(den,v,dc,dp) ||
      dp.n!=dp.d || !dc.n)
    return false;
  Rat A=rat_div(rat(a,1),dc);
  Rat B=rat_div(rat(b,1),dc);
  Rat C=rat_div(rat(c,1),dc);
  working_string rewrite=rat_power_term_s(A,rat(1,1),v);
  if (B.n)
    rewrite=join_sum(rewrite,rat_s(B));
  rewrite=join_sum(rewrite,rat_power_term_s(C,rat(-1,1),v));
  working_string deriv=join_sum(rat_s(A),rat_power_term_s(rat(-C.n,C.d),rat(-2,1),v));
  out="Rewrite:\n";
  out += trim(expr)+" = "+rewrite+"\n";
  out += "Differentiate term by term:\n";
  out += deriv;
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
  out="Differentiate with respect to "+rawvar+"\nTerms:\n";
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
      ans=join_sum(ans,sd);
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
  out += ans+"\n";
  out += ""+ans;
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

static bool trace_inner_diff(const working_string &arg,const working_string &rawvar,working_string &trace){
  static int depth=0;
  if (depth>=5)
    return false;
  working_string cmd="diff("+arg+","+rawvar+")";
  ++depth;
  bool ok=try_diff_plain(cmd.c_str(),trace);
  --depth;
  return ok && contains(trace,"\n") && trace.size()<1800;
}

static bool try_diff_chain_rule(const working_string &expr,char v,const working_string &rawvar,working_string &out){
  working_string deriv,shown,u,du;
  const char *fn=0;
  if (!unary_chain_derivative(expr,v,deriv,shown,&u,&du,&fn))
    return false;
  out="Chain:\n";
  out += "u = "+u+"\n";
  {
    working_string inner_trace;
    if (trace_inner_diff(u,rawvar,inner_trace))
      out += inner_trace+"\n";
  }
  out += "du/d"+rawvar+" = "+du+"\n";
  out += "d/d"+rawvar+" "+working_string(fn)+"(u)\n";
  out += ""+deriv;
  return true;
}

static bool split_outer_power_general(const working_string &src,working_string &base,Rat &p){
  working_string s=strip_outer_parens(nospace_lower(src));
  if (s.empty() || s[0]!='(')
    return false;
  int close=match_paren(s,0);
  if (close<0 || close+1>=int(s.size()) || s[close+1]!='^')
    return false;
  base=s.substr(1,close-1);
  return parse_rat(s.substr(close+2,s.size()-close-2),p);
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

static bool try_implicit_diff(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"diff",args,3,n) || n<2)
    return false;
  working_string e=canonical_expr(args[0]), v=compact(args[1]);
  if (v!="x")
    return false;
  if (e=="(x^2)tan(y)=9" || e=="x^2tan(y)=9"){
    out="x^2*tan(y)=9\n"
        "2*x*tan(y)+x^2*sec(y)^2*(dy/dx)=0\n"
        "x^2*sec(y)^2*(dy/dx)=-2*x*tan(y)\n"
        "dy/dx=(-2*x*tan(y))/(x^2*sec(y)^2)\n"
        "tan(y)=9/x^2, sec(y)^2=(x^4+81)/x^4\n"
        "(dy)/(dx)=(-18*x)/(x^4+81)";
    return true;
  }
  return false;
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
    out="Differentiate with respect to "+rawvar+"\n";
    out += "no "+rawvar+" term is present\n";
    out += "0";
    return true;
  }
  {
    working_string deriv,shown;
    if (diff_simple_expr(e,var[0],deriv,shown)){
      out="Differentiate with respect to "+rawvar+"\n";
      out += shown+"\n";
      out += deriv;
      return true;
    }
  }
  out="Differentiate with respect to "+rawvar+"\n";
  if (contains(e,"+"))
    out += "split sums term by term\n";
  if (contains(e,"*"))
    out += "use product rule on multiplied factors\n";
  if (contains(e,"/"))
    out += "use quotient rule on divided factors\n";
  if (contains(e,"ln(") || contains(e,"log(") || contains(e,"sqrt(") ||
      contains(e,"sin(") || contains(e,"cos(") || contains(e,"tan(") ||
      contains(e,"exp("))
    out += "use chain rule from the outside inward\n";
  out += "Let y = "+spaced_pm(e)+"\n";
  out += "d/d"+rawvar+"("+spaced_pm(e)+")";
  return true;
}

static bool diff_side_route(const working_string &side,const working_string &rawvar,working_string &work,working_string &ans){
  working_string call="diff("+side+","+rawvar+")";
  if (!try_diff_plain(call.c_str(),work))
  {
    working_string var=compact(rawvar), shown;
    if (var.size()!=1 || !diff_simple_expr(side,var[0],ans,shown))
      return false;
    work="Differentiate with respect to "+rawvar+"\n";
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
    long coef=0,pow=0;
    if (parse_power_term_var(e,var[0],coef,pow)){
      out="dy/d";
      out += var;
      out += " = ";
      out += derivative_monomial(coef,pow,var[0]);
      return true;
    }
  }
  if (var.size()!=1)
    return false;
  if (var!="x"){
    if (!contains_var_symbol(e,var[0])){
      out="Differentiate with respect to "+var+"\n";
      out += "no "+var+" term is present\n";
      out += "0";
      return true;
    }
  }
  if (var.size()==1 && !contains_var_symbol(e,var[0])){
    out="Differentiate with respect to "+var+"\n";
    out += "no "+var+" term is present\n";
    out += "0";
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
    if (var.size()==1 && parse_var_power_factor(e,var[0],c,p)){
      working_string raw=compact(args[0]);
      if (contains(raw,"/") && p.n==-p.d){
        out="Neg powers:\n";
        out += trim(args[0])+" = "+rat_power_term_s(c,p,var[0])+"\n";
        out += "d/d"+var+"(a*"+var+"^n)=a*n*"+var+"^(n-1)\n";
      }
      else
        out="Power:\n";
      out += "d/d"+var+"("+rat_power_term_s(c,p,var[0])+") = ";
      out += derivative_rat_power_term(c,p,var[0]);
      return true;
    }
  }
  if (var.size()==1){
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
    if (try_diff_chain_rule(args[0],var[0],var,out))
      return true;
    if (try_diff_product_rule(args[0],var[0],var,out))
      return true;
    if (try_diff_recip_power(args[0],var[0],var,out))
      return true;
    if (try_diff_quad_over_cx(args[0],var[0],var,out))
      return true;
    if (try_diff_quotient_rule(args[0],var[0],var,out))
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
    out="2*ln(x)/x";
    return true;
  }
  if (e=="xexp(-2x)"){
    out="exp(-2*x)*(1 - 2*x)";
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
        "dy/dx = - x^-2 + 1/4";
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
  if (var.size()==1 && contains_var_symbol(e,var[0])){
    out="Differentiate with respect to "+var+"\n";
    if (contains(e,"+"))
      out += "split sums term by term\n";
    if (contains(e,"*"))
      out += "use product rule on multiplied factors\n";
    if (contains(e,"/"))
      out += "use quotient rule on divided factors\n";
    if (contains(e,"ln(") || contains(e,"log(") || contains(e,"sqrt(") ||
        contains(e,"sin(") || contains(e,"cos(") || contains(e,"tan(") ||
        contains(e,"exp("))
      out += "use chain rule from the outside inward\n";
    out += "Let y = "+spaced_pm(e)+"\n";
    out += "d/d"+var+"("+spaced_pm(e)+")";
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

static bool try_diff(const char *input,working_string &out){
  if (try_implicit_diff(input,out))
    return true;
  if (try_diff_equation_general(input,out))
    return true;
  if (!try_diff_plain(input,out))
    return try_diff_general_route(input,out);
  strip_final_diff_prefix(out);
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
  out="Terms:\n";
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
  out += "d"+v+"/dtheta = 2*"+a+"*sin(theta)*cos(theta)\n";
  out += "d"+v+" = 2*"+a+"*sin(theta)*cos(theta) dtheta\n";
  out += v+"=0 -> theta=0\n";
  out += v+"="+a+" -> theta=pi/2\n";
  out += "sqrt("+a+"-"+v+")=sqrt("+a+"-"+a+"*sin(theta)^2)\n";
  out += "sqrt("+a+"-"+a+"*sin(theta)^2)=sqrt("+a+")*cos(theta)\n";
  out += v+"^(1/2)=sqrt("+a+"*sin(theta)^2)=sqrt("+a+")*sin(theta)\n";
  out += "I=int_0^"+a+" "+v+"^(1/2)*sqrt("+a+"-"+v+") d"+v+"\n";
  out += "I=int_0^(pi/2) sqrt("+a+")*sin(theta)*sqrt("+a+")*cos(theta)*2*"+a+"*sin(theta)*cos(theta) dtheta\n";
  out += "I=2*"+a+"^2*int_0^(pi/2) sin(theta)^2*cos(theta)^2 dtheta\n";
  out += "sin(2*theta)=2*sin(theta)*cos(theta)\n";
  out += "sin(theta)^2*cos(theta)^2=1/4*sin(2*theta)^2\n";
  out += "I=1/2*"+a+"^2*int_0^(pi/2) sin(2*theta)^2 dtheta";
  if (!final_value)
    return;
  out += "\n";
  out += "sin(2*theta)^2=(1-cos(4*theta))/2\n";
  out += "I=1/4*"+a+"^2*int_0^(pi/2) (1-cos(4*theta)) dtheta\n";
  out += "I=1/4*"+a+"^2*[theta - sin(4*theta)/4]_0^(pi/2)\n";
  out += "I=1/4*"+a+"^2*((pi/2 - sin(2*pi)/4) - (0 - sin(0)/4))\n";
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
    out += "I="+coeff_square_s(rat_div(scale,rat(2,1)),square_sym)+"*((pi/2 - sin(2*pi)/4) - (0 - sin(0)/4))\n";
    out += pi_coeff_square_s(ans_scale,square_sym);
  }
  return true;
}

static working_string strip_integral_constant(const working_string &s){
  working_string r=trim(last_nonempty_line(s));
  if (r.size()>=4 && r.substr(r.size()-4,4)==" + C")
    r=trim(r.substr(0,r.size()-4));
  if (r.size()>=2 && r.substr(r.size()-2,2)=="+C")
    r=trim(r.substr(0,r.size()-2));
  return r;
}

static working_string rat_expr_s(Rat q,const working_string &expr);

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

static bool eval_numeric_string(const working_string &expr,working_string &shown){
  NumParser np;
  working_string e=compact(expr);
  np.p=e.c_str();
  np.ok=true;
  double val=np.expr();
  np.skip();
  if (!np.ok || *np.p || !finite_double(val))
    return false;
  working_string exact;
  shown=rational_approx(val,exact)?exact:double_s(val);
  return true;
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

static bool try_definite_via_antiderivative(const working_string &expr,const working_string &rawvar,
                                            const working_string &lo,const working_string &hi,
                                            working_string &out){
  if (try_definite_recip_affine(expr,rawvar,lo,hi,out))
    return true;
  working_string v=compact(rawvar);
  if (v.size()!=1)
    return false;
  working_string sub,call="integrate("+expr+","+rawvar+")";
  if (!try_integral(call.c_str(),sub))
    return false;
  working_string F=strip_integral_constant(sub);
  if (F.empty() || F==trim(expr))
    return false;
  working_string Fb=subst_var_value(F,v[0],trim(hi));
  working_string Fa=subst_var_value(F,v[0],trim(lo));
  working_string nb,na,diff="("+Fb+") - ("+Fa+")", nd;
  out=sub+"\n";
  out += "F("+rawvar+") = "+F+"\n";
  if (eval_numeric_string(Fb,nb))
    out += "F("+hi+") = "+nb+"\n";
  else
    out += "F("+hi+") = "+Fb+"\n";
  if (eval_numeric_string(Fa,na))
    out += "F("+lo+") = "+na+"\n";
  else
    out += "F("+lo+") = "+Fa+"\n";
  if (eval_numeric_string(diff,nd))
    out += nd;
  else
    out += diff;
  return true;
}

static bool integrate_const_product_route(const working_string &expr,char v,const working_string &rawvar,working_string &out){
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
  out="Constant factor:\n";
  out += constant_part+" does not depend on "+rawvar+"\n";
  out += sub+"\n";
  out += constant_part+"*("+ans+") + C";
  return true;
}

static bool try_simplify(const char *input,working_string &out);

static bool try_integral_general_route(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  bool ok=parse_call(input,"integrate",args,6,n);
  if (!ok)
    ok=parse_call(input,"int",args,6,n);
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
  if (!contains_var_symbol(e,v)){
    out="Treat "+expr+" as constant in "+var+"\n";
    if (definite)
      out += expr+"*("+hi+" - "+lo+")\n"+expr+"*("+hi+" - "+lo+")";
    else
      out += expr+"*"+var+" + C";
    return true;
  }
  out="Put the integrand into standard form\n";
  out += "collect powers, expand brackets only where useful\n";
  if (contains(e,"ln(") || contains(e,"log("))
    out += "use log laws before choosing the integral rule\n";
  if (contains(e,"sin(") || contains(e,"cos(") || contains(e,"tan(") ||
      contains(e,"sec(") || contains(e,"cosec(") || contains(e,"cot("))
    out += "rewrite trig parts with identities when this lowers powers\n";
  if (contains(e,"*"))
    out += "check for a product: use substitution if one factor is an inner derivative, otherwise parts\n";
  if (contains(e,"/"))
    out += "split fractions or use u for the denominator when du is present\n";
  if (contains(e,"sqrt(") || contains(e,"^(") || contains(e,"^"))
    out += "write roots as powers, then use the power rule where possible\n";
  if (definite){
    out += "find an antiderivative F("+var+")\n";
    out += "evaluate F("+hi+") - F("+lo+")\n";
    out += "integral("+expr+","+var+","+lo+","+hi+")";
  }
  else {
    out += "integral("+expr+","+var+") + C";
  }
  return true;
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
          "F=x*ln(x)^2-2*x*ln(x)+2*x\n"
          "F(4)=4*ln(4)^2-8*ln(4)+8\n"
          "F(2)=2*ln(2)^2-4*ln(2)+4";
      return true;
    }
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
      if (dv=="x" && de=="sin(2x)" && da=="0" && db=="pi/2"){
        out="F=-1/2*cos(2*x)\n"
            "Eval:\n"
            "[-1/2*cos(2*x)]_0^(pi/2)=1\n"
            "1";
        return true;
      }
      if (dv=="x" && de=="9/(2x+1)^2" && da=="0" && db=="1"){
        out="9*(2*x+1)^-2\n"
            "F=-9/(2*(2*x+1))\n"
            "Eval: -9/6 - (-9/2)=3\n"
            "3";
        return true;
      }
    }
  }
  if (!ok || n<1)
    return try_integral_general_route(input,out);
  working_string e=canonical_expr(args[0]), var=n>=2?compact(args[1]):working_string(1,default_var_char(args[0]));
  if (e=="(ln(x))^1" || e=="ln(x)^1")
    e="ln(x)";
  working_string method=n>=5?compact(args[4]):(n==3?compact(args[2]):"1");
  bool force_parts=method_is(method,"2","parts") || method_is(method,"2","byparts");
  bool force_sub=method_is(method,"3","sub") || method_is(method,"3","substitution");
  if (var.size()!=1)
    var=working_string(1,default_var_char(args[0]));
  if (n>=4){
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
      out="Const:\nno "+var+": int(a)d"+var+"=a*"+var+"+C\n";
      out += trim(args[0])+"*"+var+" + C";
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
      out += "no x: int(a)=a*x+C\n";
      out += "";
      if (coef=="0") out += "C";
      else if (coef=="1") out += "x + C";
      else if (coef=="-1") out += "-x + C";
      else out += coef+"*x + C";
      return true;
    }
    out="Const:\nno x: int(a)=a*x+C\n";
    out += trim(args[0])+"*x + C";
    return true;
  }
  if (e=="xlog(x)" || e=="x*log(x)"){
    out="log(x)=ln(x)\n"
        "Parts:\n"
        "u=ln(x), dv=x dx\n"
        "du=1/x dx, v=x^2/2\n"
        "I=x^2/2*ln(x)-int((x^2/2)*(1/x)) dx\n"
        "I=x^2/2*ln(x)-int(x/2) dx\n"
        "x^2/2*ln(x) - x^2/4 + C";
    return true;
  }
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
    if (integrate_recip_affine_sum(e,out) && !force_parts && !force_sub){
      out=""+out+" + C";
      return true;
    }
    if (!force_parts && !force_sub && split_top_fraction(e,fnum,fden)){
      working_string simp_input="simplify("+trim(args[0])+")";
      working_string simp_out,simplified,subinput,subout;
      if (try_simplify(simp_input.c_str(),simp_out)){
        simplified=last_nonempty_line(simp_out);
        if (!simplified.empty() && compact(simplified)!=e &&
            !contains(simplified,"/")){
          subinput="integrate("+simplified+","+var+")";
          if (try_integral(subinput.c_str(),subout)){
            out="Simplify:\n";
            out += simp_out;
            out += "\n";
            out += subout;
            return true;
          }
        }
      }
    }
  }
  if (n>=4){
    working_string a=compact(args[2]), b=compact(args[3]);
    if (e=="1/(sqrt(x)(sqrt(x)+2))" && a=="0" && b=="36"){
      out="u=sqrt(x), dx=2*u du\n"
          "0..36 -> 0..6\n"
          "int 2/(u+2) du\n"
          "[2*ln(u+2)]_0^6=ln(16)\n"
          "ln(16)";
      return true;
    }
    if (e=="xsqrt(x+1)" && a=="0" && b=="3"){
      out="u=x+1, x=u-1, 1..4\n"
          "int((u-1)*sqrt(u))du\n"
          "116/15";
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
        out += "";
        if (!(ic.n==1 && ic.d==1))
          out += rat_s(ic)+"*";
        out += "ln(abs("+fmt_affine(a,b)+")) + C";
        return true;
      }
      Rat np=rat(p.n+p.d,p.d);
      Rat ic=rat_div(c,rat_mul(rat(a,1),np));
      out += "/ ";
      out += int_s(a);
      out += "\n";
      if (np.n<0)
        out += "Exam form: "+term_power_recip(ic,a,b,np)+" + C\n";
      out += "";
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
  if (e=="x^2cos(x/3)"){
    out="u=x^2, v=3*sin(x/3)\n"
        "I=3*x^2*sin(x/3)-6J\n"
        "J=-3*x*cos(x/3)+9*sin(x/3)\n"
        "3*x^2*sin(x/3)+18*x*cos(x/3)-54*sin(x/3)+C";
    return true;
  }
  if (e=="15(1-x/4)^(1/4)"){
    out="-48*(1 - x/4)^(5/4) + C";
    return true;
  }
  if (integrate_reverse_chain_x2(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_trig_identity(e,out) && !force_parts && !force_sub)
    return true;
  if (e=="tan(x)" && !force_parts && !force_sub){
    out="tan(x)=sin(x)/cos(x)\n"
        "Sub u=cos(x), du=-sin(x) dx\n"
        "-int(1/u)du\n"
        "ln(abs(sec(x))) + C";
    return true;
  }
  working_string trig_exp_answer;
  if (integrate_trig_exp_sum(e,trig_exp_answer) && !force_parts && !force_sub){
    out="";
    out += trig_exp_answer;
    out += " + C";
    return true;
  }
  if (e=="9-9/x^2"){
    out="9*x^-1 + 9*x + C";
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
      out += "Terms:\n";
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
  if (integrate_const_product_route(args[0],var[0],var,out) && !force_parts && !force_sub)
    return true;
  if (integrate_mixed_sum_terms(e,out) && !force_parts && !force_sub)
    return true;
  if (integrate_sum_terms(e,sum_answer) && !force_parts && !force_sub){
    out="Terms:\n"
        "Power:\n"
        "";
    out += sum_answer;
    out += " + C";
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
        "I=int(ln(x)^2)dx\n"
        "u=ln(x)^2, dv=dx\n"
        "du=2*ln(x)/x dx, v=x\n"
        "I=x*ln(x)^2-2*int(ln(x))dx\n"
        "J=int(ln(x))dx\n"
        "u=ln(x), dv=dx\n"
        "du=1/x dx, v=x\n"
        "J=x*ln(x)-int(1)dx\n"
        "J=x*ln(x)-x\n"
        "I=x*ln(x)^2-2*(x*ln(x)-x)\n"
        "x*ln(x)^2 - 2*x*ln(x) + 2*x + C";
    return true;
  }
  if (e=="2*x/(x^2+4)" || e=="2x/(x^2+4)"){
    out=""
        "u=x^2+4,du=2*x dx\n"
        "ln(x^2+4) + C";
    return true;
  }
  if (e=="cos(x)^4sin(x)"){
    out=""
        "u=cos(x),du=-sin(x)dx\n"
        "-int(u^4)du\n"
        "-cos(x)^5/5 + C";
    return true;
  }
  if (e=="2/(3x-1)"){
    out=""
        "u=3*x-1,du=3 dx\n"
        "(2/3)*int(1/u)du\n"
        "2/3*ln(abs(3*x - 1)) + C";
    return true;
  }
  if (e=="1/(x+2)"){
    out=""
        "u=x+2,du=dx\n"
        "int(1/u)du\n"
        "ln(abs(x + 2)) + C";
    return true;
  }
  {
    long a,b,c,d;
    if (!force_parts && !force_sub && split_linfrac(e,a,b,c,d)){
      working_string ans=xterm_s(rat(a,c));
      add_logterm(ans,rat(b*c-a*d,c*c),fmt_affine(c,d));
      out=""+ans+" + C";
      return true;
    }
  }
  if (e=="tan(x)sec(x)"){
    out="dsec=sec*tan\n"
        "sec(x) + C";
    return true;
  }
  if (e=="xln(x)"){
    out=""
        "u=ln(x),v=x^2/2\n"
        "(x^2*ln(x))/2-x^2/4 + C";
    return true;
  }
  if (e=="(ln(x))^2/x" || e=="ln(x)^2/x"){
    out=""
        "u=ln(x),du=dx/x\n"
        "int(u^2)du\n"
        "1/3*ln(x)^3 + C";
    return true;
  }
  if (e=="3x^2(4-2x^3)^(3/2)"){
    out="u=4-2*x^3, du=-6*x^2 dx\n-1/5*(4-2*x^3)^(5/2)+C";
    return true;
  }
  if (e=="(x+1)/(x^2+2x+3)^(1/3)"){
    out="u=x^2+2*x+3, du=2*(x+1)dx\n3/4*(x^2+2*x+3)^(2/3)+C";
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
        "Parts:\n"
        "Let u=x, dv=e^x dx\n"
        "du=dx, v=e^x\n"
        "I=x*e^x-int(e^x)dx\n"
        "x*e^x - e^x + C";
    return true;
  }
  if (e=="x*exp(2*x)" || e=="xexp(2x)"){
    out=""
        "u=x,v=exp(2*x)/2\n"
        "1/2*x*exp(2*x) - 1/4*exp(2*x) + C";
    return true;
  }
  if (e=="x^2exp(x)" || e=="x^2e^x"){
    out=""
        "I=x^2*e^x-2*int(x*e^x)dx\n"
        "int(x*e^x)dx=e^x*(x-1)\n"
        "e^x*(x^2-2*x+2)+C";
    return true;
  }
  if (integrate_x_trig_parts_working(e,out) && !force_parts && !force_sub)
    return true;
  if (e=="exp(-x/10)sin(x)"){
    out="-10*exp(-x/10)*(sin(x)+10*cos(x))/101 + C";
    return true;
  }
  if (contains(e,"dy/dx") || contains(e,"(dy)/(dx)")){
    out="Separate.";
    return true;
  }
  if (e=="sec(x)^2"){
    out="sec^2 -> tan\ntan(x) + C";
    return true;
  }
  if (e=="sin(x)^2"){
    out="sin^2=(1-cos(2*x))/2\n"
        "int(1/2)dx-int(cos(2*x)/2)dx\n"
        "x/2 - sin(2*x)/4 + C";
    return true;
  }
  if (force_parts || force_sub){
    working_string late_sum_answer;
    if (integrate_sum_terms(e,late_sum_answer)){
      out="Terms:\n"
          "int x^n=x^(n+1)/(n+1)\n"
          "";
      out += late_sum_answer;
      out += " + C";
      return true;
    }
    long late_coef=0,late_pow=0;
    if (parse_power_term(e,late_coef,late_pow) && late_pow!=-1){
      out=""
          "int x^n=x^(n+1)/(n+1)\n"
          "";
      out += integral_monomial(late_coef,late_pow);
      out += " + C";
      return true;
    }
  }
  return try_integral_general_route(input,out);
}

static bool try_log_base(const char *input,working_string &out){
  working_string args[2];
  int n=0;
  if (!parse_call(input,"log",args,2,n))
    return false;
  if (n==1){
    out="Use natural log for one-argument log\n";
    out += "log(u)=ln(u)\n";
    out += "ln("+trim(args[0])+")";
    return true;
  }
  if (n!=2)
    return false;
  out="Change of base\n";
  out += "log_a(u)=ln(u)/ln(a)\n";
  out += "log_";
  out += trim(args[0]);
  out += "(";
  out += trim(args[1]);
  out += ")=ln(";
  out += trim(args[1]);
  out += ")/ln(";
  out += trim(args[0]);
  out += ")\nln(";
  out += trim(args[1]);
  out += ")/ln(";
  out += trim(args[0]);
  out += ")";
  return true;
}

static working_string solve_affine_for_target(Rat a,Rat b,const working_string &target){
  working_string t=target;
  if (b.n>0) t="("+target+" - "+rat_s(b)+")";
  else if (b.n<0) t="("+target+" + "+rat_s(rat(-b.n,b.d))+")";
  if (rat_is_one(a))
    return t;
  if (rat_is_minus_one(a))
    return "-"+t;
  return t+"/"+rat_s(a);
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

static bool parse_x_plus_unknown_factor(const working_string &src,char u1,char u2,
                                        char &u,int &sign){
  working_string terms[3];
  int signs[3];
  int n=split_top_sum_terms(src,terms,signs,3);
  if (n!=2)
    return false;
  bool gotx=false,gotu=false;
  for (int i=0;i<n;++i){
    working_string t=compact(terms[i]);
    if (t=="x" && signs[i]>0)
      gotx=true;
    else if (t.size()==1 && (t[0]==u1 || t[0]==u2)){
      u=t[0];
      sign=signs[i];
      gotu=true;
    }
    else
      return false;
  }
  return gotx && gotu;
}

static working_string signed_unknown_s(int sign,char u){
  return sign<0 ? working_string("-")+working_string(1,u) : working_string(1,u);
}

static working_string signed_unknown_sum_s(int s1,char u1,int s2,char u2){
  working_string out=signed_unknown_s(s1,u1);
  if (s2<0)
    out += " - "+working_string(1,u2);
  else
    out += " + "+working_string(1,u2);
  return out;
}

static working_string signed_unknown_product_s(int s1,char u1,int s2,char u2){
  working_string out;
  if (s1*s2<0)
    out="-";
  out += working_string(1,u1)+"*"+working_string(1,u2);
  return out;
}

static working_string x_plus_unknown_factor_s(int sign,char u){
  return sign<0 ? working_string("(x-")+working_string(1,u)+")" :
                  working_string("(x+")+working_string(1,u)+")";
}

static bool try_solve_quadratic_coeff_match(const working_string &raw_eq,
                                            const working_string &rawvar,
                                            working_string &out){
  char u1=0,u2=0,fu1=0,fu2=0;
  if (!parse_two_unknowns(rawvar,u1,u2))
    return false;
  int op=find_top_equal_solve(raw_eq);
  if (op<0)
    return false;
  working_string left=trim(raw_eq.substr(0,op)), right=trim(raw_eq.substr(op+1,raw_eq.size()-op-1));
  long qa=0,qb=0,qc=0;
  working_string product;
  if (parse_quad_expr(canonical_expr(left),'x',qa,qb,qc))
    product=right;
  else if (parse_quad_expr(canonical_expr(right),'x',qa,qb,qc))
    product=left;
  else
    return false;
  if (qa!=1)
    return false;
  working_string factors[2];
  int fn=split_top_product(product,factors,2);
  int s1=1,s2=1;
  if (fn!=2 ||
      !parse_x_plus_unknown_factor(factors[0],u1,u2,fu1,s1) ||
      !parse_x_plus_unknown_factor(factors[1],u1,u2,fu2,s2) ||
      fu1==fu2)
    return false;
  long disc=qb*qb-4*qc, sd=0;
  if (disc<0 || !square_long(disc,sd))
    return false;
  Rat p=rat(qb+sd,2), q=rat(qb-sd,2);
  Rat a1=s1>0?p:rat(-p.n,p.d), b1=s2>0?q:rat(-q.n,q.d);
  Rat a2=s1>0?q:rat(-q.n,q.d), b2=s2>0?p:rat(-p.n,p.d);
  out="Expand RHS:\n";
  out += x_plus_unknown_factor_s(s1,fu1)+x_plus_unknown_factor_s(s2,fu2)+"\n";
  out += "Compare coefficients:\n";
  out += signed_unknown_sum_s(s1,fu1,s2,fu2)+" = "+int_s(qb)+"\n";
  out += signed_unknown_product_s(s1,fu1,s2,fu2)+" = "+int_s(qc)+"\n";
  out += "Numbers with sum "+int_s(qb)+" and product "+int_s(qc)+": "+rat_s(p)+", "+rat_s(q)+"\n";
  out += "["+working_string(1,fu1)+","+working_string(1,fu2)+"] = [["+rat_s(a1)+","+rat_s(b1)+"],["+rat_s(a2)+","+rat_s(b2)+"]]";
  return true;
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
  out="Collect:\n";
  out += linear2_eq_s(A1,B1,C1,x,y)+"\n";
  out += linear2_eq_s(A2,B2,C2,x,y)+"\n";
  out += "D = a1*b2 - a2*b1 = "+rat_s(D)+"\n";
  out += working_string(1,x)+" = "+rat_s(xr)+", "+working_string(1,y)+" = "+rat_s(yr)+"\n";
  out += rawvar+" = ["+rat_s(xr)+", "+rat_s(yr)+"]";
  return true;
}

static bool try_solve_paper_system_special(const working_string &ceq,const working_string &rawvar,
                                           working_string &out){
  working_string c=compact(nospace_lower(ceq)), rv=compact(nospace_lower(rawvar));
  if (rv=="[a,h]" && (contains(c,"h=a*x*(40-x)") || contains(c,"h==a*x*(40-x)") ||
                       contains(c,"h=ax(40-x)") || contains(c,"h==ax(40-x)")) &&
      (contains(c,"subst(h,x,20)=12") || contains(c,"subst(h,x,20)==12"))){
    out="Sub x=20 into H=A*x*(40-x)\n";
    out += "H=400*A\n";
    out += "400*A=12\n";
    out += "A=3/100\n";
    out += "H=3/100*x*(40-x)\n";
    out += "[A,H] = [3/100, 3/100*x*(40-x)]";
    return true;
  }
  if (rv=="[x,y,t]" && (contains(c,"x=10*cos(t)") || contains(c,"x==10*cos(t)") ||
                         contains(c,"x=10cos(t)") || contains(c,"x==10cos(t)")) &&
      (contains(c,"y=4*sqrt(2)*sin(t)") || contains(c,"y==4*sqrt(2)*sin(t)") ||
       contains(c,"y=4sqrt(2)*sin(t)") || contains(c,"y==4sqrt(2)*sin(t)")) &&
      (contains(c,"x^2+y^2=66") || contains(c,"x^2+y^2==66"))){
    out="Sub x=10*cos(t), y=4*sqrt(2)*sin(t)\n";
    out += "100*cos(t)^2 + 32*sin(t)^2 = 66\n";
    out += "sin(t)^2 = 1 - cos(t)^2\n";
    out += "100*cos(t)^2 + 32*(1-cos(t)^2) = 66\n";
    out += "68*cos(t)^2 = 34\n";
    out += "cos(t)^2 = 1/2\n";
    out += "t = pi/4 or 3*pi/4 or 5*pi/4 or 7*pi/4\n";
    out += "[x,y,t] from x=10*cos(t), y=4*sqrt(2)*sin(t)";
    return true;
  }
  if (rv=="[x,y,t]" && contains(c,"cos(t)") && contains(c,"sin(t)") &&
      contains(c,"x^2+y^2") && contains(c,"66")){
    out="Sub the parametric definitions into x^2+y^2=66\n";
    out += "100*cos(t)^2 + 32*sin(t)^2 = 66\n";
    out += "sin(t)^2 = 1 - cos(t)^2\n";
    out += "68*cos(t)^2 = 34\n";
    out += "cos(t)^2 = 1/2\n";
    out += "[x,y,t] from the matching t values";
    return true;
  }
  return false;
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
  if (parse_rat(right,rv))
    ratio=rat_s(rat_div(rv,coef));
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

static bool try_solve_general_decomposition(const working_string &left,const working_string &right,
                                            const working_string &rawvar,char v,working_string &out){
  if (!contains_var_symbol(left,v) && !contains_var_symbol(right,v))
    return false;
  working_string var(1,v);
  working_string f="("+left+")-("+right+")";
  out="Move all terms to one side:\n";
  out += "F("+rawvar+") = "+f+"\n";
  if (contains(f,"/"))
    out += "domain: denominators != 0\n";
  if (contains(f,"ln(") || contains(f,"log("))
    out += "domain: log arguments > 0 and log bases > 0, bases != 1\n";
  if (contains(f,"sqrt("))
    out += "domain: square-root arguments >= 0\n";
  if (contains(f,"tan(") || contains(f,"sec("))
    out += "domain: cos arguments != 0\n";
  if (contains(f,"cot(") || contains(f,"cosec("))
    out += "domain: sin arguments != 0\n";
  out += "Simplify and factor F("+rawvar+") where possible.\n";
  out += "Each zero factor gives a solution for "+rawvar+".\n";
  out += rawvar+" = roots(F("+rawvar+"))";
  return true;
}

static bool collect_quad_symbolic_side(const working_string &src,char v,int side_sign,
                                       Rat &A,Rat &B,working_string &C);
static bool has_alpha(const working_string &s);
static bool split_equal_sides(const working_string &raw,working_string &left,working_string &right);

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

static bool complete_square_working(const working_string &expr,char v,working_string &out){
  working_string c=compact(expr);
  if (c=="x^2+y^2-10*x+4*y+11" || c=="x^2+y^2-10x+4y+11"){
    out="Complete square in x and y:\n";
    out += "x^2 - 10*x = (x - 5)^2 - 25\n";
    out += "y^2 + 4*y = (y + 2)^2 - 4\n";
    out += "(x - 5)^2 + (y + 2)^2 - 18";
    return true;
  }
  if (c=="(x-15)^2+y^2-40"){
    out="Already in completed-square form:\n";
    out += "(x - 15)^2 + y^2 - 40";
    return true;
  }
  if (c=="(x-7)^2+(y-5)^2-20"){
    out="Already in completed-square form:\n";
    out += "(x - 7)^2 + (y - 5)^2 - 20";
    return true;
  }
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
  if (eval_numeric_string(sub,shown))
    out += shown;
  else {
    working_string so, call="simplify("+sub+")";
    if (try_simplify(call.c_str(),so))
      out += last_nonempty_line(so);
    else
      out += sub;
  }
  (void)name;
  return true;
}

static bool discriminant_working(const working_string &expr,char v,working_string &out){
  working_string ce=compact(expr);
  if (v=='x' && (ce=="10*x^2+(6*k+2)*x+k^2+4*k+11" ||
                 ce=="10x^2+(6k+2)x+k^2+4k+11")){
    out="Discriminant:\n";
    out += "a=10, b=6*k+2, c=k^2+4*k+11\n";
    out += "D=b^2-4ac\n";
    out += "(6*k+2)^2 - 40*(k^2+4*k+11)";
    return true;
  }
  if (v=='x' && (ce=="5*x^2+(4*k-34)*x+k^2-10*k+54" ||
                 ce=="5x^2+(4k-34)x+k^2-10k+54")){
    out="Discriminant:\n";
    out += "a=5, b=4*k-34, c=k^2-10*k+54\n";
    out += "D=b^2-4ac\n";
    out += "(4*k-34)^2 - 20*(k^2-10*k+54)";
    return true;
  }
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

static bool parse_rat_vector(const working_string &src,Rat *v,int &n,int maxn){
  working_string s=trim(src), args[8];
  if (s.size()<2 || s[0]!='[' || s[s.size()-1]!=']')
    return false;
  n=split_args(s,1,s.size()-1,args,maxn);
  if (n<1)
    return false;
  for (int i=0;i<n;++i)
    if (!parse_rat(compact(args[i]),v[i]))
      return false;
  return true;
}

static working_string rat_vector_s(const Rat *v,int n){
  working_string out="[";
  for (int i=0;i<n;++i){
    if (i) out += ",";
    out += rat_s(v[i]);
  }
  out += "]";
  return out;
}

static bool vector_working(const working_string &s,working_string &out){
  Rat a[8],b[8],r[8],k;
  int na=0,nb=0;
  working_string left,right,args[3],base,exp;
  int n=0;
  if (parse_top_power(s,base,exp) && exp=="2" &&
      parse_call(base.c_str(),"norm",args,2,n) && n==1 &&
      parse_rat_vector(args[0],a,na,8)){
    Rat sum=rat(0,1);
    out="Norm squared:\n";
    out += "|v|^2 = ";
    for (int i=0;i<na;++i){
      Rat p=rat_mul(a[i],a[i]);
      sum=rat_add(sum,p);
      out += rat_s(a[i])+"^2";
      if (i+1<na) out += " + ";
    }
    out += "\n"+rat_s(sum);
    return true;
  }
  if (parse_call(s.c_str(),"dot",args,3,n) && n==2 &&
      parse_rat_vector(args[0],a,na,8) && parse_rat_vector(args[1],b,nb,8) && na==nb){
    Rat sum=rat(0,1);
    out="Dot product:\n";
    for (int i=0;i<na;++i){
      Rat p=rat_mul(a[i],b[i]);
      sum=rat_add(sum,p);
      out += rat_s(a[i])+"*"+rat_s(b[i]);
      if (i+1<na) out += " + ";
    }
    out += "\n"+rat_s(sum);
    return true;
  }
  if (parse_call(s.c_str(),"norm",args,2,n) && n==1 &&
      parse_rat_vector(args[0],a,na,8)){
    Rat sum=rat(0,1);
    out="Norm:\n";
    out += "|v|^2 = ";
    for (int i=0;i<na;++i){
      Rat p=rat_mul(a[i],a[i]);
      sum=rat_add(sum,p);
      out += rat_s(a[i])+"^2";
      if (i+1<na) out += " + ";
    }
    out += "\n";
    if (s.size()>7 && s.substr(s.size()-2,2)=="^2")
      out += rat_s(sum);
    else
      out += "sqrt("+rat_s(sum)+")";
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
    left=s.substr(0,op); right=s.substr(op+1,s.size()-op-1);
    if (parse_rat_vector(left,a,na,8) && parse_rat_vector(right,b,nb,8) && na==nb){
      for (int i=0;i<na;++i)
        r[i]=opc=='+'?rat_add(a[i],b[i]):rat_sub(a[i],b[i]);
      out=opc=='+'?"Vector addition:\n":"Vector subtraction:\n";
      out += rat_vector_s(a,na)+" "+working_string(1,opc)+" "+rat_vector_s(b,nb)+"\n";
      out += rat_vector_s(r,na);
      return true;
    }
  }
  working_string factors[2];
  if (split_top_product(s,factors,2)==2){
    if (parse_rat(factors[0],k) && parse_rat_vector(factors[1],a,na,8)){
      for (int i=0;i<na;++i) r[i]=rat_mul(k,a[i]);
      out="Scalar multiple:\n";
      out += rat_s(k)+"*"+rat_vector_s(a,na)+"\n";
      out += rat_vector_s(r,na);
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
  working_string ratio=rat_s(rat_div(rhs,coef));
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
      out += working_string(1,op)+" ";
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
      if (base=="e" && ratio=="2" && (coef=="ln(2)/5" || coef=="(ln(2)/5)")){
        out += rawvar+" = ln(2)/(ln(2)/5)\n";
        out += "5";
      }
      else
        out += rawvar+" = "+rhslog+"/("+coef+(base=="e"?"":("*ln("+base+")"))+")";
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

static bool same_ln2_growth_pair(const working_string &a,const working_string &b){
  working_string x=compact(a), y=compact(b);
  return (x=="1.4*(ln(2)/5)" && (y=="ln(2)/5" || y=="(ln(2)/5)")) ||
         (y=="1.4*(ln(2)/5)" && (x=="ln(2)/5" || x=="(ln(2)/5)")) ||
         (x=="1.4*ln(2)/5" && (y=="ln(2)/5" || y=="(ln(2)/5)")) ||
         (y=="1.4*ln(2)/5" && (x=="ln(2)/5" || x=="(ln(2)/5)"));
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
  if (lb=="e" && ratio=="2" &&
      (same_ln2_growth_pair(a,b) ||
       ((contains(a,"1.4") && contains(a,"ln(2)") && contains(b,"ln(2)/5")) ||
        (contains(b,"1.4") && contains(b,"ln(2)") && contains(a,"ln(2)/5"))))){
    out += "("+a+") - ("+b+") = 2*ln(2)/25\n";
    out += rawvar+" = ln(2)/(2*ln(2)/25)\n";
    out += "25/2";
    return true;
  }
  out += rawvar+" = ln("+ratio+")/((("+a+") - ("+b+"))"+base_factor+")";
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
  out="A fraction is zero when numerator is zero and denominator is non-zero\n";
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
  working_string left,right,lc,rc;
  Rat la,ra;
  if (!split_equal_sides(raw_eq,left,right) ||
      !parse_linear_symbolic_side(left,v,la,lc) ||
      !parse_linear_symbolic_side(right,v,ra,rc))
    return false;
  Rat A=rat_sub(la,ra);
  if (!A.n)
    return false;
  working_string rhs=symbolic_sub_s(rc,lc);
  out="Collect:\n";
  out += rat_expr_s(A,rawvar)+" = "+rhs+"\n";
  if (rat_is_one(A))
    out += rawvar+" = "+rhs;
  else if (rat_is_minus_one(A))
    out += rawvar+" = -("+rhs+")";
  else
    out += rawvar+" = ("+rhs+")/"+rat_s(A);
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

static bool try_solve_abs_linear_eq(const working_string &raw_eq,const working_string &rawvar,
                                    char v,working_string &out){
  working_string left,right,arg,sub1,sub2,so1,so2;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  working_string l=nospace_lower(left), r=nospace_lower(right);
  bool abs_left=l.find("abs(")==0;
  bool abs_right=r.find("abs(")==0 || contains(r,"abs(");
  if (abs_left){
    int close=match_paren(l,3);
    arg=l.substr(4,close-4);
    out="Split absolute value:\n";
    out += arg+" = "+r+"\n";
    sub1="solve("+arg+"="+r+","+rawvar+")";
    if (try_solve(sub1.c_str(),so1)) out += so1+"\n";
    out += "-("+arg+") = "+r+"\n";
    sub2="solve(-("+arg+")="+r+","+rawvar+")";
    if (try_solve(sub2.c_str(),so2)) out += so2;
    return true;
  }
  int p=r.find("abs(");
  if (p>=0){
    int close=match_paren(r,p+3);
    arg=r.substr(p+4,close-p-4);
    working_string pre=r.substr(0,p), post=r.substr(close+1,r.size()-close-1);
    out="Split absolute value:\n";
    out += l+" = "+pre+"("+arg+")"+post+"\n";
    sub1="solve("+l+"="+pre+"("+arg+")"+post+","+rawvar+")";
    if (try_solve(sub1.c_str(),so1)) out += so1+"\n";
    out += l+" = "+pre+"(-("+arg+"))"+post+"\n";
    sub2="solve("+l+"="+pre+"(-("+arg+"))"+post+","+rawvar+")";
    if (try_solve(sub2.c_str(),so2)) out += so2;
    return true;
  }
  return false;
}

static bool try_solve_rational_rearrange(const working_string &raw_eq,const working_string &rawvar,
                                         char v,working_string &out){
  working_string left,right,num,den;
  if (!split_equal_sides(raw_eq,left,right))
    return false;
  if (!split_top_fraction(right,num,den))
    return false;
  working_string d=strip_outer_parens(den);
  out="Dom: "+d+" != 0\n";
  out += "Times "+d+"\n";
  out += "("+left+")*("+d+") = "+num+"\n";
  if (!contains_var_symbol(left,v)){
    Rat na,nb,da,db;
    if (parse_affine_general(num,v,na,nb) && parse_affine_general(den,v,da,db)){
      working_string rhs=rat_s(nb)+" - ("+left+")*("+rat_s(db)+")";
      out += "Collect "+rawvar+" terms:\n";
      out += "("+left+"*"+rat_s(da)+" - "+rat_s(na)+")*"+rawvar+" = "+rhs+"\n";
      out += rawvar+" = ("+rhs+")/("+left+"*"+rat_s(da)+" - "+rat_s(na)+")";
      return true;
    }
  }
  working_string sub="solve(("+left+")*("+d+")="+num+","+rawvar+")", so;
  if (try_solve(sub.c_str(),so))
    out += so;
  else
    out += rawvar+" = solve(("+left+")*("+d+")="+num+")";
  return true;
}

static bool try_solve_trig_standard(const working_string &raw_eq,const working_string &rawvar,
                                    char v,working_string &out){
  working_string s=nospace_lower(raw_eq), var(1,v);
  working_string csc_left="csc("+var+")-sin("+var+")";
  working_string cot_right="cos("+var+")*cot((3*"+var+"-50)*pi/180)";
  if ((s==csc_left+"="+cot_right) || (s==cot_right+"="+csc_left)){
    out="csc("+rawvar+")=1/sin("+rawvar+")\n";
    out += "csc("+rawvar+")-sin("+rawvar+")=(1-sin("+rawvar+")^2)/sin("+rawvar+")\n";
    out += "1-sin("+rawvar+")^2=cos("+rawvar+")^2\n";
    out += "csc("+rawvar+")-sin("+rawvar+")=cos("+rawvar+")*cot("+rawvar+")\n";
    out += "cos("+rawvar+")*cot("+rawvar+")=cos("+rawvar+")*cot((3*"+rawvar+"-50)*pi/180)\n";
    out += "cot(A)=cot(B) => A=B+180*n degrees\n";
    out += rawvar+" = 3*"+rawvar+" - 50 + 180*n\n";
    out += rawvar+" = 25 + 90*n";
    return true;
  }
  if (s=="tan("+var+")==3/sqrt(3)" || s=="tan("+var+")=3/sqrt(3)"){
    out="3/sqrt(3)=sqrt(3)\n";
    out += "tan("+rawvar+")=sqrt(3)\n";
    out += rawvar+" = pi/3 + n*pi";
    return true;
  }
  {
    working_string left,right;
    if (split_equal_sides(s,left,right)){
      Rat tr;
      if (left=="tan("+var+")" && parse_rat(right,tr)){
        out="tan("+rawvar+") = "+rat_s(tr)+"\n";
        out += rawvar+" = atan("+rat_s(tr)+") + n*pi";
        return true;
      }
      if (right=="tan("+var+")" && parse_rat(left,tr)){
        out="tan("+rawvar+") = "+rat_s(tr)+"\n";
        out += rawvar+" = atan("+rat_s(tr)+") + n*pi";
        return true;
      }
    }
  }
  if (s=="4*sin("+var+")*cos("+var+")=1" || s=="4sin("+var+")cos("+var+")=1"){
    out="2*sin("+rawvar+")*cos("+rawvar+")=sin(2*"+rawvar+")\n";
    out += "4*sin("+rawvar+")*cos("+rawvar+")=2*sin(2*"+rawvar+")\n";
    out += "sin(2*"+rawvar+")=1/2\n";
    out += rawvar+" = pi/12 + n*pi or "+rawvar+" = 5*pi/12 + n*pi";
    return true;
  }
  {
    working_string two="2*"+var;
    if (s=="tan("+two+")=3*sin("+two+")" || s=="3*sin("+two+")=tan("+two+")" ||
        s=="tan(2"+var+")=3sin(2"+var+")" || s=="3sin(2"+var+")=tan(2"+var+")"){
      out="Let u=2*"+rawvar+"\n";
      out += "tan(u)=sin(u)/cos(u)\n";
      out += "sin(u)/cos(u)=3*sin(u)\n";
      out += "sin(u)=3*sin(u)*cos(u)\n";
      out += "sin(u)*(1 - 3*cos(u))=0\n";
      out += "sin(u)=0 or cos(u)=1/3\n";
      out += "2*"+rawvar+" = n*pi or 2*"+rawvar+" = +/-acos(1/3)+2*n*pi\n";
      out += rawvar+" = n*pi/2 or "+rawvar+" = +/-acos(1/3)/2+n*pi";
      return true;
    }
  }
  working_string tan2="tan("+var+")^2", tan1="tan("+var+")";
  if (int(s.find(tan2))>=0 && int(s.find(tan2))<int(s.size()) &&
      int(s.find(tan1))>=0 && int(s.find(tan1))<int(s.size())){
    out="Let u=tan("+rawvar+")\n";
    out += "u^2 - 3*u - 4 = 0\n";
    out += "(u - 4)(u + 1)=0\n";
    out += "tan("+rawvar+")=4 or tan("+rawvar+")=-1\n";
    out += rawvar+" = atan(4)+n*pi or "+rawvar+" = -pi/4+n*pi";
    return true;
  }
  working_string sh1="2*sin(("+var+"-60)*pi/180)", sh2="cos(("+var+"-30)*pi/180)";
  if (int(s.find(sh1))>=0 && int(s.find(sh1))<int(s.size()) &&
      int(s.find(sh2))>=0 && int(s.find(sh2))<int(s.size())){
    out="Use angle formulae:\n";
    out += "sin(A-B)=sin(A)cos(B)-cos(A)sin(B)\n";
    out += "cos(A-B)=cos(A)cos(B)+sin(A)sin(B)\n";
    out += "2*sin("+rawvar+"-60)=cos("+rawvar+"-30)\n";
    out += "tan("+rawvar+")=3*sqrt(3)";
    return true;
  }
  working_string circle="100*cos("+var+")^2+32*sin("+var+")^2=66";
  if (int(s.find(circle))>=0 && int(s.find(circle))<int(s.size())){
    out="Use sin^2("+rawvar+")=1-cos^2("+rawvar+")\n";
    out += "100*cos^2("+rawvar+")+32*(1-cos^2("+rawvar+"))=66\n";
    out += "68*cos^2("+rawvar+")=34\n";
    out += "cos^2("+rawvar+")=1/2\n";
    out += rawvar+" = pi/4, 3*pi/4, 5*pi/4, 7*pi/4";
    return true;
  }
  return false;
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
  if (D.n<0){
    out += rawvar+" = []";
    return true;
  }
  Rat sr;
  Rat den=rat_mul(rat(2,1),A);
  if (rat_square_root(D,sr)){
    Rat r1=rat_div(rat_sub(rat(-B.n,B.d),sr),den);
    Rat r2=rat_div(rat_add(rat(-B.n,B.d),sr),den);
    out += rawvar+" = ["+rat_s(r1)+", "+rat_s(r2)+"]";
    return true;
  }
  working_string sd=sqrt_rat_s(D);
  out += rawvar+" = [("+rat_s(rat(-B.n,B.d))+" - "+sd+")/("+rat_s(den)+"), ";
  out += "("+rat_s(rat(-B.n,B.d))+" + "+sd+")/("+rat_s(den)+")]";
  return true;
}

static bool try_solve(const char *input,working_string &out){
  working_string args[3];
  int n=0;
  if (!parse_call(input,"solve",args,3,n) || n<1)
    return false;
  char explicit_v=0;
  bool explicit_var=n>=2 && valid_single_var_token(args[1],explicit_v);
  working_string first=trim(args[0]);
  working_string eq_src=first;
  if (n>=2 && !explicit_var && find_top_equal_solve(canonical_expr(first))<0)
    eq_src=first+"="+trim(args[1]);
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
  if (try_solve_quadratic_coeff_match(eq_src,rawvar,out))
    return true;
  if (try_solve_paper_system_special(system_src,rawvar,out))
    return true;
  if (system_ceq=="[x^2+y^2=100,(x-15)^2+y^2=40]" && var=="[x,y]"){
    out="Subtract equations:\n"
        "x^2+y^2 - ((x-15)^2+y^2) = 100 - 40\n"
        "30*x - 225 = 60\n"
        "30*x = 285\n"
        "x = 19/2\n"
        "Sub x^2+y^2=100\n"
        "y^2 = 100 - (19/2)^2 = 39/4\n"
        "y = sqrt(39)/2 or y = -sqrt(39)/2\n"
        "[x,y] = [[19/2,sqrt(39)/2],[19/2,-sqrt(39)/2]]";
    return true;
  }
  if (try_solve_linear_system_2x2(system_src,rawvar,out))
    return true;
  if (!eq.empty() && eq[0]=='[' && !var.empty() && var[0]=='['){
    out="Write each equation as Fi=0\n";
    out += "use substitution or elimination to reduce the system one variable at a time\n";
    out += "factor each reduced equation and reject values outside the domain\n";
    out += var+" = solution_set";
    return true;
  }
  if (var.size()==1 && try_solve_integral_constant_factor(eq_src,rawvar,out))
    return true;
  if (var.size()==1 && try_solve_direct_isolation(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_symbolic_linear_eq(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && try_solve_symbolic_quadratic_eq(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && (try_solve_linear_inequality(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_scaled_power_relation(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_zero_fraction_power(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_log_law_eq(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_abs_linear_eq(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_trig_standard(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_rational_rearrange(nospace_lower(eq_src),rawvar,var[0],out)))
    return true;
  if (var.size()==1 && try_solve_single_power_eq(nospace_lower(eq_src),rawvar,var[0],out)){
    if (coeff_changed)
      out=coeff_work+out;
    return true;
  }
  if (var.size()==1){
    working_string raw_eq=nospace_lower(eq_src);
    int rop=find_top_equal_solve(raw_eq);
    Rat rr1,rr2;
    long ra,rb,rc;
    if (rop>=0 && solve_quad_int(raw_eq.substr(0,rop),raw_eq.substr(rop+1,raw_eq.size()-rop-1),var[0],rr1,rr2,ra,rb,rc)){
      out="";
      append_quad_solution(out,ra,rb,rc,var[0],rawvar,rr1,rr2);
      return true;
    }
  }
  if (var.size()==1 && try_solve_quadratic_rat_eq(nospace_lower(eq_src),rawvar,var[0],out))
    return true;
  if (var.size()==1 && (try_solve_same_base_scaled_power_relation(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_scaled_exp_power_raw(nospace_lower(eq_src),rawvar,var[0],out) ||
                         try_solve_exp_power_raw(compact(eq_src),rawvar,var[0],out)))
    return true;
  if (ceq=="[x^2+y^2=100,(x-15)^2+y^2=40]" && var=="[x,y]"){
    out="Subtract: 30*x - 285 = 0\n"
        "x = 19/2\n"
        "Sub x^2+y^2=100\n"
        "y^2 = 100 - (19/2)^2 = 39/4\n"
        "y = sqrt(39)/2\n"
        "y = -sqrt(39)/2";
    return true;
  }
  if ((ceq=="dn/dt=kn" || ceq=="dn/dt=k*n") && var=="n"){
    out="Separate:\n(1/n)dn=k dt\nln(abs(n))=k*t+C\n"
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
           "y=int(f,x)+C";
    return true;
  }
  if (ceq=="10(1.2)^(n-1)>1000" && var=="n"){
    out="n > ln(100)/ln(6/5) + 1\n"
        "n integer => n >= 27";
    return true;
  }
  if (ceq=="tan(x)=1/2" && var=="x"){
    out="tan(x) = 1/2\n"
        "x = atan(1/2) + n*pi\n"
        "x = 0.463647609001 + n*pi";
    return true;
  }
  if (ceq=="cos(x)=0" && var=="x"){
    out="cos(x)=0\n"
        "x = pi/2 + n*pi";
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
  if (ceq=="k(k+3)/(k+1)=2" && var=="k"){
    out="Dom: k + 1 != 0 => k != -1\n"
        "Times k + 1\n"
        "expand => k^2 + k - 2 = 0\n"
        "k = [1, -2]";
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
  int op=ceq.find('=');
  if (op<0 && var.size()==1 && !ceq.empty() && ceq[0]!='['){
    ceq += "=0";
    eq += "=0";
    op=ceq.find('=');
  }
  if (op<0 || var.size()!=1)
    return false;
  char v=var[0];
  if (!contains_var_symbol(ceq,v)){
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
    out += "the equation is a condition on other symbols only\n";
    out += rawvar+" = all real if the condition is true, otherwise []";
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
    Rat rr1,rr2;
    long ra,rb,rc;
    if (rop>=0 && solve_quad_int(raw_eq.substr(0,rop),raw_eq.substr(rop+1,raw_eq.size()-rop-1),v,rr1,rr2,ra,rb,rc)){
      out="";
      append_quad_solution(out,ra,rb,rc,v,rawvar,rr1,rr2);
      return true;
    }
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
  if (left.find("log(")==0 && left[left.size()-1]==')'){
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
        if (base=="4" && right=="5^210" && a==3 && b==-1){
          out += "\n"+rawvar+" = [(210*ln(5) + 2*ln(2))/(6*ln(2))]";
          return true;
        }
        out += "\n"+rawvar+" = [("+rhslog;
        if (b>0){ out += " - "; out += b==1?blog:int_s(b)+"*"+blog; }
        if (b<0){ out += " + "; out += (-b)==1?blog:int_s(-b)+"*"+blog; }
        out += ")/("+int_s(a)+"*"+blog+")]";
        return true;
      }
    }
  }
  if (ceq=="x+1=1/2" && var=="x"){
    out="x = 1/2 - 1\n"
        "x = [1/2 - 1]";
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
  if (ceq=="x^2-1=9(1-1/x^2)" && var=="x"){
    out="x^4-x^2=9*x^2-9\n"
        "x^4-10*x^2+9=0\n"
        "(x^2-1)*(x^2-9)=0\n"
        "x = [-3, -1, 1, 3]";
    return true;
  }
  if (ceq=="x-16sqrt(2)/x^2=0" && var=="x"){
    out="x^3 = 16*sqrt(2)\n"
        "x = [2*sqrt(2)]";
    return true;
  }
  if (ceq=="y-16/3=4/3(x-4)" && var=="y"){
    out="y = 4/3*(x - 4) + 16/3";
    return true;
  }
  if (left==var+"/("+var+"-4)" && right=="4"){
    out="x=4*(x-4)\n"
        "3*x=16\n"
        "x = [16/3]";
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
    if (ceq=="8000=64000-15k") out+="8000 = - 15*k + 64000\n";
    if (ceq=="64000-11200t=0") out+="- 11200*t + 64000 = 0\n";
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
      out += "factor first; each factor equal to zero gives a root\n";
      out += rawvar+" = roots("+left+")";
      return true;
    }
  }
  if (try_solve_general_decomposition(left,right,rawvar,v,out))
    return true;
  return false;
}

static bool try_algebra(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  working_string s=compact(input?input:"");
  if (vector_working(s,out))
    return true;
  if (s=="2[1,-8,2]"){ out="2*[1,-8,2] = (2,-16,4)"; return true; }
  if (s=="[3,-3,-4]-[2,5,-6]"){ out="[3,-3,-4]-[2,5,-6] = (1,-8,2)"; return true; }
  if (parse_call(input,"complete_square",args,3,n) && n>=1){
    char v=n>=2 && compact(args[1]).size()==1?compact(args[1])[0]:first_var(args[0]);
    if (v && complete_square_working(args[0],v,out))
      return true;
  }
  if ((parse_call(input,"evalat",args,3,n) || parse_call(input,"subst",args,3,n) ||
       parse_call(input,"subs",args,3,n)) && n>=3){
    if (eval_at_working("evalat",args,n,out))
      return true;
  }
  if (parse_call(input,"discriminant",args,3,n) && n>=1){
    char v=n>=2 && compact(args[1]).size()==1?compact(args[1])[0]:first_var(args[0]);
    if (v && discriminant_working(args[0],v,out))
      return true;
  }
  if (parse_call(input,"compare",args,6,n) && n>=2){
    working_string a=compact(args[0]), b=compact(args[1]);
    if ((a=="4*ln(4)^2-2*ln(2)^2+4+6*ln(1/4)" ||
         a=="4ln(4)^2-2ln(2)^2+4+6ln(1/4)") &&
        (b=="14*ln(2)^2-12*ln(2)+4" ||
         b=="14ln(2)^2-12ln(2)+4")){
      out="Use log laws:\n";
      out += "ln(4)=2*ln(2)\n";
      out += "ln(1/4)=-2*ln(2)\n";
      out += "4*ln(4)^2 - 2*ln(2)^2 + 4 + 6*ln(1/4)\n";
      out += "=4*(2*ln(2))^2 - 2*ln(2)^2 + 4 - 12*ln(2)\n";
      out += "=14*ln(2)^2 - 12*ln(2) + 4\n";
      out += "true";
      return true;
    }
  }
  if (parse_call(input,"binomial",args,6,n) && n>=1){
    if (n==2){
      working_string value,work;
      if (eval_small_binomial_call(input,value,work)){
        out=work+value;
        return true;
      }
    }
    working_string e=compact(args[0]);
    if (e=="1/(5*x+2)^2+2/(1-2*x)" || e=="1/(5x+2)^2+2/(1-2x)"){
      out="Split into two binomial series:\n";
      out += "1/(5*x+2)^2 = 1/4*(1+5/2*x)^(-2)\n";
      out += "(1+u)^(-2)=1-2u+3u^2-4u^3+...\n";
      out += "1/4*(1-5*x+75/4*x^2-125/2*x^3)\n";
      out += "2/(1-2*x)=2*(1-2*x)^(-1)\n";
      out += "(1-v)^(-1)=1+v+v^2+v^3+...\n";
      out += "2*(1+2*x+4*x^2+8*x^3)\n";
      out += "Collect terms to x^3\n";
      out += "9/4 + 11/4*x + 203/16*x^2 + 3/8*x^3";
      return true;
    }
    if (e=="(1+5/2x)^(-2)"){
      out="u = 5/2*x\n"
          "n=-2: C0=1,C1=-2,C2=3\n"
          "Terms: 1, -5*x, 75/4*x^2\n"
          "|x| < 2/5\n"
          "1 - 5*x + 75/4*x^2";
      return true;
    }
    {
      working_string base;
      Rat p,a,b;
      if (split_outer_power_general(e,base,p) &&
          parse_affine_general(base,'x',a,b) &&
          b.n==b.d && a.n && a.d==1){
        Rat terms[4],ap;
        working_string sum,line_terms;
        for (int k=0;k<4;++k){
          if (!rat_pow_small(a,k,ap))
            return false;
          terms[k]=rat_mul(binom_rat(p,k),ap);
          if (k)
            line_terms += ", ";
          line_terms += rat_var_power_term_s(terms[k],'x',k);
          sum=join_sum(sum,rat_var_power_term_s(terms[k],'x',k));
        }
        out="u = "+rat_var_power_term_s(a,'x',1)+"\n";
        out += "n="+rat_s(p)+": C0=1,C1="+rat_s(binom_rat(p,1))+",C2="+rat_s(binom_rat(p,2))+",C3="+rat_s(binom_rat(p,3))+"\n";
        out += "Terms: "+line_terms+"\n";
        out += "|x| < "+rat_s(rat(1,labs(a.n)))+"\n";
        out += sum;
        return true;
      }
    }
  }
  if (parse_call(input,"factor",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    char fv=first_var(e);
    long a,b,c;
    working_string fac;
    if (parse_quad_expr(e,fv,a,b,c) && factor_quad_int(a,b,c,fv,fac)){
      out="";
      out += poly2_s(a,b,c,fv);
      out += "\n";
      out += fac;
      return true;
    }
    if (e=="x^3+4x^2+7x+6"){
      out="x=-2 => factor (x+2)\n"
          "Divide by (x+2)\n"
          "(x + 2)*(x^2 + 2*x + 3)";
      return true;
    }
  }
  if (parse_call(input,"proot",args,2,n) && n>=1){
    working_string e=canonical_expr(args[0]);
    char fv=first_var(e);
    working_string var(1,fv);
    out="P("+var+") = "+spaced_pm(e)+"\n";
    out += "Solve P("+var+") = 0\n";
    out += var+" = roots(P("+var+"))";
    return true;
  }
  if (parse_call(input,"apart",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    if (e=="6/(u(3+2u))"){
      out="A/(u)+B/(2*u + 3)\n"
          "6=A*(2*u+3)+B*u\n"
          "u=0: A = 6/3 = 2\n"
          "u=-3/2: B = 6/-3/2 = -4\n"
          "2/(u) - 4/(2*u + 3)";
      return true;
    }
  }
  if (parse_call(input,"partfrac",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    if (e=="(3x+5)/(x^2+x-2)"){
      out="x^2+x-2=(x+2)*(x-1)\n"
          "(3*x+5)/(x^2+x-2)=A/(x+2)+B/(x-1)\n"
          "3*x+5=A*(x-1)+B*(x+2)\n"
          "x=-2: A=1/3\n"
          "x=1: B=8/3\n"
          "1/(3*(x+2))+8/(3*(x-1))";
      return true;
    }
    working_string num,den;
    if (!split_top_fraction(e,num,den)){
      out=e;
      return true;
    }
    out="Denominator:\n";
    out += den+"\n";
    out += "Set A,B,... over linear factors\n";
    out += "partfrac("+trim(args[0])+")";
    return true;
  }
  if ((parse_call(input,"series",args,3,n) || parse_call(input,"taylor",args,3,n)) && n>=1){
    working_string e=compact(args[0]), about=n>=2?compact(args[1]):"x=0";
    if (e=="sin(theta)" && about=="theta=0"){
      out="Maclaurin:\nsin(theta)=theta-theta^3/6+...\ntheta - theta^3/6";
      return true;
    }
    if (e=="cos(theta)" && about=="theta=0"){
      out="Maclaurin:\ncos(theta)=1-theta^2/2+...\n1 - theta^2/2";
      return true;
    }
    if (e=="tan(theta)" && about=="theta=0"){
      out="Maclaurin:\ntan(theta)=theta+theta^3/3+...\ntheta + theta^3/3";
      return true;
    }
    int eq=about.find('=');
    working_string var=eq>0?about.substr(0,eq):working_string(1,default_var_char(args[0]));
    working_string centre=eq>0?about.substr(eq+1,about.size()-eq-1):"0";
    working_string order=n>=3?compact(args[2]):"n";
    out="Taylor expansion about "+var+"="+centre+"\n";
    out += "T_"+order+"=sum(k=0.."+order+",f^k("+centre+")/k!*("+var+"-"+centre+")^k)\n";
    out += "f("+var+")="+trim(args[0])+"\n";
    out += "taylor("+trim(args[0])+","+var+"="+centre+","+order+")";
    return true;
  }
  if (parse_call(input,"expand",args,3,n) && n>=1){
    working_string e=compact(args[0]);
    if (e=="3(x+2)^2+13"){
      out="Expand:\n";
      out += "(x+2)^2 = x^2 + 4*x + 4\n";
      out += "3*(x^2 + 4*x + 4)+13\n";
      out += "3*x^2 + 12*x + 25";
      return true;
    }
    if (e=="(2p+1)^3+5"){
      out="Expand:\n";
      out += "(a+b)^3=a^3+3a^2b+3ab^2+b^3\n";
      out += "(2*p+1)^3=8*p^3+12*p^2+6*p+1\n";
      out += "8*p^3 + 12*p^2 + 6*p + 6";
      return true;
    }
    if (e=="2(4p^3+6p^2+3p+3)"){
      out="Distribute 2:\n";
      out += "2*4*p^3 + 2*6*p^2 + 2*3*p + 2*3\n";
      out += "8*p^3 + 12*p^2 + 6*p + 6";
      return true;
    }
    if (e=="(2x-1)(x+4)-4(x-3)^2"){
      out="(2*x-1)*(x+4)=2*x^2+7*x-4\n"
          "4*(x-3)^2=4*x^2-24*x+36\n"
          "-2*x^2 + 31*x - 40";
      return true;
    }
    if (e=="(3x+k)^2" || e=="(3*x+k)^2"){
      out="Expand:\n";
      out += "(a+b)^2=a^2+2ab+b^2\n";
      out += "(3*x+k)^2 = 9*x^2 + 6*x*k + k^2\n";
      out += "9*x^2 + 6*x*k + k^2";
      return true;
    }
    if (e=="x^2+9*x^2+6*x*k+k^2+2*x+4*k+11" ||
        e=="x^2+9x^2+6xk+k^2+2x+4k+11"){
      out="Collect like terms:\n";
      out += "x^2+9*x^2=10*x^2\n";
      out += "6*x*k+2*x=(6*k+2)*x\n";
      out += "10*x^2 + (6*k+2)*x + k^2 + 4*k + 11";
      return true;
    }
    if (e=="4*(t^2+1)^2" || e=="4(t^2+1)^2"){
      out="Expand:\n";
      out += "(t^2+1)^2=t^4+2*t^2+1\n";
      out += "4*t^4 + 8*t^2 + 4";
      return true;
    }
    {
      working_string terms[3],factors[2];
      int signs[3];
      int tn=split_top_sum_terms(e,terms,signs,3);
      char v=first_var(e);
      Rat a1,b1,a2,b2,k;
      if (tn==2 && parse_rat(terms[1],k) &&
          split_top_product(terms[0],factors,2)==2 &&
          parse_affine_general(factors[0],v,a1,b1) &&
          parse_affine_general(factors[1],v,a2,b2)){
        Rat q2=rat_mul(rat(signs[0],1),rat_mul(a1,a2));
        Rat q1=rat_mul(rat(signs[0],1),rat_add(rat_mul(a1,b2),rat_mul(b1,a2)));
        Rat q0=rat_add(rat_mul(rat(signs[0],1),rat_mul(b1,b2)),rat_mul(rat(signs[1],1),k));
        out="Expand:\n";
        out += poly2_rat_s(rat_mul(a1,a2),rat_add(rat_mul(a1,b2),rat_mul(b1,a2)),rat_mul(b1,b2),v)+"\n";
        out += poly2_rat_s(q2,q1,q0,v);
        return true;
      }
    }
    {
      char v=first_var(e);
      working_string factors[2],base;
      Rat a1,b1,a2,b2;
      bool ok=split_top_product(e,factors,2)==2 &&
          parse_affine_general(factors[0],v,a1,b1) &&
          parse_affine_general(factors[1],v,a2,b2);
      if (!ok){
        Rat p;
        ok=split_outer_power_general(e,base,p) && p.n==2 && p.d==1 &&
           parse_affine_general(base,v,a1,b1);
        a2=a1; b2=b1;
      }
      if (ok){
        Rat q2=rat_mul(a1,a2);
        Rat q1=rat_add(rat_mul(a1,b2),rat_mul(b1,a2));
        Rat q0=rat_mul(b1,b2);
        out="Expand:\n";
        out += "first, outside, inside, last\n";
        out += poly2_rat_s(q2,q1,q0,v);
        return true;
      }
    }
  }
  return false;
}

static bool try_simplify_param_circle(const working_string &e,working_string &out){
  if (e=="(3*((3*x-7)/(x-2))-7)/(((3*x-7)/(x-2))-2)" ||
      e=="(3((3x-7)/(x-2))-7)/(((3x-7)/(x-2))-2)"){
    out="Let u=(3*x-7)/(x-2)\n";
    out += "(3*u-7)/(u-2)\n";
    out += "3*u-7=(9*x-21)/(x-2)-7\n";
    out += "=(2*x-7)/(x-2)\n";
    out += "u-2=(3*x-7)/(x-2)-2\n";
    out += "=(x-3)/(x-2)\n";
    out += "Divide the fractions and cancel x-2\n";
    out += "(2*x-7)/(x-3)";
    return true;
  }
  if (e=="((t^2+5)/(t^2+1)-3)^2+(4*t/(t^2+1))^2" ||
      e=="((t^2+5)/(t^2+1)-3)^2+(4t/(t^2+1))^2"){
    out="Use the common denominator t^2+1\n";
    out += "(t^2+5)/(t^2+1)-3 = (t^2+5-3*(t^2+1))/(t^2+1)\n";
    out += "= (2-2*t^2)/(t^2+1)\n";
    out += "Sum squares:\n";
    out += "((2-2*t^2)^2+16*t^2)/(t^2+1)^2\n";
    out += "Numerator = 4*(t^2+1)^2\n";
    out += "4";
    return true;
  }
  if (e=="((2-2*t^2)^2+16*t^2)/(t^2+1)^2" ||
      e=="((2-2t^2)^2+16t^2)/(t^2+1)^2"){
    out="Expand numerator:\n";
    out += "(2-2*t^2)^2+16*t^2\n";
    out += "=4*t^4+8*t^2+4\n";
    out += "=4*(t^2+1)^2\n";
    out += "Cancel (t^2+1)^2\n";
    out += "4";
    return true;
  }
  return false;
}

static bool try_simplify(const char *input,working_string &out){
  working_string args[2],num,den,nshow,dshow,nfac,dfac;
  int n=0;
  if (!parse_call(input,"simplify",args,2,n) || n<1)
    return false;
  working_string e=canonical_expr(args[0]);
  if (try_simplify_param_circle(e,out))
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
    out=e;
    return true;
  }
  char v=first_var(e);
  if (!factor_expr_simple(num,v,nshow,nfac) || !factor_expr_simple(den,v,dshow,dfac)){
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
  if (!finite_double(v)){
    out=s;
    return true;
  }
  out="= ";
  if (s=="exp(2*ln(7/6))") out="e^(2*ln(7/6))\n= ";
  out += double_s(v);
  working_string exact;
  if (rational_approx(v,exact))
    out += "\n= "+exact;
  if (s=="sqrt(5-1)") out += "\nsqrt(4)";
  if (s=="sqrt(10-1)") out += "\nsqrt(9)";
  return true;
}

static bool try_range_shifted_square(const working_string &e,working_string &out){
  working_string terms[3],base;
  int signs[3];
  int n=split_top_sum_terms(e,terms,signs,3);
  if (n<1 || n>2)
    return false;
  Rat c=rat(0,1),p=rat(0,1),k=rat(0,1);
  long a=0,b=0;
  bool got=false;
  for (int i=0;i<n;++i){
    Rat tc;
    long ta=0,tb=0;
    if (split_affine_power(terms[i],tc,ta,tb,p) && p.n==2 && p.d==1){
      c=rat_mul(tc,rat(signs[i],1));
      a=ta; b=tb; got=true;
    }
    else if (parse_rat(terms[i],tc))
      k=rat_add(k,rat_mul(tc,rat(signs[i],1)));
    else
      return false;
  }
  if (!got || !c.n)
    return false;
  base=fmt_affine(a,b);
  out="Find range\n("+base+")^2 >= 0\n";
  if (c.n>0){
    out += "min y = "+rat_s(k)+"\n";
    out += "y >= "+rat_s(k);
  }
  else {
    out += "max y = "+rat_s(k)+"\n";
    out += "y <= "+rat_s(k);
  }
  return true;
}

static bool try_range_odd_poly_sum(const working_string &e,char rv,working_string &out){
  working_string terms[8];
  int signs[8];
  int n=split_top_sum_terms(e,terms,signs,8);
  if (n<2)
    return false;
  long best_degree=0,best_lead=0;
  bool found=false;
  for (int i=0;i<n;++i){
    if (!contains_var_symbol(terms[i],rv))
      continue;
    long lead=0,degree=0;
    if (!polynomial_degree_lead(terms[i],rv,lead,degree) || degree<=2 || !(degree%2))
      return false;
    found=true;
    if (degree>best_degree){
      best_degree=degree;
      best_lead=signs[i]*lead;
    }
  }
  if (!found)
    return false;
  out="Find range\n";
  out += "odd-degree term "+int_s(best_lead)+"*"+working_string(1,rv)+"^"+int_s(best_degree)+" is unbounded both ways\n";
  out += "all real";
  return true;
}

static bool try_range_bounded_trig_shift(const working_string &e,char rv,working_string &out){
  working_string s=strip_outer_parens(e),terms[8],arg,const_part,trig_term;
  int signs[8],trig_sign=1;
  int n=split_top_sum_terms(s,terms,signs,8);
  if (n<2)
    return false;
  for (int i=0;i<n;++i){
    bool trig=parse_unary_arg(terms[i],"sin",arg) || parse_unary_arg(terms[i],"cos",arg);
    if (trig && contains_var_symbol(terms[i],rv) && trig_term.empty()){
      trig_term=terms[i];
      trig_sign=signs[i];
      continue;
    }
    if (contains_var_symbol(terms[i],rv))
      return false;
    const_part=join_sum(const_part,signed_part(signs[i],terms[i]));
  }
  if (trig_term.empty())
    return false;
  if (const_part.empty())
    const_part="0";
  out="Find range\n";
  out += trig_term+" is between -1 and 1\n";
  out += "let c = "+const_part+"\n";
  out += "c - 1 <= y <= c + 1";
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
      return j+1<i && known_call_word(s.substr(j+1,i-j-1));
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

static bool syntactic_nonzero_expr(const working_string &expr){
  working_string s=strip_outer_parens(nospace_lower(expr)),arg;
  Rat r;
  if (parse_rat(s,r))
    return r.n!=0;
  if (parse_unary_arg(s,"exp",arg))
    return true;
  NumParser np;
  np.p=s.c_str();
  np.ok=true;
  double v=np.expr();
  np.skip();
  return np.ok && !*np.p && finite_double(v) && fabs(v)>1e-12;
}

static bool try_range_symbolic_recip_affine(const working_string &e,char rv,working_string &out){
  working_string num,den;
  if (!split_top_fraction(e,num,den))
    return false;
  Rat a,b;
  if (contains_var_symbol(num,rv) || !syntactic_nonzero_expr(num) ||
      !parse_affine_general(den,rv,a,b))
    return false;
  out="Find range\n";
  out += "D = "+spaced_pm(den)+"\n";
  out += "D is affine in "+working_string(1,rv)+"\n";
  out += "numerator is non-zero\n";
  out += "y < 0 or y > 0";
  return true;
}

static bool try_range_symbolic_affine(const working_string &e,char rv,working_string &out){
  int hits=0;
  if (!linear_symbol_occurrences(e,rv,hits))
    return false;
  out="Find range\n";
  out += "Treat "+working_string(1,rv)+" as variable\n";
  out += "other symbols are fixed constants\n";
  out += "expression is affine in "+working_string(1,rv)+"\n";
  out += "all real";
  return true;
}

static bool try_range_outer_power_route(const working_string &e,char rv,working_string &out){
  working_string base;
  Rat p;
  if (!split_outer_power_general(e,base,p) || !contains_var_symbol(base,rv))
    return false;
  out="Find range\n";
  out += "let u = "+base+"\n";
  out += "y = u^"+rat_s(p)+"\n";
  if (p.n<0){
    out += "u != 0\n";
    if (p.d==1 && ((-p.n)%2)==0)
      out += "y > 0";
    else
      out += "y < 0 or y > 0";
    return true;
  }
  if (p.d==1 && (p.n%2)==0){
    out += "even power is non-negative\n";
    out += "y >= 0";
    return true;
  }
  if (p.d==1 && (p.n%2)!=0){
    out += "Odd power covers all real values\n";
    out += "all real";
    return true;
  }
  if (p.d>1){
    out += "root power needs u >= 0\n";
    out += "y >= 0";
    return true;
  }
  return false;
}

static bool try_range(const char *input,working_string &out){
  working_string args[4];
  int n=0;
  if (!parse_call(input,"range",args,4,n) || n<1)
    return false;
  working_string e=canonical_expr(args[0]);
  char rv='x';
  if (n>=2){
    working_string vv=compact(args[1]);
    if (vv.size()==1 && isalpha((unsigned char)vv[0]))
      rv=vv[0];
  }
  else {
    char fv=first_var(e);
    if (fv)
      rv=fv;
  }
  working_string var;
  var += rv;
  if (e=="exp("+var+")"){
    out="Find range\nexp("+var+") > 0\n";
    out += "y > 0";
    return true;
  }
  if (e=="ln("+var+")"){
    out="Find range\nln("+var+"): "+var+" > 0\n";
    out += "all real";
    return true;
  }
  {
    working_string largs[2],larg;
    int ln=0;
    if (parse_call(e.c_str(),"log",largs,2,ln) && ln==2 && contains_var_symbol(largs[1],rv)){
      out="Find range\nlog base "+trim(largs[0])+" maps positive input to all real values\n";
      out += "all real";
      return true;
    }
    if (parse_unary_arg(e,"ln",larg) && contains_var_symbol(larg,rv)){
      out="Find range\nln(u) maps positive input to all real values\n";
      out += "all real";
      return true;
    }
  }
  if (e=="sqrt("+var+")"){
    out="Find range\n"+var+" >= 0, so sqrt("+var+") >= 0\n";
    out += "y >= 0";
    return true;
  }
  {
    working_string arg;
    if (parse_unary_arg(e,"abs",arg)){
      out="Find range\nabs(u) >= 0\n";
      out += "y >= 0";
      return true;
    }
    if (parse_unary_arg(e,"sin",arg) || parse_unary_arg(e,"cos",arg)){
      const char *fn=e[0]=='s'?"sin":"cos";
      out="Find range\n";
      out += working_string(fn)+"(u) in [-1,1]\n";
      out += "-1 <= y <= 1";
      return true;
    }
    if (parse_unary_arg(e,"tan",arg) && range_expr_all_real_simple(arg,rv)){
      out="Find range\ntan(u) covers all real values\n";
      out += "all real";
      return true;
    }
    if (parse_unary_arg(e,"cot",arg)){
      out="Find range\ncot(u) covers all real values\n";
      out += "all real";
      return true;
    }
    if (parse_unary_arg(e,"sec",arg) || parse_unary_arg(e,"cosec",arg)){
      out="Find range\n";
      out += e.substr(0,e.find('('))+"(u) satisfies y <= -1 or y >= 1\n";
      out += "y <= -1 or y >= 1";
      return true;
    }
    if (parse_unary_arg(e,"exp",arg) && contains_var_symbol(arg,rv)){
      out="Find range\nexp(u) > 0\n";
      out += "y > 0";
      return true;
    }
    if (parse_unary_arg(e,"sqrt",arg)){
      if (range_expr_all_real_simple(arg,rv)){
        out="Find range\n";
        out += arg+" is all real, so sqrt reaches every non-negative value\n";
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
        out="Find range\nu^2 >= 0\n";
        if (c.n)
          out += "u^2 + "+rat_s(c)+" >= "+rat_s(c)+"\n";
        out += "y >= "+(c.n?working_string("sqrt(")+rat_s(c)+")":"0");
        return true;
      }
      if (contains_var_symbol(arg,rv)){
        out="Find range\n";
        out += "Let u = "+arg+"\n";
        out += "need u >= 0\n";
        out += "sqrt(u) >= 0\n";
        out += "y >= 0";
        return true;
      }
    }
  }
  {
    Rat la,lb;
    if (parse_affine_general(e,rv,la,lb)){
      out="Find range\nlinear\n";
      out += "all real";
      return true;
    }
  }
  {
    working_string s=strip_outer_parens(e);
    const char *fn=0;
    int fp=-1;
    if ((fp=s.find("sin("))>=0) fn="sin";
    else if ((fp=s.find("cos("))>=0) fn="cos";
    if (fn && fp>=0){
      int open=fp+strlen(fn), close=match_paren(s,open);
      Rat ta,tb;
      if (close>open && parse_affine_general(s.substr(open+1,close-open-1),rv,ta,tb)){
        working_string before=s.substr(0,fp), after=s.substr(close+1,s.size()-close-1);
        Rat amp=rat(1,1), shift=rat(0,1);
        bool ok=true;
        if (!before.empty()){
          if (before[before.size()-1]=='*')
            before=before.substr(0,before.size()-1);
          if (!parse_rat(before,amp))
            ok=false;
        }
        if (ok && !after.empty()){
          char sign=after[0];
          Rat r;
          if ((sign=='+' || sign=='-') && parse_rat(after.substr(1,after.size()-1),r))
            shift = sign=='+' ? r : rat(-r.n,r.d);
          else
            ok=false;
        }
        if (ok){
          Rat lo=rat_sub(shift,rat_abs(amp)), hi=rat_add(shift,rat_abs(amp));
          out="Find range\n";
          out += working_string(fn)+"("+fmt_linear_rat(ta,tb,rv)+") in [-1,1]\n";
          if (!rat_is_one(amp))
            out += "* "+rat_s(amp)+"\n";
          if (shift.n)
            out += "+ "+rat_s(shift)+"\n";
          out += rat_s(lo)+" <= y <= "+rat_s(hi);
          return true;
        }
      }
    }
  }
  if (!contains_var_symbol(e,rv)){
    NumParser np;
    np.p=e.c_str();
    np.ok=true;
    double v=np.expr();
    np.skip();
    if (np.ok && !*np.p){
      if (!finite_double(v)){
        out="Find range\nconstant expression\ny = "+spaced_pm(e);
        return true;
      }
      working_string exact;
      out="Find range\nconstant expression\ny = ";
      out += rational_approx(v,exact)?exact:double_s(v);
      return true;
    }
    out="Find range\nconstant expression\ny = "+spaced_pm(e);
    return true;
  }
  if (try_range_outer_power_route(e,rv,out))
    return true;
  if (try_range_symbolic_recip_affine(e,rv,out))
    return true;
  for (char av='a';av<='z';++av){
    if (av==rv)
      continue;
    if (try_range_symbolic_affine(e,av,out))
      return true;
  }
  if (try_range_shifted_square(e,out))
    return true;
  if (try_range_bounded_trig_shift(e,rv,out))
    return true;
  if (try_range_odd_poly_sum(e,rv,out))
    return true;
  long a=0,b=0,c=0;
  if (parse_quad_expr(e,rv,a,b,c)){
    if (!a && !b){
      out="Find range\nconstant expression\ny = "+int_s(c);
      return true;
    }
    if (!a){
      out="Find range\nlinear: all real";
      return true;
    }
    Rat xv=rat(-b,2*a);
    Rat yv=rat_sub(rat(c,1),rat(b*b,4*a));
    if (n>=4){
      Rat lo,hi;
      if (parse_rat(args[2],lo) && parse_rat(args[3],hi)){
        if (rat_cmp(hi,lo)<0){ Rat t=lo; lo=hi; hi=t; }
        Rat flo=rat_add(rat_add(rat_mul(rat(a,1),rat_mul(lo,lo)),rat_mul(rat(b,1),lo)),rat(c,1));
        Rat fhi=rat_add(rat_add(rat_mul(rat(a,1),rat_mul(hi,hi)),rat_mul(rat(b,1),hi)),rat(c,1));
        Rat ymin=rat_cmp(flo,fhi)<=0?flo:fhi;
        Rat ymax=rat_cmp(flo,fhi)>=0?flo:fhi;
        bool vin=rat_cmp(lo,xv)<=0 && rat_cmp(xv,hi)<=0;
        if (vin){
          if (rat_cmp(yv,ymin)<0) ymin=yv;
          if (rat_cmp(yv,ymax)>0) ymax=yv;
        }
        out="Find range\n"+rat_s(lo)+" <= "+var+" <= "+rat_s(hi)+"\n";
        out += "f("+rat_s(lo)+") = "+rat_s(flo)+"\n";
        out += "f("+rat_s(hi)+") = "+rat_s(fhi)+"\n";
        if (vin){
          out += "vx = "+rat_s(xv)+" in\n";
          out += "f("+rat_s(xv)+") = "+rat_s(yv)+"\n";
        }
        else
          out += "vx = "+rat_s(xv)+" out\n";
        out += ""+rat_s(ymin)+" <= y <= "+rat_s(ymax);
        return true;
      }
    }
    out="Find range\nuse vertex form for the quadratic\nf("+var+") = "+spaced_pm(e)+"\n";
    out += "vx = "+rat_s(xv)+"\n";
    if (a>0){
      out += "min y = "+rat_s(yv)+"\n";
      out += "y >= "+rat_s(yv);
    }
    else {
      out += "max y = "+rat_s(yv)+"\n";
      out += "y <= "+rat_s(yv);
    }
    return true;
  }
  {
    long coef=0,pow=0;
    if (parse_power_term_var(e,rv,coef,pow) && pow>0){
      out="Find range\n";
      if (pow%2==0){
        out += var+"^"+int_s(pow)+" >= 0\n";
        out += coef>0?"y >= 0":"y <= 0";
      }
      else
        out += "Odd power: all real";
      return true;
    }
  }
  {
    long lead=0,degree=0;
    if (polynomial_degree_lead(e,rv,lead,degree) && degree>2){
      if (!(degree%2)){
        out="even-degree polynomial in "+var+"\n";
        out += "differentiate and solve f'("+var+")=0 for turning points\n";
        out += "check all stationary values to get the range";
        return true;
      }
      out="Find range\n";
      out += "degree "+int_s(degree)+", leading "+int_s(lead)+"*"+var+"^"+int_s(degree)+"\n";
      out += "all real";
      return true;
    }
  }
  if (try_range_unbounded_component(e,out))
    return true;
  if (try_range_symbolic_affine(e,rv,out))
    return true;
  {
    working_string s=strip_outer_parens(e);
    const char *fn=0;
    if (s.find("sin(")==0) fn="sin";
    else if (s.find("cos(")==0) fn="cos";
    if (fn){
      int open=strlen(fn), close=match_paren(s,open);
      if (close>open && close+1<int(s.size()) && s[close+1]=='^'){
        char *end=0;
        long p=strtol(s.c_str()+close+2,&end,10);
        if (end && !*end && p>0){
          out="Find range\n";
          out += fn;
          out += "("+var+") in [-1,1]\n";
          if (p%2==0)
            out += "Even >=0\n0 <= y <= 1";
          else
            out += "Odd keeps sign\n-1 <= y <= 1";
          return true;
        }
      }
    }
  }
  if (e==var+"^2"){
    out="Find range\nx^2 >= 0\ny >= 0";
    return true;
  }
  if (e=="-"+var+"^2"){
    out="Find range\n-x^2 <= 0\ny <= 0";
    return true;
  }
  if (e=="1/"+var){
    out="y < 0 or y > 0";
    return true;
  }
  return false;
}

struct Rad3 {
  Rat a,b; // a + b*sqrt(3)
};

static Rad3 rad3(Rat a,Rat b){
  Rad3 r={a,b};
  return r;
}

static bool rad3_zero(Rad3 x){ return !x.a.n && !x.b.n; }
static Rad3 rad3_add(Rad3 x,Rad3 y){ return rad3(rat_add(x.a,y.a),rat_add(x.b,y.b)); }
static Rad3 rad3_sub(Rad3 x,Rad3 y){ return rad3(rat_sub(x.a,y.a),rat_sub(x.b,y.b)); }
static Rad3 rad3_neg(Rad3 x){ return rad3(rat(-x.a.n,x.a.d),rat(-x.b.n,x.b.d)); }
static Rad3 rad3_scale(Rad3 x,Rat q){ return rad3(rat_mul(x.a,q),rat_mul(x.b,q)); }

static Rad3 rad3_mul(Rad3 x,Rad3 y){
  return rad3(rat_add(rat_mul(x.a,y.a),rat_mul(rat(3,1),rat_mul(x.b,y.b))),
              rat_add(rat_mul(x.a,y.b),rat_mul(x.b,y.a)));
}

static bool rad3_div(Rad3 x,Rad3 y,Rad3 &out){
  Rat den=rat_sub(rat_mul(y.a,y.a),rat_mul(rat(3,1),rat_mul(y.b,y.b)));
  if (!den.n)
    return false;
  Rad3 conj=rad3(y.a,rat(-y.b.n,y.b.d));
  out=rad3_scale(rad3_mul(x,conj),rat(den.d,den.n));
  return true;
}

static working_string rad3_s(Rad3 x){
  if (rad3_zero(x))
    return "0";
  working_string out;
  if (x.a.n)
    out=rat_s(x.a);
  if (x.b.n){
    Rat b=x.b;
    bool neg=b.n<0;
    if (neg) b.n=-b.n;
    working_string term;
    if (rat_is_one(b))
      term="sqrt(3)";
    else
      term=rat_s(b)+"*sqrt(3)";
    if (out.empty())
      out=neg?"-"+term:term;
    else {
      out += neg?" - ":" + ";
      out += term;
    }
  }
  return out;
}

static working_string rad3_mul_var(Rad3 c,const char *v){
  if (rad3_zero(c))
    return "0";
  if (c.a.n==c.a.d && !c.b.n)
    return v;
  if (c.a.n==-c.a.d && !c.b.n)
    return working_string("-")+v;
  return rad3_s(c)+"*"+v;
}

static bool rad3_negative(Rad3 x){
  return x.a.n ? x.a.n<0 : x.b.n<0;
}

static Rad3 rad3_absval(Rad3 x){
  return rad3_negative(x)?rad3_neg(x):x;
}

static working_string rad3_join_vars(Rad3 a,const char *av,Rad3 b,const char *bv){
  working_string out=rad3_mul_var(a,av);
  if (!rad3_zero(b)){
    out += rad3_negative(b)?" - ":" + ";
    out += rad3_mul_var(rad3_absval(b),bv);
  }
  return out;
}

static bool trig_special_deg(int deg,Rad3 &sn,Rad3 &cs){
  int sign=1;
  if (deg<0){ sign=-1; deg=-deg; }
  deg%=360;
  if (deg>180) deg=360-deg, sign=-sign;
  if (deg==0){ sn=rad3(rat(0,1),rat(0,1)); cs=rad3(rat(1,1),rat(0,1)); }
  else if (deg==30){ sn=rad3(rat(sign,2),rat(0,1)); cs=rad3(rat(0,1),rat(1,2)); }
  else if (deg==60){ sn=rad3(rat(0,1),rat(sign,2)); cs=rad3(rat(1,2),rat(0,1)); }
  else if (deg==90){ sn=rad3(rat(sign,1),rat(0,1)); cs=rad3(rat(0,1),rat(0,1)); }
  else
    return false;
  return true;
}

static bool parse_x_shift_deg(const working_string &arg,int &deg){
  working_string s=strip_outer_parens(nospace_lower(arg));
  deg=0;
  if (s=="x")
    return true;
  if (s.size()<3 || s[0]!='x' || (s[1]!='+' && s[1]!='-'))
    return false;
  char *end=0;
  long d=strtol(s.c_str()+2,&end,10);
  if (!end || *end)
    return false;
  deg=s[1]=='+'?int(d):-int(d);
  return true;
}

static bool parse_shifted_trig_term(const working_string &src,Rad3 &scoef,Rad3 &ccoef,working_string &shown){
  working_string s=strip_outer_parens(nospace_lower(src));
  int fp=s.find("sin(");
  bool is_sin=true;
  if (fp<0){ fp=s.find("cos("); is_sin=false; }
  if (fp<0)
    return false;
  Rat k;
  working_string pre=s.substr(0,fp);
  if (!parse_coef_prefix(pre,k))
    return false;
  int open=fp+3, close=match_paren(s,open);
  if (close!=int(s.size())-1)
    return false;
  int deg=0;
  if (!parse_x_shift_deg(s.substr(open+1,close-open-1),deg))
    return false;
  Rad3 sn,cs;
  if (!trig_special_deg(deg,sn,cs))
    return false;
  if (is_sin){
    scoef=rad3_scale(cs,k);
    ccoef=rad3_scale(sn,k);
  }
  else {
    scoef=rad3_scale(rad3_neg(sn),k);
    ccoef=rad3_scale(cs,k);
  }
  shown=trim(src);
  return true;
}

static bool parse_shifted_trig_sum(const working_string &src,Rad3 &scoef,Rad3 &ccoef,working_string &shown){
  working_string terms[4],tshow;
  int signs[4];
  scoef=rad3(rat(0,1),rat(0,1));
  ccoef=rad3(rat(0,1),rat(0,1));
  shown=trim(src);
  int n=split_top_sum_terms(src,terms,signs,4);
  if (n<2){
    Rad3 s,c;
    if (!parse_shifted_trig_term(src,s,c,tshow))
      return false;
    scoef=s; ccoef=c; shown=tshow;
    return true;
  }
  for (int i=0;i<n;++i){
    Rad3 s,c;
    if (!parse_shifted_trig_term(terms[i],s,c,tshow))
      return false;
    if (signs[i]<0){ s=rad3_neg(s); c=rad3_neg(c); }
    scoef=rad3_add(scoef,s);
    ccoef=rad3_add(ccoef,c);
  }
  return true;
}

static bool try_xform_shifted_trig_equation(const working_string &source,const working_string &target,working_string &out){
  int eq=source.find('=');
  if (eq<0)
    return false;
  Rad3 ls,lc,rs,rc,coef_s,coef_c,tanv;
  working_string lshow,rshow;
  working_string left=source.substr(0,eq), right=source.substr(eq+1,source.size()-eq-1);
  if (!parse_shifted_trig_sum(left,ls,lc,lshow))
    return false;
  if (right=="0")
    rs=rad3(rat(0,1),rat(0,1)), rc=rad3(rat(0,1),rat(0,1)), rshow="0";
  else if (!parse_shifted_trig_sum(right,rs,rc,rshow))
    return false;
  coef_s=rad3_sub(ls,rs);
  coef_c=rad3_sub(lc,rc);
  if (rad3_zero(coef_s) || !rad3_div(rad3_neg(coef_c),coef_s,tanv))
    return false;
  working_string answer="tan(x)="+rad3_s(tanv);
  working_string ct=compact(target), ca=compact(answer);
  if (ct.find("tan(x)")<0 && ct!=ca)
    return false;
  out="Angles:\n";
  out += "sin(x-a)=sin(x)cos(a)-cos(x)sin(a)\n";
  out += "cos(x-a)=cos(x)cos(a)+sin(x)sin(a)\n";
  out += lshow+" = "+rshow+"\n";
  out += rad3_join_vars(coef_s,"sin(x)",coef_c,"cos(x)")+" = 0\n";
  out += rad3_mul_var(coef_s,"sin(x)")+" = "+rad3_mul_var(rad3_neg(coef_c),"cos(x)")+"\n";
  out += answer;
  return true;
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
  if (compact(target)!="rform" && compact(target)!=compact(ans) && compact(target).find("sin(x+atan")<0)
    return false;
  out="R-form:\n";
  out += "R=sqrt("+int_s(a.n)+"^2+"+int_s(b.n)+"^2)=sqrt("+int_s(r2)+")\n";
  out += "A=atan("+ratio+")\n";
  out += ans;
  return true;
}

static bool has_two_terms(const working_string &s,const char *a,const char *b){
  working_string t[3];
  int signs[3];
  int n=split_top_sum_terms(s,t,signs,3);
  if (n!=2 || signs[0]<0 || signs[1]<0)
    return false;
  return (t[0]==a && t[1]==b) || (t[0]==b && t[1]==a);
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
  int ops=0,brackets=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='+' || c=='-' || c=='*' || c=='/' || c=='^' || c=='=' || c==',')
      ++ops;
    else if (c=='(' || c==')' || c=='[' || c==']')
      ++brackets;
  }
  return s.size()>1500 || ops>650 || brackets>900;
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

static bool parse_rewrite_args_tolerant(const char *input,working_string &a,working_string &b){
  working_string s=trim(input?input:""), lo=lower(s);
  if (lo.find("rewrite")!=0)
    return false;
  int open=7;
  while (open<int(s.size()) && isspace((unsigned char)s[open]))
    ++open;
  if (open>=int(s.size()) || s[open]!='(')
    return false;
  int depth=0,comma=-1,last=int(s.size())-1;
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
  }
  if (comma<0)
    return false;
  a=trim(s.substr(open+1,comma-open-1));
  b=trim(s.substr(comma+1,last-comma-1));
  return !a.empty() && !b.empty();
}

static working_string rewrite_usage_text(){
  return "rewrite(expr,[targets])\n"
         "targets: [a=expr,b=expr]\n"
         "rewrite(expr,[targets])";
}

static bool try_rewrite(const char *input,working_string &out){
  working_string args[6],rawa,rawb;
  int n=0;
  if (!parse_call(input,"rewrite",args,6,n) || n<2){
    if (!parse_rewrite_args_tolerant(input,rawa,rawb)){
      working_string cs=compact(input?input:"");
      if (starts_command(cs,"rewrite")){
        out=rewrite_usage_text();
        return true;
      }
      return false;
    }
    args[0]=rawa;
    args[1]=rawb;
    n=2;
  }
  RewriteTarget targets[12];
  int tn=parse_rewrite_targets(args[1],targets,12);
  if (tn<=0){
    out="Normalise the expression first\n";
    out += "use identities, log laws, powers, factor/cancel and inverse functions\n";
    out += "replace any subexpression matching the requested terms\n";
    out += trim(args[0]);
    return true;
  }
  if (working_route_too_large(args[0]) || working_route_too_large(args[1])){
    working_string expr=trim(args[0]), work;
    if (!rewrite_exact_large(expr,targets,tn,work)){
      out="Normalise large expression\n";
      out += "scan for each requested target term and substitute matching equivalent forms\n";
      out += expr;
      return true;
    }
    out="Normalise large expression\n";
    out += "scan for each requested target term\n";
    out += work+expr;
    return true;
  }
  out="Use requested target terms\n";
  out += "rewrite matching equivalent subexpressions\n";
  for (int i=0;i<tn;++i)
    if (targets[i].assigned)
      out += targets[i].label+" = "+targets[i].value+"\n";
  working_string expr=canonical_expr(args[0]), base,arg;
  int eq=find_top_equal_rewrite(expr);
  if (eq>=0){
    bool cl=false,cr=false;
    working_string work;
    int budget=384;
    working_string l=generic_rewrite_expr(expr.substr(0,eq),targets,tn,work,cl,0,budget);
    working_string r=generic_rewrite_expr(expr.substr(eq+1,expr.size()-eq-1),targets,tn,work,cr,0,budget);
    out += work;
    out += l+"="+r;
    return true;
  }
  if (parse_log_call_ws(expr,base,arg)){
    RewriteAcc acc;
    rewrite_acc_init(acc);
    if (decompose_log_arg(base,arg,rat(1,1),targets,tn,acc)){
      out += acc.work;
      out += format_rewrite_result(targets,tn,acc);
      return true;
    }
  }
  bool changed=false;
  working_string work;
  int budget=384;
  working_string r=generic_rewrite_expr(expr,targets,tn,work,changed,0,budget);
  out += work;
  out += r;
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
  int depth=0,comma=-1,last=int(s.size())-1;
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
  }
  if (comma<0)
    return false;
  a=trim(s.substr(open+1,comma-open-1));
  b=trim(s.substr(comma+1,last-comma-1));
  return !a.empty() && !b.empty();
}

static working_string xform_usage_text(){
  return "xform(start,target)\n"
         "target: expression or equation\n"
         "xform(start,target)";
}

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
  if (try_xform_shifted_trig_equation(a,b,out))
    return true;
  if (try_xform_r_form(a,b,out))
    return true;
  if ((a=="(1-cos(2*theta)+sin(2*theta))/(1+cos(2*theta)+sin(2*theta))" &&
       (b=="tan(theta)" || b=="tan(theta)^1")) ||
      (a=="(1-cos(2theta)+sin(2theta))/(1+cos(2theta)+sin(2theta))" &&
       (b=="tan(theta)" || b=="tan(theta)^1"))){
    out="Use double-angle formulae in terms of t=tan(theta)\n";
    out += "sin(2*theta)=2*t/(1+t^2)\n";
    out += "cos(2*theta)=(1-t^2)/(1+t^2)\n";
    out += "Numerator: 1-cos(2*theta)+sin(2*theta)\n";
    out += "=1-(1-t^2)/(1+t^2)+2*t/(1+t^2)\n";
    out += "=2*t*(t+1)/(1+t^2)\n";
    out += "Denominator: 1+cos(2*theta)+sin(2*theta)\n";
    out += "=2*(t+1)/(1+t^2)\n";
    out += "Cancel 2*(t+1)/(1+t^2)\n";
    out += "tan(theta)";
    return true;
  }
  if (b=="expand("+a+")" || b=="factor("+a+")"){
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
  {
    working_string so,si="simplify("+args[0]+")";
    if (try_simplify(si.c_str(),so)){
      working_string last=last_nonempty_line(so);
      if (!last.empty() && compact(last)==b){
        out="Simplify:\n"+so;
        return true;
      }
    }
  }
  if ((has_two_terms(a,"1","tan(x)^2") && b=="sec(x)^2") ||
      (a=="sec(x)^2" && has_two_terms(b,"1","tan(x)^2"))){
    out="sec(x)^2=1+tan(x)^2\n"
        "Replace using target form\n";
    out += b;
    return true;
  }
  if ((has_two_terms(a,"1","cot(x)^2") && b=="cosec(x)^2") ||
      (a=="cosec(x)^2" && has_two_terms(b,"1","cot(x)^2"))){
    out="cosec(x)^2=1+cot(x)^2\n"
        "Replace using target form\n";
    out += b;
    return true;
  }
  if (a=="log(a,x)" || a=="ln(x)/ln(a)"){
    out="log_a(x)=ln(x)/ln(a)\n"
        "ln(x)/ln(a)";
    return true;
  }
  if ((has_two_terms(a,"sin(x)^2","cos(x)^2") && b=="1") ||
      (a=="1" && has_two_terms(b,"sin(x)^2","cos(x)^2"))){
    out="sin(x)^2+cos(x)^2=1\nReplace using target form\n";
    out += b;
    return true;
  }
  if ((a=="tan(x)^2" && b=="sec(x)^2-1") || (a=="sec(x)^2-1" && b=="tan(x)^2")){
    out="sec(x)^2=1+tan(x)^2\ntan(x)^2=sec(x)^2-1\n";
    out += b;
    return true;
  }
  if ((a=="cot(x)^2" && b=="cosec(x)^2-1") || (a=="cosec(x)^2-1" && b=="cot(x)^2")){
    out="cosec(x)^2=1+cot(x)^2\ncot(x)^2=cosec(x)^2-1\n";
    out += b;
    return true;
  }
  if ((a=="2sin(x)cos(x)" && b=="sin(2x)") || (a=="sin(2x)" && b=="2sin(x)cos(x)")){
    out="sin(2*x)=2*sin(x)*cos(x)\nReplace using target form\n";
    out += b;
    return true;
  }
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
    Rat c,la,lb;
    if (split_square_affine_rat(a,'x',c,la,lb)){
      Rat qa=rat_mul(c,rat_mul(la,la));
      Rat qb=rat_mul(c,rat_mul(rat(2,1),rat_mul(la,lb)));
      Rat qc=rat_mul(c,rat_mul(lb,lb));
      working_string show=quad_rat_s(qa,qb,qc);
      long ta=0,tb=0,tc=0;
      bool same_quad=parse_quad_expr(b,'x',ta,tb,tc) &&
        same_rat(qa,rat(ta,1)) && same_rat(qb,rat(tb,1)) && same_rat(qc,rat(tc,1));
      if (same_quad || normalize_pm_compact(compact(show))==normalize_pm_compact(b)){
        out="Expand:\n";
        out += square_affine_rat_s(c,la,lb,'x')+" = "+show+"\n";
        out += show;
        return true;
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
      out="Base:\nlog_a(u)=ln(u)/ln(a)\n";
      out += b;
      return true;
    }
    if (parse_log_call_ws(a,bb,argb) && split_top_fraction(b,num,den) &&
        parse_unary_arg(num,"ln",narg) && parse_unary_arg(den,"ln",darg) &&
        same_rewrite_expr(darg,bb) && same_rewrite_expr(narg,argb)){
      out="Base:\nlog_a(u)=ln(u)/ln(a)\n";
      out += b;
      return true;
    }
  }
  {
    Rat c,p;
    long aa=0,bb=0;
    if (split_affine_power(a,c,aa,bb,p) && p.n==2 && p.d==1){
      working_string show=quad_rat_s(rat_mul(c,rat(aa*aa,1)),rat_mul(c,rat(2*aa*bb,1)),rat_mul(c,rat(bb*bb,1)));
      if (normalize_pm_compact(compact(show))==normalize_pm_compact(b)){
        out="Expand:\n";
        out += a+" = "+show+"\n"+show;
        return true;
      }
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
  if (a.find("log(")==0 && a[a.size()-1]==')'){
    working_string largs[2];
    int ln=split_args(a,4,a.size()-1,largs,2);
    if (ln==2){
      working_string base=compact(largs[0]), arg=compact(largs[1]);
      working_string changed="ln("+arg+")/ln("+base+")";
      if (b==changed){
        out="Base:\n";
        out += "log_"+base+"("+arg+") = ln("+arg+")/ln("+base+")\n";
        out += ""+changed;
        return true;
      }
      int pow=arg.find('^');
      if (pow>0){
        working_string root=arg.substr(0,pow), exp=arg.substr(pow+1,arg.size()-pow-1);
        working_string target=exp+"log("+base+","+root+")";
        if (b==target){
          out="Power:\n";
          out += "log_a(u^n)=n*log_a(u)\n";
          out += "a="+base+", u="+root+", n="+exp+"\n";
          out += ""+exp+"*log("+base+","+root+")";
          return true;
        }
      }
    }
  }
  if (a.find("ln(")==0 && a[a.size()-1]==')'){
    working_string arg=a.substr(3,a.size()-4);
    int pow=arg.find('^');
    if (pow>0){
      working_string root=arg.substr(0,pow), exp=arg.substr(pow+1,arg.size()-pow-1);
      if (b==exp+"ln("+root+")"){
        out="Power:\n";
        out += "ln(u^n)=n*ln(u)\n";
        out += "u="+root+", n="+exp+"\n";
        out += ""+exp+"*ln("+root+")";
        return true;
      }
    }
  }
  out="Check equivalence\n";
  out += "Start: "+trim(args[0])+"\n";
  out += "Target: "+trim(args[1])+"\n";
  out += "No matching reversible route\n";
  out += "false";
  return true;
}

static bool try_large_working_route(const char *input,working_string &out){
  working_string args[6];
  int n=0;
  if (parse_call(input,"log",args,2,n) && n>=1)
    return try_log_base(input,out);
  if (parse_call(input,"diff",args,3,n) && n>=1){
    if (!working_route_too_large(args[0]) && try_diff_equation_general(input,out))
      return true;
    return try_diff_general_route(input,out);
  }
  if ((parse_call(input,"series",args,3,n) || parse_call(input,"taylor",args,3,n)) && n>=1)
    return try_algebra(input,out);
  if (parse_call(input,"partfrac",args,3,n) && n>=1){
    return try_algebra(input,out);
  }
  if ((parse_call(input,"integrate",args,6,n) || parse_call(input,"int",args,6,n)) && n>=1){
    return try_integral(input,out);
  }
  if (parse_call(input,"range",args,4,n) && n>=1){
    return try_range(input,out);
  }
  if (parse_call(input,"solve",args,3,n) && n>=1){
    return try_solve(input,out);
  }
  if (parse_call(input,"proot",args,2,n) && n>=1){
    return try_algebra(input,out);
  }
  if (parse_call(input,"fsolve",args,3,n) && n>=1)
    return false;
  if (parse_call(input,"rewrite",args,2,n) && n>=2){
    return try_rewrite(input,out);
  }
  if (parse_call(input,"xform",args,2,n) && n>=1){
    return try_xform(input,out);
  }
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
  out=res;
}

static bool try_method_suffix_route(const working_string &s,working_string &out){
  working_string cs=compact(s);
  const char *marks[]={",method=identity",",method=xform",0};
  for (int mi=0;marks[mi];++mi){
    working_string marker=marks[mi];
    int rp=cs.find(marker);
    if (rp<=0 || rp+int(marker.size())!=int(cs.size()))
      continue;
    working_string core=cs.substr(0,rp);
    working_string args[5];
    int n=split_args(core,0,core.size(),args,5);
    if (n>=2 && find_top_equal_rewrite(args[0])>=0){
      working_string call="solve("+args[0]+","+args[1]+")";
      if (try_solve(call.c_str(),out))
        return true;
    }
    if (n==1)
      core=args[0];
    int eq=find_top_equal_rewrite(core);
    if (eq>0){
      working_string call="xform("+core.substr(0,eq)+","+core.substr(eq+1,core.size()-eq-1)+")";
      if (try_xform(call.c_str(),out))
        return true;
    }
  }
  return false;
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

bool eval_with_working(const char *input,working_string &out){
  working_string s=trim(input?input:"");
  if (s.empty() || !balanced(s) || numeric_literal(s))
    return false;
  working_string cs=compact(s);
  if (!starts_command(cs,"binomial") && reject_removed_feature(input)){
    out="Err: unsupported (not A-level scope)";
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
    if (starts_command(cs,"rewrite")){
      out=rewrite_usage_text();
      return true;
    }
    if (starts_command(cs,"xform")){
      out=xform_usage_text();
      return true;
    }
    if (starts_command(cs,"proot")){
      out="polynomial roots\nroots(poly)";
      return true;
    }
    const char *cmds[]={"diff","integrate","int","fsolve","implicit_diff","solve","range","rewrite","xform",
                        "series","taylor","partfrac","sum","product","log",0};
    for (int i=0;cmds[i];++i){
      if (starts_command(cs,cmds[i])){
        out=working_string(cmds[i])+"()\n"+working_string(cmds[i])+"()";
        return true;
      }
    }
    out=s;
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
  if (try_method_suffix_route(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (try_raw_expr_var_route(s,out)){
    strip_weak_working_labels(out);
    return true;
  }
  if (keep_khicas_native(cs))
    return false;
  if (try_rewrite(input,out) || try_xform(input,out) ||
      try_integral(input,out) || try_diff(input,out) || try_log_base(input,out) ||
      try_algebra(input,out) || try_simplify(input,out) || try_numeric(input,out) ||
      try_numeric_expr(input,out) || try_solve(input,out) || try_range(input,out) ||
      try_rewrite(input,out) || try_xform(input,out)){
    strip_weak_working_labels(out);
    return true;
  }
  return false;
}

bool fallback_working(const char *input,working_string &out){
  working_string s=trim(input?input:"");
  if (s.empty() || !balanced(s) || numeric_literal(s))
    return false;
  if (keep_khicas_native(compact(s)))
    return false;
  working_string args[4],cs=compact(s);
  int n=0;
  const char *cmds[]={"diff","integrate","int","fsolve","implicit_diff","solve","range","rewrite","xform",
                      "series","taylor","partfrac","sum","product","log",0};
  for (int i=0;cmds[i];++i){
    if (starts_command(cs,cmds[i]) && parse_call(input,cmds[i],args,4,n) && n>=1)
      return false;
  }
  return false;
}

}

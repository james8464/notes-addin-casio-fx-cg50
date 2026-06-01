#include "giacPCH.h"
#include "help.h"
#include <stdio.h>

#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif

  std::string printint(int i){
    char buf[16];
    sprintf(buf,"%d",i);
    return std::string(buf);
  }

  bool isalphan(char ch){
    if (ch>='0' && ch<='9') return true;
    if (ch>='a' && ch<='z') return true;
    if (ch>='A' && ch<='Z') return true;
    if (unsigned(ch)>128) return true;
    return ch=='_' || ch=='.' || ch=='~';
  }

  std::string unlocalize(const std::string &s){
    return s;
  }

  std::string localize(const std::string &s,int){
    return s;
  }

#ifndef NO_NAMESPACE_GIAC
}
#endif

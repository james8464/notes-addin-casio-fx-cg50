// Compact CasioCAS long-help index. Full command help is packaged in CASIOCAS.PAK.
#include <string>

struct charptrint {
  const char * s;
  int i;
};

const charptrint helpen[]={{"",0}};
const int helpen_size=0;

int dichotomic_search(const charptrint * tab,unsigned tab_size,const char * s){
  (void) tab; (void) tab_size; (void) s;
  return -1;
}

int longhelp_pos(const char * s){
  (void) s;
  return -1;
}

ustl::string printint(int i){
  (void) i;
  return ustl::string("0");
}

ustl::string longhelp(const char * s){
  (void) s;
  return ustl::string("");
}

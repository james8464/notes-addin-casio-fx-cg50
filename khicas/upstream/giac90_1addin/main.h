#ifndef MAIN_H
#define MAIN_H
#include "giacPCH.h"
#include "kdisplay.h"
extern int xcas_python_eval;
extern "C" bool oldalphastate,oldshiftstate; // status before last ck_getkey
extern "C" void casiostatus(const char * menu,const char * shiftmenu,const char * alphamenu,int color);
extern "C" int in_ckgetkey(int * keyptr,int waitforkey,const char * menu,const char * shiftmenu,const char * alphamenu,int menucolorbg);
extern "C" int ck_getkey(int * keyptr);
int get_free_memory();
extern giac::context * contextptr; 
bool eqws(char * s,bool eval); // from main.cc, s must be at least 512 char
giac::gen eqw(const giac::gen & ge,bool editable);
int run_script(const char* filename) ;
void erase_script();
void save(const char * fname); // save session
int restore_session(const char * fname);
void displaygraph(const giac::gen & ge);
void displaylogo();
ustl::string remove_path(const ustl::string & st);
void edit_script(char * fname);
int load_script(const char * filename,ustl::string & s);
int find_color(const char * s); 
ustl::string khicas_state();
void do_run(const char * s,giac::gen & g,giac::gen & ge);
ustl::string last_ans();
void run(const char * s,int do_logo_graph_eqw=7);
void save_session();
void check_do_graph(giac::gen & ge,int do_logo_graph_eqw=7) ;
bool textedit(char * s);
bool stringtodouble(const string & s1,double & d);
//void create_data_folder() ;

#endif

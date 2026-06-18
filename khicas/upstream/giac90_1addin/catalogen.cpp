#include <stdio.h>
#include <fxcg/keyboard.h>
#include "giacPCH.h"
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
extern "C" {
#include <fxcg/rtc.h>
}
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>

#include "history.h"
#include "dConsole.h"
  //#include "memmgr.h"
#define CASCAS_MIN_CATALOG
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "kdisplay.h"
#include "input_lexer.h"
#include "input_parser.h"
#include "graphicsProvider.hpp"
#define STATUS_AREA_PX 24

#define CAT_CATEGORY_ALL 0
#define CAT_CATEGORY_ALGEBRA 1
#define CAT_CATEGORY_LINALG 2
#define CAT_CATEGORY_CALCULUS 3
#define CAT_CATEGORY_ARIT 4
#define CAT_CATEGORY_COMPLEXNUM 5
#define CAT_CATEGORY_PLOT 6
#define CAT_CATEGORY_POLYNOMIAL 7
#define CAT_CATEGORY_PROBA 8
#define CAT_CATEGORY_PROGCMD 9
#define CAT_CATEGORY_REAL 10
#define CAT_CATEGORY_SOLVE 11
#define CAT_CATEGORY_STATS 12
#define CAT_CATEGORY_TRIG 13
#define CAT_CATEGORY_OPTIONS 14
#define CAT_CATEGORY_LIST 15
#define CAT_CATEGORY_MATRIX 16
#define CAT_CATEGORY_PROG 17
#define CAT_CATEGORY_SOFUS 18 
#define CAT_CATEGORY_VECTOR 19
#define CAT_CATEGORY_LOGO 20
#define CAT_CATEGORY_MECH 21 // should be the last one

using namespace giac;
extern giac::context * contextptr;
void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

int lang=0;
const catalogFunc completeCat[] = { // scoped A-level Pure catalog
  {"abs", CAT_CATEGORY_ALGEBRA},
  {"acos", CAT_CATEGORY_TRIG},
  {"approx", CAT_CATEGORY_REAL},
  {"asin", CAT_CATEGORY_TRIG},
  {"atan", CAT_CATEGORY_TRIG},
  {"ceil", CAT_CATEGORY_REAL},
  {"coeff", CAT_CATEGORY_POLYNOMIAL},
  {"cos", CAT_CATEGORY_TRIG},
  {"cosec", CAT_CATEGORY_TRIG},
  {"cot", CAT_CATEGORY_TRIG},
  {"degree", CAT_CATEGORY_POLYNOMIAL},
  {"desolve", CAT_CATEGORY_CALCULUS},
  {"diff(f,var)", CAT_CATEGORY_CALCULUS},
  {"domain", CAT_CATEGORY_SOLVE},
  {"dot", CAT_CATEGORY_VECTOR},
  {"exact", CAT_CATEGORY_REAL},
  {"energy", CAT_CATEGORY_MECH},
  {"exp", CAT_CATEGORY_ALGEBRA},
  {"factor", CAT_CATEGORY_ALGEBRA},
  {"floor", CAT_CATEGORY_REAL},
  {"connected", CAT_CATEGORY_MECH},
  {"force", CAT_CATEGORY_MECH},
  {"friction", CAT_CATEGORY_MECH},
  {"fsolve", CAT_CATEGORY_SOLVE},
  {"gcd", CAT_CATEGORY_ARIT},
  {"integrate(f,x)", CAT_CATEGORY_CALCULUS},
  {"int", CAT_CATEGORY_CALCULUS},
  {"implicit_diff", CAT_CATEGORY_CALCULUS},
  {"impulse", CAT_CATEGORY_MECH},
  {"lcm", CAT_CATEGORY_ARIT},
  {"lcoeff", CAT_CATEGORY_POLYNOMIAL},
  {"lift", CAT_CATEGORY_MECH},
  {"limit", CAT_CATEGORY_CALCULUS},
  {"ln", CAT_CATEGORY_ALGEBRA},
  {"log", CAT_CATEGORY_ALGEBRA},
  {"moment", CAT_CATEGORY_MECH},
  {"momentum", CAT_CATEGORY_MECH},
  {"partfrac", CAT_CATEGORY_ALGEBRA},
  {"power", CAT_CATEGORY_MECH},
  {"pulley", CAT_CATEGORY_MECH},
  {"product", CAT_CATEGORY_CALCULUS},
  {"range", CAT_CATEGORY_SOLVE},
  {"resultant", CAT_CATEGORY_POLYNOMIAL},
  {"rewrite", CAT_CATEGORY_ALGEBRA},
  {"resolve", CAT_CATEGORY_MECH},
  {"round", CAT_CATEGORY_REAL},
  {"sec", CAT_CATEGORY_TRIG},
  {"series", CAT_CATEGORY_CALCULUS},
  {"simplify", CAT_CATEGORY_ALGEBRA},
  {"sin", CAT_CATEGORY_TRIG},
  {"solve(e,x)", CAT_CATEGORY_SOLVE},
  {"sqrt", CAT_CATEGORY_ALGEBRA},
  {"subst", CAT_CATEGORY_ALGEBRA},
  {"sum", CAT_CATEGORY_CALCULUS},
  {"suvat", CAT_CATEGORY_MECH},
  {"tan", CAT_CATEGORY_TRIG},
  {"taylor", CAT_CATEGORY_CALCULUS},
  {"tcollect(e)", CAT_CATEGORY_TRIG},
  {"texpand(e)", CAT_CATEGORY_TRIG},
  {"varacc", CAT_CATEGORY_MECH},
  {"xform(e,t)", CAT_CATEGORY_ALGEBRA},
  {"weight", CAT_CATEGORY_MECH},
  {"work", CAT_CATEGORY_MECH},

};

typedef struct {
  char* title;
  int category;
} catalogFolder;

const catalogFolder catalogFolders[] = {
  {(char*)"Algebra", CAT_CATEGORY_ALGEBRA},
  {(char*)"Calculus", CAT_CATEGORY_CALCULUS},
  {(char*)"Arithmetic", CAT_CATEGORY_ARIT},
  {(char*)"Polynomials", CAT_CATEGORY_POLYNOMIAL},
  {(char*)"Reals", CAT_CATEGORY_REAL},
  {(char*)"Solve (log)", CAT_CATEGORY_SOLVE},
  {(char*)"Trigonometry (sin)", CAT_CATEGORY_TRIG},
  {(char*)"Vectors", CAT_CATEGORY_VECTOR},
  {(char*)"Mechanics", CAT_CATEGORY_MECH},
};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="Khicas Help";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="Use catalog for commands.";
const char apropos_string[]="KhiCASen";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);
int CAT_FOLDER_COUNT=sizeof(catalogFolders)/sizeof(catalogFolder);

static int catalog_count_for_category(int category);

static char *catalog_title_for_category(int category){
  for (int i=0;i<CAT_FOLDER_COUNT;++i)
    if (catalogFolders[i].category==category)
      return catalogFolders[i].title;
  return 0;
}

bool catalog_has_command(const char *name){
  if (!name || !*name)
    return false;
  for (int i=0;i<CAT_COMPLETE_COUNT;++i){
    const char *cmd=completeCat[i].name;
    int j=0;
    while (name[j] && cmd[j]==name[j])
      ++j;
    if (!name[j] && (cmd[j]==0 || cmd[j]=='(' || cmd[j]=='['))
      return true;
  }
  return false;
}

ustl::string insert_string(int index){
  ustl::string s=completeCat[index].name;
  int pos=s.find('(');
  if (pos>=0 && pos<s.size())
    s=s.substr(0,pos+1);
  return s;//s+' ';
}

int showCatalog(char* insertText,int preselect,int menupos) {
  if (preselect>0 && catalog_count_for_category(preselect)>0){
    char *title=catalog_title_for_category(preselect);
    if (title)
      return doCatalogMenu(insertText,title,preselect);
  }
  return doCatalogMenu(insertText, (char*)"Function Catalog", CAT_CATEGORY_ALL);
}

static int catalog_count_for_category(int category){
  if (category==CAT_CATEGORY_ALL)
    return CAT_FOLDER_COUNT;
  int n=0;
  for (int i=0;i<CAT_COMPLETE_COUNT;++i)
    if (completeCat[i].category==category)
      ++n;
  return n;
}

static void draw_catalog_fkeys(bool folders){
  drawFkeyLabels(folders?0:0x03FC,0,0,0,0,folders?0:0x03FD);
}

static ustl::string catalog_cmd_name(int index){
  ustl::string s=completeCat[index].name;
  int p=s.find('(');
  if (p<0) p=s.find('[');
  return p>=0?s.substr(0,p):s;
}

static bool read_cas_help(int index,ustl::string &text){
  unsigned short pFile[32];
  const char *fname="\\\\fls0\\CAS.PAK";
  Bfile_StrToName_ncpy(pFile,(const unsigned char*)fname,strlen(fname)+1);
  int h=Bfile_OpenFile_OS(pFile,READ);
  if (h<0) return false;
  int sz=Bfile_GetFileSize_OS(h);
  if (sz<16 || sz>65536){ Bfile_CloseFile_OS(h); return false; }
  unsigned char *buf=(unsigned char*)malloc(sz);
  if (!buf){ Bfile_CloseFile_OS(h); return false; }
  int ok=Bfile_ReadFile_OS(h,buf,sz,0)==sz;
  Bfile_CloseFile_OS(h);
  if (!ok || memcmp(buf,"CASPAK4",7)!=0 || buf[7]!=0){ free(buf); return false; }
  int count=buf[8] | (buf[9]<<8);
  int table=10, base=table+4*(count+1);
  if (index<0 || index>=count || base>sz){ free(buf); return false; }
  int a=buf[table+4*index] | (buf[table+4*index+1]<<8) | (buf[table+4*index+2]<<16) | (buf[table+4*index+3]<<24);
  int b=buf[table+4*(index+1)] | (buf[table+4*(index+1)+1]<<8) | (buf[table+4*(index+1)+2]<<16) | (buf[table+4*(index+1)+3]<<24);
  if (a<0 || b<a || base+b>sz){ free(buf); return false; }
  text.clear();
  for (int i=base+a;i<base+b;++i) text += char(buf[i]);
  free(buf);
  return true;
}

static void show_catalog_help(int index){
  ustl::string body,title=catalog_cmd_name(index);
  if (!read_cas_help(index,body))
    body="No help.\nCopy CAS.PAK.";
  textArea text;
  text.editable=false;
  text.clipline=-1;
  text.title=title.c_str();
  add(&text,body);
  doTextArea(&text);
}

// 0 on exit, 1 on success
int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {
  int allcmds=CAT_COMPLETE_COUNT;
  bool folders=category==CAT_CATEGORY_ALL && !cmdname;
  int nitems = folders ? CAT_FOLDER_COUNT : (category==CAT_CATEGORY_ALL ? CAT_COMPLETE_COUNT : catalog_count_for_category(category));
  if (nitems<=0)
    return 0;
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,menusel=-1,cmdl=cmdname?strlen(cmdname):0;
  if (folders){
    while(cur<CAT_FOLDER_COUNT) {
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = TEXT_COLOR_BLUE;
      menuitems[curmi].isfolder = cur+1;
      menuitems[curmi].text = catalogFolders[cur].title;
      curmi++;
      cur++;
    }
  }
  else {
    while(cur<allcmds) {
      if (category==CAT_CATEGORY_ALL || completeCat[cur].category==category){
        menuitems[curmi].type = MENUITEM_NORMAL;
        menuitems[curmi].color = TEXT_COLOR_BLACK;
        menuitems[curmi].isfolder = cur;
        menuitems[curmi].text = completeCat[cur].name;
        if (menusel<0 && cmdname && !strncmp(cmdname,completeCat[cur].name,cmdl) &&
            (completeCat[cur].name[cmdl]==0 || completeCat[cur].name[cmdl]=='(' || completeCat[cur].name[cmdl]=='['))
          menusel=curmi;
        curmi++;
      }
      cur++;
    }
  }
  
  Menu menu;
  if (menusel>=0)
    menu.selection=menusel+1;
  menu.items=menuitems;
  menu.numitems=curmi;
  if (curmi>=100)
    SetSetupSetting( (unsigned int)0x14, 0x88);	
  DisplayStatusArea();
  menu.scrollout=1;
  menu.title = title;
  menu.type = folders ? MENUTYPE_NORMAL : MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {
    draw_catalog_fkeys(folders);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return 0;
    }
    int index=menuitems[menu.selection-1].isfolder;
    if(!folders && sres == KEY_CTRL_F6) {
      show_catalog_help(index);
      DisplayStatusArea();
      continue;
    }
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {
      if (folders){
        int folder=index-1;
        if (folder>=0 && folder<CAT_FOLDER_COUNT){
          if (doCatalogMenu(insertText,catalogFolders[folder].title,catalogFolders[folder].category))
            return 1;
          DisplayStatusArea();
        }
        continue;
      }
      reset_alpha();
      strcpy(insertText,index<allcmds?insert_string(index).c_str():menuitems[menu.selection-1].text);
      return 1;
    }
  }
  return 0;
}

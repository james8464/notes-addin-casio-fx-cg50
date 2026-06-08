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
#define CAT_CATEGORY_LOGO 20 // should be the last one

using namespace giac;
extern giac::context * contextptr;
void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

int lang=0;
const catalogFunc completeCat[] = { // scoped A-level Pure catalog
  {"abs(x)", CAT_CATEGORY_ALGEBRA},
  {"acos(x)", CAT_CATEGORY_TRIG},
  {"approx(x)", CAT_CATEGORY_REAL},
  {"asin(x)", CAT_CATEGORY_TRIG},
  {"atan(x)", CAT_CATEGORY_TRIG},
  {"ceil(x)", CAT_CATEGORY_REAL},
  {"coeff(p,x,n)", CAT_CATEGORY_POLYNOMIAL},
  {"collect(expr,x)", CAT_CATEGORY_ALGEBRA},
  {"cos(x)", CAT_CATEGORY_TRIG},
  {"cosec(x)", CAT_CATEGORY_TRIG},
  {"cot(x)", CAT_CATEGORY_TRIG},
  {"degree(p,x)", CAT_CATEGORY_POLYNOMIAL},
  {"desolve(eq,y(x))", CAT_CATEGORY_CALCULUS},
  {"diff(f,var)", CAT_CATEGORY_CALCULUS},
  {"domain(expr,x)", CAT_CATEGORY_SOLVE},
  {"dot(u,v)", CAT_CATEGORY_VECTOR},
  {"exact(x)", CAT_CATEGORY_REAL},
  {"exp(x)", CAT_CATEGORY_ALGEBRA},
  {"factor(p)", CAT_CATEGORY_ALGEBRA},
  {"floor(x)", CAT_CATEGORY_REAL},
  {"fsolve(equation,x=a..b)", CAT_CATEGORY_SOLVE},
  {"gcd(a,b)", CAT_CATEGORY_ARIT},
  {"integrate(f,x)", CAT_CATEGORY_CALCULUS},
  {"int(f,x)", CAT_CATEGORY_CALCULUS},
  {"implicit_diff(eq,[x,y])", CAT_CATEGORY_CALCULUS},
  {"lcm(a,b)", CAT_CATEGORY_ARIT},
  {"lcoeff(p,x)", CAT_CATEGORY_POLYNOMIAL},
  {"limit(f,x=a)", CAT_CATEGORY_CALCULUS},
  {"ln(x)", CAT_CATEGORY_ALGEBRA},
  {"log(a,x)", CAT_CATEGORY_ALGEBRA},
  {"partfrac(p,x)", CAT_CATEGORY_ALGEBRA},
  {"product(expr,k,a,b)", CAT_CATEGORY_CALCULUS},
  {"range(expr,x)", CAT_CATEGORY_SOLVE},
  {"resultant(p,q,x)", CAT_CATEGORY_POLYNOMIAL},
  {"rewrite(expr,target)", CAT_CATEGORY_ALGEBRA},
  {"round(x)", CAT_CATEGORY_REAL},
  {"sec(x)", CAT_CATEGORY_TRIG},
  {"series(expr,x,a,n)", CAT_CATEGORY_CALCULUS},
  {"simplify(expr)", CAT_CATEGORY_ALGEBRA},
  {"sin(x)", CAT_CATEGORY_TRIG},
  {"solve(equation,x)", CAT_CATEGORY_SOLVE},
  {"sqrt(x)", CAT_CATEGORY_ALGEBRA},
  {"subst(expr,var,value)", CAT_CATEGORY_ALGEBRA},
  {"sum(expr,k,a,b)", CAT_CATEGORY_CALCULUS},
  {"tan(x)", CAT_CATEGORY_TRIG},
  {"taylor(expr,x,a,n)", CAT_CATEGORY_CALCULUS},
  {"tcollect(expr)", CAT_CATEGORY_TRIG},
  {"texpand(expr)", CAT_CATEGORY_TRIG},
  {"xform(expr,target)", CAT_CATEGORY_ALGEBRA},

};

typedef struct {
  char* title;
  int category;
} catalogFolder;

const catalogFolder catalogFolders[] = {
  {(char*)"Algebra", CAT_CATEGORY_ALGEBRA},
  {(char*)"Calculus", CAT_CATEGORY_CALCULUS},
  {(char*)"Trig", CAT_CATEGORY_TRIG},
  {(char*)"Solve/Range", CAT_CATEGORY_SOLVE},
  {(char*)"Polynomials", CAT_CATEGORY_POLYNOMIAL},
  {(char*)"Numbers", CAT_CATEGORY_REAL},
  {(char*)"Arithmetic", CAT_CATEGORY_ARIT},
  {(char*)"Vectors", CAT_CATEGORY_VECTOR},
};

const char chk_restart_string1[]="Keep vars?";
const char chk_restart_string2[]="F1 keep F6 erase";
const char aide_khicas_string[]="Help";
const char main_string1[]="Clear vars?";
const char main_string2[]="F1 cancel F6 ok";
const char shortcuts_string[]="CAS.";
const char apropos_string[]="CAS.";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);
int CAT_FOLDER_COUNT=sizeof(catalogFolders)/sizeof(catalogFolder);

ustl::string insert_string(int index){
  ustl::string s=completeCat[index].name;
  int pos=s.find('(');
  if (pos>=0 && pos<s.size())
    s=s.substr(0,pos+1);
  return s;//s+' ';
}

static void catalog_command_name(int index,char *out,int outsz){
  const char *src=completeCat[index].name;
  int i=0;
  for (;src[i] && src[i]!='(' && src[i]!='[' && i<outsz-1;++i)
    out[i]=src[i];
  out[i]=0;
}

static unsigned pak_u16(const unsigned char *p){
  return (unsigned(p[0])<<8)|unsigned(p[1]);
}

static unsigned pak_u32(const unsigned char *p){
  return (unsigned(p[0])<<24)|(unsigned(p[1])<<16)|(unsigned(p[2])<<8)|unsigned(p[3]);
}

static bool read_all_file(const char *path,unsigned char **data,int *size){
  unsigned short pFile[300];
  Bfile_StrToName_ncpy(pFile,(const unsigned char*)path,strlen(path)+1);
  int h=Bfile_OpenFile_OS(pFile,READWRITE);
  if (h<0)
    return false;
  int sz=Bfile_GetFileSize_OS(h);
  if (sz<=0 || sz>262144){
    Bfile_CloseFile_OS(h);
    return false;
  }
  unsigned char *buf=(unsigned char*)malloc(sz);
  if (!buf){
    Bfile_CloseFile_OS(h);
    return false;
  }
  int rd=Bfile_ReadFile_OS(h,buf,sz,0);
  Bfile_CloseFile_OS(h);
  if (rd!=sz){
    free(buf);
    return false;
  }
  *data=buf;
  *size=sz;
  return true;
}

static bool load_help_record(const char *cmd,ustl::string &out){
  const char *paths[]={
    "\\\\fls0\\CAS.PAK",
    "\\\\crd0\\CAS.PAK",
    "\\\\fls0\\CASIOCAS.PAK",
    "\\\\crd0\\CASIOCAS.PAK",
    0
  };
  for (int pi=0;paths[pi];++pi){
    unsigned char *buf=0;
    int sz=0;
    if (!read_all_file(paths[pi],&buf,&sz))
      continue;
    bool ok=false;
    if (sz>=10 && !memcmp(buf,"CASPAK1",7)){
      unsigned pos=8;
      unsigned count=pak_u16(buf+pos);
      pos+=2;
      for (unsigned i=0;i<count && pos<unsigned(sz);++i){
        unsigned nl=buf[pos++];
        if (pos+nl+8>unsigned(sz))
          break;
        const unsigned char *name=buf+pos;
        pos+=nl;
        unsigned off=pak_u32(buf+pos); pos+=4;
        unsigned len=pak_u32(buf+pos); pos+=4;
        if (strlen(cmd)==nl && !memcmp(name,cmd,nl) && off+len<=unsigned(sz)){
          out.assign((const char*)(buf+off),len);
          ok=true;
          break;
        }
      }
    }
    free(buf);
    if (ok)
      return true;
  }
  return false;
}

static void show_command_help(int index){
  char cmd[40];
  catalog_command_name(index,cmd,sizeof(cmd));
  ustl::string body;
  if (!load_help_record(cmd,body))
    body="Copy CAS.PAK to storage root.\nThen press F6 on a command.";
  textArea text;
  text.editable=false;
  text.clipline=-1;
  text.title=(char*)"Help on command";
  text.allowF1=true;
  text.python=0;
  ustl::vector<textElement> & elem=text.elements;
  elem=ustl::vector<textElement>(0);
  textElement head;
  head.s=completeCat[index].name;
  head.color=COLOR_BLUE;
  elem.push_back(head);
  const char *p=body.c_str();
  while (*p){
    const char *q=p;
    while (*q && *q!='\n')
      ++q;
    textElement e;
    e.newLine=1;
    e.lineSpacing=1;
    e.s=ustl::string(p,q-p);
    elem.push_back(e);
    p=*q?q+1:q;
  }
  doTextArea(&text);
}

int showCatalog(char* insertText,int preselect,int menupos) {
  return doCatalogMenu(insertText, (char*)"Catalog", CAT_CATEGORY_ALL);
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

// 0 on exit, 1 on success
int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {
  int allcmds=CAT_COMPLETE_COUNT;
  int nitems = catalog_count_for_category(category);
  if (nitems<=0)
    return 0;
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,menusel=-1;
  if (category==CAT_CATEGORY_ALL){
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
      if (completeCat[cur].category==category){
        menuitems[curmi].type = MENUITEM_NORMAL;
        menuitems[curmi].color = TEXT_COLOR_BLACK;
        menuitems[curmi].isfolder = cur;
        menuitems[curmi].text = completeCat[cur].name;
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
  menu.type = category==CAT_CATEGORY_ALL ? MENUTYPE_NORMAL : MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {
    if (category!=CAT_CATEGORY_ALL){
      drawFkeyLabels(0x03FC, -1, -1, -1, -1, 0x03FD);
      int fkeyw=LCD_WIDTH_PX/6;
      drawRectangle(5*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
      Bdisp_MMPrint(5*fkeyw+7,LCD_HEIGHT_PX-STATUS_AREA_PX-19," HELP",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    }
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return 0;
    }
    int index=menuitems[menu.selection-1].isfolder;
    if (category!=CAT_CATEGORY_ALL && sres==KEY_CTRL_F6){
      if (index<allcmds)
        show_command_help(index);
      DisplayStatusArea();
      continue;
    }
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {
      if (category==CAT_CATEGORY_ALL){
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

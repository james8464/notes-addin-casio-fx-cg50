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
  {"incline", CAT_CATEGORY_MECH},
  {"lcm", CAT_CATEGORY_ARIT},
  {"lcoeff", CAT_CATEGORY_POLYNOMIAL},
  {"lift", CAT_CATEGORY_MECH},
  {"limit", CAT_CATEGORY_CALCULUS},
  {"ln", CAT_CATEGORY_ALGEBRA},
  {"log", CAT_CATEGORY_ALGEBRA},
  {"moment", CAT_CATEGORY_MECH},
  {"partfrac", CAT_CATEGORY_ALGEBRA},
  {"power", CAT_CATEGORY_MECH},
  {"projectile", CAT_CATEGORY_MECH},
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

ustl::string insert_string(int index){
  ustl::string s=completeCat[index].name;
  int pos=s.find('(');
  if (pos>=0 && pos<s.size())
    s=s.substr(0,pos+1);
  return s;//s+' ';
}

int showCatalog(char* insertText,int preselect,int menupos) {
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
    draw_catalog_fkeys(category==CAT_CATEGORY_ALL);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return 0;
    }
    int index=menuitems[menu.selection-1].isfolder;
    if(category!=CAT_CATEGORY_ALL && sres == KEY_CTRL_F6) {
      sres=KEY_CTRL_F1;
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

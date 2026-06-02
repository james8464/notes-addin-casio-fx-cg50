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
#define CAT_CATEGORY_LOGO 19 // should be the last one

using namespace giac;
extern giac::context * contextptr;
void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

int lang=0;
const catalogFunc completeCat[] = { // list of all functions (including some not in any category)
  {"abs(x)", 0, "Modulus. Arg x. Used for inequalities/domain work.", "-3", "x-2", CAT_CATEGORY_REAL | (CAT_CATEGORY_ALGEBRA<<8)},
  {"acos(x)", 0, "Inverse cos. Arg x in [-1,1].", "1/2", 0, CAT_CATEGORY_TRIG},
  {"approx(x)", 0, "Decimal form. Arg x.", "sqrt(2)", "pi", CAT_CATEGORY_REAL},
  {"asin(x)", 0, "Inverse sin. Arg x in [-1,1].", "1/2", 0, CAT_CATEGORY_TRIG},
  {"atan(x)", 0, "Inverse tan. Arg x.", "1", 0, CAT_CATEGORY_TRIG},
  {"ceil(x)", 0, "Smallest integer >= x.", "1.2", 0, CAT_CATEGORY_REAL},
  {"ceiling(x)", 0, "Smallest integer >= x.", "1.2", 0, CAT_CATEGORY_REAL},
  {"coeff(p,x,n)", 0, "Coefficient of x^n in p.", "3*x^2+2*x+1,x,2", 0, CAT_CATEGORY_POLYNOMIAL},
  {"collect(expr,x)", 0, "Group terms by powers of x.", "x+x^2+2*x", "sin(x)^2+cos(x)^2", CAT_CATEGORY_ALGEBRA},
  {"cos(x)", 0, "Cosine.", "pi/3", 0, CAT_CATEGORY_TRIG},
  {"cot(x)", 0, "Cotangent: cot(x)=1/tan(x).", "x", 0, CAT_CATEGORY_TRIG},
  {"degree(p,x)", 0, "Polynomial degree.", "x^4-1,x", 0, CAT_CATEGORY_POLYNOMIAL},
  {"diff(f,var,[n])", 0, "Differentiate. Supports equations and exam working routes.", "x^3,x", "(x^2)*tan(y)=9,x", CAT_CATEGORY_CALCULUS},
  {"exact(x)", 0, "Exact fraction/radical form.", "1.25", 0, CAT_CATEGORY_REAL},
  {"expand(expr)", 0, "Expand brackets/powers.", "(x+1)^3", "sin(2*x)", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_TRIG<<8)},
  {"factor(p,[x])", 0, "Factorise polynomial/expression.", "x^2-5*x+6", "x^3-1", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL<<8)},
  {"floor(x)", 0, "Greatest integer <= x.", "1.8", 0, CAT_CATEGORY_REAL},
  {"fsolve(equation,x=a..b)", 0, "Numerical solve on interval.", "cos(x)=x,x=0..1", 0, CAT_CATEGORY_SOLVE},
  {"gcd(a,b,...)", 0, "Greatest common divisor.", "x^2-1,x^3-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"integrate(f,x,[a,b])", 0, "Integrate. Method: 1 auto, 2 parts, 3 substitution, 4 partial fractions, 5 trig.", "(ln(x))^2,x,2", "(ln(x))^2/x,x,3,u=ln(x)", CAT_CATEGORY_CALCULUS},
  {"implicit_diff(eq,[x,y])", "diff(", "Implicit differentiation. Use diff(equation,x).", "(x^2)*tan(y)=9,x", 0, CAT_CATEGORY_CALCULUS},
  {"lcm(a,b,...)", 0, "Least common multiple.", "x^2-1,x^3-1", 0, CAT_CATEGORY_POLYNOMIAL},
  {"limit(f,x=a)", 0, "Limit at x=a. Optional side.", "sin(x)/x,x=0", "1/x,x=0,1", CAT_CATEGORY_CALCULUS},
  {"ln(x)", 0, "Natural log. x>0.", "e^2", 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_REAL<<8)},
  {"log(base,x)", "log(", "Log to any base. Uses ln(x)/ln(base).", "2,x", "3,81", CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_REAL<<8)},
  {"partfrac(p,x)", 0, "Partial fractions.", "1/(x^2-1),x", 0, CAT_CATEGORY_ALGEBRA},
  {"pcoeff(p)", 0, "Polynomial from roots.", "[1,2,3]", 0, CAT_CATEGORY_POLYNOMIAL},
  {"product(f,k,m,M)", 0, "Product notation.", "k,k,1,n", 0, CAT_CATEGORY_CALCULUS},
  {"proot(p)", 0, "Numerical polynomial roots.", "x^3-2*x+1", 0, CAT_CATEGORY_POLYNOMIAL | (CAT_CATEGORY_SOLVE<<8)},
  {"range(expr,[x,a,b])", "range(", "Range of a function; checks restrictions/endpoints.", "x^2", "1/x", CAT_CATEGORY_ALGEBRA},
  {"rewrite(expr,terms)", "rewrite(", "Rewrite expression/equation in terms of chosen target expressions.", "log(2,x^2+8*x),[a=log(2,x),b=log(2,x+8)]", "log(2,8+64/x),[a=log(2,x),b=log(2,x+8)]", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_TRIG<<8)},
  {"round(x)", 0, "Round to nearest integer.", "1.6", 0, CAT_CATEGORY_REAL},
  {"sec(x)", 0, "Secant: sec(x)=1/cos(x).", "x", 0, CAT_CATEGORY_TRIG},
  {"series(f,x=a,n)", 0, "Taylor/binomial series.", "sin(x),x=0,5", "(1+x)^(-1),x=0,4", CAT_CATEGORY_CALCULUS},
  {"simplify(expr)", 0, "Simplify algebra/trig.", "sin(x)^2+cos(x)^2", "sqrt(x^2)", CAT_CATEGORY_ALGEBRA},
  {"sin(x)", 0, "Sine.", "pi/6", 0, CAT_CATEGORY_TRIG},
  {"solve(equation,x)", 0, "Exact solve with exam routes before CAS fallback.", "x^2-5*x+6=0,x", "(dy)/(dx)=y,y", CAT_CATEGORY_SOLVE},
  {"sqrt(x)", 0, "Square root; real x>=0.", "9", "x^2", CAT_CATEGORY_REAL | (CAT_CATEGORY_ALGEBRA<<8)},
  {"subst(a,b=c)", 0, "Substitutes b for c in a.", "x^2,x=3", 0, CAT_CATEGORY_ALGEBRA},
  {"sum(f,k,m,M)", 0, "Summation notation.", "k^2,k,1,n", 0, CAT_CATEGORY_CALCULUS},
  {"tan(x)", 0, "Tangent.", "pi/4", 0, CAT_CATEGORY_TRIG},
  {"taylor(f,x=a,n)", 0, "Taylor expansion.", "sin(x),x=0,5", 0, CAT_CATEGORY_CALCULUS},
  {"tcollect(expr)", 0, "Collect trig terms.", "sin(x)+cos(x)", 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, "Expand trig/log/exp forms.", "sin(2*x)", "ln(x*y)", CAT_CATEGORY_TRIG | (CAT_CATEGORY_ALGEBRA<<8)},
  {"xform(expr,target)", "xform(", "Transform expr into target using supported identities.", "1+tan(x)^2,sec(x)^2", "log(a,x),ln(x)/ln(a)", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_TRIG<<8)},
};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="Khicas Help";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="A-level Pure build: use catalog for commands.";
const char apropos_string[]="KhiCASen A-level Pure build.";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);

ustl::string insert_string(int index){
  ustl::string s;
  if (completeCat[index].insert)
    s=completeCat[index].insert;
  else {
    s=completeCat[index].name;
    int pos=s.find('(');
    if (pos>=0 && pos<s.size())
      s=s.substr(0,pos+1);
  }
  return s;//s+' ';
}

static void add_help_line(ustl::vector<textElement> & elem,const ustl::string & s,color_t color,int spacing){
  textElement e;
  e.s=s;
  e.color=color;
  e.newLine=elem.size()?1:0;
  e.lineSpacing=spacing;
  elem.push_back(e);
}

static ustl::string example_string(int index,char * example){
  ustl::string s;
  if (!example)
    return s;
  if (example[0]=='#')
    s=example+1;
  else {
    s=insert_string(index);
    s += example;
    s += ")";
  }
  return s;
}

int showCatalog(char* insertText,int preselect,int menupos) {
  //int ret;
  // returns 0 on failure (user exit) and 1 on success (user chose a option)
  const int categories[]={CAT_CATEGORY_ALL,CAT_CATEGORY_ALGEBRA,CAT_CATEGORY_CALCULUS,CAT_CATEGORY_POLYNOMIAL,CAT_CATEGORY_REAL,CAT_CATEGORY_SOLVE,CAT_CATEGORY_TRIG};
  MenuItem menuitems[sizeof(categories)/sizeof(categories[0])];
  menuitems[0].text = (char*)"All";
  menuitems[1].text = (char*)"Algebra";
  menuitems[2].text = (char*)"Calculus";
  menuitems[3].text = (char*)"Polynomials";
  menuitems[4].text = (char*)"Reals";
  menuitems[5].text = (char*)"Solve";
  menuitems[6].text = (char*)"Trigonometry";
  
  Menu menu;
  menu.items=menuitems;
  menu.numitems=sizeof(menuitems)/sizeof(MenuItem);
  menu.scrollout=1;
  menu.title = (char*)"Function Catalog";
  
  while(1) {
    if (preselect)
      menu.selection=preselect;
    else {
      if (menupos>0)
	menu.selection=menupos;
      int sres = doMenu(&menu);
      if (sres != MENU_RETURN_SELECTION)
	return 0;
    }
    // puts("catalog 3");
    int category=preselect;
    if (!category)
      category=categories[menu.selection-1];
    if (category!=CAT_CATEGORY_ALL && category!=CAT_CATEGORY_ALGEBRA && category!=CAT_CATEGORY_CALCULUS && category!=CAT_CATEGORY_POLYNOMIAL && category!=CAT_CATEGORY_REAL && category!=CAT_CATEGORY_SOLVE && category!=CAT_CATEGORY_TRIG)
      category=CAT_CATEGORY_ALL;
    char *submenu_title=preselect?(char*)"Function Catalog":menuitems[menu.selection-1].text;
    if(doCatalogMenu(insertText, submenu_title, category)) {
      const char * ptr=0;
      if (strcmp("matrix ",insertText)==0 && (ptr=input_matrix(false)) )
	return 0;
      if (strcmp("list ",insertText)==0 && (ptr=input_matrix(true)) )
	return 0;
      return 1;
    }
    if (preselect)
      return 0;
  }
  return 0;
}

int find_category(const char *cmdname){
  int l=strlen(cmdname);
  for (int i=0;i<CAT_COMPLETE_COUNT;++i){
    if (!strncmp(cmdname,completeCat[i].name,l))
      return completeCat[i].category;
  }
  return -1;
}

// 0 on exit, 1 on success
int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {
  int allcmds=builtin_lexer_functions_end()-builtin_lexer_functions_begin();
  int allopts=lexer_tab_int_values_end-lexer_tab_int_values_begin;
  bool isall=category==CAT_CATEGORY_ALL;
  bool isopt=category==CAT_CATEGORY_OPTIONS;
  int nitems = isall? CAT_COMPLETE_COUNT:(isopt?allopts:CAT_COMPLETE_COUNT);
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,i=0,menusel=-1,cmdl=cmdname?strlen(cmdname):0;
  gen g;
  while(cur<nitems) {
    menuitems[curmi].type = MENUITEM_NORMAL;
    menuitems[curmi].color = TEXT_COLOR_BLACK;    
    if (isall) {
      menuitems[curmi].isfolder = cur;
      menuitems[curmi].text = completeCat[cur].name;
      ++curmi;
    }
    else if (isopt) {
      const char * text=isall?(builtin_lexer_functions_begin()+curmi)->first:(lexer_tab_int_values_begin+curmi)->keyword;
      if (menusel<0 && cmdname && !strncmp(cmdname,text,cmdl))
	menusel=curmi;
      menuitems[curmi].text = text;
      menuitems[curmi].isfolder = allcmds; // assumes allcmds>allopts
      menuitems[curmi].token=isall?find_or_make_symbol(text,g,0,false,contextptr):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
      for (;i<CAT_COMPLETE_COUNT;++i){
	const char * catname=completeCat[i].name;
	int tmp=strcmp(catname,text);
	if (tmp>=0){
	  size_t st=strlen(text),j=tmp?0:st;
	  for (;j<st;++j){
	    if (catname[j]!=text[j])
	      break;
	  }
	  if (j==st && (!isalphanum(catname[j]))){
	    menuitems[curmi].isfolder = i;
	    ++i;
	  }
	  break;
	}
      }
      // compare text with completeCat
      ++curmi;
    }
    else {
      int cat=completeCat[cur].category;
      if ( (cat & 0xff) == category ||
	   (cat & 0xff00) == (category<<8) ||
	   (cat & 0xff0000) == (category <<16)
	   ){
	menuitems[curmi].isfolder = cur; // little hack: store index of the command in the full list in the isfolder property (unused by the menu system in this case)
	menuitems[curmi].text = completeCat[cur].name;
	curmi++;
      }
    }
    cur++;
  }
  
  Menu menu;
  if (menusel>=0)
    menu.selection=menusel+1;
  menu.items=menuitems;
  menu.numitems=curmi;
  if (isopt){ menu.selection=5; menu.scroll=4; }
  if (curmi>=100)
    SetSetupSetting( (unsigned int)0x14, 0x88);	
  DisplayStatusArea();
  menu.scrollout=1;
  menu.title = title;
  menu.type = MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {
    drawFkeyLabels(0x03FC, 0x03fc, 0, 0, 0, 0x03FD);
    int fkeyw=LCD_WIDTH_PX/6;
    drawRectangle(fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXAMPL1",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(2*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(2*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXAMPL2",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    Bdisp_PutDisp_DD();
    drawCasioCasBorder();
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return 0;
    }
    int index=menuitems[menu.selection-1].isfolder;
    if(sres == KEY_CTRL_F6) {
      char * example=index<allcmds?completeCat[index].example:0;
      char * example2=index<allcmds?completeCat[index].example2:0;
      textArea text;
      text.title = (char*)"Help on command";
      text.editable=false;
      text.clipline=-1;
      text.allowF1=true;
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement>();
      add_help_line(elem,index<allcmds?completeCat[index].name:menuitems[menu.selection-1].text,COLOR_BLUE,0);
      ustl::string autoexample;
      if (index<allcmds) {
	add_help_line(elem,"Use",COLOR_BLACK,4);
	add_help_line(elem,completeCat[index].desc,COLOR_BLACK,0);
	add_help_line(elem,"Args",COLOR_BLACK,4);
	add_help_line(elem,"Required args come first; [optional] args are in brackets.",COLOR_BLACK,0);
      }
      else {
	add_help_line(elem,"Sorry, no help available.",COLOR_BLACK,4);
	int token=menuitems[menu.selection-1].token;
	if (isopt){
	  if (token==_INT_PLOT+T_NUMBER*256){
	    autoexample="display="+elem[0].s;
	    elem.back().s="Graphic option: "+autoexample;
	  }
	  if (token==_INT_COLOR+T_NUMBER*256){
	    autoexample="display="+elem[0].s;
	    elem.back().s="Color: "+autoexample;
	  }
	  if (token==_INT_SOLVER+T_NUMBER*256){
	    autoexample=elem[0].s;
	    elem.back().s="fsolve option: " + autoexample;
	  }
	  if (token==_INT_TYPE+T_TYPE_ID*256){
	    autoexample=elem[0].s;
	    elem.back().s="Object type: " + autoexample;
	  }
	}
	if (isall){
	  if (token==T_UNARY_OP || token==T_UNARY_OP_38)
	    elem.back().s=elem[0].s+"(args)";
	}
      }
      if (example){
	add_help_line(elem,"F2 example",COLOR_BLACK,4);
	add_help_line(elem,example_string(index,example),COLOR_BLACK,0);
	if (example2){
	  add_help_line(elem,"F3 example",COLOR_BLACK,4);
	  add_help_line(elem,example_string(index,example2),COLOR_BLACK,0);
	}
      }
      else {
	if (autoexample.size())
	  add_help_line(elem,autoexample,COLOR_BLACK,4);
      }
      add_help_line(elem,"Keys",COLOR_BLACK,4);
      add_help_line(elem,"F1 command. F2/F3 examples. EXIT back.",COLOR_BLACK,0);
      sres=doTextArea(&text);
    }
    if (sres == KEY_CTRL_F2 || sres==KEY_CTRL_F3) {
      reset_alpha();
      if (index<allcmds && completeCat[index].example){
	ustl::string s(insert_string(index));
	char * example=0;
	if (sres==KEY_CTRL_F2)
	  example=completeCat[index].example;
	else
	  example=completeCat[index].example2;
	if (example){
	  if (example[0]=='#')
	    s=example+1;
	  else {
	    s += example;
	    s += ")";
	  }
	}
	strcpy(insertText, s.c_str());
	return 1;
      }
      if (isopt){
	int token=menuitems[menu.selection-1].token;
	if (token==_INT_PLOT+T_NUMBER*256 || token==_INT_COLOR+T_NUMBER*256)
	  strcpy(insertText,"display=");
	else
	  *insertText=0;
	strcat(insertText,menuitems[menu.selection-1].text);
	return 1;
      }
      sres=KEY_CTRL_F1;
    }
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {
      reset_alpha();
      strcpy(insertText,index<allcmds?insert_string(index).c_str():menuitems[menu.selection-1].text);
      return 1;
    }
  }
  return 0;
}

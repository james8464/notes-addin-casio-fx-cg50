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
#include "main.h"
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
#define CAT_CATEGORY_PHYS 19
#define CAT_CATEGORY_UNIT 20
#define CAT_CATEGORY_2D 21
#define CAT_CATEGORY_3D 22
#define CAT_CATEGORY_LOGO 23 // should be the last one
#define XCAS_ONLY 0x80000000

using namespace giac;
extern giac::context * contextptr;
void reset_alpha(){
  SetSetupSetting( (unsigned int)0x14, 0);	
  DisplayStatusArea();
}

int lang=0;
const char ram_filename[]="\\\\fls0\\khicas50.8c2";
const catalogFunc completeCat[] = { // A-level scoped catalog only
  {"abs(x)", 0, "Absolute value.", "abs(-3)", 0, CAT_CATEGORY_REAL},
  {"approx(x)", 0, "Decimal approximation.", "approx(pi)", 0, CAT_CATEGORY_REAL},
  {"ceil(x)", 0, "Smallest integer not less than x.", "ceil(1.2)", 0, CAT_CATEGORY_REAL},
  {"coeff(p,x,n)", 0, "Coefficient of x^n in polynomial p.", "coeff((x+1)^3,x,2)", 0, CAT_CATEGORY_POLYNOMIAL},
  {"collect(expr,x)", 0, "Collect powers of x.", "collect(x+x^2,x)", 0, CAT_CATEGORY_ALGEBRA},
  {"degree(p,x)", 0, "Degree of polynomial p in x.", "degree(x^4-1,x)", 0, CAT_CATEGORY_POLYNOMIAL},
  {"diff(f,var,[n])", 0, "Derivative of f with respect to var.", "diff(sin(x),x)", "diff(x^3,x,2)", CAT_CATEGORY_CALCULUS},
  {"discriminant(p,x)", 0, "Discriminant of polynomial p.", "discriminant(x^2+x+1,x)", 0, CAT_CATEGORY_POLYNOMIAL},
  {"exp(x)", 0, "Exponential e^x.", "exp(2*x)", 0, CAT_CATEGORY_REAL},
  {"expand(expr)", 0, "Expand expression.", "expand((x+1)^3)", 0, CAT_CATEGORY_ALGEBRA},
  {"factor(p[,x])", 0, "Factor polynomial p.", "factor(x^2-1)", 0, CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_POLYNOMIAL << 8)},
  {"floor(x)", 0, "Greatest integer not greater than x.", "floor(1.8)", 0, CAT_CATEGORY_REAL},
  {"implicit_diff(eq,[x,y])", "implicit_diff(", "Implicit differentiation with A-level working lines.", "(x^2)*tan(y)=9,x,y", 0, CAT_CATEGORY_CALCULUS},
  {"integrate(f,x,[a,b])", 0, "Antiderivative or definite integral.", "integrate(x*sin(x),x)", "integrate(x^2,x,0,1)", CAT_CATEGORY_CALCULUS},
  {"limit(f,x=a)", 0, "Limit of f as x tends to a.", "limit(sin(x)/x,x=0)", 0, CAT_CATEGORY_CALCULUS},
  {"ln(x)", 0, "Natural logarithm.", "ln(exp(3))", 0, CAT_CATEGORY_REAL},
  {"log(base,x)", "log(", "Logarithm; log(base,x) uses any base.", "log(2,8)", "log(10,100)", CAT_CATEGORY_REAL | (CAT_CATEGORY_SOLVE << 8)},
  {"normal(expr)", 0, "Put rational expression over one denominator.", "normal(1/(x+1)+1/(x-1))", 0, CAT_CATEGORY_ALGEBRA},
  {"partfrac(p,x)", 0, "Partial fractions.", "partfrac((2*x+3)/(x^2-1),x)", 0, CAT_CATEGORY_ALGEBRA},
  {"product(f,k,m,M)", 0, "Product of f for k from m to M.", "product(k,k,1,n)", 0, CAT_CATEGORY_CALCULUS},
  {"range(expr,[x,a,b])", "range(", "Range of expression as inequalities.", "range(x^2,[x,-2,3])", 0, CAT_CATEGORY_CALCULUS | (CAT_CATEGORY_ALGEBRA << 8)},
  {"round(x)", 0, "Nearest integer.", "round(3.6)", 0, CAT_CATEGORY_REAL},
  {"series(f,x=a,n)", 0, "Taylor expansion to order n.", "series(sin(x),x=0,5)", 0, CAT_CATEGORY_CALCULUS},
  {"simplify(expr)", 0, "Simplify expression.", "simplify(sin(x)^2+cos(x)^2)", 0, CAT_CATEGORY_ALGEBRA},
  {"solve(equation,x)", 0, "Solve equation for x.", "solve(x^2-x-1=0,x)", 0, CAT_CATEGORY_SOLVE},
  {"subst(a,b=c)", 0, "Substitute c for b in a.", "subst(x^2,x=3)", 0, CAT_CATEGORY_ALGEBRA},
  {"sum(f,k,m,M)", 0, "Sum of f for k from m to M.", "sum(k^2,k,1,n)", 0, CAT_CATEGORY_CALCULUS},
  {"suvat(u,a,t)", "suvat(", "A-level constant acceleration displacement with working.", "suvat(3,2,5)", 0, CAT_CATEGORY_PHYS | (CAT_CATEGORY_CALCULUS << 8)},
  {"tcollect(expr)", 0, "Collect trigonometric terms.", "tcollect(sin(x)+cos(x))", 0, CAT_CATEGORY_TRIG},
  {"texpand(expr)", 0, "Expand trigonometric expressions.", "texpand(sin(3*x))", 0, CAT_CATEGORY_TRIG},
  {"trigcos(expr)", 0, "Rewrite trig using cos where useful.", "trigcos(sin(x)^2)", 0, CAT_CATEGORY_TRIG},
  {"trigsin(expr)", 0, "Rewrite trig using sin where useful.", "trigsin(cos(x)^2)", 0, CAT_CATEGORY_TRIG},
  {"trigtan(expr)", 0, "Rewrite trig using tan where useful.", "trigtan(cos(x)^2)", 0, CAT_CATEGORY_TRIG},
  {"xform(expr,target)", "xform(", "Transform expr into target with A-level working lines.", "x^2+2*x+1,(x+1)^2", "log(2,x^2),2*log(2,x)", CAT_CATEGORY_ALGEBRA | (CAT_CATEGORY_TRIG << 8)},
};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="Khicas Help";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="";
const char apropos_string[]="";

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

int showCatalog(char* insertText,int preselect,int menupos) {
  // returns 0 on failure (user exit) and 1 on success (user chose a option)
  MenuItem menuitems[CAT_CATEGORY_LOGO+1];
    menuitems[CAT_CATEGORY_ALL].text = (char*)((lang==1)?"Tout":"All");
    menuitems[CAT_CATEGORY_ALGEBRA].text = (char*)((lang==1)?"Algebre":"Algebra");
    menuitems[CAT_CATEGORY_LINALG].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_CALCULUS].text = (char*)((lang==1)?"Analyse":"Calculus");
    menuitems[CAT_CATEGORY_ARIT].text = (char*)"Arithmetic";
    menuitems[CAT_CATEGORY_COMPLEXNUM].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_PLOT].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_POLYNOMIAL].text = (char*)((lang==1)?"Polynomes":"Polynomials");
    menuitems[CAT_CATEGORY_PROBA].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_PROGCMD].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_REAL].text = (char*)((lang==1)?"Reels (x,theta,t)":"Reals");
    menuitems[CAT_CATEGORY_SOLVE].text = (char*)((lang==1)?"Resoudre (log)":"Solve (log)");
    menuitems[CAT_CATEGORY_STATS].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_TRIG].text = (char*)((lang==1)?"Trigonometrie (sin)":"Trigonometry (sin)");
    menuitems[CAT_CATEGORY_OPTIONS].text = (char*)"Options (cos)";
    menuitems[CAT_CATEGORY_LIST].text = (char*)((lang==1)?"Listes (tan)":"Lists (tan)");
    menuitems[CAT_CATEGORY_MATRIX].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_PROG].text = (char*)"Removed";
    menuitems[CAT_CATEGORY_SOFUS].text = (char*)((lang==1)?"Modifier variables (()":"Change variables (()");
    menuitems[CAT_CATEGORY_PHYS].text = (char*)((lang==1)?"Constantes physique ())":"Physics constants ())");
    menuitems[CAT_CATEGORY_UNIT].text = (char*)((lang==1)?"Unites physiques (,)":"Units (,)");
    menuitems[CAT_CATEGORY_2D].text = (char*)((lang==1)?"Geometrie (->)":"Geometry (->)");
    menuitems[CAT_CATEGORY_3D].text = (char*)((lang==1)?"3D (*)":"3D (*)");
    menuitems[CAT_CATEGORY_LOGO].text = (char*)"Removed";
  
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
    Bdisp_PutDisp_DD();
    drawCasioCasBorder();
      if (sres != MENU_RETURN_SELECTION)
	return 0;
    }
    // puts("catalog 3");
    if(doCatalogMenu(insertText,(char *) menuitems[menu.selection-1].text, menu.selection-1)) {
      const char * ptr=0;
      if (strcmp("_removed_array ",insertText)==0 && (ptr=input_matrix(false)) )
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

  string remove_accents(const string & s){
    string r;
    for (int i=0;i<s.size();++i){
      unsigned char ch=s[i];
      if (ch==195 && i<s.size()-1){
	++i;
	switch ((unsigned char)s[i]){
	case 160: case 161: case 162:
	  r+='a';
	  continue;
	case 168: case 169: case 170:
	  r+='e';
	  continue;
	case 172: case 173: case 174:
	  r+='i';
	  continue;
	case 178: case 179: case 180:
	  r += 'o';
	  continue;
	case 185: case 186: case 187:
	  r+='u';
	  continue;
	}
	r += '?';
	continue;
      }
      r+=(char)ch;
    }
    return r;
  }

string extract_example(const string & s,int n){
  int l=s.size(),j=0;
  string res;
  for (int i=0;i<l;++i){
    if (s[i]==';'){
      if (j==n)
	break;
      ++j;
      continue;
    }
    if (j==n)
      res+=s[i];
  }
  return res;
}

ustl::string longhelp(const char * s);
int QRdisp(const char * text,const char *msg1,const char * msg2,const char * msg3,const char * msg4);

int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {
  int allcmds=builtin_lexer_functions_end()-builtin_lexer_functions_begin();
  int allopts=lexer_tab_int_values_end-lexer_tab_int_values_begin;
  bool isall=category==CAT_CATEGORY_ALL;
  bool isopt=category==CAT_CATEGORY_OPTIONS;
  int nitems = isall? allcmds:(isopt?allopts:CAT_COMPLETE_COUNT);
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,i=0,menusel=-1,cmdl=cmdname?strlen(cmdname):0;
  gen g;
  while(cur<nitems) {
    MenuItem & curmenuitem= menuitems[curmi];
    curmenuitem.type = MENUITEM_NORMAL;
    curmenuitem.color = TEXT_COLOR_BLACK;    
    if (isall || isopt) {
      const char * text=isall?(builtin_lexer_functions_begin()+curmi)->first:(lexer_tab_int_values_begin+curmi)->keyword;
      if (menusel<0 && cmdname && !strncmp(cmdname,text,cmdl))
	menusel=curmi;
      curmenuitem.text = text;
      curmenuitem.isfolder = allcmds; // assumes allcmds>allopts
      curmenuitem.token=isall?((builtin_lexer_functions_begin()+curmi)->second.subtype+256):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
      // curmenuitem.token=isall?find_or_make_symbol(text,g,0,false,contextptr):((lexer_tab_int_values_begin+curmi)->subtype+(lexer_tab_int_values_begin+curmi)->return_value*256);
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
	    curmenuitem.isfolder = i;
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
      if ( (xcas_python_eval==0 || cat>0) &&
	   ((cat & 0xff) == category ||
	    (cat & 0xff00) == (category<<8) ||
	    (cat & 0xff0000) == (category <<16)
	    )
	   ){
	curmenuitem.isfolder = cur; // little hack: store index of the command in the full list in the isfolder property (unused by the menu system in this case)
	curmenuitem.text = completeCat[cur].name;
	curmi++;
      }
    }
    cur++;
  }
  if (curmi==0)
    return 0;
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
    int fkeyw=LCD_WIDTH_PX/6;
    drawRectangle(0,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(0,LCD_HEIGHT_PX-STATUS_AREA_PX-19,"  INPUT",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXAMPL1",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(2*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(2*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19," EXAMPL2",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(3*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    drawRectangle(4*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(4*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19,"  HELP",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    drawRectangle(5*fkeyw,LCD_HEIGHT_PX-23, fkeyw, 23, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(5*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-19,"        ",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){
      reset_alpha();
      return sres;
    }
    int index=menuitems[menu.selection-1].isfolder;
    const char *fcmdname=menuitems[menu.selection-1].text,* fhowto=0,*fsyntax=0,*fexamples=0,*frelated=0;
    bool stathelp=has_static_help(fcmdname,lang,fhowto,fsyntax,fexamples,frelated);
    if (sres==KEY_CTRL_F6 && fcmdname)
      continue;
    if (sres == KEY_CTRL_F5) {
      char * example=index<allcmds?completeCat[index].example:0;
      char * example2=index<allcmds?completeCat[index].example2:0;
      textArea text;
      text.editable=false;
      text.clipline=-1;
      text.title = (char*)"Help on command";
      text.allowF1=true;
      text.python=python_compat(contextptr);
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement> (example2?4:3);
      elem[0].s = index<allcmds?completeCat[index].name:menuitems[menu.selection-1].text;
      elem[0].color = COLOR_BLUE;
      elem[1].newLine = 1;
      elem[1].lineSpacing = 3;
      ustl::string autoexample;
      if (index<allcmds)
	elem[1].s = completeCat[index].desc;
      else {
	int token=menuitems[menu.selection-1].token;
	elem[1].s="Sorry, no help available...";
	// *logptr(contextptr) << token << endl;
	if (stathelp){
	  elem[1].s=remove_accents(fhowto);
	  //cout << fexamples << '\n';
	  example=(char *)fexamples;
	}
	else {
	  if (isopt){
	    if (token==_INT_PLOT+T_NUMBER*256){
	      autoexample="display="+elem[0].s;
	      elem[1].s ="display option: "+ autoexample;
	    }
	    if (token==_INT_COLOR+T_NUMBER*256){
	      autoexample="display="+elem[0].s;
	      elem[1].s="color option: "+ autoexample;
	    }
	    if (token==_INT_SOLVER+T_NUMBER*256){
	      autoexample=elem[0].s;
	      elem[1].s="fsolve option: " + autoexample;
	    }
	    if (token==_INT_TYPE+T_TYPE_ID*256){
	      autoexample=elem[0].s;
	      elem[1].s="Object type: " + autoexample;
	    }
	  }
	  if (isall){
	    if (token==T_UNARY_OP || token==T_UNARY_OP_38)
	      elem[1].s=elem[0].s+"(args)";
	  }
	}
      }
      ustl::string ex("Ex. (F2): "),ex2("Ex. (F3): ");
      elem[2].newLine = 1;
      elem[2].lineSpacing = 3;
      if (example){
	if (example[0]=='#')
	  ex += example+1;
	else {
	  if (index<allcmds){
	    ex += insert_string(index);
	    ex += example;
	    ex += ")";
	  }
	  else {
	    string allex(example);
	    ex += extract_example(allex,0); 
	    string tmp=extract_example(allex,1);
	    if (tmp.size()>1){
	      elem.push_back(elem[2]);
	      //cout << tmp << '\n' ;
	      elem[3].newLine = 1;
	      elem[3].lineSpacing = 3;
	      elem[3].s=ex2+tmp;
	    }
	  }
	}
	elem[2].s = ex;
	if (example2){
	  if (example2[0]=='#')
	    ex2 += example2+1;
	  else {
	    if (index<allcmds){
	      ex2 += insert_string(index);
	      ex2 += example2;
	      ex2 += ")";
	    }
	  }
	  elem[3].newLine = 1;
	  elem[3].lineSpacing = 3;
	  elem[3].s=ex2;
	}
      }
      else {
	if (autoexample.size())
	  elem[2].s=ex+autoexample;
	else
	  elem.pop_back();
      }
      sres=doTextArea(&text);
    }
    if (sres == KEY_CTRL_F2 || sres==KEY_CTRL_F3) {
      reset_alpha();
      if (isopt){
	int token=menuitems[menu.selection-1].token;
	if (token==_INT_PLOT+T_NUMBER*256 || token==_INT_COLOR+T_NUMBER*256)
	  strcpy(insertText,"display=");
	else
	  *insertText=0;
	strcat(insertText,menuitems[menu.selection-1].text);
	return 1;
      }
      ustl::string s;
      char * example=0;
      string ex;
      if (stathelp){
	ex=extract_example(fexamples,sres==KEY_CTRL_F2?0:1);
	example=(char *)ex.c_str();
      }
      if (index<allcmds && completeCat[index].example){
	s=insert_string(index);
	if (sres==KEY_CTRL_F2)
	  example=completeCat[index].example;
	else
	  example=completeCat[index].example2;
      }
      if (example){
	if (example[0]=='#')
	  s=example+1;
	else {
	  s += example;
	  if (!stathelp)
	    s += ")";
	}
	strcpy(insertText, s.c_str());
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
